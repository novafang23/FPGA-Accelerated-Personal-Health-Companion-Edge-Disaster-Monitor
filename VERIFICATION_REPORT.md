# VALOR Technical Guide Verification Report

**Target Document**: `VALOR_COMPLETE_GUIDE_1.md` (and `VALOR_COMPLETE_GUIDE.md`)  
**Repository**: `FPGA-Accelerated-Personal-Health-Companion-Edge-Disaster-Monitor` (SIH26181)  
**Date of Verification**: 2026-10-24  
**Scope**: Complete technical audit of system architecture, pinouts, SPI protocol, Verilog RTL, reset hierarchy, timing & synthesis reports, FreeRTOS dual-core firmware, TinyML risk inference, clinical rules, GNSS subsystem, dashboards, and hardware test records.

---

## 1. Executive Summary & Verdict Counts

Every technical claim in `VALOR_COMPLETE_GUIDE_1.md` was cross-referenced directly against hardware RTL (`hardware/shrikefi/`, `hardware/common/`, `hardware/zynq/`), firmware C source and headers (`firmware/shrikefi/`, `firmware/core/`), build configurations (`sdkconfig`, `sdkconfig.defaults`, `CMakeLists.txt`), constraints (`sdc`, `pcf`), and synthesis/bench logs (`synthesis_evidence/`, `docs/reports/`).

### Overall Verdict Summary

| Verdict | Definition | Count |
|:---|:---|:---:|
| **CONFIRMED** | Implementation code, hardware constraints, or official reports exactly match the claim. | **80** |
| **CONTRADICTED** | Implementation code or official reports directly conflict with the text in the guide. | **11** |
| **PARTIAL** | Core concept is accurate, but specific numeric parameters, standards, or details are erroneous. | **2** |
| **NOT FOUND** | Referenced data, external calibration, or artifact does not exist in the repository. | **1** |
| **UNMEASURED** | Claim refers to a real-time performance or timing figure with no instrumented trace in repo. | **2** |
| **TOTAL CLAIMS AUDITED** | Comprehensive technical assertions across Sections 1 through 13. | **96** |

> **Note on Mathematical Exactness**: The summary totals above are derived directly from the exact count of rows across the detail tables in Section 2. Every single claim row in Sections 1 through 13 maps to one of these verdicts.

---

## 2. Detailed Verification Tables by Category

### Section 1: Architecture and Pins

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| SPI FPGA Pin Map: PIN_16 (SCK), PIN_17 (SS_n), PIN_18 (MOSI), PIN_19 (MISO) | §4, §5 | **CONFIRMED** | `hardware/shrikefi/forgefpga_pins.pcf:10-14` | Verified against Renesas PCF; PIN_19_OE also bound to MISO pad. |
| SPI ESP32-S3 Pin Map: GPIO12 (SCK), GPIO10 (CS), GPIO11 (MOSI), GPIO13 (MISO) | §4, §5 | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:124-127` | Matches MCU SPI host hardware peripheral mapping. |
| SPI Mode 0, 8-bit frame, 1 MHz runtime clock speed | §4, §5 | **CONFIRMED** | `firmware/shrikefi/shrikefi_link_driver.c:96-97` | clock_speed_hz = 1000000 (1 MHz), mode = 0. |
| Blue User LED D12 on FPGA PIN_7 with 50 ms pulse length | §3, §4 | **CONFIRMED** | `hardware/shrikefi/forgefpga_pins.pcf:17-18; hardware/shrikefi/forgefpga_ppg_top.v:7, 209` | LED_PULSE_CYC = 2_500_000 cycles (50 ms at 50 MHz clock). |
| 50 MHz internal oscillator with clk_en driven High | §4, §5 | **CONFIRMED** | `hardware/shrikefi/forgefpga_ppg_top.v:12, 33` | assign clk_en = 1'b1 unconditionally powers internal oscillator. |
| 255-cycle power-on reset (por_cnt counting to 255) | §4, §5 | **CONFIRMED** | `hardware/shrikefi/forgefpga_ppg_top.v:57-64` | por_cnt counts 0 to 255 before deasserting rst_n. |
| I2C Addresses: MAX30102 0x57, BME280 0x76, SSD1306 0x3C, MPU6050 0x68 | §3, §6 | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:37-41; firmware/shrikefi/mpu6050.h:20, 22` | Verified on shared hardware I2C bus. |
| I2C Bus Clock: 100 kHz Standard Mode runtime speed | §3, §6 | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:34; firmware/shrikefi/main_shrikefi.c:2739` | Overrides HAL default 400 kHz to 100 kHz to prevent breadboard NACKs. |
| UART1 for PMS5003 at 9600 baud (GPIO14 RX, GPIO18 TX) | §6 | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:53-55; firmware/shrikefi/main_shrikefi.c:2876-2877` | Standard serial packet stream. |
| UART2 for GNSS at 9600 baud (GPIO16 RX, GPIO17 TX) | §6, §16 | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:60-61; firmware/shrikefi/main_shrikefi.c:2897-2898` | Quectel L89HS NMEA feed. |

---

### Section 2: SPI Link and Clock Domain

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| 2-stage synchronizers on sck, ss_n, mosi with 2'b01/2'b10 edge detection | §5 | **CONFIRMED** | `hardware/shrikefi/spi_target.v:27-29, 34-35, 43-47` | sck_r = (sck_sync == 2'b01); sck_f = (sck_sync == 2'b10). |
| Mode 0 sample-on-rising (sck_r), shift-on-falling (sck_f) | §5 | **CONFIRMED** | `hardware/shrikefi/spi_target.v:69-74, 86-93` | Samples MOSI on rising edge, drives MISO on falling edge. |
| 5-byte IBI burst sequence with magic byte 0xC3 | §5, §10 | **CONFIRMED** | `hardware/shrikefi/forgefpga_ppg_top.v:171-185; firmware/shrikefi/shrikefi_link_driver.c:265` | Sequencer emits 0xC3 followed by 4 big-endian bytes. |
| Dummy MOSI byte suppression during 5-byte IBI burst | §5, §10 | **CONFIRMED** | `hardware/shrikefi/forgefpga_ppg_top.v:86, 113` | sample_valid = rx_valid && (burst_cnt == 3'd0) blocks dummy bytes from filter. |
| SPI Target classified as a Mealy machine | §10 | **CONTRADICTED** | `hardware/shrikefi/spi_target.v:50-93` | spi_target is an 8-bit shift register with a 3-bit edge counter, not an FSM. |
| FPGA_EN and FPGA_PWR described as FPGA RTL ports | §3, §4 | **CONTRADICTED** | `firmware/shrikefi/shrikefi_pinmap.h:122-123; hardware/shrikefi/forgefpga_ppg_top.v:9-25` | Exist only as ESP32 GPIOs (GPIO8/9) for boot flashing; not in ForgeFPGA RTL. |

---

### Section 3: Moving-Average Filter (moving_average_8tap.v)

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| 8 taps, 11-bit running sum, divide-by-8 via 3-bit right shift | §8 | **CONFIRMED** | `hardware/common/moving_average_8tap.v:30-31, 55` | data_out <= next_sum[10:3] implements zero-cost truncation division. |
| Running sum mathematical formula: Sum_new = Sum_old + x[n] - x[n-7] | §8 (Line 622) | **CONTRADICTED** | `hardware/common/moving_average_8tap.v:34, 46-48` | shift_reg[7] holds x[n-8]. The 8-tap window formula is Sum_old + x[n] - x[n-8]. |
| Filter reset connection in ForgeFPGA top module | §8 | **CONFIRMED** | `hardware/shrikefi/forgefpga_ppg_top.v:60, 108` | ppg_filter connects to rst_n driven by por_cnt == POR_LAST. |

---

### Section 4: Peak Detector (ppg_peak_detector.v)

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| 2-bit binary state encoding with 4 states (ARMED, RISING, PEAK_FOUND, REFRACTORY) | §9 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:38-41` | STATE_ARMED=00, STATE_RISING=01, STATE_PEAK_FOUND=10, STATE_REFRACTORY=11. |
| Crest fall rule: drop of peak/4 when peak > 48, floor of 6 otherwise | §9 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:64-65` | (peak_val > 8'd48) ? (peak_val >> 2) : 8'd6. |
| Crest rule floor branch (: 8'd6) is unreachable dead code | §9, §23 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:137; hardware/shrikefi/forgefpga_ppg_top.v:119` | dyn_threshold tied to 120; arming requires sample_in >= 120, so peak_val > 48 is always true. |
| Refractory constant REFRACTORY_CYC = 12_500_000 (250 ms at 50 MHz) | §9 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:26` | 250 ms blanking window ignores dicrotic notch. |
| Simulation testbenches override REFRACTORY_CYC for speed | §9, §17 | **CONFIRMED** | `hardware/shrikefi/tb_forgefpga_system.v:53, 87; hardware/zynq/tb_ppg_system.v:32` | Overridden to 1800 cycles (ShrikeFi) and 500 cycles (Zynq). |
| Beat detected output is registered Moore | §9 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:91-122` | beat_detected asserted strictly in STATE_PEAK_FOUND inside clocked block. |
| First-beat suppression via first_beat_seen flag | §9 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:109-114` | Initial crest initializes interval counter without firing false beat pulse. |
| Detector can enter STATE_RISING and stay without timeout | §9, §20 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:142-149` | No watchdog or timeout exists in STATE_RISING. |
| FSM is '100% synchronous' (§9 Judge Q) | §9 (Line 792) | **PARTIAL** | `hardware/common/ppg_peak_detector.v:68` | State transitions are clock-synchronous, but reset is asynchronous (always @(posedge clk or negedge rst_n)). |

---

### Section 5: Reset (forgefpga_ppg_top.v)

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| Reset rst_n generated as combinational decode por_cnt == POR_LAST | §4, §5 | **CONFIRMED** | `hardware/shrikefi/forgefpga_ppg_top.v:60` | wire rst_n = (por_cnt == POR_LAST); |
| All asynchronous reset blocks across RTL use negedge rst_n | §5 | **CONFIRMED** | `hardware/common/moving_average_8tap.v:36; hardware/common/ppg_peak_detector.v:68; hardware/shrikefi/forgefpga_ppg_top.v:152, 204; hardware/shrikefi/spi_target.v:38, 50` | 6 sequential always blocks across the synthesis hierarchy use negedge rst_n. |

---

### Section 6: Synthesis and Timing Numbers

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| ForgeFPGA Resources: 515 / 1120 LUT5s (45.98%), 303 FFs, 105 / 140 CLBs | §4, §11 | **CONFIRMED** | `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log:7, 12, 16` | Matches official Renesas fitter report. |
| ForgeFPGA Dedicated Blocks: 0 / 8 BRAM, 0 / 1 PLL, 1 / 1 OSC, 5 / 19 GPIOs | §4, §11 | **CONFIRMED** | `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log:21-25` | Zero vendor hard blocks inferred. |
| ForgeFPGA Timing: WNS +7.316 ns, TNS 0 ps, Fmax 78.85 MHz at 50 MHz | §4, §11 | **CONFIRMED** | `hardware/shrikefi/synthesis_evidence/timing_summary_spi_link_50mhz.log:3, 7, 11; hardware/shrikefi/timing_constraints.sdc:2` | Target period 20.000 ns; timing closed cleanly. |
| Zynq Baseline Out-of-Context: 185 LUT, 16 LUTRAM, 266 FF, Fmax 69.45 MHz | §1, §4 | **CONFIRMED** | `AGENTS.md:8-12; docs/MIGRATION.md:14-18` | xc7z020clg400-1 out-of-context synthesis baseline. |
| Zynq Post-Route WNS: +12.836 ns (1806 endpoints) vs +13.168 ns (1826 endpoints) | §4, §23 | **CONFIRMED** | `hardware/zynq/synthesis_evidence/post_route_timing_summary.rpt:16; hardware/zynq/timing_impl.rpt:216` | Both exist: post-route block designs with PS7 at different design iterations. |
| Bitstream SHA-256 starts with 6930ba88... (truncated prefix) | §11 (Line 897, 1241) | **CONFIRMED** | `firmware/shrikefi/forgefpga_bitstream.h:4` | Guide cites '6930ba88...'; actual committed hash is 6930ba887b2d881c57cac6dc5c21836d5fa9e82d75317dff925040f1b9769629. |
| ForgeFPGA LUT Progression: 342 -> 363 -> 362 -> 535 -> 515 | §11 | **CONFIRMED** | `Git commits fe72433, e009439, 94442cd; docs/MIGRATION.md` | Captures architectural steps from parallel link to IBI burst and crest/4 filter. |

---

### Section 7: Firmware Architecture & Concurrency

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| FreeRTOS Tick Rate is 200 Hz (5 ms tick period) | §12 | **CONFIRMED** | `firmware/shrikefi/sdkconfig.defaults:58; firmware/shrikefi/sdkconfig:1512` | CONFIG_FREERTOS_HZ=200. |
| Wi-Fi task pinned to Core 1 | §12 | **CONFIRMED** | `firmware/shrikefi/sdkconfig.defaults:42; firmware/shrikefi/sdkconfig:1394` | CONFIG_ESP_WIFI_TASK_PINNED_TO_CORE_1=y. |
| Task Allocation: 5 tasks with prioritized core pinning and stacks | §12 | **CONFIRMED** | `firmware/shrikefi/main_shrikefi.c:2966-2970` | PPG Accel (Pri 5, Core 0), Risk Monitor (Pri 2, Core 1), PMS5003 (Pri 3, Core 1), GPS (Pri 3, Core 1), IMU (Pri 3, Core 1). |
| Inter-core Mutex: s_data_mutex with 10-20 ms acquisition timeout | §12 | **CONFIRMED** | `firmware/shrikefi/main_shrikefi.c:75, 437, 1068, 2734` | Guards vital sign telemetry between cores. |
| SOS Confirmation Window defaults: 3 seconds for vitals, 10 seconds for falls | §10 (Line 807, 815) | **CONTRADICTED** | `firmware/shrikefi/sos.c:41, 46` | critical_confirm_ms = 15000u (15 seconds, not 3 seconds); fall_confirm_ms = 10000u. |
| SOS recovery transition: SOS_ACTIVE -> SOS_IDLE after 30s | §10 (Line 819) | **CONTRADICTED** | `firmware/shrikefi/sos.c:180-189` | Transitions to SOS_CANCELLED first (with cleared_by_recovery flag); returns to SOS_IDLE only when condition clears. |
| Fall Detection FSM: 5 states, 0.60g free-fall, 2.50g impact, 80 dps gyro, 1.5s stillness | §10 | **CONFIRMED** | `firmware/shrikefi/fall_detect.h:27-31; firmware/shrikefi/fall_detect.c:21-29` | Full 5-state biomechanical impact classifier. |
| Skin temperature model formula: T_skin = b0 + b1*(T_die_pred - T_die_base) + b2*(T_amb - 27.5) | §15 | **CONFIRMED** | `firmware/core/skin_estimate.h:6, 23-25; firmware/core/skin_estimate.c:126-127` | b0=34.50, b1=1.05, b2=0.15 nominal parameters. |
| Skin temperature algorithm outputs 'accurate resting skin temperature' | §15 (Line 1188) | **CONTRADICTED** | `firmware/core/skin_estimate.c:7-14` | Uncalibrated bench estimate using fixed constant b0; no clinical gold-standard calibration was performed. |
| Q14 claim: 'we deliberately deleted the read from max30102.c' | §19 (Line 1418) | **CONTRADICTED** | `firmware/shrikefi/max30102.c:302; firmware/shrikefi/main_shrikefi.c:1012, 1749` | max30102_read_die_temp() is implemented and actively polled every second to feed skin estimator. |
| Q26 claim: 'boosting LED drive current up to 50 mA' | §19 (Line 1472) | **CONTRADICTED** | `firmware/shrikefi/max30102.c:354` | Driver register cap is 0x5A = 18.0 mA, not 50 mA. Current is statically set to 11.0 mA. |
| MAX30102 INT_ENABLE_1 writes 0x40 (PPG_RDY_EN), not 0x80 | §7, §15 | **CONFIRMED** | `firmware/shrikefi/max30102.c:208` | Enables optical sample ready interrupt. |
| MAX30102 Red channel read but never transmitted to FPGA | §5, §7 | **CONFIRMED** | `firmware/shrikefi/shrikefi_link_driver.c:125-132` | Red sample cached in MCU memory; only IR transmitted over SPI. |
| I2C HAL command timeout is 50 ms | §6, §7 | **CONFIRMED** | `firmware/shrikefi/esp32_i2c_hal.c:193` | i2c_master_cmd_begin timeout 50 / portTICK_PERIOD_MS. |
| MISO byte 7-bit sample is diagnostic only, not clinical waveform | §5 | **CONFIRMED** | `hardware/shrikefi/forgefpga_ppg_top.v:189-191; firmware/shrikefi/shrikefi_link_driver.c:287` | IBI interval is clinical source; MISO byte [6:0] is diagnostic modulo-128. |
| Section 10 states 'four other distinct state machines' but lists 5 | §10 (Line 799) | **CONTRADICTED** | `VALOR_COMPLETE_GUIDE.md:799-858` | Text claims 4 state machines, but enumerates 5 subsections (and SPI target is a counter/shifter). |

---

### Section 8: Neural Network (TinyML)

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| Header include in nn_risk_model_int8.c is nn_risk_model_int8.c.inc | §13 | **CONFIRMED** | `firmware/core/nn_risk_model_int8.c:12` | #include "nn_risk_model_int8.c.inc". |
| Architecture: 6 inputs -> 24 hidden 1 -> 16 hidden 2 -> 3 outputs | §13 | **CONFIRMED** | `firmware/core/nn_risk_model_int8.c.inc:2` | Inputs: HR, RMSSD, SpO2, Temp, Humidity, PM2.5. |
| Symmetric INT8 weights (zero-point 0), asymmetric uint8 activations | §13 | **CONFIRMED** | `firmware/core/nn_risk_model_int8.c:4-5, 30-34; firmware/core/nn_risk_model_int8.c.inc:73-81` | Dequantized float computation on ESP32-S3 FPU. |
| Total parameter count: 619 bytes | §13 | **CONFIRMED** | `firmware/core/nn_risk_model_int8.c.inc:9-60` | W1(144)+b1(24)+W2(384)+b2(16)+W3(48)+b3(3) = 619 bytes. |
| Weight Sparsity: 391 zero weights out of 619 (63.17%) | §13 | **CONFIRMED** | `firmware/core/nn_risk_model_int8.c.inc:9-60` | Verified via script: exactly 391 zeros out of 619 total weights/biases. |
| Dead Neurons: 11 dead in Hidden Layer 1, 6 dead in Hidden Layer 2 | §13 | **CONFIRMED** | `firmware/core/nn_risk_model_int8.c.inc:9-53` | 11 all-zero rows in W1, 6 all-zero rows in W2. |
| Validation Accuracy: FP32 88.47%, Deployed INT8 88.39%, Agreement 97.32% | §13 | **CONFIRMED** | `firmware/core/train_nn_risk_model.py:4, 877-890` | Reported in training script and model inc header. |
| Deployed INT8 Mean Absolute Error: MAE = 0.00790 | §13 | **CONFIRMED** | `firmware/core/train_nn_risk_model.py:888` | Evaluated against validation split. |
| Deployed INT8 Max Absolute Error: 0.09136 (~0.0914) | §13 | **CONFIRMED** | `firmware/core/train_nn_risk_model.py:889` | int8_max_err = 0.09136. |
| Deployed INT8 Max Absolute Error is 0.018 | §17, §19 (Line 1438) | **CONTRADICTED** | `firmware/core/train_nn_risk_model.py:889` | Script reports 0.09136. The 0.018 figure in §17/§19 is contradicted. |
| Forward pass executes in '< 1 microsecond' / 'single-digit microseconds' | §13 (Line 996, 1063, 1079, 1083) | **UNMEASURED** | `firmware/core/train_nn_risk_model.py:888-895; VALOR_COMPLETE_GUIDE.md:1633` | 576 float MACs with software expf cannot run in <1 us on 240 MHz MCU. Section 23 admits ESP32 latency is unmeasured. |
| Model trained on synthetic physiological dataset, not clinical ICU records | §13, §20 | **CONFIRMED** | `firmware/core/train_nn_risk_model.py:155-169` | make_dataset(N_UNIFORM) synthesizes training vitals based on physiological rules. |

---

### Section 9: Clinical & Disaster Rules

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| Royal College of Physicians mNEWS2 scoring in clinical_vitals_engine.c | §14 | **CONFIRMED** | `firmware/core/clinical_vitals_engine.c:7-80` | Implements standard UK RCP NEWS2 scoring matrix. |
| 34 unit tests for disaster risk rules in test_disaster_risk_engine.c | §14, §17 | **CONFIRMED** | `firmware/core/test_disaster_risk_engine.c` | Boundary sweep unit tests across vital and environmental tiers. |
| Heat index: Rothfusz regression clamped at 58.0 °C | §14 | **CONFIRMED** | `firmware/core/disaster_risk_engine.c:1071-1083` | Atmospheric heat index calculation with physiological core drift. |
| Heat index hazard thresholds: 35 °C, 40 °C, 45 °C, 54 °C | §14 | **CONFIRMED** | `docs/DEMO_RUNBOOK.md:144-152; firmware/core/disaster_risk_engine.c:1082` | Caution (35), Moderate (40), High (45), Critical (54). |
| Pressure fall rate: 0.5, 1.0, 2.0 hPa/hr across 180-min rolling window | §14 | **CONFIRMED** | `firmware/core/pressure_trend.c:14-22, 78` | Least-squares linear regression barograph. |
| PM2.5 threshold 35 µg/m³ labeled as 'WHO 24-hour limit exceedance' | §14 (Line 1126) | **PARTIAL** | `firmware/core/disaster_risk_engine.c:1108-1122; WHO 2021 Guidelines` | 35 µg/m³ is the US EPA NAAQS standard used in code; WHO 2021 24-hour guideline is actually 15 µg/m³. |
| Hard clinical overrides: SpO2 <= 88%, Heart Rate >= 150 BPM | §14 | **CONFIRMED** | `firmware/core/disaster_risk_engine.c:286-302` | Forces Critical triage tier regardless of neural network score. |

---

### Section 10: GNSS Subsystem

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| Quectel L89HS driver in gps_l89hs.c, NMEA parser in gnss_nmea.c | §16 | **CONFIRMED** | `firmware/shrikefi/gps_l89hs.c; firmware/shrikefi/gnss_nmea.c` | Implements NMEA-0183 RMC, GGA, VTG parsers. |
| 12 unit tests in test_gps_l89hs.c | §16, §17 | **CONFIRMED** | `firmware/shrikefi/test_gps_l89hs.c:1-350` | Exactly 12 test functions covering checksums, coordinates, and packet recovery. |
| Physical Quectel L89HS wired on GPIO 16 (RX) and GPIO 17 (TX) | §16 | **CONFIRMED** | `firmware/shrikefi/shrikefi_pinmap.h:60-61; docs/images/hardware/gps.jpeg` | Connected to ESP32 UART2. |
| UAT report confirms physical bench test on COM4 | §16 | **CONFIRMED** | `docs/reports/UAT_L89HS_GNSS.md:1-25` | Flashed on COM4; verified open-circuit pull-up resilience. |

---

### Section 11: Verification, Tests & Evidence

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| ForgeFPGA testbench: 13 assertions / self-checking tests passing | §17 | **CONFIRMED** | `hardware/shrikefi/tb_forgefpga_system.v:184-188` | Simulation testbench reports 13 passed, 0 failed. |
| Zynq testbench: 6 self-checking test cases passing | §17 | **CONFIRMED** | `hardware/zynq/tb_ppg_system.v:230-245; AGENTS.md:13` | Passes 6/6 tests covering reset, thresholds, dicrotic notch. |
| Fall detection unit tests: 16 FSM unit tests | §17 | **CONFIRMED** | `firmware/shrikefi/test_fall_detect.c; docs/DEMO_RUNBOOK.md:210` | Validates free-fall, impact, rotational jerk, immobility transitions. |
| I2C Bus bench test executed on 21 October 2026 | §15, §17 | **CONFIRMED** | `docs/I2C_BUS_BENCH_REPORT.md:8` | Measured on 21 October 2026; 4 of 5 devices answer. |
| TMP117 skin probe failed hardware bench test (silent / faulty) | §15 | **CONFIRMED** | `docs/I2C_BUS_BENCH_REPORT.md:12-20` | Returns 0xFFFF across all addresses; bus healthy at 3.18V without it. |
| Physical drop trial passed on 2026-10-19 (1/1 trial) | §10, §17 | **CONFIRMED** | `docs/DEMO_RUNBOOK.md:210; docs/JUDGE_QA.md:268` | 1/1 physical drop trial passed; exercised countdown and cancellation. |
| Multi-trial physical fall orientation statistical dataset | §17, §20 | **NOT FOUND** | `docs/JUDGE_QA.md:268` | Honest limitation: single bench trial only; multi-trial orientation dataset does not exist. |
| MIMIC-III ICU Evaluation: 16,387 records across 98 patients | §13, §17 | **CONFIRMED** | `data/mimic/mimic_eval_feed.csv:1-16388` | Empirically verified: exactly 16,387 data rows across 98 unique subject_ids. |
| MIMIC-III Confusion Matrix: TN 14,451, FP 267, TP 971, FN 698 | §13, §17 | **CONFIRMED** | `data/mimic/mimic_eval_feed.csv; docs/reports/AI_MODEL_AUDIT_2026-10-20.md` | Sum equals exactly 16,387 records. |
| MIMIC-III Metrics: Sensitivity 58.18%, Specificity 98.19%, Accuracy 94.11% | §13, §17 | **CONFIRMED** | `Recomputed mathematically from confusion matrix` | Sens: 971/1669 = 58.18%; Spec: 14451/14718 = 98.19%; Acc: 15422/16387 = 94.11%. |

---

### Section 12: Demonstration, Local SoftAP & Web Dashboard

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| Wi-Fi SoftAP SSID format VALOR-%02X%02X (e.g. VALOR-0081) | §2, §12 | **CONFIRMED** | `firmware/shrikefi/web_status.c:1113` | Uses last two octets of MAC address. |
| Default Gateway IP 192.168.4.1 with /api/status endpoint | §2, §12 | **CONFIRMED** | `firmware/shrikefi/web_status.c:172, 1160` | Local web server serves JSON and dashboard. |
| Web dashboard valor_dashboard.html embedded in firmware flash | §3, §12 | **CONFIRMED** | `firmware/shrikefi/main/CMakeLists.txt:11-13` | EMBED_FILES ../valor_dashboard.html; zero CDN dependencies. |
| Dashboard simulation profiles: Heat Wave, Smog Crisis, Flash Flood | §18 | **CONFIRMED** | `firmware/shrikefi/valor_dashboard.html:450-510; docs/DEMO_RUNBOOK.md:222-224` | Offline JavaScript hazard simulator. |
| Web Serial alternative launcher launch_dashboard.bat | §18 | **CONFIRMED** | `launch_dashboard.bat:1-15` | Launches local browser serial bridge. |
| Oscilloscope waveform rendering at 60 FPS | §18 | **UNMEASURED** | `firmware/shrikefi/valor_dashboard.html:120-140` | Uses requestAnimationFrame on HTML5 canvas; real frame rate depends on host GPU. |

---

### Section 13: Guide Internal Consistency

| Claim | Guide Section | Verdict | Evidence (file:line) | Note |
|:---|:---:|:---:|:---|:---|
| Zynq Post-Route Timing WNS (+12.836 ns in §4 vs +13.168 ns in §23) | §4, §23 | **CONFIRMED** | `hardware/zynq/synthesis_evidence/post_route_timing_summary.rpt:16; hardware/zynq/timing_impl.rpt:216` | Both runs exist in repo at different block design revisions. |
| INT8 Deployed Max Error (0.0914 in §13 vs 0.018 in §17/§19) | §13, §17, §19 | **CONTRADICTED** | `firmware/core/train_nn_risk_model.py:889` | 0.09136 is the true error. The 0.018 figure in §17/§19 is contradicted by the evaluation script. |
| Crest Fall Floor Reachability (Proportional drop vs dead-code floor of 6) | §9, §23 | **CONFIRMED** | `hardware/common/ppg_peak_detector.v:137` | dyn_threshold tied to 120 guarantees peak_val >= 120 > 48, so floor of 6 is dead code. |
| TMP117 Bench Test Date (21 Oct 2026 vs questioned 22 Oct) | §15 | **CONFIRMED** | `docs/I2C_BUS_BENCH_REPORT.md:8` | Report confirms 21 October 2026. |

---

## 3. "Must Fix" Correction List (Literal Quotes from Current Guide)

The following table documents every claim in `VALOR_COMPLETE_GUIDE_1.md` that is contradicted or partially contradicted by the codebase. Each row includes the **exact line number and literal text** from the current guide alongside the **exact code reality**.

| # | Topic | Exact Guide Line & Text (`VALOR_COMPLETE_GUIDE_1.md`) | Exact Code Reality & Supported Text | Code Evidence |
|:---:|:---|:---|:---|:---|
| 1 | **Filter Running Sum Formula** | Line 622:<br>`$$\text{Sum}_{\text{new}} = \text{Sum}_{\text{old}} + x[n] - x[n-7]$$` | The moving-average running sum subtracts sample $x[n-8]$ (`shift_reg[7]`), computing:<br>`$$\text{Sum}_{\text{new}} = \text{Sum}_{\text{old}} + x[n] - x[n-8]$$`<br>Subtracting $x[n-7]$ would create a 7-tap window instead of an 8-tap window. | `hardware/common/moving_average_8tap.v:34, 46-48` |
| 2 | **TinyML Forward Pass Latency** | Line 996:<br>`An on-device INT8 forward pass executes in less than **1 microsecond**...`<br><br>Line 1063:<br>`...runs in single-digit microseconds on the 240 MHz ESP32-S3 FPU.`<br><br>Line 1079 & 1083:<br>`...running in single-digit microseconds...` | The 6 $\to$ 24 $\to$ 16 $\to$ 3 MLP requires 576 floating-point multiply-accumulates plus 3 software runtime `expf()` library calls on the Xtensa LX7 MCU. Sub-microsecond execution is impossible; Section 23 line 1633 confirms ESP32 inference time is unmeasured on physical silicon (expected tens of microseconds). | `firmware/core/nn_risk_model_int8.c:84-170`<br>`train_nn_risk_model.py:888-895`<br>`VALOR_COMPLETE_GUIDE.md:1633` |
| 3 | **PM2.5 Caution Threshold & WHO Limit** | Line 1126:<br>`- **Caution:** $\text{PM2.5} \ge 35\ \mu\text{g/m}^3$ (WHO 24-hour limit exceedance) OR $\text{SpO}_2 \le 96\%$.` | 35 µg/m³ is the **US EPA 24-hour standard** (NAAQS). The WHO 2021 Global Air Quality Guidelines recommend an ambient 24-hour ceiling of **15 µg/m³** (and an annual mean of 5 µg/m³). The guide should label 35 µg/m³ as the US EPA standard. | `firmware/core/disaster_risk_engine.c:1108-1122`<br>WHO 2021 Air Quality Guidelines |
| 4 | **SOS Confirmation Window Default** | Line 807:<br>`...defaults: 3 seconds for vitals, 10 seconds for falls...`<br><br>Line 815:<br>`SOS_IDLE --> SOS_ARMED: Critical Vitals (>=3s) OR Fall Event` | In `sos.c`, `critical_confirm_ms` was increased from 3s to **15 seconds** (`critical_confirm_ms = 15000u`) to reject transient sensor motion artifacts. The defaults are 15s for vitals and 10s for falls. | `firmware/shrikefi/sos.c:41, 46` |
| 5 | **SOS Stand-Down Recovery Transition** | Line 819:<br>`SOS_ACTIVE --> SOS_IDLE: 30s Sustained Recovery Stand-Down` | When the 30-second physiological recovery timer expires, `sos_update()` sets `s_state = SOS_CANCELLED;` (with flag `s_cleared_by_recovery = true`), NOT `SOS_IDLE`. It returns to `SOS_IDLE` only after underlying conditions clear. | `firmware/shrikefi/sos.c:180-189` |
| 6 | **Skin Temperature Clinical Accuracy** | Line 1188:<br>`This outputs an accurate resting skin temperature ($35.7^\circ\text{C}$)...` | The skin estimator is an uncalibrated bench estimate. It uses a fixed nominal parameter $b_0 = 34.50^\circ\text{C}$ with no clinical gold-standard calibration against an FDA-cleared reference thermometer. Stating 'accurate resting skin temperature' is medically inaccurate. | `firmware/core/skin_estimate.c:7-14`<br>`firmware/core/skin_estimate.h:23` |
| 7 | **MAX30102 Temperature Sensor Deletion Claim** | Line 1418 (Q14):<br>`- *Don't Say:* "...we deliberately deleted the read from max30102.c due to LED self-heating".` | `max30102_read_die_temp()` was **never deleted**. It is fully implemented in `max30102.c:302` and is actively called every second in `main_shrikefi.c:1012, 1749` to supply raw die temperature to `skin_estimate_update()`. | `firmware/shrikefi/max30102.c:302`<br>`firmware/shrikefi/main_shrikefi.c:1012, 1749` |
| 8 | **MAX30102 LED Current Scaling Cap** | Line 1472 (Q26):<br>`...The MAX30102 driver implements automatic current scaling, boosting LED drive current up to 50 mA to maintain perfusion index.` | In `max30102.c`, the driver's register clamp is `0x5A`, which represents **18.0 mA** (`#define MAX30102_LED_CURRENT_MAX_REG 0x5A /* 18.0 mA */`), not 50 mA. Furthermore, `max30102_adjust_led_current()` is never called in runtime firmware; the current is statically initialized to 11.0 mA. | `firmware/shrikefi/max30102.c:354`<br>`firmware/shrikefi/main_shrikefi.c` |
| 9 | **INT8 Maximum Absolute Error** | Line 1438 (Q18) & Line 1243:<br>`...with only 0.018 mean absolute error vs. the FP32 baseline.` | `train_nn_risk_model.py` evaluates the deployed INT8 model against the float32 reference across 7,000 validation samples, reporting a Mean Absolute Error (MAE) of **0.00790** and a Maximum Absolute Error of **0.09136** (~0.0914). The 0.018 figure is contradicted by the evaluation script. | `firmware/core/train_nn_risk_model.py:888-889` |
| 10 | **FSM Reset Synchronicity** | Line 792 (Section 9 Q):<br>`**A:** Yes, 100% synchronous. All state transitions, running crest tracking, and interval counter updates occur strictly on the rising edge of the 50 MHz system clock...` | While state updates and counters are synchronous to `posedge clk`, the reset path in `ppg_peak_detector.v:68` uses `always @(posedge clk or negedge rst_n)`, which is an **asynchronous active-low reset**. The FSM is not 100% synchronous. | `hardware/common/ppg_peak_detector.v:68` |
| 11 | **Count of State Machines in Section 10** | Line 799 (Section 10 Header):<br>`In addition to the systolic peak detector, VALOR contains four other distinct state machines in RTL and firmware:` | Section 10 lists **five** subsections: 1. SOS FSM, 2. Fall detection FSM, 3. SPI Target bit shifter, 4. IBI Burst transmission sequencer, 5. AXI4-Lite handshake FSM. Furthermore, the SPI target bit shifter is a counter/shifter, not an FSM. | `VALOR_COMPLETE_GUIDE.md:799-858` |
| 12 | **SPI Target Classification as FSM** | Line 825, 836, 852 (Section 10 & Q&A):<br>Describing the SPI target as a state machine / Mealy machine. | In `spi_target.v:50-93`, the SPI target is implemented as a pure hardware shift register with a 3-bit bit counter (`bit_cnt`). It does not contain state registers (`state`, `next_state`) and is not an FSM. | `hardware/shrikefi/spi_target.v:50-93` |
| 13 | **FPGA Hardware Enable / Power Pins** | Section 3 & 4:<br>Describing `FPGA_EN` and `FPGA_PWR` as RTL ports controlling power and reset. | `PIN_FPGA_PWR` (GPIO9) and `PIN_FPGA_EN` (GPIO8) are board-level ESP32-S3 GPIO pins used to power-gate and hold the Renesas ForgeFPGA in reset during boot flashing. They do not exist as ports or signals in the ForgeFPGA Verilog RTL. | `firmware/shrikefi/shrikefi_pinmap.h:122-123`<br>`hardware/shrikefi/forgefpga_ppg_top.v:9-25` |

---

## 4. "Not in Repo" List

The following claims in the guide describe tests, data, or files that do **not** exist in the repository:

1. **Multi-Trial Physical Fall Orientation Statistical Dataset**:
   - *Claim*: Multi-orientation physical fall validation table.
   - *Reality*: The repository documents exactly **one** physical drop trial conducted on 2026-10-19 (`docs/DEMO_RUNBOOK.md:210`, `docs/JUDGE_QA.md:268`). While 16 unit tests exist in software simulation (`test_fall_detect.c`), no physical multi-trial drop dataset exists on disk.
2. **Clinical Reference Thermometer Calibration Dataset**:
   - *Claim*: Comparative clinical calibration curve for skin temperature estimation.
   - *Reality*: The skin temperature algorithm is explicitly documented as an uncalibrated bench estimate using nominal coefficients ($b_0=34.50$, $b_1=1.05$, $b_2=0.15$). No empirical comparative dataset against an FDA-cleared clinical contact thermometer exists in the repo.

---

## 5. Repository Files Inspected & Read

### Hardware RTL & Constraints
- `hardware/shrikefi/forgefpga_pins.pcf` (IO pin constraints for SLG47910)
- `hardware/shrikefi/forgefpga_ppg_top.v` (ForgeFPGA top-level integration & burst sequencer)
- `hardware/shrikefi/spi_target.v` (SPI Mode 0 target with 2-stage synchronizers)
- `hardware/common/moving_average_8tap.v` (8-tap running sum filter)
- `hardware/common/ppg_peak_detector.v` (4-state peak detector & refractory FSM)
- `hardware/shrikefi/timing_constraints.sdc` (50 MHz SDC timing constraint)
- `hardware/shrikefi/tb_forgefpga_system.v` (ShrikeFi self-checking testbench, 13 assertions)
- `hardware/zynq/tb_ppg_system.v` (Zynq self-checking testbench, 6 tests)
- `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log` (Official fitter report)
- `hardware/shrikefi/synthesis_evidence/timing_summary_spi_link_50mhz.log` (Official timing report)
- `hardware/zynq/synthesis_evidence/post_route_timing_summary.rpt` (Zynq post-route timing report)
- `hardware/zynq/timing_impl.rpt` (Zynq implementation timing report)

### Firmware Source & Headers
- `firmware/shrikefi/shrikefi_pinmap.h` (ESP32-S3 GPIO and I2C address definitions)
- `firmware/shrikefi/shrikefi_link_driver.c` (SPI host driver and IBI burst parser)
- `firmware/shrikefi/main_shrikefi.c` (FreeRTOS dual-core task scheduler and telemetry loop)
- `firmware/shrikefi/esp32_i2c_hal.c` & `.h` (ESP32 I2C hardware abstraction layer)
- `firmware/shrikefi/max30102.c` & `.h` (MAX30102 PPG sensor driver)
- `firmware/shrikefi/bme280.h` (BME280 barometric & environmental driver header)
- `firmware/shrikefi/mpu6050.h` (MPU6050 6-axis IMU driver header)
- `firmware/shrikefi/sos.c` & `sos.h` (Emergency assistance 4-state latching FSM)
- `firmware/shrikefi/fall_detect.c` & `fall_detect.h` (5-state biomechanical fall detection FSM)
- `firmware/core/skin_estimate.c` & `skin_estimate.h` (Newton lead skin temperature estimator)
- `firmware/core/clinical_vitals_engine.c` & `clinical_vitals_engine.h` (mNEWS2 clinical scoring)
- `firmware/core/disaster_risk_engine.c` & `disaster_risk_engine.h` (Multi-hazard triage engine)
- `firmware/core/pressure_trend.c` & `pressure_trend.h` (180-min barometric slope calculation)
- `firmware/shrikefi/gps_l89hs.c` & `gps_l89hs.h` (Quectel L89HS GNSS driver)
- `firmware/shrikefi/gnss_nmea.c` & `gnss_nmea.h` (NMEA-0183 sentence parser)
- `firmware/shrikefi/test_gps_l89hs.c` (12 unit tests for GNSS parser)
- `firmware/core/nn_risk_model_int8.c` & `.h` (Quantized neural network inference kernel)
- `firmware/core/nn_risk_model_int8.c.inc` (Shipped INT8 model weights and quantization parameters)
- `firmware/core/train_nn_risk_model.py` (Model training and quantization pipeline script)
- `firmware/shrikefi/forgefpga_bitstream.h` (Embedded binary bitstream array)
- `firmware/shrikefi/web_status.c` (SoftAP HTTP server and JSON endpoint)
- `firmware/shrikefi/valor_dashboard.html` (Embedded offline single-page web dashboard)
- `firmware/shrikefi/sdkconfig` & `sdkconfig.defaults` (ESP-IDF FreeRTOS & Wi-Fi configuration)
- `firmware/shrikefi/main/CMakeLists.txt` (Build system file embedding web dashboard)

### Evaluation Feeds & Documentation Reports
- `data/mimic/mimic_eval_feed.csv` (16,387 clinical ICU records from MIMIC-III database across exactly 98 unique patients)
- `firmware/core/mimic_harness.c` (Validation test harness for MIMIC feed)
- `docs/I2C_BUS_BENCH_REPORT.md` (Physical electrical bench test of shared I2C bus, dated 21 October 2026)
- `docs/reports/UAT_L89HS_GNSS.md` (Hardware UAT report of Quectel L89HS GNSS on COM4)
- `docs/DEMO_RUNBOOK.md` (Live hardware judging demonstration runbook)
- `docs/JUDGE_QA.md` (Technical question & answer bank)
- `docs/MIGRATION.md` (Architecture migration history from Zynq to ShrikeFi)
- `docs/HARDWARE_DEBUG_HANDOVER.md` (Hardware debugging and bench trial log)
- `AGENTS.md` (Hardware & firmware engineering guidelines and baseline invariants)
