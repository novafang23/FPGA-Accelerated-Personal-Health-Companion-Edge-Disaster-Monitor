@echo off
title ShrikeFi Live Serial Monitor
cd /d C:\Espressif\frameworks\esp-idf-v5.5.5
call export.bat
cd /d "%~dp0"
idf.py -p COM4 monitor
pause
