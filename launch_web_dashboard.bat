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

:: file:// URLs need forward slashes; ?sim=1 skips the connect gate so the
:: dashboard is live on screen immediately. Pass "live" as an argument to open
:: it without the flag, which shows the Connect button instead.
set "QUERY=?sim=1&full=1"
if /i "%~1"=="live" set "QUERY="
set "URL=file:///%HTML:\=/%"
if defined QUERY set "URL=%URL%%QUERY%"

:: Web Serial needs a Chromium browser. Find one, but do not fail if neither is
:: present - simulation mode works in anything.
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
echo   Simulation profiles: click any of the six buttons in the header.
echo   Live hardware:       run this script with  live  as the argument
echo                        (or click Connect in the page), then pick the
echo                        board's COM port.
echo.
exit /b 0
