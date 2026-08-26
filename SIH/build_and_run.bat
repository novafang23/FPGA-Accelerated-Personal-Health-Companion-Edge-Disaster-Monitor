@echo off
setlocal enabledelayedexpansion

echo ================================================================
echo   SIH26181: AI-Powered Personal Health Companion
echo   Qualcomm Hardware Challenge - Smart India Hackathon 2026
echo ================================================================
echo.

:: 1. Compile Verilog RTL Simulation
echo [1/3] Compiling Verilog RTL Accelerator with Icarus Verilog...
C:\iverilog\bin\iverilog.exe -o sim_ppg.vvp tb_ppg_system.v axi_ppg_accelerator.v moving_average_8tap.v ppg_peak_detector.v
if %errorlevel% neq 0 (
    echo [ERROR] Verilog compilation failed!
    pause
    exit /b 1
)
echo [OK] Verilog compilation successful.
echo.

:: 2. Run Testbench Simulation & Generate Waveforms
echo [2/3] Running Hardware System Testbench (vvp)...
C:\iverilog\bin\vvp.exe sim_ppg.vvp
if %errorlevel% neq 0 (
    echo [ERROR] Simulation execution failed!
    pause
    exit /b 1
)
echo [OK] VCD Waveform dumped to ppg_system.vcd
echo.

:: 3. Compile C Disaster Resilience & Health Demo
echo [3/4] Compiling C Health Monitor ^& Disaster Simulation...
C:\msys64\ucrt64\bin\gcc.exe -Wall -Wextra -o health_demo.exe main_simulation.c hrv_analysis.c spo2_engine.c disaster_risk_engine.c nn_risk_model.c -lm
if %errorlevel% neq 0 (
    echo [ERROR] C compilation failed!
    pause
    exit /b 1
)
echo [OK] health_demo.exe built successfully.
echo.

:: 4. Compile Compare Harness (Rule Engine vs NN side-by-side)
echo [4/5] Compiling Compare Harness (Rule Engine vs NN)...
C:\msys64\ucrt64\bin\gcc.exe -Wall -Wextra -o compare_harness.exe compare_harness.c hrv_analysis.c spo2_engine.c disaster_risk_engine.c nn_risk_model.c -lm
if %errorlevel% neq 0 (
    echo [ERROR] Compare harness compilation failed!
    pause
    exit /b 1
)
echo [OK] compare_harness.exe built successfully.
echo.

:: 5. Compile and Run Unit Tests
echo [5/5] Compiling Unit Tests...
C:\msys64\ucrt64\bin\gcc.exe -Wall -Wextra -o test_engine.exe test_disaster_risk_engine.c disaster_risk_engine.c nn_risk_model.c -lm
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
echo   [1] Launch Live Health ^& Disaster Simulation Dashboard (All Scenarios)
echo   [2] Launch GTKWave Waveform Viewer (PPG ^& AXI Bus signals)
echo   [3] Re-run Hardware RTL Testbench
echo   [4] Run Flood / Hypothermia Scenario Directly
echo   [5] Run Compare Harness (Rule Engine vs NN side-by-side)
echo   [6] Run Unit Tests
echo   [7] Exit
echo ================================================================
set /p choice="Enter option (1-7): "

if "%choice%"=="1" (
    cls
    health_demo.exe
    goto menu
)
if "%choice%"=="2" (
    echo Launching GTKWave...
    start "" "C:\iverilog\gtkwave\bin\gtkwave.exe" ppg_system.vcd signals.gtkw
    goto menu
)
if "%choice%"=="3" (
    C:\iverilog\bin\vvp.exe sim_ppg.vvp
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
