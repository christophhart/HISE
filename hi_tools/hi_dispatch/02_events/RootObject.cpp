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
	queues = new Queue(*this, 4096);
	sourceManagers = new Queue(*this, 64);
	sources = new Queue(*this, 4096);
	listeners = new Queue(*this, 4096);
}

RootObject::~RootObject()
{
	ownedHighPriorityThread = nullptr;

	sources->setQueueState(State::Shutdown);
	sourceManagers->setQueueState(State::Shutdown);
	listeners->setQueueState(State::Shutdown);
	queues->setQueueState(State::Shutdown);

	sources = nullptr;
	sourceManagers = nullptr;
	listeners = nullptr;
	queues = nullptr;

    jassert(numChildObjects == 0);
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
	if(queues == nullptr)
	{
		jassert(numChildObjects == 1);
		return;
	}

	if(auto typed = dynamic_cast<SourceManager*>(c))
	{
		sourceManagers->push(typed, EventType::SourcePtr, nullptr, 0);
	}
	else if(auto typed = dynamic_cast<Source*>(c))
	{
		sources->push(typed, EventType::SourcePtr, nullptr, 0);
	}
	else if(auto typed = dynamic_cast<Queue*>(c))
	{
		queues->push(typed, EventType::SourcePtr, nullptr, 0);
	}
	else if(auto typed = dynamic_cast<dispatch::Listener*>(c))
	{
		listeners->push(typed, EventType::SourcePtr, nullptr, 0);
	}
	else
	{
		jassertfalse;
	}
}

void RootObject::removeTypedChild(Child* c)
{
	if(queues == nullptr)
	{
		jassert(numChildObjects == 1);
		return;
	}
		

	if(auto typed = dynamic_cast<SourceManager*>(c))
	{
		Queue::ScopedExplicitSuspender ses(*sourceManagers);
		sourceManagers->removeFirstMatchInQueue(typed);
	}
	else if(auto typed = dynamic_cast<Source*>(c))
	{
		Queue::ScopedExplicitSuspender ses(*sources);
		sources->removeFirstMatchInQueue(typed);
	}
	else if(auto typed = dynamic_cast<Queue*>(c))
	{
		Queue::ScopedExplicitSuspender ses(*queues);
		queues->removeFirstMatchInQueue(typed);
	}
	else if(auto typed = dynamic_cast<dispatch::Listener*>(c))
	{
		Queue::ScopedExplicitSuspender ses(*listeners);
		listeners->removeFirstMatchInQueue(typed);
	}
	else
	{
		jassertfalse;
	}
}

void RootObject::clearFromAllQueues(Queueable* objectToBeDeleted, DanglingBehaviour behaviour)
{
    callForAllQueues([behaviour, objectToBeDeleted](Queue& q)
    {
	    q.invalidateQueuable(objectToBeDeleted, behaviour); return false;
    });
}

void RootObject::minimiseQueueOverhead()
{
    jassertfalse;
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
	callForAllSourceManagers([path, newState](SourceManager& sm)
	{
		sm.setState(path, newState);
		return false;
	});
}

struct QueueIteratorHelpers
{
	template <typename T> static bool forEach(Queue* q, const std::function<bool(T&)>& objectFunction)
	{
		return !q->flush([objectFunction](const Queue::FlushArgument& f)
		{
			auto& s = f.getTypedObject<T>();

			if(objectFunction(s))
				return false;

			return true;
		}, Queue::FlushType::KeepData);
	}
};

bool RootObject::callForAllQueues(const std::function<bool(Queue&)>& qf) const
{
	if(queues == nullptr)
		return false;

	return QueueIteratorHelpers::forEach<Queue>(queues, qf);
}

bool RootObject::callForAllSources(const std::function<bool(Source&)>& sf) const
{
	return QueueIteratorHelpers::forEach<Source>(sources, sf);
}

bool RootObject::callForAllSourceManagers(const std::function<bool(SourceManager&)>& sf) const
{
	return QueueIteratorHelpers::forEach<SourceManager>(sourceManagers, sf);
}

bool RootObject::callForAllListeners(const std::function<bool(dispatch::Listener&)>& lf) const
{
	return QueueIteratorHelpers::forEach<dispatch::Listener>(listeners, lf);
}

uint64 RootObject::flowCounter = 0;

void RootObject::flushHighPriorityQueues(Thread* t)
{
	callForAllSourceManagers([&](SourceManager& sm)
	{
		if(t->threadShouldExit())
			return true;

		sm.flush(DispatchType::sendNotificationAsyncHiPriority);
		return false;
	});
}


} // dispatch
} // hise