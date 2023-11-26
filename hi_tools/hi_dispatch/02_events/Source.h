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

// This baseclass will be used by all classes that have a dispatch::Source member
// for handling the events.
// You need to pass in a reference to the owner class into the constructor of a Source
class SourceOwner
{
public:
	virtual ~SourceOwner() {};
};

class SlotSender;
class ListenerQueue;

// just a interface class. 
// Subclass all classes that have a SlotSender from this class
// (the item class will keep a reference to this as member to send)
class Source: public Suspendable
{
public:

	Source(SourceManager& parent_, SourceOwner& owner, const HashedCharPtr& sourceId_ );
	~Source() override;

	HashedCharPtr getDispatchId() const override { return sourceId; }

	SourceManager& getParentSourceManager() noexcept { return parent; }
	const SourceManager& getParentSourceManager() const noexcept { return parent; }

	template <typename T> T& getOwner()
	{
		static_assert(std::is_base_of<SourceOwner, T>(), "not a base of source owner");
		return *dynamic_cast<T*>(&owner);
	}

	template <typename T> const T& getOwner() const
	{
		static_assert(std::is_base_of<SourceOwner, T>(), "not a base of source owner");
		return *dynamic_cast<const T*>(&owner);
	}

	void flushChanges(DispatchType n);

	virtual int getNumSlotSenders() const = 0;

	virtual SlotSender* getSlotSender(uint8 slotSenderIndex) = 0;
	
	ListenerQueue* getListenerQueue(uint8 slotSenderIndex, DispatchType n);

	void setState(const HashedPath& p, State newState) override;

	void forEachListenerQueue(DispatchType n, const std::function<void(uint8, DispatchType, ListenerQueue* q)>& f);

	bool matchesPath(const HashedPath& p) const override
	{
		return parent.matchesPath(p) && p.source == getDispatchId();
	}

protected:

	void setSourceId(HashedCharPtr&& newId)
	{
		sourceId = std::move(newId);
	}

private:

    std::atomic<State> currentState = { State::Running };

	SourceManager& parent;
	HashedCharPtr sourceId;
	SourceOwner& owner;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Source);
};

// defers listChanged calls until done, then send a EventAction PostDefer message
// The ScopedDelayer takes the write lock
// constructor invalidates all indexes
// deconstructor calls signal rebuild, validates all indexes & calls the listeners
class ScopedDelayer
{
public:

	
};

} // dispatch
} // hise