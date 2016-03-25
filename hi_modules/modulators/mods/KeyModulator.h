/*
  ==============================================================================

    KeyModulator.h
    Created: 1 Aug 2014 9:32:32pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef KEYMODULATOR_H_INCLUDED
#define KEYMODULATOR_H_INCLUDED


/** A constant Modulator which calculates a random value at the voice start.
*	@ingroup modulatorTypes
*
*	It can use a look up table to "massage" the outcome in order to raise the probability of some values etc.
*	In this case, the values are limited to 7bit for MIDI feeling...
*/
class KeyModulator: public VoiceStartModulator,
					public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("KeyNumber", "Notenumber Modulator")

	/** Special Parameters for the Random Modulator */
	enum Parameters
	{
		TableMode = 0,
		numTotalParameters
		
	};

	enum Mode
	{
		KeyMode = 0,
		NumberMode,
		numModes
	};

	KeyModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	void restoreFromValueTree(const ValueTree &v) override
	{
		VoiceStartModulator::restoreFromValueTree(v);

		loadAttribute(TableMode, "Mode");

		loadTable(midiTable, "MidiTableData");
		loadTable(keyTable, "KeyTableData");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = VoiceStartModulator::exportAsValueTree();

		saveAttribute(TableMode, "Mode");

		saveTable(midiTable, "MidiTableData");
		saveTable(keyTable, "KeyTableData");

		return v;
	}

	

	ProcessorEditorBody *createEditor(BetterProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override
	{
		switch(parameterIndex)
		{
		case TableMode:			mode = (Mode)(int)newValue; break;
		default:				jassertfalse; break;
		}

		
	};

	float getAttribute(int parameterIndex) const override
	{
		switch(parameterIndex)
		{
		case TableMode:			return (float)mode;
		default:				jassertfalse; return -1.0f;
		}
	};

	/** Calculates a new random value. If the table is used, it is converted to 7bit.*/
	float calculateVoiceStartValue(const MidiMessage &m) override 
	{ 
		return getTable(mode)->getReadPointer()[m.getNoteNumber()]; 
	};

	void handleMidiEvent(const MidiMessage &m) override
	{
		VoiceStartModulator::handleMidiEvent(m);

		if(m.isNoteOnOrOff())
		{
			if (mode == Mode::NumberMode)
			{
				if(m.isNoteOn()) sendTableIndexChangeMessage(false, midiTable, (float)m.getNoteNumber() / 127.0f);
			}
			else
			{
				if (m.isNoteOn()) setInputValue((float)m.getNoteNumber());
				else			 setInputValue((float)-m.getNoteNumber());

				sendChangeMessage();
			}
		}
	}

	/** returns a pointer to the look up table. Don't delete it! */
	Table *getTable(int m) const override 
	{
		const Mode returnMode = (m != numModes) ? (Mode)m : mode;

		switch(returnMode)
		{
		case KeyMode:		return keyTable;
		case NumberMode:	return midiTable;
		default:			jassertfalse; return nullptr;
		}
		
	};

private:

	ScopedPointer<DiscreteTable> keyTable;
	ScopedPointer<MidiTable> midiTable;
	Mode mode;
};




#endif  // KEYMODULATOR_H_INCLUDED
