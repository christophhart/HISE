// This is the land of the faust C interface, which is needed for windows

#include <AppConfig.h>
#if HISE_INCLUDE_FAUST && HISE_INCLUDE_FAUST_JIT
#include <faust/dsp/dsp.h>
#include <faust/gui/UI.h>
#include <faust/gui/meta.h>

// libfaust headers need to be included before hi_faust.h in hi_faust_lib.h
#include "hi_faust_lib.h"
#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include <faust/dsp/libfaust-c.h>
#include "faust_wrap/dsp/libfaust.h"
#include "faust_wrap/dsp/libfaust-c-backend.cpp"

// llvm jit
#include <faust/dsp/llvm-dsp-c.h>
#include "faust_wrap/dsp/llvm-dsp.h"
#include "faust_wrap/dsp/llvm-dsp-c-backend.cpp"
// interpreter
#include <faust/dsp/interpreter-dsp-c.h>
#include "faust_wrap/dsp/interpreter-dsp.h"
#include "faust_wrap/dsp/interpreter-dsp-c-backend.cpp"
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE

#endif // HISE_INCLUDE_FAUST && HISE_INCLUDE_FAUST_JIT
