@echo off
setlocal enabledelayedexpansion

echo ================================================================
echo   SIH26181: ShrikeFi (ESP32-S3 + Renesas ForgeFPGA) Simulation
echo   Qualcomm Hardware Challenge - Smart India Hackathon 2026
echo ================================================================
echo.

:: ---------------------------------------------------------------------------
:: Toolchain — resolved from PATH so this works on any machine and in CI.
:: Override any of these by setting the variable before calling this script.
:: ---------------------------------------------------------------------------
if not defined IVERILOG set "IVERILOG=iverilog"
if not defined VVP      set "VVP=vvp"
if not defined GTKWAVE  set "GTKWAVE=gtkwave"

:: ---------------------------------------------------------------------------
:: forgefpga_ppg_top.v instantiates moving_average_8tap and ppg_peak_detector
:: from ../common/ — it does NOT contain inline copies, so both the top and the
:: two common sources must be listed here. Do not add duplicates.
:: ---------------------------------------------------------------------------
set "RTL_SRCS=tb_forgefpga_system.v forgefpga_ppg_top.v ../common/moving_average_8tap.v ../common/ppg_peak_detector.v"

:: 1. Compile ShrikeFi Verilog RTL with Icarus Verilog
echo [1/2] Compiling ForgeFPGA 4-Bit Link RTL with Icarus Verilog...
%IVERILOG% -o sim_shrikefi.vvp %RTL_SRCS%
if %errorlevel% neq 0 (
    echo [ERROR] Verilog compilation failed!
    pause
    exit /b 1
)
echo [OK] Verilog compilation successful.
echo.

:: 2. Run Testbench Simulation
echo [2/2] Running ShrikeFi System Testbench (vvp)...
%VVP% sim_shrikefi.vvp
if %errorlevel% neq 0 (
    echo [ERROR] Simulation execution failed!
    pause
    exit /b 1
)
echo [OK] VCD waveform dumped to shrikefi_sim.vcd.
echo.

:: 3. Launch GTKWave Viewer
where %GTKWAVE% >nul 2>&1
if %errorlevel% equ 0 (
    echo [3/3] Launching GTKWave Waveform Viewer...
    start "" %GTKWAVE% shrikefi_sim.vcd shrikefi_sim.gtkw
) else (
    echo GTKWave not found on PATH. Open shrikefi_sim.vcd manually.
)

pause
