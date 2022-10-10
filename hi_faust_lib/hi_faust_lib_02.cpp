// This is the land of the faust C interface, which is needed for windows

#include <AppConfig.h>

#if HISE_INCLUDE_FAUST
#include <faust/dsp/dsp.h>
#include <faust/gui/UI.h>
#include <faust/gui/meta.h>
#include "../hi_faust_types/hi_faust_types.h"
#endif

#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include <faust/dsp/libfaust-c.h>
#include "faust_wrap/dsp/libfaust.h"
#include "faust_wrap/dsp/libfaust-c-backend.cpp"
#if HISE_FAUST_USE_LLVM_JIT
#include <faust/dsp/llvm-dsp-c.h>
#include "faust_wrap/dsp/llvm-dsp.h"
#include "faust_wrap/dsp/llvm-dsp-c-backend.cpp"
#else // !HISE_FAUST_USE_LLVM_JIT
#include <faust/dsp/interpreter-dsp-c.h>
#include "faust_wrap/dsp/interpreter-dsp.h"
#include "faust_wrap/dsp/interpreter-dsp-c-backend.cpp"
#endif // HISE_FAUST_USE_LLVM_JIT
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE
