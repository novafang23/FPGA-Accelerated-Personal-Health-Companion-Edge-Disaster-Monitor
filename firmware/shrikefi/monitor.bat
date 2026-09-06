@echo off
title ShrikeFi Live Serial Monitor
set PORT=COM5
if not "%~1"=="" set PORT=%~1

cd /d C:\Espressif\frameworks\esp-idf-v5.5.5
call export.bat
cd /d "%~dp0"
echo ================================================================
echo   Opening Serial Monitor on %PORT%...
echo ================================================================
idf.py -p %PORT% monitor
pause
