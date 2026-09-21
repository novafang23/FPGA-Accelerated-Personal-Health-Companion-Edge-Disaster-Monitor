# ShrikeFi ForgeFPGA synthesis evidence

Raw fitter reports for the ShrikeFi target. There are **two files** here, from two
different designs, and quoting the wrong one is the easiest way to misstate this
project.

| File | Design | Status |
|---|---|---|
| `resource_utilization_parallel_link_superseded.log` | 4-bit parallel link | **Superseded** — the interconnect was replaced |
| `resource_utilization_spi_link.log` | 4-wire SPI link | **Current** — latest fitter run, 2026-09-21 21:42 |

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

| Field | parallel link (superseded) | SPI link (current) |
|---|---|---|
| RTL input ports | 8 | 4 |
| RTL output ports | 6 | 5 |
| GPIOs | 1 / 19 | 5 / 19 |
| PLLs | **1 / 1** | **0 / 1** |
| OSCs | 0 / 1 | **1 / 1** |

## Comparison

| Resource | parallel link (superseded) | **SPI link (current)** |
|---|---|---|
| CLB LUT5s | 443 / 1120 (39.55%) | **363 / 1120 (32.41%)** |
| — of which Lut5 | 443 | 363 |
| — as shift-register | 0 | 0 |
| FFs | 353 | **202** (198 CLB + 4 IOB) |
| CLBs | 85 / 140 (60.71%) | **75 / 140 (53.57%)** |
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

## Fit history of the SPI design

Three different figures have been quoted for the SPI design. They are all real
measurements; they are just from different netlists.

| Fit | CLB LUT5s | FFs | CLBs | What changed |
|---|---|---|---|---|
| 2026-09-16 | 342 / 1120 (30.54%) | 194 | 76/140 | shared `hardware/common/` sources + internal power-on reset |
| 2026-09-21 | **363 / 1120 (32.41%)** | **202** | **75/140** | `STATE_ARMED` positive-slope gate (H-01) |

The +21 LUT5 / +8 FF delta is the slope comparison added to the peak detector's
arm condition, which stops a still-decaying pulse tail from being counted as a
second beat. That change is described in `hardware/common/ppg_peak_detector.v`
and covered by TEST C in `hardware/common/tb_ppg_peak_detector.v`.

The 342 figure in the 2026-09-16 row was tracked in
`resource_utilization_spi_link.log` but never propagated into the prose, which
kept quoting the earlier 222/121/40 in this file and
`hardware/shrikefi/README.md`. Both now quote the current fit.

## Timing: read the constraint before believing the report

`PNR_TIMING.log` from the 2026-09-21 run reports, at POST-ROUTE:

```
Group        No. Clocks  No. Clock Pairs  WNS(ps)   TNS(ps)     TNS Endpoints
<UDEF_clk>   1           1                -10088    -3247649    878
```

That looks alarming and is **not** a timing failure. `forgefpga_pins.pcf`
declares only `set_io` lines — there is no clock constraint anywhere in the
design — so the fitter auto-constrains `clk` to **2000 ps (500 MHz)**. The same
report gives the design's achievable period:

| Constrained period | Achievable period | Achievable frequency |
|---|---|---|
| (auto) 2000 ps | 12087 ps | **82.73 MHz** |

The reported slack is exactly the gap between those two numbers:
`2000 − 12087 = −10087 ps`, against a reported WNS of −10088 ps. Every register
in the design "fails" a 500 MHz check, which is why the 878 endpoints are spread
evenly across `u_peak_det` (1387), `u_ma_filter` (622) and `u_spi_target` (263)
rather than sitting on one path.

At the **50 MHz** this design is specified and documented at (20,000 ps), the
margin is **+7,913 ps**.

So: the auto-generated report is not evidence that timing fails, and it is not
evidence that timing closes either — the constraint it checks against is
meaningless. A real `set_frequency`/SDC constraint for 50 MHz is the missing
piece, and until it exists this log cannot be used as timing evidence either way.
Do not quote WNS −10.088 ns as a real number.

## Reproducing

`forgefpga_project/` is the vendor project (open `FPGA-SHRIKE.ffpga` in Renesas
ForgeFPGA Workshop). Regenerate the flat source set and the vendor-side
testbench copy from the shared sources first, then re-fit:

```
python hardware/shrikefi/gen_flat_source.py --check
python hardware/shrikefi/convert_bitstream.py --check
```

Both must pass before a fit is meaningful: the vendor tool compiles the flat
source set, and the firmware flashes the header that `convert_bitstream.py`
writes. If either is stale, the bitstream and the source tree describe different
designs.
