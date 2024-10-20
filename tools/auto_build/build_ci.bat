@echo off

call ConfigWindows.bat

cd..
cd..

cd tools/SDK

tar -xf sdk.zip

cd ..
cd ..


%projucerPath% --resave %standalone_projucer_project%

REM ===========================================================
REM Compiling

echo Compiling 64bit VST Plugins

set Platform=X64

echo Compiling Stereo Version...

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	exit 1
)

echo OK

echo Compiling 64bit Standalone App...

"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MsBuild.exe" %standalone_project% /t:Build /p:Configuration="CI";Platform=x64 /v:m

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	exit 1
)

echo OK


echo Running Unit Tests...

%hise_ci% run_unit_tests

if %errorlevel% NEQ 0 (
	echo ...
	echo ========================================================================
	echo Error at running unit tests. Aborting...
	cd tools\auto_build
	pause
	exit 1
)

echo Exporting Demo Project...

%hise_ci% set_project_folder "-p:%cd%/extras/demo_project/"

%hise_ci% export_ci "XmlPresetBackups/Demo.xml" -t:instrument -p:VST2 -a:x64 -nolto

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at exporting test project. Aborting...
	cd tools\auto_build
	pause
	exit 1)


"%cd%/extras/demo_project/Binaries/batchCompile.bat"

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling test project. Aborting...
	cd tools\auto_build
	pause
	exit 1)

echo OK

cd tools\auto_build
echo OK