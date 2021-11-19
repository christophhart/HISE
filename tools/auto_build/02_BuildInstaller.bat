@echo off 

call ConfigWindows.bat

del install_script.iss

set PRODUCT=HISE
set COMPANY="Hart Instruments"
set WEBSITE=http://hise.audio/

set FART_TOOL=fart

git describe --abbrev=0 > tmpFile

SET /p VERSION_POINT= < tmpFile

echo Version with point: %VERSION_POINT%

%FART_TOOL% tmpFile . _

SET /p VERSION_UNDERSCORE= < tmpFile

echo Version with underscores: %VERSION_UNDERSCORE%

del tmpFile

SET version=Enter version with underscores for the filename: 
SET filename=%PRODUCT%_%VERSION_UNDERSCORE%.exe

copy iss_installer_template.iss install_script.iss

%FART_TOOL% install_script.iss $PRODUCT %PRODUCT%
%FART_TOOL% install_script.iss $COMPANY %COMPANY%
%FART_TOOL% install_script.iss $WEBSITE %WEBSITE%
%FART_TOOL% install_script.iss $VERSION_POINT %VERSION_POINT%
%FART_TOOL% install_script.iss $VERSION_UNDERSCORE %VERSION_UNDERSCORE%

REM =======================================================================================
REM Building Installer

echo Building Installer...

SET installer_project=install_script.iss

%installer_command% %installer_project%