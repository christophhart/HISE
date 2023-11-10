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
  SomethingWithQueues(r),
  SimpleTimer(r.getUpdater()),
  treeId(typeId),
  sources(r, nullptr, sizeof(void*) * 128)
{
	jassert(!typeId.isDynamic());
}

SourceManager::~SourceManager()
{
	sources.clear();
	
}

void SourceManager::sendSlotChanges(Source& s, const uint8* values, size_t numValues, DispatchType n)
{
	if(n == sendNotificationSync)
	{
		if(getStateFromParent() != State::Running)
		{
			// TODO: implement flushing the deferred sync events (probably synchronously after resuming the running state)
			jassertfalse;
			getEventQueue(DispatchType::sendNotificationSync).push(&s, EventType::SlotChange, values, numValues);
		}
		else
		{
			if(const auto l = getRootObject().getLogger())
				l->log(&s, EventType::SlotChange, values, numValues);

			processEvents(sendNotificationSync);

			auto syncCallback = [&](const Queue::FlushArgument& f)
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
			};

			s.getListenerQueue(DispatchType::sendNotificationSync).flush(syncCallback, Queue::FlushType::KeepData);
			getListenerQueue(DispatchType::sendNotificationSync).flush(syncCallback, Queue::FlushType::KeepData);
		}
	}

	auto pushIfHasListener = [&](SomethingWithQueues& q, DispatchType t)
	{
		if(q.hasListeners(t))
			q.getEventQueue(t).push(&s, EventType::SlotChange, values, numValues);
	};
	
	pushIfHasListener(*this, sendNotificationAsync);
	pushIfHasListener(s, sendNotificationAsync);

	pushIfHasListener(*this, sendNotificationAsyncHiPriority);
	pushIfHasListener(s, sendNotificationAsyncHiPriority);
}

void SourceManager::flushHiPriorityQueue()
{
	auto thisType = DispatchType::sendNotificationAsyncHiPriority;

	if(hasEvents(thisType) && getStateFromParent() == State::Running)
	{
		StringBuilder b;
		b << "Hi priority async callback " << getDispatchId();
		TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));

		MessageManagerLock mm;

		// Tell the sources to flush their async changes
		sources.flush([](const Queue::FlushArgument& a)
		{
			a.getTypedObject<Source>().flushAsyncChanges();
			a.getTypedObject<Source>().processEvents(DispatchType::sendNotificationAsyncHiPriority);

			return true;
		}, Queue::FlushType::KeepData);

		processEvents(DispatchType::sendNotificationAsyncHiPriority);
	}
}

void SourceManager::timerCallback()
{
	flushHiPriorityQueue();

	auto thisType = DispatchType::sendNotificationAsync;

	StringBuilder b;
	b << "UI timer callback " << getDispatchId();
	TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));

	// skip
	if(getStateFromParent() != State::Running)
		return;

	// Tell the sources to flush their async changes
	sources.flush([](const Queue::FlushArgument& a)
	{
		a.getTypedObject<Source>().flushAsyncChanges();
		a.getTypedObject<Source>().processEvents(DispatchType::sendNotificationAsync);
		return true;
	}, Queue::FlushType::KeepData);

	processEvents(DispatchType::sendNotificationAsync);
}


SomethingWithQueues::SomethingWithQueues(RootObject& r):
	Suspendable(r, nullptr),
	asyncQueue(r, this, 1024),
	asyncHiPriorityQueue(r, this, 1024),
	deferedSyncEvents(r, this, 128),
	asyncListeners(r, nullptr, 8192),
	syncListeners(r, nullptr, 128),
	asyncHighPriorityListeners(r, nullptr, 128)
{
	asyncListeners.addPushCheck([](Queueable* s)
	{
		return dynamic_cast<Listener*>(s) != nullptr;
	});

	syncListeners.addPushCheck([](Queueable* s)
	{
		return dynamic_cast<Listener*>(s) != nullptr;
	});

	asyncHighPriorityListeners.addPushCheck([](Queueable* s)
	{
		return dynamic_cast<Listener*>(s) != nullptr;
	});
}

SomethingWithQueues::~SomethingWithQueues()
{
	asyncListeners.clear();
	asyncHighPriorityListeners.clear();
	syncListeners.clear();
	asyncQueue.clear();
	asyncHiPriorityQueue.clear();
	deferedSyncEvents.clear();
}

Queue& SomethingWithQueues::getEventQueue(DispatchType n)
{
	switch(n)
	{
	case DispatchType::sendNotificationAsync:			return asyncQueue;
	case DispatchType::sendNotificationAsyncHiPriority: return asyncHiPriorityQueue;
	case DispatchType::sendNotificationSync:			return deferedSyncEvents;
	default: jassertfalse; return asyncListeners;
	}
}

const Queue& SomethingWithQueues::getEventQueue(DispatchType n) const
{
	switch(n)
	{
	case DispatchType::sendNotificationAsync:			return asyncQueue;
	case DispatchType::sendNotificationAsyncHiPriority: return asyncHiPriorityQueue;
	case DispatchType::sendNotificationSync:			return deferedSyncEvents;
	default: jassertfalse; return asyncListeners;
	}
}

Queue& SomethingWithQueues::getListenerQueue(DispatchType n)
{
	switch(n)
	{
	case DispatchType::sendNotificationAsync:			return asyncListeners;
	case DispatchType::sendNotificationAsyncHiPriority: return asyncHighPriorityListeners;
	case DispatchType::sendNotificationSync:			return syncListeners;
	default: jassertfalse; return asyncListeners;
	}
}

const Queue& SomethingWithQueues::getListenerQueue(DispatchType n) const
{
	switch(n)
	{
	case DispatchType::sendNotificationAsync:			return asyncListeners;
	case DispatchType::sendNotificationAsyncHiPriority: return asyncHighPriorityListeners;
	case DispatchType::sendNotificationSync:			return syncListeners;
	default: jassertfalse; return asyncListeners;
	}
}

void SomethingWithQueues::processEvents(DispatchType n)
{
	if(hasEvents(n) && hasListeners(n))
	{
		StringBuilder b2;
		b2 << "process " << getDispatchId();
		TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b2));

		getEventQueue(n).flush([&](const Queue::FlushArgument& eventData)
		{
			getListenerQueue(n).flush([&](const Queue::FlushArgument& f)
			{
				auto& l = f.getTypedObject<dispatch::Listener>();
				const dispatch::Listener::EventParser p(l);
				if(const auto ld = p.parseData(f, eventData))
				{
					StringBuilder b;
					b << "process listener " << l.getDispatchId();
					TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));
					l.slotChanged(ld);
				}
					

				return true;
			}, Queue::FlushType::KeepData);

			return true;
		});
	}
}

void SomethingWithQueues::setState(State newState)
{
	if(newState != currentState)
	{
		newState = currentState;
			
		if(hasListeners(DispatchType::sendNotificationAsyncHiPriority))
			getEventQueue(DispatchType::sendNotificationAsyncHiPriority).setState(newState);

		if(hasListeners(DispatchType::sendNotificationAsync))
			getEventQueue(DispatchType::sendNotificationAsync).setState(newState);

		if(hasListeners(DispatchType::sendNotificationSync))
			getEventQueue(DispatchType::sendNotificationSync).setState(newState);
	}
}

void SourceManager::addSource(Source* s)
{
	sources.push(s, EventType::SourcePtr, nullptr, 0);
}

void SourceManager::removeSource(Source* s)
{
	sources.removeAllMatches(s);
}

void SourceManager::setState(State newState)
{
	SomethingWithQueues::setState(newState);

	sources.flush([newState](const Queue::FlushArgument& f)
	{
		f.getTypedObject<Source>().setState(newState);
		return true;
	}, Queue::FlushType::KeepData);
}

} // dispatch
} // hise