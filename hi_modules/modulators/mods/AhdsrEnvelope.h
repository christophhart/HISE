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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#ifndef HI_AHDSRENVELOPE_H_INCLUDED
#define HI_AHDSRENVELOPE_H_INCLUDED


/** @brief A pretty common envelope type with 5 states. @ingroup modulatorTypes

### AHDSR Envelope

A pretty common envelope type with 5 states. (Go to http://en.wikiaudio.org/ADSR_envelope for a general description on how an envelope works)

The Modulator has five states: Attack, Hold, Decay, Sustain and Release and allows modulation of 
the attack time and level, the decay time and the release time with VoiceStartModulators.

Unlike the [SimpleEnvelope](#SimpleEnvelope), this envelope has a exponential curve, so it sounds nicer (but is a little bit more resource-hungry).

ID | Parameter | Description
-- | --------- | -----------
0 | Attack | the attack time in milliseconds
1 | AttackLevel | the attack level in decibel
2 | Hold | the hold time in milliseconds
3 | Decay | the decay time in milliseconds
4 | Sustain | the sustain level in decibel
5 | Release | the release time in milliseconds
*/
class AhdsrEnvelope: public EnvelopeModulator	
{
public:

	SET_PROCESSOR_NAME("AHDSR", "AHDSR Envelope")

	/// @brief special parameters for AhdsrEnvelope
	enum SpecialParameters
	{
		Attack,		 ///< the attack time in milliseconds
		AttackLevel, ///< the attack level in decibel
		Hold,		 ///< the hold time in milliseconds
		Decay,		 ///< the decay time in milliseconds
		Sustain,	 ///< the sustain level in decibel
		Release,	 ///< the release time in milliseconds
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

	enum InternalChains
	{
		AttackTimeChain = 0,
		AttackLevelChain,
		DecayTimeChain,
		SustainLevelChain,
		ReleaseTimeChain,
		numInternalChains
	};

	AhdsrEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
	~AhdsrEnvelope() {};

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;
	int getNumChildProcessors() const override { return numInternalChains; };
	int getNumInternalChains() const override {return numInternalChains;};

	void startVoice(int voiceIndex) override;	
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;;

	void calculateBlock(int startSample, int numSamples);;

	void handleMidiEvent(MidiMessage const &m);
	

	ProcessorEditorBody *createEditor(BetterProcessorEditor* parentEditor) override;

	

	float getDefaultValue(int parameterIndex) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;;


	float getAttribute(int parameter_index) const override;;


	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	/** @brief returns \c true, if the envelope is not IDLE and not bypassed. */
	bool isPlaying(int voiceIndex) const override;;
    
	
    
	float calcAttackRate(float newRate);

	
    /** The container for the envelope state. */
    struct AhdsrEnvelopeState: public EnvelopeModulator::ModulatorState
	{
	public:

		AhdsrEnvelopeState(int voiceIndex, const AhdsrEnvelope *ownerEnvelope):
			ModulatorState(voiceIndex),
			current_state(IDLE),
			holdCounter(0),
			envelope(ownerEnvelope),
			current_value(0.0f)
		{
			for(int i = 0; i < numInternalChains; i++) modValues[i] = 1.0f;
		};

		/** The internal states that this envelope has */
		enum EnvelopeState
		{
			ATTACK, ///< attack phase (isPlaying() returns \c true)
			HOLD, ///< hold phase
			DECAY, ///< decay phase
			SUSTAIN, ///< sustain phase (isPlaying() returns \c true)
			RELEASE, ///< attack phase (isPlaying() returns \c true)
			IDLE ///< idle state (isPlaying() returns \c false.
		};

		/** Calculate the attack rate for the state. If the modulation value is 1.0f, they are simply copied from the envelope. */
		void setAttackRate(float rate);;
		
		/** Calculate the decay rate for the state. If the modulation value is 1.0f, they are simply copied from the envelope. */
		void setDecayRate(float rate);;
		
		/** Calculate the release rate for the state. If the modulation value is 1.0f, they are simply copied from the envelope. */
		void setReleaseRate(float rate);;

		const AhdsrEnvelope *envelope;

        /// the uptime
		int holdCounter;
		float current_value;

		/** The ratio in which the attack time is altered. This is calculated by the internal ModulatorChain attackChain*/
		float modValues[numInternalChains];

		float attackLevel;
		float attackCoef;
		float attackBase;

		float decayCoef;
		float decayBase;

		float releaseCoef;
		float releaseBase;
		float release_delta;

		EnvelopeState current_state;
	};

	ModulatorState *createSubclassedState(int voiceIndex) const override {return new AhdsrEnvelopeState(voiceIndex, this); };

private:

	void setAttackRate(float rate);
	void setDecayRate(float rate);
	void setReleaseRate(float rate);
	void setSustainLevel(float level);
	void setHoldTime(float holdTimeMs);
	void setTargetRatioA(float targetRatio);
	void setTargetRatioDR(float targetRatio);

	float calcCoef(float rate, float targetRatio) const;

	float calculateNewValue ();

	float inputValue;

	float attack;
	float attackCoef;
	float attackBase;
	float targetRatioA;

	float attackLevel;

	float hold;
	float holdTimeSamples;

	float decay;
	float decayCoef;
	float decayBase;
	float targetRatioDR;

	float sustain;

	float release;
	float releaseCoef;
	float releaseBase;

	AhdsrEnvelopeState *state;

	float release_delta;

	OwnedArray<ModulatorChain> internalChains;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AhdsrEnvelope)
};




#endif