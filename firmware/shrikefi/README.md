# ShrikeFi firmware (ESP32-S3)

ESP-IDF application for the **ShrikeFi** platform (ESP32-S3 + Renesas ForgeFPGA
`SLG47910C`). The port is implemented — see
[docs/MIGRATION.md](../../docs/MIGRATION.md) for the platform roadmap and
[docs/SHRIKEFI_LINK_PROTOCOL.md](../../docs/SHRIKEFI_LINK_PROTOCOL.md) for the
SPI link protocol.

## What the application does

1. Boots under FreeRTOS and brings up I2C (MAX30102 PPG + BME280 environment),
   UART (PMSA003 / PMS5003 particulate sensor) and the SSD1306 OLED.
2. Configures the ForgeFPGA over **SPI2** from the bitstream embedded in
   `forgefpga_bitstream.h` (`shrikefi_fpga_flash_init()`), so the FPGA is
   programmed on every boot without an external programmer.
3. Streams raw IR samples to the FPGA over the SPI link and reads back
   `{beat_latched, filt_sample[6:0]}` — one 8-bit transaction per sample. There
   is no IBI register and no interrupt pin: the beat is bit 7 of the reply and
   the MCU derives the IBI from its own microsecond timestamps.
4. Runs the shared, hardware-agnostic algorithm core from `../core/`
   (HRV, SpO2, mNEWS2 triage, INT8 TinyML multi-hazard model).
5. Publishes telemetry over USB UART and WiFi/MQTT, and drives the OLED with an
   offline advisory.

## Layout

| File | Purpose |
|---|---|
| `main_shrikefi.c` | FreeRTOS application: sensor task, link task, telemetry, OLED |
| `shrikefi_link_driver.c/.h` | SPI2 link driver (8-bit frame, no command map) |
| `shrikefi_pinmap.h` | Single source of truth for every GPIO assignment |
| `esp32_i2c_hal.c/.h` | ESP-IDF I2C HAL implementing the shared HAL interface |
| `max30102.c/.h`, `bme280.c/.h`, `pms5003.c/.h`, `pmsa003.c/.h` | Sensor drivers |
| `ssd1306.c/.h` | 128x64 OLED driver |
| `wifi_mqtt_manager.c/.h` | WiFi + MQTT telemetry |
| `forgefpga_bitstream.h` | ForgeFPGA bitstream as a C array (auto-generated) |
| `shrikefi_dashboard.c` | Native Windows GUI dashboard (host-side, not flashed) |
| `CMakeLists.txt`, `build_and_flash.bat`, `monitor.bat`, `sdkconfig` | Build / flash / monitor |

## Build and flash

```bat
:: ESP-IDF v5.x must be on PATH (idf.py)
build_and_flash.bat          :: sets target esp32s3, builds, flashes, monitors
monitor.bat                  :: attach the serial monitor only
```

The firmware target is `esp32s3`; see `sdkconfig` for the partition table and
PSRAM settings.

## Host-side test and dashboard

```bat
run_host_test.bat            :: compile + run the link/driver logic on the PC
run_dashboard.bat            :: build and launch the Windows GUI dashboard
```

> The dashboard `.exe` and the other `*.exe` / `*.vvp` / `*.vcd` artifacts are
> gitignored build products, not tracked files — a fresh clone must build them.

## Bring-up procedure (first flash on real hardware)

ShrikeFi has **two USB Type-C ports** (power and programming). You only flash the
**ESP32-S3** — the FPGA is *not* programmed separately. At boot, `app_main()`
calls `shrikefi_fpga_flash_init()`, which programs the Renesas ForgeFPGA over
**SPI2** from the bitstream embedded in `forgefpga_bitstream.h`. On the real
ShrikeFi board the SPI link runs over internal PCB traces, so no jumper wires
are needed for it.

1. **The bitstream is up to date.** `forgefpga_bitstream.h` was regenerated from
   `FPGA_bitstream_MCU.bin` (the 2026-09-07 build output) and verified
   byte-for-byte: 46,408 bytes, zero differing bytes. See the firmware/FPGA note
   at the end of this file for a caveat about how it is delivered.
2. **Find your COM port.** Device Manager → Ports (COM & LPT). The scripts default
   to `COM5`; override with an argument, e.g. `build_and_flash.bat COM7`. If no
   port appears, the USB-serial driver is missing.
3. **Build and flash through the supplied script.** ESP-IDF v5.5.5 is expected at
   `C:\Espressif\frameworks\esp-idf-v5.5.5`; the `.bat` files `call export.bat`
   themselves, so `idf.py` does not need to be on PATH:
   ```bat
   build_and_flash.bat COM5      :: runs: idf.py -p COM5 flash monitor
   ```
   A build is required — do **not** copy a prebuilt binary out of `build/`, which
   is gitignored and may predate the current sources.
4. **Read the boot log.** Expect the sensor init lines, then `[TELEMETRY] HR=...`
   frames once a finger is on the MAX30102. `NO_FINGER` means the PPG sensor has
   no skin contact.
5. **Optional live GUI.** With the firmware running, launch
   `launch_dashboard.bat` (or `run_dashboard.bat`) on the PC against the same COM
   port.

### Build configuration notes

`sdkconfig` is gitignored (2400+ lines of derived state); `sdkconfig.defaults`
captures the build identity — target, flash, partition table, CPU frequency and
console. Two things worth changing before a demo:

* **CPU is configured at 160 MHz**, while `README.md` and `docs/theory/THEORY_NOTES.md`
  describe the part as "ESP32-S3 @ 240 MHz". Either raise
  `CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ` to 240 or soften that wording.
* **The current build is a debug build** (`-Og`). Moving to a release
  optimization level will materially speed up INT8 inference and shrink the
  image, which matters on a 2 MB flash.

## FPGA delivery: how the bitstream reaches the FPGA

**Bitstream is current.** `forgefpga_bitstream.h` matches the ForgeFPGA build
output exactly. It was regenerated from `FPGA_bitstream_MCU.bin` (2026-09-07) with
`hardware/shrikefi/convert_bitstream.py` and verified byte-for-byte — 46,408
bytes, 0 differing bytes, `forgefpga_bitstream_length = 46408`.

**Delivery is over SPI2, not I2C.** `shrikefi_fpga_flash_init()` follows the
Vicharak `Web_FPGA_programmer.ino` sequence:

1. PWR=0, EN=0, SS=1 → 5 ms
2. PWR=1, EN=1, SS=0 → 15 ms (boot-mode latch)
3. SS=1 → 2 ms
4. Image streamed in 256-byte chunks, SS toggled LOW/HIGH per chunk, SPI mode 0
   at 16 MHz
5. 50 ms settle, then the SPI2 device is released so the runtime SPI link can
   take the bus

This is verified on hardware. The boot log reads:

```
I (502) SHRIKEFI_LINK:   Programming Renesas ForgeFPGA SLG47910 via SPI2
I (621) SHRIKEFI_LINK: ForgeFPGA SLG47910 configuration COMPLETE! (46408 bytes loaded)
I (621) SHRIKEFI_MAIN: ForgeFPGA SLG47910 bitstream programmed successfully over SPI!
I (631) SHRIKEFI_LINK: ForgeFPGA runtime link handshake: probe sent 0x55, received 0x00
I (641) SHRIKEFI_LINK: MISO Pin 13 Physical Line Test: Pulldown=0, Pullup=0 (ACTIVELY DRIVEN BY FPGA)
```

**The `0x00` reply is expected, and the probe is not evidence of anything.** At reset the
RTL initialises its response register to `0xA5`, but the next clock overwrites it with
`{beat_latched, filt_sample[6:0]}` - which is `0x00` before any beat and before the filter
has filled. The byte sent on MOSI is never echoed back, so the handshake cannot distinguish
a programmed FPGA from a floating MISO. (An earlier revision of this file showed
`received 0x80` and called it "the real evidence"; both were wrong.)

What does show a configured design is running: the `ACTIVELY DRIVEN BY FPGA` line above, and
then `[FPGA ACCEL] Systolic crest detected!` once a finger is on the sensor - which only the
programmed design can produce.

### The transfer is open-loop

SPI writes cannot tell us whether a ForgeFPGA is listening, so `SHRIKEFI_OK`
means "the bytes were clocked out", not "the device confirmed receipt". The
link answers its own way once a finger is on the sensor. That is why
`app_main()` logs the result and **continues in every case** — the
SPI link is the runtime bus and does not depend on this call:

| Result | Meaning |
|---|---|
| `SHRIKEFI_OK` | Image clocked out over SPI2. |
| `SHRIKEFI_ERR_TIMEOUT` | SPI2 bus/device setup, or the DMA chunk alloc, failed. |
| `SHRIKEFI_ERR_SPI_WRITE` | A chunk failed mid-transfer. |

### The image is always embedded

`forgefpga_bitstream[]` is 46,408 bytes and is **always** compiled in — there is
no build switch to remove it. It costs 0.6% of the 7 MB app partition (89% free),
and an earlier revision that made it conditional silently disabled the FPGA
programming on any checkout whose gitignored `sdkconfig` predated the option. The
footgun was removed rather than documented.

> **Caveat.** `hardware/shrikefi/forgefpga_pins.pcf` declares no configuration
> interface, so it is possible the FPGA also self-configures from OTP/NVM or the
> onboard W25Q32JV QSPI flash, and that this transfer is redundant rather than
> load-bearing. Either way it is harmless, and disabling it is not supported.
