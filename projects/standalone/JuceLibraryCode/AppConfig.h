/*

    IMPORTANT! This file is auto-generated each time you save your
    project - if you alter its contents, your changes may be overwritten!

    There's a section below where you can add your own custom code safely, and the
    Introjucer will preserve the contents of that block, but the best way to change
    any of these definitions is by using the Introjucer's project settings.

    Any commented-out settings will assume their default values.

*/

#ifndef __JUCE_APPCONFIG_JUHBUI__
#define __JUCE_APPCONFIG_JUHBUI__

//==============================================================================
// [BEGIN_USER_CODE_SECTION]

// (You can add your own code in this section, and the Introjucer will not overwrite it)

#define USE_IPP 1
#include "ipp.h"

#define HI_RUN_UNIT_TESTS 1

// [END_USER_CODE_SECTION]

//==============================================================================
#define JUCE_MODULE_AVAILABLE_hi_backend                      1
#define JUCE_MODULE_AVAILABLE_hi_core                         1
#define JUCE_MODULE_AVAILABLE_hi_dsp_library                  1
#define JUCE_MODULE_AVAILABLE_hi_modules                      1
#define JUCE_MODULE_AVAILABLE_hi_scripting                    1
#define JUCE_MODULE_AVAILABLE_juce_audio_basics               1
#define JUCE_MODULE_AVAILABLE_juce_audio_devices              1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats              1
#define JUCE_MODULE_AVAILABLE_juce_audio_processors           1
#define JUCE_MODULE_AVAILABLE_juce_audio_utils                1
#define JUCE_MODULE_AVAILABLE_juce_core                       1
#define JUCE_MODULE_AVAILABLE_juce_cryptography               1
#define JUCE_MODULE_AVAILABLE_juce_data_structures            1
#define JUCE_MODULE_AVAILABLE_juce_events                     1
#define JUCE_MODULE_AVAILABLE_juce_graphics                   1
#define JUCE_MODULE_AVAILABLE_juce_gui_basics                 1
#define JUCE_MODULE_AVAILABLE_juce_gui_extra                  1
#define JUCE_MODULE_AVAILABLE_juce_opengl                     1
#define JUCE_MODULE_AVAILABLE_juce_tracktion_marketplace      1

//==============================================================================
#ifndef    JUCE_STANDALONE_APPLICATION
 #define   JUCE_STANDALONE_APPLICATION 1
#endif

//==============================================================================
// hi_core flags:

#ifndef    USE_BACKEND
 #define   USE_BACKEND 1
#endif

#ifndef    USE_FRONTEND
 //#define USE_FRONTEND
#endif

#ifndef    IS_STANDALONE_APP
 #define   IS_STANDALONE_APP 1
#endif

#ifndef    USE_COPY_PROTECTION
 #define   USE_COPY_PROTECTION 0
#endif

#ifndef    USE_IPP
 #define   USE_IPP 1
#endif

#ifndef    USE_GLITCH_DETECTION
 #define   USE_GLITCH_DETECTION 1
#endif

#ifndef    ENABLE_PLOTTER
 #define   ENABLE_PLOTTER 1
#endif

#ifndef    ENABLE_SCRIPTING_SAFE_CHECKS
 #define   ENABLE_SCRIPTING_SAFE_CHECKS 1
#endif

#ifndef    ENABLE_ALL_PEAK_METERS
 #define   ENABLE_ALL_PEAK_METERS 1
#endif

#ifndef    ENABLE_CONSOLE_OUTPUT
 #define   ENABLE_CONSOLE_OUTPUT 1
#endif

#ifndef    ENABLE_HOST_INFO
 #define   ENABLE_HOST_INFO 1
#endif

#ifndef    ENABLE_CPU_MEASUREMENT
 #define   ENABLE_CPU_MEASUREMENT 1
#endif

//==============================================================================
// hi_dsp_library flags:

#ifndef    HI_EXPORT_DSP_LIBRARY
 #define   HI_EXPORT_DSP_LIBRARY 0
#endif

#ifndef    IS_STATIC_DSP_LIBRARY
 //#define IS_STATIC_DSP_LIBRARY
#endif

//==============================================================================
// juce_audio_devices flags:

#ifndef    JUCE_ASIO
 #define   JUCE_ASIO 1
#endif

#ifndef    JUCE_WASAPI
 //#define JUCE_WASAPI
#endif

#ifndef    JUCE_WASAPI_EXCLUSIVE
 //#define JUCE_WASAPI_EXCLUSIVE
#endif

#ifndef    JUCE_DIRECTSOUND
 #define   JUCE_DIRECTSOUND 1
#endif

#ifndef    JUCE_ALSA
 //#define JUCE_ALSA
#endif

#ifndef    JUCE_JACK
 //#define JUCE_JACK
#endif

#ifndef    JUCE_USE_ANDROID_OPENSLES
 //#define JUCE_USE_ANDROID_OPENSLES
#endif

#ifndef    JUCE_USE_CDREADER
 //#define JUCE_USE_CDREADER
#endif

#ifndef    JUCE_USE_CDBURNER
 //#define JUCE_USE_CDBURNER
#endif

//==============================================================================
// juce_audio_formats flags:

#ifndef    JUCE_USE_FLAC
 //#define JUCE_USE_FLAC
#endif

#ifndef    JUCE_USE_OGGVORBIS
 //#define JUCE_USE_OGGVORBIS
#endif

#ifndef    JUCE_USE_MP3AUDIOFORMAT
 //#define JUCE_USE_MP3AUDIOFORMAT
#endif

#ifndef    JUCE_USE_LAME_AUDIO_FORMAT
 //#define JUCE_USE_LAME_AUDIO_FORMAT
#endif

#ifndef    JUCE_USE_WINDOWS_MEDIA_FORMAT
 //#define JUCE_USE_WINDOWS_MEDIA_FORMAT
#endif

//==============================================================================
// juce_audio_processors flags:

#ifndef    JUCE_PLUGINHOST_VST
 #define   JUCE_PLUGINHOST_VST 0
#endif

#ifndef    JUCE_PLUGINHOST_VST3
 #define   JUCE_PLUGINHOST_VST3 0
#endif

#ifndef    JUCE_PLUGINHOST_AU
 #define   JUCE_PLUGINHOST_AU 0
#endif

//==============================================================================
// juce_core flags:

#ifndef    JUCE_FORCE_DEBUG
 //#define JUCE_FORCE_DEBUG
#endif

#ifndef    JUCE_LOG_ASSERTIONS
 //#define JUCE_LOG_ASSERTIONS
#endif

#ifndef    JUCE_CHECK_MEMORY_LEAKS
 //#define JUCE_CHECK_MEMORY_LEAKS
#endif

#ifndef    JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
 //#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
#endif

#ifndef    JUCE_INCLUDE_ZLIB_CODE
 //#define JUCE_INCLUDE_ZLIB_CODE
#endif

#ifndef    JUCE_USE_CURL
 //#define JUCE_USE_CURL
#endif

//==============================================================================
// juce_graphics flags:

#ifndef    JUCE_USE_COREIMAGE_LOADER
 //#define JUCE_USE_COREIMAGE_LOADER
#endif

#ifndef    JUCE_USE_DIRECTWRITE
 #define   JUCE_USE_DIRECTWRITE 1
#endif

//==============================================================================
// juce_gui_basics flags:

#ifndef    JUCE_ENABLE_REPAINT_DEBUGGING
 //#define JUCE_ENABLE_REPAINT_DEBUGGING
#endif

#ifndef    JUCE_USE_XSHM
 //#define JUCE_USE_XSHM
#endif

#ifndef    JUCE_USE_XRENDER
 //#define JUCE_USE_XRENDER
#endif

#ifndef    JUCE_USE_XCURSOR
 //#define JUCE_USE_XCURSOR
#endif

//==============================================================================
// juce_gui_extra flags:

#ifndef    JUCE_WEB_BROWSER
 //#define JUCE_WEB_BROWSER
#endif

#ifndef    JUCE_ENABLE_LIVE_CONSTANT_EDITOR
 //#define JUCE_ENABLE_LIVE_CONSTANT_EDITOR
#endif


#endif  // __JUCE_APPCONFIG_JUHBUI__
