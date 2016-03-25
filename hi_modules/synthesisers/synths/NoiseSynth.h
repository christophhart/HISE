class NoiseSound : public ModulatorSynthSound
{
public:
    NoiseSound() {}

    bool appliesToNote (int /*midiNoteNumber*/) override   { return true; }
    bool appliesToChannel (int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity (int /*midiChannel*/) override  { return true; }
};


class NoiseVoice: public ModulatorSynthVoice
{
public:

	NoiseVoice(ModulatorSynth *ownerSynth):
		ModulatorSynthVoice(ownerSynth)
	{
	};

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote (int /*midiNoteNumber*/, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/) override
	{
		ModulatorSynthVoice::startNote(0, 0.0f, nullptr, -1);

        voiceUptime = 0.0;
        
        uptimeDelta = 1.0;
    }

	void calculateBlock(int startSample, int numSamples) override
	{
		const int startIndex = startSample;
		const int samplesToCopy = numSamples;

		const float *modValues = getVoiceGainValues(startSample, numSamples);

		while (--numSamples >= 0)
        {
			const float currentSample = getNextValue();

			// Stereo mode assumed
			voiceBuffer.setSample (0, startSample, currentSample);
			voiceBuffer.setSample (1, startSample, currentSample);

			voiceUptime++;

            ++startSample;    
        }

		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
	};

private:

	inline float getNextValue() const
	{
		return 2.0f * ((float)(rand()) / (float)(RAND_MAX)) - 1.0f;
	};

};

class NoiseSynth: public ModulatorSynth
{
public:

	SET_PROCESSOR_NAME("Noise", "Noise Generator")

	NoiseSynth(MainController *mc, const String &id, int numVoices):
		ModulatorSynth(mc, id, numVoices)
	{
		for(int i = 0; i < numVoices; i++) addVoice(new NoiseVoice(this));
		addSound (new NoiseSound());	
	};

	ProcessorEditorBody* createEditor(BetterProcessorEditor *parentEditor) override;
};