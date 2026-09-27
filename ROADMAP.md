# Project Roadmap

This document outlines the short-term and long-term goals for the **SIH26181 Health Companion** project. 

## ✅ Phase 1: Code Quality, CI/CD, and Hygiene (Completed)
We recently completed a major audit and hygiene pass of the repository:
- **CI/CD Integration**: Added GitHub Actions for automated C/Verilog compilation, unit testing, and Python execution (`.github/workflows/ci.yml`).
- **Security**: Added TruffleHog secret scanning to prevent credential leaks (`.github/workflows/security.yml`).
- **Linting & Formatting**: Enforced LLVM-style formatting for C code via `.clang-format` and PEP8 styling for Python via `.flake8`.
- **Engine Hardening**: Removed magic numbers in the C `disaster_risk_engine` and replaced them with centralized macros in the header.
- **Unit Testing**: Introduced a standalone test suite (`test_disaster_risk_engine.c`) to verify disaster boundary conditions.

## 🚀 Phase 2: Qualcomm Snapdragon Wear Migration (Next Steps)
With the Verilog and C firmware validated, the next major milestone is porting the logic to Qualcomm silicon. Note that the **current platform is the ShrikeFi port (ESP32-S3 + Renesas ForgeFPGA)**, which is the near-term target and where active development happens; Qualcomm Snapdragon Wear W5+ Gen 1 is the scale-out target that the ShrikeFi work feeds into, not a replacement for it:
1. **Hexagon DSP Migration**: Port the Verilog O(1) moving average filter to the Qualcomm Hexagon DSP using Hexagon Vector eXtensions (HVX) for sub-milliwatt continuous execution.
2. **AI Engine Quantization**: Package the shipped INT8 TinyML model (`firmware/core/nn_risk_model_int8.c` / `.h`) into a Qualcomm `.dlc` container using the Snapdragon Neural Processing Engine (SNPE) SDK for execution on the Hexagon NPU. The model is already INT8-quantized in-tree and is produced by the NumPy-only training script `firmware/core/train_nn_risk_model.py`; there is no PyTorch dependency in the flow.
3. **Sensor Hub Integration**: Migrate the physical I2C/UART sensor polling from the Zynq ARM CPU down to the Qualcomm Sensor Core (Low-Power Island).

## 🌍 Phase 3: Field Validation
- Conduct simulated disaster scenario tests using thermal chambers to validate the *Cardiovascular Drift* 15-minute early warning hypothesis.
- Package the final application for Android Wear OS utilizing the Snapdragon Wear W5+ Gen 1 hardware accelerators.
