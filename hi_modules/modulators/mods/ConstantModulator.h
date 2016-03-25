/*
  ==============================================================================

    ConstantModulator.h
    Created: 15 Jun 2014 1:43:59pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef CONSTANTMODULATOR_H_INCLUDED
#define CONSTANTMODULATOR_H_INCLUDED

/** This modulator simply returns a constant value that can be used to change the gain or something else.
*
*	@ingroup modulatorTypes
*/
class ConstantModulator: public VoiceStartModulator
{
public:

	SET_PROCESSOR_NAME("Constant", "Constant")	

	/// Additional Parameters for the constant modulator
	enum SpecialParameters
	{
		numTotalParameters
	};

	ConstantModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m):
		VoiceStartModulator(mc, id, numVoices, m),
		Modulation(m)
	{ };

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	/// sets the constant value. The only valid parameter_index is Intensity
	void setInternalAttribute(int, float ) override
	{
	};

	/// returns the constant value
	float getAttribute(int ) const override
	{
		return 0.0f;
	};

	

	/** Returns the 0.0f and let the intensity do it's job. */
	float calculateVoiceStartValue(const MidiMessage &) override
	{
		return (getMode() == GainMode) ? 0.0f : 1.0f;
	};

private:

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConstantModulator)
};



#endif  // CONSTANTMODULATOR_H_INCLUDED
