/*

    IMPORTANT! This file is auto-generated each time you save your
    project - if you alter its contents, your changes may be overwritten!

    There's a section below where you can add your own custom code safely, and the
    Projucer will preserve the contents of that block, but the best way to change
    any of these definitions is by using the Projucer's project settings.

    Any commented-out settings will assume their default values.

*/

#pragma once

//==============================================================================
// [BEGIN_USER_CODE_SECTION]

// (You can add your own code in this section, and the Projucer will not overwrite it)

// [END_USER_CODE_SECTION]

/*
  ==============================================================================

   In accordance with the terms of the JUCE 5 End-Use License Agreement, the
   JUCE Code in SECTION A cannot be removed, changed or otherwise rendered
   ineffective unless you have a JUCE Indie or Pro license, or are using JUCE
   under the GPL v3 license.

   End User License Agreement: www.juce.com/juce-5-licence

  ==============================================================================
*/

// BEGIN SECTION A

#ifndef JUCE_DISPLAY_SPLASH_SCREEN
 #define JUCE_DISPLAY_SPLASH_SCREEN 0
#endif

#ifndef JUCE_REPORT_APP_USAGE
 #define JUCE_REPORT_APP_USAGE 0
#endif

// END SECTION A

#define JUCE_USE_DARK_SPLASH_SCREEN 1

//==============================================================================
#define JUCE_MODULE_AVAILABLE_hi_lac                     1
#define JUCE_MODULE_AVAILABLE_hi_snex                    1
#define JUCE_MODULE_AVAILABLE_hi_tools                   1
#define JUCE_MODULE_AVAILABLE_hi_zstd                    1
#define JUCE_MODULE_AVAILABLE_juce_audio_basics          1
#define JUCE_MODULE_AVAILABLE_juce_audio_devices         1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats         1
#define JUCE_MODULE_AVAILABLE_juce_audio_processors      1
#define JUCE_MODULE_AVAILABLE_juce_core                  1
#define JUCE_MODULE_AVAILABLE_juce_cryptography          1
#define JUCE_MODULE_AVAILABLE_juce_data_structures       1
#define JUCE_MODULE_AVAILABLE_juce_events                1
#define JUCE_MODULE_AVAILABLE_juce_graphics              1
#define JUCE_MODULE_AVAILABLE_juce_gui_basics            1
#define JUCE_MODULE_AVAILABLE_juce_gui_extra             1
#define JUCE_MODULE_AVAILABLE_juce_opengl                1

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1

//==============================================================================
// hi_lac flags:

#ifndef    HI_ENABLE_LEGACY_CPU_SUPPORT
 //#define HI_ENABLE_LEGACY_CPU_SUPPORT 0
#endif

#ifndef    HLAC_MEASURE_DECODING_PERFORMANCE
 //#define HLAC_MEASURE_DECODING_PERFORMANCE 0
#endif

#ifndef    HLAC_DEBUG_LOG
 //#define HLAC_DEBUG_LOG 0
#endif

#ifndef    HLAC_INCLUDE_TEST_SUITE
 //#define HLAC_INCLUDE_TEST_SUITE 0
#endif

//==============================================================================
// hi_snex flags:

#ifndef    HISE_INCLUDE_SNEX
 //#define HISE_INCLUDE_SNEX 0
#endif

//==============================================================================
// juce_audio_devices flags:

#ifndef    JUCE_USE_WINRT_MIDI
 //#define JUCE_USE_WINRT_MIDI 0
#endif

#ifndef    JUCE_ASIO
 //#define JUCE_ASIO 0
#endif

#ifndef    JUCE_WASAPI
 //#define JUCE_WASAPI 1
#endif

#ifndef    JUCE_WASAPI_EXCLUSIVE
 //#define JUCE_WASAPI_EXCLUSIVE 0
#endif

#ifndef    JUCE_DIRECTSOUND
 //#define JUCE_DIRECTSOUND 1
#endif

#ifndef    JUCE_ALSA
 //#define JUCE_ALSA 1
#endif

#ifndef    JUCE_JACK
 //#define JUCE_JACK 0
#endif

#ifndef    JUCE_BELA
 //#define JUCE_BELA 0
#endif

#ifndef    JUCE_USE_ANDROID_OBOE
 //#define JUCE_USE_ANDROID_OBOE 0
#endif

#ifndef    JUCE_USE_ANDROID_OPENSLES
 //#define JUCE_USE_ANDROID_OPENSLES 0
#endif

#ifndef    JUCE_DISABLE_AUDIO_MIXING_WITH_OTHER_APPS
 //#define JUCE_DISABLE_AUDIO_MIXING_WITH_OTHER_APPS 0
#endif

//==============================================================================
// juce_audio_formats flags:

#ifndef    JUCE_USE_FLAC
 //#define JUCE_USE_FLAC 1
#endif

#ifndef    JUCE_USE_OGGVORBIS
 //#define JUCE_USE_OGGVORBIS 1
#endif

#ifndef    JUCE_USE_MP3AUDIOFORMAT
 //#define JUCE_USE_MP3AUDIOFORMAT 0
#endif

#ifndef    JUCE_USE_LAME_AUDIO_FORMAT
 //#define JUCE_USE_LAME_AUDIO_FORMAT 0
#endif

#ifndef    JUCE_USE_WINDOWS_MEDIA_FORMAT
 //#define JUCE_USE_WINDOWS_MEDIA_FORMAT 1
#endif

//==============================================================================
// juce_audio_processors flags:

#ifndef    JUCE_PLUGINHOST_VST
 //#define JUCE_PLUGINHOST_VST 0
#endif

#ifndef    JUCE_PLUGINHOST_VST3
 //#define JUCE_PLUGINHOST_VST3 0
#endif

#ifndef    JUCE_PLUGINHOST_AU
 //#define JUCE_PLUGINHOST_AU 0
#endif

#ifndef    JUCE_PLUGINHOST_LADSPA
 //#define JUCE_PLUGINHOST_LADSPA 0
#endif

//==============================================================================
// juce_core flags:

#ifndef    JUCE_FORCE_DEBUG
 //#define JUCE_FORCE_DEBUG 0
#endif

#ifndef    JUCE_LOG_ASSERTIONS
 //#define JUCE_LOG_ASSERTIONS 0
#endif

#ifndef    JUCE_CHECK_MEMORY_LEAKS
 //#define JUCE_CHECK_MEMORY_LEAKS 1
#endif

#ifndef    JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES
 //#define JUCE_DONT_AUTOLINK_TO_WIN32_LIBRARIES 0
#endif

#ifndef    JUCE_INCLUDE_ZLIB_CODE
 //#define JUCE_INCLUDE_ZLIB_CODE 1
#endif

#ifndef    JUCE_USE_CURL
 //#define JUCE_USE_CURL 0
#endif

#ifndef    JUCE_LOAD_CURL_SYMBOLS_LAZILY
 #define   JUCE_LOAD_CURL_SYMBOLS_LAZILY 1
#endif

#ifndef    JUCE_CATCH_UNHANDLED_EXCEPTIONS
 //#define JUCE_CATCH_UNHANDLED_EXCEPTIONS 1
#endif

#ifndef    JUCE_ALLOW_STATIC_NULL_VARIABLES
 //#define JUCE_ALLOW_STATIC_NULL_VARIABLES 1
#endif

#ifndef    JUCE_STRICT_REFCOUNTEDPOINTER
 //#define JUCE_STRICT_REFCOUNTEDPOINTER 0
#endif

#ifndef    JUCE_ENABLE_AUDIO_GUARD
 //#define JUCE_ENABLE_AUDIO_GUARD 0
#endif

//==============================================================================
// juce_events flags:

#ifndef    JUCE_EXECUTE_APP_SUSPEND_ON_IOS_BACKGROUND_TASK
 //#define JUCE_EXECUTE_APP_SUSPEND_ON_IOS_BACKGROUND_TASK 0
#endif

//==============================================================================
// juce_graphics flags:

#ifndef    JUCE_USE_COREIMAGE_LOADER
 //#define JUCE_USE_COREIMAGE_LOADER 1
#endif

#ifndef    JUCE_USE_DIRECTWRITE
 //#define JUCE_USE_DIRECTWRITE 1
#endif

#ifndef    JUCE_DISABLE_COREGRAPHICS_FONT_SMOOTHING
 //#define JUCE_DISABLE_COREGRAPHICS_FONT_SMOOTHING 0
#endif

//==============================================================================
// juce_gui_basics flags:

#ifndef    JUCE_ENABLE_REPAINT_DEBUGGING
 //#define JUCE_ENABLE_REPAINT_DEBUGGING 0
#endif

#ifndef    JUCE_ENABLE_REPAINT_PROFILING
 //#define JUCE_ENABLE_REPAINT_PROFILING 1
#endif

#ifndef    JUCE_USE_XRANDR
 //#define JUCE_USE_XRANDR 1
#endif

#ifndef    JUCE_USE_XINERAMA
 //#define JUCE_USE_XINERAMA 1
#endif

#ifndef    JUCE_USE_XSHM
 //#define JUCE_USE_XSHM 1
#endif

#ifndef    JUCE_USE_XRENDER
 //#define JUCE_USE_XRENDER 0
#endif

#ifndef    JUCE_USE_XCURSOR
 //#define JUCE_USE_XCURSOR 1
#endif

#ifndef    JUCE_HEADLESS_PLUGIN_CLIENT
 //#define JUCE_HEADLESS_PLUGIN_CLIENT 0
#endif

#ifndef    JUCE_WIN_PER_MONITOR_DPI_AWARE
 //#define JUCE_WIN_PER_MONITOR_DPI_AWARE 1
#endif

//==============================================================================
// juce_gui_extra flags:

#ifndef    JUCE_WEB_BROWSER
 //#define JUCE_WEB_BROWSER 1
#endif

#ifndef    JUCE_ENABLE_LIVE_CONSTANT_EDITOR
 //#define JUCE_ENABLE_LIVE_CONSTANT_EDITOR 0
#endif

//==============================================================================
#ifndef    JUCE_STANDALONE_APPLICATION
 #if defined(JucePlugin_Name) && defined(JucePlugin_Build_Standalone)
  #define  JUCE_STANDALONE_APPLICATION JucePlugin_Build_Standalone
 #else
  #define  JUCE_STANDALONE_APPLICATION 1
 #endif
#endif
