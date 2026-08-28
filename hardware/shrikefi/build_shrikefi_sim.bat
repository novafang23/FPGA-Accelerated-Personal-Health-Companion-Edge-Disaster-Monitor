@echo off
setlocal enabledelayedexpansion

echo ================================================================
echo   SIH26181: ShrikeFi (ESP32-S3 + Renesas ForgeFPGA) Simulation
echo   Qualcomm Hardware Challenge - Smart India Hackathon 2026
echo ================================================================
echo.

:: 1. Compile ShrikeFi Verilog RTL with Icarus Verilog
echo [1/2] Compiling ForgeFPGA 4-Bit Link RTL with Icarus Verilog...
C:\iverilog\bin\iverilog.exe -o sim_shrikefi.vvp tb_forgefpga_system.v forgefpga_ppg_top.v ../common/moving_average_8tap.v ../common/ppg_peak_detector.v
if %errorlevel% neq 0 (
    echo [ERROR] Verilog compilation failed!
    pause
    exit /b 1
)
echo [OK] Verilog compilation successful.
echo.

:: 2. Run Testbench Simulation
echo [2/2] Running ShrikeFi System Testbench (vvp)...
C:\iverilog\bin\vvp.exe sim_shrikefi.vvp
if %errorlevel% neq 0 (
    echo [ERROR] Simulation execution failed!
    pause
    exit /b 1
)
echo [OK] VCD Waveform dumped to shrikefi_sim.vcd.
echo.

:: 3. Launch GTKWave Viewer
if exist C:\iverilog\gtkwave\bin\gtkwave.exe (
    echo [3/3] Launching GTKWave Waveform Viewer...
    start "" C:\iverilog\gtkwave\bin\gtkwave.exe shrikefi_sim.vcd shrikefi_sim.gtkw
) else (
    echo GTKWave not found at default path. Open shrikefi_sim.vcd manually.
)

pause

