# System Architecture & Mindmap

This document provides a high-level visual overview of the **SIH26181 Health Companion** hardware and software architecture. These diagrams are generated using Mermaid.js and render directly in GitHub.

## 🧠 Project Mindmap
This mindmap illustrates the core features, technologies, and disaster scenarios covered by the project.

```mermaid
mindmap
  root((SIH26181
  Health Companion))
    Sensors
      MAX30102 (PPG - Heart Rate & SpO2)
      BME280 (Temp & Humidity)
      PMS5003 (PM2.5 Air Quality)
    Hardware Acceleration (FPGA)
      AXI4-Lite Bus Interface
      Zero-DSP Moving Average Filter
      Systolic Peak Detector
    Edge AI & Firmware (ARM CPU)
      Feature Extraction
        HRV Analysis (RMSSD)
        SpO2 Engine
      Risk Fusion
        Rule-Based (CTSI / PRSI)
        TinyML Neural Network
    Disaster Scenarios
      Heat Wave (Cardio Drift)
      Severe Smog
      Flash Flood (Hypothermia)
    Qualcomm Migration
      Snapdragon Wear W5+
      Hexagon DSP / NPU
```

---

## 🔀 System Data Flow
This flowchart traces the path of physiological and environmental data from the physical sensors, through the custom FPGA RTL pipeline, into the ARM CPU for AI processing and risk assessment.

```mermaid
graph TD
    %% Styling
    classDef hardware fill:#0052cc,stroke:#fff,stroke-width:2px,color:#fff;
    classDef software fill:#00875a,stroke:#fff,stroke-width:2px,color:#fff;
    classDef sensor fill:#ff991f,stroke:#fff,stroke-width:2px,color:#fff;

    %% Sensors
    S1[MAX30102 PPG Sensor]:::sensor -->|Raw Data| FPGA
    S2[BME280 Env Sensor]:::sensor -->|Temp/Hum| CPU
    S3[PMS5003 Air Sensor]:::sensor -->|PM2.5| CPU

    %% FPGA Subsystem
    subgraph FPGA [FPGA Hardware Acceleration - Verilog RTL]
        F1[O'1' 8-Tap Moving Average Filter]:::hardware
        F2[Systolic Peak Detector FSM]:::hardware
        F1 --> F2
    end
    
    %% ARM CPU Subsystem
    FPGA -->|AXI4-Lite & Interrupts| CPU
    subgraph CPU [ARM Cortex-A9 Firmware & AI]
        C1[HRV & SpO2 Extraction]:::software
        C2[Rule-Based Disaster Engine]:::software
        C3[TinyML Neural Network '123 Params']:::software
        C1 --> C2
        C1 --> C3
    end
    
    %% Output
    C2 --> OUT((Local OLED Advisory / Warning))
    C3 --> OUT
```
