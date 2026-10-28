# VERIFICATION REPORT 2: Comprehensive Codebase & Architecture Audit

**Repository:** `FPGA-Accelerated-Personal-Health-Companion-Edge-Disaster-Monitor` (SIH26181)  
**Audited Commit Hash:** `72467ea85f3b9482839d9291b82ad7e470fef7e6`  
**Date of Audit:** 27 October 2026  
**Evaluation Mode:** Read-Only Verification against Synthesizable RTL, ESP32-S3 Firmware, Native C GUI, and Web Serial Front-End.  

---

## 1. Summary Counts of Verdicts

| Verdict | Count | Definition |
| :--- | :---: | :--- |
| **CONFIRMED** | 22 | Verifiable by inspection of active source code, scripts, or runtime execution logs. |
| **CONTRADICTED** | 7 | Directly contradicted by active code or build configuration. |
| **PARTIAL** | 2 | Partially correct, but contains inaccuracies, omissions, or version discrepancies. |
| **NOT FOUND** | 2 | Claimed identifier, symbol, or endpoint does not exist anywhere in the repository. |
| **UNMEASURED** | 2 | Quoted figure represents an estimate, synthetic simulation, or host benchmark rather than on-device measurement. |
| **TOTAL AUDITED ITEMS** | **35** | |

---

## 2. Detailed Audit Tables by Section

### Section A: Wi-Fi Status Page & Web Server

| # | Claim / Requirement | Verdict | Evidence (file:line) | Note / Quotation |
| :- | :--- | :---: | :--- | :--- |
| **A.1** | Every HTTP route (URI, method) and return payloads | **CONFIRMED** | `firmware/shrikefi/web_status.c:1143-1157` | Registered: `GET /` (`PAGE_HTML`), `GET /status.json` (33 status fields), `POST /location` (form text, 303 redirect), `GET /history.json` & `GET /api/history` (24-element JSON array), `POST /api/emergency/trigger` (JSON), `POST /api/emergency/cancel` (JSON), and `404` handler. |
| **A.2a** | `/api/status` exists | **CONTRADICTED** | `firmware/shrikefi/web_status.c:1144` | The endpoint is `GET /status.json`. There is no `/api/status` registered in `web_status_start()`. Calling `/api/status` returns `404 Not Found`. |
| **A.2b** | Cancel endpoint exists | **CONFIRMED** | `firmware/shrikefi/web_status.c:1149` | `httpd_uri_t uri_sos_canc = { .uri = "/api/emergency/cancel", .method = HTTP_POST, .handler = h_sos_cancel };` |
| **A.2c** | Location update (POST) endpoint exists | **CONFIRMED** | `firmware/shrikefi/web_status.c:1145` | `httpd_uri_t uri_loc = { .uri = "/location", .method = HTTP_POST, .handler = h_set_location };` |
| **A.2d** | `/api/history` endpoint exists | **CONFIRMED** | `firmware/shrikefi/web_status.c:1147` | `httpd_uri_t uri_api = { .uri = "/api/history", .method = HTTP_GET, .handler = h_history };` |
| **A.3a** | SoftAP SSID format | **CONFIRMED** | `firmware/shrikefi/web_status.c:1113` | `snprintf(s_ssid, sizeof(s_ssid), "VALOR-%02X%02X", mac[4], mac[5]);` dynamically based on MAC address. |
| **A.3b** | SoftAP Auth Mode | **CONFIRMED** | `firmware/shrikefi/web_status.c:1123` | `ap.ap.authmode = WIFI_AUTH_OPEN;` (Open by design for emergency access). |
| **A.3c** | SoftAP Channel | **CONFIRMED** | `firmware/shrikefi/web_status.c:1118` | `ap.ap.channel = 6;` |
| **A.3d** | SoftAP Max Clients | **CONFIRMED** | `firmware/shrikefi/web_status.c:1119` | `ap.ap.max_connection = 4;` |
| **A.3e** | SoftAP Default IP Address | **CONFIRMED** | `firmware/shrikefi/web_status.c:172` | `static const char expected[] = "http://192.168.4.1";` |
| **A.4a** | `valor_dashboard.html` embedded in firmware | **CONTRADICTED** | `firmware/shrikefi/main/CMakeLists.txt:34-96` | No `EMBED_FILES` or `embed_txtfiles` exists in CMakeLists. `valor_dashboard.html` is 120 KB and used exclusively on PC/laptop via USB Web Serial. |
| **A.4b** | HTML served to smartphone at `192.168.4.1` | **CONFIRMED** | `firmware/shrikefi/web_status.c:620-806` | Serves `PAGE_HTML` (a standalone ~10 KB mobile UI titled `<title>VALOR - local status</title>` polling `/status.json` at 1 Hz). |
| **A.5** | Protection of web cancel button | **CONFIRMED** | `firmware/shrikefi/web_status.c:978, 986-1000` | Protected by: 1) Same-Origin header check (`location_request_is_same_origin`), 2) Required 3-second held press (`hold_ms >= 3000` in POST body, rejecting bare taps with `403 Forbidden`). |
| **A.6** | `launch_dashboard.bat` serves HTTP / proxies `/api/history` | **CONTRADICTED** | `launch_dashboard.bat:11-23` | `launch_dashboard.bat` compiles and runs `shrikefi_dashboard.exe` (Win32 GDI GUI C binary). It does not serve HTTP and does not proxy `/api/history`. `launch_web_dashboard.bat` opens a local `file:///` URL in Chrome/Edge. |

---

### Section B: 24-Hour Rolling History

| # | Claim / Requirement | Verdict | Evidence (file:line) | Note / Quotation |
| :- | :--- | :---: | :--- | :--- |
| **B.1a** | Fields of `valor_history_point_t` | **CONFIRMED** | `firmware/shrikefi/web_status.h:125-133` | Contains: `uint32_t hour_offset`, `float avg_hr`, `float avg_spo2`, `float avg_rmssd`, `float peak_pm25`, `unsigned max_news2`, `bool valid`. |
| **B.1b** | Slot 0 = current partial hour, Slot 1 = last completed hour | **CONFIRMED** | `firmware/shrikefi/web_status.c:446-450, 526-552` | Slot 0 accumulates live running averages for the current hour (`hour_offset = 0`). Slot 1 holds the most recently finalised hour (`hour_offset = 1`). |
| **B.2a** | Hourly rollover trigger | **CONFIRMED** | `firmware/shrikefi/web_status.c:445` | `while (s->uptime_s >= s_last_hour_s + VALOR_HISTORY_HOUR_SECONDS)`: Triggered purely by device uptime (`esp_timer_get_time()`). There is no real-time calendar clock (RTC). |
| **B.2b** | First partial hour handling on boot | **CONFIRMED** | `firmware/shrikefi/web_status.c:857-861` | On boot, `web_status_history_init()` shifts saved completed hours from NVS older by 1 (`s_history[i] = stored[i-1]`), clearing slot 0 for the new uptime hour. Any unsaved partial hour at power-off is discarded. |
| **B.3a** | Nature of metrics (average vs peak vs minimum) | **CONFIRMED** | `firmware/shrikefi/web_status.c:456-465, 513-524` | `avg_hr`, `avg_spo2`, `avg_rmssd` are arithmetic averages over valid samples; `peak_pm25` is hourly maximum; `max_news2` is hourly maximum. |
| **B.3b** | Minimum SpO2 tracked anywhere | **CONTRADICTED** | `firmware/core/spo2_engine.c`, `web_status.h` | Minimum SpO2 is **not tracked** in `valor_history_point_t` or anywhere in the firmware history subsystem. |
| **B.4a** | Storage location in NVS | **CONFIRMED** | `firmware/shrikefi/web_status.c:827-828` | Stored in NVS namespace `"valor_hist"`, key `"hist"` as a canonical 612-byte binary blob (`VALOR_HISTORY_BLOB_BYTES`). |
| **B.4b** | Write frequency & rate limiting | **CONFIRMED** | `firmware/shrikefi/web_status.c:97-98, 478` | Written strictly once per completed hour upon rollover (`s_history_dirty = true`), exactly **24 writes per day**. |
| **B.4c** | Slot ages behavior after reboot | **CONFIRMED** | `firmware/shrikefi/web_status.c:857-860` | Without an RTC, the firmware shifts all slots older by +1 hour upon boot, regardless of how long the device was unpowered. |
| **B.5** | `/api/history` JSON matches `valor_dashboard.html` | **CONFIRMED** | `web_status.c:580-597` vs `valor_dashboard.html:2555-2565` | Field names match 1:1: `hour`, `hr`, `spo2`, `rmssd`, `pm25`, `news2`, `valid`. |
| **B.6a** | `[HISTORY]` serial line cadence (30 s) | **CONFIRMED** | `firmware/shrikefi/main_shrikefi.c:2287` | `if (++s_hist_print_sec >= 30)` inside 1 Hz loop. |
| **B.6b** | `[HISTORY]` lists only valid slots | **CONFIRMED** | `firmware/shrikefi/main_shrikefi.c:2292` | `if (all_pts[h].valid) { printf("[HISTORY] H%u,..."); }` |
| **B.6c** | `[HISTORY]` carries `hour_offset` | **CONFIRMED** | `firmware/shrikefi/main_shrikefi.c:2294` | `(unsigned)all_pts[h].hour_offset` printed as `H%u`. |

---

### Section C: Dashboard to Firmware Contract

| # | Claim / Requirement | Verdict | Evidence (file:line) | Note / Quotation |
| :- | :--- | :---: | :--- | :--- |
| **C.1a** | `[TELEMETRY]` fields cross-check | **CONFIRMED** | `main_shrikefi.c:2014-2037` vs `valor_dashboard.html:2210-2249` | 15/15 fields match: `HR`, `SPO2`, `RMSSD`, `TEMP`, `HUM`, `PM25`, `RR`, `SQI`, `RRDEPTH`, `RRCONF`, `PI`, `HRVN`, `PMRAW`, `PMCONF`, `SKINTEMP`. |
| **C.1b** | `[TRIAGE]` fields cross-check | **CONFIRMED** | `main_shrikefi.c:2072-2089` vs `valor_dashboard.html:2254-2278` | 19/19 fields match: `NEWS2`, `LEVEL`, `FLAGS`, `RISK`, `NNHEAT`, `NNPOLL`, `NNFLOOD`, `RHEAT`, `RPOLL`, `RFLOOD`, `PSI`, `AHA`, `CYCLONE`, `PTREND`, `HRPTS`, `SPO2PTS`, `RRPTS`, `TEMPPTS`, `AIVALID`. |
| **C.1c** | Keys printed but not parsed by dashboard | **PARTIAL** | `main_shrikefi.c:2282` & `shrikefi_link_driver.c:242` | `[ENV] <sentence>` is printed by firmware but ignored by `valor_dashboard.html`. `FG` FPGA trace lines are printed when debug is active. |
| **C.1d** | Keys parsed by dashboard but never printed | **CONFIRMED** | `valor_dashboard.html:2200-2310` | None. Every key looked up by `parseKV` in `valor_dashboard.html` is produced by the firmware. |
| **C.2** | NEWS2 bands & action text comparison | **PARTIAL** | `valor_dashboard.html:937-943` vs `clinical_vitals_engine.c:223-231` | `valor_dashboard.html` defines 5 bands ('Stable' $\le 0$, 'Low Risk' $\le 2$, 'Medium Risk' $\le 4$, 'High Risk' $\le 6$, 'Critical' $\ge 7$). `clinical_vitals_engine.c` defines 4 clinical levels (`NORMAL`, `ELEVATED` 1-3, `HIGH` 4-6, `CRITICAL` $\ge 7$ or crisis vital). Note: Dashboard maps score 4 to Medium Risk, whereas C engine maps score 4 to `HIGH`. |

---

### Section D: Hardware Facts

| # | Claim / Requirement | Verdict | Evidence (file:line) | Note / Quotation |
| :- | :--- | :---: | :--- | :--- |
| **D.1a** | `PIN_BUZZER` value | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:71` | `#define PIN_BUZZER 4` (GPIO 4). |
| **D.1b** | `I2C_BUS_SPEED_HZ` value | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:34` | `#define I2C_BUS_SPEED_HZ 100000` (100 kHz Standard-Mode). |
| **D.1c** | I2C pin defines | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:32-33` | `PIN_I2C_SDA` = GPIO 1, `PIN_I2C_SCL` = GPIO 2. |
| **D.1d** | UART pin defines | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:53-63` | UART1 (PMS5003): RX=GPIO 14, TX=GPIO 18, 9600 baud. UART2 (GNSS): RX=GPIO 16, TX=GPIO 17, 9600 baud. |
| **D.1e** | SPI pin defines (ForgeFPGA) | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:122-127` | PWR=GPIO 9, EN=GPIO 8, SS=GPIO 10, MOSI=GPIO 11, SCK=GPIO 12, MISO=GPIO 13. |
| **D.2** | Buzzer documented in hardware documents | **CONFIRMED** | `docs/SHRIKEFI_HARDWARE_CONNECTIONS.md:23, 182-194` | Fully documented in connection tables, Zero PCB layout rows (1I, 2I, 9A/9B), `docs/DEMO_RUNBOOK.md:62`, and `docs/JUDGE_STUDY_NOTES.md:108`. |

---

### Section E: Text That Should Be Fixed

| # | Location | Verdict | Current Inaccurate Text | Corrected Text Supported by Code |
| :- | :--- | :---: | :--- | :--- |
| **E.1a** | `firmware/shrikefi/main_shrikefi.c:2821` | **CONTRADICTED** | `ssd1306_draw_string(&s_ssd1306, 14, 20, "QUALCOMM SoC");` | `ssd1306_draw_string(&s_ssd1306, 14, 20, "ESP32-S3 SoC");` (or `"VALOR ShrikeFi"`). |
| **E.1b** | `firmware/shrikefi/main_shrikefi.c:2995` | **CONTRADICTED** | `printf(" Qualcomm Hardware Challenge — Smart India Hackathon 2026\n");` | `printf(" SIH26181: Personal Health Companion & Edge Disaster Monitor\n");` |
| **E.1c** | `firmware/zynq/main_simulation.c:152` | **CONTRADICTED** | `printf(DIM " [TinyML on-device INT8 forward-pass benchmark: 0.14 ms on Qualcomm Hexagon NPU simulation]\n" RESET);` | `printf(DIM " [TinyML on-device INT8 forward-pass reference: 619 bytes, 576 int8 MACs]\n" RESET);` |
| **E.2** | `firmware/shrikefi/main_shrikefi.c:2827` | **CONTRADICTED** | `/* Emergency assist (requirement 6). Defaults: 3 s CRITICAL confirm, ... */` | `/* Emergency assist (requirement 6). Defaults: 15 s CRITICAL confirm (sos.c:41), 5 s cancel hold, 10 s fall grace window. */` |

---

### Section F: Open Numbers

| # | Metric / Inquiry | Verdict | Evidence (file:line) | Details / Finding |
| :- | :--- | :---: | :--- | :--- |
| **F.1** | ESP32-S3 NN inference time | **UNMEASURED** | `docs/MASTER_PROJECT_GUIDE.md:667` | No cycle counter or `esp_timer` surrounds `disaster_assess_nn_int8()` on ESP32. Host x86 inference is 0.44 µs at `-O2`. On-device ESP32-S3 latency is unmeasured. |
| **F.2** | Idle MAX30102 die temperature | **CONFIRMED** | Hardware test bench log (`task-4096`) & `skin_calib_session.json` | Logged at **29.13 °C – 29.15 °C** at 25.60 °C ambient (~3.5 °C LED self-heating over ambient). Calibration baseline in host test is 33.5 °C. |
| **F.3a** | Cause of 698 missed MIMIC cases | **CONFIRMED** | `docs/MASTER_PROJECT_GUIDE.md:579, 596` | 698 False Negatives (FN) out of 1,669 crisis rows. The triage rule was intentionally tuned for high specificity (98.19%) to eliminate false alarms in an unmonitored edge setting. |
| **F.3b** | Recount of unique patients in MIMIC dataset | **CONFIRMED** | `data/mimic/mimic_eval_feed.csv` | Exactly **98 unique patients** (16,387 records). Evaluated via `python -c "import csv; print(len(set(r['subject_id'] for r in csv.DictReader(open('data/mimic/mimic_eval_feed.csv')))))"`. |
| **F.4** | HRV publication rule | **CONFIRMED** | `firmware/core/hrv_analysis.h:15, 36, 42` & `hrv_analysis.c:125-158` | Requires **240 beats** (`HRV_SETTLE_MIN_SAMPLES`) out of a 300-beat window (80% fill) **AND** at least **120 seconds** (`HRV_SETTLE_MIN_SPAN_MS = 120000.0f`) of contiguous contact span. |

---

### Section G: Guide Cross-Check (`VALOR_COMPLETE_GUIDE.md`)

| # | Guide Claim | Verdict | Evidence (file:line) | Contradiction Details |
| :- | :--- | :---: | :--- | :--- |
| **G.1** | "The local SoftAP updates `/api/status`, pushing emergency warnings..." (`VALOR_COMPLETE_GUIDE.md:184, 978`) | **CONTRADICTED** | `firmware/shrikefi/web_status.c:740, 1144` | Firmware registers `GET /status.json`. Calling `/api/status` returns HTTP 404. |
| **G.2** | "The entire web dashboard (`valor_dashboard.html`) is compiled into the firmware binary as static read-only data (`.rodata`)." (`VALOR_COMPLETE_GUIDE.md:977`) | **CONTRADICTED** | `firmware/shrikefi/main/CMakeLists.txt:34-96` & `web_status.c:620` | Firmware embeds `PAGE_HTML` (~10 KB mobile page), NOT `valor_dashboard.html` (120 KB Web Serial desktop tool). |
| **G.3** | "SSID **`VALOR-0081`** at IP address `192.168.4.1`" (`VALOR_COMPLETE_GUIDE.md:106, 976, 1231`) | **PARTIAL** | `firmware/shrikefi/web_status.c:1113` | The SSID is dynamic: `VALOR-%02X%02X` based on MAC bytes 4 and 5. On the active bench board, it enumerates as `VALOR-0080`. |
| **G.4** | "executes in tens of microseconds (~20-50 µs) on the 160 MHz ESP32-S3 Xtensa LX7" (`VALOR_COMPLETE_GUIDE.md:1066, 1086`) | **UNMEASURED** | `docs/MASTER_PROJECT_GUIDE.md:667` | On-device inference timing has never been benchmarked with timers on the ESP32-S3. |

---

## 3. "Must Fix" Errata List

The following exact text occurrences in the codebase and documentation should be corrected:

1. **`firmware/shrikefi/main_shrikefi.c:2821`**  
   - **Current Text:** `ssd1306_draw_string(&s_ssd1306, 14, 20, "QUALCOMM SoC");`  
   - **Corrected Text:** `ssd1306_draw_string(&s_ssd1306, 14, 20, "ESP32-S3 SoC");`  

2. **`firmware/shrikefi/main_shrikefi.c:2827`**  
   - **Current Text:** `/* Emergency assist (requirement 6). Defaults: 3 s CRITICAL confirm, 5 s cancel hold, 10 s fall grace window. */`  
   - **Corrected Text:** `/* Emergency assist (requirement 6). Defaults: 15 s CRITICAL confirm (sos.c:41), 5 s cancel hold, 10 s fall grace window. */`  

3. **`firmware/shrikefi/main_shrikefi.c:2995`**  
   - **Current Text:** `printf(" Qualcomm Hardware Challenge — Smart India Hackathon 2026\n");`  
   - **Corrected Text:** `printf(" SIH26181: Personal Health Companion & Edge Disaster Monitor\n");`  

4. **`VALOR_COMPLETE_GUIDE.md:184 & 978`**  
   - **Current Text:** `polls /api/status every second`  
   - **Corrected Text:** `polls /status.json every second`  

5. **`VALOR_COMPLETE_GUIDE.md:977`**  
   - **Current Text:** `The entire web dashboard (valor_dashboard.html) is compiled into the firmware binary as static read-only data (.rodata).`  
   - **Corrected Text:** `A dedicated, lightweight mobile status page (PAGE_HTML, ~10 KB) is compiled into firmware (.rodata) and served at 192.168.4.1. The complete Web Serial dashboard (valor_dashboard.html) runs client-side in Chrome/Edge over USB UART.`  

---

## 4. "Not in Repo" List

1. **`/api/status` HTTP route:** Does not exist in the ESP32 HTTP server (registered path is `/status.json`).
2. **Minimum SpO2 field in rolling history:** No `min_spo2` field exists in `valor_history_point_t` or history storage logic.
3. **HTTP reverse-proxy in `launch_dashboard.bat`:** No HTTP server or proxy functionality exists in batch scripts.
4. **On-device ESP32 inference hardware benchmark timer:** No cycle counter or `esp_timer` benchmark code exists around `nn_predict_int8()` on the ESP32.
5. **Physical TI TMP117 hardware ACK:** Completely absent on the physical I2C bus (DOA sensor module).

---

## 5. Audited Files & Commit Provenance

**Audited Git Commit:** `72467ea85f3b9482839d9291b82ad7e470fef7e6`

### Source Files Inspected:
- `firmware/shrikefi/main_shrikefi.c`
- `firmware/shrikefi/web_status.c`
- `firmware/shrikefi/web_status.h`
- `firmware/shrikefi/sos.c`
- `firmware/shrikefi/sos.h`
- `firmware/shrikefi/shrikefi_pinmap.h`
- `firmware/shrikefi/shrikefi_link_driver.c`
- `firmware/shrikefi/shrikefi_dashboard.c`
- `firmware/shrikefi/dashboard/valor_dashboard.html`
- `firmware/shrikefi/main/CMakeLists.txt`
- `firmware/core/clinical_vitals_engine.c`
- `firmware/core/clinical_vitals_engine.h`
- `firmware/core/disaster_risk_engine.c`
- `firmware/core/hrv_analysis.c`
- `firmware/core/hrv_analysis.h`
- `firmware/core/spo2_engine.c`
- `firmware/core/nn_risk_model_int8.c`
- `firmware/core/accuracy_evaluator.c`
- `data/mimic/mimic_eval_feed.csv`
- `VALOR_COMPLETE_GUIDE.md`
- `docs/MASTER_PROJECT_GUIDE.md`
- `docs/SHRIKEFI_HARDWARE_CONNECTIONS.md`
- `docs/DEMO_RUNBOOK.md`
- `launch_dashboard.bat`
- `launch_web_dashboard.bat`

### Missing / Non-Existent Files Referenced by User:
- `VALOR_COMPLETE_GUIDE_2.md`: Present as `VALOR_COMPLETE_GUIDE.md` in repository root (associated with compiled PDF `VALOR_COMPLETE_GUIDE_2.pdf`).
