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
  Thread("Dispatch HiPriority Thread"),
	updater(updater_)
{
    childObjects.ensureStorageAllocated(8192);
    sourceManagers.ensureStorageAllocated(64);
    sources.ensureStorageAllocated(4096);
    listeners.ensureStorageAllocated(4096);

    startThread(7);
}

RootObject::~RootObject()
{
    jassert(childObjects.isEmpty());
}

void RootObject::addChild(Child* c)
{
    childObjects.add(c);

    if(auto typed = dynamic_cast<SourceManager*>(c))
    {
	    sourceManagers.add(typed);
    }
    if(auto typed = dynamic_cast<Source*>(c))
    {
	    sources.add(typed);
    }
    if(auto typed = dynamic_cast<Queue*>(c))
    {
	    queues.add(typed);
    }
    if(auto typed = dynamic_cast<Listener*>(c))
    {
	    listeners.add(typed);
    }
    
}

void RootObject::removeChild(Child* c)
{
    int indexInRoot = childObjects.indexOf(c);
    childObjects.remove(indexInRoot);

    if(auto typed = dynamic_cast<SourceManager*>(c))
    {
        int indexInTyped = sourceManagers.indexOf(typed);
	    sourceManagers.remove(indexInTyped);
    }
    if(auto typed = dynamic_cast<Source*>(c))
    {
	    int indexInTyped = sources.indexOf(typed);
	    sources.remove(indexInTyped);
    }
    if(auto typed = dynamic_cast<Queue*>(c))
    {
	    int indexInTyped = queues.indexOf(typed);
	    queues.remove(indexInTyped);
    }
    if(auto typed = dynamic_cast<Listener*>(c))
    {
	    int indexInTyped = listeners.indexOf(typed);
	    listeners.remove(indexInTyped);
    }
}

int RootObject::clearFromAllQueues(Queueable* objectToBeDeleted, DanglingBehaviour behaviour)
{
    jassert(childObjects.contains(objectToBeDeleted));
    int indexInRoot = childObjects.indexOf(objectToBeDeleted);
    callForAllQueues([behaviour, objectToBeDeleted](Queue& q){ q.invalidateQueuable(objectToBeDeleted, behaviour); return false; });
    return indexInRoot;
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

void RootObject::setState(const HashedCharPtr& sourceManagerId, State newState)
{
	callForAllSourceManagers([sourceManagerId, newState](SourceManager& sm)
	{
		if(sm.getDispatchId() == sourceManagerId)
		{
			sm.setState(newState);
			return true;
		}

		return false;
	});
}

bool RootObject::callForAllQueues(const std::function<bool(Queue&)>& qf) const
{
    for(auto q: queues)
    {
        if(qf(*q))
        	return true;
    }
    
    return false;
}

bool RootObject::callForAllSources(const std::function<bool(Source&)>& sf) const
{
	for(auto s: sources)
	{
		if(sf(*s))
			return true;
	}
	    
	return false;
}

bool RootObject::callForAllSourceManagers(const std::function<bool(SourceManager&)>& sf) const
{
	for(auto s: sourceManagers)
	{
		if(sf(*s))
			return true;
	}
	    
	return false;
}

bool RootObject::callForAllListeners(const std::function<bool(Listener&)>& lf) const
{
	for(auto l: listeners)
	{
		if(lf(*l))
			return true;
	}
	    
	return false;
}

void RootObject::run()
{
    while(threadShouldExit())
    {
        callForAllSourceManagers([](SourceManager& sm){ sm.flushHiPriorityQueue(); return false; });
        Thread::sleep(5);
    }
}
} // dispatch
} // hise