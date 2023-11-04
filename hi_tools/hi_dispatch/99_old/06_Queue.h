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


struct GlobalManager;

struct ObjectUnderGlobalManager
{
public:

	ObjectUnderGlobalManager(GlobalManager* gm_):
	  gm(gm_)
	{};

	virtual ~ObjectUnderGlobalManager()
	{
		gm = nullptr;
	}

	const GlobalManager* getGlobalManager() const { return gm; }
	GlobalManager* getGlobalManager() { return gm; }

	LoggerBase::Ptr getCurrentLogger();

private:
	GlobalManager* gm = nullptr;
};

template <typename T> struct Queue
{
	struct Resolver
	{
		/** Overwrite this and return the object for the given index. */
		virtual T* resolve(manager_int index) = 0;

		/** Overwrite this and return the number of event sources to use. */
		virtual manager_int getNumEventSources() const = 0;
		/** Overwrite this and return the number of event slots for the given event source. */
		virtual manager_int getNumSlots(manager_int index) const = 0;
	};

	struct Entry
	{
		uint8 somethingChanged = 0;
		uint8 unused = 0;
		manager_int index = 0;
		manager_int numSlots = 0;
		uint16 offset = 0;
	};

	using FlushFunction = std::function<void(T&, uint8*, size_t)>;

	Queue(Resolver& resolver_);;

	bool push(manager_int index, manager_int slotIndex);
	void callAllSlots();
	void flush(const FlushFunction& ff);
	void reallocate(manager_int newSize, const Array<Entry>& entryList);

	Resolver& resolver;
	hise::SimpleReadWriteLock lock;
	Entry* entries = nullptr;
	manager_int numEntries = 0;
	HeapBlock<uint8> data;
	manager_int numAllocated = 0;
};




template <typename T> struct Sender: public ObjectUnderGlobalManager
{
	using ResolverType = typename Queue<T>::Resolver;
	using QueueItem = typename Queue<T>::Entry;

	Sender(ResolverType* r, GlobalManager* gm, Category c);

	const Category category;
	ResolverType& resolver;

	struct Listener
	{
		virtual void changed(T& obj, uint8* values, size_t numValues) = 0;
		virtual ~Listener() {};

		virtual NotificationType getNotificationType() const = 0;
		virtual bool shouldInitValues() const = 0;
	};
	
	void push(manager_int thisIndex, manager_int changedIndex);
	void flushAsync();
	void cleanup();
	bool isEmptyList(Listener* l) const;
	void addListener(Array<Listener*>& l);
	bool performEvent(Event e);
	bool onDeferEvent(EventAction n);

	void removeListener(Array<Listener*>& listOfListenersToRemove)
	{
		if(listOfListenersToRemove.isEmpty())
			return;

		auto n = listOfListenersToRemove.getFirst()->getNotificationType();
		auto& list = n == sendNotificationSync ? syncListener : asyncListener;

		Array<Listener*> newList;
		newList.ensureStorageAllocated(list.size() - listOfListenersToRemove.size());

		{
			SimpleReadWriteLock::ScopedReadLock sl(listenerLock);

			for(auto& l: list)
			{
				if(!listOfListenersToRemove.contains(l))
					newList.add(l);
			}
		}

		SimpleReadWriteLock::ScopedMultiWriteLock sl(listenerLock);
		std::swap(newList, list);
	}
	bool rebuildQueue();

	Array<QueueItem> entries;
	Queue<T> asyncQueue;
	Queue<T> syncQueue;

	mutable SimpleReadWriteLock listenerLock;
	Array<Listener*> syncListener;
	Array<Listener*> asyncListener;
	
	bool defer = false;
	bool preAll = false;
};

} // dispatch
} // hise