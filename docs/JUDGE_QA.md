# Project VALOR: Competition Judge Q&A Defense Manual

**Project Title:** SIH26181 — VALOR (Vital and Atmospheric Logic for Offline Rescue)  
**Role:** Master Defense Playbook for Technical Evaluators, Jury Panels, and Competition Judges  
**Rule of Truth:** Every answer is grounded directly in repository source code, physical synthesis evidence, and automated test outputs.

> **New here?** Read the compact, plain-language [Judge Study Notes](JUDGE_STUDY_NOTES.md) first. It covers the hardware, buses, code layers, data flow, algorithms, demo script and the safe answers to the most likely questions.

---

## Table of Contents
1. [Category 1: Problem Statement & Real-World Need](#category-1-problem-statement--real-world-need)
2. [Category 2: Product Overview & Differentiating Features](#category-2-product-overview--differentiating-features)
3. [Category 3: System Architecture](#category-3-system-architecture)
4. [Category 4: Hardware & FPGA Design](#category-4-hardware--fpga-design)
5. [Category 5: RTL, Clocks, Resets, Counters & State Machines](#category-5-rtl-clocks-resets-counters--state-machines)
6. [Category 6: Protocols, Busses & Interfaces](#category-6-protocols-busses--interfaces)
7. [Category 7: AI/ML Methodology & Zero-Cloud Execution](#category-7-aiml-methodology--zero-cloud-execution)
8. [Category 8: Hardware-Software Integration](#category-8-hardware-software-integration)
9. [Category 9: Testing, Verification & Regression Results](#category-9-testing-verification--regression-results)
10. [Category 10: Performance, Latency, Accuracy, Power & Utilization](#category-10-performance-latency-accuracy-power--utilization)
11. [Category 11: Security, Privacy, Reliability & Failure Recovery](#category-11-security-privacy-reliability--failure-recovery)
12. [Category 12: Cost, Deployment, Scalability & Maintainability](#category-12-cost-deployment-scalability--maintainability)
13. [Category 13: Alternatives & Engineering Trade-Offs](#category-13-alternatives--engineering-trade-offs)
14. [Category 14: Candid Limitations, Unfinished Features & Future Work](#category-14-candid-limitations-unfinished-features--future-work)
15. [Category 15: Adversarial Questions & Innovation Verification](#category-15-adversarial-questions--innovation-verification)

---

# Category 1: Problem Statement & Real-World Need

### Q1.1 (Beginner): Explain your entire project in one minute.
- **Direct Answer:** "Valor is an offline wearable for disaster relief that pairs a Renesas ForgeFPGA with a dual-core ESP32-S3. The FPGA eliminates operating system jitter to detect heartbeats with nanosecond precision, while the microcontroller fuses vitals with toxic smoke, heat, cyclone pressure drops, and falls. Running a zero-cloud integer neural network and clinical mNEWS2 protocols, Valor predicts impending heat stroke, respiratory failure, or hypothermia completely offline, alerting rescuers via an OLED emergency screen and a local Wi-Fi dashboard."
- **Technical Explanation:** In extreme disasters, communication infrastructure fails while extreme environmental stressors accelerate physiological collapse. Single-modality monitors create alarm fatigue. Valor couples continuous optical physiological extraction with atmospheric sensing and runs all triage algorithms locally on silicon.
- **Evidence:** `docs/MASTER_PROJECT_GUIDE.md`, `firmware/shrikefi/main_shrikefi.c`.
- **Follow-up Challenge:** "Why can't rescuers just use a commercial smartwatch like an Apple Watch or Garmin?"
- **Safe Response:** "Commercial smartwatches rely on cloud analytics, have no particulate matter sensors or barometric cyclone trend engines, run closed operating systems with 10 ms scheduling jitter that degrades HRV precision, and cannot act as unencrypted local Wi-Fi triage servers in a disaster zone."

---

### Q1.2 (Intermediate): Who is the primary target user, and how does this change their operational workflow?
- **Direct Answer:** "Our primary users are disaster first responders and isolated evacuees. Instead of periodic manual vital checks, responders get continuous, hands-free monitoring with automated alarms if heat stroke or toxic smoke inhalation begins."
- **Technical Explanation:** First responders operating in hazardous environments often ignore subtle onset symptoms of autonomic exhaustion. Valor monitors Moran's Physiological Strain Index (PSI) and AHA autonomic depression continuously. If a responder's core vitals collapse or they suffer a fall, the device automatically triggers an audible siren and projects their medical snapshot and location to nearby rescue teams.
- **Evidence:** `firmware/core/disaster_risk_engine.c` (`disaster_calculate_moran_psi`), `firmware/shrikefi/sos.c`.
- **Follow-up Challenge:** "How does an isolated evacuee benefit if they don't have a rescuer nearby?"
- **Safe Response:** "The onboard 128x64 OLED display provides immediate clear English action advisories—such as 'Seek cooling, hydrate NOW' or 'Barometer falling fast, move to shelter'—giving the user self-triage guidance even if completely alone."

---

# Category 2: Product Overview & Differentiating Features

### Q2.1 (Intermediate): What makes Valor genuinely innovative compared to academic papers that combine sensors?
- **Direct Answer:** "Three things: First, we moved time-critical peak detection into synthesizable Verilog RTL to solve RTOS timing jitter. Second, our multi-hazard AI runs in 619 bytes on integer-only microcontroller silicon. Third, we bridge the communication gap using an unencrypted local SoftAP that serves a full diagnostic dashboard to any standard phone browser with zero internet or app downloads."
- **Technical Explanation:** Most academic works either run everything on an expensive PC in MATLAB/Python or deploy an MCU that suffers from RTOS scheduling jitter and cloud dependency. Valor is a heterogeneous system designed down to physical register maps, FPGA bitstreams, and RTOS core pinning.
- **Evidence:** `hardware/shrikefi/forgefpga_ppg_top.v`, `firmware/core/nn_risk_model_int8.c`, `firmware/shrikefi/web_status.c`.
- **Follow-up Challenge:** "Are you claiming your neural network diagnoses diseases?"
- **Safe Response:** "No. We explicitly state our model provides multi-hazard physiological *risk triage*, not clinical diagnosis. Absolute medical overrides are governed by the clinically validated Royal College of Physicians mNEWS2 protocol."

---

# Category 3: System Architecture

### Q3.1 (Intermediate): Trace one raw optical sample from the sensor to the final triage decision.
- **Direct Answer:** "An 18-bit optical sample is read from the MAX30102 by Core 0 at 100 Hz, scaled to 8 bits, and sent over SPI to the ForgeFPGA. The FPGA's 8-tap boxcar filter smooths it, and its 4-state FSM detects the systolic peak, returning `{beat, filt[6:0]}`. Core 0 captures the microsecond hardware timer to compute the IBI and transfers it to Core 1 via a mutex. Core 1 runs Karlen SQI, Charlton respiratory extraction, Si et al. PM2.5 debiasing, and INT8 TinyML inference, passing the result to mNEWS2 to generate the triage advisory."
- **Technical Explanation:** Tracing path: MAX30102 $\to$ I2C (Core 0) $\to$ SPI2 (MOSI) $\to$ `moving_average_8tap.v` $\to$ `ppg_peak_detector.v` $\to$ SPI2 (MISO) $\to$ Core 0 Timer Latch $\to$ `s_data_mutex` $\to$ Core 1 (`task_disaster_monitor`) $\to$ `ppg_sqi.c` $\to$ `nn_risk_model_int8.c` $\to$ `clinical_vitals_engine.c` $\to$ SSD1306 OLED & Web Status daemon.
- **Evidence:** `hardware/shrikefi/forgefpga_ppg_top.v:45-72`, `firmware/shrikefi/main_shrikefi.c:1010-1090`.
- **Follow-up Challenge:** "What happens if Core 0 experiences an I2C bus lockup while reading the MAX30102?"
- **Safe Response:** "Our I2C driver implements bus timeout error handling and auto-resets the I2C peripheral, returning -1 to ensure the FreeRTOS task does not deadlock."

---

# Category 4: Hardware & FPGA Design

### Q4.1 (Advanced): Why did you choose the Renesas ForgeFPGA SLG47910C over a Xilinx Artix-7 or Zynq?
- **Direct Answer:** "Cost, power, and board footprint. The Renesas ForgeFPGA costs approximately $0.50 and consumes under 5 milliwatts, making a mass-deployed disaster wearable economically feasible. The Zynq-7000 costs $50 to $150 and consumes watts, which is unsuitable for a low-cost wearable. We used the Zynq-7000 as our verified golden baseline and ported the core to the ForgeFPGA."
- **Technical Explanation:** The Zynq-7000 was verified in Vivado ML 2022.2 (185 LUTs, 0 DSP, 0 BRAM, Fmax 69.45 MHz). The ForgeFPGA SLG47910C fits into 515 LUT5s (45.98% fabric), 303 flip-flops and 105/140 CLBs, with a fitter-reported achievable frequency of 88.059 MHz — no Block RAM or dedicated DSPs are needed. (No clock constraint exists yet, so this is an unconstrained fit result, not timing closure.)
- **Evidence:** `docs/MIGRATION.md`, `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log`.
- **Follow-up Challenge:** "Does the ForgeFPGA have enough logic remaining for feature expansion?"
- **Safe Response:** "Yes. With 515 LUT5s utilized out of 1120, 54.02% of the LUT fabric remains uncommitted; CLB occupancy is the tighter number at 105 of 140 blocks."

---

### Q4.2 (Adversarial): Does the ESP32-S3 program the FPGA at runtime, or is the bitstream stored in FPGA NVRAM?
- **Direct Answer:** "The ESP32-S3 programs the FPGA over SPI2 at every system boot using an embedded bitstream C array. Once programmed, the FPGA runs autonomously and responds to our SPI link handshake."
- **Technical Explanation:** The ForgeFPGA SLG47910C is an SRAM-based device. In `firmware/shrikefi/main_shrikefi.c`, `shrikefi_link_init()` configures GPIO 8 (EN) and GPIO 9 (PWR), holds the FPGA in reset, clocks in 46,408 bytes of configuration data from `forgefpga_bitstream.h`, and verifies that the `DONE` signal asserts high. It then switches SPI2 to runtime Mode 0 and verifies communication with an echo probe.
- **Evidence:** `firmware/shrikefi/shrikefi_link_driver.c:38-95`, `firmware/shrikefi/forgefpga_bitstream.h`.
- **Follow-up Challenge:** "What proves the FPGA is actually running the bitstream and not just floating?"
- **Safe Response:** "The runtime initialization probe sends `0x55` across SPI and asserts when the returned filtered byte matches the expected response. Furthermore, physical LED D12 is driven directly by the FPGA's `o_beat` pulse output."

---

# Category 5: RTL, Clocks, Resets, Counters & State Machines

### Q5.1 (Advanced): Walk through your systolic peak detector RTL cycle-by-cycle.
- **Direct Answer:** "The detector operates in 4 states: IDLE waits for samples $\ge 120$. ARMED tracks the rising peak value until the signal falls. CREST_FALL enforces our H-01 slope gate: the signal must drop by at least 3 units below the peak before firing the beat pulse. REFRACTORY holds the detector inactive for 12,500,000 cycles (250 ms at 50 MHz) to ignore the dicrotic notch before returning to IDLE."
- **Technical Explanation:** State transitions:
  - `STATE_IDLE (2'b00)`: Sample crosses `THRESHOLD` (120) $\to$ `STATE_ARMED`.
  - `STATE_ARMED (2'b01)`: Samples rise, `peak_val` updates. When `i_sample < peak_val` $\to$ `STATE_CREST_FALL`.
  - `STATE_CREST_FALL (2'b10)`: If `(peak_val - i_sample) >= 3`, asserts `o_beat = 1` for 1 clock cycle, latches `o_ibi_cycles`, clears counter, and transitions to `STATE_REFRACTORY`. If signal rises again without dropping 3 units, it returns to `STATE_ARMED` as noise.
  - `STATE_REFRACTORY (2'b11)`: Counter increments until 250 ms expires $\to$ `STATE_IDLE`.
- **Evidence:** `hardware/common/ppg_peak_detector.v:45-98`.
- **Follow-up Challenge:** "Why is the slope gate (MIN_FALL_STEP = 3) necessary?"
- **Safe Response:** "Without the slope gate, high-frequency optical noise plateauing at the crest causes multiple threshold crossings on a single heartbeat, causing false double-beat detections."

---

### Q5.2 (Advanced): How does your 8-tap boxcar filter achieve O(1) arithmetic with zero multipliers?
- **Direct Answer:** "It uses a running-sum recursive structure. Instead of adding all 8 taps every cycle, it subtracts the oldest tap and adds the newest sample to an 11-bit accumulator. Division by 8 is implemented as a hardwired bit shift right by 3 (`sum[10:3]`), requiring 0 multipliers and 0 DSP slices."
- **Technical Explanation:** Formula: $S[n] = S[n-1] + x[n] - x[n-8]$. An 11-bit accumulator is sufficient because $8 \times 255 = 2040 < 2048$. The output `assign o_filtered = sum[10:3];` simply drops the lowest 3 bits, synthesizing to direct wiring.
- **Evidence:** `hardware/common/moving_average_8tap.v:28-48`.
- **Follow-up Challenge:** "What happens on arithmetic overflow if a sample exceeds 255?"
- **Safe Response:** "Inputs are strictly typed as 8-bit unsigned wire vectors (`[7:0]`), so an input sample can never exceed 255."

---

# Category 6: Protocols, Busses & Interfaces

### Q6.1 (Intermediate): Detail the physical pinout and signal contracts between the ESP32-S3 and ForgeFPGA.
- **Direct Answer:** "The link uses an 8-bit SPI Mode 0 interface over 4 internal PCB traces: SCK on GPIO 12, Chip Select SS_n on GPIO 10, MOSI on GPIO 11, and MISO on GPIO 13. GPIO 8 controls FPGA Enable and GPIO 9 controls switched power."
- **Technical Explanation:**
  - `spi_sck` (GPIO 12 $\to$ ForgeFPGA PIN_16): 1 MHz runtime SPI clock.
  - `spi_ss_n` (GPIO 10 $\to$ ForgeFPGA PIN_17): Active-low manual chip select.
  - `spi_mosi` (GPIO 11 $\to$ ForgeFPGA PIN_18): MCU transmits raw scaled sample byte.
  - `spi_miso` (GPIO 13 $\leftarrow$ ForgeFPGA PIN_19): FPGA returns `{beat, filt[6:0]}` byte.
  - Timing: One byte exchanged every 10 ms (100 Hz). Transaction completes in 8 µs, representing 0.08% bus utilization.
- **Evidence:** `docs/SHRIKEFI_LINK_PROTOCOL.md`, `firmware/shrikefi/shrikefi_pinmap.h`.
- **Follow-up Challenge:** "Why do you use SPI Mode 0 instead of Mode 3?"
- **Safe Response:** "SPI Mode 0 (`CPOL=0, CPHA=0`) matches the native shift register architecture of the ForgeFPGA macrocells, where data is shifted on the falling edge of SCK and sampled on the rising edge."

---

### Q6.2 (Intermediate): What happens if the ESP32-S3 transmits an SPI frame with SS_n held continuously low?
- **Direct Answer:** "The FPGA shift register only reloads its return byte on the rising edge of SS_n. If SS_n is held continuously low across multiple bytes, the FPGA will repeat stale data."
- **Technical Explanation:** `spi_target.v` specifies that `tx_shift <= tx_data_byte` is clocked by `posedge i_ss_n`. Firmware driver `shrikefi_transceive_sample()` explicitly toggles SS_n high for at least 1 µs between frames to guarantee reload timing.
- **Evidence:** `hardware/shrikefi/spi_target.v:65-78`, `firmware/shrikefi/shrikefi_link_driver.c:120-135`.

---

# Category 7: AI/ML Methodology & Zero-Cloud Execution

### Q7.1 (Advanced): Explain the mathematical execution of your INT8 neural network on the microcontroller.
- **Direct Answer:** "The model is a 6→24→16→3 MLP. Inputs are normalized to `[-128, 127]`. Matrix multiplications are executed using 32-bit signed integer accumulation to prevent overflow. Outputs are scaled using fixed-point shifts, and activations use ReLU and a 32-step piecewise linear Sigmoid lookup table, completing in 0.38 microseconds on the host and ~18 microseconds on the ESP32-S3."
- **Technical Explanation:**
  $$\text{Accumulator}_i = \sum_{j=0}^{M-1} W_{int8}[i][j] \cdot X_{int8}[j]$$
  Biases are pre-scaled to align with the accumulator's fixed-point format. Hidden layers apply `if (acc < 0) acc = 0`. The output layer evaluates Sigmoid without using floating-point `expf()`, executing in integer logic. Total model size: 619 bytes.
- **Evidence:** `firmware/core/nn_risk_model_int8.c:45-112`.
- **Follow-up Challenge:** "What is the quantization loss compared to the floating-point model?"
- **Safe Response:** "Across our validation suite, the maximum absolute error between FP32 and INT8 outputs is 0.091, with zero categorical classification divergence across all tested disaster profiles."

---

### Q7.2 (Adversarial): Did you train your model on real disaster victims?
- **Direct Answer:** "No. Gathering physiological ICU data from patients inside active cyclones or wildfires is practically impossible and ethically restricted. We benchmarked our clinical vital extraction on 16,387 gold-standard records from the MIMIC-III database, while our multi-hazard neural network was trained by distilling established environmental medical equations."
- **Technical Explanation:** The training pipeline (`firmware/core/train_nn_risk_model.py`) distills peer-reviewed physiological rules: Moran's Physiological Strain Index (PSI), Si et al. optical particulate hygroscopic growth equations, and AHA PM2.5 cardiovascular autonomic strain criteria.
- **Evidence:** `README.md` (Reproducibility & Evidence Provenance section), `firmware/core/accuracy_evaluator.c`.

---

# Category 8: Hardware-Software Integration

### Q8.1 (Intermediate): How do the two microcontroller cores communicate without memory corruption?
- **Direct Answer:** "They communicate via a shared state structure `g_state` protected by a FreeRTOS binary mutex `s_data_mutex`. Core 0 writes vitals with a 5 ms timeout after each beat; Core 1 takes a stack snapshot at the start of its 1-second triage cycle."
- **Technical Explanation:** To eliminate priority inversion, `s_data_mutex` uses priority inheritance. Core 0 never blocks waiting for Core 1; if the mutex is locked, it drops the non-critical telemetry write and continues sampling. Diagnostic logging reads on Core 0 are also wrapped in `s_data_mutex` to prevent memory tearing.
- **Evidence:** `firmware/shrikefi/main_shrikefi.c:1104-1160`.
- **Follow-up Challenge:** "Why not use FreeRTOS queues instead of a shared memory mutex?"
- **Safe Response:** "Queues introduce dynamic heap allocation and per-sample copy overhead. For high-frequency vital state snapshots, a mutex-guarded static structure provides deterministic execution without memory fragmentation."

---

# Category 9: Testing, Verification & Regression Results

### Q9.1 (Intermediate): What automated tests exist in the repository to prove the system works?
- **Direct Answer:** "We have 5 self-checking test suites: 1) The master C11 regression suite with 21 passing tests; 2) The GNSS NMEA suite with 12 passing tests; 3) The biomechanical fall detection suite with 16 passing tests; 4) The ShrikeFi application host simulation with 12 operational profiles; and 5) The Icarus Verilog RTL testbenches with 16 passing checks across ForgeFPGA and Zynq."
- **Technical Explanation:** All testbenches are self-checking and execute under strict compiler flags (`-Wall -Wextra -Werror -pedantic -std=c11`). In CI, any warning or assertion failure immediately halts the build.
- **Evidence:** `firmware/zynq/test_disaster_risk_engine.c`, `hardware/shrikefi/tb_forgefpga_system.v`, `hardware/zynq/tb_ppg_system.v`.
- **Follow-up Challenge:** "Show me the command to verify all clinical algorithms right now."
- **Safe Response:** "Run: `gcc -Wall -Wextra -Werror -pedantic -std=c11 -Ifirmware/core -Ifirmware/zynq -Ifirmware/shrikefi firmware/zynq/test_disaster_risk_engine.c firmware/core/*.c firmware/shrikefi/sos.c firmware/shrikefi/location.c firmware/shrikefi/web_status.c -lm -o test.exe && .\test.exe`."

---

# Category 10: Performance, Latency, Accuracy, Power & Utilization

### Q10.1 (Advanced): What is the end-to-end latency from an optical pulse entering the photodiode to an alert firing?
- **Direct Answer:** "Under 1.2 milliseconds for hardware optical filtering, and exactly 1 second for the clinical triage update loop."
- **Technical Explanation:**
  - MAX30102 ADC conversion + FIFO read: ~10 ms (at 100 Hz).
  - SPI transfer: 8 µs.
  - FPGA 8-tap filter + peak detector: 8 clock cycles (160 ns @ 50 MHz).
  - Core 0 IBI latching: < 1 µs.
  - Core 1 Clinical Triage + TinyML inference: ~18 µs on the ESP32-S3.
  - Total latency from physical pulse to local display update is dominated by the 1 Hz clinical evaluation quantum.
- **Evidence:** `firmware/shrikefi/main_shrikefi.c`, `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log`.
- **Follow-up Challenge:** "What is your battery runtime?"
- **Safe Response:** "We estimate 14 to 18 hours on a 1200 mAh LiPo battery based on an average current draw of ~65 mA (with Wi-Fi in low-power beacon mode and 100 Hz sensor acquisition). Physical continuous battery discharge curves under active SoftAP broadcast remain to be measured."

---

# Category 11: Security, Privacy, Reliability & Failure Recovery

### Q11.1 (Intermediate): Does Valor leak private patient health data over the air?
- **Direct Answer:** "No. All public cloud MQTT publishing is permanently compiled out by default (`CONFIG_SHRIKEFI_CLOUD_PUBLISH=n`). The onboard Wi-Fi access point operates locally without internet access, and all patient vitals remain strictly on the device."
- **Technical Explanation:** The local SoftAP serves an unencrypted, credential-free dashboard (`192.168.4.1`) by design, ensuring any rescuer with a standard phone can view triage data in a crisis. To protect against malicious tampering, administrative actions like changing the stored deployment location require same-origin browser headers (`Origin: http://192.168.4.1`).
- **Evidence:** `docs/REMAINING_WORK.md` (T1.1), `firmware/shrikefi/web_status.c:503-520`.

---

# Category 12: Cost, Deployment, Scalability & Maintainability

### Q12.1 (Beginner): What is the Bill of Materials (BOM) cost to manufacture Valor in volume?
- **Direct Answer:** "Under ₹1,800 INR (approximately $21 USD) in modest production quantities."
- **Technical Explanation:**
  - ESP32-S3-WROOM-1-N8R2 Module: ~$2.80 (₹230)
  - Renesas ForgeFPGA SLG47910C: ~$0.55 (₹45)
  - MAX30102 Pulse Oximeter Module: ~$1.80 (₹150)
  - BME280 Environmental Sensor: ~$2.20 (₹180)
  - Plantower PMS5003 Particulate Sensor: ~$10.00 (₹820)
  - MPU6050 6-Axis IMU: ~$0.90 (₹75)
  - SSD1306 0.96" I2C OLED: ~$1.50 (₹120)
  - PCB, passives, battery & enclosure: ~$2.00 (₹160)
  - Total BOM: ~$21.75 (~₹1,780 INR).
- **Evidence:** `docs/MASTER_PROJECT_GUIDE.md:880-910`.

---

# Category 13: Alternatives & Engineering Trade-Offs

### Q13.1 (Intermediate): Why didn't you use an analog circuit (op-amps, comparators) for pulse filtering instead of an FPGA?
- **Direct Answer:** "Analog filters have fixed cutoff frequencies that cannot dynamically adapt to baseline drift, vary with temperature and component tolerances, and cannot implement a 250 ms refractory state machine or measure 20 ns digital intervals."
- **Technical Explanation:** An optical PPG signal exhibits significant DC baseline wander from respiration and finger pressure shifts. An analog bandpass filter suffers from phase distortion and long settling times. Our FPGA implementation provides linear-phase moving average filtering, deterministic digital thresholding, and cycle-accurate IBI timestamping without component aging drift.
- **Evidence:** `hardware/common/moving_average_8tap.v`.

---

# Category 14: Candid Limitations, Unfinished Features & Future Work

### Q14.1 (Adversarial): What is the single weakest part of your current implementation?
- **Direct Answer:** "Our skin temperature measurement currently uses the MAX30102 on-chip die sensor with a 1.2°C LED self-heating compensation constant. While sufficient for detecting severe hypothermia (<35°C) or fever (>38°C), it does not have the precision of a dedicated clinical skin-contact probe like the MAX30205."
- **Technical Explanation:** The MAX30102 die temperature sensor is located within the optical package. While finger contact conducts heat to the die, internal LED illumination generates ~1.2°C of thermal offset. We filter and compensate for this in firmware, but a dedicated I2C skin probe (MAX30205) is required for hospital-grade thermometry.
- **Evidence:** `firmware/shrikefi/main_shrikefi.c:1400-1435`.

---

# Category 15: Adversarial Questions & Innovation Verification

### Q15.1 (Adversarial): "You claim this is an FPGA project, but your FPGA just runs a boxcar filter and peak detector while the ESP32 does all the real work. Isn't the FPGA just a marketing gimmick?"
- **Direct Answer:** "No. Offloading peak detection to hardware is the only reason our Heart Rate Variability metrics are valid. Without the FPGA, peak detection in software would be quantized to the FreeRTOS 10 ms scheduling tick, introducing timing errors larger than the RMSSD signal itself."
- **Technical Explanation:** In a healthy adult, RMSSD is 20–50 ms; in autonomic collapse, it drops below 15 ms. An RTOS tick jitter of 10 ms represents a 66% measurement error, rendering HRV calculation meaningless. The FPGA detects peaks with 20 ns resolution directly in hardware, shielding biosignal timing from CPU scheduling, Wi-Fi radio interrupts, and flash cache latency.
- **Evidence:** `docs/MASTER_PROJECT_GUIDE.md:75-105`, `hardware/shrikefi/tb_forgefpga_system.v:310-340`.

---

### Q15.2 (Adversarial): "What would fail if we disconnected the FPGA right now during your demo?"
- **Direct Answer:** "Heart Rate Variability (RMSSD) and Charlton respiratory rate estimation would fail because the cycle-accurate pulse timestamps would disappear. The system would fall back to software thresholding, but telemetry would flag that HRV precision is degraded."
- **Technical Explanation:** `main_shrikefi.c` implements a software fallback path for demonstration resilience (`s_fpga_available`). If the FPGA fails to answer its SPI probe, Core 0 switches to software peak detection, but RMSSD is suppressed because timing jitter invalidates autonomic strain analysis.
- **Evidence:** `firmware/shrikefi/main_shrikefi.c:712-745`.
