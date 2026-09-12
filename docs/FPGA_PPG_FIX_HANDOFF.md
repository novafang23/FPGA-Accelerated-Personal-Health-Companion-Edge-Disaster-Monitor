# Handoff — ForgeFPGA PPG pipeline fixes

**Audience:** Antigravity (or any agent/human continuing this work)
**Board:** ShrikeFi — ESP32-S3-WROOM-1-N8R2 + Renesas ForgeFPGA SLG47910C
**Firmware base:** `d5baf3f`
**Head after this work:** `fccfaea`
**Build status:** clean, `shrikefi_health_companion.bin` = 0xf9ed0 bytes

---

## 0. Read this first

Three things changed, in three commits:

| Commit | What |
|---|---|
| `59ebaf1` | HRV window no longer mixes intervals from two detectors; IBI series median-filtered |
| `4a353ac` | Replay tool + evidence report proving the FPGA's 8-tap average runs in hardware |
| `fccfaea` | **Reverts a bug introduced by `59ebaf1`** — the median-relative rejection floor latched |

`fccfaea` matters most. One of the changes in `59ebaf1` looked smarter than the
code it replaced and was actively harmful. Section 3 explains it in full, because
the same mistake is easy to make again.

**Do not re-introduce a rejection threshold that is relative to a running
statistic.** That is the single most important line in this document.

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

## 6. Change: baseline re-seed on sustained ADC railing

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

## 7. Link diagnostic logging

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

## 8. What was deliberately NOT changed

### The RTL

Two changes are recommended but **not applied**, because they require
re-synthesising the bitstream in the Renesas ForgeFPGA GUI:

1. **Require a deeper fall before confirming a crest.** The filtered pulse on
   some beats rises to ~187, dips through the dicrotic notch to ~171, then rises
   to ~199. The FSM confirms a crest after two falling samples, so it latches onto
   the *first* maximum; the second lands ~100 ms later, inside the 250 ms
   refractory, and is discarded. That produces one short and one long interval
   (600 ms then 1010 ms against an ~800 ms rhythm). Requiring a fall of, say, 8%
   of the peak — or widening the averaging window — would fix it at the source.
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

## 9. Files changed

| File | Change |
|---|---|
| `firmware/core/hrv_analysis.h` | Added `hrv_median_t`, `hrv_median_init()`, `hrv_median_push()`, `HRV_MEDIAN_WINDOW` (5), `HRV_MEDIAN_MIN` (3). Header comment carries the rationale and the low-bias caveat. |
| `firmware/core/hrv_analysis.c` | Added `hrv_median_of()` (insertion sort over ≤5 floats) and the two public functions. `hrv_median_peek()` added then removed — see §3. |
| `firmware/shrikefi/main_shrikefi.c` | Added `ibi_pipeline_t`, `ibi_pipeline_reset()`, `ibi_pipeline_submit()`; hoisted `IBI_MIN_MS`/`IBI_MAX_MS` to file scope; added `IBI_REJECT_ESCAPE`; routed **both** detectors through the pipeline; added the rail re-seed; fixed the `NO_FINGER` telemetry mislabel; reset the pipeline on finger removal; rewrote the split-beat comment now that the cause is established. |
| `firmware/shrikefi/shrikefi_dashboard.c` | Added the `ACQUIRING` telemetry branch (Packet 2b). |
| `firmware/shrikefi/shrikefi_link_driver.c` | Decimated logging by default; added `SHRIKEFI_LINK_FULL_RATE_LOG`; documented the 7-bit truncation. |
| `hardware/shrikefi/tools/replay_fpga_link_log.py` | **New.** Bit-accurate RTL replay tool. |
| `reports/FPGA_PPG_WAVEFORM_ANALYSIS.md` | **New.** Full evidence report, including §5.1 on the latch. |

---

## 10. How to build, flash and verify

```powershell
# one-time per shell
. C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1

cd C:\Users\abhin\OneDrive\Desktop\verilog\firmware\shrikefi
idf.py -p COM5 build flash monitor
```

Hold a finger on the MAX30102 for **60 seconds**, then `Ctrl+]`.

**What to look for:**

| Check | Expected |
|---|---|
| `IBI detector handover -> ForgeFPGA` | Exactly one per contact session, shortly after the finger lands |
| `IBI samples: N` | Climbs **monotonically**. If it stalls for more than ~10 s, the escape hatch should log `IBI filter rejected 12 intervals in a row` — report that, it means a new latch |
| `[FPGA ACCEL]` beat cadence | Steady, one per cardiac cycle |
| `HR:` | Starts near the true rate, does **not** ramp down from ~119 |
| `RMSSD:` | ~35–40 ms at rest once the counter passes 10. **Ignore any RMSSD printed with fewer than 10 samples** — with 1–4 intervals it is arithmetic on noise, and values like 744 ms are meaningless |
| `[TELEMETRY]` | `ACQUIRING,...` with real HR/SpO2 while filling, `NO_FINGER` only with no contact |

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

## 11. Open items

| Item | Owner | Notes |
|---|---|---|
| Confirm the counter now climbs steadily | next flash | The whole point of `fccfaea` |
| Recapture at full rate and re-run the replay | next flash | `SHRIKEFI_LINK_FULL_RATE_LOG = 1` |
| Decide whether the RTL crest-confirmation change is needed | needs evidence | If notch pairs persist at >1 in 6 beats after the firmware fix, yes. Firmware cannot recover a crest time measured 100 ms early. |
| WiFi never associates | open | Log shows `reason 201: SSID NOT FOUND`. `Airtel_Abhi-506` in `wifi_credentials.h` is either misspelled or a 5 GHz-only AP — the ESP32-S3 has no 5 GHz radio. **The cloud dashboard receives nothing until this is fixed.** |
| `tb_forgefpga_system.v` does not compile | open | Still instantiates the old `rst_n` / `link_strobe` / `link_dir` / `link_din` / `link_dout` port set. Two copies exist (`hardware/shrikefi/` and `forgefpga_project/ffpga/sim/`). |
| Docs claim 443 LUT5s | open | ForgeFPGA fitter reports **202/1120 (18.04%)**, 123 FFs, 37/140 CLBs. Deck must follow the fitter report. |
| `ppg_sqi.c`, `ppg_respiratory_rate.c` not in the ESP build | known | Cited in docs (Elgendi, Charlton/Addison) but not running on hardware. Do not claim otherwise. |
| Flood/hypothermia risk not instrumented | known | No skin-temperature sensor. The engine reports an ambient proxy, capped at `RISK_HIGH`, and labels it `(ambient proxy)` in the log. State this honestly. |

---

## 12. Guardrails

* **Do not `git add -A`.** The working tree contains another agent's
  `docs/presentation/*` changes and a set of untracked reference files
  (`Web_FPGA_programmer.ino`, `esp.rs`, `universal.rs`, `shrike_*.txt`/`.svg`,
  `vicharak_spi_target.v`, `shrike.pdc`). Stage explicit paths only.
* **Do not edit the RTL and the bitstream independently.** They are a matched
  pair; see §8.
* **Do not make any rejection threshold relative to a running statistic.** See §3.
* **Do not call `hrv_add_ibi()` directly** from either detector. Go through
  `ibi_pipeline_submit()`, or the two-detector mixing bug comes straight back.
* **Do not share baseline or railing-counter state between RED and IR.** See §6.
* **Do not remove the rail re-seed or `IBI_REJECT_ESCAPE`.** Both exist because of
  a measured hardware failure, and both are documented at their definitions.
* If you change a `[TELEMETRY]` packet format, **update both** `main_shrikefi.c`
  and `shrikefi_dashboard.c`.

---

## 13. Reference: how the analysis was done

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
