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

Source::Source(SourceManager& parent_, SourceOwner& owner_, const HashedCharPtr& sourceId_):
	Suspendable(parent_.getRootObject(), &parent_),
	parent(parent_),
	owner(owner_),
	sourceId(sourceId_)
{
	getRootObject().addTypedChild(this);
	parent.addSource(this);
}

Source::~Source()
{
	getRootObject().removeTypedChild(this);
	parent.removeSource(this);
	cleared();
}




void Source::flushChanges(DispatchType n)
{
    

    
    
	//TRACE_FLUSH(getDispatchId());

	if(currentState != State::Running)
		return;

    if(n == DispatchType::sendNotificationAsyncHiPriority)
    {
        StringBuilder b;
        b << "flush " << getDispatchId();
        TRACE_EVENT_BEGIN("dispatch", DYNAMIC_STRING_BUILDER(b));
    }
    
	for(int i = 0; i < getNumSlotSenders(); i++)
	{
		getSlotSender(i)->flush(n);
	}
    
    if(n == DispatchType::sendNotificationAsyncHiPriority)
    {
        TRACE_EVENT_END("dispatch");
    }
}

ListenerQueue* Source::getListenerQueue(uint8 slotSenderIndex, DispatchType n)
{
	return getSlotSender(slotSenderIndex)->getListenerQueue(n);
}

void Source::setState(const HashedPath& p, State newState)
{
	if(!matchesPath(p))
		return;
	
	currentState = newState;

	forEachListenerQueue(DispatchType::sendNotification, [p, newState, this](uint8 s, DispatchType n, ListenerQueue* q)
	{
		auto sender = this->getSlotSender(s);

		if(sender->matchesPath(p, n))
		{
			q->setQueueState(newState);

			if(newState == State::Running && n == sendNotificationSync)
			{
				sender->flush(n);
			}
		}
	});
}

void Source::forEachListenerQueue(DispatchType n, const std::function<void(uint8, DispatchType, ListenerQueue* q)>& f)
{
	for(int i = 0; i < getNumSlotSenders(); i++)
	{
		if (n != DispatchType::sendNotification)
		{
			f(i, n, getListenerQueue(i, n));
			continue;
		}
		else
		{
			f(i, DispatchType::sendNotificationSync, getListenerQueue(i, DispatchType::sendNotificationSync));
			f(i, DispatchType::sendNotificationAsync, getListenerQueue(i, DispatchType::sendNotificationAsync));
			f(i, DispatchType::sendNotificationAsyncHiPriority, getListenerQueue(i, DispatchType::sendNotificationAsyncHiPriority));
		}
	}
}
} // dispatch
} // hise
