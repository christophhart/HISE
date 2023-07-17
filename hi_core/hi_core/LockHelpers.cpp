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

namespace hise {
using namespace juce;


bool LockHelpers::freeToGo(MainController* mc)
{
	if (mc->isBeingDeleted())
	{
		return true;
	}

	if (mc->getSampleManager().isNonRealtime())
		return true;

	if (!mc->isInitialised())
	{
		return true;
	}

#if USE_BACKEND
	if (mc->isFlakyThreadingAllowed())
	{
		return true;
	}
#endif

	if (!mc->getKillStateHandler().initialised())
	{
		// As long as it's not initialised, we're not that restrictive..
		return true;
	}


	if (!noMessageLockFromAnotherThread())
	{
		// you're holding the message lock
		jassertfalse;
		return false;
	}

	if (AudioThreadGuard::isAudioThread())
	{
		// We're rolling on the audio thread, which is really bad...
		jassertfalse;
		return false;
	}

	if (MessageManager::getInstance()->isThisTheMessageThread())
	{
		// you're calling this from the message thread
		//jassertfalse;
		return false;
	}

	if (mc->getKillStateHandler().isAudioRunning())
	{
		if (mc->getJavascriptThreadPool().isCurrentlySleeping())
			return true;

		// The audio engine is not suspended. Wrap this call
		// into a killVoicesAndCall lambda.
		jassertfalse;

		return false;
	}

	return true;
}

bool LockHelpers::noMessageThreadBeyondInitialisation(const MainController* mc)
{
	bool ok = !isMessageThreadBeyondInitialisation(mc);

	// If you hit this assertion, it means you have called a function
	// from the message thread that you are not supposed to do
	jassert(ok);

	return ok;

}

bool LockHelpers::isMessageThreadBeyondInitialisation(const MainController* mc)
{
#if USE_BACKEND
	if (CompileExporter::isExportingFromCommandLine())
		return false;
#endif

	if (!mc->isInitialised() || mc->isFlakyThreadingAllowed())
	{
		return false;
	}

	return MessageManager::getInstance()->isThisTheMessageThread();
}

bool LockHelpers::isLockedBySameThread(const MainController* mc, Type lockToCheck)
{
	if (lockToCheck == MessageLock)
		return MessageManager::getInstance()->currentThreadHasLockedMessageManager();
	else
		return mc->getKillStateHandler().currentThreadHoldsLock(lockToCheck);
}

void* LockHelpers::getCurrentThreadHandleOrMessageManager()
{
	if (MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread())
		return MessageManager::getInstanceWithoutCreating();
	else
		return Thread::getCurrentThreadId();
}

bool LockHelpers::isDuringInitialisation(const MainController* mc)
{
	return mc->isInitialised() && MessageManager::getInstance()->isThisTheMessageThread();
}

bool LockHelpers::noMessageLockFromAnotherThread()
{
	if (MessageManager::getInstance()->isThisTheMessageThread())
		return true;

	return !MessageManager::getInstance()->currentThreadHasLockedMessageManager();
}

const juce::CriticalSection& LockHelpers::getLockChecked(const MainController* mc, Type lockType)
{
	if (mc->isFlakyThreadingAllowed())
		return getLockUnchecked(mc, lockType);

	if (isLockedBySameThread(mc, lockType))
	{
		// The reentrant case should be handled before acquiring the lock.
		jassertfalse;
		throw BadLockException(lockType);
	}

	if (lockType == MessageLock)
	{
		// The message thread is not locked by a critical section, 
		//so calling this is stupid and you should be ashamed.
		jassertfalse;
		throw BadLockException(lockType);
	}

#if !JUCE_LINUX
	if (!noMessageLockFromAnotherThread())
	{
		// You can't acquire any lock from another thread than the message thread
		// while holding the message lock.
		jassertfalse;
		throw BadLockException(lockType);
	}
#endif

	if (lockType != IteratorLock)
	{
		for (int i = (int)lockType + 1; i < Type::numLockTypes; i++)
		{
			Type t = (Type)i;

			if (isLockedBySameThread(mc, t))
			{
				// If you hit this assertion, it means that you tried
				// to acquire a lock with a lower priority after a lock
				// with higher priority. This might cause deadlocks
				jassertfalse;
				throw BadLockException(lockType);
			}
		}
	}
	
	if (lockType == IteratorLock && isLockedBySameThread(mc, SampleLock))
	{
		// You can't hold the sample lock while trying to acquire the iterator lock
		jassertfalse;
		throw BadLockException(lockType);
	}

	// All checks passed, return the lock you wanted...
	return getLockUnchecked(mc, lockType);
}

const juce::CriticalSection& LockHelpers::getLockUnchecked(const MainController* mc, Type lockType)
{
	static CriticalSection dummy;

	switch (lockType)
	{
	case AudioLock:		return mc->getLockNew();
	case ScriptLock:	return mc->getJavascriptThreadPool().getLock();
	case SampleLock:	return mc->getSampleManager().getSampleLock();
	case IteratorLock:	return mc->getIteratorLock();
	default:
		break;
	}

	jassertfalse;
	return dummy;
}

LockHelpers::SafeLock::SafeLock(const MainController* mc_, Type t, bool useRealLock) :
	mc(mc_),
	type(t),
	holdsLock(false),
	lock(nullptr)
{
	if (useRealLock && t == Type::AudioLock)
	{
		// If you hit this assertion, it means that you didn't create an audio guard 
		// before trying to acquire the audio lock. This ensures that you won't do 
		// something stupid while the audio thread is locked.
		jassert(true);
	}

	if (useRealLock && !mc->getKillStateHandler().currentThreadHoldsLock(type))
	{
		try
		{
			lock = &getLockChecked(mc, type);

			if (lock != nullptr)
			{
				lock->enter();
				mc->getKillStateHandler().setLockForCurrentThread(type, true);
				holdsLock = true;
			}
		}
		catch (BadLockException& )
		{
			jassertfalse;
			lock = nullptr;
		}
	}
}

LockHelpers::SafeLock::~SafeLock()
{
	if (holdsLock)
	{
		jassert(lock != nullptr);
		mc->getKillStateHandler().setLockForCurrentThread(type, false);
		lock->exit();
	}
}


namespace SuspendHelpers
{

ScopedTicket::ScopedTicket(MainController* mc_) noexcept:
mc(mc_)
{
	request();
}


ScopedTicket::ScopedTicket(ScopedTicket&& other) noexcept
{
	std::swap(mc, other.mc);
	std::swap(ticket, other.ticket);
}

hise::SuspendHelpers::ScopedTicket& ScopedTicket::operator=(ScopedTicket&& other) noexcept
{
	std::swap(mc, other.mc);
	std::swap(ticket, other.ticket);

	return *this;
}

ScopedTicket::~ScopedTicket()
{
	invalidate();
}

void ScopedTicket::request()
{
	if (mc != nullptr && mc->isInitialised())
	{
		ticket = mc->getKillStateHandler().requestNewTicket();
	}
}

void ScopedTicket::invalidate()
{
	if (mc != nullptr && mc->isInitialised())
	{
		

		LockHelpers::freeToGo(mc);
		jassert(ticket != 0);
		mc->getKillStateHandler().invalidateTicket(ticket);
		mc = nullptr;
		ticket = 0;
	}
}

} // namespace SuspendHelpers




}
