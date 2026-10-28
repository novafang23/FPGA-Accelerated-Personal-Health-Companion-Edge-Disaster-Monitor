# HRV Robust Stabilization & Judge-Ready Windowing Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate RMSSD outlier inflation in short acquisition windows by introducing a consecutive beat-to-beat difference delta gate (`HRV_MAX_DELTA_IBI_MS = 120.0f`) and right-sizing the stabilization window to 30 beats (~24 seconds) for live judge demonstrations.

**Architecture:** Causal artifact-rejection pipeline in C11 firmware. Beat intervals are filtered through a physiological consecutive difference gate (|ΔIBI| ≤ 120 ms) in [`firmware/core/hrv_analysis.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.c), right-sizing `HRV_SETTLE_MIN_SAMPLES` to 30 beats and `HRV_SETTLE_MIN_SPAN_MS` to 20,000 ms (~20–25s). Dual-stage user feedback provides immediate Heart Rate on beat 1 while presenting an interactive settling countdown (`HRV settle n/30`) until locked clinical RMSSD is published.

**Tech Stack:** C11, ESP-IDF FreeRTOS (ESP32-S3), Renesas ForgeFPGA SPI link, Vanilla JS/HTML5 Dashboard, GCC / MinGW-w64 regression test harness.

**Spec:** `docs/superpowers/plans/2026-10-28-hrv-stabilization-and-windowing.md`

## Global Constraints

- Hardware Pin & Voltage Limits: Strict 3.3V maximum on all ESP32-S3 and ForgeFPGA GPIOs.
- Zero Mentions: Do not discuss battery life, battery capacity (mAh or operating hours), power budgets, or bill-of-materials (BOM) cost.
- Maintain Documentation Integrity: Preserve all existing comments and docstrings unrelated to this change.
- Strict C Safety: Single-precision FPU only (`float`, `sqrtf()`, `fabsf()`); bounds-check all string operations with `snprintf()`; never use `strcpy` or `sprintf()`.
- Offline Edge Operation: Zero external CDN scripts or remote dependencies in dashboard and firmware.
- Clickable links for all files and symbols formatted as `file:///...`

## Review Focus

1. **Lone Outlier Jump Resilience:** A sudden ectopic or motion jump (|ΔIBI| > 120 ms) must have its pair invalidated (`pair_valid = 0`), ensuring RMSSD remains within normal physiological bounds (20–45 ms) without discarding the beat from Heart Rate calculation.
2. **Consecutive Valid Beats Across Normal RSA:** Normal respiratory sinus arrhythmia modulation (|ΔIBI| ≤ 100 ms) must remain valid (`pair_valid = 1`) so genuine vagal tone is preserved.
3. **Publication Window Timing:** Settle gate must require both ≥30 contiguous valid intervals AND ≥20,000 ms summed IBI span before unlocking `hrv_is_settled()`.
4. **OLED & Dashboard Countdown Synchronization:** Both the physical 0.96" SSD1306 OLED and the web dashboard must dynamically display `HRV settle n/30` during acquisition, switching to `HRV: XX.Xms` upon completion.
5. **CSP Integrity:** Inline script edits in `valor_dashboard.html` must have their SHA-256 hash strictly updated in the `<meta http-equiv="Content-Security-Policy">` header and verified by `verify_dashboard.py`.

---

### Task 1: Clinical Consecutive Delta Gate in HRV Core

**Files:**
- Modify: [`firmware/core/hrv_analysis.h:40-60`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.h#L40-L60)
- Modify: [`firmware/core/hrv_analysis.c:22-50`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.c#L22-L50)
- Test: [`firmware/zynq/test_disaster_risk_engine.c:68-108`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/zynq/test_disaster_risk_engine.c#L68-L108)

**Interfaces:**
- Consumes: `float ibi_ms`, `hrv_state_t *state`
- Produces: `#define HRV_MAX_DELTA_IBI_MS 120.0f`, `state->pair_valid[state->head]` gating on both `has_previous_ibi` and `fabsf(ibi_ms - prev_ibi) <= HRV_MAX_DELTA_IBI_MS`.

- [ ] **Step 1: Write failing test in `firmware/zynq/test_disaster_risk_engine.c`**

Add unit test `test_hrv_consecutive_delta_gate()`:
```c
static void test_hrv_consecutive_delta_gate(void) {
    hrv_state_t hrv;
    hrv_init(&hrv);

    /* Feed 15 steady beats at 800 ms with +/- 20 ms normal RSA variation */
    for (int i = 0; i < 15; i++) {
        float ibi = 800.0f + ((i % 2 == 0) ? 20.0f : -20.0f);
        hrv_add_ibi(&hrv, ibi);
    }
    hrv_compute(&hrv);
    float baseline_rmssd = hrv.rmssd;
    assert(baseline_rmssd > 35.0f && baseline_rmssd < 45.0f);

    /* Inject a single 250 ms motion/dicrotic jump (800 -> 1050 ms), then return to 800 ms */
    hrv_add_ibi(&hrv, 1050.0f);
    hrv_add_ibi(&hrv, 800.0f);
    hrv_compute(&hrv);

    /* Without the delta gate, the 250 ms jump would inflate RMSSD to >70 ms.
     * With HRV_MAX_DELTA_IBI_MS (120 ms), both jumps are excluded from RMSSD
     * while the beats are still counted in hrv.count and mean_hr. */
    assert(hrv.rmssd < 50.0f);
    assert(hrv.count == 17);
    printf("test_hrv_consecutive_delta_gate: PASS (RMSSD=%.1f ms)\n", hrv.rmssd);
}
```

- [ ] **Step 2: Run test to verify it fails**

Run:
```powershell
python -c "
import subprocess, os
env = os.environ.copy()
env['PATH'] = r'C:\msys64\ucrt64\bin;' + env.get('PATH', '')
cmd = ['gcc', '-std=c11', '-Ifirmware/core', '-Ifirmware/zynq', '-Ifirmware/shrikefi',
       'firmware/zynq/test_disaster_risk_engine.c', 'firmware/core/hrv_analysis.c', 'firmware/core/spo2_engine.c',
       'firmware/core/disaster_risk_engine.c', 'firmware/core/nn_risk_model.c', 'firmware/core/nn_risk_model_int8.c',
       'firmware/core/clinical_vitals_engine.c', 'firmware/core/ppg_sqi.c', 'firmware/core/ppg_respiratory_rate.c',
       'firmware/core/ppg_baseline_rr.c', 'firmware/core/pm25_correction.c', 'firmware/core/pressure_trend.c',
       'firmware/shrikefi/sos.c', 'firmware/shrikefi/web_status.c', 'firmware/shrikefi/location.c',
       '-lm', '-o', 'test_engine.exe']
subprocess.run(cmd, check=True, env=env)
subprocess.run(['test_engine.exe'], check=True, env=env)
"
```
Expected: FAIL with `Assertion failed: hrv.rmssd < 50.0f`.

- [ ] **Step 3: Implement consecutive delta gate in `hrv_analysis.h` and `hrv_analysis.c`**

In [`firmware/core/hrv_analysis.h`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.h):
```c
/* Maximum physiological beat-to-beat difference (Task Force 1996, Kamath 2012).
 * Sinus rhythm cannot accelerate/decelerate by >120 ms in a single beat without
 * an ectopic beat, missed peak, or optical motion artifact. Intervals exceeding
 * this jump are retained for Heart Rate but excluded from RMSSD squaring. */
#define HRV_MAX_DELTA_IBI_MS 120.0f
```

In [`firmware/core/hrv_analysis.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.c):
In `hrv_add_ibi(hrv_state_t *state, float ibi_ms)`:
Check if `state->has_previous_ibi` is set. If set, retrieve previous interval at `(state->head - 1 + HRV_BUFFER_SIZE) % HRV_BUFFER_SIZE`.
If `fabsf(ibi_ms - prev_ibi) > HRV_MAX_DELTA_IBI_MS`, set `pair_valid = 0` (and do not let the jump contaminate RMSSD), but keep `state->has_previous_ibi = 1` so the subsequent beat can compare against this new accepted baseline.

- [ ] **Step 4: Run test to verify it passes**

Run test harness.
Expected: PASS with `test_hrv_consecutive_delta_gate: PASS (RMSSD=... ms)`.

- [ ] **Step 5: Commit**

```bash
git add firmware/core/hrv_analysis.h firmware/core/hrv_analysis.c firmware/zynq/test_disaster_risk_engine.c
git commit -m "feat(hrv): add 120ms consecutive delta gate to eliminate RMSSD outlier inflation"
```

---

### Task 2: Right-Size Settle Window to 30 Beats & 20s Span

**Files:**
- Modify: [`firmware/core/hrv_analysis.h:40-46`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.h#L40-L46)
- Modify: [`firmware/core/hrv_analysis.c:120-156`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.c#L120-L156)
- Modify: [`firmware/shrikefi/main_shrikefi.c:2820-2835`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c#L2820-L2835)
- Modify: [`firmware/shrikefi/main_shrikefi.c:3800-3815`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c#L3800-L3815)
- Test: [`firmware/zynq/test_disaster_risk_engine.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/zynq/test_disaster_risk_engine.c)

**Interfaces:**
- Consumes: `HRV_SETTLE_MIN_SAMPLES`, `HRV_SETTLE_MIN_SPAN_MS`
- Produces: `hrv_is_settled(state)` returning 1 only when `count >= 30` and `span_ms >= 20000.0f`.

- [ ] **Step 1: Write failing test in `test_disaster_risk_engine.c`**

Add unit test `test_hrv_settle_window_thresholds()`:
```c
static void test_hrv_settle_window_thresholds(void) {
    hrv_state_t hrv;
    hrv_init(&hrv);

    /* Feed 20 beats at 800 ms (16.0 s) */
    for (int i = 0; i < 20; i++) {
        hrv_add_ibi(&hrv, 800.0f);
    }
    hrv_compute(&hrv);
    /* Structural readiness requires 10 beats */
    assert(hrv_is_ready(&hrv));
    /* Under 30 beats / 20 s gate, 20 beats must NOT be settled */
    assert(!hrv_is_settled(&hrv));

    /* Feed 10 more beats to reach 30 beats (24.0 s span) */
    for (int i = 0; i < 10; i++) {
        hrv_add_ibi(&hrv, 800.0f);
    }
    hrv_compute(&hrv);
    /* Now settled and ready for clinical publication */
    assert(hrv_is_settled(&hrv));
    printf("test_hrv_settle_window_thresholds: PASS\n");
}
```

- [ ] **Step 2: Run test to verify it fails**

Run test harness.
Expected: FAIL with `Assertion failed: !hrv_is_settled(&hrv)` (since current window is 20 beats).

- [ ] **Step 3: Update `HRV_SETTLE_MIN_SAMPLES` and `HRV_SETTLE_MIN_SPAN_MS`**

In [`firmware/core/hrv_analysis.h`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/hrv_analysis.h):
```c
#define HRV_SETTLE_MIN_SAMPLES 30
#define HRV_SETTLE_MIN_SPAN_MS 20000.0f
```

In [`firmware/shrikefi/main_shrikefi.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c):
Update OLED display logic:
```c
if (g_state.hrv_count >= HRV_SETTLE_MIN_SAMPLES && g_state.hrv_rmssd > 0.0f) {
    snprintf(buf, sizeof(buf), "HRV: %4.1fms", g_state.hrv_rmssd);
} else {
    snprintf(buf, sizeof(buf), "HRV settle %d/%d", g_state.hrv_count, HRV_SETTLE_MIN_SAMPLES);
}
```
Update test/mock fixture summary structs initializing `summary.count = HRV_SETTLE_MIN_SAMPLES`.

- [ ] **Step 4: Run test to verify it passes**

Run test harness.
Expected: PASS with `test_hrv_settle_window_thresholds: PASS`.

- [ ] **Step 5: Commit**

```bash
git add firmware/core/hrv_analysis.h firmware/core/hrv_analysis.c firmware/shrikefi/main_shrikefi.c firmware/zynq/test_disaster_risk_engine.c
git commit -m "feat(hrv): right-size settle window to 30 beats and 20s span for judge demonstrations"
```

---

### Task 3: Dashboard & Telemetry Update

**Files:**
- Modify: [`firmware/shrikefi/dashboard/valor_dashboard.html`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/dashboard/valor_dashboard.html)
- Modify: [`firmware/shrikefi/dashboard/verify_dashboard.py`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/dashboard/verify_dashboard.py)
- Modify: [`firmware/shrikefi/dashboard/README.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/dashboard/README.md)

**Interfaces:**
- Consumes: WebSocket / HTTP telemetry payload `hrv_count`, `hrv_rmssd`
- Produces: Updated DOM subtitle `settling · ${count} of 30 beats`, updated CSP script hash in `<meta>`.

- [ ] **Step 1: Write failing check in `verify_dashboard.py`**

In [`firmware/shrikefi/dashboard/verify_dashboard.py`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/dashboard/verify_dashboard.py), update the settlement check:
Ensure it verifies `30-beat settlement threshold logic exists` instead of 20-beat.
Run:
```powershell
python firmware/shrikefi/dashboard/verify_dashboard.py
```
Expected: FAIL due to missing 30-beat threshold logic or CSP hash mismatch.

- [ ] **Step 2: Update `valor_dashboard.html`**

Update `valor_dashboard.html` script logic:
- Change settle gate condition `hrv_count < 30` and subtitle text `settling · ${hrv_count} of 30 beats`.
- Update explanation comments.
- Recalculate SHA-256 hash of the `<script>` tag:
  `sha256 = base64(hashlib.sha256(script_bytes).digest())`
- Update `<meta http-equiv="Content-Security-Policy">` with the new hash.

- [ ] **Step 3: Run `verify_dashboard.py` to verify it passes**

Run:
```powershell
python firmware/shrikefi/dashboard/verify_dashboard.py
```
Expected: PASS across all security and functional element checks.

- [ ] **Step 4: Commit**

```bash
git add firmware/shrikefi/dashboard/valor_dashboard.html firmware/shrikefi/dashboard/verify_dashboard.py firmware/shrikefi/dashboard/README.md
git commit -m "feat(dashboard): synchronize HRV settle threshold to 30 beats with updated CSP hash"
```

---

### Task 4: Full Regression Verification

**Files:**
- Verify: [`firmware/zynq/test_disaster_risk_engine.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/zynq/test_disaster_risk_engine.c)
- Verify: [`firmware/shrikefi/dashboard/verify_dashboard.py`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/dashboard/verify_dashboard.py)

- [ ] **Step 1: Execute complete host C test suite**

Run:
```powershell
python -c "
import subprocess, os
env = os.environ.copy()
env['PATH'] = r'C:\msys64\ucrt64\bin;' + env.get('PATH', '')
cmd = ['gcc', '-Wall', '-Wextra', '-std=c11', '-Ifirmware/core', '-Ifirmware/zynq', '-Ifirmware/shrikefi',
       'firmware/zynq/test_disaster_risk_engine.c', 'firmware/core/hrv_analysis.c', 'firmware/core/spo2_engine.c',
       'firmware/core/disaster_risk_engine.c', 'firmware/core/nn_risk_model.c', 'firmware/core/nn_risk_model_int8.c',
       'firmware/core/clinical_vitals_engine.c', 'firmware/core/ppg_sqi.c', 'firmware/core/ppg_respiratory_rate.c',
       'firmware/core/ppg_baseline_rr.c', 'firmware/core/pm25_correction.c', 'firmware/core/pressure_trend.c',
       'firmware/shrikefi/sos.c', 'firmware/shrikefi/web_status.c', 'firmware/shrikefi/location.c',
       '-lm', '-o', 'test_engine.exe']
subprocess.run(cmd, check=True, env=env)
res = subprocess.run(['test_engine.exe'], capture_output=True, text=True, env=env)
print(res.stdout)
assert res.returncode == 0
if os.path.exists('test_engine.exe'): os.remove('test_engine.exe')
"
```
Expected: PASS with `ALL TESTS PASSED (Disaster Risk Engine)`.

- [ ] **Step 2: Execute dashboard test suite**

Run:
```powershell
python firmware/shrikefi/dashboard/verify_dashboard.py
```
Expected: PASS with `ALL CHECKS PASSED: Dashboard is production-ready`.

- [ ] **Step 3: Commit**

```bash
git commit --allow-empty -m "test(hrv): all host regression and dashboard verification suites passing"
```

---

### Task 5: Hardware Target Build & COM4 Deployment

**Files:**
- Compile: ESP-IDF target binary
- Flash: `COM4`

- [ ] **Step 1: Build firmware with ESP-IDF**

Run:
```powershell
C:\Espressif\frameworks\esp-idf-v5.4\export.ps1 ; idf.py -C firmware/shrikefi build
```
Expected: `Project build complete. To flash, run idf.py flash`.

- [ ] **Step 2: Flash firmware to ESP32-S3 on COM4**

Run:
```powershell
C:\Espressif\frameworks\esp-idf-v5.4\export.ps1 ; idf.py -C firmware/shrikefi -p COM4 flash
```
Expected: `Hash of data verified. Leaving... Staying in bootloader... Done!`.

- [ ] **Step 3: Verify live serial stream**

Open serial monitor or run test monitor script:
Verify:
1. Contact detection triggers immediately.
2. Beat 1 to 29 displays: `IBI 0 <ms> ok ref=... n=... rmssd=...` and OLED shows `HRV settle n/30`.
3. Beat 30 (with >20s contact) transitions to: OLED `HRV: XX.Xms`, where RMSSD is in physiological range (25–45 ms).
4. No outlier spikes past 50 ms.
