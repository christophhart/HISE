#if !(defined (_WIN32) || defined (_WIN64))
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "hi_zstd.h"

#define ZDICT_STATIC_LINKING_ONLY

#include "zstd/zstd.h"
#include "zstd/dictBuilder/zdict.h"


// Contains common files

#include "zstd/common/debug.c"
#include "zstd/common/entropy_common.c"
#include "zstd/common/error_private.c"
#include "zstd/common/fse_decompress.c"
#include "zstd/common/pool.c"
#include "zstd/common/threading.c"
#include "zstd/common/xxhash.c"
#include "zstd/common/zstd_common.c"

#if !(defined (_WIN32) || defined (_WIN64))
#pragma warning( pop ) 
#endif