/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
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
