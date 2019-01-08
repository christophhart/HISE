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

namespace hise { using namespace juce;

HiseEvent::HiseEvent(const MidiMessage& message)
{
	const uint8* data = message.getRawData();

	channel = (uint8)message.getChannel();

	if (message.isNoteOn()) type = Type::NoteOn;
	else if (message.isNoteOff()) type = Type::NoteOff;
	else if (message.isPitchWheel()) type = Type::PitchBend;
	else if (message.isController()) type = Type::Controller;
	else if (message.isChannelPressure() || message.isAftertouch()) type = Type::Aftertouch;
	else if (message.isAllNotesOff() || message.isAllSoundOff()) type = Type::AllNotesOff;
	else if (message.isProgramChange()) type = Type::ProgramChange;
	else
	{
		type = Type::Empty;
		number = 0;
		value = 0;
		channel = 0;
		return;
	}
	
	number = data[1];
	value = data[2];
}

HiseEvent::HiseEvent(Type type_, uint8 number_, uint8 value_, uint8 channel_ /*= 1*/) :
	type(type_),
	number(number_),
	value(value_),
	channel(channel_)
{

}

HiseEvent::HiseEvent(const HiseEvent &other) noexcept
{
	// Only works with struct size of 16 bytes...
	jassert(sizeof(HiseEvent) == 16);

	uint64* data = reinterpret_cast<uint64*>(this);

	const uint64* otherData = reinterpret_cast<const uint64*>(&other);

	data[0] = otherData[0];
	data[1] = otherData[1];
}

void HiseEvent::swapWith(HiseEvent &other)
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

bool HiseEvent::operator==(const HiseEvent &other) const
{
	// Only works with struct size of 16 bytes...
	jassert(sizeof(HiseEvent) == 16);

	const uint64* data = reinterpret_cast<const uint64*>(this);

	const uint64* otherData = reinterpret_cast<const uint64*>(&other);

	return data[0] == otherData[0] && data[1] == otherData[1];
}

String HiseEvent::getTypeAsString() const noexcept
{
	switch (type)
	{
	case HiseEvent::Type::Empty: return "Empty";
	case HiseEvent::Type::NoteOn: return "NoteOn";
	case HiseEvent::Type::NoteOff: return "NoteOff";
	case HiseEvent::Type::Controller: return "Controller";
	case HiseEvent::Type::PitchBend: return "PitchBend";
	case HiseEvent::Type::Aftertouch: return "Aftertouch";
	case HiseEvent::Type::AllNotesOff: return "AllNotesOff";
	case HiseEvent::Type::SongPosition: return "SongPosition";
	case HiseEvent::Type::MidiStart: return "MidiStart";
	case HiseEvent::Type::MidiStop: return "MidiStop";
	case HiseEvent::Type::VolumeFade: return "VolumeFade";
	case HiseEvent::Type::PitchFade: return "PitchFade";
	case HiseEvent::Type::TimerEvent: return "TimerEvent";
	case HiseEvent::Type::ProgramChange: return "ProgramChange";
	case HiseEvent::Type::numTypes: jassertfalse;
	default: jassertfalse;
	}

	return "Undefined";
}



double HiseEvent::getPitchFactorForEvent() const
{
	if (semitones == 0 && cents == 0) return 1.0;

	const double detuneFactor = (double)semitones + (double)cents / 100.0;

	return std::exp2(detuneFactor / 12.0);
}

double HiseEvent::getFrequency() const
{
	int n = getNoteNumber();
	n += getTransposeAmount();
	auto freq = MidiMessage::getMidiNoteInHertz(n);
	return freq * getPitchFactorForEvent();
}

HiseEvent HiseEvent::createVolumeFade(uint16 eventId, int fadeTimeMilliseconds, int8 targetValue)
{
	HiseEvent e(Type::VolumeFade, 0, 0, 1);

	e.setEventId(eventId);
	e.setGain(targetValue);
	e.setPitchWheelValue(fadeTimeMilliseconds);
	e.setArtificial();

	return e;
}

HiseEvent HiseEvent::createPitchFade(uint16 eventId, int fadeTimeMilliseconds, int8 coarseTune, int8 fineTune)
{
	HiseEvent e(Type::PitchFade, 0, 0, 1);

	e.setEventId(eventId);
	e.setCoarseDetune((int)coarseTune);
	e.setFineDetune(fineTune);
	e.setPitchWheelValue(fadeTimeMilliseconds);
	e.setArtificial();

	return e;
}

HiseEvent HiseEvent::createTimerEvent(uint8 timerIndex, uint16 offset)
{
	HiseEvent e(Type::TimerEvent, 0, 0, timerIndex);

	e.setArtificial();
	e.setTimeStamp(offset);

	return e;
}

void HiseEvent::setTimeStamp(int newTimestamp) noexcept
{
	timeStamp = static_cast<uint16>(jlimit<int>(0, UINT16_MAX, newTimestamp));
}

void HiseEvent::setTimeStampRaw(uint16 newTimestamp) noexcept
{
	timeStamp = newTimestamp;
}

void HiseEvent::addToTimeStamp(int16 delta) noexcept
{
	if (delta < 0)
	{
		int v = (int)timeStamp + delta;
		timeStamp = (uint16)jmax<int>(0, v);
	}
	else
	{
		int v = (int)timeStamp + delta;
		timeStamp = (uint16)jmin<int>(UINT16_MAX, v);
	}
}

bool HiseEvent::isNoteOn(bool returnTrueForVelocity0 /*= false*/) const noexcept
{
	ignoreUnused(returnTrueForVelocity0);

	return type == Type::NoteOn;
}

void HiseEvent::setNoteNumber(int newNoteNumber) noexcept
{

	jassert(isNoteOnOrOff());
	number = jmin<uint8>((uint8)newNoteNumber, 127);
}

int HiseEvent::getPitchWheelValue() const noexcept
{
	return number | (value << 7);
}

void HiseEvent::setPitchWheelValue(int position) noexcept
{
	number = position & 127;
	value = (position >> 7) & 127;
}

void HiseEvent::setStartOffset(uint16 newStartOffset) noexcept
{
	startOffset = newStartOffset;
}

uint16 HiseEvent::getStartOffset() const noexcept
{
	return startOffset;
}

void HiseEvent::setSongPositionValue(int positionInMidiBeats)
{
	number = positionInMidiBeats & 127;
	value = (positionInMidiBeats >> 7) & 127;
}

HiseEventBuffer::HiseEventBuffer()
{
	numUsed = HISE_EVENT_BUFFER_SIZE;
	clear();
}

void HiseEventBuffer::clear()
{
	if (numUsed != 0)
	{
		memset(buffer, 0, numUsed * sizeof(HiseEvent));

		numUsed = 0;
	}
}

void HiseEventBuffer::addEvent(const HiseEvent& hiseEvent)
{
	if (numUsed >= HISE_EVENT_BUFFER_SIZE)
	{
		// Buffer full..
		jassertfalse;
		return;
	}

	if (numUsed == 0)
	{
		insertEventAtPosition(hiseEvent, 0);
		return;
	}

	jassert(numUsed < HISE_EVENT_BUFFER_SIZE);

    const int numToLookFor = jmin<int>(numUsed, HISE_EVENT_BUFFER_SIZE);
    
	for (int i = 0; i < numToLookFor; i++)
	{
		const int timestampInBuffer = buffer[i].getTimeStamp();
		const int messageTimestamp = hiseEvent.getTimeStamp();

		if (timestampInBuffer > messageTimestamp)
		{
			insertEventAtPosition(hiseEvent, i);
			return;
		}
	}

	insertEventAtPosition(hiseEvent, numUsed);
}

void HiseEventBuffer::addEvent(const MidiMessage& midiMessage, int sampleNumber)
{
	HiseEvent e(midiMessage);
	e.setTimeStamp(sampleNumber);

	addEvent(e);
}

void HiseEventBuffer::addEvents(const MidiBuffer& otherBuffer)
{
	clear();

	MidiMessage m;
	int samplePos;

	int index = 0;

	MidiBuffer::Iterator it(otherBuffer);

	while (it.getNextEvent(m, samplePos))
	{
		jassert(index < HISE_EVENT_BUFFER_SIZE);

		HiseEvent e(m);

		if (e.isEmpty()) continue;

		e.swapWith(buffer[index]);

		buffer[index].setTimeStamp(samplePos);

		numUsed++;

		if (numUsed >= HISE_EVENT_BUFFER_SIZE)
		{
			// Buffer full..
			jassertfalse;
			return;
		}

		index++;
	}
}


void HiseEventBuffer::addEvents(const HiseEventBuffer &otherBuffer)
{
	Iterator iter(otherBuffer);

	while (HiseEvent* e = iter.getNextEventPointer(false, false))
	{
		addEvent(*e);
	}
}

bool HiseEventBuffer::timeStampsAreSorted() const
{
	if (numUsed == 0) return true;

	uint16 timeStamp = 0;

	for (int i = 0; i < numUsed; i++)
	{
		auto thisStamp = buffer[i].getTimeStamp();

		if (thisStamp < timeStamp)
			return false;

		timeStamp = thisStamp;
	}

	return true;
}

uint16 HiseEventBuffer::getMinTimeStamp() const
{
	jassert(timeStampsAreSorted());

	if (numUsed == 0)
		return 0;

	return buffer[0].getTimeStamp();
}

uint16 HiseEventBuffer::getMaxTimeStamp() const
{
	jassert(timeStampsAreSorted());

	if (numUsed == 0)
		return 0;

	return buffer[numUsed - 1].getTimeStamp();
}

HiseEvent HiseEventBuffer::getEvent(int index) const
{
	if (index >= 0 && index < HISE_EVENT_BUFFER_SIZE)
	{
		return buffer[index];
	}

	return HiseEvent();
}

void HiseEventBuffer::subtractFromTimeStamps(int delta)
{
	if (numUsed == 0) return;

	jassert(getMinTimeStamp() >= delta);

	jassert(timeStampsAreSorted());

	for (int i = 0; i < numUsed; i++)
	{
		buffer[i].addToTimeStamp((int16)-delta);
	}

	jassert(timeStampsAreSorted());
}

void HiseEventBuffer::moveEventsBelow(HiseEventBuffer& targetBuffer, int highestTimestamp)
{
	if (numUsed == 0) return;

	HiseEventBuffer::Iterator iter(*this);

	int numCopied = 0;

	jassert(targetBuffer.timeStampsAreSorted());
	jassert(timeStampsAreSorted());

	while (HiseEvent* e = iter.getNextEventPointer())
	{
		if (e->getTimeStamp() < (uint32)highestTimestamp)
		{
			targetBuffer.addEvent(*e);
			numCopied++;
		}
		else
		{
			break;
		}
	}

	const int numRemaining = numUsed - numCopied;

	for (int i = 0; i < numRemaining; i++)
		buffer[i] = buffer[i + numCopied];

	HiseEvent::clear(buffer + numRemaining, numCopied);

	numUsed = numRemaining;

	jassert(targetBuffer.timeStampsAreSorted());
	jassert(timeStampsAreSorted());
}

void HiseEventBuffer::moveEventsAbove(HiseEventBuffer& targetBuffer, int lowestTimestamp)
{
	if (numUsed == 0 || (buffer[numUsed - 1].getTimeStamp() < (uint32)lowestTimestamp)) 
		return; // Skip the work if no events with bigger timestamps

	int indexOfFirstElementToMove = -1;

	for (int i = 0; i < numUsed; i++)
	{
		if (buffer[i].getTimeStamp() >= (uint32)lowestTimestamp)
		{
			indexOfFirstElementToMove = i;
			break;
		}
	}

	if (indexOfFirstElementToMove == -1) return;

	for (int i = indexOfFirstElementToMove; i < numUsed; i++)
	{
		targetBuffer.addEvent(buffer[i]);
	}

	HiseEvent::clear(buffer + indexOfFirstElementToMove, numUsed - indexOfFirstElementToMove);

	numUsed = indexOfFirstElementToMove;
}

void HiseEventBuffer::copyFrom(const HiseEventBuffer& otherBuffer)
{
    const int eventsToCopy = jmin<int>(otherBuffer.numUsed, HISE_EVENT_BUFFER_SIZE);
    
	memcpy(buffer, otherBuffer.buffer, sizeof(HiseEvent) * eventsToCopy);

	jassert(otherBuffer.numUsed < HISE_EVENT_BUFFER_SIZE);

	numUsed = otherBuffer.numUsed;
}


HiseEventBuffer::Iterator::Iterator(const HiseEventBuffer& b) :
buffer(const_cast<HiseEventBuffer*>(&b)),
index(0)
{

}

bool HiseEventBuffer::Iterator::getNextEvent(HiseEvent& b, int &samplePosition, bool skipIgnoredEvents/*=false*/, bool skipArtificialEvents/*=false*/) const
{
	while (index < buffer->numUsed && 
		  ((skipArtificialEvents && buffer->buffer[index].isArtificial()) ||
		  (skipIgnoredEvents && buffer->buffer[index].isIgnored())))
	{
		index++;
		jassert(index < HISE_EVENT_BUFFER_SIZE);
	}
		
	if (index < buffer->numUsed)
	{
		b = buffer->buffer[index];
		samplePosition = b.getTimeStamp();
		index++;
		return true;
	}
	else
		return false;
}



HiseEvent* HiseEventBuffer::Iterator::getNextEventPointer(bool skipIgnoredEvents/*=false*/, bool skipArtificialNotes /*= false*/)
{
	const HiseEvent* returnEvent = getNextConstEventPointer(skipIgnoredEvents, skipArtificialNotes);

	return const_cast <HiseEvent*>(returnEvent);
}


const HiseEvent* HiseEventBuffer::Iterator::getNextConstEventPointer(bool skipIgnoredEvents/*=false*/, bool skipArtificialNotes /*= false*/) const
{
	while (index < buffer->numUsed && 
		  ((skipArtificialNotes && buffer->buffer[index].isArtificial()) || 
		  (skipIgnoredEvents && buffer->buffer[index].isIgnored())))
	{
		index++;
		jassert(index < HISE_EVENT_BUFFER_SIZE);
	}

	if (index < buffer->numUsed)
	{
		return &buffer->buffer[index++];

	}
	else
	{
		return nullptr;
	}
}

void HiseEventBuffer::insertEventAtPosition(const HiseEvent& e, int positionInBuffer)
{
	if (numUsed == 0)
	{
		buffer[0] = HiseEvent(e);

		numUsed = 1;

		return;
	}

	if (numUsed > positionInBuffer)
	{
		for (int i = jmin<int>(numUsed-1, HISE_EVENT_BUFFER_SIZE-2); i >= positionInBuffer; i--)
		{
			jassert(i + 1 < HISE_EVENT_BUFFER_SIZE);

			buffer[i + 1] = buffer[i];
		}
	}

    if(positionInBuffer < HISE_EVENT_BUFFER_SIZE)
    {
        buffer[positionInBuffer] = HiseEvent(e);
        numUsed++;
    }
    else
    {
        jassertfalse;
    }
}



EventIdHandler::EventIdHandler(HiseEventBuffer& masterBuffer_) :
	masterBuffer(masterBuffer_),
	currentEventId(1)
{
	firstCC.store(-1);
	secondCC.store(-1);

	//for (int i = 0; i < 128; i++)
	//realNoteOnEvents[i] = HiseEvent();

	memset(realNoteOnEvents, 0, sizeof(HiseEvent) * 128);

	artificialEvents.calloc(HISE_EVENT_ID_ARRAY_SIZE, sizeof(HiseEvent));
}

EventIdHandler::~EventIdHandler()
{

}

void EventIdHandler::handleEventIds()
{
	if (transposeValue != 0)
	{
		HiseEventBuffer::Iterator transposer(masterBuffer);

		while (HiseEvent* m = transposer.getNextEventPointer())
		{
			if (m->isNoteOnOrOff())
			{
				int newNoteNumber = jlimit<int>(0, 127, m->getNoteNumber() + transposeValue);

				m->setNoteNumber(newNoteNumber);
			}


		}
	}

	HiseEventBuffer::Iterator it(masterBuffer);

	while (HiseEvent *m = it.getNextEventPointer())
	{
		// This operates on a global level before artificial notes are possible
		jassert(!m->isArtificial());

		if (m->isAllNotesOff())
		{
			memset(realNoteOnEvents, 0, sizeof(HiseEvent) * 128 * 16);
		}

		if (m->isNoteOn())
		{
			auto channel = jlimit<int>(0, 15, m->getChannel() - 1);

			if (realNoteOnEvents[channel][m->getNoteNumber()].isEmpty())
			{
				m->setEventId(currentEventId);
				realNoteOnEvents[channel][m->getNoteNumber()] = HiseEvent(*m);
				currentEventId++;
			}
			else
			{
				// There is something fishy here so deactivate this event
				m->ignoreEvent(true);
			}
		}
		else if (m->isNoteOff())
		{
			auto channel = jlimit<int>(0, 15, m->getChannel() - 1);

			if (!realNoteOnEvents[channel][m->getNoteNumber()].isEmpty())
			{
				HiseEvent* on = &realNoteOnEvents[channel][m->getNoteNumber()];

				uint16 id = on->getEventId();
				m->setEventId(id);
				m->setTransposeAmount(on->getTransposeAmount());
				*on = HiseEvent();
			}
			else
			{
				// There is something fishy here so deactivate this event
				m->ignoreEvent(true);
			}
		}
		else if (firstCC != -1 && m->isController())
		{
			const int ccNumber = m->getControllerNumber();

			if (ccNumber == firstCC)
			{
				m->setControllerNumber(secondCC);
			}
			else if (ccNumber == secondCC)
			{
				m->setControllerNumber(firstCC);
			}
		}
	}
}

uint16 EventIdHandler::getEventIdForNoteOff(const HiseEvent &noteOffEvent)
{
	jassert(noteOffEvent.isNoteOff());

	const int noteNumber = noteOffEvent.getNoteNumber();

	if (!noteOffEvent.isArtificial())
	{
		auto channel = jlimit<int>(0, 15, noteOffEvent.getChannel() - 1);

		const HiseEvent* e = realNoteOnEvents[channel] + noteNumber;

		return e->getEventId();

		//return realNoteOnEvents +noteNumber.getEventId();
	}
	else
	{
		const uint16 eventId = noteOffEvent.getEventId();

		if (eventId != 0)
			return eventId;

		else
			return lastArtificialEventIds[noteOffEvent.getNoteNumber()];
	}
}

void EventIdHandler::pushArtificialNoteOn(HiseEvent& noteOnEvent) noexcept
{
	jassert(noteOnEvent.isNoteOn());
	jassert(noteOnEvent.isArtificial());

	noteOnEvent.setEventId(currentEventId);
	artificialEvents[currentEventId % HISE_EVENT_ID_ARRAY_SIZE] = noteOnEvent;
	lastArtificialEventIds[noteOnEvent.getNoteNumber()] = currentEventId;

	currentEventId++;
}


HiseEvent EventIdHandler::peekNoteOn(const HiseEvent& noteOffEvent)
{
	if (noteOffEvent.isArtificial())
	{
		if (noteOffEvent.getEventId() != 0)
		{
			return artificialEvents[noteOffEvent.getEventId() % HISE_EVENT_ID_ARRAY_SIZE];
		}
		else
		{
			jassertfalse;

			return HiseEvent();
		}
	}
	else
	{
		auto channel = jlimit<int>(0, 15, noteOffEvent.getChannel() - 1);

		return realNoteOnEvents[channel][noteOffEvent.getNoteNumber()];
	}
}

HiseEvent EventIdHandler::popNoteOnFromEventId(uint16 eventId)
{
	HiseEvent e;
	e.swapWith(artificialEvents[eventId % HISE_EVENT_ID_ARRAY_SIZE]);

	return e;
}

void EventIdHandler::setGlobalTransposeValue(int newTransposeValue)
{
	transposeValue = newTransposeValue;
}

void EventIdHandler::addCCRemap(int firstCC_, int secondCC_)
{
	firstCC = firstCC_;
	secondCC = secondCC_;

	if (firstCC_ == secondCC_)
	{
		firstCC_ = -1;
		secondCC_ = -1;
	}
}


} // namespace hise