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

namespace hise {
using namespace juce;

// ============================================================================
// ProcessorMetadataRegistry
// ============================================================================

ProcessorMetadataRegistry::ProcessorMetadataRegistry()
{
	registerAllMetadata();
}

const ProcessorMetadata* ProcessorMetadataRegistry::get(const Identifier& typeId) const
{
	if (data->entries.find(typeId) != data->entries.end())
		return &(data->entries.at(typeId));

	return nullptr;
}

void ProcessorMetadataRegistry::add(const Identifier& typeId, const ProcessorMetadata& md)
{
	jassert(data->entries.find(typeId) == data->entries.end());
	data->entries[typeId] = md;
}

void ProcessorMetadataRegistry::add(const ProcessorMetadata& md)
{
	jassert(md.id.isValid());
	add(md.id, md);
}

Array<Identifier> ProcessorMetadataRegistry::getRegisteredTypes() const
{
	Array<Identifier> result;

	for (const auto& [key, pd] : data->entries)
		result.add(key);

	return result;
}

int ProcessorMetadataRegistry::getNumRegistered() const
{
	return data->entries.size();
}

var ProcessorMetadataRegistry::toJSON() const
{
	DynamicObject::Ptr o = new DynamicObject();

	o->setProperty("version", "4.1.0");
	o->setProperty("categories", var(data->categories.get()));

	Array<var> result;

	for (const auto& [key, pd] : data->entries)
		result.add(pd.toJSON());
	
	o->setProperty("modules", var(result));

	return var(o.get());
}

void ProcessorMetadataRegistry::Data::init()
{

}

void ProcessorMetadataRegistry::registerAllMetadata()
{
	if (data->initialised)
		return;

	// --- VoiceStartModulators ---

	add(ConstantModulator::createMetadata());
	add(VelocityModulator::createMetadata());
	add(KeyModulator::createMetadata());
	add(RandomModulator::createMetadata());
	add(GlobalVoiceStartModulator::createMetadata());
	add(GlobalStaticTimeVariantModulator::createMetadata());
	add(ArrayModulator::createMetadata());
	add(JavascriptVoiceStartModulator::createMetadata());
	add(EventDataModulator::createMetadata());

	// --- TimeVariantModulators ---

	add(ControlModulator::createMetadata());
	add(LfoModulator::createMetadata());
	add(PitchwheelModulator::createMetadata());
	add(MacroModulator::createMetadata());
	add(GlobalTimeVariantModulator::createMetadata());
	add(JavascriptTimeVariantModulator::createMetadata());
	add(HardcodedTimeVariantModulator::createMetadata());

	// --- EnvelopeModulators ---

	add(SimpleEnvelope::createMetadata());
	add(AhdsrEnvelope::createMetadata());
	add(TableEnvelope::createMetadata());
	add(JavascriptEnvelopeModulator::createMetadata());
	add(MPEModulator::createMetadata());
	add(ScriptnodeVoiceKiller::createMetadata());
	add(GlobalEnvelopeModulator::createMetadata());
	add(EventDataEnvelope::createMetadata());
	add(HardcodedEnvelopeModulator::createMetadata());
	add(MatrixModulator::createMetadata());
	add(FlexAhdsrEnvelope::createMetadata());

	// --- MidiProcessors ---

	add(JavascriptMidiProcessor::createMetadata());
	add(Transposer::createMetadata());
	add(MidiPlayer::createMetadata());
	add(ChokeGroupProcessor::createMetadata());

	// Hardcoded script processors
	add(LegatoProcessor::createMetadata());
	add(CCSwapper::createMetadata());
	add(ReleaseTriggerScriptProcessor::createMetadata());
	add(CCToNoteProcessor::createMetadata());
	add(ChannelFilterScriptProcessor::createMetadata());
	add(ChannelSetterScriptProcessor::createMetadata());
	add(MuteAllScriptProcessor::createMetadata());
	add(hise::Arpeggiator::createMetadata());

	// --- Effects ---

	add(PolyFilterEffect::createMetadata());
	add(HarmonicFilter::createMetadata());
	add(HarmonicMonophonicFilter::createMetadata());
	add(CurveEq::createMetadata());
	add(StereoEffect::createMetadata());
	add(SimpleReverbEffect::createMetadata());
	add(GainEffect::createMetadata());
	add(ConvolutionEffect::createMetadata());
	add(DelayEffect::createMetadata());
	add(ChorusEffect::createMetadata());
	add(PhaseFX::createMetadata());
	add(RouteEffect::createMetadata());
	add(SendEffect::createMetadata());
	add(SaturatorEffect::createMetadata());
	add(JavascriptMasterEffect::createMetadata());
	add(JavascriptPolyphonicEffect::createMetadata());
	add(SlotFX::createMetadata());
	add(EmptyFX::createMetadata());
	add(DynamicsEffect::createMetadata());
	add(AnalyserEffect::createMetadata());
	add(ShapeFX::createMetadata());
	add(PolyshapeFX::createMetadata());
	add(HardcodedMasterFX::createMetadata());
	add(HardcodedPolyphonicFX::createMetadata());
	add(MidiMetronome::createMetadata());
	add(NoiseGrainPlayer::createMetadata());

	// --- SoundGenerators ---

	add(ModulatorSampler::createMetadata());
	add(SineSynth::createMetadata());
	add(ModulatorSynthChain::createMetadata());
	add(GlobalModulatorContainer::createMetadata());
	add(WaveSynth::createMetadata());
	add(NoiseSynth::createMetadata());
	add(WavetableSynth::createMetadata());
	add(AudioLooper::createMetadata());
	add(ModulatorSynthGroup::createMetadata());
	add(JavascriptSynthesiser::createMetadata());
	add(MacroModulationSource::createMetadata());
	add(SendContainer::createMetadata());
	add(SilentSynth::createMetadata());
	add(HardcodedSynthesiser::createMetadata());
	
	static std::map<Identifier, std::vector<Identifier>> categoryMap = {
	{ "AHDSR",                          { ProcessorMetadataIds::generator } },
	{ "FlexAHDSR",                      { ProcessorMetadataIds::generator } },
	{ "SimpleEnvelope",                 { ProcessorMetadataIds::generator } },
	{ "TableEnvelope",                  { ProcessorMetadataIds::generator } },
	{ "LFO",                            { ProcessorMetadataIds::generator, ProcessorMetadataIds::oscillator } },
	{ "Random",                         { ProcessorMetadataIds::generator } },
	{ "Constant",                       { ProcessorMetadataIds::generator } },
	{ "Velocity",                       { ProcessorMetadataIds::input, ProcessorMetadataIds::note_processing } },
	{ "KeyNumber",                      { ProcessorMetadataIds::input, ProcessorMetadataIds::note_processing } },
	{ "MidiController",                 { ProcessorMetadataIds::input } },
	{ "PitchWheel",                     { ProcessorMetadataIds::input } },
	{ "MPEModulator",                   { ProcessorMetadataIds::input } },
	{ "ArrayModulator",                 { ProcessorMetadataIds::input } },
	{ "GlobalEnvelopeModulator",        { ProcessorMetadataIds::routing } },
	{ "GlobalTimeVariantModulator",     { ProcessorMetadataIds::routing } },
	{ "GlobalVoiceStartModulator",      { ProcessorMetadataIds::routing } },
	{ "GlobalStaticTimeVariantModulator", { ProcessorMetadataIds::routing } },
	{ "MatrixModulator",               { ProcessorMetadataIds::routing } },
	{ "EventDataModulator",            { ProcessorMetadataIds::routing } },
	{ "EventDataEnvelope",             { ProcessorMetadataIds::routing } },
	{ "MacroModulator",                { ProcessorMetadataIds::routing } },
	{ "ScriptVoiceStartModulator",     { ProcessorMetadataIds::custom } },
	{ "ScriptEnvelopeModulator",       { ProcessorMetadataIds::custom } },
	{ "ScriptTimeVariantModulator",    { ProcessorMetadataIds::custom } },
	{ "HardcodedEnvelopeModulator",    { ProcessorMetadataIds::custom } },
	{ "HardcodedTimevariantModulator", { ProcessorMetadataIds::custom } },
	{ "ScriptnodeVoiceKiller",         { ProcessorMetadataIds::custom } },
	{ "SineSynth",                     { ProcessorMetadataIds::oscillator } },
	{ "WaveSynth",                     { ProcessorMetadataIds::oscillator } },
	{ "WavetableSynth",                { ProcessorMetadataIds::oscillator } },
	{ "Noise",                         { ProcessorMetadataIds::oscillator } },
	{ "StreamingSampler",              { ProcessorMetadataIds::sample_playback } },
	{ "AudioLooper",                   { ProcessorMetadataIds::sample_playback, ProcessorMetadataIds::sequencing } },
	{ "SynthChain",                    { ProcessorMetadataIds::container } },
	{ "SynthGroup",                    { ProcessorMetadataIds::container, ProcessorMetadataIds::oscillator } },
	{ "SendContainer",                 { ProcessorMetadataIds::container, ProcessorMetadataIds::routing } },
	{ "GlobalModulatorContainer",      { ProcessorMetadataIds::routing, ProcessorMetadataIds::container } },
	{ "MacroModulationSource",         { ProcessorMetadataIds::routing } },
	{ "ScriptSynth",                   { ProcessorMetadataIds::custom } },
	{ "HardcodedSynth",                { ProcessorMetadataIds::custom } },
	{ "SilentSynth",                   { ProcessorMetadataIds::custom } },
	{ "Arpeggiator",                   { ProcessorMetadataIds::sequencing, ProcessorMetadataIds::generator } },
	{ "MidiPlayer",                    { ProcessorMetadataIds::sequencing, ProcessorMetadataIds::generator } },
	{ "ReleaseTrigger",                { ProcessorMetadataIds::note_processing } },
	{ "LegatoWithRetrigger",           { ProcessorMetadataIds::note_processing } },
	{ "ChokeGroupProcessor",           { ProcessorMetadataIds::note_processing } },
	{ "CC2Note",                       { ProcessorMetadataIds::note_processing, ProcessorMetadataIds::routing } },
	{ "Transposer",                    { ProcessorMetadataIds::note_processing } },
	{ "ChannelFilter",                 { ProcessorMetadataIds::routing, ProcessorMetadataIds::note_processing } },
	{ "ChannelSetter",                 { ProcessorMetadataIds::routing, ProcessorMetadataIds::note_processing } },
	{ "CCSwapper",                     { ProcessorMetadataIds::routing } },
	{ "MidiMuter",                     { ProcessorMetadataIds::routing, ProcessorMetadataIds::mixing } },
	{ "ScriptProcessor",              { ProcessorMetadataIds::custom } },
	{ "Dynamics",                      { ProcessorMetadataIds::dynamics, ProcessorMetadataIds::mixing } },
	{ "Saturator",                     { ProcessorMetadataIds::dynamics } },
	{ "ShapeFX",                       { ProcessorMetadataIds::dynamics } },
	{ "PolyshapeFX",                   { ProcessorMetadataIds::dynamics } },
	{ "PolyphonicFilter",             { ProcessorMetadataIds::filter } },
	{ "HarmonicFilter",               { ProcessorMetadataIds::filter } },
	{ "HarmonicFilterMono",           { ProcessorMetadataIds::filter } },
	{ "CurveEq",                       { ProcessorMetadataIds::filter, ProcessorMetadataIds::mixing } },
	{ "Delay",                         { ProcessorMetadataIds::delay } },
	{ "Chorus",                        { ProcessorMetadataIds::delay } },
	{ "PhaseFX",                       { ProcessorMetadataIds::delay } },
	{ "SimpleReverb",                  { ProcessorMetadataIds::reverb } },
	{ "Convolution",                   { ProcessorMetadataIds::reverb } },
	{ "SimpleGain",                    { ProcessorMetadataIds::mixing, ProcessorMetadataIds::utility } },
	{ "StereoFX",                      { ProcessorMetadataIds::mixing } },
	{ "SendFX",                        { ProcessorMetadataIds::routing, ProcessorMetadataIds::mixing } },
	{ "RouteFX",                       { ProcessorMetadataIds::routing } },
	{ "EmptyFX",                       { ProcessorMetadataIds::utility } },
	{ "SlotFX",                        { ProcessorMetadataIds::utility, ProcessorMetadataIds::routing } },
	{ "MidiMetronome",                 { ProcessorMetadataIds::utility, ProcessorMetadataIds::sequencing } },
	{ "Analyser",                      { ProcessorMetadataIds::utility } },
	{ "NoiseGrainPlayer",             { ProcessorMetadataIds::utility } },
	{ "ScriptFX",                      { ProcessorMetadataIds::custom } },
	{ "PolyScriptFX",                  { ProcessorMetadataIds::custom } },
	{ "HardcodedMasterFX",            { ProcessorMetadataIds::custom } },
	{ "HardcodedPolyphonicFX",        { ProcessorMetadataIds::custom } },
	};

	auto obj = new DynamicObject();
	obj->setProperty(ProcessorMetadataIds::oscillator, 
		"Modules that generate audio or modulation signals from oscillators or synthesis algorithms. Contains sound generators and modulators.");
	obj->setProperty(ProcessorMetadataIds::sample_playback, 
		"Modules that play back audio samples");
	obj->setProperty(ProcessorMetadataIds::container, 
		"Modules that hold and combine other sound generators");
	obj->setProperty(ProcessorMetadataIds::sequencing, 
		"MIDI processors that generate or play back note sequences");
	obj->setProperty(ProcessorMetadataIds::note_processing, 
		"MIDI processors that transform, filter, or react to incoming note events. Contains MIDI processors and modulators.");
	obj->setProperty(ProcessorMetadataIds::dynamics, 
		"Effects that shape the amplitude or add distortion and saturation");
	obj->setProperty(ProcessorMetadataIds::filter, 
		"Effects that shape the frequency spectrum of the audio signal");
	obj->setProperty(ProcessorMetadataIds::delay, 
		"Effects based on delayed signal copies, including chorus and phaser");
	obj->setProperty(ProcessorMetadataIds::reverb, 
		"Effects that simulate room acoustics and spatial reflections");
	obj->setProperty(ProcessorMetadataIds::mixing, 
		"Effects that control volume, stereo width, or stereo balance. Contains effects and MIDI processors.");
	obj->setProperty(ProcessorMetadataIds::input, 
		"Modulators that convert external events like MIDI or MPE into modulation signals");
	obj->setProperty(ProcessorMetadataIds::generator, 
		"Modulators that create modulation signals internally, such as envelopes and LFOs. Contains modulators and MIDI processors.");
	obj->setProperty(ProcessorMetadataIds::routing, 
		"Modules that forward, distribute, or proxy signals or events across the module tree. Contains sound generators, MIDI processors, effects, and modulators.");
	obj->setProperty(ProcessorMetadataIds::utility, 
		"Modules for analysis, placeholders, or structural purposes without audio processing");
	obj->setProperty(ProcessorMetadataIds::custom, 
		"Modules that run user-defined DSP logic via scriptnode networks, compiled C++ code, or HiseScript callbacks. Contains sound generators, MIDI processors, effects, and modulators.");

	data->categories = obj;

	for (auto& [id, md] : data->entries)
	{
		auto tags = categoryMap[id];

		for (const auto& t : tags)
			md.categories.add(t);
	}

	data->initialised = true;
}

} // namespace hise
