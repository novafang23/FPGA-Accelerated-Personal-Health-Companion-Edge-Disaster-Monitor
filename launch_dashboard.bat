@echo off
title EdgeGuard / ShrikeFi - Launching GUI Dashboard...
cd /d "%~dp0"

echo ================================================================
echo   Starting EdgeGuard / ShrikeFi Native Graphical Dashboard...
echo   (SIH26181: FPGA-Accelerated Health Companion & Disaster Triage)
echo ================================================================
echo.

if not exist "%~dp0shrikefi_dashboard.exe" (
    echo [INFO] shrikefi_dashboard.exe not found. Compiling with GCC...
    gcc -O2 -s -Ifirmware/shrikefi -Ifirmware/core -o shrikefi_dashboard.exe firmware/shrikefi/shrikefi_dashboard.c firmware/core/clinical_vitals_engine.c firmware/core/disaster_risk_engine.c firmware/core/nn_risk_model.c firmware/core/nn_risk_model_int8.c firmware/core/pm25_calibration_int8.c firmware/core/ppg_sqi.c firmware/core/ppg_respiratory_rate.c firmware/core/hrv_analysis.c firmware/core/spo2_engine.c -mwindows -lgdi32 -luser32 -lkernel32 -lcomctl32 -lm
    if not exist "%~dp0shrikefi_dashboard.exe" (
        echo [ERROR] GCC build failed. Please install GCC/MinGW or run from msys2.
        pause
        exit /b 1
    )
    echo [OK] Built shrikefi_dashboard.exe successfully.
)

start "" "%~dp0shrikefi_dashboard.exe"
echo [OK] Dashboard launched successfully in a separate window.
timeout /t 2 >nul
exit /b 0
