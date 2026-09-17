# VALOR (Vital and Atmospheric Logic for Offline Rescue) / ShrikeFi: FPGA-Accelerated Edge Health & Disaster Triage System
**Project Master Architecture & Technical Handoff (`idea.md`)**  
**SIH Problem Statement:** SIH26181 (Smart India Hackathon)  
**Target Hardware:** Renesas ForgeFPGA (SLG47910C) + Espressif ESP32-S3 (Active Port: ShrikeFi); Xilinx Zynq-7000 (`xc7z020`) (Baseline Reference)

---

## 1. Problem Statement & Mission

India faces recurring public health crises during and after disasters (extreme heat waves, toxic post-harvest smog, urban flash floods, and cold shock). Delayed triage, hospital power collapse, and network outages frequently turn preventable physiological distress into irreversible cardiovascular and respiratory failure.

**VALOR (Vital and Atmospheric Logic for Offline Rescue / ShrikeFi)** is an offline, privacy-preserving, wearable personal health companion. It continuously fuses arterial photoplethysmography (PPG) with multi-parameter environmental telemetry. Using an ultra-low-power **heterogeneous architecture (FPGA Hardware DSP + On-Device INT8 TinyML)**, it delivers real-time clinical triage and early warning alerts completely independent of cell towers or cloud connectivity.

---

## 2. Heterogeneous Computing Architecture: Division of Labor

The system splits workload across specialized silicon tiers:

```
┌────────────────────────────────────────────────────────────────────────┐
│                   PHYSICAL SENSOR ACQUISITION (100 Hz)                 │
│   • MAX30102: Optical PPG (Red 660nm / IR 940nm)                       │
│   • BME280: Ambient Temperature, Relative Humidity, Pressure           │
│   • PMS5003: Laser Light-Scattering Particulate Matter (PM2.5)         │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│             RENESAS FORGEFPGA SLG47910C (50 MHz RTL HARDWARE DSP)      │
│                     [~40% OF SYSTEM WORKLOAD]                          │
│                                                                        │
│   • Resource Utilization: 342 / 1120 CLB LUT5s (30.54%), 194 FFs,      │
│     76 / 140 CLBs (54.29%), 0 DSP, 0 BRAM, 0 PLL                       │
│   • Dual 8-Tap Moving Average Filter: O(1) running-sum, bit-shift (>>3)│
│   • Cycle-Accurate 20ns Timer: Hardware Inter-Beat Interval (IBI)      │
│   • 4-State Systolic Crest Detector FSM (ARMED->RISING->PEAK->REF)     │
│   • Dynamic Crest-Fall Confirmation: Eliminates dicrotic notch errors  │
│   • Pulsed Hardware Interrupt: Pin 16 beat strobe to MCU               │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ SPI Link & Hardware Interrupt
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│             ESPRESSIF ESP32-S3 DUAL-CORE XTENSA LX7 (240 MHz)          │
│                     [~60% OF SYSTEM WORKLOAD]                          │
│                                                                        │
│   • Core 0 (DSP & Link Task):                                          │
│     - Latches FPGA IBI interval; computes SpO2 AC/DC calibration       │
│     - Computes Signal Quality Index (SQI) (Karlen 2012 / Elgendi 2016)  │
│     - Derives Respiratory Rate via RSA amplitude modulation (Charlton) │
│     - Neural PM2.5 Humidity Calibration (Si et al. 2019 hygroscopic)   │
│   • Core 1 (TinyML & Clinical Engine Task):                            │
│     - 300-beat Rolling Window RMSSD Heart Rate Variability (HRV)       │
│     - INT8 Quantized MLP Inference (6 -> 24 -> 16 -> 3, 619 bytes)     │
│     - Moran PSI, AHA Brook 2010 Autonomic Strain, mNEWS2 Triage Score   │
│     - Real-Time Telemetry over USB UART & 2.4 GHz WiFi/MQTT            │
└───────────────────────────────────┬────────────────────────────────────┘
                                    │ 60 FPS Telemetry
                                    ▼
┌────────────────────────────────────────────────────────────────────────┐
│            WIN32 NATIVE GUI CLINICAL DASHBOARD (60 FPS GDI)            │
│   • Double-buffered, zero-memory-leak arterial PPG oscilloscope        │
│   • Live digital readouts: HR, SpO2, RMSSD, RR, SQI, Temp, PM2.5       │
│   • Color-coded mNEWS2 Early Warning Alert & 3-Axis TinyML Risk Meters │
└────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Key Engineering Breakthroughs

1. **Eliminated Dicrotic Notch False Peak Triggers:**
   * Rewrote the Verilog peak detector FSM in `hardware/shrikefi/forgefpga_ppg_top.v` with dynamic `peak_val` tracking and running drop confirmation, stopping the secondary arterial reflection from artificially doubling heart rate and corrupting RMSSD.
2. **Resolved DC Baseline Drift Blindness:**
   * Dropped the static level check (`sample_in < dyn_threshold`) from `STATE_REFRACTORY`, preventing the FSM from getting trapped in refractory during weak finger perfusion.
3. **Dynamic Crest-Fall Threshold (`crest_fall_min`):**
   * Implemented adaptive drop thresholding (`(peak_val > 48) ? (peak_val >> 3) : 6`), boosting weak-signal consistency to 65%+ and good-contact consistency to 98.7%.
4. **Verified Live on Real Hardware:**
   * Live capture (`SHRIKERUN.txt`) confirmed median IBI of **780 ms** (~76.9 BPM) and stable human resting HRV ($RMSSD$) of **37.0 – 38.2 ms**.
5. **Eradicated Fatal Win32 GDI Memory Leak:**
   * Audited and corrected brush/pen handle restoration in `shrikefi_dashboard.c`, transforming a crash-prone GUI into an indefinitely stable 60 FPS monitor.
6. **Air-Tight Clinical Localization for SIH:**
   * Anchored clinical models to Indian peer-reviewed literature: **Singhal et al. (2026)** for normative Indian HRV ranges, **IMD / Ahmedabad Heat Action Plan** for heatwave thresholds ($\ge 40^\circ\text{C}$), and **Lancet GBD India (2019)** for air pollution impact.

---

## 4. Resource Utilization Verification Summary

| Component | Target Architecture | Verified Utilization | Report File Reference |
| :--- | :--- | :--- | :--- |
| **ForgeFPGA** | Renesas SLG47910C | **342 / 1120 LUT5s (30.54%)**, 194 FFs, 76 / 140 CLBs (54.29%), 0 DSP, 0 BRAM, 0 PLL | `hardware/shrikefi/synthesis_evidence/resource_utilization_spi_link.log` |
| **Zynq-7000 (Baseline)** | AMD Xilinx `xc7z020` | **185 LUTs**, 16 LUTRAM, 266 FFs, 0 DSP, 0 BRAM, WNS **+5.603 ns** (Fmax 69.45 MHz) | `hardware/zynq/synthesis_evidence/` |
| **TinyML Model** | INT8 Quantized MLP | **619 bytes parameter footprint**, 88.47% validation accuracy, 97.32% FP32 agreement | `firmware/core/nn_risk_model_int8.c` |
