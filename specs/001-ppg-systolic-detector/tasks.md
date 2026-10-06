# Tasks: FPGA PPG Systolic Peak Detector & Signal Processing Pipeline

**Input**: Feature specification from [`spec.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/specs/001-ppg-systolic-detector/spec.md) and technical architecture from [`plan.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/specs/001-ppg-systolic-detector/plan.md).  
**Review Checklist**: [`checklists/hardware.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/specs/001-ppg-systolic-detector/checklists/hardware.md)  
**Governance**: [Constitution](file:///C:/Users/abhin/OneDrive/Desktop/verilog/.specify/memory/constitution.md)

---

## Phase 1: Setup & Environment Baseline

**Purpose**: Validate toolchains, file structure, and project constraints.

- [x] T001 Verify synthesizable RTL tree structure in `hardware/common/`, `hardware/shrikefi/`, and `hardware/zynq/`
- [x] T002 [P] Inspect physical constraint definitions in `hardware/shrikefi/forgefpga_pins.pcf` against ShrikeFi 3.3V LVCMOS pin allocations
- [x] T003 [P] Verify Vivado synthesis and simulation scripts for Zynq baseline (`hardware/zynq/`)

---

## Phase 2: Foundational RTL Quality & Invariant Checks (Blocking Prerequisites)

**Purpose**: Core RTL safety checks that MUST pass before feature extensions or integration.

- [x] T004 Audit common RTL (`moving_average_8tap.v`, `ppg_peak_detector.v`) with `verilog-rtl-auditor` for latch avoidance and non-blocking assignments (`<=`)
- [x] T005 [P] Verify zero-DSP and zero-BRAM design rules in `hardware/common/moving_average_8tap.v`
- [x] T006 [P] Verify 2-stage D flip-flop synchronizer topology on all asynchronous control signals entering the 50 MHz clock domain in `hardware/shrikefi/forgefpga_ppg_top.v`

**Checkpoint**: Foundation verified — synthesizable RTL hygiene and CDC safety established.

---

## Phase 3: User Story 1 - Real-Time Systolic Crest Detection & Filtering (Priority: P1) 🌟 MVP

**Goal**: Deliver verified $O(1)$ 8-tap moving average filtering and 4-state refractory peak detection in hardware.

### Tests for User Story 1
- [x] T007 [P] [US1] Run self-checking simulation on `hardware/common/tb_ppg_peak_detector.v` to verify 4-state FSM transitions (3/3 passing)
- [x] T008 [P] [US1] Validate dicrotic notch rejection during the 250 ms refractory window (`REFRACTORY_CYC = 12,500,000` cycles @ 50 MHz)

### Implementation & Verification for User Story 1
- [x] T009 [US1] Verify running sum accumulator bit-width (11-bit) and arithmetic shift division (`>> 3`) in `hardware/common/moving_average_8tap.v`
- [x] T010 [US1] Verify dynamic threshold comparison logic and peak detection latching in `hardware/common/ppg_peak_detector.v`
- [x] T011 [US1] Verify 32-bit IBI cycle interval counter incrementation and timestamp latching on detected systolic peaks

**Checkpoint**: User Story 1 functional and verified under standalone simulation testbench.

---

## Phase 4: User Story 2 - Host Interfacing & Clock Domain Crossing (Priority: P2)

**Goal**: Seamless SPI Mode 0 and AXI4-Lite communication with host processors.

### Tests for User Story 2
- [x] T012 [P] [US2] Execute end-to-end SPI link testbench in `hardware/shrikefi/tb_forgefpga_system.v` (10/10 checks passing)
- [x] T013 [P] [US2] Execute Vivado Zynq AXI testbench `hardware/zynq/tb_ppg_system.v` (verify 6/6 tests passing)

### Implementation & Verification for User Story 2
- [x] T014 [US2] Validate 8-bit SPI Mode 0 target shift register in `hardware/shrikefi/spi_target.v`
- [x] T015 [US2] Confirm MISO payload packing `{beat_latched, filt_sample[6:0]}` in `hardware/shrikefi/forgefpga_ppg_top.v`
- [x] T016 [US2] Validate decoupled AW/W channels and W1C status register (`0x0C`) in `hardware/zynq/axi_ppg_accelerator.v`

**Checkpoint**: Both SPI and AXI interfaces functionally verified against their respective host specifications.

---

## Phase 5: User Story 3 - Resource & Timing Closure within Strict Hardware Budgets (Priority: P3)

**Goal**: Guarantee bitstream synthesis fits inside target FPGA resource and timing limits.

- [x] T017 [US3] Synthesize ForgeFPGA target and verify total 5-input LUT usage $\le 1120$ LUTs (verified within limits)
- [x] T018 [US3] Verify ForgeFPGA Fmax $\ge 50.0\text{ MHz}$ (achievable period 12,087 ps = 82.73 MHz, slack +7.913 ns at 50 MHz)
- [x] T019 [US3] Re-verify Zynq-7000 baseline synthesis report (185 LUT, 16 LUTRAM, 266 FF, 0 DSP, 0 BRAM, WNS +5.603 ns)

**Checkpoint**: Hardware budgets confirmed on both silicon targets with timing closure met.

---

## Phase 6: Polish & Cross-Cutting Verification

**Purpose**: Documentation synchronization and review alignment.

- [x] T020 Review all items in [`checklists/hardware.md`](file:///C:/Users/abhin/OneDrive/Desktop/verilog/specs/001-ppg-systolic-detector/checklists/hardware.md) for requirements quality sign-off
- [x] T021 [P] Ensure `docs/HARDWARE_ARCHITECTURE.md` and `docs/SHRIKEFI_LINK_PROTOCOL.md` stay synchronized with RTL register maps
- [x] T022 [P] Audit C firmware link driver (`firmware/shrikefi/shrikefi_link_driver.c`) with `c-firmware-safety` for single-precision FPU and mutex safety
