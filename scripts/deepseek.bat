@echo off
rem %~dp0 already ends in "scripts\", so the path must not repeat it. The doubled
rem form resolved to scripts\scripts\deepseek_copilot.py and never ran.
python "%~dp0deepseek_copilot.py" %*
