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

  ID:               hi_dsp
  vendor:           Hart Instruments
  version:          2.0.0
  name:             HISE DSP Module
  description:      The DSP base classes for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_core, hi_dsp_library

END_JUCE_MODULE_DECLARATION

******************************************************************************/


#ifndef HI_DSP_H_INCLUDED
#define HI_DSP_H_INCLUDED

/** @defgroup utility Utility Classes
*
*	A collection of small helper classes.
*/


#include "Processor.h"
#include "ProcessorInterfaces.h"

#include "Routing.h"

#if USE_BACKEND
#include "RoutingEditor.h"
#endif

#if USE_BACKEND

#include "editor/ProcessorEditor.h"
#include "editor/ProcessorEditorHeaderExtras.h"
#include "editor/ProcessorEditorHeader.h"
#include "editor/ProcessorEditorChainBar.h"
#include "editor/ProcessorEditorPanel.h"


#else

// Return type for all empty createEditor methods
class ProcessorEditor;

#endif


/**	@defgroup modulator Modulator classes
*	@ingroup dsp
*
*	Classes related to the modulation architecture of HISE.
*/

#include "modules/Modulators.h"
#include "modules/ModulatorChain.h"

#include "modules/MidiProcessor.h"
#include "modules/MidiPlayer.h"


#include "modules/EffectProcessor.h"
#include "modules/EffectProcessorChain.h"

#include "modules/ModulatorSynth.h"
#include "modules/ModulatorSynthChain.h"
#include "modules/ModulatorSynthGroup.h"

// Plugin Parameters

#include "plugin_parameter/PluginParameterProcessor.h"

/** This defines the tail duration for suspending effects when the input is silent. */
#ifndef HISE_SUSPENSION_TAIL_MS
#define HISE_SUSPENSION_TAIL_MS 500
#endif



#endif  // HI_DSP_H_INCLUDED
