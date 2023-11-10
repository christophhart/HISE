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
	r.addTypedChild(this);
	jassert(!typeId.isDynamic());
}

SourceManager::~SourceManager()
{
	getRootObject().removeTypedChild(this);
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
	else
	{
		auto pushIfHasListener = [&](SomethingWithQueues& q, DispatchType t)
		{
			if(q.hasListeners(t))
			{
				q.getEventQueue(t).push(&s, EventType::SlotChange, values, numValues);
				return true;
			}

			return false;
		};

		auto doneSomething = pushIfHasListener(*this, n);
		doneSomething |= pushIfHasListener(s, n);

		if(doneSomething && n == DispatchType::sendNotificationAsyncHiPriority)
			getRootObject().notify();
	}
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
		sources.flush([thisType](const Queue::FlushArgument& a)
		{
			a.getTypedObject<Source>().flushChanges(thisType);
			a.getTypedObject<Source>().processEvents(thisType);

			return true;
		}, Queue::FlushType::KeepData);

		processEvents(thisType);
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
	sources.flush([thisType](const Queue::FlushArgument& a)
	{
		a.getTypedObject<Source>().flushChanges(thisType);
		a.getTypedObject<Source>().processEvents(thisType);
		return true;
	}, Queue::FlushType::KeepData);

	processEvents(thisType);
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