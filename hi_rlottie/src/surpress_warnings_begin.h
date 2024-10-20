

#if defined (__arm__) || defined (__arm64__)
  #define JUCE_ARM 1
#else
  #define JUCE_INTEL 1
#endif



#ifndef HISE_INCLUDE_RLOTTIE
#define HISE_INCLUDE_RLOTTIE 1
#endif

#if (defined (_WIN32) || defined (_WIN64))

#pragma warning( push )
#pragma warning( disable : 4245)
#pragma warning( push )
#pragma warning( disable : 4251)
#pragma warning( push )
#pragma warning( disable : 4267)
#pragma warning( push )
#pragma warning( disable : 4334)
#pragma warning( push )
#pragma warning( disable : 4456)
#pragma warning( push )
#pragma warning( disable : 4458)
#pragma warning( push )
#pragma warning( disable : 4505)
#pragma warning( push )
#pragma warning( disable : 4706)
#pragma warning( push )
#pragma warning( disable : 4702)
#pragma warning( push )
#pragma warning( disable : 4701)
#pragma warning( push )
#pragma warning( disable : 4611)
#pragma warning( push )
#pragma warning( disable : 4838)
#pragma warning( push )
#pragma warning( disable : 4996)

#else

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wimplicit-int-conversion"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcomma"


#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshorten-64-to-32"


#endif





