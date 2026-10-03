# Feature Specification: FPGA PPG Systolic Peak Detector & Signal Processing Pipeline

**Feature Branch**: `001-ppg-systolic-detector`  
**Created**: 2026-10-03  
**Status**: Draft  
**Target Architectures**: 
- Primary Port: Renesas ForgeFPGA SLG47910 (1120 5-input LUT budget, 50 MHz clock)
- Baseline: Xilinx Zynq-7000 xc7z020clg400-1 (Vivado ML 2022.2, 50 MHz clock, 0 DSP, 0 BRAM)

---

## 1. User Scenarios & Engineering Journeys

### User Story 1 - Real-Time Systolic Crest Detection & Moving Average Filtering (Priority: P1)
As an embedded biomedical firmware engineer, I need the FPGA to continuously filter raw optical photoplethysmogram (PPG) samples with an 8-tap moving average filter and detect systolic crests via a 4-state finite state machine (FSM), so that systolic pulses are identified in hardware with zero host CPU load.

**Why this priority**: Core accelerator function. Without hardware filtering and peak detection, the host MCU (ESP32-S3) must process high-frequency raw optical streams in software, draining battery and violating edge-disaster power budgets.

**Independent Test**: Provide an optical synthetic PPG pulse train to the FPGA; verify that filtered samples emerge with 1-sample latency and that a systolic peak flag pulses high once per pulse.

**Acceptance Scenarios**:
1. **Given** a sequence of 8-bit raw PPG optical samples (`sample_in`), **When** samples stream into `moving_average_8tap`, **Then** filtered samples $y[n] = y[n-1] + \frac{x[n] - x[n-8]}{8}$ are output without using any DSP48 multipliers or Block RAM.
2. **Given** filtered samples exceeding dynamic threshold, **When** the signal transitions from rising slope to falling slope (`sample_in < prev_sample`), **Then** the FSM transitions to `STATE_PEAK_FOUND`, latching the peak and asserting `beat_detected`.
3. **Given** a detected systolic peak, **When** the 250 ms refractory window (`REFRACTORY_CYC`) is active, **Then** secondary dicrotic notches and motion artifacts are rejected without asserting false beat flags.

---

### User Story 2 - Low-Pin-Count Host MCU Interfacing (Priority: P2)
As a systems integration engineer, I need the FPGA to communicate with the host processor (ESP32-S3 via 8-bit SPI mode 0 or Zynq PS via AXI4-Lite) using vendor-neutral common RTL and safe clock-domain crossing (CDC), so that filtered samples and beat events can be reliably read without bus contention or metastabilities.

**Why this priority**: Enables heterogeneous SoC integration across both ForgeFPGA (ShrikeFi) and Zynq platforms.

**Independent Test**: Stream SPI transactions at 1 MHz from ESP32-S3 to FPGA; verify that bit 7 of MISO returns `beat_latched` and bits [6:0] return the low 7 bits of filtered data, while all CDC inputs pass through 2-stage synchronizers.

**Acceptance Scenarios**:
1. **Given** an 8-bit SPI transaction in Mode 0 (CPOL=0, CPHA=0), **When** MOSI clocks in an 8-bit raw optical sample, **Then** MISO returns `{beat_latched, filt_sample[6:0]}` on the exact same 8-clock cycle full-duplex transfer.
2. **Given** asynchronous SPI inputs (`spi_sck`, `spi_ss_n`, `spi_mosi`), **When** crossing into the 50 MHz FPGA system clock domain, **Then** all signals are synchronized using 2-stage D flip-flop synchronizers to prevent metastability.
3. **Given** all I/O pins, **When** operating on the ShrikeFi PCB, **Then** all voltages must strictly adhere to 3.3V LVCMOS limits without exceeding maximum ratings.

---

### User Story 3 - Resource & Timing Closure within Strict Hardware Budgets (Priority: P3)
As an FPGA design engineer, I need the entire signal processing pipeline to synthesize within 1120 5-input LUTs on ForgeFPGA and meet positive timing slack at 50 MHz logic clock, so that the bitstream fits the resource-constrained target.

**Why this priority**: Hard hardware constraint; exceeding 1120 LUTs prevents bitstream generation for Renesas ForgeFPGA.

**Independent Test**: Run synthesis and place-and-route; inspect resource report to verify total LUT5 <= 1120, DSP48 = 0, BRAM = 0, and setup slack WNS > 0 ns.

**Acceptance Scenarios**:
1. **Given** synthesis on Renesas ForgeFPGA SLG47910, **Then** total resource utilization must not exceed 1120 5-input LUTs.
2. **Given** baseline synthesis on Xilinx Zynq xc7z020clg400-1, **Then** resources must remain <= 185 LUTs, 16 LUTRAM, 266 FFs, 0 DSP, 0 BRAM, with positive slack (WNS >= +5.6 ns at 50 MHz).

---

## 2. Requirements

### Functional Requirements

- **FR-001**: The pipeline MUST provide a dual-channel 8-tap moving average filter (`moving_average_8tap.v`) using an $O(1)$ running sum accumulator and arithmetic right-shift division (`>> 3`).
- **FR-002**: The moving average filter MUST consume zero DSP multiplier blocks and zero Block RAM (BRAM) blocks, using only flip-flops and logic LUTs.
- **FR-003**: The peak detector (`ppg_peak_detector.v`) MUST implement a 4-state FSM:
  - `STATE_ARMED` (`2'b00`): Armed when sample exceeds dynamic threshold.
  - `STATE_RISING` (`2'b01`): Track rising slope until local maximum is passed.
  - `STATE_PEAK_FOUND` (`2'b10`): Assert `beat_detected` single-cycle pulse and latch IBI interval.
  - `STATE_REFRACTORY` (`2'b11`): Blanking window countdown of 250 ms to suppress dicrotic notch false triggers.
- **FR-004**: The system MUST compute Inter-Beat Interval (IBI) using a 32-bit counter running at the 50 MHz system clock (20 ns per tick resolution).
- **FR-005**: For ShrikeFi ForgeFPGA target, the link interface MUST implement 8-bit SPI Mode 0 target protocol:
  - MOSI payload: 8-bit raw optical IR sample `sample[7:0]`.
  - MISO payload: 1-bit `beat_latched` (bit 7) and 7-bit filtered sample `filt_sample[6:0]` (bits 6:0).
- **FR-006**: For Zynq AXI target, the accelerator MUST expose a 32-bit memory-mapped register file (`0x43C00000`) with decoupled AW/W channels and Write-1-to-Clear (W1C) beat status register.
- **FR-007**: All asynchronous inputs crossing into the 50 MHz clock domain (`spi_sck`, `spi_ss_n`, `spi_mosi`) MUST pass through two-stage D flip-flop synchronizers.
- **FR-008**: The common RTL in `hardware/common/` MUST remain 100% vendor-agnostic, free from Xilinx, Renesas, or vendor-specific simulation/synthesis primitives.
- **FR-009**: The peak detector dynamic threshold MUST default to 120 and support runtime programming or parameterized defaults.

### Non-Functional & Invariant Requirements

- **NFR-001**: **Clock Frequency**: The core FPGA logic must close timing at 50.0 MHz ($\ge 50\text{ MHz}$ Fmax).
- **NFR-002**: **Strict LUT Budget**: For ForgeFPGA SLG47910, total logic utilization MUST NOT exceed 1120 5-input LUTs.
- **NFR-003**: **Zero DSP/BRAM Invariant**: The hardware signal processing path MUST use 0 DSP slices and 0 Block RAM slices.
- **NFR-004**: **I/O Voltage Safety**: All FPGA and MCU digital I/O lines MUST operate strictly at 3.3V LVCMOS maximum.
- **NFR-005**: **Verilog RTL Quality**: Sequential logic MUST use non-blocking assignments (`<=`) exclusively in `always @(posedge clk)` blocks to eliminate latch inference.

---

## 3. Success Criteria & Verification Targets

- **SC-001**: 100% pass rate on testbench `hardware/zynq/tb_ppg_system.v` (all 6 automated test cases passing).
- **SC-002**: 100% pass rate on ForgeFPGA testbench `hardware/shrikefi/tb_forgefpga_system.v`.
- **SC-003**: Worst Negative Slack (WNS) $\ge +5.0\text{ ns}$ on Zynq baseline and positive margin ($\ge +7.0\text{ ns}$) at 50 MHz on ForgeFPGA.
- **SC-004**: Peak detection sensitivity $\ge 98\%$ on clean PPG pulse inputs with heart rate range 40–200 BPM.
- **SC-005**: Rejection of dicrotic notch false peaks during the 250 ms refractory window.
