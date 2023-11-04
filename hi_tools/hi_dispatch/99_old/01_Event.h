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

enum class Category: uint16
{
	Nothing = 0,
	SlotEvent		= 0x10,
	SourceEvent		= 0x20,
	ManagerEvent	= 0x40,
	AllCategories   = 0xF0
};

enum class EventAction: uint16
{
	Nothing     = 0x00,
	Change	    = 0x01,
	FlushAsync  = 0x02,
	Add	        = 0x03,
	Remove	    = 0x04,
	ListenerAdd = 0x05,
	ListenerRem = 0x06,
	Defer	    = 0x07,
	PostDefer   = 0x08,
	PreAll      = 0x09,
	AllChange   = 0x0A,
	AllEvents   = 0x0F,
	numEventActions
};

#define COMBINE(a, b) (uint16)a | (uint16)b

enum class EventType: uint16 // => Event.h
{
	Nothing = 0,
	// Event Slot events		// 0x10
	EventSlotValueChanged		 = COMBINE(Category::SlotEvent, EventAction::Change),		 // 0x11,
	EventSlotFlushAsync			 = COMBINE(Category::SlotEvent, EventAction::FlushAsync),	 // 0x12,
	DeferEventSlotChanges		 = COMBINE(Category::SlotEvent, EventAction::Defer),		 // 0x16,
	EventSlotListenerAdded       = COMBINE(Category::SlotEvent, EventAction::ListenerAdd),   // 0x15,
	EventSlotListenerRemoved     = COMBINE(Category::SlotEvent, EventAction::ListenerRem),   // 0x16,
	EventSlotChangesDeferred	 = COMBINE(Category::SlotEvent, EventAction::PostDefer),	 // 0x17,
	PreAllEventSlotsChanged		 = COMBINE(Category::SlotEvent, EventAction::PreAll),		 // 0x18,
	AllEventSlotsChanged		 = COMBINE(Category::SlotEvent, EventAction::AllChange),	 // 0x19,
	// Event Source events		// 0x20
	EventSourceChange			 = COMBINE(Category::SourceEvent, EventAction::Change),		 // 0x21,
	EventSourceAdded			 = COMBINE(Category::SourceEvent, EventAction::Add),		 // 0x22,
	EventSourceFlushAsync		 = COMBINE(Category::SourceEvent, EventAction::FlushAsync),	 // 0x22,
	EventSourceRemoved			 = COMBINE(Category::SourceEvent, EventAction::Remove),		 // 0x25,
	EventSourceListenerAdded     = COMBINE(Category::SourceEvent, EventAction::ListenerAdd), // 0x25,
	EventSourceListenerRemoved   = COMBINE(Category::SourceEvent, EventAction::ListenerRem), // 0x25,
	DeferEventSourceChanges		 = COMBINE(Category::SourceEvent, EventAction::Defer),		 // 0x26,
	EventSourceChangesDeferred	 = COMBINE(Category::SourceEvent, EventAction::PostDefer),	 // 0x27,
	PreAllEventSourcesChanged	 = COMBINE(Category::SourceEvent, EventAction::PreAll),		 // 0x28,
	AllEventSourcesChanged		 = COMBINE(Category::SourceEvent, EventAction::AllChange),	 // 0x29
	// EventManager events	    // 0x40
	EventManagerChange			 = COMBINE(Category::ManagerEvent, EventAction::Change),	 // 0x42
	EventManagerAdded			 = COMBINE(Category::ManagerEvent, EventAction::Add),		 // 0x41
	EventManagerFlushAsync		 = COMBINE(Category::ManagerEvent, EventAction::FlushAsync),	 // 0x23,
	EventManagerRemoved			 = COMBINE(Category::ManagerEvent, EventAction::Remove),	 // 0x45
	EventManagerListenerAdded    = COMBINE(Category::ManagerEvent, EventAction::ListenerAdd), // 0x25,
	EventManagerListenerRemoved  = COMBINE(Category::ManagerEvent, EventAction::ListenerRem), // 0x25,
	DeferEventManagerChanges	 = COMBINE(Category::ManagerEvent, EventAction::Defer),		 // 0x46
	EventManagerChangesDeferred  = COMBINE(Category::ManagerEvent, EventAction::PostDefer),  // 0x47
	PreAllEventManagersChanged	 = COMBINE(Category::ManagerEvent, EventAction::PreAll),	 // 0x48,
	AllEventManagerChanged		 = COMBINE(Category::ManagerEvent, EventAction::AllChange),  // 0x49
	numEventTypes
};	

#undef COMBINE

using manager_int = uint16;
using source_int = uint16;
using slot_int = uint16;

#define EVENT_BITWISE_AND(x) (uint16)eventType & (uint16)x
#define EVENT_BITWISE_OR(x) (uint16)eventType | (uint16)x
#define EVENT_COMBINE(x, y) (EventType)((uint16)x | (uint16)y)


struct Event
{
	Event() = default;

	Event(Category category, EventAction action):
	  eventType(EVENT_COMBINE(category, action))
	{};
	  
	using Type = EventType;
	static constexpr int ByteSize = sizeof(manager_int) + sizeof(Type) + sizeof(source_int) + sizeof(slot_int);

	// action tests
	constexpr bool isChangeEvent()    const noexcept { return getEventActionType() == EventAction::Change; }
	constexpr bool isAdd()			  const noexcept { return getEventActionType() == EventAction::Add;}
	constexpr bool isFlushAsync()	  const noexcept { return getEventActionType() == EventAction::FlushAsync;}
	constexpr bool isRemove()		  const noexcept { return getEventActionType() == EventAction::Remove;}
	constexpr bool isDeferEvent()     const noexcept { return getEventActionType() == EventAction::Defer; }
	constexpr bool isPostDeferEvent() const noexcept { return getEventActionType() == EventAction::PostDefer; }
	constexpr bool isPreAllEvent()    const noexcept { return getEventActionType() == EventAction::PreAll; }
	constexpr bool isAllEvent()       const noexcept { return getEventActionType() == EventAction::AllChange; }
	constexpr bool isAnyDelayEvent()  const noexcept { return isAllEvent() || isPreAllEvent() || isDeferEvent() || isPostDeferEvent(); }
	constexpr bool isEmpty()		  const noexcept { return eventManager == 0 && source == 0 && slot == 0; }

	// category tests
	constexpr bool isCategory(Category c) const noexcept { return EVENT_BITWISE_AND(c); }
	constexpr bool isSlotEvent()	    const noexcept { return isCategory(Category::SlotEvent); };
	constexpr bool isSourceEvent()    const noexcept { return isCategory(Category::SourceEvent); };
	constexpr bool isManagerEvent()   const noexcept { return isCategory(Category::ManagerEvent); };
	constexpr bool isListenerEvent() const noexcept { return getEventActionType() == EventAction::ListenerAdd || getEventActionType() == EventAction::ListenerRem; }
	constexpr bool isAnyAddEvent() const noexcept { return isAdd() || getEventActionType() == EventAction::ListenerAdd; }
	constexpr bool isAnyPreEvent() const noexcept { return isPreAllEvent() || isDeferEvent(); }

	constexpr Event asPostEvent() const noexcept
	{
		jassert(isAnyDelayEvent());

		if(isDeferEvent())
			return withEventActionType(EventAction::PostDefer);
		if(isAllEvent())
			return withEventActionType(EventAction::AllChange);
	}

	constexpr static Category getPrevCategory(Category C)
	{
		if (C == Category::SlotEvent)
			return Category::SourceEvent;
		if (C == Category::SourceEvent)
			return Category::ManagerEvent;

		return Category::AllCategories;
	}

	template <Category C> constexpr static Category getPrevCategory()
	{
		if constexpr (C == Category::SlotEvent)
			return Category::SourceEvent;
		if constexpr (C == Category::SourceEvent)
			return Category::ManagerEvent;
		return Category::AllCategories;
	}

	template <Category C> constexpr static Category getNextCategory()
	{
		if constexpr (C == Category::AllCategories)
			return Category::ManagerEvent;
		if constexpr (C == Category::ManagerEvent)
			return Category::SourceEvent;
		if constexpr (C == Category::SourceEvent)
			return Category::SlotEvent;
		
		return Category::Nothing;
	}

	static Category getNextCategory(Category C)
	{
		if (C == Category::AllCategories)
			return Category::ManagerEvent;
		if (C == Category::ManagerEvent)
			return Category::SourceEvent;
		if (C == Category::SourceEvent)
			return Category::SlotEvent;
		
		return Category::Nothing;
	}
	
	void setCategory(Category c, uint16 newValue)
	{
		switch(c)
		{
		case Category::ManagerEvent:
			eventManager = newValue; break;
		case Category::SourceEvent:
			source = newValue; break;
		case Category::SlotEvent:
			slot = newValue; break;
		default: break;
		}
	}

	template <Category C> bool matchesCategory() const { return isCategory(C); }

	EventType   getEventType()		 const noexcept { return eventType; }
	Category	getEventCategory()   const noexcept { return static_cast<Category>((EVENT_BITWISE_AND(Category::AllCategories))); }
	constexpr EventAction getEventActionType() const noexcept { return static_cast<EventAction>((EVENT_BITWISE_AND(EventAction::AllEvents))); }

	static String getString(Type t);

	/** Checks whether this event should supercede the other event. This uses this order for the categories (more important first):
	 *
	 *  - ManagerEvents (adding / removing managers)
	 *  - EventSource events (adding / removing event sources)
	 *	- EventListener events (adding / removing event listeners)
	 *	- Slot events (values are changed).
	 *
	 *	and this order for the Event Actions (more important first):
	 *
	 *	- All event
	 *	- PostDefer
	 *	- Defer
	 *  - Add / Remove / Insert / Swap
	 *	- Change
	 */
	bool isMoreImportantThan(const Event& other) const noexcept  { return eventType > other.eventType; }
	bool isSameCategory(const Event& other) const noexcept		 { return getEventCategory() == other.getEventCategory(); }
	bool isSameCategoryButMoreImportantThan(const Event& other) const noexcept { return isSameCategory(other) && isMoreImportantThan(other); };

	bool sameManager(const Event& other) const noexcept			 { return eventManager == other.eventManager; };
	bool sameManagerAndSource(const Event& other) const noexcept { return sameManager(other) && source == other.source; };

	bool matchesPath(manager_int g, manager_int src, manager_int sl) const noexcept { return eventManager == g && source == src && sl == slot; }

	bool isSubCategory(Category c) const
	{
		auto category = getEventCategory();
		return c < category;
	}

	manager_int getIndexForType() const
	{
		auto c = getEventCategory();
		return getIndex(c);
	}

	Event withManager(manager_int newManager) { Event copy = *this; copy.eventManager = newManager; return copy; }
	Event withEventType(EventType t) const noexcept { Event copy = *this; copy.eventType = t; return copy; }
	Event withEventActionType(EventAction at) const noexcept { return withEventType(EVENT_COMBINE(getEventCategory(), at)); }
	Event withCategory(Category c) const noexcept { return withEventType(EVENT_COMBINE(getEventActionType(), c)); }

	
 
	Event withNextCategory() const
	{
		jassert(getEventCategory() != Category::SlotEvent);
		Event copy = *this;

		if(copy.getEventCategory() == Category::ManagerEvent)
			return copy.withCategory(Category::SourceEvent);
		else if(copy.getEventCategory() == Category::SourceEvent)
			return copy.withCategory(Category::SlotEvent);
		else
		{
			jassertfalse;
		}
	}

	Event withSameCategoryButDifferentIndex(Category C, uint16 value) const
	{
		Event copy = *this;

		if (C == Category::ManagerEvent)
			copy.eventManager = eventManager;
		if (C == Category::SourceEvent)
			copy.source = source;
		if (C == Category::SlotEvent)
			copy.slot = slot;

		return copy;
	}

	manager_int getIndex(Category C) const
	{
		if (C == Category::ManagerEvent)
			return eventManager;
		if (C == Category::SourceEvent)
			return source;
		if (C == Category::SlotEvent)
			return slot;
		return 0;
	}

	template <Category C> constexpr manager_int getIndex() const
	{
		if constexpr (C == Category::ManagerEvent)
			return eventManager;
		if constexpr (C == Category::SourceEvent)
			return source;
		if constexpr (C == Category::SlotEvent)
			return slot;
	}

	template <Category C> constexpr bool sameCategoryAndIndex(const Event& other) const
	{
		if(isSameCategory(other))
			return other.getIndex<C>() == getIndex<C>();

		return false;
	}

	Type eventType = Type::Nothing;
	manager_int eventManager = 0;
	source_int source = 0;
	slot_int slot = 0;
};

#undef EVENT_BITWISE_AND
#undef EVENT_BITWISE_OR
#undef EVENT_COMBINE

} // dispatch
} // hise