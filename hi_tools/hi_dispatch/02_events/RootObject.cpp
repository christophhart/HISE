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
    childObjects.ensureStorageAllocated(8192);
}

RootObject::~RootObject()
{
    jassert(childObjects.isEmpty());
}

void RootObject::addChild(Child* c)
{
    childObjects.add(c);
}

void RootObject::removeChild(Child* c)
{
    int indexInRoot = childObjects.indexOf(c);
    childObjects.remove(indexInRoot);
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

void RootObject::setState(State newState)
{
    if(newState != currentState)
    {
        currentState = newState;
        callForAllQueues([newState](Queue& q){ q.setState(newState); return false; });
    }
}

bool RootObject::callForAllQueues(const std::function<bool(Queue&)>& qf)
{
    for(auto c: childObjects)
    {
        if(auto q = dynamic_cast<Queue*>(c))
        {
            if(qf(*q))
                return true;
        }
    }
    
    return false;
}

} // dispatch
} // hise