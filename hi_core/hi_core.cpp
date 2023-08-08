
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

#include <BinaryData.h>

#include "hi_core.h"

#if USE_BACKEND
#include "../hi_backend/hi_backend.h"
#else
#include "../hi_frontend/hi_frontend.h"
#include "../hi_scripting/hi_scripting.h"
#endif



#if HISE_IOS
#include "Foundation/NSFileManager.h"
#include "Foundation/NSString.h"
#endif

hise::HiseColourScheme::Scheme hise::HiseColourScheme::currentColourScheme = Dark;

#include "hi_core/hi_core.cpp"

#include "hi_dsp/hi_dsp.cpp"
#include "hi_components/hi_components.cpp"
#include "hi_sampler/hi_sampler.cpp"
#include "hi_modules/hi_modules_01.cpp"