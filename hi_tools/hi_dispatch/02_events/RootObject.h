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

class Queueable;
class Source;
class SourceManager;
class Source;
class Listener;

class RootObject
{
public:

	class Child
	{
	public:

		Child(RootObject& root_);;
		virtual ~Child();

		/** return the ID of this object. */
		virtual HashedCharPtr getDispatchId() const = 0;

		virtual bool matchesPath(const HashedPath& p) const { return false; }

		RootObject& getRootObject() { return root; }
		const RootObject& getRootObject() const { return root; }
		
	private:

		RootObject& root;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Child);
	};

	explicit RootObject(PooledUIUpdater* updater_);
	~RootObject();

	void addChild(Child* c);
	void removeChild(Child* c);

	// Call this in the constructor / destructor of the subclasses
	// Source / Queue / SourceManager / Listener
	void addTypedChild(Child* c);

	void removeTypedChild(Child* c);

	/** Removes the element from all queues. */
	void clearFromAllQueues(Queueable* q, DanglingBehaviour danglingBehaviour);
	
	PooledUIUpdater* getUpdater();
	void setLogger(Logger* l);
	Logger* getLogger() const { return currentLogger; }

	int getNumChildren() const;

	void setState(const HashedPath& path, State newState);

#if 0
	/** Iterates all registered queues and calls the given function. */
	bool callForAllQueues(Behaviour b, const std::function<bool(Queue&)>& qf) const;

	/** Iterates all registered sources and calls the given function. */
	bool callForAllSources(Behaviour b, const std::function<bool(Source&)>& sf) const;

	/** Iterates all registered source managers and calls the given function. */
	bool callForAllSourceManagers(const std::function<bool(SourceManager&)>& sf) const;

	/** Iterates all registered listener and calls the given function. */
	bool callForAllListeners(const std::function<bool(dispatch::Listener&)>& lf) const;
#endif

	/** Call this from a thread that is running periodically. */
	void flushHighPriorityQueues(Thread* t);

	/** Call this if you want the root object to use its own background thread for flushing the high priority queue. */
	void setUseHighPriorityThread(bool shouldUse);

	uint64_t bumpFlowCounter() { return ++flowCounter; }

	struct ScopedGlobalSuspender
	{
		ScopedGlobalSuspender(RootObject& r, State newState, const CharPtr& description);

		~ScopedGlobalSuspender();

	private:

		RootObject& root;
		const State prevValue;
		bool isUsed = false;
	};
	
private:

	State globalState = State::Running;

	static uint64_t flowCounter;

	struct HiPriorityThread: public Thread
	{
		HiPriorityThread(RootObject& r):
		  Thread("Dispatch HiPriority Thread"),
		  parent(r)
		{
			startThread(7);
		}

		~HiPriorityThread()
		{
			notify();
			stopThread(500);
		}
		  
		void run() override
		{
			while(!threadShouldExit())
			{
				parent.flushHighPriorityQueues(this);

				wait(500);
			}
		}

		RootObject& parent;
	};

	ScopedPointer<HiPriorityThread> ownedHighPriorityThread;

	Logger* currentLogger = nullptr;
	PooledUIUpdater* updater;

	std::atomic<int> numChildObjects = { 0 };

	enum ChildType
	{
		SourceManagerType,
		SourceType,
		QueueType,
		ListenerType,
		numChildTypes
	};

	template <typename T, Behaviour B> bool forEach(const std::function<bool(T&)>& objectFunction) const
	{
		ChildType CT;

		if constexpr(std::is_same<T, SourceManager>())
			CT = ChildType::SourceManagerType;
		if constexpr(std::is_same<T, Source>())
			CT = ChildType::SourceType;
		if constexpr(std::is_same<T, Queue>())
			CT = ChildType::QueueType;
		if constexpr(std::is_same<T, Listener>())
			CT = ChildType::ListenerType;

		ScopedReadLock sl(childLock[CT]);

		for(auto element: children[CT])
		{
			if constexpr (B == Behaviour::BreakIfPaused)
			{
				if(globalState != State::Running)
					return false;
			}

			if(objectFunction(*dynamic_cast<T*>(element)))
					return true;
		}

		return false;
	}

	Array<Child*> children[numChildTypes];
	ReadWriteLock childLock[numChildTypes];
};

} // dispatch
} // hise