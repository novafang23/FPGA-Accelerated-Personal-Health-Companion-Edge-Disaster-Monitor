# Contributing to SIH26181 Health Companion

Thank you for your interest in contributing to this project! We welcome all contributions including bug fixes, new features, documentation improvements, and feedback.

## Getting Started

1. Fork the repository and clone your fork.
2. Ensure you have the required toolchains installed:
   - **C Compiler**: GCC or MinGW
   - **Verilog Simulator**: Icarus Verilog (`iverilog` & `vvp`)
   - **Waveform Viewer**: GTKWave (optional but recommended)
   - **Python**: Python 3.8+ (for ML scripts and linting)

## Development Workflow

### C and Verilog
We use a unified script to build and run the C health dashboard and Verilog RTL testbenches.
On Windows, you can use:
```cmd
.\run.bat
```
This script gives you an interactive menu to run the simulation, start the dashboard, view waveforms, and run unit tests.

### Running Unit Tests
Before opening a pull request, ensure that the core disaster risk engines haven't regressed by running the unit tests:
```cmd
# Using the interactive menu:
.\run.bat
# Select option [6] Run Unit Tests
```

Alternatively, you can compile and run it directly in `SIH/`:
```bash
gcc -Wall -Wextra -o test_engine.exe test_disaster_risk_engine.c hrv_analysis.c spo2_engine.c disaster_risk_engine.c nn_risk_model.c nn_risk_model_int8.c -lm
./test_engine.exe
```

## Coding Standards

- **C Code**: We follow standard LLVM/WebKit style rules (4-space indent). We provide a `.clang-format` file in the root. Please format your code before submitting a PR.
- **Python**: We use `flake8` for linting. The config is provided in `.flake8`.
- **Verilog**: Please adhere to Verilog-2001 standards and keep logic synthesizable unless writing a testbench.

## Pull Requests

1. Create a descriptive branch name (e.g., `fix-temp-threshold` or `feat-new-sensor`).
2. Make your changes and write unit tests for any new logic.
3. Push to your fork and open a Pull Request against the `main` branch.
4. Our CI pipeline will automatically run `clang-format`, `flake8`, compile the C code, and run your unit tests. Ensure CI passes!

Thank you for helping us build a robust open-source health and disaster monitor!
