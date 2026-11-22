@echo off
setlocal
py -3 "%~dp0buildandtest.py"
exit /b %errorlevel%
