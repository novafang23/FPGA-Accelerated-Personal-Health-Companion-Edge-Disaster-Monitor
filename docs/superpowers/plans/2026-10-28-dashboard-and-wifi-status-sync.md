# VALOR Desktop Dashboard (`valor_dashboard.html` & `valor_dashboard_2.html`) and Wi-Fi Status Dashboard (`web_status.c`) Synchronization & Fix Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Synchronize `firmware/shrikefi/dashboard/valor_dashboard.html` and `docs/archive/valor_dashboard_2.html` so both contain the full set of clinical/UI fixes without drift, and fix four live-telemetry/UX issues in the embedded ESP32-S3 Wi-Fi dashboard (`firmware/shrikefi/web_status.h`, `firmware/shrikefi/web_status.c`, `firmware/shrikefi/main_shrikefi.c`).

**Architecture:**
1. **Desktop Dashboard Synchronization (`firmware/shrikefi/dashboard/valor_dashboard.html` & `docs/archive/valor_dashboard_2.html`):**
   - Port the `NEWS2_BANDS` alignment (`0: Normal`, `1–3: Elevated`, `4–6: High`, `>=7: Critical`), 24h SpO₂ hourly-average label (`24h SpO₂ Low (hourly avg)`), `renderIdle()` live-history preservation (`if (mode === 'idle')`), and `connect()` history reset from `valor_dashboard_2.html` into `valor_dashboard.html`.
   - Port the 30-beat / 20s HRV settling progress (`HRV_SETTLE_MIN_SAMPLES = 30`), `SKINTEMP` handling, and `/api/history` polling from `valor_dashboard.html` into `valor_dashboard_2.html`.
   - Fix three shared bugs in both desktop dashboards:
     1. When `!V.contact` on a normal `[TELEMETRY]` line (which is what the firmware sends when `ambient_valid == true` and no finger is present), clear `V.skinTemp = 0; V.n2hrPts = V.n2spo2Pts = V.n2rrPts = V.n2tempPts = null; V.hrvN = null;` so stale patient sub-scores or skin temperature never linger after finger removal.
     2. Keep `Temp` in the NEWS2 sub-score grid `na: true` unless `V.n2tempPts > 0` (since MAX30102 die-temp estimates peripheral skin temperature rather than RCP core temperature, and `main_shrikefi.c` passes `news2_temp = 0.0f`), and display settled peripheral skin temperature (`V.skinTemp`) clearly on the Row 3 `Ambient Temp` card subtitle (`BME280 · ambient · Skin 34.9 °C`) as well as the top environment strip (`Ta ... · Ts ...`).
     3. Accept both `'MODERATE'` (actual firmware string from `risk_level_to_string(RISK_MODERATE)`) and `'ELEVATED'` in the Storm & Cyclone badge/gauge color and percentage mapping, and reset `skinTemp: 0, n2tempPts: null` in `applyProfile()`.
2. **Wi-Fi SoftAP Dashboard (`firmware/shrikefi/web_status.h`, `firmware/shrikefi/web_status.c`, `firmware/shrikefi/main_shrikefi.c`):**
   - **Always-Populated `st.rr` & `st.sqi` (`main_shrikefi.c`):** Initialize `rr_for_news2 = rr_pub; sqi_for_news2 = sqi_pub; rr_conf_dbg = rrconf_pub; rr_depth_dbg = rrdepth_pub;` directly from the Core 0 snapshot instead of leaving them `0.0f` when `!vitals_ready && !ambient_valid`, and remove the duplicate mutex lock.
   - **Beat-by-Beat HRV Settling Progress (`web_status.h`, `web_status.c`, `main_shrikefi.c`):** Add `int hrv_n;` to `valor_status_t` and `"hrvn":%d` to `web_status_json()` so `PAGE_HTML` displays `hrvn/30` while RMSSD is settling during active finger contact instead of an unexplained `--`.
   - **Symmetrical 3×2 Vitals Grid (`web_status.c`):** Change the `Signal quality` card in `PAGE_HTML` from `card wide` to `card` so `Skin temp` and `Signal quality` sit side-by-side in the 3rd row of the 2-column vitals grid (eliminating the empty grid cell next to `Skin temp`).
   - **Reusable 3-Second Hold `CANCEL SOS` Button (`web_status.c`):** Reset `ok = false` inside `stop()` in `PAGE_HTML` so the `CANCEL SOS` button can be used more than once per browser session without requiring a page reload, and update the `PAGE_HTML` inline `<script>` SHA-256 CSP digest in both the `<meta>` tag and `h_root()` HTTP header.

**Tech Stack:** C11 (ESP-IDF / FreeRTOS / host GCC unit tests), HTML5/CSS3/ES6 (zero external dependencies, strict SHA-256 CSP).

---

### Task 1: Add Wi-Fi Status JSON & HRV Settling Field (`web_status.h`, `web_status.c`, `main_shrikefi.c`, `test_disaster_risk_engine.c`)

**Files:**
- Modify: `firmware/shrikefi/web_status.h`
- Modify: `firmware/shrikefi/web_status.c`
- Modify: `firmware/shrikefi/main_shrikefi.c`
- Modify: `firmware/zynq/test_disaster_risk_engine.c`

- [ ] **Step 1: Add `hrv_n` assertion to `test_web_status_json()` in `firmware/zynq/test_disaster_risk_engine.c`**

In `firmware/zynq/test_disaster_risk_engine.c` around line 1629, set `s.hrv_n = 18;` and assert `"\"hrvn\":18"` is present in `buf`:

```c
    s.hr = 72.1f; s.spo2 = 97.5f; s.rr = 0.0f; s.sqi = 0.87f; s.rmssd = 38.3f;
    s.hrv_n = 18;
```
and:
```c
    assert(strstr(buf, "\"rmssd\":38.3")     != NULL);
    assert(strstr(buf, "\"hrvn\":18")        != NULL);
```

- [ ] **Step 2: Add `int hrv_n;` to `valor_status_t` in `firmware/shrikefi/web_status.h` and serialize `"hrvn":%d` in `firmware/shrikefi/web_status.c`**

In `firmware/shrikefi/web_status.h`:
```c
    float sqi;          /* [0, 1]                                            */
    float rmssd;        /* ms, 0 when withheld by the SQI / 9-pair gate      */
    int   hrv_n;        /* beats currently in the HRV window (0..30+)        */
    float temp;         /* deg C, < -40 when BME280 has no reading           */
```

In `firmware/shrikefi/web_status.c`:
1. In `web_status_json()`:
```c
    if (!json_append_format(
        buf, cap, &used,
        "{\"hr\":%.1f,\"spo2\":%.1f,\"rr\":%.1f,\"sqi\":%.2f,\"rmssd\":%.1f,\"hrvn\":%d,"
        "\"temp\":",
        (double)s->hr, (double)s->spo2, (double)s->rr, (double)s->sqi,
        (double)s->rmssd, s->hrv_n < 0 ? 0 : s->hrv_n)) return 0;
```
2. In `PAGE_HTML` (`firmware/shrikefi/web_status.c`):
   - Change `<div class='card wide'><div class='k'>Signal quality</div>` to `<div class='card'><div class='k'>Signal quality</div>` so `Skin temp` and `Signal quality` form a symmetrical 3rd row in the 2-column `.grid`.
   - In `poll()`, show settling progress when `s.rmssd <= 0 && s.contact && s.hrvn > 0 && s.hrvn < 30`:
     ```c
     "t('hr',positive(s.hr,0));t('spo2',positive(s.spo2,1));"
     "t('rmssd',s.rmssd>0?s.rmssd.toFixed(1):(s.contact&&typeof s.hrvn==='number'&&s.hrvn>0&&s.hrvn<30?(s.hrvn+'/30'):D));"
     ```
   - In the `sos-cancel` IIFE (`lines 797-804`), fix `stop()` so `ok` is reset after release and `CANCEL SOS` can be used repeatedly:
     ```c
     "(function(){var b=document.getElementById('sos-cancel'),tm=null,ok=false;"
     "function start(e){if(tm)return;ok=false;if(e&&e.preventDefault)e.preventDefault();t('sos-note','Hold for 3 s to cancel...');"
     "tm=setTimeout(function(){tm=null;ok=true;postEmergency('/api/emergency/cancel','SOS cancel',3000)},3000)}"
     "function stop(){if(tm){clearTimeout(tm);tm=null;if(!ok)t('sos-note','Cancel aborted - hold for 3 s')}ok=false}"
     ```
   - Recompute the SHA-256 CSP digest of the inline `<script>` in `PAGE_HTML` and update both line 623 (`PAGE_HTML` `<meta http-equiv='Content-Security-Policy' ...>`) and line 909 (`h_root()` `httpd_resp_set_hdr(req, "Content-Security-Policy", ...)`).

- [ ] **Step 3: Populate `st.hrv_n` and unify `rr_for_news2` / `sqi_for_news2` in `firmware/shrikefi/main_shrikefi.c`**

In `firmware/shrikefi/main_shrikefi.c`:
1. Initialize `rr_for_news2`, `sqi_for_news2`, `rr_conf_dbg`, and `rr_depth_dbg` directly from `rr_pub`, `sqi_pub`, `rrconf_pub`, `rrdepth_pub` at lines 1871–1875 and remove the duplicate `s_data_mutex` lock at lines 1992–1998.
2. Set `st.hrv_n = hrv_snapshot.count;` at line 2252 before `web_status_publish(&st);`.

- [ ] **Step 4: Run host unit tests and `shrikefi_host` suite**

Run:
```bash
gcc -Wall -Wextra -Werror -std=c11 -O2 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi firmware/zynq/test_disaster_risk_engine.c firmware/core/disaster_risk_engine.c firmware/core/clinical_vitals_engine.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/pm25_correction.c firmware/core/ppg_sqi.c firmware/core/ppg_respiratory_rate.c firmware/core/ppg_baseline_rr.c firmware/core/skin_estimate.c firmware/core/pressure_trend.c firmware/shrikefi/sos.c firmware/shrikefi/web_status.c firmware/shrikefi/location.c -lm -o test_runner.exe && ./test_runner.exe
```
and `firmware/shrikefi/run_host_test.bat`.

---

### Task 2: Synchronize and Fix `firmware/shrikefi/dashboard/valor_dashboard.html` and `docs/archive/valor_dashboard_2.html`

**Files:**
- Modify: `firmware/shrikefi/dashboard/valor_dashboard.html`
- Modify: `docs/archive/valor_dashboard_2.html`

- [ ] **Step 1: Apply all unified clinical & UI fixes to `firmware/shrikefi/dashboard/valor_dashboard.html`**

1. **`NEWS2_BANDS` alignment with `clinical_vitals_engine.c` (`lines 937-943`):**
   ```javascript
   const NEWS2_BANDS = [
     { max: 0, label: 'Normal',   key: 'emerald', sub: 'Score 0 — routine monitoring.' },
     { max: 3, label: 'Elevated', key: 'amber',   sub: 'Score 1–3 — increased monitoring.' },
     { max: 6, label: 'High',     key: 'crimson', sub: 'Score 4–6 — urgent clinical review.' },
     { max: 99, label: 'Critical', key: 'purple', sub: 'Score ≥ 7 — emergency response.' },
   ];
   ```
   And update the 24h history NEWS2 thresholds (`lines 1365, 1475, 1521`) to `>= 7 ? ' (CRITICAL)' : peakN2 >= 4 ? ' (HIGH)' : ' pts'` and `pt.news2 >= 7 ? C.purple : pt.news2 >= 4 ? C.crimson : C.amber`, plus label `h-min-spo2` as `24h SpO&#8322; Low (hourly avg)`.
2. **Preserve 24h history on live telemetry stall (`renderIdle()` and `connect()`):**
   In `renderIdle()`, wrap `HIST.reset()` in `if (mode === 'idle') { ... }`, and in `connect()`, call `if (typeof HIST !== 'undefined') HIST.reset();` when switching into `mode = 'live'`.
3. **Display settled skin temperature on the `Ambient Temp` card subtitle (`id="temp-note"`) and keep uninstrumented RCP core `Temp` tile as `n/a`:**
   - Add `id="temp-note"` to `<div style="margin-top:6px" class="dim" id="temp-note">BME280 &middot; ambient</div>` in Row 3.
   - In `renderIdle()`, reset `if ($('temp-note')) $('temp-note').textContent = 'BME280 \u00b7 ambient';`.
   - In `render()`, update `$('temp-note').textContent = (V.skinTemp > 0) ? ('BME280 ambient \u00b7 Skin ' + V.skinTemp.toFixed(1) + ' \u00b0C') : 'BME280 \u00b7 ambient';`.
   - In `render()` NEWS2 sub-scores, set `const haveDevTemp = (V.n2tempPts !== null && V.n2tempPts !== undefined && V.n2tempPts > 0);` so when `news2_temp == 0.0f` on device (`TEMPPTS=0`), the RCP core `Temp` sub-score tile stays `n/a` (consistent with `n2-note`), while settled peripheral skin temperature is displayed in the header strip (`Ts`) and `temp-note`.
4. **Clear patient state when `!V.contact` on normal `[TELEMETRY]` lines and in `applyProfile()`:**
   - In `handleLine()` after `V.contact = (V.hr > 0 || V.sqi > 0);`:
     ```javascript
     if (!V.contact) {
       V.skinTemp = 0;
       V.n2hrPts = null; V.n2spo2Pts = null; V.n2rrPts = null; V.n2tempPts = null;
       V.hrvN = null;
     }
     ```
   - In `applyProfile()`, include `skinTemp: 0, n2tempPts: null,` in `Object.assign(V, ...)`.
5. **Fix `CYCLONE=MODERATE` storm badge and gauge mapping (`lines 1983, 2013`):**
   Check `(stormRisk === 'MODERATE' || stormRisk === 'ELEVATED')` and `(V.cyclone === 'MODERATE' || V.cyclone === 'ELEVATED')`.
6. **Recompute and update the `<meta http-equiv="Content-Security-Policy" ...>` SHA-256 script hash** in `firmware/shrikefi/dashboard/valor_dashboard.html`.

- [ ] **Step 2: Synchronize `docs/archive/valor_dashboard_2.html` with `firmware/shrikefi/dashboard/valor_dashboard.html`**

Copy the synchronized, CSP-verified `firmware/shrikefi/dashboard/valor_dashboard.html` to `docs/archive/valor_dashboard_2.html` so both files have identical fixes (30-beat HRV settle progress, `SKINTEMP`, `NEWS2_BANDS`, `MODERATE` cyclone support, and matching CSP SHA-256 hash).

- [ ] **Step 3: Verify CSP SHA-256 hashes on all three HTML sources (`valor_dashboard.html`, `valor_dashboard_2.html`, and `web_status.c`)**

Run a Python verification script confirming that the LF-normalized `<script>` content SHA-256 digest matches the declared `script-src 'sha256-...'` policy in:
1. `firmware/shrikefi/dashboard/valor_dashboard.html`
2. `docs/archive/valor_dashboard_2.html`
3. `firmware/shrikefi/web_status.c` (`PAGE_HTML` meta tag and `h_root()` header)

- [ ] **Step 4: Commit, push, and flash to ESP32-S3 (`COM4`)**

Commit the changes, push to `origin/main`, and build/flash onto the connected ESP32-S3 (`COM4`).
