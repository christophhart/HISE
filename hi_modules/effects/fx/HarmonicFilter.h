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

#ifndef HARMONICFILTER_H_INCLUDED
#define HARMONICFILTER_H_INCLUDED



class BaseHarmonicFilter
{
public:

    virtual ~BaseHarmonicFilter() {};
    
	virtual SliderPackData *getSliderPackData(int i) = 0;
	virtual void setCrossfadeValue(double normalizedCrossfadeValue) = 0;

};

 

class HarmonicFilter : public VoiceEffectProcessor,
					   public BaseHarmonicFilter
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

	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	
	SliderPackData *getSliderPackData(int i) override;
	void setCrossfadeValue(double normalizedCrossfadeValue) override;

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




class HarmonicMonophonicFilter : public MonophonicEffectProcessor,
								 public BaseHarmonicFilter
{
public:

	SET_PROCESSOR_NAME("HarmonicFilterMono", "Harmonic Filter Monophonic");

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

	HarmonicMonophonicFilter(MainController *mc, const String &uid);

	void setInternalAttribute(int parameterIndex, float newValue) override;
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
	
	/** Resets the filter state if a new voice is started. */
	void startMonophonicVoice(int noteNumber) override;

	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) override;

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	SliderPackData *getSliderPackData(int i);
	void setCrossfadeValue(double normalizedCrossfadeValue);

private:

	int getNumBandForFilterBandIndex(FilterBandNumbers number) const;

	int filterBandIndex;
	float currentCrossfadeValue;
	int semiToneTranspose;
	float q;

	ScopedPointer<SliderPackData> dataA;
	ScopedPointer<SliderPackData> dataB;
	ScopedPointer<SliderPackData> dataMix;

	OwnedArray<MonoFilterEffect> harmonicFilters;
	ScopedPointer<ModulatorChain> xFadeChain;
	AudioSampleBuffer timeVariantFreqModulatorBuffer;
};





#endif  // HARMONICFILTER_H_INCLUDED
