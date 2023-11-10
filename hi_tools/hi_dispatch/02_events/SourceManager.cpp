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
  SimpleTimer(r.getUpdater()),
  treeId(typeId),
  asyncQueue(r, 1024),
  deferedSyncEvents(r, 128),
  asyncListeners(r, 1024),
  syncListeners(r, 128),
  items(r, sizeof(void*) * 128)
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

void SourceManager::sendSlotChanges(Source& s, const uint8* values, size_t numValues, DispatchType n)
{
	if(n == sendNotification || n == sendNotificationSync)
	if(getRootObject().getState() != State::Running)
	{
		// TODO: implement flushing the deferred sync events (probably synchronously after resuming the running state)
		jassertfalse;
		deferedSyncEvents.push(&s, EventType::SlotChange, values, numValues);
	}
	else
	{
		if(const auto l = getRootObject().getLogger())
			l->log(&s, EventType::SlotChange, values, numValues);

		syncListeners.flush([&](const Queue::FlushArgument& f)
		{
			Queue::FlushArgument eventData;
			eventData.source = &s;
			eventData.eventType = EventType::SlotChange;
			eventData.data = const_cast<uint8*>(values);		// data[0] = slotIndex, data[1+] = the slot values 
			eventData.numBytes = numValues;

			auto& l = f.getTypedObject<dispatch::Listener>();
			const dispatch::Listener::EventParser p(l);
			if(const auto ld = p.parseData(f, eventData))
				l.slotChanged(ld);

			return true;
		}, Queue::FlushType::KeepData);
	}

	if(n == sendNotification || n == sendNotificationAsync)
	{
		if(!asyncListeners.isEmpty())
		{
			asyncQueue.push(&s, EventType::SlotChange, values, numValues);
		}
	}
}

void SourceManager::timerCallback()
{
	StringBuilder b;
	b << "UI timer callback " << getDispatchId();
	TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));

	// skip
	if(getRootObject().getState() != State::Running)
		return;

	// Tell the sources to flush their async changes
	
	items.flush([](const Queue::FlushArgument& a)
	{
		a.getTypedObject<Source>().flushAsyncChanges();
		return true;
	}, Queue::FlushType::KeepData);

	// Implement: flush the queue of async listeners with all pending events that match
	asyncQueue.flush([&](const Queue::FlushArgument& eventData)
	{
		asyncListeners.flush([&](const Queue::FlushArgument& f)
		{
			auto& l = f.getTypedObject<dispatch::Listener>();
			const dispatch::Listener::EventParser p(l);
			if(const auto ld = p.parseData(f, eventData))
				l.slotChanged(ld);

			return true;
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

void SourceManager::addSource(Source* s)
{ items.push(s, EventType::SourcePtr, nullptr, 0); }

void SourceManager::removeSource(Source* s)
{ items.removeAllMatches(s); }
} // dispatch
} // hise