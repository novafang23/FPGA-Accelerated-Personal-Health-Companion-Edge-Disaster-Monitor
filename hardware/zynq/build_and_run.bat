@echo off
setlocal enabledelayedexpansion

echo ================================================================
echo   SIH26181: AI-Powered Personal Health Companion
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
if not defined CC       set "CC=gcc"

:: 1. Compile Verilog RTL Simulation
echo [1/3] Compiling Verilog RTL Accelerator with Icarus Verilog...
%IVERILOG% -o sim_ppg.vvp tb_ppg_system.v axi_ppg_accelerator.v ../common/moving_average_8tap.v ../common/ppg_peak_detector.v
if %errorlevel% neq 0 (
    echo [ERROR] Verilog compilation failed!
    pause
    exit /b 1
)
echo [OK] Verilog compilation successful.
echo.

:: 2. Run Testbench Simulation & Generate Waveforms
echo [2/3] Running Hardware System Testbench (vvp)...
%VVP% sim_ppg.vvp
if %errorlevel% neq 0 (
    echo [ERROR] Simulation execution failed!
    pause
    exit /b 1
)
echo [OK] VCD Waveform dumped to ppg_system.vcd
echo.

:: 3. Compile C Disaster Resilience & Health Demo
echo [3/4] Compiling C Health Monitor ^& Disaster Simulation...
%CC% -Wall -Wextra -I../../firmware/core -I../../firmware/zynq -o health_demo.exe ../../firmware/zynq/main_simulation.c ../../firmware/core/hrv_analysis.c ../../firmware/core/spo2_engine.c ../../firmware/core/disaster_risk_engine.c ../../firmware/core/nn_risk_model.c ../../firmware/core/nn_risk_model_int8.c -lm
if %errorlevel% neq 0 (
    echo [ERROR] C compilation failed!
    pause
    exit /b 1
)
echo [OK] health_demo.exe built successfully.
echo.

:: 4. Compile Compare Harness (Rule Engine vs NN side-by-side)
echo [4/5] Compiling Compare Harness (Rule Engine vs NN)...
%CC% -Wall -Wextra -I../../firmware/core -I../../firmware/zynq -o compare_harness.exe ../../firmware/zynq/compare_harness.c ../../firmware/core/hrv_analysis.c ../../firmware/core/spo2_engine.c ../../firmware/core/disaster_risk_engine.c ../../firmware/core/nn_risk_model.c ../../firmware/core/nn_risk_model_int8.c -lm
if %errorlevel% neq 0 (
    echo [ERROR] Compare harness compilation failed!
    pause
    exit /b 1
)
echo [OK] compare_harness.exe built successfully.
echo.

:: 5. Compile and Run Unit Tests
echo [5/5] Compiling Unit Tests...
%CC% -Wall -Wextra -I../../firmware/core -I../../firmware/zynq -o test_engine.exe ../../firmware/zynq/test_disaster_risk_engine.c ../../firmware/core/hrv_analysis.c ../../firmware/core/spo2_engine.c ../../firmware/core/disaster_risk_engine.c ../../firmware/core/nn_risk_model.c ../../firmware/core/nn_risk_model_int8.c -lm
if %errorlevel% neq 0 (
    echo [ERROR] Unit test compilation failed!
    pause
    exit /b 1
)
echo [OK] test_engine.exe built successfully.
echo.

:menu
echo ================================================================
echo   Select an action to launch:
echo ================================================================
echo   [0] Launch Standalone Graphical Windows Dashboard (GUI .exe with Live PPG Oscilloscope)
echo   [1] Launch Live Health ^& Disaster Simulation Dashboard (Console)
echo   [2] Launch GTKWave Waveform Viewer (PPG ^& AXI Bus signals)
echo   [3] Re-run Hardware RTL Testbench
echo   [4] Run Flood / Hypothermia Scenario Directly
echo   [5] Run Compare Harness (Rule Engine vs NN side-by-side)
echo   [6] Run Unit Tests
echo   [7] Exit
echo ================================================================
set /p choice="Enter option (0-7): "

if "%choice%"=="0" (
    if exist "%~dp0..\..\shrikefi_dashboard.exe" (
        echo Launching Graphical Dashboard window...
        start "" "%~dp0..\..\shrikefi_dashboard.exe"
    ) else (
        echo Dashboard not built. The .exe is a generated artifact and is not tracked in git.
        echo Build it from firmware\shrikefi\shrikefi_dashboard.c, or run
        echo   firmware\shrikefi\run_dashboard.bat
    )
    goto menu
)

if "%choice%"=="1" (
    cls
    health_demo.exe
    goto menu
)
if "%choice%"=="2" (
    echo Launching GTKWave...
    start "" %GTKWAVE% ppg_system.vcd signals.gtkw
    goto menu
)
if "%choice%"=="3" (
    %VVP% sim_ppg.vvp
    goto menu
)
if "%choice%"=="4" (
    cls
    health_demo.exe --hypothermia
    goto menu
)
if "%choice%"=="5" (
    cls
    compare_harness.exe
    goto menu
)
if "%choice%"=="6" (
    cls
    test_engine.exe
    pause
    goto menu
)
if "%choice%"=="7" (
    exit /b 0
)

goto menu
