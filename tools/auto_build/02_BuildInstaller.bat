@echo off
setlocal

REM Run from the repository root, just like the installer workflow step.
if not exist "projects\standalone\HISE Standalone.jucer" (
	echo Run 02_BuildInstaller.bat from the repository root: %CD%
	exit /b 1
)

for /f "delims=" %%V in ('JUCE\projucer\Projucer.exe --get-version "projects\standalone\HISE Standalone.jucer"') do set "VERSION=%%V"
if not defined VERSION (
	echo Failed to read the version from projects\standalone\HISE Standalone.jucer using Projucer.
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

set "ISCC=%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" set "ISCC=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist "%ISCC%" (
	echo Inno Setup compiler not found in the user or system installation directories.
	exit /b 1
)

"%ISCC%" /DMyAppVersion="%VERSION%" /DMyOutputVersion="%VERSION_UNDERSCORE%" /DMyHiseExe="%CD%\%HISE_EXE%" /O"%CD%\%OUTPUT_DIR%" "%CD%\%INSTALLER_SCRIPT%"
if errorlevel 1 exit /b 1

echo Installer created in %OUTPUT_DIR%
endlocal

