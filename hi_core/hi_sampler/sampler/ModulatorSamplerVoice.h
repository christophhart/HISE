#ifndef MODULATORSAMPLERVOICE_H_INCLUDED
#define MODULATORSAMPLERVOICE_H_INCLUDED


/** A ModulatorSamplerVoice is a wrapper around a StreamingSamplerVoice with logic for modulation stuff
*	@ingroup sampler
*
*/
class ModulatorSamplerVoice : public ModulatorSynthVoice
{
public:

	// ================================================================================================================

	ModulatorSamplerVoice(ModulatorSynth *ownerSynth);
	~ModulatorSamplerVoice() {};

	// ================================================================================================================

	bool canPlaySound(SynthesiserSound*) { return true; };
	void startNote(int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;
	void stopNote(float velocity, bool allowTailoff) override;

	// ================================================================================================================

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;
	void resetVoice() override;

	void handlePlaybackPosition(const StreamingSamplerSound * sound, ModulatorSampler * sampler);

	// ================================================================================================================

	virtual void setLoaderBufferSize(int newBufferSize);
	virtual double getDiskUsage();
	virtual size_t getStreamingBufferSize() const;

	// ================================================================================================================

	const float *getCrossfadeModulationValues(int startSample, int numSamples);
	void setSampleStartModValue(float modValue) { if (modValue >= 0.0f) sampleStartModValue = modValue; };
	void enablePitchModulation(bool shouldBeEnabled);
	
	/** returns the sound that is played by the voice.
	*
	*	use this method instead of SynthesiserVoice::getCurrentlyPlayingSound(), because ModulatorSynthGroups
	*	do not access this member at startNote() and it will return nullptr.
	*/
	ModulatorSamplerSound *getCurrentlyPlayingSamplerSound() const 	{ return currentlyPlayingSamplerSound; }
	
	// ================================================================================================================

protected:
	
	ModulatorSamplerSound *currentlyPlayingSamplerSound;

	float velocityXFadeValue;
	float sampleStartModValue;

	// ================================================================================================================

private:

	StreamingSamplerVoice wrappedVoice;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSamplerVoice)
};


/** A ModulatorSamplerVoice is a wrapper around a StreamingSamplerVoice with logic for modulation stuff
*	@ingroup sampler
*
*/
class MultiMicModulatorSamplerVoice : public ModulatorSamplerVoice
{
public:

	// ================================================================================================================

	MultiMicModulatorSamplerVoice(ModulatorSynth *ownerSynth, int numMicPositions);
	~MultiMicModulatorSamplerVoice() {};

	// ================================================================================================================

	void startNote(int midiNoteNumber, float velocity, SynthesiserSound* s, int /*currentPitchWheelPosition*/) override;
	void calculateBlock(int startSample, int numSamples) override;
	void prepareToPlay(double sampleRate, int samplesPerBlock);

	// ================================================================================================================

	void setLoaderBufferSize(int newBufferSize) override;
	double getDiskUsage() override;
	size_t getStreamingBufferSize() const override;

	/** Resets the display value for the current note. */
	void resetVoice() override;

	// ================================================================================================================
private:

	OwnedArray<StreamingSamplerVoice> wrappedVoices;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiMicModulatorSamplerVoice)
};


#endif  // MODULATORSAMPLERVOICE_H_INCLUDED
