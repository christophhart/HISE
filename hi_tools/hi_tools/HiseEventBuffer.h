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

// Apparently, windows doesn't like the alignment specification
#if JUCE_WINDOWS
#define event_alignment
#else
#define event_alignment alignas(16)
#endif

/** The event type of HISE.
	@ingroup core
	
	This is an enhancement of the MIDI Standard and is used for all internal
	events in the audio path of HISE.
	
	The MIDI standard (and its implementation of JUCE) have a few limitations
	and misses some convenient data. Therefore, a new event type was introduced,
	with the following additions:
	
	- fixed size. The MIDI message has to have a dynamic size for handling
	  SysEx messages, which is not used in 99,9999% of all cases. The HiseEvent
	  simply ignores SysEx (there are better ways to communicate bigger chunks
	  of data anyways) and uses a fixed size of 128bit per message. This makes
	  copying / clearing the HiseEventBuffer trivially fast (just a memset / memcpy)
	- note-on / note-off messages will be associated with a unique index (the
	  EventID), which can be used to alter all voices that are started by
	  the event.
	- more types for internal actions like timer events, pitch / volume fades
	- a timestamp that is baked into the message.
	- 128 channels instead of 16 MIDI channels. 16 is a pretty low number and
	  if you use the channel information to route the signals to certain 
	  Processors, you might hit this limitation pretty easily.
	- a transpose amount that will be added on top of the actual note number.
	  This heavily simplifies any MIDI processing that changes the note number
	  because the note off event does not need to be transposed to match the
	  note on in order to prevent stuck notes
	- a few flags that describe the state and origin of the note (whether the
	  message should be processed at all or if it was created internally).

	Most of its methods aim to be fully compatible to the juce::MidiMessage class,
	so if you're used to this class, you will find your way around this class 
	pretty quickly.


	*/
class event_alignment HiseEvent
{
public:

	static constexpr int PitchWheelCCNumber = 128;
	static constexpr int AfterTouchCCNumber = 129;

	/** The type of the event. The most important MIDI types are there, but there are a few
	    more interesting types for internal HISE stuff. */
	enum class Type : uint8
	{
		Empty = 0, ///< an empty event (as created by the default constructor)
		NoteOn,	///< a note on event (which will get a unique EventID upon creation).
		NoteOff, ///< a note-off event (with the same EventID as its corresponding note-on)
		Controller, ///< a MIDI CC message
		PitchBend, ///< a 14-bit pitch-bend message
		Aftertouch, ///< an aftertouch message (both channel aftertouch and polyphonic aftertouch)
		AllNotesOff, ///< an all notes off message.
		SongPosition, ///< the position of the DAW transport
		MidiStart, ///< indicated the start of the playback in the DAW
		MidiStop, ///< indicates the stop of the DAW playback
		VolumeFade, ///< a volume fade that is applied to all voices started with the given EventID
		PitchFade, ///< a pitch fade that is applied to all voices started with the given EventID
		TimerEvent, ///< this event will fire the onTimer callback of MIDI Processors.
		ProgramChange, ///< the MIDI ProgramChange message.
		numTypes
	};

	/** Creates an empty HiseEvent. */
	HiseEvent() {};

	/** Creates a HiseEvent from a MIDI message. */
	HiseEvent(const MidiMessage& message);

	/** Creates a HiseEvent with the given data. */
	HiseEvent(Type type_, uint8 number_, uint8 value_, uint8 channel_ = 1);

	/** Creates a bit-wise copy of another event. */
	HiseEvent(const HiseEvent &other) noexcept;

	/** Converts the HiseEvent back to a MidiMessage. This isn't lossless obviously. */
	MidiMessage toMidiMesage() const;

	/** Allows using the empty check in a scoped if-condition. */
	explicit operator bool() const noexcept { return !isEmpty(); }

	/** checks whether the event is equal to another. This checks for
		bit-equality. */
	bool operator==(const HiseEvent &other) const;

	/** Clears the event (so that isEmpty() returns true. */
	void clear();

	/** Swaps the event with another. */
	void swapWith(HiseEvent &other);

	/** Returns the Type of the HiseEvent. */
	Type getType() const noexcept { return type; }

	/** Returns a String representation of the type. */
	String getTypeAsString() const noexcept;

	/** Returns a String identifier for the given type. */
	static String getTypeString(Type t);

	/** Changes the type. Don't use this unless you know why. */
	void setType(Type t) noexcept { type = t; }

	/** Checks if the message was marked as ignored (by a script). */
    bool isIgnored() const noexcept;;

	/** Ignores the event. Ignored events will not be processed, but remain in the buffer (they are not cleared). */
    void ignoreEvent(bool shouldBeIgnored) noexcept;;

	/** Returns the event ID of the message. The event IDs will be automatically created
	    by HISE when it is processing the incoming MIDI messages and associates sequentially
		increasing IDS for each note-on and its corresponding note-off event. 
		
		Be aware the the event ID is stored as unsigned 16 bit integer, so it will wrap around
		65536. It's highly unlikely that you will hit any collisions, but you can't expect that 
		older notes have a higher event ID.
	*/
	uint16 getEventId() const noexcept{ return eventId; };

	/** Sets the event ID of the HiseEvent. Normally you don't need to do this because HISE
	    will automatically assign this to note-on / note-off messages, but for all the types
		that alter an existing event (like volume-fades), this can be used for setting the
		target event.
	*/
	void setEventId(uint16 newEventId) noexcept{ eventId = (uint16)newEventId; };

	/** If the event was created artificially by a MIDI Processor, it will call this method.
		You don't need to use this yourself. */
    void setArtificial() noexcept;

	/** Returns true if this method was created artificially. 
	
		Events that come in as MIDI message (no matter if their origin is in an actual key
		press or if there was a previous MIDI processor (like an arpeggiator) that created
		it, will be flagged as "non-artificial". Events that are created within HISE are 
		flagged as "artificial".
		
		This information can be useful sometimes in order to prevent endless recursive loops.
		Also, the HiseEventBuffer::Iterator class can be told to skip artificial events. 
	*/
    bool isArtificial() const noexcept;;

	/** Sets the transpose amount of the given event ID.
	
		Unlike changing the note-number directly, this method will keep the original note
		number so that you don't have to process the note-off number to match the note-on.

		This is the recommended way of handling all note-number processing in HISE.
	*/
	void setTransposeAmount(int newTransposeValue) noexcept{ transposeValue = (int8)newTransposeValue; };

	/** Returns the transpose amount. Be aware that you need to take this into account when
		you need the actual note-number of an HiseEvent.
	*/
	int getTransposeAmount() const noexcept{ return (int)transposeValue; };

	/** Sets the coarse detune amount in semitones. */
	void setCoarseDetune(int semiToneDetune) noexcept{ semitones = (int8)semiToneDetune; };

	/** Returns the coarse detune amount in semitones. */
	int getCoarseDetune() const noexcept{ return (int)semitones; }

	/** Sets the fine detune amount in cents. */
	void setFineDetune(int newCents) noexcept{ cents = (int8)newCents; };

	/** Returns the fine detune amount int cents. */
	int getFineDetune() const noexcept{ return (int)cents; };

	/** Returns a ready to use pitch factor (from 0.5 ... 2.0) */
	double getPitchFactorForEvent() const;

	/** Returns the frequency in hertz. Uses all event properties. */
	double getFrequency() const;

	/** Sets the gain in decibels for this note. */
	void setGain(int decibels) noexcept;;

	/** returns the gain in decibels. */
	int getGain() const noexcept{ return gain; };

	/** Returns the gain factor (from 0...1) for the given event. */
	float getGainFactor() const noexcept { return Decibels::decibelsToGain((float)gain); };

	/** Creates a volume fade.
		@param eventId the event ID that this fade should be applied to.
		@param fadeTimeMilliseconds the fade time (it will be a linear fade).
		@targetValue the target gain in decibels.
	*/
	static HiseEvent createVolumeFade(uint16 eventId, int fadeTimeMilliseconds, int8 targetValue);

	/** Creates a pitch fade.
		@param eventID the ID of the event that will be changed.
		@param fadeTimeMilliseconds the length of the fade.
		@param coarseTune the target pitch in semitones
		@param fineTune the target pitch detune in cent.
	*/
	static HiseEvent createPitchFade(uint16 eventId, int fadeTimeMilliseconds, int8 coarseTune, int8 fineTune);

	/** Creates a timer event.
		@param timerIndex There are 4 timer slots per sound generator and this will
		                  contain the index (0-3).
		@param offset the sample offset within the current buffer [0 - buffer size).
	*/
	static HiseEvent createTimerEvent(uint8 timerIndex, int offset);

	/** This is a less strict comparison that does not take the event ID and the timestamp into account. */
	bool matchesMidiData(const HiseEvent& other) const;

	/** Returns true if the event is a volume fade. */
	bool isVolumeFade() const noexcept{ return type == Type::VolumeFade; };

	/** Returns true if the event is a pitch fade. */
	bool isPitchFade() const noexcept { return type == Type::PitchFade; };
	
	/** Returns the fade time for both pitch and volume fades. */
	int getFadeTime() const noexcept{ return getPitchWheelValue(); };

	/** Returns true if the event is a timer event. */
	bool isTimerEvent() const noexcept { return type == Type::TimerEvent; };

	/** Returns the index of the timer slot. */
	int getTimerIndex() const noexcept { return channel; }	

	// ========================================================================================================================== MIDI Message methods

	/** Returns the timestamp of the message. The timestamp is the offset from 
	    the current buffer start. If the timestamp is bigger than the current
		buffer size, the message will be delayed until the buffer range contains
		the time stamp.
	*/
	int getTimeStamp() const noexcept;;

	/** Sets the timestamp to a sample offset in the future. */
	void setTimeStamp(int newTimestamp) noexcept;

	template <int Alignment> void alignToRaster(int maxTimestamp) noexcept
	{
		int thisTimeStamp = getTimeStamp();

		const int odd = thisTimeStamp % (int)Alignment;
		constexpr int half = static_cast<int>(Alignment) / 2;

		int roundUpValue = (int)(static_cast<int>(odd > half) * static_cast<int>(Alignment));
		int delta = roundUpValue - odd;

		thisTimeStamp += delta;

		int limitRoundDownValue = static_cast<int>(thisTimeStamp >= maxTimestamp) * static_cast<int>(Alignment);

		thisTimeStamp -= limitRoundDownValue;

		setTimeStamp(thisTimeStamp);
	}

	/** Adds the delta value to the timestamp. */
	void addToTimeStamp(int delta) noexcept;

	/** Returns the MIDI channel. */
	int getChannel() const noexcept{ return (int)channel; };

	/** Sets the MIDI channel. Note that in HISE you have 256 MIDI channels. */
	void setChannel(int newChannelNumber) noexcept{ channel = (uint8)newChannelNumber; };

	/** Copied from MidiMessage. */
	bool isNoteOn(bool returnTrueForVelocity0 = false) const noexcept;;

	/** Copied from MidiMessage. */
	bool isNoteOff() const noexcept { return type == Type::NoteOff; }

	/** Copied from MidiMessage. */
	bool isNoteOnOrOff() const noexcept { return type == Type::NoteOn || type == Type::NoteOff; };

	/** Copied from MidiMessage. */
	int getNoteNumber() const noexcept{ return (int)number; };

	/** Returns the "actual" note number that might result from a transpose value. */
	int getNoteNumberIncludingTransposeAmount() const noexcept { return (int)number + getTransposeAmount(); }

	/** Copied from MidiMessage. */
	void setNoteNumber(int newNoteNumber) noexcept;;

	/** Copied from MidiMessage. */
	uint8 getVelocity() const noexcept{ return value; };

	/** Copied from MidiMessage. */
	float getFloatVelocity() const noexcept{ return (float)value / 127.0f; }

	/** Copied from MidiMessage. */
	void setVelocity(uint8 newVelocity) noexcept{ value = newVelocity; };

	/** Copied from MidiMessage. */
	bool isPitchWheel() const noexcept{ return type == Type::PitchBend; };

	/** Copied from MidiMessage. */
	int getPitchWheelValue() const noexcept;;

	/** Copied from MidiMessage. */
	void setPitchWheelValue(int position) noexcept;;

	/** Sets the fade time for the event type. Only valid for VolumeFade and PitchFade types. */
	void setFadeTime(int fadeTime) noexcept
	{
		setPitchWheelValue(fadeTime);
	}

	/** Adds a offset to the event. Unlike the timestamp, this will not delay the
		event to the future, but tell the sound generator to skip the given amount
		when the voice starts. This can be used for eg. skipping the attack phase of samples. */
	void setStartOffset(uint16 startOffset) noexcept;

	/** Returns the start offset of the event. */
	uint16 getStartOffset() const noexcept;;

	/** Copied from MidiMessage. */
	bool isChannelPressure() const noexcept{ return type == Type::Aftertouch; };

	/** Copied from MidiMessage. */
	int getChannelPressureValue() const noexcept{ return value; };

	/** Copied from MidiMessage. */
	void setChannelPressureValue(int pressure) noexcept{ value = (uint8)pressure; };

	/** Copied from MidiMessage. */
	bool isAftertouch() const noexcept { return type == Type::Aftertouch; };

	/** Copied from MidiMessage. */
	int getAfterTouchValue() const noexcept { return (uint8)value; };

	/** Copied from MidiMessage. */
	void setAfterTouchValue(int noteNumber, int aftertouchAmount) noexcept{ number = (uint8)noteNumber; value = (uint8)aftertouchAmount; };

	/** Copied from MidiMessage. */
	bool isController() const noexcept{ return type == Type::Controller; }

	/** Copied from MidiMessage. */
	bool isControllerOfType(int controllerType) const noexcept{ return type == Type::Controller && controllerType == (int)number; };

	/** Copied from MidiMessage. */
	int getControllerNumber() const noexcept;;

	/** Copied from MidiMessage. */
	int getControllerValue() const noexcept{ return value; };

	/** Copied from MidiMessage. */
	void setControllerNumber(int controllerNumber) noexcept{ number = (uint8)controllerNumber; };

	/** Copied from MidiMessage. */
	void setControllerValue(int controllerValue) noexcept{ value = (uint8)controllerValue; };

	/** Copied from MidiMessage. */
	bool isProgramChange() const noexcept { return type == Type::ProgramChange; };

	/** Copied from MidiMessage. */
	int getProgramChangeNumber() const noexcept { return number; };

	/** Returns true if the HiseEvent is empty. */
	bool isEmpty() const noexcept{ return type == Type::Empty; };

	/** Copied from MidiMessage. */
	bool isAllNotesOff() const noexcept{ return type == Type::AllNotesOff; };

	/** Copied from MidiMessage. */
	bool isMidiStart() const noexcept{ return type == Type::MidiStart; };
	
	/** Copied from MidiMessage. */
	bool isMidiStop() const noexcept{ return type == Type::MidiStop; };

	/** Copied from MidiMessage. */
	bool isSongPositionPointer() const noexcept{ return type == Type::SongPosition; };

	/** Copied from MidiMessage. */
	int getSongPositionPointerMidiBeat() const noexcept{ return number | (value << 7); };

	/** Copied from MidiMessage. */
	void setSongPositionValue(int positionInMidiBeats);

	/** This clears the events using the fast memset operation. */
	static void clear(HiseEvent* eventToClear, int numEvents = 1)
	{
		memset(eventToClear, 0, sizeof(HiseEvent) * numEvents);
	}

	bool operator< (const HiseEvent& right) const 
	{ 
		return getTimeStamp() < right.getTimeStamp();
	}

	/** Returns a string for debugging purposes. */
	String toDebugString() const;

	struct ChannelFilterData
	{
		ChannelFilterData();

		void restoreFromData(int data);

		int exportData() const;

		void setEnableAllChannels(bool shouldBeEnabled) noexcept;
		bool areAllChannelsEnabled() const noexcept;

		void setEnableMidiChannel(int channelIndex, bool shouldBeEnabled) noexcept;

		bool isChannelEnabled(int channelIndex) const noexcept;

		bool activeChannels[16];
		bool enableAllChannels;
	};

private:

	Type type = Type::Empty;		// DWord 1
	uint8 channel = 0;
	uint8 number = 0;
	uint8 value = 0;

	int8 transposeValue = 0;		// DWord 2
	int8 gain = 0;
	int8 semitones = 0;
	int8 cents = 0;

	uint16 eventId = 0;				// DWord 3
	uint16 startOffset = 0;

    uint32 timestamp = 0;
};

#define HISE_EVENT_BUFFER_SIZE 256

/** The buffer type for the HiseEvent.

*/
class HiseEventBuffer
{
public:

	/** A simple stack type with 16 slots. */
	class EventStack
	{
	public:

		EventStack();

		/** Inserts an event. */
		void push(const HiseEvent &newEvent);

		/** Removes and returns an event. */
		HiseEvent pop();

		bool peekNoteOnForEventId(uint16 eventId, HiseEvent& eventToFill);

		bool popNoteOnForEventId(uint16 eventId, HiseEvent& eventToFill);

		void clear();

		const HiseEvent* peek() const;

		HiseEvent* peek();

		int getNumUsed();;

	private:

		HiseEvent data[16];
		int size = 0;
	};

	HiseEventBuffer();

	bool operator==(const HiseEventBuffer& other);

	/** Clears the buffer. */
	void clear();

	/** checks if the buffer is empty. */
	bool isEmpty() const noexcept{ return numUsed == 0; };

	/** Returns the number of events in this buffer. */
	int getNumUsed() const { return numUsed; }

	int size() const { return getNumUsed(); }

	HiseEvent getEvent(int index) const;

	HiseEvent popEvent(int index);

	void subtractFromTimeStamps(int delta);
	void moveEventsBelow(HiseEventBuffer& targetBuffer, int highestTimestamp);
	void moveEventsAbove(HiseEventBuffer& targetBuffer, int lowestTimestamp);

	void copyFrom(const HiseEventBuffer& otherBuffer);

	void addEvent(const HiseEvent& hiseEvent);

	void addEvent(const MidiMessage& midiMessage, int sampleNumber);
	void addEvents(const MidiBuffer& otherBuffer);

	void addEvents(const HiseEventBuffer &otherBuffer);
	
	void sortTimestamps();
	
	void multiplyTimestamps(int factor);


	template <int Alignment> void alignEventsToRaster(int maxTimeStamp)
	{
		for (auto& e : *this)
			e.alignToRaster<Alignment>(maxTimeStamp);
	}

	bool timeStampsAreSorted() const;
	
	int getMinTimeStamp() const;

	int getMaxTimeStamp() const;

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

	/** A iterator type for the HiseEventBuffer. */
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

	/** compatibility for standard C++ type iterators. */
	inline HiseEvent* begin() const noexcept
	{
		return const_cast<HiseEvent*>(buffer);
	}

	/** compatibility for standard C++ type iterators. */
	inline HiseEvent* end() const noexcept
	{
		return const_cast<HiseEvent*>(buffer + numUsed);
	}

private:

	friend class Iterator;

	void insertEventAtPosition(const HiseEvent& e, int positionInBuffer);

	event_alignment HiseEvent buffer[HISE_EVENT_BUFFER_SIZE];

	int numUsed = 0;
};

#undef event_alignment



/** This class will iterate over incoming MIDI messages, and transform them
*	into HiseEvents with a succesive index for note-on / note-off messages.
*
*	Normally, you won't use this class, but rather benefit from it in the MIDI
*	processing world using Message.getEventId(), but there are a few methods
*	that can access these things directly.
*/
class EventIdHandler
{
public:

	struct ChokeListener
	{
		virtual ~ChokeListener();;

		virtual void chokeMessageSent() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ChokeListener);

		int getChokeGroup() const;;

	protected:

		void setChokeGroup(int newChokeGroup);

	private:

		int chokeGroup = 0;
	};

	// ===========================================================================================================

	EventIdHandler(HiseEventBuffer& masterBuffer_);
	~EventIdHandler();

	// ===========================================================================================================

	/** Fills note on / note off messages with the event id and returns the current value for external storage. */
	void handleEventIds();

	/** Removes the matching noteOn event for the given noteOff event. */
	uint16 getEventIdForNoteOff(const HiseEvent &noteOffEvent);

	/** Adds the artificial event to the internal stack array. */
	void pushArtificialNoteOn(HiseEvent& noteOnEvent) noexcept;

	/** Searches all active note on events and returns the one with the given event id. */
	HiseEvent popNoteOnFromEventId(uint16 eventId);

	/** Checks whether the event ID points to an active artificial event. */
	bool isArtificialEventId(uint16 eventId) const;

	/** Sends a choke message to all registered listeners for the given event. */
	void sendChokeMessage(ChokeListener* source, const HiseEvent& e);

	void addChokeListener(ChokeListener* l);

	void removeChokeListener(ChokeListener* l);

	// ===========================================================================================================

private:

	Array<WeakReference<ChokeListener>> chokeListeners;

	const HiseEventBuffer &masterBuffer;
	HeapBlock<HiseEvent> artificialEvents;
	uint16 lastArtificialEventIds[16][128];
	HiseEvent realNoteOnEvents[16][128];
	uint16 currentEventId;

	UnorderedStack<HiseEvent, 256> overlappingNoteOns;

	// ===========================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EventIdHandler)
};


} // namespace hise

#endif  // HISEEVENTBUFFER_H_INCLUDED
