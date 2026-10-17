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
  - **RR still publishes unavailable** at rest: the respiratory SNR on the
    25 Hz FIFO + peak-detector path is marginal. This is the honest limit.
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
