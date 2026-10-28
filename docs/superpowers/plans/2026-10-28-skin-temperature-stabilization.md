# MAX30102 Skin Temperature Stabilization & Presentation Optimization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Eliminate the MAX30102 skin temperature upward inflation ($>39^\circ\text{C}$) and sample-to-sample up-and-down oscillation by replacing the derivative lead term with a median-of-5 filtered, peak-holding monotonic thermal estimator that climbs smoothly like a digital thermometer and differentiates cold, neutral, and warm fingers accurately.

**Architecture:** Replace the derivative lead predictor ($\tau_{\text{lead}} \cdot \frac{dT}{dt}$) in `firmware/core/skin_estimate.c` with a 5-sample median filter feeding a contact-session peak tracker (`contact_peak_die_c`) and a first-order monotonic charging integrator. Normal and warm fingers climb smoothly from $32.2^\circ\text{C}$ without ever stepping downward (`T(t) >= T(t-1)`), settling at $34.5^\circ\text{C} - 35.0^\circ\text{C}$ for a neutral finger and $\sim 35.9^\circ\text{C} - 36.1^\circ\text{C}$ for a warm finger, while a cold finger cooling the die below baseline still drops below $31.0^\circ\text{C}$ to trigger `Skin: <31.0C (COLD)`.

**Tech Stack:** C11 (ESP-IDF / Xtensa LX7 FreeRTOS + GCC host unit tests), HTML5 / Vanilla JS Web Serial Dashboard (`valor_dashboard.html`).

**Spec:** [`docs/reference/valor_rigorous_skin_temperature_calibration_fusion_master_spec.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/docs/reference/valor_rigorous_skin_temperature_calibration_fusion_master_spec.md), [`docs/JUDGE_QA.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/docs/JUDGE_QA.md), [`AGENTS.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/AGENTS.md).

## Global Constraints

- **Strict Monotonic Rise for Warm Contact:** Whenever `target_c >= st->estimated_skin_c`, `st->estimated_skin_c` advances smoothly at $\le +0.25^\circ\text{C/s}$ and **never decreases** while `finger_present == true`.
- **Median-of-5 Spike Rejection:** Peak-hold tracking (`contact_peak_die_c`) must operate on the **5-sample median** of `raw_die_c`, never on a single unvalidated sample, so a 1-sample I2C/ADC spike (like `37.88 °C` in `tools/skin_calib_session.json`) cannot permanently ratchet the target upward.
- **Real Sensor Differentiation (Cold / Neutral / Warm):**
  - **Cold finger** ($\Delta T_{\text{die}} \le -2.0^\circ\text{C}$): settles $< 31.0^\circ\text{C}$, setting `is_below_range = true` so the OLED displays `Skin: <31.0C (COLD)`.
  - **Neutral resting finger** (Trace 1 in `skin_calib_session.json`, $\Delta T_{\text{die}} \approx +1.5^\circ\text{C}$): settles at **$34.6^\circ\text{C} - 35.1^\circ\text{C}$**.
  - **Warm rubbed finger** (Trace 2 in `skin_calib_session.json`, $\Delta T_{\text{die}} \approx +3.5^\circ\text{C}$): settles at **$35.8^\circ\text{C} - 36.2^\circ\text{C}$** (matching the `36.1 °C` DT-112 reference).
  - **Cold-boot LED self-heating cap:** `delta_die` is bounded to $\le +3.8^\circ\text{C}$ above `effective_baseline = fmaxf(contact_baseline_die_c, last_amb_c + 4.8f)` so LED self-heating from a cold boot never drives the estimate above $36.5^\circ\text{C}$.
- **Single-Precision FPU Safety:** Use `float` literals (`f` suffix) and `fabsf()`/`fminf()`/`fmaxf()` exclusively; must pass `-Wdouble-promotion -Werror`.
- **Hardware & Voltage Safety:** All GPIO and I2C lines strictly observe 3.3 V maximum.
- **Forbidden Topics:** Zero mentions of battery life, battery capacity (mAh/hours), power budget, or BOM cost.
- **100% Offline Edge Operation:** Zero external CDN dependencies; CSP script hash in `valor_dashboard.html` must match the inline `<script>` SHA-256 digest.

## Review Focus

1. **Noisy Die Temperature Trace (Real Hardware Replay):** Feeding both Trace 1 (Neutral) and Trace 2 (Warm) from `tools/skin_calib_session.json` must produce strictly non-decreasing `estimated_skin_c` sequences with zero downward dips, settling near $\sim 34.8^\circ\text{C}$ (Neutral) and $\sim 35.9^\circ\text{C}$ (Warm).
2. **Single-Sample Spike Immunity:** Injecting a single $+3.0^\circ\text{C}$ 1-sample spike mid-trace must be rejected by the 5-sample median filter without ratcheting `contact_peak_die_c`.
3. **Cold-Boot Touch Immunity:** Touching the sensor immediately after cold boot (`idle_die_c = 27.8 °C`, die rising to `37.5 °C` from LED self-heating + finger) must settle $\le 36.2^\circ\text{C}$, never $> 37.0^\circ\text{C}$.
4. **Cold-Finger Hypothermia Detection:** Feeding a cold-finger trace where die temperature drops $2.5^\circ\text{C}$ below baseline (`33.5 -> 30.5 °C`) must drive `estimated_skin_c < 31.0 °C` and set `is_below_range = true`, `is_settled = true`.
5. **Immediate Finger Release Reset & CI Consistency:** Lifting the finger (`finger_present = false` / `NO_FINGER`) must clear `estimated_skin_c = 0.0f`, `is_valid = false`, `V.skinTemp = 0`, and `V.n2tempPts = null`, and all build scripts (`Makefile`, `ci.yml`, `run_host_test.bat`) must link `skin_estimate.c`.

---

### Task 1: Median-of-5 Peak-Holding Monotonic Estimator in `skin_estimate.h` & `skin_estimate.c`

**Files:**
- Modify: [`firmware/core/skin_estimate.h`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/skin_estimate.h)
- Modify: [`firmware/core/skin_estimate.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/skin_estimate.c)
- Modify: [`firmware/zynq/Makefile`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/zynq/Makefile)
- Modify: [`firmware/zynq/test_disaster_risk_engine.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/zynq/test_disaster_risk_engine.c)

**Interfaces:**
- Consumes: `raw_die_c` (`float`), `amb_c` (`float`), `finger_present` (`bool`), `now_ms` (`uint32_t`)
- Produces: `skin_estimate_t` (`b0`, `b1`, `b2`, `med_buf[5]`, `contact_peak_die_c`, `estimated_skin_c`, `slope_c_per_sec`, `is_settled`, `is_valid`, `is_below_range`) and functions `skin_estimate_init()`, `skin_estimate_update()`, `skin_estimate_reset()`, `skin_estimate_set_calibration()`

- [x] **Step 1: Add `skin_estimate.c` to `firmware/zynq/Makefile` and write the failing unit test in `firmware/zynq/test_disaster_risk_engine.c`**

Add `$(CORE_DIR)/skin_estimate.c` to `SRCS_CORE` in `firmware/zynq/Makefile`, `#include "skin_estimate.h"` in `firmware/zynq/test_disaster_risk_engine.c`, and add `test_skin_estimate_monotonic_and_bounded()` testing:
1. Trace 1 (Neutral finger from `tools/skin_calib_session.json`) $\to$ strictly non-decreasing, settles in `[34.4, 35.3] °C`.
2. Trace 2 (Warm finger from `tools/skin_calib_session.json`, including the `37.88 °C` 1-sample spike) $\to$ strictly non-decreasing, settles higher than Trace 1 in `[35.5, 36.3] °C`.
3. Cold-boot touch (`idle = 27.8 °C` $\to$ `37.5 °C`) $\to$ never exceeds `36.4 °C`.
4. Cold-finger touch (`idle = 33.5 °C` $\to$ `30.5 °C`) $\to$ drops below `31.0 °C` and sets `is_below_range = true`, `is_settled = true`.
5. Finger release (`finger_present = false`) $\to$ resets `estimated_skin_c = 0.0f` and `is_valid = false`.

- [x] **Step 2: Run the test to verify it fails on the existing estimator**

Run:
```bash
gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi firmware/zynq/test_disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/hrv_analysis.c firmware/core/pm25_correction.c firmware/core/clinical_vitals_engine.c firmware/core/spo2_engine.c firmware/core/ppg_sqi.c firmware/core/disaster_risk_engine.c firmware/core/pressure_trend.c firmware/core/ppg_respiratory_rate.c firmware/core/ppg_baseline_rr.c firmware/core/skin_estimate.c firmware/shrikefi/sos.c firmware/shrikefi/location.c firmware/shrikefi/web_status.c -lm -o test_engine.exe && ./test_engine.exe
```
Expected: FAIL because the old derivative lead causes downward dips on Trace 1 & Trace 2 and cold-boot touch spikes past `43 °C`.

- [x] **Step 3: Implement the median-of-5 monotonic thermal model in `firmware/core/skin_estimate.h` and `firmware/core/skin_estimate.c`**

In `firmware/core/skin_estimate.h`:
- `SKIN_EST_DEFAULT_B0` = `34.00f` (neutral contact base at $27.5^\circ\text{C}$ ambient)
- `SKIN_EST_DEFAULT_B1` = `0.55f` (contact die delta gain: $+1.5^\circ\text{C} \to 34.8^\circ\text{C}$, $+3.5^\circ\text{C} \to 35.9^\circ\text{C}$)
- `SKIN_EST_DEFAULT_B2` = `0.12f` (ambient room correction gain)
- `SKIN_EST_INITIAL_TOUCH_C` = `32.20f` (initial touch anchor at $t=0$)
- `SKIN_EST_MAX_RISE_PER_SEC` = `0.25f`
- `SKIN_EST_SETTLE_MIN_MS` = `12000u`
- Add `float med_buf[5]; uint8_t med_count; uint8_t med_idx; float contact_peak_die_c;` to `skin_estimate_t`.

In `firmware/core/skin_estimate.c`:
- Compute 5-sample median `med_die_c` of `raw_die_c`, then smooth `last_filtered_die_c = 0.65f * last_filtered_die_c + 0.35f * med_die_c`.
- On contact start:
  - `float min_led_idle = (st->last_amb_c >= 10.0f) ? (st->last_amb_c + 5.0f) : st->idle_die_c;`
  - `st->contact_baseline_die_c = fmaxf((st->idle_die_c >= 15.0f) ? st->idle_die_c : st->last_filtered_die_c, min_led_idle);`
  - `st->contact_peak_die_c = st->last_filtered_die_c;`
  - `st->estimated_skin_c = SKIN_EST_INITIAL_TOUCH_C;`
- During active contact:
  - Update `contact_peak_die_c`: if `last_filtered_die_c > contact_peak_die_c`, `contact_peak_die_c = last_filtered_die_c`.
  - Check for genuine cold-finger cooling (`last_filtered_die_c < st->contact_baseline_die_c - 0.8f`):
    - If cooling significantly below baseline (`raw_delta < -0.8f`), use `delta_die = fmaxf(-5.0f, raw_delta) * 2.0f` so a cold finger drops below `31.0 °C` and triggers `is_below_range = true`.
    - Otherwise (normal/warm finger), use peak-held `delta_die = fmaxf(0.0f, fminf(st->contact_peak_die_c - st->contact_baseline_die_c, 3.8f))` and enforce **strictly non-decreasing** updates (`if (target_c > st->estimated_skin_c)` advance by up to `SKIN_EST_MAX_RISE_PER_SEC`; never decrease).

- [x] **Step 4: Run `test_engine` and `-Wdouble-promotion` check to verify they pass**

Run:
```bash
gcc -Wall -Wextra -Werror -pedantic -Wdouble-promotion -std=c11 -Ifirmware/core -fsyntax-only firmware/core/skin_estimate.c
```
and run `test_engine.exe`.
Expected: `test_skin_estimate_monotonic_and_bounded: PASS` and `ALL TESTS PASSED.`

- [x] **Step 5: Commit Task 1**

```bash
git add firmware/core/skin_estimate.h firmware/core/skin_estimate.c firmware/zynq/Makefile firmware/zynq/test_disaster_risk_engine.c
git commit -m "feat(skin): implement median-of-5 peak-holding monotonic skin temperature estimator"
```

---

### Task 2: Synchronize Host Self-Tests, CI Workflow, and Dashboard Reset

**Files:**
- Modify: [`firmware/shrikefi/main_shrikefi.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c)
- Modify: [`firmware/shrikefi/run_host_test.bat`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/run_host_test.bat)
- Modify: [`.github/workflows/ci.yml`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/.github/workflows/ci.yml)
- Modify: [`firmware/shrikefi/dashboard/valor_dashboard.html`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/dashboard/valor_dashboard.html)

**Interfaces:**
- Consumes: `skin_estimate_t` and `skin_estimate_update()` from Task 1
- Produces: Passing `shrikefi_host` Profile 12 self-test, clean `NO_FINGER` reset in `valor_dashboard.html` with updated CSP SHA-256 hash, and fixed `.github/workflows/ci.yml`

- [x] **Step 1: Update Profile 12 host test in `firmware/shrikefi/main_shrikefi.c` and OLED label**

- In `firmware/shrikefi/main_shrikefi.c` line 2404: format settled reading clearly (`Skin: %4.1f C`).
- In `Profile 12` (lines 3881–3895): assert monotonic non-decreasing behavior across 18 seconds of contact and verify `test_skin.is_settled && test_skin.is_valid`.

- [x] **Step 2: Fix `NO_FINGER` skin temperature state reset in `firmware/shrikefi/dashboard/valor_dashboard.html` and update CSP hash**

In `firmware/shrikefi/dashboard/valor_dashboard.html` inside `if (line.indexOf('NO_FINGER') >= 0)` (line 2210):
- Add `V.skinTemp = 0; V.n2tempPts = null;`.
- Recompute and update the inline `<script>` SHA-256 hash in `<meta http-equiv="Content-Security-Policy">`.

- [x] **Step 3: Fix `firmware/shrikefi/run_host_test.bat` and `.github/workflows/ci.yml`**

- Add `../../firmware/core/ppg_baseline_rr.c` to `firmware/shrikefi/run_host_test.bat`.
- Replace deleted `firmware/shrikefi/tmp117.c` and `test_tmp117.c` references in `.github/workflows/ci.yml` with `firmware/core/skin_estimate.c`.

- [x] **Step 4: Run `shrikefi_host` to verify all 12 profiles pass**

Run: `firmware/shrikefi/run_host_test.bat`
Expected: Profile 12 passes and `shrikefi_host` exits 0.

- [x] **Step 5: Commit Task 2**

```bash
git add firmware/shrikefi/main_shrikefi.c firmware/shrikefi/run_host_test.bat .github/workflows/ci.yml firmware/shrikefi/dashboard/valor_dashboard.html
git commit -m "test(shrikefi): sync host tests, CI workflow, and dashboard NO_FINGER reset for skin temp"
```

---

### Task 3: Update `JUDGE_QA.md` to Match Actual Board Hardware & Flash ESP32-S3

**Files:**
- Modify: [`docs/JUDGE_QA.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/docs/JUDGE_QA.md)

**Interfaces:**
- Consumes: Final estimator behavior from Tasks 1–2
- Produces: Honest, accurate judge Q&A in `docs/JUDGE_QA.md` and flashed ESP32-S3 firmware

- [x] **Step 1: Replace the stale TMP117 claim in `docs/JUDGE_QA.md` §263–264**

Update `docs/JUDGE_QA.md` lines 263–264 to accurately reflect that the board uses the MAX30102 on-die temperature sensor fused with the BME280 ambient sensor via a median-of-5 peak-holding thermal estimator (display-only peripheral skin estimate, `news2_temp = 0.0f` in NEWS2).

- [x] **Step 2: Build and flash ESP32-S3 firmware (if board connected) and commit**

```bash
git add docs/JUDGE_QA.md docs/superpowers/plans/2026-10-28-skin-temperature-stabilization.md
git commit -m "docs(qa): update JUDGE_QA.md skin temperature section to match MAX30102+BME280 estimator"
```
