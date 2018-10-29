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
  version:          1.6.0
  name:             HISE DSP Module
  description:      The DSP base classes for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_core, hi_dsp_library

END_JUCE_MODULE_DECLARATION

******************************************************************************/


#ifndef HI_DSP_H_INCLUDED
#define HI_DSP_H_INCLUDED

#include "../hi_core/hi_core.h"
#include "../hi_dsp_library/hi_dsp_library.h"


/** @defgroup utility Utility Classes
*
*	A collection of small helper classes.
*/

/** @defgroup dsp DSP classes
*
*	All classes used for signal processing / sound generation.
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
#include "editor/ProcessorViews.h"

#else

// Return type for all empty createEditor methods
class ProcessorEditor;

#endif


/**	@defgroup modulator Modulator classes
*	@ingroup dsp
*
*	Processors that create a modulation signal that can be used to control parameters of another module.
*/

#include "modules/Modulators.h"
#include "modules/ModulatorChain.h"


/**	@defgroup midiProcessor MidiProcessor classes
*	@ingroup dsp
*
*	Contains all classes related to Midi-Processors
*/

#include "modules/MidiProcessor.h"

/**	@defgroup effect Effect classes
*	@ingroup dsp
*
*	The classes for audio effects in HISE.
*/

#include "modules/EffectProcessor.h"
#include "modules/EffectProcessorChain.h"

/**	@defgroup modulatorSynth Sound Generator classes
*	@ingroup dsp
*
*	Contains all classes related to Modulator-Synthesisers.
*/

#include "modules/ModulatorSynth.h"
#include "modules/ModulatorSynthChain.h"
#include "modules/ModulatorSynthGroup.h"
#include "modules/DspCoreModules.h"

// Plugin Parameters

#include "plugin_parameter/PluginParameterProcessor.h"



#endif  // HI_DSP_H_INCLUDED
