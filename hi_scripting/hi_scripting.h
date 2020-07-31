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
  version:          2.0.0
  name:             HISE Scripting Module
  description:      The scripting engine module for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_core, hi_dsp, hi_components, hi_dsp_library, hi_sampler

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#pragma once

/** Config: INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION

If this is true, then it will include the bigger multi-template objects in scriptnode in the
compilation process. This will obviously slow down the compilation, so if you're in a tight
compile / debug cycle and don't need all nodes in scriptnode you might want to turn this off during development.
*/
#ifndef INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION
#define INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION 1
#endif


#define INCLUDE_TCC 0

#define MAX_SCRIPT_HEIGHT 700

#define INCLUDE_NATIVE_JIT 0

#include "AppConfig.h"
#include "../hi_sampler/hi_sampler.h"
#include "../hi_dsp_library/hi_dsp_library.h"
#include "../hi_snex/hi_snex.h"

#if INCLUDE_NATIVE_JIT
#include "../hi_native_jit/hi_native_jit_public.h"
#endif



#include "scripting/api/ScriptMacroDefinitions.h"
#include "scripting/engine/JavascriptApiClass.h"
#include "scripting/api/ScriptingBaseObjects.h"

#if JUCE_IOS
#elif INCLUDE_TCC
#include "scripting/api/TccContext.h"
#endif

//#include "scripting/api/DspFactory.h"
#include "scripting/engine/DebugHelpers.h"
#include "scripting/api/DspInstance.h"


#include "scripting/scripting_audio_processor/ScriptDspModules.h"

#include "scripting/scriptnode/api/Properties.h"
#include "scripting/scriptnode/api/RangeHelpers.h"
#include "scripting/scriptnode/api/DspHelpers.h"
#include "scripting/scriptnode/api/NodeBase.h"




#include "scripting/scriptnode/api/DspNetwork.h"


#include "scripting/scriptnode/ui/NodeComponent.h"
#include "scripting/scriptnode/api/Base.h"
#include "scripting/scriptnode/api/Bypass.h"

#if INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION
#include "scripting/scriptnode/api/Containers.h"
#include "scripting/scriptnode/api/Container_Chain.h"
#include "scripting/scriptnode/api/Container_Split.h"
#include "scripting/scriptnode/api/Container_Multi.h"
#endif

#include "scripting/scriptnode/api/Processors.h"

#include "scripting/scriptnode/api/ModulationSourceNode.h"
#include "scripting/scriptnode/api/StaticNodeWrappers.h"



#include "scripting/scriptnode/api/NodeProperty.h"

#include "scripting/scriptnode/soul/soul_includes.h"
#include "scripting/scriptnode/nodes/JitNode.h"

#include "scripting/scriptnode/ui/ScriptNodeFloatingTiles.h"



#include "scripting/engine/HiseJavascriptEngine.h"

#include "scripting/api/XmlApi.h"
#include "scripting/api/ScriptingApiObjects.h"
#include "scripting/api/ScriptExpansion.h"
#include "scripting/api/ScriptingApi.h"
#include "scripting/api/ScriptingApiContent.h"
#include "scripting/api/ScriptComponentEditBroadcaster.h"
#include "scripting/api/CubeApi.h"


#include "scripting/ScriptProcessor.h"
#include "scripting/ScriptProcessorModules.h"
#include "scripting/HardcodedScriptProcessor.h"
#include "scripting/hardcoded_modules/Arpeggiator.h"

#include "scripting/api/ScriptComponentWrappers.h"
#include "scripting/components/ScriptingContentComponent.h"

#include "scripting/scriptnode/nodes/AudioFileNodeBase.h"

#if USE_BACKEND

#include "scripting/components/ScriptingPanelTypes.h"
#include "scripting/components/PopupEditors.h"
#include "scripting/components/ScriptingCodeEditor.h"
#include "scripting/components/AutoCompletePopup.h"
#include "scripting/components/ScriptingContentOverlay.h"
#include "scripting/components/ScriptingEditor.h"

#endif 





