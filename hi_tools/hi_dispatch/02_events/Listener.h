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
class Listener: public Queueable
{
public:

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

		SlotBitmap toBitMap() const
		{
			jassert(numBytes == SlotBitmap::getNumBytes());
			return SlotBitmap(changes);
		}

		uint8 toSingleSlotIndex() const
		{
			jassert(numBytes == 1);
			return *changes;
		}

		EventType t = EventType::Nothing;
		uint8 slotIndex;
		const uint8* changes;
		size_t numBytes;
	};

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

	/** Registers the listener to all slot changes of a subset of source slots. */
	void addListenerToSingleSource(Source* source, uint8* slotIndexes, uint8 numSlots, DispatchType n);

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

	ListenerOwner& owner;
	bool removed = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Listener);
};

using Listener = dispatch::Listener;


template <typename T> class SingleValueSource: public Source
{
public:

	class Listener final : private dispatch::Listener
	{
	public:

		using Callback = std::function<void(int, T)>;

		Listener(RootObject& r, ListenerOwner& o, const Callback& c):
		  dispatch::Listener(r, o),
		  f(c)
		{};

	private:

		void slotChanged(const ListenerData& d) override
		{
			jassert(d.t == EventType::ListenerWithoutData);
			jassert(f);
			jassert(d.s != nullptr);
			
			auto cd = static_cast<SingleValueSource*>(d.s);

#if PERFETTO
			StringBuilder b;
			b << "listener callback for " << cd->getDispatchId();
			TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));
#endif

			f(cd->index, cd->value);
		}

		friend class SingleValueSource;

		Callback f;
	};

	SingleValueSource(SingleValueSourceManager<T>& parent, SourceOwner& owner, int index_, const HashedCharPtr& id):
	  Source(parent, owner, id),
	  index(index_),
	  valueSender(*this, 0, "value")
	{
		valueSender.setNumSlots(1);
	};

	void addValueListener(Listener* l, bool initialiseValue, DispatchType n)
	{
		valueSender.getListenerQueue(n)->push(l, EventType::ListenerWithoutData, nullptr, 0);

		if(initialiseValue)
		{
			StringBuilder n;
			n << "init call " << getDispatchId();
			TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(n));
			dispatch::Listener::ListenerData d;
			d.t = EventType::ListenerWithoutData;
			d.s = this;
			d.slotIndex = 0;
			d.numBytes = 0;
			d.changes = nullptr;
			l->slotChanged(d);
		}
	}

	void removeValueListener(Listener* l, DispatchType n=DispatchType::sendNotification)
	{
		l->removeListener(*this, n);
	}

	template <typename T> int getNumListenersWithClass(DispatchType n=DispatchType::sendNotification) const
	{
		int numListeners = 0;

		const_cast<SingleValueSource*>(this)->forEachListenerQueue(n, [&numListeners](uint8, DispatchType, Queue* q)
		{
			q->flush([&numListeners](const Queue::FlushArgument& f)
			{
				auto& l = f.getTypedObject<dispatch::Listener>();

				auto t = dynamic_cast<T*>(&l.getOwner<ListenerOwner>());

				if(t != nullptr)
					numListeners++;

				return true;
			}, Queue::FlushType::KeepData);
		});

		return numListeners;
	}

	void setValue(T v, DispatchType n)
	{
#if PERFETTO
		StringBuilder b;
		b << getDispatchId() << "(" << index << ")";
		TRACE_DISPATCH(DYNAMIC_STRING_BUILDER(b));
#endif

		value = v;
		valueSender.sendChangeMessage(0, n);
	}

	int getNumSlotSenders() const override { return 1; }
	SlotSender* getSlotSender(uint8) override { return &valueSender; }

private:

	T value = T();
	const int index;
	SlotSender valueSender;
};

} // dispatch
} // hise