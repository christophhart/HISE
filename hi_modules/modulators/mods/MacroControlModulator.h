/*
  ==============================================================================

    MacroControlModulator.h
    Created: 23 Sep 2014 11:08:23pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef MACROCONTROLMODULATOR_H_INCLUDED
#define MACROCONTROLMODULATOR_H_INCLUDED

/** A Modulator that enhances the macro control system.
*	@ingroup macroControl
*
*	It can be used if the traditional way of controlling parameters via the macro controls are not sufficient.
*	These are the following features:
*
*	- Sample Accurate. While the standard macro control protocoll operates on block size level, this allow sample accurate timing.
*	- Parameter Smoothing. Whenever fast parameter changes are intended, this class should be used to prevent zipping noises.
*	- Lookup Tables. Use a table to modify the control function.
*/
class MacroModulator: public TimeVariantModulator,
					  public MacroControlledObject,
					  public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("MacroModulator", "Macro Control Modulator")

	/** The special Parameters for the Modulator. */
	enum SpecialParameters
	{
		MacroIndex = 0, ///< the macro index of the target macro control
		SmoothTime, ///< the smoothing time
		UseTable, ///< use a look up table for the value calculation
		MacroValue,
		numSpecialParameters
	};

	MacroModulator(MainController *mc, const String &id, Modulation::Mode m);;

	void restoreFromValueTree(const ValueTree &v) override;;

	ValueTree exportAsValueTree() const override;;

#if USE_BACKEND

	Path getSpecialSymbol() const override;;
		
#endif

	/** Returns a new ControlEditor */
	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	virtual Processor *getChildProcessor(int /*processorIndex*/) override final {return nullptr;};

	virtual const Processor *getChildProcessor(int /*processorIndex*/) const override final {return nullptr;};

	virtual int getNumChildProcessors() const override final {return 0;};

	float getAttribute (int parameter_index) const override;
	void setInternalAttribute (int parameter_index, float newValue) override;

	/** sets the new target value if the controller number matches. */
	void handleMidiEvent (const MidiMessage &m) override;

	/** returns a pointer to the look up table. Don't delete it! */
	Table *getTable(int=0) const override {return table; };

	/** sets up the smoothing filter. */
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	void calculateBlock(int startSample, int numSamples) override;;

	void macroControllerMoved(float newValue);

	NormalisableRange<double> getRange() const final override { return NormalisableRange<double>(0.0, 1.0); };

private:

	// Do nothing, since the data is updated anyway...
	void updateValue() override {};

	void addToMacroController(int newMacroIndex) override;
	
	bool learnMode;

	Smoother smoother;

	int macroIndex;

	float currentMacroValue;

	float smoothTime;
	
	bool useTable;

	ScopedPointer<MidiTable> table;

	float inputValue;

	float currentValue;
	float targetValue;

};



#endif  // MACROCONTROLMODULATOR_H_INCLUDED
