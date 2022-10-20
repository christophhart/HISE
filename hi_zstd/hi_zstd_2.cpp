
#if !(defined (_WIN32) || defined (_WIN64))
#pragma clang diagnostic ignored "-Weverything"
#endif

#include "hi_zstd.h"

// Contains compression files

#include "zstd/compress/fse_compress.c"
#include "zstd/compress/hist.c"
#include "zstd/compress/huf_compress.c"
#include "zstd/compress/zstd_compress.c"
#include "zstd/compress/zstd_double_fast.c"
#include "zstd/compress/zstd_fast.c"
#include "zstd/compress/zstd_lazy.c"
#include "zstd/compress/zstd_ldm.c"
#include "zstd/compress/zstd_opt.c"


#include "zstd/decompress/huf_decompress.c"
#include "zstd/decompress/zstd_decompress.c"

#if !(defined (_WIN32) || defined (_WIN64))
#pragma warning( pop ) 
#endif