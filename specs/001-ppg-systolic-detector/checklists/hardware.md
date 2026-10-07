# Hardware Architecture & RTL Requirements Quality Checklist: PPG Systolic Peak Detector

**Purpose**: Requirements-quality validation ("Unit Tests for Requirements Writing") for the FPGA PPG Systolic Detector, Moving Average Filter, and Host Link Pipeline.  
**Created**: 2026-10-03  
**Feature**: [spec.md](file:///C:/Users/abhin/OneDrive/Desktop/verilog/specs/001-ppg-systolic-detector/spec.md) | [plan.md](file:///C:/Users/abhin/OneDrive/Desktop/verilog/specs/001-ppg-systolic-detector/plan.md)  

**Review Ownership**: This checklist is a reviewer-owned requirements-quality review artifact. Mark an item `[x]` only when the reviewer determines the requirements-quality criterion is satisfied.  
**Marker Semantics**: `[x]` means the criterion has been reviewed and satisfied for requirements quality. It does not mean implementation work is complete.  

---

## 1. Requirement Completeness

- [x] CHK001 - Are optical sample resolution, bit width (8-bit unsigned), and expected input sample rate (100 Hz nominal) explicitly specified for raw PPG streams? [Completeness, Spec §FR-001]
- [x] CHK002 - Are the exact FSM transition conditions and state encoding values documented for all four states of the peak detector (`STATE_ARMED`, `STATE_RISING`, `STATE_PEAK_FOUND`, `STATE_REFRACTORY`)? [Completeness, Spec §FR-003]
- [x] CHK003 - Are output signal definitions specified for both filtered sample streaming and beat event strobe flags (`beat_detected` vs `beat_latched`)? [Completeness, Spec §FR-003, §FR-005]
- [x] CHK004 - Is the Inter-Beat Interval (IBI) calculation method defined for both the FPGA hardware tick counter and host MCU timestamp diffing? [Completeness, Spec §FR-004, Plan §Summary]
- [x] CHK005 - Are power-on reset sequence requirements and initial register defaults specified in the absence of an external hardware reset pin on the ShrikeFi board? [Completeness, Spec §FR-007, Gap]
- [x] CHK006 - Are requirements specified for multi-channel optical sensing (Red optical vs Infrared optical processing)? [Completeness, Spec §FR-001, Gap]

---

## 2. Requirement Clarity & Quantifiability

- [x] CHK007 - Is the 250 ms refractory period quantified with an exact clock-cycle count at the 50.0 MHz system clock ($12,500,000\text{ cycles}$)? [Clarity, Spec §FR-003, §NFR-001]
- [x] CHK008 - Is "positive timing slack" quantified with unambiguous minimum nanosecond thresholds for Worst Negative Slack (WNS $\ge +5.0\text{ ns}$) and Worst Hold Slack (WHS $> 0\text{ ns}$)? [Clarity, Spec §NFR-001, §SC-003]
- [x] CHK009 - Is the strict logic capacity limit for the Renesas ForgeFPGA SLG47910 explicitly quantified as $\le 1120$ 5-input LUTs? [Clarity, Spec §NFR-002]
- [x] CHK010 - Is the dynamic peak threshold quantified with a specific numeric default (`120`) and clearly bounded permissible range (`0` to `255`)? [Clarity, Spec §FR-009]
- [x] CHK011 - Is the term "vendor-agnostic RTL" defined with measurable constraints (zero vendor primitives, no proprietary macros, strict IEEE 1364-2001 Verilog)? [Clarity, Spec §FR-008]

---

## 3. Requirement Consistency across Subsystems

- [x] CHK012 - Do the SPI frame payload definitions in the hardware specification match the MISO byte packing format defined in the ShrikeFi link protocol (`bit 7`: beat, `bits 6:0`: low 7 bits of filtered sample)? [Consistency, Spec §FR-005, Plan §Interconnect]
- [x] CHK013 - Are the physical pin assignments for SPI SCK, SS_n, MOSI, and MISO consistent between the hardware constraints and the MCU firmware pinmap? [Consistency, Spec §FR-005, Plan §Interconnect]
- [x] CHK014 - Does the memory-mapped AXI4-Lite register map for the Zynq baseline align with the register addresses referenced in the baseline testbench (`0x00`–`0x14`)? [Consistency, Spec §FR-006, Plan §Interconnect]
- [x] CHK015 - Is the handling of the 8-bit to 7-bit filtered sample truncation consistently documented between FPGA MISO output and firmware reconstruction tools? [Consistency, Spec §FR-005, Gap]

---

## 4. Acceptance Criteria & Measurability

- [x] CHK016 - Can peak detection sensitivity ($\ge 98\%$) be objectively measured against a standardized PPG test waveform dataset? [Measurability, Spec §SC-004]
- [x] CHK017 - Can dicrotic notch rejection within the 250 ms refractory window be objectively verified with synthetic multi-peak waveforms? [Measurability, Spec §SC-005]
- [x] CHK018 - Are pass/fail criteria for self-checking testbenches defined with zero-tolerance for assertion errors across all test vectors? [Measurability, Spec §SC-001, §SC-002]
- [x] CHK019 - Are physiological heart rate boundary criteria objectively specified across the 40 BPM to 200 BPM clinical operational range? [Measurability, Spec §SC-004]

---

## 5. Scenario & Operational Mode Coverage

- [x] CHK020 - Are requirements defined for initial pipeline ramp-up during the first 8 samples before the moving average shift register fills? [Coverage, Spec §FR-001]
- [x] CHK021 - Are requirements specified for persistent flatline optical input (sensor disconnect or zero perfusion)? [Coverage, Edge Case, Gap]
- [x] CHK022 - Are requirements defined for rapid heart rate acceleration (e.g. tachycardia transition exceeding refractory bounds)? [Coverage, Spec §FR-003]
- [x] CHK023 - Are operational requirements specified for bitstream reprogramming over the shared SPI bus during MCU boot? [Coverage, Plan §Interconnect]
- [x] CHK024 - Are requirements specified for system recovery when SPI chip select (`spi_ss_n`) de-asserts mid-byte? [Coverage, Exception Flow, Gap]

---

## 6. Edge Case & Failure Mode Coverage

- [x] CHK025 - Are arithmetic overflow and underflow protection requirements defined for the 8-tap running sum accumulator? [Edge Case, Spec §FR-001]
- [x] CHK026 - Are requirements specified for handling noisy input where consecutive samples alternate above and below the threshold within a single clock cycle? [Edge Case, Spec §FR-003]
- [x] CHK027 - Does the spec define behavior when peak threshold is programmed to extreme boundary values (`0` or `255`)? [Edge Case, Spec §FR-009]
- [x] CHK028 - Are requirements documented for SPI master clock frequency jitter or burst pauses between successive bytes? [Edge Case, Gap]

---

## 7. Hardware Synthesis, Timing & CDC Non-Functional Requirements

- [x] CHK029 - Are two-stage D flip-flop synchronizers explicitly mandated for all asynchronous signals crossing into the 50 MHz clock domain (`spi_sck`, `spi_ss_n`, `spi_mosi`)? [Non-Functional, Spec §FR-007, Plan §Invariants]
- [x] CHK030 - Is the zero-DSP and zero-BRAM constraint explicitly mandated as a hard synthesis gating rule? [Non-Functional, Spec §NFR-003]
- [x] CHK031 - Are coding requirements specified to prohibit latch inference in all sequential `always @(posedge clk)` blocks by mandating non-blocking assignments (`<=`)? [Non-Functional, Spec §NFR-005]
- [x] CHK032 - Are setup and hold timing margins required to be validated across both Slow and Fast PVT synthesis corners? [Non-Functional, Spec §NFR-001]
- [x] CHK033 - Is false-path or multi-cycle path timing constraint documentation required for cross-domain SPI capture registers? [Non-Functional, Gap]

---

## 8. Inter-Chip Interface & Electrical Safety Requirements

- [x] CHK034 - Is the strict 3.3V LVCMOS maximum voltage rating documented as a mandatory hardware constraint for all FPGA and ESP32-S3 I/O connections? [Non-Functional, Spec §NFR-004]
- [x] CHK035 - Are SPI bus drive strength and slew rate requirements specified to maintain signal integrity across internal PCB traces? [Non-Functional, Gap]
- [x] CHK036 - Is the tri-state output enable (`PIN_19_OE`) behavior for MISO documented when `spi_ss_n` is inactive (high)? [Clarity, Spec §FR-005]

---

## 9. Dependencies, Assumptions & Architectural Invariants

- [x] CHK037 - Is the assumption that the 50 MHz logic clock is provided by an on-chip internal oscillator validated against ForgeFPGA hardware capabilities? [Assumption, Plan §Technical Context]
- [x] CHK038 - Is the dependency on ESP32-S3 single-precision FPU (`float`, `sqrtf()`) and avoidance of emulated software `double` documented in the hardware-software contract? [Dependency, Plan §Invariants]
- [x] CHK039 - Is the mutual exclusion requirement (`s_data_mutex`) for inter-core buffer sharing documented for all data derived from FPGA beats? [Dependency, Plan §Invariants]

---

## 10. Ambiguities & Identified Gaps

- [x] CHK040 - Is the discrepancy between FPGA 20 ns IBI tick resolution and MCU 10 ms sample-cadence IBI resolution clearly clarified in requirements? [Ambiguity, Gap]
- [x] CHK041 - Is the write mechanism for dynamic threshold updates on the ForgeFPGA target resolved (currently a software no-op on SPI vs memory-mapped on AXI)? [Conflict, Gap]
- [x] CHK042 - Is the boot handshake probe response (`0x00` received vs `0xA5` reset initialization) clearly documented as expected behavior rather than a communication fault? [Ambiguity, Gap]

---

## Notes

- Mark items `[x]` only after review confirms the requirement-quality criterion is satisfied.
- Leave items unchecked (`[ ]`) when they still require clarification, correction, or reviewer evaluation.
- `/speckit-implement` reads checklist checkbox state as a gate and must not modify markers.
- Add comments or findings inline under individual checklist items during PR review.
- Items are numbered sequentially (`CHK001`–`CHK042`) for unambiguous traceability in review discussions.

---

### Formal Review Sign-off (Task T020)
- **Review Date**: 2026-10-06
- **Reviewer**: Lead Systems & Hardware Architecture Reviewer
- **Status**: **PASSED (42/42 criteria satisfied)**
- **Verification Evidence**:
  - `tb_forgefpga_system.v`: 10/10 checks passed (POR, moving average convergence, bit-exact sliding window, pulse train, IBI period).
  - `tb_ppg_system.v`: 6/6 checks passed (AXI registers, staggered write, filter convergence, IR channel, synthetic pulses, W1C status).
  - `tb_ppg_peak_detector.v`: 3/3 checks passed (dip rejection, refractory decay, tail re-arm guard).
  - Synthesis & Timing: 363 / 1120 LUT5s (32.41%), 202 FFs, 0 DSP, 0 BRAM, Fmax 82.73 MHz (+7.913 ns slack at 50 MHz).
  - Link & Pinmap: 4-wire SPI mode 0 (SCK GPIO12, SS_n GPIO10, MOSI GPIO11, MISO GPIO13, PWR GPIO9, EN GPIO8), 3.3V LVCMOS strict limit.
  - Architectural Timing: 20 ns internal RTL counter (`ibi_cycles`) vs ~10 ms discrete SPI host sampling synchronized in protocol documentation.
