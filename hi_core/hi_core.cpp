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


#include "additional_libraries/additional_libraries.cpp"
#include "hi_binary_data/hi_binary_data.cpp"
#include "hi_components/hi_components.cpp"
#include "hi_core/hi_core.cpp"
#include "hi_dsp/hi_dsp.cpp"

#include "hi_sampler/hi_sampler.cpp"
