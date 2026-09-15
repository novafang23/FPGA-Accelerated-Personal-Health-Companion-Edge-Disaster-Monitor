# ShrikeFi hardware (Renesas ForgeFPGA `SLG47910C`)

RTL and synthesis evidence for the **ShrikeFi** target: an ESP32-S3 plus a
Renesas ForgeFPGA (1120 five-input LUTs) connected by a **4-wire SPI link**.
The port is implemented — see [docs/MIGRATION.md](../../docs/MIGRATION.md).

> **The interconnect is SPI, not the 4-bit parallel link described in some older
> notes.** The parallel nibble protocol (`link_strobe` / `link_dir` /
> `link_din[3:0]` / `link_dout[3:0]` / `irq_beat`) was replaced by commit
> `a44013f`. If you find a document, a comment or a testbench that drives those
> signals, it is stale — see [docs/SHRIKEFI_LINK_PROTOCOL.md](../../docs/SHRIKEFI_LINK_PROTOCOL.md)
> for current protocol status.

## RTL layout

| File | Purpose |
|---|---|
| `forgefpga_ppg_top.v` | ForgeFPGA top level: SPI target, 8-tap filter + crest detector instantiation, beat latch, LED pulse stretcher |
| `../common/moving_average_8tap.v` | 8-tap moving-average filter — shared with the Zynq build |
| `../common/ppg_peak_detector.v` | 4-state systolic crest detector with 250 ms refractory — shared with the Zynq build |
| `tb_forgefpga_system.v` | Self-checking SPI testbench (5 test groups, 10 checks) |
| `gen_flat_source.py` | Generates the vendor-facing flat source set + vendor-side testbench copy |
| `forgefpga_project/` | Renesas ForgeFPGA Workshop project |
| `forgefpga_pins.pcf` / `.pdc` | Placement constraints — **authoritative for pin assignment** |
| `build_shrikefi_sim.bat` | Compile + run the testbench with Icarus Verilog |
| `convert_bitstream.py` | Convert the vendor bitstream `.bin` into `firmware/shrikefi/forgefpga_bitstream.h` |

### The shared DSP modules and the flat vendor source set

`forgefpga_ppg_top.v` **instantiates** `moving_average_8tap` and
`ppg_peak_detector` from `hardware/common/`. It deliberately does **not** contain
inline copies of them.

The Renesas toolchain needs one flat source set, which is the only reason
duplication exists at all. That duplication is **generated**, not maintained by
hand:

```
python gen_flat_source.py            # regenerate
python gen_flat_source.py --check    # fail if stale (runs in CI)
```

This replaces the previous arrangement, in which the two DSP modules were pasted
into this file by hand. They drifted: the pasted copy gained two crest-detection
fixes that `hardware/common/` never received, so the Zynq path silently kept an
older detector. `gen_flat_source.py` refuses to run if you paste a module back
into the top level.

> **Gotcha (still true, now enforced):** compiling `forgefpga_ppg_top.v`
> *together with* an inline copy of either DSP module fails with
> `'moving_average_8tap' has already been declared in this scope`. Compile the
> top **once** with the two `../common/` sources, nothing more — that is exactly
> what `build_shrikefi_sim.bat` and CI do.

## Simulate

```bat
build_shrikefi_sim.bat
```

Expected result: `RESULTS: 10 passed, 0 failed`. The same command runs in CI.

The testbench drives the DUT as the ESP32-S3 firmware actually does (8-bit SPI,
mode 0, MSB first) and checks: the internal power-on reset releases and the link
is defined; the 8-tap average converges and is bit-exact against a sliding-window
model; a synthetic pulse train yields exactly one beat flag per crest; and beat
spacing matches the stimulus period.

## Synthesis results

Latest fitter run of the SPI design (`synthesis_evidence/resource_utilization_spi_link.log`):

| Resource | Usage |
|---|---|
| CLB LUT5s | **222 / 1120 (19.82%)** |
| FFs | **121** (117 CLB + 4 IOB) |
| CLBs | **40 / 140 (28.57%)** |
| Block RAMs | **0 / 8** |
| PLLs | **0 / 1** (runs from the on-chip oscillator) |

**Do not quote 443 LUT5s / 353 FFs / 85 CLBs.** Those figures are from the
superseded 4-bit parallel design and roughly double the real usage — see
[`synthesis_evidence/README.md`](synthesis_evidence/README.md) for the full
comparison and the port signature that tells the two logs apart.

**These numbers are themselves stale.** The RTL was subsequently refactored to
use the shared DSP modules and to generate its own power-on reset, which changes
the netlist. Re-run the fitter before quoting a footprint figure.

## Interconnect

The link is 8-bit SPI, mode 0, MSB first, one transfer per PPG sample: MOSI
carries the raw sample, MISO returns `{beat_latched, filt_sample[6:0]}`. Pin
assignment is defined by `forgefpga_pins.pcf`, not by any prose document. The
driver side is `firmware/shrikefi/shrikefi_link_driver.c`.

`docs/SHRIKEFI_LINK_PROTOCOL.md` still specifies the superseded parallel nibble
protocol, including pin numbers that now collide with the SPI assignments in the
`.pcf`. Treat that document as describing an earlier revision until it is
rewritten.

## Rebuilding the bitstream

`convert_bitstream.py` reads
`forgefpga_project/ffpga/build/bitstream/FPGA_bitstream_MCU.bin` and writes
`firmware/shrikefi/forgefpga_bitstream.h`, which the ESP32 firmware attempts to
flash to the FPGA over I2C at boot.

> **The embedded bitstream no longer matches the RTL.** The shared-source
> refactor and the internal power-on reset changed the netlist, and no ForgeFPGA
> toolchain was available to re-fit it. Re-run the fitter and regenerate
> `forgefpga_bitstream.h` before treating the flashed bitstream as
> corresponding to this RTL. The detector logic it was built from is
> behaviourally identical to `hardware/common/` (verified by differential
> simulation) — the new part is the reset distribution.

Also note the firmware's I2C delivery routine may not actually program the part
at all — see the FPGA-delivery note in
[`firmware/shrikefi/README.md`](../../firmware/shrikefi/README.md) before relying
on the auto-flash path.
