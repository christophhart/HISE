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

#ifndef TABLEENVELOPE_H_INCLUDED
#define TABLEENVELOPE_H_INCLUDED

namespace hise { using namespace juce;

/** A Envelope that uses two Tables for the attack and release time.
*	@ingroup modulatorTypes
*
*	It uses an internal uptime counter to switch between states to allow the tables to reach 1.0 or 0.0 without accidently switching states. 
*	If the release phase is started, while the attack phase was still active, it adjust its release gain to prevent a value jump.
*
*	The TableEnvelopeEditor has two TableEditors that display the time in the domain (converted from samples) and two ModulatorChainEditors.
*/
class TableEnvelope: public EnvelopeModulator,
					 public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("TableEnvelope", "Table Envelope", "A Envelope that uses two Tables for the attack and release time.")


	/** SpecialParameters for the TableEnvelope */
	enum SpecialParameters
	{
		Attack = EnvelopeModulator::Parameters::numParameters, ///< the attack time in milliseconds
		Release, ///< the release time in milliseconds
		numTotalParameters
	};

	enum InternalChains
	{
		AttackChain = 0,
		ReleaseChain,
		numTotalChains
	};

	enum EditorStates
	{
		AttackChainShown = Processor::numEditorStates,
		ReleaseChainShown,
		numEditorStates
	};

	TableEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m, float attackTimeMs=20.0f, float releaseTimeMs=20.0f);

	~TableEnvelope();

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;

	int getNumInternalChains() const override
	{
		return numTotalChains;
	}

	Processor *getChildProcessor(int processorIndex) override
	{
		if(processorIndex == AttackChain) return attackChain;
		else							  return releaseChain;
	};

	const Processor *getChildProcessor(int processorIndex) const override
	{
		if(processorIndex == AttackChain) return attackChain;
		else							  return releaseChain;
	};

	int getNumChildProcessors() const override {return numTotalChains;};

	float startVoice(int voiceIndex) override;
	
	void stopVoice(int voiceIndex) override;

	void calculateBlock(int startSample, int numSamples) override;;

	void reset(int voiceIndex) override;;

	/// @brief handles note-on and note-off messages and switches the internal state
	void handleHiseEvent(const HiseEvent &m) override;
	
	float getAttribute (int parameterIndex) const
	{
		if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
		{
			return EnvelopeModulator::getAttribute(parameterIndex);
		}

		switch(parameterIndex)
		{
			case Attack:				return attack;
			case Release:				return release;
			default:					jassertfalse; return -1;
		}	
	};

		/** sets the envelope time and calculates the delta values per sample */
	void setInternalAttribute (int parameterIndex, float newValue) override
	{
		if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
		{
			EnvelopeModulator::setInternalAttribute(parameterIndex, newValue);
			return;
		}

		switch(parameterIndex)
		{
			case Attack:
				attack = newValue;
				attackUptimeDelta = calculateTableDelta(newValue);
				break;
			case Release:
				release = newValue;
				releaseUptimeDelta = calculateTableDelta(newValue);
				break;
			default:
				jassertfalse;
		}	
	};

	float getDefaultValue(int parameterIndex) const
	{
		if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
		{
			return EnvelopeModulator::getDefaultValue(parameterIndex);
		}

		switch (parameterIndex)
		{
		case Attack:
			return 20.0f;
		case Release:
			return 20.0f;
		default:
			jassertfalse;
			return -1;
		}
	}

	double calculateTableDelta(float ms)
	{
		const auto numSamplesForMs  = (double)ms * getControlRate() / 1000.0;

		if (numSamplesForMs == 0.0)
			return (double)SAMPLE_LOOKUP_TABLE_SIZE;

		return (double)SAMPLE_LOOKUP_TABLE_SIZE / numSamplesForMs;
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	/** @brief returns \c true, if the envelope is not IDLE and not bypassed. */
	bool isPlaying(int voiceIndex) const override;;

	/** @internal The container for the envelope state. */
    struct TableEnvelopeState: public EnvelopeModulator::ModulatorState
	{
	public:

		TableEnvelopeState(int voiceIndex):
			ModulatorState(voiceIndex),
			current_state(IDLE),
			uptime(0),
			current_value(0.0f),
			releaseGain(1.0f),
			attackModValue(1.0f),
			releaseModValue(1.0f)
		{

		};

		/** The internal states that this envelope has */
		enum EnvelopeState
		{
			ATTACK, ///< attack phase (isPlaying() returns \c true)
			SUSTAIN, ///< sustain phase (isPlaying() returns \c true)
			RETRIGGER, ///< retrigger phase (in monophonic mode)
			RELEASE, ///< attack phase (isPlaying() returns \c true)
			IDLE ///< idle state (isPlaying() returns \c false.
		};

		float current_value;

		float attackModValue;
		float releaseModValue;

		float releaseGain;
		
		float uptime;

		EnvelopeState current_state;

	};

	ModulatorState *createSubclassedState(int voiceIndex) const override {return new TableEnvelopeState(voiceIndex); };

	void referenceShared(ExternalData::DataType dt, int index) override
    {
        if(index == 0)
        {
	        attackTable = getTableUnchecked(index);
        }
		if(index == 1)
        {
	        releaseTable = getTableUnchecked(index);
        }

		updateTables();
    }

private:

	void updateTables();

	hise::ExecutionLimiter<DummyCriticalSection> uiUpdater;

	double attackUptimeDelta = 1.0;
	double releaseUptimeDelta = 1.0;

	float calculateNewValue(int voiceIndex);

	ScopedPointer<ModulatorChain> attackChain;
	ScopedPointer<ModulatorChain> releaseChain;

	SampleLookupTable* attackTable;
	SampleLookupTable* releaseTable;

	float attack, release;
};


} // namespace hise

#endif  // TABLEENVELOPE_H_INCLUDED

