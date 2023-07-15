#include <AppConfig.h>

#if HISE_INCLUDE_SNEX


#define MIR_NO_INTERP 1

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

#include "snex_mir/src/mir.c"
#endif

#pragma clang diagnostic pop
