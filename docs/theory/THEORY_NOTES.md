# SIH26181 Master Theory Notes & Judge Defense Companion

**Project Title:** AI-Powered Personal Health Companion & Edge Disaster Monitor  
**Challenge:** Qualcomm Hardware Challenge — Smart India Hackathon 2026 (Problem SIH26181)  
**Target Platforms:** Xilinx Zynq-7000 (Reference Baseline) ➔ Renesas ForgeFPGA + ESP32-S3 ShrikeFi (Ultra-Low-Cost Wearable) ➔ Qualcomm Snapdragon Wear / QCS6490 (Commercial Road-map)

---

# 1. THE ULTIMATE PITCH GUIDE (Executive Summary)

If you have 3 minutes to pitch this to a panel of judges, hit these exact points in this order:

### 1. What are we solving? (The Problem)
In 2024 alone, extreme heatwaves in India claimed over 700 lives in just three months, while 380 million outdoor workers (75% of the national workforce) faced severe risk of heatstroke, acute kidney injury, and lost wages. Current early-warning systems only monitor the *environment* (e.g., weather apps or city air monitors), leaving vulnerable outdoor workers guessing how much physiological danger their own bodies are actually in.

### 2. How does it relate to the SIH Problem Statement? (Qualcomm Hardware Challenge)
Problem **SIH26181** calls for innovative hardware architectures leveraging Qualcomm platforms for societal impact. We built a hardware-accelerated, edge-AI wearable that acts as a personal disaster shield. It proves custom silicon offloading while solving a massive humanitarian crisis: protecting vulnerable populations during heatwaves, toxic smog, and flood immersion **100% offline with zero cloud dependency**.

*(HARDWARE WOW-FACTOR)*: Most hackathon health wearables run everything in software on a standard microcontroller. We went much deeper: we designed custom digital hardware circuits in synthesizable Verilog that filter noisy optical signals and extract heartbeat intervals physically at the silicon gate level, taking 0 DSP slices and 0 Block RAM.

### 3. Who is doing this in the world? (The Competitor Gap)
* **Fitness Wearables (Apple Watch / Garmin / Fitbit):** Track heart rate in air-conditioned gyms, but have zero awareness if you are standing in 46°C heat or toxic PM2.5 = 400 smog.
* **Environmental Monitors (PurpleAir / Weather Apps):** Measure ambient air quality, but don't know if *your* specific lungs or cardiovascular system are collapsing because of it. Furthermore, they depend on cellular networks that fail during disasters.

### 4. How are we improving / What is our Novelty?
We bridge the "Context Gap" and eliminate "Cloud Vulnerability" via **Hardware-Accelerated Sensor Fusion**:
* **The Fusion:** We fuse 6 biometric and environmental vitals (Heart Rate, RMSSD/HRV, SpO2, Ambient Temperature, Humidity, and PM2.5) locally.
* **The Intelligence:** If ambient heat hits 45°C and humidity prevents sweat evaporation, our INT8 TinyML Neural Network and Rule Engine detect cardiovascular drift and HRV collapse in real-time, sounding a life-saving alarm before heatstroke occurs.

### 5. Who is this for? (Target Demographics)
* **Frontline & Outdoor Workers:** Construction laborers, agricultural workers, traffic police, and disaster relief personnel.
* **Vulnerable Citizens:** Elderly individuals, asthma patients, and individuals with cardiovascular disease during extreme climate events.

### 6. The Vision (Closing the Pitch)
*"We have validated our custom hardware on the Xilinx Zynq-7000 baseline, deployed an ultra-affordable $15 wearable architecture on the Renesas ForgeFPGA + ESP32-S3 ShrikeFi platform, and mapped the full migration path to Qualcomm Snapdragon Wear."*

---

# 2. THE DUAL-FPGA STRATEGY — WHY TWO FPGAs?

A common question from technical judges is: *"Why does your repository have both Xilinx Zynq-7000 and Renesas ForgeFPGA (ShrikeFi) implementations?"*

Here is the exact story and rationale to present:

```
+-----------------------------------------------------------------------------------+
|                            THE TWO-TARGET HARDWARE STORY                          |
+-----------------------------------------------------------------------------------+
|  1. XILINX ZYNQ-7000 (Reference Baseline)                                         |
|     - Role: High-End Prototyping & Cycle-Accurate Verification                    |
|     - Target Part: xc7z020clg400-1 (Vivado ML 2022.2)                             |
|     - Performance: Timing closed @ 69.45 MHz (WNS +5.603 ns, WHS +0.184 ns)       |
|     - Interconnect: 32-bit AXI4-Lite Memory-Mapped Bus                            |
|     - Purpose: Proved silicon math & zero DSP/BRAM footprint                      |
|                                                                                   |
|                                        ▼                                          |
|                                                                                   |
|  2. RENESAS ForgeFPGA + ESP32-S3 (ShrikeFi Target)                                |
|     - Role: Mass-Deployable, Ultra-Low-Cost (<$20 BOM) Pocket Wearable            |
|     - Target Part: SLG47910C (1120 5-input LUTs) + Dual-Core ESP32-S3             |
|     - Utilization: Only 195 / 1120 LUTs (17.41%), 110 FFs, 0 DSP, 0 BRAM          |
|     - Interconnect: Custom 4-Bit Parallel Link (5.0 MB/s max @ 10MHz; 500kHz bring-up)|
|     - Purpose: Proved commercial feasibility for millions of workers              |
|                                                                                   |
|                                        ▼                                          |
|                                                                                   |
|  3. QUALCOMM SNAPDRAGON WEAR (Commercial Road-map)                                |
|     - Role: Mass-Market Integrated Smartwatch SoC (QCS6490 / Snapdragon Wear)     |
|     - Hardware Blocks: Hexagon DSP (PPG filtering) + NPU (INT8 TinyML Model)      |
+-----------------------------------------------------------------------------------+
```

### The "Formula 1 Rig vs. The Pocket Commuter" Analogy
* **Xilinx Zynq-7000 = The Formula 1 Wind Tunnel:** A heavy, multi-hundred-dollar development platform used by aerospace engineers to test aerodynamic wings under extreme conditions. It proved our Verilog RTL was mathematically flawless and met strict 20 ns timing resolution.
* **ShrikeFi (ForgeFPGA + ESP32-S3) = The Mass-Market Commuter:** You cannot hand a $250 development board to a construction worker in Delhi. ShrikeFi pairs a $1.50 Renesas ForgeFPGA with a $3.00 ESP32-S3 to deliver the exact same silicon-level acceleration in a $15 total Bill of Materials (BOM).
* **Vendor-Agnostic Core:** We did **not** rewrite the filter or peak detector for ShrikeFi. The core Verilog math in `hardware/common/` is 100% vendor-agnostic and synthesized directly onto both Xilinx (6-input LUTs) and Renesas (5-input LUTs).

---

# 3. SYSTEM ARCHITECTURE & HARDWARE INTERCONNECTS

### 1. Zynq-7000: The 32-Bit AXI4-Lite Register Bus
On the Zynq SoC, the ARM Cortex-A9 CPU communicates with the FPGA fabric over an AMBA AXI4-Lite memory-mapped bus (`0x43C00000`):
* **Direct Register Map:** `REG_RED_RAW` (0x00), `REG_RED_FILTERED` (0x04), `REG_IBI_CYCLES` (0x08), `REG_STATUS_THRESH` (0x0C).
* **Decoupled Handshake:** Independent Address Write (`AW`) and Data Write (`W`) channels prevent interconnect deadlocks.
* **Write-1-to-Clear (W1C):** Bit [0] of `REG_STATUS_THRESH` latches when a heartbeat occurs and clears atomically on write, eliminating CPU race conditions.

### 2. ShrikeFi: The 4-Bit Parallel Nibble Link
The Renesas ForgeFPGA is a compact chip with limited I/O pins. A 32-bit AXI bus is physically impossible. We designed an ultra-efficient **4-bit parallel nibble link**:

![ShrikeFi Pinout & Interconnect Diagram](../images/shrikefi_pinout.png)

* **Physical Wires (3.3V LVCMOS):**
  * `mcu_data_in[3:0]`: 4-bit data bus from ESP32-S3 to FPGA (samples & commands).
  * `fpga_data_out[3:0]`: 4-bit data bus from FPGA to ESP32-S3 (IBI timestamps & status).
  * `nibble_strobe`: Clock/handshake line toggled by MCU on each 4-bit transfer.
  * `fpga_irq`: Active-high hardware interrupt asserting on detected R-peaks.

#### How Data Travels Across 4 Pins:
* **Writing an 8-bit PPG Sample:**
  1. Send Command Nibble (`0x1` for Red, `0x2` for IR). Toggle strobe.
  2. High Nibble: Send bits `[7:4]`. Toggle strobe.
  3. Low Nibble: Send bits `[3:0]`. Toggle strobe.
  4. FPGA Link Receiver FSM inside `forgefpga_ppg_top.v` latches the full byte and triggers the moving-average filter.
* **Reading a 32-bit IBI Timestamp:**
  1. The FPGA asserts `irq_beat` (GPIO 10) when a heart peak occurs.
  2. ESP32-S3 sends `CMD_READ_IBI` (`0x6`), switches direction to Read, and reads 8 consecutive 4-bit nibbles (`[31:28]` down to `[3:0]`).
  3. Link Latency & Throughput:
     - **Measured Bring-Up Driver:** At ~500 kHz software bit-banging (~2 µs strobe period), reading all 9 cycles takes **~18 µs**—occupying <0.1% of the 20,000 µs (50 Hz) optical sampling period.
     - **Protocol Max (Hardware-Timer / SPI-Assisted Target):** At 10 MHz strobe rate ($T_{\text{strobe}} = 100\text{ ns}$), reading 9 cycles takes **900 ns** (5.0 MB/s raw bandwidth).

![ShrikeFi 4-Bit Parallel Link Protocol Timing Waveform](../images/shrikefi_waveform.png)

### 3. ESP32-S3 Dual-Core FreeRTOS Partitioning
The ESP32-S3 contains two 240 MHz Xtensa LX7 cores. We strictly partitioned the tasks using FreeRTOS core pinning:

```
+------------------------------------+    +------------------------------------+
|         CORE 0: ACQUISITION        |    |       CORE 1: INTELLIGENCE & UI    |
|   (The Dedicated Paramedic)        |    |      (The AI Chief Physician)      |
+------------------------------------+    +------------------------------------+
| • 50 Hz MAX30102 Optical Sampling  |    | • BME280 (Temp/Hum) & PMS5003      |
| • 4-Bit FPGA Link Streaming        |    | • INT8 Quantized TinyML Inference  |
| • Cycle-accurate IBI IRQ capture   |    | • Rule Engine (CTSI / PRSI Risk)   |
| • Rolling HRV Calculation (RMSSD)  |    | • SSD1306 OLED Real-Time Display   |
+------------------------------------+    +------------------------------------+
                  │                                         ▲
                  └─────── Thread-Safe Mutex Queue ─────────┘
```

---

# 4. THE CUSTOM VERILOG RTL — EXPLAINED FOR NON-HARDWARE PEOPLE

Digital hardware circuits do not "execute instructions" like C code or Python. They are physical arrays of logic gates, flip-flops, and wires operating simultaneously at 50,000,000 clock cycles per second.

```
                      RAW PPG SAMPLES (50 Hz)
                                 │
                                 ▼
                     +───────────────────────+
                     |  moving_average_8tap  |  <--- 8-Tap O(1) Running Sum Filter
                     |  (No DSP, No BRAM)    |       Smooths wrist motion artifacts
                     +───────────────────────+
                                 │
                                 ▼
                     +───────────────────────+
                     |   ppg_peak_detector   |  <--- 4-State Systolic FSM
                     |   (20 ns Resolution)  |       Armed -> Rising -> Peak -> Refractory
                     +───────────────────────+
                                 │
                 ┌───────────────┴───────────────┐
                 ▼                               ▼
       [ ZYNQ-7000 TARGET ]            [ SHRIKEFI TARGET ]
    axi_ppg_accelerator.v              forgefpga_ppg_top.v
    (32-bit AXI4-Lite Bus)             (4-bit Parallel Nibble Bus)
```

### The Core Hardware Modules & Architecture:

1. **`moving_average_8tap.v` (`hardware/common/` — The Smart Filter):**
   * *What it does:* Smooths high-frequency noise from skin contact and tremors on both Red and IR channels.
   * *Mathematical Formula:* $y[n] = y[n-1] + \frac{x[n] - x[n-8]}{8}$
   * *Why it's clever:* A standard filter adds 8 numbers every time. Our $O(1)$ running-sum filter only subtracts the oldest sample and adds the newest sample, using a simple bit-shift (`>> 3`) instead of a divider. Takes **0 DSP multiplier slices** and executes in **1 clock cycle (20 ns)**.

2. **`ppg_peak_detector.v` (`hardware/common/` — The Systolic Peak Radar):**
   * *What it does:* Tracks systolic pressure waves and measures the Inter-Beat Interval (IBI) between consecutive heartbeats with **20-nanosecond accuracy**.
   * *The 4-State Flowchart:*
     1. `STATE_ARMED (00)`: Waiting for the signal to rise above the dynamic threshold.
     2. `STATE_RISING (01)`: Tracking the rising slope until the sample drops (`sample < prev_sample`), confirming the local maximum.
     3. `STATE_PEAK_FOUND (10)`: Captures the exact clock cycle count, asserts `beat_detected`, and resets the timer.
     4. `STATE_REFRACTORY (11)`: Enforces a 250 ms blanking window so the dicrotic notch (secondary rebound wave in arteries) does not trigger a false second heartbeat.

3. **`axi_ppg_accelerator.v` (`hardware/zynq/` — The Zynq AXI4-Lite Wrapper):**
   * *What it does:* Memory-mapped AXI4-Lite slave wrapper for the Xilinx Zynq-7000 SoC (`0x43C00000`), providing register-level access (`REG_RED_RAW`, `REG_IBI_CYCLES`, `REG_STATUS_THRESH`) and Write-1-to-Clear interrupt management.

4. **`forgefpga_ppg_top.v` (`hardware/shrikefi/` — ShrikeFi Top-Level Gateway & Link Engine):**
   * *What it does:* Integrates the dual 8-tap moving-average filters and systolic peak detector with a custom 4-bit parallel link transceiver FSM inside the Renesas ForgeFPGA (SLG47910).
   * *Internal FSM Sub-Blocks:*
     * **Command Decoder & Nibble Reassembler (RX Stage):** Decodes command headers (`CMD_WRITE_RED`, `CMD_WRITE_IR`, `CMD_WRITE_THRESH`) and combines sequential 4-bit nibbles into 8-bit sample bytes for DSP execution.
     * **32-Bit IBI Serializer (TX Stage):** Latches the 32-bit `peak_ibi_cycles` timestamp on a heartbeat and streams it across the 4-bit bus as 8 sequential nibbles (`[31:28]` down to `[3:0]`) upon receiving `CMD_READ_IBI`.
   * *Resource Footprint:* Uses only **195 out of 1120 LUT5s (17.41%)**, **110 Flip-Flops**, and **35 CLBs**, leaving >82% of the chip available.

![Renesas ForgeFPGA Workshop GUI Resources Report](../images/forgefpga_resources_report.png)

![Renesas SLG47910C Chip Top-Level Schematic](../images/forgefpga_chip_schematic.png)

![Renesas ForgeFPGA Resource Footprint](../images/forgefpga_utilization.png)

---

# 5. THE SENSOR SUITE — WHAT EACH SENSOR DOES & WHY

| Sensor | Vital Metric Measured | Why It Is Essential for Disasters |
|---|---|---|
| **MAX30102** | Red & IR Photoplethysmography (PPG), Blood Oxygen (SpO2), On-Chip Die Temperature | Detects heart rate, Heart Rate Variability (HRV), hypoxemia from smoke inhalation, and skin microclimate cooling during flood immersion. |
| **BME280** | Ambient Temperature (°C) & Relative Humidity (%) | Calculates the Steadman Heat Index. High humidity prevents sweat evaporation, causing lethal core temperature spikes even at 38°C. |
| **PMS5003** | Fine Particulate Matter (PM2.5 in $\mu\text{g}/\text{m}^3$) | Measures microscopic smoke and dust particles small enough to penetrate lungs into the bloodstream during crop fires, smog, and wildfires. |
| **SSD1306** | 128×64 Monochrome OLED Display | Provides real-time visual triaging (Green/Yellow/Orange/Red) directly on the worker's wrist without needing a paired phone or cellular signal. |

---

# 6. THE SOFTWARE & EDGE AI (TinyML) SIDE

Our firmware features a **Hybrid Risk Assessment Engine** combining deterministic clinical rules with an ultra-fast INT8 Quantized Neural Network.

```
       [ 6-INPUT SENSOR FUSION VECTOR ]
  ┌───────────────────────────────────────────────┐
  │ 1. Heart Rate (BPM)    4. Ambient Temp (°C)   │
  │ 2. HRV RMSSD (ms)      5. Relative Hum (%)    │
  │ 3. SpO2 Saturation (%) 6. PM2.5 (ug/m3)       │
  └──────────────────────┬────────────────────────┘
                         │
        ┌────────────────┴────────────────┐
        ▼                                 ▼
┌──────────────────────────────┐  ┌──────────────────────────────┐
│     CLINICAL RULE ENGINE     │  │   INT8 QUANTIZED TinyML NN   │
│  (disaster_risk_engine.c)    │  │   (nn_risk_model_int8.c)     │
├──────────────────────────────┤  ├──────────────────────────────┤
│ • Cardio-Thermal Strain (CTSI│  │ • 6 Inputs -> 12 Hidden -> 3 │
│ • Pollution Strain (PRSI)    │  │ • INT8 Weights & Activations │
│ • Deterministic Safety Net   │  │ • < 1 microsecond inference  │
└──────────────┬───────────────┘  └──────────────┬───────────────┘
               │                                 │
               └───────────────┬─────────────────┘
                               ▼
            [ UNIFIED TRIAGE & ADVISORY OUTPUT ]
            • Normal (Green)     • High (Orange)
            • Moderate (Yellow)  • Critical (Red Alert)
```

### 1. The Clinical Rule Engine (`disaster_risk_engine.c`)
Calculates medical indices developed specifically for occupational heat and smog strain:
* **Cardio-Thermal Strain Index (CTSI):** Combines ambient heat index with cardiovascular drift ($\Delta\text{HR}$) and HRV collapse. If Temp > 42°C and HR > 130 BPM with RMSSD < 10 ms, flags **CRITICAL HEAT RISK**.
* **Pollution Respiratory Strain Index (PRSI):** Fuses PM2.5 particulate concentration with blood oxygen desaturation. If PM2.5 > 300 $\mu\text{g}/\text{m}^3$ and $\text{SpO}_2 < 88\%$, flags **CRITICAL POLLUTION RISK**.

### 2. INT8 Quantized Neural Network (`nn_risk_model_int8.c`)
* **Architecture:** Multi-Layer Perceptron (6 Input neurons $\rightarrow$ 12 Hidden neurons with ReLU $\rightarrow$ 3 Output neurons with Sigmoid: Heat, Pollution, and Flood risk scores).
* **INT8 Quantization:** All floating-point operations were converted to 8-bit integer matrix multiplications using pre-calculated scale factors and zero-points. Inference takes **under 1 microsecond** on an ESP32-S3 or ARM CPU, consuming negligible battery.
* **Knowledge Distillation Training:**
  * The deterministic Rule Engine served as the **"Teacher"**.
  * We synthesized 50,000 extreme multi-variable disaster scenarios and trained the **"Student"** (Neural Network) to mimic the clinical score gradient.
  * Achieved **83.44% classification accuracy** on a 5,000-scenario unseen validation test set while running 10× faster than full floating-point evaluation.

---

# 7. KILLER ANALOGIES CHEAT-SHEET (For Judges & Team Defense)

Use these exact analogies when explaining the project to non-technical judges or team members:

### 1. The "CEO and the Automated Assembly Line" (Why FPGA Acceleration?)
> *"Think of the microcontroller CPU as a brilliant CEO. If you force the CEO to manually inspect 50 raw optical heartbeat readings every second, they have to stop everything, do math, and burn battery. They become overwhelmed and have no time to make big decisions.  
> Instead, we used the FPGA fabric to build an **automated factory conveyor belt**. The raw light signals pass through our hardware filter and peak detector, which smooth the wave and count the pulse intervals in silicon. The hardware conveyor belt simply hands the CEO a finished note saying: 'Heart rate is 128, IBI is 468 ms.' Now the CEO's entire brain is free to run our TinyML Neural Network."*

### 2. The "Formula 1 Wind Tunnel vs. The Pocket Commuter" (Why Two FPGAs?)
> *"The Xilinx Zynq-7000 was our Formula 1 wind tunnel: an industrial-grade testing platform where we proved our custom Verilog circuits were mathematically sound and closed timing at 69.45 MHz.  
> But you can't give a $250 development board to an agricultural worker in rural Bihar. The ShrikeFi platform (Renesas ForgeFPGA + ESP32-S3) is our production pocket commuter: it runs the exact same Verilog filter logic on a $1.50 micro-FPGA, cutting the entire device cost under $20 while retaining hardware acceleration."*

### 3. The "Smart Elevator Scale" (Why is the Filter $O(1)$?)
> *"Imagine calculating the average weight of 8 people inside an elevator. When a new person steps in and the oldest person leaves, a naive algorithm asks all 8 people to step on the scale again, adds up their weights, and divides by 8. That wastes time.  
> Our $O(1)$ hardware filter acts like a smart scale: it remembers the previous running sum, subtracts the weight of the person who left, and adds the new person. It does this in a single 20-nanosecond clock cycle, regardless of how large the filter window is."*

### 4. The "4-Lane Walkie-Talkie" (How the ShrikeFi 4-bit Link Works)
> *"On large computer chips, components talk over 32 separate parallel copper tracks (like a 32-lane highway). On our compact micro-FPGA, we only have 4 data pins.  
> We built a 4-lane high-speed walkie-talkie: when the ESP32 needs to send an 8-bit sensor reading, it sends the command code, slices the sample into two 4-bit 'nibbles', sends the first half, rings a digital doorbell (strobe), sends the second half, and rings the bell again. The FPGA snaps the two halves together in hardware in single-digit microseconds during software bring-up, and under 300 nanoseconds at maximum protocol clocking."*

### 5. The "Paramedic & The AI Chief Physician" (Dual-Core FreeRTOS Partitioning)
> *"On the ESP32-S3, Core 0 is our dedicated paramedic in the ambulance: it never leaves the patient, sampling optical vitals at 50 Hz and exchanging packets with the FPGA link without ever dropping a beat.  
> Core 1 is the Chief Physician at the hospital: it takes the clean data, gathers environmental readings, runs the AI Neural Network, calculates disaster risk scores, and updates the wrist display."*

### 6. The "Teacher and the Student" (How the AI was Trained)
> *"Our clinical Rule Engine is a strict medical professor: 100% accurate according to published papers, but heavy and slow to calculate.  
> Our Neural Network is an eager student. We gave the student 50,000 flashcards of extreme heatwaves and toxic smog scenarios. The professor graded every flashcard. By learning from its mistakes over 100 epochs of gradient descent, the student learned to make the exact same life-saving triage decisions in under 1 microsecond."*

---

# 8. LIKELY JUDGE QUESTIONS & WINNING DEFENSE ANSWERS

### Q1: "Why did you build custom Verilog RTL instead of just doing everything in C on the ESP32-S3?"
* **Answer:** *"Three reasons: Determinism, CPU offloading, and Power. Software signal processing on an RTOS suffers from interrupt latency and task jitter when WiFi or sensor interrupts fire. Our FPGA accelerator processes raw PPG samples at hardware clock speeds with zero CPU overhead, allowing the microcontroller to stay in low-power sleep modes between assessments."*

### Q2: "How did you manage to fit your design into Renesas ForgeFPGA's tiny 1120 LUT capacity?"
* **Answer:** *"Our architecture was designed from day one to be ultra-lean. By using an $O(1)$ running-sum filter with bit-shift division and an accumulator-based peak detector, our entire ShrikeFi design consumes just **195 LUTs (17.41%)** and **110 flip-flops**, with zero DSP multiplier blocks and zero Block RAM. Over 82% of the FPGA remains free."*

### Q3: "What makes your 4-bit link better than standard SPI or I2C?"
* **Answer:** *"I2C is too slow (400 kHz) with heavy bus addressing, and SPI has protocol framing overhead. Our custom 4-bit parallel nibble link provides a dedicated hardware strobe and direct register-level latching. In our current bit-banged bring-up driver, a complete 32-bit IBI timestamp transfer takes only **18 microseconds** (occupying less than 0.1% of the 50 Hz optical sampling window). At our verified 10 MHz simulation target with hardware-assisted strobing, it transfers in **900 nanoseconds** (5.0 MB/s raw bandwidth) with zero protocol bloat."*

### Q4: "What happens if the PM2.5 air quality sensor gets clogged or malfunctions?"
* **Answer:** *"Our system uses cross-sensor validation. If the PMS5003 reports hazardous PM2.5 = 500 but the user's blood oxygen is a healthy 99% and heart rate is 65 BPM, our sensor fusion engine recognizes the biometric mismatch, suppresses panic sirens, and flags a 'Sensor Check Advisory' on the OLED display."*

### Q5: "How does this project transition to Qualcomm Snapdragon Wear?"
* **Answer:** *"Our architecture is explicitly decoupled for platform migration. The Verilog DSP pipeline in `hardware/common/` maps directly onto Qualcomm's Hexagon DSP core, while our C-based INT8 Neural Network executes natively on the Qualcomm Snapdragon Neural Processing Engine (SNPE) / NPU without architectural changes."*

---

# 9. CLINICAL REFERENCES & SCIENTIFIC FOUNDATIONS

| Clinical Index | Scientific Source | Formulation & Thresholds in SIH26181 |
|---|---|---|
| **Steadman Heat Index** | Robert G. Steadman (1979), *"The Assessment of Sultriness"*, Journal of Applied Meteorology. | Combines Ambient Temp + Humidity. Index $> 40^\circ\text{C}$ triggers Caution; $> 54^\circ\text{C}$ triggers Critical Heat Hazard. |
| **Cardio-Thermal Strain (CTSI)** | Moran et al., Physiological Strain Index (PSI) adapted for multi-sensor edge wearables. | Fuses Heat Index with Cardiovascular Drift ($\text{HR} > 130\text{ BPM}$) and HRV collapse ($\text{RMSSD} < 10\text{ ms}$). |
| **Pollution Respiratory Strain (PRSI)** | WHO Global Air Quality Guidelines (2021) & EPA AQI Technical Assistance Document. | Fuses $\text{PM2.5} > 300\,\mu\text{g}/\text{m}^3$ (EPA "Hazardous" ceiling) with clinical hypoxemia ($\text{SpO}_2 < 88\%$). |
| **Hypothermia & Cold Shock** | Golden & Tipton, *"Essentials of Sea Survival"*, Cold Water Immersion Stages. | Tracks skin temperature proxy $< 32^\circ\text{C}$ combined with initial cold-shock tachycardia followed by severe bradycardia ($\text{HR} < 50\text{ BPM}$). |

---

# 10. GLOSSARY OF TERMS

* **AXI4-Lite:** Advanced eXtensible Interface; standard memory-mapped point-to-point bus protocol for ARM SoC chips.
* **CTSI:** Cardio-Thermal Strain Index; our custom index quantifying physiological heat stress.
* **FSM:** Finite State Machine; a sequential digital hardware circuit transitioning between distinct operating states.
* **IBI:** Inter-Beat Interval; the exact elapsed time (in milliseconds or 20 ns clock ticks) between consecutive R-wave peaks.
* **INT8 Quantization:** Compressing 32-bit floating-point neural network weights into 8-bit signed integers for high-speed integer ALU execution.
* **Nibble:** A 4-bit aggregation of binary data (half an 8-bit byte).
* **PRSI:** Pollution Respiratory Strain Index; our custom index quantifying respiratory distress during smoke/smog events.
* **RMSSD:** Root Mean Square of Successive Differences; the clinical gold standard for measuring parasympathetic Heart Rate Variability (HRV).
* **SoC:** System on Chip; an integrated circuit combining CPU cores, memory, peripherals, and FPGA fabric.
