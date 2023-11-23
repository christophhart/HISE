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



SlotSender::SlotSender(Source& s, uint8 index_, const HashedCharPtr& id_):
	index(index_),
	id(id_),
	obj(s),
	listeners(s.getRootObject(), 128),
	numSlots(0),
	flowManager(s.getRootObject(), s.getDispatchId(), id_)
{
	
}

SlotSender::~SlotSender()
{
}

void SlotSender::setNumSlots(uint8 newNumSlots)
{
	if(numSlots < newNumSlots)
	{
		numSlots = newNumSlots;
	}
}

bool SlotSender::flush(DispatchType n)
{
	auto doneSomething = false;

	// make sure to flush the other slots first
	if(n == DispatchType::sendNotificationAsync ||
	   n == DispatchType::sendNotificationAsyncHiPriority)
	{
		doneSomething |= flush(DispatchType::sendNotificationSync);
	}

	if(n == DispatchType::sendNotificationAsync)
	{
		doneSomething |= flush(DispatchType::sendNotificationAsyncHiPriority);
	}

	auto& thisBitmap = data.get(n);

	if(thisBitmap.isEmpty())
		return doneSomething;

	auto& listenerList = listeners.get(n);

	if(!listenerList.isEmpty())
	{
		doneSomething = true;

		TRACE_FLUSH_FLOW(id, flowManager.closeFlow(n));

		listenerList.flush([&](const Queue::FlushArgument& f)
		{
			switch(f.eventType)
			{
			case EventType::Listener: jassertfalse; break;
			case EventType::ListenerWithoutData:
			{
				Listener::ListenerData d;
				d.s = &obj;
				d.t = f.eventType;
				d.numBytes = 0;
				d.slotIndex = index;
				d.changes = nullptr;

				auto& l = f.getTypedObject<Listener>();
				l.slotChanged(d);
				break;
			}

			case EventType::ListenerAnySlot:
			{
				Listener::ListenerData d;
				d.s = &obj;
				d.t = f.eventType;
				d.numBytes = thisBitmap.getNumBytes();
				d.slotIndex = index;
				d.changes = reinterpret_cast<const uint8*>(thisBitmap.getData());

				auto& l = f.getTypedObject<Listener>();
				l.slotChanged(d);
					break;
			};
			case EventType::ListenerSingleSlot: jassertfalse; break;
			case EventType::SingleListener: jassertfalse; break;
			case EventType::SingleListenerSingleSlot:
			{
				jassert(f.numBytes == 1);
				auto slotIndex = *f.data;

				if(thisBitmap[slotIndex])
				{
					Listener::ListenerData d;
					d.s = &obj;
					d.t = f.eventType;
					d.slotIndex = index;
					d.numBytes = 1;
					d.changes = &slotIndex;

					auto& l = f.getTypedObject<Listener>();
					l.slotChanged(d);
				}

				break;
			}
			case EventType::SingleListenerSubset:
			{
				jassert(f.numBytes == SlotBitmap::getNumBytes());
				SlotBitmap listenerSlots(f.data);

				if(listenerSlots.hasSomeBitsAs(thisBitmap))
				{
					Listener::ListenerData d;
					d.s = &obj;
					d.t = f.eventType;
					d.numBytes = thisBitmap.getNumBytes();
					d.slotIndex = index;
					d.changes = reinterpret_cast<const uint8*>(thisBitmap.getData());

					auto& l = f.getTypedObject<Listener>();
					l.slotChanged(d);
				}

				break;
			}
			case EventType::SubsetListener: jassertfalse; break;
 			case EventType::AllListener: 
			{
				Listener::ListenerData d;
				d.s = &obj;
				d.slotIndex = index;
				d.numBytes = thisBitmap.getNumBytes();
				d.changes = reinterpret_cast<const uint8*>(thisBitmap.getData());

				auto& l = f.getTypedObject<Listener>();
				l.slotChanged(d);
 				break;
			}

			default: jassertfalse; break;
			}

			return true;
		}, Queue::FlushType::KeepData);
	}

	if(listenerList.getState() == State::Running)
		thisBitmap.clear();

	return doneSomething;
}

bool SlotSender::sendChangeMessage(uint8 indexInSlot, DispatchType n)
{
	if(!isPositiveAndBelow(indexInSlot, numSlots))
	{
		jassertfalse;
		return false;
	}

	// Write the change in all slot values
	data.forEachWithDispatchType([&](DispatchType sn, SlotBitmap& d)
	{
		if(sn == DispatchType::sendNotificationSync)
			obj.getParentSourceManager().bumpMessageCounter(false);

		if(d[indexInSlot])
			return;

		if(sn == DispatchType::sendNotificationSync)
			obj.getParentSourceManager().bumpMessageCounter(true);

		d.setBit(indexInSlot, true);

		if(!listeners.get(sn).isEmpty())
			flowManager.openFlow(sn, indexInSlot);
	});

	// If the message wasn't explicitely sent as sendNotificationAsync, it will flush the sync changes immediately
	if(n == sendNotification || n == sendNotificationSync)
	{
		flush(sendNotificationSync);
	}
	
	return true;
}


} // dispatch
} // hise