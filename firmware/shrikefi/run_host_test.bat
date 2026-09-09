@echo off
title ShrikeFi Host Test (AI & Multi-Sensor Simulation)
cd /d "%~dp0"
echo ================================================================
echo   Compiling ShrikeFi Host Test with GCC...
echo ================================================================
gcc -I. -I../../firmware/core -o shrikefi_host.exe main_shrikefi.c shrikefi_link_driver.c esp32_i2c_hal.c max30102.c bme280.c pms5003.c ssd1306.c ../../firmware/core/hrv_analysis.c ../../firmware/core/spo2_engine.c ../../firmware/core/disaster_risk_engine.c ../../firmware/core/nn_risk_model.c ../../firmware/core/nn_risk_model_int8.c ../../firmware/core/pm25_calibration_int8.c wifi_mqtt_manager.c -lm
if %errorlevel% neq 0 (
    echo [ERROR] Compilation failed!
    pause
    exit /b 1
)
echo [OK] Compilation successful. Running test...
echo.
shrikefi_host.exe
echo.
pause
