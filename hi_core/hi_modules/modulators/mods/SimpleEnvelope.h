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

#ifndef SIMPLEENVELOPE_H_INCLUDED
#define SIMPLEENVELOPE_H_INCLUDED

namespace hise { using namespace juce;


/** @ingroup modulatorTypes

The most simple envelope (only attack and release).

It has an attack and release time that can be modified by an internal modulator chain, which are calculated at the note-on (attack) and note-off (release) time.
You have to specify a sampleRate using the prepareToPlay method or the modulator will not be working!

ID | Parameter | Description
-- | --------- | -----------
0 | Attack | the attack time in milliseconds
1 | Release | the release time in milliseconds

It simply ramps up the signal linearly, so it does not sound very good when used for longer attack / release times.
*/
class SimpleEnvelope: public EnvelopeModulator	
{
public:

	SET_PROCESSOR_NAME("SimpleEnvelope", "Simple Envelope", "The most simple envelope (only attack and release).")

	/// @brief special parameters for SimpleEnvelope
	enum SpecialParameters
	{
		Attack = EnvelopeModulator::Parameters::numParameters, ///< the attack time in milliseconds
		Release, ///< the release time in milliseconds
		LinearMode, ///< toggles between linear and exponential mode
		numTotalParameters
	};

	enum InternalChains
	{
		AttackChain = 0,
		//ReleaseChain,
		numTotalChains
	};

	enum EditorStates
	{
		AttackChainShown = Processor::numEditorStates,
		//ReleaseChainShown,
		numEditorStates
	};

	SimpleEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);
	~SimpleEnvelope();

	void setInternalAttribute(int parameter_index, float newValue) override;
	float getDefaultValue(int parameterIndex) const override;
	float getAttribute(int parameter_index) const;

	void restoreFromValueTree(const ValueTree &v) override;
	ValueTree exportAsValueTree() const override;

	int getNumInternalChains() const override { return getNumChildProcessors(); };
	int getNumChildProcessors() const override { return numTotalChains; };
	Processor *getChildProcessor(int ) override { return attackChain; };
	const Processor *getChildProcessor(int ) const override  { return attackChain; };

	float startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;
	bool isPlaying(int voiceIndex) const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;
	void handleHiseEvent(const HiseEvent& m) override;
	
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

    /** @internal The container for the envelope state. */
    struct SimpleEnvelopeState: public EnvelopeModulator::ModulatorState
	{
	public:

		SimpleEnvelopeState(int voiceIndex): 
			ModulatorState(voiceIndex),
			current_state(IDLE), 
			uptime(0),
			current_value(0.0f),
			attackDelta(1.0f)
		{};

		/** The internal states that this envelope has */
		enum EnvelopeState
		{
			ATTACK, ///< attack phase (isPlaying() returns \c true)
			SUSTAIN, ///< sustain phase (isPlaying() returns \c true)
			RETRIGGER, ///< retrigger state (only valid in monophonic mode
			RELEASE, ///< attack phase (isPlaying() returns \c true)
			IDLE ///< idle state (isPlaying() returns \c false.
		};

        /// the uptime
		int uptime;
		float current_value;

		/** The ratio in which the attack time is altered. This is calculated by the internal ModulatorChain attackChain*/
		float attackDelta;
		
		
		float expAttackCoef;
		float expAttackBase;

		float expReleaseCoef;
		float expReleaseBase;

		EnvelopeState current_state;
	};

	ModulatorState *createSubclassedState(int voiceIndex) const override {return new SimpleEnvelopeState(voiceIndex); };

private:

	float calcCoefficient(float time, float targetRatio=1.0f) const;

	void setAttackRate(float rate, SimpleEnvelopeState* state=nullptr);
	void setReleaseRate(float rate);
	
	/** @brief returns the envelope value. 
	
	The calculation is linear and not logarithmic, so it may be sounding cheep
	*/
	float calculateNewValue(int voiceIndex);
	float calculateNewExpValue();

	float inputValue;
	float attack;
	float release;
	float release_delta;

	float expAttackDelta;
	float expAttackCoef;
	float expAttackBase;

	float expReleaseDelta;
	float expReleaseCoef;
	float expReleaseBase;

	bool linearMode;

	ScopedPointer<ModulatorChain> attackChain;

	SimpleEnvelopeState *state;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEnvelope);
	JUCE_DECLARE_WEAK_REFERENCEABLE(SimpleEnvelope);
};


} // namespace hise

#endif  // SIMPLEENVELOPE_H_INCLUDED
