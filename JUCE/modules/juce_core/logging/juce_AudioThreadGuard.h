/*
==============================================================================

This file is part of the JUCE library.
Copyright (c) 2017 - ROLI Ltd.

JUCE is an open source library subject to commercial or open-source
licensing.

The code included in this file is provided under the terms of the ISC license
http://www.isc.org/downloads/software-support-policy/isc-license. Permission
To use, copy, modify, and/or distribute this software for any purpose with or
without fee is hereby granted provided that the above copyright notice and
this permission notice appear in all copies.

JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
DISCLAIMED.

==============================================================================
*/

namespace juce 
{


namespace IllegalAudioThreadOps
{
	enum ID
	{
		Unspecified = 0,
		HeapBlockAllocation,
		HeapBlockDeallocation,
		StringCreation,
		AsyncUpdater,
		MessageManagerLock,
		BadLock,
		numIllegalOperationTypes
	};
}


    
#if JUCE_ENABLE_AUDIO_GUARD

class CriticalSection;
    
/** This class watches certain methods that are not supposed to be called in the
audio thread.

Eg: HeapBlock methods, String allocations, AsyncUpdates, dynamic Array
operations, OS calls

If you want to use this class:
1. enable JUCE_ENABLE_AUDIO_GUARD
2. inherit from AudioThreadGuard::Instance and implement a test() function
and a warn() function depending on your plugin architecture.
3. in your processBlock method, create a AudioThreadGuard so it
will just look in your code.

If you want to suspend the guard (eg. during printing debug strings), just create
a AudioThreadGuard::Suspender,
which will suspend the tests for its lifetime (@see DEBUG_WITH_AUDIO_GUARD())

You can exclude your own methods from being called on the audio thread by calling
the public methods warn() and warnIf()
*/
class JUCE_API AudioThreadGuard
{
public:

	//==============================================================================

	/** This is the base class for the implementation of your AudioThreadGuard. */
	struct Handler
	{
		/** Make sure you call setInstance(nullptr) before deleting this instance. */
		virtual ~Handler();;

		/** This method will be called when there's a illegal operation going on in
		the audio thread.

		You can log this, throw an assertion, delete C:\, whatever you think is appropriate.
		*/
		virtual void warn(int operationType) = 0;

		/** This method will be called on the audio thread if there is an illegal
		operation going on and can be used to run additional tests before firing
		the warning. You can use this to prevent false positives during
		initialisation.
		*/
		virtual bool test() const noexcept { return true; };

		/** If you use the AudioThreadGuard::warn() function in your own code, you
		can supply it with a custom code that will be resolved to a String in the
		case of a warning using this method.

		Make sure you call the base class method as it returns the Strings defined
		in AudioThreadGuard::IllegalOperationType
		*/
		virtual String getOperationName(int operationType);
	};

	//==============================================================================

	//==============================================================================

	/** Creates an AudioThreadGuard. Make sure you have set an instance before this.
	*/
	AudioThreadGuard();

	/** Creates an AudioThreadGuard and uses the given instance for the lifetime of
		the object.
	*/
	AudioThreadGuard(Handler* handler);

	~AudioThreadGuard();

	//==============================================================================

	/** This can be used to lock a CriticalSection with additional safety measures.
	
		It's a dropin-replacement for the standard ScopedLock, but offers a assertion
		if you're running into a priority inversion.
		It will also temporarily enable the same rules as the audio thread for the
		lifetime of this object. This means that you can't call any of the operations
		considered illegal in the thread that has created this object until it goes 
		out of scope (and releases the audio thread lock).

		Note that if you use this object, the method AudioThreadGuard::isAudioThread()
		will return true for the lifetime of the object (this indicates that you are
		in real-time-safe area)
	*/
	class ScopedLockType
	{
	public:

		/** Locks the given CriticalSection and increases the warning level for the 
			current thread until it goes out of scope.
			
			If you want to catch priority inversions during development, call
			assertIfPriorityInversion set to true.
		*/
		explicit ScopedLockType(const CriticalSection& lock, bool assertIfPriorityInversion=true);

		~ScopedLockType();

	private:

		const CriticalSection& lock_;
	};

	class ScopedTryLockType
	{
	public:

		explicit ScopedTryLockType(const CriticalSection& lock, bool assertIfPriorityInversion = true);

		~ScopedTryLockType();

		/** Returns true if the lock could be acquired.
		If you created this object with forceLock=true, this will always
		return true, but if not, this mimics the behaviour of
		ScopedTryLock::isLocked(). */
		bool isLocked() const noexcept { return holdsLock; }

	private:

		const CriticalSection& lock_;
		bool holdsLock = false;

	};

	//==============================================================================

	/** A RAII object that suspends the tests until it goes out of scope. */
	struct Suspender
	{
		/** Creates a suspender. 
			
			You can pass in false to bypass the suspension which is handy because
			it might prevent unnecessary branching
		*/
		Suspender(bool shouldDoSomething = true);

		~Suspender();

	private:

		bool isSuspended = false;
		JUCE_DECLARE_NON_COPYABLE(Suspender)
	};

	//==============================================================================

	/** This object can be used to temporarily switch the AudioThreadGuard logic
	during the lifetime of this object.

	You can use this to change the behaviour within a certain code path.
	It will restore the old handler when it goes out of scope.
	*/
	struct ScopedHandlerSetter
	{
		ScopedHandlerSetter(Handler* newHandler, bool doSomething=true);
		~ScopedHandlerSetter();

	private:

		bool isActive = false;
		Handler * previousHandler = nullptr;
		JUCE_DECLARE_NON_COPYABLE(ScopedHandlerSetter);
	};

	//==============================================================================

	/** This checks the condition and warns if it's called on the audio thread.

	Some operations are not necesserily evil (eg. calling free with a nullptr),
	so this gives you the possibility to avoid false positives.
	*/
	static void JUCE_CALLTYPE warnIf(bool condition, int operationType = 0);

	/** This checks if the method is executed on the audio thread. */
	static void JUCE_CALLTYPE warn(int operationType = 0);

	/** This sets the current AudioThreadGuard. Be aware that this is using a global
	static variable, so it's not super smart to do this in a multi-instance environment.
	*/
	static void JUCE_CALLTYPE setHandler(Handler* newInstance);

	/** Checks if we're rolling in the audio thread. */
	static bool JUCE_CALLTYPE isAudioThread();

	//==============================================================================

	/** Call this at the app shutdown to delete the global data object. */
	static void deleteInstance();

private:

	bool useCustomInstance = false;
	Handler* previousInstance = nullptr;

	//==============================================================================

	struct GlobalData;
	static GlobalData& getGlobalData();
	static GlobalData* instance;

	JUCE_DECLARE_NON_COPYABLE(AudioThreadGuard);

	//==============================================================================
};

using GuardedScopedLock = AudioThreadGuard::ScopedLockType;
using GuardedScopedTryLock = AudioThreadGuard::ScopedTryLockType;

#else

/** A dummy object that will most likely be compiled out of existence above O0. */
struct DummyAudioGuard
{
	struct Handler
	{
        virtual ~Handler() {};
        
		virtual bool test() const noexcept { jassertfalse; return false; }
		virtual void warn(int /*operationType*/)  { jassertfalse; }
		virtual String getOperationName(int /*operationType*/) { jassertfalse;  return {}; }
	};
	struct Suspender 
	{ 
		Suspender(bool /*unused*/) {}; 
		Suspender() = default;
	};

	struct ScopedHandlerSetter {};

	DummyAudioGuard() {};
	DummyAudioGuard(Handler* /*s*/) {};
	~DummyAudioGuard() {};

	static void deleteInstance() {};

	/** Checks if we're rolling in the audio thread. */
	static bool JUCE_CALLTYPE isAudioThread()
	{
		return false;
	}

	void warn(int /*operationType*/=0) {};
	void warnIf(bool /*condition*/, int /*operationType*/=0) {}
};

using AudioThreadGuard = DummyAudioGuard;

#endif

}
