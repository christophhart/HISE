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

	PooledUIUpdater* getUpdater();
	void setLogger(Logger* l);
	Logger* getLogger() const { return currentLogger; }

	int getNumChildren() const;

	void setState(const HashedPath& path, State newState);
	
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
	
    bool isHighPriorityFlushPending() const { return hiPriorityPending; }
    
private:

    bool hiPriorityPending = false;
    
	State globalState = State::Running;

	// must be static for unit tests...
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
		  
		void run() override;

		RootObject& parent;
	};

	ScopedPointer<HiPriorityThread> ownedHighPriorityThread;

	Logger* currentLogger = nullptr;
	PooledUIUpdater* updater;

	std::atomic<int> numChildObjects = { 0 };

	enum ChildType
	{
		SourceManagerType,
		numChildTypes
	};

	template <typename T, Behaviour B> bool forEach(const std::function<bool(T&)>& objectFunction) const
	{
		ChildType CT = ChildType::SourceManagerType;
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

// A subclass of RootObject child that will automatically remove itself from all queues. */
class Queueable: public RootObject::Child
{
public:

	explicit Queueable(RootObject& r);;
	~Queueable() override;

protected:
	
	/** Call this in your destructor after you made sure to remove it from all queues.
	 *  If this is false, it will iterate all queues of the root object with a huge performance penalty! */
	void cleared()
	{
		isCleared = true;
	}

	/** Call this in your destructor if you want to remove it from all queues. */
	void clearFromRoot()
	{
		cleared();
	}

private:

	bool isCleared = false;
	DanglingBehaviour danglingBehaviour = DanglingBehaviour::Undefined;
};

class Suspendable: public Queueable
{
public:

	Suspendable(RootObject& r, Suspendable* parent_):
	  Queueable(r),
	  parent(parent_)
	{};
	  
	~Suspendable() override {};
	virtual void setState(const HashedPath& p, State newState) = 0;
	virtual State getStateFromParent() const { return parent != nullptr ? parent->getStateFromParent() : State::Running; }

	bool hasParent() const { return parent != nullptr; }

private:

	Suspendable* parent = nullptr;
};

template <typename T> struct DispatchTypeContainer
{
	template <typename... Args> DispatchTypeContainer(Args&&... args):
	  sync(std::forward<Args>(args)...),
	  asyncHiPrio(std::forward<Args>(args)...),
	  async(std::forward<Args>(args)...)
	{};

	T& get(DispatchType n)
	{
		switch(n)
		{
		case DispatchType::sendNotificationAsync:			return async;
		case DispatchType::sendNotificationAsyncHiPriority: return asyncHiPrio;
		case DispatchType::sendNotificationSync:			return sync;
		default: jassertfalse; return async;
		}
	}

	void forEach(const std::function<void(T&)>& f)
	{
		f(sync);
		f(asyncHiPrio);
		f(async);
	}

	void forEachWithDispatchType(const std::function<void(DispatchType n, T&)>& f)
	{
		f(DispatchType::sendNotificationSync,  sync);
		f(DispatchType::sendNotificationAsyncHiPriority, asyncHiPrio);
		f(DispatchType::sendNotificationAsync, async);
	}

	const T& get(DispatchType n) const
	{
		switch(n)
		{
		case DispatchType::sendNotificationAsync:			return async;
		case DispatchType::sendNotificationAsyncHiPriority: return asyncHiPrio;
		case DispatchType::sendNotificationSync:			return sync;
		default: jassertfalse; return async;
		}
	}

private:

	T sync;
	T asyncHiPrio;
	T async;
};

} // dispatch
} // hise