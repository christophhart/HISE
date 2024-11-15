/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_core
  vendor:           Hart Instruments
  version:          4.1.0
  name:             HISE Core module
  description:      The core classes for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_lac
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_CORE_INCLUDED
#define HI_CORE_INCLUDED



#include "AppConfig.h"

#ifndef DOUBLE_TO_STRING_DIGITS
#define DOUBLE_TO_STRING_DIGITS 8
#endif


#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"
#include "../JUCE/modules/juce_gui_basics/juce_gui_basics.h"
#include "../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../JUCE/modules/juce_audio_utils/juce_audio_utils.h"
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"
#include "../JUCE/modules/juce_product_unlocking/juce_product_unlocking.h"
#include "../JUCE/modules/juce_dsp/juce_dsp.h"
#include "../hi_zstd/hi_zstd.h"
#include "../hi_dsp_library/hi_dsp_library.h"
#include "../hi_rlottie/hi_rlottie.h"

#include <complex>


//=============================================================================
/** Config: USE_BACKEND

If true, then the plugin uses the backend system including IDE editor & stuff.
*/
#ifndef USE_BACKEND
#define USE_BACKEND 0
#endif

/** Config: USE_FRONTEND

If true, this project uses the frontend module and some special file reference handling.
*/
#ifndef USE_FRONTEND
#define USE_FRONTEND 1
#endif

/** Config: USE_RAW_FRONTEND

If true, this project will not use the embedded module architecture and the script UI.
Use this to create the HISE project with C++ only. */
#ifndef USE_RAW_FRONTEND
#define USE_RAW_FRONTEND 0
#endif

/** Config: IS_STANDALONE_APP

If true, then this will use some additional features for the standalone app (popup out windows, audio device settings etc.)
*/
#ifndef IS_STANDALONE_APP
#define IS_STANDALONE_APP 0
#endif

/** Config: DONT_CREATE_USER_PRESET_FOLDER

Set this to 1 to disable the creation of the User Presets folder at init (i.e. for non-audio related app)
*/
#ifndef DONT_CREATE_USER_PRESET_FOLDER
#define DONT_CREATE_USER_PRESET_FOLDER 0
#endif

/** Config: DONT_CREATE_EXPANSIONS_FOLDER

Set this to 1 to disable the creation of the Expansions folder at init (i.e. for non-audio related app)
*/
#ifndef DONT_CREATE_EXPANSIONS_FOLDER
#define DONT_CREATE_EXPANSIONS_FOLDER 0
#endif

/** Config: HISE_OVERWRITE_OLD_USER_PRESETS

If true, then the plugin will silently overwrite user presets with the same name but an older version number. This will also overwrite user-modified factory presets
but will not modify or delete user-created user presets (with the exception of a name collision).
*/
#ifndef HISE_OVERWRITE_OLD_USER_PRESETS
#define HISE_OVERWRITE_OLD_USER_PRESETS 0
#endif

/** Config: HISE_BACKEND_AS_FX
 
 Set this to 1 in order to use HISE as a effect plugin. This will simulate the processing setup of an FX plugin (so child sound generators will not be processed etc).
*/
#ifndef HISE_BACKEND_AS_FX
#define HISE_BACKEND_AS_FX 0
#endif

/** Config: USE_COPY_PROTECTION

If true, then the copy protection will be used
*/
#ifndef USE_COPY_PROTECTION
#define USE_COPY_PROTECTION 0
#endif

/** Config: USE_SCRIPT_COPY_PROTECTION

	Uses the scripted layer to the JUCE unlock class for copy protection
*/
#ifndef USE_SCRIPT_COPY_PROTECTION
#define USE_SCRIPT_COPY_PROTECTION 0
#endif

// Ensure that USE_COPY_PROTECTION is true when the USE_SCRIPT_COPY_PROTECTION macro is being used
#if USE_SCRIPT_COPY_PROTECTION && !USE_COPY_PROTECTION
#undef USE_COPY_PROTECTION
#define USE_COPY_PROTECTION 1
#endif

/** Config: USE_IPP

Use the Intel Performance Primitives Library for the convolution reverb.
*/
#ifndef USE_IPP
#define USE_IPP 1
#endif

/** Config: USE_VDSP_FFT
*
* Use the vDsp FFT on Apple devices.
*/
#ifndef USE_VDSP_FFT
#define USE_VDSP_FFT JUCE_MAC
#endif

/** Config: FRONTEND_IS_PLUGIN

If set to 1, the compiled plugin will be a effect (stereo in / out).
*/
#ifndef FRONTEND_IS_PLUGIN
#if USE_BACKEND
#define FRONTEND_IS_PLUGIN HISE_BACKEND_AS_FX
#else
#define FRONTEND_IS_PLUGIN 0
#endif
#endif


/** Config: PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN

If set to 1, then the FX plugin will also process child sound generators (eg. global modulators or macro modulation sources). 
*/
#ifndef PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN
#define PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN 1
#endif

/** Config: FORCE_INPUT_CHANNELS

If set to 1, the compiled plugin will use a stereo input channel pair and render the master containers effect chain on top of it.
This can be used to simulate an audio effect routing setup (when the appropriate plugin type is selected in the projucer settings).

*/
#ifndef FORCE_INPUT_CHANNELS
#define FORCE_INPUT_CHANNELS USE_BACKEND
#endif

/** Config: HI_DONT_SEND_ATTRIBUTE_UPDATES
 
If enabled, this will skip the internal UI message update when calling setAttribute from a scripting callback. If you're calling
 this method a lot, setting this to true might help with certain stability issues and UI message clogging.
*/
#ifndef HI_DONT_SEND_ATTRIBUTE_UPDATES
#define HI_DONT_SEND_ATTRIBUTE_UPDATES 0
#endif

/** Config: HISE_DEACTIVATE_OVERLAY
	If enabled, this will deactivate the dark overlay that shows error messages so you
	can define your own thing.
*/
#ifndef HISE_DEACTIVATE_OVERLAY
#define HISE_DEACTIVATE_OVERLAY 0
#endif

/** Config: HISE_MIDIFX_PLUGIN

If set to 1, then the plugin will be a MIDI effect plugin.
*/
#ifndef HISE_MIDIFX_PLUGIN
#define HISE_MIDIFX_PLUGIN 0
#endif

/** Config: USE_CUSTOM_FRONTEND_TOOLBAR

If set to 1, you can specify a customized toolbar class which will be used instead of the default one. 
*/
#ifndef USE_CUSTOM_FRONTEND_TOOLBAR
#define USE_CUSTOM_FRONTEND_TOOLBAR 0
#endif

/** Config: HI_SUPPORT_MONO_CHANNEL_LAYOUT

If enabled, the plugin will also be compatible to mono track configurations. 
*/
#ifndef HI_SUPPORT_MONO_CHANNEL_LAYOUT
#define HI_SUPPORT_MONO_CHANNEL_LAYOUT 0
#endif

/** Config: HI_SUPPORT_MONO_TO_STEREO

If enabled, the plugin will accept mono input channels for stereo processing. 
*/
#ifndef HI_SUPPORT_MONO_TO_STEREO
#define HI_SUPPORT_MONO_TO_STEREO 0
#endif

/** Config: HI_SUPPORT_FULL_DYNAMICS_HLAC

If enabled, the sample extraction dialog will show the option "Full Dynamics" when extracting the sample files. 
*/
#ifndef HI_SUPPORT_FULL_DYNAMICS_HLAC
#define HI_SUPPORT_FULL_DYNAMICS_HLAC 0
#endif

/** Config: IS_STANDALONE_FRONTEND

If set to 1, you can specify a customized toolbar class which will be used instead of the default one. 
*/
#ifndef IS_STANDALONE_FRONTEND
#define IS_STANDALONE_FRONTEND 0
#endif

/** Config: USE_GLITCH_DETECTION

Enable this to add a glitch detector to some performance crititcal functions
*/
#ifndef USE_GLITCH_DETECTION
#define USE_GLITCH_DETECTION 0
#endif

/** Config: ENABLE_PLOTTER

Set this to 0 to deactivate the plotter data collection
*/
#ifndef ENABLE_PLOTTER
#define ENABLE_PLOTTER 1
#endif

/** Config: HISE_NUM_MACROS

Set this to the number of macros you want in your project. */
#ifndef HISE_NUM_MACROS
#define HISE_NUM_MACROS 8
#endif

/** Config: ENABLE_SCRIPTING_SAFE_CHECKS

Set this to 0 to deactivate the safe checks for scripting
*/
#ifndef ENABLE_SCRIPTING_SAFE_CHECKS
#define ENABLE_SCRIPTING_SAFE_CHECKS 1
#endif

/** Config: CRASH_ON_GLITCH
 
If this is set to 1, the application will crash instantly if there is a drop out or a burst in the signal (values above 32dB = +36dB ). Use this to get a crash dump with the location.
*/
#ifndef CRASH_ON_GLITCH
#define CRASH_ON_GLITCH 0
#endif


/** Config: ENABLE_SCRIPTING_BREAKPOINTS

*/
#ifndef ENABLE_SCRIPTING_BREAKPOINTS
#define ENABLE_SCRIPTING_BREAKPOINTS 0
#endif


/** Config: HISE_ENABLE_MIDI_INPUT_FOR_FX
If true, then the FX plugin will have a MIDI input and the MIDI processor chain is being processed.
*/
#ifndef HISE_ENABLE_MIDI_INPUT_FOR_FX
#define HISE_ENABLE_MIDI_INPUT_FOR_FX 0
#endif

/** Config: HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE

If true then the plugin will complain about the buffer size not being a multiple of HISE_EVENT_RASTER. 
*/
#ifndef HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE
#define HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE 1
#endif

/** Config: ENABLE_ALL_PEAK_METERS

Set this to 0 to deactivate peak collection for any other processor than the main synth chain
*/
#ifndef ENABLE_ALL_PEAK_METERS
#define ENABLE_ALL_PEAK_METERS 1
#endif

/** Config: READ_ONLY_FACTORY_PRESETS 

Set this to 1 to enable read only presets that are shipped with the plugin / expansion.
*/
#ifndef READ_ONLY_FACTORY_PRESETS
#define READ_ONLY_FACTORY_PRESETS 0
#endif

/** Config: CONFIRM_PRESET_OVERWRITE

Set this to 0 to disable preset overwriting confirmation popup. The preset will be overwritten automatically.
*/
#ifndef CONFIRM_PRESET_OVERWRITE
#define CONFIRM_PRESET_OVERWRITE 1
#endif

/** Config: ENABLE_CONSOLE_OUTPUT

Set this to 0 to deactivate the console output
*/
#ifndef ENABLE_CONSOLE_OUTPUT
#define ENABLE_CONSOLE_OUTPUT 1
#endif

/** Config: ENABLE_HOST_INFO

Set this to 0 to disable host information like tempo, playing position etc...
*/
#ifndef ENABLE_HOST_INFO
#define ENABLE_HOST_INFO 1
#endif

/** Config: HISE_USE_OPENGL_FOR_PLUGIN
 
 Enables OpenGL support for your project.
 */
#ifndef HISE_USE_OPENGL_FOR_PLUGIN
#define HISE_USE_OPENGL_FOR_PLUGIN 0
#endif

/** Config: HISE_DEFAULT_OPENGL_VALUE
 
 If HISE_USE_OPENGL_FOR_PLUGIN is enabled, this can be used to specify whether
OpenGL should be enabled by default or not.
  
*/
#ifndef HISE_DEFAULT_OPENGL_VALUE
#define HISE_DEFAULT_OPENGL_VALUE 1
#endif

/** Config: HISE_USE_SYSTEM_APP_DATA_FOLDER

    If enabled, the compiled plugin will use the global app data folder instead of the local one.
    This flag will be set automatically based on the project setting. In HISE this must not be changed
    as the app data directory will be checked dynamically using this setting value.
*/
#ifndef HISE_USE_SYSTEM_APP_DATA_FOLDER
#define HISE_USE_SYSTEM_APP_DATA_FOLDER 0
#endif

/** Config: ENABLE_STARTUP_LOGGER

If this is enabled, compiled plugins will write a startup log to the desktop for debugging purposes
*/
#ifndef ENABLE_STARTUP_LOG
#define ENABLE_STARTUP_LOG 0
#endif

/** Config: HISE_MAX_PROCESSING_BLOCKSIZE

This is the maximum block size that is used for the audio rendering. If the host calls the render
callback with a bigger blocksize, it will be split up internally into chunks of the given size.

Usually a bigger block size means less CPU usage, however there is a break even point where a bigger buffer
size stops being helpful and starts wasting memory because of the internal buffer allocations (and causing
some weird side effects in the streaming engine).
*/
#ifndef HISE_MAX_PROCESSING_BLOCKSIZE
#define HISE_MAX_PROCESSING_BLOCKSIZE 512
#endif

/** Config: ENABLE_CPU_MEASUREMENT

Set this to 0 to deactivate the CPU peak meter.
*/
#ifndef ENABLE_CPU_MEASUREMENT
#define ENABLE_CPU_MEASUREMENT 1
#endif


#ifndef ENABLE_APPLE_SANDBOX
#define ENABLE_APPLE_SANDBOX 0
#endif

/** Config: USE_HARD_CLIPPER

Set this to 1 to enable hard clipping of the output (brickwall everything over 1.0)
*/
#ifndef USE_HARD_CLIPPER
#define USE_HARD_CLIPPER 0
#endif

/** Config: USE_SPLASH_SCREEN

If your project contains a SplashScreen.png image file, it will use this as splash screen while loading the instrument in the background.
*/
#ifndef USE_SPLASH_SCREEN
#define USE_SPLASH_SCREEN 0
#endif

/** Config: HISE_USE_CUSTOM_EXPANSION_TYPE

If your project uses a custom C++ implementation for the expansion system, set this to 1 and implement ExpansionHandler::createCustomExpansion().
*/
#ifndef HISE_USE_CUSTOM_EXPANSION_TYPE
#define HISE_USE_CUSTOM_EXPANSION_TYPE 0
#endif


/** Config: HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON

Set this to false to disable the install samples button. This might be useful if you don't use the
HR1 resource file system.
*/
#ifndef HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON
#define HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON 1
#endif

/** Config: HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON

Set this to false to not give the user the ability to set the sample location on first launch. It will use the default location in the user doc folder on windows or the music folder on macOS / Linux until the user changed the location in the settings manually.
*/
#ifndef HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON
#define HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON 1
#endif

/** Config: HISE_MACROS_ARE_PLUGIN_PARAMETERS

If enabled, the plugin will ignore any plugin parameter definitions from the script components (or custom automation data) and will only propagate the macros as plugin parameters
(Note: You might want to define HISE_NUM_MACROS along with the plugin parameters to ensure that it will only use as much parameters as you want).

 */
#ifndef HISE_MACROS_ARE_PLUGIN_PARAMETERS
#define HISE_MACROS_ARE_PLUGIN_PARAMETERS 0
#endif

#ifndef HISE_INCLUDE_BEATPORT
#define HISE_INCLUDE_BEATPORT 0
#endif

// for iOS apps, the external files don't need to be embedded. Enable this to simulate this behaviour on desktop projects (not recommended for production)
//#define DONT_EMBED_FILES_IN_FRONTEND 1

#if JUCE_IOS
#ifndef DONT_EMBED_FILES_IN_FRONTEND
#define DONT_EMBED_FILES_IN_FRONTEND 1
#endif

#ifndef HISE_IOS
#define HISE_IOS 1
#endif
#endif

/** This flag is set when the build configuration is CI (mild optimisation, no debug symbols). */
#ifndef HISE_CI
#define HISE_CI 0
#endif

/**Appconfig file

Use this file to enable the modules that are needed

For all defined variables:

- 1 if the module is used
- 0 if the module should not be used

*/


/** Add new subgroups here and in hi_module.cpp
*
*	New files must be added in the specific subfolder header / .cpp file.
*/


#if USE_IPP
#include "ipp.h"
#endif





#include "LibConfig.h"
#include "Macros.h"

#include "additional_libraries/additional_libraries.h"

#include "hi_core/hi_core.h" // has its own namespace definition

#include "hi_dsp/hi_dsp.h"
#include "hi_components/hi_components.h"
#include "hi_sampler/hi_sampler.h"
#include "hi_modules/hi_modules.h"






#endif   // HI_CORE_INCLUDED
