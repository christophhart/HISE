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

#include <mutex>
#include <shared_mutex>
#include <array>
#include <thread>
#include <atomic>
#include <emmintrin.h>




namespace hise {
using namespace juce;


struct audio_spin_mutex 
{
	void lock() noexcept {
		constexpr std::array iterations = { 5, 10, 3000 };

		for (int i = 0; i < iterations[0]; ++i) {
			if (try_lock())
				return;
		}

		for (int i = 0; i < iterations[1]; ++i) {
			if (try_lock())
				return;

			_mm_pause();
		}

		while (true) 
		{
			for (int i = 0; i < iterations[2]; ++i) {
				if (try_lock())
					return;

				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
			}

			jassertfalse;
		}
	}

	bool try_lock() noexcept
	{
		return !flag.test_and_set(std::memory_order_acquire);
	}

	void unlock() noexcept 
	{
		flag.clear(std::memory_order_release);
	}

private:

	std::atomic_flag flag = ATOMIC_FLAG_INIT;
};

struct audio_spin_mutex_shared
{
	void lock() noexcept
	{
		w.lock();

		constexpr std::array iterations = { 5, 10, 3000 };

		for (int i = 0; i < iterations[0]; ++i) {
			if (sharedCounter.load() == 0)
				return;
		}

		for (int i = 0; i < iterations[1]; ++i) {
			if (sharedCounter.load() == 0)
				return;

			_mm_pause();
		}

		while (true)
		{
			for (int i = 0; i < iterations[2]; ++i) {
				if (sharedCounter.load() == 0)
					return;

				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
				_mm_pause();
			}

			jassertfalse;
		}
	}

	bool try_lock() noexcept
	{
		return w.try_lock();
	}

	void unlock() noexcept
	{
		w.unlock();
	}

	void lock_shared() noexcept
	{
		w.lock();
		sharedCounter.fetch_add(1, std::memory_order_acquire);
		w.unlock();
	}

	bool try_lock_shared() noexcept
	{
		if (w.try_lock())
		{
			sharedCounter.fetch_add(1, std::memory_order_acquire);
			w.unlock();
			return true;
		}

		return false;
	}

	void unlock_shared() noexcept
	{
		sharedCounter.fetch_sub(1, std::memory_order_release);
	}

	audio_spin_mutex w;
	std::atomic<int> sharedCounter = 0;
};


/** Same as MessageManager::callAsync, but uses a Safe pointer for a component and cancels the async execution if the object is deleted. */
struct LambdaWithComponent
{
	template <class T> static void callAsync(T* b_, const std::function<void(T*)>& f)
	{
		Component::SafePointer<T> b = b_;

		auto f2 = [b, f]()
		{
			if (auto stillB = b.getComponent())
			{
				f(stillB);
			}
		};

		MessageManager::callAsync(f2);
	}
};

#define CALL_ASYNC_WITH_COMPONENT(ComponentClass, component) LambdaWithComponent::callAsync<ComponentClass>(closeButton, [](ComponentClass* component)

class SuspendableTimer
{
public:

	struct Manager
	{
        virtual ~Manager() {};
        
		virtual void suspendStateChanged(bool shouldBeSuspended) = 0;
	};

	SuspendableTimer() :
		internalTimer(*this)
	{};

	virtual ~SuspendableTimer() { internalTimer.stopTimer(); };

	void startTimer(int milliseconds)
	{
		lastTimerInterval = milliseconds;

#if !HISE_HEADLESS
		if (!suspended)
			internalTimer.startTimer(milliseconds);
#endif
	}

	void stopTimer()
	{
		lastTimerInterval = -1;

		if (!suspended)
		{
			internalTimer.stopTimer();
		}
		else
		{
			// Must be stopped by suspendTimer
			jassert(!internalTimer.isTimerRunning());
		}
	}

	void suspendTimer(bool shouldBeSuspended)
	{
		if (shouldBeSuspended != suspended)
		{
			suspended = shouldBeSuspended;

#if !HISE_HEADLESS
			if (suspended)
				internalTimer.stopTimer();
			else if (lastTimerInterval != -1)
				internalTimer.startTimer(lastTimerInterval);
#endif
		}
	}

	virtual void timerCallback() = 0;

    bool isSuspended()
    {
        return suspended;
    }
    
private:

	struct Internal : public Timer
	{
		Internal(SuspendableTimer& parent_) :
			parent(parent_)
		{};

		void timerCallback() override { parent.timerCallback(); };

		SuspendableTimer& parent;
	};

	Internal internalTimer;

	bool suspended = false;

	int lastTimerInterval = -1;
};

/** Coallescates timer updates.
	@ingroup event_handling
	
*/
class PooledUIUpdater : public SuspendableTimer
{
public:

	PooledUIUpdater() :
		pendingHandlers(8192)
	{
        suspendTimer(false);
		startTimer(30);
	}

	class Broadcaster;

	class Listener
	{
	public:

		virtual ~Listener() {};

		virtual void handlePooledMessage(Broadcaster* b) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	class SimpleTimer
	{
	public:
		SimpleTimer(PooledUIUpdater* h):
			updater(h)
		{
			start();
		}

		virtual ~SimpleTimer()
		{
			stop();
		}

		void start()
		{
			startOrStop(true);
		}

		void stop()
		{
			startOrStop(false);
		}

		virtual void timerCallback() = 0;

	private:

		void startOrStop(bool shouldStart)
		{
			WeakReference<SimpleTimer> safeThis(this);

			auto f = [safeThis, shouldStart]()
			{
				if (safeThis.get() != nullptr)
				{
					if(shouldStart)
						safeThis.get()->updater->simpleTimers.addIfNotAlreadyThere(safeThis);
					else
						safeThis.get()->updater->simpleTimers.removeAllInstancesOf(safeThis);
				}
			};

			if (MessageManager::getInstance()->currentThreadHasLockedMessageManager())
				f();
			else
				MessageManager::callAsync(f);
		}

		JUCE_DECLARE_WEAK_REFERENCEABLE(SimpleTimer);

		WeakReference<PooledUIUpdater> updater;
	};

	class Broadcaster
	{
	public:
		Broadcaster() {};
		virtual ~Broadcaster() {};

		void setHandler(PooledUIUpdater* handler_)
		{
			handler = handler_;
		}

		void sendPooledChangeMessage();

		void addPooledChangeListener(Listener* l) { pooledListeners.addIfNotAlreadyThere(l); };
		void removePooledChangeListener(Listener* l) { pooledListeners.removeAllInstancesOf(l); };

		bool isHandlerInitialised() const { return handler != nullptr; };

		bool pending = false;

	private:

		friend class PooledUIUpdater;

		
		Array<WeakReference<Listener>> pooledListeners;

		WeakReference<PooledUIUpdater> handler;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Broadcaster);
	};

	void timerCallback() override
	{
		for (auto st : simpleTimers)
		{
			if (st != nullptr)
				st->timerCallback();
		}

		WeakReference<Broadcaster> b;

		while (pendingHandlers.pop(b))
		{
			if (b.get() != nullptr)
			{
				b->pending = false;

				for (auto l : b->pooledListeners)
				{
					if (l != nullptr)
						l->handlePooledMessage(b);
				}
			}
		}
	}

private:

	Array<WeakReference<SimpleTimer>> simpleTimers;
	LockfreeQueue<WeakReference<Broadcaster>> pendingHandlers;

	JUCE_DECLARE_WEAK_REFERENCEABLE(PooledUIUpdater);
};




/** This class is used by multiple complex UI classes to handle the notification and updates. 

	There are three main events that can happen with complex data types:
	1. The values have changed (but the data pointer stays the same).
	2. The data has been changed (so that the data pointer points to a different location)
	3. The index has been changed (used for displaying purposes).

	This class manages the communication and notification to connected UI objects for these
	data types.
*/
class ComplexDataUIUpdaterBase
{
public:

	enum class EventType
	{
		Idle,
		DisplayIndex,
		ContentChange,
		ContentRedirected,
		numEventTypes
	};

	struct EventListener
	{
		virtual ~EventListener() {};

		virtual void onComplexDataEvent(EventType t, var data) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(EventListener);
	};

	virtual ~ComplexDataUIUpdaterBase() {};

	void addEventListener(EventListener* l)
	{
		ScopedLock sl(updateLock);
		listeners.addIfNotAlreadyThere(l);
		updateUpdater();
	}

	void removeEventListener(EventListener* l)
	{
		ScopedLock sl(updateLock);
		listeners.removeAllInstancesOf(l);
		updateUpdater();
	}

	void setUpdater(PooledUIUpdater* updater)
	{
		if (globalUpdater == nullptr)
		{
			ScopedLock sl(updateLock);
			globalUpdater = updater;
			updateUpdater();
		}
	}

	void sendDisplayChangeMessage(float newIndexValue, NotificationType notify) const
	{
		sendMessageToListeners(EventType::DisplayIndex, var(newIndexValue), notify);
	}

	void sendContentChangeMessage(NotificationType notify, int indexThatChanged)
	{
		sendMessageToListeners(EventType::ContentChange, var(indexThatChanged), notify);
	}

	void sendContentRedirectMessage()
	{
		sendMessageToListeners(EventType::ContentRedirected, {}, sendNotificationSync, true);
	}

	PooledUIUpdater* getGlobalUIUpdater()
	{
		return globalUpdater;
	}

private:

	void updateUpdater()
	{
		if (globalUpdater != nullptr && currentUpdater == nullptr && listeners.size() > 0)
			currentUpdater = new Updater(*this);

		if (listeners.size() == 0 || globalUpdater == nullptr)
			currentUpdater = nullptr;
	}

	PooledUIUpdater* globalUpdater = nullptr;

	struct Updater : public PooledUIUpdater::SimpleTimer
	{
		void timerCallback() override
		{
			if (parent.lastChange != EventType::Idle)
			{
				parent.sendMessageToListeners(parent.lastChange, parent.lastValue, sendNotificationSync, true);
				parent.lastChange = EventType::Idle;
			}
		}

		Updater(ComplexDataUIUpdaterBase& parent_) :
			SimpleTimer(parent_.globalUpdater),
			parent(parent_)
		{
			start();
		};

		ComplexDataUIUpdaterBase& parent;
	};

	CriticalSection updateLock;
	ScopedPointer<Updater> currentUpdater;

	void sendMessageToListeners(EventType t, var v, NotificationType n, bool forceUpdate = false) const
	{
		if (n == dontSendNotification)
			return;

		if (n == sendNotificationSync)
		{
			bool isMoreImportantChange = t >= lastChange;
			bool valueHasChanged = lastValue != v;

			if (forceUpdate || (isMoreImportantChange && valueHasChanged))
			{
				lastChange = jmax(t, lastChange);

				for (auto l : listeners)
				{
					if (l.get() != nullptr)
						l->onComplexDataEvent(t, v);
				}
			}
		}
		else
		{
			bool isMoreImportantChange = t >= lastChange;
			
			if (isMoreImportantChange)
			{
				lastChange = jmax(lastChange, t);
				lastValue = v;
			}
		}
	}

	mutable EventType lastChange = EventType::Idle;
	mutable var lastValue;

	Array<WeakReference<EventListener>> listeners;
};




class SafeChangeBroadcaster;

/** A class for message communication between objects.
*	@ingroup event_handling
*
*	This class has the same functionality as the JUCE ChangeListener class, but it uses a weak reference for the internal list,
*	so deleted listeners will not crash the application.
*/
class SafeChangeListener : public PooledUIUpdater::Listener
{
public:

	virtual ~SafeChangeListener()
	{
		masterReference.clear();
	}

	/** Overwrite this and handle the message. */
	virtual void changeListenerCallback(SafeChangeBroadcaster *b) = 0;

	void handlePooledMessage(PooledUIUpdater::Broadcaster* b) override;

private:

	friend class WeakReference < SafeChangeListener >;

	WeakReference<SafeChangeListener>::Master masterReference;
};





/** A drop in replacement for the ChangeBroadcaster class from JUCE but with weak references.
*	@ingroup event_handling
*
*	If you use the normal class and forget to unregister a listener in its destructor, it will crash the application.
*	This class uses a weak reference (but still throws an assertion so you still recognize if something is funky), so it handles this case much more gracefully.
*
*	Also you can add a string to your message for debugging purposes (with the JUCE class you have no way of knowing what caused the message if you call it asynchronously.
*/
class SafeChangeBroadcaster: public PooledUIUpdater::Broadcaster
{
public:

	SafeChangeBroadcaster(const String& name_ = {}) :
		dispatcher(this),
		name(name_)
	{};

	virtual ~SafeChangeBroadcaster()
	{
		dispatcher.cancelPendingUpdate();
	};

	/** Sends a synchronous change message to all the registered listeners.
	*
	*	This will immediately call all the listeners that are registered. For thread-safety reasons, you must only call this method on the main message thread.
	*/
	void sendSynchronousChangeMessage();;

	/** Registers a listener to receive change callbacks from this broadcaster.
	*
	*	Trying to add a listener that's already on the list will have no effect.
	*/
	void addChangeListener(SafeChangeListener *listener);

	/**	Unregisters a listener from the list.
	*
	*	If the listener isn't on the list, this won't have any effect.
	*/
	void removeChangeListener(SafeChangeListener *listener);

	/** Removes all listeners from the list. */
	void removeAllChangeListeners();

	/** Causes an asynchronous change message to be sent to all the registered listeners.
	*
	*	The message will be delivered asynchronously by the main message thread, so this method will return immediately.
	*	To call the listeners synchronously use sendSynchronousChangeMessage().
	*/
	void sendChangeMessage(const String &/*identifier*/ = String());;

	/** This will send a message without allocating a message slot.
	*
	*   Use this in the audio thread to prevent malloc calls, but don't overuse this feature.
	*/
	void sendAllocationFreeChangeMessage();

	void enablePooledUpdate(PooledUIUpdater* updater);

	bool hasChangeListeners() const noexcept { return !listeners.isEmpty(); }

private:

	class AsyncBroadcaster : public AsyncUpdater
	{
	public:
		AsyncBroadcaster(SafeChangeBroadcaster *parent_) :
			parent(parent_)
		{}

		void handleAsyncUpdate() override
		{
			parent->sendSynchronousChangeMessage();
		}

		SafeChangeBroadcaster *parent;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AsyncBroadcaster)
	};

	const String name;

	AsyncBroadcaster dispatcher;

	String currentString;

	Array<WeakReference<SafeChangeListener>, CriticalSection> listeners;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SafeChangeBroadcaster)
	JUCE_DECLARE_WEAK_REFERENCEABLE(SafeChangeBroadcaster);
};

class CopyPasteTarget;

class CopyPasteTargetHandler
{
public:

	virtual ~CopyPasteTargetHandler() {};

	virtual void setCopyPasteTarget(CopyPasteTarget* target) = 0;
};

/** Subclass your component from this class and the main window will focus it to allow copy pasting with shortcuts.
*
*   Then, in your mouseDown method, call grabCopyAndPasteFocus().
*	If you call paintOutlineIfSelected from your paint method, it will be automatically highlighted.
*/
class CopyPasteTarget
{
public:

	struct HandlerFunction
	{
		using Func = std::function<CopyPasteTargetHandler*(Component*)>;

		CopyPasteTargetHandler* getHandler(Component* c)
		{
			return f(c);
		}

		Func f;
	};

	

	CopyPasteTarget() : isSelected(false) {};
	virtual ~CopyPasteTarget()
	{
		masterReference.clear();
	};

	virtual String getObjectTypeName() = 0;
	virtual void copyAction() = 0;
	virtual void pasteAction() = 0;

	void grabCopyAndPasteFocus();

	void dismissCopyAndPasteFocus();

	bool isSelectedForCopyAndPaste() { return isSelected; };

	void paintOutlineIfSelected(Graphics &g);

	void deselect();

	/* Use this method to supply a lambda that returns the specific CopyPasteTargetHandler for the given application. */
	static void setHandlerFunction(HandlerFunction* f);

	static CopyPasteTargetHandler * getNothing(Component*) { return nullptr; }

	static HandlerFunction* handlerFunction;

private:

	
	WeakReference<CopyPasteTarget>::Master masterReference;
	friend class WeakReference < CopyPasteTarget >;

	//WeakReference<Processor> processor;

	bool isSelected;

};




/** A small helper class that uses RAII for enabling flush to zero mode. */
class ScopedNoDenormals
{
public:
	ScopedNoDenormals();;

	~ScopedNoDenormals();;

	int oldMXCSR;
};

/** A simple mutex without locking.

	It allows reading & writing from different threads with multiple simultaneous readers
	but only one active writer at the same time.

	If the lock can't be acquired (so either because a reader has it and a writer wants it
	or if the writer has it and a reader wants it), it doesn't lock, but just return false
	so it will never halt the execution.

	This is suitable for situations where the execution of the reader / writer task is
	somewhat optional but locking must never occur.
*/
struct SingleWriteLockfreeMutex
{
	/** A scoped read lock for the given mutex. */
	struct ScopedReadLock
	{
		ScopedReadLock(SingleWriteLockfreeMutex& parent_) :
			parent(parent_)
		{
			if (!parent.currentlyWriting)
			{
				++parent.numAccessors;
				locked = true;
			}
			else
				locked = false;
		}

		operator bool() const
		{
			return locked;
		}

		~ScopedReadLock()
		{
			if (locked)
				--parent.numAccessors;
		}

		SingleWriteLockfreeMutex& parent;

		bool locked;
	};

	/** A scoped write lock for the given mutex. */
	struct ScopedWriteLock
	{
		ScopedWriteLock(SingleWriteLockfreeMutex& parent_) :
			parent(parent_),
			prevWriteState(parent.currentlyWriting.load())
		{
			if (parent.numAccessors == 0)
			{
				jassert(!parent.currentlyWriting);
				parent.currentlyWriting.store(true);
				++parent.numAccessors;

				locked = true;
			}
			else
				locked = false;
		}

		operator bool() const
		{
			return locked;
		}

		~ScopedWriteLock()
		{
			if (locked)
			{
				parent.currentlyWriting.store(prevWriteState);
				--parent.numAccessors;
			}

		}

		SingleWriteLockfreeMutex& parent;

		bool prevWriteState;
		bool locked;
	};

private:

    std::atomic<int> numAccessors { 0 };
    std::atomic<bool> currentlyWriting { false };
};

#if 0
/** A simple lock with read-write access. The read lock is non-reentrant (however it will not lock itself out, but the write access is reentrant but expected to be single writer.
*/
struct SimpleReadWriteLock
{
	struct ScopedReadLock
	{
		ScopedReadLock(SimpleReadWriteLock &lock_, bool busyWait = false);
		~ScopedReadLock();
		SimpleReadWriteLock& lock;

		bool anotherThreadHasWriteLock() const;
	};

	struct ScopedWriteLock
	{
		ScopedWriteLock(SimpleReadWriteLock &lock_, bool busyWait = false);
		~ScopedWriteLock();
		SimpleReadWriteLock& lock;

	private:

		bool holdsLock = false;
	};

	struct ScopedTryReadLock
	{
		ScopedTryReadLock(SimpleReadWriteLock &lock_);
		~ScopedTryReadLock();

		bool hasLock() const { return is_locked; }

	private:
		bool is_locked;
		SimpleReadWriteLock& lock;
	};

	bool tryRead()
	{
		if (writerThread == nullptr || writerThread == Thread::getCurrentThreadId())
		{
			numReadLocks++;
			return true;
		}
		
		return false;
	}

	void exitRead()
	{
		numReadLocks--;
	}

	std::atomic<int> numReadLocks{ 0 };
    std::atomic<void*> writerThread{ nullptr };
};
#endif




struct SimpleReadWriteLock
{
	struct ScopedWriteLock
	{
		ScopedWriteLock(SimpleReadWriteLock& l, bool tryToAcquireLock=true):
			lock(l)
		{
			auto thisId = std::this_thread::get_id();
            auto i = std::thread::id();

			if (!tryToAcquireLock)
				lock.fakeWriteLock = true;

			holdsLock = tryToAcquireLock && lock.enabled && lock.writer.compare_exchange_weak(i, thisId);

			if (holdsLock)
			{
				lock.mutex.lock();
			}
		}

		~ScopedWriteLock()
		{
			lock.fakeWriteLock = false;

			unlock();
		}

		void unlock()
		{
			if (holdsLock)
			{
				lock.writer.store(std::thread::id());
				lock.mutex.unlock();
				holdsLock = false;
			}
		}

	private:

		bool holdsLock = false;
		SimpleReadWriteLock& lock;
	};

	bool enterReadLock()
	{
		if (enabled && std::this_thread::get_id() != writer)
		{
			mutex.lock_shared();
			return true;
		}

		return false;
	}

	void exitReadLock(bool& holdsLock)
	{
		if (holdsLock)
		{
			mutex.unlock_shared();
			holdsLock = false;
		}
	}

	struct ScopedReadLock
	{
		ScopedReadLock(SimpleReadWriteLock& l):
			lock(l)
		{
			holdsLock = l.enterReadLock();
		}

		~ScopedReadLock()
		{
			lock.exitReadLock(holdsLock);
		}

	private:

		bool holdsLock = false;
		SimpleReadWriteLock& lock;
	};

	struct ScopedDisabler
	{
		ScopedDisabler(SimpleReadWriteLock& l):
			lock(l)
		{
			lock.enabled = false;
		}

		~ScopedDisabler()
		{
			lock.enabled = true;
		}

		SimpleReadWriteLock& lock;
	};

	bool writeAccessIsLocked() const { return writer.load() != std::thread::id(); }

	bool writeAccessIsSkipped() const { return fakeWriteLock; }

	struct ScopedTryReadLock
	{
		ScopedTryReadLock(SimpleReadWriteLock& l):
			lock(l)
		{
			holdsLock = lock.mutex.try_lock_shared();
		}

		~ScopedTryReadLock()
		{
			unlock();
			
		}
		
		operator bool() const 
		{
			if (holdsLock)
				return true;

			return lock.writer == std::this_thread::get_id();
		};

		void unlock()
		{
			if (holdsLock)
			{
				lock.mutex.unlock_shared();
				holdsLock = false;
			}
		}

	private:

		bool holdsLock = false;
		SimpleReadWriteLock& lock;
	};

	using LockType = audio_spin_mutex_shared;
	//using LockType = std::shared_mutex;

	LockType mutex;

	std::atomic<std::thread::id> writer;
	bool enabled = true;
	bool fakeWriteLock = false;
};

/** This is a non allocating alternative to the AsyncUpdater.
*	@ingroup event_handling
*
*	If you're creating a lot of these object's, it will clog the Timer thread,
*	but for single objects that need an update once in a while, it's better,
*	because it won't post a update message that needs to be allocated.
*/
class LockfreeAsyncUpdater
{
private:

	struct TimerPimpl : public SuspendableTimer
	{
		explicit TimerPimpl(LockfreeAsyncUpdater* p_) :
			parent(*p_)
		{
			dirty = false;
			startTimer(30);
		}

		~TimerPimpl()
		{
			dirty = false;
			stopTimer();
		}

		void timerCallback() override
		{
			bool v = true;
			if (dirty.compare_exchange_strong(v, false))
			{
				parent.handleAsyncUpdate();
			}
		}

		void triggerAsyncUpdate()
		{
			dirty.store(true);
		};

		void cancelPendingUpdate()
		{
			dirty.store(false);
		};

	private:

		LockfreeAsyncUpdater & parent;
		std::atomic<bool> dirty;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimerPimpl);
	};

public:

	virtual ~LockfreeAsyncUpdater();

	virtual void handleAsyncUpdate() = 0;

	void triggerAsyncUpdate();
	void cancelPendingUpdate();

	void suspend(bool shouldBeSuspended)
	{
		pimpl.suspendTimer(shouldBeSuspended);
	}

protected:

	LockfreeAsyncUpdater();

	TimerPimpl pimpl;

	static int instanceCount;
};


struct ComplexDataUIBase : public ReferenceCountedObject
{
	/** A listener that will be notified about changes of the complex data source. */
	struct SourceListener
	{
		virtual ~SourceListener() {};

		virtual void sourceHasChanged(ComplexDataUIBase* oldSource, ComplexDataUIBase* newSource) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SourceListener);
	};

	struct EditorBase
	{
		virtual ~EditorBase() {};

		virtual void setComplexDataUIBase(ComplexDataUIBase* newData) = 0;
	};

	/** A SourceWatcher notifies its registered listeners about changes to a source. */
	struct SourceWatcher
	{
		void setNewSource(ComplexDataUIBase* newSource)
		{
			if (newSource != currentSource)
			{
				for (auto l : listeners)
				{
					if (l != nullptr)
						l->sourceHasChanged(currentSource, newSource);
				}

				currentSource = newSource;
			}
		}

		void addSourceListener(SourceListener* l)
		{
			listeners.addIfNotAlreadyThere(l);
		}

		void removeSourceListener(SourceListener* l)
		{
			listeners.removeAllInstancesOf(l);
		}

	private:

		Array<WeakReference<SourceListener>> listeners;

		WeakReference<ComplexDataUIBase> currentSource;
	};

	virtual ~ComplexDataUIBase() {};

	void setGlobalUIUpdater(PooledUIUpdater* updater)
	{
		internalUpdater.setUpdater(updater);
	}

	

	void sendDisplayIndexMessage(float n)
	{
		internalUpdater.sendDisplayChangeMessage(n, sendNotificationAsync);
	}

	virtual bool fromBase64String(const String& b64) = 0;
	virtual String toBase64String() const = 0;

	ComplexDataUIUpdaterBase& getUpdater() { return internalUpdater; };
	const ComplexDataUIUpdaterBase& getUpdater() const { return internalUpdater; };

	void setUndoManager(UndoManager* managerToUse)
	{
		undoManager = managerToUse;
	}

	UndoManager* getUndoManager(bool useUndoManager = true) { return useUndoManager ? undoManager : nullptr; };

	hise::SimpleReadWriteLock& getDataLock() const { return dataLock; }

protected:

	ComplexDataUIUpdaterBase internalUpdater;

private:

	mutable hise::SimpleReadWriteLock dataLock;

	UndoManager* undoManager = nullptr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ComplexDataUIBase);
};


/** A fuzzy search algorithm that uses the Levenshtein distance algorithm to find approximate strings. */
class FuzzySearcher
{
public:

	/** Matches the string against the search term with the given fuzzyness (0.0 - 1.0). */
	static bool fitsSearch(const String &searchTerm, const String &stringToMatch, double fuzzyness);

	/** Returns a string array with the results. */
	static StringArray searchForResults(const String &word, const StringArray &wordList, double fuzzyness);

	/** Returns a index array with the results for the given wordlist. */
	static Array<int> searchForIndexes(const String &word, const StringArray &wordList, double fuzzyness);

private:

	static void search(void *outputArray, bool useIndexes, const String &word, const StringArray &wordList, double fuzzyness);

	static int getLevenshteinDistance(const String &src, const String &dest);
};



/** A Helper class that encapsulates the regex operations.
*	@ingroup utility */
class RegexFunctions
{
public:

	static Array<StringArray> findSubstringsThatMatchWildcard(const String &regexWildCard, const String &stringToTest);

	/** Searches a string and returns a StringArray with all matches.
	*	You can specify and index of a capture group (if not, the entire match will be used). */
	static StringArray search(const String& wildcard, const String &stringToTest, int indexInMatch = 0);

	/** Returns the first match of the given wildcard in the test string. The first entry will be the whole match, followed by capture groups. */
	static StringArray getFirstMatch(const String &wildcard, const String &stringToTest);

	/** Checks if the given string matches the regex wildcard. */
	static bool matchesWildcard(const String &wildcard, const String &stringToTest);

};



/** A collection of little helper functions to clean float arrays.
*	@ingroup utility
*
*	Source: http://musicdsp.org/showArchiveComment.php?ArchiveID=191
*/
struct FloatSanitizers
{
	template <typename ContainerType> static void sanitizeArray(ContainerType& d)
	{
		for (auto& s : d)
			sanitizeFloatNumber(s);
	}

	static void sanitizeArray(float* data, int size);;

	static bool sanitizeFloatNumber(float& input);;

	struct Test : public UnitTest
	{
		Test() :
			UnitTest("Testing float sanitizer")
		{

		};

		void runTest() override;
	};
};


static FloatSanitizers::Test floatSanitizerTest;


/** This class is used to simulate different devices.
*
*	In the backend application you can choose the current device. In compiled apps
*	it will be automatically set to the actual model.
*
*	It will use different UX paradigms depending on the model as well.
*
*	Due to simplicity, it uses a static variable which may cause some glitches when used with plugins, so
*	it's recommended to use this only in standalone mode.
*/
class HiseDeviceSimulator
{
public:
	enum class DeviceType
	{
		Desktop = 0,
		iPad,
		iPadAUv3,
		iPhone,
		iPhoneAUv3,
		numDeviceTypes
	};

	static void init(AudioProcessor::WrapperType wrapper);

	static void setDeviceType(DeviceType newDeviceTye)
	{
		currentDevice = newDeviceTye;
	}

	static DeviceType getDeviceType() { return currentDevice; }

	static String getDeviceName(int index = -1);

	static bool fileNameContainsDeviceWildcard(const File& f);

	static bool isMobileDevice() { return currentDevice > DeviceType::Desktop; }

	static bool isAUv3() { return currentDevice == DeviceType::iPadAUv3 || currentDevice == DeviceType::iPhoneAUv3; };

	static bool isiPhone()
	{
		return currentDevice == DeviceType::iPhone || currentDevice == DeviceType::iPhoneAUv3;
	}

	static bool isStandalone()
	{
#if HISE_IOS
		return !isAUv3();
#else

#if IS_STANDALONE_FRONTEND || (USE_BACKEND && IS_STANDALONE_APP)
		return true;
#else
		return false;
#endif

#endif
	}

	static Rectangle<int> getDisplayResolution();

private:

	static DeviceType currentDevice;
};


#if JUCE_DEBUG
#define GUI_UPDATER_FRAME_RATE 150
#else
#define GUI_UPDATER_FRAME_RATE 30
#endif


/** Utility class that reduces the update rate to a common framerate (~30 fps)
*
*	Unlike the UpdateMerger class, this class checks the time between calls to shouldUpdate() and returns true, if 30ms have passed since the last succesfull call to shouldUpdate().
*
*/
class GUIUpdater
{
public:

	GUIUpdater() :
		timeOfLastCall(Time::currentTimeMillis()),
		timeOfDebugCall(Time::currentTimeMillis())
	{}

	/** Call this to check if the last update was longer than 30 ms ago.
	*
	*	If debugInterval is true, then the interval between calls will be printed in debug mode.
	*/
	bool shouldUpdate(bool debugInterval = false)
	{
		ignoreUnused(debugInterval);

		const int64 currentTime = Time::currentTimeMillis();

#ifdef JUCE_DEBUG

		if (debugInterval)
		{
			timeOfDebugCall = currentTime;
		}

#endif

		if ((currentTime - timeOfLastCall) > GUI_UPDATER_FRAME_RATE)
		{
			timeOfLastCall = currentTime;
			return true;
		}

		return false;
	}

private:

	int64 timeOfLastCall;

	int64 timeOfDebugCall;
};


/** A utility class for linear interpolation between samples.
*	@ingroup utility
*
*/
class Interpolator
{
public:

	/** A simple linear interpolation.
	*
	*	@param lowValue the value of the lower index.
	*	@param highValue the value of the higher index.
	*	@param delta the sub-integer part between the two indexes (must be between 0.0f and 1.0f)
	*	@returns the interpolated value.
	*/

	static float interpolateLinear(const float lowValue, const float highValue, const float delta)
	{
		jassert(isPositiveAndNotGreaterThan(delta, 1.0f));

		const float invDelta = 1.0f - delta;
		return invDelta * lowValue + delta * highValue;
	}

	static double interpolateLinear(const double lowValue, const double highValue, const double delta)
	{
		jassert(isPositiveAndNotGreaterThan(delta, 1.0));

		const double invDelta = 1.0 - delta;
		return invDelta * lowValue + delta * highValue;
	}

};



}
