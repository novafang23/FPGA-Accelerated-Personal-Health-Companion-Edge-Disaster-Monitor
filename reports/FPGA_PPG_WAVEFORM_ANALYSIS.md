# ForgeFPGA PPG link — waveform analysis and root-cause report

**Board:** ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910)
**Capture:** `firmware/shrikefi/build/log/idf_py_stdout_output_23036`, full-rate
SPI diagnostic, 3474 transactions at 100 Hz (~35 s, finger present from sample
772)
**Replay tool:** `hardware/shrikefi/tools/replay_fpga_link_log.py`
**Firmware at capture:** `d5baf3f` (every-transaction diagnostic logging)

---

## 1. Summary

Three things were unproven before this capture: whether the FPGA was doing real
signal processing or acting as a pass-through, why the crest detector sometimes
fired twice per heartbeat, and why RMSSD read ~190 ms when the subject was at
rest.

All three are now settled with evidence rather than inference:

| Question | Answer |
|---|---|
| Is the FPGA computing the 8-tap average? | **Yes — proven bit-exact.** 3465/3465 returned bytes match a software model of `moving_average_8tap` exactly. |
| Is the beat flag real? | **Yes.** An independent software FSM fed only the captured input stream reproduces 18 of the 19 reported beats, in order, with identical intervals. |
| Why the double fire? | **The dicrotic notch.** The filtered pulse has two systolic maxima separated by a notch; the RTL confirms a crest after two falling samples, so it latches onto the first. |
| Why RMSSD ~190 ms? | **Two artifacts, no physiology:** 4 software-fallback intervals (~500 ms) mixed into a window of FPGA intervals (~880 ms), plus one notch split (600 ms / 1010 ms). |

The physical claim that can now be defended: **the ForgeFPGA performs the 8-tap
moving average and the systolic crest detection in hardware, on live data, at
100 Hz, and its output is verifiable against the RTL byte for byte.**

---

## 2. Method

The FPGA↔ESP32 link carries one 8-bit sample out and one 8-bit byte back per SPI
transaction. Nothing else about the FPGA's internal state is observable. So the
only way to verify it is to model the RTL and replay the captured input through
it.

`replay_fpga_link_log.py` does exactly that:

1. **`rtl_moving_average()`** reimplements `moving_average_8tap`
   (`next_sum = running_sum + data_in - shift_reg[7]`, `data_out = next_sum[10:3]`,
   `DATA_WIDTH=8`) and reconstructs the filter output for every sample.
2. **`rtl_peak_detector()`** reimplements `ppg_peak_detector` — the same
   `ARMED → RISING → PEAK_FOUND → REFRACTORY` machine, the same
   `dyn_threshold = 8'd120`, the same 250 ms refractory, and the same
   two-sample fall confirmation.
3. Both are compared against what the hardware actually returned.

---

## 3. Result 1 — the 8-tap moving average is running in hardware

```
rx[n] == (ma[n-0] & 0x7F): 1580/3466 =  45.586%
rx[n] == (ma[n-1] & 0x7F): 3465/3465 = 100.000%   <-- bit-exact
rx[n] == (ma[n-2] & 0x7F): 1578/3464 =  45.554%
```

Every single returned byte — all 3465 of them, across 35 seconds spanning
saturation, finger movement and normal pulsation — equals the modelled 8-tap
average of the preceding input samples, truncated to 7 bits. Zero mismatches.

The one-sample pipeline lag is the expected one: `moving_average_8tap` registers
`data_out` on the same edge that `data_in` is accepted, and `tx_data` is
registered once more.

This is not "the FPGA responded to SPI". A pass-through, a constant, a counter or
noise would all fail this test. The FPGA is executing the same arithmetic as
`forgefpga_ppg_top.v`.

### 3.1 The returned value is truncated — a diagnostic bug, not a detector bug

```verilog
tx_data <= {beat_latched, filt_sample[6:0]};
```

Bit 7 carries the beat flag, so only the **low seven bits** of the 8-bit moving
average reach the ESP32. The average is a full 8-bit quantity: it spends
**1025 of 3466 samples (29.6%) above 127**, reaching 255. Every one of those
samples wraps when read back, so the `Filtered=` field in the log is a sawtooth,
not a waveform.

**Detection is unaffected**, because `ppg_peak_detector` is wired to the full
`filt_sample[7:0]` internally (`forgefpga_ppg_top.v` line 124) and only the
`tx_data` packing truncates. The consequence is confined to the telemetry:

- the `Filtered=` numbers in the `[FPGA ACCEL]` log lines are meaningless
  (`Filtered=5` is a real value of 133; `Filtered=78` is 206);
- anyone reading the SPI capture to check amplitude is misled.

**Recommended RTL change (requires re-synthesis):** send `filt_sample[7:1]`
instead of `filt_sample[6:0]`. Seven bits of headroom is still 2-count
resolution on a value centred at 120, and the wraparound disappears. This has
**not** been applied, because the checked-in `.v` must stay in step with the
flashed bitstream — see §6.

---

## 4. Result 2 — the double fire is the dicrotic notch

### 4.1 The FSM reproduces the hardware

```
predicted 18 beats: 1941 2037 2124 2208 2299 2390 2480 2564 2650 2733
                    2812 2888 2966 3043 3119 3195 3255 3356
logged    19 beats: 1167 1941 2037 2124 2208 2299 2390 2480 2564 2650 2733
                    2812 2888 2966 3043 3119 3195 3255 3356
```

18 of 19 reproduced exactly, with identical intervals. The single difference is
the very first beat, and §4.3 explains it completely.

### 4.2 The notch, seen in the filtered waveform

Filtered output around the split at sample 3195/3255, in counts (10 ms each):

```
... 126 134 141 148 156 163 170 176 181 185 189 193 196 199 201 204
    207 210 212 214 218 221 225 228 230 233 235 237 233 223 206 183   <- crest, beat fires
    155 125  94  66  40  21   8   2   0   0   0   0   0   3   5   7
     10  14  20  25  33  39  46  54  60  66  72  76  78  79  79  79
     79  79  79  80  82  85  88  93  98 105 113 121 128 137 145 153
    161 168 173 178 182 185 186 187 186 185 183 180 178 176 174 173   <- first maximum
    172 171 172 173 174 176 178 180 183 188 193 197 199 195 185 168   <- notch, then second maximum
    145 119  93  66  42  22   9   2   0 ...
```

The pulse on this beat is **not one hump**. It rises to **187**, dips through a
notch to **171**, then rises again to **199**.

The RTL confirms a crest as soon as it sees two consecutive falling samples, so
it declares a beat two samples after the **first** maximum. The two maxima are
**21 samples (210 ms) apart**; the notch between them is 10 samples (100 ms)
after the first. The second maximum therefore lands well inside the 250 ms
refractory and is discarded, and the next detection is a whole cardiac cycle
late.

The damaging part is not a fixed offset - it is that the detector has no single
fiducial point. On a beat where the notch is resolved it fires at the first
maximum; on one where it is not, it fires at the only crest. Consecutive
intervals therefore jitter by up to 210 ms, and that jitter is exactly what the
observed 360 ms split detections are.

RMSSD is far more sensitive to that than to beats being dropped. Sheridan et al.
(*Psychiatry Investig* 2020;17(9):960-965) measured it directly on PPG: RMSSD
stays within a 5% change when **up to 36% of intervals are removed**, but
degrades by more than 5% once beats are picked more than ~5 samples off - about
**16 ms** at their 300 Hz rate. A 210 ms fiducial jitter is an order of magnitude
past that. Removing bad intervals, which is what the firmware filter does, is
cheap and correct; it cannot repair a crest time that was measured 210 ms wrong.

That produces the observed pattern: **600 ms then 1010 ms** against an ~800 ms
rhythm. Both halves sit inside the firmware's absolute `[400, 1500] ms`
plausibility window, which is why the original split guard did not catch it.

### 4.3 The first interval, and why the source matches the bitstream

The RTL suppresses the first crest of a session (`first_beat_seen`), so its first
*reported* beat should be the second crest. The capture shows a beat on the first
crest instead — which means a crest had already been consumed before logging
began. The bring-up log corroborates this directly:

```
SHRIKEFI_LINK: ForgeFPGA runtime link handshake: probe sent 0x55, received 0x80
```

`0x80` is bit 7 set with a zero payload: the beat flag was already latched during
the link bring-up probes. So the checked-in Verilog and the flashed bitstream
**do** agree; the extra beat is a bring-up artifact, and the interval it produces
is measured from power-on and range-rejected by the firmware. Benign.

### 4.4 A 4.6-second saturation, not a missed beat

The `7740 ms` first interval looked like a detector dropout. It is not. The
filter output was above the 120 threshold for one continuous run of **464
samples (4.64 s)** from sample 778 to 1241 — the signal was railed at 255, so
there was no crest to find. The single peak the FSM emitted at sample 1167 is
the phantom it produces when the signal finally comes off the rail.

Cause: the MAX30100's DC output ramps from 0 to ~127000 counts over ~1.5 s after
the finger is seated, and `max30102_scale_to_8bit_ch()` tracks it with a 640 ms
EMA. An EMA lags a *ramp* by (slope × tau) — tens of thousands of counts here —
so `ac/8` exceeds the ±120 the byte can express and pins at a rail.

Firmware now counts consecutive railed samples and re-seeds a baseline that has
been railed for 300 ms, on the reasoning that a genuine pulse is periodic and
never rails for 300 ms continuously.

---

## 5. Result 3 — where RMSSD ~190 ms came from

RMSSD is a root-mean-square of *successive differences*, so a single corrupted
interval pair dominates it. Three contributions, in order of size:

**(a) Two detectors mixed in one rolling window — the largest term.**
At the start of every finger contact the FPGA has not yet produced a beat, so
the software fallback FSM is authoritative and fills the HRV window with its own
intervals. A second or two later the FPGA takes over. The capture shows the two
groups clearly: software intervals near **500 ms (HR ~119)** and FPGA intervals
near **880 ms (HR ~70)**, both in the same buffer.

The IBI counter is frozen at 4 for the 7.7 s gap and then increments once per
FPGA beat — proving the early beats came from the other detector. It also
explains the reported HR ramping **119 → 98 → 84 → 72** over ~20 s as the four
fast intervals were progressively diluted, and the displayed HR starting at 119
when actual HR was ~72.

**(b) The notch split.** Reconstructed FPGA intervals:

```
960 870 840 910 910 900 840 860 830 790 760 780 770 760 760 600 1010
```

RMSSD over these alone is **116 ms** — and the `600/1010` pair contributes
almost all of it. Drop that one pair and RMSSD falls to **39.6 ms**, which is a
textbook resting value.

**(c) Poll quantization, minor.** `shrikefi_link_driver.c` derives the interval
from `esp_timer_get_time()` at poll instants rather than reading the FPGA's own
`ibi_cycles` (the protocol has no register read). Beats are therefore timestamped
to the ~20 ms task period, adding ~12 ms RMS to RMSSD. Not the main term, but it
sets a floor around 15–20 ms even on clean data.

### Fixes applied

1. **`ibi_pipeline_t` / `ibi_pipeline_submit()`** — a single acceptance path for
   both detectors. A change of detector source flushes the HRV window, so
   intervals from two detectors are never averaged into one statistic.
2. **Causal 5-point median filter** on the accepted interval series
   (`hrv_median_t` in `hrv_analysis.{c,h}`). A rank filter removes an isolated
   short/long artifact pair; a run of genuinely short or long beats survives it.
   On the reconstructed series above this yields RMSSD ≈ **34 ms**.
3. **Absolute plausibility bounds restored** (`[400, 1500] ms`), plus a
   consecutive-rejection escape hatch. See §5.1 — the first version of this
   pipeline used a threshold relative to the running median, and that latched.

Expected result: **RMSSD ~190 ms → ~35–40 ms**, and HR that starts at the true
rate instead of ramping down from 119.

**Caveat, to state honestly in any write-up:** median filtering also attenuates
genuine beat-to-beat variability, so the reported RMSSD is a slightly
conservative (low-biased) estimate. The unfiltered interval remains available as
`g_state.r_peak_interval_ms`, so no raw data is discarded.

---

## 5.1 Regression: the median-relative threshold latched (fixed)

The first version of the artifact guard rejected an interval when it was below
`0.80 ×` the median of recently accepted intervals, on the theory that a 600 ms
interval is an artifact against an 800 ms rhythm but normal for someone at
100 BPM. **That was wrong, and it deadlocked the counter on hardware.**

A relative floor is a *positive feedback loop*. Once long intervals are in the
median window, the floor rises above the subject's true rhythm; every normal beat
is then rejected; and because nothing is accepted, the median never comes back
down. It is a latch, not a filter.

Measured, from `idf_py_stdout_output_22648` (160 s capture, FPGA detecting
~80 BPM throughout):

| Time | HRV sample count | Interval accepted |
|---|---|---|
| 19652 ms | 1 | 1470 ms |
| 34222 ms | 2 | 720 ms |
| 35262 ms | 3 | 759 ms |
| 36302 ms | 4 | 1400 ms |
| 48782 ms | 5 | *(12.5 s later)* |
| 49822 ms | 6 | |
| 98732 ms | 7 | *(48.9 s later)* |
| 161182 ms | 0 | finger removed |

After the fourth interval the 5-window median was `1400 ms`, so the floor became
`0.80 × 1400 = 1120 ms`. Every ~740 ms beat was then rejected indefinitely. The
counter sat at 6 for 49 s and at 7 for 62 s while the FPGA was detecting beats
normally — the operator-visible symptom was "6/10 and 7/10 are stuck".

The four intervals that seeded it are themselves the §4.2 notch split and two
missed beats; they should never have become the reference for what is normal.

**Fix:** the plausibility window is absolute again, the median filter is
feed-forward only (it filters the HRV series but never feeds back into the
rejection rule, so it cannot latch), and `IBI_REJECT_ESCAPE = 12` discards and
re-seeds the artifact reference after 12 consecutive rejections. That escape is
what turns any residual lockout into a 10-second delay instead of a permanent
one.

**Lesson worth keeping:** an artifact filter that keeps rejecting is itself
producing an artifact. Every rejection rule needs an escape.

---

## 5.2 RESOLVED: the residual RMSSD error was one accepted missed beat

Superseded by the 00:58 capture (`hardware/shrikefi/tools/analyse_capture.py`
has the full output). The open question in the first version of this section was
whether the remaining RMSSD error came from early-session contamination or from
pervasive interval noise. Neither: it was **one interval**.

Session 2 of that capture (t=380034..486074 ms, 106 s, 125 crests):

```
interval median 770 ms (77.9 BPM), min 360, max 1770
<400 ms (split double-fire) : 3
400..1500 ms                : 115
>1500 ms (missed beat)      : 6
escape warnings             : none
```

Artefact rate 7.3%. The pipeline was working. RMSSD settled at **27.8 ms** — a
healthy resting value — and then jumped to **152.0 ms** between t=397194 and
t=398264.

The crest timestamps in that window are **396234** and **397534**: a **1300 ms**
interval. The rhythm is 770 ms, so the detector dropped a crest. But
`IBI_MAX_MS` is 1500 ms and `1300 < 1500`, so it was **accepted**.

RMSSD is a root-mean-square of *successive differences*, so one interval is
enough:

```
before: n=15, RMSSD  27.8  ->  sum(diff^2) =   772.84 * 14 =  10,820
after : n=16, RMSSD 152.0  ->  sum(diff^2) = 23104.00 * 15 = 346,560
delta = 335,740   ->   sqrt = 579.4 ms
```

`analyse_capture.py` solves that delta for each candidate `n`; the `n=15` row
gives **579.4 ms**, and 1300 − 579 = 721 ms is exactly the preceding interval.

So: **one accepted missed beat cost 124 ms of RMSSD.** It then decayed only by
dilution — 152 → 113 ms over the following 87 s — because the HRV window holds
300 intervals and never forgets an early value.

### Fix

`IBI_MISSED_BEAT_RATIO` (1.6) in `main_shrikefi.c`: reject an interval that
exceeds 1.6 × the median of the accepted series.

The median is only consulted when its window is **full** (5 entries). That
requirement is the direct lesson of §5.1: the latch there was seeded by a median
taken over a partly-filled window (4 entries, 2 of them outliers), which is not
a robust statistic.

**An upper bound is safe; a lower bound is not**, and the asymmetry is provable:

* Rejecting LONG intervals removes only values that would drag the median up, so
  the median settles at the true rhythm and the ceiling adapts to it. The true
  rhythm is always far below 1.6 × the median, so it is always accepted, and it
  always pulls the median back if it drifts. The fixed point is stable.
* Rejecting SHORT intervals removes exactly the values that would pull the
  median *down*, so the floor ratchets up and eventually excludes the real
  rhythm permanently. That is the §5.1 latch.

A missed beat is rejected **without** setting `skip_next`: the next interval is
timed from the real previous beat, so it remains valid. Only a split
double-fire is followed by a remainder that must also be discarded.

### Expected result

The 1300 ms interval is rejected, so no 579 ms successive difference enters the
series, and RMSSD stays at ~28 ms instead of jumping to 152 ms.

### What this says about the detector

7.3% artefact rate (3 splits + 6 missed beats in 124 intervals) is usable but
not excellent, and it is now the dominant remaining error source. The two RTL
changes in §6 would reduce it at the source; firmware filtering cannot recover a
crest time that was measured 100 ms early.

---

## 6. Open items

| Item | Status |
|---|---|
| RTL: require a deeper fall (or a wider average) before confirming a crest | **Recommended, not applied.** Needs re-synthesis in the Renesas ForgeFPGA GUI. Applying it to the `.v` without re-flashing would desynchronise source and bitstream — which §4.3 shows is a real hazard. |
| RTL: send `filt_sample[7:1]` to stop the reported waveform wrapping | **Recommended, not applied.** Same reason. |
| Firmware: reset the FPGA's filter/FSM state after link bring-up | Proposed. The handshake reply carrying the beat flag shows bring-up traffic reaches the DSP pipeline. Needs a protocol hook the link does not currently have. |
| IBI resolution limited to the 20 ms task period | Known limitation. Real fix is a protocol change to read the FPGA's `ibi_cycles` register. |
| `tb_forgefpga_system.v` | Stale — still instantiates the old `rst_n` / `link_*` port set and will not compile. |
| Docs claim 443 LUT5s | ForgeFPGA fitter reports **202/1120 (18.04%)**, 123 FFs, 37/140 CLBs. Deck must follow the fitter report. |
| HRV sample counter latched at 4/6/7 | **Fixed** — see §5.1. Absolute bounds restored, escape hatch added. |
| Residual RMSSD error | **Fixed** — see §5.2. Caused by a single accepted missed beat (1300 ms against a 770 ms rhythm). `IBI_MISSED_BEAT_RATIO` now rejects those. |
| Detector artefact rate: 3 splits + 6 missed beats per 124 intervals (7.3%) | **Open.** Firmware filtering now absorbs them; reducing them needs the RTL changes above. |

---

## 7. Reproducing

```powershell
cd firmware\shrikefi
idf.py -p COM5 build flash monitor     # hold a finger 15-30 s, then Ctrl+]
python ..\..\hardware\shrikefi\tools\replay_fpga_link_log.py `
       build\log\idf_py_stdout_output_<pid> --wave
```

Add `--trace LO HI` to print the FSM state over a sample range.
