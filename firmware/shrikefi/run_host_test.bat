@echo off
title "ShrikeFi Host Test (AI & Multi-Sensor Simulation)"
cd /d "%~dp0"
echo ================================================================
echo   Compiling ShrikeFi Host Test with GCC...
echo ================================================================
gcc -Wall -Wextra -Werror -pedantic -std=c11 -I. -I../../firmware/core -o "%TEMP%\shrikefi_host.exe" main_shrikefi.c shrikefi_link_driver.c esp32_i2c_hal.c max30102.c bme280.c pms5003.c gnss_nmea.c gps_l89hs.c mpu6050.c fall_detect.c ssd1306.c ../../firmware/core/hrv_analysis.c ../../firmware/core/spo2_engine.c ../../firmware/core/disaster_risk_engine.c ../../firmware/core/nn_risk_model.c ../../firmware/core/nn_risk_model_int8.c ../../firmware/core/pm25_correction.c ../../firmware/core/clinical_vitals_engine.c ../../firmware/core/ppg_sqi.c ../../firmware/core/ppg_respiratory_rate.c ../../firmware/core/pressure_trend.c location.c sos.c web_status.c -lm
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    if not "%1"=="--no-pause" if "%CI%"=="" pause
    exit /b 1
)
echo [OK] Compilation successful. Running test...
echo.
"%TEMP%\shrikefi_host.exe"
echo.
if not "%1"=="--no-pause" if "%CI%"=="" pause

