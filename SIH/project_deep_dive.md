# System Architecture & Hardware Deep Dive

This document outlines the raw architecture of the Health Companion SoC. It strips away high-level abstractions to focus on the exact data flow, the register maps, and the physical constraints of the hardware. 

## 1. The Survival Architecture

If you run biometric tracking and DSP filtering on a main Application Processor, the OS thread scheduling introduces 5-20 ms of jitter. When extracting Heart Rate Variability (HRV) to predict heat stroke, a 20 ms timing error completely corrupts the Root Mean Square of Successive Differences (RMSSD) calculation. Garbage in, garbage out. The patient dies while the AI analyzes corrupted data.

To solve this, we physically separate the DSP and timing logic into the Programmable Logic (PL). The FPGA captures the Inter-Beat Interval (IBI) with 20 nanoseconds precision and interrupts the CPU only when valid data is ready.

```
PHYSICAL WORLD                          FPGA FABRIC (PL)                      CPU (PS)                          OUTPUT
═══════════════                         ════════════════                      ════════                          ══════

┌─────────────┐   I2C    ┌──────────────────────────────────┐   AXI4-Lite   ┌───────────────────┐
│  MAX30102   │─────────▶│  axi_ppg_accelerator.v (TOP)     │◄════════════▶│  driver_ppg.c     │
│ (PPG Sensor)│  Red+IR  │  ┌──────────────────────┐       │    Bus       │  (HW register API)│
│ 660nm+940nm │  samples │  │ moving_average_8tap  │×2     │              └────────┬──────────┘
└─────────────┘          │  │ (Red & IR filters)   │       │                       │
                         │  └──────────┬───────────┘       │              ┌────────▼──────────┐
┌─────────────┐          │  ┌──────────▼───────────┐       │              │  hrv_analysis.c   │
│   BME280    │          │  │ ppg_peak_detector    │       │              │ (RMSSD/SDNN from  │
│ (Temp/Hum/  │          │  │ (4-state FSM)        │──irq──╫─────────────▶│  IBI intervals)   │
│  Pressure)  │          │  │ (50MHz cycle timer)  │       │              └────────┬──────────┘
└─────────────┘          │  └──────────────────────┘       │                       │
                         └──────────────────────────────────┘              ┌────────▼──────────┐
┌─────────────┐                                                           │  spo2_engine.c    │
│  PMS5003    │                                                           │ (Beer-Lambert law │
│ (PM2.5 Air  │──UART──────────────────────────────────────────────────▶  │  from Red/IR)     │
│  Quality)   │                                                           └────────┬──────────┘
└─────────────┘                                                                    │
                                                                          ┌────────▼──────────┐
┌─────────────┐                                                           │ disaster_risk_    │     ┌──────────┐
│  SSD1306    │◀──I2C──────────────────────────────────────────────────── │ engine.c          │────▶│ SSD1306  │
│ (0.96" OLED)│                                                           │ (CTSI / PRSI /    │     │ Display  │
└─────────────┘                                                           │  Flood scoring)   │     └──────────┘
                                                                          └───────────────────┘
```

---

## 2. File Map & Subsystem Responsibilities

### 2.1 FPGA Hardware (Verilog RTL)

| File | Purpose | Technical Reality |
|:-----|:--------|:------------------|
| `axi_ppg_accelerator.v` | **Top-level AXI4-Lite slave** | Manages the 32-byte address space. Uses decoupled AW/W handshakes and `aw_done`/`w_done` flags to prevent bus deadlocks. |
| `moving_average_8tap.v` | **DSP noise filter** | O(1) running-sum 8-tap moving average. Uses 0 DSP48 slices and 0 BRAM. Replaces heavy adder trees with an 11-bit accumulator and a `>> 3` hardwired bit-shift. |
| `ppg_peak_detector.v` | **4-state FSM** | Extracts heartbeats and manages the cycle-accurate timer. Uses a 250 ms refractory blanking state to ignore the dicrotic notch. |
| `tb_ppg_system.v` | **Simulation Testbench** | Automated verification of register R/W, staggered handshakes, and W1C clearing using an AXI Bus Functional Model (BFM). |

### 2.2 Embedded C Software (Drivers & AI)

| File | Purpose | Technical Reality |
|:-----|:--------|:------------------|
| `driver_ppg.c` | **FPGA accelerator driver** | Uses volatile pointer casts (`Xil_In32`) to access the memory-mapped registers. Handles the W1C clearing of the `beat_flag` without race conditions. |
| `max30102.c` / `bme280.c` / `pms5003.c` | **Sensor Drivers** | I2C and UART HAL interfaces. Extracts data from the hardware FIFOs. Minimal CPU polling to save power. |
| `hrv_analysis.c` | **HRV Engine** | Maintains a 20-beat circular buffer of IBI intervals. Calculates RMSSD to track parasympathetic nervous system collapse. |
| `spo2_engine.c` | **SpO2 Engine** | Applies the Beer-Lambert ratio (`R = (AC_Red/DC_Red) / (AC_IR/DC_IR)`) to track alveolar hypoxia. |
| `disaster_risk_engine.c` | **Multi-disaster Fusion Engine** | Fuses physiological state with environmental data into composite risk scores (CTSI, PRSI). |
| `nn_risk_model.c` | **TinyML Neural Network** | Replaces static thresholds with non-linear correlations. 123 parameters, 108 MACs. Quantized to INT8 on production hardware. |

---

## 3. Register Map

Memory footprint is minimized. 6 registers over 32 bytes.

```
Offset  Name               Access    Reset       Constraint Notes
──────  ──────────────────  ────────  ──────────  ────────────────────────────────────────────────
0x00    REG_RED_RAW         R/W       0x00000000  Write pushes data to DSP pipeline.
0x04    REG_RED_FILTERED    RO        0x00000000  Smoothed Red output.
0x08    REG_IBI_CYCLES      RO        0x00000000  1 cycle = 20ns.
0x0C    REG_STATUS_THRESH   Mixed     0x00007800  [0] beat_flag (W1C to prevent race condition).
0x10    REG_IR_RAW          R/W       0x00000000  Write pushes data to DSP pipeline.
0x14    REG_IR_FILTERED     RO        0x00000000  Smoothed IR output.
```

---

## 4. Hardware Defensive Posture

**Why decoupling the AXI handshake is mandatory:**
If `AWVALID` and `WVALID` arrive on different clock cycles (as permitted by the ARM AMBA spec), a naive slave that expects them simultaneously will lock up. Our `aw_done` and `w_done` state machine survives arbitrary bus delays.

**Why the First-Beat Guard exists in the FSM:**
On power-up, the IBI counter starts from 0 but hasn't measured a gap between two actual beats. The first peak resets the counter but does NOT output an IBI. Without this, the first data point sent to the AI is garbage (time since power-up), poisoning the HRV buffer.

**Why W1C (Write-1-to-Clear) is non-negotiable:**
If software reads the status register, flips bit 0, and writes it back, a new heartbeat could trigger exactly during those CPU cycles. The heartbeat is erased. W1C pushes the clearing logic down into the hardware, eliminating read-modify-write race conditions.
