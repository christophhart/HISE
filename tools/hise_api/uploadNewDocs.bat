@echo off

del html /S /Q

doxygen hise.doxyfile

echo "Uploading to FTP..."

SET hostname=hartinstruments.net

echo ========================================================================
echo API build complete. Entering FTP upload...

Set /p user=Enter FTP User-Name:
SET /p password=Enter new FTP-Password: 

echo ========================================================================

echo open %hostname%>upload.ftp
echo %user%>>upload.ftp
echo %password%>>upload.ftp
echo prompt>>upload.ftp
echo mdel "html/hise/hise_api/*.*">> upload.ftp
echo mdel "html/hise/hise_api/search/*.*">> upload.ftp
echo cd "html/hise/hise_api/">> upload.ftp

echo mput html/*.*>> upload.ftp
echo mkdir search>>upload.ftp
echo cd search>>upload.ftp
echo mput html/search/*.*>>upload.ftp
echo bye>> upload.ftp

ftp -s:upload.ftp

del /S /Q upload.ftp

echo ========================================================================
echo 
pause