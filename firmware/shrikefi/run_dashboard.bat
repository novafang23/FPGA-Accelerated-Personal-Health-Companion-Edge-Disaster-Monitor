@echo off
title EdgeGuard / ShrikeFi - Compile & Launch GUI Dashboard
cd /d "%~dp0"

echo ================================================================
echo   Compiling EdgeGuard / ShrikeFi GUI Dashboard with GCC...
echo ================================================================
gcc -O2 -s -I. -I../../firmware/core -o shrikefi_dashboard.exe shrikefi_dashboard.c ../../firmware/core/clinical_vitals_engine.c ../../firmware/core/disaster_risk_engine.c ../../firmware/core/nn_risk_model.c ../../firmware/core/nn_risk_model_int8.c ../../firmware/core/pm25_calibration_int8.c ../../firmware/core/ppg_sqi.c ../../firmware/core/ppg_respiratory_rate.c ../../firmware/core/hrv_analysis.c ../../firmware/core/spo2_engine.c -mwindows -lgdi32 -luser32 -lkernel32 -lcomctl32 -lm

if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)

copy /y shrikefi_dashboard.exe ..\..\shrikefi_dashboard.exe >nul
echo [OK] Compilation successful.
echo Launching GUI Dashboard window...
start "" shrikefi_dashboard.exe
exit /b 0
