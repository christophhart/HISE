@echo off

set /p old=Enter old version (using point notation): 
set /p new=Enter new version (usint point notation): 

fart hi_backend\hi_backend.h %old% %new%
fart hi_components\hi_components.h %old% %new%
fart hi_core\hi_core.h %old% %new%
fart hi_dsp\hi_dsp.h %old% %new%
fart hi_dsp_library\hi_dsp_library.h %old% %new%
fart hi_frontend\hi_frontend.h %old% %new%
fart hi_lac\hi_lac.h %old% %new%
fart hi_modules\hi_modules.h %old% %new%
fart hi_sampler\hi_sampler.h %old% %new%
fart hi_scripting\hi_scripting.h %old% %new%
fart hi_streaming\hi_streaming.h %old% %new%