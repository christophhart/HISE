echo off

call ConfigWindows.bat

echo "Uploading to FTP..."

SET /p version=Enter version with underscores for the filename: 
SET filename=HISE_%version%.exe

cd %nightly_build_folder%

SET hostname=hartinstruments.net

Set /p user=Enter FTP User-Name:
SET /p password=Enter new FTP-Password: 

echo open %hostname%>upload.ftp
echo %user%>>upload.ftp
echo %password%>>upload.ftp
echo cd "html/hise/download/">> upload.ftp
echo bin>>upload.ftp
echo hash>>upload.ftp
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