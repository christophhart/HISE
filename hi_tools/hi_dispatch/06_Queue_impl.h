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

template <typename T>
Queue<T>::Queue(Resolver& resolver_):
	resolver(resolver_)
{}

template <typename T>
bool Queue<T>::push(manager_int index, manager_int slotIndex)
{
	if(isPositiveAndBelow(index-1, numEntries))
	{
		SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
		auto& e = entries[index-1];
		jassert(isPositiveAndBelow(slotIndex, e.numSlots));
		e.somethingChanged = true;
		data[e.offset + slotIndex] = 1;
		return true;
	}

	// can't find the event source in the list of registered items...
	//jassertfalse;
	return false;
}

template <typename T>
void Queue<T>::callAllSlots()
{
	for(int i = 0; i < numEntries; i++)
	{
		auto e = entries[i];
		if(auto obj = resolver.resolve(e.index))
		{
			auto full = (uint8*)alloca(e.numSlots);
			memset(full, 1, e.numSlots);
			auto ptr = data.get() + e.offset;

			{
				hise::SimpleReadWriteLock::ScopedReadLock sl(lock);
				memset(ptr, 0, sizeof(manager_int) * e.numSlots);
				e.somethingChanged = false;
			}
				
			size_t idx = 0;

			while(ptr[idx] != 0 && idx < e.numSlots)
				idx++;

			ff(*obj, full, idx);
		}
	}
}

template <typename T>
void Queue<T>::flush(const FlushFunction& ff)
{
	for(int i = 0; i < numEntries; i++)
	{
		auto e = entries[i];

		if(e.somethingChanged)
		{
			if(auto obj = resolver.resolve(e.index))
			{
				auto copy = (uint8*)alloca(sizeof(uint8) * e.numSlots);
				auto ptr = data.get() + e.offset;

				{
					hise::SimpleReadWriteLock::ScopedReadLock sl(lock);
					memcpy(copy, ptr, e.numSlots);
					memset(ptr, 0, e.numSlots);
					e.somethingChanged = false;
				}
					
				ff(*obj, copy, e.numSlots);
			}
		}
	}
}

template <typename T>
void Queue<T>::reallocate(manager_int newSize, const Array<Entry>& entryList)
{
	HeapBlock<uint8> newData;
	{
		newData.calloc(newSize);
		hise::SimpleReadWriteLock::ScopedReadLock sl(lock);
		memcpy(newData.get(), data.get(), jmin<manager_int>(numAllocated, newSize));
	}
	{
		hise::SimpleReadWriteLock::ScopedMultiWriteLock sl(lock);
		std::swap(data, newData);
		numAllocated = newSize;
		entries = const_cast<Entry*>(entryList.begin());
		numEntries = entryList.size();
	}
}

template <typename T>
Sender<T>::Sender(ResolverType* r, GlobalManager* gm, Category c):
	ObjectUnderGlobalManager(gm),
	category(c),
	resolver(*r),
	entries(),
	asyncQueue(*r),
	syncQueue(*r)
{
	rebuildQueue();
}

template <typename T>
void Sender<T>::push(manager_int thisIndex, manager_int changedIndex)
{
	if(!preAll)
	{
		if(!syncListener.isEmpty())
		{
			syncQueue.push(thisIndex, changedIndex);
					
			if(!defer)
			{
				syncQueue.flush([&](T& obj, uint8* values, size_t numValues)
				{
					if(auto lg = getCurrentLogger())
						lg->logListenerEvent(sendNotificationSync, &obj, values, numValues);

					SimpleReadWriteLock::ScopedReadLock sl(listenerLock);

					for(auto& l: syncListener)
						l->changed(obj, values, numValues);
				});
			}
		}

		if(!asyncListener.isEmpty())
		{
			asyncQueue.push(thisIndex, changedIndex);
		}
	}
}

template <typename T>
void Sender<T>::flushAsync()
{
	jassert(MessageManager::getInstance()->isThisTheMessageThread() || MessageManager::getInstance()->currentThreadHasLockedMessageManager());

	if(!defer && !preAll)
	{
		asyncQueue.flush([&](T& obj, uint8* values, size_t numValues)
		{
			if(auto lg = getCurrentLogger())
				lg->logListenerEvent(sendNotificationAsync, &obj, values, numValues);

			for(auto& l: this->asyncListener)
				l->changed(obj, values, numValues);
		});
	}
}

template <typename T>
void Sender<T>::cleanup()
{
	jassert(asyncListener.isEmpty());
		

	SimpleReadWriteLock::ScopedMultiWriteLock sl(listenerLock);
	asyncListener.clearQuick();
	syncListener.clearQuick();
}

template <typename T>
bool Sender<T>::isEmptyList(Listener* l) const
{
	auto n = l->getNotificationType();
	auto& list = n == sendNotificationSync ? syncListener : asyncListener;
	return list.isEmpty();
}

template <typename T>
void Sender<T>::addListener(Array<Listener*>& l)
{
	if(l.isEmpty())
		return;

	auto n = l.getFirst()->getNotificationType();
	auto initValue = l.getFirst()->shouldInitValues();
	auto& list = n == sendNotificationSync ? syncListener : asyncListener;

	Array<Listener*> newList;
	newList.ensureStorageAllocated(list.size() + l.size());

	{
		SimpleReadWriteLock::ScopedReadLock sl(listenerLock);
		newList.addArray(list);	
	}
		
	newList.addArray(l);

	{
		SimpleReadWriteLock::ScopedMultiWriteLock sl(listenerLock);
		std::swap(newList, list);
	}
		
	if(initValue)
	{
		// todo
		jassertfalse;
	}
}

template <typename T>
bool Sender<T>::performEvent(Event e)
{
	if(syncListener.isEmpty() && asyncListener.isEmpty())
		return false;

	if(e.isAnyDelayEvent())
		return onDeferEvent(e.getEventActionType());
	else if(e.isChangeEvent())
	{
		push(e.source, e.slot);
		return true;
	}

	return false;
}

template <typename T>
bool Sender<T>::onDeferEvent(EventAction n)
{
	if(!preAll)
	{
		if(n == EventAction::Defer)
		{
			defer = true;
			return true;
		}
		if(n == EventAction::PostDefer && !preAll)
		{
			defer = false;

			syncQueue.flush([&](T& obj, uint8* values, size_t numValues)
			{
				if(auto lg = getCurrentLogger())
					lg->logListenerEvent(sendNotificationSync, &obj, values, numValues);

				SimpleReadWriteLock::ScopedReadLock sl(listenerLock);

				for(auto& l: syncListener)
					l->changed(obj, values, numValues);
			});

			return true;
		}
		if(n == EventAction::PreAll)
		{
			preAll = true;
			defer = false;
			return true;
		}
		if(n == EventAction::AllEvents)
		{
			preAll = false;

			syncQueue.flush([](T& obj, uint8* values, size_t numValues)
				{});

			return true;
		}
	}

	return false;
}



template <typename T>
bool Sender<T>::rebuildQueue()
{
	auto numSources = resolver.getNumEventSources();
	auto numToAllocate = 0;
	for(int i = 0; i < numSources; i++)
		numToAllocate += resolver.getNumSlots(i);

	if(numSources == entries.size() && numToAllocate == asyncQueue.numAllocated)
		return false;

	if(auto lg = getCurrentLogger())
		lg->logReallocateEvent(asyncQueue.numAllocated, numToAllocate);

	Array<QueueItem> newEntries;
	newEntries.ensureStorageAllocated(numSources);
	manager_int offset = 0;
	for(int i = 0; i < numSources; i++)
	{
		QueueItem ne;
		ne.index = i;
		ne.numSlots = resolver.getNumSlots(i);
		ne.offset = offset;
		offset += ne.numSlots;
		newEntries.add(ne);
	}
		
	std::swap(entries, newEntries);
	asyncQueue.reallocate(numToAllocate, entries);
	syncQueue.reallocate(numToAllocate, entries);
	return true;
}
} // dispatch
} // hise