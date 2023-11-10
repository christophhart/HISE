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

// A base class for all classes that have a Listener as a member
struct ListenerOwner
{
	virtual ~ListenerOwner() {};
};

// A listener object that receives notifications about events of the SerialisedTree
// Features:
// - adding & removing listeners is a lockfree operation
// - can be registered to listen to diffent event types: 
//   1. list changes (???)
//   2. single slot changes 
//   3. multiple slot changes of a single source
//   4. slot changes to a subset of Sources
//   5  all slot changes
struct Listener: public Queueable
{
	using ValueChangedFunction = const std::function<void(uint8 index)>;

	// The listener data that holds information about the source event.
	struct ListenerData
	{
		explicit operator bool() const { return t != EventType::Nothing; }
		Source* s = nullptr;

		template <typename T> T* to_static_cast() const
		{
			jassert(dynamic_cast<T*>(s) != nullptr);
			return static_cast<T*>(s);
		}

		void callForEachSetValue(const ValueChangedFunction& f) const
		{
			for(int i = 0; i < numBytes; i++)
			{
				if(changes[i] != 0)
					f(i);
			}
		}

		EventType t = EventType::Nothing;
		uint8 slotIndex;
		uint8* changes;
		size_t numBytes;
	};

	template <typename T> T& getOwner()
	{
		static_assert(std::is_base_of<ListenerOwner, T>(), "not a base of listener owner");
		return *dynamic_cast<T*>(&owner);
	}

	template <typename T> const T& getOwner() const
	{
		static_assert(std::is_base_of<ListenerOwner, T>(), "not a base of listener owner");
		return *dynamic_cast<const T*>(&owner);
	}

	HashedCharPtr getDispatchId() const override { return "listener"; }

	// a function that iterates all children of a source manager and writes the pointers into the byte array
	// returns the number of bytes written...
	using SubsetFunction = std::function<size_t(uint8**)>;

	/** Used for reading and writing the byte data from the listener event. */
	struct EventParser
	{
		explicit EventParser(Listener& l_);
		void writeData(Queue& q, EventType t, Queueable* c, uint8* values, uint8 numValues) const;

		/** Parses the data and returns a ListenerData object that is sent to the listener if the listener matches. */
		ListenerData parseData(const Queue::FlushArgument& listenerData, const Queue::FlushArgument& eventData) const;

		/** Writes the source pointer to the data slot (and bumps the data pointer). Use inside the Subset function. */
		static size_t writeSourcePointer(Source* s, uint8** data);

		Listener& l;
	};

	explicit Listener(RootObject& r, ListenerOwner& owner);
	~Listener() override;;

	/** Override this method and implement whatever you want to do with the notification. */
	virtual void slotChanged(const ListenerData& d) = 0;
	
	/** Registers the listener to all slot changes of a subset of source slots. */
	void addListenerToSingleSource(Source* source, uint8* slotIndexes, uint8 numSlots, DispatchType n);

	/** Registers the listener to receive updates from a single slot with a defined slot subset. */
	void addListenerToSingleSourceAndSlotSubset(Source* source, uint8 slotIndex, const uint8* slotIndexes, uint8 numSlots,
	                                            DispatchType n)
	{
		jassertfalse;
	}

	/** Registers the listener to all sources of a given source manager. */
	void addListenerToAllSources(SourceManager& sourceManager, DispatchType n);

	/** Registers the listener to a subset of sources of the given manager. */ 
	void addListenerToSubset(SourceManager& sourceManager, const SubsetFunction& sf, DispatchType n);

	/** Removes the listener. */
	void removeListener(Source& s, DispatchType n = sendNotification);

private:

	ListenerOwner& owner;
};

using Listener = dispatch::Listener;

} // dispatch
} // hise