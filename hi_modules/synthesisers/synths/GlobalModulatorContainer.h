/*
  ==============================================================================

    GlobalModulatorContainer.h
    Created: 9 Aug 2015 7:32:27pm
    Author:  Chrisboy

  ==============================================================================
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

	void saveValuesToBuffer(int startIndex, int numSamples, int voiceIndex = 0);
	const float *getModulationValues(int startIndex, int voiceIndex = 0);
	float getConstantVoiceValue();

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
	float getConstantVoiceValue(Processor *p);

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
