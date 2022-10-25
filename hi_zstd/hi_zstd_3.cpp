#if !(defined (_WIN32) || defined (_WIN64))
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "hi_zstd.h"

// Contains dict builder files

#include "zstd/dictBuilder/cover.c"
#include "zstd/dictBuilder/divsufsort.c"
#include "zstd/dictBuilder/zdict.c"

#include "hi_zstd/ZstdHelpers.cpp"
#include "hi_zstd/ZstdInputStream.cpp"
#include "hi_zstd/ZstdOutputStream.cpp"

#include "hi_zstd/ZstdUnitTests.cpp"

#if !(defined (_WIN32) || defined (_WIN64))
#pragma warning( pop ) 
#endif