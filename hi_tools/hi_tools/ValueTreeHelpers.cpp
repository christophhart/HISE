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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace hise
{

using namespace juce;

namespace valuetree
{


void PropertyListener::setCallback(ValueTree d, const Array<Identifier>& ids_, AsyncMode asyncMode, const PropertyCallback& f_)
{
	if (v.isValid())
		v.removeListener(this);

	v = d;
	v.addListener(this);

	ids = ids_;
	f = f_;

	mode = asyncMode;

	sendMessageForAllProperties();
}


void PropertyListener::sendMessageForAllProperties()
{
	switch (mode)
	{
	case AsyncMode::Unregistered:
		break;
	case AsyncMode::Synchronously:
	{
		for (auto id : ids)
			f(id, v[id]);
		break;
	}
	case AsyncMode::Asynchronously:
	{
		ScopedLock sl(asyncLock);
		changedIds.clear();
		changedIds.addArray(ids);
		triggerAsyncUpdate();
		break;
	}
	case AsyncMode::Coallescated:
	{
		ScopedLock sl(asyncLock);
		changedIds.clear();
		changedIds.add("Coallescated");
		triggerAsyncUpdate();
		break;
	}
    default:
        break;
    
	}
}


void PropertyListener::handleAsyncUpdate()
{
	ScopedLock sl(asyncLock);

	for (auto id : changedIds)
		f(id, v[id]);

	changedIds.clear();
}

void PropertyListener::valueTreePropertyChanged(ValueTree& v_, const Identifier& id)
{
	if (v == v_ && ids.contains(id))
	{
		switch (mode)
		{
		case AsyncMode::Unregistered:
			break;
		case AsyncMode::Synchronously:
			f(id, v[id]);
			break;
		case AsyncMode::Asynchronously:
		{
			ScopedLock sl(asyncLock);
			changedIds.addIfNotAlreadyThere(id);
			triggerAsyncUpdate();
			break;
		}
			
		case AsyncMode::Coallescated:
		{
			ScopedLock sl(asyncLock);
			changedIds.addIfNotAlreadyThere("Coallescated");
			triggerAsyncUpdate();
			break;
		}
		default:
			break;
		}
	}
}



void RecursivePropertyListener::setCallback(ValueTree parent, const Array<Identifier>& ids_, AsyncMode asyncMode, const RecursivePropertyCallback& f_)
{
	if (v.isValid())
		v.removeListener(this);

	v = parent;
	v.addListener(this);

	ids = ids_;
	f = f_;

	mode = asyncMode;

	// Can't coallescate these...
	jassert(mode != AsyncMode::Coallescated);
}

void RecursivePropertyListener::handleAsyncUpdate()
{
	ScopedLock sl(asyncLock);

	for (auto c : pendingChanges)
		f(c.v, c.id);

	pendingChanges.clear();
}

void RecursivePropertyListener::valueTreePropertyChanged(ValueTree& changedTree, const Identifier& id)
{
	if (!ids.contains(id))
		return;

	switch (mode)
	{
	case AsyncMode::Unregistered:
		break;
	case AsyncMode::Synchronously:
		f(changedTree, id);
		break;
	case AsyncMode::Asynchronously:
	{
		ScopedLock sl(asyncLock);
		pendingChanges.add({ changedTree, id });
		triggerAsyncUpdate();
		break;
	}
	case AsyncMode::Coallescated:
		jassertfalse;
		break;
	case AsyncMode::numAsyncModes:
	default:
		break;
	}
}


RemoveListener::~RemoveListener()
{
	
	cancelPendingUpdate();
	parent.removeListener(this);
}

void RemoveListener::setCallback(ValueTree childToListenTo, AsyncMode asyncMode, bool checkParentsToo, const Callback& c)
{
	if (parent.isValid())
		parent.removeListener(this);

	if (!parent.isValid())
		parent = childToListenTo.getParent();

	WeakReference<RemoveListener> tmp = this;

	auto f = [tmp, childToListenTo, asyncMode, c, checkParentsToo]()
	{
		if (tmp.get() == nullptr)
			return;

		tmp.get()->fireRecursively = checkParentsToo;

		tmp.get()->mode = asyncMode;
		tmp.get()->child = childToListenTo;

		if (checkParentsToo)
			tmp.get()->parent = childToListenTo.getRoot();
		else
			tmp.get()->parent = childToListenTo.getParent();

		
		tmp.get()->parent.addListener(tmp.get());
		tmp.get()->cb = c;
	};

	if (parent.isValid())
	{
		f();
	}
	else
	{
		// It might be possible that the value tree you want to listen
		// has not yet been added to a parent (most likely in case of a coallescated
		// undo action). In this case, we'll execute the registration asynchronously.
		MessageManager::callAsync(f);
	}
}



void RemoveListener::handleAsyncUpdate()
{
	cb(child);
}


void RemoveListener::valueTreeChildRemoved(ValueTree& p, ValueTree& c, int)
{
	bool shouldFire = false;

	if (fireRecursively)
	{
		shouldFire = child.isAChildOf(c) && p.isAChildOf(parent);
	}
	else
		shouldFire = p == parent && c == child;

	if (shouldFire)
	{
		if (mode == AsyncMode::Asynchronously)
			triggerAsyncUpdate();
		else
			cb(c);
	}
}


PropertySyncer::~PropertySyncer()
{
	first.removeListener(this);
	second.removeListener(this);
}

void PropertySyncer::setPropertiesToSync(ValueTree& firstTree, ValueTree& secondTree, Array<Identifier> idsToSync, UndoManager* undoManagerToUse)
{
	if (first.isValid())
		first.removeListener(this);
	if (second.isValid())
		second.removeListener(this);

	first = firstTree;
	second = secondTree;
	first.addListener(this);
	second.addListener(this);
	um = undoManagerToUse;
	syncedIds = idsToSync;

	for (auto id : syncedIds)
	{
		if (first[id] != second[id])
			second.setPropertyExcludingListener(this, id, first[id], undoManagerToUse);
	}
}


void PropertySyncer::valueTreePropertyChanged(ValueTree& v, const Identifier& id)
{
	if (!syncedIds.contains(id))
		return;

	if (v == first)
		second.setPropertyExcludingListener(this, id, first[id], um);
	if (v == second)
		first.setPropertyExcludingListener(this, id, second[id], um);
}


ChildListener::~ChildListener()
{
	cancelPendingUpdate();
	v.removeListener(this);
}


void ChildListener::setCallback(ValueTree treeToListenTo, AsyncMode asyncMode, const ChildChangeCallback& newCallback)
{
	if (v.isValid())
		v.removeListener(this);

	mode = asyncMode;
	v = treeToListenTo;
	v.addListener(this);
	cb = newCallback;

	sendAddMessageForAllChildren();
}


void ChildListener::sendAddMessageForAllChildren()
{
	switch (mode)
	{
	case AsyncMode::Unregistered: break;
	case AsyncMode::Synchronously:
	{
		IterationProtector ip(v);

		for (auto& c : v)
			cb(c, true);

		break;
	}
		
	case AsyncMode::Asynchronously:
	{
		ScopedLock sl(asyncLock);

		pendingChanges.clear();
		for (auto& c : v)
			pendingChanges.addIfNotAlreadyThere({ c, true });

		triggerAsyncUpdate();
		break;
	}
		
	case AsyncMode::Coallescated:
	{
		ScopedLock sl(asyncLock);

		pendingChanges.clear();
		pendingChanges.addIfNotAlreadyThere({ v, true });
		break;
	}
	default:
		break;
	}
}


void ChildListener::handleAsyncUpdate()
{
	ScopedLock sl(asyncLock);

	for (auto& pc : pendingChanges)
	{
		if (pc.v == v)
		{
			jassert(mode == AsyncMode::Coallescated);

			for (auto c : v)
				cb(c, pc.wasAdded);
		}
		else
			cb(pc.v, pc.wasAdded);
	}

	pendingChanges.clear();
}

void ChildListener::valueTreeChildAdded(ValueTree& p, ValueTree& c)
{
	if (!cb || ((p != v) && !allowCallbacksForChildEvents))
		return;

	switch (mode)
	{
	case AsyncMode::Unregistered:
		break;
	case AsyncMode::Synchronously:
		cb(c, true);
		break;
	case AsyncMode::Asynchronously:
	{
		ScopedLock sl(asyncLock);

		pendingChanges.addIfNotAlreadyThere({ c, true });
		triggerAsyncUpdate();
		break;
	}
		
	case valuetree::AsyncMode::Coallescated:
	{
		ScopedLock sl(asyncLock);

		pendingChanges.add({ v, true });
		break;
	}
		
	default:
		break;
	}
}


void ChildListener::valueTreeChildRemoved(ValueTree& p, ValueTree& c, int)
{
	if (!cb || ((p != v) && !allowCallbacksForChildEvents))
		return;

	switch (mode)
	{
	case AsyncMode::Unregistered: break;
	case AsyncMode::Synchronously:
		cb(c, false);
		break;
	case valuetree::AsyncMode::Coallescated: // don't coallescate removals
	case AsyncMode::Asynchronously:
	{
		ScopedLock sl(asyncLock);

		pendingChanges.addIfNotAlreadyThere({ c, false });
		triggerAsyncUpdate();
		break;
	}
    default:
        break;
	}
}



void RecursiveTypedChildListener::valueTreeChildAdded(ValueTree& p, ValueTree& c)
{
	if (p.getType() != parentType)
		return;

	ChildListener::valueTreeChildAdded(p, c);

}

void RecursiveTypedChildListener::valueTreeChildRemoved(ValueTree& p, ValueTree& c, int)
{
	if (p.getType() != parentType)
		return;

	ChildListener::valueTreeChildRemoved(p, c, 0);
}

}


LockFreeUpdater::LockFreeUpdater(PooledUIUpdater* updater)
{
	setHandler(updater);
	addChangeListener(this);
}

}


