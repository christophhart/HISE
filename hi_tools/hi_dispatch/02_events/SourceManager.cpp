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
  Suspendable(r, nullptr),
  SimpleTimer(r.getUpdater()),
  treeId(typeId),
  sources(r, sizeof(void*) * 128)
{
	r.addTypedChild(this);
	jassert(!typeId.isDynamic());
}

SourceManager::~SourceManager()
{
	getRootObject().removeTypedChild(this);
	sources.clear();
	
}

void SourceManager::flushHiPriorityQueue()
{
	
}

void SourceManager::timerCallback()
{
	flush(DispatchType::sendNotificationAsyncHiPriority);
	flush(DispatchType::sendNotificationAsync);
}

void SourceManager::addSource(Source* s)
{
	sources.push(s, EventType::SourcePtr, nullptr, 0);
}

void SourceManager::removeSource(Source* s)
{
	sources.removeAllMatches(s);
}

void SourceManager::setState(const HashedPath& p, State newState)
{
	if(!matchesPath(p))
		return;

	sources.flush([newState, p](const Queue::FlushArgument& f)
	{
		f.getTypedObject<Source>().setState(p, newState);
		return true;
	}, Queue::FlushType::KeepData);
}

void SourceManager::flush(DispatchType n)
{
	if(getStateFromParent() == State::Running)
	{
		TRACE_FLUSH(getDispatchId());
		
		sources.flush([n](const Queue::FlushArgument& f)
		{
			auto& typed = f.getTypedObject<Source>();

			typed.flushChanges(n);

			return true;
		}, Queue::FlushType::KeepData);
	}

	
}
} // dispatch
} // hise