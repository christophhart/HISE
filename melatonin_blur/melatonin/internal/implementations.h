#pragma once

// ARGB on Windows and macOS fallback when no vImage
#include "../implementations/gin.h"


#pragma warning(push)
#pragma warning(disable: 4267)

// These are *compile-time* flags for implementation choices
// There are also runtime considerations
#if JUCE_MAC || JUCE_IOS

    // https://developer.apple.com/documentation/accelerate/4172615-vimagesepconvolve_argb8888
    #if (defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 140000) \
        || (defined(__IPHONE_OS_VERSION_MAX_ALLOWED) && __IPHONE_OS_VERSION_MAX_ALLOWED >= 170000)

        #define MELATONIN_BLUR_VIMAGE 1
        #define MELATONIN_BLUR_VIMAGE_MACOS14 1
        #include "../implementations/vImage_macOS14.h"
    #elif (defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 110000)
    // *Compiling* has to happen on macOS > 11.0 to support vImageSepConvolve_Planar8
    // Once compiled, we will check at runtime before relying on the vImage function
        #define MELATONIN_BLUR_VIMAGE 1
        #include "../implementations/vImage.h" // Single channel
    #else
        #include "../implementations/float_vector_stack_blur.h"
    #endif
#elif JUCE_WINDOWS
    #if defined(PAMPLEJUCE_IPP) || defined(_IPP_SEQUENTIAL_STATIC)
        #define MELATONIN_BLUR_IPP 1
        #include "../implementations/ipp_vector.h" // single channel
    #else
        #include "../implementations/float_vector_stack_blur.h"
    #endif
#elif JUCE_LINUX
    #include "../implementations/float_vector_stack_blur.h"
#else
  #error "Unsupported platform!"
#endif

#if JUCE_MAC || JUCE_IOS
    #include <TargetConditionals.h>
#endif


// *Runtime* checks for vImage
// Even if it compiles, we need to check when running on older devices
namespace melatonin::internal
{
    [[maybe_unused]] [[nodiscard]] static bool vImageARGBAvailable()
    {
#if defined(JUCE_MAC)
        if (__builtin_available (macOS 14.0, *))
            return true;
#elif defined(JUCE_IOS)
        if (__builtin_available (iOS 17.0, *))
            return true;
#endif
        return false;
    }

    [[maybe_unused]] [[nodiscard]] static bool vImageSingleChannelAvailable()
    {
#if defined(JUCE_MAC)
        if (__builtin_available (macOS 11.0, *))
            return true;
#elif defined(JUCE_IOS)
        if (__builtin_available (iOS 14.0, *))
            return true;
#endif
        return false;
    }
}

// Don't use these directly, use melatonin::CachedBlur!
namespace melatonin::blur
{
    static void singleChannel (juce::Image& img, size_t radius)
    {
#if MELATONIN_BLUR_VIMAGE
        if (internal::vImageSingleChannelAvailable())
            melatonin::blur::vImageSingleChannel (img, radius);
        else
            melatonin::stackBlur::ginSingleChannel (img, static_cast<unsigned int> (radius));
#elif defined(MELATONIN_BLUR_IPP)
        ippVectorSingleChannel (img, radius);
#else
        melatonin::blur::juceFloatVectorSingleChannel (img, static_cast<int>(radius));
#endif
    }

    static void argb ([[maybe_unused]] juce::Image& srcImage, juce::Image& dstImage, size_t radius)
    {
#if MELATONIN_BLUR_VIMAGE_MACOS14
        if (internal::vImageARGBAvailable())
            melatonin::blur::vImageARGB (srcImage, dstImage, radius);
        else
            melatonin::stackBlur::ginARGB (dstImage, static_cast<unsigned int> (radius));
#else
        stackBlur::ginARGB (dstImage, static_cast<unsigned int>(radius));
#endif
    }
}

#pragma warning(pop)
