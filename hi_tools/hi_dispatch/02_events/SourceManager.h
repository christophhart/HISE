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

#pragma once

namespace hise {
namespace dispatch {	
using namespace juce;

class SourceManager  : public Suspendable,
					    public PooledUIUpdater::SimpleTimer
{
public:

	SourceManager(RootObject& r, const HashedCharPtr& typeId);
	~SourceManager() override;

	HashedCharPtr treeId;

	HashedCharPtr getDispatchId() const override { return treeId; }

	bool matchesPath(const HashedPath& p) const override
	{
		return p.handler == getDispatchId();
	}
	
	void timerCallback() override;

	size_t getNumChildSources() const { return sources.getNumAllocated(); }

	void addSource(Source* s);
	void removeSource(Source* s);

	void setState(const HashedPath& p, State newState) override;

	void forEachSource(const std::function<void(Source&)>& sf)
	{
		sources.flush([sf](const Queue::FlushArgument& f)
		{
			sf(f.getTypedObject<Source>());
			return true;
		}, Queue::FlushType::KeepData);
	}

	void resetMessageCounter();

	void bumpMessageCounter(bool used)
	{
		auto& v = used ? messageCounter :		  skippedCounter;
		auto b = used ?  messageCounterId.get() : skippedCounterId.get();
		TRACE_COUNTER("dispatch", perfetto::CounterTrack(b), ++v);
	}

	void flush(DispatchType n);

private:

	int messageCounter = 0;
	int skippedCounter = 0;

	StringBuilder messageCounterId, skippedCounterId;
	Queue sources;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceManager);
};

template <typename T> struct SingleValueSourceManager: public SourceManager
{
	SingleValueSourceManager(RootObject& r, const HashedCharPtr& id):
	  SourceManager(r, id)
	{};
};


} // dispatch
} // hise