
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

#include "additional_libraries/additional_libraries.cpp"


#if HISE_IOS
#include "Foundation/NSFileManager.h"
#include "Foundation/NSString.h"
#endif

hise::HiseColourScheme::Scheme hise::HiseColourScheme::currentColourScheme = Dark;



#include "hi_core/hi_core.cpp"

