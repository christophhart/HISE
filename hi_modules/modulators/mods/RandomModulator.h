/*
  ==============================================================================

    RandomModulator.h
    Created: 24 Jun 2014 3:49:09pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef RANDOMMODULATOR_H_INCLUDED
#define RANDOMMODULATOR_H_INCLUDED

/** A constant Modulator which calculates a random value at the voice start.
*	@ingroup modulatorTypes
*
*	It can use a look up table to "massage" the outcome in order to raise the probability of some values etc.
*	In this case, the values are limited to 7bit for MIDI feeling...
*/
class RandomModulator: public VoiceStartModulator,
					   public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("Random", "Random Modulator")

	/** Special Parameters for the Random Modulator */
	enum Parameters
	{
		UseTable = 0,
		numTotalParameters
	};

	RandomModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	void restoreFromValueTree(const ValueTree &v) override;;
	ValueTree exportAsValueTree() const override;

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override;;
	float getAttribute(int parameterIndex) const override;;

	/** Calculates a new random value. If the table is used, it is converted to 7bit.*/
	float calculateVoiceStartValue(const MidiMessage &) override;;

	/** returns a pointer to the look up table. Don't delete it! */
	Table *getTable(int=0) const override {return table; };

private:

	volatile float currentValue;
	bool useTable;
	Random generator;

	ScopedPointer<MidiTable> table;
};



#endif  // RANDOMMODULATOR_H_INCLUDED
