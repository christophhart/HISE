/*
  ==============================================================================

    CCDucker.h
    Created: 13 Dec 2015 1:52:01pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef CCDUCKER_H_INCLUDED
#define CCDUCKER_H_INCLUDED

class CCDucker : public TimeVariantModulator,
					   public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("CCDucker", "CC Ducker")

		/** The special Parameters for the Modulator. */
	enum SpecialParameters
	{
		CCNumber = 0,
		DuckingTime,
		SmoothingTime,
		numSpecialParameters
	};

	CCDucker(MainController *mc, const String &id, Modulation::Mode m);;

	~CCDucker() { table = nullptr; }

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;;

	/** Returns a new ControlEditor */
	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final { return nullptr; };
	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final { return nullptr; };
	virtual int getNumChildProcessors() const override final { return 0; };

	float getDefaultValue(int parameterIndex) const override;
	float getAttribute(int parameterIndex) const override;
	void setInternalAttribute(int parameterIndex, float newValue) override;

	/** sets the new target value if the controller number matches. */
	void handleMidiEvent(const MidiMessage &m) override;

	/** returns a pointer to the look up table. Don't delete it! */
	Table *getTable(int = 0) const override { return table; };

	/** sets up the smoothing filter. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;

	void calculateBlock(int startSample, int numSamples) override;;
	
private:

	void setDuckingTime(float newValue);
	float getNextValue();
	Smoother smoother;

	float smoothTime;
	float attackTime;
	int ccNumber;

	float currentValue;
	float targetValue;

	float currentUptime;
	float uptimeDelta;

	ScopedPointer<SampleLookupTable> table;

};




#endif  // CCDUCKER_H_INCLUDED
