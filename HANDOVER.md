# HANDOVER — read this before touching anything

**For:** the next AI agent (Antigravity) or human working in this repo.
**From:** the session that ended at commit `3caa299`.
**Repo:** `FPGA-Accelerated-Personal-Health-Companion-Edge-Disaster-Monitor` (SIH26181, branded **VALOR**).
**Hardware:** ShrikeFi — ESP32-S3-WROOM-1-N8R2 + Renesas ForgeFPGA SLG47910C.

This file is a working record, not marketing. It says what was changed, what was
measured, what broke, what was reverted, and what is still wrong. Where something
is a guess, it says so. Read section 9 before making changes — it lists the
specific things that will waste your time or make the device unsafe.

---

## 0. State right now

| | |
|---|---|
| Branch | `main`, in sync with `origin/main` at `3caa299` |
| Working tree | clean |
| Host tests | 20 tests, **all passing** |
| Firmware | built and flashed to the board, running |
| Uncommitted | nothing |

**Nothing in this project is externally validated.** Every number below is
internally consistent and measured on this one board with one subject. See
section 8.

---

## 1. Environment — the exact things that work

**ESP-IDF v5.5.5** at `C:\Espressif\frameworks\esp-idf-v5.5.5`.

Build and flash (the board is on **COM4**):

```powershell
. C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1
cd firmware\shrikefi
idf.py -p COM4 build flash
```

Exit the serial monitor with **`Ctrl+]`** before running any capture script —
the monitor holds COM4 and the script will fail to open it.

**Two USB-C ports on the board: `POWER` and `UART`. Use `UART` alone** (it
carries both power and data). The bridge is a WCH CH9102.

### Host tests — the exact command

The tests are host-compiled C and are the fastest way to check you have not
broken the algorithm layer. **The obvious `firmware/core/*.c` glob does NOT
work** — it pulls in `mimic_harness.c`, `accuracy_evaluator.c` and
`test_pm25_calibration.c`, each of which defines `main()`, and the link fails.
List the sources explicitly:

```powershell
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi `
  firmware/zynq/test_disaster_risk_engine.c `
  firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c `
  firmware/core/hrv_analysis.c firmware/core/pm25_calibration_int8.c `
  firmware/core/clinical_vitals_engine.c firmware/core/spo2_engine.c `
  firmware/core/ppg_sqi.c firmware/core/disaster_risk_engine.c `
  firmware/core/pressure_trend.c firmware/core/ppg_respiratory_rate.c `
  firmware/shrikefi/sos.c firmware/shrikefi/location.c firmware/shrikefi/web_status.c `
  -lm -o build_host_tests.exe
.\build_host_tests.exe
```

`main()` calls `setvbuf(stdout, NULL, _IONBF, 0)` deliberately: a failing
`assert` calls `abort()`, which does **not** flush stdio, so without it the
diagnostic printed just before the failure is thrown away and all you see is
the assertion text.

### Capture scripts live in `.capture/`

`.capture/` is **gitignored**, so these are on disk but not in git. They are
still the right place for scratch tooling; do not treat their absence from
`git status` as them not existing.

| Script | Purpose |
|---|---|
| `lock_check.ps1` | 25 s — is the sensor actually detecting beats before you spend a minute? |
| `paced_cue.ps1` | Paced-breathing staircase with audible cues + analysis |
| `breath_hold.ps1` | Breath-hold protocol (settle / hold / recover) |
| `rr_forensics.ps1` | Captures `[RRDIAG]` and tabulates what the RR estimator found |
| `sensor_check.ps1` | 100 s — FPGA beats, SpO2 latch time, `[SPO2DIAG]` gate-by-gate breakdown |

All of them take `-Port` (default COM4) and most take `-Seconds`. `paced_cue.ps1`
takes `-DryRun`, which prints the protocol and exits without opening the port —
use it to verify timing changes without holding a finger on the sensor for three
minutes.

---

## 2. The single most important operational fact

**The ForgeFPGA can stop executing while powered, and nothing in the logs says
so.** The symptom is the onboard blue LED (D12) sitting **lit or dark instead of
flickering** with the heartbeat.

Why it is invisible: a wedged chip still drives MISO from a frozen register, so
it replies with a constant byte that reads as a perfectly valid zero sample. No
error, no dropout — the device just silently reports a dead sensor.

**Recovery, in order:**

1. The firmware now **detects and fixes this by itself** (see `1d7cb74`). Every
   ~5 s it checks whether MISO is actually being driven; if not it re-programs
   the FPGA from the embedded bitstream (~150 ms) and resets the HRV and
   respiration state. You will see:
   ```
   E (…) SHRIKEFI_MAIN: [FPGA] MISO is floating - the FPGA has stopped executing
                        (the blue LED will be stuck on). Re-programming it from the embedded bitstream.
   W (…) SHRIKEFI_MAIN: [FPGA] re-programmed; HRV and respiration state reset
   ```
   One occurrence is fine — it recovered. Repeated occurrences mean it is not
   merely wedged.
2. **Reset the board** — `app_main()` re-programs the FPGA at boot, so a reset
   is a valid recovery.
3. **Unplug the USB cable completely, wait ~10 s, plug it back in.** This is the
   only thing that clears a fully wedged chip, because it removes power. A reset
   does *not*: the FPGA keeps power and is only re-streamed.

Also written into `firmware/shrikefi/README.md` under
"⚠ If the blue LED stops flickering".

**The LED is driven by the FPGA, not the ESP32**, from the same internal
`beat_raw` signal that becomes MISO bit 7 (`forgefpga_ppg_top.v`, lines ~174 and
~196). They cannot disagree. So a stuck LED is never a firmware bug — do not go
looking for one.

**Do not confuse "LED off with no finger" with a fault.** With no finger there
are no beats, so off is correct. It only flickers with a finger on.

---

## 3. What this session was about, and why

The user's instruction was blunt: *"i just want my project to work man smoothly
thats all just fix it"*, and later *"man fuck wifi bluetooth i want core logic
and core project to work"*. So: the sensing pipeline, not the connectivity.

Two threads were pulled:

1. **The respiratory rate (RR) never produced a usable value.** Traced through
   four separate estimator defects, all of which are now fixed, and the feature
   is now proven to work.
2. **SpO2 took 20+ seconds** to show a number. Partially improved, then one
   change made it *dangerously* worse and was reverted. This thread is
   **unfinished and should not be re-attempted blind** — see section 7.

A third, unplanned discovery — the FPGA wedge — is section 2.

### The method that worked, and that you should keep using

**Every real bug in this session was found by making the device report its own
internals, not by reading code.** Three examples:

- `[RRDIAG]` showed the RR estimator picking **lag 2** — the very first lag the
  search scans — in 42 of 108 frames. That single field explained a false
  emergency-grade tachypnoea that no amount of staring at the published rate
  could have explained.
- `[SPO2DIAG]` showed SpO2 blocked with **46 consecutive valid windows** and a
  spread of 3.38 against a 2.0 bar — i.e. the window count was fine and the
  settle gate was the problem.
- `[FPGA ACCEL]` prints on every beat the FPGA reports, which gives a beat count
  from the **FPGA side**, independent of anything the ESP32 does with it. That
  is how the FPGA was cleared of blame for the LED.

**Guessing wasted real time.** Three separate attempts at the RR problem were
aimed at the wrong thing before the diagnostics existed. If you are about to
change a threshold "because it looks too strict", add a diagnostic line first.

### Diagnostic lines that exist now

| Line | Emitted by | What it gives you |
|---|---|---|
| `[RRDIAG]` | `main_shrikefi.c`, 1 Hz from `ppg_update_respiration_and_sqi()` | `lag`, interpolated lag, `r` (periodicity), `r1` (lag-1 correlation — the alternans tell), depth, confidence, reliable. `lag=0` means the estimator never ran; `lag=-1` means it ran and found nothing. |
| `[SPO2DIAG]` | `main_shrikefi.c`, 1 Hz while SpO2 is not valid | windows completed, how many passed, `consecutive_valid`, the **rejection reason** (1 DC, 2 AC, 3 perfusion index, 4 ratio R), both channels' DC and AC, the programmed LED currents, the settle spread and verdict |
| `[FPGA ACCEL]` | `shrikefi_link_driver.c`, on every FPGA beat | an FPGA-side beat count. Count these over a minute and you have the rate the **chip** saw. |
| `FG <n> <tx> <rx> <beat>` | `shrikefi_link_driver.c`, every 25th SPI transaction | raw SPI traffic. `SHRIKEFI_LINK_FULL_RATE_LOG` (in that file) logs every transaction — the console becomes unreadable, so turn it on for a 30 s capture and off again. |

---

## 4. Respiratory rate — fixed, and now proven to work

The estimator is `firmware/core/ppg_respiratory_rate.c`. Four defects were found
and fixed. **All four were invisible from the published value**, and the test
that existed could not see any of them.

### 4.1 Result that matters

A paced staircase run (settle → **10 br/min** → **24 br/min**, all in ONE run so
the subject's own drifting baseline appears in neither clean window):

| phase | cued | reported | published |
|---|---|---|---|
| settle | – | never | 0 of 14 |
| **P1** | **10 br/min** | **11.3** | 19 of 27 |
| **P2** | **24 br/min** | **26.9** | 14 of 27 |

A 14 br/min change in the cue produced a **15.6 br/min change in the estimate**.
**The estimator tracks respiration.** The "it is locked on a Mayer wave"
hypothesis that the previous session's mitigation was built on is **refuted**.

### 4.2 The four defects

**(a) The output could only land on a grid** — `72de249`.
The autocorrelation was sampled at integer beat lags and the period formed as
`lag * mean_ibi`, quantising the output to `60000/(k*mean_ibi)`. At an 800 ms IBI
that is 25.0, 18.75, 15.0, 12.5, 10.71 — steps of up to 6.25 br/min, and up to
3.1 br/min of error between them. Coarser than NEWS2's own 21 br/min decision
point and coarser than the 3 br/min agreement band this file grants itself.
**Fixed** by storing the correlation at every lag and fitting a parabola through
the three points around the winner. Worst-case error 3.1 → **0.27 br/min**.

**(b) Octave errors** — `72de249`.
A periodic series correlates at every *multiple* of its period, and when the
fundamental lag is not near an integer the sampled fundamental drops while a
harmonic can still land near one. A clean 21.9 br/min modulation at 800 ms scores
0.68 at its true lag 3 but **0.89 at lag 7** — so the global maximum reports
**half the true rate** (measured: 21.88 → 10.72). **Fixed** by preferring the
lowest local maximum reaching `PPG_RR_HARMONIC_FRAC` (0.60) of the strongest one.

**(c) The band was a search filter, not a verdict** — `2164704`.
Excluding out-of-band lags from the search does not make the estimator refuse an
out-of-band rate — it makes it **snap to the nearest admissible lag and report
that**. A genuine 8 br/min peaks at lag 8.5; with that excluded the winner was
lag 7 (9.74), the parabola clamped, and **9.09 was published as RELIABLE**.
NEWS2 scores ≤8 as +3 and 9–11 as +1: a severe bradypnoea reported as a mild one,
and the `clipped` guard never fired because 9.09 is just inside the band. The
header had claimed since the previous mitigation that "below 9 br/min this now
reports unavailable" — it did not. **Fixed** by searching all lags and applying
the band to the interpolated peak.

**(d) A 2-beat alternans was published as tachypnoea** — `6115bb2`.
`[RRDIAG]` showed the estimator picking **lag 2 in 42 of 108 frames** — every
second of a breath hold and 25 s of normal breathing afterwards. Lag 2 is the
first lag the search scans, so it wins whenever its correlation clears the bar,
and at the capture's 861 ms IBI it is 34.8 br/min: inside the band, so nothing
caught it. It was published as a reliable 27.9–30.4 br/min.
Three separate causes, all fixed:
1. Lag 2 is not respiration — a cycle of two heartbeats cannot be resolved by any
   beat-to-beat method, and at two beats it is indistinguishable from beat
   *alternans*, a common PPG peak-detection artefact. `PPG_RR_MIN_LAG` bars it
   from winning (it is still computed, because lag 3's parabola needs it).
2. **The harmonic search had to obey that bar too.** Barring lag 2 in the main
   search was not enough: with a winning lag of 4, lag 2 is a legitimate L/2, so
   the sub-multiple walk put it straight back.
3. **Interpolation was walking out-of-band peaks INTO the band.** The integer
   winning lag must now already be inside the band before interpolation refines
   it. The file already held that "a rate that only exists because we clamped it
   is not a rate we measured"; interpolation deserved the same rule.

Regression test: `test_rr_rejects_alternans`. Pre-fix it publishes **34.11** for a
true 12; post-fix it reports **11.36 at lag 6**.

### 4.3 Why the old tests could not see any of this

The pre-existing test used 15 br/min at an 800 ms IBI, which is **exactly lag 5**
— a grid point, with the fundamental as the global maximum. It passed for
structural reasons.

`test_rr_resolution()` now places every rate at the **midpoint between two
adjacent lags** (worst case by construction) at two different IBIs. Against the
pre-fix estimator it fails **4 of 10, worst 11.16 br/min**.

### 4.4 The band floor: measured, and left at 9.0 for a *different* reason

`PPG_RR_MIN_BPM` was raised 6.0 → 9.0 in a previous session on evidence of a
6.6–7.3 br/min oscillation with 158 ms of modulation. **That reason is now void:**
at that capture's 880 ms IBI, 6.8 br/min is lag 10, which is also the second
harmonic of a genuine **13.6 br/min** respiratory rate, and lag 10 was admissible
while the floor was 6.0. The original observation was probably defect (b).

A breath-hold test was run 2026-09-28 with the floor temporarily at 6.0 (so a
6–7 br/min oscillation *could* be reported). **No slow rhythm appeared** — so
there is no Mayer wave to guard against.

| phase | frames | RR>0 | RR p50 | depth p50 | conf p50 | HR p50 |
|---|---|---|---|---|---|---|
| A settle | 10 | 0 | – | 135.5 | 0.50 | 66.0 |
| **B HOLD** | 14 | 10 | **27.4** | 273.5 | 0.66 | 82.0 |
| C recover | 14 | 0 | – | 134.1 | 0.50 | 69.1 |

The same hold produced a **reliable 27.4 br/min** while the subject was not
breathing at all — which is defect (d). So the floor stays at **9.0**, but because
**this estimator will report a non-respiratory rhythm confidently at either end of
the band**, not because of Mayer waves. A narrow band is cheap insurance.

### 4.5 What is still open in RR

- **A consistent ~+12 % high reading.** Paced 10 → 11.3, paced 24 → 26.9.
  **This is not an estimator bias**: a synthetic probe at the same rates, depths
  and beat-to-beat noise (`.capture/bias_probe.c`) reproduces **none** of it —
  accurate to under 1 br/min, typically under 0.5. So it is most likely the
  subject anticipating the cue, or a real-RSA waveform shape the sinusoid model
  does not capture. **Only a reference device (T4.1) can separate these.**
- **Barring lag 2 caps the highest reportable rate at about `60000/(3*IBI)`** —
  roughly **23 br/min at 70 bpm**. That is the method's real ceiling, not a
  regression: RSA cannot measure tachypnoea at a resting heart rate. It does mean
  the device cannot see NEWS2's ≥25 band.
- **The RSA depth threshold is saturated.** Depth runs 73–300 ms against a
  10–35 ms scoring window, so the depth term always contributes its maximum and
  discriminates nothing. `PPG_RSA_DEPTH_NONE_MS` / `PPG_RSA_DEPTH_CLEAR_MS` need
  re-scaling from measured data.
- **The IBI buffer needs ~45 s** to reach the 30 beats `PPG_RR_MIN_IBIS` requires
  — not the ~25 s previously assumed. So the settle phase of every paced run was
  still warming up and could never have produced a baseline measurement.

---

## 5. Telemetry contract — keep it append-only

Established convention, and parsers depend on it: **new fields are appended,
never inserted or removed.**

```
[TELEMETRY] HR=,SPO2=,RMSSD=,TEMP=,HUM=,PM25=,RR=,SQI=,RRDEPTH=,RRCONF=
[TELEMETRY] NO_FINGER,TEMP=,HUM=,PM25=
[TELEMETRY] ACQUIRING,HR=,SPO2=,RMSSD=,TEMP=,HUM=,PM25=,RR=,SQI=,RRDEPTH=,RRCONF=
[TRIAGE] NEWS2=,LEVEL=,FLAGS=0x, RISK=,NNHEAT=,NNPOLL=,NNFLOOD=,RHEAT=,RPOLL=,RFLOOD=,PSI=,AHA=,CYCLONE=,PTREND=
[PPG] <raw IR>
FG <n> <tx> <rx> <beat>
[SOS CARD] <framebuffer dump>
```

`0b2f3e2` appended `RR/SQI/RRDEPTH/RRCONF` to the **ACQUIRING** line. Before that,
RR was being computed every second the whole time but only ever *printed* on the
triage line, which is gated behind `vitals_ready` (which needs SpO2 to have
latched). A 100 s paced run therefore showed 92 frames of HR and **not one RR
value**, and the capture could not answer the question it was taken for.

**Telemetry runs at ~0.89 Hz, not 1 Hz** (125 frames over 140 s). Reading a
capture by frame index instead of elapsed time puts phase boundaries ~13 s out —
this caused a warm-up artefact to be mistaken for a measurement. `paced_cue.ps1`
now records elapsed time per row and writes a CSV.

---

## 6. SpO2 — partially improved, then a change that had to be reverted

### 6.1 What helped

`5a74f24` — **22 s → 14 s.** `max30102_adjust_led_current()` ran only on the
50-sample AC window boundary (once every 0.5 s), one step at a time, and on the
MAX30100 the current register is **4 bits — 16 levels**. So reaching the
40000..220000 band took more than a dozen steps, and **every step moves the DC
baseline**, which moves the ratio-of-ratios the SpO2 engine watches. The
acquisition gate asks "has the smoothed estimate stopped moving?", so it was
waiting on the LED driver rather than on the finger.

It was also **ungated by contact**: with no finger the sample reads ambient, so
the loop ramped the current to **maximum**, then had to walk all the way back down
when a finger arrived.

Now: no hunting without a finger, a 10× faster cadence until in band, and the
original slow cadence once SpO2 has latched.

`25e8cb2` — the settle gate used plain max-min over the 8-deep history, which is
decided by the single worst **pair** of entries, so one noisy window vetoed
latching for up to 8 windows (4 s). It now discards the single lowest and highest
entry. A genuine ramp is still caught.

`232821b` — the pulse amplitude was measured as `max - min` over the window,
which is decided by exactly **two of the 50 samples**. Changed to
`2*sqrt(2)*sigma` (the equivalent peak-to-peak of a sinusoid — the same formula
already used for RSA depth in `ppg_respiratory_rate.c`, for the same reason).
For a clean pulse this equals max-min exactly, so **R is unbiased and the
`110 - 25R` calibration still applies** — confirmed by
`test_spo2_clinical_rejection`, which still reports 97.5 % at PI 1.00 %.

### 6.2 What failed, and why it matters

`0c4e5ed` → **reverted in `3caa299`.**

The diagnosis behind it was **correct**: the 0.5 s window holds only **63 % of one
heartbeat** at 76 bpm, so how much of the pulse lands inside depends on where in
the cardiac cycle the window starts. Measured: consecutive windows disagreed by a
**factor of 3** (ir_ac 662 → 2164 with nothing changed).

**The remedy was wrong.** A rolling ~2 s mean and variance is slow to forget the
**finger-placement transient**. On hardware, `ir_ac` ran **208,756 → 10,941 over
18 s**, so the perfusion index stayed out of range for **11 of 16 logged seconds**
and the windows were rejected — and the value that eventually latched was computed
while the variance was still carrying the transient.

**The result was a false desaturation: 85.6 %** on a subject who had read 97–99 %
on every previous run. That drove NEWS2 to 3, the triage to ELEVATED and then
CRITICAL, and the **emergency/SOS card onto the screen.**

**Do not re-attempt this change.** A long rolling span trades a phase-dependent
measurement for a transient-dependent one, and the transient is far more damaging
because it fabricates a desaturation. If you want to fix the phase dependence
properly, the principled route is to size the averaging to a **whole number of
cardiac cycles** using the heart rate the beat detector already provides — but
that is a redesign, and it must be validated against a reference before it goes
near a demo.

### 6.3 Where SpO2 actually stands

- ~20 s to first reading, varying 13–22 s. Slow, but **safe** — it refuses to
  show a number until the estimate has stopped moving.
- The measured values are correct (97–99 % on the test subject).
- **The honest conclusion: this sensor's red channel on this board is not good
  enough for a fast reading.** The fix is a reference comparison (T4.1), not more
  firmware tuning. The user was told this plainly and accepted it.

### 6.4 A safety issue that is still open and worth doing

**A single bad SpO2 reading took the device into emergency mode.** The SOS state
machine currently activates after only **3 s** of a critical triage level.

A device that cries wolf is its own kind of dangerous. **Recommendation: require
the critical state to persist for 15–20 s before the SOS card takes over**, so a
momentary glitch cannot trigger an emergency while a real collapse still would.
This is small, safe, and **independent of the SpO2 speed problem**. It was
offered to the user and not yet answered. See `sos_update()` in `main_shrikefi.c`
and `firmware/shrikefi/sos.c`.

---

## 7. Commit map — what each change was for

| Commit | What |
|---|---|
| `0b2f3e2` | RR/SQI/RRDEPTH/RRCONF appended to the `ACQUIRING` telemetry line |
| `72de249` | RR: parabolic ACF interpolation + harmonic preference (defects a and b) |
| `5691ffe` | docs: reopen T3.5 |
| `6a445e4` | RR floor temporarily to 6.0, for the breath-hold test |
| `803844e` | Breath-hold result: no Mayer wave; floor back to 9.0 for a better reason |
| `4c5e5c0` | docs: T3.5 resolved — the estimator tracks respiration |
| `2164704` | RR: band applied as a verdict, not a search filter (defect c) |
| `6346a66` | RR: expose what the estimator found (`[RRDIAG]`) |
| `6115bb2` | RR: lag-2 alternans no longer published as tachypnoea (defect d) |
| `7d16108` | docs: the blue LED can wedge the FPGA — unplug, do not reset |
| `1d7cb74` | FPGA self-heal: detect a dead FPGA and re-program it |
| `5a74f24` | SpO2: faster LED-current hunt, gated on contact |
| `f233a98` | SpO2: record which acquisition gate fails (`[SPO2DIAG]`) |
| `25e8cb2` | SpO2: robust settle spread (discard the extremes) |
| `224ad5c` | SpO2: log red DC + programmed LED currents |
| `232821b` | SpO2: measure the pulse from all 50 samples, not the two extremes |
| `0c4e5ed` | SpO2: rolling-window estimator — **WRONG, reverted** |
| `3caa299` | Revert of `0c4e5ed` |

Files touched: `firmware/core/ppg_respiratory_rate.{c,h}`,
`firmware/core/spo2_engine.{c,h}`, `firmware/shrikefi/main_shrikefi.c`,
`firmware/shrikefi/shrikefi_link_driver.{c,h}`, `firmware/shrikefi/max30102.{c,h}`,
`firmware/zynq/test_disaster_risk_engine.c`, `firmware/shrikefi/README.md`,
`docs/REMAINING_WORK.md`.

---

## 8. What is verified vs what is assumed

Be careful with this distinction — it is the difference between a demo and a
claim you can defend.

**Verified by measurement on this board:**
- The RR estimator tracks respiration (section 4.1).
- The FPGA detects heartbeats at the correct rate (116 beats in 100 s = 70 bpm,
  and independent confirmation from the 3-of-400 sampled beat flags).
- The FPGA is programmed successfully at every boot (46,408 bytes,
  "configuration COMPLETE", MISO actively driven).
- 20 host tests pass, including the safety cases (a drifting SpO2 acquisition
  must not be published; a jittery RR series must not be published).
- The FPGA-side beat rate agrees with the ESP32-side HR.
- The LED is driven from the same signal as the beat flag (RTL, not measurement).

**Internally consistent but NOT externally validated:**
- Every HR, HRV, SpO2 and RR *value*. There is no reference device. The ~12 %
  RR offset (4.5) and the SpO2 accuracy both fall in this bucket.

**Known-good baselines you must not misquote:**
- Zynq: `185 LUT, 16 LUTRAM, 266 FF, 0 DSP48, 0 BRAM`, WNS **+5.603 ns**,
  69.45 MHz.
- ShrikeFi ForgeFPGA fit: **363/1120 LUT5s (32.41 %)**, 202 FFs, 75/140 CLBs.
  Achievable period **12,087 ps (82.73 MHz)**; at 50 MHz the margin is
  **+7.913 ns**.
- **Never quote WNS −10.088 ns.** It is an artefact of the auto-generated
  2000 ps constraint, not a real result.
- Model: **6→24→16→3, 619 INT8 params, 576 MACs**. The 0.44 µs figure is an
  x86-64 host measurement and nothing to do with the FPGA.

**Measured sample rate ≈ 105/s** (7838 `[PPG]` lines over 81 telemetry
intervals). "50 Hz" labels describe the *poll loop*, which drains ~2 samples per
20 ms. **CPU is 160 MHz**, not the 240 MHz some docs still say.

**FPGA↔MCU link is 8-bit SPI mode 0 on `SPI2_HOST`** — GPIO10=CS (manual),
11=MOSI, 12=SCK, 13=MISO, 8=EN, 9=PWR. FPGA pads: PIN_16=SCK, PIN_17=SS_n,
PIN_18=MOSI, PIN_19=MISO. **Not** the 4-bit parallel nibble bus most older docs
describe — that specification was retired before implementation.

---

## 9. Things that will waste your time, or hurt the user

1. **Do not re-attempt the rolling-window SpO2 change.** See 6.2. It produced a
   false desaturation and a false emergency.
2. **Do not go looking for a firmware cause for the blue LED.** It is driven by
   the FPGA from `beat_raw`. A stuck LED means the chip has stopped, full stop.
   See section 2.
3. **Do not move the `s_last_fpga_beat_ms = now_ms` latch inside the
   `ibi_pipeline_submit()` block.** It was tried. It livelocks: `submit()` returns
   false for every "prime" interval, priming needs consecutive plausible beats,
   and the first interval after a handover resets the counter — so liveness never
   latches, the software fallback keeps running, and every software beat hands the
   source back and flushes the window. Measured in that state: the two detectors
   alternated on every beat, `hrv_state.count` stayed at 0 forever, and the device
   sat on ACQUIRING with a perfect signal. The reasoning is documented at the call
   site. Read it before touching it.
4. **Do not trust `firmware/core/*.c` as a compile glob** (section 1).
5. **Do not use `max - min` as an amplitude estimate anywhere.** It is decided by
   two samples. Use `2*sqrt(2)*sigma`.
6. **Do not call `CLAUDE.md`/`AGENTS.md` assumptions facts.** `AGENTS.md` is
   **gitignored** (`.gitignore:97`), so corrections in it are local-only and not
   in any clone.
7. **Never `git add -A`** — Antigravity works concurrently in this tree. Stage
   explicit paths. Use `git commit -F <file>` for long messages.
8. **Do not force-push or rewrite history.**
9. **PowerShell traps that have already cost time here:**
   - `[System.IO.File]` resolves relative paths against the *process* CWD, not
     `Set-Location`. Use absolute paths.
   - `$Home` is read-only.
   - `@(@(a,b))` flattens to a single element — this once silently corrupted a
     whole source file (`$s[0]` became a character and every `F` became `P`).
     Restore from git rather than trying to patch such damage.
   - `pwsh` is **not on PATH inside the agent harness shell** (it is in the user's
     shell). Invoke scripts with `& "path\to\script.ps1"`.
   - `git show HEAD:file > out` through PowerShell injects NUL bytes and fails to
     compile. Use `cmd /c "git show HEAD:file > out"`.
   - A PowerShell format string continued onto the next line needs a trailing
     backtick before the newline, or `-f` is parsed as a separate command.
   - PowerShell has no `if` *expression*: `(if (…) {…} else {…})` is a syntax
     error. Assign to a variable first.
   - `Get-Content` line counts can disagree with the file's real line count; trust
     the byte length.
10. **`--` placeholders in analysis output mean the script did not run.** Do not
    read them as data.
11. **`docs/REMAINING_WORK.md` is the requirements scorecard** and is kept
    honest. Update it when you change something it describes — it currently
    records T3.5 as RESOLVED and describes the SpO2 situation accurately.
12. **Nothing is externally validated.** Do not let any document imply otherwise.

---

## 10. Where to go next

**Suggested order:**

1. **Emergency-trigger persistence** (6.4). Small, safe, independent, and it
   removes a false-alarm path the user has already seen fire. Do this first.
2. **T4.1 — validate against a reference device.** This is the only thing that
   turns any of the numbers into defensible claims. Recommended stack:
   **AD8232 (~₹700) + fingertip oximeter (~₹1,500)**, under ₹2,200 total. Use
   **Bland–Altman limits of agreement, not correlation** — a high *r* with wide
   LoA is worthless. **Do not use a watch's PPG-derived HRV as ground truth.**
   This also resolves the unexplained ~12 % RR offset (4.5).
3. **T3.3** — cosmetic: handover log spam (~30 min).
4. **T1.7** — README images, blocked on images from the user.
5. **T2.1** — MAX30205 skin temperature (~₹200). Closes requirement 1's only gap.
6. **T2.2** — SOS button + buzzer; also unlocks the phone POST trigger.
7. **T2.3** — IMU.

**Do not** start another SpO2 accuracy effort without a reference device in hand.

---

## 11. Demo notes

- Flash the **UART** port only. Join WiFi `VALOR-0081` (open), then
  `http://192.168.4.1`. The SoftAP **replaces** the WiFi station path
  (`CONFIG_SHRIKEFI_SOFTAP`); cloud publishing is off by default.
- **Power the board on at least 30 minutes before showing it.**
  `CYCLONE=UNKNOWN` in every triage line for the first half hour is expected — the
  storm advisory needs 30 minutes of pressure history at 1 sample/min. It looks
  like a fault and is not.
- SpO2 shows `--` for ~20 s after a finger goes on. That is the safety design,
  not a hang. Do not put a finger on and immediately declare it broken.
- The blue LED flickers once per heartbeat **only with a finger on**. Off with no
  finger is correct.
- If the LED goes solid, see section 2.
- `[FPGA ACCEL] Systolic crest detected!` printing about once a second with a
  finger on is the cleanest single proof the whole sensing chain is alive.
