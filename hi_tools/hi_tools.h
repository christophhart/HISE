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

  ID:               hi_tools
  vendor:           Hart Instruments
  version:          1.6.0
  name:             HISE Tools module
  description:      Contains all dependency free general purpose tool classes used in HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_formats, juce_core, juce_graphics,  juce_data_structures, juce_events
  OSXFrameworks:    Accelerate
  iOSFrameworks:    Accelerate

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#pragma once

#include "AppConfig.h"




/** Config: HISE_NO_GUI_TOOLS

	Set this to true to remove some UI code from this module
	This will reduce the build times and compilation size for headless projects.
*/
#ifndef HISE_NO_GUI_TOOLS
#define HISE_NO_GUI_TOOLS 0
#endif


/** Config: HISE_USE_NEW_CODE_EDITOR

	Set this to false in order to use the old code editor for HiseScript files.
	The new editor might be a bit quirky until it's been tested more, so if you are
	on a tight schedule you might want to revert to the old one until the kinks are
	sorted out.
*/
#ifndef HISE_USE_NEW_CODE_EDITOR
#define HISE_USE_NEW_CODE_EDITOR 1
#endif

/** Config: IS_MARKDOWN_EDITOR

	Set this to true if you want to build the markdown editor (it will deactivate
	some code that would require the entire HISE codebase).
*/
#ifndef IS_MARKDOWN_EDITOR
#define IS_MARKDOWN_EDITOR 0
#endif

/** Config: HISE_INCLUDE_PITCH_DETECTION
 
 Includes the pitch detection code. Disable this if you don't have the dsp_library module.
 
*/
#ifndef HISE_INCLUDE_PITCH_DETECTION
#define HISE_INCLUDE_PITCH_DETECTION 1
#endif


/** Config: HISE_ENABLE_LORIS_ON_FRONTEND
 
 Includes the Loris Manager for compiled plugins. Be aware that the Loris library is only licensed under
 the GPLv3 license, so you must not enable this flag for proprietary products!.
 
 */
#ifndef HISE_ENABLE_LORIS_ON_FRONTEND
#define HISE_ENABLE_LORIS_ON_FRONTEND 0
#endif

/** Config: HISE_USE_EXTENDED_TEMPO_VALUES

If this is true, the tempo mode will contain lower values than 1/1. This allows eg. the LFO to run slower, however it 
will break compatibility with older projects / presets because the tempo indexes will change.

*/
#ifndef HISE_USE_EXTENDED_TEMPO_VALUES
#define HISE_USE_EXTENDED_TEMPO_VALUES 0
#endif

/** Reenables using the mouse wheel to control the table curve if set to 1. */
#ifndef HISE_USE_MOUSE_WHEEL_FOR_TABLE_CURVE
#define HISE_USE_MOUSE_WHEEL_FOR_TABLE_CURVE 0
#endif

#include "../JUCE/modules/juce_core/juce_core.h"
#include "../JUCE/modules/juce_audio_basics/juce_audio_basics.h"

#include "../JUCE/modules/juce_graphics/juce_graphics.h"

#include "../JUCE/modules/juce_audio_devices/juce_audio_devices.h"
#include "../JUCE/modules/juce_audio_utils/juce_audio_utils.h"

#include "../JUCE/modules/juce_gui_basics/juce_gui_basics.h"

#if !HISE_NO_GUI_TOOLS
#include "../JUCE/modules/juce_gui_extra/juce_gui_extra.h"
#include "../JUCE/modules/juce_opengl/juce_opengl.h"
#include "../hi_rlottie/hi_rlottie.h"
#endif


#include "../hi_streaming/hi_streaming.h"


#if JUCE_ARM
#include "hi_tools/sse2neon.h"
#endif

#if USE_BACKEND || USE_FRONTEND
#define HI_REMOVE_HISE_DEPENDENCY_FOR_TOOL_CLASSES 0
#else
#define HI_REMOVE_HISE_DEPENDENCY_FOR_TOOL_CLASSES 1
#endif

#ifndef DOUBLE_TO_STRING_DIGITS
#define DOUBLE_TO_STRING_DIGITS 8
#endif

#ifndef HISE_HEADLESS
#define HISE_HEADLESS 0
#endif



#if HISE_HEADLESS
#ifndef ASSERT_STRICT_PROCESSOR_STRUCTURE
#define ASSERT_STRICT_PROCESSOR_STRUCTURE(x) 
#endif
#define IF_NOT_HEADLESS(x) 
#else
#ifndef ASSERT_STRICT_PROCESSOR_STRUCTURE
#define ASSERT_STRICT_PROCESSOR_STRUCTURE(x) jassert(x);
#endif
#define IF_NOT_HEADLESS(x) x
#endif

#ifndef HI_MARKDOWN_ENABLE_INTERACTIVE_CODE
#if USE_BACKEND
#define HI_MARKDOWN_ENABLE_INTERACTIVE_CODE 0
#else
#define HI_MARKDOWN_ENABLE_INTERACTIVE_CODE 0
#endif
#endif


#ifndef HISE_USE_ONLINE_DOC_UPDATER
#define HISE_USE_ONLINE_DOC_UPDATER 0
#endif

/** Set this to 0 if you want to use the old Oxygen font. */
#ifndef USE_LATO_AS_DEFAULT
#define USE_LATO_AS_DEFAULT 1
#endif

#include "hi_binary_data/hi_binary_data.h"


#include "Macros.h"

#include "hi_tools/CustomDataContainers.h"
#include "hi_tools/HiseEventBuffer.h"

#include "hi_tools/UpdateMerger.h"

#include "hi_tools/MiscToolClasses.h"

#include "hi_tools/PathFactory.h"
#include "hi_tools/HI_LookAndFeels.h"

#if USE_BACKEND || HISE_ENABLE_LORIS_ON_FRONTEND
#include "hi_tools/LorisManager.h"
#endif


#include "hi_tools/PitchDetection.h"
#include "hi_tools/VariantBuffer.h"
#include "hi_tools/Tables.h"

#include "hi_tools/ValueTreeHelpers.h"

#if USE_IPP

#include "ipp.h"
#include "hi_tools/IppFFT.h"
#endif

#if !HISE_NO_GUI_TOOLS

#include "gin_images/gin_imageeffects.h"
#include "hi_tools/PostGraphicsRenderer.h"

#include "hi_markdown/MarkdownHeader.h"
#include "hi_markdown/MarkdownLink.h"
#include "hi_markdown/MarkdownDatabase.h"
#include "hi_markdown/MarkdownLayout.h"
#include "hi_markdown/Markdown.h"
#include "hi_markdown/MarkdownDefaultProviders.h"

#include "hi_markdown/MarkdownHtmlExporter.h"
#include "hi_markdown/MarkdownDatabaseCrawler.h"
#include "hi_markdown/MarkdownRenderer.h"



#include "mcl_editor/mcl_editor.h"



#include "hi_tools/JavascriptTokeniser.h"



#include "hi_standalone_components/ChocWebView.h"
#include "hi_standalone_components/CodeEditorApiBase.h"
#include "hi_standalone_components/AdvancedCodeEditor.h"
#include "hi_standalone_components/ScriptWatchTable.h"
#include "hi_standalone_components/ComponentWithPreferredSize.h"
#include "hi_standalone_components/ZoomableViewport.h"
#else
using ComponentWithMiddleMouseDrag = juce::Component;
#define CHECK_MIDDLE_MOUSE_DOWN(e) ignoreUnused(e);
#define CHECK_MIDDLE_MOUSE_UP(e) ignoreUnused(e);
#define CHECK_MIDDLE_MOUSE_DRAG(e) ignoreUnused(e);
#define CHECK_VIEWPORT_SCROLL(e, details) ignoreUnused(e, details);
#endif



#if HISE_INCLUDE_RLOTTIE
#include "hi_standalone_components/RLottieDevComponent.h"
#endif

#include "hi_standalone_components/Plotter.h"

#include "hi_standalone_components/RingBuffer.h"

#include "hi_standalone_components/SliderPack.h"
#include "hi_standalone_components/TableEditor.h"

#include "hi_standalone_components/VuMeter.h"


#include "hi_standalone_components/SampleDisplayComponent.h"


#include "hi_standalone_components/eq_plot/FilterInfo.h"
#include "hi_standalone_components/eq_plot/FilterGraph.h"




