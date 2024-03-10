#include "melatonin_blur.h"
#include "melatonin/cached_blur.cpp"
#include "melatonin/internal/cached_shadows.cpp"
#include "melatonin/internal/rendered_single_channel_shadow.cpp"

#if RUN_MELATONIN_TESTS
    #include "benchmarks/benchmarks.cpp"
    #include "tests/blur_implementations.cpp"
    #include "tests/drop_shadow.cpp"
    #include "tests/inner_shadow.cpp"
    #include "tests/shadow_scaling.cpp"
#endif
