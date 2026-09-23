@echo off
setlocal
title VALOR - Web Dashboard (SIH26181)
cd /d "%~dp0"

echo ================================================================
echo   VALOR - Vital and Atmospheric Logic for Offline Rescue
echo   Web dashboard  ^|  ShrikeFi ESP32-S3 + Renesas ForgeFPGA
echo ================================================================
echo.

set "HTML=%~dp0firmware\shrikefi\dashboard\valor_dashboard.html"
if not exist "%HTML%" (
    echo [ERROR] Dashboard not found at:
    echo         %HTML%
    pause
    exit /b 1
)

:: Usage:
::   launch_web_dashboard.bat           open the page; choose Connect or Simulation
::   launch_web_dashboard.bat sim       start straight in the simulation
::   launch_web_dashboard.bat full      simulation, profile buttons hidden (kiosk)
::
:: There is intentionally no "connect automatically" option: Web Serial requires
:: a user gesture before it will grant port access, so the port picker cannot be
:: triggered by a script.
if /i "%~1"=="sim"  set "QUERY=?sim=1"
if /i "%~1"=="full" set "QUERY=?sim=1&full=1"
set "URL=file:///%HTML:\=/%"
if defined QUERY set "URL=%URL%%QUERY%"

:: Web Serial needs a Chromium browser. Find one, but do not fail if neither is
:: present - the page itself explains the limitation.
set "BROWSER="
if not defined BROWSER if exist "%ProgramFiles%\Google\Chrome\Application\chrome.exe" set "BROWSER=%ProgramFiles%\Google\Chrome\Application\chrome.exe"
if not defined BROWSER if exist "%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe" set "BROWSER=%ProgramFiles(x86)%\Google\Chrome\Application\chrome.exe"
if not defined BROWSER if exist "%ProgramFiles%\Microsoft\Edge\Application\msedge.exe" set "BROWSER=%ProgramFiles%\Microsoft\Edge\Application\msedge.exe"
if not defined BROWSER if exist "%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe" set "BROWSER=%ProgramFiles(x86)%\Microsoft\Edge\Application\msedge.exe"
if not defined BROWSER if exist "%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe" set "BROWSER=%LOCALAPPDATA%\Google\Chrome\Application\chrome.exe"

if defined BROWSER (
    echo [OK] Opening in: %BROWSER%
    start "" "%BROWSER%" "%URL%"
) else (
    echo [!] Chrome or Edge not found.
    echo     Opening in the default browser - simulation works there, but
    echo     LIVE mode needs Chrome or Edge for the Web Serial API.
    start "" "%URL%"
)

echo.
echo   To read the board:   close idf.py monitor / PuTTY / the Win32 dashboard
echo                        first - only one program can hold a COM port - then
echo                        press "Connect to device" and pick the port.
echo   No hardware?         press "Simulation" for the six clinical profiles.
echo.
exit /b 0
