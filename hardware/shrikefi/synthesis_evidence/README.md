# ShrikeFi ForgeFPGA synthesis evidence

Raw fitter reports for the ShrikeFi target. There are **two files** here, from two
different designs, and quoting the wrong one is the easiest way to misstate this
project.

| File | Design | Status |
|---|---|---|
| `resource_utilization_parallel_link_superseded.log` | 4-bit parallel link | **Superseded** — the interconnect was replaced |
| `resource_utilization_spi_link.log` | 4-wire SPI link + 32-bit IBI burst | **Current** — latest fitter run, 2026-10-18 01:40 |

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

| Resource | parallel link (superseded) | **SPI + IBI burst (current)** |
|---|---|---|
| CLB LUT5s | 443 / 1120 (39.55%) | **535 / 1120 (47.77%)** |
| — of which Lut5 | 443 | 535 |
| — as shift-register | 0 | 0 |
| FFs | 353 | **303** (298 CLB + 5 IOB) |
| CLBs | 85 / 140 (60.71%) | **108 / 140 (77.14%)** |
| 4k BRAMs | 0 / 8 | **0 / 8** |
| PLLs | 1 / 1 (100%) | **0 / 1 (0%)** |
| OSCs | 0 / 1 | **1 / 1 (100%)** |

Consequences worth knowing:

* **The footprint figures previously quoted in `README.md` and
  `docs/MIGRATION.md` (443 LUT5s / 353 FFs / 85 CLBs) belong to the superseded
  parallel design.** They describe a design that no longer exists.
* **The "consumes the part's only PLL" warning was an artefact of that same
  superseded build.** The SPI design runs from the on-chip oscillator and leaves
  the PLL free.
* **CLB occupancy is now the binding constraint.** The current fit uses 108 of
  140 CLBs (77.14%, 32 free) even though only 535 of 1120 LUT5s are used
  (47.77%, 585 free). Check new logic against CLB count first; the LUT headroom
  alone is misleading.

## Fit history of the SPI design

Every figure below is a real measurement of a different netlist. Quote only the
last row as current.

| Fit | CLB LUT5s | FFs | CLBs | What changed |
|---|---|---|---|---|
| 2026-09-16 | 342 / 1120 (30.54%) | 194 | 76/140 | shared `hardware/common/` sources + internal power-on reset |
| 2026-09-21 | 363 / 1120 (32.41%) | 202 | 75/140 | `STATE_ARMED` positive-slope gate (H-01) |
| 2026-10-17 | 362 / 1120 (32.32%) | 204 | 74/140 | re-fit of the pre-burst flat source; bitstream `2b204d00…` |
| 2026-10-18 | **535 / 1120 (47.77%)** | **303** | **108/140** | 32-bit IBI burst (`0xC3`) + dummy-byte suppression; bitstream `50623aa2…` |

The 342 → 363 delta was the slope comparison added to the peak detector's arm
condition, which stops a still-decaying pulse tail from being counted as a
second beat. The 2026-10-17 re-fit of the same pre-burst sources landed at 362
LUT5s / 204 FFs (one LUT fewer, two FFs more), which is normal fitter variation.

The jump to 535 LUT5s / 303 FFs is the **IBI burst feature, not fitter noise**:
the FPGA now latches a 32-bit IBI and queues the five-byte `0xC3` response,
which widens the interval/IBI counters and adds the burst state machine and
byte-select muxes. It also suppresses the dummy MOSI bytes during a burst so
they never enter the 8-tap filter. That is a deliberate capability increase; it
is still well inside the fabric, but it is no longer a "small" fit.

## Timing: read the constraint before believing the report

`PNR_TIMING.log` from the 2026-10-18 run reports, at POST-ROUTE:

```
Group        No. Clocks  No. Clock Pairs  WNS(ps)   TNS(ps)     TNS Endpoints
<UDEF_clk>   1           1                -10219    -6515944    1355
```

That looks alarming and is **not** a timing failure. `forgefpga_pins.pcf`
declares only `set_io` lines — there is no clock constraint anywhere in the
design — so the fitter auto-constrains `clk` to **2000 ps (500 MHz)**. The same
report gives the design's achievable period:

| Constrained period | Achievable period | Achievable frequency |
|---|---|---|
| (auto) 2000 ps | 12218 ps | **81.846 MHz** |

The reported slack is exactly the gap between those two numbers:
`2000 − 12218 = −10218 ps`, against a reported WNS of −10219 ps. Every register
in the design "fails" a 500 MHz check, which is why the 1355 endpoints are spread
across the whole design rather than sitting on one path.

At the **50 MHz** this design is specified and documented at (20,000 ps), the
margin is **+7,782 ps (+7.782 ns)**.

So: the auto-generated report is not evidence that timing fails, and it is not
evidence that timing closes either — the constraint it checks against is
meaningless. A real `set_frequency`/SDC constraint for 50 MHz is the missing
piece, and until it exists this log cannot be used as timing evidence either way.
**Do not quote WNS −10.219 ns as a real number**, and do not quote 81.846 MHz as
"timing closure".

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
designs. The RTL testbench is a separate gate: run
`build_shrikefi_sim.bat` from `hardware/shrikefi/` and expect
**13/13 checks passing**.
