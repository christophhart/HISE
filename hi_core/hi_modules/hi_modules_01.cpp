#if JUCE_MSVC
#pragma warning (push)
#pragma warning (disable: 4127 4706)
#endif

#include "synthesisers/synths/PolyBlep.cpp"

#if JUCE_MSVC
#pragma warning (pop)
#endif

#include "modulators/mods/ConstantModulator.cpp"
#include "modulators/mods/ControlModulator.cpp"
#include "modulators/mods/LFOModulator.cpp"
#include "modulators/mods/MacroControlModulator.cpp"
#include "modulators/mods/RandomModulator.cpp"
#include "modulators/mods/SimpleEnvelope.cpp"
#include "modulators/mods/KeyModulator.cpp"
#include "modulators/mods/AhdsrEnvelope.cpp"
#include "modulators/mods/PitchWheelModulator.cpp"
#include "modulators/mods/TableEnvelope.cpp"
#include "modulators/mods/VelocityModulator.cpp"
#include "modulators/mods/ArrayModulator.cpp"
#include "modulators/mods/GlobalModulators.cpp"
#include "modulators/mods/MPEModulators.cpp"
#include "modulators/mods/MPEComponents.cpp"
#include "modulators/mods/HardcodedNetworkModulators.cpp"

#if USE_BACKEND

#include "modulators/editors/AhdsrEnvelopeEditor.cpp"
#include "modulators/editors/ConstantEditor.cpp"
#include "modulators/editors/ControlEditor.cpp"
#include "modulators/editors/LfoEditor.cpp"
#include "modulators/editors/KeyEditor.cpp"
#include "modulators/editors/MacroControlModulatorEditor.cpp"
#include "modulators/editors/PitchWheelEditor.cpp"
#include "modulators/editors/RandomEditor.cpp"
#include "modulators/editors/SimpleEnvelopeEditor.cpp"
#include "modulators/editors/TableEnvelopeEditor.cpp"
#include "modulators/editors/VelocityEditor.cpp"
#include "modulators/editors/ArrayModulatorEditor.cpp"
#include "modulators/editors/GlobalModulatorEditor.cpp"
#include "modulators/editors/MPEModulatorEditors.cpp"

#endif

#include "midi_processor/mps/Transposer.cpp"


#if USE_BACKEND

#include "midi_processor/editors/TransposerEditor.cpp"
#include "midi_processor/editors/MidiPlayerEditor.cpp"

#endif

/** @defgroup effectTypes HISE Effects
*	@ingroup dsp
*
*	A list of all available HISE Effects
*/

#include "effects/MdaEffectWrapper.cpp"

#include "effects/fx/RouteFX.cpp"
#include "effects/fx/FilterTypes.cpp"
#include "effects/fx/FilterHelpers.cpp"
#include "effects/fx/Filters.cpp"
#include "effects/fx/HarmonicFilter.cpp"
#include "effects/fx/CurveEq.cpp"
#include "effects/fx/StereoFX.cpp"
#include "effects/fx/SimpleReverb.cpp"
#include "effects/fx/Delay.cpp"
#include "effects/fx/GainEffect.cpp"
#include "effects/fx/Chorus.cpp"
#include "effects/fx/Phaser.cpp"
#include "effects/fx/Convolution.cpp"
#include "effects/mda/mdaLimiter.cpp"
#include "effects/mda/mdaDegrade.cpp"
#include "effects/fx/Dynamics.cpp"
#include "effects/fx/Saturator.cpp"
#include "effects/fx/SlotFX.cpp"
#include "effects/fx/Analyser.cpp"
#include "effects/fx/WaveShapers.cpp"
#include "effects/fx/ShapeFX.cpp"
#include "effects/fx/HardcodedNetworkEffect.cpp"

#if USE_BACKEND

#include "effects/editors/FilterEditor.cpp"
#include "effects/editors/HarmonicFilterEditor.cpp"
#include "effects/editors/CurveEqEditor.cpp"
#include "effects/editors/StereoEditor.cpp"
#include "effects/editors/ReverbEditor.cpp"
#include "effects/editors/DelayEditor.cpp"
#include "effects/editors/GainEditor.cpp"
#include "effects/editors/ChorusEditor.cpp"
#include "effects/editors/PhaserEditor.cpp"
#include "effects/editors/ConvolutionEditor.cpp"
#include "effects/editors/MdaLimiterEditor.cpp"
#include "effects/editors/MdaDegradeEditor.cpp"
#include "effects/editors/RouteFXEditor.cpp"
#include "effects/editors/SaturationEditor.cpp"
#include "effects/editors/DynamicsEditor.cpp"

#include "effects/editors/SlotFXEditor.cpp"
#include "effects/editors/AnalyserEditor.cpp"
#include "effects/editors/ShapeFXEditor.cpp"
#include "effects/editors/PolyShapeFXEditor.cpp"

#endif

#include "synthesisers/synths/GlobalModulatorContainer.cpp"
#include "synthesisers/synths/MacroModulationSource.cpp"
#include "synthesisers/synths/SineSynth.cpp"
#include "synthesisers/synths/NoiseSynth.cpp"
#include "synthesisers/synths/WaveSynth.cpp"
#include "synthesisers/synths/WavetableTools.cpp"
#include "synthesisers/editors/WavetableComponents.cpp"
#include "synthesisers/synths/WavetableSynth.cpp"
#include "synthesisers/synths/AudioLooper.cpp"
#include "synthesisers/synths/HardcodedNetworkSynth.cpp"

#if USE_BACKEND

#include "synthesisers/editors/SineSynthBody.cpp"
#include "synthesisers/editors/WaveSynthBody.cpp"
#include "synthesisers/editors/GroupBody.cpp"
#include "synthesisers/editors/ModulatorSynthChainBody.cpp"
#include "synthesisers/editors/WavetableBody.cpp"
#include "synthesisers/editors/AudioLooperEditor.cpp"

#endif

#include "raw/raw_misc.cpp"
#include "raw/raw_builder.cpp"
#include "raw/raw_positioner.cpp"
#include "raw/raw_main_processor.cpp"
#include "raw/raw_main_editor.cpp"
#include "raw/raw_UserPreset.cpp"
#include "raw/raw_PluginParameter.cpp"

