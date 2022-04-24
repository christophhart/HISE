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

/* HI Module */

#include "JuceHeader.h"


#include "scripting/scriptnode/nodes/CodeGenerator.h"
#include "scripting/scriptnode/nodes/NodeContainer.h"
#include "scripting/scriptnode/nodes/NodeContainerTypes.h"
#include "scripting/scriptnode/nodes/NodeWrapper.h"

#include "scripting/scriptnode/nodes/ProcessNodes.h"
#include "scripting/scriptnode/nodes/DspNode.h"

#include "scripting/scriptnode/ui/ParameterSlider.h"

#include "scripting/scriptnode/ui/ModulationSourceComponent.h"



#include "scripting/scriptnode/ui/NodeContainerComponent.h"



#include "scripting/scriptnode/ui/DspNodeComponent.h"
#include "scripting/scriptnode/ui/DspNetworkComponents.h"


#include "scripting/scriptnode/api/Properties.cpp"
#include "scripting/scriptnode/api/DynamicProperty.cpp"
#include "scripting/scriptnode/api/RangeHelpers.cpp"
#include "scripting/scriptnode/api/DspHelpers.cpp"

#include "scripting/scriptnode/api/NodeBase.cpp"
#include "scripting/scriptnode/api/NodeProperty.cpp"



#include "scripting/scriptnode/api/ModulationSourceNode.cpp"
#include "scripting/scriptnode/api/DspNetwork.cpp"

#if USE_BACKEND
#include "scripting/scriptnode/api/TestClasses.cpp"
#endif

#include "scripting/scriptnode/api/StaticNodeWrappers.cpp"

#include "scripting/scriptnode/dynamic_elements/DynamicParameterList.cpp"
#include "scripting/scriptnode/dynamic_elements/DynamicComplexData.cpp"

#if HISE_INCLUDE_SNEX
#include "scripting/scriptnode/snex_nodes/SnexSource.cpp"
#include "scripting/scriptnode/snex_nodes/SnexNode.cpp"
#include "scripting/scriptnode/snex_nodes/SnexShaper.cpp"
#include "scripting/scriptnode/snex_nodes/SnexOscillator.cpp"

#include "scripting/scriptnode/snex_nodes/SnexEnvelope.cpp"
#include "scripting/scriptnode/snex_nodes/SnexDynamicExpression.cpp"
#endif

#include "scripting/scriptnode/snex_nodes/SnexTimer.cpp"
#include "scripting/scriptnode/snex_nodes/SnexMidi.cpp"

#include "scripting/scriptnode/dynamic_elements/DynamicEventNodes.cpp"
#include "scripting/scriptnode/dynamic_elements/DynamicFaderNode.cpp"
#include "scripting/scriptnode/dynamic_elements/DynamicSmootherNode.cpp"
#include "scripting/scriptnode/dynamic_elements/GlobalRoutingNodes.cpp"
#include "scripting/scriptnode/dynamic_elements/DynamicRoutingNodes.cpp"


#include "scripting/scriptnode/nodes/AudioFileNodeBase.cpp"
#include "scripting/scriptnode/nodes/CodeGenerator.cpp"
#include "scripting/scriptnode/nodes/NodeContainer.cpp"
#include "scripting/scriptnode/nodes/NodeContainerTypes.cpp"
#include "scripting/scriptnode/nodes/NodeWrapper.cpp"
#include "scripting/scriptnode/nodes/ProcessNodes.cpp"
#include "scripting/scriptnode/nodes/JitNode.cpp"
#include "scripting/scriptnode/nodes/DspNode.cpp"


#include "scripting/scriptnode/ui/ParameterSlider.cpp"
#include "scripting/scriptnode/ui/PropertyEditor.cpp"
#include "scripting/scriptnode/ui/NodeComponent.cpp"
#include "scripting/scriptnode/ui/ModulationSourceComponent.cpp"
#include "scripting/scriptnode/ui/NodeContainerComponent.cpp"
#include "scripting/scriptnode/ui/FeedbackNodeComponents.cpp"
#include "scripting/scriptnode/ui/DspNodeComponent.cpp"
#include "scripting/scriptnode/ui/DspNetworkComponents.cpp"
#include "scripting/scriptnode/ui/ScriptNodeFloatingTiles.cpp"

