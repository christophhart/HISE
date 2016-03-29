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

SET installer_project_path=tools\auto_build\

SET installer_project=hise_nightly_build.iss

REM ===============================================================================
REM Update the build version file

cd..
cd..

echo Previous version:
 
git describe --abbrev=0 > tmpFile
set /p prev_version= < tmpFile 
del tmpFile 

ECHO %prev_version%

SET /p build_version=Enter new build version: 

echo %build_version%

SET filename=HISE_099_build%build_version%.exe

if  %build_version% LSS %prev_version% (

	echo ========================================================================
	echo The new build number must be bigger than the old number. Aborting...
	cd tools\auto_build
	pause
    goto:eof
)

REM ===========================================================
REM Tag the current git commit

if %build_version% NEQ %prev_version% (

	echo Creating the changelog

	git log %prev_version%..master --oneline > changelog.txt

	del %nightly_build_folder%\changelog_%build_version%.txt
	
	xcopy changelog.rtf %nightly_build_folder% /Y

	ren %nightly_build_folder%\changelog.txt changelog_%build_version%.txt
	
	echo Writing new header file"

	echo #define BUILD_SUB_VERSION %build_version% > hi_core/BuildVersion.h
	
	echo Tagging the current git commit

	git commit -a -m "Build version update: %build_version%"

	git tag -a %build_version% -m "Build version: %build_version%"

	git push --all
)

REM ===========================================================
REM Compiling

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x86

echo Compiling 32bit VST Plugins

msbuild.exe %plugin_project% /t:Build /p:Configuration=Release;Platform=Win32

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	pause
    goto:eof
)

echo OK

echo Compiling 32bit Standalone App...

msbuild.exe %standalone_project% /t:Build /p:Configuration=Release;Platform=Win32

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	pause
    goto:eof
)

echo Compiling 64bit VST Plugins

call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat"

msbuild.exe %plugin_project% /t:Build /p:Configuration="Release 64bit";Platform=x64 /v:m

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	pause
    goto:eof
)

echo OK

echo Compiling 64bit Standalone App...

msbuild.exe %standalone_project% /t:Build /p:Configuration="Release 64bit";Platform=x64 /v:m

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at compiling. Aborting...
	cd tools\auto_build
	pause
    goto:eof
)

echo "OK"

REM =======================================================================================
REM Building Installer

echo Building Installer...

%installer_command% %installer_project_path%%installer_project%

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at building installer. Aborting...
	cd tools\auto_build
	pause
    goto:eof
)

xcopy %installer_project_path%\Output\RenameInstaller.exe %nightly_build_folder% /Y

del %nightly_build_folder%\HISE_099_build%build_version%.exe

ren %nightly_build_folder%\RenameInstaller.exe %filename%

rmdir /S /Q %installer_project_path%\Output\

echo "OK"

REM =======================================================================================
REM FTP Upload

echo "Uploading to FTP..."

cd %nightly_build_folder%

SET hostname=hartinstruments.net

Set /p user=Enter FTP User-Name:
SET /p password=Enter new FTP-Password: 

echo open %hostname%>upload.ftp
echo %user%>>upload.ftp
echo %password%>>upload.ftp
echo cd "html/hise/download/nightly_builds/">> upload.ftp
echo bin>>upload.ftp
echo hash>>upload.ftp
echo put changelog_%build_version%.rtf>> upload.ftp
echo put %filename%>> upload.ftp
echo bye>> upload.ftp

ftp -s:upload.ftp

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at uploading FTP. Aborting...
	del /S /Q upload.ftp
	pause
    goto:eof
)

del /S /Q upload.ftp

echo ========================================================================
echo Commited, Compiled, Built Installer, uploaded to FTP sucessfull!
pause