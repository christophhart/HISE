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



namespace hise {
namespace dispatch {	
using namespace juce;

Listener::EventParser::EventParser(Listener& l_):
	l(l_)
{}

void Listener::EventParser::writeData(Queue& q, EventType t, Queueable* c, uint8* values, uint8 numValues) const
{
	if(t == EventType::SingleListener)
	{
		// The listener data for a single listener uses the following byte layout:
		// Source* ptr [8 bytes]    - the source that it's listening to
		// uint8 numSlots [1 bytes] - the number of slots
		// slotIndexes [numValues]	- the slot indexes it listens to
		const auto data = (uint8*)alloca(sizeof(void*) + 1 + numValues);
		auto offset = 0;
		*reinterpret_cast<RootObject::Child**>(data + offset) = c;
		offset += sizeof(Source*);
		data[offset] = numValues;
		offset += 1;
		memcpy(data + offset, values, numValues);
		offset += numValues;
		q.push(&l, t, data, offset);
	}
	if(t == EventType::SubsetListener)
	{
		// uses the following byte layout:
		// Source* ptr [8 bytes]
		// uint8 numSources [1 byte]
		// sources [numSources bytes * sizeof(void*)]
		// uint8 numSlots [1 byte]
		// slotIndexes [numSlots bytes]
	}
	if(t == EventType::AllListener)
	{
		q.push(&l, t, nullptr, 0);
	}
}

Listener::ListenerData Listener::EventParser::parseData(const Queue::FlushArgument& listenerData, const Queue::FlushArgument& eventData) const
{
	jassert(listenerData.source == &l);
	jassert(eventData.source != &l);
	jassert(eventData.eventType == EventType::SlotChange);
	jassert(eventData.numBytes >= 2); // data[0] = numSlots && data[1] = slotValue with numSlots >= 1

	auto sourceFromEvent = dynamic_cast<Source*>(eventData.source);

	// the slot index of the event (the first byte in the data)
	auto eventSlotIndex = eventData.data[0];

	// the changed values of the event (offset by 1)
	auto eventSlotPtr = eventData.data + 1;

	// the number of slots in the data
	auto numSlotsInEvent = eventData.numBytes - 1;

	if(listenerData.eventType == EventType::SingleListener)
	{
		// the single listener pointer is written at the start of the data
		auto ptr1 = *reinterpret_cast<RootObject::Child**>(listenerData.data);
		auto sourceFromListener = dynamic_cast<Source*>(ptr1);

		if(sourceFromListener == sourceFromEvent)
		{
			// the number of slot indexes that the listener is registered too
			auto numListenedSlots = *(listenerData.data + sizeof(void*));

			// the pointer to the slot index array
			uint8* slotPtr = listenerData.data + sizeof(void*) + 1;

			for(int i = 0; i < numListenedSlots; i++)
			{
				if(slotPtr[i] == eventSlotIndex)
				{
					ListenerData l;

					l.s = sourceFromEvent;
					l.t = EventType::SlotChange;
					l.slotIndex = eventSlotIndex;
					l.numBytes = numSlotsInEvent;
					l.changes = eventSlotPtr;

					return l;
				}
			}
		}

		return {};
	}
	if(listenerData.eventType == EventType::SubsetListener)
	{
		jassertfalse;
		return {}; // TODO
	}
	if(listenerData.eventType == EventType::AllListener)
	{
		ListenerData l;
		l.t = eventData.eventType;
		l.s = nullptr;
		return l;
	}
	return {};
}

size_t Listener::EventParser::writeSourcePointer(Source* s, uint8** data)
{
	auto offset = 0;
	// align...

	// aligned:
	*reinterpret_cast<Source**>(data) = s;
	*data += sizeof(Source*);
	return offset + sizeof(Source*);
}

Listener::Listener(RootObject& r, ListenerOwner& owner_): Queueable(r), owner(owner_) {}

Listener::~Listener() {}

void Listener::addListenerToSingleSource(Source* source, uint8* slotIndexes, uint8 numSlots, NotificationType n)
{
	const EventParser writer(*this);
	auto& q = source->getParentSourceManager().getListenerQueue(n);
	writer.writeData(q, EventType::SingleListener, source, slotIndexes, numSlots);
}

void Listener::addListenerToAllSources(SourceManager& sourceManager, NotificationType n)
{
	const EventParser writer(*this);
	auto& q = sourceManager.getListenerQueue(n);
	writer.writeData(q, EventType::AllListener, &sourceManager, nullptr, 0);
}

void Listener::addListenerToSubset(SourceManager& sourceManager, const SubsetFunction& sf, NotificationType n)
{
	auto numMax = sizeof(Source*) * sourceManager.getNumChildSources();
	auto ptr = (uint8*)alloca(numMax);
	auto numWritten = sf(&ptr);
	EventParser writer(*this);
	auto& q = sourceManager.getListenerQueue(n);
	writer.writeData(q, EventType::SubsetListener, &sourceManager, ptr, numWritten);
	q.push(this, EventType::SubsetListener, ptr, numWritten);
}

void Listener::removeListener(SourceManager& s, NotificationType n)
{
	if(n == sendNotification || n == sendNotificationSync)
		s.getListenerQueue(sendNotificationSync).removeAllMatches(this);
	if(n == sendNotification || n == sendNotificationAsync)
		s.getListenerQueue(sendNotificationAsync).removeAllMatches(this);
}

} // dispatch
} // hise