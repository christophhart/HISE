@echo off

git rev-parse HEAD > currentGitHash.txt

set /p commit=<currentGitHash.txt

echo #define PREVIOUS_HISE_COMMIT "%commit%" > "%~dp0\hi_backend\backend\currentGit.h"

echo Wrote Git commit info...

pause