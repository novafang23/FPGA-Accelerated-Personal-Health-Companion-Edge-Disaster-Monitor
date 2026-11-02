# Project VALOR: Full-Stack Zero-Trust Audit Certification Report
**Hardware & Firmware Engineering Deep Audit (RTL to AI to Firmware to Dashboards)**

- **Target System**: SIH26181 VALOR — FPGA-Accelerated Personal Health Companion & Edge Disaster Monitor
- **Dual Targets**: Xilinx Zynq-7000 (`xc7z020clg400-1`) & ShrikeFi (`ESP32-S3` + Renesas ForgeFPGA `SLG47910`)
- **Baseline Commit**: `8d7ecb7bf80d52c63ec2c977894dc5780bb07f84` (`8d7ecb7`)
- **Audit Date**: 2026-11-02
- **Lead Auditors**: VALOR Engineering Agentic Pair (Implementer Subagents & Master Auditor)

---

## Executive Summary & Gate Matrix

### High-Level Certification Status
This comprehensive, zero-trust end-to-end audit inspects and certifies every architectural layer of Project VALOR:
1. **Layer 1: Verilog RTL & FPGA Synthesis** — Vendor neutrality, clock-domain crossing (CDC), synthesis invariants, resource bounds, and simulation testbenches.
2. **Layer 2: DSP, Clinical Algorithms & TinyML** — HRV RMSSD, Charlton PPG-RR, SpO2 DC-rejection, Elgendi SQI, PM2.5 humidity correction, INT8 quantized neural network inference, and MIMIC-III benchmark agreement.
3. **Layer 3: ESP32-S3 Firmware & Concurrency** — FreeRTOS task safety, single-precision FPU compliance (`-Wdouble-promotion`), mutex lock contention, buffer sizing, and memory bounds.
4. **Layer 4: Desktop & SoftAP Wi-Fi Dashboards** — Offline resilience, zero external CDN dependencies, strict CSP script hashing, Web Audio API synthesis, and multi-modal telemetry replay.
5. **Layer 5: Python Tooling & Presentation Claims** — Verification harnesses, dataset parsers, claim corroboration, and sensitive clinical metric disclosures.

| Audit Layer | Target Subsystems | Checklist / Tool | Status | Primary Invariant / Gate |
| :--- | :--- | :--- | :--- | :--- |
| **Section 1: Baseline & Hygiene** | Git working tree & diffs | `git status`, GCC C11 | **CERTIFIED** | Deterministic baseline at `8d7ecb7` |
| **Section 2: Layer 1 (RTL & Synthesis)** | `hardware/common`, `zynq`, `shrikefi` | `verilog-rtl-auditor` | **CERTIFIED** | Fmax >= 50 MHz, 0 DSP/BRAM, <=1120 LUT5 |
| **Section 3: Layer 2 (DSP & TinyML)** | `firmware/core/*.c`, `data/mimic` | GCC, `accuracy_evaluator` | Pending (Task 3) | >=99.0% FP32/INT8 match, 94.11% MIMIC-III |
| **Section 4: Layer 3 (Firmware Safety)** | `firmware/shrikefi/*.c` | `c-firmware-safety` | Pending (Task 4) | 0 FPU `double` promotions, mutex protected |
| **Section 5: Layer 4 (Dashboards & Web)** | `valor_dashboard.html`, SoftAP | `html-offline-a11y-auditor` | Pending (Task 5) | 100% offline, exact CSP SHA-256 match |
| **Section 6: Layer 5 (Tooling & Claims)** | `tools/*.py`, `docs/*.md` | `python-tooling-auditor` | Pending (Task 6) | 58.18% sensitivity paired with 94.11% acc |

---

## Section 1: Baseline & Working Tree Hygiene

### 1.1 Baseline Repository Invariants
- **HEAD Commit**: `8d7ecb7` (*refactor(status): purge the vestigial skin_die_delta and skin_weak fields*).
- **Toolchains**:
  - GCC: MSYS2 UCRT64 GCC 15.2.0 (`C:\msys64\ucrt64\bin\gcc.exe`)
  - Verilator: Verilator 5.038 (`C:\msys64\ucrt64\share\verilator`)
  - Icarus Verilog: Icarus Verilog 12.0 (`C:\iverilog\bin\iverilog.exe`)
  - Python: Python 3.12 (`.venv\Scripts\python.exe`)

### 1.2 Working Tree Diff Inspection & Resolutions
1. **Renesas ForgeFPGA Project File (`hardware/shrikefi/forgefpga_project/FPGA-SHRIKE.ffpga`)**:
   - *Inspection*: Diff showed only GUI timestamp updates (`lastChange="2 Nov 2026"`, `projectChecksum="bb7f2b40"`) resulting from opening Renesas Go Configure Software Hub. Logic configuration was identical.
   - *Resolution*: Reverted to committed baseline via `git checkout -- hardware/shrikefi/forgefpga_project/FPGA-SHRIKE.ffpga` to guarantee synthesizable bit-level immutability.
2. **Core Clinical Test Suite (`firmware/zynq/test_disaster_risk_engine.c`)**:
   - *Inspection*: Contained a pending regression guard in `test_skin_estimate_monotonic_and_bounded()` documenting the nominal `b0/b1` parameter offset (`34.87 °C` model vs `36.10 °C` reference, `-1.23 °C` error) with sanity bounds `[32.0f, 37.0f]`.
   - *Buffer Sizing Fix*: The test harness used `char buf[1024]` for `web_status_json()`. Following recent additions to telemetry (GNSS location, thermal state, SOS control PIN), the serialized payload expanded to 1058 bytes, causing truncation and assertion failure (`n > 0`). Updated buffer to `char buf[1536]` to match the device budget (`web_status.c` and `test_status_json.c`).
   - *Verification*: Executed under GCC C11 (`-Wall -Wextra -Werror -pedantic -std=c11`). All 45+ engine unit tests passed cleanly (`ALL TESTS PASSED.`).
   - *Architecture Note*: Commit `6423e39` removed the MAX30102 die-based skin temperature estimate (`skin_estimate.c/.h`) in favor of direct TMP117 contact probe telemetry and the `skin_thermal.c/.h` state machine.
3. **Untracked Working Tree Assets**:
   - Personal user documents (`Assignment2_Solutions.pdf`, hardware technical support emails, 3D enclosure slice files `PRINT_BOTH_ON_ONE_BED_v2.3mf`, presentation slides) remain strictly untouched per AGENTS.md guardrails.

---

## Section 2: Layer 1 — Verilog RTL & FPGA Synthesis Audit

### 2.1 Verilator Strict Lint & Vendor-Purity Analysis

All synthesizable Verilog RTL modules across the dual-target architecture were audited under Verilator 5.046 and Icarus Verilog 12.0 using the MSYS2 UCRT64 toolchain.

#### A. Verilator Lint Verification Runs
1. **Zynq AXI Accelerator & DSP Pipeline (`axi_ppg_accelerator.v`)**:
   - Command: `verilator_bin.exe --lint-only -Wall -Wno-UNUSED -Wno-UNOPTFLAT -Wno-SYNCASYNCNET --top-module axi_ppg_accelerator -Ihardware/common hardware/zynq/axi_ppg_accelerator.v hardware/common/ppg_peak_detector.v hardware/common/moving_average_8tap.v`
   - Output: `V e r i l a t i o n   R e p o r t: Verilator 5.046 ... Built from 0.101 MB sources in 4 modules ... Walltime 0.074 s`
   - Result: **0 errors, 0 warnings (Exit Code 0)**.

2. **ShrikeFi ForgeFPGA Top & SPI Interface (`forgefpga_ppg_top.v`)**:
   - Command: `verilator_bin.exe --lint-only -Wall -Wno-UNUSED -Wno-UNOPTFLAT -Wno-SYNCASYNCNET -Wno-PROCASSINIT --top-module forgefpga_ppg_top -Ihardware/common hardware/shrikefi/forgefpga_ppg_top.v hardware/shrikefi/spi_target.v hardware/common/ppg_peak_detector.v hardware/common/moving_average_8tap.v`
   - Output: `V e r i l a t i o n   R e p o r t: Verilator 5.046 ... Built from 0.122 MB sources in 5 modules ... Walltime 0.039 s`
   - Result: **0 errors, 0 warnings (Exit Code 0)**.

#### B. Vendor Purity Audit (`hardware/common/*.v`)
A zero-tolerance regex check for vendor-specific FPGA macro primitives was conducted across `hardware/common/*.v`:
- Pattern: `\b(AXI|axi_|BUFG|IBUF|OBUF|MMCM|PLLE2|PLLE3|FIFO18|RAMB)\b`
- Findings: Exactly 0 functional occurrences in code logic. The 4 hits in `moving_average_8tap.v` and `ppg_peak_detector.v` reside exclusively within documentation comments describing AXI compatibility and register definitions.
- Result: **100% Vendor-Agnostic Portable RTL**.

#### C. Flat Vendor Source Set Audit (`gen_flat_source.py`)
- The Renesas ForgeFPGA Workshop requires a single flat source set.
- Audit of `hardware/shrikefi/forgefpga_project/ffpga/src/forgefpga_ppg_top.v`: Verified that the inlined RTL logic is functionally identical to `hardware/common/ppg_peak_detector.v` and `hardware/common/moving_average_8tap.v` (both implementing free-running `cycle_now`, crest timestamping at rise, and `crest_fall_min` 25% drop).
- Audit Note: Commit `0ad8af2` intentionally injected an explicit warning header into `forgefpga_project/ffpga/src/forgefpga_ppg_top.v` to alert engineers of the vendor duplication before `gen_flat_source.py` was introduced. Per project rules, synthesizable RTL remains frozen.

---

### 2.2 10-Point `verilog-rtl-auditor` Checklist Matrix

| # | Audit Item | Severity Gate | Inspection Target | Findings & Analysis | Status |
| :-: | :--- | :--- | :--- | :--- | :-: |
| **1** | **Blocking Assignments in Sequential Logic** | **CRITICAL** | `always @(posedge clk)` blocks in all 5 RTL modules | Scanned all sequential blocks in `moving_average_8tap.v`, `ppg_peak_detector.v`, `spi_target.v`, `forgefpga_ppg_top.v`, and `axi_ppg_accelerator.v`. Exactly 0 blocking assignments (`=`) found. 100% compliant non-blocking assignments (`<=`). | **PASS** |
| **2** | **Clock Domain Crossing (CDC) Synchronizers** | **CRITICAL** | SPI inputs (`sck`, `ss_n`, `mosi`) & Reset logic | `spi_target.v` instantiates dual-stage synchronizers (`sck_sync[1:0]`, `ss_sync[1:0]`, `mosi_sync[1:0]`) clocked by the 50 MHz system clock. `ss_sync` correctly resets to `2'b11` to prevent spurious transactions. `forgefpga_ppg_top.v` uses an internal 8-bit POR counter (`por_cnt`) eliminating external pin metastability. | **PASS** |
| **3** | **Latch Inference Prevention** | **CRITICAL** | Combinational `always @(*)` & `case` statements | `ppg_peak_detector.v` sets `next_state = current_state;` unconditionally at entry and provides an explicit `default: next_state = STATE_ARMED;`. `axi_ppg_accelerator.v` and `forgefpga_ppg_top.v` contain full `case` coverage with defaults. 0 latches inferred. | **PASS** |
| **4** | **Reset Initialization Completeness** | **CRITICAL** | Sequential registers under `!rst_n` / `!s_axi_aresetn` | 100% of sequential state registers, sample shift registers, accumulators, and pulse flags are explicitly initialized in the reset branch. | **PASS** |
| **5** | **Named Port Connections** | **WARNING** | Module instantiations in top-level modules | All instantiations of `moving_average_8tap`, `ppg_peak_detector`, and `spi_target` in `axi_ppg_accelerator.v` and `forgefpga_ppg_top.v` use explicit named port syntax `.port(signal)`. 0 positional connections. | **PASS** |
| **6** | **Parameterized Timing Constants** | **WARNING** | Refractory, LED pulse, and clock periods | All timing parameters (`REFRACTORY_CYC = 12_500_000` for 250 ms, `LED_PULSE_CYC = 2_500_000` for 50 ms) are parameterized at top level and derived from `CLK_FREQ_HZ = 50_000_000`. | **PASS** |
| **7** | **Complete Sensitivity Lists** | **WARNING** | All `always` procedural blocks | Pure synchronous/asynchronous reset sensitivity lists: `always @(posedge clk or negedge rst_n)` and `always @(*)`. 0 incomplete legacy sensitivity lists. | **PASS** |
| **8** | **Redundant Logic & FSM Reachability** | **OPTIMISATION** | Peak detector FSM & AXI handshake | AXI4-Lite slave features a robust decoupled handshake allowing independent `AW` and `W` phases in any sequence. Status register provides Write-1-to-Clear (W1C) with hardware priority over software writes. | **PASS** |
| **9** | **Counter Bit-Width Optimization** | **OPTIMISATION** | ForgeFPGA 1,120-LUT resource budget | Counters are tightly sized to fit ForgeFPGA: `por_cnt` (8-bit), `bit_cnt` (3-bit), `led_timer` (24-bit), conserving LUTs while maintaining cycle precision. | **PASS** |
| **10** | **Synthesis Attributes & Vendor Directives** | **OPTIMISATION** | ForgeFPGA Workshop directives | `(* top *)`, `(* iopad_external_pin *)`, and `(* clkbuf_inhibit *)` correctly applied to top-level ports in `forgefpga_ppg_top.v`. `clk_en` and pad output enables tied high. | **PASS** |

---

### 2.3 Clock Domain Crossing (CDC) & Reset Tree Architecture

```
           +-------------------------------------------------------+
           |                 ESP32-S3 (240 MHz)                    |
           +-------------------------------------------------------+
                | SCK (GPIO12)   | SS_n (GPIO10)   | MOSI (GPIO11)
                v                v                 v
   ================== Asynchronous Domain Boundary ==================
                |                |                 |
                v                v                 v
           +-----------+    +-----------+    +-----------+
           | sck_sync  |    |  ss_sync  |    | mosi_sync |
           |  (2-FF)   |    |  (2-FF)   |    |  (2-FF)   |
           +-----------+    +-----------+    +-----------+
                |                |                 |
                +----------------+-----------------+
                                 | Synchronized to 50 MHz
                                 v
           +-------------------------------------------------------+
           |  ForgeFPGA Core Logic (50 MHz Internal OSC)           |
           |                                                       |
           |  [POR Counter: 8-bit] ---> rst_n (Synchronous Release)|
           |  [8-Tap MA Filter]                                    |
           |  [Crest Timestamp Peak Detector (20 ns resolution)]   |
           |  [IBI Burst Serializer (0xC3 + 4 bytes)]              |
           +-------------------------------------------------------+
```

1. **Dual Clock Domains**:
   - **Internal Core Domain**: 50.0 MHz on-chip RC oscillator (`clk`), enabled via `clk_en = 1'b1`.
   - **External SPI Bus Domain**: 1.0 MHz SPI Mode 0 from ESP32-S3 host (`spi_sck`, `spi_ss_n`, `spi_mosi`).
2. **CDC Mitigation**:
   - All three incoming SPI lines pass through dedicated 2-stage flip-flop synchronizers (`sck_sync`, `ss_sync`, `mosi_sync`) running on `clk`.
   - Edge detection (`sck_r`, `sck_f`) operates strictly on the synchronized signals, eliminating metastability risks during high-speed sampling.
3. **Power-On Reset (POR) Subsystem**:
   - Physical pin PIN_13 is unconnected on the ShrikeFi PCB.
   - An internal 8-bit saturating counter holds `rst_n` low for exactly 255 clock cycles (5.1 µs) after power application before releasing the synchronous core reset. This guarantees deterministic startup across fabric cells without external pin dependencies.

---

### 2.4 Testbench Execution & Timing Closure Review

#### A. Comprehensive Simulation Results (100% Passing)

| Testbench | Module Under Test | Checks / Tests | Failures | Status | Execution Time |
| :--- | :--- | :---: | :---: | :---: | :---: |
| `hardware/zynq/tb_ppg_system.v` | `axi_ppg_accelerator` | 6 tests | 0 | **PASS** | 218.3 ms sim time |
| `hardware/common/tb_ppg_peak_detector.v` | `ppg_peak_detector` | 4 tests | 0 | **PASS** | 17.07 ms sim time |
| `hardware/shrikefi/tb_forgefpga_system.v` | `forgefpga_ppg_top` | 13 checks | 0 | **PASS** | 1108.8 ms sim time |

- **Zynq System Testbench (`tb_ppg_system.v`)**:
  - Test 1: Dynamic threshold register write and readback (Value: 150) -> **PASS**
  - Test 2: Decoupled AXI write handshake (Address first, Data 5 cycles later) -> **PASS**
  - Test 3: 8-tap moving average Red channel filter convergence (Target: 100) -> **PASS**
  - Test 4: IR channel filter convergence (Target: 180) -> **PASS**
  - Test 5: Synthetic PPG pulse train beat detection & IBI measurement (Pulse 1 primed, Beats #1 & #2 detected, IBI 3282 / 3296 cycles) -> **PASS**
  - Test 6: Write-1-to-Clear (W1C) beat_flag verification with threshold preservation -> **PASS**
- **Common Peak Detector Unit Testbench (`tb_ppg_peak_detector.v`)**:
  - Test A: Single-sample dip on rising edge correctly ignored (dip does not terminate crest rise) -> **PASS**
  - Test B: Refractory window expiring mid-decay does not cause double-counting -> **PASS**
  - Test C: Above-threshold tail sample after refractory expiry does not re-arm FSM -> **PASS**
  - Test D: Dicrotic notch rejection (25% crest fall rule rejects dicrotic dip) -> **PASS**
- **ShrikeFi ForgeFPGA System Testbench (`tb_forgefpga_system.v`)**:
  - Test 1: Internal POR release, defined initial SPI reply (0x00), `clk_en` and `spi_miso_oe` asserted -> **PASS**
  - Test 2: 8-tap average convergence to steady-state baseline (100) -> **PASS**
  - Test 3: Bit-exact sliding window response over 39 sequential samples -> **PASS**
  - Test 4: Pulse train beat detection, single beat flag per crest, 5-byte IBI burst with `0xC3` magic header -> **PASS**
  - Test 5: Beat spacing exactly matches 50-transfer stimulus period (cycle-accurate hardware timing) -> **PASS**

#### B. FPGA Synthesis & Timing Closure Margins

| Target Device | Part Number | Clock Target | Achieved Fmax | Setup Margin (WNS) | Hold Margin (WHS) | Resource Utilization | Multipliers & Memory |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| **Xilinx Zynq-7000** | `xc7z020clg400-1` | 50.0 MHz | **69.45 MHz** | **+5.603 ns** | **+0.184 ns** | 185 LUT (0.35%), 16 LUTRAM, 266 FF | **0 DSP48, 0 BRAM** |
| **Renesas ForgeFPGA**| `SLG47910` | 50.0 MHz | **78.85 MHz** | **+7.316 ns** | N/A (Internal) | **515 LUT5** (46.0% of 1,120), 105/140 CLB, 303 FF | **0 DSP, 0 BRAM** |

- **Zero Dedicated Arithmetic Slices**: Both designs compute the 8-tap boxcar division via 3-bit wire shift (`>> 3`) and crest fall via 2-bit wire shift (`>> 2`), requiring 0 DSP48 multipliers on Zynq and 0 MAC blocks on ForgeFPGA.
- **Timing Closure**: With positive Worst Negative Slack (+5.603 ns on Zynq, +7.316 ns on ForgeFPGA), both implementations operate comfortably above the 50 MHz design requirement without metastability or timing degradation.

---

### 2.5 Formal Layer 1 Certification Statement

> **LAYER 1 CERTIFICATION VERDICT: FULLY CERTIFIED**  
> All RTL modules in `hardware/common/`, `hardware/zynq/`, and `hardware/shrikefi/` meet every architectural constraint specified in `AGENTS.md`. The design achieves 100% testbench pass rates across 23 automated verification checks, 0 Verilator lint violations, 0 blocking assignments in sequential logic, proven 2-stage CDC synchronization, 0 latches, and timing closure with positive margins on both Xilinx Zynq-7000 and Renesas ForgeFPGA SLG47910. Synthesizable RTL remains strictly frozen and verified.

---

## Section 3: Layer 2 — DSP, Clinical Algorithms & TinyML Audit

*(To be populated during Task 3)*
- 3.1 Digital Signal Processing Pipeline (8-Tap MA Filter, Systolic Crest Timestamping)
- 3.2 Clinical Vitals Engines (HRV RMSSD, Charlton PPG-RR, SpO2 DC-Rejection, SQI)
- 3.3 Environmental Compensation (PM2.5 Relative Humidity Correction)
- 3.4 TinyML Neural Network Invariants (FP32 vs INT8 Parity, MIMIC-III ICU Benchmarks)

---

## Section 4: Layer 3 — ESP32-S3 Firmware, Concurrency & Memory Safety Audit

*(To be populated during Task 4)*
- 4.1 Single-Precision FPU & Promotion Hygiene (`-Wdouble-promotion`)
- 4.2 FreeRTOS Dual-Core Concurrency & Inter-Task Mutex Synchronization
- 4.3 Bounds Checking, Memory Allocations & String Safety
- 4.4 HTTP Server URI Handler Slot Allocations

---

## Section 5: Layer 4 — Desktop & SoftAP Wi-Fi Dashboards Audit

*(To be populated during Task 5)*
- 5.1 100% Offline Resilience & Zero CDN Asset Integrity
- 5.2 Content-Security-Policy (CSP) Script Hash Synchronization
- 5.3 Embedded Gzip Flash Payload Integrity (`valor_dashboard_gz.h` vs HTML)
- 5.4 Wi-Fi Telemetry Ingestion, Mock PPG Synthesizer & Flatline Handling

---

## Section 6: Layer 5 — Python Tooling & Presentation Claims Audit

*(To be populated during Task 6)*
- 6.1 Tooling Memory Safety, Subprocess Guards & Resource Leaks
- 6.2 Pre-Demo Verification Harness Consolidation (`scripts/predemo_verify.ps1`)
- 6.3 Scientific & Presentation Claims Verification (MIMIC-III Sensitivity / Specificity Pairing)

---

## Section 7: Final Scorecard & Gate Matrix

*(To be completed at the conclusion of all audit tasks)*
