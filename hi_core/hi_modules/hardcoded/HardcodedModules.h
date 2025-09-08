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

namespace hise
{
using namespace juce;



class HardcodedMasterFX: public MasterEffectProcessor,
						 public HardcodedSwappableEffect
{
public:

	SET_PROCESSOR_NAME("Hardcoded Master FX", "HardcodedMasterFX", "A master effect wrapper around a compiled DSP network");

	HardcodedMasterFX(MainController* mc, const String& uid);
	~HardcodedMasterFX() override;

	bool hasTail() const override;

	bool isSuspendedOnSilence() const override;

    bool isFadeOutPending() const noexcept override;

	Processor *getChildProcessor(int processorIndex) override;;
    const Processor *getChildProcessor(int processorIndex) const override;;

	int getNumInternalChains() const override;;

	int getNumChildProcessors() const override;;

	ModulatorChain::Collection* getModulationChains() override { return &modChains; }
	const ModulatorChain::Collection* getModulationChains() const override { return &modChains; }

	void connectionChanged() override;
	bool setEffect(const String& newEffect, bool) override;

	void connectToRuntimeTargets(scriptnode::OpaqueNode& on, bool shouldAdd) override;
	ModulationDisplayValue::QueryFunction::Ptr getModulationQueryFunction(int parameterIndex) const override;

	void voicesKilled() override;
	void setInternalAttribute(int index, float newValue) override;
	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree& v) override;
	float getAttribute(int index) const override;

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	Path getSpecialSymbol() const override;
	void handleHiseEvent(const HiseEvent &m) override;
	void applyEffect(AudioSampleBuffer &b, int startSample, int numSamples) final override;

	void renderWholeBuffer(AudioSampleBuffer &buffer) override;

private:

	ModulatorChain::ExtraModulatorRuntimeTargetSource extraMods;
};


class HardcodedPolyphonicFX : public VoiceEffectProcessor,
						      public HardcodedSwappableEffect,
							  public RoutableProcessor,
						      public VoiceResetter
{
public:

	SET_PROCESSOR_NAME("HardcodedPolyphonicFX", "Hardcoded Polyphonic FX", "A polyphonic hardcoded FX.");

	HardcodedPolyphonicFX(MainController *mc, const String &uid, int numVoices);;
	~HardcodedPolyphonicFX() override;

	float getAttribute(int parameterIndex) const override;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	bool hasTail() const override;;
	bool isSuspendedOnSilence() const final override;

	Processor *getChildProcessor(int index) override;;
	const Processor *getChildProcessor(int index) const override;;
	int getNumChildProcessors() const override { return (int)modChains.size(); ; };
	int getNumInternalChains() const override { return (int)modChains.size(); };

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) final override;
	void startVoice(int voiceIndex, const HiseEvent& e) final override;
	void applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) final override;
	void renderData(ProcessDataDyn& data) override;
	void renderNextBlock(AudioSampleBuffer &/*buffer*/, int /*startSample*/, int /*numSamples*/) override;
	void reset(int voiceIndex) override;
	void handleHiseEvent(const HiseEvent &m) override;
	void connectionChanged() override;;

	void numSourceChannelsChanged() override {};
	void numDestinationChannelsChanged() override {};

	/** renders a voice and applies the effect on the voice. */
	void renderVoice(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples) override;

	int getNumActiveVoices() const override;
	bool isVoiceResetActive() const override;
	void onVoiceReset(bool allVoices, int voiceIndex) override;

	bool setEffect(const String& effectName, bool) override;

	ModulatorChain::Collection* getModulationChains() override { return &modChains; }
	const ModulatorChain::Collection* getModulationChains() const override { return &modChains; }

	void connectToRuntimeTargets(scriptnode::OpaqueNode& on, bool shouldAdd) override;

	ModulationDisplayValue::QueryFunction::Ptr getModulationQueryFunction(int parameterIndex) const override;

private:

	VoiceDataStack voiceStack;
	ModulatorChain::ExtraModulatorRuntimeTargetSource extraModSources;
};

class HardcodedTimeVariantModulator: public TimeVariantModulator,
                                     public HardcodedSwappableEffect
{
public:
    
    SET_PROCESSOR_NAME("Hardcoded Timevariant Modulator", "HardcodedTimeVariantModulator", "Atime variant modulator wrapper around a compiled DSP network");

    HardcodedTimeVariantModulator(MainController* mc, const String& uid, Modulation::Mode m);
    ~HardcodedTimeVariantModulator() override;;

    Processor *getChildProcessor(int processorIndex) override { return nullptr; };
    const Processor *getChildProcessor(int processorIndex) const override { return nullptr; }

    int getNumInternalChains() const override { return 0; };
    int getNumChildProcessors() const override { return 0; };

	StringArray getIllegalParameterIds() const override
	{
		auto sa = HardcodedSwappableEffect::getIllegalParameterIds();
		sa.add("Intensity");
		return sa;
	}

    void setInternalAttribute(int index, float newValue) override;
    ValueTree exportAsValueTree() const override;
    void restoreFromValueTree(const ValueTree& v) override;
    float getAttribute(int index) const override;

    bool checkHardcodedChannelCount() override;
    
    ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void handleHiseEvent(const HiseEvent &m) override;
    void calculateBlock(int startSample, int numSamples) override;
    Result prepareOpaqueNode(OpaqueNode* n) override;
};


class HardcodedEnvelopeModulator: public EnvelopeModulator,
								  public HardcodedSwappableEffect,
								  public snex::Types::VoiceResetter
{
public:

	SET_PROCESSOR_NAME("Hardcoded Envelope Modulator", "HardcodedEnvelopeModulator", "A envelope modulator wrapper around a compiled DLL node");

	HardcodedEnvelopeModulator(MainController* mc, const String& id, int numVoices, Modulation::Mode m);
	~HardcodedEnvelopeModulator() override;

	// Child processor methods
	Processor *getChildProcessor(int processorIndex) override { return nullptr; };
    const Processor *getChildProcessor(int processorIndex) const override { return nullptr; }
	int getNumInternalChains() const override { return 0; };
    int getNumChildProcessors() const override { return 0; };

	// ValueTree methods
    ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree& v) override;

	StringArray getIllegalParameterIds() const override
	{
		auto sa = HardcodedSwappableEffect::getIllegalParameterIds();
		sa.add("Intensity");
		sa.add("Monophonic");
		sa.add("Retrigger");
		return sa;
	}

	// Attribute methods
	int getParameterOffset() const override;
	void setInternalAttribute(int index, float newValue) override;
	float getAttribute(int index) const override;
	float getDefaultValue(int index) const override;

	bool checkHardcodedChannelCount() override;

	ProcessorEditorBody* createEditor(ProcessorEditor* parentEditor) override;

	// voice resetter methods
	void onVoiceReset(bool allVoices, int voiceIndex) override;
	bool isVoiceResetActive() const override;
	int getNumActiveVoices() const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	Result prepareOpaqueNode(scriptnode::OpaqueNode *n) override;

	// envelope callbacks
	float startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;
	bool isPlaying(int voiceIndex) const override;
	void calculateBlock(int startSample, int numSamples) override;
	void handleHiseEvent(const HiseEvent& m) override;

	ModulatorState *createSubclassedState(int voiceIndex) const override;;

private:

	HiseEvent nextEvent;
	VoiceDataStack voiceStack;
};

class HardcodedSynthesiser: public ModulatorSynth,
							public HardcodedSwappableEffect,
							public snex::VoiceResetter
{
	public:

	SET_PROCESSOR_NAME("HardcodedSynth", "Hardcoded Synthesiser", "A polyphonic synthesiser that uses a compiled network as sound generator.");

	struct Sound : public ModulatorSynthSound
	{
		bool appliesToNote(int ) final;
		bool appliesToChannel(int ) final;
		bool appliesToVelocity(int ) final;
	};

	struct Voice : public ModulatorSynthVoice
	{
		Voice(HardcodedSynthesiser* p);;

		void calculateBlock(int startSample, int numSamples) override;
		void setVoiceStartDataForNextRenderCallback() { isVoiceStart = true; }
		void resetVoice() override;
		void prepareToPlay(double sampleRate, int samplesPerBlock) override;

		HardcodedSynthesiser* synth;
		bool isVoiceStart = false;
	};

	HardcodedSynthesiser(MainController* mc, const String& id, int numVoices);
	~HardcodedSynthesiser() override;

	StringArray getIllegalParameterIds() const override
	{
		auto sa = HardcodedSwappableEffect::getIllegalParameterIds();
		sa.add("Gain");
		sa.add("Balance");
		sa.add("VoiceLimit");
		sa.add("KillFadeTime");
		sa.add("IconColour");
		return sa;
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void connectionChanged() override;

	void preHiseEventCallback(HiseEvent &e) override;
	void preStartVoice(int voiceIndex, const HiseEvent& e) override;
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	// child processor methods
	Processor* getChildProcessor(int processorIndex) override;
	const Processor* getChildProcessor(int processorIndex) const override;
	int getNumInternalChains() const override;
	int getNumChildProcessors() const override;

	// value tree methods
	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	// parameter methods
	float getAttribute(int parameterIndex) const override;
	void setInternalAttribute(int parameterIndex, float newValue) override;
	float getDefaultValue(int parameterIndex) const override;

	int getParameterOffset() const override { return ModulatorSynth::Parameters::numModulatorSynthParameters; }

	// extra mod methods
	void connectToRuntimeTargets(scriptnode::OpaqueNode& opaqueNode, bool shouldAdd) override;
	ModulationDisplayValue::QueryFunction::Ptr getModulationQueryFunction(int parameterIndex) const override;
	ModulatorChain::Collection* getModulationChains() override { return &modChains; }
	const ModulatorChain::Collection* getModulationChains() const override { return &modChains; }

	bool setEffect(const String& effectName, bool cond) override;

	// voice resetter methods
	void onVoiceReset(bool allVoices, int voiceIndex) override;
	bool isVoiceResetActive() const override;
	int getNumActiveVoices() const override;

	int getExtraModulationIndex(int modulationSlotIndexWithoutOffset) const override
	{
		return modulationSlotIndexWithoutOffset + extraModSources.getExtraOffset();
	}

private:

	ModulatorChain::ExtraModulatorRuntimeTargetSource extraModSources;
	VoiceDataStack voiceData;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HardcodedSynthesiser);
};


}