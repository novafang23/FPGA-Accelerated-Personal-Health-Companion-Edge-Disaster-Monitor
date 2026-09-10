# Zynq-7000 synthesis evidence

Raw report snapshots backing the Zynq resource and timing figures quoted in the
top-level `README.md`. `.runs/` is gitignored, so these copies are the tracked
evidence — they are byte-identical to the Vivado-generated originals.

## 1. `ooc_utilization_synth.rpt` — out-of-context accelerator utilization

Source: `vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.runs/design_ZYNQ_axi_ppg_accelerator_0_0_synth_1/`

| Field | Value |
|---|---|
| Tool | Vivado v.2026.1 (win64) |
| Date | Fri Sep 4 13:54:12 2026 |
| Top module | `axi_ppg_accelerator` (out-of-context) |
| Slice LUTs | **198** (182 as Logic + **16** as Memory/LUTRAM) |
| Slice Registers | **267** |
| DSPs | **0** |
| Block RAM Tile | **0** |

## 2. `post_route_timing_summary.rpt` — integrated post-route timing

Source: `vivado_project/FPGA MEDTECH DEVICE ZYNQ-7000.runs/impl_1/`

This is the **full block design** (`design_ZYNQ_wrapper`: PS7 + AXI SmartConnect
+ accelerator), post place-and-route — not the out-of-context accelerator.

| Field | Value |
|---|---|
| WNS | **+12.836 ns** |
| WHS | **+0.106 ns** |
| Total endpoints | 1,806 |
| Result | *All user specified timing constraints are met.* |
| Slice LUTs (placed) | 562 |
| Slice Registers (placed) | 667 |

## Which numbers to use where — read this before quoting

There are **two distinct runs** in this project's history, and conflating them is
the easiest way to misstate the design:

| Figure | Run | Evidence |
|---|---|---|
| **185 LUT / 16 LUTRAM / 266 FF / WNS +5.603 ns / Fmax 69.45 MHz** @ 50 MHz target | Tagged baseline `v1.0-zynq-SIH`, Vivado ML 2022.2, out-of-context | Tag `v1.0-zynq-SIH`; screenshots in `docs/images/timing_summary.png`, `utilization_percentage.png`. **No raw `.rpt` for this run survives** — the `.runs/` tree in this repo is from the re-run below. |
| **198 LUT / 267 FF**, integrated post-route **WNS +12.836 ns** | Re-synthesis, Vivado 2026.1, Sep 2026 | `ooc_utilization_synth.rpt` and `post_route_timing_summary.rpt` in this directory |

Notes for anyone citing these:

* The **185-LUT / +5.603 ns** figures are the frozen hackathon baseline and are
  still valid as the cited result for the tagged commit, but their only in-repo
  evidence is the PNG screenshots — the raw report was not preserved before the
  `.runs/` tree was regenerated.
* The **+12.836 ns** WNS is *not* a regression of the +5.603 ns OOC result:
  it is a different measurement of a larger design (the whole block design with
  the PS7 and interconnect included, 1,806 endpoints rather than the
  accelerator alone).
* Both runs agree on the two claims that matter most for the architecture:
  **0 DSP48** and **0 BRAM**.
