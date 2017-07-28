
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

#include "JuceHeader.h"

HiseColourScheme::Scheme HiseColourScheme::currentColourScheme = Dark;

#include "additional_libraries/additional_libraries.cpp"
#include "hi_binary_data/hi_binary_data.cpp"
#include "hi_core/hi_core.cpp"
