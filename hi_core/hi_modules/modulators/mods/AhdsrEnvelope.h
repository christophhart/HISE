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

#ifndef HI_AHDSRENVELOPE_H_INCLUDED
#define HI_AHDSRENVELOPE_H_INCLUDED

namespace hise { using namespace juce;





class AhdsrEnvelope: public EnvelopeModulator,
					 public scriptnode::envelope::pimpl::ahdsr_base
{
public:

	SET_PROCESSOR_NAME("AHDSR", "AHDSR Envelope", "")

	enum SpecialParameters
	{
		Attack = EnvelopeModulator::Parameters::numParameters,
		AttackLevel,
		Hold,
		Decay,
		Sustain,
		Release,
		AttackCurve,
		DecayCurve,
		EcoMode,
		numTotalParameters
	};

	enum EditorStates
	{
		AttackTimeChainShown = Processor::numEditorStates,
		AttackLevelChainShown,
		DecayTimeChainShown,
		SustainLevelChainShown,
		ReleaseTimeChainShown,
		numEditorStates
	};

	

	AhdsrEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
	~AhdsrEnvelope() {};

	const bool metadataInitialised;

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;
	int getNumChildProcessors() const override { return numInternalChains; };
	int getNumInternalChains() const override {return numInternalChains;};

	float startVoice(int voiceIndex) override;	
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;;

	void calculateBlock(int startSample, int numSamples);;

	void handleHiseEvent(const HiseEvent &e) override;

	ProcessorEditorBody *createEditor(ProcessorEditor* parentEditor) override;

	static ProcessorMetadata createMetadata();

	void setInternalAttribute (int parameter_index, float newValue) override;;

	float getAttribute(int parameter_index) const override;;

	ModulationDisplayValue::QueryFunction::Ptr getModulationQueryFunction(int parameterIndex) const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void setExternalData(const snex::ExternalData& d, int index) override
	{
		ahdsr_base::setExternalData(d, index);

		if (displayBuffer->getWriteBuffer().getNumSamples() > 0)
		{
			for (int i = 0; i < SpecialParameters::DecayCurve - SpecialParameters::Attack; i++)
			{
				displayBuffer->getWriteBuffer().setSample(0, i, getAttribute(i + SpecialParameters::Attack));
			}
		}
	}

	/** @brief returns \c true, if the envelope is not IDLE and not bypassed. */
	bool isPlaying(int voiceIndex) const override;;
    
	ModulatorState *createSubclassedState(int voiceIndex) const override;;

	

	struct StateInfo
	{
		bool operator ==(const StateInfo& other) const
		{
			return other.state == state;
		};

		state_base::EnvelopeState state = state_base::IDLE;
		double changeTime = 0.0;
	};

	StateInfo getStateInfo() const
	{
		return stateInfo;
	};

	class Panel : public PanelWithProcessorConnection
	{
	public:

		SET_PANEL_NAME("AHDSRGraph");

		Panel(FloatingTile* parent);

		Identifier getProcessorTypeId() const override { return AhdsrEnvelope::getClassType(); }
		Component* createContentComponent(int /*index*/) override;
		void fillModuleList(StringArray& moduleList) override;
	};

private:

	hise::ExecutionLimiter<DummyCriticalSection> ballUpdater;

	SimpleRingBuffer::Ptr displayBuffer;

	float calculateNewValue(int voiceIndex);

	struct AhdsrEnvelopeState : public EnvelopeModulator::ModulatorState,
								public state_base
	{
		AhdsrEnvelopeState(int voiceIndex, const ahdsr_base *ownerBase) :
			ModulatorState(voiceIndex),
			state_base()
		{
			this->envelope = ownerBase;
		};
	};

	StateInfo stateInfo;
	AhdsrEnvelopeState *state;

	ModulatorChain::Collection internalChains;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AhdsrEnvelope)
	JUCE_DECLARE_WEAK_REFERENCEABLE(AhdsrEnvelope);
};






} // namespace hise

#endif