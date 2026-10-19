@echo off
title Build and Flash ShrikeFi Firmware
rem Default port. The board's CH9102 UART enumerated as COM5 on the old hub and
rem COM4 after it was re-plugged; pass a port as the first argument to override.
set PORT=COM4
if not "%~1"=="" set PORT=%~1

cd /d C:\Espressif\frameworks\esp-idf-v5.5.5
call export.bat
cd /d "%~dp0"
echo ================================================================
echo   Flashing ShrikeFi ESP32-S3 on %PORT%...
echo ================================================================
idf.py -p %PORT% flash monitor
pause
