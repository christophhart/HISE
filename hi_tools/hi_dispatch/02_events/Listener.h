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

class ListenerQueue
{
public:

	ListenerQueue(RootObject& r, size_t unused)
	{
		
	}

	struct EventData
	{
		EventData()
		{
			static_assert(sizeof(EventData) == 16, "not 16 byte");
		}

		explicit operator bool() const { return t != EventType::Nothing; }

		template <typename T> T* to_static_cast() const
		{
			jassert(dynamic_cast<T*>(s) != nullptr);
			return static_cast<T*>(s);
		}

		const SlotBitmap& toBitMap() const
		{
			jassert(t == EventType::SingleListenerSubset);
			return changes;
		}

		uint8 toSingleSlotIndex() const
		{
			jassert(t == EventType::SingleListenerSingleSlot);
			return indexWithinSlot;
		}

		Source* s = nullptr;
		EventType t = EventType::Nothing;
		uint8 slotIndex = 0;
		uint8 indexWithinSlot = 0;
		SlotBitmap changes;
	};
	
	sigslot::connection addListener(Listener* l);

	bool isEmpty()
	{
		auto numSlots = sig.slot_count();
		return numSlots == 0;
	}

	State getState() const { return currentState; }

	void setQueueState(State s)
	{
		if(s != currentState)
		{
			currentState = s;

			if(s == State::Paused)
				sig.block();
			else
				sig.unblock();
		}
	}

	
	/*
	void removeConnection(const sigslot::connection& c)
	{
		sig.disconnect(c);
	}
	*/

	void removeAllMatches(Listener* l);

	void clear()
	{
		sig.disconnect_all();
	}

	bool flush(const EventData& d)
	{
		sig(d);

		return currentState == State::Running;
	}

private:


	State currentState = State::Running;
	sigslot::signal<const EventData&> sig;
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
class Listener: public Queueable,
				public sigslot::observer
{
public:

	using ValueChangedFunction = const std::function<void(uint8 index)>;

	// The listener data that holds information about the source event.
	using ListenerData = ListenerQueue::EventData;

	template <typename T> T& getOwner()
	{
		static_assert(std::is_same<T, ListenerOwner>() || std::is_base_of<ListenerOwner, T>(), "not a base of listener owner");
		return *dynamic_cast<T*>(&owner);
	}

	template <typename T> const T& getOwner() const
	{
		static_assert(std::is_same<T, ListenerOwner>() || std::is_base_of<ListenerOwner, T>(), "not a base of listener owner");
		return *dynamic_cast<const T*>(&owner);
	}

	HashedCharPtr getDispatchId() const override { return "listener"; }
	
	explicit Listener(RootObject& r, ListenerOwner& owner);
	~Listener() override;;

	/** Override this method and implement whatever you want to do with the notification. */
	virtual void slotChanged(const ListenerData& d) = 0;

	struct Connection
	{
		Connection(Source* s, Listener* l_):
		  source(s),
		  l(l_)
		{};

		const void* get_object_ptr() const { return l; }

		operator bool() const noexcept { return l != nullptr; }

		bool operator==(const Connection& other)
		{
			return l == other.l;
		}

		Listener* l = nullptr;
		Source* source = nullptr;
		EventType listenerType;
		uint8 slotIndex = 0;
		uint8 indexWithinSlot = 0;
		DispatchType n = DispatchType::sendNotification;
		SlotBitmap registeredSlots;

		Connection()
		{
			static_assert(sizeof(Connection) == 32, "not 32 bytes");
		}

		bool call(const ListenerQueue::EventData& f) const
		{
			auto d = f;
			bool call = false;

			jassert(source != nullptr);

			if(source != f.s)
				return false;

			switch(listenerType)
			{
			case EventType::ListenerWithoutData:
				{
					call = true;
					break;
				}
			case EventType::ListenerAnySlot:
				{
					d.changes = registeredSlots;
					call = true;
					break;
				};
			case EventType::SingleListenerSingleSlot:
				{
					call = d.changes[indexWithinSlot];
					d.indexWithinSlot = indexWithinSlot;
					break;
				}
			case EventType::SingleListenerSubset:
				{
					call = d.changes.hasSomeBitsAs(registeredSlots);
					break;
				}
			case EventType::AllListener: 
				{
					d.changes = registeredSlots;
					call = true;
					break;
				}
			default: jassertfalse; break;
			}

			if(call)
			{
				d.t = listenerType;
				l->slotChanged(d);	
			}

			return call;
		}
	};

	hise::UnorderedStack<std::pair<ListenerQueue*, Connection>, 32> currentConnection;

	void onSigSlot(const ListenerQueue::EventData& f)
	{
		for(auto c: currentConnection)
		{
			c.second.call(f);
		}
	}

#if 0
	void addListenerToRawQueue(ListenerQueue* q, uint8 slotIndex, DispatchType n)
	{
		Connection info(nullptr, this);
		info.listenerType = EventType::ListenerAnySlot;
		info.slotIndex = slotIndex;
		info.n = n;
		addToQueueInternal(q, info);
	}
#endif

	/** Registers the listener to all slot changes of a subset of source slots. */
	void addListenerToSingleSource(Source* source, uint8* slotIndexes, uint8 numSlots, DispatchType n);

	/** Registers a listener without any filters. */
	void addListenerWithoutData(Source* source, uint8 slotIndex, DispatchType n);

	/** Registers the listener to slot changes of a certain index within the given slot. */
	void addListenerToSingleSlotIndexWithinSlot(Source* source, uint8 slotIndex, uint8 indexWithinSlot,
	                                            DispatchType n);

	/** Registers the listener to receive updates from a single slot with a defined slot subset. */
	void addListenerToSingleSourceAndSlotSubset(Source* source, uint8 slotIndex, const uint8* slotIndexes, uint8 numSlots,
	                                            DispatchType n);
	;

	/** Registers the listener to all sources of a given source manager. */
	void addListenerToAllSources(SourceManager& sourceManager, DispatchType n);
	
	/** Removes the listener. */
	void removeListener(Source& s, DispatchType n = sendNotification);
	
private:

	void addToQueueInternal(ListenerQueue* q, const Connection& con)
	{
		jassert(currentConnection.size() < 31);
		currentConnection.insertWithoutSearch({q, con});
		q->addListener(this);
	}

	/*
	struct ConnectionType
	{
		bool operator==(const ConnectionType& other) const
		{
			return q == other.q;
		}

		ListenerQueue* q = nullptr;
		sigslot::connection con;
	};
	*/

	ListenerOwner& owner;
	bool removed = true;

	static constexpr int NumMaxConnections = 256;

	/*
	hise::UnorderedStack<ConnectionType, NumMaxConnections> connections;
	*/

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Listener);
};

using Listener = dispatch::Listener;



} // dispatch
} // hise
