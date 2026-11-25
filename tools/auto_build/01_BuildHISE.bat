echo off

REM Automatic Build script for HISE
REM
REM This script executes these steps:
REM
REM Build the Standalone application for the latest Git tag.
REM The Release configuration disables plugin export for downloadable builds.

REM ===============================================================================

call ConfigWindows.bat

echo Building Standalone application

echo Resaving projects...

cd..
cd..

cd tools/SDK

tar -xf sdk.zip

cd ..
cd ..

powershell -NoProfile -ExecutionPolicy Bypass -File tools\auto_build\SetExportFlag.ps1 -Value 1
if errorlevel 1 exit /b 1

%projucerPath% --resave %standalone_projucer_project%
if errorlevel 1 exit /b 1

REM ===========================================================
REM Compiling

echo Compiling 64bit Standalone App

set Platform=X64
set PreferredToolArchitecture=x64

echo Compiling 64bit Standalone App...

"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MsBuild.exe"  %standalone_project% /t:Build /p:Configuration="Release";Platform=x64 /v:m

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	pause
	exit 1
)

echo "OK"
