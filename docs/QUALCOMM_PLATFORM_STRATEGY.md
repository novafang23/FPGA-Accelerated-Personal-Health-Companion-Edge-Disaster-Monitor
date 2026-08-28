# Qualcomm Silicon Migration Strategy

## Overview
This document outlines the architectural migration path from the Xilinx Zynq-7000 FPGA prototype to the **Qualcomm® Snapdragon® Wear W5+ Gen 1** hybrid architecture for commercial wearable deployment.

## Subsystem Architectural Mapping

| Prototype Block (Zynq-7000) | Qualcomm Production Subsystem | Migration Path & Benefit |
|---|---|---|
| **Verilog Digital Filter (`moving_average_8tap.v`)** | **Qualcomm Hexagon™ DSP + HVX** | Vectorized SIMD sliding-window filtering on the Low-Power Island (< 1 mW active power). |
| **Peak FSM (`ppg_peak_detector.v`)** | **Qualcomm Sensor Core Hardware Timer** | 64-bit microsecond timestamp counter for jitter-free continuous 24/7 cardiac monitoring. |
| **TinyML Model (`nn_risk_model.c`)** | **Qualcomm AI Engine (Hexagon NPU)** | Quantized to INT8 `.dlc` via SNPE/QNN SDK (< 100 ns inference, < 0.1 mJ energy per pass). |
| **Sensor Drivers (`max30102.c`, `bme280.c`, etc.)** | **Qualcomm Universal Peripheral (QUP v3)** | Direct DMA transfer via Bus Access Manager (BAM) with zero CPU wakeups. |
| **AMBA AXI4-Lite Interface** | **Qualcomm System Network-on-Chip (NoC)** | Native AMBA standard memory-mapped interoperability. |

## Power Budget & Energy Scaling
* **Low-Power Island (< 5 mW):** The Qualcomm Sensor Core & Hexagon DSP run the continuous 50 Hz PPG acquisition, moving average filtering, and systolic peak timestamping without waking the main application processor (Cortex-A53).
* **AI Wakeup on Exertion / Drift:** The 6→12→3 TinyML model executes periodically (e.g. every 10 seconds or when cardiac drift is detected) on the NPU, consuming negligible battery power.
