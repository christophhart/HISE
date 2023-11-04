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