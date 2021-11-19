#include <AppConfig.h>

#if defined (__arm__) || defined (__arm64__)
  #define JUCE_ARM 1
#else
  #define JUCE_INTEL 1
#endif

#ifndef HISE_INCLUDE_RLOTTIE
#define HISE_INCLUDE_RLOTTIE 1
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomma"


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"

#pragma warning( push )
#pragma warning( disable : 4245)
#pragma warning( push )
#pragma warning( disable : 4100)
#pragma warning( push )
#pragma warning( disable : 4456)
#pragma warning( push )
#pragma warning( disable : 4458)
#pragma warning( push )
#pragma warning( disable : 4706)
#pragma warning( push )
#pragma warning( disable : 4702)
#pragma warning( push )
#pragma warning( disable : 4838)


