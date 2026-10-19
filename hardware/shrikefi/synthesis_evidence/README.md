# ShrikeFi ForgeFPGA synthesis evidence

Raw fitter reports for the ShrikeFi target. There are **two files** here, from two
different designs, and quoting the wrong one is the easiest way to misstate this
project.

| File | Design | Status |
|---|---|---|
| `resource_utilization_parallel_link_superseded.log` | 4-bit parallel link | **Superseded** — the interconnect was replaced |
| `resource_utilization_spi_link.log` | 4-wire SPI link + 32-bit IBI burst | **Current** — latest fitter run, 2026-10-19 |
| `timing_summary_spi_link_50mhz.log` | 4-wire SPI link + 32-bit IBI burst | **Current** — first fit checked against a real 50 MHz constraint, 2026-10-20 |

All are byte-identical copies of Renesas ForgeFPGA Workshop fitter output
(`forgefpga_project/ffpga/build/resource-utilization-report.log` and
`.../PNR_TIMING.log`); the whole `ffpga/build/` tree is gitignored, so these are
the tracked copies. `resource_utilization_spi_link.log` is still byte-identical
after the 2026-10-20 constrained re-fit: adding the clock constraint moved
routing but changed no resource count. The timing file is the report header only
— the full `PNR_TIMING.log` is 6.8 MB of per-path detail and the header carries
the summary.

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
| CLB LUT5s | 443 / 1120 (39.55%) | **515 / 1120 (45.98%)** |
| — of which Lut5 | 443 | 515 |
| — as shift-register | 0 | 0 |
| FFs | 353 | **303** (298 CLB + 5 IOB) |
| CLBs | 85 / 140 (60.71%) | **105 / 140 (75.00%)** |
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
* **CLB occupancy is now the binding constraint.** The current fit uses 105 of
  140 CLBs (75.00%, 35 free) even though only 515 of 1120 LUT5s are used
  (45.98%, 605 free). Check new logic against CLB count first; the LUT headroom
  alone is misleading.

## Fit history of the SPI design

Every figure below is a real measurement of a different netlist. Quote only the
last row as current.

| Fit | CLB LUT5s | FFs | CLBs | What changed |
|---|---|---|---|---|
| 2026-09-16 | 342 / 1120 (30.54%) | 194 | 76/140 | shared `hardware/common/` sources + internal power-on reset |
| 2026-09-21 | 363 / 1120 (32.41%) | 202 | 75/140 | `STATE_ARMED` positive-slope gate (H-01) |
| 2026-10-17 | 362 / 1120 (32.32%) | 204 | 74/140 | re-fit of the pre-burst flat source; bitstream `2b204d00…` |
| 2026-10-18 | 535 / 1120 (47.77%) | 303 | 108/140 | 32-bit IBI burst (`0xC3`) + dummy-byte suppression; bitstream `50623aa2…` |
| 2026-10-19 | **515 / 1120 (45.98%)** | **303** | **105/140 (75.00%)** | `crest_fall_min` raised from peak/8 to **peak/4** to reject the dicrotic notch; bitstream `d9d58daa…` |

| 2026-10-20 | **515 / 1120 (45.98%)** | **303** | **105/140 (75.00%)** | no RTL change: a real 50 MHz `create_clock` was added, so this is the first fit timed against its actual clock; bitstream `6930ba88...` |

The 2026-10-19 crest refit is **smaller** than the 2026-10-18 burst fit:
515 LUT5s / 303 FFs / 105 CLBs, with **35 CLBs free**. The change was a constant
bit-select in an existing comparator, so this is consistent with fitter
variation and not a resource increase.

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

## Timing: constrained at 50 MHz and closed (2026-10-20)

Until 2026-10-20 the project carried **no clock constraint at all**:
`forgefpga_pins.pcf` declares only `set_io` lines and `FPGA-SHRIKE.ffpga` had an
empty `<timing-constraints/>` element, so the fitter auto-constrained every
register to a **2000 ps (500 MHz)** placeholder. Every WNS reported before that
date was therefore measured against a frequency this design never runs at, and
was unusable in both directions.

The project now has `ffpga/timing-constraints/timing_constraints.sdc`, referenced
from `FPGA-SHRIKE.ffpga`, and it contains one line:

```tcl
create_clock -name clk {clk} -period 20.000 -waveform {0 10.000}
```

That is the **complete** constraint, because the design has exactly one clock:
`clk`, the SLG47910 internal 50 MHz oscillator (`clk_en -> OSC_EN`; it has no
`set_io` line because it is an internal resource, not a pin). `spi_sck` is **not**
a clock domain — the SPI lines are sampled inside the `clk` domain. The ESP32
drives SCK at 1 MHz for the runtime link and 16 MHz while flashing, both far
below 50 MHz, so a second `create_clock` and a `set_clock_groups` would describe
hardware that does not exist. The syntax follows Renesas AN-FG-036 and the
installed `an-fg-024` example project verbatim.

`PNR_TIMING.log` from the 2026-10-20 constrained run reports, at POST-ROUTE:

```
Group        No. Clocks  No. Clock Pairs  WNS(ps)  TNS(ps)  TNS Endpoints
<DEFAULT>    1           1                +7316    0        0
```

| Clock | Constrained period | Waveform | Achievable period | Achievable frequency |
|---|---|---|---|---|
| `clk` | **20,000 ps (50 MHz)** | (0, 10,000) | 12,683 ps | 78.846 MHz |

**WNS +7,316 ps (+7.316 ns), TNS 0, zero failing endpoints** across 1260
endpoints: the design closes timing at the 50 MHz it is specified for, and the
fitter still finds headroom to 78.846 MHz. The tracked copy is
`timing_summary_spi_link_50mhz.log`.

The earlier unconstrained run reported `-9357 ps` against `(auto) 2000 ps` with
an achievable period of 11,356 ps (88.059 MHz); the 2026-10-18 run reported
-10.219 ns / 81.846 MHz. Both are historical. Adding the constraint changed
routing without changing resources: the bitstream hash moved from `d9d58daa…` to
`6930ba88…`, while `resource_utilization_spi_link.log` is byte-identical.

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
