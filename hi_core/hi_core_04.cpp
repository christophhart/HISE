
#if JUCE_MAC && USE_VDSP_FFT
// This deactivates some duplicate classes in the Apple API
#define Point DummyPoint
#define Component DummyComponent
#define MemoryBlock DummyMB
#include <Accelerate/Accelerate.h>
#undef Point
#undef Component
#undef MemoryBlock
#endif

#include "hi_core.h"

#if USE_BACKEND
#include "../hi_backend/hi_backend.h"
#else
#include "../hi_frontend/hi_frontend.h"
#include "../hi_scripting/hi_scripting.h"
#endif

#include "hi_sampler/hi_sampler.cpp"
#include "hi_modules/hi_modules_01.cpp"