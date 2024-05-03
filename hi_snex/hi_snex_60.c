

#if (defined (_WIN32) || defined (_WIN64))
#pragma warning( push )
#pragma warning( disable : 4018)
#pragma warning( disable : 4090)
#pragma warning( disable : 4130)
#pragma warning( disable : 4152)
#pragma warning( disable : 4204)
#pragma warning( disable : 4214)
#pragma warning( disable : 4267)
#pragma warning( disable : 4310)
#pragma warning( disable : 4389)
#pragma warning( disable : 4459)
#pragma warning( disable : 4701)
#pragma warning( disable : 4702)
#pragma warning( disable : 4703)
#pragma warning( disable : 4706)
#else
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#endif

#if HISE_INCLUDE_SNEX
#include "snex_mir/src/mir-gen.c"
#endif

#if (defined (_WIN32) || defined (_WIN64))
#pragma warning( pop ) 
#else
#pragma clang diagnostic pop
#endif


