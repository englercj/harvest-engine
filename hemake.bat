@echo off
setlocal EnableExtensions
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0hemake.ps1" %*
exit /b %ERRORLEVEL%
