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
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;

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

	SET_PROCESSOR_NAME("KeyNumber", "Notenumber Modulator", "Creates a modulation value based on the note-number.")

	KeyModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m);

	void restoreFromValueTree(const ValueTree &v) override
	{
		VoiceStartModulator::restoreFromValueTree(v);
		loadTable(getTableUnchecked(0), "MidiTableData");
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = VoiceStartModulator::exportAsValueTree();
		saveTable(getTableUnchecked(0), "MidiTableData");
		return v;
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	void setInternalAttribute(int parameterIndex, float newValue) override
	{

	};

	float getAttribute(int parameterIndex) const override
	{
		return 0.0f;
	};

	/** Calculates a new random value. If the table is used, it is converted to 7bit.*/
	float calculateVoiceStartValue(const HiseEvent &m) override
	{
        return getTableUnchecked(0)->getInterpolatedValue(m.getNoteNumber() / 127.0, sendNotificationAsync);
	};
};


} // namespace hise

