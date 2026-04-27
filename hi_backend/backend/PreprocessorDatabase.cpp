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

// Some third-party integration macros are only defined when the matching SDK
// is present, so we guard them here so that .withValue(MACRO) still compiles
// when the SDK is not wired up.
#ifndef HISE_INCLUDE_MUSEHUB
#define HISE_INCLUDE_MUSEHUB 0
#endif

#ifndef AUDIOFFT_FFTW3
#define AUDIOFFT_FFTW3 0
#endif

#ifndef DONT_EMBED_FILES_IN_FRONTEND
#define DONT_EMBED_FILES_IN_FRONTEND 0
#endif

#ifndef HISE_ENABLE_EXPANSIONS
#define HISE_ENABLE_EXPANSIONS 1
#endif

#ifndef HISE_ALLOW_OFFLINE_ACTIVATION
#define HISE_ALLOW_OFFLINE_ACTIVATION 0
#endif

#ifndef JUCE_ALLOW_EXTERNAL_UNLOCK
#define JUCE_ALLOW_EXTERNAL_UNLOCK 0
#endif

#ifndef USE_MIDI_AUTOMATION_MIGRATION
#define USE_MIDI_AUTOMATION_MIGRATION 0
#endif

namespace hise {
using namespace juce;

/*

Unprocessed Preprocessors:

UNPROCESSED_BEGIN



UNPROCESSED_END

*/

PreprocessorDataBase::PreprocessorDataBase()
{
	data["HISE_USE_EXTENDED_TEMPO_VALUES"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Adds longer tempo divisions (up to eight bars) to every tempo-synced parameter in the project.")
		.withDescriptionLine("The built-in tempo list that feeds every TempoSync slider, the arpeggiator's Tempo parameter and the `Engine.getTempoInMilliSeconds` / `TransportHandler` scripting APIs only goes up to a whole note by default. Enabling this flag prepends five longer values (EightBar, SixBar, FourBar, ThreeBar, TwoBars) to the front of the list, which is useful for slowly evolving ambient patches or long arpeggiator phrases.")
		.withDescriptionLine("> Turning this on shifts every existing tempo index by 5, so any user preset that stored a tempo as an integer index before the change will map to a different note value after recompiling. The flag must be set consistently for both the HISE build and the exported plugin, otherwise stored indices will not round-trip.")
		.withDefault(0)
		.withValue(HISE_USE_EXTENDED_TEMPO_VALUES)
		.withCrossReference(LinkType::Module, "Arpeggiator", "exposes the extended divisions on its Tempo parameter")
		.withCrossReference(LinkType::ScriptingApi, "Engine", "tempoIndex arguments on getTempoInMilliSeconds and related methods shift by five")
		.withCrossReference(LinkType::ScriptingApi, "TransportHandler", "beat-grid callbacks use the extended tempo enum");

	data["USE_MOD2_WAVETABLESIZE"] = Entry()
		.withCategory(Category::DspAndFilters)
		.withBrief("Fast-path wavetable playback that requires power-of-two cycle lengths in every .hwt file.")
		.withDescriptionLine("When enabled, the wavetable synthesiser indexes its buffer with a bitmask wrap instead of a modulo and a branch, which is the hot inner loop during every voice render and noticeably cheaper on the audio thread. Loading a wavetable whose cycle length is not a power of two logs an error and plays back incorrectly. Since HISE 3.5 the wavetable converter produces power-of-two cycles by default, so this only bites for projects that still ship legacy .hwt files.")
		.withDescriptionLine("> Disable (set to 0) only if you need to keep playing pre-3.5 wavetables and can't reconvert them. The slower inner loop is the price of that compatibility.")
		.withDefault(1)
		.withValue(USE_MOD2_WAVETABLESIZE)
		.withCrossReference(LinkType::Module, "WavetableSynth", "requires every wavetable cycle length to be a power of two")
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_LORIS", "Loris-based cycle extraction produces power-of-two wavetables that rely on this fast path");

	data["USE_RELATIVE_PATH_FOR_AUDIO_FILES"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Store non-streaming audio file references in presets as paths relative to an installed AudioFiles folder.")
		.withDescriptionLine("When enabled, the exported plugin stores and resolves audio file references using the `{AUDIO_FILES}` wildcard against an AudioFiles subfolder inside the plugin's AppData directory, and the folder is created automatically if it does not exist yet. When disabled, user presets store the full absolute path from the machine that saved them, which breaks on any other computer.")
		.withDescriptionLine("> Only affects exported plugins. The HISE IDE always resolves audio files against the current project folder. Turn this off only if you deliberately manage the audio file location in HISEScript and don't want the AppData subfolder to be created.")
		.withDefault(1)
		.withValue(USE_RELATIVE_PATH_FOR_AUDIO_FILES)
		.withCrossReference(LinkType::Preprocessor, "DONT_EMBED_FILES_IN_FRONTEND", "companion flag that skips embedding of external asset files entirely")
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_SYSTEM_APP_DATA_FOLDER", "selects whether the AppData root is the per-user or the system-wide folder");

	data["INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Historical switch for excluding large scriptnode template instantiations from the build.")
		.withDescriptionLine("The original purpose was to skip the bigger multi-template scriptnode objects during compilation so that iterative debug builds finished faster. The macro is still defined but no code reads it anywhere, so setting it has no effect on the build output or the runtime. It's kept around to avoid breaking user projects that still list it in their ExtraDefinitions.")
		.withDefault(1)
		.withValue(INCLUDE_BIG_SCRIPTNODE_OBJECT_COMPILATION)
		.withVestigal();

	data["PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN"] = Entry()
		.withCategory(Category::PluginType)
		.withBrief("Keeps child sound generators running inside an exported FX plugin.")
		.withDescriptionLine("Only relevant when the project is exported as an effect plugin. If enabled, every sound generator in the master chain is still rendered so that global modulators, macro modulation sources, LFOs and envelopes that feed modulation slots on the effect chain keep producing their values. If disabled, only the effect chain itself is processed which saves a bit of CPU but breaks any modulation that originates from a sound generator.")
		.withDescriptionLine("> The HISE export dialog writes this flag automatically based on the 'Process Sound Generators in FX Plugin' checkbox, so you normally don't need to set it manually in the ExtraDefinitions field.")
		.withDefault(1)
		.withValue(PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN)
		.withAutoConfig()
		.withCrossReference(LinkType::Preprocessor, "FORCE_INPUT_CHANNELS", "sibling FX plugin bus-configuration switch")
		.withCrossReference(LinkType::Preprocessor, "HISE_ENABLE_MIDI_INPUT_FOR_FX", "MIDI input has to be enabled on an FX plugin before keeping sound generators alive delivers any MIDI-triggered modulation");

	data["HISE_MAX_DELAY_TIME_SAMPLES"] = Entry()
		.withCategory(Category::DspAndFilters)
		.withBrief("Maximum buffer size (in samples) for every delay line in the project.")
		.withDescriptionLine("Every delay line allocates a fixed power-of-two buffer at compile time, so this value defines the longest possible delay regardless of sample rate. The default of 65536 samples equals roughly 1.36 seconds at 48 kHz and drops to about 680 ms at 96 kHz. If your project uses long delays, big reverb tails or high sample rates, increase this to 131072, 262144 or 524288 to avoid the delay time being clamped at runtime.")
		.withDescriptionLine("> The value must be a power of two. Doubling it doubles the memory footprint of every delay instance, so don't set it higher than you actually need.")
		.withDefault(65536)
		.withValue(HISE_MAX_DELAY_TIME_SAMPLES)
		.withCrossReference(LinkType::Module, "Delay", "sets the internal delay line buffer size that caps the maximum delay time")
		.withCrossReference(LinkType::Scriptnode, "core.fix_delay", "scriptnode delay line that uses the same compile-time buffer size");

	data["FORCE_INPUT_CHANNELS"] = Entry()
		.withCategory(Category::PluginType)
		.withBrief("Routes the host audio input into the master chain of an instrument plugin.")
		.withDescriptionLine("By default an instrument plugin only produces audio from its sound generators and ignores any audio coming from the host. Setting this to 1 adds a stereo input bus that feeds directly into the top of the master chain, so the master effect chain processes the incoming host audio alongside the generated instrument output. Use this when you need an instrument plugin that also reacts to audio on the track, for example a sampler with a built-in input effect path or a hybrid instrument/effect hosted on an audio track.")
		.withDescriptionLine("> Only meaningful in stereo instrument builds. It is automatically ignored when the plugin is configured for more than two output channels, and it has no effect on effect plugin exports (which always have an input bus).")
		.withDefault(0)
		.withValue(FORCE_INPUT_CHANNELS)
		.withCrossReference(LinkType::Preprocessor, "PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN", "sibling FX plugin bus-configuration switch");

	data["HI_SUPPORT_MONO_CHANNEL_LAYOUT"] = Entry()
		.withCategory(Category::PluginType)
		.withBrief("Makes an exported effect plugin accept a mono track configuration in addition to stereo.")
		.withDescriptionLine("Only relevant when the project is exported as an effect plugin. When enabled, the plugin reports both a mono-in/mono-out and a stereo-in/stereo-out bus layout to the host, so it can be inserted on mono tracks as well as stereo tracks. When disabled, the plugin advertises stereo only and hosts will either refuse to load it on a mono track or silently wrap it in an adapter.")
		.withDescriptionLine("> The HISE export dialog writes this flag automatically from the 'Support Mono FX' project setting, so you normally don't need to set it manually in the ExtraDefinitions field.")
		.withDefault(0)
		.withValue(HI_SUPPORT_MONO_CHANNEL_LAYOUT)
		.withAutoConfig()
		.withCrossReference(LinkType::Preprocessor, "HI_SUPPORT_MONO_TO_STEREO", "companion flag that extends the mono layout with a mono-in/stereo-out bus");

	data["HI_SUPPORT_MONO_TO_STEREO"] = Entry()
		.withCategory(Category::PluginType)
		.withBrief("Advertises a mono-input / stereo-output bus layout on an effect plugin with mono support enabled.")
		.withDescriptionLine("Extends the mono track support so that the plugin accepts a mono input feeding a stereo output, rather than only mono-in/mono-out and stereo-in/stereo-out. The incoming mono signal is duplicated to both channels before processing, which lets a naturally stereo effect (reverb, ping-pong delay, stereo widener) be inserted on a mono source without the host summing the output back to mono. Requires the mono channel layout support to also be enabled.")
		.withDescriptionLine("> Do not set this flag by hand. Use the 'Force Stereo Output' project setting together with 'Support Mono FX' instead, because setting the preprocessor manually causes FL Studio VST3 builds to collapse the stereo output back to mono.")
		.withDefault(0)
		.withValue(HI_SUPPORT_MONO_TO_STEREO)
		.withCrossReference(LinkType::Preprocessor, "HI_SUPPORT_MONO_CHANNEL_LAYOUT", "requires the mono channel layout support to be enabled as well");

	data["HISE_PLAY_ALL_CROSSFADE_GROUPS_WHEN_EMPTY"] = Entry()
		.withCategory(Category::BackwardsCompatibility)
		.withBrief("Plays every crossfade group at full gain when no crossfade modulator is connected.")
		.withDescriptionLine("Affects the sampler's group crossfade behaviour. When the crossfade modulation chain has no active modulator, the sampler returns a constant gain of 1.0 for every group instead of using the stored crossfade table value, so every group plays simultaneously. The default matches the original HISE behaviour and is almost always what you want, because it lets you audition grouped samples without having to wire up a modulator first.")
		.withDescriptionLine("> Disable this only if you have a legacy project that relied on the stored crossfade value being used when the chain is empty.")
		.withDefault(1)
		.withValue(HISE_PLAY_ALL_CROSSFADE_GROUPS_WHEN_EMPTY)
		.withCrossReference(LinkType::Module, "StreamingSampler", "governs the crossfade group behaviour when no crossfade modulator is wired up")
		.withCrossReference(LinkType::ScriptingApi, "Sampler", "scripted crossfade-group playback relies on the empty-chain fallback configured here")
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_WRONG_VOICE_RENDERING_ORDER", "related sampler backwards-compatibility switch");

	data["HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO"] = Entry()
		.withCategory(Category::BackwardsCompatibility)
		.withBrief("Ramps monophonic envelopes down to zero before retriggering instead of jumping straight into the attack phase.")
		.withDescriptionLine("Changes how the RETRIGGER state works in the AHDSR, Simple Envelope and the equivalent scriptnode envelope nodes when they run in monophonic mode. The default behaviour immediately transitions from RETRIGGER into ATTACK and continues from the current envelope value. With this flag enabled, the envelope first ramps towards zero at a fixed rate of 0.005 per sample and only then enters the attack phase, which gives a slightly softer restart but introduces a small retrigger delay that scales with the current envelope level.")
		.withDescriptionLine("> Polyphonic mode is unaffected because a retrigger there allocates a new voice.")
		.withDefault(0)
		.withValue(HISE_RAMP_RETRIGGER_ENVELOPES_FROM_ZERO)
		.withCrossReference(LinkType::Module, "AHDSR", "monophonic retrigger state takes the ramp-to-zero path instead of snapping to attack")
		.withCrossReference(LinkType::Module, "SimpleEnvelope", "monophonic retrigger state takes the ramp-to-zero path instead of snapping to attack")
		.withCrossReference(LinkType::Scriptnode, "envelope.ahdsr", "scriptnode envelope equivalent that follows the same retrigger rule")
		.withCrossReference(LinkType::Scriptnode, "envelope.simple_ar", "scriptnode envelope equivalent that follows the same retrigger rule");

	data["HISE_SMOOTH_FIRST_MOD_BUFFER"] = Entry()
		.withCategory(Category::BackwardsCompatibility)
		.withBrief("Historical switch for smoothing the first control-rate modulation buffer after a voice starts.")
		.withDescriptionLine("The original intent was to suppress a one-block jump in the modulation signal at voice start by ramping the first rendered buffer instead of writing the initial value straight away. The macro is still defined but no code reads it anywhere, so toggling it has no effect on modulation behaviour or on audio output. It is kept around so that older user projects which list it in their ExtraDefinitions still compile.")
		.withDefault(0)
		.withValue(HISE_SMOOTH_FIRST_MOD_BUFFER)
		.withVestigal();

	data["HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS"] = Entry()
		.withCategory(Category::BackwardsCompatibility)
		.withBrief("Subtracts one audio block from event timestamps generated on the audio thread for compatibility with older patches.")
		.withDescriptionLine("Adjusts the timestamp of artificial note-on and note-off events that scripts add through the Message and Synth scripting APIs while running on the audio thread. When enabled, the engine removes one buffer's worth of samples from the requested timestamp (clamped to zero) before queuing the event. This compensates for an old off-by-one-block scheduling behaviour so that presets built against the original timing keep sounding the same. Events triggered from other threads and artificial events created outside a MIDI callback are not affected.")
		.withDescriptionLine("> Turning this off gives you the newer, more accurate timestamp semantics but can shift the position of scripted notes by one block in existing projects.")
		.withDefault(1)
		.withValue(HISE_USE_BACKWARDS_COMPATIBLE_TIMESTAMPS)
		.withCrossReference(LinkType::ScriptingApi, "Synth", "scheduled note-on and note-off timestamps are shifted by one block when this is enabled")
		.withCrossReference(LinkType::ScriptingApi, "Message.setTimestamp", "timestamps set through Message.setTimestamp are adjusted the same way");

	data["HISE_USE_SQUARED_TIMEVARIANT_MOD_VALUES_BUG"] = Entry()
		.withCategory(Category::BackwardsCompatibility)
		.withBrief("Historical switch for reproducing a bug that squared time-variant modulation values before applying them.")
		.withDescriptionLine("The original purpose was to keep the sound of legacy patches that were tuned against an incorrect modulation path where time-variant modulator output was inadvertently multiplied by itself. The macro is still defined but no code path reads it anywhere, so enabling it does not reintroduce the bug. It is kept only so that projects which still list it in their ExtraDefinitions field keep compiling.")
		.withDefault(0)
		.withValue(HISE_USE_SQUARED_TIMEVARIANT_MOD_VALUES_BUG)
		.withVestigal();

	data["HISE_USE_WRONG_VOICE_RENDERING_ORDER"] = Entry()
		.withCategory(Category::BackwardsCompatibility)
		.withBrief("Restores the pre-fix voice render order in which voice effects ran before the polyphonic gain modulation.")
		.withDescriptionLine("Affects the Sampler, Waveform Generator, Audio Looper and Synth Group. In older HISE builds these sound generators rendered their voice effect chain before applying the polyphonic gain, crossfade and group modulation values, which is the reverse of the other generators and produces a slightly different sound whenever the voice effect is non-linear (saturation, compression, tanh wave shaping). The default behaviour is the corrected order. Enabling this flag swaps back to the legacy order so that user presets that were voiced against the old output keep sounding the same.")
		.withDescriptionLine("> Only enable this if you are maintaining an existing product and need bit-exact compatibility with user presets built against the old order.")
		.withDefault(0)
		.withValue(HISE_USE_WRONG_VOICE_RENDERING_ORDER)
		.withCrossReference(LinkType::Module, "StreamingSampler", "voice render order is reverted to the legacy pre-fix behaviour")
		.withCrossReference(LinkType::Module, "WaveSynth", "voice render order is reverted to the legacy pre-fix behaviour")
		.withCrossReference(LinkType::Module, "AudioLooper", "voice render order is reverted to the legacy pre-fix behaviour")
		.withCrossReference(LinkType::Module, "SynthGroup", "voice render order is reverted to the legacy pre-fix behaviour")
		.withCrossReference(LinkType::Preprocessor, "HISE_PLAY_ALL_CROSSFADE_GROUPS_WHEN_EMPTY", "related sampler backwards-compatibility switch");

	data["USE_OLD_MONOLITH_FORMAT"] = Entry()
		.withCategory(Category::BackwardsCompatibility)
		.withBrief("Historical switch for loading samples from the pre-HLAC monolith container format.")
		.withDescriptionLine("The original purpose was to fall back to the very first monolith container layout that HISE used before the current HLAC-based format was introduced. The macro is still defined with a hard-coded value of zero and no code reads it anywhere, so toggling it has no effect on the sampler or on how monolith files are decoded. It is kept only so that older user projects which list it in their ExtraDefinitions still compile.")
		.withDefault(0)
		.withValue(USE_OLD_MONOLITH_FORMAT)
		.withVestigal();

	data["HISE_LOG_FILTER_FREQMOD"] = Entry()
		.withCategory(Category::DspAndFilters)
		.withBrief("Applies a logarithmic skew to filter frequency modulation so the modulated cutoff tracks pitch more naturally.")
		.withDescriptionLine("By default, any modulator feeding a filter's frequency input scales linearly between 20 Hz and 20 kHz, which makes the same modulation depth sound dramatic at low cutoffs and almost inaudible at high cutoffs. Enabling this flag wraps the frequency modulation in a log-style skew (the same perceptual curve used by the frequency knob itself), so an LFO or envelope moves the cutoff by roughly the same number of semitones regardless of where the base frequency sits. The skew is only applied when modulation is actually active, so a patch with no frequency modulator sounds identical either way.")
		.withDescriptionLine("> Left off by default because existing presets were voiced against the linear curve and flipping it changes the depth and shape of every filter modulation in the project.")
		.withDefault(0)
		.withValue(HISE_LOG_FILTER_FREQMOD)
		.withCrossReference(LinkType::Module, "PolyphonicFilter", "applies the log-curve skew to the frequency modulation input")
		.withCrossReference(LinkType::Module, "CurveEq", "applies the log-curve skew to the frequency modulation input of every band");

	data["HISE_NEURAL_NETWORK_WARMUP_TIME"] = Entry()
		.withCategory(Category::DspAndFilters)
		.withBrief("Number of silent samples that the neural network node processes on reset to flush its internal state.")
		.withDescriptionLine("Whenever a voice is started or the network is reset, the node feeds this many zero-valued samples through the model before real audio arrives. This warms up any recurrent layers and IIR-style history inside the network so the first output sample does not carry the noise or click that an uninitialised model produces. Values between 128 and 2048 are typical; larger networks with long temporal memory need higher values, whereas stateless feedforward models can leave it at zero.")
		.withDescriptionLine("> Read from the project's Extra Definitions at runtime, so changing the value in the Preferences window and recompiling the scriptnode network is enough, no full HISE rebuild required.")
		.withDefault(0)
		.withValue(HISE_NEURAL_NETWORK_WARMUP_TIME)
		.withHotReload()
		.withCrossReference(LinkType::Scriptnode, "math.neural", "warmup sample count used when the neural node resets")
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_RT_NEURAL", "only takes effect in builds where the RTNeural inference library is compiled in");

	data["HISE_UPDATE_CONVOLUTION_DAMPING_ASYNC"] = Entry()
		.withCategory(Category::DspAndFilters)
		.withBrief("Recomputes the convolution Damping filter asynchronously so UI drags stay smooth.")
		.withDescriptionLine("Changing the Damping parameter of a convolution reverb applies a one-pole low-pass to the impulse response, which is an expensive operation. With this flag on, the recomputation is dispatched to the worker thread so a slider drag or automation stream can keep feeding new values without stalling the UI or glitching the audio. Loading an impulse response or changing its sample range always stays synchronous, regardless of this setting.")
		.withDescriptionLine("> Disable only if you need strict ordering when a script sets Damping back-to-back with an IR swap or range change, because the async path can otherwise apply the damping after the next IR has already loaded.")
		.withDefault(1)
		.withValue(HISE_UPDATE_CONVOLUTION_DAMPING_ASYNC)
		.withCrossReference(LinkType::Module, "Convolution", "Damping parameter changes are dispatched to the worker thread instead of stalling the audio thread")
		.withCrossReference(LinkType::Scriptnode, "filters.convolution", "scriptnode convolution node that uses the same damping-update path");

	data["HISE_USE_SVF_FOR_CURVE_EQ"] = Entry()
		.withCategory(Category::DspAndFilters)
		.withBrief("Swaps the biquad implementation inside the Parametriq EQ for state-variable filters.")
		.withDescriptionLine("The Parametriq EQ uses stock biquad coefficients for every band by default, which sound fine on static settings but produce audible zipper noise when the frequency, gain or Q of a band is automated or scripted in real time. Enabling this flag switches every band to the state-variable filter topology (the same one exposed as filters.svf_eq in scriptnode), which is zipper-free under modulation and additionally gives the LowPass and HighPass bands a working Q parameter instead of the fixed one-pole response. The static sound is slightly different from the biquad version, so existing mix references will shift.")
		.withDescriptionLine("> Must be set in both the HISE build and the exported plugin's Extra Definitions, because the filter type is baked into the binary at compile time.")
		.withDefault(0)
		.withValue(HISE_USE_SVF_FOR_CURVE_EQ)
		.withCrossReference(LinkType::Module, "CurveEq", "every band switches from biquad coefficients to SVF topology")
		.withCrossReference(LinkType::Scriptnode, "filters.svf_eq", "scriptnode node that uses the same SVF topology by default");

	data["HISE_NUM_FX_PLUGIN_CHANNELS"] = Entry()
		.withCategory(Category::PolyphonyAndChannels)
		.withBrief("Number of audio channels exposed to the host when the project is exported as an effect plugin.")
		.withDescriptionLine("Controls the channel count that an exported FX plugin advertises to the DAW, which is the standard way to enable sidechain input or multi-channel effect processing (set to 4 for a stereo main plus stereo sidechain bus, 6 for 5.1, and so on). The value must be an even number and must not be smaller than the master container's routing matrix channel count; it may be larger, which lets you keep additional stereo pairs for hidden internal routing that the host never sees. Only affects effect plugin exports.")
		.withDescriptionLine("> Has no effect on instrument plugins or the standalone app, and is independent from the main plugin channel count used by instrument exports.")
		.withDefault(2)
		.withValue(HISE_NUM_FX_PLUGIN_CHANNELS)
		.withCrossReference(LinkType::Preprocessor, "NUM_MAX_CHANNELS", "must not exceed the project-wide channel ceiling");

	data["HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS"] = Entry()
		.withCategory(Category::PolyphonyAndChannels)
		.withBrief("Upper channel limit for scriptnode frame processing containers.")
		.withDescriptionLine("Frame processing containers in scriptnode produce one sample per channel at a time instead of processing in blocks, and their channel dispatch is resolved at compile time through a fixed set of template specialisations. This value caps how many channels those specialisations cover, so a frame container configured for more channels than the limit will fail to compile or fall back to the non-frame path. The default of 8 covers all common surround formats and only needs to be raised for unusual routing setups such as ambisonics or bespoke multichannel effects.")
		.withDescriptionLine("> Increasing this adds template instantiations and slightly grows compile time and binary size, so only raise it when you actually need wider frame containers.")
		.withDefault(8)
		.withValue(HISE_NUM_MAX_FRAME_CONTAINER_CHANNELS)
		.withCrossReference(LinkType::Scriptnode, "container.frame1_block", "frame container whose channel dispatch is bounded by this value")
		.withCrossReference(LinkType::Scriptnode, "container.framex_block", "multichannel frame container whose maximum width is set by this value");

	data["HISE_NUM_PLUGIN_CHANNELS"] = Entry()
		.withCategory(Category::PolyphonyAndChannels)
		.withBrief("Number of output channels advertised by an exported instrument plugin.")
		.withDescriptionLine("Defines the output bus width of the compiled instrument plugin and the destination channel count of the master routing matrix in plugin builds. The value must be even, a multiple of 2, and must not exceed the project-wide channel ceiling; pairing it with a correspondingly sized routing matrix in the master container lets you build multi-output instruments where each stereo pair lands on its own DAW bus. The HISE export dialog derives this value automatically from the master container's routing matrix (or clamps it to 2 when 'Force Stereo Output' is set), so it should rarely be set by hand.")
		.withDescriptionLine("> Manually overriding this in ExtraDefinitions bypasses the routing-matrix derivation and is only useful for edge cases such as exporting a plugin with more output channels than the master chain currently uses.")
		.withDefault(2)
		.withValue(HISE_NUM_PLUGIN_CHANNELS)
		.withAutoConfig()
		.withCrossReference(LinkType::Preprocessor, "NUM_MAX_CHANNELS", "must not exceed the project-wide channel ceiling")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_STANDALONE_OUTPUTS", "standalone output count defaults to this value");

	data["HISE_NUM_STANDALONE_OUTPUTS"] = Entry()
		.withCategory(Category::PolyphonyAndChannels)
		.withBrief("Number of output channels used when the project runs as a standalone application.")
		.withDescriptionLine("Sets how many channels the standalone HISE app (and any exported standalone build) requests from the audio device and feeds to the master routing matrix. The value must be even and defaults to the main plugin channel count, which keeps standalone and plugin behaviour in sync for typical stereo projects. Raise this together with the plugin channel count when building a multi-output standalone instrument so that the extra buses reach the audio interface.")
		.withDescriptionLine("> When the standalone device settings and this value disagree, the standalone startup code will prompt to reset the saved audio configuration to match.")
		.withDefault(2)
		.withValue(HISE_NUM_STANDALONE_OUTPUTS)
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_PLUGIN_CHANNELS", "default tracks the plugin channel count so standalone and plugin builds stay aligned");

	data["NUM_MAX_CHANNELS"] = Entry()
		.withCategory(Category::PolyphonyAndChannels)
		.withBrief("Project-wide ceiling on the number of channels that the routing matrix and multichannel buffers can handle.")
		.withDescriptionLine("Sets the fixed array size used throughout HISE for channel connections, routing matrices, microphone position bookkeeping and the multichannel audio buffer, so every container, effect and sampler instance is limited to this many channels regardless of its own configuration. The value must be a multiple of 2 and defaults to 16, which is enough for most surround and multi-mic sampler setups. Raise it (for example to 32) only when a project genuinely needs more than 8 stereo pairs, because the extra channels grow every per-channel array in the runtime.")
		.withDescriptionLine("> Must match between the HISE build and any project DLL or exported plugin; a mismatch corrupts routing-matrix state because the array sizes no longer line up.")
		.withDefault(16)
		.withValue(NUM_MAX_CHANNELS)
		.withCrossReference(LinkType::Module, "RouteFX", "routing-matrix channel count ceiling applied to the RouteFX module")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_PLUGIN_CHANNELS", "instrument plugin channel count is capped by this value")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_FX_PLUGIN_CHANNELS", "effect plugin channel count is capped by this value");

	data["NUM_POLYPHONIC_VOICES"] = Entry()
		.withCategory(Category::PolyphonyAndChannels)
		.withBrief("Compile-time upper bound on the number of simultaneously active voices per sound generator.")
		.withDescriptionLine("Sets the fixed voice array size for every polyphonic sound generator, modulator chain, polyphonic envelope and polyphonic scriptnode state, so no sound generator can ever play more than this many voices regardless of its runtime voice limit. The default is 256 on desktop and 128 on iOS, which is the ceiling of the per-generator VoiceLimit parameter. Raising it increases the memory footprint of every polyphonic modulator and envelope and is only needed for extreme voice counts; the VoiceLimit parameter on each synth is the right place to tune per-patch polyphony, not this macro.")
		.withDescriptionLine("> Must match exactly between the HISE build and any project DLL, otherwise the voice-indexed arrays disagree in size and audio glitches or crashes occur when the DLL is loaded.")
		.withDefault(256)
		.withValue(NUM_POLYPHONIC_VOICES)
		.withCrossReference(LinkType::Module, "StreamingSampler", "fixes the maximum voice count that the sampler can allocate")
		.withCrossReference(LinkType::Module, "SynthGroup", "fixes the maximum voice count that the Synth Group can allocate");

	data["AUDIOFFT_FFTW3"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Switches the FFT convolution backend over to the FFTW3 library instead of the built-in Ooura implementation.")
		.withDescriptionLine("Selects FFTW3 as the FFT engine used by the convolution reverb and the FFT convolver inside scriptnode's filters.convolution node. The built-in Ooura backend is lightweight and always available, whereas FFTW3 is noticeably faster for long impulse responses but has to be linked into the build separately and ships under a copyleft licence that may not suit a commercial product. The HISE exporter will only wire up an FFTW build when IPP is not active on the same platform, so enabling both at once has no effect.")
		.withDescriptionLine("> FFTW3 has to be present as an external dependency at both compile and link time. Check the licensing terms of FFTW3 before shipping the resulting binary.")
		.withDefault(0)
		.withValue(AUDIOFFT_FFTW3)
		.withCrossReference(LinkType::Module, "Convolution", "FFT backend used by the convolution reverb switches over to FFTW3")
		.withCrossReference(LinkType::Scriptnode, "filters.convolution", "scriptnode FFT convolver switches over to FFTW3 as well")
		.withCrossReference(LinkType::Preprocessor, "USE_IPP", "exporter picks IPP over FFTW3 when both are enabled on the same platform");

	data["HISE_INCLUDE_BEATPORT"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Compiles the Beatport authentication integration into the build.")
		.withDescriptionLine("Enables the real implementation of the BeatportManager scripting object, which talks to the Beatport SDK to validate a product against a user's Beatport account. With the flag off, the scripting object still loads so that existing scripts compile, but setProductId is a no-op, validate returns an empty object and isBeatportAccess always reports false. The Beatport SDK has to be supplied separately and linked into the project; HISE does not bundle it.")
		.withDescriptionLine("> Only meaningful for products distributed through Beatport's plugin catalogue. Leave this off for everything else.")
		.withDefault(0)
		.withValue(HISE_INCLUDE_BEATPORT)
		.withCrossReference(LinkType::ScriptingApi, "BeatportManager", "scripting object that is only functional when this flag is on")
		.withCrossReference(LinkType::Preprocessor, "JUCE_ALLOW_EXTERNAL_UNLOCK", "Beatport sits alongside MuseHub as an external unlock path that relies on this JUCE back door");

	data["HISE_INCLUDE_BX_LICENSER"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Compiles the Brainworx / Plugin Alliance BX Licenser integration into the build.")
		.withDescriptionLine("Enables the Engine.createBXLicenser scripting factory and pulls in the BX Licenser wrapper sources, which let a plugin authenticate against the Plugin Alliance licence system and redeem installer codes. With the flag off, createBXLicenser returns undefined, the wrapper sources are excluded from the unity build and no Plugin Alliance dependency is linked. The BX licensing library has to be provided separately and is only available to Plugin Alliance partner developers.")
		.withDescriptionLine("> Only set this for a product that ships through the Plugin Alliance distribution channel, because the licensing library is not publicly available.")
		.withDefault(0)
		.withValue(HISE_INCLUDE_BX_LICENSER)
		.withCrossReference(LinkType::ScriptingApi, "Engine.createBXLicenser", "Engine.createBXLicenser is only registered when this flag is on");

	data["HISE_INCLUDE_LORIS"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Includes the Loris analysis and resynthesis library so scripts can access the LorisManager API.")
		.withDescriptionLine("Loris is an additive analysis and resynthesis toolkit that HISE uses for partial tracking, time stretching and spectral morphing of audio files. Enabling this flag compiles the entire hi_loris module (around fifty translation units), exposes Engine.getLorisManager and the LorisManager scripting class, and lets the wavetable converter use Loris-based resynthesis for cycle extraction. The flag is on by default in the HISE IDE because the Loris workflows are part of the authoring tooling, and off in the exported plugin template to keep the runtime binary small.")
		.withDescriptionLine("> Disable this in the HISE build only if compile times are a problem and you never need Loris-based tools. The plugin template already disables it automatically for exports.")
		.withDefault(1)
		.withValue(HISE_INCLUDE_LORIS)
		.withAutoConfig()
		.withCrossReference(LinkType::ScriptingApi, "LorisManager", "entire LorisManager scripting class is only available when this flag is on")
		.withCrossReference(LinkType::ScriptingApi, "Engine.getLorisManager", "Engine.getLorisManager returns a real object only when this flag is on")
		.withCrossReference(LinkType::ScriptingApi, "WavetableController", "wavetable converter can use Loris-based resynthesis when this flag is on")
		.withCrossReference(LinkType::Preprocessor, "USE_MOD2_WAVETABLESIZE", "Loris cycle extraction produces power-of-two wavetables that pair with the fast wavetable path");

	data["HISE_INCLUDE_MUSEHUB"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Compiles the MuseHub SDK integration into an exported plugin.")
		.withDescriptionLine("Enables the real MuseHub licence check inside ScriptUnlocker so that Unlocker.checkMuseHub validates the plugin against a user's MuseHub account through the MuseHub SDK. With the flag off, the backend build falls back to a simulated 50/50 result after a two second delay and the frontend does nothing at all. The MuseHub SDK headers and static library have to be supplied separately by MuseHub and are only available to partners distributing through their catalogue.")
		.withDescriptionLine("> Only takes effect in exported plugin builds. The HISE IDE always runs the simulated path regardless of this setting so that the scripting API can be tested without the SDK.")
		.withDefault(0)
		.withValue(HISE_INCLUDE_MUSEHUB)
		.withCrossReference(LinkType::ScriptingApi, "Unlocker.checkMuseHub", "Unlocker.checkMuseHub only performs a real licence check when this flag is on")
		.withCrossReference(LinkType::Preprocessor, "JUCE_ALLOW_EXTERNAL_UNLOCK", "provides the external-unlock back door that the MuseHub licence check routes through");

	data["HISE_INCLUDE_NKS_SDK"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Compiles the Native Instruments NKS integration into the build.")
		.withDescriptionLine("Enables the Engine.createNKSManager scripting factory, pulls in the NKS wrapper sources and links the NKS VST3 interface hooks so that an exported plugin can show preset information, tag metadata and a factory browser inside Komplete Kontrol and Maschine. With the flag off, createNKSManager returns undefined, the NKS wrapper sources are excluded from the unity build and the frontend uses a dummy NKSVST3Interface stub. The NKS SDK has to be requested from Native Instruments separately; HISE does not ship it.")
		.withDescriptionLine("> The exporter inspects the project's Extra Definitions for this flag and skips the NKS wiring automatically when it is not present, so in most cases you only need to set it in ExtraDefinitionsWindows or ExtraDefinitionsOSX rather than on the HISE build itself.")
		.withDefault(0)
		.withValue(HISE_INCLUDE_NKS_SDK)
		.withAutoConfig()
		.withCrossReference(LinkType::ScriptingApi, "Engine.createNKSManager", "Engine.createNKSManager is only registered when this flag is on");

	data["HISE_INCLUDE_PITCH_DETECTION"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Compiles the dywapitchtrack pitch detection library into the build.")
		.withDescriptionLine("Enables the PitchDetection helper class and exposes it to scripting through Buffer.detectPitch, which runs an autocorrelation-based pitch estimator over an audio buffer and returns the fundamental in Hz. With the flag off, the detection code is excluded and the scripting method is not registered, which trims a small amount from the compiled binary. Leave this on unless you are stripping HISE down to a minimal build and know that no script in the project calls detectPitch.")
		.withDefault(1)
		.withValue(HISE_INCLUDE_PITCH_DETECTION)
		.withCrossReference(LinkType::ScriptingApi, "Buffer.detectPitch", "Buffer.detectPitch is only registered when this flag is on");

	data["HISE_INCLUDE_RLOTTIE"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Compiles the rLottie vector animation library so scripts can play Lottie animations in a panel.")
		.withDescriptionLine("Pulls in the entire hi_rlottie module and wires up the Panel animation API (setAnimation, setAnimationFrame, getAnimationData) that plays JSON-based Lottie vector animations inside a ScriptPanel. The library is statically linked by default so no extra dynamic dependency ships with the plugin. Disabling this flag removes the animation methods, skips the rLottie translation units and noticeably cuts compile time for projects that do not use vector animations at all.")
		.withDescriptionLine("> Enabled by default in both the HISE build and the exported plugin template so that animations work out of the box in new projects.")
		.withDefault(1)
		.withValue(HISE_INCLUDE_RLOTTIE)
		.withAutoConfig()
		.withCrossReference(LinkType::UI, "Components.ScriptPanel", "Panel animation API (setAnimation, setAnimationFrame) is only compiled in when this flag is on");

	data["HISE_INCLUDE_RT_NEURAL"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Compiles the RTNeural inference library so scriptnode can run neural network models in real time.")
		.withDescriptionLine("Enables the NeuralNetwork scripting class, the math.neural scriptnode node and the loaders for TensorFlow, PyTorch and ONNX models, which are the entire realtime-inference stack in HISE. With the flag off, the math.neural node still appears in the factory but renders nothing and shows 'This node is disabled. Recompile HISE with HISE_INCLUDE_RT_NEURAL' in its panel, and the NeuralNetwork holder on the MainController is compiled out so load calls fail silently.")
		.withDescriptionLine("> Must be set identically in the HISE build and the exported plugin, because the NeuralNetwork storage and the node binary layout both change when the flag is toggled.")
		.withDefault(1)
		.withValue(HISE_INCLUDE_RT_NEURAL)
		.withCrossReference(LinkType::ScriptingApi, "NeuralNetwork", "entire NeuralNetwork scripting class is only available when this flag is on")
		.withCrossReference(LinkType::Scriptnode, "math.neural", "node renders silence and shows a disabled message when this flag is off")
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_XSIMD", "RTNeural inference relies on the bundled xsimd header")
		.withCrossReference(LinkType::Preprocessor, "HISE_NEURAL_NETWORK_WARMUP_TIME", "warmup length that only takes effect together with this integration");

	data["HISE_INCLUDE_XSIMD"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Signals that the surrounding project already provides the xsimd SIMD header, so HISE skips its own copy.")
		.withDescriptionLine("When off, HISE pulls in the xsimd header that is bundled with RTNeural from hi_tools and from MiscToolClasses so that the SIMD wrappers compile against a known version. When on, the include is skipped and the project is expected to have made a different xsimd installation visible on the include path before hi_tools is parsed. Use this only if your own code also includes xsimd and the two copies clash at compile time; leaving it off is correct for almost every project.")
		.withDefault(0)
		.withValue(HISE_INCLUDE_XSIMD)
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_RT_NEURAL", "RTNeural inference consumes the xsimd header that this flag controls");

	data["USE_IPP"] = Entry()
		.withCategory(Category::ThirdPartyModules)
		.withBrief("Enables the Intel Integrated Performance Primitives fast paths for FFT, vector math and sample playback.")
		.withDescriptionLine("IPP is Intel's hand-optimised SIMD library, and enabling this flag routes the FFT convolver, the sampler pitch accumulation and several vector operations through ippsAdd, ippsSum and the IPP FFT instead of the portable fallbacks. The gain on Windows is large for long convolutions and busy multi-voice samplers, but IPP has to be installed through Intel oneAPI and its libraries linked into the build separately. The flag is auto-defined to 1 on Windows when the Projucer detects any of the _IPP_SEQUENTIAL_STATIC / _IPP_PARALLEL_* macros, and is forced to 0 on macOS and Linux because IPP is Windows-only in HISE.")
		.withDescriptionLine("> The exporter writes USE_IPP=1 into the Windows Extra Definitions automatically when the 'Use IPP' project setting is on, so it should normally not be set by hand. Setting it manually on macOS or Linux fails a static_assert in hi_lac.")
		.withDefault(0)
		.withValue(USE_IPP)
		.withAutoConfig()
		.withCrossReference(LinkType::Module, "Convolution", "IPP FFT routines replace the portable convolution backend")
		.withCrossReference(LinkType::ScriptingApi, "Settings.getUseIpp", "Settings.getUseIpp reports the compile-time value of this flag")
		.withCrossReference(LinkType::Preprocessor, "AUDIOFFT_FFTW3", "IPP takes precedence over FFTW3 when both are enabled on the same platform");

	data["HISE_BROWSE_FOLDER_WHEN_RELOCATING_SAMPLES"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Controls whether relocating samples opens a folder picker or a file picker for a .ch1 file.")
		.withDescriptionLine("Changes the behaviour of the 'Relocate Samples' button in the standalone settings window of an exported plugin. When enabled, clicking it opens a folder browser so the user can point at the directory that contains the extracted sample archive. When disabled, it opens a file browser filtered to .ch1 files and uses the parent folder of the chosen file as the new sample location, which is useful if the installer produces a nested folder layout and you want the user to click on a known landmark file instead of navigating to the correct subfolder.")
		.withDescriptionLine("> Only affects the standalone settings dialog in the exported plugin. Has no effect inside the HISE IDE.")
		.withDefault(1)
		.withValue(HISE_BROWSE_FOLDER_WHEN_RELOCATING_SAMPLES)
		.withCrossReference(LinkType::Preprocessor, "HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON", "the locate button driven by that flag picks either a folder or file picker depending on this one");

	data["HISE_ENABLE_CROSSFADE_MODULATION_THRESHOLD"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Skips per-sample crossfade modulation when the control-rate signal is effectively constant over a block.")
		.withDescriptionLine("Affects how the sampler renders its group crossfade modulation chain. When enabled, the first and last control-rate values of each block are compared and if they differ by less than -80 dB the engine writes a single ramp value for the whole block instead of expanding the full modulation signal. This is a measurable CPU saving for patches with many voices in crossfaded groups where the modulation value is usually static. Disabling it always uses the full modulation signal and removes any chance of audible stepping when a crossfade slider is automated very slowly.")
		.withDescriptionLine("> Disable only if you hear subtle artifacts when crossfading between sampler groups and need the full per-sample resolution.")
		.withDefault(1)
		.withValue(HISE_ENABLE_CROSSFADE_MODULATION_THRESHOLD)
		.withCrossReference(LinkType::Module, "StreamingSampler", "crossfade modulation chain of the sampler uses this control-rate shortcut")
		.withCrossReference(LinkType::ScriptingApi, "Sampler", "scripted crossfade modulation relies on the same threshold behaviour");

	data["HISE_LOAD_ENTIRE_SAMPLE_THRESHHOLD"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Preload size in samples above which the streaming engine loads the entire sample into memory.")
		.withDescriptionLine("When a sample's preload buffer would exceed this threshold, the streaming engine skips disk streaming for that sample and loads it into memory in full. The default is INT_MAX which effectively disables the shortcut so every sample is streamed normally. Lower values (for example 28000) are useful as a workaround for rare edge cases where short samples produce clicks during streamed playback. The value is interpreted as a sample frame count, so memory cost scales with bit depth and channel count.")
		.withDescriptionLine("> Setting this too low forces a lot of short samples to stay fully in RAM and can noticeably inflate memory usage on large sample sets.")
		.withDefault(INT_MAX)
		.withValue(HISE_LOAD_ENTIRE_SAMPLE_THRESHHOLD)
		.withCrossReference(LinkType::Module, "StreamingSampler", "threshold above which the streaming engine falls back to a full in-memory load")
		.withCrossReference(LinkType::Preprocessor, "USE_FALLBACK_READERS_FOR_MONOLITH", "companion streaming-backend knob that forces per-instance file handles regardless of size");

	data["HISE_SAMPLER_ALLOW_RELEASE_START"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Enables the release start feature on the sampler, including scripting APIs and the release start editor.")
		.withDescriptionLine("When enabled, every sample can define a release start marker that the voice jumps to when the note is released, with configurable fade time, fade curve, zero crossing alignment, gain matching and peak smoothing. The full configuration is exposed in the sample editor and through Sampler.setReleaseStartOptions, Sampler.getReleaseStartOptions and Sampler.setAllowReleaseStart. Disabling it removes the feature entirely, which saves a small amount of per-voice state and removes the release start properties from the sample map format.")
		.withDescriptionLine("> Must be set consistently for both the HISE build and the exported plugin. Calling the release start scripting APIs with this disabled raises a script error.")
		.withDefault(1)
		.withValue(HISE_SAMPLER_ALLOW_RELEASE_START)
		.withCrossReference(LinkType::Module, "StreamingSampler", "release start marker and its sample editor UI are gated on this flag")
		.withCrossReference(LinkType::ScriptingApi, "Sampler", "setReleaseStartOptions, getReleaseStartOptions and setAllowReleaseStart are only available when this flag is on");

	data["HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Shows the 'Install Samples' button on the SamplesNotInstalled overlay in an exported plugin.")
		.withDescriptionLine("Controls the install button in the overlay that appears when the exported plugin cannot find its sample archive. When enabled, the user can pick a downloaded .hr1 archive and the plugin extracts it into the sample folder using the built-in HLAC archiver. Disable this if you ship samples through a custom installer or a separate application and do not want the plugin to offer the HR1 extraction workflow. The default message shown on the overlay also changes depending on which of the install and locate buttons are enabled.")
		.withDescriptionLine("> If both this and the locate button are disabled, the SamplesNotInstalled overlay is suppressed completely.")
		.withDefault(1)
		.withValue(HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON)
		.withCrossReference(LinkType::ScriptingApi, "ErrorHandler", "controls the install button on the SamplesNotInstalled overlay raised by the sample error handler")
		.withCrossReference(LinkType::Preprocessor, "HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON", "the overlay is suppressed entirely when both buttons are disabled");

	data["HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Shows the 'Locate Samples' button on the SamplesNotInstalled overlay in an exported plugin.")
		.withDescriptionLine("Controls the locate button in the overlay that appears when the exported plugin cannot find its sample archive. When enabled, the user can point the plugin at an existing sample folder on first launch. When disabled, the plugin silently picks a sensible default location (the user documents folder on Windows, the music folder on macOS and Linux) and keeps using that until the location is changed manually in the settings window. The default message on the overlay also changes depending on which of the install and locate buttons are enabled.")
		.withDescriptionLine("> If both this and the install button are disabled, the SamplesNotInstalled overlay is suppressed completely.")
		.withDefault(1)
		.withValue(HISE_SAMPLE_DIALOG_SHOW_LOCATE_BUTTON)
		.withCrossReference(LinkType::ScriptingApi, "ErrorHandler", "controls the locate button on the SamplesNotInstalled overlay raised by the sample error handler")
		.withCrossReference(LinkType::Preprocessor, "HISE_SAMPLE_DIALOG_SHOW_INSTALL_BUTTON", "the overlay is suppressed entirely when both buttons are disabled")
		.withCrossReference(LinkType::Preprocessor, "HISE_BROWSE_FOLDER_WHEN_RELOCATING_SAMPLES", "picker type used by the locate button is selected by this flag");

	data["HI_SUPPORT_FULL_DYNAMICS_HLAC"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Exposes the Full Dynamics sample encoding option in the exported plugin's sample install dialog.")
		.withDescriptionLine("When enabled, the sample installer dialog inside the exported plugin shows the 'Sample bit depth' selector so the user can opt into the Full Dynamics HLAC encoding that preserves the original 24 bit dynamic range instead of the standard 16 bit path. The HISE IDE always shows this option. If disabled in an exported plugin, the install dialog falls back to the standard encoding with no user choice, which keeps the archive smaller and slightly speeds up decoding at the cost of extra quantisation noise.")
		.withDescriptionLine("> The HISE export dialog writes this flag automatically from the 'Support Full Dynamics' project setting, so you normally don't need to set it manually in the ExtraDefinitions field.")
		.withDefault(0)
		.withValue(HI_SUPPORT_FULL_DYNAMICS_HLAC)
		.withAutoConfig();

	data["USE_FALLBACK_READERS_FOR_MONOLITH"] = Entry()
		.withCategory(Category::SamplerAndStreaming)
		.withBrief("Uses a per-instance file handle for every monolith reader instead of memory mapping the sample archive.")
		.withDescriptionLine("Switches the streaming backend from memory-mapped monolith access to regular FileInputStream-based HLAC readers. The default picks memory mapping on 64 bit desktop builds and falls back to file handles on iOS and 32 bit builds where the address space is too small to map large sample sets. Enabling this explicitly forces the file handle path on every platform, which is useful when the plugin must run under sandboxing restrictions that block mmap, or when debugging a sample loading issue that seems to be related to the memory mapper.")
		.withDescriptionLine("> Turning this on costs one open file handle per monolith part per voice, so it can hit per-process file descriptor limits on large sample libraries.")
		.withDefault(0)
		.withValue(USE_FALLBACK_READERS_FOR_MONOLITH)
		.withCrossReference(LinkType::Module, "StreamingSampler", "forces the sampler's monolith reader to use file handles instead of memory mapping")
		.withCrossReference(LinkType::Preprocessor, "HISE_LOAD_ENTIRE_SAMPLE_THRESHHOLD", "companion streaming-backend knob that controls when to bypass disk streaming entirely");

	data["HISE_NUM_MODULATORS_PER_CHAIN"] = Entry()
		.withCategory(Category::ModulatorSlots)
		.withBrief("Maximum number of modulators a single modulation chain can hold.")
		.withDescriptionLine("Caps how many modulators (LFOs, envelopes, velocity modulators, script modulators and so on) may be inserted into any gain, pitch or parameter modulation chain in the project. The value also sizes the active-modulator lookup tables inside each chain and the scriptnode modulation source registry, so raising it increases per-chain memory footprint even when the extra slots stay empty. Only increase this if you hit the limit with a genuinely dense modulation setup.")
		.withDescriptionLine("> Sensible range is 16 to 256.")
		.withDefault(64)
		.withValue(HISE_NUM_MODULATORS_PER_CHAIN);

	data["HISE_NUM_SCRIPTNODE_FX_MODS"] = Entry()
		.withCategory(Category::ModulatorSlots)
		.withBrief("Number of extra modulation chain slots exposed by monophonic scriptnode effects.")
		.withDescriptionLine("Adds modulation chain slots to every Script FX module so that standard HISE modulators can drive root parameters of the embedded scriptnode network, either via the External Modulation property on a parameter or via a core.extra_mod node placed inside the network. Slots are evaluated at control rate and carry no measurable CPU cost when left empty, so the value can be sized to the worst-case network without penalty. Changing the value after a project has been saved will prompt users to reload the affected effects.")
		.withDescriptionLine("> Sensible range is 0 to 8.")
		.withDefault(0)
		.withValue(HISE_NUM_SCRIPTNODE_FX_MODS)
		.withHotReload()
		.withCrossReference(LinkType::Module, "ScriptFX", "host module that exposes these modulation slots")
		.withCrossReference(LinkType::Scriptnode, "core.extra_mod", "node that reads the modulation signal inside the network")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS", "polyphonic counterpart for per-voice script effects")
		.withCrossReference(LinkType::Preprocessor, "NUM_HARDCODED_FX_MODS", "equivalent slot count for compiled hardcoded effects")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MODULATORS_PER_CHAIN", "master cap on modulators that can occupy each slot");

	data["HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS"] = Entry()
		.withCategory(Category::ModulatorSlots)
		.withBrief("Number of extra modulation chain slots exposed by polyphonic scriptnode effects.")
		.withDescriptionLine("Adds per-voice modulation chain slots to every Polyphonic Script FX module so that polyphonic modulators can drive root parameters of the embedded scriptnode network. Because the slots run per voice, each active slot contributes to the polyphonic voice cost and memory footprint, so keep this as low as the project actually needs. Changing the value after a project has been saved will prompt users to reload the affected effects.")
		.withDescriptionLine("> Sensible range is 0 to 4.")
		.withDefault(0)
		.withValue(HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS)
		.withHotReload()
		.withCrossReference(LinkType::Module, "PolyScriptFX", "host module that exposes these modulation slots")
		.withCrossReference(LinkType::Scriptnode, "core.extra_mod", "node that reads the modulation signal inside the network")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_SCRIPTNODE_FX_MODS", "monophonic counterpart for master script effects")
		.withCrossReference(LinkType::Preprocessor, "NUM_HARDCODED_POLY_FX_MODS", "equivalent slot count for compiled polyphonic effects")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MODULATORS_PER_CHAIN", "master cap on modulators that can occupy each slot");

	data["HISE_NUM_SCRIPTNODE_SYNTH_MODS"] = Entry()
		.withCategory(Category::ModulatorSlots)
		.withBrief("Number of extra modulation chain slots exposed by scriptnode synthesisers.")
		.withDescriptionLine("Adds per-voice modulation chain slots to every Scriptnode Synthesiser so that standard HISE modulators can drive root parameters of the embedded scriptnode network in addition to the built-in gain and pitch chains. The default of two covers common filter-cutoff and amp-modulation patches; raise it only when the synth needs more independent modulation targets. Changing the value after a project has been saved will prompt users to reload the affected synths.")
		.withDescriptionLine("> Sensible range is 0 to 8.")
		.withDefault(2)
		.withValue(HISE_NUM_SCRIPTNODE_SYNTH_MODS)
		.withHotReload()
		.withCrossReference(LinkType::Module, "ScriptSynth", "host module that exposes these modulation slots")
		.withCrossReference(LinkType::Scriptnode, "core.extra_mod", "node that reads the modulation signal inside the network")
		.withCrossReference(LinkType::Preprocessor, "NUM_HARDCODED_SYNTH_MODS", "equivalent slot count for compiled hardcoded synthesisers")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MODULATORS_PER_CHAIN", "master cap on modulators that can occupy each slot");

	data["NUM_HARDCODED_FX_MODS"] = Entry()
		.withCategory(Category::ModulatorSlots)
		.withBrief("Number of extra modulation chain slots exposed by hardcoded master effects.")
		.withDescriptionLine("Adds modulation chain slots to every Hardcoded Master FX module so that standard HISE modulators can drive parameters of the compiled C++ DSP network. The project exporter writes this value into the exported plugin automatically, so configuring it from Extra Definitions while developing is usually enough. Slots are evaluated at control rate and carry no measurable CPU cost when left empty. Changing the value after a project has been saved will prompt users to reload the affected effects.")
		.withDescriptionLine("> Sensible range is 0 to 8.")
		.withDefault(0)
		.withValue(NUM_HARDCODED_FX_MODS)
		.withHotReload()
		.withAutoConfig()
		.withCrossReference(LinkType::Module, "HardcodedMasterFX", "host module that exposes these modulation slots")
		.withCrossReference(LinkType::Scriptnode, "core.extra_mod", "node that reads the modulation signal inside the network")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_SCRIPTNODE_FX_MODS", "equivalent slot count for interpreted script effects")
		.withCrossReference(LinkType::Preprocessor, "NUM_HARDCODED_POLY_FX_MODS", "polyphonic counterpart for per-voice hardcoded effects")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MODULATORS_PER_CHAIN", "master cap on modulators that can occupy each slot");

	data["NUM_HARDCODED_POLY_FX_MODS"] = Entry()
		.withCategory(Category::ModulatorSlots)
		.withBrief("Number of extra modulation chain slots exposed by hardcoded polyphonic effects.")
		.withDescriptionLine("Adds per-voice modulation chain slots to every Hardcoded Polyphonic FX module so that polyphonic modulators can drive parameters of the compiled C++ DSP network. The project exporter writes this value into the exported plugin automatically. Each active slot runs once per voice, so keep this as low as the project actually needs to avoid scaling the polyphonic voice cost. Changing the value after a project has been saved will prompt users to reload the affected effects.")
		.withDescriptionLine("> Sensible range is 0 to 4.")
		.withDefault(0)
		.withValue(NUM_HARDCODED_POLY_FX_MODS)
		.withHotReload()
		.withAutoConfig()
		.withCrossReference(LinkType::Module, "HardcodedPolyphonicFX", "host module that exposes these modulation slots")
		.withCrossReference(LinkType::Scriptnode, "core.extra_mod", "node that reads the modulation signal inside the network")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_POLYPHONIC_SCRIPTNODE_FX_MODS", "equivalent slot count for interpreted polyphonic script effects")
		.withCrossReference(LinkType::Preprocessor, "NUM_HARDCODED_FX_MODS", "monophonic counterpart for master hardcoded effects")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MODULATORS_PER_CHAIN", "master cap on modulators that can occupy each slot");

	data["NUM_HARDCODED_SYNTH_MODS"] = Entry()
		.withCategory(Category::ModulatorSlots)
		.withBrief("Number of extra modulation chain slots exposed by hardcoded synthesisers.")
		.withDescriptionLine("Adds per-voice modulation chain slots to every Hardcoded Synthesiser so that standard HISE modulators can drive parameters of the compiled C++ DSP network in addition to the built-in gain and pitch chains. The default of two covers common filter-cutoff and amp-modulation patches; raise it only when the synth needs more independent modulation targets. Changing the value after a project has been saved will prompt users to reload the affected synths.")
		.withDescriptionLine("> Sensible range is 0 to 8.")
		.withDefault(2)
		.withValue(NUM_HARDCODED_SYNTH_MODS)
		.withHotReload()
		.withCrossReference(LinkType::Module, "HardcodedSynth", "host module that exposes these modulation slots")
		.withCrossReference(LinkType::Scriptnode, "core.extra_mod", "node that reads the modulation signal inside the network")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_SCRIPTNODE_SYNTH_MODS", "equivalent slot count for interpreted scriptnode synthesisers")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MODULATORS_PER_CHAIN", "master cap on modulators that can occupy each slot");

	data["CONFIRM_PRESET_OVERWRITE"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Shows a confirmation popup before overwriting an existing user preset file.")
		.withDescriptionLine("When enabled, saving a user preset on top of an existing file name opens a yes/no dialog asking whether to replace the file or cancel the save. When disabled, the existing file is deleted and replaced silently, which is useful when a scripted save flow already handles overwrite logic through its own UI. Only affects the built-in save path in the HISE IDE and the stock preset browser, not scripted saves that write preset files directly.")
		.withDescriptionLine("> Disable this if your plugin ships a custom preset browser that handles overwrite confirmation itself, otherwise the user sees two confirmation popups in a row.")
		.withDefault(1)
		.withValue(CONFIRM_PRESET_OVERWRITE)
		.withCrossReference(LinkType::UI, "FloatingTiles.PresetBrowser", "preset browser save action is the main caller of the confirmation dialog")
		.withCrossReference(LinkType::Preprocessor, "HISE_OVERWRITE_OLD_USER_PRESETS", "related flag that governs silent overwrite behaviour for shipped user presets in the exported plugin");

	data["DONT_CREATE_USER_PRESET_FOLDER"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Skips the automatic creation of the UserPresets folder on first launch of the exported plugin.")
		.withDescriptionLine("On first launch, an exported plugin normally extracts the embedded user preset bank into a UserPresets folder inside its AppData directory. Enabling this flag suppresses that extraction step entirely, so the folder is never created and no presets are written to disk. This is only useful for non-audio projects (utilities, licensing helpers, installers) that happen to be built with HISE but do not need the preset system at all.")
		.withDescriptionLine("> Only takes effect in exported plugins. Leaving it off is the right choice for every regular instrument or effect build.")
		.withDefault(0)
		.withValue(DONT_CREATE_USER_PRESET_FOLDER)
		.withCrossReference(LinkType::Preprocessor, "DONT_CREATE_EXPANSIONS_FOLDER", "sibling folder-suppression flag that skips the automatic Expansions folder for non-audio utility builds");

	data["DONT_EMBED_FILES_IN_FRONTEND"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Exports the plugin without baking the project's image, audio and sample map assets into the binary.")
		.withDescriptionLine("By default the compile exporter embeds every external file (images, audio files, sample maps, MIDI files, scripts) into the compiled plugin so the binary is self-contained. Enabling this flag skips the embedding step, which produces a much smaller binary and dramatically faster load times, but the plugin will then look for every asset inside its project subfolder at runtime. On iOS builds the flag is forced on automatically because the platform ships external resources alongside the app bundle.")
		.withDescriptionLine("> Not recommended for desktop production builds because the plugin will fail to load any asset that is missing from the end user's file system.")
		.withDefault(0)
		.withValue(DONT_EMBED_FILES_IN_FRONTEND)
		.withCrossReference(LinkType::Preprocessor, "USE_RELATIVE_PATH_FOR_AUDIO_FILES", "companion flag that controls how non-embedded audio files are located at runtime");

	data["HISE_INCLUDE_TEMPO_IN_PLUGIN_STATE"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Stores and restores the current host tempo as part of the plugin's DAW project state.")
		.withDescriptionLine("When enabled, the plugin writes the last known host BPM into the state chunk that the DAW saves with the project, and reads it back when the project is reopened. This is useful for standalone testing and for plugins that override the tempo through the scripting API, because the custom value survives a save/reload cycle. The side effect is a short window at project load where the stored tempo briefly takes effect before the host updates the value on the next audio block, which can cause subtle glitches on patches that are tightly locked to tempo-synced LFOs or delays.")
		.withDescriptionLine("> Disable this if your plugin always follows the host tempo and you prefer clean startups over persisting a custom BPM.")
		.withDefault(1)
		.withValue(HISE_INCLUDE_TEMPO_IN_PLUGIN_STATE)
		.withHotReload()
		.withCrossReference(LinkType::ScriptingApi, "TransportHandler", "host BPM reported through the transport handler is the value that gets persisted")
		.withCrossReference(LinkType::ScriptingApi, "Engine.getHostBpm", "Engine.getHostBpm reflects the restored tempo after a project reload");

	data["HISE_OVERWRITE_OLD_USER_PRESETS"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Silently replaces shipped user presets on disk when the plugin version is newer than the one that wrote them.")
		.withDescriptionLine("Controls how the exported plugin reacts when its bundled user preset bank on disk was written by an older build. When disabled, an existing UserPresets folder is left untouched and the embedded bank is only extracted on first launch. When enabled, the plugin compares the version stamp in an info.json file and, if the current plugin version is newer, rewrites every factory-shipped preset (including any that the end user has modified), while leaving presets that the user created themselves alone unless a name collision occurs.")
		.withDescriptionLine("> The HISE export dialog writes this flag automatically from the 'Overwrite Old User Presets' project setting, so you normally don't need to set it manually in the ExtraDefinitions field.")
		.withDefault(0)
		.withValue(HISE_OVERWRITE_OLD_USER_PRESETS)
		.withAutoConfig()
		.withCrossReference(LinkType::Preprocessor, "READ_ONLY_FACTORY_PRESETS", "related flag that stops the user from modifying shipped presets in the first place")
		.withCrossReference(LinkType::Preprocessor, "CONFIRM_PRESET_OVERWRITE", "related flag that governs the overwrite confirmation on manual saves in the HISE IDE");

	data["HISE_UNDO_INTERVAL"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Milliseconds between undo coalescing ticks for the global undo manager.")
		.withDescriptionLine("Sets the period of the timer that flushes pending undo transactions on the main controller. Rapid parameter changes inside one tick are merged into a single undoable step, so a lower value gives finer-grained undo history at the cost of longer undo stacks, while a higher value coalesces more aggressively and keeps the history shorter. The default of 500 ms matches a comfortable knob-drag granularity for most users; values below about 100 ms start creating one undo step per UI message, which quickly fills the history with noise.")
		.withDescriptionLine("> Sensible range is roughly 100 to 2000 milliseconds. Only changing this is worthwhile if you have specific undo granularity requirements in a large utility project.")
		.withDefault(500)
		.withValue(HISE_UNDO_INTERVAL);

	data["HISE_USE_SYSTEM_APP_DATA_FOLDER"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Stores user presets, audio files and plugin state under the shared system AppData folder rather than the per-user one.")
		.withDescriptionLine("Switches the AppData directory that the exported plugin uses from the current user's application data folder to the system-wide shared folder. This means every user on the machine reads and writes the same preset bank and resource folder, which is useful for multi-user studio setups and for installers that place content in a shared location. Writing to the shared folder usually requires elevated privileges, so the installer and the plugin need to be configured accordingly. The HISE IDE always checks the project setting at runtime regardless of this compile-time value.")
		.withDescriptionLine("> The HISE export dialog writes this flag automatically from the 'Use Global AppData Folder' project setting, so you normally don't need to set it manually in the ExtraDefinitions field.")
		.withDefault(0)
		.withValue(HISE_USE_SYSTEM_APP_DATA_FOLDER)
		.withAutoConfig()
		.withCrossReference(LinkType::Preprocessor, "USE_RELATIVE_PATH_FOR_AUDIO_FILES", "audio files are resolved against the same AppData location selected by this flag");

	data["READ_ONLY_FACTORY_PRESETS"] = Entry()
		.withCategory(Category::PresetAndState)
		.withBrief("Marks every shipped user preset as read-only so users cannot overwrite factory content.")
		.withDescriptionLine("When enabled, the exported plugin tracks the list of file paths that were extracted from the embedded preset bank and expansions, and refuses to overwrite any of them from the preset save path. Users can still save new presets alongside the factory bank, they just cannot replace a shipped preset file. The Engine.isUserPresetReadOnly scripting method reports this status so a custom preset browser can hide or grey out the save button on factory entries.")
		.withDescriptionLine("> The HISE export dialog writes this flag automatically from the 'Read Only Factory Presets' project setting, so you normally don't need to set it manually in the ExtraDefinitions field.")
		.withDefault(0)
		.withValue(READ_ONLY_FACTORY_PRESETS)
		.withAutoConfig()
		.withCrossReference(LinkType::ScriptingApi, "Engine.isUserPresetReadOnly", "Engine.isUserPresetReadOnly reports the read-only status that this flag enables")
		.withCrossReference(LinkType::ScriptingApi, "UserPresetHandler", "scripted preset handler queries the factory path list that this flag populates")
		.withCrossReference(LinkType::Preprocessor, "HISE_OVERWRITE_OLD_USER_PRESETS", "related flag that governs automatic overwriting of shipped presets across plugin versions");

	data["DONT_CREATE_EXPANSIONS_FOLDER"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Disables automatic creation of the Expansions subfolder in the application data directory.")
		.withDescriptionLine("When an exported plugin boots for the first time it normally ensures an Expansions subfolder exists so the expansion handler has somewhere to discover installed content. Setting this flag to 1 suppresses that directory creation, which is useful for projects that either ship without any expansion support or use a bespoke install location managed outside of the plugin. The directory is not deleted if it already exists, only the creation step is skipped.")
		.withDescriptionLine("> Only meaningful when the expansion system is enabled, otherwise no folder would be created anyway.")
		.withDefault(0)
		.withValue(DONT_CREATE_EXPANSIONS_FOLDER)
		.withCrossReference(LinkType::Preprocessor, "HISE_ENABLE_EXPANSIONS", "gates the expansion subsystem whose default folder this flag suppresses")
		.withCrossReference(LinkType::Preprocessor, "DONT_CREATE_USER_PRESET_FOLDER", "sibling flag that suppresses the automatic user preset folder in the same way");

	data["HISE_ALLOW_OFFLINE_ACTIVATION"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Enables an offline activation path in the copy protection flow.")
		.withDescriptionLine("When enabled the unlocker accepts an activation response file generated on a separate, internet-connected machine instead of performing a live server round trip. The end user requests a challenge from the plugin, submits it to the licence server through a browser, and loads the returned response back into the plugin to finalise registration. This is typically required for studio machines that are permanently offline.")
		.withDescriptionLine("> Only takes effect when copy protection is compiled in.")
		.withDefault(0)
		.withValue(HISE_ALLOW_OFFLINE_ACTIVATION)
		.withCrossReference(LinkType::Preprocessor, "USE_COPY_PROTECTION", "enables the licensing subsystem that this flag extends with an offline path")
		.withCrossReference(LinkType::ScriptingApi, "Unlocker", "scripting surface that drives the activation flow affected by this flag");

	data["HISE_ENABLE_EXPANSIONS"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Compiles the expansion handler and related infrastructure into the plugin.")
		.withDescriptionLine("Expansions are optional content packs that can be loaded at runtime from a dedicated folder, each carrying their own samples, presets, and images. Turning this on activates the expansion handler so projects can enumerate, load, and protect expansion packs via the scripting API. Disabling it strips the related code paths and is appropriate for projects that ship as a single fixed instrument.")
		.withDescriptionLine("> The guard is applied locally in the build rather than through the existing compile-time infrastructure.")
		.withDefault(1)
		.withValue(HISE_ENABLE_EXPANSIONS)
		.withVestigal()
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_UNLOCKER_FOR_EXPANSIONS", "routes expansion protection through the main unlocker when expansions are active")
		.withCrossReference(LinkType::Preprocessor, "DONT_CREATE_EXPANSIONS_FOLDER", "suppresses the default folder that this subsystem would otherwise create")
		.withCrossReference(LinkType::ScriptingApi, "ExpansionHandler", "scripting entry point exposed only when this subsystem is compiled in")
		.withCrossReference(LinkType::ScriptingApi, "Expansion", "represents a single pack managed by the subsystem this flag enables")
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_XML_FOR_HXI", "container format for encrypted expansions produced by this subsystem");

	data["HISE_INCLUDE_UNLOCKER_OVERLAY"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Includes the built-in registration overlay UI for the copy protection flow.")
		.withDescriptionLine("This overlay presents the licence entry dialog and activation status messages on top of the plugin interface whenever the copy protection state requires user input. Disabling it removes the default UI so a project can implement a fully custom registration screen using the script copy protection API. Projects that want the standard look without extra work should leave this enabled.")
		.withDescriptionLine("> Has no effect when copy protection is not compiled in.")
		.withDefault(1)
		.withValue(HISE_INCLUDE_UNLOCKER_OVERLAY)
		.withCrossReference(LinkType::Preprocessor, "USE_COPY_PROTECTION", "provides the licensing state that this overlay visualises")
		.withCrossReference(LinkType::Preprocessor, "USE_SCRIPT_COPY_PROTECTION", "alternative registration UI path used when the built-in overlay is disabled");

	data["HISE_USE_UNLOCKER_FOR_EXPANSIONS"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Routes expansion authorisation through the main plugin unlocker.")
		.withDescriptionLine("When enabled each expansion checks its encryption key against the licence information held by the main unlocker instead of maintaining its own per-expansion key. This simplifies the activation flow for the end user because a single serial grants access to both the base plugin and its expansions. When disabled each expansion is protected by its own independent key stored in the expansion metadata.")
		.withDescriptionLine("> Ignored when the expansion subsystem is not compiled in.")
		.withDefault(0)
		.withValue(HISE_USE_UNLOCKER_FOR_EXPANSIONS)
		.withHotReload()
		.withCrossReference(LinkType::Preprocessor, "USE_COPY_PROTECTION", "provides the unlocker instance that expansions are routed through")
		.withCrossReference(LinkType::Preprocessor, "HISE_ENABLE_EXPANSIONS", "gates the expansion subsystem whose authorisation path this flag redirects")
		.withCrossReference(LinkType::ScriptingApi, "Unlocker", "licence object consulted for each expansion when this flag is set")
		.withCrossReference(LinkType::ScriptingApi, "ExpansionHandler", "manages the expansions whose protection path is altered by this flag")
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_XML_FOR_HXI", "container format for the encrypted expansions that this authorisation path gates");

	data["JUCE_ALLOW_EXTERNAL_UNLOCK"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Allows third-party storefronts to unlock the plugin using their own licence tokens.")
		.withDescriptionLine("When enabled the unlocker accepts activation tokens issued by external distribution platforms in addition to the built-in licence server. This is required when the plugin is sold through marketplaces such as MuseHub or Beatport, which manage activation through their own account systems. Disabling it restricts registration to the first-party licence flow only.")
		.withDescriptionLine("> Local guard is added at the top of the translation unit that consumes this flag.")
		.withDefault(0)
		.withValue(JUCE_ALLOW_EXTERNAL_UNLOCK)
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_MUSEHUB", "enables the MuseHub storefront whose external activation relies on this flag")
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_BEATPORT", "enables the Beatport storefront whose external activation relies on this flag")
		.withCrossReference(LinkType::ScriptingApi, "Unlocker", "scripting surface that receives the external token accepted by this flag");

	data["JUCE_USE_BETTER_MACHINE_IDS"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Switches the machine identification to a more stable hardware fingerprint.")
		.withDescriptionLine("The default machine identifier can change after OS reinstalls or hardware driver updates, which forces legitimate users to reactivate more often than necessary. Enabling this flag selects an alternative fingerprint that is more resilient to such changes while still uniquely identifying the host. Existing activations remain tied to the old identifier so this should be set before the first public release.")
		.withDescriptionLine("> Changing this value after a release will invalidate previously issued licences.")
		.withDefault(0)
		.withValue(JUCE_USE_BETTER_MACHINE_IDS)
		.withCrossReference(LinkType::Preprocessor, "USE_COPY_PROTECTION", "provides the licensing subsystem that consumes the machine identifier")
		.withCrossReference(LinkType::ScriptingApi, "Unlocker", "exposes the machine identifier to scripts for activation messages");

	data["USE_COPY_PROTECTION"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Compiles the licence verification and activation subsystem into the plugin.")
		.withDescriptionLine("Enabling this flag links the unlocker, overlay, and server communication code that challenge the user for a licence key on first launch and persist the result for subsequent sessions. Without it the plugin starts in an always-authorised state and no registration UI is shown, which is appropriate for free or internal builds. The flag is substituted by the compile exporter so it can differ between development and release builds of the same project.")
		.withDescriptionLine("> Many downstream licensing flags are inert unless this is enabled.")
		.withDefault(0)
		.withValue(USE_COPY_PROTECTION)
		.withAutoConfig()
		.withCrossReference(LinkType::Preprocessor, "USE_SCRIPT_COPY_PROTECTION", "adds the scripting layer on top of this subsystem")
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_UNLOCKER_OVERLAY", "supplies the default registration UI for this subsystem")
		.withCrossReference(LinkType::Preprocessor, "HISE_ALLOW_OFFLINE_ACTIVATION", "extends this subsystem with an offline activation path")
		.withCrossReference(LinkType::Preprocessor, "JUCE_USE_BETTER_MACHINE_IDS", "controls the fingerprint used by this subsystem to identify the host")
		.withCrossReference(LinkType::ScriptingApi, "Unlocker", "scripting surface exposed when this subsystem is compiled in");

	data["USE_SCRIPT_COPY_PROTECTION"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Exposes the copy protection flow to HISEScript for custom registration UIs.")
		.withDescriptionLine("This flag forwards the licence state, challenge, and activation response through the scripting API so a project can present a fully custom registration screen instead of relying on the built-in overlay. It is typically paired with a bespoke interface that matches the plugin branding and handles purchase links or trial logic. The underlying licensing checks still run natively, only the presentation layer moves into script.")
		.withDescriptionLine("> Has no effect unless the licensing subsystem is also compiled in.")
		.withDefault(0)
		.withValue(USE_SCRIPT_COPY_PROTECTION)
		.withCrossReference(LinkType::Preprocessor, "USE_COPY_PROTECTION", "provides the underlying licensing state that this scripting layer exposes")
		.withCrossReference(LinkType::Preprocessor, "HISE_INCLUDE_UNLOCKER_OVERLAY", "default UI path replaced by custom scripts when this flag is enabled")
		.withCrossReference(LinkType::ScriptingApi, "Unlocker", "scripting object that surfaces the licence state enabled by this flag");

	data["MIN_FILTER_FREQ"] = Entry()
		.withCategory(Category::DspAndFilters)
		.withBrief("Historical knob for the lowest cutoff frequency any filter can be tuned to.")
		.withDescriptionLine("The original intent was to raise or lower the hard floor that every filter clamps its frequency parameter against so that projects working below 20 Hz could still open the cutoff all the way down. The macro is still defined but the filter limit values are hard-coded to 20 Hz in the same header, so setting it has no effect on the audible frequency range. It is kept around only so that older user projects which list it in their ExtraDefinitions still compile.")
		.withDefault(20)
		.withValue((int)MIN_FILTER_FREQ)
		.withVestigal();

	data["HISE_USE_XML_FOR_HXI"] = Entry()
		.withCategory(Category::LicensingAndExpansions)
		.withBrief("Stores encrypted expansion (.hxi) files as XML text instead of a compact binary value tree.")
		.withDescriptionLine("Switches the container format used by the encrypted expansion workflow so that the resulting .hxi files are human-readable XML rather than the default compact value-tree binary. Binary is smaller and loads faster at runtime; XML is noticeably larger but lets you diff the contents during development and inspect what a given expansion actually ships. The format is chosen per-write, so existing binary expansions keep loading regardless of this flag; only newly encoded expansions pick up the change.")
		.withDescriptionLine("> Read at runtime, so changing it in the Extra Definitions and re-encoding an expansion is enough without rebuilding HISE itself.")
		.withDefault(0)
		.withValue(HISE_USE_XML_FOR_HXI)
		.withHotReload()
		.withCrossReference(LinkType::ScriptingApi, "ExpansionHandler", "encrypted expansion workflow driven by the expansion handler uses the format selected by this flag")
		.withCrossReference(LinkType::ScriptingApi, "Expansion", "individual expansion objects serialise into the container format selected by this flag")
		.withCrossReference(LinkType::Preprocessor, "HISE_ENABLE_EXPANSIONS", "only meaningful in builds that compile the expansion subsystem in")
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_UNLOCKER_FOR_EXPANSIONS", "companion flag that governs how encrypted expansions are authorised");

	data["HISE_ENABLE_FULL_CONTROL_RATE_PITCH_MOD"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Historical switch for retaining full-resolution pitch modulation on the control-rate path.")
		.withDescriptionLine("The original intent was to keep pitch modulation at audio rate even when the rest of the modulation system ran at the reduced control rate, so that vibrato and pitch-envelope detail did not get stepped by the downsampling. The macro is still defined with a hard-coded value of zero and no code reads it anywhere, so toggling it has no effect on the pitch modulation path or on audio output. It is kept only so that older user projects which list it in their ExtraDefinitions field still compile.")
		.withDefault(0)
		.withValue(HISE_ENABLE_FULL_CONTROL_RATE_PITCH_MOD)
		.withVestigal();

	data["HISE_EVENT_RASTER"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Downsampling factor for the modulation and event-timestamp raster.")
		.withDescriptionLine("Sets the divisor that every modulator, envelope and scriptnode modchain uses to step down from the audio rate, and also defines the sample grid that MIDI note, controller and timer events get aligned to inside a block. The default is 8 for instrument plugins (modulation runs at sampleRate / 8, which is about 5.5 kHz at 44.1 kHz) and 1 for effect plugins where no downsampling happens. Setting it to 4, 2 or 1 in instrument projects gives finer modulation resolution at the cost of proportionally more CPU in every modulator and scriptnode modchain, which is the standard workaround for LFO aliasing above roughly 30 Hz.")
		.withDescriptionLine("> The host buffer size must be an integer multiple of this value or the engine refuses to render. Set it consistently in both the HISE build and the exported plugin, and keep it a power of two (1, 2, 4 or 8) to stay compatible with the event scheduler.")
		.withDefault(8)
		.withValue(HISE_EVENT_RASTER)
		.withCrossReference(LinkType::Module, "LFO", "LFO accuracy above roughly 30 Hz improves when this divisor is lowered")
		.withCrossReference(LinkType::ScriptingApi, "Engine.getControlRateDownsamplingFactor", "Engine.getControlRateDownsamplingFactor returns this value at runtime")
		.withCrossReference(LinkType::ScriptingApi, "Synth", "sample-accurate timer events on the synth are rastered to this grid")
		.withCrossReference(LinkType::Scriptnode, "container.modchain", "children of a modchain run at sampleRate divided by this factor")
		.withCrossReference(LinkType::Preprocessor, "HISE_MAX_PROCESSING_BLOCKSIZE", "maximum block size must stay a multiple of this raster")
		.withCrossReference(LinkType::Preprocessor, "HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE", "triggers a user-facing overlay when the host buffer size is not a multiple of this raster");

	data["HISE_MAX_PROCESSING_BLOCKSIZE"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Upper ceiling for the internal audio block size used by the rendering loop.")
		.withDescriptionLine("Caps the largest block size that the engine will process in a single pass; if the host calls with a bigger buffer, the render callback splits it into chunks no larger than this value before dispatching to the voice renderer and effect chain. Bigger values mean fewer dispatch overheads but also grow every internal scratch buffer that is sized to the maximum block, so very high ceilings waste memory and can introduce streaming engine edge cases. The default of 512 is the sweet spot for the built-in filter and polyphonic effect paths; the setMaximumBlockSize scripting method clamps its argument to the range 16 to this value.")
		.withDescriptionLine("> Keep this as a power of two and at least equal to the event raster, otherwise block splitting will not align cleanly with modulation updates.")
		.withDefault(512)
		.withValue(HISE_MAX_PROCESSING_BLOCKSIZE)
		.withCrossReference(LinkType::ScriptingApi, "Engine.setMaximumBlockSize", "Engine.setMaximumBlockSize clamps its argument to this ceiling")
		.withCrossReference(LinkType::Module, "PolyphonicFilter", "filter quality and internal scratch buffers are sized to this block cap")
		.withCrossReference(LinkType::Preprocessor, "HISE_EVENT_RASTER", "block size must stay a multiple of the event raster")
		.withCrossReference(LinkType::Preprocessor, "HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE", "illegal buffer sizes caught by the raster check raise the overlay message when this flag is on");

	data["HISE_SILENCE_THRESHOLD_DB"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Threshold in negative decibels below which the engine treats a parameter value or envelope level as silent.")
		.withDescriptionLine("Sets the gain value that parameter smoothers, the polyphonic envelope decay stage and the sampler voice crossfade logic use to decide that a ramp or decay has effectively reached its target. The stored value is interpreted as the negative dB, so the default of 60 means anything below -60 dB counts as silence. Lowering it (for example to 80 or 90) makes envelope decays run closer to true zero at the cost of a slightly longer voice tail; raising it shortens tails but can cut off quiet release material before it has finished. The value is a compile-time constant baked into every generator that uses the silence test.")
		.withDescriptionLine("> The voice kill-fade-out time is still calibrated against a fixed -60 dB target regardless of this setting, so lowering the threshold can extend the actual kill fade beyond the scripted value.")
		.withDefault(60)
		.withValue(HISE_SILENCE_THRESHOLD_DB)
		.withCrossReference(LinkType::Module, "AHDSR", "envelope decay stage transitions to sustain or idle when the value falls below this threshold")
		.withCrossReference(LinkType::Module, "StreamingSampler", "voice crossfade smoothing uses this threshold to decide when interpolation can stop");

	data["HISE_SUSPENSION_TAIL_MS"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Length in milliseconds that an effect keeps rendering after its input has gone silent before suspending itself.")
		.withDescriptionLine("Defines the tail window that every master and voice effect with suspend-on-silence enabled waits out before freezing its processing and outputting silence directly. The value is converted into a callback count based on the current sample rate and block size, so longer tails cover reverbs and long delays whose output keeps sounding well after the input stops, while shorter tails save more CPU on dry effects. The default of 500 ms is a compromise that works for most mixed effect chains; raise it to 2000 or higher for convolution reverbs with long impulse responses, lower it towards 100 for dry clippers or EQs.")
		.withDescriptionLine("> Read from the project's Extra Definitions at runtime, so changing the value in the Preferences window takes effect on the next prepareToPlay without a full HISE rebuild.")
		.withDefault(500)
		.withValue(HISE_SUSPENSION_TAIL_MS)
		.withHotReload()
		.withCrossReference(LinkType::Module, "Convolution", "reverb tail needs a suspension window long enough to cover the impulse response")
		.withCrossReference(LinkType::Module, "Delay", "delay tail needs a suspension window at least as long as the maximum delay time")
		.withCrossReference(LinkType::ScriptingApi, "Effect.isSuspendedOnSilence", "isSuspendedOnSilence flag on effects uses this window to decide when to freeze processing");

	data["HI_DONT_SEND_ATTRIBUTE_UPDATES"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Suppresses the async UI notification when a script changes a module parameter in an exported plugin.")
		.withDescriptionLine("Every call to a scripted setAttribute normally posts an async message so that any attached UI controls and plotters update to reflect the new value. Enabling this flag replaces that notification with a silent write, which can noticeably reduce message-thread pressure in plugins that drive parameters from tight control-rate scripts. UI controls bound directly to those parameters will stop updating visually until the user interacts with them again or the script pushes an explicit repaint. Only applies to exported plugin builds; the HISE IDE always sends notifications so that parameter edits stay visible in the editor.")
		.withDescriptionLine("> Only enable this when scripted parameter changes are verifiably clogging the message thread, because the lost UI feedback will confuse users interacting with automated controls.")
		.withDefault(0)
		.withValue(HI_DONT_SEND_ATTRIBUTE_UPDATES);

	data["USE_HARD_CLIPPER"] = Entry()
		.withCategory(Category::AudioProcessing)
		.withBrief("Replaces the soft output safety clip with a hard brickwall limit at the final render stage.")
		.withDescriptionLine("Controls the very last step in the master render callback where output samples are kept inside a sane range. With the flag off, iOS builds still clip to the range from -1.0 to 1.0 to avoid the nasty digital distortion that mobile audio hardware produces for out-of-range samples, and desktop builds pass the signal through untouched. With the flag on, every platform receives a hard brickwall clip at unity gain regardless of the device, which is useful for defensive plugin builds that must never overshoot full scale. The clip is always at 0 dBFS and is not configurable.")
		.withDescriptionLine("> A hard clip adds harmonic distortion to every sample above 1.0, so only enable this if you actually need the safety net. Use a dedicated limiter on the master chain for musical peak control.")
		.withDefault(0)
		.withValue(USE_HARD_CLIPPER);

	data["HISE_DEACTIVATE_OVERLAY"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Hides the built-in dark error overlay so a project can present its own UI for engine errors.")
		.withDescriptionLine("Controls the default value of the overlay that the engine draws on top of the interface whenever something goes wrong (samples not installed, licence missing, incorrect audio folder and similar). When enabled, the overlay is suppressed entirely and the user only sees whatever the project itself shows in response. When disabled, the stock overlay with its default message and buttons is drawn automatically. The scripted error handler object flips the same switch from HISEScript, so creating an ErrorHandler from a script has the same end result as setting this flag at compile time.")
		.withDescriptionLine("> Suppressing the overlay without providing a scripted replacement leaves the user with no feedback when samples are missing or the licence has expired, so only set this when you implement custom error handling in the interface.")
		.withDefault(0)
		.withValue(HISE_DEACTIVATE_OVERLAY)
		.withCrossReference(LinkType::ScriptingApi, "ErrorHandler", "scripted error handler disables the same overlay at runtime and should replace it with custom UI");

	data["HISE_DEFAULT_OPENGL_VALUE"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Initial OpenGL rendering state when the plugin is launched for the first time on a machine.")
		.withDescriptionLine("Picks the default for the Open GL toggle in the audio settings window and the standalone preferences, so a fresh installation starts either with hardware-accelerated rendering on or off depending on this value. Only consulted when the user has not yet stored an OPEN_GL choice in their settings file; subsequent launches read whatever the user selected last. Ignored entirely unless OpenGL support has been compiled in through the companion flag.")
		.withDescriptionLine("> The user can still flip the renderer in the settings window at runtime, so this only influences the very first launch on a given machine.")
		.withDefault(1)
		.withValue(HISE_DEFAULT_OPENGL_VALUE)
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_OPENGL_FOR_PLUGIN", "only consulted when OpenGL support is actually compiled into the plugin")
		.withCrossReference(LinkType::ScriptingApi, "Settings.isOpenGLEnabled", "Settings.isOpenGLEnabled reports the persisted value that this default seeds");

	data["HISE_USE_CUSTOM_ALERTWINDOW_LOOKANDFEEL"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Lets the project supply its own look and feel for JUCE alert windows instead of the HISE default.")
		.withDescriptionLine("By default HISE installs an alert-window look and feel that paints native popups (confirmation dialogs, error alerts, prompt boxes) in the same dark style as the rest of the editor, and falls back to the currently active scripted look and feel when one is attached. Enabling this flag skips that registration so the project can install a bespoke alert window style through C++ and avoid the stock HISE styling. Only relevant for projects that extend HISE on the C++ side; scripts cannot substitute the alert window style through this flag alone.")
		.withDescriptionLine("> With this enabled, nothing styles the alert windows until your own code installs a replacement, so confirm dialogs will fall back to the default JUCE appearance.")
		.withDefault(0)
		.withValue(HISE_USE_CUSTOM_ALERTWINDOW_LOOKANDFEEL)
		.withCrossReference(LinkType::ScriptingApi, "ScriptLookAndFeel", "scripted look and feel is no longer consulted for alert windows when the project provides its own override");

	data["HISE_USE_MOUSE_WHEEL_FOR_TABLE_CURVE"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Legacy default that lets the mouse wheel adjust the curve tension on every table editor.")
		.withDescriptionLine("Sets the initial value of the useMouseWheelForCurve property on every table editor in the project so the mouse wheel bends the curve between two points instead of scrolling the parent viewport. The property is also exposed per instance in HISEScript, so the preprocessor only controls the starting value for components that have not overridden it. Leave this at the default unless you are maintaining a project built against older HISE versions where the mouse wheel always adjusted the curve.")
		.withDescriptionLine("> Prefer setting the useMouseWheelForCurve property on individual table components instead, because flipping this globally hijacks the scroll wheel for every table in the interface.")
		.withDefault(0)
		.withValue(HISE_USE_MOUSE_WHEEL_FOR_TABLE_CURVE)
		.withCrossReference(LinkType::UI, "Components.ScriptTable", "provides the default for the component's useMouseWheelForCurve property");

	data["HISE_USE_OPENGL_FOR_PLUGIN"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Compiles OpenGL rendering support into the plugin so the user can enable hardware-accelerated drawing.")
		.withDescriptionLine("Links the JUCE OpenGL code paths into the build and exposes the Open GL toggle in the settings window, which routes the interface rendering through the GPU instead of the CPU rasteriser. This is a significant quality-of-life improvement for large interfaces with animated panels or vector graphics, but adds a graphics driver dependency and a few megabytes to the binary, and can trigger driver-specific issues on older Windows GPUs. With the flag off, the toggle is absent and the plugin always draws through the software renderer regardless of the companion default value.")
		.withDescriptionLine("> Must be set consistently in both the HISE build and the exported plugin. On iOS OpenGL support is always present regardless of this flag.")
		.withDefault(0)
		.withValue(HISE_USE_OPENGL_FOR_PLUGIN)
		.withCrossReference(LinkType::Preprocessor, "HISE_DEFAULT_OPENGL_VALUE", "selects whether OpenGL starts on or off when the user launches the plugin for the first time")
		.withCrossReference(LinkType::ScriptingApi, "Settings.isOpenGLEnabled", "Settings.isOpenGLEnabled reports the active renderer state only when this integration is compiled in");

	data["HI_ENABLE_EXTERNAL_CUSTOM_TILES"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Enables a registration hook for custom floating tile panel types supplied in C++.")
		.withDescriptionLine("Activates the REGISTER_EXTERNAL_FLOATING_TILE macro and the registerExternalPanelTypes hook, so a project that subclasses HISE on the C++ side can add its own panel types to the floating tile factory and make them available as ContentType options in the interface. With the flag off, the registration hook compiles out and only the built-in panel types are offered. Only relevant for custom HISE builds; projects that stay inside the scripting API never need to touch this.")
		.withDefault(0)
		.withValue(HI_ENABLE_EXTERNAL_CUSTOM_TILES)
		.withCrossReference(LinkType::ScriptingApi, "ScriptFloatingTile", "custom panel types registered through this hook become available as ContentType options on a script floating tile");

	data["USE_LATO_AS_DEFAULT"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Uses the bundled Lato typeface as the default system font throughout the interface.")
		.withDescriptionLine("Selects which embedded typeface HISE binds to the GLOBAL_FONT and GLOBAL_BOLD_FONT macros and to the Linux font handler, so every label, popup, combobox and debug console picks it up unless a script explicitly overrides the font. The default since mid-2019 has been Lato Regular and Lato Bold, which read better at small sizes and ship with a wider glyph coverage. Setting this to 0 reverts to the older Oxygen typeface that shipped with early HISE releases, which is only useful for maintaining visual consistency with a project that was laid out against Oxygen metrics.")
		.withDescriptionLine("> Oxygen and Lato have different x-heights and advance widths, so flipping this in an existing project will reflow every label that uses the global font.")
		.withDefault(1)
		.withValue(USE_LATO_AS_DEFAULT);

	data["USE_SPLASH_SCREEN"] = Entry()
		.withCategory(Category::UiAndGraphics)
		.withBrief("Shows a SplashScreen image while the standalone app loads its samples in the background.")
		.withDescriptionLine("When enabled, the standalone build of the exported plugin displays a bundled splash bitmap (SplashScreen.png for desktop and iPad, SplashScreeniPhone.png for iPhone) while the plugin loads its sample content and initialises the audio engine. The image has to be present in the project's Images folder for the embed to pick it up; without it the plugin will fail to compile. Only affects standalone targets, because plugin windows in a DAW do not present a splash screen. The HISE export dialog writes this flag automatically from the 'Use Splash Screen' project setting.")
		.withDescriptionLine("> Only meaningful for standalone iOS and desktop exports. Regular VST, AU and AAX builds never display the splash image regardless of this flag.")
		.withDefault(0)
		.withValue(USE_SPLASH_SCREEN)
		.withAutoConfig();

	data["HISE_ENABLE_MIDI_INPUT_FOR_FX"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Routes incoming MIDI data into the MIDI processor chain of an exported effect plugin.")
		.withDescriptionLine("Only relevant when the project is exported as an effect plugin. By default an FX plugin ignores MIDI entirely, because most DAWs do not route MIDI to audio track inserts and the keyboard-state processing step is pure overhead in that case. Enabling this flag makes the plugin advertise a MIDI input, consume note and controller messages from the host and run them through the master chain's MIDI processors and the MIDI automation handler. Use this for effects that need keytracking, note-triggered modulation or host-side MIDI automation of a plugin parameter.")
		.withDescriptionLine("> The HISE export dialog writes this flag automatically from the 'Enable Midi Input For Effect Plugins' project setting, so you normally do not need to set it manually in the ExtraDefinitions field.")
		.withDefault(0)
		.withValue(HISE_ENABLE_MIDI_INPUT_FOR_FX)
		.withAutoConfig()
		.withCrossReference(LinkType::Preprocessor, "PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN", "sibling FX export flag that keeps sound generators running so MIDI-triggered modulators still produce values")
		.withCrossReference(LinkType::Preprocessor, "HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION", "MIDI channel filtering only takes effect once MIDI actually reaches the FX plugin");

	data["HISE_ENABLE_MIDI_LEARN"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Lets the end user assign MIDI CC numbers to knobs and buttons through the right-click menu.")
		.withDescriptionLine("Controls whether the MIDI Learn entry appears on the context menu of macro-controlled UI components and whether incoming CC messages can be bound to a control at runtime. The default is on for instrument plugins and the standalone app and off for exported effect plugins, because an effect plugin normally does not receive MIDI anyway. Disable this on an instrument build only if you ship your own CC-to-parameter workflow through scripting and want to hide the stock assignment UI.")
		.withDescriptionLine("> If you turn this on for an exported effect plugin, pair it with the MIDI input flag so that CC messages actually reach the automation handler in the first place.")
		.withDefault(1)
		.withValue(HISE_ENABLE_MIDI_LEARN)
		.withCrossReference(LinkType::ScriptingApi, "MidiAutomationHandler", "MIDI learn assignments are stored and queried through this scripting object")
		.withCrossReference(LinkType::Preprocessor, "HISE_ENABLE_MIDI_INPUT_FOR_FX", "effect plugins need MIDI input enabled before the learn menu can receive CC messages");

	data["HISE_MACROS_ARE_PLUGIN_PARAMETERS"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Publishes every macro control as a dedicated plugin parameter in front of the scripted automation slots.")
		.withDescriptionLine("When enabled, the exported plugin advertises one plugin parameter per active macro slot before any custom automation slot or scripted plugin parameter, which lets the host automate macros directly instead of routing through a UI component. The plugin parameter index reported to the host changes because the macro slots occupy the first positions, and macro state no longer restores from user presets when the preset system is running in exclusive mode (which matches how plugin parameters are normally managed by the DAW rather than by the preset). Combine this with a reduced macro count to keep the parameter list compact.")
		.withDescriptionLine("> Read at runtime from the Extra Definitions, so no HISE rebuild is required. Pair it with HISE_NUM_MACROS so the number of exposed plugin parameters matches the macros you actually use.")
		.withDefault(0)
		.withValue(HISE_MACROS_ARE_PLUGIN_PARAMETERS)
		.withHotReload()
		.withCrossReference(LinkType::ScriptingApi, "MacroHandler", "macro slots exposed as plugin parameters are the ones managed through this scripting object")
		.withCrossReference(LinkType::Module, "MacroModulationSource", "every macro slot is mirrored as a front-of-list plugin parameter when this is on")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MACROS", "determines how many macro plugin parameters the host sees");

	data["HISE_NUM_MACROS"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Number of active macro control slots that the project exposes on the master chain.")
		.withDescriptionLine("Sets how many macro slots are visible to scripts, to the macro handler, to the Macro Modulation Source synthesiser and to the macro plugin-parameter publishing path. The default of 8 matches the original HISE layout and the UI panels that show a fixed eight-knob strip; values between 1 and the project-wide ceiling let you trim a compact product or scale up to a modular rig with many assignment targets. The value is read at runtime, so changing it in the Extra Definitions and reopening the project is enough to see the new slot count without a HISE rebuild.")
		.withDescriptionLine("> Must not exceed the project-wide macro ceiling of 64, otherwise compilation fails with a hard error.")
		.withDefault(8)
		.withValue(HISE_NUM_MACROS)
		.withHotReload()
		.withCrossReference(LinkType::ScriptingApi, "MacroHandler", "slot count seen by every scripting call that enumerates macros")
		.withCrossReference(LinkType::ScriptingApi, "Engine.setFrontendMacros", "setFrontendMacros expects a name list sized to this value")
		.withCrossReference(LinkType::Module, "MacroModulationSource", "chain count of the macro modulation source synthesiser matches this value")
		.withCrossReference(LinkType::Module, "MacroModulator", "macro modulator slot index is validated against this value")
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MAX_MACROS", "hard upper ceiling that this value must not exceed")
		.withCrossReference(LinkType::Preprocessor, "HISE_MACROS_ARE_PLUGIN_PARAMETERS", "determines how many plugin parameters are published when macros become plugin parameters");

	data["HISE_NUM_MAX_MACROS"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Compile-time ceiling on the number of macro slots that internal fixed-size arrays reserve space for.")
		.withDescriptionLine("Sets the fixed array size used for macro bookkeeping inside the main controller and the macro modulation source, so that the macro slot count can be changed at runtime without reallocating per-slot state. The default of 64 covers every realistic macro layout and only needs to be raised if a project genuinely needs more than 64 simultaneous macro assignments. Raising it grows every per-macro array proportionally, so do not inflate this value beyond what the project actually uses.")
		.withDescriptionLine("> Must be at least as large as the active macro count, otherwise compilation fails with a hard error. This value is baked into the binary at compile time and must match between the HISE build and any project DLL or exported plugin.")
		.withDefault(64)
		.withValue(HISE_NUM_MAX_MACROS)
		.withCrossReference(LinkType::Preprocessor, "HISE_NUM_MACROS", "active macro slot count that must stay at or below this ceiling");

	data["HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Enables per-channel MIDI CC assignment so the same CC number on different channels can control different parameters.")
		.withDescriptionLine("By default the MIDI automation handler ignores the channel field of an incoming CC message, so CC 1 on channel 1 and CC 1 on channel 2 both drive the same assigned control. Enabling this flag switches the automation iterator to a channel-aware mode, stores a channel field alongside every assignment and makes the JSON automation objects include that field. This lets you build a plugin that responds to the same CC number differently depending on which MIDI channel carries it, for example separate knobs locked to channels 1 and 2 of a multi-timbral controller.")
		.withDescriptionLine("> Read at runtime from the Extra Definitions, so no HISE rebuild is required. Enabling MIDI learn while this is on retains assignments on other channels and only replaces connections that share the new assignment's channel or that were stored as Omni.")
		.withDefault(0)
		.withValue(HISE_USE_MIDI_CHANNELS_FOR_AUTOMATION)
		.withHotReload()
		.withCrossReference(LinkType::ScriptingApi, "MidiAutomationHandler", "channel field on the JSON automation object only appears when this flag is on")
		.withCrossReference(LinkType::Preprocessor, "HISE_ENABLE_MIDI_LEARN", "channel filtering is applied on top of the MIDI learn assignment workflow");

	data["HI_NUM_MIDI_AUTOMATION_SLOTS"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Historical switch that was intended to size the MIDI automation slot table.")
		.withDescriptionLine("The original purpose was to set the number of MIDI controller automation slots allocated by the MIDI automation handler. The macro is still defined with a hard-coded value of 8 but no code reads it anywhere, so toggling or redefining it has no effect on the number of available CC assignments or on how the automation handler stores its data. It is kept around only so that older user projects which list it in their ExtraDefinitions still compile.")
		.withDefault(8)
		.withValue(HI_NUM_MIDI_AUTOMATION_SLOTS)
		.withVestigal();

	data["USE_MIDI_AUTOMATION_MIGRATION"] = Entry()
		.withCategory(Category::AutomationAndMacros)
		.withBrief("Opt-in flag for supplying a custom old-to-new automation index conversion when loading legacy user presets.")
		.withDescriptionLine("When a user preset from an older plugin version is loaded, the MIDI automation restore path calls a conversion helper that maps the stored integer automation index onto the current identifier-based scheme. The default implementation does nothing and returns an empty identifier, so legacy CC assignments are silently dropped on load. Enabling this flag and providing your own implementation of the conversion function lets you keep CC assignments working across a parameter reshuffle, for example when you have renamed or reordered controls between plugin versions.")
		.withDescriptionLine("> Only useful if you also supply the custom conversion function in your project code. Without that, enabling the flag has no effect beyond skipping the default empty-identifier fallback.")
		.withDefault(0)
		.withValue(USE_MIDI_AUTOMATION_MIGRATION)
		.withCrossReference(LinkType::ScriptingApi, "MidiAutomationHandler", "legacy user preset migration path for CC assignments is governed by this flag");

	data["CRASH_ON_GLITCH"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Historical switch for crashing the plugin on the first audio drop-out or burst above +36 dB.")
		.withDescriptionLine("The original intent was to hard-crash the application as soon as the glitch detector spotted an output sample louder than +36 dB or a buffer-level drop-out, so that a debugger attached to the process would break at the exact stack frame responsible. The macro is still defined but no code path reads it anywhere, so toggling it has no effect on how glitches or bursts are handled at runtime. It is kept around so that older user projects which list it in their ExtraDefinitions field keep compiling.")
		.withDefault(0)
		.withValue(CRASH_ON_GLITCH)
		.withVestigal();

	data["ENABLE_ALL_PEAK_METERS"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Collects output-level data on every module so that scripted peak meters keep working in an exported plugin.")
		.withDescriptionLine("Every modulator, effect and sound generator writes its current output magnitude into a per-module meter slot while this flag is on, which is what feeds the module header meters in the HISE IDE, the MatrixPeakMeter tile in an exported plugin and every timer-based getCurrentLevel call on a modulator. Turning it off saves the per-block magnitude scan on every module in the signal path and shrinks the per-module state, but any scripted peak readback or meter tile will read zero and modulator graphs like the animated AHDSR envelope stop updating. Needed in exported plugins because the default project template disables it to save CPU.")
		.withDescriptionLine("> Add `ENABLE_ALL_PEAK_METERS=1` to the ExtraDefinitions field before exporting if your plugin uses peak meters, scripted level readback or modulator graph displays, otherwise they will read zero at runtime.")
		.withDefault(1)
		.withValue(ENABLE_ALL_PEAK_METERS)
		.withCrossReference(LinkType::Module, "LFO", "getCurrentLevel returns zero in exported plugins unless per-module peak collection is enabled")
		.withCrossReference(LinkType::Module, "AHDSR", "animated envelope graph display relies on the per-module peak collection")
		.withCrossReference(LinkType::Module, "Convolution", "wet input and output magnitudes used by the module meter are only scanned when this flag is on")
		.withCrossReference(LinkType::Scriptnode, "core.peak", "MatrixPeakMeter tile and scriptnode peak readback fall back to zero when per-module collection is off")
		.withCrossReference(LinkType::Preprocessor, "ENABLE_PEAK_METERS_FOR_GAIN_EFFECT", "companion flag for the simple gain effect which scans its output magnitude through a separate switch");

	data["ENABLE_CPU_MEASUREMENT"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Measures per-block rendering time so the CPU meter and sample loading thread report live load figures.")
		.withDescriptionLine("Wraps every audio render pass in a high-resolution benchmark so that the main CPU usage tile and the sample-loader busy-ratio readout have real numbers to display. The sampler streaming thread also uses the same timing hooks to report how much of each sample load window was spent doing work. Disabling it removes two high-resolution timer reads per render block and one more per streaming job, which is measurable on very small buffer sizes but usually negligible; the trade-off is that the CPU meter reads zero and the streaming diagnostics report no busy ratio.")
		.withDefault(1)
		.withValue(ENABLE_CPU_MEASUREMENT);

	data["ENABLE_HOST_INFO"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Pulls transport and tempo information from the host into the engine on every audio block.")
		.withDescriptionLine("Queries the DAW playhead each render block to update the engine's tempo, playing position, time signature and grid tick information so that tempo-synced modulators, transport-aware scripts and the TransportHandler callbacks receive live values. Turning this off skips the playhead query entirely; the engine then falls back to its internal clock and any tempo-synced module will freeze at its last known BPM. Only disable it for a very lean utility build where the plugin must not react to host transport at all, because the cost of the query is small compared to what downstream modules do with the data.")
		.withDefault(1)
		.withValue(ENABLE_HOST_INFO);

	data["ENABLE_PLOTTER"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Historical switch for the per-modulator plotter ring buffer used by the IDE's Plotter panel.")
		.withDescriptionLine("The original purpose was to gate the sample collection that fed the IDE-side Plotter panel so that exported plugins could skip the per-sample push onto the plot ring buffer. The macro is still defined but no code reads it anywhere in the current engine; the project template still writes it to keep older ExtraDefinitions lists compiling, but toggling the value has no effect on CPU load or on whether the Plotter panel updates. It is kept only so that legacy user projects which list it keep working.")
		.withDefault(1)
		.withValue(ENABLE_PLOTTER)
		.withVestigal();

	data["ENABLE_PEAK_METERS_FOR_GAIN_EFFECT"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Collects post-gain output magnitudes on the Simple Gain effect so its meter reads live values.")
		.withDescriptionLine("Scans the left and right output magnitude of the Simple Gain effect after the gain stage has been applied, which feeds the module's output meter and any scripted getCurrentLevel call against it. This is split out from the global peak meter flag because the Simple Gain effect is used heavily on the master chain and in voice effect chains, so exporting without it can save the magnitude scan on every instance. Turning it off does not affect any other effect, only the Simple Gain.")
		.withDescriptionLine("> Read together with the global peak meter flag: disabling the global flag still leaves the Simple Gain meter alive if this one is on, and vice versa.")
		.withDefault(1)
		.withValue(ENABLE_PEAK_METERS_FOR_GAIN_EFFECT)
		.withCrossReference(LinkType::Module, "SimpleGain", "output magnitude scan used by the module meter is gated by this flag")
		.withCrossReference(LinkType::Preprocessor, "ENABLE_ALL_PEAK_METERS", "global peak meter switch that every other module uses; this flag carves out the Simple Gain effect separately");

	data["ENABLE_STARTUP_LOG"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Writes a timestamped plain-text startup log to the user's desktop while the plugin initialises.")
		.withDescriptionLine("Installs a lightweight logger that appends every major initialisation step (preset load, sample scan, scriptnode compile, UI build) to a StartLog.txt file on the user's desktop, together with the elapsed milliseconds since the previous entry. Useful when debugging a plugin that fails on a customer machine where no debugger or console is available, because the file survives a crash and shows exactly which step hung. Carries essentially no overhead during normal operation (one file append per milestone) but writes to the desktop unconditionally, so do not ship a release build with this enabled.")
		.withDescriptionLine("> The log file is always created at the same path on the desktop and is overwritten on every startup, so remind users to grab the file before relaunching the plugin.")
		.withDefault(0)
		.withValue(ENABLE_STARTUP_LOG);

	data["HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Shows an overlay message when the host asks for a buffer size that is not a multiple of the event raster.")
		.withDescriptionLine("Every modulation block and MIDI event timestamp in HISE is aligned to the event raster grid, so any host buffer size that is not an integer multiple of that raster forces the engine to pad the block internally and can cause subtle timing drift. With this flag on, the engine pops up a user-facing error overlay the moment such a buffer size is negotiated, which catches misconfigured audio devices during development. Turn it off when shipping a plugin that is expected to run on fixed hardware where the buffer size is known to be compatible and the overlay would only confuse end users.")
		.withDefault(1)
		.withValue(HISE_COMPLAIN_ABOUT_ILLEGAL_BUFFER_SIZE)
		.withCrossReference(LinkType::Preprocessor, "HISE_EVENT_RASTER", "defines the raster divisor that the buffer size must be a multiple of for the check to pass")
		.withCrossReference(LinkType::Preprocessor, "HISE_MAX_PROCESSING_BLOCKSIZE", "caps the internal block size used after rastering and must itself stay a multiple of the event raster");

	data["HISE_INCLUDE_PROFILING_TOOLKIT"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Compiles the in-engine profiler that records thread, scriptnode and script execution traces.")
		.withDescriptionLine("Enables the full profiling toolkit: the Threads.startProfiling scripting API that captures a base64-encoded timeline, the per-node execution timing inside scriptnode, the paint-function timing on ScriptLookAndFeel, the broadcaster send trace and the script engine's sample/startSampling debug hooks. Without this flag the profiler entry points still compile but behave as no-ops or throw a script error, so calls from HISEScript fail silently in an exported plugin. Leave this off for release builds because the timeline collection adds measurable overhead to every instrumented code path and the recorded data is only useful inside the HISE IDE or when exported back out for analysis.")
		.withDescriptionLine("> Read from the project's Extra Definitions at runtime, so a scripted profiling call can check the flag and warn the user when the compiled plugin was not built with it.")
		.withDefault(0)
		.withValue(HISE_INCLUDE_PROFILING_TOOLKIT)
		.withHotReload()
		.withCrossReference(LinkType::ScriptingApi, "Threads.startProfiling", "Threads.startProfiling throws a script error unless this flag is enabled")
		.withCrossReference(LinkType::ScriptingApi, "Console", "Console.startSampling and Console.sample become no-ops when the profiling toolkit is not compiled in")
		.withCrossReference(LinkType::ScriptingApi, "Broadcaster", "sendMessage instrumentation and per-callback timing collection are only active with this flag")
		.withCrossReference(LinkType::ScriptingApi, "ScriptLookAndFeel", "per-paint-routine timing for LAF callbacks requires the profiling toolkit");

	data["HISE_SCRIPT_SERVER_TIMEOUT"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Timeout in milliseconds for every HTTP request issued by the scripting server and download APIs.")
		.withDescriptionLine("Sets the connection and read timeout used by every HTTP call the engine makes on behalf of a script, which covers Server.callWithGET and callWithPOST, Server.isOnline, the Download and background-task HTTP helpers, and the internal update checker. The default of 10000 ms is a reasonable balance for typical authentication pings and small metadata requests; raise it to 30000 or more if the plugin talks to a slow REST endpoint or downloads larger responses synchronously, and lower it only if the script polls a URL fast enough that a stuck request must not block the script thread. The value is baked in at compile time, so overriding it requires an ExtraDefinitions entry rather than a runtime API.")
		.withDefault(10000)
		.withValue(HISE_SCRIPT_SERVER_TIMEOUT)
		.withCrossReference(LinkType::ScriptingApi, "Server", "every callWithGET, callWithPOST and isOnline request uses this timeout")
		.withCrossReference(LinkType::ScriptingApi, "Download", "HTTP download resume and start use this as the connection timeout");

	data["USE_GLITCH_DETECTION"] = Entry()
		.withCategory(Category::DebugAndProfiling)
		.withBrief("Instruments performance-critical audio functions with a scoped timer that logs when they overrun the buffer budget.")
		.withDescriptionLine("Wraps marked audio-thread functions in a stack-allocated timer that logs a warning to the system logger whenever the function runs longer than the current buffer budget, with deduplication so that a single glitch only produces one log entry even if the call appears multiple times in the stack. Useful during development to locate which module is responsible for an audible drop-out, but the per-scope timer and identifier bookkeeping add non-trivial overhead on the audio thread, which is why the default project template disables it for exported plugins. Leave this off for release builds.")
		.withDefault(0)
		.withValue(USE_GLITCH_DETECTION);

	// <-- ADD NEW PREPROCESSORS DEFINITIONS BEFORE THIS LINE
}

}