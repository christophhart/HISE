
#if defined (__arm__) || defined (__arm64__)
  #define JUCE_ARM 1
#else
  #define JUCE_INTEL 0
#endif

#define ASMJIT_STATIC 1
#define ASMJIT_EMBED 1

#pragma warning( push )
#pragma warning( disable : 4245)
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
#pragma warning( push )
#pragma warning( disable : 4307)


