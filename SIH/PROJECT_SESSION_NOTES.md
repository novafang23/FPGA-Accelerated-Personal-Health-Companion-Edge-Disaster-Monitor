# SIH26181 — Project Session Summary & Complete Reference Guide
## Qualcomm Hardware Challenge — Smart India Hackathon 2026
**Theme**: MedTech / HealthTech / Disaster Resilience  
**Project**: AI-Powered Personal Health Companion & Edge Disaster Monitor  
**Date**: August 2026

---

## 1. Summary of Actions Completed in This Session

### A. IDE Diagnostics & Build Fixes
- **Root Cause**: The IDE language server (`clangd` / IntelliSense) was missing target compiler paths and defines, causing standard C headers (`<stdio.h>`, `<math.h>`, `<windows.h>`) to show as missing and causing built-in conflicts with `__rdtsc`.
- **Files Configured**:
  - [`.vscode/c_cpp_properties.json`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/.vscode/c_cpp_properties.json): Linked GCC path (`C:/msys64/ucrt64/bin/gcc.exe`), system includes, and Windows defines.
  - [`.clangd`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/.clangd): Configured target triple `--target=x86_64-w64-mingw32`, system include paths, and suppressed duplicate intrinsics.
  - [`compile_commands.json`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/compile_commands.json): Generated compilation database for all 10 C driver files.
  - [`SIH/main_simulation.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/main_simulation.c): Added `WIN32_LEAN_AND_MEAN` guard and removed redundant unused headers.
  - [`SIH/ssd1306.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/ssd1306.c): Cleaned up unused variable and routed `ssd1306_set_contrast` through `ssd1306_send_cmd2`.
- **Status**: C application and all drivers build with **0 errors and 0 warnings**.

---

### B. Hardware Implementation & PC Simulation Suite
- **Verilog RTL Core (`SIH/`)**:
  - [`axi_ppg_accelerator.v`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/axi_ppg_accelerator.v): AXI4-Lite slave engine (`0x00`–`0x14`), decoupled write handshake, W1C status register.
  - [`moving_average_8tap.v`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/moving_average_8tap.v): $O(1)$ running-sum dual-channel DSP filter (0 DSP slices).
  - [`ppg_peak_detector.v`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/ppg_peak_detector.v): 4-state FSM, 50 MHz cycle-accurate IBI interval timer ($20\text{ ns}$ resolution).
  - [`tb_ppg_system.v`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/tb_ppg_system.v): Self-checking testbench covering all 6 system tests (**100% Passed**).
- **Edge AI & TinyML Inference Engine (`SIH/`)**:
  - [`nn_risk_model.h`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/nn_risk_model.h) / [`nn_risk_model.c`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/nn_risk_model.c): 2-layer TinyML Neural Network ($6 \rightarrow 12 \rightarrow 3$ architecture) performing on-device multi-disaster prediction in 108 MACs (< $1\ \mu\text{s}$ latency).
- **PC Tools & Qualcomm Specs Created**:
  - [`SIH/QUALCOMM_PLATFORM_STRATEGY.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/QUALCOMM_PLATFORM_STRATEGY.md): Comprehensive production mapping to Qualcomm Snapdragon Wear (W5+ Gen 1), Hexagon DSP (HVX), Qualcomm AI Engine (QNN/SNPE), and QUP v3 sensor interfaces.
  - [`SIH/build_and_run.bat`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/build_and_run.bat): One-click Windows runner to compile/run simulation, launch GTKWave, and run TinyML C demo.
  - [`run.bat`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/run.bat): Root launcher script.
  - [`SIH/signals.gtkw`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/signals.gtkw): Color-coded, grouped waveform layout for GTKWave.
  - [`SIH/ppg_accelerator.xdc`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/ppg_accelerator.xdc): Timing constraints for Vivado Static Timing Analysis (50 MHz).
  - [`SIH/run_vivado_synth.tcl`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/run_vivado_synth.tcl): Automated Vivado synthesis batch script.
  - [`SIH/HARDWARE_ARCHITECTURE.md`](file:///c:/Users/abhin/OneDrive/Desktop/verilog/SIH/HARDWARE_ARCHITECTURE.md): Microarchitecture specification and interview defense sheet.

---

## 2. Low-Cost Strategy & Student Guidance

- **No Expensive Board Needed**: In the VLSI industry, 95% of digital design/ASIC work is conducted in simulation, synthesis, and STA before silicon exists.
- **₹0 Option**: Use Icarus Verilog + GTKWave for simulation, and free **Vivado ML Standard Edition** for synthesis reports.
- **Physical SIH Prototype**: Connect real sensors (MAX30102, BME280, SSD1306) to an ESP32/Pico (~₹800 total) to stream data to your PC while showcasing the FPGA RTL accelerator.

---

## 3. How to Run Everything on Your PC

1. **All-in-One Menu**:
   ```powershell
   .\run.bat
   ```
2. **Open GTKWave Waveform Viewer Directly**:
   ```powershell
   cd SIH
   C:\iverilog\gtkwave\bin\gtkwave.exe ppg_system.vcd signals.gtkw
   ```
3. **Run C Health & TinyML Simulation Dashboard Directly**:
   ```powershell
   cd SIH
   .\health_demo.exe
   ```

---

## 4. Top Qualcomm & VLSI Interview Defense Talking Points

1. **AMBA AXI4-Lite Protocol Compliance**:
   - Decoupled `AW` (address) and `W` (data) channels with independent `aw_done` and `w_done` latching flags, preventing bus deadlocks on Qualcomm NoC interconnects.
2. **$O(1)$ Area DSP Architecture**:
   - Running-sum moving average ($\text{Sum}[n] = \text{Sum}[n-1] + x[n] - x[n-8]$) with bit-shift division (`>> 3`), requiring **zero DSP48 slices**.
3. **Hardware Precision vs Software Jitter**:
   - 50 MHz FPGA hardware timer captures IBI with **$20\text{ ns}$ precision** and zero CPU load, vs 5–20 ms operating system scheduling jitter in software polling.
4. **On-Device TinyML Neural Network**:
   - 2-layer neural network ($6 \rightarrow 12 \rightarrow 3$) executes 108 MACs on-device with zero cloud dependency. Compiles to Qualcomm AI Engine / Hexagon NPU via Qualcomm Neural Processing SDK (QNN/SNPE) in INT8.
5. **Qualcomm Platform Portability**:
   - Hardware IP and memory map directly translate to Qualcomm Snapdragon Wear 5100 / QCS6490: signal processing to Hexagon DSP (HVX), sensor streams to QUP v3 I2C/UART engines, and AI risk prediction to Hexagon NPU.

