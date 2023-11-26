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



RootObject::Child::Child(RootObject& root_):
root(root_)
{
    root.addChild(this);
}

RootObject::Child::~Child()
{
    root.removeChild(this);
}

RootObject::RootObject(PooledUIUpdater* updater_):
	updater(updater_)
{
	auto initQueueSize = 32768;

	children[QueueType].ensureStorageAllocated(initQueueSize);
	children[SourceManagerType].ensureStorageAllocated(initQueueSize/8);
	children[SourceType].ensureStorageAllocated(initQueueSize);
	children[ListenerType].ensureStorageAllocated(initQueueSize);
}

RootObject::~RootObject()
{
	globalState = State::Shutdown;

	ownedHighPriorityThread = nullptr;

	for(int i = 0; i < numChildTypes; i++)
	{
		ScopedWriteLock sl(childLock[i]);
		children[i].clear();
	}
}

void RootObject::addChild(Child* c)
{
	++numChildObjects;
}

void RootObject::removeChild(Child* c)
{
	--numChildObjects;
}

void RootObject::addTypedChild(Child* c)
{
	if(auto typed = dynamic_cast<SourceManager*>(c))
	{
		ScopedWriteLock sl(childLock[SourceManagerType]);
		children[SourceManagerType].add(c);
	}
	else if(auto typed = dynamic_cast<Source*>(c))
	{
		ScopedWriteLock sl(childLock[SourceManagerType]);
		children[SourceType].add(c);
	}
	else if(auto typed = dynamic_cast<Queue*>(c))
	{
		ScopedWriteLock sl(childLock[QueueType]);
		children[QueueType].add(c);
	}
	else if(auto typed = dynamic_cast<dispatch::Listener*>(c))
	{
		ScopedWriteLock sl(childLock[ListenerType]);
		children[ListenerType].add(c);
	}
	else
	{
		jassertfalse;
	}
}

void RootObject::removeTypedChild(Child* c)
{
	if(auto typed = dynamic_cast<SourceManager*>(c))
	{
		ScopedWriteLock sl(childLock[SourceManagerType]);
		children[SourceManagerType].removeFirstMatchingValue(c);
	}
	else if(auto typed = dynamic_cast<Source*>(c))
	{
		ScopedWriteLock sl(childLock[SourceManagerType]);
		children[SourceType].removeFirstMatchingValue(c);
	}
	else if(auto typed = dynamic_cast<Queue*>(c))
	{
		ScopedWriteLock sl(childLock[QueueType]);
		children[QueueType].removeFirstMatchingValue(c);
	}
	else if(auto typed = dynamic_cast<dispatch::Listener*>(c))
	{
		ScopedWriteLock sl(childLock[ListenerType]);
		children[ListenerType].removeFirstMatchingValue(c);
	}
	else
	{
		jassertfalse;
	}
}

void RootObject::clearFromAllQueues(Queueable* objectToBeDeleted, DanglingBehaviour behaviour)
{
	if(globalState == State::Shutdown)
		return;

	StringBuilder n;
	n << "remove queuable";
	ScopedGlobalSuspender sgs(*this, State::Paused, n.toCharPtr());

    forEach<Queue, Behaviour::AlwaysRun>([behaviour, objectToBeDeleted](Queue& q)
    {
	    q.invalidateQueuable(objectToBeDeleted, behaviour); return false;
    });
}

PooledUIUpdater* RootObject::getUpdater()
{ return updater; }

void RootObject::setLogger(Logger* l)
{
    if(currentLogger != nullptr)
        currentLogger->flush();
    currentLogger = l;
}

int RootObject::getNumChildren() const
{ return numChildObjects; }

void RootObject::setState(const HashedPath& path, State newState)
{
	if(newState == State::Shutdown)
		globalState = State::Shutdown;

	forEach<SourceManager, Behaviour::AlwaysRun>([path, newState](SourceManager& sm)
	{
		sm.setState(path, newState);
		return false;
	});
}

#if 0
bool RootObject::callForAllQueues(const std::function<bool(Queue&)>& qf) const
{
	return forEach<Queue, Behaviour::AlwaysRun>(qf);
}

bool RootObject::callForAllSources(const std::function<bool(Source&)>& sf) const
{
	return forEach<Source>(SourceType, sf);
}

bool RootObject::callForAllSourceManagers(const std::function<bool(SourceManager&)>& sf) const
{
	return forEach<SourceManager>(SourceManagerType, sf);
}

bool RootObject::callForAllListeners(const std::function<bool(dispatch::Listener&)>& lf) const
{
	return forEach<Listener>(ListenerType, lf);
}
#endif

uint64 RootObject::flowCounter = 0;

void RootObject::flushHighPriorityQueues(Thread* t)
{
    ScopedValueSetter<bool> svs(hiPriorityPending, true);
    
	forEach<SourceManager, Behaviour::BreakIfPaused>([&](SourceManager& sm)
	{
		if(t->threadShouldExit())
			return true;

		sm.flush(DispatchType::sendNotificationAsyncHiPriority);
		return false;
	});
}

void RootObject::setUseHighPriorityThread(bool shouldUse)
{
	if(shouldUse)
		ownedHighPriorityThread = new HiPriorityThread(*this);
	else
		ownedHighPriorityThread = nullptr;
}

RootObject::ScopedGlobalSuspender::ScopedGlobalSuspender(RootObject& r, State newState, const CharPtr& description):
	root(r),
	prevValue(r.globalState)
{
	isUsed = prevValue != newState;

	if(isUsed)
	{
		StringBuilder b;
		b << "global suspend: " << description;
		TRACE_EVENT_BEGIN("dispatch", DYNAMIC_STRING_BUILDER(b));

		root.setState(HashedPath(CharPtr::Type::Wildcard), newState);
		r.globalState = newState;
	}
}

RootObject::ScopedGlobalSuspender::~ScopedGlobalSuspender()
{
	if(isUsed)
	{
		TRACE_EVENT_END("dispatch");
		root.setState(HashedPath(CharPtr::Type::Wildcard), prevValue);
		root.globalState = prevValue;
	}
}
} // dispatch
} // hise
