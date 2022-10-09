#include <faust/dsp/dsp.h>
#include <faust/gui/UI.h>
#include <faust/gui/meta.h>
#include <faust/dsp/libfaust.h>

#include "../hi_faust_types/hi_faust_types.h"
#include "faust_wrap/dsp/libfaust.h"
#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/libfaust-c-backend-placeholder.cpp"
#else // !HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/libfaust.cpp"
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE


#if HISE_FAUST_USE_LLVM_JIT
#include <faust/dsp/llvm-dsp.h>
#include "faust_wrap/dsp/llvm-dsp.h"
#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/llvm-dsp-c-backend-placeholder.cpp"
#else // HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/llvm-dsp.cpp"
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE

#else // !HISE_FAUST_USE_LLVM_JIT
#include <faust/dsp/interpreter-dsp.h>
#include "faust_wrap/dsp/interpreter-dsp.h"
#if HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/interpreter-dsp-c-backend-placeholder.cpp"
#else // !HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#include "faust_wrap/dsp/interpreter-dsp.cpp"
#endif // HISE_FAUST_USE_LIBFAUST_C_INTERFACE
#endif // HISE_FAUST_USE_LLVM_JIT
