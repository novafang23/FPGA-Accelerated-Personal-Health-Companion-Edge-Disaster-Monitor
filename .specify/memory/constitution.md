# VALOR Hardware & Heterogeneous SoC Constitution

<!-- SIH26181: FPGA-Accelerated Personal Health Companion & Edge Disaster Monitor -->

## Core Principles

### I. Strict Hardware Resource & Timing Invariants (NON-NEGOTIABLE)
- **Zero DSP / Zero BRAM**: The digital signal processing pipeline (`moving_average_8tap.v`, `ppg_peak_detector.v`) must never instantiate DSP48 multiplier slices or Block RAM. All arithmetic must rely exclusively on shift registers, logic LUTs, and bit-shift operations (`>> 3`).
- **Renesas ForgeFPGA Budget**: Logic utilization on the target SLG47910 must not exceed **1120 5-input LUTs**.
- **Xilinx Zynq-7000 Baseline**: Must maintain verified out-of-context synthesis budget: $\le 185\text{ LUT}$, $16\text{ LUTRAM}$, $266\text{ FF}$, $0\text{ DSP}$, $0\text{ BRAM}$, with positive slack ($WNS \ge +5.0\text{ ns}$ at 50 MHz).
- **50 MHz System Clock**: Fmax must meet or exceed 50.0 MHz on all target platforms.

### II. Clock Domain Crossing (CDC) & Synchronization Safety
- All asynchronous control, strobe, and serial inputs crossing into the 50 MHz FPGA system clock domain (including `spi_sck`, `spi_ss_n`, `spi_mosi`) MUST pass through dedicated two-stage D flip-flop synchronizers to prevent metastability.
- Handshakes and status flags must eliminate race conditions using Write-1-to-Clear (W1C) or single-transaction pulse latching.

### III. RTL Vendor Neutrality & Synthesis Hygiene
- Common core RTL in `hardware/common/` must remain 100% vendor-agnostic (pure IEEE 1364-2001 synthesizable Verilog, free from Xilinx-specific, Renesas-specific, or proprietary primitives).
- Sequential logic must strictly use non-blocking assignments (`<=`) within `always @(posedge clk)` blocks to eliminate latch inference.
- Simulation and synthesis constructs must remain consistent without hidden race conditions.

### IV. Electrical Safety & Pin Voltage Caps
- **3.3V LVCMOS Strict Maximum**: Every I/O pin on both the ESP32-S3 and the ForgeFPGA is strictly 3.3V maximum. Exceeding 3.3V permanently destroys the ICs.
- 5V power rails must remain strictly isolated to the external sensor VCC rail (PMSA003) and must never connect directly to FPGA/MCU digital logic pins.

### V. Firmware Concurrency, Precision & Offline Disaster Integrity
- **Single-Precision FPU Only**: ESP32-S3 has a single-precision hardware FPU. All firmware must use `float`, `sqrtf()`, and `fabsf()`, avoiding emulated double-precision software arithmetic.
- **Inter-Core Concurrency**: Shared buffers across FreeRTOS cores must be protected using `s_data_mutex`.
- **Memory & String Safety**: Never use `sprintf` or `strcpy`. All string and buffer operations must be bounded (`snprintf`, `strncpy`).
- **100% Offline Edge Operation**: The system must run autonomously in disaster zones without external cloud or CDN dependencies.

---

## Governance & Quality Gates

1. **Gate 1 - Requirements Quality**: Every feature must define verifiable functional requirements, non-functional invariants, and a Spec Kit review checklist (`checklists/*.md`).
2. **Gate 2 - Synthesis & Timing Verification**: No RTL change may be accepted if it increases DSP/BRAM above 0, exceeds 1120 LUTs on ForgeFPGA, or yields negative timing slack ($WNS < 0$).
3. **Gate 3 - Self-Checking Testbenches**: All regression testbenches (`tb_ppg_system.v`, `tb_forgefpga_system.v`) must pass with 100% assertion success before integration.

**Version**: 1.0.0 | **Ratified**: 2026-10-03 | **Last Amended**: 2026-10-03
