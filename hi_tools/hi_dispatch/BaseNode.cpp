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

BaseNode::BaseNode(BaseNode* parent_, GlobalManager* gm_, Category c):
	ObjectUnderGlobalManager(gm_),
	category(c),
	parent(parent_)
{}

Event BaseNode::createEvent(EventAction a) const
{
	Event e(category, a);
	//jassert(globalIndex != 0 || category == Category::AllCategories);
	e.eventManager = globalIndex;
	e.source = sourceIndex;
	e.slot = slotIndex;
	return e;
}

Event BaseNode::createEmptyEvent() const
{
	return createEvent(EventAction::Nothing);
}

manager_int BaseNode::getIndexForCategory() const
{
	return createEmptyEvent().getIndex(category);
}

void BaseNode::updateIndex(Category c, manager_int v)
{
	auto e = createEmptyEvent();
	e.setCategory(c, v);
	setIndexes(e.eventManager, e.source, e.slot);
}

void BaseNode::setIndexes(manager_int gi, manager_int src, manager_int slot)
{
	if(!createEmptyEvent().matchesPath(gi, src, slot))
	{
		globalIndex = gi;
		sourceIndex = src;
		slotIndex = slot;

		if(category < Category::ManagerEvent)
		{
			jassert(sourceIndex != 0);
		}
		if(category < Category::SourceEvent)
		{
			jassert(slotIndex != 0);
		}
	}
}

bool BaseNode::shouldPerform(Event e) const
{
	auto thisPath = createEmptyEvent();

	switch(category)
	{
	case Category::AllCategories:
		{
			jassert(getGlobalManager() == this);
			return true;
		}
	case Category::ManagerEvent:
		{
			jassert(dynamic_cast<const EventSourceManager*>(this) != nullptr);
			return e.eventManager == thisPath.eventManager;
		}
	case Category::SourceEvent:
		{
			jassert(dynamic_cast<const EventSourceBase*>(this) != nullptr);
			return e.eventManager == thisPath.eventManager &&
				   e.source == thisPath.source;
		}
	default: jassertfalse;
	}
}

bool BaseNode::performInternal(Event e)
{
	if(shouldPerform(e))
		return performMatch(e);
	return false;
}

ManagerBase::ScopedDelayer::ScopedDelayer(BaseNode* mb, EventAction at):
	m(mb->getGlobalManager())
{
	e = mb->createEvent(at);
	jassert(e.isDeferEvent() || e.isPreAllEvent());
	mb->getGlobalManager()->sendMessage(e);
}

ManagerBase::ScopedDelayer::~ScopedDelayer()
{
	if(e.isDeferEvent())
	{
		auto pe = e.withEventActionType(EventAction::PostDefer);
		jassert(pe.isPostDeferEvent());
		jassert(pe.getEventCategory() == e.getEventCategory());
		jassert(pe.eventManager == e.eventManager);
		m->sendMessage(pe);
	}
	else if (e.isPreAllEvent())
	{
		auto pe = e.withEventActionType(EventAction::AllChange);
		jassert(pe.isPostDeferEvent());
		jassert(pe.getEventCategory() == e.getEventCategory());
		jassert(pe.eventManager == e.eventManager);
		m->sendMessage(pe);
	}
}

ManagerBase::ManagerBase(BaseNode* parent, GlobalManager* gm_, Category c):
	BaseNode(parent, gm_, c)
{}

bool ManagerBase::performManagerEvent(Event e)
{
	jassert(e.getEventCategory() == category);

	if(e.getEventActionType() == EventAction::Add)
	{
		auto childIndex = e.getIndex(Event::getNextCategory(category));

		Array<BaseNode*> thisChildren;

		{
			SpinLock::ScopedLockType sl(addLock);
			std::swap(thisChildren, childrenToBeAdded);
		}

		{
			SimpleReadWriteLock::ScopedMultiWriteLock sl(iteratorLock);

			for(auto& child: thisChildren)
			{
				jassert(child != nullptr);
				jassert(!children.contains(child));
				children.add(child);
				
			}

			getGlobalManager()->rebuildIndexValuesRecursive();
		}

		for(auto& child: thisChildren)
				childAdded(child);

		return true;
	}
	if(e.getEventActionType() == EventAction::Remove)
	{
		Array<BaseNode*> thisChildren;
		{
			SpinLock::ScopedLockType sl(addLock);
			std::swap(thisChildren, childrenToBeRemoved);
		}

		{
			SimpleReadWriteLock::ScopedMultiWriteLock sl(iteratorLock);

			for(auto& c: thisChildren)
			{
				jassert(children.contains(c));
				childRemoved(c);
				children.remove(children.indexOf(c));
			}

			getGlobalManager()->rebuildIndexValuesRecursive();
		}
		
		return true;
	}
	return false;
}

bool ManagerBase::performMatch(Event e)
{
	if(e.getEventCategory() == category)
	{
		return performManagerEvent(e);
	}
	else
	{
		SimpleReadWriteLock::ScopedReadLock sl(iteratorLock);

		auto eventIndexOnChildLevel = e.getIndex(Event::getNextCategory(category));

		if(auto c = getChildByIndex(eventIndexOnChildLevel))
			return c->performInternal(e);
			
		return false;
	}
}

void ManagerBase::addChild(BaseNode* child)
{
	Event e;

	e = createEvent(EventAction::Add);

	{
		SpinLock::ScopedLockType sl(addLock);
		childrenToBeAdded.add(child);
	}
	
	getGlobalManager()->sendMessage(e);
}

void ManagerBase::removeChild(BaseNode* child)
{
	jassert(children.contains(child));
	auto index = children.indexOf(child) + 1;
	auto e = createEvent(EventAction::Remove);
	child->globalIndex = 9900;
	
	{
		SpinLock::ScopedLockType sl(addLock);
		childrenToBeRemoved.add(child);
	}
	
	getGlobalManager()->sendMessage(e);
}

manager_int ManagerBase::getNumChildren() const
{
	return children.size();
}

BaseNode* ManagerBase::getChildByIndex(manager_int index)
{
	jassert(index != 0);
	return index > 0 ? children[index - 1] : nullptr;
}

const BaseNode* ManagerBase::getChildByIndex(manager_int index) const
{
	jassert(index != 0);
	return index > 0 ? children[index - 1] : nullptr;
}

void ManagerBase::assertIntegrity()
{
#if JUCE_DEBUG

	SimpleReadWriteLock::ScopedReadLock sl(iteratorLock);

	if(auto pc = dynamic_cast<ManagerBase*>(parent))
	{
		jassert(pc->getChildByIndex(getIndexForCategory()) == this);
	}

	for(int i = 1; i <= getNumChildren(); i++)
	{
		auto child = getChildByIndex(i);
		auto childIndex = child->getIndexForCategory();
		//jassert(childIndex == i);
		if(auto c = dynamic_cast<ManagerBase*>(getChildByIndex(i)))
			c->assertIntegrity();
	}
#endif
		
}
} // dispatch
} // hise