/*
  ==============================================================================

    ControlModulator.h
    Created: 24 Jun 2014 1:29:30pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef CONTROLMODULATOR_H_INCLUDED
#define CONTROLMODULATOR_H_INCLUDED

 

/**	A ControlModulator is a non polyphonic TimeVariantModulator which processes midi CC messages.
*
*	@ingroup modulatorTypes
*
*	It uses a simple low pass filter to smooth value changes.  
*/
class ControlModulator: public TimeVariantModulator,
						public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("MidiController", "Midi Controller")

	/** Special Parameters for the ControlModulator. */
	enum Parameters
	{
		Inverted = 0, ///< inverts the modulation.
		UseTable, ///< use a Table object for a look up table
		ControllerNumber, ///< the controllerNumber that this controller reacts to.
		SmoothTime, ///< the smoothing time
		DefaultValue, ///< the default value before a control message is received.
		numSpecialParameters
	};

	ControlModulator(MainController *mc, const String &id, Modulation::Mode m);
	~ControlModulator();

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;;

	Processor *getChildProcessor(int /*processorIndex*/) override final {return nullptr;};
	const Processor *getChildProcessor(int /*processorIndex*/) const override final {return nullptr;};
	int getNumChildProcessors() const override final {return 0;};

	float getAttribute (int parameter_index) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;

	void calculateBlock(int startSample, int numSamples) override;;
	void handleMidiEvent (const MidiMessage &m) override;
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	Table *getTable(int = 0) const override { return table; };

	void enableLearnMode() { learnMode = true; };
	void disableLearnMode() { learnMode = false; sendChangeMessage(); }
	bool learnModeActive() const { return learnMode; }

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

private:

	float calculateNewValue();

	int controllerNumber;
	float defaultValue;
	bool inverted;
	float smoothTime;
	bool useTable;

	float polyValues[128];

	bool learnMode;
	float targetValue;
	int64 uptime;
	float inputValue;

	// the smoothed version of the target value
	float currentValue;
	float lastCurrentValue;
	float intensity;

	Smoother smoother;
	ScopedPointer<SampleLookupTable> table;
};



#endif  // CONTROLMODULATOR_H_INCLUDED
