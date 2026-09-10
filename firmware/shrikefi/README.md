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

## Known issue: the embedded bitstream may be stale

`forgefpga_bitstream.h` was last regenerated in commit `06129b2` (2026-08-31),
while the ForgeFPGA build output on disk
(`forgefpga_project/ffpga/build/bitstream/FPGA_bitstream_MCU.bin`) was written
**2026-09-07** and differs from the embedded copy in roughly **10% of its bytes**
(31,089 of 301,890 file bytes).

That means the RTL currently committed under
`hardware/shrikefi/forgefpga_project/ffpga/src/` is **not proven** to be the
configuration this firmware flashes.

Before flashing hardware, regenerate and re-verify:

```bat
cd hardware\shrikefi
python convert_bitstream.py
:: writes firmware\shrikefi\forgefpga_bitstream.h from the current build output
```

Then rebuild the firmware and confirm the FPGA enumerates and the link test
passes on real hardware. This was deliberately **not** changed automatically,
because swapping ~10% of a bitstream is a hardware-visible change that cannot be
validated without the board.
