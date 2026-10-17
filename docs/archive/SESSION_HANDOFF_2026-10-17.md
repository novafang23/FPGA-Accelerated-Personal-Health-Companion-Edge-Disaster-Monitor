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
