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

#ifndef HISEEVENTBUFFER_H_INCLUDED
#define HISEEVENTBUFFER_H_INCLUDED

namespace hise { using namespace juce;

#define HISE_EVENT_ID_ARRAY_SIZE 16384

/** This is a replacement of the standard midi message with more data. */
class HiseEvent
{
public:

	enum class Type : uint8
	{
		Empty = 0,
		NoteOn,
		NoteOff,
		Controller,
		PitchBend,
		Aftertouch,
		AllNotesOff,
		SongPosition,
		MidiStart,
		MidiStop,
		VolumeFade,
		PitchFade,
		TimerEvent,
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
		channel(channel_)
	{

	}

	HiseEvent(const HiseEvent &other) noexcept
	{
		// Only works with struct size of 16 bytes...
		jassert(sizeof(HiseEvent) == 16);

		uint64* data = reinterpret_cast<uint64*>(this);

		const uint64* otherData = reinterpret_cast<const uint64*>(&other);

		data[0] = otherData[0];
		data[1] = otherData[1];
	}

	

	bool operator==(const HiseEvent &other) const
	{
		// Only works with struct size of 16 bytes...
		jassert(sizeof(HiseEvent) == 16);

		const uint64* data = reinterpret_cast<const uint64*>(this);

		const uint64* otherData = reinterpret_cast<const uint64*>(&other);

		return data[0] == otherData[0] && data[1] == otherData[1];
	}

	void swapWith(HiseEvent &other)
	{
		// Only works with struct size of 16 bytes...
		jassert(sizeof(HiseEvent) == 16);

		uint64* data = reinterpret_cast<uint64*>(this);

		const uint64 first = data[0];
		const uint64 second = data[1];

		uint64* otherData = reinterpret_cast<uint64*>(&other);

		*data++ = *otherData;
		*data = otherData[1];
		*otherData++ = first;
		*otherData = second;
	}

	Type getType() const noexcept { return type; }

	String getTypeAsString() const noexcept;

	void setType(Type t) noexcept { type = t; }

	/** Checks if the message was marked as ignored (by a script). */
    bool isIgnored() const noexcept{ return ignored; };

	/** Ignores the event. Ignored events will not be processed, but remain in the buffer (they are not cleared). */
    void ignoreEvent(bool shouldBeIgnored) noexcept{ ignored = shouldBeIgnored; };

	uint16 getEventId() const noexcept{ return eventId; };

	void setEventId(uint16 newEventId) noexcept{ eventId = (uint16)newEventId; };

    void setArtificial() noexcept { artificial = true; }
    bool isArtificial() const noexcept{ return artificial; };

	void setTransposeAmount(int newTransposeValue) noexcept{ transposeValue = (int8)newTransposeValue; };
	int getTransposeAmount() const noexcept{ return (int)transposeValue; };

	/** Sets the coarse detune amount in semitones. */
	void setCoarseDetune(int semiToneDetune) noexcept{ semitones = (int8)semiToneDetune; };

	/** Returns the coarse detune amount in semitones. */
	int getCoarseDetune() const noexcept{ return (int)semitones; }

	/** Sets the fine detune amount in cents. */
	void setFineDetune(int newCents) noexcept{ cents = (int8)newCents; };

	/** Returns the fine detune amount int cents. */
	int getFineDetune() const noexcept{ return (int)cents; };

	/** Returns a ready to use pitchfactor (from 0.5 ... 2.0) */
	double getPitchFactorForEvent() const;

	/** Sets the gain in decibels for this note. */
	void setGain(int decibels) noexcept{ gain = (int8)decibels; };

	int getGain() const noexcept{ return gain; };

	float getGainFactor() const noexcept { return Decibels::decibelsToGain((float)gain); };

	static HiseEvent createVolumeFade(uint16 eventId, int fadeTimeMilliseconds, int8 targetValue);

	static HiseEvent createPitchFade(uint16 eventId, int fadeTimeMilliseconds, int8 coarseTune, int8 fineTune);

	static HiseEvent createTimerEvent(uint8 timerIndex, uint16 offset);

	bool isVolumeFade() const noexcept{ return type == Type::VolumeFade; };
	bool isPitchFade() const noexcept { return type == Type::PitchFade; };
	
	int getFadeTime() const noexcept{ return getPitchWheelValue(); };

	bool isTimerEvent() const noexcept { return type == Type::TimerEvent; };
	int getTimerIndex() const noexcept { return channel; }	

	// ========================================================================================================================== MIDI Message methods

	uint16 getTimeStamp() const noexcept{ return timeStamp; };
	void setTimeStamp(uint16 newTimestamp) noexcept{ timeStamp = (uint16)newTimestamp; };
	void addToTimeStamp(int16 delta) noexcept{ timeStamp += delta; };

	int getChannel() const noexcept{ return (int)channel; };
	void setChannel(int newChannelNumber) noexcept{ channel = (uint8)newChannelNumber; };

	bool isNoteOn(bool returnTrueForVelocity0 = false) const noexcept
	{
		ignoreUnused(returnTrueForVelocity0);

		return type == Type::NoteOn; 
	};
	bool isNoteOff() const noexcept { return type == Type::NoteOff; }
	bool isNoteOnOrOff() const noexcept { return type == Type::NoteOn || type == Type::NoteOff; };
	int getNoteNumber() const noexcept{ return (int)number; };
	void setNoteNumber(int newNoteNumber) noexcept
	{ 
		jassert(isNoteOnOrOff());
		number = jmin<uint8>((uint8)newNoteNumber, 127); 
	};
	uint8 getVelocity() const noexcept{ return value; };
	float getFloatVelocity() const noexcept{ return (float)value / 127.0f; }
	void setVelocity(uint8 newVelocity) noexcept{ value = newVelocity; };

	bool isPitchWheel() const noexcept{ return type == Type::PitchBend; };
	int getPitchWheelValue() const noexcept;;
	void setPitchWheelValue(int position) noexcept;;

	void setFadeTime(int fadeTime) noexcept
	{
		setPitchWheelValue(fadeTime);
	}

	void setStartOffset(uint16 startOffset) noexcept;

	uint16 getStartOffset() const noexcept;;

	bool isChannelPressure() const noexcept{ return type == Type::Aftertouch; };
	int getChannelPressureValue() const noexcept{ return value; };
	void setChannelPressureValue(int pressure) noexcept{ value = (uint8)pressure; };

	bool isAftertouch() const noexcept { return type == Type::Aftertouch; };
	int getAfterTouchValue() const noexcept { return (uint8)value; };
	void setAfterTouchValue(int noteNumber, int aftertouchAmount) noexcept{ number = (uint8)noteNumber; value = (uint8)aftertouchAmount; };

	bool isController() const noexcept{ return type == Type::Controller; }
	bool isControllerOfType(int controllerType) const noexcept{ return type == Type::Controller && controllerType == (int)number; };

	int getControllerNumber() const noexcept{ return number; };
	int getControllerValue() const noexcept{ return value; };

	void setControllerNumber(int controllerNumber) noexcept{ number = (uint8)controllerNumber; };
	void setControllerValue(int controllerValue) noexcept{ value = (uint8)controllerValue; };

	bool isEmpty() const noexcept{ return type == Type::Empty; };

	bool isAllNotesOff() const noexcept{ return type == Type::AllNotesOff; };

	bool isMidiStart() const noexcept{ return type == Type::MidiStart; };

	bool isMidiStop() const noexcept{ return type == Type::MidiStop; };

	bool isSongPositionPointer() const noexcept{ return type == Type::SongPosition; };

	int getSongPositionPointerMidiBeat() const noexcept{ return number | (value << 7); };

	void setSongPositionValue(int positionInMidiBeats)
	{
		number = positionInMidiBeats & 127;
		value = (positionInMidiBeats >> 7) & 127;
	}

	/** This clears the events using the fast memset operation. */
	static void clear(HiseEvent* eventToClear, int numEvents = 1)
	{
		memset(eventToClear, 0, sizeof(HiseEvent) * numEvents);
	}

	struct ChannelFilterData
	{
		ChannelFilterData():
			enableAllChannels(true)
		{
			for (int i = 0; i < 16; i++) activeChannels[i] = false;
		}

		void restoreFromData(int data)
		{
			BigInteger d(data);

			enableAllChannels = d[0];
			for (int i = 0; i < 16; i++) activeChannels[i] = d[i + 1];
		}
		
		int exportData() const
		{
			BigInteger d;

			d.setBit(0, enableAllChannels);
			for (int i = 0; i < 16; i++) d.setBit(i + 1, activeChannels[i]);

			return d.toInteger();
		}

		void setEnableAllChannels(bool shouldBeEnabled) noexcept { enableAllChannels = shouldBeEnabled; }
		bool areAllChannelsEnabled() const noexcept { return enableAllChannels; }

		void setEnableMidiChannel(int channelIndex, bool shouldBeEnabled) noexcept
		{
			activeChannels[channelIndex] = shouldBeEnabled;
		}

		bool isChannelEnabled(int channelIndex) const noexcept
		{
			return activeChannels[channelIndex];
		}

		bool activeChannels[16];
		bool enableAllChannels;
	};

private:

	Type type = Type::Empty;

	uint8 channel = 0;
	uint8 number = 0;
	uint8 value = 0;

	int8 transposeValue = 0;

	int8 gain = 0;
	int8 semitones = 0;
	int8 cents = 0;

	uint16 eventId = 0;
	uint16 timeStamp = 0;
	uint16 startOffset = 0;
	
	bool ignored = false;
	bool artificial = false;
	
	
};

#define HISE_EVENT_BUFFER_SIZE 256

class HiseEventBuffer
{
public:

	/** A simple stack type with 16 slots. */
	class EventStack
	{
	public:

		EventStack()
		{
			clear();
		}

		/** Inserts an event. */
		void push(const HiseEvent &newEvent)
		{
			size = jmin<int>(16, size + 1);

			data[size-1] = HiseEvent(newEvent);

		}

		/** Removes and returns an event. */
		HiseEvent pop()
		{
			if (size == 0) return HiseEvent();

			HiseEvent returnEvent = data[size - 1];
			data[size - 1] = HiseEvent();

			size = jmax<int>(0, size-1);

			return returnEvent;
		}

		bool peekNoteOnForEventId(uint16 eventId, HiseEvent& eventToFill)
		{
			for (int i = 0; i < size; i++)
			{
				if (data[i].getEventId() == eventId)
				{
					eventToFill = data[i];
					return true;
				}
			}

			return false;
		}

		bool popNoteOnForEventId(uint16 eventId, HiseEvent& eventToFill)
		{
			int thisIndex = -1;

			for (int i = 0; i < size; i++)
			{
				if (data[i].getEventId() == eventId)
				{
					thisIndex = i;
					break;
				}
			}

			if (thisIndex == -1) return false;
			
			eventToFill = data[thisIndex];

			for (int i = thisIndex; i < size-1; i++)
			{
				data[i] = data[i + 1];
			}

			data[size-1] = HiseEvent();
			size--;

			return true;
		}

		void clear()
		{
			for (int i = 0; i < 16; i++)
				data[i] = HiseEvent();
			size = 0;
		}

		const HiseEvent* peek() const
		{
			if (size == 0) return nullptr;

			return &data[size - 1];
		}

		HiseEvent* peek()
		{ 
			if (size == 0) return nullptr;

			return &data[size - 1];
		}

		int getNumUsed() { return size; };

	private:

		HiseEvent data[16];
		int size = 0;
	};

	HiseEventBuffer();

	bool operator==(const HiseEventBuffer& other)
	{
		if (other.getNumUsed() != numUsed) return false;

		const HiseEventBuffer::Iterator iter(other);

		for (int i = 0; i < numUsed; i++)
		{
			const HiseEvent* e = iter.getNextConstEventPointer();

			if (e == nullptr)
			{
				jassertfalse;
				return false;
			}

			if (!(*e == buffer[i])) 
				return false;
			
		}

		return true;
	}

	void clear();
	bool isEmpty() const noexcept{ return numUsed == 0; };
	int getNumUsed() const { return numUsed; }

	HiseEvent getEvent(int index) const;

	void subtractFromTimeStamps(int delta);
	void moveEventsBelow(HiseEventBuffer& targetBuffer, int highestTimestamp);
	void moveEventsAbove(HiseEventBuffer& targetBuffer, int lowestTimestamp);

	void copyFrom(const HiseEventBuffer& otherBuffer);

	void addEvent(const HiseEvent& hiseEvent);
	void addEvent(const MidiMessage& midiMessage, int sampleNumber);
	void addEvents(const MidiBuffer& otherBuffer);

	void addEvents(const HiseEventBuffer &otherBuffer);

	
	struct CopyHelpers
	{
		static void copyEvents(HiseEvent* destination, const HiseEvent* source, int numElements)
		{
			memcpy(destination, source, sizeof(HiseEvent) * numElements);
		}

		static void copyEvents(HiseEventBuffer &destination, int offsetInDestination, const HiseEventBuffer& source, int offsetInSource, int numElements)
		{
			memcpy(destination.buffer + offsetInDestination, source.buffer + offsetInSource, sizeof(HiseEvent) * numElements);
		}
	};

	class Iterator
	{
	public:

		/** Creates an iterator which allows access to the HiseEvents in the buffer. */
		Iterator(const HiseEventBuffer& b);

		/** Saves the next event into the given HiseEvent address. 
		@param e - the event adress. Remember this will copy the event. If you want to alter the event in the buffer, 
		           use the other iterator methods which return a pointer to the element in the buffer. 
		@param samplePosition - the timestamp position. This will be sorted and compatible to the MidiBuffer::Iterator method.
		@param skipIgnoredEvents - skips HiseEvents which are ignored.
		@param skipArtificialNotes - skips artificial notes. Use this to avoid loops when processing HiseEventBuffers.
		*/
		bool getNextEvent(HiseEvent& e, int &samplePosition, bool skipIgnoredEvents=false, bool skipArtificialEvents=false) const;

		/** Returns a read pointer to the event in the buffer. */
		const HiseEvent* getNextConstEventPointer(bool skipIgnoredEvents=false, bool skipArtificialNotes = false) const;

		/** Returns a write pointer to the event in the buffer. */
		HiseEvent* getNextEventPointer(bool skipIgnoredEvents=false, bool skipArtificialNotes = false);

	private:

		HiseEventBuffer *buffer;

		mutable int index;
	};

private:

	friend class Iterator;

	void insertEventAtPosition(const HiseEvent& e, int positionInBuffer);

	HiseEvent buffer[HISE_EVENT_BUFFER_SIZE];

	int numUsed = 0;

	
};



} // namespace hise

#endif  // HISEEVENTBUFFER_H_INCLUDED
