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

#ifndef TRANSPOSER_H_INCLUDED
#define TRANSPOSER_H_INCLUDED

namespace hise { using namespace juce;

/** Transposes all midi note messages by the specified amount.
*	@ingroup midiTypes
*
*	If the amount is changed, a all note off message is sent to prevent hanging notes.
*/
class Transposer: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("Transposer", "Transposer");

	Transposer(MainController *mc, const String &id):
		MidiProcessor(mc, id),
		transposeAmount(0)
	{};


	~Transposer()
	{
		transposeAmount = 0;
	}

	enum SpecialParameters
	{
		TransposeAmount = 0
	};

	ValueTree exportAsValueTree() const override
	{
		ValueTree v = MidiProcessor::exportAsValueTree(); 

		saveAttribute(TransposeAmount , "TransposeAmount");
		
		return v;
	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		MidiProcessor::restoreFromValueTree(v);
    
		loadAttribute(TransposeAmount , "TransposeAmount");
	}

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	

	float getAttribute(int) const
	{
		return (float)transposeAmount;
	};

	void setInternalAttribute(int, float newAmount) override
	{
		transposeAmount = (int)newAmount;
	};

	void processHiseEvent(HiseEvent &m) noexcept override
	{
		if(m.isNoteOnOrOff())
		{
			const int noteNumber = m.getNoteNumber();
			m.setNoteNumber(noteNumber + transposeAmount);
		}
	}

private:

	int transposeAmount;

};

} // namespace hise

#endif  // TRANSPOSER_H_INCLUDED
