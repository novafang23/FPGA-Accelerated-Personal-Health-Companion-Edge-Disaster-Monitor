@echo off
title EdgeGuard / ShrikeFi - Launching GUI Dashboard...
cd /d "%~dp0"

echo ================================================================
echo   Starting EdgeGuard / ShrikeFi Native Graphical Dashboard...
echo   (SIH26181: FPGA-Accelerated Health Companion & Disaster Triage)
echo ================================================================
echo.
start "" "%~dp0shrikefi_dashboard.exe"
echo [OK] Dashboard launched successfully in a separate window.
timeout /t 2 >nul
exit /b 0
