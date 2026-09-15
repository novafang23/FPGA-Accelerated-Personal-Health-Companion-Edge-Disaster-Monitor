# ShrikeFi ForgeFPGA synthesis evidence

Raw fitter reports for the ShrikeFi target. There are **two**, from two different
designs, and quoting the wrong one is the easiest way to misstate this project.

| File | Design | When | Status |
|---|---|---|---|
| `resource_utilization_parallel_link_superseded.log` | 4-bit parallel link | committed 2026-09-11 | **Superseded** — the interconnect was replaced |
| `resource_utilization_spi_link.log` | 4-wire SPI link | fitter run 2026-09-13 03:35 | Latest measurement of the SPI design |

Both are byte-identical copies of the Renesas ForgeFPGA Workshop fitter output
(`forgefpga_project/ffpga/build/resource-utilization-report.log`); the whole
`ffpga/build/` tree is gitignored, so these are the tracked copies.

## Why there are two

Commit `a44013f` (2026-09-12 23:24) replaced the 4-bit parallel MCU link with a
4-wire SPI target and replaced the PLL clocking with the on-chip oscillator. The
older log was committed *before* that rewrite, so its numbers describe a design
that no longer exists.

The port signature makes the difference unambiguous — it is visible directly in
each log:

| Field | parallel link (superseded) | SPI link (latest) |
|---|---|---|
| RTL input ports | 8 | 4 |
| RTL output ports | 6 | 5 |
| GPIOs | 1 / 19 | 5 / 19 |
| PLLs | **1 / 1** | **0 / 1** |
| OSCs | 0 / 1 | **1 / 1** |

## Comparison

| Resource | parallel link (superseded) | **SPI link (latest)** |
|---|---|---|
| CLB LUT5s | 443 / 1120 (39.55%) | **222 / 1120 (19.82%)** |
| — of which Lut5 | 443 | 206 |
| — as shift-register | 0 | 16 |
| FFs | 353 | **121** (117 CLB + 4 IOB) |
| CLBs | 85 / 140 (60.71%) | **40 / 140 (28.57%)** |
| 4k BRAMs | 0 / 8 | **0 / 8** |
| PLLs | 1 / 1 (100%) | **0 / 1 (0%)** |
| OSCs | 0 / 1 | **1 / 1 (100%)** |

Two consequences worth knowing:

* **The footprint figures previously quoted in `README.md` and
  `docs/MIGRATION.md` (443 LUT5s / 353 FFs / 85 CLBs) belong to the superseded
  parallel design.** They roughly double the real usage of the SPI design.
* **The "consumes the part's only PLL" warning was an artefact of that same
  superseded build.** The SPI design runs from the on-chip oscillator and leaves
  the PLL free. Headroom is therefore better than previously documented, though
  CLB occupancy is still the metric to watch — LUTs are the comfortable one.

## These numbers are already stale again

The RTL has since been refactored to take the filter and peak detector from
`hardware/common/` (generated into the flat source set by
`gen_flat_source.py`) and to generate its own power-on reset instead of tying
`rst_n` to `1'b1`. Both change the netlist, so **the SPI figures above do not
describe the current `hardware/shrikefi/forgefpga_ppg_top.v` either.** The
detector behaviour is unchanged (proved by differential simulation against the
pre-refactor inline copy), but the reset distribution is new.

Re-run the ForgeFPGA fitter and replace `resource_utilization_spi_link.log`
before quoting any footprint number again. No ForgeFPGA toolchain was available
in the environment where this refactor was done, so this is a known gap, not a
verified figure.

## Reproducing

`forgefpga_project/` is the vendor project (open `FPGA-SHRIKE.ffpga` in Renesas
ForgeFPGA Workshop). Regenerate the flat source set and the vendor-side
testbench copy from the shared sources first:

```
python hardware/shrikefi/gen_flat_source.py
```
