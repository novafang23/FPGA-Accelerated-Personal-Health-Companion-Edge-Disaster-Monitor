# AGENTS.md — Repository Restructuring & ShrikeFi Migration

This file is instructions for an AI coding agent (e.g. Google Antigravity) working
inside a clone of this repository. Read this whole file before making any changes.
Work through the task list in order. Do not skip the guardrails section.

## 1. Project context

Repo: `FPGA-Accelerated-Personal-Health-Companion-Edge-Disaster-Monitor`
(SIH26181, built for the Qualcomm Hardware Challenge, Smart India Hackathon 2026).

It's a heterogeneous SoC design: synthesizable Verilog RTL (moving-average filter +
systolic peak detector) plus a TinyML risk model, originally verified on a Xilinx
Zynq-7000. All current source lives in a single flat folder called `SIH/`. It has NOT
been reorganized yet — that is this file's job.

**Verified Zynq baseline (do not lose or misrepresent these numbers):**
- Target part: `xc7z020clg400-1`, Vivado ML 2022.2, out-of-context synthesis
- WNS +5.603 ns, WHS +0.184 ns, Fmax 69.45 MHz (against a 50 MHz clock target)
- Resource usage: 185 LUT, 16 LUTRAM, 266 FF, **0 DSP48, 0 BRAM**
- Testbench: `tb_ppg_system.v`, 6/6 self-checking tests passing (Icarus Verilog)

**Why we're doing this:** the Zynq board is too expensive to scale past the
hackathon prototype. We're adding a second, much cheaper target board
("ShrikeFi") while keeping the Zynq work fully intact and citable as verified
evidence. This is a restructuring + migration-prep task, not a full port —
the actual ShrikeFi RTL/firmware work happens after this restructuring, in a
separate branch.

## 2. Target board reference (ShrikeFi) — verified facts, don't invent beyond this

Source: https://store.vicharak.in/?product=shrikefi

- MCU: ESP32-S3 (dual-core, on-chip WiFi 4 802.11 b/g/n + Bluetooth 5 LE)
- FPGA: Renesas ForgeFPGA, **1120 five-input LUTs** (this is a different LUT
  architecture from Xilinx's 6-input LUTs — do not assume 1:1 resource parity,
  the actual post-synthesis LUT count on this part must be re-measured, not
  estimated from the Zynq report)
- FPGA↔MCU interface: a **4-bit** parallel link — there is no AXI-style bus and
  no ARM processing system on this board. This is a fundamentally different
  interconnect, not a faster/slower AXI.
- I/O: 24 MCU GPIO, 14 FPGA GPIO, all **3.3 V only** (exceeding 3.3V damages
  ICs beyond repair per vendor documentation — flag this prominently in any
  ShrikeFi hardware doc)
- PMOD-compatible connector, breadboard compatible
- 2x USB Type-C (power + programming)
- Flash: W25Q32JV, 32Mb/4MB QSPI (stores FPGA bitstream + firmware)
- Optional (DNP by default, solder-it-yourself): 8MB PSRAM (LY68L6400SLIT,
  designator U10); onboard battery management/charging circuit (BOM available
  on the product page if this is ever populated)
- Toolchain: Renesas's free ForgeFPGA design software — no license fees, has
  both a schematic/macrocell mode and an **HDL (Verilog) mode**
- **Unknown / needs verification before writing ShrikeFi docs**: exact clock
  source(s) and available frequencies on the ForgeFPGA part, and its FF/BRAM
  capacity. Do not reuse the Zynq's 50 MHz / 20 ns timing figures for ShrikeFi
  — pull real numbers from the Renesas ForgeFPGA datasheet once you're doing
  the actual port.

## 3. Guardrails — read before touching anything

- **Never delete or overwrite the original `SIH/` content until every file is
  confirmed moved.** Use `git mv`, not manual copy + delete, so history and
  blame survive.
- **Tag the current state before any restructuring** (see Task 1). This is
  the single most important step — it's the permanent, citable proof of the
  verified Zynq result, independent of anything that happens afterward.
- Do not force-push. Do not rewrite existing commit history.
- Do not delete `LICENSE` or `.gitignore`, and do not remove existing README
  badges/content — restructure and extend the README, don't replace it
  wholesale.
- If any filename referenced in this doc doesn't match what's actually in the
  repo, trust the repo and use your judgement to categorize the real file
  according to the rules in Task 2 — don't skip a file just because it wasn't
  explicitly named here. Note: the README's Quickstart section references
  `test_disaster_risk_engine.c`, which is not listed in the README's own
  directory structure diagram — resolve this discrepancy by checking what
  actually exists in `SIH/` and placing it correctly.
- After each numbered task below, run `git status` and review the diff before
  committing. Commit after each numbered task separately (small, reviewable
  commits), not as one giant commit at the end.

## 4. Task list

### Task 1 — Freeze the verified baseline
```bash
git tag -a v1.0-zynq-SIH -m "Verified Zynq-7000 baseline: 6/6 tests passing, timing closed at 69.45 MHz"
git push origin v1.0-zynq-SIH
```
Confirm the tag exists on the remote before proceeding.

### Task 2 — Restructure by platform, not by file type
Run `ls -la SIH/` first to get the ground-truth file list. Then categorize
every file using these rules and `git mv` it into the new layout below.

**New layout:**
```
hardware/
├── common/       # vendor-agnostic Verilog — no bus protocol, no vendor primitives
├── zynq/         # everything Vivado/AXI/Zynq-specific, plus all synthesis evidence
└── shrikefi/     # empty for now — created in Task 5, populated on the port branch
firmware/
├── core/         # pure algorithm C — no hardware register access
├── zynq/         # Zynq PS-specific HAL/drivers/harness
└── shrikefi/     # empty for now — created in Task 5
docs/
├── HARDWARE_ARCHITECTURE.md   (moved, content unchanged for now)
├── QUALCOMM_PLATFORM_STRATEGY.md   (moved, content unchanged for now)
├── MIGRATION.md                (new — see Task 3)
└── SHRIKEFI_LINK_PROTOCOL.md   (new — see Task 4)
```

**Categorization rules:**
| Rule | Destination |
|---|---|
| Verilog module with no AXI/bus signals, no vendor-specific primitives (e.g. the moving-average filter, the peak-detector FSM) | `hardware/common/` |
| Verilog that IS the AXI wrapper, references Zynq-specific signals, or is a testbench built around the AXI interface | `hardware/zynq/` |
| `.xdc`, `.tcl`, `.gtkw`, and all waveform/timing/utilization/schematic/floorplan images | `hardware/zynq/` (these are evidence of the Zynq run specifically) |
| C file that's pure math/logic with no register or peripheral access (e.g. HRV analysis, SpO2 calculation, the NN inference engine, the disaster risk scoring engine, the model training script) | `firmware/core/` |
| C file that's a HAL, sensor driver, or hardware-specific harness (I2C HAL, sensor drivers, the interactive simulation harness, the rule-engine-vs-NN comparison harness) | `firmware/zynq/` |
| `build_and_run.bat` | `hardware/zynq/` (it's a Vivado-flow launcher) |

Update any relative `#include` paths or build-script paths broken by the move.
Root-level `run.bat` can stay at root as a dispatcher, updated to point at
`hardware/zynq/build_and_run.bat`.

### Task 3 — Write `docs/MIGRATION.md`
Use this table as the starting content (expand status as work progresses):

| Component | Zynq-7000 (baseline) | ShrikeFi | Status |
|---|---|---|---|
| Filter + peak detector RTL | Verified, 0 DSP/BRAM | Same source (`hardware/common/`), targeting ForgeFPGA HDL mode | Likely direct reuse — unverified until synthesized |
| FPGA↔host interface | AXI4-Lite (memory-mapped registers) | Custom protocol over the 4-bit link | Full redesign — see `SHRIKEFI_LINK_PROTOCOL.md` |
| Application code (HRV, SpO2, NN, risk engine) | Runs on ARM Cortex-A9 (Zynq PS) | Runs on ESP32-S3 (ESP-IDF or Arduino-ESP32) | Rehost — algorithm logic unchanged, runtime environment changes |
| Toolchain | Vivado ML 2022.2 | Renesas ForgeFPGA design software (free, HDL mode) | New flow, new constraints format |
| Connectivity | None | WiFi 4 + BLE 5 (onboard ESP32-S3) | New capability, not required for parity |
| Timing figures | 50 MHz clock, 20 ns IBI resolution, 69.45 MHz Fmax | Not yet measured | Do not reuse Zynq numbers — pull real ForgeFPGA clock/timing data during the port |

Also include, in prose above the table: why the migration is happening (cost
accessibility beyond the hackathon prototype), and a one-line pointer to
Task 4's protocol doc for anyone who wants interface-level detail.

### Task 4 — Create `docs/SHRIKEFI_LINK_PROTOCOL.md` (skeleton)
This replaces the role of the old AXI register table
(`REG_RED_RAW`, `REG_IBI_CYCLES`, etc.) for the new interface. Create the file
with these section headers, each containing a `TODO` placeholder — do not
invent register values or timing numbers:

```markdown
# ShrikeFi FPGA-MCU link protocol

## Physical interface
TODO: pin assignment for the 4-bit link, direction of each line, clock source

## Data framing
TODO: how filtered PPG samples and IBI counts are serialized over 4 bits

## Timing
TODO: link clock frequency, sample rate, latency budget

## Comparison to the Zynq AXI4-Lite interface
TODO: explicit note on what functionality from the old REG_* map has no
equivalent yet and needs a protocol decision (e.g. status/interrupt flags,
which had a dedicated W1C register on Zynq and currently has no analog here)
```

### Task 5 — Create the porting branch
```bash
git checkout -b port/shrikefi
mkdir -p hardware/shrikefi firmware/shrikefi
```
Leave these directories empty except for a `.gitkeep` file each, or a short
`README.md` stub stating "ShrikeFi port in progress — see docs/MIGRATION.md".
Do not attempt the actual RTL port or firmware rehost as part of this task —
that's separate, future work. This branch's job right now is just to exist
as the place that work will happen, so `main` stays clean.

### Task 6 — Update the root README
On `main` (not the port branch): add a short "Platform status" section near
the top of `README.md`, above or below the existing badges, that states the
Zynq build is verified/baseline and links to `docs/MIGRATION.md` for the
ShrikeFi roadmap. Keep all existing badges, architecture diagrams, and
verification content intact — this is an addition, not a rewrite.

## 5. Definition of done

- [ ] `v1.0-zynq-SIH` tag exists on the remote
- [ ] No files were deleted — everything moved via `git mv` with history intact
- [ ] `hardware/common/`, `hardware/zynq/`, `firmware/core/`, `firmware/zynq/`,
      `docs/` exist and are populated per the categorization rules
- [ ] `docs/MIGRATION.md` and `docs/SHRIKEFI_LINK_PROTOCOL.md` exist with the
      content specified above
- [ ] `port/shrikefi` branch exists with empty `hardware/shrikefi/` and
      `firmware/shrikefi/` scaffolding
- [ ] README updated with a platform-status pointer, existing content preserved
- [ ] Any build scripts/includes broken by the file moves are fixed and the
      Zynq flow still builds/runs from its new path
- [ ] Each task above is its own commit, and `git log` reads as a clear,
      reviewable history of the restructuring
