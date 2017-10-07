@echo off 

call ConfigWindows.bat

SET /p version=Enter version with underscores for the filename: 
SET filename=HISE_%version%.exe

cd..
cd..


REM =======================================================================================
REM Building Installer

echo Building Installer...

SET installer_project_path=tools\auto_build\

SET installer_project=hise_nightly_build.iss

%installer_command% %installer_project_path%%installer_project%

if %errorlevel% NEQ 0 (
	echo ========================================================================
	echo Error at building installer. Aborting...
	cd tools\auto_build
	pause
    goto:eof
)

xcopy %installer_project_path%\Output\RenameInstaller.exe %nightly_build_folder% /Y

ren %nightly_build_folder%\RenameInstaller.exe %filename%


pause




echo "OK"