@echo off
chcp 65001 >nul
title 体育理论题库查询系统
echo 正在启动题库查询系统...
"%~dp0.venv\Scripts\python.exe" "%~dp0main.py"
pause
