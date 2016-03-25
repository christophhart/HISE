/*
  ==============================================================================

    AudioLooper.h
    Created: 2 Jul 2015 2:19:57pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef AUDIOLOOPER_H_INCLUDED
#define AUDIOLOOPER_H_INCLUDED


class AudioLooper;

class AudioLooperSound : public ModulatorSynthSound
{
public:
	AudioLooperSound() {}

	bool appliesToNote(int /*midiNoteNumber*/) override   { return true; }
	bool appliesToChannel(int /*midiChannel*/) override   { return true; }
	bool appliesToVelocity(int /*midiChannel*/) override  { return true; }
};

class AudioLooperVoice : public ModulatorSynthVoice
{
public:

	AudioLooperVoice(ModulatorSynth *ownerSynth);;

	bool canPlaySound(SynthesiserSound *) override
	{
		return true;
	};

	void startNote(int midiNoteNumber, float /*velocity*/, SynthesiserSound*, int /*currentPitchWheelPosition*/) override;

	void calculateBlock(int startSample, int numSamples) override;;

private:

	friend class AudioLooper;

	float syncFactor;


};

class AudioLooper : public ModulatorSynth,
					public AudioSampleProcessor,
					public TempoListener
{
public:

	SET_PROCESSOR_NAME("AudioLooper", "Audio Loop Player");

	enum SpecialParameters
	{
		SyncMode = ModulatorSynth::numModulatorSynthParameters,
		LoopEnabled,
		PitchTracking,
		RootNote,
		numLooperParameters
	};

	AudioLooper(MainController *mc, const String &id, int numVoices);;

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	void tempoChanged(double /*newTempo*/) override
	{
		setSyncMode(syncMode);
	}

	float getAttribute(int parameterIndex) const override;;

	float getDefaultValue(int parameterIndex) const override;

	void setInternalAttribute(int parameterIndex, float newValue) override;

	

	ProcessorEditorBody* createEditor(BetterProcessorEditor *parentEditor) override;
	void setSyncMode(int newSyncMode);

private:

	UpdateMerger inputMerger;

	bool loopEnabled;
	bool pitchTrackingEnabled;
	int rootNote;

	friend class AudioLooperVoice;

	AudioSampleProcessor::SyncToHostMode syncMode;

};







#endif  // AUDIOLOOPER_H_INCLUDED
