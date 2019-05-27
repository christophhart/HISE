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

#include "scripting/scriptnode/data/Properties.cpp"
#include "scripting/scriptnode/data/RangeHelpers.cpp"
#include "scripting/scriptnode/data/ValueTreeHelpers.cpp"
#include "scripting/scriptnode/data/DspHelpers.cpp"
#include "scripting/scriptnode/data/NodeBase.cpp"

#include "scripting/scriptnode/data/CodeGenerator.cpp"
#include "scripting/scriptnode/data/DspNetwork.cpp"
#include "scripting/scriptnode/data/StaticNodeWrappers.cpp"
#include "scripting/scriptnode/data/ModulationSourceNode.cpp"
#include "scripting/scriptnode/data/NodeContainer.cpp"
#include "scripting/scriptnode/data/NodeWrapper.cpp"
#include "scripting/scriptnode/data/ProcessNodes.cpp"
#include "scripting/scriptnode/data/DspNode.cpp"
#include "scripting/scriptnode/data/FeedbackNode.cpp"

#include "scripting/scriptnode/ui/ParameterSlider.cpp"
#include "scripting/scriptnode/ui/PropertyEditor.cpp"
#include "scripting/scriptnode/ui/NodeComponent.cpp"
#include "scripting/scriptnode/ui/ModulationSourceComponent.cpp"
#include "scripting/scriptnode/ui/NodeContainerComponent.cpp"
#include "scripting/scriptnode/ui/FeedbackNodeComponents.cpp"
#include "scripting/scriptnode/ui/DspNodeComponent.cpp"
#include "scripting/scriptnode/ui/DspNetworkComponents.cpp"