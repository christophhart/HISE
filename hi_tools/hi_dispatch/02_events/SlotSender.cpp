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

#if PERFETTO
PerfettoFlowManager::PerfettoFlowManager(RootObject& r, const HashedCharPtr& parent, const HashedCharPtr& object):
	root(&r)
{
	if(parent || object)
		b << "send " << parent << "::" << object;
}

void PerfettoFlowManager::openFlow(DispatchType n, uint8 slotIndex)
{
	StringBuilder b2;
	b2 << b;
	b2 << "[" << (int)slotIndex << "] (" << n << ")";

	auto& p = pendingFlows.get(n);

	if(p == 0)
		p = root->bumpFlowCounter();
		
	TRACE_EVENT("dispatch", DYNAMIC_STRING_BUILDER(b2), perfetto::Flow::ProcessScoped(p));
}

PerfettoFlowManager::PerfettoFlowType PerfettoFlowManager::closeFlow(DispatchType n)
{
	//b.clearQuick();
	//b << "flush " << n << " " << parent << "::" << object;

	auto pending = pendingFlows.get(n);

	pendingFlows.get(n) = 0;
	return perfetto::TerminatingFlow::ProcessScoped(pending);
}

StringBuilder& PerfettoFlowManager::getStringBuilder()
{
	b.clearQuick();
	return b;
}

void AccumulatedFlowManager::flushSingle(uint64_t idx, dispatch::StringBuilder& b)
{
	if(idx != 0)
	{
		TRACE_EVENT("scripting", DYNAMIC_STRING_BUILDER(b), perfetto::TerminatingFlow::ProcessScoped(idx));
	}
}

void AccumulatedFlowManager::continueFlow(uint64_t idx, const char* stepName)
{
	if(idx != 0)
	{
		pendingTracks.insert(idx);

		dispatch::StringBuilder b;
		b << stepName;
		TRACE_EVENT("scripting", DYNAMIC_STRING_BUILDER(b), perfetto::Flow::ProcessScoped(idx));
	}
}

void AccumulatedFlowManager::flushAll(const char* stepName)
{
	dispatch::StringBuilder b;
	b << stepName;
	// Close all but the last one...
	for(auto& pt: pendingTracks)
	{
		flushSingle(pt, b);
	}

	pendingTracks.clearQuick();
}

uint64_t AccumulatedFlowManager::flushAllButLastOne(const char* stepName, const Identifier& id)
{
	if(pendingTracks.isEmpty())
		return 0;

	dispatch::StringBuilder b;
	b << stepName << id;

	int numPerfettoIds = pendingTracks.size();
	auto lastId = pendingTracks[numPerfettoIds - 1];
	// Close all but the last one...
	for(int i = 0; i < numPerfettoIds-1; i++)
	{
		flushSingle(pendingTracks[i], b);
	}

	pendingTracks.clearQuick();
	return lastId;
}

#endif

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

		Listener::ListenerData l;
		l.slotIndex = index;
		l.s = &obj;
		l.changes = thisBitmap;
		l.t = EventType::SlotChange;

		listenerList.flush(l);

#if 0
		listenerList.flush([&](const SlotQueueType::FlushArgument& f)
		{
			auto d = f;
#if 0
			Listener::ListenerData d;
			d.s = &obj;
			d.t = f.eventType;
			d.slotIndex = index;
#endif

			bool call = false;
			
			switch(d.t)
			{
			case EventType::Listener: jassertfalse; break;
			case EventType::ListenerWithoutData:
			{
				call = true;
				break;
			}
			case EventType::ListenerAnySlot:
			{
				d.changes = thisBitmap;
				call = true;
				break;
			};
			case EventType::ListenerSingleSlot: jassertfalse; break;
			case EventType::SingleListener: jassertfalse; break;
			case EventType::SingleListenerSingleSlot:
			{
				auto slotIndex = d.indexWithinSlot;

				if(thisBitmap[slotIndex])
				{
					d.indexWithinSlot = slotIndex;
					call = true;
				}

				break;
			}
			case EventType::SingleListenerSubset:
			{
				if(d.changes.hasSomeBitsAs(thisBitmap))
				{
					d.changes = thisBitmap;
					call = true;
				}

				break;
			}
			case EventType::SubsetListener: jassertfalse; break;
 			case EventType::AllListener: 
			{
				d.changes = thisBitmap;
				call = true;
 				break;
			}
			default: jassertfalse; break;
			}

			if(call)
			{
				auto& l = f.getTypedObject<Listener>();
				l.slotChanged(d);	
			}

			return true;
		}, Queue::FlushType::KeepData);
#endif
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