/*
  ==============================================================================

    HarmonicFilter.h
    Created: 8 Aug 2015 10:29:51pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef HARMONICFILTER_H_INCLUDED
#define HARMONICFILTER_H_INCLUDED


 

class HarmonicFilter : public VoiceEffectProcessor
{
public:

	SET_PROCESSOR_NAME("HarmonicFilter", "Harmonic Filter");

	enum InternalChains
	{
		XFadeChain = 0,
		numInternalChains
	};

	enum EditorStates
	{
		MixChainShown = Processor::numEditorStates,
		numEditorStates
	};

	enum SpecialParameters
	{
		NumFilterBands = 0,
		QFactor,
		Crossfade,
		SemiToneTranspose,
		numParameters
	};

	enum SliderPacks
	{
		A = 0,
		B,
		Mix,
		numPacks
	};

	enum FilterBandNumbers
	{
		OneBand = 0,
		TwoBands,
		FourBands,
		EightBands,
		SixteenBands,
		numFilterBandNumbers
	};

	HarmonicFilter(MainController *mc, const String &uid, int numVoices_);;

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;;

	int getNumInternalChains() const override { return numInternalChains; };
	Processor *getChildProcessor(int /*processorIndex*/) override { return xFadeChain; };
	const Processor *getChildProcessor(int /*processorIndex*/) const override { return xFadeChain; };
	int getNumChildProcessors() const override { return 1; };
	AudioSampleBuffer & getBufferForChain(int /*index*/) override { return timeVariantFreqModulatorBuffer; };

	void setQ(float newQ);
	void setNumFilterBands(int numBands);
	void setSemitoneTranspose(float newValue);

	bool hasTail() const override { return true; };
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void renderNextBlock(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSample*/) {}
	/** Calculates the frequency chain and sets the q to the current value. */
	void preVoiceRendering(int voiceIndex, int startSample, int numSamples);
	/** Resets the filter state if a new voice is started. */
	void startVoice(int voiceIndex, int noteNumber) override;
	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;

	
	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;
	SliderPackData *getSliderPackData(int i);
	void setCrossfadeValue(double normalizedCrossfadeValue);

private:
	
	int getNumBandForFilterBandIndex(FilterBandNumbers number) const;

	int filterBandIndex;
	float currentCrossfadeValue;
	int semiToneTranspose;
	const int numVoices;
	float q;

	ScopedPointer<SliderPackData> dataA;
	ScopedPointer<SliderPackData> dataB;
	ScopedPointer<SliderPackData> dataMix;

	OwnedArray<PolyFilterEffect> harmonicFilters;
	ScopedPointer<ModulatorChain> xFadeChain;
	AudioSampleBuffer timeVariantFreqModulatorBuffer;
};



#endif  // HARMONICFILTER_H_INCLUDED
