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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

/******************************************************************************

BEGIN_JUCE_MODULE_DECLARATION

  ID:               hi_scripting
  vendor:           Hart Instruments
  version:          0.999
  name:             HISE Scripting Module
  description:      The scripting engine module for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, juce_opengl, hi_core, hi_dsp_library

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_SCRIPTING_INCLUDED
#define HI_SCRIPTING_INCLUDED

#define MAX_SCRIPT_HEIGHT 700

#define INCLUDE_NATIVE_JIT 0

#include "AppConfig.h"
#include "../hi_core/hi_core.h"
#include "../hi_dsp_library/hi_dsp_library.h"

#if INCLUDE_NATIVE_JIT
#include "../hi_native_jit/hi_native_jit_public.h"
#endif


#include "scripting/api/ScriptMacroDefinitions.h"
#include "scripting/engine/JavascriptApiClass.h"
#include "scripting/api/ScriptingBaseObjects.h"

#if JUCE_IOS
#else
#include "scripting/api/TccContext.h"
#endif

//#include "scripting/api/DspFactory.h"
#include "scripting/api/DspInstance.h"
#if JUCE_IOS
#else
#include "scripting/api/TccDspObject.h"
#endif
#include "scripting/scripting_audio_processor/ScriptDspModules.h"
#include "scripting/scripting_audio_processor/ScriptedAudioProcessor.h"

#include "scripting/engine/DebugHelpers.h"
#include "scripting/engine/HiseJavascriptEngine.h"

#include "scripting/api/XmlApi.h"
#include "scripting/api/ScriptingApiObjects.h"
#include "scripting/api/ScriptingApi.h"
#include "scripting/api/ScriptingApiContent.h"

#include "scripting/ScriptProcessor.h"
#include "scripting/ScriptProcessorModules.h"
#include "scripting/HardcodedScriptProcessor.h"

#include "scripting/api/ScriptComponentWrappers.h"
#include "scripting/components/ScriptingContentComponent.h"

#if USE_BACKEND

#include "scripting/components/PopupEditors.h"
#include "scripting/components/ScriptingCodeEditor.h"
#include "scripting/components/AutoCompletePopup.h"
#include "scripting/components/ScriptingEditor.h"

#endif 
#endif
