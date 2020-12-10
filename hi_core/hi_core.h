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
  version:          2.0.0
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
#include "../hi_tools/hi_tools.h"

#include <complex>


#ifndef HISE_VERSION
#define HISE_VERSION "2.0.0"
#endif


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

/** Config: USE_COPY_PROTECTION

If true, then the copy protection will be used
*/
#ifndef USE_COPY_PROTECTION
#define USE_COPY_PROTECTION 0
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
#define USE_VDSP_FFT 0
#endif

/** Config: FRONTEND_IS_PLUGIN

If set to 1, the compiled plugin will be a effect (stereo in / out). */
#ifndef FRONTEND_IS_PLUGIN
#define FRONTEND_IS_PLUGIN 0
#endif

/** Config: HISE_MIDIFX_PLUGIN

If set to 1, then the plugin will be a MIDI effect plugin. */
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


#ifndef IS_MARKDOWN_EDITOR
#define IS_MARKDOWN_EDITOR 0
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
If true, then the FX plugin will have a MIDI input and the MIDI processor chain is being processed. */
#ifndef HISE_ENABLE_MIDI_INPUT_FOR_FX
#define HISE_ENABLE_MIDI_INPUT_FOR_FX 0
#endif

/** Config: ENABLE_ALL_PEAK_METERS

Set this to 0 to deactivate peak collection for any other processor than the main synth chain
*/
#ifndef ENABLE_ALL_PEAK_METERS
#define ENABLE_ALL_PEAK_METERS 1
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


#ifndef HISE_USE_OPENGL_FOR_PLUGIN
#define HISE_USE_OPENGL_FOR_PLUGIN 0
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







#endif   // HI_CORE_INCLUDED
