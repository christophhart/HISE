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

#if !JUCE_ARM
#include <emmintrin.h>
#endif




namespace hise {
using namespace juce;

/** This class will safely call the given function on the message thread as long as the object that was passed in is still alive.

	You can pass in any lambda with a reference parameter to a class object and it will be guaranteed to be executed only if the object wasn't deleted.

	Obviously the class you want to use here needs to be JUCE_DECLARE_WEAK_REFERENCEABLE.
*/
class SafeAsyncCall
{
	template <typename T> struct SafeAsyncCaller
	{
		using Func = std::function<void(T&)>;

		void operator()()
		{
			if (obj_.get() != nullptr)
				f_(*obj_.get());
		}

		SafeAsyncCaller(T* o, const Func& f2_) :
			obj_(o),
			f_(f2_)
		{};

	private:

		WeakReference<T> obj_;
		Func f_;
	};

public:

	template <typename T> static void call(T& object, std::function<void(T&)> f)
	{
		MessageManager::callAsync(SafeAsyncCaller<T>(&object, f));
	}

	template <typename T> static void callAsyncIfNotOnMessageThread(T& object, std::function<void(T&)> f)
	{
		if (MessageManager::getInstance()->isThisTheMessageThread())
			f(object);
		else
			MessageManager::callAsync(SafeAsyncCaller<T>(&object, f));
	}

	template <typename T> static void callWithDelay(T& object, std::function<void(T&)> f, int milliseconds)
	{
		Timer::callAfterDelay(milliseconds, SafeAsyncCaller<T>(&object, f));
	}

	static void resized(Component* c)
	{
		callAsyncIfNotOnMessageThread<Component>(*c, [](Component& c) { c.resized(); });
	}

	static void repaint(Component* c)
	{
		callAsyncIfNotOnMessageThread<Component>(*c, [](Component& c) { c.repaint(); });
	}
};


struct audio_spin_mutex 
{
	void lock() noexcept {
		constexpr std::array<int, 3> iterations = { 5, 10, 3000 };

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

			//jassertfalse;
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
		// We need to check the counter before
		// locking the internal mutex to allow reentrant
		// read locks
		while (sharedCounter.load() > 0)
		{
			_mm_pause();
			_mm_pause();
		}

		w.lock();

		constexpr std::array<int, 3> iterations = { 5, 10, 3000 };

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
    std::atomic<int> sharedCounter {0};
};


struct WeakErrorHandler
{
	using Ptr = WeakReference<WeakErrorHandler>;

	virtual ~WeakErrorHandler() {};
	virtual void handleErrorMessage(const String& error) = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(WeakErrorHandler);
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

#if !HISE_NO_GUI_TOOLS


class GlContextHolder
	: private juce::ComponentListener,
	private juce::OpenGLRenderer
{
public:

	GlContextHolder(juce::Component& topLevelComponent);

	//==============================================================================
	// The context holder MUST explicitely call detach in their destructor
	void detach();

	//==============================================================================
	// Clients MUST call unregisterOpenGlRenderer manually in their destructors!!
	void registerOpenGlRenderer(juce::Component* child);

	void unregisterOpenGlRenderer(juce::Component* child);

	void setBackgroundColour(const juce::Colour c);

	juce::OpenGLContext context;

private:
	//==============================================================================
	void checkComponents(bool isClosing, bool isDrawing);

	//==============================================================================
	void componentParentHierarchyChanged(juce::Component& component) override;

	void componentVisibilityChanged(juce::Component& component) override;

	void componentBeingDeleted(juce::Component& component) override;

	//==============================================================================
	void newOpenGLContextCreated() override;

	void renderOpenGL() override;

	void openGLContextClosing() override;

	//==============================================================================
	juce::Component& parent;

	struct Client
	{
		enum class State
		{
			suspended,
			running
		};

		Client(juce::Component* comp, State nextStateToUse = State::suspended);


		juce::Component* c = nullptr;
		State currentState = State::suspended, nextState = State::suspended;
	};

	juce::CriticalSection stateChangeCriticalSection;
	juce::Array<Client, juce::CriticalSection> clients;

	//==============================================================================
	int findClientIndexForComponent(juce::Component* comp);

	Client* findClientForComponent(juce::Component* comp);

	//==============================================================================
	juce::Colour backgroundColour{ juce::Colours::black };
};


struct TopLevelWindowWithKeyMappings
{
	static KeyPress getKeyPressFromString(Component* c, const String& s);

	static void addShortcut(Component* c, const String& category, const Identifier& id, const String& description, const KeyPress& k);

	static KeyPress getFirstKeyPress(Component* c, const Identifier& id);

	static bool matches(Component* c, const KeyPress& k, const Identifier& id);

	/*
	static KeyPress getKeyPress(Component* c, const Identifier& id)
	{
		if (auto t = getFromComponent(c))
		{
			if (auto idx = t->shortcutIds.indexOf(id) + 1)
				return t->keyMap.getKeyPressesAssignedToCommand(idx).getFirst();
		}

		return {};
	}
	*/

	KeyPressMappingSet& getKeyPressMappingSet();;

protected:

	TopLevelWindowWithKeyMappings();;

	virtual ~TopLevelWindowWithKeyMappings();;

	/** Call this function and initialise all key presses that you want to define. */
	virtual void initialiseAllKeyPresses();

	void saveKeyPressMap();

	void loadKeyPressMap();

	virtual File getKeyPressSettingFile() const = 0;

    bool saved = false;

private:

	bool initialised = false;
	
	bool loaded = false;

	static TopLevelWindowWithKeyMappings* getFromComponent(Component* c);

	Array<Identifier> shortcutIds;
	juce::ApplicationCommandManager m;
	juce::KeyPressMappingSet keyMap;
};

/** A small helper interface class that allows you to find the topmost component
	that might have a OpenGL context attached.

	This */
struct TopLevelWindowWithOptionalOpenGL
{
	virtual ~TopLevelWindowWithOptionalOpenGL();

	/** Use this method instead of getTopLevelComponent() to find the topmost component that might have an OpenGL context attached. */
	static Component* findRoot(Component* c);

	struct ScopedRegisterState
	{
		ScopedRegisterState(TopLevelWindowWithOptionalOpenGL& t_, Component* c_);

		~ScopedRegisterState();;

		TopLevelWindowWithOptionalOpenGL& t;
		Component* c;
	};

    bool isOpenGLEnabled() const;

protected:

	void detachOpenGl();

	void setEnableOpenGL(Component* c);

	void addChildComponentWithOpenGLRenderer(Component* c);

	void removeChildComponentWithOpenGLRenderer(Component* c);

public:

	ScopedPointer<GlContextHolder> contextHolder;
};
#endif


class SuspendableTimer
{
public:

	struct Manager
	{
        virtual ~Manager();;
        
		virtual void suspendStateChanged(bool shouldBeSuspended) = 0;
	};

	SuspendableTimer();;

	virtual ~SuspendableTimer();;

	void startTimer(int milliseconds);

	void stopTimer();

	void suspendTimer(bool shouldBeSuspended);

	virtual void timerCallback() = 0;

    bool isSuspended();

private:

	struct Internal : public Timer
	{
		Internal(SuspendableTimer& parent_);;

		void timerCallback() override;;

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

	PooledUIUpdater();

	class Broadcaster;

	class Listener
	{
	public:

		virtual ~Listener();;

		virtual void handlePooledMessage(Broadcaster* b) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	class SimpleTimer
	{
	public:
		SimpleTimer(PooledUIUpdater* h, bool shouldStart=true);

		virtual ~SimpleTimer();

		void start();

		void stop();

		bool isTimerRunning() const;;

		virtual void timerCallback() = 0;

	private:

		void startOrStop(bool shouldStart);

		JUCE_DECLARE_WEAK_REFERENCEABLE(SimpleTimer);

		bool isRunning = false;
		WeakReference<PooledUIUpdater> updater;
	};

	class Broadcaster
	{
	public:
		Broadcaster();;
		virtual ~Broadcaster();;

		void setHandler(PooledUIUpdater* handler_);

		void sendPooledChangeMessage();

		void addPooledChangeListener(Listener* l);;
		void removePooledChangeListener(Listener* l);;

		bool isHandlerInitialised() const;;

		bool pending = false;

	private:

		friend class PooledUIUpdater;

		
		Array<WeakReference<Listener>> pooledListeners;

		WeakReference<PooledUIUpdater> handler;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Broadcaster);
	};

	void timerCallback() override;

private:

	Array<WeakReference<SimpleTimer>, CriticalSection> simpleTimers;
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
		virtual ~EventListener();;

		virtual void onComplexDataEvent(EventType t, var data) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(EventListener);
	};

	virtual ~ComplexDataUIUpdaterBase();;

	void addEventListener(EventListener* l);

	void removeEventListener(EventListener* l);

	void setUpdater(PooledUIUpdater* updater);

	void sendDisplayChangeMessage(float newIndexValue, NotificationType notify, bool forceUpdate=false) const;

	void sendContentChangeMessage(NotificationType notify, int indexThatChanged);

	void sendContentRedirectMessage();

	PooledUIUpdater* getGlobalUIUpdater();

	float getLastDisplayValue() const;

private:

	

	void updateUpdater();

	PooledUIUpdater* globalUpdater = nullptr;

	struct Updater : public PooledUIUpdater::SimpleTimer
	{
		void timerCallback() override;

		Updater(ComplexDataUIUpdaterBase& parent_);;

		ComplexDataUIUpdaterBase& parent;
	};

	CriticalSection updateLock;
	ScopedPointer<Updater> currentUpdater;

	void sendMessageToListeners(EventType t, var v, NotificationType n, bool forceUpdate = false) const;

	mutable float lastDisplayValue = 1.0f;
	mutable EventType lastChange = EventType::Idle;
	mutable var lastValue;

	static constexpr int NumListenerSlots = 128;
	hise::UnorderedStack<WeakReference<EventListener>, NumListenerSlots> listeners;
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

		CopyPasteTargetHandler* getHandler(Component* c);

		Func f;
	};
	

	CopyPasteTarget();;
	virtual ~CopyPasteTarget();;

	virtual String getObjectTypeName() = 0;
	virtual void copyAction() = 0;
	virtual void pasteAction() = 0;

	void grabCopyAndPasteFocus();

	void dismissCopyAndPasteFocus();

	bool isSelectedForCopyAndPaste();;

	void paintOutlineIfSelected(Graphics &g);

	void deselect();

	/* Use this method to supply a lambda that returns the specific CopyPasteTargetHandler for the given application. */
	static void setHandlerFunction(HandlerFunction* f);

	static CopyPasteTargetHandler * getNothing(Component*);

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

struct SimpleReadWriteLock
{
	struct ScopedMultiWriteLock
	{
		ScopedMultiWriteLock(SimpleReadWriteLock& l, bool tryToAcquireLock=true):
			lock(l)
		{
			auto thisId = std::this_thread::get_id();
            auto i = std::thread::id();

			if (!tryToAcquireLock)
				lock.fakeWriteLock = true;

			auto wantsLock = tryToAcquireLock && lock.enabled;

			if(wantsLock)
			{
				// If this hits, then you're using a reentrant lock. Consider the ScopedWriteLock instead...
				jassert(thisId != lock.writer.load());

				lock.mutex.lock();
				lock.writer.store(thisId);
				holdsLock = true;
			}
		}

		~ScopedMultiWriteLock()
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

	struct ScopedWriteLock
	{
		ScopedWriteLock(SimpleReadWriteLock& l, bool tryToAcquireLock=true):
			lock(l)
		{
			auto thisId = std::this_thread::get_id();
            auto i = std::thread::id();

			if (!tryToAcquireLock)
				lock.fakeWriteLock = true;

			// if this hits, you're using multiple writer threads. consider the ScopedMultiWriteLock instead
			jassert(lock.writer == thisId || lock.writer == i);

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

	bool enterTryReadLock()
	{
		if (enabled && std::this_thread::get_id() != writer)
		{
			return mutex.try_lock_shared();
		}

		return false;
	}

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
		ScopedReadLock(SimpleReadWriteLock& l, bool tryToAquireLock=true):
			lock(l)
		{
			if(tryToAquireLock)
				holdsLock = l.enterReadLock();
		}

		~ScopedReadLock()
		{
			if(holdsLock)
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
			return ok();
		};

		void unlock()
		{
			if (holdsLock)
			{
				lock.mutex.unlock_shared();
				holdsLock = false;
			}
		}

	protected:

		bool ok() const
		{
			if (holdsLock)
				return true;

			return lock.writer == std::this_thread::get_id();
		}

	private:

		bool holdsLock = false;
		SimpleReadWriteLock& lock;
	};

	using LockType = audio_spin_mutex_shared;
	//using LockType = std::shared_mutex;

	LockType mutex;

    std::atomic<std::thread::id> writer = {};
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
		explicit TimerPimpl(LockfreeAsyncUpdater* p_);

		~TimerPimpl();

		void timerCallback() override;

		void triggerAsyncUpdate();;

		void cancelPendingUpdate();;

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

	void suspend(bool shouldBeSuspended);

protected:

	LockfreeAsyncUpdater();

	TimerPimpl pimpl;

	static int instanceCount;
};

template <typename ReturnType, typename... Ps> struct SafeLambdaBase
{
	ReturnType operator()(Ps...parameters)
	{
		return call(parameters...);
	}

	virtual ~SafeLambdaBase() {};

	virtual ReturnType call(Ps... parameters) = 0;

	virtual bool isValid() const = 0;

	virtual bool matches(void* other) const = 0;

	
};

template <class T, typename ReturnType, typename...Ps> struct SafeLambda : public SafeLambdaBase<ReturnType, Ps...>
{
	using InternalLambda = std::function<ReturnType(T&, Ps...)>;

	SafeLambda(T& obj, const InternalLambda& f_) :
		item(&obj),
		f(f_)
	{

	};

	bool isValid() const final override { return item.get() != nullptr; };

	bool matches(void* obj) const final override { return static_cast<void*>(item.get()) == obj; };

	ReturnType call(Ps... parameters) override
	{
		if (item != nullptr)
			return f(*item, parameters...);

		return ReturnType();
	}

private:

	WeakReference<T> item;
	InternalLambda f;
};

/** A listener class that can be used as member to implement a safe listener communication system.

	You can specify the parameters that the callback will contain as template parameters.

	This is most suitable for non-performance critical tasks that require the least amount of boilerplate
	where you just want to implement a safe and simple listener system with a lean syntax.
*/
template <typename...Ps> struct LambdaBroadcaster final
{
	/** Creates a broadcaster. */
	LambdaBroadcaster() :
		updater(*this)
	{
	};

	/** This will cancel all pending callbacks. */
	~LambdaBroadcaster()
	{
		updater.cancelPendingUpdate();
		lockfreeUpdater = nullptr;

        removeAllListeners();
	}

	/** Use this method to add a listener to this class. You can use any object as obj. The second parameter
		can be either a function pointer to a static function or a lambda with the signature

		void callback(T& obj, Ps... parameters)

		You don't need to bother about removing the listeners, they are automatically removed as soon before a
		message is being sent out.

		It will also fire the callback once with the last value so that the object will be initialised correctly.
	*/
	template <typename T, typename F> void addListener(T& obj, const F& f, bool sendWithInitialValue=true)
	{
        {
            removeDanglingObjects();
            
            auto t = new SafeLambda<T, void, Ps...>(obj, f);
            SimpleReadWriteLock::ScopedWriteLock sl(lock);
            listeners.add(t);

			if (lockfreeUpdater != nullptr && !lockfreeUpdater->isTimerRunning())
				lockfreeUpdater->start();
        }

		if(sendWithInitialValue)
			std::apply(*listeners.getLast(), lastValue);
	}

	/** Returns the number of listeners with the given class T (!= not a base class) that are registered to this object. */
	template <typename T> int getNumListenersWithClass() const
	{
		using TypeToLookFor = SafeLambda<T, void, Ps...>;

		int numListeners = 0;

		for (auto l : listeners)
		{
			if (l->isValid() && dynamic_cast<TypeToLookFor*>(l) != nullptr)
				numListeners++;
		}

		return numListeners;
	}

	/** Removes all callbacks for the given object. 
	
		You don't need to call this method unless you explicitely want to stop listening 
		for a certain object as the broadcaster class will clean up dangling objects automatically. 
	*/
	template <typename T> bool removeListener(T& obj)
	{
		bool found = false;

        SimpleReadWriteLock::ScopedWriteLock sl(lock);
        
		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i]->matches(&obj))
			{
				listeners.remove(i--);
				found = true;
			}
		}

		if (listeners.isEmpty() && lockfreeUpdater != nullptr)
			lockfreeUpdater->stop();

		removeDanglingObjects();

		return true;
	}

	void removeAllListeners()
	{
        OwnedArray<SafeLambdaBase<void, Ps...>> pendingDelete;
        
		SimpleReadWriteLock::ScopedWriteLock sl(lock);
        std::swap(listeners, pendingDelete);

		if (lockfreeUpdater != nullptr)
			lockfreeUpdater->stop();
	}

	void enableLockFreeUpdate(PooledUIUpdater* updater)
	{
		if (updater != nullptr && lockfreeUpdater != nullptr)
			lockfreeUpdater = new LockFreeUpdater(*this, updater);
	}

	void resendLastMessage(NotificationType n)
	{
		sendMessageInternal(n, lastValue);
	}

	/** Sends a message to all registered listeners. Be aware that this call is not realtime safe as this class
		is supposed to be used for non-audio related tasks.
	*/
	void sendMessage(NotificationType n, Ps... parameters)
	{
		lastValue = std::make_tuple(parameters...);
        
        if(!listeners.isEmpty())
            sendMessageInternal(n, lastValue);
	}

	/** By default, the lambda broadcaster will be called only with the last element whic
	    might result in skipped notifications if there are multiple calls to sendMessage in between. 
		
		Use this method to enable a lockfree queue that will ensure that all values that have been send
		with sendMessage will result in a notification to all listeners. 
	*/
	void setEnableQueue(bool shouldUseQueue, int numElements=512)
	{
		if (shouldUseQueue)
			valueQueue = new LockfreeQueue<std::tuple<Ps...>>(numElements);
		else
			valueQueue = nullptr;
	}

    /** During the execution of the send message the listener list will be locked by default so that
        adding / removing listeners while the message executes will cause a lock. This can be disabled
        with this function so that it will take a copy of the listener list before sending the message so
        that the listener list is not locked during the execution of the listener callbacks.
    */
    void setLockListenersDuringMessage(bool lockFreeListener)
    {
        lockFreeSendMessage = lockFreeListener;
    }

	bool hasListeners() const noexcept
	{
		return !listeners.isEmpty();
	}

	template <int P=0> auto getLastValue() const noexcept
	{
		return std::get<P>(lastValue);
	}

private:
    
	void sendMessageInternal(NotificationType n, const std::tuple<Ps...>& value)
	{
        if(n == dontSendNotification)
            return;
        
		if (valueQueue != nullptr)
			valueQueue.get()->push(value);

		if (n != sendNotificationAsync)
			sendInternal();
		else
		{
			if (lockfreeUpdater != nullptr)
				lockfreeUpdater->triggerAsyncUpdate();
			else
				updater.triggerAsyncUpdate();
		}
	}

	void removeDanglingObjects()
	{
		for (int i = 0; i < listeners.size(); i++)
		{
			if (!listeners[i]->isValid())
			{
				SimpleReadWriteLock::ScopedWriteLock sl(lock);
				listeners.remove(i--);
			}
		}
	}

    using ListenerType = SafeLambdaBase<void, Ps...>;
    
	std::tuple<Ps...> lastValue;

    void sendInternalForArray(ListenerType** l, int numListeners)
    {
        if (valueQueue != nullptr)
        {
            valueQueue.get()->callForEveryElementInQueue([&](std::tuple<Ps...>& v)
            {
                for(int i = 0; i < numListeners; i++)
                {
                    if (l[i]->isValid())
                        std::apply(*l[i], v);
                }
                
                return true;
            });
        }
        else
        {
            for(int i = 0; i < numListeners; i++)
            {
                if (l[i]->isValid())
                    std::apply(*l[i], lastValue);
            }
        }
    }
    
	void sendInternal()
	{
		removeDanglingObjects();

        if(lockFreeSendMessage)
        {
            int numListeners = listeners.size();
            
            auto tempData = (ListenerType**)alloca(sizeof(ListenerType*) * numListeners);
            
            if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
            {
                numListeners = jmin(numListeners, listeners.size());
                memcpy(tempData, listeners.begin(), numListeners * sizeof(ListenerType*));
            }
            else
            {
                updater.triggerAsyncUpdate();
                return;
            }
            
            sendInternalForArray(tempData, numListeners);
        }
        else
        {
            if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
            {
                sendInternalForArray(listeners.begin(), listeners.size());
            }
            else
                updater.triggerAsyncUpdate();
        }
        
        
        
        
        
		
	}

	struct Updater : public AsyncUpdater
	{
		Updater(LambdaBroadcaster& p) :
			parent(p)
		{};

		void handleAsyncUpdate() override
		{
			parent.sendInternal();
		};

		LambdaBroadcaster& parent;
	} updater;

	struct LockFreeUpdater : public PooledUIUpdater::SimpleTimer
	{
		LockFreeUpdater(LambdaBroadcaster& p, PooledUIUpdater* updater) :
			SimpleTimer(updater),
			parent(p)
		{
			if(!p.listeners.isEmpty())
				start();
		};

		void timerCallback() override
		{
			if (dirty.load())
			{
				// set it before since one of the callbacks could send a message again
				dirty.store(false);
				parent.sendInternal();
			}
		}

		void triggerAsyncUpdate()
		{
			dirty.store(true);
		}

		LambdaBroadcaster& parent;

		std::atomic<bool> dirty = { false };
	};

	ScopedPointer<LockFreeUpdater> lockfreeUpdater;

	ScopedPointer<hise::LockfreeQueue<std::tuple<Ps...>>> valueQueue;

    bool lockFreeSendMessage = false;
	hise::SimpleReadWriteLock lock;
                      
                      
                      
	OwnedArray<ListenerType> listeners;
};



struct ComplexDataUIBase : public ReferenceCountedObject
{
    using Ptr = ReferenceCountedObjectPtr<ComplexDataUIBase>;
    using List = ReferenceCountedArray<ComplexDataUIBase>;
    
	/** A listener that will be notified about changes of the complex data source. */
	struct SourceListener
	{
		virtual ~SourceListener();;

		virtual void sourceHasChanged(ComplexDataUIBase* oldSource, ComplexDataUIBase* newSource) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SourceListener);
	};

	struct EditorBase
	{
		virtual ~EditorBase();;

		virtual void setComplexDataUIBase(ComplexDataUIBase* newData) = 0;

		virtual void setSpecialLookAndFeel(LookAndFeel* l, bool shouldOwn = false);

		template <typename T> T* getSpecialLookAndFeel()
        {
            return dynamic_cast<T*>(laf);
        }

	private:

		ScopedPointer<LookAndFeel> ownedLaf;
		LookAndFeel* laf = nullptr;
	};

	/** A SourceWatcher notifies its registered listeners about changes to a source. */
	struct SourceWatcher
	{
		void setNewSource(ComplexDataUIBase* newSource);

		void addSourceListener(SourceListener* l);

		void removeSourceListener(SourceListener* l);

	private:

		Array<WeakReference<SourceListener>> listeners;

		WeakReference<ComplexDataUIBase> currentSource;
	};

	virtual ~ComplexDataUIBase();;

	void setGlobalUIUpdater(PooledUIUpdater* updater);

    void sendDisplayIndexMessage(float n);

    virtual bool fromBase64String(const String& b64) = 0;
	virtual String toBase64String() const = 0;

	ComplexDataUIUpdaterBase& getUpdater();;
	const ComplexDataUIUpdaterBase& getUpdater() const;;

	void setUndoManager(UndoManager* managerToUse);

    UndoManager* getUndoManager(bool useUndoManager = true);;

	hise::SimpleReadWriteLock& getDataLock() const;

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

    static Array<Range<int>> findRangesThatMatchWildcard(const String& regexWildcard, const String& stringToTest);
    
	/** Searches a string and returns a StringArray with all matches.
	*	You can specify and index of a capture group (if not, the entire match will be used). */
	static StringArray search(const String& wildcard, const String &stringToTest, int indexInMatch = 0);

	/** Returns the first match of the given wildcard in the test string. The first entry will be the whole match, followed by capture groups. */
	static StringArray getFirstMatch(const String &wildcard, const String &stringToTest);

	/** Checks if the given string matches the regex wildcard. */
	static bool matchesWildcard(const String &wildcard, const String &stringToTest);

};









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

struct ScrollbarFader : public Timer,
                        public ScrollBar::Listener
{
    ScrollbarFader();

    ~ScrollbarFader();

    struct Laf : public LookAndFeel_V4
    {
        void drawScrollbar(Graphics& g, ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown);

        void drawStretchableLayoutResizerBar (Graphics& g, int w, int h, bool /*isVerticalBar*/,
                                                              bool isMouseOver, bool isMouseDragging);

        Colour bg = Colours::transparentBlack;
    } slaf;
    
    void timerCallback() override;

    void startFadeOut();

    void scrollBarMoved(ScrollBar* sb, double ) override;

    bool fadeOut = false;

    void addScrollBarToAnimate(ScrollBar& b);

    Array<Component::SafePointer<ScrollBar>> scrollbars;
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

/** A class that handles tempo syncing.
*	@ingroup utility
*
*	All methods are static and it holds no data, so you have to get the host bpm before
*	you can use this class.
*
*	You can use the slider mode TempoSync, which automatically maps the slider values
*	to the tempo indexes and shows the corresponding text representation.
*
*	If the supplied hostTempo is invalid (= 0.0), a default tempo of 120.0 is used.
*/
class TempoSyncer
{
public:

	/** The note values. */
	enum Tempo
	{
#if HISE_USE_EXTENDED_TEMPO_VALUES
		EightBar = 0,
		SixBar,
		FourBar,
		ThreeBar,
		TwoBars,
		Whole,
#else
		Whole = 0, ///< a whole note (1/1)
#endif
		HalfDuet, ///< a half note duole (1/2D)
		Half, ///< a half note (1/2)
		HalfTriplet, ///< a half triplet note (1/2T)
		QuarterDuet, ///< a quarter note duole (1/4D)
		Quarter, ///< a quarter note (1/4)
		QuarterTriplet, ///< a quarter triplet note (1/4T)
		EighthDuet, ///< a eight note duole (1/8D)
		Eighth, ///< a eighth note (1/8)
		EighthTriplet, ///< a eighth triplet note (1/8T)
		SixteenthDuet, ///< a sixteenth duole (1/16D)
		Sixteenth, ///< a sixteenth note (1/16)
		SixteenthTriplet, ///< a sixteenth triplet (1/16T)
		ThirtyTwoDuet, ///< a 32th duole (1/32D)
		ThirtyTwo, ///< a 32th note (1/32)
		ThirtyTwoTriplet, ///< a 32th triplet (1/32T)
		SixtyForthDuet, ///< a 64th duole (1/64D)
		SixtyForth, ///< a 64th note (1/64)
		SixtyForthTriplet, ///> a 64th triplet 1/64T)
		numTempos
	};

	/** Returns the sample amount for the specified tempo. */
	static double getTempoInSamples(double hostTempoBpm, double sampleRate, Tempo t);;

	static double getTempoInSamples(double hostTempoBpm, double sampleRate, float tempoFactor);

	static StringArray getTempoNames();;

	/** Returns the time for the specified tempo in milliseconds. */
	static float getTempoInMilliSeconds(double hostTempoBpm, Tempo t);;

	/** Returns the tempo as frequency (in Hertz). */
	static float getTempoInHertz(double hostTempoBpm, Tempo t);

	/** Returns the name of the tempo with the index 't'. */
	static String getTempoName(int t);;

	/** Returns the next Tempo index for the given time.
	*
	*	This is not a super fast operation, but it helps with dealing between the
	*	two realms.
	*/
	static Tempo getTempoIndexForTime(double currentBpm, double milliSeconds);

	/** Returns the index of the tempo with the name 't'. */
	static Tempo getTempoIndex(const String &t);;

	/** Fills the internal arrays. Call this on application start. */
	static void initTempoData();

	static float getTempoFactor(Tempo t);;

private:

	using TempoString = char[6];

	static TempoString tempoNames[numTempos];
	static float tempoFactors[numTempos];

};

struct MasterClock
{
	enum class State
	{
		Idle,
		InternalClockPlay,
		ExternalClockPlay,
		numStates
	};

	enum class SyncModes
	{
		Inactive, //> No syncing going on
		ExternalOnly, //< only reacts on external clock events
		InternalOnly, //< only reacts on internal clock events
		PreferInternal, //< override the clock value with the internal clock if it plays
		PreferExternal, //< override the clock value with the external clock if it plays
		SyncInternal, //< sync the internal clock when external playback starts
		numSyncModes
	};

	void setNextGridIsFirst();

	void setSyncMode(SyncModes newSyncMode);

	bool changeState(int timestamp, bool internalClock, bool startPlayback);

	struct GridInfo
	{
		bool change = false;
		bool firstGridInPlayback = false;
        bool resync = false;
		int16 timestamp;
		int gridIndex;
	};

	GridInfo processAndCheckGrid(int numSamples, const AudioPlayHead::CurrentPositionInfo& externalInfo);

	bool isPlaying() const;

	SyncModes getSyncMode() const;

	GridInfo updateFromExternalPlayHead(const AudioPlayHead::CurrentPositionInfo& info, int numSamples);

	AudioPlayHead::CurrentPositionInfo createInternalPlayHead();

	void checkInternalClockForExternalStop(AudioPlayHead::CurrentPositionInfo& infoToUse, const AudioPlayHead::CurrentPositionInfo& externalInfo);

	double getBpmToUse(double hostBpm, double internalBpm) const;

	void prepareToPlay(double newSampleRate, int blockSize);

	void setBpm(double newBPM);

	void setLinkBpmToSyncMode(bool should);

	TempoSyncer::Tempo getCurrentClockGrid() const;

	bool allowExternalSync() const;

	void setStopInternalClockOnExternalStop(bool shouldStop);

	bool shouldCreateInternalInfo(const AudioPlayHead::CurrentPositionInfo& externalInfo) const;

	void setClockGrid(bool enableGrid, TempoSyncer::Tempo t);

	bool isGridEnabled() const;

	double getPPQPos(int timestampFromNow) const;

	void reset();

private:

	void updateGridDelta();

	bool shouldPreferInternal() const;

	bool gridEnabled = false;
	TempoSyncer::Tempo clockGrid = TempoSyncer::numTempos;

	SyncModes currentSyncMode = SyncModes::Inactive;
	
	int64 uptime = 0;
	int samplesToNextGrid = 0;
	int gridDelta = 1;
	int currentGridIndex = 0;

    int currentBlockSize = 512;
    
	bool internalClockIsRunning = false;

    bool stopInternalOnExternalStop = false;
	bool externalClockWasPlayingLastTime = false;
    
	bool linkBpmToSync = false;

	double sampleRate = 44100.0;
	double bpm = 120.0;

	int nextTimestamp = 0;
	State currentState = State::Idle;
	State nextState = State::Idle;

	bool waitForFirstGrid = false;
};

/** This class is a listener class that can react to tempo changes.
*	@ingroup utility
*
*	In order to use this, subclass this and implement the behaviour in the tempoChanged callback.
*/
class TempoListener
{
public:

	enum CallbackTypes
	{
		TempoChange,
		TranportChange,
		MusicalPositionChange,
		SignatureChange,
		numCallbackTypes
	};

	virtual ~TempoListener() {};

	/** The callback function that will be called if the tempo was changed.
	*
	*	This is called synchronously in the audio callback before the processing, so make sure you don't
	*	make something stupid here.
	*
	*	It will be called once per block, so you can't do sample synchronous tempo stuff, but that should be enough.
	*/
	virtual void tempoChanged(double newTempo) {};

	/** The callback function that will be called when the transport state changes (=the user presses play on the DAW). */
	virtual void onTransportChange(bool isPlaying, double ppqPosition) {};

    /** This callback will be called whenever the transport position needs to be resynced (eg. when the playback position was changed in the DAW or the playback position was wrapped around the loop range. */
    virtual void onResync(double ppqPosition) {};
    
	/** The callback function that will be called for each musical pulse.

		It takes the denominator from the time signature into account so if the host time signature is set to 6/8, it will
		be called twice as often as with 3/4.

		By default, this function is disabled, so you need to call addPulseListener() to activate this feature.
		*/
	virtual void onBeatChange(int beatIndex, bool isNewBar) {};

	/** The callback function that is called on every grid change. This can be used to implement sample accurate sequencers. 
	
		By default this is disabled so you need to call addPulseListener() to activate this feature.
	*/
	virtual void onGridChange(int gridIndex, uint16 timestamp, bool firstGridEventInPlayback) {};

	/** Called whenever a time signature change occurs. */
	virtual void onSignatureChange(int newNominator, int numDenominator) {};

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(TempoListener);
};




/** A interface class for getting notified when the realtime mode changed (eg. at DAW export). */
class NonRealtimeProcessor
{
public:

	virtual ~NonRealtimeProcessor() {};

	virtual void nonRealtimeModeChanged(bool isNonRealtime) = 0;
};

struct FFTHelpers
{
    enum WindowType
    {
        Rectangle,
		Triangle,
        Hamming,
        Hann,
        BlackmanHarris,
		Kaiser,
        FlatTop,
        numWindowType
    };
    
    static Array<WindowType> getAvailableWindowTypes();

    static Array<var> getAvailableWindowTypeNames()
    {
		Array<var> sa;

		for(auto w: getAvailableWindowTypes())
			sa.add(getWindowType(w));

		return sa;
    }

    static String getWindowType(WindowType w);

    static void applyWindow(WindowType t, AudioSampleBuffer& b, bool normalise=true);
    
    static void applyWindow(WindowType t, float* d, int size, bool normalise=true);
    
	static float getFreqForLogX(float xPos, float width);

	static float getPixelValueForLogXAxis(float freq, float width);

	static void toComplexArray(const AudioSampleBuffer& phaseBuffer, const AudioSampleBuffer& magBuffer, AudioSampleBuffer& out);

    static void toPhaseSpectrum(const AudioSampleBuffer& inp, AudioSampleBuffer& out);

    static void toFreqSpectrum(const AudioSampleBuffer& inp, AudioSampleBuffer& out);

    static void scaleFrequencyOutput(AudioSampleBuffer& b, bool convertToDb, bool invert=false);
};

struct Spectrum2D
{
	struct LookupTable: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<LookupTable>;

		enum class ColourScheme
		{
			blackWhite,
			rainbow,
			violetToOrange,
			hiseColours,
			preColours,
			numColourSchemes
		};

		static StringArray getColourSchemes();

		void setColourScheme(ColourScheme cs);

		static constexpr int LookupTableSize = 512;

		PixelRGB getColouredPixel(float normalisedInput);

		LookupTable();

		ColourGradient grad;
		ColourScheme colourScheme;
		juce::PixelARGB data[LookupTableSize];
	};

	struct Parameters: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Parameters>;

		void setFromBuffer(const AudioSampleBuffer& originalSource);

		struct Editor : public Component,
		                public ComboBox::Listener
		{
			static constexpr int RowHeight = 32;

			Editor(Parameters::Ptr p);

			void addEditor(const Identifier& id);

			void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

			void paint(Graphics& g) override;

			void resized() override;

			OwnedArray<ComboBox> editors;
			OwnedArray<Label> labels;

			ScopedPointer<LookAndFeel> claf;

			Parameters::Ptr param;
		};

		Parameters():
		  lut(new LookupTable())
		{
			
		}

		void set(const Identifier& id, var value, NotificationType n);

		var get(const Identifier& id) const;

		void saveToJSON(var v) const;
		void loadFromJSON(const var& v);

		static Array<Identifier> getAllIds();

		LambdaBroadcaster<Identifier, int> notifier;

        int minDb = 110;
		int order;
		int oversamplingFactor = 4;
		int Spectrum2DSize;

		int gainFactorDb = 1000;
		int gammaPercent = 60;

		float getGamma() const
		{
			return (float)gammaPercent / 100.0f;
		}

		float getGainFactor() const
		{
			if(gainFactorDb == 1000)
				return 0.0f;

			return Decibels::decibelsToGain(gainFactorDb);
		}

		Graphics::ResamplingQuality quality = Graphics::ResamplingQuality::lowResamplingQuality;

		FFTHelpers::WindowType currentWindowType = FFTHelpers::WindowType::BlackmanHarris;

		LookupTable::Ptr lut;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Parameters);
	};

    struct Holder
    {
        virtual ~Holder();;

		virtual Parameters::Ptr getParameters() const = 0;

		virtual float getXPosition(float input) const;

        virtual float getYPosition(float input) const;

    private:
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(Holder);
    };
    
    Spectrum2D(Holder* h, const AudioSampleBuffer& s);;

	static void draw(Graphics& g, const Image& img, Rectangle<int> area, Graphics::ResamplingQuality quality);

	Parameters::Ptr parameters;
    WeakReference<Holder> holder;
    const AudioSampleBuffer& originalSource;
    
    Image createSpectrumImage(AudioSampleBuffer& lastBuffer);

	bool useAlphaChannel = false;

    AudioSampleBuffer createSpectrumBuffer();
};

/** A interface class that can attach mouse events to the JSON object provided in the mouse event callback of a broadcaster. */
struct ComponentWithAdditionalMouseProperties
{
    virtual ~ComponentWithAdditionalMouseProperties() {};
    
    /** Override this method and add more properties to the event callback object. This is used when a Broadcaster is attached to the mouse events of this component. */
    virtual void attachAdditionalMouseProperties(const MouseEvent& e, var& obj) = 0;
    
    /** Call this method with a mouse event and it will go up the parent hierarchy and call attachAdditionalMouseProperties() if possible. */
    static void attachMousePropertyFromParent(const MouseEvent& e, var& obj)
    {
        auto c = e.eventComponent;
        
        using LongName = ComponentWithAdditionalMouseProperties;
        
        if(auto typed = dynamic_cast<LongName*>(c))
        {
            typed->attachAdditionalMouseProperties(e, obj);
            return;
        }
        
        if(auto typedChild = c->findParentComponentOfClass<LongName>())
        {
            typedChild->attachAdditionalMouseProperties(e, obj);
        }
    }
};


class SemanticVersionChecker
{
public:

    SemanticVersionChecker(const String& oldVersion_, const String& newVersion_);;

	SemanticVersionChecker(const std::array<int, 3>& oldVersion_, const std::array<int, 3>& newVersion_);

    bool isUpdate() const;
	bool isExactMatch() const
	{
		return newVersion.validVersion && newVersion == oldVersion;
	}

    bool isMajorVersionUpdate() const;;
    bool isMinorVersionUpdate() const;;
    bool isPatchVersionUpdate() const;;
    bool oldVersionNumberIsValid() const;
    bool newVersionNumberIsValid() const;

	String getErrorMessage(const String& oldVersionName, const String& newVersionName) const
	{
	    String m;
		m << oldVersionName << ": " << oldVersion.toString();
		m << ", " << newVersionName << ": " << newVersion.toString();
		return m;
	}

private:

    struct VersionInfo
    {
		bool operator== (const VersionInfo& other) const
		{
		    return majorVersion == other.majorVersion &&
				   minorVersion == other.minorVersion &&
				   patchVersion == other.patchVersion;
		}

		String toString() const
		{
		    String m;
			m << String(majorVersion) << "." << String(minorVersion) << "." << String(patchVersion);
			return m;
		}

        bool validVersion = false;
        int majorVersion = 0;
        int minorVersion = 0;
        int patchVersion = 0;
    };

    static void parseVersion(VersionInfo& info, const String& v);;

    VersionInfo oldVersion;
    VersionInfo newVersion;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SemanticVersionChecker);
};

struct AdditionalEventStorage
{
	using BroadcasterType = LambdaBroadcaster<uint16, uint8, double>;

	static constexpr uint16 NumEventSlots = 1024;
	static constexpr uint8 NumDataSlots = 16;
	
	void setValue(uint16 eventId, uint8 slotIndex, double newValue, NotificationType n)
	{
		auto i1 = eventId & (NumEventSlots -1);
		auto i2 = slotIndex & (NumDataSlots - 1);

		auto& element = data[i1][i2];

		element.first = eventId;
		element.second = newValue;

		getBroadcaster().sendMessage(n, eventId, slotIndex, newValue);
	}

	std::pair<bool, double> getValue(uint16 eventId, uint8 slotIndex) const
	{
		auto i1 = eventId & (NumEventSlots -1);
		auto i2 = slotIndex & (NumDataSlots - 1);

		auto& element = data[i1][i2];

		if(element.first == eventId)
		{
			return { true, element.second };
		}

		return { false, 0.0 };
	}

	bool changed(uint16 eventId, uint8 slotIndex, double& value) const
	{
		auto nv = getValue(eventId, slotIndex);

		if(nv.first && nv.second != value)
		{
			value = nv.second;
			return true;
		}

		return false;
	}
	
	BroadcasterType& getBroadcaster() { return broadcaster; }

private:

	BroadcasterType broadcaster;
	std::array<std::array<std::pair<uint16, double>, NumDataSlots>, NumEventSlots> data;
};

}
