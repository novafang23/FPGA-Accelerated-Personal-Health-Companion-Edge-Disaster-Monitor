@echo off
title Flash ShrikeFi ESP32-S3
setlocal

rem Default port; pass another one as the first argument if the board moved.
set PORT=COM4
if not "%~1"=="" set PORT=%~1

echo ================================================================
echo   SIH26181 ShrikeFi -- Target Firmware Flasher
echo   Target Port : %PORT% (ESP32-S3 USB Serial/JTAG)
echo ================================================================
echo.

cd /d "%~dp0firmware\shrikefi"
call build_and_flash.bat %PORT%
