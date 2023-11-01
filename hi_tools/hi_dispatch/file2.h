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


struct SourceManager: public Queueable, // not queuable
					  public PooledUIUpdater
{
	SourceManager(RootObject& r, const Identifier& typeId);
	~SourceManager();

	const Identifier treeId;

	String getDispatchId() const override { return treeId.toString(); }

	/*+ Sends slot changes (values[0] == slotIndex) to the matching listeners. */
	void sendSlotChanges(Source& s, const uint8* values, size_t numValues);
	
	void timerCallback();

	Queue& getListenerQueue(NotificationType n);
	const Queue& getListenerQueue(NotificationType n) const;
	int getNumChildSources() const { return items.size(); }

private:

	Queue asyncQueue;
	Queue deferedSyncEvents;
	Queue asyncListeners;
	Queue syncListeners;
	Array<Source*> items;
};



// just a interface class. 
// Subclass all classes that have a SlotSender from this class
// (the item class will keep a reference to this as member to send)
struct Source: public Queueable
{
	Source(SourceManager& parent_, const String& sourceId );
	~Source() override;

	String getDispatchId() const override { return sourceId; }

	SourceManager& getParentSourceManager() noexcept { return parent; }
	const SourceManager& getParentSourceManager() const noexcept { return parent; }

	SourceManager& parent;
	const String sourceId;
};

// A object that can send slot change messages to a SourceManager
// Ownership: a member of a subclassed Source object
// Features: 
// - dynamic number of slots
// - a constant sender index (one byte)
// - no heap allocation for <16 byte senders
struct SlotSender
{		
	SlotSender(Source& s, uint8 slotIndex_);;

	const uint8 slotIndex;
	const Identifier slotId; // REPLACE WITH WILDCARD PARSER
	static const uint8 senderId; // REMOVE
	
	void setNumSlots(int newNumSlots);
	bool flush();
	bool sendChangeMessage(int indexInSlot, NotificationType notify);

	Source& obj;
	ObjectStorage<64, 16> data;
	size_t numSlots;
	bool pending = false;
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
	// The listener data that holds information about the source event.
	struct ListenerData
	{
		operator bool() const { return t != EventType::Nothing; }
		Source* s = nullptr;
		EventType t = EventType::Nothing;
		uint8 slotIndex;
		uint8* changes;
		size_t numBytes;
	};

	String getDispatchId() const override { return "listener"; }

	// a function that iterates all children of a source manager and writes the pointers into the byte array
	// returns the number of bytes written...
	using SubsetFunction = std::function<size_t(uint8**)>;

	/** Used for reading and writing the byte data from the listener event. */
	struct EventParser
	{
		EventParser(Listener& l_);
		void writeData(Queue& q, EventType t, Queueable* c, uint8* values, uint8 numValues) const;

		/** Parses the data and returns a ListenerData object that is sent to the listener if the listener matches. */
		ListenerData parseData(const Queue::FlushArgument& f) const;

		/** Writes the source pointer to the data slot (and bumps the data pointer). Use inside the Subset function. */
		static size_t writeSourcePointer(Source* s, uint8** data);

		Listener& l;
	};

	Listener(RootObject& r);
	~Listener();;

	/** Override this method and implement whatever you want to do with the notification. */
	virtual void slotChanged(const ListenerData& d) = 0;
	
	/** Registers the listener to a single source. */
	void addListenerToSingleSource(Source* source, uint8* slotIndexes, uint8 numSlots, NotificationType n);

	/** Registers the listener to all sources of a given source manager. */
	void addListenerToAllSources(SourceManager& sourceManager, NotificationType n);

	/** Registers the listener to a subset of sources of the given manager. */ 
	void addListenerToSubset(SourceManager& sourceManager, const SubsetFunction& sf, NotificationType n);

	/** Removes the listener. */
	void removeListener(SourceManager& s, NotificationType n = sendNotification);
};

using Listener = dispatch::Listener;

// defers listChanged calls until done, then send a EventAction PostDefer message
// The ScopedDelayer takes the write lock
// constructor invalidates all indexes
// deconstructor calls signal rebuild, validates all indexes & calls the listeners
struct ScopedDelayer
{
	
};

} // dispatch
} // hise