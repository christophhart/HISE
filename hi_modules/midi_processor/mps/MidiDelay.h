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

#ifndef MIDIDELAY_H_INCLUDED
#define MIDIDELAY_H_INCLUDED


namespace hise { using namespace juce;

/** Delays all midi events by the specified amount. 
*	@ingroup midiTypes
*/
class MidiDelay: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("MidiDelay", "MidiDelay", "Delays all midi events by the specified amount.")

	MidiDelay(MainController *m, const String &id):
		MidiProcessor(m, id),
		delayTime(44100)
	{};

	void processHiseEvent(HiseEvent &m) override
	{
		m.setTimeStamp(delayTime);
	};

	

	float getAttribute(int) const override { return (float)delayTime; };

	void setInternalAttribute(int, float newValue) override { delayTime = (int)newValue;	};

private:

	/** Returns the event id of the note message or -1 if none found. */
	int findEventIdToMatch(const MidiMessage &noteOff, bool deleteIfMatched)
	{
		if(noteOff.isNoteOff())
			{
			for(int i = 0; i < events.size(); i++)
			{
				if(events[i].m.getNoteNumber() == noteOff.getNoteNumber())
				{
					const int thisId = events[i].eventId;

					if(deleteIfMatched) events.remove(i);

					return thisId;
				}

			};
		}

		return -1;

	}

	struct MidiEvent
	{
		MidiMessage m;
		int eventId;

		

	};

	Array<MidiEvent> events;

	int delayTime;
};

} // namespace hise

#endif  // MIDIDELAY_H_INCLUDED
