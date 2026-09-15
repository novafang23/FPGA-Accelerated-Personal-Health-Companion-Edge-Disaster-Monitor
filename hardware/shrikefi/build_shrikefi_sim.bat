@echo off
setlocal enabledelayedexpansion

echo ================================================================
echo   SIH26181: ShrikeFi (ESP32-S3 + Renesas ForgeFPGA) Simulation
echo   Qualcomm Hardware Challenge - Smart India Hackathon 2026
echo ================================================================
echo.

:: ---------------------------------------------------------------------------
:: Toolchain - resolved from PATH so this works on any machine and in CI.
:: Override any of these by setting the variable before calling this script.
:: ---------------------------------------------------------------------------
if not defined IVERILOG set "IVERILOG=iverilog"
if not defined VVP      set "VVP=vvp"
if not defined GTKWAVE  set "GTKWAVE=gtkwave"
if not defined PYTHON   set "PYTHON=python"

:: ---------------------------------------------------------------------------
:: The ForgeFPGA vendor tool needs ONE flat source set, so
:: forgefpga_project/ffpga/src/forgefpga_ppg_top.v is generated from
:: forgefpga_ppg_top.v + ../common/ by gen_flat_source.py. Check it is current
:: before simulating, otherwise the vendor build and this simulation are
:: exercising different RTL.
:: ---------------------------------------------------------------------------
echo [0/3] Checking generated vendor sources are up to date...
%PYTHON% gen_flat_source.py --check
if %errorlevel% neq 0 (
    echo [WARN] Generated sources are stale. Regenerating...
    %PYTHON% gen_flat_source.py
)

:: ---------------------------------------------------------------------------
:: forgefpga_ppg_top.v instantiates moving_average_8tap and ppg_peak_detector
:: from ../common/ - it does NOT contain inline copies, so both the top and the
:: two common sources must be listed here. Do not add duplicates.
:: ---------------------------------------------------------------------------
set "RTL_SRCS=tb_forgefpga_system.v forgefpga_ppg_top.v ../common/moving_average_8tap.v ../common/ppg_peak_detector.v"

:: 1. Compile ShrikeFi Verilog RTL with Icarus Verilog
echo [1/3] Compiling ForgeFPGA SPI Link RTL with Icarus Verilog...
%IVERILOG% -o sim_shrikefi.vvp %RTL_SRCS%
if %errorlevel% neq 0 (
    echo [ERROR] Verilog compilation failed!
    pause
    exit /b 1
)
echo [OK] Verilog compilation successful.
echo.

:: 2. Run Testbench Simulation
::    The testbench calls $finish, which exits 0 whether or not a check failed,
::    so success is asserted on the summary line rather than on %errorlevel%.
echo [2/3] Running ShrikeFi System Testbench (vvp)...
%VVP% sim_shrikefi.vvp > sim_shrikefi.log
type sim_shrikefi.log
findstr /C:"10 passed, 0 failed" sim_shrikefi.log >nul
if %errorlevel% neq 0 (
    echo [ERROR] ShrikeFi testbench did not report 10 passed, 0 failed.
    pause
    exit /b 1
)
echo [OK] All 10 checks passed. VCD waveform dumped to shrikefi_sim.vcd.
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
