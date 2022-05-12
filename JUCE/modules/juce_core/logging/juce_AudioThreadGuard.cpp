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

#if JUCE_ENABLE_AUDIO_GUARD


struct AudioThreadGuard::GlobalData
{
	GlobalData():
		suspended(false),
		currentHandler(nullptr)
	{
		audioThreadIds.ensureStorageAllocated(32);
	}

	~GlobalData()
	{
		jassert(audioThreadIds.size() == 0);
		jassert(currentHandler == nullptr);
	}

	bool suspended = false;
	Handler* currentHandler = nullptr;
	Array<void*, DummyCriticalSection, 32> audioThreadIds;
	ReadWriteLock arrayLock;

	// We'll leave with this leak..
	// JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GlobalData);
};

juce::AudioThreadGuard::GlobalData& AudioThreadGuard::getGlobalData()
{
	if (instance == nullptr)
	{
		instance = new GlobalData();
	}
	
	return *instance;
}

AudioThreadGuard::GlobalData* AudioThreadGuard::instance = nullptr;

AudioThreadGuard::AudioThreadGuard()
{
	auto id = Thread::getCurrentThreadId();
	auto& d = getGlobalData();

	d.audioThreadIds.ensureStorageAllocated(32);
	d.audioThreadIds.addIfNotAlreadyThere(id);
}

AudioThreadGuard::AudioThreadGuard(Handler* currentInstance)
{
	if (currentInstance != nullptr && previousInstance == currentInstance)
	{
		// Either you didn't clean up correctly or you're hitting
		jassertfalse;
	}

	auto& d = getGlobalData();

	previousInstance = d.currentHandler;
	useCustomInstance = true;
	setHandler(currentInstance);

	auto id = Thread::getCurrentThreadId();
	d.audioThreadIds.addIfNotAlreadyThere(id);
}

AudioThreadGuard::~AudioThreadGuard()
{
	if (useCustomInstance)
		setHandler(previousInstance);

	auto id = Thread::getCurrentThreadId();
	getGlobalData().audioThreadIds.removeAllInstancesOf(id);
}

void AudioThreadGuard::warnIf(bool condition, int op)
{
	if (condition)
	{
		warn(op);
	}
}

void AudioThreadGuard::warn(int op)
{
	if (instance == nullptr)
	{
		// Prevent recursive calls during initialisation
		return;
	}

	auto& d = getGlobalData();

	if (d.suspended)
		return;

	if (d.currentHandler == nullptr)
		return;

	if (!isAudioThread())
		return;

	if (d.currentHandler->test())
	{
		// You might want to do log something so we turn of the guard for this method.
		Suspender suspender;
		d.currentHandler->warn((int)op);
	}
}

void AudioThreadGuard::setHandler(Handler* newInstance)
{
	getGlobalData().currentHandler = newInstance;
}

bool AudioThreadGuard::isAudioThread()
{
	auto threadId = Thread::getCurrentThreadId();
	return getGlobalData().audioThreadIds.contains(threadId);
}

void AudioThreadGuard::deleteInstance()
{
	if (instance != nullptr)
	{
		// Use a temporary pointer and reset instance so that there are no recursive calls
		auto tmpInstance = instance;
		instance = nullptr;
		deleteAndZero(tmpInstance);
	}
}

AudioThreadGuard::Suspender::Suspender(bool shouldDoSomething /*= true*/)
{
	auto& d = AudioThreadGuard::getGlobalData();

	if (shouldDoSomething && isAudioThread() && d.currentHandler != nullptr && d.currentHandler->test())
	{
		d.suspended = true;
		isSuspended = true;
	}
}

AudioThreadGuard::Suspender::~Suspender()
{
	if (isSuspended)
		AudioThreadGuard::getGlobalData().suspended = false;
}

AudioThreadGuard::Handler::~Handler()
{
	jassert(AudioThreadGuard::getGlobalData().currentHandler != this);
}

juce::String AudioThreadGuard::Handler::getOperationName(int operationType)
{
	switch (operationType)
	{
	case IllegalAudioThreadOps::Unspecified:			return "Unspecified";
	case IllegalAudioThreadOps::HeapBlockAllocation:	return "HeapBlock allocation";
	case IllegalAudioThreadOps::HeapBlockDeallocation:	return "HeapBlock free";
	case IllegalAudioThreadOps::StringCreation:			return "String creation";
	case IllegalAudioThreadOps::AsyncUpdater:			return "AsyncUpdater call";
	case IllegalAudioThreadOps::MessageManagerLock:		return "MessageManager lock";
	case IllegalAudioThreadOps::BadLock:				return "Bad locking";
	default:
		break;
	}

	return {};
}

AudioThreadGuard::ScopedHandlerSetter::ScopedHandlerSetter(Handler* handler, bool doSomething) :
	previousHandler(AudioThreadGuard::getGlobalData().currentHandler)
{
	if (doSomething && isAudioThread())
	{
		isActive = true;
		AudioThreadGuard::setHandler(handler);
	}
	
}

AudioThreadGuard::ScopedHandlerSetter::~ScopedHandlerSetter()
{
	if(isActive && isAudioThread())
		AudioThreadGuard::setHandler(previousHandler);
}

AudioThreadGuard::ScopedLockType::ScopedLockType(const CriticalSection& lock, 
												 bool assertIfPriorityInversion /*= true*/) : 
	lock_(lock)
{
	// If you hit this assertion, it means you didn't create an AudioThreadGuard object
	// before creating this object.
	jassert(AudioThreadGuard::isAudioThread());

	if (assertIfPriorityInversion)
	{
		bool holdsLock = lock_.tryEnter();

		if (!holdsLock)
		{
			// If you hit this assertion, it means that you tried to gain the
			// lock in the audio thread but failed to do so because something
			// else is blocking.
			jassertfalse;
			
			lock_.enter();
		}
	}
	else
	{
		lock_.enter();
	}
}

AudioThreadGuard::ScopedLockType::~ScopedLockType()
{
	lock_.exit();
}


AudioThreadGuard::ScopedTryLockType::ScopedTryLockType(const CriticalSection& lock, bool assertIfPriorityInversion /*= true*/):
	lock_(lock)
{
	ignoreUnused(assertIfPriorityInversion);

	// If you hit this assertion, it means you didn't create an AudioThreadGuard object
	// before creating this object.
	jassert(AudioThreadGuard::isAudioThread());

	// Let's try this first
	holdsLock = lock_.tryEnter();
}

AudioThreadGuard::ScopedTryLockType::~ScopedTryLockType()
{
	if (holdsLock)
		lock_.exit();
}



#endif

} // namespace juce