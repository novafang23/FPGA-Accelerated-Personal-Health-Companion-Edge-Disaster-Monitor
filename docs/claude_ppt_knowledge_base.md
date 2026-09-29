# VALOR (FPGA-Accelerated Personal Health Companion & Edge Disaster Monitor)
**Project Code:** SIH26181 (Smart India Hackathon 2026 / Qualcomm Hardware Challenge)

## 1. Project Overview
**VALOR** is a heterogeneous SoC design that acts as a wearable health companion and environmental disaster monitor. It fuses physiological data (heart rate, SpO2) with environmental data (air quality, barometric pressure) to predict both clinical emergencies and external environmental hazards (heatwaves, cyclones, pollution).

**The Core Architecture:** The system offloads computationally heavy signal processing (PPG filtering, peak detection) into an ultra-low-power FPGA, freeing the MCU to run quantized TinyML models and an interactive UI, all while operating completely offline in a disaster zone.

---

## 2. Hardware Architecture & Signal Flow

The system runs on the **Vicharak ShrikeFi Board**, leveraging a dual-target architecture:

### MCU Layer (ESP32-S3 - FreeRTOS)
* **Specs:** Dual-core Xtensa LX7, WiFi 4, BLE 5, single-precision FPU, 3.3V I/O.
* **Core 0:** Dedicated to SPI acquisition (reads from the FPGA).
* **Core 1:** Runs ML Inference, the Triage Engine, and the Web Server.

### FPGA Layer (Renesas ForgeFPGA SLG47910)
* **Specs:** Ultra-low cost, 1120 five-input LUTs.
* **Interconnect:** Custom 4-wire SPI (Mode 0, 1 MHz runtime) communicates directly with the ESP32-S3.
* **Hardware Blocks:**
  * **Moving Average Filter:** 8-tap filter to smooth raw PPG data.
  * **Peak Detector:** Systolic peak detection FSM with 250ms refractory blanking.
  * **Observable Output:** Drives a blue user LED on every heartbeat; sends 7-bit filtered amplitude + beat flag to the MCU over SPI.

### Sensors (The Inputs)
1. **MAX30102:** PPG (Red + IR optical) for cardiac/oxygen data.
2. **BME280:** Temperature, Humidity, Barometric Pressure.
3. **PMS5003:** PM2.5 particulate matter sensor.

---

## 3. The AI & Clinical Triage Engines

The system does not blindly trust ML. It uses a layered, defensively-coded approach combining peer-reviewed clinical rules with Neural Networks.

### A. Signal Processing (Feature Extraction)
* **SQI (Karlen 2012):** Signal Quality Index based on Perfusion Index, regularity, and skewness. Suppresses false alarms during motion artifacts.
* **SpO2 (Beer-Lambert):** Uses a 50-sample Welford variance window.
* **HRV:** Root Mean Square of Successive Differences (RMSSD).
* **Respiratory Rate (Charlton 2018 EDR):** Extracted from PPG using FM + AM autocorrelation.
* **Pressure Trend:** Least-squares barograph (30-min window) to track cyclone pressure drops.

### B. TinyML Models (INT8 Quantized)
The neural networks are quantized to INT8, deploying in just 619 bytes of flash, bypassing the ESP32's lack of a 64-bit FPU.
1. **Risk Model (6→24→16→3 MLP):**
   * **Inputs:** HR, RMSSD, SpO2, Temp, Hum, PM2.5.
   * **Outputs:** Heat, Pollution, Flood risk scores.
   * *Trained via teacher-distillation (70k synthetic samples).*
2. **PM2.5 Calibrator (3→8→4→1 MLP):**
   * Integer-only math. Debiases PMS5003 optical scattering errors caused by high humidity.

### C. Clinical & Disaster Rule Engines
* **Clinical Engine (`clinical_vitals_engine.c`):** Uses modified **mNEWS2 scoring**. It has absolute crisis overrides (e.g., HR > 150 bpm immediately flags CRITICAL). 
* **Disaster Engine (`disaster_risk_engine.c`):** Calculates CTSI (Heat), PRSI (Pollution), and Cyclone risk (based on barometric rate-of-fall, not absolute pressure).
* **Safety Guards:** "Cross-domain false-alarm guards". Example: The heat index is forcibly gated if ambient air is cold, preventing a hypothermia-induced HRV drop from triggering a false "heatwave" alert.

### D. Emergency SOS System (`sos.c`)
* Implements a state machine (`IDLE` -> `ARMED` -> `ACTIVE` -> `CANCELLED`).
* Requires a 15-second "critical confirm" window to prevent transient false alarms.
* Includes a "cancel hold" feature (press and hold to stand down an active alarm).

---

## 4. UI: The Offline Dashboard

* **File:** `valor_dashboard.html`
* **Purpose:** A mission-critical, interactive UI that works in completely degraded network environments (e.g., after a hurricane destroys cell towers). Served directly from the ESP32 SPIFFS filesystem.
* **Features:** 
  * **Live Canvas Rendering:** Real-time ECG-style trace, Respiratory Sinus Arrhythmia envelope, and dynamic Heat/Pollution radar charts.
  * **Performance:** Uses heavily optimized `requestAnimationFrame` and canvas primitives for 60fps rendering on mobile devices.
  * **Accessibility (WCAG 2.1):** Fully compliant with semantic `<main>` tags and ARIA labels for all canvas elements and dynamic data.
  * **Adaptive Styling:** Dark-mode by default, responsive flexbox layout, zero external CSS/JS dependencies.
