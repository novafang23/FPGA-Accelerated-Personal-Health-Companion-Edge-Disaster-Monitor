# ShrikeFi ForgeFPGA synthesis evidence

Raw utilization report backing the ShrikeFi figures quoted in `README.md` and
`docs/MIGRATION.md`.

## `resource_utilization.log`

Source: `forgefpga_project/ffpga/build/resource-utilization-report.log`
(the whole `ffpga/build/` tree is gitignored, so this is the tracked copy)

| Field | Value |
|---|---|
| Tool | Renesas ForgeFPGA Workshop v6.55 |
| Target | `SLG47910C` (1120 five-input LUTs) |
| Top module | `forgefpga_ppg_top` |
| **CLB LUT5s** | **443 / 1120 (39.55%)** |
| FFs | **353** (345 CLB FFs @ 30.80% + 8 IOB FFs @ 1.09%) |
| CLBs | **85 / 140 (60.71%)** |
| BRAMs | **0 / 8** |
| PLLs | **1 / 1 (100%)** |
| GPIOs | 1 / 19 |

## Do not quote the older 195-LUT figure

Earlier revisions of `README.md` and `docs/theory/THEORY_NOTES.md` quoted
**195 / 1120 LUT5s (17.41%), 110 FFs, 35/140 CLBs**. That is a stale
pre-link-transceiver build. The design grew when the 4-bit MCU link FSM and
status/IRQ registers were added. The numbers in the table above are the
post-synthesis values for the shipped RTL, and `docs/MIGRATION.md` already
matches them.

## Headroom note

At **443 / 1120 LUT5s (39.55%)** and **85 / 140 CLBs (60.71%)** the design is
comfortable on LUTs but noticeably tighter on CLBs, and it consumes the part's
only PLL. Any significant expansion of the DSP chain should re-check the CLB
budget, not just the LUT count.
