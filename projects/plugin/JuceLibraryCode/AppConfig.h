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

#define JUCE_ENABLE_AUDIO_GUARD 1

// [END_USER_CODE_SECTION]

#include "JucePluginDefines.h"

/*
  ==============================================================================

   In accordance with the terms of the JUCE 6 End-Use License Agreement, the
   JUCE Code in SECTION A cannot be removed, changed or otherwise rendered
   ineffective unless you have a JUCE Indie or Pro license, or are using JUCE
   under the GPL v3 license.

   End User License Agreement: www.juce.com/juce-6-licence

  ==============================================================================
*/

// BEGIN SECTION A

#ifndef JUCE_DISPLAY_SPLASH_SCREEN
 #define JUCE_DISPLAY_SPLASH_SCREEN 0
#endif

// END SECTION A

#define JUCE_USE_DARK_SPLASH_SCREEN 1

#define JUCE_PROJUCER_VERSION 0x60103

//==============================================================================
#define JUCE_MODULE_AVAILABLE_hi_backend                    1
#define JUCE_MODULE_AVAILABLE_hi_core                       1
#define JUCE_MODULE_AVAILABLE_hi_dsp_library                1
#define JUCE_MODULE_AVAILABLE_hi_faust                      1
#define JUCE_MODULE_AVAILABLE_hi_faust_jit                  1
#define JUCE_MODULE_AVAILABLE_hi_faust_lib                  1
#define JUCE_MODULE_AVAILABLE_hi_faust_types                1
#define JUCE_MODULE_AVAILABLE_hi_lac                        1
#define JUCE_MODULE_AVAILABLE_hi_loris                      1
#define JUCE_MODULE_AVAILABLE_hi_rlottie                    1
#define JUCE_MODULE_AVAILABLE_hi_scripting                  1
#define JUCE_MODULE_AVAILABLE_hi_snex                       1
#define JUCE_MODULE_AVAILABLE_hi_streaming                  1
#define JUCE_MODULE_AVAILABLE_hi_tools                      1
#define JUCE_MODULE_AVAILABLE_hi_zstd                       1
#define JUCE_MODULE_AVAILABLE_juce_audio_basics             1
#define JUCE_MODULE_AVAILABLE_juce_audio_devices            1
#define JUCE_MODULE_AVAILABLE_juce_audio_formats            1
#define JUCE_MODULE_AVAILABLE_juce_audio_plugin_client      1
#define JUCE_MODULE_AVAILABLE_juce_audio_processors         1
#define JUCE_MODULE_AVAILABLE_juce_audio_utils              1
#define JUCE_MODULE_AVAILABLE_juce_core                     1
#define JUCE_MODULE_AVAILABLE_juce_cryptography             1
#define JUCE_MODULE_AVAILABLE_juce_data_structures          1
#define JUCE_MODULE_AVAILABLE_juce_dsp                      1
#define JUCE_MODULE_AVAILABLE_juce_events                   1
#define JUCE_MODULE_AVAILABLE_juce_graphics                 1
#define JUCE_MODULE_AVAILABLE_juce_gui_basics               1
#define JUCE_MODULE_AVAILABLE_juce_gui_extra                1
#define JUCE_MODULE_AVAILABLE_juce_opengl                   1
#define JUCE_MODULE_AVAILABLE_juce_osc                      1
#define JUCE_MODULE_AVAILABLE_juce_product_unlocking        1
#define JUCE_MODULE_AVAILABLE_melatonin_blur                1

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1

//==============================================================================
// hi_backend flags:

#ifndef    USE_WORKBENCH_EDITOR
 //#define USE_WORKBENCH_EDITOR 0
#endif

#ifndef    HISE_PAINT_GLOBAL_MOD_CONNECTIONS
 //#define HISE_PAINT_GLOBAL_MOD_CONNECTIONS 0
#endif

//==============================================================================
// hi_core flags:

#ifndef    USE_BACKEND
 #define   USE_BACKEND 1
#endif

#ifndef    USE_FRONTEND
 #define   USE_FRONTEND 0
#endif

#ifndef    USE_RAW_FRONTEND
 #define   USE_RAW_FRONTEND 0
#endif

#ifndef    IS_STANDALONE_APP
 #define   IS_STANDALONE_APP 0
#endif

#ifndef    DONT_CREATE_USER_PRESET_FOLDER
 //#define DONT_CREATE_USER_PRESET_FOLDER 0
#endif

#ifndef    DONT_CREATE_EXPANSIONS_FOLDER
 //#define DONT_CREATE_EXPANSIONS_FOLDER 0
#endif

#ifndef    HISE_OVERWRITE_OLD_USER_PRESETS
 //#define HISE_OVERWRITE_OLD_USER_PRESETS 0
#endif

#ifndef    HISE_BACKEND_AS_FX
 //#define HISE_BACKEND_AS_FX 0
#endif

#ifndef    USE_COPY_PROTECTION
 #define   USE_COPY_PROTECTION 0
#endif

#ifndef    USE_SCRIPT_COPY_PROTECTION
 //#define USE_SCRIPT_COPY_PROTECTION 0
#endif

#ifndef    USE_IPP
 #define   USE_IPP 0
#endif

#ifndef    USE_VDSP_FFT
 //#define USE_VDSP_FFT 1
#endif

#ifndef    FRONTEND_IS_PLUGIN
 //#define FRONTEND_IS_PLUGIN 0
#endif

#ifndef    PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN
 //#define PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN 1
#endif

#ifndef    FORCE_INPUT_CHANNELS
 //#define FORCE_INPUT_CHANNELS 1
#endif

#ifndef    HI_DONT_SEND_ATTRIBUTE_UPDATES
 //#define HI_DONT_SEND_ATTRIBUTE_UPDATES 0
#endif

#ifndef    HISE_DEACTIVATE_OVERLAY
 //#define HISE_DEACTIVATE_OVERLAY 0
#endif

#ifndef    HISE_MIDIFX_PLUGIN
 //#define HISE_MIDIFX_PLUGIN 0
#endif

#ifndef    USE_CUSTOM_FRONTEND_TOOLBAR
 //#define USE_CUSTOM_FRONTEND_TOOLBAR 0
#endif

#ifndef    HI_SUPPORT_MONO_CHANNEL_LAYOUT
 //#define HI_SUPPORT_MONO_CHANNEL_LAYOUT 0
#endif

#ifndef    HI_SUPPORT_MONO_TO_STEREO
 //#define HI_SUPPORT_MONO_TO_STEREO 0
#endif

#ifndef    HI_SUPPORT_FULL_DYNAMICS_HLAC
 #define   HI_SUPPORT_FULL_DYNAMICS_HLAC 1
#endif

#ifndef    IS_STANDALONE_FRONTEND
 #define   IS_STANDALONE_FRONTEND 0
#endif

#ifndef    USE_GLITCH_DETECTION
 //#define USE_GLITCH_DETECTION 0
#endif

#ifndef    ENABLE_PLOTTER
 #define   ENABLE_PLOTTER 1
#endif

#ifndef    HISE_NUM_MACROS
 //#define HISE_NUM_MACROS 1
#endif

#ifndef    ENABLE_SCRIPTING_SAFE_CHECKS
 #define   ENABLE_SCRIPTING_SAFE_CHECKS 1
#endif

#ifndef    CRASH_ON_GLITCH
 //#define CRASH_ON_GLITCH 0
#endif

#ifndef    ENABLE_SCRIPTING_BREAKPOINTS
 #define   ENABLE_SCRIPTING_BREAKPOINTS 1
#endif

#ifndef    HISE_ENABLE_MIDI_INPUT_FOR_FX
 #define   HISE_ENABLE_MIDI_INPUT_FOR_FX 1
#endif

#ifndef    HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE
 //#define HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE 1
#endif

#ifndef    ENABLE_ALL_PEAK_METERS
 #define   ENABLE_ALL_PEAK_METERS 1
#endif

#ifndef    READ_ONLY_FACTORY_PRESETS
 //#define READ_ONLY_FACTORY_PRESETS 0
#endif

#ifndef    CONFIRM_PRESET_OVERWRITE
 //#define CONFIRM_PRESET_OVERWRITE 1
#endif

#ifndef    ENABLE_CONSOLE_OUTPUT
 #define   ENABLE_CONSOLE_OUTPUT 1
#endif

#ifndef    ENABLE_HOST_INFO
 #define   ENABLE_HOST_INFO 1
#endif

#ifndef    HISE_USE_OPENGL_FOR_PLUGIN
 //#define HISE_USE_OPENGL_FOR_PLUGIN 0
#endif

#ifndef    HISE_DEFAULT_OPENGL_VALUE
 //#define HISE_DEFAULT_OPENGL_VALUE 1
#endif

#ifndef    HISE_USE_SYSTEM_APP_DATA_FOLDER
 //#define HISE_USE_SYSTEM_APP_DATA_FOLDER 0
#endif

#ifndef    ENABLE_STARTUP_LOGGER
 //#define ENABLE_STARTUP_LOGGER 0
#endif

#ifndef    HISE_MAX_PROCESSING_BLOCKSIZE
 //#define HISE_MAX_PROCESSING_BLOCKSIZE 1
#endif

#ifndef    ENABLE_CPU_MEASUREMENT
 //#define ENABLE_CPU_MEASUREMENT 1
#endif

#ifndef    USE_HARD_CLIPPER
 //#define USE_HARD_CLIPPER 0
#endif

#ifndef    USE_SPLASH_SCREEN
 //#define USE_SPLASH_SCREEN 0
#endif

#ifndef    HISE_USE_CUSTOM_EXPANSION_TYPE
 //#define HISE_USE_CUSTOM_EXPANSION_TYPE 0
#endif

#ifndef    HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON
 //#define HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON 1
#endif

#ifndef    HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
 //#define HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON 1
#endif

#ifndef    HISE_MACROS_ARE_PLUGIN_PARAMETERS
 //#define HISE_MACROS_ARE_PLUGIN_PARAMETERS 0
#endif

//==============================================================================
// hi_dsp_library flags:

#ifndef    HI_EXPORT_AS_PROJECT_DLL
 //#define HI_EXPORT_AS_PROJECT_DLL 0
#endif

#ifndef    HI_EXPORT_DSP_LIBRARY
 #define   HI_EXPORT_DSP_LIBRARY 0
#endif

#ifndef    IS_STATIC_DSP_LIBRARY
 //#define IS_STATIC_DSP_LIBRARY 1
#endif

#ifndef    HISE_LOG_FILTER_FREQMOD
 //#define HISE_LOG_FILTER_FREQMOD 0
#endif

//==============================================================================
// hi_faust flags:

#ifndef    HISE_INCLUDE_FAUST
 //#define HISE_INCLUDE_FAUST 0
#endif

#ifndef    HISE_FAUST_USE_LLVM_JIT
 //#define HISE_FAUST_USE_LLVM_JIT 1
#endif

#ifndef    HISE_INCLUDE_FAUST_JIT
 //#define HISE_INCLUDE_FAUST_JIT 0
#endif

//==============================================================================
// hi_faust_types flags:

#ifndef    FAUST_NO_WARNING_MESSAGES
 //#define FAUST_NO_WARNING_MESSAGES 0
#endif

//==============================================================================
// hi_lac flags:

#ifndef    HI_ENABLE_LEGACY_CPU_SUPPORT
 //#define HI_ENABLE_LEGACY_CPU_SUPPORT 0
#endif

#ifndef    HLAC_MEASURE_DECODING_PERFORMANCE
 #define   HLAC_MEASURE_DECODING_PERFORMANCE 0
#endif

#ifndef    HLAC_DEBUG_LOG
 #define   HLAC_DEBUG_LOG 0
#endif

#ifndef    HLAC_INCLUDE_TEST_SUITE
 #define   HLAC_INCLUDE_TEST_SUITE 0
#endif

//==============================================================================
// hi_loris flags:

#ifndef    HISE_INCLUDE_LORIS
 #define   HISE_INCLUDE_LORIS 1
#endif

#ifndef    HISE_USE_LORIS_DLL
 //#define HISE_USE_LORIS_DLL 0
#endif

//==============================================================================
// hi_rlottie flags:

#ifndef    HISE_INCLUDE_RLOTTIE
 //#define HISE_INCLUDE_RLOTTIE 1
#endif

#ifndef    HISE_RLOTTIE_DYNAMIC_LIBRARY
 //#define HISE_RLOTTIE_DYNAMIC_LIBRARY 0
#endif

//==============================================================================
// hi_scripting flags:

#ifndef    INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION
 //#define INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION 1
#endif

//==============================================================================
// hi_snex flags:

#ifndef    SNEX_ENABLE_SIMD
 //#define SNEX_ENABLE_SIMD 0
#endif

#ifndef    HISE_INCLUDE_SNEX
 #define   HISE_INCLUDE_SNEX 1
#endif

#ifndef    SNEX_STANDALONE_PLAYGROUND
 //#define SNEX_STANDALONE_PLAYGROUND 0
#endif

#ifndef    SNEX_INCLUDE_MEMORY_ADDRESS_IN_DUMP
 //#define SNEX_INCLUDE_MEMORY_ADDRESS_IN_DUMP 0
#endif

//==============================================================================
// hi_streaming flags:

#ifndef    STANDALONE_STREAMING
 //#define STANDALONE_STREAMING 1
#endif

#ifndef    HISE_SAMPLER_CUBIC_INTERPOLATION
 //#define HISE_SAMPLER_CUBIC_INTERPOLATION 0
#endif

#ifndef    HISE_SAMPLER_ALLOW_RELEASE_START
 //#define HISE_SAMPLER_ALLOW_RELEASE_START 1
#endif

//==============================================================================
// hi_tools flags:

#ifndef    HISE_NO_GUI_TOOLS
 //#define HISE_NO_GUI_TOOLS 0
#endif

#ifndef    HISE_USE_NEW_CODE_EDITOR
 //#define HISE_USE_NEW_CODE_EDITOR 1
#endif

#ifndef    IS_MARKDOWN_EDITOR
 //#define IS_MARKDOWN_EDITOR 0
#endif

#ifndef    HISE_INCLUDE_PITCH_DETECTION
 //#define HISE_INCLUDE_PITCH_DETECTION 1
#endif

#ifndef    HISE_INCLUDE_RT_NEURAL
 //#define HISE_INCLUDE_RT_NEURAL 1
#endif

#ifndef    HISE_USE_EXTENDED_TEMPO_VALUES
 //#define HISE_USE_EXTENDED_TEMPO_VALUES 0
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
 //#define JUCE_USE_ANDROID_OBOE 1
#endif

#ifndef    JUCE_USE_OBOE_STABILIZED_CALLBACK
 //#define JUCE_USE_OBOE_STABILIZED_CALLBACK 0
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
// juce_audio_plugin_client flags:

#ifndef    JUCE_VST3_CAN_REPLACE_VST2
 //#define JUCE_VST3_CAN_REPLACE_VST2 1
#endif

#ifndef    JUCE_FORCE_USE_LEGACY_PARAM_IDS
 //#define JUCE_FORCE_USE_LEGACY_PARAM_IDS 0
#endif

#ifndef    JUCE_FORCE_LEGACY_PARAMETER_AUTOMATION_TYPE
 //#define JUCE_FORCE_LEGACY_PARAMETER_AUTOMATION_TYPE 0
#endif

#ifndef    JUCE_USE_STUDIO_ONE_COMPATIBLE_PARAMETERS
 //#define JUCE_USE_STUDIO_ONE_COMPATIBLE_PARAMETERS 1
#endif

#ifndef    JUCE_AU_WRAPPERS_SAVE_PROGRAM_STATES
 //#define JUCE_AU_WRAPPERS_SAVE_PROGRAM_STATES 0
#endif

#ifndef    JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE
 //#define JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE 0
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

#ifndef    JUCE_CUSTOM_VST3_SDK
 //#define JUCE_CUSTOM_VST3_SDK 0
#endif

//==============================================================================
// juce_audio_utils flags:

#ifndef    JUCE_USE_CDREADER
 //#define JUCE_USE_CDREADER 0
#endif

#ifndef    JUCE_USE_CDBURNER
 //#define JUCE_USE_CDBURNER 0
#endif

//==============================================================================
// juce_core flags:

#ifndef    JUCE_FORCE_DEBUG
 //#define JUCE_FORCE_DEBUG 0
#endif

#ifndef    JUCE_ENABLE_AUDIO_GUARD
 //#define JUCE_ENABLE_AUDIO_GUARD 0
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
 //#define JUCE_USE_CURL 1
#endif

#ifndef    JUCE_LOAD_CURL_SYMBOLS_LAZILY
 #define   JUCE_LOAD_CURL_SYMBOLS_LAZILY 1
#endif

#ifndef    JUCE_CATCH_UNHANDLED_EXCEPTIONS
 //#define JUCE_CATCH_UNHANDLED_EXCEPTIONS 0
#endif

#ifndef    JUCE_ALLOW_STATIC_NULL_VARIABLES
 //#define JUCE_ALLOW_STATIC_NULL_VARIABLES 0
#endif

#ifndef    JUCE_STRICT_REFCOUNTEDPOINTER
 #define   JUCE_STRICT_REFCOUNTEDPOINTER 1
#endif

#ifndef    JUCE_ENABLE_ALLOCATION_HOOKS
 //#define JUCE_ENABLE_ALLOCATION_HOOKS 0
#endif

//==============================================================================
// juce_dsp flags:

#ifndef    JUCE_ASSERTION_FIRFILTER
 //#define JUCE_ASSERTION_FIRFILTER 1
#endif

#ifndef    JUCE_DSP_USE_INTEL_MKL
 //#define JUCE_DSP_USE_INTEL_MKL 0
#endif

#ifndef    JUCE_DSP_USE_SHARED_FFTW
 //#define JUCE_DSP_USE_SHARED_FFTW 0
#endif

#ifndef    JUCE_DSP_USE_STATIC_FFTW
 //#define JUCE_DSP_USE_STATIC_FFTW 0
#endif

#ifndef    JUCE_DSP_ENABLE_SNAP_TO_ZERO
 //#define JUCE_DSP_ENABLE_SNAP_TO_ZERO 1
#endif

//==============================================================================
// juce_events flags:

#ifndef    JUCE_EXECUTE_APP_SUSPEND_ON_BACKGROUND_TASK
 //#define JUCE_EXECUTE_APP_SUSPEND_ON_BACKGROUND_TASK 0
#endif

//==============================================================================
// juce_graphics flags:

#ifndef    JUCE_USE_COREIMAGE_LOADER
 //#define JUCE_USE_COREIMAGE_LOADER 1
#endif

#ifndef    JUCE_USE_DIRECTWRITE
 #define   JUCE_USE_DIRECTWRITE 1
#endif

#ifndef    JUCE_DISABLE_COREGRAPHICS_FONT_SMOOTHING
 //#define JUCE_DISABLE_COREGRAPHICS_FONT_SMOOTHING 0
#endif

//==============================================================================
// juce_gui_basics flags:

#ifndef    JUCE_ENABLE_REPAINT_DEBUGGING
 //#define JUCE_ENABLE_REPAINT_DEBUGGING 0
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

#ifndef    JUCE_WIN_PER_MONITOR_DPI_AWARE
 //#define JUCE_WIN_PER_MONITOR_DPI_AWARE 1
#endif

//==============================================================================
// juce_gui_extra flags:

#ifndef    JUCE_WEB_BROWSER
 //#define JUCE_WEB_BROWSER 1
#endif

#ifndef    JUCE_USE_WIN_WEBVIEW2
 //#define JUCE_USE_WIN_WEBVIEW2 0
#endif

#ifndef    JUCE_ENABLE_LIVE_CONSTANT_EDITOR
 //#define JUCE_ENABLE_LIVE_CONSTANT_EDITOR 0
#endif

//==============================================================================
// juce_product_unlocking flags:

#ifndef    JUCE_USE_BETTER_MACHINE_IDS
 //#define JUCE_USE_BETTER_MACHINE_IDS 0
#endif

//==============================================================================
#ifndef    JUCE_STANDALONE_APPLICATION
 #if defined(JucePlugin_Name) && defined(JucePlugin_Build_Standalone)
  #define  JUCE_STANDALONE_APPLICATION JucePlugin_Build_Standalone
 #else
  #define  JUCE_STANDALONE_APPLICATION 0
 #endif
#endif
