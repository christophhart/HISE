/*
  ==============================================================================

    MidiDelay.h
    Created: 5 Jul 2014 6:03:15pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef MIDIDELAY_H_INCLUDED
#define MIDIDELAY_H_INCLUDED

/** Delays all midi events by the specified amount. 
*	@ingroup midiTypes
*/
class MidiDelay: public MidiProcessor
{
public:

	SET_PROCESSOR_NAME("MidiDelay", "MidiDelay")

	MidiDelay(MainController *m, const String &id):
		MidiProcessor(m, id),
		delayTime(44100)
	{};

	void processMidiMessage(MidiMessage &m) override
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
					const int id = events[i].eventId;

					if(deleteIfMatched) events.remove(i);

					return id;
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


#endif  // MIDIDELAY_H_INCLUDED
