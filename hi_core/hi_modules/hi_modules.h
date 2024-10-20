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

  ID:               hi_modules
  vendor:           Hart Instruments
  version:          2.0.0
  name:             HISE Processor Modules
  description:      All processors for HISE
  website:          http://hise.audio
  license:          GPL / Commercial

  dependencies:      juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_processors, juce_core, juce_cryptography, juce_data_structures, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra, hi_scripting, hi_sampler

END_JUCE_MODULE_DECLARATION

******************************************************************************/

#ifndef HI_MODULES_INCLUDED
#define HI_MODULES_INCLUDED


/** Defines the number of modulation slots to use in the hardcoded master FX. */
#ifndef NUM_HARDCODED_FX_MODS
#define NUM_HARDCODED_FX_MODS 0
#endif

/** Defines the number of modulation slots for polyphonic effects. */
#ifndef NUM_HARDCODED_POLY_FX_MODS
#define NUM_HARDCODED_POLY_FX_MODS 0
#endif

/** This can be used to decrease the block size between modulation values for polyphonic FX modulation. */
#ifndef HARDCODED_POLY_FX_BLOCKSIZE
#if NUM_HARDCODED_POLY_FX_MODS
#define HARDCODED_POLY_FX_BLOCKSIZE 32
#else 
#define HARDCODED_POLY_FX_BLOCKSIZE 1024
#endif
#endif

#include "synthesisers/synths/PolyBlep.h"



#include "synthesisers/synths/WavetableTools.h"
#include "synthesisers/editors/WavetableComponents.h"

#ifndef ENABLE_PEAK_METERS_FOR_GAIN_EFFECT
#define ENABLE_PEAK_METERS_FOR_GAIN_EFFECT 1
#endif



/** @defgroup modulatorTypes HISE Modulators
*	@ingroup types
*
*	A list of all available HISE modulators.
*/

#include "modulators/mods/ConstantModulator.h"
#include "modulators/mods/ControlModulator.h"
#include "modulators/mods/LFOModulator.h"
#include "modulators/mods/MacroControlModulator.h"
#include "modulators/mods/RandomModulator.h"
#include "modulators/mods/SimpleEnvelope.h"
#include "modulators/mods/KeyModulator.h"
#include "modulators/mods/AhdsrEnvelope.h"
#include "modulators/mods/EventDataModulator.h"
#include "modulators/mods/PitchWheelModulator.h"
#include "modulators/mods/TableEnvelope.h"
#include "modulators/mods/VelocityModulator.h"
#include "modulators/mods/GlobalModulators.h"
#include "modulators/mods/ArrayModulator.h"
#include "modulators/mods/MPEModulators.h"
#include "modulators/mods/MPEComponents.h"
#include "modulators/mods/HardcodedNetworkModulators.h"


#if USE_BACKEND

#include "modulators/editors/AhdsrEnvelopeEditor.h"
#include "modulators/editors/ConstantEditor.h"
#include "modulators/editors/ControlEditor.h"
#include "modulators/editors/LfoEditor.h"
#include "modulators/editors/KeyEditor.h"
#include "modulators/editors/MacroControlModulatorEditor.h"
#include "modulators/editors/PitchWheelEditor.h"
#include "modulators/editors/RandomEditor.h"
#include "modulators/editors/SimpleEnvelopeEditor.h"
#include "modulators/editors/TableEnvelopeEditor.h"
#include "modulators/editors/VelocityEditor.h"
#include "modulators/editors/ArrayModulatorEditor.h"
#include "modulators/editors/GlobalModulatorEditor.h"
#include "modulators/editors/MPEModulatorEditors.h"

#endif


/** @defgroup midiTypes HISE MidiProcessors
*	@ingroup types
*
*	A list of all available HISE MIDI processors.
*/

#include "midi_processor/mps/Transposer.h"

#if USE_BACKEND

#include "midi_processor/editors/TransposerEditor.h"
#include "midi_processor/editors/MidiPlayerEditor.h"

#endif

/** @defgroup effectTypes HISE Effects
*	@ingroup types
*
*	A list of all available HISE Effects
*/

#include "effects/fx/RouteFX.h"
#include "effects/fx/FilterTypes.h"
#include "effects/fx/FilterHelpers.h"
#include "effects/fx/Filters.h"
#include "effects/fx/HarmonicFilter.h"
#include "effects/fx/CurveEq.h"
#include "effects/fx/StereoFX.h"
#include "effects/fx/SimpleReverb.h"
#include "effects/fx/Delay.h"
#include "effects/fx/GainEffect.h"
#include "effects/fx/Chorus.h"
#include "effects/fx/Phaser.h"
#include "effects/fx/Convolution.h"
#include "effects/fx/Dynamics.h"
#include "effects/fx/Saturator.h"
#include "effects/fx/Analyser.h"
#include "effects/fx/ShapeFX.h"
#include "effects/fx/SlotFX.h"
#include "effects/fx/HardcodedNetworkEffect.h"


#if USE_BACKEND

#include "effects/editors/FilterEditor.h"
#include "effects/editors/HarmonicFilterEditor.h"
#include "effects/editors/CurveEqEditor.h"
#include "effects/editors/StereoEditor.h"
#include "effects/editors/ReverbEditor.h"
#include "effects/editors/DelayEditor.h"
#include "effects/editors/GainEditor.h"
#include "effects/editors/ChorusEditor.h"
#include "effects/editors/PhaserEditor.h"
#include "effects/editors/ConvolutionEditor.h"
#include "effects/editors/RouteFXEditor.h"
#include "effects/editors/SaturationEditor.h"
#include "effects/editors/DynamicsEditor.h"
#include "effects/editors/ShapeFXEditor.h"
#include "effects/editors/PolyShapeFXEditor.h"
#include "effects/editors/SlotFXEditor.h"
#include "effects/editors/AnalyserEditor.h"

#endif


/** @defgroup synthTypes HISE Sound Generators
*	@ingroup types
*
*	A list of all available HISE sound generators.
*/

#include "synthesisers/synths/GlobalModulatorContainer.h"
#include "synthesisers/synths/MacroModulationSource.h"
#include "synthesisers/synths/SineSynth.h"
#include "synthesisers/synths/NoiseSynth.h"
#include "synthesisers/synths/WaveSynth.h"

#include "synthesisers/synths/WavetableSynth.h"
#include "synthesisers/synths/AudioLooper.h"
#include "synthesisers/synths/HardcodedNetworkSynth.h"

#if USE_BACKEND

#include "synthesisers/editors/SineSynthBody.h"
#include "synthesisers/editors/WaveSynthBody.h"
#include "synthesisers/editors/GroupBody.h"
#include "synthesisers/editors/ModulatorSynthChainBody.h"
#include "synthesisers/editors/WavetableBody.h"
#include "synthesisers/editors/AudioLooperEditor.h"

#endif



#include "raw/raw_ids.h"

#include "raw/raw_misc.h"
#include "raw/raw_misc_impl.h"
#include "raw/raw_builder.h"
#include "raw/raw_builder_impl.h"
#include "raw/raw_main_processor.h"
#include "raw/raw_main_processor_impl.h"
#include "raw/raw_main_editor.h"

#include "raw/raw_positioner.h"
#include "raw/raw_UserPreset.h"
#include "raw/raw_PluginParameter.h"

#endif   // HI_MODULES_INCLUDED
