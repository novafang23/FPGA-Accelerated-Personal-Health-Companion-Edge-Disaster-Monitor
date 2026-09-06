@echo off
title Build and Flash ShrikeFi Firmware
set PORT=COM5
if not "%~1"=="" set PORT=%~1

cd /d C:\Espressif\frameworks\esp-idf-v5.5.5
call export.bat
cd /d "%~dp0"
echo ================================================================
echo   Flashing ShrikeFi ESP32-S3 on %PORT%...
echo ================================================================
idf.py -p %PORT% flash monitor
pause
