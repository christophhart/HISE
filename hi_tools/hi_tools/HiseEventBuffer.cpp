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

    if(message.isChannelPressure())
        value = number;
    
	setTimeStamp((int)message.getTimeStamp());
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

juce::MidiMessage HiseEvent::toMidiMesage() const
{
	switch (type)
	{
	case Type::NoteOn:		return MidiMessage::noteOn(channel, number + transposeValue, value);
	case Type::NoteOff:		return MidiMessage::noteOff(channel, number + transposeValue);
	case Type::Controller:	return MidiMessage::controllerEvent(channel, number, value);
	case Type::PitchBend:	return MidiMessage::pitchWheel(channel, getPitchWheelValue());
	case Type::Aftertouch:	return MidiMessage::aftertouchChange(channel, number, value);
	case Type::ProgramChange: return MidiMessage::programChange(channel, getPitchWheelValue());
	case Type::AllNotesOff:	return MidiMessage::allNotesOff(channel);
    default: break;
	}

	// the other types can't be converted correctly...
	jassertfalse;
	return MidiMessage();
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
	return getTypeString(type);
}

String HiseEvent::getTypeString(HiseEvent::Type t)
{
	switch (t)
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

bool HiseEvent::isIgnored() const noexcept
{

	constexpr int ignoreMask = 0x40000000;
	bool ignored = timestamp & ignoreMask;
	return ignored;
}

void HiseEvent::ignoreEvent(bool shouldBeIgnored) noexcept
{

	constexpr int ignoreMask = 0x40000000;
	constexpr int everythingElse = 0xBFFFFFFF;

	if (shouldBeIgnored)
		timestamp |= ignoreMask;
	else
		timestamp &= everythingElse;
}

void HiseEvent::setArtificial() noexcept
{

	constexpr int aMask = 0x80000000;
	timestamp |= aMask;
}

bool HiseEvent::isArtificial() const noexcept
{

	constexpr int aMask = 0x80000000;
	bool artificial = (timestamp & aMask) != 0;
	return artificial;
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

void HiseEvent::setGain(int decibels) noexcept
{
	gain = (int8)jlimit<int>(-100, 36, decibels);
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

HiseEvent HiseEvent::createTimerEvent(uint8 timerIndex, int offset)
{
	HiseEvent e(Type::TimerEvent, 0, 0, timerIndex);

	e.setArtificial();
	e.setTimeStamp(offset);

	return e;
}

bool HiseEvent::matchesMidiData(const HiseEvent& other) const
{
	return type == other.type &&
		   getNoteNumberIncludingTransposeAmount() == other.getNoteNumberIncludingTransposeAmount() &&
		   value == other.value;
}

int HiseEvent::getTimeStamp() const noexcept
{
	constexpr uint32 tsMask = 0x0FFFFFFF;
	return static_cast<int>(timestamp & tsMask);
}

void HiseEvent::setTimeStamp(int newTimestamp) noexcept
{
	jassert(isPositiveAndBelow(newTimestamp, 0x3FFFFFFF));
	newTimestamp = jlimit(0, 0x3FFFFFFF, newTimestamp);

	constexpr uint32 tsMask = 0x3FFFFFFF;
	constexpr uint32 flagMask = 0xC0000000;
	uint32 flagValues = timestamp & flagMask;
	timestamp = flagValues | (static_cast<uint32>(newTimestamp) & tsMask);
}

void HiseEvent::addToTimeStamp(int delta) noexcept
{
	int v = getTimeStamp() + delta;
	v = jmax<int>(0, v);
	setTimeStamp(v);
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

int HiseEvent::getControllerNumber() const noexcept
{
	if (type == Type::PitchBend)
		return PitchWheelCCNumber;
	if (type == Type::Aftertouch)
		return AfterTouchCCNumber;

	return  number;
}

void HiseEvent::setSongPositionValue(int positionInMidiBeats)
{
	number = positionInMidiBeats & 127;
	value = (positionInMidiBeats >> 7) & 127;
}

void HiseEvent::clear()
{
	*this = {};
}

juce::String HiseEvent::toDebugString() const
{
	String s;

	s << getTypeAsString() << ", Number: " << number << ", Value: " << value;
	s << ", Channel: " << channel;
	s << ", Timestamp: " << getTimeStamp();
	s << ", Event ID: " << String(getEventId());
	s << (isArtificial() ? ", artficial" : "");
	s << (isIgnored() ? ", ignored" : "");

	return s;
}

HiseEvent::ChannelFilterData::ChannelFilterData():
	enableAllChannels(true)
{
	for (int i = 0; i < 16; i++) activeChannels[i] = false;
}

void HiseEvent::ChannelFilterData::restoreFromData(int data)
{
	BigInteger d(data);

	enableAllChannels = d[0];
	for (int i = 0; i < 16; i++) activeChannels[i] = d[i + 1];
}

int HiseEvent::ChannelFilterData::exportData() const
{
	BigInteger d;

	d.setBit(0, enableAllChannels);
	for (int i = 0; i < 16; i++) d.setBit(i + 1, activeChannels[i]);

	return d.toInteger();
}

void HiseEvent::ChannelFilterData::setEnableAllChannels(bool shouldBeEnabled) noexcept
{ enableAllChannels = shouldBeEnabled; }

bool HiseEvent::ChannelFilterData::areAllChannelsEnabled() const noexcept
{ return enableAllChannels; }

void HiseEvent::ChannelFilterData::setEnableMidiChannel(int channelIndex, bool shouldBeEnabled) noexcept
{
	activeChannels[channelIndex] = shouldBeEnabled;
}

bool HiseEvent::ChannelFilterData::isChannelEnabled(int channelIndex) const noexcept
{
	return activeChannels[channelIndex];
}

HiseEventBuffer::EventStack::EventStack()
{
	clear();
}

void HiseEventBuffer::EventStack::push(const HiseEvent& newEvent)
{
	size = jmin<int>(16, size + 1);

	data[size-1] = HiseEvent(newEvent);

}

HiseEvent HiseEventBuffer::EventStack::pop()
{
	if (size == 0) return HiseEvent();

	HiseEvent returnEvent = data[size - 1];
	data[size - 1] = HiseEvent();

	size = jmax<int>(0, size-1);

	return returnEvent;
}

bool HiseEventBuffer::EventStack::peekNoteOnForEventId(uint16 eventId, HiseEvent& eventToFill)
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

bool HiseEventBuffer::EventStack::popNoteOnForEventId(uint16 eventId, HiseEvent& eventToFill)
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

void HiseEventBuffer::EventStack::clear()
{
	for (int i = 0; i < 16; i++)
		data[i] = HiseEvent();
	size = 0;
}

const HiseEvent* HiseEventBuffer::EventStack::peek() const
{
	if (size == 0) return nullptr;

	return &data[size - 1];
}

HiseEvent* HiseEventBuffer::EventStack::peek()
{ 
	if (size == 0) return nullptr;

	return &data[size - 1];
}

int HiseEventBuffer::EventStack::getNumUsed()
{ return size; }

HiseEventBuffer::HiseEventBuffer()
{
	numUsed = HISE_EVENT_BUFFER_SIZE;
	clear();
}

bool HiseEventBuffer::operator==(const HiseEventBuffer& other)
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

#if JUCE_DEBUG
			if (!timeStampsAreSorted())
			{
				for (int j = 0; j < numToLookFor; j++)
					DBG(buffer[j].toDebugString());
			}
#endif

			jassert(timeStampsAreSorted());

			return;
		}
	}

	insertEventAtPosition(hiseEvent, numUsed);

	jassert(timeStampsAreSorted());
}

void HiseEventBuffer::addEvent(const MidiMessage& midiMessage, int sampleNumber)
{
	HiseEvent e(midiMessage);
	e.setTimeStamp(sampleNumber);

	addEvent(e);
	jassert(timeStampsAreSorted());
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

	jassert(timeStampsAreSorted());
}


void HiseEventBuffer::addEvents(const HiseEventBuffer &otherBuffer)
{
	Iterator iter(otherBuffer);

	while (HiseEvent* e = iter.getNextEventPointer(false, false))
	{
		addEvent(*e);
	}

	jassert(timeStampsAreSorted());
}

void HiseEventBuffer::sortTimestamps()
{
	switch(numUsed)
	{
		case 0: 
		case 1: return;
		case 2: 
			if (buffer[1] < buffer[0]) 
				std::swap(buffer[0], buffer[1]);
			return;
		default:
			std::sort(begin(), end());
			return;
	}
}

void HiseEventBuffer::multiplyTimestamps(int factor)
{
	for (auto& e : *this)
		e.setTimeStamp(e.getTimeStamp() * factor);
}

bool HiseEventBuffer::timeStampsAreSorted() const
{
	if (numUsed == 0) return true;

	int timeStamp = 0;

	for (int i = 0; i < numUsed; i++)
	{
		auto thisStamp = buffer[i].getTimeStamp();

		if (thisStamp < timeStamp)
			return false;

		timeStamp = thisStamp;
	}

	return true;
}

int HiseEventBuffer::getMinTimeStamp() const
{
	jassert(timeStampsAreSorted());

	if (numUsed == 0)
		return 0;

	return buffer[0].getTimeStamp();
}

int HiseEventBuffer::getMaxTimeStamp() const
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

hise::HiseEvent HiseEventBuffer::popEvent(int index)
{
	if (isPositiveAndBelow(index, numUsed))
	{
		auto e = getEvent(index);

		for (int i = index; i < numUsed; i++)
			buffer[i] = buffer[i + 1];

		buffer[numUsed - 1] = {};
		numUsed--;

		return e;
	}
	else
		return {};
}

void HiseEventBuffer::subtractFromTimeStamps(int delta)
{
	if (numUsed == 0) return;

	jassert(getMinTimeStamp() >= delta);

	jassert(timeStampsAreSorted());

	for (int i = 0; i < numUsed; i++)
	{
		buffer[i].addToTimeStamp(-delta);
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
		if (e->getTimeStamp() < highestTimestamp)
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
	if (numUsed == 0 || (buffer[numUsed - 1].getTimeStamp() < lowestTimestamp)) 
		return; // Skip the work if no events with bigger timestamps

	int indexOfFirstElementToMove = -1;

	for (int i = 0; i < numUsed; i++)
	{
		if (buffer[i].getTimeStamp() >= lowestTimestamp)
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

EventIdHandler::ChokeListener::~ChokeListener()
{}

int EventIdHandler::ChokeListener::getChokeGroup() const
{ return chokeGroup; }

void EventIdHandler::ChokeListener::setChokeGroup(int newChokeGroup)
{
	chokeGroup = newChokeGroup;
}

bool EventIdHandler::isArtificialEventId(uint16 eventId) const
{
	return !artificialEvents[eventId % HISE_EVENT_ID_ARRAY_SIZE].isEmpty();
}

void EventIdHandler::addChokeListener(ChokeListener* l)
{
	chokeListeners.addIfNotAlreadyThere(l);
}

void EventIdHandler::removeChokeListener(ChokeListener* l)
{
	chokeListeners.removeAllInstancesOf(l);
}

EventIdHandler::EventIdHandler(HiseEventBuffer& masterBuffer_) :
	masterBuffer(masterBuffer_),
	currentEventId(1)
{
	memset(realNoteOnEvents, 0, sizeof(HiseEvent) * 128 * 16);
	memset(lastArtificialEventIds, 0, sizeof(uint16) * 128 * 16);

	artificialEvents.calloc(HISE_EVENT_ID_ARRAY_SIZE, sizeof(HiseEvent));
}

EventIdHandler::~EventIdHandler()
{

}

void EventIdHandler::handleEventIds()
{
	HiseEventBuffer::Iterator it(masterBuffer);

	while (HiseEvent *m = it.getNextEventPointer())
	{
		// This operates on a global level before artificial notes are possible
		jassert(!m->isArtificial());

		if (m->isAllNotesOff())
        {
			//overlappingNoteOns.clear();

			for (int nc = 0; nc < 16; nc++)
			{
				for (int noteNumber = 0; noteNumber < 128; noteNumber++)
				{
					auto& e = realNoteOnEvents[nc][noteNumber];

					if (!e.isEmpty())
					{
						overlappingNoteOns.insert(e);
						e = {};
					}
				}
			}

			//memset(realNoteOnEvents, 0, sizeof(HiseEvent) * 128 * 16);
        }

		if (m->isNoteOn())
		{
			auto channel = jlimit<int>(0, 15, m->getChannel() - 1);

			m->setEventId(currentEventId++);

			if (realNoteOnEvents[channel][m->getNoteNumber()].isEmpty())
				realNoteOnEvents[channel][m->getNoteNumber()] = HiseEvent(*m);
			else
				overlappingNoteOns.insertWithoutSearch(HiseEvent(*m));
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
				int s = overlappingNoteOns.size();
				bool found = false;

				for (int i = 0; i < s; i++)
				{
					auto on = overlappingNoteOns[i];

					if (on.getNoteNumber() == m->getNoteNumber() && on.getChannel() == m->getChannel())
					{
						auto id = on.getEventId();
						m->setEventId(id);
						m->setTransposeAmount(on.getTransposeAmount());
						overlappingNoteOns.removeElement(i);
						found = true;
						break;
					}
				}

				if (!found)
				{
					m->setEventId(currentEventId++);
					// There is something fishy here so deactivate this event
					m->ignoreEvent(true);
				}
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

		if (!e->isEmpty())
		{
			return e->getEventId();
		}
		else
		{
			for (auto no : overlappingNoteOns)
			{
				if (noteOffEvent.getNoteNumber() == no.getNoteNumber() && noteOffEvent.getChannel() == no.getChannel())
					return no.getEventId();
			}

			jassertfalse;
			return 0;
		}
	}
	else
	{
		const uint16 eventId = noteOffEvent.getEventId();

		if (eventId != 0)
			return eventId;

		else
			return lastArtificialEventIds[noteOffEvent.getChannel() % 16][noteOffEvent.getNoteNumber()];
	}
}

void EventIdHandler::pushArtificialNoteOn(HiseEvent& noteOnEvent) noexcept
{
	jassert(noteOnEvent.isNoteOn());
	jassert(noteOnEvent.isArtificial());

	noteOnEvent.setEventId(currentEventId);
	artificialEvents[currentEventId % HISE_EVENT_ID_ARRAY_SIZE] = noteOnEvent;
	lastArtificialEventIds[noteOnEvent.getChannel() % 16][noteOnEvent.getNoteNumber()] = currentEventId;

	currentEventId++;
}

void EventIdHandler::reinsertArtificialNoteOn(HiseEvent& noteOnEvent) noexcept
{
	jassert(noteOnEvent.isNoteOn());
	jassert(noteOnEvent.isArtificial());

	artificialEvents[noteOnEvent.getEventId() % HISE_EVENT_ID_ARRAY_SIZE] = noteOnEvent;
	lastArtificialEventIds[noteOnEvent.getChannel() % 16][noteOnEvent.getNoteNumber()] = noteOnEvent.getEventId();
}


HiseEvent EventIdHandler::popNoteOnFromEventId(uint16 eventId)
{
	HiseEvent e;
	e.swapWith(artificialEvents[eventId % HISE_EVENT_ID_ARRAY_SIZE]);

	return e;
}


void EventIdHandler::sendChokeMessage(ChokeListener* source, const HiseEvent& e)
{
	if (auto group = source->getChokeGroup())
	{
		for (auto l : chokeListeners)
		{
			if (l == source || l == nullptr || l->getChokeGroup() != group)
				continue;

			l->chokeMessageSent();
		}
	}
}

} // namespace hise
