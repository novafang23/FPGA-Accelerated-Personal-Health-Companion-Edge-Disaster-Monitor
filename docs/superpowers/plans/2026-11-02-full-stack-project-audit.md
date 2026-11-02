# Full-Stack Project VALOR Audit (RTL to AI to Firmware to Dashboards) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Execute a comprehensive, zero-trust end-to-end audit across every layer of Project VALOR (Verilog RTL, DSP & Clinical Algorithms, TinyML INT8 Neural Network, ESP32-S3 FreeRTOS Firmware, Desktop & SoftAP Wi-Fi Dashboards, Python Tooling, and Documentation/Presentation Claims), closing any test coverage gaps introduced by the wireless desktop dashboard and certifying 100% gate compliance.

**Architecture:** The audit progresses bottom-up across 6 self-contained subsystem tasks using the repository's four specialized auditor checklists (`verilog-rtl-auditor`, `c-firmware-safety`, `html-offline-a11y-auditor`, `python-tooling-auditor`) plus automated C/Verilog/headless-Chrome verification harnesses. Synthesizable RTL (`hardware/**/*.v`) and core clinical math (`firmware/core/*.c`) remain frozen unless a P0 defect is proven by a failing test; verification scripts (`verify_dashboard.py`, `verify_softap_page.py`, `predemo_verify.ps1`) are strengthened to permanently guard the new Wi-Fi desktop mode and gzipped flash bundle.

**Tech Stack:** Verilog-2001 (Verilator 5.x, Icarus Verilog `iverilog`/`vvp`), C11 (MSYS2 UCRT64 GCC `-Wall -Wextra -Werror -pedantic -Wdouble-promotion`, ESP-IDF v5.5.5 for ESP32-S3), Python 3 (`.venv`), Headless Google Chrome (DOM & CSP verification), PowerShell (`scripts/predemo_verify.ps1`).

**Spec:** `AGENTS.md`, `.agents/skills/verilog-rtl-auditor/SKILL.md`, `.agents/skills/c-firmware-safety/SKILL.md`, `.agents/skills/html-offline-a11y-auditor/SKILL.md`, `.agents/skills/python-tooling-auditor/SKILL.md`, `docs/HARDWARE_ARCHITECTURE.md`, `docs/MASTER_PROJECT_GUIDE.md`.

## Global Constraints

- **Hardware Baselines (Immutable):**
  - Zynq-7000 (`xc7z020clg400-1`, Vivado ML 2022.2 OOC): WNS +5.603 ns, WHS +0.184 ns, Fmax 69.45 MHz @ 50 MHz target; 185 LUT, 16 LUTRAM, 266 FF, 0 DSP48, 0 BRAM; `tb_ppg_system.v` 6/6 passing.
  - ShrikeFi ForgeFPGA (`SLG47910`): 515 / 1120 LUT5 (45.98%), 303 FF, Fmax 78.85 MHz, WNS +7.316 ns; `tb_forgefpga_system.v` 13/13 passing; `tb_ppg_peak_detector.v` 4/4 passing.
- **TinyML & Clinical Baselines (Immutable):**
  - Architecture `6-24-16-3`, 619 INT8 parameters (576 weights + 43 biases), 576 MACs, 88.47% FP32 / 88.39% INT8 teacher agreement, 0.44 µs host x86 latency.
  - MIMIC-III v1.4 evaluation feed (`data/mimic/mimic_eval_feed.csv`): 16,387 rows across 98 patients; 94.11% accuracy, 58.18% sensitivity, 98.19% specificity, >=99.0% FP32/INT8 agreement.
- **Safety & Offline Guardrails:**
  - All FPGA and MCU I/O pins are strictly 3.3 V LVCMOS maximum.
  - ESP32-S3 has a single-precision FPU only: zero unguarded `double` promotions in target firmware (`-Werror -Wdouble-promotion`).
  - 100% offline operation: zero external CDN links, fonts, scripts, or stylesheets in any HTML/C file; strict SHA-256 CSP script hashes.
  - Never force-push or rewrite git history; leave personal untracked PDFs/3MF files untouched.

## Review Focus

1. **Stale Gzip Flash Payload Drift (`valor_dashboard_gz.h` vs `valor_dashboard.html`):** If `valor_dashboard.html` is edited in the future without re-running `scripts/bundle_desktop_dashboard.py`, `GET /desktop` on the ESP32-S3 will silently serve an outdated dashboard while local tests pass on the raw HTML file. → Pin with an automated byte-for-byte gzip decompression assertion in `firmware/shrikefi/dashboard/verify_dashboard.py` (Task 5).
2. **Untested Wi-Fi Transport & `wifiPPG(dt)` Synthesizer in Headless Chrome:** `verify_dashboard.py` currently tests `idle`, `sim`, `full`, `profile`, `tooltip`, and `serial` modes, but does not execute `connectWifi()` (`mode === 'wifi'`), `/status.json` polling, `wifiPPG(dt)` waveform generation, or `contact: false` flatlining over Wi-Fi. → Add the `wifi` test case in `firmware/shrikefi/dashboard/verify_dashboard.py` (Task 5).
3. **Uncommitted Working Tree Changes (`test_disaster_risk_engine.c`, `FPGA-SHRIKE.ffpga`, `docs/DEMO_SHEET.md`):** Working tree contains an uncommitted skin-temperature calibration gap guard in `test_disaster_risk_engine.c`, a timestamp touch in `FPGA-SHRIKE.ffpga`, and a new `docs/DEMO_SHEET.md`. → Audit, verify against all gates and claim rules, and cleanly commit or revert in Task 1 & Task 6.
4. **ESP-IDF HTTP Server URI Handler Slot Exhaustion:** `web_status_start()` registers 13 URI handlers (`GET /`, `GET /desktop`, `GET /dashboard`, `GET /status.json`, `POST /location`, `GET /history.json`, `GET /api/history`, `POST /api/emergency/trigger`, `POST /api/emergency/cancel`, plus 4 `HTTP_OPTIONS` handlers). If `max_uri_handlers` is ever < 13, later registrations silently fail with `ESP_ERR_HTTPD_HANDLERS_FULL`. → Verify `hcfg.max_uri_handlers >= 20` and assert handler registration count in `verify_softap_page.py` (Task 4 & Task 5).
5. **Presentation Claim Warnings in `README.md`, `MASTER_PROJECT_GUIDE.md`, and `DEMO_SHEET.md`:** `check_presentation_claims.py` flags 9 non-fatal `[WARN:SENSITIVITY-HIDDEN]` notices where 94.11% MIMIC-III accuracy is cited without adjacent 58.18% sensitivity, and must also scan `docs/DEMO_SHEET.md`. → Audit and clean up in Task 6.

---

### Task 1: Working Tree Hygiene, Uncommitted Diffs & Baseline Gate Replay

**Files:**
- Inspect/Modify: `firmware/zynq/test_disaster_risk_engine.c:2547-2580`
- Inspect/Restore: `hardware/shrikefi/forgefpga_project/FPGA-SHRIKE.ffpga`
- Create: `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`

**Interfaces:**
- Produces: Clean, deterministic git baseline and initialized audit report `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md` consumed by Tasks 2–6.

- [ ] **Step 1: Inspect and resolve working tree diffs**

Run:
```powershell
git status --short
git diff firmware/zynq/test_disaster_risk_engine.c hardware/shrikefi/forgefpga_project/FPGA-SHRIKE.ffpga
```
Expected:
- `hardware/shrikefi/forgefpga_project/FPGA-SHRIKE.ffpga` has only a metadata `lastChange` / `projectChecksum` touch from opening Go Configure Hub with zero logic changes. Restore it with `git checkout -- hardware/shrikefi/forgefpga_project/FPGA-SHRIKE.ffpga` so hardware artifacts stay bit-frozen.
- `firmware/zynq/test_disaster_risk_engine.c` has the explicit `[KNOWN GAP]` regression guard in `test_skin_estimate_monotonic_and_bounded()` documenting the nominal `b0/b1` offset (`34.93 C` vs `36.1 C` ref) while asserting `estimated_skin_c` stays within `[32.0f, 37.0f]`.

- [ ] **Step 2: Compile and run `test_disaster_risk_engine.c` with strict C11 flags**

Run:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
gcc -Wall -Wextra -Werror -pedantic -Wdouble-promotion -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi firmware/zynq/test_disaster_risk_engine.c firmware/core/disaster_risk_engine.c firmware/core/clinical_vitals_engine.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c firmware/core/ppg_sqi.c firmware/core/pm25_correction.c firmware/core/skin_estimate.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/shrikefi/web_status.c firmware/shrikefi/emergency_assist.c -lm -o "$env:TEMP\test_engine_audit.exe"
& "$env:TEMP\test_engine_audit.exe"
Remove-Item "$env:TEMP\test_engine_audit.exe" -ErrorAction SilentlyContinue
```
Expected: Exits 0 with `ALL TESTS PASSED` and prints `[KNOWN GAP] neutral finger: model 34.93 C vs reference 36.10 C`.

- [ ] **Step 3: Initialize `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md` and record Section 1 (Baseline & Hygiene)**

Create `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md` with sections for:
1. Executive Summary & Gate Scorecard
2. Layer 1: Verilog RTL & FPGA Synthesis Audit (`verilog-rtl-auditor`)
3. Layer 2: DSP, Clinical Algorithms & TinyML Audit
4. Layer 3: ESP32-S3 Firmware, Concurrency & Memory Safety Audit (`c-firmware-safety`)
5. Layer 4: Desktop & SoftAP Wi-Fi Dashboards Audit (`html-offline-a11y-auditor`)
6. Layer 5: Python Tooling & Presentation Claims Audit (`python-tooling-auditor`)

- [ ] **Step 4: Commit the skin-estimate regression test guard**

Run:
```powershell
git add firmware/zynq/test_disaster_risk_engine.c docs/presentation/FULL_STACK_AUDIT_2026-11-02.md
git commit -m "test(core): document nominal skin estimator calibration gap in unit test"
```

---

### Task 2: Hardware & Verilog RTL Deep Audit (`verilog-rtl-auditor`)

**Files:**
- Audit (Read-only): `hardware/common/moving_average_8tap.v`, `hardware/common/ppg_peak_detector.v`, `hardware/shrikefi/spi_target.v`, `hardware/shrikefi/forgefpga_ppg_top.v`, `hardware/zynq/axi_ppg_accelerator.v`, `hardware/shrikefi/forgefpga_ppg_flat.v`
- Modify: `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`

**Interfaces:**
- Consumes: `.agents/skills/verilog-rtl-auditor/SKILL.md` 10-point checklist.
- Produces: Completed Layer 1 RTL audit table and simulation/lint verification log in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`.

- [ ] **Step 1: Run strict Verilator lint and vendor-purity checks on all RTL modules**

Run:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;C:\iverilog\bin;$env:PATH"
$env:VERILATOR_ROOT = "C:\msys64\ucrt64\share\verilator"
verilator_bin.exe --lint-only -Wall -Wno-UNUSED -Wno-UNOPTFLAT -Wno-SYNCASYNCNET --top-module axi_ppg_accelerator -Ihardware/common hardware/zynq/axi_ppg_accelerator.v hardware/common/ppg_peak_detector.v hardware/common/moving_average_8tap.v
verilator_bin.exe --lint-only -Wall -Wno-UNUSED -Wno-UNOPTFLAT -Wno-SYNCASYNCNET -Wno-PROCASSINIT --top-module forgefpga_ppg_top -Ihardware/common hardware/shrikefi/forgefpga_ppg_top.v hardware/shrikefi/spi_target.v hardware/common/ppg_peak_detector.v hardware/common/moving_average_8tap.v
.venv\Scripts\python.exe hardware/shrikefi/gen_flat_source.py --check
Select-String -Path hardware/common/*.v -Pattern 'AXI|axi_|BUFG|IBUF|MMCM|PLLE'
```
Expected: Both Verilator runs exit 0; `gen_flat_source.py --check` exits 0 (`OK: forgefpga_ppg_flat.v is up to date`); `Select-String` returns 0 matches (confirming `hardware/common/` is 100% vendor-agnostic).

- [ ] **Step 2: Run all 3 Icarus Verilog testbenches (`tb_ppg_system.v`, `tb_ppg_peak_detector.v`, `tb_forgefpga_system.v`)**

Run:
```powershell
$env:PATH = "C:\iverilog\bin;$env:PATH"
iverilog -o "$env:TEMP\sim_zynq.vvp" hardware/zynq/tb_ppg_system.v hardware/zynq/axi_ppg_accelerator.v hardware/common/moving_average_8tap.v hardware/common/ppg_peak_detector.v && vvp "$env:TEMP\sim_zynq.vvp"
iverilog -o "$env:TEMP\sim_common.vvp" hardware/common/tb_ppg_peak_detector.v hardware/common/moving_average_8tap.v hardware/common/ppg_peak_detector.v && vvp "$env:TEMP\sim_common.vvp"
iverilog -o "$env:TEMP\sim_shrike.vvp" hardware/shrikefi/tb_forgefpga_system.v hardware/shrikefi/forgefpga_ppg_top.v hardware/shrikefi/spi_target.v hardware/common/moving_average_8tap.v hardware/common/ppg_peak_detector.v && vvp "$env:TEMP\sim_shrike.vvp"
Remove-Item "$env:TEMP\sim_*.vvp" -ErrorAction SilentlyContinue
```
Expected:
- Zynq testbench: `6 passed, 0 failed`
- Common peak detector testbench: `ALL 4 TESTS PASSED`
- ShrikeFi ForgeFPGA system testbench: `13 passed, 0 failed`

- [ ] **Step 3: Audit every RTL file line-by-line against the 10-point `verilog-rtl-auditor` checklist**

Inspect `moving_average_8tap.v`, `ppg_peak_detector.v`, `spi_target.v`, `forgefpga_ppg_top.v`, and `axi_ppg_accelerator.v` for:
1. Blocking `=` in sequential `always @(posedge clk)` blocks (expect 0).
2. 2-stage CDC synchronizers on SPI signals (`sck_sync`, `ss_sync`, `mosi_sync` in `spi_target.v` and reset synchronizer in `forgefpga_ppg_top.v`).
3. Combinational latch inference (every `always @(*)` has default assignments and full `case` `default:` branches).
4. Complete reset initialization of all sequential registers.
5. Explicit named port connections `.port(sig)` on all module instantiations.
6. Refractory period and watchdog timing constants derived from `CLK_FREQ_HZ`.
7. Sensitivity lists (`@(posedge clk)` or `@(*)`).
8. Counter bit-widths sized for the 1,120-LUT ForgeFPGA ceiling (515 LUT5 / 303 FF used).

- [ ] **Step 4: Record Layer 1 findings in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md` and commit**

Run:
```powershell
git add docs/presentation/FULL_STACK_AUDIT_2026-11-02.md
git commit -m "docs(audit): complete Layer 1 Verilog RTL and testbench audit"
```

---

### Task 3: Core Clinical Algorithms, DSP & TinyML AI Pipeline Audit

**Files:**
- Audit (Read-only): `firmware/core/clinical_vitals_engine.c`, `firmware/core/disaster_risk_engine.c`, `firmware/core/hrv_analysis.c`, `firmware/core/spo2_engine.c`, `firmware/core/ppg_sqi.c`, `firmware/core/pm25_correction.c`, `firmware/core/skin_estimate.c`, `firmware/core/nn_risk_model.c`, `firmware/core/nn_risk_model_int8.c`, `firmware/core/accuracy_evaluator.c`, `firmware/core/train_nn_risk_model.py`
- Modify: `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`

**Interfaces:**
- Produces: Certified Layer 2 clinical & TinyML verification matrix in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`.

- [ ] **Step 1: Verify FP32 vs INT8 parity (`compare_harness.c`), PM2.5 correction (`test_pm25_correction.c`), and MIMIC-III benchmark (`accuracy_evaluator.c`)**

Run:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/core firmware/core/test_pm25_correction.c firmware/core/pm25_correction.c -lm -o "$env:TEMP\test_pm25.exe" && & "$env:TEMP\test_pm25.exe"
gcc -Wall -Wextra -Ifirmware/core -Ifirmware/zynq firmware/zynq/compare_harness.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c firmware/core/disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c -lm -o "$env:TEMP\compare_harness.exe" && & "$env:TEMP\compare_harness.exe"
gcc -O2 -Ifirmware/core firmware/core/accuracy_evaluator.c firmware/core/clinical_vitals_engine.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c firmware/core/disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c -lm -o "$env:TEMP\acc_eval.exe"
& "$env:TEMP\acc_eval.exe" data/mimic/mimic_eval_feed.csv
Remove-Item "$env:TEMP\test_pm25.exe","$env:TEMP\compare_harness.exe","$env:TEMP\acc_eval.exe" -ErrorAction SilentlyContinue
```
Expected:
- `test_pm25.exe` exits 0 (`ALL PM2.5 CORRECTION TESTS PASSED`).
- `compare_harness.exe` exits 0 with 100% profile tier parity.
- `acc_eval.exe` outputs `METRICS_JSON` with `"total":16387`, `"accuracy":94.11`, `"sensitivity":58.18`, `"specificity":98.19`, `"agreement_pct":99.95`.

- [ ] **Step 2: Verify TinyML training reproducibility and parameter footprint (`train_nn_risk_model.py`)**

Run:
```powershell
Push-Location firmware/core
..\..\.venv\Scripts\python.exe train_nn_risk_model.py --min-accuracy 88.0
Pop-Location
git checkout -- firmware/core/nn_risk_model_int8.c.inc firmware/core/nn_risk_model_trained.c.inc
```
Expected: Architecture `6-24-16-3`, `619` parameters (576 weights + 43 biases), teacher agreement >= 88.0%, working tree `.inc` files restored cleanly.

- [ ] **Step 3: Audit clinical edge-case invariants in `firmware/core/`**

Verify and document in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`:
1. **No-contact safety:** When `contact == false` or `hr <= 0`, `disaster_risk_engine_eval()` returns `RISK_UNKNOWN` ("Patient not assessed") rather than falsely reporting `RISK_NORMAL`.
2. **HRV settlement & AI gating:** `hrv_is_settled()` requires `>= 30` beats and `>= 15` valid NN pairs with `SQI >= 0.70f`; while unsettled, `ai_valid` is `false` so the neural network is never fed a bogus `0.0f` RMSSD.
3. **PM2.5 honest null:** Negative or absent PM2.5 readings report `not assessed` / `null` rather than `0.0 ug/m3`.
4. **mNEWS2 disaster threshold:** Medium risk triggers at score `4` (vs standard hospital `5`) for early field warning, and any single parameter score of `3` (e.g. `SpO2 <= 91%`, `HR <= 40` or `>= 131`) escalates to at least Medium/High risk.

- [ ] **Step 4: Commit Layer 2 audit report section**

Run:
```powershell
git add docs/presentation/FULL_STACK_AUDIT_2026-11-02.md
git commit -m "docs(audit): complete Layer 2 clinical algorithms and TinyML AI audit"
```

---

### Task 4: ESP32-S3 Firmware, Concurrency & Memory Safety Audit (`c-firmware-safety`)

**Files:**
- Audit (Read-only): `firmware/shrikefi/main_shrikefi.c`, `firmware/shrikefi/web_status.c`, `firmware/shrikefi/web_status.h`, `firmware/shrikefi/shrikefi_spi_link.c`, `firmware/shrikefi/fall_detect.c`, `firmware/shrikefi/gnss_nmea.c`, `firmware/shrikefi/emergency_assist.c`, `firmware/shrikefi/ssd1306_oled.c`, `firmware/shrikefi/max30102.c`, `firmware/shrikefi/bme280.c`, `firmware/shrikefi/pms5003.c`, `firmware/shrikefi/mpu6050.c`
- Modify: `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`

**Interfaces:**
- Consumes: `.agents/skills/c-firmware-safety/SKILL.md` 12-point checklist.
- Produces: Completed Layer 3 firmware safety, concurrency, and HTTP server audit in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`.

- [ ] **Step 1: Run static safety greps and `-Wdouble-promotion` syntax check across all firmware sources**

Run:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
Select-String -Path firmware/core/*.c,firmware/core/*.h,firmware/shrikefi/*.c,firmware/shrikefi/*.h -Pattern '\b(strcpy|sprintf|strcat|gets)\s*\('
gcc -Wall -Wextra -Werror -pedantic -Wdouble-promotion -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi -fsyntax-only firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/hrv_analysis.c firmware/core/pm25_correction.c firmware/core/clinical_vitals_engine.c firmware/core/spo2_engine.c firmware/core/ppg_sqi.c firmware/core/disaster_risk_engine.c firmware/core/skin_estimate.c firmware/shrikefi/fall_detect.c firmware/shrikefi/gnss_nmea.c firmware/shrikefi/web_status.c firmware/shrikefi/emergency_assist.c
```
Expected: Zero unsafe string function matches; `gcc -Wdouble-promotion` exits 0 with zero warnings.

- [ ] **Step 2: Run host unit test suites for Fall Detection, GNSS NMEA, and ShrikeFi Host Integration**

Run:
```powershell
$env:PATH = "C:\msys64\ucrt64\bin;$env:PATH"
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/shrikefi firmware/shrikefi/test_fall_detect.c firmware/shrikefi/fall_detect.c -lm -o "$env:TEMP\test_fall.exe" && & "$env:TEMP\test_fall.exe"
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/shrikefi firmware/shrikefi/test_gnss_nmea.c firmware/shrikefi/gnss_nmea.c -lm -o "$env:TEMP\test_gnss.exe" && & "$env:TEMP\test_gnss.exe"
gcc -Wall -Wextra -Werror -pedantic -std=c11 -DSHRIKEFI_HOST_TEST -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi firmware/shrikefi/main_shrikefi.c firmware/shrikefi/shrikefi_spi_link.c firmware/shrikefi/max30102.c firmware/shrikefi/bme280.c firmware/shrikefi/pms5003.c firmware/shrikefi/mpu6050.c firmware/shrikefi/ssd1306_oled.c firmware/shrikefi/fall_detect.c firmware/shrikefi/gnss_nmea.c firmware/shrikefi/web_status.c firmware/shrikefi/emergency_assist.c firmware/core/clinical_vitals_engine.c firmware/core/disaster_risk_engine.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c firmware/core/ppg_sqi.c firmware/core/pm25_correction.c firmware/core/skin_estimate.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c -lm -o "$env:TEMP\shrikefi_host.exe" && & "$env:TEMP\shrikefi_host.exe"
Remove-Item "$env:TEMP\test_fall.exe","$env:TEMP\test_gnss.exe","$env:TEMP\shrikefi_host.exe" -ErrorAction SilentlyContinue
```
Expected: All three executables exit 0 with all assertions passing.

- [ ] **Step 3: Audit FreeRTOS task concurrency, stack depths, and HTTP server handlers in `main_shrikefi.c` & `web_status.c`**

Verify and document in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`:
1. **Inter-core mutex discipline:** Every read/write to `s_shared_data` in `main_shrikefi.c` is bracketed by `xSemaphoreTake(s_data_mutex, ...)` and `xSemaphoreGive(s_data_mutex)`, and `web_status.c` protects `s_snap` and `s_history` with `WS_LOCK()` / `WS_UNLOCK()`.
2. **HTTP URI handler capacity:** Count `httpd_register_uri_handler()` calls in `web_status_start()` (13 handlers) vs `hcfg.max_uri_handlers = 20` (7 spare slots).
3. **4 KB chunked streaming:** Confirm `h_desktop()` streams `VALOR_DASHBOARD_GZ` in `<= 4096`-byte chunks and terminates with `httpd_resp_send_chunk(req, NULL, 0)`, keeping `VALOR_DASHBOARD_GZ` in Flash `.rodata` (`static const uint8_t`).
4. **Origin & CORS check:** Confirm `web_status_request_is_same_origin()` accepts `NULL`, `http://192.168.4.1`, and `"null"` (for local `file:///` dashboard) while rejecting external origins like `http://evil.example`.

- [ ] **Step 4: Commit Layer 3 audit report section**

Run:
```powershell
git add docs/presentation/FULL_STACK_AUDIT_2026-11-02.md
git commit -m "docs(audit): complete Layer 3 ESP32-S3 firmware safety and concurrency audit"
```

---

### Task 5: Dashboards, Gzip Sync & Offline Accessibility Audit + Automated Wi-Fi Regression Tests

**Files:**
- Audit (Read-only): `firmware/shrikefi/dashboard/valor_dashboard.html`, `docs/archive/valor_dashboard_2.html`, `firmware/shrikefi/valor_dashboard_gz.h`
- Modify: `firmware/shrikefi/dashboard/verify_dashboard.py`
- Modify: `firmware/shrikefi/dashboard/verify_softap_page.py`
- Modify: `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`

**Interfaces:**
- Consumes: `valor_dashboard.html` (`connectWifi(targetUrl)`, `wifiPPG(dt)`), `firmware/shrikefi/valor_dashboard_gz.h` (`VALOR_DASHBOARD_GZ`), `firmware/shrikefi/web_status.c` (`h_desktop`, `max_uri_handlers`).
- Produces: Hardened `verify_dashboard.py` and `verify_softap_page.py` that fail immediately if `valor_dashboard_gz.h` drifts from `valor_dashboard.html`, if `connectWifi()` breaks in Chrome, or if `max_uri_handlers` is too small.

- [ ] **Step 1: Write new assertions in `firmware/shrikefi/dashboard/verify_softap_page.py` for `/desktop` gzip streaming and URI handler capacity**

In `firmware/shrikefi/dashboard/verify_softap_page.py` right after the `Access-Control-Allow-Origin` check (around line 109), add checks that:
1. `h_desktop` is registered for `"/desktop"` and `"/dashboard"` and streams in `4096`-byte chunks (`httpd_resp_send_chunk`).
2. `max_uri_handlers` in `web_status.c` is strictly `>=` the number of `httpd_register_uri_handler(` calls in `web_status.c`.

```python
uri_regs = len(re.findall(r"httpd_register_uri_handler\s*\(", source))
max_hdr_m = re.search(r"max_uri_handlers\s*=\s*(\d+)", source)
if not max_hdr_m or int(max_hdr_m.group(1)) < uri_regs:
    raise RuntimeError(f"max_uri_handlers too small for {uri_regs} registered handlers")
if '"/desktop"' not in source or '"/dashboard"' not in source or "4096" not in source:
    raise RuntimeError("Gzipped /desktop chunked handler missing from web_status.c")
```

- [ ] **Step 2: Write gzip byte-identity check and headless Chrome `wifi` mode test in `firmware/shrikefi/dashboard/verify_dashboard.py`**

In `firmware/shrikefi/dashboard/verify_dashboard.py`:
1. Add a helper `verify_gzip_header_sync()` that parses hex bytes from `firmware/shrikefi/valor_dashboard_gz.h`, decompresses them with `gzip.decompress()`, and asserts `decompressed == DASHBOARD.read_bytes()`.
2. In the injected test harness inside `verify_dashboard.py`, add a `case === 'wifi'` branch that mocks `window.fetch` for `/status.json`, `/api/history`, and `/api/emergency/trigger`, invokes `connectWifi('http://192.168.4.1')`, verifies live telemetry rendering (`hr == '78'`, `spo2 == '98.2'`, `wifiPPG(0.016)` produces non-zero arterial pulse samples while `contact: true`), verifies that a subsequent poll with `contact: false` clears `hr` to `'--'` and flatlines `wifiPPG(0.016)` to `0`, and verifies `triggerWifiSos()` posts to `/api/emergency/trigger`.

- [ ] **Step 3: Run both updated dashboard verification suites**

Run:
```powershell
.venv\Scripts\python.exe firmware/shrikefi/dashboard/verify_softap_page.py
.venv\Scripts\python.exe firmware/shrikefi/dashboard/verify_dashboard.py
Remove-Item -Force -Path firmware/shrikefi/dashboard/tmp*.html -ErrorAction SilentlyContinue
```
Expected: Both scripts exit 0 and print `PASS` / `PASSED`.

- [ ] **Step 4: Run `html-offline-a11y-auditor` checks across all HTML surfaces and document in `FULL_STACK_AUDIT_2026-11-02.md`**

Run:
```powershell
Select-String -Path firmware/shrikefi/dashboard/valor_dashboard.html,docs/archive/valor_dashboard_2.html -Pattern 'https?://' | Where-Object { $_.Line -notmatch '192\.168\.4\.1|localhost|www\.w3\.org/2000/svg' }
(Get-FileHash firmware/shrikefi/dashboard/valor_dashboard.html).Hash -eq (Get-FileHash docs/archive/valor_dashboard_2.html).Hash
```
Expected: 0 external URL matches; hash comparison returns `True`. Record full 12-point `html-offline-a11y-auditor` table in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`.

- [ ] **Step 5: Commit dashboard test improvements and Layer 4 audit report**

Run:
```powershell
git add firmware/shrikefi/dashboard/verify_softap_page.py firmware/shrikefi/dashboard/verify_dashboard.py docs/presentation/FULL_STACK_AUDIT_2026-11-02.md
git commit -m "test(dashboard): add gzip flash sync and headless Chrome Wi-Fi mode verification"
```

---

### Task 6: Python Tooling, Documentation & Presentation Claims Audit + Final 19-Gate Replay

**Files:**
- Audit (Read-only): `scripts/bundle_desktop_dashboard.py`, `scripts/check_presentation_claims.py`, `hardware/shrikefi/gen_flat_source.py`, `firmware/core/train_nn_risk_model.py`
- Inspect/Modify: `README.md`, `docs/MASTER_PROJECT_GUIDE.md`, `docs/JUDGE_STUDY_NOTES.md`, `docs/DEMO_SHEET.md`
- Modify: `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`, `docs/presentation/PREDEMO_VERIFY_LOG.txt`

**Interfaces:**
- Consumes: `.agents/skills/python-tooling-auditor/SKILL.md`, `scripts/check_presentation_claims.py`, `scripts/predemo_verify.ps1`.
- Produces: Zero-warning, zero-violation presentation claim report, completed `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`, and fresh 19/19 PASS `docs/presentation/PREDEMO_VERIFY_LOG.txt`.

- [ ] **Step 1: Audit Python scripts against `python-tooling-auditor` checklist**

Run:
```powershell
Select-String -Path scripts/*.py,hardware/shrikefi/*.py,firmware/core/*.py,firmware/shrikefi/dashboard/*.py -Pattern '\b(eval|exec)\s*\(|shell\s*=\s*True'
```
Expected: 0 matches for `eval()`, `exec()`, or `shell=True`. Document the 11-point `python-tooling-auditor` table in `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md`.

- [ ] **Step 2: Run `scripts/check_presentation_claims.py` and resolve any warnings in tracked docs and `docs/DEMO_SHEET.md`**

Run:
```powershell
.venv\Scripts\python.exe scripts/check_presentation_claims.py
```
Inspect any `[WARN:SENSITIVITY-HIDDEN]` lines in `README.md`, `docs/MASTER_PROJECT_GUIDE.md`, `docs/JUDGE_STUDY_NOTES.md`, or `docs/DEMO_SHEET.md`. Where 94.11% MIMIC-III accuracy is cited in prose or tables without adjacent sensitivity (58.18%), include `(58.18% sensitivity, 98.19% specificity)` on the same line so that `check_presentation_claims.py` reports **0 fatal violations and 0 warnings**.

- [ ] **Step 3: Re-run `scripts/check_presentation_claims.py` to confirm 0 warnings and 0 violations**

Run:
```powershell
.venv\Scripts\python.exe scripts/check_presentation_claims.py
```
Expected: `ALL CHECKS PASSED: 0 fatal violations found across docs and slides.` with 0 warnings.

- [ ] **Step 4: Run full 19-gate `scripts/predemo_verify.ps1` end-to-end replay**

Run:
```powershell
powershell -ExecutionPolicy Bypass -File scripts/predemo_verify.ps1
```
Expected: `SUMMARY: 19/19 PASS, 0 WARN, 0 FAIL`.

- [ ] **Step 5: Finalize `docs/presentation/FULL_STACK_AUDIT_2026-11-02.md` Executive Summary and commit**

Run:
```powershell
git add README.md docs/MASTER_PROJECT_GUIDE.md docs/JUDGE_STUDY_NOTES.md docs/DEMO_SHEET.md docs/presentation/FULL_STACK_AUDIT_2026-11-02.md docs/presentation/PREDEMO_VERIFY_LOG.txt
git commit -m "docs(audit): complete full-stack project audit and zero-warning claim verification"
```
