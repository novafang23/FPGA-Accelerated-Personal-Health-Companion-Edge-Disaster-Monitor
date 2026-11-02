# VALOR Standalone Battery & Wi-Fi Desktop Dashboard Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replicate the rich, dark-glassmorphism desktop dashboard (`valor_dashboard.html` with 60 FPS PPG oscilloscope, ECG rhythm, mNEWS2 arc, EPA AQI/NWS heat badges, and 24h history) completely wirelessly over Wi-Fi (`VALOR-0081`), enabling 100% standalone battery-powered operation of the Vicharak ShrikeFi with zero USB wiring connected to the presentation laptop.

**Architecture:** 
1. **Client-Side Wi-Fi Transport in `valor_dashboard.html`:** Add a native `Connect Wi-Fi (192.168.4.1)` transport to the desktop dashboard. It polls `http://192.168.4.1/status.json` and `/api/history` at 2–3 Hz, feeding the live telemetry into internal state `V`, while driving the 60 FPS canvas oscilloscope with physiological beat waves synchronized to the wearer's real-time heart rate and IBI.
2. **CORS & Preflight Support in ESP32 Firmware (`web_status.c`):** Add `Access-Control-Allow-Origin: *` headers to `/status.json`, `/api/history`, and emergency endpoints so that browsers opening `valor_dashboard.html` locally (`file:///` or `localhost`) can query the SoftAP without browser CORS blocking.
3. **Embedded Gzipped Endpoint (`GET /desktop`):** Embed a pre-compressed 33 KB gzip payload of `valor_dashboard.html` into ESP32 flash memory and serve it over HTTP at `http://192.168.4.1/desktop`. This allows any laptop, tablet, or phone joining `VALOR-0081` to view the full aesthetic desktop dashboard in Chrome simply by typing the URL.

**Tech Stack:** 
- HTML5, Canvas 2D, Vanilla JS (ES6+), Web Serial API, Web Fetch API, Content Security Policy (CSP SHA-256).
- ESP-IDF v5.5.5, FreeRTOS, ESP32-S3 HTTP Server (`esp_http_server`), C11.
- Python 3 standard library (`hashlib`, `gzip`, `base64`).

**Spec:** User request: *"As I have connected the battery, I want to make it a standalone project that doesn't need any wiring from the laptop. That dashboard that I show through COM4 is amazing and very aesthetically pleasing, with all the PPG, heart rate, score, 24-hour history, etc. Can we replicate that and can I get that thing wirelessly through Wi-Fi? Not that Wi-Fi dashboard which I already have (that's very plain). I want that aesthetically pleasing dashboard that I get through COM4 when I wire it through the UART of ShrikeFi."*

---

## Global Constraints

- **100% Offline Edge Operation:** Zero external internet, CDN, or third-party cloud dependencies; all fonts, SVG icons, and styles remain 100% self-contained.
- **Strict Content-Security-Policy (CSP):** Every modification to inline `<script>` in `valor_dashboard.html` must compute the exact SHA-256 hash and update `<meta http-equiv="Content-Security-Policy">`.
- **RTL & Core Algorithms Frozen:** Zero changes to `hardware/**/*.v` or core clinical math (`firmware/core/*.c`).
- **ESP-IDF Stability:** HTTP response buffers must remain within 4 KB chunks to prevent heap starvation or watchdog triggers on Core 1.
- **Hardware Fallback Preserved:** The existing "Connect Serial (COM)" and "Simulated Profiles" modes must remain 100% functional and untouched for cable/USB fallback.
- **Test Integrity:** All 19 pre-demo verification gates in `scripts/predemo_verify.ps1` must pass cleanly (19/19 PASS, 0 FAIL).

---

## Review Focus

1. **CORS Failure on Local Browser:** Opening `valor_dashboard.html` from `file:///` on a laptop connected to Wi-Fi `VALOR-0081` triggers a CORS violation if `/status.json` lacks `Access-Control-Allow-Origin: *`.
2. **Wi-Fi Disconnect / Stale Telemetry:** If Wi-Fi signal drops or the user walks out of range, the dashboard must not freeze on old values; after 3 seconds of missed polls it must transition to `RECONNECTING...` and clear live vital displays.
3. **PPG Waveform Sync on Wi-Fi:** Over Wi-Fi, raw 100 Hz ADC samples are not streamed to avoid saturating the 2.4 GHz SoftAP TCP buffers; the 60 FPS PPG oscilloscope must be driven by physiological pulse synthesis synchronized to the live `hr` and `ibi` reported by the device, flatlining cleanly to zero when `contact == false`.
4. **SoftAP Heap & Concurrency:** Serving a 33 KB gzipped payload over `httpd_resp_send_chunk()` must not exhaust FreeRTOS heap or block `task_disaster_monitor` on Core 1.
5. **CSP Digest Mismatch:** Any script edit in `valor_dashboard.html` must synchronize with `docs/archive/valor_dashboard_2.html` and pass `verify_dashboard.py`.

---

## Task Breakdown

### Task 1: Add Wi-Fi Transport & Real-Time Waveform Sync to `valor_dashboard.html`

**Files:**
- Modify: `firmware/shrikefi/dashboard/valor_dashboard.html`
- Modify: `docs/archive/valor_dashboard_2.html` (synchronize byte-identical copy)
- Test: `firmware/shrikefi/dashboard/verify_dashboard.py`

**Interfaces:**
- Consumes: `http://192.168.4.1/status.json`, `http://192.168.4.1/api/history`, `http://192.168.4.1/api/emergency/trigger`, `http://192.168.4.1/api/emergency/cancel`.
- Produces: `connectWifi(targetUrl)` function, Wi-Fi polling loop, dynamic pulse waveform generator for Wi-Fi mode.

- [ ] **Step 1: Inspect `#gate-controls` and add "Connect Wi-Fi (192.168.4.1)" button**
  In `valor_dashboard.html` inside `#gate-controls`:
  Add a primary emerald-gradient button `#btn-connect-wifi`:
  `[Connect Wi-Fi (192.168.4.1)]`
  alongside the existing `[Connect Serial (COM)]` and `[Simulated Profiles ▾]` buttons.
  Also add `#btn-hdr-wifi` in the sticky top header for seamless reconnection.

- [ ] **Step 2: Implement `connectWifi(url)` and polling loop in `valor_dashboard.html`**
  ```javascript
  let wifiPollingInterval = null;
  let wifiTarget = 'http://192.168.4.1';
  
  async function connectWifi(targetUrl) {
    if (targetUrl) wifiTarget = targetUrl;
    mode = 'wifi';
    setStatus('live', 'CONNECTING TO VALOR-0081...');
    $('gate').classList.add('hidden');
    $('foot-note').textContent = 'live device telemetry over standalone Wi-Fi SoftAP';
    
    // Start polling loop at 400 ms interval (2.5 Hz)
    if (wifiPollingInterval) clearInterval(wifiPollingInterval);
    pollWifiStatus();
    wifiPollingInterval = setInterval(pollWifiStatus, 400);
    pollHttpHistory();
  }
  ```

- [ ] **Step 3: Implement `pollWifiStatus()` to parse status JSON into state `V`**
  Fetch `wifiTarget + '/status.json'` with `cache: 'no-store'`:
  - When data arrives:
    - Update `lastTelemetryAt = performance.now()`
    - Map `s.hr`, `s.spo2`, `s.rmssd`, `s.hrvn`, `s.rr`, `s.rrconf`, `s.rrdepth`, `s.pi`, `s.sqi * 100`, `s.temp`, `s.hum`, `s.pm25`, `s.pm25raw`, `s.pm25conf`, `s.skintemp`, `s.storm`, `s.ptrend`, `s.loc`, `s.news2`, `s.risk`, `s.sos`, `s.trigger`.
    - Set `V.contact = Boolean(s.contact && (s.hr > 0 || s.sqi > 0))`.
    - Update status pill: `🟢 LIVE · WI-FI (192.168.4.1) · STANDALONE`.
  - Handle timeouts / offline: If fetch fails for > 3000 ms, set status `RECONNECTING...`, zero out vitals (`renderIdle()`).

- [ ] **Step 4: Implement physiological pulse wave generator for Wi-Fi mode**
  In `frame(ts)` inside `valor_dashboard.html`:
  When `mode === 'wifi'` and `V.contact` is true and `V.hr > 0`:
  - Run the physiological pulse synthesis function (using `V.hr` and `dt`) to stream simulated blood volume pulses into `PPG.push(val)` and `ECG.tick(dt)` at 60 FPS.
  - When `V.contact` is false (`NO FINGER`), push flatline (0.0) so the green oscilloscope flattens immediately.

- [ ] **Step 5: Wire Emergency SOS Buttons for Wi-Fi mode**
  Ensure the "TRIGGER SOS" and "CANCEL SOS" buttons on `valor_dashboard.html` perform `fetch(wifiTarget + '/api/emergency/...', { method: 'POST' })` when `mode === 'wifi'`.

- [ ] **Step 6: Compute new CSP SHA-256 hash and update `<meta>` tag**
  Calculate `base64.b64encode(hashlib.sha256(script.encode('utf-8')).digest())`.
  Update `<meta http-equiv="Content-Security-Policy" content="... script-src 'sha256-<NEW_HASH>' ...">`.
  Copy `firmware/shrikefi/dashboard/valor_dashboard.html` to `docs/archive/valor_dashboard_2.html` so both remain byte-identical.

- [ ] **Step 7: Run verification test**
  Run: `.venv\Scripts\python.exe firmware/shrikefi/dashboard/verify_dashboard.py`
  Expected: `OK (13 cases rendered and asserted)`

- [ ] **Step 8: Commit**
  `git add firmware/shrikefi/dashboard/valor_dashboard.html docs/archive/valor_dashboard_2.html; git commit -m "feat(dashboard): add wireless Wi-Fi connection mode and physiological oscilloscope generator"`

---

### Task 2: Enable CORS Headers & Local Access in ESP32 Firmware (`web_status.c`)

**Files:**
- Modify: `firmware/shrikefi/web_status.c`
- Modify: `firmware/shrikefi/web_status.h`
- Test: `test_engine.exe` (run via `powershell scripts/predemo_verify.ps1`)

**Interfaces:**
- Consumes: ESP-IDF HTTP server headers (`httpd_resp_set_hdr`).
- Produces: `Access-Control-Allow-Origin: *`, `Access-Control-Allow-Methods: GET, POST, OPTIONS`, and CORS OPTIONS preflight handler.

- [ ] **Step 1: Add CORS headers to `h_json` and `h_history` in `web_status.c`**
  In `h_json(httpd_req_t *req)` and `h_history(httpd_req_t *req)`:
  ```c
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
  ```

- [ ] **Step 2: Add OPTIONS preflight handler in `web_status.c`**
  ```c
  static esp_err_t h_options(httpd_req_t *req) {
      httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
      httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type");
      httpd_resp_set_status(req, "204 No Content");
      return httpd_resp_send(req, NULL, 0);
  }
  ```
  Register `OPTIONS` handler for `/api/emergency/trigger` and `/api/emergency/cancel`.

- [ ] **Step 3: Update `location_request_is_same_origin()` for local dashboard control**
  Allow requests where Origin is `http://192.168.4.1` OR `null` (standard browser origin for `file:///` local files).

- [ ] **Step 4: Verify C compilation**
  Run: `powershell -ExecutionPolicy Bypass -File scripts/predemo_verify.ps1`
  Expected: All 19 gates PASS.

- [ ] **Step 5: Commit**
  `git add firmware/shrikefi/web_status.c firmware/shrikefi/web_status.h; git commit -m "feat(firmware): add CORS headers to SoftAP JSON endpoints for wireless dashboard"`

---

### Task 3: Embed Gzipped Desktop Dashboard in ESP32 Flash (`GET /desktop`)

**Files:**
- Create: `scripts/bundle_desktop_dashboard.py`
- Create: `firmware/shrikefi/valor_dashboard_gz.h`
- Modify: `firmware/shrikefi/web_status.c`
- Modify: `firmware/shrikefi/main/CMakeLists.txt` (if needed for header include)

**Interfaces:**
- Consumes: `firmware/shrikefi/dashboard/valor_dashboard.html`
- Produces: `VALOR_DASHBOARD_GZ` byte array, HTTP handler `h_desktop` responding to `GET /desktop` with `Content-Encoding: gzip`.

- [ ] **Step 1: Write `scripts/bundle_desktop_dashboard.py`**
  Python script using standard library `gzip` to read `valor_dashboard.html`, compress it at level 9 (~33 KB), and emit a C header `firmware/shrikefi/valor_dashboard_gz.h`:
  ```c
  #pragma once
  #include <stdint.h>
  #include <stddef.h>
  static const uint8_t VALOR_DASHBOARD_GZ[] = { 0x1f, 0x8b, ... };
  static const size_t VALOR_DASHBOARD_GZ_LEN = 33326;
  ```

- [ ] **Step 2: Run `bundle_desktop_dashboard.py` to generate header**
  Run: `.venv\Scripts\python.exe scripts/bundle_desktop_dashboard.py`
  Expected: Generated `firmware/shrikefi/valor_dashboard_gz.h` (file size ~33 KB).

- [ ] **Step 3: Implement `h_desktop` in `firmware/shrikefi/web_status.c`**
  ```c
  #include "valor_dashboard_gz.h"
  
  static esp_err_t h_desktop(httpd_req_t *req) {
      httpd_resp_set_type(req, "text/html");
      httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
      httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=3600");
      
      /* Send in 4 KB chunks to protect FreeRTOS socket heap */
      const size_t chunk_size = 4096;
      size_t offset = 0;
      while (offset < VALOR_DASHBOARD_GZ_LEN) {
          size_t len = VALOR_DASHBOARD_GZ_LEN - offset;
          if (len > chunk_size) len = chunk_size;
          esp_err_t err = httpd_resp_send_chunk(req, (const char *)(VALOR_DASHBOARD_GZ + offset), (ssize_t)len);
          if (err != ESP_OK) return err;
          offset += len;
      }
      return httpd_resp_send_chunk(req, NULL, 0);
  }
  ```
  Register URI handler for `GET /desktop` and `GET /dashboard`.

- [ ] **Step 4: Add link button to mobile dashboard (`PAGE_HTML`)**
  In `PAGE_HTML` inside `web_status.c`, add an unobtrusive desktop link button:
  `<a href='/desktop' style='display:inline-block;padding:6px 10px;background:#1E293B;color:#38BDF8;border:1px solid #334155;border-radius:6px;font-size:12px;font-weight:700;text-decoration:none;margin-left:8px'>🖥️ Desktop View</a>`

- [ ] **Step 5: Verify build with ESP-IDF**
  Run: `powershell -ExecutionPolicy Bypass -File scripts/predemo_verify.ps1`
  Expected: Gate 19 `fw-build` PASS.

- [ ] **Step 6: Commit**
  `git add scripts/bundle_desktop_dashboard.py firmware/shrikefi/valor_dashboard_gz.h firmware/shrikefi/web_status.c; git commit -m "feat(firmware): serve gzipped desktop dashboard over Wi-Fi at /desktop"`

---

### Task 4: Live Hardware Bench Rehearsal & Runbook Update

**Files:**
- Modify: `docs/DEMO_RUNBOOK.md`
- Modify: `docs/presentation/FACT_SHEET.md`
- Modify: `docs/presentation/HW_REHEARSAL_2026-10-29.md`

**Interfaces:**
- Consumes: ShrikeFi powered via battery, laptop connected to `VALOR-0081` over Wi-Fi.
- Produces: Verified wireless dashboard demonstration guide with step-by-step instructions.

- [ ] **Step 1: Flash new firmware to ShrikeFi on COM4**
  Run: `cmd /c "call C:\Espressif\frameworks\esp-idf-v5.5.5\export.bat && idf.py -p COM4 flash"`
  Expected: Flashing complete, board reboots autonomously.

- [ ] **Step 2: Unplug USB-C cable — Board running standalone on battery**
  Confirm board operates from battery: OLED displays telemetry, blue LED D12 pulses, Wi-Fi `VALOR-0081` active.

- [ ] **Step 3: Connect laptop to Wi-Fi `VALOR-0081`**
  Verify laptop connects to `VALOR-0081` without USB cable.

- [ ] **Step 4: Test Option 1 (Local Dashboard in Chrome)**
  Double-click `launch_web_dashboard.bat`.
  Click **[Connect Wi-Fi (192.168.4.1)]**.
  Confirm:
  - Status switches to `🟢 LIVE · WI-FI (192.168.4.1) · STANDALONE`.
  - Place finger on sensor: HR, SpO2, and RMSSD update.
  - Green oscilloscope pulses in real time at 60 FPS.
  - Remove finger: Display clears to `--` within 1 second.

- [ ] **Step 5: Test Option 2 (Browser Direct URL)**
  Open Chrome and navigate to `http://192.168.4.1/desktop`.
  Confirm the full aesthetic desktop dashboard loads in under 1 second from the ESP32-S3 SoftAP with full live telemetry and canvases.

- [ ] **Step 6: Update `docs/DEMO_RUNBOOK.md`**
  Add Section 4 Act 6: *"Wireless Standalone Battery Demonstration (Zero Wires to Laptop)"*.
  Explain how to impress judges by holding the standalone wearable in hand with no cables while the presentation laptop across the room displays the live 60 FPS glowing dashboard!

- [ ] **Step 7: Final CI Replay & Commit**
  Run: `powershell -ExecutionPolicy Bypass -File scripts/predemo_verify.ps1`
  Expected: 19/19 PASS, 0 WARN, 0 FAIL.
  Run: `.venv\Scripts\python.exe scripts/check_presentation_claims.py`
  Expected: 0 fatal violations.
  Commit: `git add docs/DEMO_RUNBOOK.md docs/presentation/FACT_SHEET.md docs/presentation/HW_REHEARSAL_2026-10-29.md; git commit -m "docs(demo): add wireless battery standalone demo guide and verification"`
