@echo off
setlocal

call ConfigWindows.bat

for /f "delims=" %%V in ('git describe --abbrev=0') do set "VERSION=%%V"
if not defined VERSION (
	echo Failed to determine the latest Git tag.
	exit /b 1
)

set "VERSION_UNDERSCORE=%VERSION:.=_%"
set "OUTPUT_DIR=tools\auto_build\Output"
set "INSTALLER_SCRIPT=tools\auto_build\hise_installer.iss"
set "HISE_EXE=projects\standalone\Builds\VisualStudio2026\x64\Release\App\HISE.exe"

if not exist "%HISE_EXE%" (
	echo HISE Standalone binary not found: %HISE_EXE%
	echo Build it with 01_BuildHISE.bat first.
	exit /b 1
)

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

%installer_command% /DMyAppVersion="%VERSION%" /DMyOutputVersion="%VERSION_UNDERSCORE%" /DMyHiseExe="%CD%\%HISE_EXE%" /O"%CD%\%OUTPUT_DIR%" "%CD%\%INSTALLER_SCRIPT%"
if errorlevel 1 exit /b 1

echo Installer created in %OUTPUT_DIR%
endlocal

