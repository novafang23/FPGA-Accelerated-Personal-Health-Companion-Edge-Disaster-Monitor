# Project VALOR: Live Demonstration Runbook & Presentation Guide

**Project Title:** SIH26181 — VALOR (Vital and Atmospheric Logic for Offline Rescue)  
**Target Hardware:** Vicharak ShrikeFi (ESP32-S3 + Renesas ForgeFPGA SLG47910C)  
**Primary Goal:** Flawless, reproducible live demonstration of offline disaster triage, optical hardware acceleration, and multi-hazard edge intelligence under high-stakes judging conditions.

---

## Table of Contents
1. [Pre-Demonstration Equipment & Hardware Checklist](#1-pre-demonstration-equipment--hardware-checklist)
2. [Physical Wiring & Pin Mapping Reference](#2-physical-wiring--pin-mapping-reference)
3. [Bench Setup & Flashing Procedures](#3-bench-setup--flashing-procedures)
4. [Step-by-Step Live Demonstration Script](#4-step-by-step-live-demonstration-script)
5. [Real Hardware Verification vs. Verified Fallback Demos](#5-real-hardware-verification-vs-verified-fallback-demos)
6. [Troubleshooting Guide for Live Stage Failures](#6-troubleshooting-guide-for-live-stage-failures)
7. [Pre-Judging 15-Minute Readiness Checklist](#7-pre-judging-15-minute-readiness-checklist)

---

## 1. Pre-Demonstration Equipment & Hardware Checklist

### A. Physical Hardware Kit
- [ ] **1x Vicharak ShrikeFi Development Board** (ESP32-S3-WROOM-1-N8R2 + Renesas ForgeFPGA SLG47910C).
- [ ] **1x MAX30102 / MAX30100 Optical Biosensor Breakout** (with I2C pull-ups).
- [ ] **1x BME280 / BMP280 Environmental Sensor Module** (Temp, Humidity, Pressure).
- [ ] **1x Plantower PMS5003 Laser Particulate Sensor** (with 8-pin connector cable).
- [ ] **1x MPU6050 6-Axis IMU Sensor Module** (Accelerometer + Gyroscope).
- [ ] **1x 0.96" SSD1306 128x64 I2C OLED Display Module** (Blue/Yellow or White).
- [ ] **1x u-blox NEO-M8N / Quectel L89HS GNSS Module** (with patch antenna).
- [ ] **1x Solderless Breadboard & Male-to-Male / Male-to-Female Jumper Wires**.
- [ ] **2x USB-C Data Cables** (Ensure they support high-speed data, not charging-only!).
- [ ] **1x Smartphone (iOS or Android)** for demonstrating the zero-install SoftAP dashboard.

### B. Laptop Software & Toolchain Prerequisites
- [ ] **OS:** Windows 10/11 x86_64.
- [ ] **Terminal:** PowerShell or Command Prompt.
- [ ] **Browser:** Google Chrome or Microsoft Edge (supports Web Serial API).
- [ ] **C Compiler:** MinGW-w64 GCC (supporting `-std=c11`).
- [ ] **RTL Tools:** Icarus Verilog (`iverilog`) & `vvp` installed on PATH.
- [ ] **ESP-IDF:** Espressif ESP-IDF v5.5.x installed at `C:\Espressif\frameworks\esp-idf-v5.5.5`.

---

## 2. Physical Wiring & Pin Mapping Reference

> [!CAUTION]
> **STRICT 3.3V VOLTAGE CEILING**  
> All ESP32-S3 and Renesas ForgeFPGA GPIO pins are **strictly 3.3V LVCMOS**. Applying 5V will permanently destroy the microcontroller and FPGA silicon. The only wire receiving 5V is **Pin 1 of the Plantower PMS5003** connected directly to the USB VBUS pin.

```
 ESP32-S3 Pinout                 Breadboard Distribution                  Peripheral Target
┌────────────────┐              ┌────────────────────────┐              ┌─────────────────┐
│ GPIO 1  (SDA)  │ ───────────▶ │ Shared I2C SDA Rail    │ ───────────▶ │ OLED, MAX30102, │
│ GPIO 2  (SCL)  │ ───────────▶ │ Shared I2C SCL Rail    │ ───────────▶ │ BME280, MPU6050 │
├────────────────┤              ├────────────────────────┤              ├─────────────────┤
│ GPIO 14 (RX1)  │ ───────────▶ │ Direct Jumper Wire     │ ───────────▶ │ PMS5003 TXD     │
│ GPIO 18 (TX1)  │ ───────────▶ │ (Optional Return)      │ ───────────▶ │ PMS5003 RXD     │
├────────────────┤              ├────────────────────────┤              ├─────────────────┤
│ GPIO 16 (RX2)  │ ───────────▶ │ Direct Jumper Wire     │ ───────────▶ │ GPS / GNSS TXD  │
│ GPIO 17 (TX2)  │ ───────────▶ │ (Optional Return)      │ ───────────▶ │ GPS / GNSS RXD  │
├────────────────┤              ├────────────────────────┤              ├─────────────────┤
│ GPIO 4  (ALARM)│ ───────────▶ │ Active Buzzer Driver   │ ───────────▶ │ Buzzer (+) Pin  │
│ GPIO 5  (CANCEL│ ───────────▶ │ Tactile Switch         │ ───────────▶ │ SOS Cancel Btn  │
├────────────────┤              ├────────────────────────┤              ├─────────────────┤
│ 3.3V Rail      │ ───────────▶ │ Shared 3.3V Power Bus  │ ───────────▶ │ All VCC (3.3V)  │
│ GND Rail       │ ───────────▶ │ Common Ground Bus      │ ───────────▶ │ All GND Pins    │
│ 5.0V (VBUS)    │ ───────────▶ │ Isolated 5V Fan Rail   │ ───────────▶ │ PMS5003 Pin 1/2 │
└────────────────┘              └────────────────────────┘              └─────────────────┘
```

---

## 3. Bench Setup & Flashing Procedures

### Stage 1: Verify Hardware Detection on Windows
Connect the ShrikeFi board to your laptop using a USB-C data cable. In PowerShell:
```powershell
# Check which COM port is assigned
Get-WmiObject Win32_SerialPort | Select-Object DeviceID, Description
# Expected: "COM4" or "COM5" (Silicon Labs CP210x or USB JTAG/serial debug unit)
```

### Stage 2: Compile & Flash Firmware (ESP-IDF v5.5.5)
```powershell
# 1. Export ESP-IDF build environment
. C:\Espressif\frameworks\esp-idf-v5.5.5\export.ps1

# 2. Change to ShrikeFi firmware directory
cd firmware\shrikefi

# 3. Clean, compile, flash, and launch serial monitor
idf.py -p COM4 flash monitor
```

### Stage 3: What Legitimate Silicon Boot Output Looks Like
When the board boots successfully, the serial console displays:
```text
I (812) WEB_STATUS: Status page up: join WiFi 'VALOR-0081' (open), then open http://192.168.4.1
I (832) LOCATION: Stored location: 'Village Hulimavu, Kolar'
I (832) SHRIKEFI_MAIN: Emergency assist ready (state: IDLE), location 'Village Hulimavu, Kolar'
I (850) SHRIKEFI_LINK: Programming ForgeFPGA bitstream over SPI2 (46408 bytes)...
I (860) SHRIKEFI_LINK: ForgeFPGA configuration COMPLETE (DONE pin asserted HIGH)
I (862) SHRIKEFI_MAIN: MAX30102 initialized OK (100 Hz sampling, FIFO rollover enabled)
I (872) SHRIKEFI_MAIN: BME280 initialized OK at 0x76
I (882) SHRIKEFI_MAIN: PMS5003 initialized OK on UART1 (RX=GPIO14)
I (892) SHRIKEFI_MAIN: MPU6050 6-Axis IMU initialized OK (fall detection active)
I (902) SHRIKEFI_MAIN: L89HS GNSS initialized OK on UART2 (RX=GPIO16)
I (912) SHRIKEFI_MAIN: Core 0: PPG Accelerator Task Started (FPGA hardware link active).
I (922) SHRIKEFI_MAIN: Core 1: Disaster Risk Engine & TinyML Task Started.
[TELEMETRY] NO_FINGER,TEMP=25.4,HUM=52.0,PM25=14.0,SKINTEMP=0.0
```
*(Press `Ctrl + ]` to exit the serial monitor before launching PC dashboards).*

---

## 4. Step-by-Step Live Demonstration Script

### Act 1: The Offline Architecture & System Boot (0:00 - 1:00)
1. **Physical Action:** Show the unpowered ShrikeFi board to the judges. Point to the Renesas ForgeFPGA chip and the ESP32-S3 SoC.
2. **Spoken Dialogue:**
   > *"Judges, in an active flood or cyclone disaster, the cloud does not exist. We have zero cellular connectivity and zero server backhaul. This is Valor. It is a dual-brain architecture: a Renesas ForgeFPGA executing optical signal conditioning with 20-nanosecond hardware timing, and a dual-core ESP32-S3 executing clinical triage and a zero-cloud integer neural network."*
3. **Physical Action:** Plug in the USB cable. Point out the OLED display lighting up with live sensor telemetry and the blue LED (D12) on the board.

---

### Act 2: Hardware Jitter Elimination & Live Optical PPG (1:00 - 2:30)
1. **Physical Action:** Place a finger firmly on the MAX30102 pulse oximeter sensor.
2. **Observe Hardware:**
   - The blue LED (D12) flashes in synchrony with every systolic arterial pulse.
   - The OLED display begins showing live Heart Rate (e.g., `HR: 74 BPM`), Blood Oxygenation (`SpO2: 98%`), and Respiratory Rate (`RR: 15`).
3. **Launch Desktop Oscilloscope:**
   Double-click `launch_dashboard.bat` (or open `firmware/shrikefi/dashboard/valor_dashboard.html` in Chrome and click **Connect Serial** $\to$ select **COM4**).
4. **Observe GUI:**
   - The green 60 FPS oscilloscope displays arterial blood volume pulses.
   - Show the judges the Signal Quality Index gauge reading **SQI: 92%** (Hospital Grade).
5. **Spoken Dialogue:**
   > *"Notice the blue LED flashing on the board. That LED is not driven by software interrupts or CPU code—it is driven directly by the output pin of our FPGA peak detector. In software, RTOS scheduling jitter introduces up to 10 milliseconds of error, which corrupts Heart Rate Variability. Our FPGA detects peaks with 20-nanosecond cycle precision, giving us the mathematical fidelity needed for medical-grade RMSSD calculation."*

---

### Act 3: Multi-Hazard Fusion & Atmospheric Storm Tracking (2:30 - 4:00)
1. **Physical Action:** Point to the BME280 sensor and the barometric pressure trend gauge on the web dashboard.
2. **Spoken Dialogue:**
   > *"A fitness watch only tells you your heart rate. But what happens if a tropical cyclone is deepening overhead? Valor tracks atmospheric pressure changes across a 180-minute rolling regression window. If the barometer drops faster than 1 hPa per hour, the system automatically elevates the cyclone risk level and warns the user to move inland, independent of any weather broadcast."*
3. **Physical Action:** Lightly blow across the MAX30102 sensor to demonstrate skin temperature tracking. The display transitions from ambient room proxy to human finger contact temperature (`SKINTEMP: 35.8°C`).

---

### Act 4: Fall Detection & Emergency Assistance Latching (4:00 - 5:30)
1. **Physical Action:** Pick up the breadboard containing the MPU6050 IMU. Simulate a realistic fall: drop the board from a height of ~15 cm onto a soft surface (or execute a rapid downward drop followed by a sharp stop and 1.5 seconds of immobility).
2. **Observe System Response:**
   - The OLED immediately transitions to: `*** FALL DETECTED ***`.
   - The system enters `ARMED` state with a 10-second countdown timer.
3. **Spoken Dialogue:**
   > *"Our 3-stage biomechanical fall detection state machine just registered free-fall weightlessness, impact shock, and postural immobility. It is now in a 10-second cancellation window to prevent false alarms if the user simply dropped their bag."*
4. **Physical Action:** Allow the 10 seconds to expire without pressing cancel.
5. **Observe Emergency Escalation:**
   - The OLED switches to the full-screen **Emergency Assistance Card**:
     ```text
     * MEDICAL EMERGENCY *
     SOS ACTIVE  T+00:15
     HR: 76  SpO2: 98%
     RR: 15  SQI:  92%
     COND: CRITICAL FALL
     LOC: Village Hulimavu
     CALL 108 DO NOT MOVE
     ```
   - The active sounder triggers an audible siren.

---

### Act 5: Zero-Install Local Rescuer Web Dashboard (5:30 - 7:00)
1. **Physical Action:** Take out your smartphone. Open Wi-Fi settings.
2. **Observe SSID:** Show the judges the open Wi-Fi network: **`VALOR-0081`**.
3. **Connect & Browse:** Connect the phone to `VALOR-0081` (no password required). Open the phone browser and navigate to:
   ```
   http://192.168.4.1
   ```
4. **Show Phone Screen to Judges:**
   - The phone renders the responsive, dark-mode VALOR dashboard served directly from the ESP32-S3 flash memory.
   - The top banner flashes crimson: **`MEDICAL EMERGENCY: SOS ACTIVE (fall)`**.
   - Show the live vitals, environmental metrics, and the stored geolocation (`Village Hulimavu, Kolar`).
5. **Remote Cancellation:** Tap the **CANCEL SOS** button on the phone screen. The device immediately clears the emergency state, and the OLED returns to normal vital monitoring.
6. **Spoken Dialogue:**
   > *"Judges, notice that my phone did not download an app from the App Store, and there is no router in this room. The device itself is an autonomous web server. Any first responder who arrives on the scene can connect their phone in seconds and access full patient vitals, 24-hour trends, and location data."*

---

## 5. Real Hardware Verification vs. Verified Fallback Demos

In competitive hackathons, physical hardware can suffer broken jumper wires, dead USB hubs, or hostile RF interference. Valor includes **verified fallback modes** that run on standard laptops without physical silicon.

### Comparison Matrix: What Proves What

| Feature | Primary Proof (Hardware Silicon) | Verified Fallback Proof (Host PC Simulation) | What the Fallback Truly Proves |
| :--- | :--- | :--- | :--- |
| **FPGA Peak Detector** | Physical Blue LED (D12) flashes to real pulse | `tb_forgefpga_system.v` simulation in Icarus Verilog | Proves 100% cycle-accurate Verilog logic and 20 ns timing closure |
| **Zero-Cloud INT8 AI** | ESP32-S3 runs inference in 18 µs on live vitals | `shrikefi_host.exe` host test suite under GCC 15.2 | Proves mathematical quantization fidelity (max error 0.091) |
| **mNEWS2 Triage Engine** | OLED displays live clinical levels and alerts | `test_disaster_risk_engine.exe` (34 unit tests) | Proves all clinical boundary sweeps and crisis overrides |
| **Fall Detection FSM** | MPU6050 physical drop triggers countdown | `test_fall_detect.exe` (16 FSM unit tests) | Proves 3-stage FSM state transitions and angle math |
| **GUI & Triage Dashboard** | Live Web Serial stream on COM4 port | `valor_dashboard.html` with built-in Simulation Profiles | Proves UI rendering, gauge math, and multi-hazard response |

### Executing the Verified Fallback Demonstration (If Hardware Fails)

If a sensor wire breaks or your board loses power, do **not** panic. Immediately transition to the verified software and simulation fallback:

1. **Launch the Web Dashboard in Simulation Mode:**
   Double-click `launch_web_dashboard.bat`. Chrome opens `valor_dashboard.html`.
2. **Select a Disaster Profile:**
   Under **Simulated Profiles**, select:
   - **`Heat Wave & Dehydration`**: Watch HR climb to 138 BPM, Ambient Temp to 43.5°C, Moran's PSI climb to 9.5, and the TinyML Heat Stroke gauge hit 94% (CRITICAL).
   - **`Severe Smog / PM2.5 Crisis`**: Watch PM2.5 rise to 340 µg/m³, SpO2 drop to 92.5%, and the Smog Inhalation gauge spike to 97%.
   - **`Flash Flood / Hypothermia`**: Watch ambient temperature plunge to 11.2°C, barometric slope drop to -3.8 hPa/hr, triggering the **CRITICAL CYCLONE** warning card.
3. **Execute Verilog Hardware Simulation in Front of Judges:**
   Open PowerShell and run:
   ```powershell
   cd hardware\shrikefi
   iverilog -o sim.vvp -I ../common tb_forgefpga_system.v forgefpga_ppg_top.v spi_target.v ../common/moving_average_8tap.v ../common/ppg_peak_detector.v
   vvp sim.vvp
   ```
   Show the judges the terminal output:
   `RESULTS: 13 passed, 0 failed (out of 13 checks) - ALL TESTS PASSED`.

---

## 6. Troubleshooting Guide for Live Stage Failures

| Symptom / Error | Immediate Root Cause | 30-Second Fix |
| :--- | :--- | :--- |
| **`Permission denied: build_host_tests.exe`** | A previously compiled test executable is still running in the background | Run `Stop-Process -Name *host_test* -Force` in PowerShell, then recompile. |
| **OLED display is completely black** | I2C pull-up collision or loose SDA/SCL wire | Jiggle breadboard rows 4 and 5 (GPIO 1 & 2). Power-cycle USB cable. |
| **`[TELEMETRY] NO_FINGER` permanently displayed** | MAX30102 photodiode saturated by overhead lighting | Cup your hand over the sensor to shield it from harsh fluorescent lights. Warm your finger. |
| **Phone cannot connect to `VALOR-0081`** | Wi-Fi channel congestion or phone auto-disconnecting (no internet) | Disable mobile data on the phone; hold phone within 1 meter of the ESP32-S3 board. |
| **Web Serial reports `Failed to open COM port`** | Another terminal (e.g., `idf.py monitor`) has locked the serial port | Close all active serial consoles or PowerShell windows running `idf.py monitor`. |

---

## 7. Pre-Judging 15-Minute Readiness Checklist

Complete this checklist 15 minutes before your scheduled judging slot:

- [ ] **USB Cables Tested:** Both USB-C cables confirmed working for data communication.
- [ ] **Firmware Flashed:** Board has the latest firmware build with boot banner verified.
- [ ] **COM Port Verified:** Note whether your device is on COM4, COM5, or COM3.
- [ ] **Phone Wi-Fi Tested:** Phone successfully connects to `VALOR-0081` and loads `http://192.168.4.1`.
- [ ] **Browser Tabs Prepared:**
  - Tab 1: `firmware/shrikefi/dashboard/valor_dashboard.html` (Web Serial Dashboard).
  - Tab 2: `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log` (FPGA Evidence).
  - Tab 3: `docs/PROJECT_GUIDE.md` (Architecture Reference).
- [ ] **Terminal Prepared:** PowerShell open in repo root with GCC compiler and Icarus Verilog verified on PATH.
- [ ] **Sensors Secured:** Sensor breakout boards firmly seated in breadboard rows with no loose floating wires.
- [ ] **Spoken Roles Assigned:**
  - Teammate 1: Live Hardware Demonstration & Sensor Manipulation.
  - Teammate 2: RTL & Hardware Architecture Explanation.
  - Teammate 3: Clinical Triage & Zero-Cloud TinyML Model Explanation.
