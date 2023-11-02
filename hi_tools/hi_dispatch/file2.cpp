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

SourceManager::SourceManager(RootObject& r, const HashedCharPtr& typeId):
  Queueable(r),
  treeId(typeId),
  asyncQueue(r, 1024),
  deferedSyncEvents(r, 128),
  asyncListeners(r, 1024),
  syncListeners(r, 128)
{
	jassert(!typeId.isDynamic());
}

SourceManager::~SourceManager()
{
	items.clear();
	asyncListeners.clear();
	syncListeners.clear();
	asyncQueue.clear();
	deferedSyncEvents.clear();
}

void SourceManager::sendSlotChanges(Source& s, const uint8* values, size_t numValues)
{
	if(getRootObject().getState() != State::Running)
	{
		deferedSyncEvents.push(&s, EventType::SlotChange, values, numValues);
	}
	else
	{
		if(const auto l = getRootObject().getLogger())
			l->log(&s, values, numValues, EventType::SlotChange);

		syncListeners.flush([&](const Queue::FlushArgument& f)
		{
			auto& l = f.getTypedObject<dispatch::Listener>();
			const dispatch::Listener::EventParser p(l);
			if(const auto ld = p.parseData(f))
				l.slotChanged(ld);

			return false;
		}, Queue::FlushType::KeepData);
	}

	if(!asyncListeners.isEmpty())
	{
		asyncQueue.push(&s, EventType::SlotChange, values, numValues);
	}
}

void SourceManager::timerCallback()
{
	// skip
	if(getRootObject().getState() != State::Running)
		return;

	// Implement: flush the queue of async listeners with all pending events that match
	asyncQueue.flush([&](const Queue::FlushArgument& queuedArgs)
	{
		asyncListeners.flush([&](const Queue::FlushArgument& f)
		{
			auto& l = f.getTypedObject<dispatch::Listener>();
			const dispatch::Listener::EventParser p(l);
			if(const auto ld = p.parseData(f))
				l.slotChanged(ld);

			return false;
		}, Queue::FlushType::KeepData);

		return false;
	});
}

Queue& SourceManager::getListenerQueue(NotificationType n)
{
	return n == sendNotificationAsync ? asyncListeners : syncListeners;
}

const Queue& SourceManager::getListenerQueue(NotificationType n) const
{
	return n == sendNotificationAsync ? asyncListeners : syncListeners;
}

Source::Source(SourceManager& parent_, const String& sourceId_):
	Queueable(parent_.getRootObject()),
	parent(parent_),
	sourceId(sourceId_)
{}

Source::~Source()
{}

SlotSender::SlotSender(Source& s, uint8 slotIndex_):
	slotIndex(slotIndex_),
	obj(s),
	numSlots(0)
{
}

void SlotSender::setNumSlots(int newNumSlots)
{
	if((numSlots + 1) < newNumSlots)
	{
		data.setSize(newNumSlots + 1);
		numSlots = newNumSlots;
		const auto ptr = (uint8*)data.getObjectPtr();
		ptr[0] = slotIndex;
	}
}

bool SlotSender::flush()
{
	if(!pending)
		return false;
			
	obj.parent.sendSlotChanges(obj, static_cast<uint8*>(data.getObjectPtr()), numSlots + 1);
	memset((uint8*)data.getObjectPtr() + 1, 0, numSlots);
	pending = false;
	return true;
}

bool SlotSender::sendChangeMessage(int indexInSlot, NotificationType notify)
{
	jassert(isPositiveAndBelow(indexInSlot, numSlots));

	// data[0] points to the slotIndex
	const auto ptr = static_cast<uint8*>(data.getObjectPtr()) + 1;

	if(ptr[indexInSlot])
		return false;
			
	ptr[indexInSlot] = true;
	pending = true;
		
	if(notify == sendNotificationSync)
		flush();
			
	return true;
}

Listener::EventParser::EventParser(Listener& l_):
	l(l_)
{}

void Listener::EventParser::writeData(Queue& q, EventType t, Queueable* c, uint8* values, uint8 numValues) const
{
	if(t == EventType::SingleListener)
	{
		// Uses the following byte layout:
		// Source* ptr [8 bytes]
		// uint8 numSlots [1 bytes]
		// slotIndexes [numValues]
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

Listener::ListenerData Listener::EventParser::parseData(const Queue::FlushArgument& f) const
{
	jassert(f.source == &l);

	if(f.eventType == EventType::SingleListener)
	{
		// Extract the Source* pointer and the values
		// match the source values against the
		return {}; // TODO
	}
	if(f.eventType == EventType::SubsetListener)
	{
		return {}; // TODO
	}
	if(f.eventType == EventType::AllListener)
	{
		ListenerData l;
		l.t = f.eventType;
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

Listener::Listener(RootObject& r): Queueable(r) {}

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