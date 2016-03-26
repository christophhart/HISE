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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#include "hi_dsp.h"


#include "Processor.cpp"

#include "Routing.cpp"



#if USE_BACKEND

#include "RoutingEditor.cpp"

#include "editor/ProcessorEditor.cpp"
#include "editor/ProcessorEditorHeaderExtras.cpp"
#include "editor/ProcessorEditorHeader.cpp"
#include "editor/ProcessorEditorChainBar.cpp"

#include "editor/ProcessorEditorPanel.cpp"
#include "editor/ProcessorViews.cpp"

#endif

#include "modules/Modulators.cpp"
#include "modules/ModulatorChain.cpp"
#include "modules/MidiProcessor.cpp"
#include "modules/EffectProcessor.cpp"
#include "modules/EffectProcessorChain.cpp"
#include "modules/ModulatorSynth.cpp"
#include "modules/ModulatorSynthChain.cpp"

#if INCLUDE_PROTOPLUG
#include "protoplug/hi_protoplug.cpp"
#endif

#include "plugin_parameter/PluginParameterProcessor.cpp"