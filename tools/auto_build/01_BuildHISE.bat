echo off

REM Automatic Build script for HISE
REM
REM This script executes these steps:
REM
REM 1. Tag the latest commit with a build ID
REM 2. Replace the BuildVersion.h file
REM 3. Commit and tag with the given build ID
REM 4. Export a changelog
REM 5. Compile the plugins and the standalone app
REM 6. Build the installer and the DMG file in the format "HISE_v_buildXXX_OSX.dmg"
REM 7. Uploads the installer and the changelog into the nightly build folder 

REM ===============================================================================

call ConfigWindows.bat

git describe --abbrev=0 > tmpFile

SET /p versionPoint= < tmpFile

del tmpFile

echo Building Installer %filename%

echo Resaving projects...

cd..
cd..

cd tools/SDK

tar -xf sdk.zip

cd ..
cd ..

echo Setting version number %versionPoint%

%projucerPath% --set-version %versionPoint% %standalone_projucer_project%
%projucerPath% --set-version %versionPoint% %plugin_projucer_project%
%projucerPath% --set-version %versionPoint% %multipagecreator_projucer_project%

%projucerPath% --resave %standalone_projucer_project%
%projucerPath% --resave %plugin_projucer_project%
%projucerPath% --resave %multipagecreator_projucer_project%

REM ===========================================================
REM Compiling

echo Compiling 64bit VST Plugins

set Platform=X64
set PreferredToolArchitecture=x64

echo Compiling Stereo Version...

REM Skip this for now...
REM "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\MSBuild\15.0\Bin\MsBuild.exe"  %plugin_project% /t:Build /p:Configuration="Release";Platform=x64 /v:m

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	pause
	exit 1
)

echo OK

echo Compiling 64bit Standalone App...

"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MsBuild.exe"  %standalone_project% /t:Build /p:Configuration="Release";Platform=x64 /v:m

echo Compiling 64bit VST plugin

"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MsBuild.exe"  %plugin_project% /t:Build /p:Configuration="Release";Platform=x64 /v:m

echo Compiling multipage creator

"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MsBuild.exe"  %multipagecreator_project% /t:Build /p:Configuration="Release";Platform=x64 /v:m

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	pause
	exit 1
)

echo "OK"
pause
