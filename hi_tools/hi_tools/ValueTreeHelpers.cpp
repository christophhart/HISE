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

bool Helpers::isBetween(IterationType l, IterationType u, IterationType v)
{
	return v >= l && v <= u;
}

bool Helpers::isBackwards(IterationType t)
{
	return t == IterationType::Backwards || t == IterationType::ChildrenFirstBackwards || t == IterationType::OnlyChildrenBackwards;
}

bool Helpers::isRecursive(IterationType t)
{
	return !isBetween(IterationType::OnlyChildren, IterationType::OnlyChildrenBackwards, t);
}

bool Helpers::forEach(ValueTree v, const std::function<bool(ValueTree& v)>& f, IterationType type)
{
	if (isBetween(IterationType::Forward, IterationType::Backwards, type))
	{
		if (f(v))
			return true;
	}

	if (isBackwards(type))
	{
		for (int i = v.getNumChildren() - 1; i >= 0; i--)
		{
			if (isRecursive(type))
			{
				if (forEach(v.getChild(i), f, type))
					return true;
			}
			else
			{
				auto c = v.getChild(i);
				if (f(c))
					return true;
			}
		}
	}
	else
	{
		for (auto c : v)
		{
			if (isRecursive(type))
			{
				if (forEach(c, f, type))
					return true;
			}
			else
			{
				if (f(c))
					return true;
			}
		}
	}

	if (isBetween(IterationType::ChildrenFirst, IterationType::ChildrenFirstBackwards, type))
	{
		if (f(v))
			return true;
	}

	return false;
}

void Helpers::dump(const ValueTree& v)
{
#if JUCE_DEBUG
	auto xml = v.createXml();

	auto xmlText = xml->createDocument("");

	DBG(xmlText);
#endif
}


var Helpers::valueTreeToJSON(const ValueTree& v)
{
	DynamicObject::Ptr p = new DynamicObject();

	for (int i = 0; i < v.getNumProperties(); i++)
	{
		auto id = v.getPropertyName(i);
		p->setProperty(id, v[id]);
	}

	bool hasChildrenWithSameName = v.getNumChildren() > 0;
	auto firstType = v.getChild(0).getType();

	for (auto c : v)
	{
		if (c.getType() != firstType)
		{
			hasChildrenWithSameName = false;
			break;
		}
	}

	Array<var> childList;



	for (auto c : v)
	{
		if (c.getNumChildren() == 0 && c.getNumProperties() == 0)
		{
			p->setProperty(c.getType(), new DynamicObject());
			continue;
		}

		auto jsonChild = valueTreeToJSON(c);

		if (hasChildrenWithSameName)
			childList.add(jsonChild);
		else
			p->setProperty(c.getType(), jsonChild);
	}

	if (hasChildrenWithSameName)
	{
		p->setProperty("ChildId", firstType.toString());
		p->setProperty("Children", var(childList));
	}

	return var(p.get());
}

juce::ValueTree Helpers::jsonToValueTree(var data, const Identifier& typeId, bool isParentData)
{
	if (isParentData)
	{
		data = data.getProperty(typeId, {});
		jassert(data.isObject());
	}

	ValueTree v(typeId);

	if (data.hasProperty("ChildId"))
	{
		Identifier childId(data.getProperty("ChildId", "").toString());

		for (auto& nv : data.getDynamicObject()->getProperties())
		{
			if (nv.name.toString() == "ChildId")
				continue;

			if (nv.name.toString() == "Children")
				continue;

			v.setProperty(nv.name, nv.value, nullptr);
		}

		auto lv = data.getProperty("Children", var());
		if (auto l = lv.getArray())
		{
			for (auto& c : *l)
			{
				v.addChild(jsonToValueTree(c, childId, false), -1, nullptr);
			}
		}
	}
	else
	{
		if (auto dyn = data.getDynamicObject())
		{
			for (const auto& nv : dyn->getProperties())
			{
				if (nv.value.isObject())
				{
					v.addChild(jsonToValueTree(nv.value, nv.name, false), -1, nullptr);
				}
				else if (nv.value.isArray())
				{
					// must not happen
					jassertfalse;
				}
				else
				{
					v.setProperty(nv.name, nv.value, nullptr);
				}
			}
		}
	}

	return v;
}

ValueTree Helpers::findParentWithType(const ValueTree& v, const Identifier& id)
{
	auto p = v.getParent();

	if (!p.isValid())
		return {};

	if (p.getType() == id)
		return p;

	return findParentWithType(p, id);
}

bool Helpers::isLast(const ValueTree& v)
{
	return (v.getParent().getNumChildren() - 1) == getIndexInParent(v);
}

bool Helpers::isParent(const ValueTree& v, const ValueTree& possibleParent)
{
	if (!v.isValid())
		return false;

	if (v == possibleParent)
		return true;

	return isParent(v.getParent(), possibleParent);
}

int Helpers::getIndexInParent(const ValueTree& v)
{
	return v.getParent().indexOf(v);
}

ValueTree Helpers::getRoot(const ValueTree& v)
{
	auto p = v.getParent();

	if (p.isValid())
		return getRoot(p);

	return v;
}

bool Helpers::forEachParent(ValueTree& v, const Function& f)
{
	if (!v.isValid())
		return false;

	if (f(v))
		return true;


	auto p = v.getParent();
	return forEachParent(p, f);
}




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
		if(ids.isEmpty())
		{
			for(int i = 0; i <  v.getNumProperties(); i++)
			{
				auto id = v.getPropertyName(i);
				f(id, v[id]);
			}
		}
		else
		{
			for (auto id : ids)
				f(id, v[id]);
		}
		
		break;
	}
	case AsyncMode::Asynchronously:
	{
		ScopedLock sl(asyncLock);
		changedIds.clear();

		if(ids.isEmpty())
		{
			changedIds.ensureStorageAllocated(v.getNumProperties());

			for(int i = 0; i < v.getNumProperties(); i++)
				changedIds.add(v.getPropertyName(i));
		}
		else
		{
            for(int i = 0; i < v.getNumProperties(); i++)
            {
                auto id = v.getPropertyName(i);
                
                if(ids.contains(id))
                    changedIds.add(id);
            }
		}

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
    {
        jassert(v.hasProperty(id) || (id == Identifier("Coallescated")));
        f(id, v[id]);
    }

	changedIds.clear();
}

void PropertyListener::valueTreePropertyChanged(ValueTree& v_, const Identifier& id)
{
	Identifier id2(id.toString());

	if (v == v_ && (ids.isEmpty() || ids.contains(id2)))
	{
		auto thisValue = v[id];

		if (v.hasProperty(id) && lastValue == thisValue)
		{
			//probably priorised
			return;
		}

		lastValue = thisValue;

		if (auto pb = dynamic_cast<PropertyListener*>(priorisedListener.get()))
		{
			pb->valueTreePropertyChanged(v_, id);
		}

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
		shouldFire = (c == child || child.isAChildOf(c)) && p.isAChildOf(parent);
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

		for (auto c : v)
			cb(c, true);

		break;
	}
		
	case AsyncMode::Asynchronously:
	{
		ScopedLock sl(asyncLock);

		pendingChanges.clear();
		for (auto c : v)
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
		currentParent = p;
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


void ChildListener::valueTreeChildRemoved(ValueTree& p, ValueTree& c, int r)
{
	if (!cb || ((p != v) && !allowCallbacksForChildEvents))
		return;

	

	switch (mode)
	{
	case AsyncMode::Unregistered: break;
	case AsyncMode::Synchronously:
		removeIndex = r;
		currentParent = p;
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

juce::ValueTree ChildListener::getCurrentParent() const
{
	jassert(mode == AsyncMode::Synchronously); 
	return currentParent;
}

int ChildListener::getRemoveIndex() const
{
	jassert(mode == AsyncMode::Synchronously);
	return removeIndex;
}

void forEach(ValueTree& v, const std::function<void(ValueTree&)>& f)
{
	f(v);

	for (auto c : v)
		forEach(c, f);
}

void RecursiveTypedChildListener::sendAddMessageForAllChildren()
{
	if (mode == AsyncMode::Synchronously)
	{
		forEach(v, [this](ValueTree& vt)
		{
			this->currentParent = vt.getParent();
			this->removeIndex = -1;

			if (parentTypes.contains(vt.getType()))
				this->cb(vt, true);
		});
	}
	else
		ChildListener::sendAddMessageForAllChildren();
}

void RecursiveTypedChildListener::valueTreeChildAdded(ValueTree& p, ValueTree& c)
{
	if (!parentTypes.contains(p.getType()))
		return;

	ChildListener::valueTreeChildAdded(p, c);

}

void RecursiveTypedChildListener::valueTreeChildRemoved(ValueTree& p, ValueTree& c, int)
{
	if (!parentTypes.contains(p.getType()))
		return;

	ChildListener::valueTreeChildRemoved(p, c, 0);
}

AnyListener::AnyListener(AsyncMode mode_) :
	Base()
{
	mode = mode_;

	for (int i = 0; i < numCallbackTypes; i++)
		setForwardCallback((CallbackType)i, true);
}

void AnyListener::setMillisecondsBetweenUpdate(int milliSeconds)
{
	if (milliSeconds == 0)
		mode = AsyncMode::Asynchronously;
	else
	{
		mode = AsyncMode::Coallescated;
		milliSecondsBetweenUpdate = milliSeconds;
	}
}

void AnyListener::setEnableLogging(bool shouldLog)
{
	loggingEnabled = shouldLog;
}

void AnyListener::setRootValueTree(const ValueTree& d)
{
	data = d;
	data.addListener(this);

	anythingChanged(lastCallbackType);
}

void AnyListener::setForwardCallback(CallbackType c, bool shouldForward)
{
	forwardCallbacks[c] = shouldForward;
}

void AnyListener::setPropertyCondition(const PropertyConditionFunc& f)
{
	pcf = f;
}

void AnyListener::handleAsyncUpdate()
{
	anythingChanged(lastCallbackType);

	lastCallbackType = Nothing;
}

void AnyListener::valueTreeChildAdded(ValueTree&, ValueTree& c)
{
	if (forwardCallbacks[ChildAdded])
	{
		logIfEnabled(ChildAdded, c, {});
		triggerUpdate(ChildAdded);
	}
}

void AnyListener::valueTreeChildOrderChanged(ValueTree& c, int, int)
{
	if (forwardCallbacks[ChildOrderChanged])
	{
		logIfEnabled(ChildAdded, c, {});
		triggerUpdate(ChildOrderChanged);
	}
}

void AnyListener::valueTreeChildRemoved(ValueTree&, ValueTree& c, int)
{
	if (forwardCallbacks[ChildDeleted])
	{
		logIfEnabled(ChildDeleted, c, {});
		triggerUpdate(ChildDeleted);
	}
}

void AnyListener::valueTreePropertyChanged(ValueTree& v, const Identifier& id)
{
	if (!forwardCallbacks[PropertyChange])
		return;
	
	if (pcf && !pcf(v, id))
		return;
	
	logIfEnabled(PropertyChange, v, id);
	triggerUpdate(PropertyChange);
}

void AnyListener::valueTreeParentChanged(ValueTree&)
{
	if (forwardCallbacks[ValueTreeRedirected]) triggerUpdate(ValueTreeRedirected);
}

void AnyListener::logIfEnabled(CallbackType b, ValueTree& v, const Identifier& id)
{
	if (!loggingEnabled)
		return;

	String s;

	switch (b)
	{
	case CallbackType::ChildAdded:
		s << "Add child " << v.getType(); break;
	case CallbackType::ChildDeleted:
		s << "Remove child " << v.getType(); break;
	case CallbackType::PropertyChange:
		s << "Set property " << id << " for " << v.getType(); break;
	case CallbackType::ValueTreeRedirected:
		s << "redirected " << v.getType(); break;
	default:
		break;
	}

	s << "\n";
	ValueTree copy = v.createCopy();
	copy.removeAllChildren(nullptr);
	auto xml = copy.createXml();
	s << xml->createDocument("", true);
	s << "\n--------------------------------------------------------------------";

	DBG(s);

	
}

void AnyListener::triggerUpdate(CallbackType t)
{
	auto thisCallbackType = jmax(lastCallbackType, t);

	if (lastCallbackType != thisCallbackType)
	{
		lastCallbackType = thisCallbackType;

		if (mode == AsyncMode::Synchronously)
			handleAsyncUpdate();
		else if (mode == AsyncMode::Coallescated)
		{
			startTimer(milliSecondsBetweenUpdate);
		}
		else
			triggerAsyncUpdate();
	}
}

void AnyListener::timerCallback()
{
	handleAsyncUpdate();
	stopTimer();
}

}


LockFreeUpdater::LockFreeUpdater(PooledUIUpdater* updater)
{
	setHandler(updater);
	addChangeListener(this);
}

}


