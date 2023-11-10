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


struct SourceManager  : public Queueable, // not queuable
					    public PooledUIUpdater::SimpleTimer
{
	SourceManager(RootObject& r, const HashedCharPtr& typeId);
	~SourceManager() override;

	HashedCharPtr treeId;

	HashedCharPtr getDispatchId() const override { return treeId; }

	/*+ Sends slot changes (values[0] == slotIndex) to the matching listeners. */
	void sendSlotChanges(Source& s, const uint8* values, size_t numValues, DispatchType n=sendNotification);
	
	void timerCallback() override;

	Queue& getListenerQueue(NotificationType n);
	const Queue& getListenerQueue(NotificationType n) const;
	int getNumChildSources() const { return items.getNumAllocated(); }

	void addSource(Source* s);
	void removeSource(Source* s);
	
private:

	Queue asyncQueue;
	Queue deferedSyncEvents;
	Queue asyncListeners;
	Queue syncListeners;
	Queue items;
};




} // dispatch
} // hise