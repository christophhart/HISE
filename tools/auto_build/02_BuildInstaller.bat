@echo off

call ConfigWindows.bat


cd ..
cd ..
%multipage_binary% --export:"%CD%/tools/auto_build/installer/hise_installer.json" --hisepath:"%CD%"
cd tools/auto_build/installer/Binaries/

echo %CD%

call batchCompile.bat

pause

