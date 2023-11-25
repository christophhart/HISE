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

void ListenerQueue::ListenerInformation::operator()(const EventData& f) const
{
	auto d = f;
	bool call = false;

	switch(listenerType)
	{
	case EventType::ListenerWithoutData:
		{
			call = true;
			break;
		}
	case EventType::ListenerAnySlot:
		{
			d.changes = registeredSlots;
			call = true;
			break;
		};
	case EventType::SingleListenerSingleSlot:
		{
			call = d.changes[indexWithinSlot];
			d.indexWithinSlot = indexWithinSlot;
			break;
		}
	case EventType::SingleListenerSubset:
		{
			call = d.changes.hasSomeBitsAs(registeredSlots);
			break;
		}
	case EventType::AllListener: 
		{
			d.changes = registeredSlots;
			call = true;
			break;
		}
	default: jassertfalse; break;
	}

	if(call)
	{
		d.t = listenerType;
		l->slotChanged(d);	
	}
}

void ListenerQueue::removeAllMatches(Listener* l)
{
	if(!isEmpty())
	{
		l->removeFromListenerQueue(this);
	}
}

Listener::Listener(RootObject& r, ListenerOwner& owner_): Queueable(r), owner(owner_)
{
	getRootObject().addTypedChild(this);
}

Listener::~Listener()
{
	this->disconnect_all();

	getRootObject().removeTypedChild(this);
	// Ouch... you need to remove the listener before it gets destroyed!
	jassert(removed);
}

void Listener::addListenerToSingleSource(Source* source, uint8* slotIndexes, uint8 numSlots, DispatchType n)
{
	for(int i = 0; i < numSlots; i++)
	{
		auto lq = source->getListenerQueue(slotIndexes[i], n);
		ListenerQueue::ListenerInformation info(source, this);
		info.listenerType = EventType::ListenerAnySlot;
		info.slotIndex = i;
		addToQueueInternal(lq, info);
		//source->getListenerQueue(slotIndexes[i], n)->push(this, EventType::ListenerAnySlot, nullptr, 0);
	}
	
	removed = false;
}

void Listener::addListenerToSingleSlotIndexWithinSlot(Source* source, uint8 slotIndex, uint8 indexWithinSlot,
	DispatchType n)
{
	auto lq = source->getListenerQueue(slotIndex, n);

	ListenerQueue::ListenerInformation info(source, this);
	info.listenerType = EventType::SingleListenerSingleSlot;
	info.slotIndex = slotIndex;
	info.indexWithinSlot = indexWithinSlot;
	addToQueueInternal(lq, info);
	//q->push(this, EventType::SingleListenerSingleSlot, data, 1);
	removed = false;
}

void Listener::addListenerToSingleSourceAndSlotSubset(Source* source, uint8 slotIndex, const uint8* slotIndexes,
	uint8 numSlots, DispatchType n)
{
	auto lq = source->getListenerQueue(slotIndex, n);

	ListenerQueue::ListenerInformation info(source, this);
	info.listenerType = EventType::SingleListenerSubset;
	info.slotIndex = slotIndex;
	
	for(int i = 0; i < numSlots; i++)
		info.registeredSlots.setBit(slotIndexes[i], true);

	addToQueueInternal(lq, info);
	//q->push(this, EventType::SingleListenerSubset, bitmask.getData(), bitmask.getNumBytes());
}

void Listener::addListenerToAllSources(SourceManager& sourceManager, DispatchType n)
{
	auto l = this;
	sourceManager.forEachSource<Behaviour::AlwaysRun>([n, l](Source& s)
	{
		s.forEachListenerQueue(n, [l, &s](uint8, DispatchType, ListenerQueue* q)
		{
			ListenerQueue::ListenerInformation info(&s, l);
			info.listenerType = EventType::AllListener;
			l->addToQueueInternal(q, info);
			//q->push(l, EventType::AllListener, nullptr, 0);
		});
	});

	removed = false;
}

void Listener::removeListener(Source& s, DispatchType n)
{
	cleared();
	removed = true;

	if(connections.isEmpty())
		return;

	s.forEachListenerQueue(n, [&](uint8, DispatchType, ListenerQueue* q)
	{
		q->removeAllMatches(this);
	});

}

void Listener::removeFromListenerQueue(ListenerQueue* q)
{
	while(connections.removeWithLambda([q](const ConnectionType& ct)
	{
		if(ct.q == q)
		{
			q->removeConnection(ct.con);
			return true;
		}

		return false;
	}))
	{
		;
	}
}
} // dispatch
} // hise