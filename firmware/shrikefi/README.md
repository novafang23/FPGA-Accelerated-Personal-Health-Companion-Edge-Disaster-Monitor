# ShrikeFi firmware (ESP32-S3)

ESP-IDF application for the **ShrikeFi** platform (ESP32-S3 + Renesas ForgeFPGA
`SLG47910C`). The port is implemented — see
[docs/MIGRATION.md](../../docs/MIGRATION.md) for the platform roadmap and
[docs/SHRIKEFI_LINK_PROTOCOL.md](../../docs/SHRIKEFI_LINK_PROTOCOL.md) for the
4-bit link protocol.

## What the application does

1. Boots under FreeRTOS and brings up I2C (MAX30102 PPG + BME280 environment),
   UART (PMSA003 / PMS5003 particulate sensor) and the SSD1306 OLED.
2. Configures the ForgeFPGA over I2C from the bitstream embedded in
   `forgefpga_bitstream.h` (`shrikefi_fpga_flash_init()`), so the FPGA is
   programmed on every boot without an external programmer.
3. Streams raw Red/IR samples to the FPGA over the 4-bit parallel link and reads
   back the filtered samples, the 32-bit IBI cycle timestamp and the beat IRQ.
4. Runs the shared, hardware-agnostic algorithm core from `../core/`
   (HRV, SpO2, mNEWS2 triage, INT8 TinyML multi-hazard model).
5. Publishes telemetry over USB UART and WiFi/MQTT, and drives the OLED with an
   offline advisory.

## Layout

| File | Purpose |
|---|---|
| `main_shrikefi.c` | FreeRTOS application: sensor task, link task, telemetry, OLED |
| `shrikefi_link_driver.c/.h` | 4-bit parallel link driver (command/register map) |
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
calls `shrikefi_fpga_flash_init()`, which configures the Renesas ForgeFPGA over
I2C from the bitstream embedded in `forgefpga_bitstream.h`. On the real ShrikeFi
board the 4-bit link runs over internal PCB traces, so no jumper wires are needed
for it.

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

**The I2C delivery path reports honestly now, but remains unverified.**
`shrikefi_fpga_flash_init()` previously returned `SHRIKEFI_OK` on *every* path,
including both failure modes, so a silent no-op was indistinguishable from a
successful program. It now returns a real code and the boot log says which branch
ran:

| Boot log line | Meaning |
|---|---|
| `No I2C configuration interface at 0x%02X` → `SHRIKEFI_ERR_FPGA_NOT_DETECTED` | Nothing ACKed. **Expected** — the FPGA configures itself from OTP/NVM or the onboard W25Q32JV QSPI flash. Boot continues normally. |
| `Device ACKed at I2C 0x%02X` then `ForgeFPGA bitstream transmitted` | Something answered and the write completed. |
| `Bitstream write failed at offset ...` → `SHRIKEFI_ERR_I2C_WRITE` | Something ACKed but the transfer failed part-way. Worth investigating. |

`app_main()` logs the result and **continues in every case** — the 4-bit parallel
link is the runtime bus between the ESP32-S3 and the FPGA and does not depend on
this call. A "not detected" result is not a fault.

### Why the write path is still unverified

`esp32_i2c_hal_write()` takes an **8-bit** register address, so
`(uint8_t)(offset & 0xFF)` wraps every 256 bytes and cannot address a 46,408-byte
image. Widening it (typically a 16-bit word address sent as two bytes) without the
Renesas ForgeFPGA I2C programming specification would only move the guess, so the
transfer is left as-is and labelled accordingly rather than silently "fixed".

The SLG47910 is an FPGA, not a GreenPAK CMIC, and this design's pin constraints
(`hardware/shrikefi/forgefpga_pins.pcf`) declare no I2C or SPI configuration
interface — supporting the view that the part loads itself from OTP/NVM or the
onboard QSPI flash, and that this routine is a best-effort fallback.

**How to settle it:** flash once and read the boot log. If the first line in the
table appears, nothing was transmitted and the FPGA is running whatever was
programmed into it previously — which is fine.

### The bitstream is not embedded by default (saves 47 KB of flash)

`forgefpga_bitstream[]` (46,408 bytes) is now compiled in **only** when enabled:

```
idf.py menuconfig  ->  ShrikeFi configuration
                   ->  Embed the ForgeFPGA bitstream and try to send it over I2C at boot
```

`CONFIG_SHRIKEFI_ENABLE_I2C_BITSTREAM_FLASH` defaults to **n**. Measured effect on
the image (compiling `shrikefi_link_driver.c` for ESP32 both ways):

| | `.rdata` | object total |
|---|---|---|
| disabled (default) | 240 B | 2,000 B |
| enabled | 47,040 B | 49,216 B |

**47,216 bytes saved** on the default build — meaningful on the 2 MB flash
configuration, and it matters more because `sdkconfig` currently selects a debug
optimization level.

With the flag off, the boot log reads:

```
ForgeFPGA I2C bitstream delivery is compiled out (CONFIG_SHRIKEFI_ENABLE_I2C_BITSTREAM_FLASH=n, saves 46 KB).
FPGA is expected to self-configure from OTP/NVM or onboard QSPI flash. The 4-bit link is unaffected.
```

Nothing else in the project includes `forgefpga_bitstream.h`, so this single
compile-time switch is sufficient to remove the data. Enable it only if you have
confirmed a device ACKs at `0x08` **and** you have the Renesas I2C programming
sequence — without that sequence the write is a guess, and it cannot be correct
for a 46 KB image regardless because of the 8-bit address field.
