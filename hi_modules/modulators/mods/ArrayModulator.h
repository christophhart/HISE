/*
  ==============================================================================

    ArrayModulator.h
    Created: 15 Mar 2016 7:35:52pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef ARRAYMODULATOR_H_INCLUDED
#define ARRAYMODULATOR_H_INCLUDED

/** This modulator simply returns a constant value that can be used to change the gain or something else.
*
*	@ingroup modulatorTypes
*/
class ArrayModulator : public VoiceStartModulator,
                       public SliderPackProcessor
{
public:

	SET_PROCESSOR_NAME("ArrayModulator", "Array Modulator")

		/// Additional Parameters for the constant modulator
	enum SpecialParameters
	{
		numTotalParameters
	};

	ArrayModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m) :
		VoiceStartModulator(mc, id, numVoices, m),
		Modulation(m),
		data(new SliderPackData())
	{
		data->setNumSliders(128);
		data->setRange(0.0, 1.0, 0.001);
        
        for(int i = 0; i < 128; i++)
        {
            data->setValue(i, 1.0f, dontSendNotification);
        }
	};

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	/// sets the constant value. The only valid parameter_index is Intensity
	void setInternalAttribute(int index, float value) override
	{
		data->setValue(index, value, sendNotification);
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = VoiceStartModulator::exportAsValueTree();
		
		v.setProperty("SliderPackData", data->toBase64(), nullptr);

		return v;
	}

	void restoreFromValueTree(const ValueTree &v) override
	{
		VoiceStartModulator::restoreFromValueTree(v);

		data->fromBase64(v.getProperty("SliderPackData"));
	}


	/// returns the constant value
	float getAttribute(int index) const override
	{
		return data->getValue(index);
	};


	/** Returns the 0.0f and let the intensity do it's job. */
	float calculateVoiceStartValue(const MidiMessage &m) override
	{
        const int number = m.getNoteNumber();
        
        data->setDisplayedIndex(number);
        
		return data->getValue(number);
	};

	SliderPackData *getSliderPackData(int /*index*/) override { return data; };

    const SliderPackData *getSliderPackData(int /*index*/) const override { return data; };

private:

	ScopedPointer<SliderPackData> data;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ArrayModulator)
};




#endif  // ARRAYMODULATOR_H_INCLUDED
