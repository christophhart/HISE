#pragma once

#ifndef RTNEURAL_NAMESPACE
#define RTNEURAL_NAMESPACE RTNeural
#endif

#ifndef RTNEURAL_DEFAULT_ALIGNMENT
#if _MSC_VER
#pragma message("RTNEURAL_DEFAULT_ALIGNMENT was not defined! Using default alignment = 16.")
#else
#warning "RTNEURAL_DEFAULT_ALIGNMENT was not defined! Using default alignment = 16."
#endif
#define RTNEURAL_DEFAULT_ALIGNMENT 16
#endif

/**
    Facilitate testing real-time safety with RealtimeSanitizer (RADSan)

    For more information, see https://github.com/realtime-sanitizer/radsan.
    The `[[clang::realtime]]` attribute is unique to a RADSan-modified
    version of clang, and its appearance will result in an error for other
    compilers. Here, we make its presence configurable. RealtimeSanitizer is
    very early stage, and this configuration may change.

    This real-time safety checking is designed to function mostly in CI. If you
    wish to test it locally on your dev machine, you'll need to either:

        i) use the RADSan clang Docker image (recommended), or
       ii) get the RADSan clang compiler,

    for which instructions may be found in the RADSan repository above.
*/
#ifdef RTNEURAL_RADSAN_ENABLED
  #define RTNEURAL_REALTIME [[clang::realtime]]
#else
  #define RTNEURAL_REALTIME
#endif
