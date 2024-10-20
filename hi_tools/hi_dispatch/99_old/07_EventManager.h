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

// SecondLevel manager, contains EventSources that have slots
struct EventSourceManager: public ManagerBase
{
	EventSourceManager(GlobalManager* gm):
	  ManagerBase(gm, gm, Category::ManagerEvent)
	{
		
	};

	~EventSourceManager()
	{
		// CALL CLEANUP() IN YOUR CONSTRUCTOR!!!!
		jassert(deinitialised);
	}

	// call this in your constructor
	void init()
	{
		getGlobalManager()->addChild(this);
	}

	// Call this in your destructor
	void cleanup()
	{
		getGlobalManager()->removeChild(this);
		deinitialised = true;
	}

	
private:
	
	bool deinitialised = false;
};

struct Processor;

struct EventSourceBase: public BaseNode
{
	EventSourceBase(BaseNode* parent):
	  BaseNode(parent, parent->getGlobalManager(), Category::SourceEvent)
	{}

	~EventSourceBase()
	{
		// CALL CLEANUP() IN YOUR CONSTRUCTOR!!!!
		jassert(deinitialised);
	}

	// call this in your constructor
	void init()
	{
		dynamic_cast<ManagerBase*>(parent)->addChild(this);
	}

	// Call this in your destructor
	void cleanup()
	{
		globalIndex = 9900;
		dynamic_cast<ManagerBase*>(parent)->removeChild(this);
		deinitialised = true;
	}

	virtual bool performSendEvent(Event e) = 0;

	void sendChangeMessage(manager_int slotIndex)
	{
		auto e = createEvent(EventAction::Change);
		e.slot = slotIndex;
		getGlobalManager()->sendMessage(e);
	}

	bool recursion = false;

	bool performMatch(Event e) final
	{
		if(e.isChangeEvent() || e.isAnyDelayEvent())
		{
			performSendEvent(e);
			return true;
		}
		
		return false;
	}

private:

	bool deinitialised = false;
};

template <typename T> struct TypedSourceManager: public EventSourceManager,
												 public Queue<T>::Resolver
{
	using ListenerType = typename Sender<T>::Listener;
	using ChildListenerType = typename Sender<BaseNode>::Listener;

	struct ChildResolver: public Queue<BaseNode>::Resolver
	{
		ChildResolver(TypedSourceManager& parent_):
		  parent(parent_)
		{};

		BaseNode* resolve(manager_int idx) override { return parent.getChildByIndex(idx);}
		TypedSourceManager& parent;

		manager_int getNumEventSources() const override
		{
			return parent.getNumChildren();
		}
		
		virtual manager_int getNumSlots(manager_int index) const
		{
			if(auto ep = dynamic_cast<TypedSource*>(parent.getChildByIndex(index)))
			{
				return ep->obj.getNumSlots();
			}

			return 0;
		}

	} childResolver;

	struct TypedSource: public EventSourceBase,
						public Queue<T>::Resolver
	{
		TypedSource(TypedSourceManager* parent, T& p):
		  EventSourceBase(parent),
		  obj(p),
		  tp(*parent),
		  sender(this, parent->getGlobalManager(), category)
		{
			init();
		};
		
		~TypedSource()
		{
			sender.cleanup();
			cleanup();
		}

		bool performSendEvent(Event e) final override
		{
			auto parentResult = tp.sender.performEvent(e);

			// set the category to one because the listener only has a single source
			jassert(sender.entries.size() == 1);
			e.setCategory(Category::SourceEvent, 1);

			auto thisResult = sender.performEvent(e);
			return parentResult && thisResult;
		}

		void setLogger(LoggerBase* l)
		{
			sender.currentLogger = l;
		}

		
		SpinLock addLock;
		Array<ListenerType*> pendingListenersToAdd;
		Array<ListenerType*> pendingListenersToRemove;

		void flushListenerChanges(Event e)
		{
			Array<ListenerType*> newList;

			{
				SpinLock::ScopedLockType sl(addLock);
				std::swap(newList, e.isAnyAddEvent() ? pendingListenersToAdd : pendingListenersToRemove);
			}

			if(e.isAnyAddEvent())
				sender.addListener(newList);
			else
				sender.removeListener(newList);
		}

		void addListener(typename Sender<T>::Listener* l)
		{
			jassert(globalIndex != 9900);

			{
				SpinLock::ScopedLockType sl(addLock);
				pendingListenersToAdd.add(l);
			}
			
			auto e = createEvent(EventAction::ListenerAdd);
			getGlobalManager()->sendMessage(e);
		}

		void removeListener(typename Sender<T>::Listener* l)
		{
			if(globalIndex == 9900)
				return;

			auto e = createEvent(EventAction::ListenerRem);

			if(e.isEmpty())
				return;

			if(sender.isEmptyList(l))
				return;

			{
				SpinLock::ScopedLockType sl(addLock);
				pendingListenersToRemove.add(l);
			}

			

			getGlobalManager()->sendMessage(e);
		}

		/** Overwrite this and return the object for the given index. */
		virtual T* resolve(manager_int) { return &obj; }

		/** Overwrite this and return the number of event sources to use. */
		virtual manager_int getNumEventSources() const override { return 1; }
		/** Overwrite this and return the number of event slots for the given event source. */
		virtual manager_int getNumSlots(manager_int) const override { return obj.getNumSlots(); }

		TypedSourceManager& tp;
		T& obj;
		Sender<T> sender;

		JUCE_DECLARE_WEAK_REFERENCEABLE(TypedSource);
	};

	TypedSourceManager(GlobalManager* gm):
	  EventSourceManager(gm),
	  childResolver(*this),
	  sender(this, gm, category),
	  childSender(&childResolver, gm, category)
	{
		init();
	}

	~TypedSourceManager()
	{
		cleanup();
	}

	bool callForEachChild(const std::function<bool(TypedSource&)>& f)
	{
		SimpleReadWriteLock::ScopedReadLock sl(iteratorLock);

		for(int i = 0; i < getNumChildren(); i++)
		{
			auto b = getChildByIndex(i+1);
			if(auto t = dynamic_cast<TypedSource*>(b))
			{
				if(f(*t))
					return true;
			}
		}

		return false;
	}

	

	bool performMatch(Event e) override
	{
		if(EventSourceManager::performMatch(e))
			return true;

		if(e.isListenerEvent())
		{
			flushListenerChanges(e);
			callForEachChild([&](TypedSource& t) { t.flushListenerChanges(e); return false; });
		}

		if(e.isFlushAsync())
		{
			sender.flushAsync();
			childSender.flushAsync();
			callForEachChild([&](TypedSource& t) { t.sender.flushAsync(); return false; });
		}

		callForEachChild([&](TypedSource& t) { t.sender.performEvent(e); return false; });
		return sender.performEvent(e);
	}

	manager_int getNumSlots(manager_int index) const override
	{
		if(auto t = dynamic_cast<const TypedSource*>(getChildByIndex(index+1)))
			return t->obj.getNumSlots();

		return 0;
	}

	manager_int getNumEventSources() const override
	{
		return getNumChildren();
	}

	void childAdded(BaseNode* b) override
	{
		getGlobalManager()->assertIntegrity();
		jassert(b->category == Category::SourceEvent);
		if(auto ep = dynamic_cast<TypedSource*>(b))
		{
			auto idx = b->sourceIndex;
			auto numSlots = ep->obj.getNumSlots();
			sender.rebuildQueue();
			ep->sender.rebuildQueue();
			childSender.push(this->globalIndex, idx);
		}
	}

	void childRemoved(BaseNode* b) override
	{
		getGlobalManager()->assertIntegrity();
		jassert(b->category == Category::SourceEvent);
		sender.rebuildQueue();
		callForEachChild([](TypedSource& t){ t.sender.rebuildQueue(); return false; });
		childSender.push(this->globalIndex, b->sourceIndex);
	}

	SpinLock addLock;
	Array<ListenerType*> pendingListenersToAdd;
	Array<ListenerType*> pendingListenersToRemove;
	Array<ChildListenerType*> pendingChildListenersToAdd;
	Array<ChildListenerType*> pendingChildListenersToRemove;

	void addListener(ListenerType* l)
	{
		{
			SpinLock::ScopedLockType sl(addLock);
			pendingListenersToAdd.add(l);
		}
		
		auto e = createEvent(EventAction::ListenerAdd);
		getGlobalManager()->sendMessage(e);
	}

	void removeListener(ListenerType* l)
	{
		{
			SpinLock::ScopedLockType sl(addLock);
			pendingListenersToRemove.add(l);
		}

		auto e = createEvent(EventAction::ListenerRem);
		getGlobalManager()->sendMessage(e);
	}

	void addChildListener(Sender<BaseNode>::Listener* l)
	{
		{
			SpinLock::ScopedLockType sl(addLock);
			pendingChildListenersToAdd.add(l);
		}

		auto e = createEvent(EventAction::ListenerRem);
		getGlobalManager()->sendMessage(e);
	}

	void removeChildListener(ChildListenerType* l)
	{
		{
			SpinLock::ScopedLockType sl(addLock);
			pendingChildListenersToRemove.add(l);
		}

		auto e = createEvent(EventAction::ListenerRem);
		getGlobalManager()->sendMessage(e);
	}

	void flushListenerChanges(Event e)
	{
		jassert(e.isListenerEvent());

		auto shouldAdd = e.isAnyAddEvent();

		Array<ListenerType*> newList;
		Array<ChildListenerType*> newChildList;

		{
			SpinLock::ScopedLockType sl(addLock);
			std::swap(newList, shouldAdd ? pendingListenersToAdd : pendingListenersToRemove);
			std::swap(newChildList, shouldAdd ? pendingChildListenersToAdd : pendingChildListenersToRemove);
		}

		if(shouldAdd)
			sender.addListener(newList);
		else
			sender.removeListener(newList);

		if(shouldAdd)
			childSender.addListener(newChildList);
		else
			childSender.removeListener(newChildList);
	}

	T* resolve(manager_int index) override
	{
		if(auto s = getChildByIndex(index))
		{
			if(auto s2 = dynamic_cast<TypedSource*>(s))
				return &s2->obj;
		}
		return nullptr;
	}

	Sender<T> sender;
	Sender<BaseNode> childSender;

	JUCE_DECLARE_WEAK_REFERENCEABLE(TypedSourceManager);
};

} // dispatch
} // hise