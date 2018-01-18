@echo off

if "%~1"=="" (
   echo.
   echo SYNTAX: set_version old new
   echo ==================================================================
   echo.
   echo Sets the version number of all hi_modules, the installer projects
   echo and the projucer projects from the old version to the new version.
   echo You need to use semantic versioning major.minor.patch
   echo ==================================================================
   goto :end
)

echo Bumping version "%1" to "%2"

set old_version="%1"
set new_version="%2"


fart "hi_backend\hi_backend.h" %old_version% %new_version%
fart "hi_components\hi_components.h" %old_version% %new_version%
fart "hi_core\hi_core.h" %old_version% %new_version%
fart "hi_dsp\hi_dsp.h" %old_version% %new_version%
fart "hi_frontend\hi_frontend.h" %old_version% %new_version%
fart "hi_modules\hi_modules.h" %old_version% %new_version%
fart "hi_sampler\hi_sampler.h" %old_version% %new_version%
fart "hi_scripting\hi_scripting.h" %old_version% %new_version%
fart "hi_streaming\hi_streaming.h" %old_version% %new_version%

fart "projects\standalone\HISE Standalone.jucer" %old_version% %new_version%
fart "projects\plugin\HISE.jucer" %old_version% %new_version%

:end