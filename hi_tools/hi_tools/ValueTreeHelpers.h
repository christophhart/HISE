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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace hise
{
using namespace juce;

namespace valuetree
{

struct Helpers
{
	using Function = std::function<bool(ValueTree&)>;

	static bool foreach(ValueTree v, const Function& f)
	{
		if (f(v))
			return true;

		for (auto c : v)
		{
			if (foreach(c, f))
				return true;
		}

		return false;
	}
};

enum class AsyncMode
{
	Unregistered,
	Synchronously, /// will be exectued synchronously
	Asynchronously, /// will be executed asynchronously for all changed properties
	Coallescated, /// will be fired once for all properties. The ID will be "Coallescated" so you don't try to actually use an ID
	numAsyncModes
};

class Base: public ValueTree::Listener,
			public AsyncUpdater
{
public:

	Base()
	{};

	virtual ~Base() {};


protected:

	// Grab this lock whenever you push or process an async queue
	CriticalSection asyncLock;

	AsyncMode mode = AsyncMode::Unregistered;

	void valueTreeChildAdded(ValueTree&, ValueTree&) override {}
	void valueTreeChildOrderChanged(ValueTree& , int , int ) override { }
	void valueTreeChildRemoved(ValueTree&, ValueTree&, int) override { }
	void valueTreePropertyChanged(ValueTree&, const Identifier&) override {};
	void valueTreeParentChanged(ValueTree&) override {}
};

/** This class fires the given callback whenever the property changes. Can be used as member object
	instead of deriving. */
struct PropertyListener : public Base
{
	using PropertyCallback = std::function<void(Identifier, var)>;

	PropertyListener():
		f({})
	{}

	~PropertyListener()
	{
		cancelPendingUpdate();
		v.removeListener(this);
	}

	void setCallback(ValueTree d, const Array<Identifier>& ids_, AsyncMode asyncMode, const PropertyCallback& f_);

	void sendMessageForAllProperties();

	bool isRegisteredTo(const ValueTree& t) const
	{
		return v == t;
	}

private:

	void handleAsyncUpdate();

	void valueTreePropertyChanged(ValueTree& v_, const Identifier& id_) override;

	PropertyCallback f;

	ValueTree v;
	Array<Identifier> ids;
	Array<Identifier> changedIds;
	var lastValue;
};


struct RecursivePropertyListener : public Base
{

	~RecursivePropertyListener()
	{
		cancelPendingUpdate();
		v.removeListener(this);
	}

	using RecursivePropertyCallback = std::function<void(ValueTree, Identifier)>;

	void setCallback(ValueTree parent, const Array<Identifier>& ids_, AsyncMode asyncMode, const RecursivePropertyCallback& f_);

private:

	void handleAsyncUpdate();

	void valueTreePropertyChanged(ValueTree& v, const Identifier&) override;

	ValueTree v;
	RecursivePropertyCallback f;
	Array<Identifier> ids;

	struct Change
	{
		bool operator==(const Change& other) const
		{
			return v == other.v && id == other.id;
		}

		ValueTree v;
		Identifier id;
	};

	Array<Change> pendingChanges;

};

/** Register it and give it a callback that will be fired when the child is being removed from its parent. */
struct RemoveListener : public Base
{
	using Callback = std::function<void(ValueTree& removedChild)>;

	~RemoveListener();

	/** Set a callback that will be fired when the given child is removed. 
		
		If checkParentsToo is true, then the callback will also fire if 
		one of the parent nodes of this node was removed from its parent. 
	*/
	void setCallback(ValueTree childToListenTo, AsyncMode asyncMode, bool checkParentsToo, const Callback& c);

private:

	bool fireRecursively = false;

	Callback cb;

	void handleAsyncUpdate() override;
	void valueTreeChildRemoved(ValueTree& p, ValueTree& c, int) override;

	ValueTree parent;
	ValueTree child;

	JUCE_DECLARE_WEAK_REFERENCEABLE(RemoveListener);
};


/** Syncs two properties with each other. */
struct PropertySyncer : private ValueTree::Listener
{
	~PropertySyncer();

	/** Syncs the properties of the two value trees.

		It also copies the property values from the first ValueTree to the second.
	*/
	void setPropertiesToSync(ValueTree& firstTree,
		ValueTree& secondTree,
		Array<Identifier> idsToSync,
		UndoManager* undoManagerToUse);

private:

	Array<Identifier> syncedIds;

	void valueTreeChildAdded(ValueTree& , ValueTree& ) override { }
	void valueTreeChildRemoved(ValueTree& , ValueTree& , int) override {}
	void valueTreeChildOrderChanged(ValueTree&, int, int) override { }
	void valueTreePropertyChanged(ValueTree& v, const Identifier& id) override;
	void valueTreeParentChanged(ValueTree&) override {}

	UndoManager* um = nullptr;
	ValueTree first;
	ValueTree second;
};




struct ChildListener : public Base
{
	/** Callback when a child was added / removed. The second parameter is true if its added. */
	using ChildChangeCallback = std::function<void(ValueTree, bool)>;

	~ChildListener();

	/** Register a ValueTree with a callback that will be fired when a child was added / removed.
		Will also send a initial message to all existing children. */
	void setCallback(ValueTree treeToListenTo, AsyncMode asyncMode, const ChildChangeCallback& newCallback);

	/** Sends a message for all children of the parent. */
	void sendAddMessageForAllChildren();

	void forwardCallbacksForChildEvents(bool shouldFireCallbacksForChildEvents)
	{
		allowCallbacksForChildEvents = shouldFireCallbacksForChildEvents;
	}

protected:

	bool allowCallbacksForChildEvents = false;

	struct Change
	{
		bool operator==(const Change& other) const
		{
			return v == other.v && wasAdded == other.wasAdded;
		}

		ValueTree v; 
		bool wasAdded;
	};

	void handleAsyncUpdate() override;

	Array<Change> pendingChanges;

	void valueTreeChildAdded(ValueTree& p, ValueTree& c) override;
	void valueTreeChildRemoved(ValueTree& p, ValueTree& c, int) override;

	ValueTree v;
	ChildChangeCallback cb;
};

/** A listener that watches all children of the root tree with the given ID for child changes. */
struct RecursiveTypedChildListener : public ChildListener
{
	RecursiveTypedChildListener()
	{
		forwardCallbacksForChildEvents(true);
	}

	void setTypeToWatch(Identifier newParentType)
	{
		parentType = newParentType;
	}

private:

	Identifier parentType;

	void valueTreeChildAdded(ValueTree& p, ValueTree& c) override;
	void valueTreeChildRemoved(ValueTree& p, ValueTree& c, int) override;
};


}


class LockFreeUpdater : private SafeChangeBroadcaster,
						private SafeChangeListener
{
public:

	using Function = std::function<void(void)>;

	LockFreeUpdater(PooledUIUpdater* updater);

	~LockFreeUpdater()
	{
		removeChangeListener(this);
	}

	void triggerUpdateWithLambda()
	{
		if(f)
			sendPooledChangeMessage();
	}

	void setFunction(const Function& newFunction) { f = newFunction; }

private:

	void changeListenerCallback(SafeChangeBroadcaster *) override
	{
		if(f)
			f();
	}

	Function f;
};







}
