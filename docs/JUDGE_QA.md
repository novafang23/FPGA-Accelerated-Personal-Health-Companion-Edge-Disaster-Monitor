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
- **Direct Answer:** "An 18-bit optical sample is read from the MAX30102 by Core 0 at 100 Hz, scaled to 8 bits, and sent over SPI to the ForgeFPGA. The FPGA's 8-tap boxcar filter smooths it, and its 4-state FSM detects the systolic peak, returning `{beat, filt[6:0]}`. Core 0 captures the microsecond hardware timer to compute the IBI and transfers it to Core 1 via a mutex. Core 1 runs Karlen SQI, Charlton respiratory extraction, the published PMS5003 PM2.5 correction, and INT8 TinyML inference, passing the result to mNEWS2 to generate the triage advisory."
- **Technical Explanation:** Tracing path: MAX30102 $\to$ I2C (Core 0) $\to$ SPI2 (MOSI) $\to$ `moving_average_8tap.v` $\to$ `ppg_peak_detector.v` $\to$ SPI2 (MISO) $\to$ Core 0 Timer Latch $\to$ `s_data_mutex` $\to$ Core 1 (`task_disaster_monitor`) $\to$ `ppg_sqi.c` $\to$ `nn_risk_model_int8.c` $\to$ `clinical_vitals_engine.c` $\to$ SSD1306 OLED & Web Status daemon.
- **Evidence:** `hardware/shrikefi/forgefpga_ppg_top.v:45-72`, `firmware/shrikefi/main_shrikefi.c:1010-1090`.
- **Follow-up Challenge:** "What happens if Core 0 experiences an I2C bus lockup while reading the MAX30102?"
- **Safe Response:** "Our I2C driver implements bus timeout error handling and auto-resets the I2C peripheral, returning -1 to ensure the FreeRTOS task does not deadlock."

---

# Category 4: Hardware & FPGA Design

### Q4.1 (Advanced): Why did you choose the Renesas ForgeFPGA SLG47910C over a Xilinx Artix-7 or Zynq?
- **Direct Answer:** "Cost, power, and board footprint. The Renesas ForgeFPGA costs approximately $0.50 and consumes under 5 milliwatts, making a mass-deployed disaster wearable economically feasible. The Zynq-7000 costs $50 to $150 and consumes watts, which is unsuitable for a low-cost wearable. We used the Zynq-7000 as our verified golden baseline and ported the core to the ForgeFPGA."
- **Technical Explanation:** The Zynq-7000 was verified in Vivado ML 2022.2 (185 LUTs, 0 DSP, 0 BRAM, Fmax 69.45 MHz). The ForgeFPGA SLG47910C fits into 515 LUT5s (45.98% fabric), 303 flip-flops and 105/140 CLBs, and **closes timing at its 50 MHz clock (WNS +7.316 ns, TNS 0, zero failing endpoints)** — no Block RAM or dedicated DSPs are needed.
- **Evidence:** `docs/MIGRATION.md`, `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log`, `hardware/shrikefi/synthesis_evidence/timing_summary_spi_link_50mhz.log`.
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
- **Direct Answer:** "The model is a 6→24→16→3 MLP with 619 parameters stored in 619 bytes. Inputs are min-max normalized to `[0,1]`; the weights are stored as signed 8-bit integers (per-tensor symmetric, zero-point 0) and the activations are requantized to uint8 between layers. The multiply-accumulate runs in **single-precision float** — the kernel dequantizes the INT8 weights per layer — and the sigmoid is a real `expf()` call, not a lookup table. It executes in ~0.44 µs on a host x86-64 compiler; the ESP32-S3 figure has not been measured."
- **Technical Explanation:**
  $$\text{Accumulator}_i = \sum_{j=0}^{M-1} W_{float}[i][j] \cdot X_{uint8}[j]$$
  where \(W_{float} = s_W \cdot W_{int8}\) with \(s_W = 0.03131007\) (layer 1). Hidden layers apply ReLU (`if (acc < 0) acc = 0`); the output layer applies a true sigmoid \(1/(1+e^{-x})\). The **quantization** is what saves flash and RAM and makes the shipped weights reproducible; it is **not** an integer MAC unit, and the shipped hazard model has no 32-bit fixed-point accumulator and no sigmoid LUT.

  There is no second network in this repository any more. An earlier build carried a private INT8 **PM2.5 calibration** net (`pm25_calibration_int8.c`) that used `[-128,127]` int8 inputs, int32 accumulation and a 32-step piecewise-linear sigmoid LUT. It was removed on 2026-10-20 because its weights could not be reproduced (no training script, no data, no citation) and because measurement showed it cut readings by 31–53 % in dry air and collapsed small readings to zero at high humidity — a hazardous failure direction. The published correction described in Q7.5 replaced it.

  **Honest verification hook:** the linker map of the shipped firmware lists `expf` and `roundf` as the only math functions pulled in by `nn_risk_model_int8.c.obj` — that is the evidence that the deployed sigmoid is a real exponential, not a table. The float32 model is not in the device image at all: the ESP32 build compiles only `nn_risk_model_int8.c` (`NN_ENABLE_FLOAT32_MODEL=0`).
- **Evidence:** `firmware/core/nn_risk_model_int8.c:45-112`, `firmware/shrikefi/main/CMakeLists.txt` (INT8-only build), `firmware/shrikefi/build/shrikefi_health_companion.map` (link map of the shipped image).
- **Follow-up Challenge:** "What is the quantization loss compared to the floating-point model?"
- **Safe Response:** "Measured on the 7,000-sample held-out split: mean absolute error **0.0079**, max **0.0914**, and **97.32%** tier agreement between the FP32 reference and the INT8 model. The INT8 model is not identical to FP32 on every sample — 2.68% of samples land in a different risk tier — but the three documented archetype scenarios agree exactly. The **shipped INT8 model's own agreement with the rule-based teacher is 88.39%** (float32 reference: 88.47%); that is the number in the header of `nn_risk_model_int8.c.inc`."

---

### Q7.2 (Adversarial): Did you train your model on real disaster victims?
- **Direct Answer:** "No. Gathering physiological ICU data from patients inside active cyclones or wildfires is practically impossible and ethically restricted. We benchmarked our clinical vital extraction on 16,387 gold-standard records from the MIMIC-III database, while our multi-hazard neural network was trained by distilling established environmental medical equations."
- **Technical Explanation:** The training pipeline (`firmware/core/train_nn_risk_model.py`) is a **distillation of this project's own rule engine** (`disaster_risk_engine.c`), whose rules are built from published ideas: Moran's Physiological Strain Index (heat strain), the AHA PM2.5 cardiovascular autonomic-strain criteria (pollution), and a cold/flood exposure proxy (chill + heart-rate strain). Two honest caveats: the pollution teacher is a **direct port** of our own pollution rule, while the heat teacher is a **documented linear proxy** for the heat index (not the Rothfusz formula the rule engine itself uses), and the flood teacher is an adaptation, because the 6-feature vector has no skin-temperature channel. So the network reproduces *our rules*, not a clinical ground truth — which is exactly why it is deployed as an advisory second opinion that may raise the verdict, never lower it.
- **Evidence:** `README.md` (Reproducibility & Evidence Provenance section), `firmware/core/accuracy_evaluator.c`.

### Q7.3 (Intermediate): Your HRV tile reads `settling` — how do you still produce a NORMAL/MODERATE/CRITICAL verdict, and why does RMSSD matter at all?
- **Direct Answer:** "Two scores produce that verdict and neither is built from HRV. NEWS2 uses heart rate, SpO2, respiratory rate and temperature (scored in bands: 0 Normal, 1–3 Low/Elevated, 4–6 High/Medium, ≥7 Critical); the multi-hazard engine uses those vitals plus PM2.5, ambient temperature, humidity and pressure trend. RMSSD requires our physiological settlement gate—30 beats covering at least 20 seconds and ≥15 valid NN pairs (`hrv_is_settled()`)—before publishing. It is an additional autonomic term, not a prerequisite, so a withheld HRV removes one contribution rather than freezing the verdict. It matters because it is the earliest channel we have: a falling RMSSD precedes the HR/SpO2/RR changes NEWS2 needs, and two people with identical vitals can differ in physiological reserve."
- **Technical Explanation:** `clinical_vitals_engine.c` computes NEWS2 from HR, SpO2, RR and temperature; RMSSD appears only in the condition text and only when a value has been published. `disaster_risk_engine.c` guards every HRV term with `if (rmssd > 0)` and documents why: zero means "no valid HRV", not "maximal suppression", so an absent measurement contributes zero points instead of the maximum. The INT8 network takes RMSSD as an input feature, so when no value has been published the engine declines to score, the device streams `AIVALID=0`, and both dashboards render `--` instead of a fabricated `0.0 %`. A zero normalized into that feature would sit at the most-suppressed end of its range and could raise an alarm from a missing measurement. The same discipline applies before contact: a "hold" (invalid vitals, SQI 0, or SQI < 0.70 without corroboration) is flagged as signal noise and contributes no clinical level, so an untouched device reports `RISK=UNKNOWN` ("Patient not assessed") rather than `MODERATE`. The same guard refuses implausible input instead of clamping it: a non-finite value, bpm outside 30-220, ambient outside -20..65 C, humidity outside 0-100 % or a positive SpO2 outside 50-100 % decline with `RISK_UNKNOWN`, and the serial log names the reason once per change. `hr = 0` with no finger is the case that was reachable on the device - it used to score as flood 0.31 (MODERATE).
- **Evidence:** `firmware/core/clinical_vitals_engine.c`, `firmware/core/disaster_risk_engine.c` (`disaster_assess_nn_int8`), `firmware/shrikefi/main_shrikefi.c` (`[TRIAGE]` `AIVALID`), `firmware/shrikefi/dashboard/valor_dashboard.html` (`aiAbsent`).
- **Honest Limitation:** HRV has not been validated against ECG (T4.1), so its contribution is a bounded nudge, not a diagnosis. The vitals-and-environment verdict is defensible without it; the HRV term alone is not.

---

### Q7.4 (Adversarial): Your neural network is trained on synthetic labels. Is it actually better than the rule engine it was distilled from?
- **Direct Answer:** "No — and we do not claim it is. The network is a *second, independent opinion* that can add caution, not a better risk engine. Measured on the 7,000-sample held-out split, the rule engine agrees with the teacher 85.57% of the time, the network alone 81.56%, and the fused verdict 83.67%. Where the network is better: heat (92.69% vs 82.39%) and flood (86.40% vs 78.16%). Where it is worse: pollution (87.11% vs 99.09%), because the teacher's hard 'clean air ≤ 35 µg/m³ ⇒ risk 0' step is not something a smooth network can represent. The fusion is deliberately 'worst wins': the network may raise the verdict, never lower it."
- **Technical Explanation:** The network's training labels come from the same teacher functions the rule engine implements (`train_nn_risk_model.py` distils `ctsi_score`, `prsi_score`, `flood_score` — with an explicit note that the heat teacher is a *proxy*, not a port of the Rothfusz heat index the rule engine uses). So the comparison above is "network versus its own teacher", and the network cannot be better than the teacher in any systematic sense; what it adds is a different *smoothness* of generalisation. On the seven documented demo archetypes the two agree on the overall level in six of seven, differing only on hypothermia (rules HIGH, network CRITICAL). The network changes the fused verdict in 3.61% of cases, always upward.
- **Evidence:** `docs/reports/AI_MODEL_AUDIT_2026-10-20.md` §6–§7, `firmware/core/train_nn_risk_model.py` (teacher functions and the fidelity note), `firmware/core/clinical_vitals_engine.c` (`clinical_fuse_triage`).
- **Honest Limitation:** Three things a judge should hold against us. (1) The "accuracy" figures are **agreement with a rule-based teacher**, never with clinical outcomes. (2) The network **ignores humidity** (layer-1 weight mass 0.21 versus 5.78 for PM2.5; a full-range humidity sweep moves the outputs by ≤0.004) because the teacher barely uses it. (3) The network is only 619 bytes and 17 of its 41 weight rows are dead after distillation — it is a small, smooth model, and it is deployed as advisory.

---

### Q7.5 (Adversarial): How do you know your PM2.5 number is right? Your sensor is a cheap particle counter.
- **Direct Answer:** "We do not publish the sensor's raw number. The PMS5003 reports an optical particle count converted to a mass concentration with an assumed particle density and size, and that assumption over-reads: the published collocation work measures roughly a 40 % over-read for the PurpleAir across most of the US (Barkjohn, Gantt & Clements 2021, AMT 14, 4617-4637) and about 50 % for the PMS5003 above 10-20 µg/m³ (AirGradient, 12,513 daily collocation points in 17 outdoor locations in 11 countries). We apply the **published** correction for this exact sensor channel — the PMS5003 'atm' (environmental) banded equations that AirGradient publishes openly — to a 60 s rolling mean of the raw readings, and we publish the raw value, the corrected value and a confidence code together. We deleted our own private calibration network because it was not reproducible and because it could report a hazardous atmosphere as clean air."
- **Technical Explanation:** The correction is a piecewise function of the raw value with a small humidity term (0.0862 µg/m³ per %RH, i.e. ~8.6 µg/m³ across the whole RH range): `0.524·AGraw − 0.0862·RH + 5.75` below 30 µg/m³, `0.786·AGraw − 0.0862·RH + 5.75` from 50 to 210, and a quadratic `2.966 + 0.69·AGraw + 8.84e-4·AGraw²` from 260. The humidity term exists because hygroscopic growth of the aerosol at high RH scatters more laser light (Hua et al. 2021; Tejada et al. 2022). Three deliberate rules: a **missing reading is never published as 0** (the value is negative, the pollution modality becomes UNKNOWN, the AQI reads 'Not assessed'); a **stale reading is withdrawn** after 10 s without a fresh sample; and **dust suppresses the correction**, because under elevated dust loading the PMS5003 *under*-reads (Kaur, Mangin & Kelly 2026, AMT 19, 1077) and a correction that only ever lowers the number would make that case worse.
- **Evidence:** `firmware/core/pm25_correction.c` (equations with their source), `firmware/core/test_pm25_correction.c` (29 checks), `docs/reports/PM25_CALIBRATION_2026-10-20.md`, `firmware/core/disaster_risk_engine.c` (`assess_pollution_risk`, negative-PM2.5 guard).
- **Honest Limitation:** We have **no local reference instrument**, so we cannot validate the correction on our bench; we inherit the published performance (nRMSE ≈ 38 % across 17 sites, worse at 3 of them) and that is all we can claim. The correction was fitted on ambient North-American/European aerosol. Our humidity input comes from the BME280 rather than the sensor's own hygrometer. And the estimate is a 60 s mean, so a sudden smoke event appears within about a minute. This is an environmental estimate, not a clinical measurement, and it has never been validated against a reference instrument.

---

# Category 8: Hardware-Software Integration

### Q8.1 (Intermediate): How do the two microcontroller cores communicate without memory corruption?
- **Direct Answer:** "They communicate via a shared state structure `g_state` protected by a FreeRTOS binary mutex `s_data_mutex`. Core 0 writes vitals with a bounded 10 ms timeout after each beat; Core 1 takes a stack snapshot at the start of its 1-second triage cycle."
- **Technical Explanation:** To eliminate priority inversion, `s_data_mutex` uses priority inheritance. Core 0 can wait at most 10 ms (two ticks at 200 Hz) for Core 1; if the mutex is still locked it drops the non-critical telemetry write and continues sampling. Diagnostic logging reads on Core 0 are also wrapped in `s_data_mutex` to prevent memory tearing.
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
  - Core 1 Clinical Triage + TinyML inference: **not yet measured on the ESP32-S3.** The host figure is ~0.44 µs for the INT8 hazard model; an ESP32-S3 number has not been instrumented, and none is claimed.
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
- **Direct Answer:** "Skin temperature. Because the TMP117 footprint on our current board revision is unpopulated, we estimate fingertip skin temperature by fusing the MAX30102 on-die temperature sensor (`0x1F`/`0x20`) with the BME280 ambient temperature sensor via a median-of-5 peak-holding thermal convergence filter (`skin_estimate.c`), calibrated against a DT-112 clinical reference thermometer (`34.4–36.4 °C`). Because fingertip skin temperature is a peripheral measurement subject to vasoconstriction and LED package self-heating, we treat it strictly as a display-only indicator and pass `0.0f` to the NEWS2 core-temperature scorer so an estimated peripheral temperature never fabricates a clinical fever or hypothermia alert."
- **Technical Explanation:** The MAX30102 on-die sensor has a slow thermal RC time constant through the optical glass package and ~5.5 °C of steady-state LED self-heating above ambient. Firmware applies a 5-sample median filter to reject single-sample ADC glitches, enforces a cold-boot baseline floor (`ambient + 5.5 °C`), and runs a monotonically non-decreasing peak-hold integrator during warm finger contact (starting at `32.2 °C` and climbing smoothly at `<= +0.28 °C/s` to the settled fingertip equilibrium) while preserving a cooling detection path (`<31.0 °C COLD`) when a cold finger actively cools the die below baseline.
- **Evidence:** `firmware/core/skin_estimate.h`, `firmware/core/skin_estimate.c`, `firmware/zynq/test_disaster_risk_engine.c`, `tools/skin_calib_session.json`.

### Q14.2 (Intermediate): Has the physical fall/SOS path actually been tested?
- **Direct Answer:** "Yes — one physical drop trial on 2026-10-19 was detected, and the full SOS path was exercised: the 10-second countdown ran, the buzzer sounded, and the hold-to-cancel worked (5-second hold on physical button GPIO5, 3-second hold on the web dashboard). It is a single bench trial; false positives and multi-trial/orientation testing are still open, so we call it physically demonstrated, not field-validated."
- **Technical Explanation:** The MPU6050 is sampled at 40 Hz. The 5-state FSM requires a free-fall phase, a high-g impact with rotation, and 1.5 s of post-impact stillness (with a separate soft-fall/posture path). Confirmation calls `sos_arm_fall()`, which starts the 10 s grace window; the OLED shows the countdown and the buzzer chirps. A 5 s hold cancels; otherwise the SOS latches ACTIVE and shows the rescue card.
- **Evidence:** `firmware/shrikefi/fall_detect.c`, `firmware/shrikefi/sos.c`, `firmware/shrikefi/test_fall_detect.c` (16/16 host tests), `docs/HARDWARE_DEBUG_HANDOVER.md`.

---

# Category 15: Adversarial Questions & Innovation Verification

### Q15.1 (Adversarial): "You claim this is an FPGA project, but your FPGA just runs a boxcar filter and peak detector while the ESP32 does all the real work. Isn't the FPGA just a marketing gimmick?"
- **Direct Answer:** "No. Offloading peak detection to hardware is the only reason our Heart Rate Variability metrics are valid. Without the FPGA, peak detection in software would be quantized to the FreeRTOS 5 ms scheduling tick, introducing timing errors larger than the RMSSD signal itself."
- **Technical Explanation:** In a healthy adult, RMSSD is 20–50 ms; in autonomic collapse, it drops below 15 ms. A 5 ms RTOS tick is already the same size as the variation we are trying to measure, which makes software-timestamped HRV meaningless. The FPGA detects peaks with 20 ns resolution directly in hardware, shielding biosignal timing from CPU scheduling, Wi-Fi radio interrupts, and flash cache latency.
- **Evidence:** `docs/MASTER_PROJECT_GUIDE.md:75-105`, `hardware/shrikefi/tb_forgefpga_system.v:310-340`.

---

### Q15.2 (Adversarial): "What would fail if we disconnected the FPGA right now during your demo?"
- **Direct Answer:** "Heart Rate Variability (RMSSD) and Charlton respiratory rate estimation would fail because the cycle-accurate pulse timestamps would disappear. The system would fall back to software thresholding, but telemetry would flag that HRV precision is degraded."
- **Technical Explanation:** `main_shrikefi.c` implements a software fallback path for demonstration resilience (`s_fpga_link_ready`). If the runtime SPI probe fails, Core 0 stops streaming to the FPGA and switches to software peak detection, but RMSSD is suppressed because timing jitter invalidates autonomic strain analysis.
- **Evidence:** `firmware/shrikefi/main_shrikefi.c:712-745`.
