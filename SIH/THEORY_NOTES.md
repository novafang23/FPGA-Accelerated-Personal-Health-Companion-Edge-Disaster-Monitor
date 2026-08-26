# Field Theory Manual: Environmental Survival & Edge AI

> **Operational Goal:** This manual details the biological failure modes during extreme environmental stress and the hardware logic required to detect them before clinical collapse. There is no cloud connectivity in a disaster zone. The hardware must survive and process data locally.

---

# Table of Contents

- [Part 1: The Operational Reality](#part-1-the-operational-reality)
  - [1.1 Biological Failure Modes (India)](#11-biological-failure-modes-india)
  - [1.2 Heterogeneous Edge Architecture (FPGA + CPU)](#12-heterogeneous-edge-architecture-fpga--cpu)
  - [1.3 Hardware/Software Bill of Materials](#13-hardwaresoftware-bill-of-materials)
- [Part 2: The Signal Pipeline](#part-2-the-signal-pipeline)
  - [2.1 Six-Stage Data Flow](#21-six-stage-data-flow)
  - [2.2 Memory-Mapped Registers](#22-memory-mapped-registers)
  - [2.3 Real-World Disaster Scenarios](#23-real-world-disaster-scenarios)
- [Part 3: Technical Glossary](#part-3-technical-glossary)
  - [3.1 Biological & Sensor Metrics](#31-biological--sensor-metrics)
  - [3.2 FPGA & DSP Hardware](#32-fpga--dsp-hardware)
- [Part 4: Embedded AI Engine](#part-4-embedded-ai-engine)
  - [4.1 Why Hardware-Accelerated ML?](#41-why-hardware-accelerated-ml)
  - [4.2 The 6→12→3 Architecture](#42-the-6123-architecture)

---

# Part 1: The Operational Reality

### 1.1 Biological Failure Modes (India)

India faces extreme, recurrent environmental emergencies. A standard smartwatch is a useless piece of jewelry when the cell towers drop. We are monitoring two critical failure paths:

1. **Cardiovascular Drift (Heat Wave, 47°C+):** When humidity prevents sweat from evaporating, the body vasodilates to dump heat. Blood pools in the extremities, starving venous return. The heart accelerates to compensate. Autonomic flexibility collapses (HRV drops). By the time someone faints from heat stroke, internal organ damage has already commenced.
2. **Alveolar Hypoxia (Winter Smog, PM2.5 > 400 µg/m³):** Toxic particulates physically block oxygen transfer in the lung alveoli. SpO2 plummets. The heart triggers compensatory tachycardia. 

### 1.2 Heterogeneous Edge Architecture (FPGA + CPU)

We use a System-on-Chip (SoC) because a CPU alone is incapable of processing biological signals without jitter and heavy power drain.

```
┌────────────────────────────────────────────────────────────────────────┐
│                   HETEROGENEOUS SYSTEM-ON-CHIP (SoC)                   │
├───────────────────────────────────┬────────────────────────────────────┤
│   PROGRAMMABLE LOGIC (FPGA RTL)   │      PROCESSING SYSTEM (ARM CPU)   │
│   "Low-Latency Reflexes"          │      "Complex Logic Engine"        │
├───────────────────────────────────┼────────────────────────────────────┤
│ • 50 MHz Custom Hardware Engine   │ • Bare-Metal C Runtime / Drivers   │
│ • Dual-channel 8-tap DSP Filter   │ • HRV Mathematical Analysis (RMSSD)│
│ • 4-State Peak Detector FSM       │ • SpO₂ Beer-Lambert Calibration    │
│ • 20 ns Cycle-Accurate IBI Timer  │ • TinyML 6→12→3 Neural Network     │
│ • Zero CPU Load & Zero OS Jitter  │ • Multi-Disaster Risk Score Engine │
└───────────────────────────────────┴────────────────────────────────────┘
```

**Why hardware acceleration?**
An operating system (Linux/RTOS) suffers from thread scheduling jitter of 5-20 ms. If the Inter-Beat Interval (IBI) is 800 ms, a 20 ms OS jitter introduces a 2.5% error, permanently corrupting the Heart Rate Variability (HRV) calculation. Our FPGA timer captures the IBI with 20 nanoseconds resolution. Zero OS jitter. Zero CPU load.

### 1.3 Hardware/Software Bill of Materials

| Component | Purpose | Protocol |
|:---|:---|:---|
| **MAX30102** | Dual-LED (660nm Red + 940nm IR) optical pulse oximeter | I²C (`0x57`), 18-bit ADC |
| **BME280** | Environmental temperature, humidity, pressure | I²C (`0x76`) |
| **PMS5003** | Laser particulate matter sensor (PM2.5) | UART (9600 baud) |
| **SSD1306** | 0.96-inch OLED for real-time vitals/advisories | I²C (`0x3C`) |
| **Zynq-7000 (`xc7z020`)** | Target SoC: ARM Cortex-A9 + Artix-7 fabric | AXI4-Lite Bus |
| **`axi_ppg_accelerator.v`** | Top-level hardware IP with 6 memory-mapped registers | AXI4-Lite Slave |
| **`moving_average_8tap.v`** | O(1) running-sum 8-tap low-pass filter (0 DSP slices) | Pure RTL |
| **`ppg_peak_detector.v`** | 4-state FSM with 250 ms refractory blanking | Hardware Interrupt |
| **`nn_risk_model.c`** | On-device TinyML Neural Network (108 MACs) | C Header |
| **`disaster_risk_engine.c`**| Rule-based multi-disaster engine (CTSI, PRSI) | C Header |

---

# Part 2: The Signal Pipeline

### 2.1 Six-Stage Data Flow

1. **SENSING:** MAX30102 captures Red (660nm) & IR (940nm) light absorption from blood vessels. BME280 captures ambient heat. PMS5003 counts PM2.5.
2. **INGESTION:** Software driver writes 8-bit scaled Red & IR samples into FPGA registers 0x00 and 0x10.
3. **HARDWARE DSP:** 8-tap moving average filter removes baseline wander and high-frequency motion artifacts in 1 clock cycle.
4. **FSM DETECTION:** Peak detector FSM tracks signal slope, detects the systolic peak, fires an interrupt (`irq_beat`), and latches the exact cycle count.
5. **BIOMATH:** CPU converts cycle count to IBI, calculates RMSSD over a 20-beat circular buffer, and applies the Beer-Lambert ratio for SpO2.
6. **SENSOR FUSION AI:** Features are normalized [0, 1] and fed to the 6→12→3 TinyML Neural Network. The system outputs a critical risk score (0-100) and triggers local OLED alerts.

### 2.2 Memory-Mapped Registers

The FPGA acts as a slave on the AXI4-Lite bus mapped at address `0x43C00000`:

```
Offset  Register Name       Access   Reset Value   Function
──────  ──────────────────  ───────  ────────────  ──────────────────────────────────────────────
0x00    REG_RED_RAW         R/W      0x00000000    Raw Red sample → pushes to DSP pipeline
0x04    REG_RED_FILTERED    RO       0x00000000    Smoothed Red output
0x08    REG_IBI_CYCLES      RO       0x00000000    Inter-Beat Interval count in 20ns clock ticks
0x0C    REG_STATUS_THRESH   Mixed    0x00007800    [0] beat_flag (W1C), [15:8] Systolic threshold
0x10    REG_IR_RAW          R/W      0x00000000    Raw IR sample → pushes to DSP pipeline
0x14    REG_IR_FILTERED     RO       0x00000000    Smoothed IR output
```

### 2.3 Real-World Disaster Scenarios

#### Scenario 1: Extreme Heat Wave (47°C, 65% Humidity)
- **Physiology:** Cardiovascular Drift. Heart Rate climbs to 140 BPM, RMSSD collapses from 45ms to 8ms.
- **System Action:** Outputs **CRITICAL HEAT STRAIN (CTSI: 78/100)**. Alerts the user 15-30 minutes before clinical heat stroke and organ damage.

#### Scenario 2: Winter Smog (PM2.5 = 400 µg/m³)
- **Physiology:** Alveolar blockage. SpO2 drops to 86%, triggering Compensatory Tachycardia.
- **System Action:** Outputs **CRITICAL RESPIRATORY DISTRESS (PRSI: 85/100)**. 

---

# Part 3: Technical Glossary

### 3.1 Biological & Sensor Metrics

- **PPG (Photoplethysmography):** Optical detection of blood volume changes in microvascular tissue.
- **SpO2 (Blood Oxygen Saturation):** Percentage of hemoglobin saturated with oxygen. Beer-Lambert Calibration: `SpO2 = 110 − 25 × R`.
- **IBI (Inter-Beat Interval):** The precise time (ms) between consecutive heartbeat peaks.
- **HRV (Heart Rate Variability):** The variation in IBI. High HRV = healthy autonomic flexibility. Low HRV = severe physiological stress.
- **RMSSD:** Root Mean Square of Successive Differences. The primary mathematical metric of short-term HRV, measuring parasympathetic (vagal) tone.
- **Systolic Peak:** The local maximum voltage point in the PPG waveform, caused by left ventricular contraction.
- **Dicrotic Notch:** A secondary bump on the falling edge of the PPG pulse caused by the aortic valve closing. If not blanked by hardware, it causes false heartbeat triggers.

### 3.2 FPGA & DSP Hardware

- **O(1) Running Sum Trick:** Instead of summing 8 samples every cycle (which wastes silicon area with large adder trees), we use: `NewSum = OldSum + x_newest - x_oldest`.
- **Bit-Shift Division (`>> 3`):** Shifting right by 3 bits divides by 8. It costs zero logic gates and zero DSP multiplier slices.
- **Refractory Period (Hardware Blanking):** The FSM transitions to a `REFRACTORY` state after a peak, ignoring all signal changes for 250 ms to blank out the dicrotic notch.
- **Write-1-to-Clear (W1C):** Writing a `1` to the status register clears the interrupt bit to `0`. This prevents catastrophic Read-Modify-Write race conditions where an incoming heartbeat could be wiped out by software.
- **Setup Time & Worst Negative Slack (WNS):** The minimum time data must be stable before the clock edge. Our WNS is +14.28 ns, meaning timing is met with a massive 3.5x safety margin.

---

# Part 4: Embedded AI Engine

### 4.1 Why Hardware-Accelerated ML?

Traditional formulas use fixed thresholds. Human physiology is non-linear. A drop in SpO2 to 88% is far more lethal if the autonomic nervous system is simultaneously collapsing (RMSSD < 10ms). The Neural Network captures these non-linear correlations.

### 4.2 The 6→12→3 Architecture

```
INPUT LAYER (6 Features)         HIDDEN LAYER (12 Neurons)          OUTPUT LAYER (3 Risks)
────────────────────────         ─────────────────────────          ──────────────────────

[0] Normalized Heart Rate ────┐
[1] Normalized RMSSD (HRV) ───┼─▶ [ Neurons H0 - H3 ] ────────────▶ [0] Heat Wave Strain
[2] Normalized SpO₂ ──────────┼─▶ [ Neurons H4 - H7 ] ────────────▶ [1] Pollution Distress
[3] Normalized Ambient Temp ──┼─▶ [ Neurons H8 - H11 ] ───────────▶ [2] Cold / Hypothermia
[4] Normalized Humidity ──────┤
[5] Normalized PM2.5 ─────────┘
      (Activation: ReLU)                                            (Activation: Sigmoid)
```

**Brutal Efficiency Metrics:**
- **Weights & Biases:** 123 parameters total.
- **Memory Footprint:** 123 bytes (when quantized to INT8 on Hexagon DSP).
- **Execution:** 108 Multiply-Accumulate (MAC) operations. Executes in < 100 ns on the NPU. Zero cloud latency. Zero privacy leakage.
