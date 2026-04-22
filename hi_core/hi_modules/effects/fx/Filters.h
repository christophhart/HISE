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

#pragma once

namespace hise { using namespace juce;

/** The filter module of HISE. 
	@ingroup effectTypes

	This is the filter module of HISE that will apply monophonic or polyphonic filtering
	on the signal depending on the modulators that are inserted into its modulation chains.
*/
class PolyFilterEffect: public VoiceEffectProcessor,
						public FilterEffect,
						public ModulatorChain::Handler::Listener,
						public scriptnode::data::filter_base,
						public ProcessorWithCustomFilterStatistics
{
public:

	SET_PROCESSOR_NAME("PolyphonicFilter", "Filter", "");

	static ProcessorMetadata createMetadata();

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

	const bool metadataInitialised;

	~PolyFilterEffect();

	void processorChanged(EventType t, Processor* p) override;

	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	ModulationDisplayValue::QueryFunction::Ptr getModulationQueryFunction(int parameterIndex) const override;

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

	FilterDataObject::CoefficientData getApproximateCoefficients() const override;

	bool hasPolyMods() const noexcept;

	
private:

	void updateDisplayCoefficients()
	{
		getFilterData(0)->getUpdater().sendDisplayChangeMessage(0, sendNotificationAsync);
	}

	friend class FilterEditor;

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


