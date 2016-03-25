/*
  ==============================================================================

    SimpleEnvelope.h
    Created: 15 Jun 2014 1:42:40pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef SIMPLEENVELOPE_H_INCLUDED
#define SIMPLEENVELOPE_H_INCLUDED

class ExponentialStateCalculator
{

};

/** @ingroup modulatorTypes

### Simple Envelope

This modulator is the most simple envelope (only attack and release).

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

	SET_PROCESSOR_NAME("SimpleEnvelope", "Simple Envelope")

	/// @brief special parameters for SimpleEnvelope
	enum SpecialParameters
	{
		Attack, ///< the attack time in milliseconds
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

	void startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;
	bool isPlaying(int voiceIndex) const override;

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void calculateBlock(int startSample, int numSamples) override;
	void handleMidiEvent(MidiMessage const &m);
	
	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

    /** The container for the envelope state. */
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
			RELEASE, ///< attack phase (isPlaying() returns \c true)
			IDLE ///< idle state (isPlaying() returns \c false.
		};

        /// the uptime @todo check if needed
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

	void setAttackRate(float rate, int voiceIndex=-1);
	void setReleaseRate(float rate);
	
	/** @brief returns the envelope value. 
	
	The calculation is linear and not logarithmic, so it may be sounding cheep
	*/
	float calculateNewValue ();
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

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SimpleEnvelope)
};



#endif  // SIMPLEENVELOPE_H_INCLUDED
