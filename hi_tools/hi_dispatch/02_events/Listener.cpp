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


sigslot::connection ListenerQueue::addListener(Listener* l)
{
	return sig.connect(&Listener::onSigSlot, l);
}

void ListenerQueue::removeAllMatches(Listener* l)
{
	sig.disconnect(l);
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
		Connection info(source, this);
		info.listenerType = EventType::ListenerAnySlot;
		info.slotIndex = i;
		info.n = n;
		addToQueueInternal(lq, info);
		//source->getListenerQueue(slotIndexes[i], n)->push(this, EventType::ListenerAnySlot, nullptr, 0);
	}
	
	removed = false;
}

void Listener::addListenerWithoutData(Source* source, uint8 slotIndex, DispatchType n)
{
	Connection info(source, this);
	info.listenerType = EventType::ListenerWithoutData;
	info.slotIndex = slotIndex;
	info.n = n;
	auto lq = source->getListenerQueue(slotIndex, n);

	addToQueueInternal(lq, info);
}

void Listener::addListenerToSingleSlotIndexWithinSlot(Source* source, uint8 slotIndex, uint8 indexWithinSlot,
                                                      DispatchType n)
{
	auto lq = source->getListenerQueue(slotIndex, n);

	Connection info(source, this);
	info.listenerType = EventType::SingleListenerSingleSlot;
	info.slotIndex = slotIndex;
	info.n = n;
	info.indexWithinSlot = indexWithinSlot;
	addToQueueInternal(lq, info);
	//q->push(this, EventType::SingleListenerSingleSlot, data, 1);
	removed = false;
}

void Listener::addListenerToSingleSourceAndSlotSubset(Source* source, uint8 slotIndex, const uint8* slotIndexes,
	uint8 numSlots, DispatchType n)
{
	auto lq = source->getListenerQueue(slotIndex, n);

	Connection info(source, this);
	info.listenerType = EventType::SingleListenerSubset;
	info.slotIndex = slotIndex;
	info.n = n;
	
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
		s.forEachListenerQueue(n, [l, &s, n](uint8, DispatchType, ListenerQueue* q)
		{
			Connection info(&s, l);
			info.listenerType = EventType::AllListener;
			info.n = n;
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

	if(currentConnection.isEmpty())
		return;

	s.forEachListenerQueue(n, [&](uint8, DispatchType, ListenerQueue* q)
	{
		for(int i = 0; i < currentConnection.size(); i++)
		{
			if(currentConnection[i].first == q)
			{
				currentConnection[i].first->removeAllMatches(this);
				currentConnection.removeElement(i--);
			}
		}
	});

}


} // dispatch
} // hise