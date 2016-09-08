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

#ifndef HISEEVENTBUFFER_H_INCLUDED
#define HISEEVENTBUFFER_H_INCLUDED

/** This is a replacement of the standard midi message with more data. */
class HiseEvent
{
public:

	enum class Type : uint8
	{
		NoteOn = 0,
		NoteOff,
		Controller,
		PitchBend,
		Aftertouch,
		AllNotesOff,
		numTypes
	};

	/** Creates an empty Hise event. */
	HiseEvent() {};

	/** Creates a Hise event from a MIDI message. */
	HiseEvent(const MidiMessage& message);

	HiseEvent(Type type_, uint8 number_, uint8 value_, uint8 channel_ = 1):
		type(type_),
		number(number_),
		value(value_),
		channel(channel_),
		cleared(false)
	{

	}

	/** This marks the event as unused. It won't be deleted, but overriden when a new message is written. */
	void clear() noexcept{ cleared = false; };

	/** Checks if the message was cleared. */
	bool isClear() const noexcept{ return cleared; };

	/** Checks if the message was marked as ignored (by a script). */
	bool isIgnored() const noexcept{ return ignored; };

	/** Ignores the event. Ignored events will not be processed, but remain in the buffer (they are not cleared). */
	void ignoreEvent(bool shouldBeIgnored) noexcept{ ignored = shouldBeIgnored; };

	uint32 getEventId() const noexcept{ return eventId; };

	void setEventId(uint32 newEventId) noexcept{ eventId = newEventId; };

	void setArtificial() noexcept { artificial = true; }
	bool isArtificial() const noexcept{ return artificial; }

	// ========================================================================================================================== MIDI Message methods

	uint32 getTimeStamp() const noexcept{ return timeStamp; };
	void setTimeStamp(uint32 newTimestamp) noexcept{ timeStamp = newTimestamp; };
	void addToTimeStamp(int32 delta) noexcept{ timeStamp += delta; };

	int getChannel() const noexcept{ return (int)channel; };
	void setChannel(int newChannelNumber) noexcept{ channel = (uint8)newChannelNumber; };

	bool isNoteOn(bool returnTrueForVelocity0 = false) const noexcept { return type == Type::NoteOn; };
	bool isNoteOff() const noexcept { return type == Type::NoteOff; }
	bool isNoteOnOrOff() const noexcept { return type == Type::NoteOn || type == Type::NoteOff; };
	int getNoteNumber() const noexcept{ return (int)number; };
	void setNoteNumber(int newNoteNumber) noexcept { number = jmin<uint8>((uint8)newNoteNumber, 127); };
	uint8 getVelocity() const noexcept{ return value; };
	float getFloatVelocity() const noexcept{ return (float)value / 127.0f; }
	void setVelocity(uint8 newVelocity) noexcept{ value = newVelocity; };

	bool isPitchWheel() const noexcept{ return type == Type::PitchBend; };
	int getPitchWheelValue() const noexcept{ return number | (value << 7); };
	void setPitchWheelValue(int position) noexcept
	{ 
		number = position & 127;
		value =  (position >> 7) & 127;
	}

	bool isChannelPressure() const noexcept{ return type == Type::Aftertouch; };
	int getChannelPressureValue() const noexcept{ return value; };
	void setChannelPressureValue(int pressure) noexcept{ value = (uint8)pressure; };

	bool isAftertouch() const noexcept { return type == Type::Aftertouch; };
	int getAfterTouchValue() const noexcept { return (uint8)value; };
	void setAfterTouchValue(int noteNumber, int aftertouchAmount) noexcept{ number = noteNumber; value = aftertouchAmount; };

	bool isController() const noexcept{ return type == Type::Controller; }
	bool isControllerOfType(int controllerType) const noexcept{ return type == Type::Controller && controllerType == (int)number; };

	int getControllerNumber() const noexcept{ return number; }
	int getControllerValue() const noexcept{ return value; }

	void setControllerNumber(int controllerNumber) noexcept{ number = controllerNumber; }
	void setControllerValue(int controllerValue) noexcept{ value = controllerValue; }

	bool isAllNotesOff() const noexcept{ return type == Type::AllNotesOff; };


private:

	uint8 channel = 1;
	uint8 number = 0;
	uint8 value = 0;
	Type type = Type::NoteOn;

	bool ignored = false;
	bool cleared = true;
	bool artificial = false;
	
	uint32 eventId = 0;

	uint32 timeStamp = 0;

	float gain = 1.0f;
	
	int8 semitones;
	int8 cents;
};

#define HISE_EVENT_BUFFER_SIZE 256

class HiseEventBuffer
{
public:

	HiseEventBuffer();

	void clear();
	bool isEmpty() const noexcept{ return numUsed == 0; };

	void subtractFromTimeStamps(int delta);
	void moveEvents(HiseEventBuffer& otherBuffer, int numSamples);
	void moveEventsBeyond(HiseEventBuffer& otherBuffer, int numSamples);

	void copyFrom(const HiseEventBuffer& otherBuffer);

	void addEvent(const HiseEvent& hiseEvent);
	void addEvent(const MidiMessage& midiMessage, int sampleNumber);
	void addEvents(const MidiBuffer& otherBuffer);

	class EventIdHandler
	{
	public:

		EventIdHandler(HiseEventBuffer& masterBuffer_) :
			masterBuffer(masterBuffer_),
			currentEventId(1)
		{
			for (int i = 0; i < 128; i++) eventIds[i] = 0;
		}


		/** Fills note on / note off messages with the event id and returns the current value for external storage. */
		void handleEventIds();

		int requestEventIdForArtificialNote() noexcept;

	private:

		HiseEventBuffer &masterBuffer;

		uint32 eventIds[128];

		uint32 currentEventId;
	};

	

	class Iterator
	{
	public:

		Iterator(HiseEventBuffer &b);

		bool getNextEvent(HiseEvent& b, int &samplePosition);
		HiseEvent* getNextEventPointer(bool skipArtificialNotes=false);

	private:

		HiseEventBuffer &buffer;
		int index;
	};

private:

	friend class Iterator;

	void insertEventAtPosition(const HiseEvent& e, int positionInBuffer);

	HiseEvent buffer[HISE_EVENT_BUFFER_SIZE];

	int numUsed = 0;

	
};



#endif  // HISEEVENTBUFFER_H_INCLUDED
