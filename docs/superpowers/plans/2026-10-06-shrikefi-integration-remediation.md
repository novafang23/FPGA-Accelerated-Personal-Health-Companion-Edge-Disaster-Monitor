# ShrikeFi Hardware-to-Software Integration Remediation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Resolve critical timing, filtering, and telemetry discrepancies across the VALOR (SIH26181) ESP32-S3 firmware, FreeRTOS vitals pipeline, and ForgeFPGA SLG47910C SPI link, unblocking clinical tachycardia alerts (HR $\ge$ 150 BPM) and preventing touch-disconnect interval corruption.

**Architecture:** Heterogeneous dual-target embedded architecture. An on-chip ForgeFPGA executes 8-tap moving average filtering and 4-state systolic peak detection at 50 MHz, communicating via 8-bit SPI Mode 0 with an ESP32-S3 host. The firmware runs dual-core FreeRTOS processing real-time optical PPG samples, Karlen SQI quality scoring, HRV statistics, and Royal College of Physicians NEWS2 clinical triage.

**Tech Stack:** Synthesizable Verilog-2001 (Icarus Verilog 12.0+), C99 / ESP-IDF FreeRTOS (GCC 12+ / MinGW-w64), GTKWave 3.3+.

**Spec:** [`docs/SHRIKEFI_LINK_PROTOCOL.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/docs/SHRIKEFI_LINK_PROTOCOL.md), [`docs/MIGRATION.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/docs/MIGRATION.md), [`AGENTS.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/AGENTS.md).

---

## Global Constraints

- **Strict Voltage Limit:** All FPGA and MCU I/O pins are 3.3 V LVCMOS strict maximum.
- **RTL Synthesis Invariance:** Common RTL in `hardware/common/` must remain vendor-agnostic (no AXI, no vendor primitives).
- **Single-Precision FPU:** ESP32-S3 has a single-precision FPU only (`float`, `sqrtf()`, `fabsf()`; no software `double`).
- **Memory Safety:** Protect shared inter-core buffers (`s_data_mutex` in FreeRTOS); bounds-check all string operations (`snprintf`, `strncpy`; never `sprintf` or `strcpy`).
- **Zero-Dependency Host Harness:** Firmware unit testing must compile and run cleanly under host GCC without external SDK or runtime dependencies.

---

## Review Focus

1. **Sustained Tachycardia Rejection:** In genuine physiological tachycardia (e.g. HR = 175 BPM, IBI = 343 ms), the filter must accept intervals and update `g_state.heart_rate` rather than locking into an infinite rejection loop.
2. **Touch-Disconnect Spurious Delta:** When finger contact is broken for $> 10$ seconds, the first beat on reconnection must not calculate a multi-second delta ($> 5 \times 10^8$ cycles).
3. **Zero-SQI False Crisis Prevention:** When a sensor is unseated or warming up (`SQI == 0.0`), baseline drift must not trigger a false `CLINICAL_CRITICAL` code before a physiological pulse is established.
4. **FPGA Hardware Refractory Alignment:** The firmware minimum interval floor (`IBI_MIN_MS`) must be safely above the FPGA's 250 ms hardware blanking period while remaining low enough to detect life-threatening tachycardias.
5. **Dicrotic Notch Protection:** Lowering `IBI_MIN_MS` must not cause false split-beat double-counting on noisy or notched finger PPG waveforms.

---

## User Review Required

> [!IMPORTANT]
> **Tachycardia Floor Alignment (Task 1):**  
> We lower `IBI_MIN_MS` in [`firmware/shrikefi/main_shrikefi.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c#L181) from `400.0f` ms (150 BPM ceiling) to `272.0f` ms (220 BPM ceiling). This is safely above the FPGA's 250 ms (240 BPM) hardware refractory period and allows `clinical_vitals_engine.c` to trigger critical triage when HR $\ge 150$ BPM.

> [!WARNING]
> **Sensor Warmup Gating (Task 2):**  
> In [`firmware/core/clinical_vitals_engine.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/clinical_vitals_engine.c#L129-L134), `out->sqi == 0.0f` will unconditionally hold triage with `"SENSOR WARMUP: Calibrating signal baseline, holding triage"`. An uncalibrated sensor with zero signal quality cannot reliably measure physiological vitals.

---

## Proposed Changes

```
┌────────────────────────────────────────────────────────────────────────┐
│ Phase 1: Clinical Safety Critical Fixes                                │
│   ├── Task 1: Unblock Tachycardia Floor (main_shrikefi.c)              │
│   └── Task 2: Clinical Crisis & Zero-SQI Hardening (clinical_vitals)   │
├────────────────────────────────────────────────────────────────────────┤
│ Phase 2: Firmware Telemetry & Driver Hygiene                           │
│   └── Task 3: Touch-Disconnect Reset & Stubs (shrikefi_link_driver)    │
├────────────────────────────────────────────────────────────────────────┤
│ Phase 3: Hardware Protocol & Tooling Cleanup                           │
│   ├── Task 4: GTKWave SPI Waveform Configs (*.gtkw)                    │
│   └── Task 5: Documentation & Migration Alignment (MIGRATION.md)       │
└────────────────────────────────────────────────────────────────────────┘
```

---

### Component 1: Firmware Clinical Pipeline & Tachycardia Floor

#### [MODIFY] [`firmware/shrikefi/main_shrikefi.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c)
- Lower `IBI_MIN_MS` from `400.0f` to `272.0f`.
- In `ibi_pipeline_submit()`, reset `p->prime_count = 0` on `IBI_REJECT_ESCAPE` so that a true sustained rate shift re-seeds the median filter cleanly.
- In `no optical contact` block (line 950), call `shrikefi_link_reset_beat_tracking()`.
- Update line 731 comment to accurately reflect MISO Bit 7 detection instead of GPIO 10.

```diff
- #define IBI_MIN_MS    400.0f   /* hard floor: below this the detector double-fired  */
+ #define IBI_MIN_MS    272.0f   /* hard floor (220 BPM): allows tachycardia >= 150 BPM; above 250 ms FPGA blanking */

@@ -468,6 +468,7 @@
     } else if (++p->reject_streak >= IBI_REJECT_ESCAPE) {
         ESP_LOGW(TAG, "IBI filter rejected %d intervals in a row; discarding the "
                       "rhythm reference and re-seeding from live beats",
                  p->reject_streak);
         hrv_median_init(&p->ref);
         p->skip_next = false;
         p->reject_streak = 0;
+        p->prime_count = 0;
     }

@@ -730,3 +731,3 @@
             if (fpga_beat && optical_contact) {
-                /* Hardware beat detected by ForgeFPGA on GPIO 10 with verified optical tissue contact */
+                /* Hardware beat detected by ForgeFPGA on MISO Bit 7 with verified optical tissue contact */

@@ -948,4 +950,5 @@
                 hrv_init(&hrv_state);   /* Reset HRV history on finger removal */
                 ibi_pipeline_reset(&ibi_pipe); /* ...and the detector-handover state */
+                shrikefi_link_reset_beat_tracking(); /* Clear inter-beat timing baseline */
                 ppg_history_reset();  /* ...and the respiration/SQI windows */
```

#### [MODIFY] [`firmware/core/clinical_vitals_engine.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/clinical_vitals_engine.c)
- Ensure that `out->sqi == 0.0f` strictly holds triage without false alarm triggers while preserving the emergency crisis override for low SQI (`0.0f < sqi < 0.70f`).

```diff
-    if (out->sqi == 0.0f && !is_absolute_crisis) {
+    if (out->sqi == 0.0f) {
         out->level = CLINICAL_ELEVATED;
         out->alert_flags = ALERT_SIGNAL_NOISE;
         snprintf(out->advisory, sizeof(out->advisory),
                  "SENSOR WARMUP: Calibrating signal baseline, holding triage.");
         return;
     } else if (out->sqi < 0.70f && !is_absolute_crisis) {
```

---

### Component 2: ShrikeFi Link Driver & Telemetry Tracking

#### [MODIFY] [`firmware/shrikefi/shrikefi_link_driver.h`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/shrikefi_link_driver.h)
- Declare `void shrikefi_link_reset_beat_tracking(void);`.
- Document hardware reality in `shrikefi_set_threshold()` (hardwired to 120 in RTL) and `shrikefi_write_red_sample()` (software buffered).

#### [MODIFY] [`firmware/shrikefi/shrikefi_link_driver.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/shrikefi_link_driver.c)
- Implement `shrikefi_link_reset_beat_tracking()`: resets `s_last_beat_time_us = 0`, `s_last_beat = false`, `s_beat_detected_latched = false`, and `s_sim_ibi = 0`.
- Initialize `s_sim_ibi = 0` (prevent publishing static 3280 cycles before first interval).

```c
void shrikefi_link_reset_beat_tracking(void) {
#ifdef ESP_PLATFORM
    s_last_beat_time_us = 0;
    s_last_beat = false;
    s_beat_detected_latched = false;
    s_sim_ibi = 0;
#else
    s_sim_ibi = 0;
#endif
}
```

---

### Component 3: Hardware Waveform Configuration & Documentation

#### [MODIFY] [`hardware/shrikefi/shrikefi_sim.gtkw`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/hardware/shrikefi/shrikefi_sim.gtkw)
- Replace legacy parallel signals with modern SPI bus signals: `spi_sck`, `spi_ss_n`, `spi_mosi`, `spi_miso`, `clk_en`, `dut.rx_data[7:0]`, `dut.filt_sample[7:0]`, `dut.beat_latched`, `dut.tx_data[7:0]`, `dut.u_peak_det.current_state[1:0]`.

#### [MODIFY] [`hardware/shrikefi/presentation.gtkw`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/hardware/shrikefi/presentation.gtkw)
- Modernize signal labels to reflect 8-bit SPI mode 0 and current 363 LUT5 post-route synthesis metrics.

#### [MODIFY] [`docs/MIGRATION.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/docs/MIGRATION.md)
- Clarify timing specification: the 20 ns IBI resolution is an internal RTL counter (`ibi_cycles`), whereas firmware currently samples beats at the ~10 ms SPI sample boundary.
- Confirm post-route utilization figures: 363 / 1120 LUT5s (32.41%), 202 FFs, 75 CLBs.

---

## Step-by-Step Implementation Tasks

### Task 1: Unblock Tachycardia Detection Floor in Firmware

**Files:**
- Modify: [`firmware/shrikefi/main_shrikefi.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c)
- Test: Add Host Test Profile 9 to [`firmware/shrikefi/main_shrikefi.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c)

**Interfaces:**
- Consumes: `ibi_pipeline_submit(ibi_pipeline_t *p, hrv_state_t *hrv, int source, float ibi_ms)`
- Produces: `g_state.heart_rate` valid up to 220 BPM; `IBI_MIN_MS = 272.0f`.

- [ ] **Step 1: Write host test for tachycardia acceptance**
Add test case in `main_shrikefi.c` (`#ifndef ESP_PLATFORM`) verifying that a sequence of 350 ms intervals (171 BPM) passes through `ibi_pipeline_submit()`, yields `verdict = "ok"`, and computes `inst_hr > 150`.
- [ ] **Step 2: Update `IBI_MIN_MS` to `272.0f` and `p->prime_count = 0` on escape**
Apply changes to [`main_shrikefi.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c).
- [ ] **Step 3: Compile and run host tests**
Run: `gcc -O2 -Wall -I. -I../core -o shrikefi_host.exe main_shrikefi.c ../core/hrv_analysis.c ../core/spo2_engine.c ../core/disaster_risk_engine.c ../core/nn_risk_model.c ../core/nn_risk_model_int8.c ../core/pm25_calibration_int8.c ../core/clinical_vitals_engine.c ../core/ppg_respiratory_rate.c ../core/ppg_sqi.c ../core/pressure_trend.c sos.c location.c web_status.c shrikefi_link_driver.c -lm && ./shrikefi_host.exe`
- [ ] **Step 4: Commit**
`git commit -am "fix(firmware): lower IBI_MIN_MS to 272ms to unblock clinical tachycardia"`

---

### Task 2: Clinical Crisis Tachycardia & Zero-SQI Warmup Hardening

**Files:**
- Modify: [`firmware/core/clinical_vitals_engine.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/core/clinical_vitals_engine.c)
- Test: [`firmware/shrikefi/main_shrikefi.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c) host test harness

**Interfaces:**
- Consumes: `clinical_vitals_assess_full(float hr, float spo2, float rmssd, float rr, float sqi, clinical_assessment_t *out)`
- Produces: `CLINICAL_CRITICAL` when $HR \ge 150$; holds triage with `"SENSOR WARMUP"` when $SQI == 0.0$.

- [ ] **Step 1: Update SQI evaluation in `clinical_vitals_engine.c`**
Modify lines 129–134 so that `out->sqi == 0.0f` returns warmup status unconditionally.
- [ ] **Step 2: Verify existing Profile 3 and Profile 4 clinical benchmarks**
Run host test suite and verify all ICU emergency and motion artifact assertions pass.
- [ ] **Step 3: Commit**
`git commit -am "fix(clinical): harden SQI warmup gate and verify critical tachycardia alerts"`

---

### Task 3: Touch-Disconnect Reset & Driver Hygiene

**Files:**
- Modify: [`firmware/shrikefi/shrikefi_link_driver.h`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/shrikefi_link_driver.h)
- Modify: [`firmware/shrikefi/shrikefi_link_driver.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/shrikefi_link_driver.c)
- Modify: [`firmware/shrikefi/main_shrikefi.c`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/firmware/shrikefi/main_shrikefi.c)

**Interfaces:**
- Produces: `void shrikefi_link_reset_beat_tracking(void);`
- Consumes: Called in `main_shrikefi.c` whenever `!optical_contact`.

- [ ] **Step 1: Declare and implement `shrikefi_link_reset_beat_tracking`**
Add declaration in `shrikefi_link_driver.h` and implementation in `shrikefi_link_driver.c`.
- [ ] **Step 2: Call reset in `main_shrikefi.c` and fix comment at line 731**
Call `shrikefi_link_reset_beat_tracking()` in finger removal block. Correct line 731 comment.
- [ ] **Step 3: Compile and run host tests**
Verify build succeeds with zero compiler warnings.
- [ ] **Step 4: Commit**
`git commit -am "fix(driver): reset beat timing on touch disconnect and clarify SPI comments"`

---

### Task 4: GTKWave Waveform Modernization

**Files:**
- Modify: [`hardware/shrikefi/shrikefi_sim.gtkw`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/hardware/shrikefi/shrikefi_sim.gtkw)
- Modify: [`hardware/shrikefi/presentation.gtkw`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/hardware/shrikefi/presentation.gtkw)

- [ ] **Step 1: Re-structure GTKWave signal groups for SPI link**
Update `shrikefi_sim.gtkw` and `presentation.gtkw` to reference `tb_forgefpga_system.spi_*` signals.
- [ ] **Step 2: Run Verilog RTL simulation**
Run: `iverilog -o sim_shrikefi.vvp -s tb_forgefpga_system tb_forgefpga_system.v forgefpga_ppg_top.v spi_target.v ../common/moving_average_8tap.v ../common/ppg_peak_detector.v && vvp sim_shrikefi.vvp`
Verify 10/10 tests pass and `shrikefi_sim.vcd` is generated.
- [ ] **Step 3: Commit**
`git commit -am "chore(sim): update GTKWave save files for 8-bit SPI mode 0 signals"`

---

### Task 5: Documentation & Migration Alignment

**Files:**
- Modify: [`docs/MIGRATION.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/docs/MIGRATION.md)
- Modify: [`docs/SHRIKEFI_LINK_PROTOCOL.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/docs/SHRIKEFI_LINK_PROTOCOL.md)

- [ ] **Step 1: Synchronize timing and resource documentation**
Align timing rows in comparison table with ~10 ms SPI sampling reality; re-verify 363 LUT5 utilization citations.
- [ ] **Step 2: Commit**
`git commit -am "docs(migration): align timing resolution specifications and resource tables"`

---

## Verification Plan

### Automated Tests
1. **Verilog RTL Full-System Simulation:**
   ```bash
   cd hardware/shrikefi
   iverilog -o sim_shrikefi.vvp -s tb_forgefpga_system tb_forgefpga_system.v forgefpga_ppg_top.v spi_target.v ../common/moving_average_8tap.v ../common/ppg_peak_detector.v
   vvp sim_shrikefi.vvp
   ```
   *Expected:* `10 passed, 0 failed (out of 10 checks) >>> ALL TESTS PASSED <<<`

2. **Host C Firmware Algorithmic Validation Suite:**
   ```bash
   cd firmware/shrikefi
   gcc -O2 -Wall -I. -I../core -o shrikefi_host.exe main_shrikefi.c ../core/hrv_analysis.c ../core/spo2_engine.c ../core/disaster_risk_engine.c ../core/nn_risk_model.c ../core/nn_risk_model_int8.c ../core/pm25_calibration_int8.c ../core/clinical_vitals_engine.c ../core/ppg_respiratory_rate.c ../core/ppg_sqi.c ../core/pressure_trend.c sos.c location.c web_status.c shrikefi_link_driver.c -lm
   ./shrikefi_host.exe
   ```
   *Expected:* All 9 profiles complete with zero assertion failures.

### Manual Verification
1. Inspect GTKWave save file `hardware/shrikefi/shrikefi_sim.gtkw` against `shrikefi_sim.vcd` to confirm clean signal rendering without unresolved symbols.
2. Verify that `git diff` contains no unexpected changes to baseline Zynq RTL or hardware pin assignments.
