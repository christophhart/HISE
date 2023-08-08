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

#ifndef FILTERS_H_INCLUDED
#define FILTERS_H_INCLUDED

namespace hise { using namespace juce;

#define USE_STATE_VARIABLE_FILTERS 1


#if HISE_INCLUDE_OLD_MONO_FILTER

class MonoFilterEffect: public MonophonicEffectProcessor,
						public FilterEffect
{
public:

	SET_PROCESSOR_NAME("MonophonicFilter", "Monophonic Filter", "deprecated");

	enum InternalChains
	{
		FrequencyChain = 0,
		GainChain,
		BipolarFrequencyChain,
		numInternalChains
	};

	enum EditorStates
	{
		FrequencyChainShown = Processor::numEditorStates,
		GainChainShown,
		BipolarFrequencyChainShown,
		numEditorStates
	};

	enum Parameters
	{
		Gain = 0,
		Frequency,
		Q,
		Mode,
        Quality,
		BipolarIntensity,
		numEffectParameters
	};

	MonoFilterEffect(MainController *mc, const String &id);;

	void setUseInternalChains(bool shouldBeUsed);;
	
	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples) override;
    
    void processBlockPartial(AudioSampleBuffer &buffer, int startSample, int numSamples);
	
	bool hasTail() const override {return false; };

	int getNumInternalChains() const override { return useInternalChains ? numInternalChains : 0; };
	int getNumChildProcessors() const override { return useInternalChains ? numInternalChains : 0; };
	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;
	
	IIRCoefficients getCurrentCoefficients() const override
	{
		return filterCollection.getCurrentCoefficients();
	}

private:

	void setMode(int filterMode);

	void calcCoefficients();

	bool useInternalChains;
	bool useFixedFrequency;

	ModulatorChain* freqChain;
	ModulatorChain* gainChain;
	ModulatorChain* bipolarFreqChain;

	friend class PolyFilterEffect;
	friend class HarmonicFilter;
	friend class HarmonicMonophonicFilter;

	bool changeFlag;

	bool useBipolarIntensity = false;
	float bipolarIntensity = 0.0f;

	FilterBank filterCollection;

	double lastSampleRate = 0.0;

};
#endif

/** The filter module of HISE. 
	@ingroup effectTypes

	This is the filter module of HISE that will apply monophonic or polyphonic filtering
	on the signal depending on the modulators that are inserted into its modulation chains.
*/
class PolyFilterEffect: public VoiceEffectProcessor,
						public FilterEffect,
						public ModulatorChain::Handler::Listener
{
public:

	SET_PROCESSOR_NAME("PolyphonicFilter", "Filter", "The filter module of HISE.");

	enum InternalChains
	{
		FrequencyChain = 0,
		GainChain,
		BipolarFrequencyChain,
		ResonanceChain,
		numInternalChains
	};

	enum EditorStates
	{
		FrequencyChainShown = Processor::numEditorStates,
		GainChainShown,
		BipolarFreqChainShown,
		ResonanceChainShown,
		numEditorStates
	};

	enum Parameters
	{
		Gain = 0,
		Frequency,
		Q,
		Mode,
		Quality,
		BipolarIntensity,
		numEffectParameters
	};

	PolyFilterEffect(MainController *mc, const String &uid, int numVoices);;

	~PolyFilterEffect();

	void processorChanged(EventType t, Processor* p) override;

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getDefaultValue(int parameterIndex) const override;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	int getNumInternalChains() const override { return numInternalChains; };
	int getNumChildProcessors() const override { return numInternalChains; };
	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;
	void renderNextBlock(AudioSampleBuffer &/*b*/, int /*startSample*/, int /*numSample*/);
	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;
	/** Resets the filter state if a new voice is started. */
	void startVoice(int voiceIndex, const HiseEvent& e) override;
	bool hasTail() const override { return false; };
	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	IIRCoefficients getCurrentCoefficients() const override;;

	bool hasPolyMods() const noexcept;

private:

	

	bool blockIsActive = false;
	int polyWatchdog = 0;


	BlockDivider<64> monoDivider;

	bool polyMode = false;

	FilterBank::FilterMode mode;
	float frequency;
	float q;
	float gain;

	bool changeFlag;

	float bipolarParameterValue = 0.0f;
	LinearSmoothedValue<float> bipolarIntensity;

	FilterBank voiceFilters;
	FilterBank monoFilters;

	mutable WeakReference<Processor> ownerSynthForCoefficients;

	JUCE_DECLARE_WEAK_REFERENCEABLE(PolyFilterEffect)
};




} // namespace hise


#endif  // FILTERS_H_INCLUDED
