# Handoff — ForgeFPGA PPG pipeline fixes

**Audience:** Antigravity (or any agent/human continuing this work)
**Board:** ShrikeFi — ESP32-S3-WROOM-1-N8R2 + Renesas ForgeFPGA SLG47910C
**Firmware base:** `d5baf3f`
**Head after this work:** see `git log`; the numbered sections below are stable
**Build status:** clean, `shrikefi_health_companion.bin` = 0xf9f30 bytes

---

## 0. Read this first

Four things changed:

| Commit | What |
|---|---|
| `59ebaf1` | HRV window no longer mixes intervals from two detectors; IBI series median-filtered |
| `4a353ac` | Replay tool + evidence report proving the FPGA's 8-tap average runs in hardware |
| `fccfaea` | **Reverts a bug introduced by `59ebaf1`** — the median-relative rejection floor latched |
| *(latest)* | Missed-beat ceiling: reject intervals above 1.6× the subject's own median |

`fccfaea` matters most. One of the changes in `59ebaf1` looked smarter than the
code it replaced and was actively harmful. Section 3 explains it in full, because
the same mistake is easy to make again.

**Do not re-introduce a rejection threshold that is relative to a running
statistic — unless it is an UPPER bound.** That is the single most important line
in this document. Section 6.1 gives the asymmetry, which is provable rather than
a rule of thumb.

---

## 1. Background: what was actually wrong

The device reported a resting RMSSD of ~190 ms when the subject was sitting still
at ~72 BPM. RMSSD that high is not physiological at rest, and RMSSD feeds the
autonomic-strain terms in the heat, pollution and cold-stress risk engines, so the
patient was being scored as less strained than they really were.

Two captures were taken and analysed:

| Log | Mode | Length | Used for |
|---|---|---|---|
| `idf_py_stdout_output_23036` | every SPI transaction (100 Hz) | 35 s | waveform analysis, RTL replay |
| `idf_py_stdout_output_22648` | every 25th transaction (4 Hz) | 160 s | the latch regression |

Both are in `firmware/shrikefi/build/log/`. They are build artefacts and will be
overwritten by the next `idf.py build`, so copy one aside before rebuilding if you
need to re-run the analysis.

---

## 2. The FPGA was never the problem

This is worth stating plainly, because a lot of effort went into suspecting it.

`hardware/shrikefi/tools/replay_fpga_link_log.py` reconstructs the RTL's 8-tap
moving average in software and compares it against every byte the FPGA actually
returned:

```
rx[n] == (ma[n-1] & 0x7F): 3465/3465 = 100.000%
```

**Zero mismatches across 35 seconds and 3465 samples**, spanning saturation,
finger movement and normal pulsation. A pass-through, a constant, a counter or
noise could not pass that test. The FPGA is executing the same arithmetic as
`forgefpga_ppg_top.v`, on live data, at 100 Hz.

The same model reproduces 18 of the 19 reported beats with identical intervals.

So when something looks wrong in the vitals, **check the firmware first**. That
is where all three defects below were.

---

## 3. Defect 1 (the serious one): the IBI rejection floor latched

### Symptom

The operator-visible symptom was the HRV sample counter freezing:

```
IBI samples: 6 (need 10 for risk engines)   ... for 49 seconds
IBI samples: 7 (need 10 for risk engines)   ... for 62 seconds
```

while the FPGA kept reporting crest detections at a normal rate.

### Evidence

Count change points extracted from `idf_py_stdout_output_22648`:

| Time | Counter | Interval accepted |
|---|---|---|
| 19652 ms | 1 | 1470 ms |
| 34222 ms | 2 | 720 ms |
| 35262 ms | 3 | 759 ms |
| 36302 ms | 4 | 1400 ms |
| 48782 ms | 5 | *(12.5 s later)* |
| 49822 ms | 6 | |
| 98732 ms | 7 | *(48.9 s later)* |
| 161182 ms | 0 | finger removed |

Meanwhile `[FPGA ACCEL] Systolic crest detected!` lines run continuously through
that whole window — roughly 160 beats in 160 seconds. Detection was fine. The
counter was not.

### Root cause

Commit `59ebaf1` replaced the pre-existing fixed plausibility window with a
threshold relative to the running median:

```c
/* THIS IS THE BUG. Do not restore it. */
float med = hrv_median_peek(&p->median);
float lo  = (med > 0.0f) ? (0.80f * med) : IBI_MIN_MS;
if (ibi_ms < lo) { p->skip_next = true; return false; }
```

The intent was reasonable: *a 600 ms interval is an artifact against an 800 ms
rhythm, but perfectly normal for someone at 100 BPM, so judge it against this
subject's own recent beats.*

**But a relative floor is a positive feedback loop:**

1. A couple of long intervals enter the median window. Here those were 1470 ms
   and 1400 ms — a missed beat and the first interval after lock, both of which
   should never have been in the reference set.
2. The 5-window median settles at 1400 ms.
3. The floor becomes `0.80 × 1400 = 1120 ms`.
4. The subject's true rhythm is ~740 ms, so **every** normal beat is rejected.
5. Nothing is accepted, so the median never updates, so the floor never comes
   down.

It is a latch, not a filter. Once it engaged it could only be escaped by a rare
interval that happened to land in `[1120, 1500] ms`, which is why the counter
crawled from 4 to 7 over two minutes instead of climbing steadily.

### Fix

1. **The plausibility window is absolute again** — `[IBI_MIN_MS, IBI_MAX_MS]` =
   `[400, 1500] ms`. These are constants in `main_shrikefi.c` and there is a
   long comment above them explaining why they must stay absolute. Do not
   "improve" them.
2. **The median filter is feed-forward only.** It smooths the series that goes
   into the HRV statistics. It never feeds back into the accept/reject decision,
   so it structurally cannot latch.
3. **`IBI_REJECT_ESCAPE = 12`.** After 12 consecutive rejections the artifact
   reference is discarded and rebuilt from live data, with an `ESP_LOGW`. Any
   residual lockout now costs ~10 s instead of being permanent. This is the
   general lesson: *an artifact filter that keeps rejecting is itself producing
   an artifact, and every rejection rule needs a way out.*

`hrv_median_peek()` was added in `59ebaf1` and **removed** in `fccfaea` precisely
so that nothing can read the median back into the rejection path. If you find
yourself wanting it, that is a signal you are rebuilding the bug.

---

## 4. Defect 2: two detectors averaged into one HRV window

### Symptom

The displayed HR ramped `119 → 98 → 84 → 72` over about 20 seconds while the
subject was at a steady ~72 BPM the whole time.

### Root cause

Two independent peak detectors run on this board:

* the **ForgeFPGA** crest detector, which answers once per SPI transaction and
  timestamps a beat to within ~1 ms, and
* the **software fallback FSM** in `main_shrikefi.c`, which runs on the 18-bit IR
  stream with its own threshold, refractory and confirmation delay.

Whichever is authoritative changes mid-session. At the start of every finger
contact the FPGA has not yet produced a beat, so the software FSM fills the HRV
window with intervals near **500 ms (HR ~119)**. A second or two later the FPGA
takes over with intervals near **880 ms (HR ~70)**. Both ended up in the same
5-minute rolling buffer.

That single ~400 ms step between the two groups was the largest single
contributor to the 190 ms RMSSD. It also produced the ramping HR: the four fast
intervals were progressively diluted as FPGA intervals were added.

Independently confirmed by the IBI counter: it froze for the entire 7.7 s FPGA
gap and then incremented exactly once per FPGA beat, proving the early beats came
from the other detector.

### Fix

`ibi_pipeline_t` + `ibi_pipeline_submit()` in `main_shrikefi.c` — a single
acceptance path that **every** interval from **either** detector goes through.
When the source detector changes, the HRV window and the median filter are
flushed, so intervals measured by two different detectors are never averaged into
one statistic. The handover is logged:

```
I (16072) SHRIKEFI_MAIN: IBI detector handover -> ForgeFPGA; HRV window flushed
          so the two detectors' intervals are never averaged together
```

`ibi_pipeline_reset()` is called on finger removal, and the pipeline state lives
in the PPG task so it is scoped to one contact session.

### Consequence to be aware of

After handover the HRV window is empty and the UI reads `Reading... (0/10)`
again. That is correct behaviour — those early beats were not usable — but it
does cost a couple of seconds of extra lock time.

---

## 5. Defect 3: `[TELEMETRY] NO_FINGER` was printed while a finger was present

### Symptom

For the first ~8 seconds of every contact the host dashboard was told there was
no finger, while the device was locked on and reporting a heart rate and a valid
SpO2.

### Root cause

`main_shrikefi.c` has two vitals-logging branches: one for when the HRV window is
full, one for while it is still filling. The second one printed a hardcoded
`NO_FINGER` telemetry packet regardless of contact state:

```c
printf("[TELEMETRY] NO_FINGER,TEMP=%.1f,HUM=%.1f,PM25=%.1f\n", ...);
```

"HRV window not full" is not the same thing as "no finger", but the code treated
them as the same.

`shrikefi_dashboard.c` parses `[TELEMETRY] NO_FINGER` as "finger absent", so it
discarded the HR and SpO2 values and flickered its contact indicator.

### Fix

* Firmware emits `[TELEMETRY] NO_FINGER,...` only when the signal status really
  is `SIGNAL_STATUS_NO_FINGER`.
* Otherwise it emits a new packet:
  `[TELEMETRY] ACQUIRING,HR=%.1f,SPO2=%.1f,RMSSD=%.1f,TEMP=%.1f,HUM=%.1f,PM25=%.1f`
* `shrikefi_dashboard.c` gained a matching `ACQUIRING` branch (Packet 2b) that
  accepts the values and sets `is_finger_present = 1`, with `sqi = 0.60f` to
  distinguish "acquiring" from "locked".

**If you add any other telemetry packet, update both sides.** The parsers are
`sscanf`-based and silently ignore anything they do not match.

---

## 6. Defect 4: one accepted missed beat cost 124 ms of RMSSD

Found by the 00:58 capture, after the latch fix was in. Analysis:
`hardware/shrikefi/tools/analyse_capture.py`.

### Symptom

RMSSD settled at **27.8 ms** — a healthy resting value, exactly what you want —
then jumped to **152.0 ms** in a single step, and decayed only slowly:
152 → 113 ms over the following 87 seconds.

### Evidence

Session t=380034..486074 ms, 106 s, 125 crests:

```
interval median 770 ms (77.9 BPM), min 360, max 1770
<400 ms (split double-fire) : 3
400..1500 ms                : 115
>1500 ms (missed beat)      : 6
escape warnings             : none
```

The crest timestamps bracketing the jump are **396234** and **397534** — a
**1300 ms** interval. The rhythm is 770 ms, so the detector dropped a crest.
`IBI_MAX_MS` is 1500 ms, and **1300 < 1500, so it was accepted**.

RMSSD is a root-mean-square of *successive differences*, so one interval is all
it takes:

```
before: n=15, RMSSD  27.8  ->  sum(diff^2) =   772.84 * 14 =  10,820
after : n=16, RMSSD 152.0  ->  sum(diff^2) = 23104.00 * 15 = 346,560
delta = 335,740   ->   sqrt = 579.4 ms
```

`analyse_capture.py` solves that delta for each candidate `n`; the `n=15` row
gives **579.4 ms**, and 1300 − 579 = 721 ms is exactly the preceding interval.
Two independent routes to the same number.

It then decayed only by dilution, because the HRV window holds **300** intervals
and never forgets an early value. That is inherent to a 5-minute RMSSD and is not
a bug — but it means one accepted artefact poisons the metric for minutes.

### Root cause

The plausibility window was **absolute** and the absolute ceiling was too high.
A missed beat does not have to exceed 1500 ms: at a 770 ms rhythm it produces
~1300–1770 ms, and everything below 1500 slipped through.

### Fix — and why this one is safe

`IBI_MISSED_BEAT_RATIO = 1.6f` in `main_shrikefi.c`: reject an interval above
1.6 × the median of the accepted series.

Two conditions make it safe, and both are load-bearing:

**1. The median window must be FULL (5 entries) before it is consulted.**
`hrv_median_value()` returns `0.0f` until then. This is the direct lesson of §3:
the latch was seeded by a median over a *partly-filled* window (4 entries, 2 of
them outliers) — not a robust statistic, and `hrv_median_of()` picks the
upper-middle element for an even count, which makes it worse.

**2. It is an UPPER bound only. Never add a lower one.** The asymmetry is
provable, not a rule of thumb:

* **Rejecting LONG intervals** removes only values that would drag the median
  *up*, so the median settles at the true rhythm and the ceiling adapts to it.
  The true rhythm is always far below 1.6 × the median, so it is always accepted,
  and it always pulls the median back if it drifts. The fixed point is stable.
* **Rejecting SHORT intervals** removes exactly the values that would pull the
  median *down*, so the floor ratchets up and eventually excludes the real rhythm
  permanently. That is §3.

A missed beat is rejected **without** setting `skip_next`, because the next
interval is timed from the real previous beat and remains valid. Only a split
double-fire has a remainder that must also be discarded.

### Expected result

The 1300 ms interval is rejected, no 579 ms successive difference enters the
series, and RMSSD stays near 28 ms instead of jumping to 152 ms.

---

## 7. Change: baseline re-seed on sustained ADC railing

Not a correctness bug on its own, but it was costing ~7 seconds of lock time
after every finger placement.

### Symptom

After the finger was seated the byte sent to the FPGA (`tx`) pinned at 255 for
**464 consecutive samples (4.64 s)**. The FPGA's 8-tap average railed with it, so
the peak detector had no crest to find for 7.7 s, and the one beat it reported in
that window was a phantom emitted when the signal finally came off the rail. That
phantom produced a bogus 7740 ms first interval.

### Root cause

The MAX30100's DC output ramps from 0 to ~127000 counts over the ~1.5 s after
contact. `max30102_scale_to_8bit_ch()` tracks it with a 640 ms EMA, and **an EMA
lags a ramp by (slope × tau)** — tens of thousands of counts here. Since the
scaler centres on 120 and clamps to `[0, 255]`, and at `MAX30102_AC_DIV = 8` it
can only express ±120 counts of AC, the output pins at a rail instead of carrying
the pulse.

### Fix

In `task_ppg_accelerator()`, count consecutive railed samples per channel and
re-seed the baseline after 30 of them (300 ms):

```c
s_rail_red = (raw_red == 0 || raw_red == 255) ? (uint8_t)(s_rail_red + 1) : 0;
s_rail_ir  = (raw_ir  == 0 || raw_ir  == 255) ? (uint8_t)(s_rail_ir  + 1) : 0;
if (s_rail_red > 30) { s_base_red = 0; s_rail_red = 0; }
if (s_rail_ir  > 30) { s_base_ir  = 0; s_rail_ir  = 0; }
```

Setting the baseline to 0 makes `max30102_scale_to_8bit_ch()` adopt the current
sample on its next call.

**Why 30 consecutive samples and not an instantaneous test:** a genuine pulse is
periodic, so it rails briefly at the peak and trough of each cycle but never for
300 ms continuously. Only a tracker that has lost the DC level rails that long.
Verified: in the second capture IR reached 134416 within ~1 s of contact and beats
started immediately, with no railed acquisition phase.

**Per-channel counters are mandatory.** RED and IR sit 50–70k counts apart. A
single shared counter state is exactly the bug that was fixed earlier by
`a689ac1` (shared DC baseline); do not reintroduce it.

---

## 8. Link diagnostic logging

`shrikefi_link_driver.c` now has a compile-time switch:

```c
#ifndef SHRIKEFI_LINK_FULL_RATE_LOG
#define SHRIKEFI_LINK_FULL_RATE_LOG 0
#endif
```

* `0` (default) — logs every 25th transaction, ~4 Hz. Enough to see the link is
  alive and what range the samples occupy.
* `1` — logs **every** transaction at 100 Hz (~2.5 KB/s). This is the mode that
  makes RTL verification possible. The console is unreadable while it runs, so
  turn it on, capture 30 s, turn it back off.

Output format: `FG <index> <tx> <rx&0x7F> <beat flag>`.

### The returned value is truncated — this is a known, documented artefact

`forgefpga_ppg_top.v` packs its reply as:

```verilog
tx_data <= {beat_latched, filt_sample[6:0]};
```

Bit 7 is the beat flag, so only the **low seven bits** of the 8-bit moving
average reach the ESP32. The average is a full 8-bit quantity — it spends 29.6%
of samples above 127 and reaches 255 — so **every one of those samples wraps when
read back**. The `Filtered=` field in the `[FPGA ACCEL]` log lines is a sawtooth,
not a waveform (`Filtered=5` is a real value of 133; `Filtered=78` is 206).

**Detection is unaffected.** `ppg_peak_detector` is wired to the full
`filt_sample[7:0]` internally; only the `tx_data` packing truncates. Do not chase
this as a detector bug. It is logged in the report as a recommended RTL change.

---

## 9. What was deliberately NOT changed

### The RTL

Two changes are recommended but **not applied**, because they require
re-synthesising the bitstream in the Renesas ForgeFPGA GUI:

1. **Require a deeper fall before confirming a crest.** The filtered pulse on
   some beats rises to ~187, dips through the dicrotic notch to ~171, then rises
   to ~199. The FSM confirms a crest after two falling samples, so it latches onto
   the *first* maximum. The two maxima are **21 samples (210 ms) apart**, so the
   second lands well inside the 250 ms refractory and is discarded. That produces
   one short and one long interval (600 ms then 1010 ms against an ~800 ms
   rhythm). Requiring a fall of, say, 8% of the peak — or widening the averaging
   window — would give the detector one consistent fiducial point.
   This matters more than the raw artefact count suggests: Sheridan et al.
   (*Psychiatry Investig* 2020;17(9):960-965) show RMSSD tolerates **removing up
   to 36% of intervals** within a 5% change, but degrades by >5% once beats are
   picked more than ~16 ms off. Fiducial jitter of 210 ms is an order of
   magnitude past that, and no removal strategy can repair it.
2. **Send `filt_sample[7:1]`** instead of `filt_sample[6:0]`, so the reported
   waveform stops wrapping at 128.

**Why they were not applied:** the checked-in `.v` and the flashed bitstream are
a matched pair. Editing the Verilog without re-synthesising silently desynchronises
them, and the next person to debug a waveform would be reading the wrong source.
The flashed bitstream is `FPGA_bitstream_MCU.bin`, embedded in
`firmware/shrikefi/forgefpga_bitstream.h`. If you change one, change both and
re-verify with the replay tool.

### The presentation

`docs/presentation/*` belongs to other contributors and was left untouched. It
still contains claims that have been corrected elsewhere in this repo — see
`reports/FINAL_A_TO_Z_SYSTEM_AUDIT_REPORT.md` for the list of banned figures.

---

## 10. Files changed

| File | Change |
|---|---|
| `firmware/core/hrv_analysis.h` | Added `hrv_median_t`, `hrv_median_init()`, `hrv_median_push()`, `hrv_median_value()`, `HRV_MEDIAN_WINDOW` (5), `HRV_MEDIAN_MIN` (3). Header comments carry the rationale, the low-bias caveat, and the warning that the median is only safe as an upper bound. |
| `firmware/core/hrv_analysis.c` | Added `hrv_median_of()` (insertion sort over ≤5 floats) and the three public functions. `hrv_median_peek()` was added in `59ebaf1` then removed in `fccfaea` — see §3. `hrv_median_value()` replaces it and refuses to return anything until the window is full. |
| `firmware/shrikefi/main_shrikefi.c` | Added `ibi_pipeline_t`, `ibi_pipeline_reset()`, `ibi_pipeline_submit()`; hoisted `IBI_MIN_MS`/`IBI_MAX_MS` to file scope; added `IBI_REJECT_ESCAPE` and `IBI_MISSED_BEAT_RATIO`; routed **both** detectors through the pipeline; added the rail re-seed; fixed the `NO_FINGER` telemetry mislabel; reset the pipeline on finger removal; rewrote the split-beat comment now that the cause is established. |
| `firmware/shrikefi/shrikefi_dashboard.c` | Added the `ACQUIRING` telemetry branch (Packet 2b). |
| `firmware/shrikefi/shrikefi_link_driver.c` | Decimated logging by default; added `SHRIKEFI_LINK_FULL_RATE_LOG`; documented the 7-bit truncation. |
| `hardware/shrikefi/tools/replay_fpga_link_log.py` | **New.** Bit-accurate RTL replay tool — checks the FPGA. |
| `hardware/shrikefi/tools/analyse_capture.py` | **New.** Session analyser — checks the firmware pipeline that consumes the FPGA. Reports detector cadence, artefact rate, IBI count progression, RMSSD jumps and what caused them. |
| `reports/FPGA_PPG_WAVEFORM_ANALYSIS.md` | **New.** Full evidence report: §5.1 the latch, §5.2 the missed-beat ceiling. |
| `.gitignore` | Added `.capture/` so raw serial dumps cannot be committed by accident. |

---

## 11. How to build, flash and verify

```powershell
# one-time per shell
. C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1

cd C:\Users\abhin\OneDrive\Desktop\verilog\firmware\shrikefi
idf.py -p COM5 build flash monitor
```

Hold a finger on the MAX30102 for **60–120 seconds**, then `Ctrl+]`.

**What to look for:**

| Check | Expected |
|---|---|
| `IBI detector handover -> ForgeFPGA` | Exactly one per contact session, shortly after the finger lands |
| `IBI samples: N` | Climbs **monotonically**. If it stalls for more than ~10 s the escape hatch logs `IBI filter rejected 12 intervals in a row` — report that, it means a new latch |
| `[FPGA ACCEL]` beat cadence | Steady, ~70–80 crests/min, with occasional splits and missed beats |
| `HR:` | Starts near the true rate, does **not** ramp down from ~119 |
| `RMSSD:` | **~28–40 ms at rest, and it must not spike.** A step from ~28 to >100 ms means an artefact was accepted — see §6 |
| `[TELEMETRY]` | `ACQUIRING,...` with real HR/SpO2 while filling, `NO_FINGER` only with no contact |

Then analyse the capture rather than eyeballing it:

```powershell
python hardware\shrikefi\tools\analyse_capture.py `
       firmware\shrikefi\build\log\idf_py_stdout_output_<pid>
```

It prints the detector's artefact rate and flags every RMSSD jump with the crest
intervals responsible. A clean run shows `escape warnings: none` and no jumps.

### To take a diagnostic capture

1. Set `SHRIKEFI_LINK_FULL_RATE_LOG` to `1` in `shrikefi_link_driver.c`.
2. `idf.py -p COM5 build flash monitor`, hold a finger 30 s, `Ctrl+]`.
3. Copy `firmware/shrikefi/build/log/idf_py_stdout_output_<pid>` somewhere safe —
   the next build overwrites it.
4. Set the flag back to `0` and rebuild.
5. Run the replay tool:

```powershell
python hardware\shrikefi\tools\replay_fpga_link_log.py `
       <path-to-log> --wave
```

Add `--trace LO HI` to print the FSM state over a sample range.

A healthy capture reports `rx[n] == (ma[n-1] & 0x7F): 100.000%`. If it does not,
the bitstream and the `.v` have diverged, or a pin has moved.

---

## 12. Open items

| Item | Owner | Notes |
|---|---|---|
| Confirm RMSSD no longer spikes | next flash | The whole point of the latest change — see §6. Run `analyse_capture.py` and check for jumps |
| Reduce the detector's 7.3% artefact rate | needs a full-rate capture | Measured: 3 splits + 6 missed beats per 124 intervals. Firmware filtering now absorbs them, but they are the dominant remaining error source |
| Decide whether the RTL crest-confirmation changes are needed | needs evidence | If notch pairs persist after the firmware fix, yes. Firmware cannot recover a crest time measured ~210 ms off, and RMSSD degrades past ~16 ms of beat-picking error (Sheridan 2020) |
| WiFi never associates | open | Log shows `reason 201: SSID NOT FOUND`. `Airtel_Abhi-506` in `wifi_credentials.h` is either misspelled or a 5 GHz-only AP — the ESP32-S3 has no 5 GHz radio. **The cloud dashboard receives nothing until this is fixed.** |
| `tb_forgefpga_system.v` does not compile | open | Still instantiates the old `rst_n` / `link_strobe` / `link_dir` / `link_din` / `link_dout` port set. Two copies exist (`hardware/shrikefi/` and `forgefpga_project/ffpga/sim/`). |
| Docs claim 443 LUT5s | open | ForgeFPGA fitter reports **202/1120 (18.04%)**, 123 FFs, 37/140 CLBs. Deck must follow the fitter report. |
| `ppg_sqi.c`, `ppg_respiratory_rate.c` not in the ESP build | known | Cited in docs (Elgendi, Charlton/Addison) but not running on hardware. Do not claim otherwise. |
| Flood/hypothermia risk not instrumented | known | No skin-temperature sensor. The engine reports an ambient proxy, capped at `RISK_HIGH`, and labels it `(ambient proxy)` in the log. State this honestly. |

---

## 13. Guardrails

* **Do not `git add -A`.** The working tree contains another agent's
  `docs/presentation/*` changes and a set of untracked reference files
  (`Web_FPGA_programmer.ino`, `esp.rs`, `universal.rs`, `shrike_*.txt`/`.svg`,
  `vicharak_spi_target.v`, `shrike.pdc`). Stage explicit paths only.
* **Do not edit the RTL and the bitstream independently.** They are a matched
  pair; see §9.
* **Never add a rejection threshold that is a LOWER bound on a running
  statistic.** An upper bound is safe and is used for missed beats; a lower
  bound latched the pipeline on hardware. See §3 for the failure and §6 for the
  proof of the asymmetry.
* **Only consult the median when its window is full.** `hrv_median_value()`
  enforces this by returning `0.0f` early. Do not "optimise" that away.
* **Do not call `hrv_add_ibi()` directly** from either detector. Go through
  `ibi_pipeline_submit()`, or the two-detector mixing bug comes straight back.
* **Do not share baseline or railing-counter state between RED and IR.** See §7.
* **Do not remove the rail re-seed, `IBI_REJECT_ESCAPE` or the missed-beat
  ceiling.** Each exists because of a measured hardware failure and is
  documented at its definition.
* If you change a `[TELEMETRY]` packet format, **update both** `main_shrikefi.c`
  and `shrikefi_dashboard.c`.

---

## 14. Reference: how the analysis was done

`hardware/shrikefi/tools/replay_fpga_link_log.py` is self-documenting and its
module docstring contains the full method and the first capture's results. In
short:

1. Capture the link at full rate (`FG <n> <tx> <rx7> <beat>` per transaction).
2. `rtl_moving_average()` reimplements `moving_average_8tap`
   (`next_sum = running_sum + data_in - shift_reg[7]`, `data_out = next_sum[10:3]`)
   and reconstructs the filter output for every sample.
3. `rtl_peak_detector()` reimplements `ppg_peak_detector` — same state machine,
   same `dyn_threshold = 8'd120`, same 250 ms refractory, same two-sample fall
   confirmation.
4. Compare both against what the hardware returned.

The tool searches pipeline lags 0/1/2 and reports the match rate for each, so a
wiring or bitstream change shows up immediately as a drop from 100%.

Deeper narrative, including the dicrotic-notch waveform trace and the RMSSD
decomposition, is in `reports/FPGA_PPG_WAVEFORM_ANALYSIS.md`.
