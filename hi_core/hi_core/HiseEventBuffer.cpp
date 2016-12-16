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
	else
	{
		type = Type::Empty;
		// unsupported Message type, add another...
		jassertfalse;
	}
	
	number = data[1];
	value = data[2];
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
	case HiseEvent::Type::numTypes: jassertfalse;
	default: jassertfalse;
	}

	return "Undefined";
}



double HiseEvent::getPitchFactorForEvent() const
{
	if (semitones == 0 && cents == 0) return 1.0;

	const float detuneFactor = (float)semitones + (float)cents / 100.0f;

	return (double)Modulation::PitchConverters::octaveRangeToPitchFactor(detuneFactor);
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
	e.setTimeStamp((uint16)sampleNumber);

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

		buffer[index].setTimeStamp((uint16)samplePos);

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

	for (int i = 0; i < numUsed; i++)
	{
		buffer[i].addToTimeStamp((int16)-delta);
	}
}

void HiseEventBuffer::moveEventsBelow(HiseEventBuffer& targetBuffer, int highestTimestamp)
{
	if (numUsed == 0) return;

	HiseEventBuffer::Iterator iter(*this);

	int numCopied = 0;

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
    memset(buffer + eventsToCopy, 0, (HISE_EVENT_BUFFER_SIZE - eventsToCopy) * sizeof(HiseEvent));
    
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
