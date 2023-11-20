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

Listener::Listener(RootObject& r, ListenerOwner& owner_): Queueable(r), owner(owner_)
{
	getRootObject().addTypedChild(this);
}

Listener::~Listener()
{
	getRootObject().removeTypedChild(this);
	// Ouch... you need to remove the listener before it gets destroyed!
	jassert(removed);
}

void Listener::addListenerToSingleSource(Source* source, uint8* slotIndexes, uint8 numSlots, DispatchType n)
{
	for(int i = 0; i < numSlots; i++)
		source->getListenerQueue(slotIndexes[i], n)->push(this, EventType::ListenerAnySlot, nullptr, 0);

	removed = false;
}

void Listener::addListenerToSingleSlotIndexWithinSlot(Source* source, uint8 slotIndex, uint8 indexWithinSlot,
	DispatchType n)
{
	uint8 data[1] = { indexWithinSlot };
	auto q = source->getListenerQueue(slotIndex, n);
	q->push(this, EventType::SingleListenerSingleSlot, data, 1);
	removed = false;
}

void Listener::addListenerToSingleSourceAndSlotSubset(Source* source, uint8 slotIndex, const uint8* slotIndexes,
	uint8 numSlots, DispatchType n)
{
	auto q = source->getListenerQueue(slotIndex, n);

	SlotBitmap bitmask;

	for(int i = 0; i < numSlots; i++)
		bitmask.setBit(slotIndexes[i], true);

	q->push(this, EventType::SingleListenerSubset, bitmask.getData(), bitmask.getNumBytes());
}

void Listener::addListenerToAllSources(SourceManager& sourceManager, DispatchType n)
{
	auto l = this;
	sourceManager.forEachSource([n, l](Source& s)
	{
		s.forEachListenerQueue(n, [l](uint8, DispatchType, Queue* q)
		{
			q->push(l, EventType::AllListener, nullptr, 0);
		});
	});

	removed = false;
}

void Listener::removeListener(Source& s, DispatchType n)
{
	s.forEachListenerQueue(n, [&](uint8, DispatchType, Queue* q)
	{
		q->removeAllMatches(this);
	});

	removed = true;
}

} // dispatch
} // hise