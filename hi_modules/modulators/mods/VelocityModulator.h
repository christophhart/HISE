/*
  ==============================================================================

    VelocityModulator.h
    Created: 15 Jun 2014 1:42:25pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef VELOCITYMODULATOR_H_INCLUDED
#define VELOCITYMODULATOR_H_INCLUDED



/** This modulator changes the output depending on the velocity of note on messages.
*	@ingroup modulatorTypes
*
*/
class VelocityModulator: public VoiceStartModulator,
						 public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("Velocity", "Velocity Modulator")

	/// Additional parameters
	enum SpecialParameters
	{
		Inverted = 0, ///< On, **Off** | if `true`, then the modulator works inverted, so that high velocity values are damped.
		UseTable, ///< On, **Off** | if `true` then a look up table is used to calculate the value
		numTotalParameters
	};

	VelocityModulator(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m):
		VoiceStartModulator(mc, id, voiceAmount, m),
		Modulation(m),
		tableUsed(false),
		inverted(false),
		velocityTable(new MidiTable())
	{ 
		parameterNames.add("Inverted");
		parameterNames.add("UseTable");
	};

	void restoreFromValueTree(const ValueTree &v) override
	{
		VoiceStartModulator::restoreFromValueTree(v);

		loadAttribute(UseTable, "UseTable");
		loadAttribute(Inverted, "Inverted");

		if(tableUsed) loadTable(velocityTable, "VelocityTableData");
		
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = VoiceStartModulator::exportAsValueTree();

		saveAttribute(UseTable, "UseTable");
		saveAttribute(Inverted, "Inverted");

		if(tableUsed) saveTable(velocityTable, "VelocityTableData");

		return v;
	}

	void setInternalAttribute(int p, float newValue) override
	{
		switch(p)
		{
		case Inverted:
			inverted = newValue == 1.0f;
			break;
		case UseTable:
			tableUsed = newValue == 1.0f;
			break;
		}
	}

	float getAttribute(int p) const
	{
		switch(p)
		{
		case Inverted:	return inverted;
		case UseTable:	return tableUsed;

		// This should not happen!
		default: jassertfalse; return 0.0f;
		}
	}
	
	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;


	float calculateVoiceStartValue(const MidiMessage &m) override
	{
		float value = m.getFloatVelocity();

		


		if(inverted) value = 1.0f - value;

		if (tableUsed)
		{
			value = velocityTable->get((int)(value * 127));
			sendTableIndexChangeMessage(false, velocityTable, m.getFloatVelocity());
		}
			
		debugMod(String(value, 2));

		return value;
	};


	Table *getTable(int =0) const override
	{
		return velocityTable;
	};

	/// \brief enables the look up table
	void setUseTable(bool enableLookUpTable)
	{
		tableUsed = enableLookUpTable;
	};

private:

	float inputValue;

	ScopedPointer<MidiTable> velocityTable;

	/// checks if the look up table should be used
	bool tableUsed;

	bool inverted;

	ScopedPointer<XmlElement> tableGraph;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VelocityModulator)
};



#endif  // VELOCITYMODULATOR_H_INCLUDED
