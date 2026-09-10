# ShrikeFi hardware (Renesas ForgeFPGA `SLG47910C`)

RTL and synthesis evidence for the **ShrikeFi** target: an ESP32-S3 plus a
Renesas ForgeFPGA (1120 five-input LUTs) connected by a 4-bit parallel link.
The port is implemented — see [docs/MIGRATION.md](../../docs/MIGRATION.md).

## RTL layout

| File | Purpose |
|---|---|
| `forgefpga_ppg_top.v` | ForgeFPGA top level: 4-bit link FSM, command decode, status/IRQ registers |
| `../common/moving_average_8tap.v` | 8-tap moving-average filter (shared, vendor-agnostic) |
| `../common/ppg_peak_detector.v` | 4-state systolic peak-detector FSM with 250 ms refractory window |
| `tb_forgefpga_system.v` | Self-checking testbench (5 tests) for the link + DSP path |
| `forgefpga_project/` | Renesas ForgeFPGA Workshop project (flat source set the vendor tool consumes) |
| `forgefpga_pins.pcf` / `.pdc` | Placement constraints |
| `build_shrikefi_sim.bat` | Compile + run the testbench with Icarus Verilog |
| `convert_bitstream.py` | Convert the vendor bitstream `.bin` into `firmware/shrikefi/forgefpga_bitstream.h` |

`forgefpga_project/ffpga/src/*.v` is the **flattened copy** the Renesas tool
needs (it consumes one flat source set). `forgefpga_ppg_top.v` at this level is
the simulator/lint copy and deliberately does **not** inline the DSP modules.

> **Gotcha:** `forgefpga_ppg_top.v` instantiates `moving_average_8tap` and
> `ppg_peak_detector` from `hardware/common/`. If you compile it *and* also add
> inline copies of those modules, Icarus Verilog fails with
> `'moving_average_8tap' has already been declared in this scope`. Compile the
> top **once** with the two common sources, nothing more.

## Simulate

```bat
build_shrikefi_sim.bat
```

Expected result: `Passed: 5 / 5`. The same command runs in CI.

## Synthesis results

Post-synthesis, Renesas ForgeFPGA Workshop v6.55, top module `forgefpga_ppg_top`:

| Resource | Usage |
|---|---|
| CLB LUT5s | **443 / 1120 (39.55%)** |
| FFs | **353** (345 CLB + 8 IOB) |
| CLBs | **85 / 140 (60.71%)** |
| Block RAMs | **0 / 8** |
| PLLs | **1 / 1 (100%)** |

Raw log: [`synthesis_evidence/resource_utilization.log`](synthesis_evidence/resource_utilization.log),
with context in [`synthesis_evidence/README.md`](synthesis_evidence/README.md).

LUT usage is comfortable, but **CLB occupancy is 60.7% and the part's only PLL is
consumed** — re-check the CLB budget, not just the LUT count, before expanding
the DSP chain.

## Interconnect

The 4-bit link, its command/register map and the differences from the Zynq
AXI4-Lite interface are specified in
[docs/SHRIKEFI_LINK_PROTOCOL.md](../../docs/SHRIKEFI_LINK_PROTOCOL.md). The
command map is consistent across that document, `firmware/shrikefi/shrikefi_link_driver.h`
and the `localparam` decode in `forgefpga_ppg_top.v` (`0x1`..`0x8`).

## Rebuilding the bitstream

`convert_bitstream.py` reads
`forgefpga_project/ffpga/build/bitstream/FPGA_bitstream_MCU.bin` and writes
`firmware/shrikefi/forgefpga_bitstream.h`, which the ESP32 firmware flashes to
the FPGA over I2C at boot.

See the known-issue note in [`firmware/shrikefi/README.md`](../../firmware/shrikefi/README.md):
the committed header is currently **older** than the build output on disk, so the
header must be regenerated and the result verified on hardware before flashing.
