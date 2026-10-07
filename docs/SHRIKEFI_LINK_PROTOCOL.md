# ShrikeFi FPGA–MCU link protocol

> **This document was rewritten on 2026-09-28.** It previously specified a
> synchronous **4-bit parallel nibble bus** with a strobe line, a direction line,
> a command codebook (`0x1`–`0x8`) and a dedicated `irq_beat` interrupt pin, with
> pin numbers that collided with the real ones. **That design was retired before
> it was ever built.** Nothing in the firmware or the RTL implements it. The link
> is 8-bit SPI, as described below. See §8 for what the old design was and why
> references to it may still be found elsewhere in the repository.

## 1. Overview

The ESP32-S3 and the Renesas ForgeFPGA (SLG47910) are connected by a plain
**4-wire SPI bus, mode 0**, with the ESP32-S3 as controller and the ForgeFPGA as
target. The ESP32-S3 drives it with the ESP-IDF `spi_master` driver on
`SPI2_HOST`; the ForgeFPGA implements a shift register in
`hardware/shrikefi/forgefpga_ppg_top.v`.

There is no AXI bus and no ARM processing system on this board, so there is no
memory-mapped register file — but there is also no bespoke parallel protocol.
The ForgeFPGA is an SPI peripheral that happens to run the PPG filter and peak
detector, and the whole interface is one byte in each direction.

> **WARNING — 3.3 V ONLY.** Every I/O pin on both the ESP32-S3 and the ForgeFPGA
> is 3.3 V LVCMOS. Applying 5 V to any GPIO permanently destroys the IC. 5 V is
> used only for the PMS5003 sensor's VCC rail, sourced from USB VBUS.

## 2. Physical interface and pin assignment

These pins are **internal PCB traces** on the ShrikeFi board — no external wiring
is required for the link.

| Signal | FPGA pad | ESP32-S3 GPIO | Direction | Notes |
|---|:---:|:---:|---|---|
| `spi_sck` | PIN_16 | **GPIO 12** | MCU → FPGA | SPI clock, mode 0 |
| `spi_ss_n` | PIN_17 | **GPIO 10** | MCU → FPGA | Chip select, active low, driven manually per transaction |
| `spi_mosi` | PIN_18 | **GPIO 11** | MCU → FPGA | Sample data to the FPGA |
| `spi_miso` | PIN_19 | **GPIO 13** | FPGA → MCU | Result byte from the FPGA (PIN_19_OE is its output enable) |
| — | — | **GPIO 8** | MCU → FPGA | `PIN_FPGA_EN` — enable / boot-mode latch |
| — | — | **GPIO 9** | MCU → FPGA | `PIN_FPGA_PWR` — FPGA power control |
| `clk_en` | OSC_EN | — | — | On-chip oscillator enable (a resource, not a GPIO) |
| `led_user` | PIN_7 | — | — | Blue user LED D12, pulsed on each detected beat (PIN_7_OE) |

**There is no reset pin.** PIN_13 is not connected to anything; the RTL resets
from an internal power-on counter. Any table listing `rst_n` on GPIO 3 or GPIO 11
is describing the retired design and is wrong — GPIO 11 is MOSI.

**There is no interrupt pin.** The beat is reported as **bit 7 of the byte
returned on MISO**, not by a separate line.

Sources of truth: `firmware/shrikefi/shrikefi_pinmap.h` (ESP32 side),
`hardware/shrikefi/forgefpga_pins.pcf` (FPGA side),
`firmware/shrikefi/shrikefi_link_driver.c` (behaviour).

> **A trap when reading the `.pcf`.** Its inline comments look like
> `set_io spi_sck PIN_16 # Connected to ESP32 GPIO12 (F_SP_CLK) -> GPIO3_IN`.
> The `-> GPIO3_IN` suffix is an **FPGA-side port designator**, not an ESP32 GPIO
> number. Reading it as one is the most likely origin of the wrong pin tables
> this document was rewritten to remove.

## 3. Data framing

There is **no command map, no header, no payload length and no address**. Each
optical sample is one 8-bit full-duplex SPI transaction:

```
        ┌─────────────────────────────────────────────────┐
 MCU ───┤ MOSI: [ sample[7:0] ]  — the raw IR sample     ├───▶ FPGA
        │                                                 │
 MCU ◀──┤ MISO: [ beat | filt[6:0] ]  — one result byte   ├──── FPGA
        └─────────────────────────────────────────────────┘
                 one 8-bit transfer, CS pulsed low
```

| Byte | Field | Meaning |
|---|---|---|
| MOSI | `sample[7:0]` | The 8-bit IR sample the MCU wants filtered |
| MISO bit 7 | `beat_latched` | 1 = the peak detector latched a systolic crest |
| MISO bits 6:0 | `filt_sample[6:0]` | The **low seven bits** of the 8-tap moving average |

Two consequences that matter when reading a capture:

- **The returned byte is not a waveform.** The moving average is a full 8-bit
  quantity, so the FPGA packs the beat flag into bit 7 and only the low 7 bits
  come back. The logged value therefore wraps at 128 and looks like a sawtooth.
  The FPGA's own peak detector uses the full 8 bits internally, so detection is
  unaffected — only the diagnostic is. `hardware/shrikefi/tools/replay_fpga_link_log.py`
  reconstructs the true 8-bit value from the input stream.
- **The handshake probe byte never comes back.** At reset the RTL initialises its
  response register to `0xA5`, but the next clock overwrites it with
  `{beat_latched, filt_sample[6:0]}`, which is `0x00` before any beat and before
  the filter has filled. So the boot log's
  `handshake: probe sent 0x55, received 0x00` is the expected result, not a
  fault. Do not use the probe byte as a liveness signature. Liveness comes from
  the beat flag, and the firmware latches it from the first beat the FPGA
  reports.

**Beat timing is measured on the MCU, not the FPGA.** The RTL computes a 32-bit
IBI counter internally (`ibi_cycles` at 50 MHz clock = 20 ns per tick), but there is no
room for a 32-bit integer in the 8-bit SPI reply `{beat, filt[6:0]}` and it is not
transmitted across the link. `shrikefi_link_driver.c` timestamps each rising beat flag
with `esp_timer_get_time()` and differences consecutive timestamps, expressing the
delta in 50 MHz-equivalent cycles (`s_sim_ibi = delta_us * 50`).

This is the architectural contract: peak detection in hardware executes at 50 MHz with
cycle-accurate fidelity, but the reported IBI resolution in software is determined by
the discrete SPI sample cadence (~10 ms at 100 Hz sampling) and the ESP32 timer (1 µs),
not the FPGA's 20 ns tick. Because RMSSD is the RMS of successive interval *differences*, this
quantisation is an inherent property of the ~100 Hz discrete sampling architecture,
and it is one reason T4.1 (validate against a reference device) is documented for field verification.

## 4. Timing

| Quantity | Value | Source |
|---|---|---|
| Link mode | SPI mode 0 (CPOL=0, CPHA=0) | `shrikefi_link_driver.c` |
| Runtime link clock | **1 MHz** | `shrikefi_link_driver.c` |
| One transaction | 8 bits = **8 µs** | derived |
| Sample cadence | **~100 Hz** (measured ~105/s over an 81 s capture) | `reports/live_test_runs/` |
| Link duty cycle at 100 Hz | 8 µs per 10 ms ≈ **0.08 %** | derived |
| Flashing clock | **16 MHz** | `shrikefi_link_driver.c` |
| Bitstream | **46,408 bytes** in 256-byte chunks (182 transfers) | `forgefpga_bitstream.h` |
| Pure clocking time to flash | 371,264 bits ÷ 16 MHz ≈ **23.2 ms** | derived |
| FPGA logic clock | **50 MHz** on-chip oscillator, enabled via `OSC_EN` | `docs/reference/shrike_board/` |

**On ForgeFPGA timing evidence.** The fitter report `PNR_TIMING.log` shows
WNS −10.088 ns, but that figure is an artefact: the design declares no clock
constraint, so the tool auto-constrains `clk` to 500 MHz (2000 ps). The
**achievable period is 12,087 ps (82.73 MHz)**, so at the documented 50 MHz the
margin is **+7.913 ns**. Do not quote −10.088 ns as a result. Adding a real
constraint to the flow is still open work (T4.5 in `docs/REMAINING_WORK.md`).

## 5. FPGA configuration

The bitstream is delivered over the **same SPI bus**, not by a separate
programmer. `shrikefi_fpga_flash_init()` in `shrikefi_link_driver.c`:

1. Configures PWR, EN and SS as outputs.
2. Initialises `SPI2_HOST` at 16 MHz, mode 0, with **manual CS**.
3. Runs the vendor boot-mode latch sequence — PWR=0/EN=0/SS=1, settle 3 ms;
   PWR=1/EN=1/SS=0, settle 10 ms; SS=1, settle 1 ms. (These use
   `esp_rom_delay_us`, not `vTaskDelay(pdMS_TO_TICKS(..))`: at
   `CONFIG_FREERTOS_HZ=100` a tick is 10 ms, so `pdMS_TO_TICKS(5)` is **zero
   ticks** and the latch window would never be held.)
4. Streams the 46,408-byte image in 256-byte chunks with SS toggled per chunk.
5. Waits 50 ms for user mode, removes the flashing device and leaves
   `SPI2_HOST` up for runtime communication.

The image costs 0.6 % of the 7 MB application partition, which is why it is
always embedded rather than made optional — an option that can silently turn the
FPGA path off is a worse failure than 46 KB of flash.

## 6. Comparison with the Zynq AXI4-Lite interface

The Zynq baseline (`hardware/zynq/`, `docs/HARDWARE_ARCHITECTURE.md`) remains
valid and citable; it is a different interconnect, not a port of this one.

| Function | Xilinx Zynq-7000 (AXI4-Lite) | ForgeFPGA on ShrikeFi (SPI) |
|---|---|---|
| Interconnect | 32-bit memory-mapped at `0x43C00000` | 8-bit full-duplex SPI, one transfer per sample |
| Write a raw sample | Write `REG_RED_RAW` / `REG_IR_RAW` | The MOSI byte of the transaction |
| Read the filtered output | Read the filtered-output registers | Bits 6:0 of the MISO byte |
| Read the IBI | Read `REG_IBI_CYCLES` (32-bit) | **Not available** — measured on the MCU |
| Set the peak threshold | Write `REG_STATUS_THRESH[15:8]` | **Not implemented** — `shrikefi_set_threshold()` only stores a value; the RTL hard-codes `dyn_threshold` |
| Clear the beat flag | Write 1 to `REG_STATUS_THRESH[0]` (W1C) | Implicit: the flag is high for the one transaction that carries it |
| Interrupt to the host | AXI fabric interrupt to the ARM GIC | **None** — polled as bit 7 of the MISO byte |

The two "not available" rows are the honest gaps between the platforms. Neither
is required for the device to work, and both are recorded rather than papered
over.

## 7. What is not on the wire

- **No command map.** There is no `CMD_*` decode in the RTL and no `SHRIKEFI_CMD_*`
  constant in use in the driver.
- **No strobe, no direction line, no nibble framing.**
- **No separate beat interrupt.**
- **No threshold write path.** `shrikefi_set_threshold()` is a no-op against real
  hardware; the FPGA's peak threshold is fixed at synthesis.
- **No FPGA-side IBI.** See §3.

## 8. History: the retired 4-bit parallel design

For anyone who finds a reference to it: ShrikeFi was originally specified with a
synchronous 4-bit parallel link — a `link_strobe` clock, a `link_dir` direction
line, a bidirectional `link_data[3:0]` bus, a `CMD_*` codebook and an `irq_beat`
interrupt — on GPIO 4/5/6-9/10 with FPGA pads PIN_14-19 and PIN_24. It was
abandoned before implementation in favour of SPI: the ForgeFPGA is a small
device (1120 LUT5s) with no need for a bespoke bus, the ESP32-S3 has a hardware
SPI controller, and one 8-bit transaction per sample carries everything the link
actually needs.

The old pin numbers **collide** with the real ones (its `link_data[0..3]` sat on
PIN_16-19, which are now SCK/SS/MOSI/MISO), so a document still carrying them is
not merely out of date — it will miswire a board.

If you find the retired design described elsewhere, it is a stale reference.
`docs/REMAINING_WORK.md` tracks the remaining cleanup.

### Figures

The two images previously referenced here — `images/shrikefi_pinout.png` and
`images/shrikefi_waveform.png` — are **no longer referenced**. They were produced
for the retired design, and their contents cannot be verified from the
repository (the pinout SVG carries an embedded raster, so its labels are not
searchable text). Treat the tables above as authoritative, and regenerate those
figures from them before using either in a document or a presentation.
