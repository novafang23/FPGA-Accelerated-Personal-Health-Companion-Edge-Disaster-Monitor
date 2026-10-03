# Implementation Plan: FPGA PPG Systolic Peak Detector & Signal Processing Pipeline

**Branch**: `001-ppg-systolic-detector` | **Date**: 2026-10-03 | **Spec**: [spec.md](file:///C:/Users/abhin/OneDrive/Desktop/verilog/specs/001-ppg-systolic-detector/spec.md)

**Input**: Feature specification from `specs/001-ppg-systolic-detector/spec.md`

## Summary

Implement and verify a synthesizable digital signal processing pipeline targeting both Renesas ForgeFPGA SLG47910 (1120 LUT budget, SPI target) and Xilinx Zynq-7000 (AXI4-Lite baseline). The pipeline processes 8-bit optical PPG samples at 50 MHz clock with zero DSP and zero BRAM consumption. It combines an 8-tap running sum moving average filter and a 4-state refractory finite state machine for systolic crest detection, coupled across clock domains with 2-stage synchronizers.

## Technical Context

- **Language/HDL**: Synthesizable Verilog IEEE 1364-2001 (vendor-neutral common core).
- **Target Platforms**: 
  - Primary Port: Renesas ForgeFPGA SLG47910 (1120 5-input LUT ceiling, 50 MHz on-chip oscillator).
  - Baseline: Xilinx Zynq-7000 `xc7z020clg400-1` (Vivado ML 2022.2, 50 MHz target).
- **MCU Host**: ESP32-S3 (Xtensa dual-core 240 MHz, single-precision FPU).
- **Interconnect Protocols**:
  - ForgeFPGA: 4-wire SPI Mode 0 target (`spi_sck`, `spi_ss_n`, `spi_mosi`, `spi_miso`) @ 1–16 MHz.
  - Zynq: AXI4-Lite 32-bit slave (`0x43C00000`).
- **Resource Limits**:
  - DSP48 multiplier slices: 0.
  - Block RAM slices: 0.
  - Total ForgeFPGA LUT5s: <= 1120.
  - Total Zynq LUTs: <= 185 LUTs, 16 LUTRAM, 266 FFs.
- **Timing Constraint**: 50.0 MHz system clock ($T = 20\text{ ns}$). Setup slack $\ge +5.0\text{ ns}$.
- **I/O Voltage**: 3.3V LVCMOS strict maximum across all pins.

## Constitution & Invariants Check

- **Invariant 1: Synthesis Resource Budget**: No DSP48 multipliers or Block RAM allowed in the signal pipeline. Shift register and bit-shift division used exclusively.
- **Invariant 2: Clock Domain Crossing (CDC)**: All external inputs (`spi_sck`, `spi_ss_n`, `spi_mosi`) crossing into the 50 MHz FPGA system clock domain must pass through 2-stage D flip-flop synchronizers.
- **Invariant 3: Vendor Neutrality**: Common RTL in `hardware/common/` must be 100% vendor-agnostic (no AXI, no vendor primitives).
- **Invariant 4: Firmware Safety**: ESP32-S3 driver must avoid `double` precision; `s_data_mutex` must protect FreeRTOS shared data structures.

## Project Structure & Source Modules

```text
hardware/
├── common/
│   ├── moving_average_8tap.v    # 8-tap O(1) moving average filter (0 DSP, 0 BRAM)
│   ├── ppg_peak_detector.v      # 4-state refractory systolic peak detector FSM
│   └── tb_ppg_peak_detector.v   # Self-checking unit testbench
├── shrikefi/
│   ├── forgefpga_ppg_top.v      # ForgeFPGA top-level wrapper with SPI target
│   ├── spi_target.v             # 8-bit SPI mode 0 shift register
│   ├── forgefpga_pins.pcf       # Physical constraints file (3.3V LVCMOS)
│   └── tb_forgefpga_system.v    # End-to-end SPI link testbench
└── zynq/
    ├── axi_ppg_accelerator.v    # AXI4-Lite slave wrapper for Zynq-7000
    └── tb_ppg_system.v          # Vivado self-checking baseline testbench (6/6 passing)
```
