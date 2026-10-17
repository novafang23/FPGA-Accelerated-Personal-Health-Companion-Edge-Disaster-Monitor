# Session Handoff — 2026-10-17 (DeepSeek AI)

This file is the durable record of the 2026-10-17 hardware/firmware session.
Read this first in a new session; it supersedes chat memory.

## Repository state

- Branch: `main`, clean, pushed.
- HEAD: `383639f` (see `git log --oneline -6`).
- `origin/main` is the only remote branch (four stale branches were deleted).
- 265 tracked files; generated Vivado `.gen/` is untracked and ignored.
- Firmware currently flashed on the board: app `0xf0f80`, including the
  regenerated ForgeFPGA bitstream.

## Commits from this session

```
383639f feat(rtl): deploy the current ForgeFPGA bitstream; prefer AM when FM is weak
e32705e ci: add secret scan, raise NN accuracy floor, verify bitstream header when present
c4f90a7 fix: resolve four correctness/safety defects found in the A-to-Z audit
15b05e9 docs: add project guides and verification reports; group media under docs/
6cbe763 feat(firmware): harden clinical pipeline, calibrate SpO2, remove skin-temp proxy
587e806 chore(repo): untrack generated Vivado output and tighten .gitignore
```

## Hardware / tooling facts

- ESP32-S3 + Renesas ForgeFPGA SLG47910 (ShrikeFi), on `COM4`.
- Power is via the **power port** (charger/power bank); **UART port** to the laptop.
- Connected: MAX30102, BME280, PMS5003, MPU6050, SSD1306 OLED, buzzer, GNSS
  (u-blox NEO-8N class), ForgeFPGA.
- **SOS cancel button: GPIO 5**, active-low to GND, internal pull-up, 5 s hold.
  Enabled by `CONFIG_SHRIKEFI_SOS_CANCEL_GPIO=5` in `sdkconfig.defaults`
  (verified in the boot log: "SOS hardware cancel button configured on GPIO5").
  With no button fitted, the pull-up holds the pin high, so it reads released.
- Renesas ForgeFPGA Workshop (Go Configure Software Hub) is installed and was
  used to re-fit the bitstream.
- ESP-IDF 5.5.5 at `C:\Espressif\frameworks\esp-idf-v5.5.5`.

## What was fixed (and the decisions behind it)

### Repo hygiene
- Untracked 120 generated Vivado `.gen` files (~16 MB incl. DLLs); added
  `*.gen/`, `.venv/`, scratch/build patterns to `.gitignore`.
- Moved root photos/hand-off/verification docs into `docs/`.
- GitHub now shows only `main` and 265 source/evidence files.

### Firmware
- **Skin temperature removed** from acquisition, NEWS2, OLED, telemetry, status
  JSON and both dashboards. The MAX30102 die-temperature proxy read 40–41 °C
  from LED self-heating and produced false "High Fever" CRITICAL alarms.
  `env.skin_temp_c` stays `0.0f` until a **TMP117** is fitted.
- **SpO2 two-point calibration**: `SpO2 = 101.8 − 6.67·R` (constants
  `SPO2_CAL_INTERCEPT`, `SPO2_CAL_SLOPE` in `spo2_engine.h`). Generic
  `110 − 25R` over-reacted (rest +2, breath-hold nadir −5).
  Verified: rest 98.8 vs reference 98; nadir 96.6 vs reference 96.
- **Respiratory-rate hardening**:
  - `PPG_RR_TRUST_MAX_BPM = 24` — never publish a rate that NEWS2 scores +3.
  - RSA depth measured at the winning lag (`2√2σ√r[L]`), not total IBI sigma.
  - AM series detrended; interpolated lag clamped to `PPG_RR_MIN_LAG`.
  - AM (pulse-amplitude) corroboration wired in with `am=`/`amc=` diagnostics;
    AM rate is preferred when FM periodicity is at/below its noise floor.
  - Verified: no false Tachypnea/CRITICAL/SOS in breath-hold tests.
  - **Baseline (DC) respiratory path added** (`firmware/core/ppg_baseline_rr`):
    25 Hz IR -> 1 s baseline -> 5 Hz -> 0.1-0.5 Hz -> autocorrelation. It is a
    third, beat-rate-independent estimate and only publishes as a fallback when
    the IBI/AM path is unreliable, at its own calibrated ACF bar (see the paced
    validation below) and the same 24 br/min ceiling. Boundary-pinned lags and
    pulse-only signals are rejected. Host tests recover 9/12/18 br/min and
    reject a pulse-only signal.
  - **Paced validation (2026-10-17):** paced 12 br/min produced a stable
    13.0-13.4 br/min at ACF 0.48-0.57 (winning lag 22-23 = 4.4-4.6 s), while the
    RSA path still found no periodicity. The baseline path therefore publishes
    at its own calibrated 0.50 ACF bar (the primary path keeps 0.60) after 20 s
    of history; boundary-pinned lags and pulse-only signals are still rejected.
  - **RR at rest** still reads unavailable (8.9 br/min at conf 0.21-0.30): the
    respiratory SNR on the 25 Hz FIFO + peak-detector path is marginal. This is
    the honest limit; a dedicated respiration sensor remains the most reliable
    option.
  - **IBI-over-SPI burst (RTL, verified on hardware):** after a beat byte the
    FPGA queues `0xC3` + the 32-bit 20 ns IBI counter (MSB first); the driver
    reads it and falls back to the timer only if the magic is absent. The five
    dummy burst bytes are gated out of the filter. Simulation 13/13; on hardware
    the log shows `IBI FPGA=...` with the counter matching the beat interval.
  - **100 Hz FIFO + 320 ms pre-smoothing:** the MAX30102 now runs SMP_AVE_1
    (100 Hz), so the peak detector is quantised to 10 ms instead of 40 ms. The
    8-bit IR is pre-smoothed over 32 samples (320 ms) on the ESP32 to reproduce
    the old 25 Hz waveform shaping (the FPGA's 8-tap filter is only 80 ms at
    100 Hz). The software detector (full 18-bit, 400 ms refractory) is the IBI
    source for RSA; it removed the alternans (`r1` +0.27 vs -0.43).
  - **RR now publishes on hardware:** paced 12 br/min -> **11.7 br/min at
    confidence 0.69** via the baseline path. The RSA/FM path locks onto a 3-beat
    harmonic (34 br/min, conf 0.54) and is correctly not published. HR reads
    ~108 during the paced run (86 at rest).
  - **FM boundary rule:** a winning lag pinned to `PPG_RR_MIN_LAG` (or to the top
    of the search) is now refused when the STRONGEST lag itself is the boundary,
    so a 3-beat artifact can no longer be published as ~34 br/min. A fundamental
    selected by the harmonic walk is kept, so the genuine 21.9 br/min near the
    band edge still resolves. Tests: resolution 0/10 outside 1.5 br/min (worst
    0.27). On hardware the FM now reports `lag=-1` when only the artifact is
    present, and RR stays unavailable rather than inventing tachypnoea.
  - **Baseline band extended to 6-36** (was 6-24) with a harmonic guard that
    checks both half-lags (odd and even), so the 30->15 and 12->6 subharmonics
    are rejected. Host tests: 9/12/18/27/30 br/min all resolve; pulse-only
    signal rejected.
  - **Finger-only paced calibration (2026-10-17):**

    | Target | Baseline | FM | Published |
    |---|---|---|---|
    | 6 | 6.4 (conf 0.58) | none | 6.4 |
    | 12 | 11.9-12.8 (0.33-0.51) | none | 12.0-12.4 |
    | 18-19 (counted 27-29/90 s) | 11.7 / 16.7 (0.21-0.31) | 20.4 (lag 4) | 20.7 (0.63) |
    | 27 | - | - | - |

    The baseline is validated at 6-12. At 18-19 the FM publishes 20.7 - a +1.4 to
    +2.7 bias within the lag-4 resolution and normal PPG RR accuracy - while the
    baseline is erratic. Perfusion/contact (SpO2 92-95, SQI 0.3-0.5) is the
    limiting factor. Above ~19 the device withholds; 27 is outside the finger
    sensor's reach without a dedicated respiration sensor.
- **A2 safety**: an absolute-crisis override requires SQI ≥ 0.70 or two
  independent crisis criteria, so one noisy SpO2 cannot fire the SOS.
- **C1**: GPS location is published from static storage (was a dangling
  pointer to a task-local buffer).
- **H1**: SpO2 seating gate compares against the **previous** window's DC
  (`prev_ir_dc`); it previously compared the current window to itself.
- **H2**: `clinical_fuse_triage()` no longer turns `RISK_UNKNOWN` into
  `RISK_NORMAL`; a critical vital still overrides.
- **H3**: SOS state has one dedicated lock (`sos_lock()`/`sos_unlock()`),
  used by both the monitor task and the HTTP handlers.
- Local `sdkconfig` Wi-Fi task moved to **Core 1** (matches
  `sdkconfig.defaults`; `sdkconfig` is gitignored).

### Bitstream
- The committed `forgefpga_bitstream.h` was **stale** (`7cd810b5…`); the
  newly fitted `.bin` is `2b204d00…` (46 408 bytes). `convert_bitstream.py`
  regenerated the header and `--check` now passes.
- Current fit: **362/1120 LUT5s, 204 FFs, 74 CLBs, 0 PLL, 0 BRAM**.
  Evidence refreshed at
  `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log`.
- The docs still quote the previous 363 LUT5s / 202 FFs — needs reconciliation.

### CI
- New secret-scan job (tracked `wifi_credentials.h` / literal Wi-Fi password).
- NN training floor raised 86% → 88%.
- `convert_bitstream.py --check` runs when the vendor `.bin` is present, and
  warns/skips on a clean clone.

## Verification evidence

- `firmware/zynq/test_disaster_risk_engine.c`: `ALL TESTS PASSED`.
- Full firmware host build with `-Wall -Wextra -Werror -pedantic -std=c11`: clean.
- ESP-IDF build + flash: OK (87% partition free).
- `verify_dashboard.py` and `verify_softap_page.py`: PASS.
- Hardware captures:
  - SpO2 rest 98.8 vs 98; nadir 96.6 vs 96.
  - No false Tachypnea/CRITICAL/SOS after the RR hardening.
  - RR unavailable at rest (see above).

## Remaining work / options

1. **Make RR functional.** Options, in order of reliability:
   - add a dedicated respiration sensor (chest band / thermistor / airflow);
   - capture raw PPG at a higher rate and derive respiration offline;
   - accept RR unavailable (current, safe).
   Do **not** lower the confidence bar just to publish.
2. **H7 — unauthenticated SOS cancel on the open AP.** Recommendation:
   keep the remote *trigger*, require a deliberate **5-second hold** for
   remote *cancel* (mirrors the physical button). Not yet implemented.
3. **Docs reconciliation.** Update the ForgeFPGA numbers (363/202 → 362/204/74),
   and reconcile the known contradictions (latency 0.44/0.38/18 µs, Fmax
   "82.73 MHz", DONE/probe liveness, "integer-only", SOS confirm 3 s vs 15 s).
   The human must choose the canonical document per number.
4. **End-to-end hardware verification** of GNSS, PMS5003, MPU6050/fall and
   BME280, since all are connected now.
5. **H5 — Zynq PS I2C slave address** is written unshifted in
   `firmware/zynq/i2c_hal.c`; only matters on a real Zynq board.
6. **Rotate the Wi-Fi password**: an older revision exposed it in git history.
   History must not be rewritten; rotate the credential instead.

## Gotchas / guardrails

- Never force-push or rewrite git history.
- Never lower `PPG_SQI_THRESHOLD_VALID` to make the dashboard look ready.
- Do not restore MAX30102 LED `0x4F` (saturated IR).
- `firmware/shrikefi/wifi_credentials.h` and `sdkconfig` are gitignored; never
  commit them.
- The vendor `FPGA_bitstream_MCU.bin` is gitignored; regenerate the header with
  `python hardware/shrikefi/convert_bitstream.py` after re-fitting.

## Useful commands

```powershell
# Host unit tests
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi `
  firmware/zynq/test_disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c `
  firmware/core/hrv_analysis.c firmware/core/pm25_calibration_int8.c firmware/core/clinical_vitals_engine.c `
  firmware/core/spo2_engine.c firmware/core/ppg_sqi.c firmware/core/disaster_risk_engine.c `
  firmware/core/pressure_trend.c firmware/core/ppg_respiratory_rate.c firmware/shrikefi/sos.c `
  firmware/shrikefi/location.c firmware/shrikefi/web_status.c -lm -o test_engine
./test_engine

# Target build + flash (from firmware/shrikefi)
cmd.exe /d /s /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py build && idf.py -p COM4 flash"

# Bitstream header
python hardware/shrikefi/convert_bitstream.py --check
python hardware/shrikefi/convert_bitstream.py

# Dashboard verifiers
python firmware/shrikefi/dashboard/verify_dashboard.py
python firmware/shrikefi/dashboard/verify_softap_page.py
```

## Hardware verification (2026-10-17, all sensors connected)

Boot log after the final flash confirmed:

- I2C scan: `0x3C` SSD1306, `0x57` MAX30102, `0x68` MPU6050, `0x76` BME280.
- MAX30102 Part ID `0x15`; BME280 Chip ID `0x60`; PMS5003 UART task; GNSS
  NEO-M8N NMEA; MPU6050 (fall detection) — all initialized OK.
- BME280 live: ~28.8 °C / 63.8 % RH. PMS5003 live: PM2.5 ~49.6 µg/m³.
- **Location priority:** live GNSS now takes precedence over the stored NVS
  location in both the SOS card and the status JSON/page; the stored location is
  only the fallback when there is no fix. The USB dashboard shows a `[LOC]` line
  (live coordinate when fixed, otherwise the stored location). The GNSS parser
  additionally requires `satellites > 0` before it will declare a fix, so a
  transient GGA cannot publish a bogus coordinate.
- **GNSS re-test (2026-10-17, after re-seating the wiring):** 3-D fix, 7–8
  satellites, altitude 167.5–167.8 m, HDOP 3.2–5.0. The `[LOC]` line prints the
  live coordinate (`28.56050N, 77.44006E`), confirming the GNSS-first location
  priority end to end. The earlier `[NO UART DATA]` was a loose TX->GPIO16/VCC/GND
  connection.
- GNSS: **3-D fix acquired** — `28.56046N, 77.44006E`, 5–7 satellites,
  altitude 167.5 m, HDOP 1.8–8.1. Cold start to first fix took ~9.5 minutes
  (normal for a cold u-blox NEO-M8N at a window). The parser/UART path and the
  coordinate formatting are verified.
  Note: a stored NVS location (`SRM IST DELIHI-NCR`) takes precedence over the
  live GNSS coordinate on the SOS card and status page; clear it to exercise
  the live-GPS fallback.
- SpO2/HR: validated earlier (rest 98.8 vs 98; nadir 96.6 vs 96).
- No false CRITICAL/SOS during the sensor pass.

Not yet exercised end-to-end: an actual GNSS fix, a physical fall event, and
the PMS5003 response to a known particulate source.

## Compaction note

OpenCode automatic compaction is **on by default** (`compaction.auto = true`,
`keep.tokens = 15000`) and the global config does not override it. This file is
the mitigation: if a summary loses detail, re-read it. To preserve more recent
context, set in `~/.config/opencode/opencode.jsonc`:

```jsonc
{
  "compaction": { "auto": true, "keep": { "tokens": 30000 } }
}
```
