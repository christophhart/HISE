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

#ifndef GLOBALMODULATORCONTAINER_H_INCLUDED
#define GLOBALMODULATORCONTAINER_H_INCLUDED

 

class GlobalModulatorContainerSound : public ModulatorSynthSound
{
public:
	GlobalModulatorContainerSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override   { return true; }
	bool appliesToChannel(int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override  { return true; }
};

class GlobalModulatorContainerVoice : public ModulatorSynthVoice
{
public:

	GlobalModulatorContainerVoice(ModulatorSynth *ownerSynth) :
		ModulatorSynthVoice(ownerSynth)
	{};

	bool canPlaySound(SynthesiserSound *) override { return true; };

	void startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
	void calculateBlock(int startSample, int numSamples) override;;

};

class GlobalModulatorData
{
public:

	GlobalModulatorData(Processor *modulator);

	/** Sets up the buffers depending on the type of the modulator. */
	void prepareToPlay(double sampleRate, int blockSize);

	void saveValuesToBuffer(int startIndex, int numSamples, int voiceIndex = 0, int noteNumber=-1);
	const float *getModulationValues(int startIndex, int voiceIndex = 0);
	float getConstantVoiceValue(int noteNumber);

	const Processor *getProcessor() const { return modulator.get(); }

	VoiceStartModulator *getVoiceStartModulator() { return dynamic_cast<VoiceStartModulator*>(modulator.get()); }
	const VoiceStartModulator *getVoiceStartModulator() const { return dynamic_cast<VoiceStartModulator*>(modulator.get()); }
	TimeVariantModulator *getTimeVariantModulator() { return dynamic_cast<TimeVariantModulator*>(modulator.get()); }
	const TimeVariantModulator *getTimeVariantModulator() const { return dynamic_cast<TimeVariantModulator*>(modulator.get()); }
	EnvelopeModulator *getEnvelopeModulator() { return dynamic_cast<EnvelopeModulator*>(modulator.get()); }
	const EnvelopeModulator *getEnvelopeModulator() const { return dynamic_cast<EnvelopeModulator*>(modulator.get()); }

private:

	WeakReference<Processor> modulator;
	GlobalModulator::ModulatorType type;

	int numVoices;
	AudioSampleBuffer valuesForCurrentBuffer;
	Array<float> constantVoiceValues;
};

class GlobalModulatorContainer : public ModulatorSynth,
								 public SafeChangeListener
{
public:

	SET_PROCESSOR_NAME("GlobalModulatorContainer", "Global Modulator Container");

	float getVoiceStartValueFor(const Processor *voiceStartModulator);

	GlobalModulatorContainer(MainController *mc, const String &id, int numVoices);;

	void restoreFromValueTree(const ValueTree &v) override;

	const float *getModulationValuesForModulator(Processor *p, int startIndex, int voiceIndex = 0);
	float getConstantVoiceValue(Processor *p, int noteNumber);

	ProcessorEditorBody* createEditor(BetterProcessorEditor *parentEditor) override;

	void changeListenerCallback(SafeChangeBroadcaster *) { refreshList(); }
	void addChangeListenerToHandler(SafeChangeListener *listener);
	void removeChangeListenerFromHandler(SafeChangeListener *listener);

	void preStartVoice(int voiceIndex, int noteNumber);
	void postVoiceRendering(int startSample, int numThisTime);

	void addProcessorsWhenEmpty() override {};

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

private:

	friend class GlobalModulatorContainerVoice;

	void refreshList();

	OwnedArray<GlobalModulatorData> data;
};



#endif  // GLOBALMODULATORCONTAINER_H_INCLUDED
