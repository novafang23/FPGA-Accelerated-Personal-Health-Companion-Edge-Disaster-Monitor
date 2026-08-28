# Hardware Architecture Specification

## Overview
The SIH26181 hardware accelerator is an FPGA-based digital signal processing pipeline connected via an ARM AMBA AXI4-Lite slave interface to an ARM Cortex-A9 Processing System (or host MCU).

## Microarchitecture Blocks

### 1. Dual-Channel 8-Tap Moving Average Filter (`moving_average_8tap.v`)
* **Algorithm:** $O(1)$ running sum buffer with hardware bit-shift division (`>> 3`).
* **Formula:**
  $$y[n] = y[n-1] + \frac{x[n] - x[n-8]}{8}$$
* **Resource Optimization:** Utilizes an 8-stage shift register and an accumulator. Requires **0 DSP48 multiplier slices** and **0 Block RAMs**.
* **Throughput:** 1 sample per clock cycle latency on `data_valid`.

### 2. 4-State Systolic Peak Detector & IBI Counter (`ppg_peak_detector.v`)
* **FSM States:**
  * `STATE_ARMED (00)`: Waiting for sample to exceed dynamic threshold.
  * `STATE_RISING (01)`: Tracking rising slope until local peak is detected (`sample_in < prev_sample`).
  * `STATE_PEAK_FOUND (10)`: Latching peak timestamp, asserting `beat_detected` pulse, and capturing IBI cycle interval.
  * `STATE_REFRACTORY (11)`: 250 ms blanking window countdown (`REFRACTORY_CYC`) to reject dichrotic notches and motion artifacts.
* **Timing Resolution:** 50 MHz clock counter $\implies$ **20 ns per tick** resolution.

### 3. AXI4-Lite Slave Wrapper (`axi_ppg_accelerator.v`)
* **Decoupled Handshake:** Independent `AW` (Address Write) and `W` (Data Write) channels prevent out-of-order interconnect deadlocks.
* **Status Register (0x0C):**
  * `Bit [0]`: `beat_flag` with **Write-1-to-Clear (W1C)** to eliminate race conditions between FPGA interrupt assertion and CPU read.
  * `Bits [15:8]`: Dynamically programmable peak detection threshold (default: 120).

## Memory-Mapped Register Map (Base: `0x43C00000`)

| Offset | Register Name | Access | Width | Description |
|---|---|---|---|---|
| `0x00` | `REG_RED_RAW` | R/W | 8-bit | Raw Red optical PPG sample input |
| `0x04` | `REG_RED_FILTERED` | RO | 8-bit | Filtered Red output (for SpO2 / visual) |
| `0x08` | `REG_IBI_CYCLES` | RO | 32-bit | Inter-Beat Interval in 20 ns clock ticks |
| `0x0C` | `REG_STATUS_THRESH` | Mixed | 16-bit | `[0]` `beat_flag` (W1C), `[15:8]` Threshold |
| `0x10` | `REG_IR_RAW` | R/W | 8-bit | Raw IR optical PPG sample input |
| `0x14` | `REG_IR_FILTERED` | RO | 8-bit | Filtered IR output (for SpO2 calculation) |

## Verified Synthesis Baseline (`xc7z020clg400-1`, Vivado ML 2022.2)
* **WNS:** +5.603 ns (Setup Met)
* **WHS:** +0.184 ns (Hold Met)
* **Fmax:** 69.45 MHz (Target: 50.0 MHz)
* **LUT Usage:** 185 (0.35%)
* **LUTRAM:** 16 (0.09%)
* **FF:** 266 (0.25%)
* **DSP48 / BRAM:** 0 / 0
