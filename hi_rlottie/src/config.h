/* #undef LOTTIE_MODULE */

#ifdef LOTTIE_MODULE
#define LOTTIE_IMAGE_MODULE_SUPPORT
#endif

#define LOTTIE_IMAGE_MODULE_PLUGIN "librlottie-image-loader.dylib"

#define LOTTIE_THREAD

#ifdef LOTTIE_THREAD
#define LOTTIE_THREAD_SUPPORT
#endif

#define LOTTIE_CACHE

#ifdef LOTTIE_CACHE
#define LOTTIE_CACHE_SUPPORT
#endif
