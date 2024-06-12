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
  version:          4.0.0
  name:             HISE Scripting Module
  description:      The scripting engine module for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_core, hi_dsp_library

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

// Periodically dumps the value tree of a dsp network
#define DUMP_SCRIPTNODE_VALUETREE 1

/** This will determine the timeout duration (in milliseconds) after which a server call will be aborted. */
#ifndef HISE_SCRIPT_SERVER_TIMEOUT
#define HISE_SCRIPT_SERVER_TIMEOUT 10000
#endif

/** This preprocessor will prevent throwing compilation errors when calling a dynamic function with a undefined parameter.
    This is necessary because of a recent change that detects undefined parameters which went unnoticed before.
 
    If you have a big project and you'll experience many of these errors popping up in the latest HISE build, you can enable
    this preprocessor in order to keep the lights on - in that case it will only print a warning to the console but keep executing
    the script so you can go through fixing these on a rainy day.
 
    Be aware that this is just a temporar solution and I'll remove this sometime in the future.
*/
#ifndef HISE_WARN_UNDEFINED_PARAMETER_CALLS
#define HISE_WARN_UNDEFINED_PARAMETER_CALLS 1
#endif

/** If this is set to 1, then a compiled node will also create a DSP network that you can freeze / unfreeze.
 *  This was the default behaviour pre HISE 3.7.0, but it introduced a lot of subtle glitches and bugs just for the ability to toggle between
 *  frozen and unfrozen network. 
 */
#ifndef HISE_CREATE_DSP_NETWORKS_FOR_HARDCODED_NODES
#define HISE_CREATE_DSP_NETWORKS_FOR_HARDCODED_NODES 0
#endif

#define MAX_SCRIPT_HEIGHT 700

#include "AppConfig.h"
#include "../JUCE/modules/juce_osc/juce_osc.h"
#include "../hi_core/hi_core.h"
#include "../hi_dsp_library/hi_dsp_library.h"
#include "../hi_snex/hi_snex.h"
#include "../hi_rlottie/hi_rlottie.h"

#include "scripting/api/ScriptMacroDefinitions.h"
#include "scripting/engine/JavascriptApiClass.h"
#include "scripting/api/ScriptingBaseObjects.h"


#include "scripting/engine/DebugHelpers.h"
#include "scripting/api/DspInstance.h"

#include "scripting/scriptnode/api/RangeHelpers.h"
#include "scripting/scriptnode/api/DynamicProperty.h"
#include "scripting/scriptnode/api/DspHelpers.h"
#include "scripting/scriptnode/api/Properties.h"
#include "scripting/scriptnode/api/NodeBase.h"
#include "scripting/scriptnode/api/DspNetwork.h"

#if USE_BACKEND
#include "scripting/components/ScriptingCodeEditor.h"
#include "scripting/scriptnode/node_library/BackendHostFactory.h"
#if HISE_INCLUDE_SNEX
#include "scripting/scriptnode/api/TestClasses.h"
#endif
#endif

#include "scripting/scriptnode/ui/ScriptNodeFloatingTiles.h"

#include "scripting/engine/HiseJavascriptEngine.h"
#include "scripting/api/ScriptExpansion.h"

#include "scripting/api/XmlApi.h"
#include "scripting/api/ScriptingApiObjects.h"
#include "scripting/api/ScriptTableListModel.h"
#include "scripting/api/ScriptingGraphics.h"

#include "scripting/api/GlobalServer.h"
#include "scripting/api/ScriptingApi.h"
#include "scripting/api/ScriptingApiContent.h"
#include "scripting/api/ScriptComponentEditBroadcaster.h"

#include "scripting/ScriptProcessor.h"
#include "scripting/ScriptProcessorModules.h"
#include "scripting/HardcodedScriptProcessor.h"

#include "scripting/api/ScriptComponentWrappers.h"
#include "scripting/components/ScriptingContentComponent.h"

#include "scripting/scriptnode/dynamic_elements/GlobalRoutingManager.h"

#if USE_BACKEND
#include "scripting/components/ScriptingPanelTypes.h"
#include "scripting/components/PopupEditors.h"
#include "scripting/components/ScriptingContentOverlay.h"
#endif








#include "scripting/scriptnode/api/NodeProperty.h"
#include "scripting/scriptnode/api/ModulationSourceNode.h"
#include "scripting/scriptnode/dynamic_elements/DynamicParameterList.h"
