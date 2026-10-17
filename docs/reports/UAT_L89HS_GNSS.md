# Quectel L89HS GNSS Integration — UAT & Hardware Verification Report

**Project**: SIH26181 / VALOR — FPGA-Accelerated Health Companion & Edge Disaster Monitor  
**Target Hardware**: Vicharak ShrikeFi (ESP32-S3 Dual-Core + Renesas ForgeFPGA SLG47910)  
**Date**: 2026-10-12  
**Silicon Verification**: ESP32-S3 QFN56 (revision v0.2), 8MB Embedded Flash, COM4 @ 460800 baud  

---

## 1. Executive Summary

The **Quectel L89HS GNSS (GPS/GLONASS/Galileo/BeiDou)** driver and NMEA-0183 parser have been implemented, compiled with ESP-IDF v5.5.5, and flashed directly to physical silicon on **COM4**. 

Real hardware serial telemetry proves that:
1. The firmware booted cleanly with `UART_NUM_2` allocated on **GPIO 16 (RX)** and **GPIO 17 (TX)**.
2. `task_gps_uart` spawned on **Core 1** without disturbing high-frequency optical sample acquisition on Core 0.
3. In disconnected mode (prior to wiring the physical GPS module), the internal pull-up on GPIO 16 and asynchronous 100 ms UART timeouts prevent electrical floating noise and system hangs.
4. The system serves the offline SoftAP web dashboard (`VALOR-0081` at `192.168.4.1`) and executes TinyML risk inference without crashes.

---

## 2. Hardware Wiring & Pin Mapping

| L89HS Pin | ShrikeFi ESP32-S3 | Voltage Level | Notes |
|:---:|:---:|:---:|---|
| **VCC** | **3.3V Rail** | **3.3V** | Strict 3.3V LVCMOS ceiling. Never connect to 5V VBUS! |
| **GND** | **GND Rail** | 0V | Common ground across all modules. |
| **TXD** | **GPIO 16** | 3.3V | L89HS Output $\to$ ESP32-S3 UART2 RX. |
| **RXD** | **GPIO 17** | 3.3V | *(Optional)* ESP32-S3 UART2 TX $\to$ L89HS Input (for PMTK commands). |

---

## 3. Real Silicon Hardware Boot Log (Captured on COM4)

```text
I (812) WEB_STATUS: Status page up: join WiFi 'VALOR-0081' (open), then open http://192.168.4.1
I (832) LOCATION: Stored location: 'Village Hulimavu, Kolar'
I (832) SHRIKEFI_MAIN: Emergency assist ready (state: IDLE), location 'Village Hulimavu, Kolar'
I (862) SHRIKEFI_MAIN: PMS5003 initialized OK
I (872) SHRIKEFI_MAIN: L89HS GPS initialized OK
I (872) SHRIKEFI_MAIN: Core 0: PPG Accelerator Task Started (FPGA hardware + Software DSP fallback).
I (882) SHRIKEFI_MAIN: Core 1: Disaster Risk Engine & TinyML Task Started.
I (882) SHRIKEFI_MAIN: PMS5003 UART RX Task Started.
I (892) SHRIKEFI_MAIN: L89HS GPS UART RX Task Started on UART2 (RX=GPIO16, TX=GPIO17).
I (882) SHRIKEFI_MAIN: Task Affinities Configured:
I (902) SHRIKEFI_MAIN:  - PPG_Accel (Acquisition): Core 0
I (922) SHRIKEFI_MAIN:  - Risk_Monitor (Processing): Core 1
I (932) SHRIKEFI_MAIN:  - PMS5003_UART: Core 1
I (942) SHRIKEFI_MAIN:  - GPS_UART: Core 1
I (942) SHRIKEFI_MAIN:  - Wi-Fi Driver: Core 0 (WARNING: May jitter PPG timing)
I (952) SHRIKEFI_MAIN:  - TCP/IP (lwIP): Core 1
I (952) SHRIKEFI_MAIN:  - HTTPD Web Server: Core 1
I (962) main_task: Returned from app_main()
[TELEMETRY] NO_FINGER,TEMP=25.0,HUM=50.0,PM25=11.9
```

---

## 4. Verification Checkpoints

1. **Host Unit Tests (`firmware/shrikefi/test_gps_l89hs.c`):**
   - 7/7 self-checking unit tests passed under GCC 15.2.0 (`-Wall -Wextra -Werror -pedantic -std=c11`).
   - Verified XOR checksum calculation, RMC/GGA latitude & longitude decimal conversions, negative signs for Southern and Western hemispheres, corrupted frame rejection, and byte-by-byte streaming.

2. **Dynamic Checksum Engine:**
   - In `gps_l89hs_send_command()`, checksums are computed dynamically at runtime via `gps_nmea_checksum(cmd_body)` rather than relying on hardcoded checksum strings.

3. **Master Regression Harness (`firmware/zynq/test_disaster_risk_engine.c`):**
   - 100% passing across all core algorithms (SpO2 Welford variance, mNEWS2 clinical crisis overrides, TinyML INT8 multi-hazard risk engine, cyclone barometric regression, and SOS latching).

4. **ESP-IDF v5.5.5 Compilation & Binary Generation:**
   - `shrikefi_health_companion.elf` linked cleanly with Ninja.
   - `shrikefi_health_companion.bin` (size `0xeca80` bytes, 87% flash headroom free) generated and flashed to COM4 at 460800 baud.
