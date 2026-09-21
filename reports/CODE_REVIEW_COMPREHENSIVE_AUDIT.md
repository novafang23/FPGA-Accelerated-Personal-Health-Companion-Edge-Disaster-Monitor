# Comprehensive Adversarial Code Review Report
**Project:** VALOR — SIH26181 Personal Health Companion & Edge Disaster Monitor  
**Target Architectures:** Renesas ForgeFPGA (SLG47910) + ESP32-S3 (ShrikeFi) & Xilinx Zynq-7000 (XC7Z020)  
**Review Mode:** Full Adversarial Audit (`gsd-code-reviewer` standard)  
**Date:** 2026-09-20  

---

## Executive Summary

An adversarial code review was conducted across the heterogeneous firmware, hardware RTL, and host application stack. All testbenches and unit gates currently pass:
- **Zynq AXI Testbench:** 6/6 passing
- **ShrikeFi SPI Testbench:** 10/10 passing
- **Common Peak Detector Testbench:** 2/2 passing
- **Firmware Unit Engine Test:** PASS
- **MIMIC-III Accuracy Gate:** 94.11% accuracy, 100% INT8/FP32 tier agreement

However, thorough static analysis and code tracing uncovered **3 BLOCKERS**, **5 WARNINGS**, and **3 INFORMATIONAL** issues that compromise patient privacy, data integrity, thread safety, and cross-subsystem telemetry synchronization.

---

## Severity Classification Table

| ID | Component | Severity | Description |
|:---|:---|:---|:---|
| **S-01** | `wifi_mqtt_manager.c` | **BLOCKER** | Patient health vitals broadcast unencrypted to public MQTT broker |
| **C-01** | `pms5003.c` / `main_shrikefi.c` | **BLOCKER** | Unsynchronized concurrent access to 28-byte sensor struct (torn reads) |
| **L-01** | `clinical_vitals_engine.c` | **BLOCKER** | Zero SQI during warm-up masquerades as 95% hospital-grade quality |
| **D-01** | `shrikefi_dashboard.c` | **WARNING** | PC GUI Dashboard fails to parse live `RR` and hardcodes `SQI` to 0.95 |
| **L-02** | `main_shrikefi.c` | **WARNING** | Lingering `RR` and `SQI` state across finger removal sessions |
| **L-03** | `main_shrikefi.c` | **WARNING** | Oscilloscope PPG stream drops samples exactly at systolic peaks |
| **C-02** | `clinical_vitals_engine.c` | **WARNING** | Non-reentrant static buffer in `clinical_fuse_triage` |
| **C-03** | `wifi_mqtt_manager.c` | **WARNING** | Unconditional `esp_mqtt_client_start()` on WiFi re-connect |
| **H-01** | `ppg_peak_detector.v` | **WARNING** | Downslope re-triggering in FSM after refractory timer expiration |
| **B-01** | `forgefpga_bitstream.h` | **INFO** | Non-static 46 KB array defined in header with external linkage |
| **B-02** | `Kconfig.projbuild` | **INFO** | Stale documentation describing legacy I2C bitstream flashing |
| **B-03** | `pms5003.c` | **INFO** | UART framing sync dropped on duplicate `0x42` start bytes |

---

## Detailed Findings

### 1. S-01 (BLOCKER): Patient Health Vitals Broadcast to Public Broker
- **Files:** `firmware/shrikefi/wifi_mqtt_manager.c:59-61, 362-374`
- **Mechanism:**
  ```c
  #define MQTT_BROKER_URI "mqtt://broker.hivemq.com"  // Free public test broker
  #define MQTT_TOPIC      "sih26181/shrikefi/health"
  ```
  `cloud_publish_health_data()` formats a JSON string containing heart rate, SpO2, RMSSD, ambient temperature, PM2.5, and triage risk level, and transmits it at 1 Hz over unencrypted plaintext MQTT (`mqtt://`, port 1883) to HiveMQ's public test broker.
- **Impact:** HiveMQ is completely unauthenticated and indexed by public MQTT scrapers. Anyone in the world can subscribe to `sih26181/shrikefi/health` and intercept real-time physiological telemetry. This directly violates SIH Problem Statement Requirement 5 (*"Edge-first privacy: minimize transmission of sensitive personal information"*).
- **Remediation:**
  1. Introduce `CONFIG_SHRIKEFI_CLOUD_PUBLISH` in `main/Kconfig.projbuild` defaulting to `n`.
  2. Guard `cloud_publish_health_data()` and MQTT client startup behind this configuration flag.
  3. Support TLS/MQTTS (`mqtts://`) when cloud publishing is explicitly enabled.

---

### 2. C-01 (BLOCKER): Unsynchronized Concurrent Access to PMS5003 Data
- **Files:** `firmware/shrikefi/pms5003.c:118-122`, `firmware/shrikefi/main_shrikefi.c:868, 1134`
- **Mechanism:**
  ```c
  // In task_pms5003_uart (Priority 3, Core 1):
  pms5003_feed_byte(&s_pms5003, byte); // writes dev->last_data in pms5003_parse_frame

  // In task_disaster_monitor (Priority 2, Core 1):
  read_pms5003_data(&pms_data); // calls pms5003_get_data() -> *data = dev->last_data;
  ```
  `dev->last_data` is a 28-byte multi-field struct (`pms5003_data_t`). Priority 3 preempts Priority 2 on FreeRTOS without any mutex or critical section guarding `dev->last_data`.
- **Impact:** Torn reads where PM1.0, PM2.5, and PM10 values come from different measurement frames, potentially injecting invalid particulate spikes into the PM2.5 neural calibration engine.
- **Remediation:**
  Add a FreeRTOS mutex or `portENTER_CRITICAL()` / `portEXIT_CRITICAL()` inside `pms5003_parse_frame()` and `pms5003_get_data()`.

---

### 3. L-01 (BLOCKER): Zero SQI Masquerades as 95% Quality Signal
- **Files:** `firmware/core/clinical_vitals_engine.c:116, 128`, `firmware/shrikefi/main_shrikefi.c:106-118`
- **Mechanism:**
  In `clinical_vitals_engine.c`:
  ```c
  out->sqi = (sqi > 0.0f && isfinite(sqi)) ? sqi : 0.95f;
  ...
  if (sqi > 0.0f && sqi < 0.70f && !is_absolute_crisis) {
      // Hold triage and report sensor noise
      return;
  }
  ```
  In `main_shrikefi.c`:
  When fewer than 30 raw IR samples exist (`s_sqi_raw_n < 30`), `sqi` remains zeroed by `memset(&sqi, 0, sizeof(sqi))`, setting `g_state.ppg_sqi = 0.0f`.
- **Impact:**
  When `sqi == 0.0f`, `sqi > 0.0f` is false! Line 116 assigns `out->sqi = 0.95f` (reporting 95% hospital-grade quality), and line 128 bypasses the `< 0.70f` noise rejection check entirely! An uncalibrated or zero-quality sensor signal is triaged as near-perfect quality.
- **Remediation:**
  Differentiate between uncomputed SQI and valid SQI. If `sqi <= 0.0f`, report `out->sqi = 0.0f` and gate clinical vitals triage appropriately until the 30-sample window has populated.

---

### 4. D-01 (WARNING): GUI Dashboard Ignores Live `RR` & Hardcodes `SQI`
- **Files:** `firmware/shrikefi/shrikefi_dashboard.c:229-240, 255-265`
- **Mechanism:**
  `main_shrikefi.c` transmits:
  ```c
  printf("[TELEMETRY] HR=%.1f,SPO2=%.1f,RMSSD=%.1f,TEMP=%.1f,HUM=%.1f,PM25=%.1f,RR=%.1f,SQI=%.2f\n", ...);
  ```
  `shrikefi_dashboard.c` uses a 6-parameter `sscanf`:
  ```c
  if (sscanf(line_buf, "[TELEMETRY] HR=%f,SPO2=%f,RMSSD=%f,TEMP=%f,HUM=%f,PM25=%f",
             &r_hr, &r_spo2, &r_rmssd, &r_temp, &r_hum, &r_pm) == 6) {
      ...
      g_state.sqi = 0.95f; // HARDCODED!
  }
  ```
- **Impact:** In Live Hardware Mode, `g_state.derived_rr` is never updated from the UART stream, remaining at the simulation default (15.0 br/min). `g_state.sqi` is forcibly overwritten with 0.95. The live respiration and signal quality measurements computed on the ESP32 are discarded by the GUI.
- **Remediation:**
  Update `sscanf` in `SerialThreadProc` to check for `,RR=%f,SQI=%f` and update `g_state.derived_rr` and `g_state.sqi`.

---

### 5. L-02 (WARNING): State Retention of `RR` and `SQI` on Finger Removal
- **Files:** `firmware/shrikefi/main_shrikefi.c:791-808`
- **Mechanism:**
  When optical contact is lost (`IR <= 1500`):
  ```c
  g_state.heart_rate         = 0.0f;
  g_state.r_peak_interval_ms = 0.0f;
  g_state.hrv_rmssd          = 0.0f;
  g_state.spo2_valid         = 0;
  g_state.spo2_percent       = 0.0f;
  g_state.signal_status      = SIGNAL_STATUS_NO_FINGER;
  ```
  `g_state.respiratory_rate_bpm` and `g_state.ppg_sqi` are missing from this reset block.
- **Impact:** The previous subject's breathing rate and SQI remain stored in `g_state` and continue to be emitted over serial and OLED while the sensor is empty.
- **Remediation:**
  Add `g_state.respiratory_rate_bpm = 0.0f; g_state.ppg_sqi = 0.0f;` inside the `No optical contact` block.

---

### 6. L-03 (WARNING): Oscilloscope Drops Samples on Systolic Peaks
- **Files:** `firmware/shrikefi/main_shrikefi.c:591, 638, 760`
- **Mechanism:**
  ```c
  if (shrikefi_is_beat_detected()) {
      // Peak detected branch
  } else if (optical_contact) {
      // Normal sample processing branch
      printf("[PPG] %lu\n", (unsigned long)ppg_sample.ir);
  }
  ```
- **Impact:** On the exact sample where the systolic peak is confirmed by the FPGA, execution takes the `if` branch, skipping the `else if` branch where `printf("[PPG] ...")` lives. The oscilloscope stream drops the peak sample on every heartbeat.
- **Remediation:**
  Move the `printf("[PPG] %lu\n", ...)` call outside the conditional check so all optical samples are streamed uniformly.

---

### 7. C-02 (WARNING): Non-Reentrant Static Buffer in `clinical_fuse_triage`
- **Files:** `firmware/core/clinical_vitals_engine.c:237, 284`
- **Mechanism:**
  ```c
  static char s_fused_advisory_buf[512];
  ...
  fused->overall_advisory = s_fused_advisory_buf;
  ```
- **Impact:** Returns a pointer to a shared static buffer. If `clinical_fuse_triage()` is called from more than one task, concurrent writes corrupt the advisory string.
- **Remediation:**
  Embed the advisory buffer directly inside `risk_assessment_t` (e.g. `char overall_advisory[256]`) instead of storing a `const char*` pointing to static memory.

---

### 8. C-03 (WARNING): Unconditional `esp_mqtt_client_start` on IP Event
- **Files:** `firmware/shrikefi/wifi_mqtt_manager.c:273-276`
- **Mechanism:**
  ```c
  else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
      if (mqtt_client != NULL) {
          esp_mqtt_client_start(mqtt_client);
      }
  }
  ```
- **Impact:** `IP_EVENT_STA_GOT_IP` is dispatched on initial connection and on every subsequent DHCP renewal or reconnection. Calling `esp_mqtt_client_start()` on a client that has already started violates the ESP-IDF MQTT client lifecycle contract and returns `ESP_ERR_INVALID_STATE`.
- **Remediation:**
  Guard `esp_mqtt_client_start()` with a boolean flag indicating whether the client was already started.

---

### 9. H-01 (WARNING): FSM Downslope Peak Re-Triggering After Refractory Expiration
- **Files:** `hardware/common/ppg_peak_detector.v:128-140, 147-157`
- **Mechanism:**
  In commit `59f3bd9`, the requirement for the signal to drop below `dyn_threshold` before leaving `STATE_REFRACTORY` was removed. Once `refractory_cnt == 0`, the FSM transitions to `STATE_ARMED`.
  If the pulse is wide (e.g. bradycardia or high DC baseline where signal is still > 120 at 250 ms), `STATE_ARMED` immediately transitions to `STATE_RISING` on the next sample without verifying that `sample_in > prev_sample` (positive slope).
  `peak_val` is set to that sample, and as the waveform continues its downward decay, `(peak_val - sample_in) >= crest_fall_min` trips, triggering a false secondary peak.
- **Impact:** Generates phantom split-beat detections on wide pulses.
- **Remediation:**
  In `STATE_ARMED`, require both `sample_in >= dyn_threshold` and `sample_in > prev_sample` before transitioning to `STATE_RISING`.

---

### 10. B-01, B-02, B-03 (INFORMATIONAL)
- **B-01:** `forgefpga_bitstream.h:8-9` defines non-static `const uint8_t forgefpga_bitstream[]` in a header file. If included across multiple translation units, the linker will report duplicate symbol collisions. Fix: mark `static const` or declare `extern`.
- **B-02:** `firmware/shrikefi/main/Kconfig.projbuild` contains legacy options for I2C bitstream flashing that were superseded by SPI2. Fix: update Kconfig entries to reflect SPI2.
- **B-03:** `firmware/shrikefi/pms5003.c:98-100` resets `rx_pos = 0` if `byte != 0x4D` after `0x42`. If the byte is another `0x42`, the start of a valid frame is dropped. Fix: check `if (byte == PMS5003_START_BYTE_1) dev->rx_pos = 1;`.

---

## Action Plan & Recommendations

1. **Immediate Privacy Fix (T1.1):** Disable HiveMQ cloud publishing by default in `wifi_mqtt_manager.c` and gate behind `CONFIG_SHRIKEFI_CLOUD_PUBLISH`.
2. **Dashboard Synchronization:** Update `shrikefi_dashboard.c` to parse `RR` and `SQI` from telemetry lines and display live patient values.
3. **Firmware Hardening:**
   - Add a mutex around `pms5003_t.last_data`.
   - Clear `respiratory_rate_bpm` and `ppg_sqi` when optical contact is lost.
   - Guard `clinical_vitals_engine.c` against `sqi == 0.0f` false quality assumption.
   - Stream `[PPG]` samples unconditionally.
