/*
  ==============================================================================

    CCEnvelope.h
    Created: 31 Aug 2015 11:51:44am
    Author:  Christoph

  ==============================================================================
*/

#ifndef CCENVELOPE_H_INCLUDED
#define CCENVELOPE_H_INCLUDED

 


class IntegerStack
{
public:

	void push(int newNumber)
	{
		data.insert(-1, newNumber);
	}

	void remove(int number)
	{
		data.removeAllInstancesOf(number);
	}

	int getCurrentNumber() const
	{
		return data.getLast();
	}

private:

	Array<int> data;

	int currentIndex;

};


/** @ingroup modulatorTypes


*/
class CCEnvelope : public EnvelopeModulator,
				   public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("CCEnvelope", "Midi CC Attack Envelope")

	/// @brief special parameters for SimpleEnvelope
	enum SpecialParameters
	{
		UseTable,
		ControllerNumber, ///< the controllerNumber that this controller reacts to.
		SmoothTime, ///< the smoothing time
		DefaultValue, ///< the default value before a control message is received.
		StartLevel,
		HoldTime,
		EndLevel,
		FixedNoteOff,
		DecayTime,
		numSpecialParameters
	};

	enum InternalChains
	{
		StartLevelChain = 0,
		HoldChain,
		EndLevelChain,
		DecayChain,
		numTotalChains
	};

	enum EditorStates
	{
		StartLevelChainShown = Processor::numEditorStates,
		HoldChainShown,
		EndLevelChainShown,
		DecayChainShown,
		numEditorStates
	};

	CCEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m);

	~CCEnvelope();

	void restoreFromValueTree(const ValueTree &v) override;
	ValueTree exportAsValueTree() const override;

	float getAttribute(int parameter_index) const;
	void setInternalAttribute(int parameter_index, float newValue) override;

	int getNumInternalChains() const override;
	int getNumChildProcessors() const override;
	Processor *getChildProcessor(int processorIndex) override;
	const Processor *getChildProcessor(int processorIndex) const override;;

	void startVoice(int voiceIndex) override;
	void stopVoice(int voiceIndex) override;
	void reset(int voiceIndex) override;
	bool isPlaying(int voiceIndex) const override;

	void calculateBlock(int startSample, int numSamples) override;;
	void handleMidiEvent(MidiMessage const &m);
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	float calcCoefficient(float time) const { return 1.0f / ((time / 1000.0f) * (float)this->getSampleRate()); };
	
	ModulatorState *createSubclassedState(int voiceIndex) const override { return new CCEnvelopeState(voiceIndex); };

	Table *getTable(int = 0) const override { return table; };

	bool learnModeActive() const { return learnMode; }
	void enableLearnMode() { learnMode = true; };
	void disableLearnMode() { learnMode = false; sendChangeMessage(); }

	float getCurrentCCValue() const noexcept
	{
		return smoothedCCValue;
	}

private:

	struct CCEnvelopeState : public EnvelopeModulator::ModulatorState
	{
	public:

		CCEnvelopeState(int voiceIndex) :
			ModulatorState(voiceIndex),
			current_state(IDLE),
			uptime(0),
			holdTimeInSamples(0),
			decayTimeInSamples(0),
			actualValue(1.0f),
			idleValue(1.0f)
		{};

		enum EnvelopeState ///< The internal states that this envelope has.
		{
			HOLD, ///< attack phase (isPlaying() returns \c true)
			DECAY, ///< sustain phase (isPlaying() returns \c true)
			SUSTAIN, ///< attack phase (isPlaying() returns \c true)
			IDLE ///< idle state (isPlaying() returns \c false.
		};

		float voiceStartLevel;
		float voiceEndLevel;

		EnvelopeState current_state;
		int64 uptime;
		int holdTimeInSamples;
		int decayTimeInSamples;

		float idleValue;

		float actualValue;
	};

	float calculateNewValue();

	float getSmoothedCCValue(float targetValue)
	{
		smoothedCCValue = smoother.smooth(targetValue);

		return smoothedCCValue;
	}

	int dutyVoice; // the first active voice will handle the smoothing

	ScopedPointer<ModulatorChain> startLevelChain;
	ScopedPointer<ModulatorChain> holdChain;
	ScopedPointer<ModulatorChain> endLevelChain;
	ScopedPointer<ModulatorChain> decayChain;

	ScopedPointer<SampleLookupTable> table;

	float defaultValue;
	float smoothTime;
	bool useTable;
	int controllerNumber;
	float hold;
	float decay;
	float startLevel;
	float endLevel;

	bool fixedNoteOff;


	bool learnMode;
	float inputValue;
	float targetValue;
	
	float smoothedCCValue;

	

	Smoother smoother;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CCEnvelope)
};

#endif  // CCENVELOPE_H_INCLUDED
