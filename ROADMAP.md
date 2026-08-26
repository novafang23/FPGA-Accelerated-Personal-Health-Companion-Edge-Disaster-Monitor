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
With the Verilog and C firmware validated, the next major milestone is porting the logic to Qualcomm silicon:
1. **Hexagon DSP Migration**: Port the Verilog O(1) moving average filter to the Qualcomm Hexagon DSP using Hexagon Vector eXtensions (HVX) for sub-milliwatt continuous execution.
2. **AI Engine Quantization**: Use the Snapdragon Neural Processing Engine (SNPE) SDK to quantize the PyTorch TinyML model (`nn_risk_model.c`) to an INT8 `.dlc` format for execution on the Hexagon NPU.
3. **Sensor Hub Integration**: Migrate the physical I2C/UART sensor polling from the Zynq ARM CPU down to the Qualcomm Sensor Core (Low-Power Island).

## 🌍 Phase 3: Field Validation
- Conduct simulated disaster scenario tests using thermal chambers to validate the *Cardiovascular Drift* 15-minute early warning hypothesis.
- Package the final application for Android Wear OS utilizing the Snapdragon Wear W5+ Gen 1 hardware accelerators.
