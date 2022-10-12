#include <AppConfig.h>

#if HISE_INCLUDE_FAUST && HISE_INCLUDE_FAUST_JIT

#include <faust/dsp/dsp.h>
#include <faust/gui/UI.h>
#include <faust/gui/meta.h>
#include <faust/dsp/libfaust.h>

// libfaust headers need to be included before hi_faust.h in hi_faust_lib.h
#include "hi_faust_lib.h"

#include "faust_wrap/dsp/libfaust.h"
#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/libfaust-c-backend-placeholder.cpp"
#else // !HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/libfaust.cpp"
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE

// llvm jit
#include <faust/dsp/llvm-dsp.h>
#include "faust_wrap/dsp/llvm-dsp.h"
#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/llvm-dsp-c-backend-placeholder.cpp"
#else // HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/llvm-dsp.cpp"
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE

// interpreter
#include <faust/dsp/interpreter-dsp.h>
#include "faust_wrap/dsp/interpreter-dsp.h"
#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/interpreter-dsp-c-backend-placeholder.cpp"
#else // !HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/interpreter-dsp.cpp"
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#endif // HISE_INCLUDE_FAUST && HISE_INCLUDE_FAUST_JIT
