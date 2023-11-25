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
  treeId(typeId)
{
	messageCounterId << typeId << "messages";
	skippedCounterId << "skipped " << typeId << "messages";

	r.addTypedChild(this);
	jassert(!typeId.isDynamic());
}

SourceManager::~SourceManager()
{
	resetMessageCounter();
	getRootObject().removeTypedChild(this);
	cleared();
	sources.clear();
 }

void SourceManager::resetMessageCounter()
{
	messageCounter = 0;
	skippedCounter = 0;

	TRACE_COUNTER("dispatch", perfetto::CounterTrack(messageCounterId.get()), 0);
	TRACE_COUNTER("dispatch", perfetto::CounterTrack(skippedCounterId.get()), 0);
}

void SourceManager::timerCallback()
{
	resetMessageCounter();

	flush(DispatchType::sendNotificationAsyncHiPriority);
	flush(DispatchType::sendNotificationAsync);
}

void SourceManager::addSource(Source* s)
{
	ScopedWriteLock sl(sourceLock);
	sources.add(s);
	
}

void SourceManager::removeSource(Source* s)
{
	ScopedWriteLock sl(sourceLock);
	sources.removeFirstMatchingValue(s);
}

void SourceManager::setState(const HashedPath& p, State newState)
{
	if(!matchesPath(p))
		return;

	forEachSource<Behaviour::AlwaysRun>([newState, p](Source& s)
	{
		s.setState(p, newState);
	});
}

void SourceManager::flush(DispatchType n)
{
	if(getStateFromParent() == State::Running)
	{
		//TRACE_FLUSH(getDispatchId());

		forEachSource<Behaviour::BreakIfPaused>([n](Source& s)
		{
			s.flushChanges(n);
		});
	}
}
} // dispatch
} // hise