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

#ifndef LOCKHELPERS_H_INCLUDED
#define LOCKHELPERS_H_INCLUDED

namespace hise {
using namespace juce;



class LockHelpers
{
public:

	/** These are the locks. They are sorted in ascending priority, which means
	that you can't acquire a lock with lower priority while holding a lock
	with higher priority on the same thread. This prevents priority inversion
	and deadlocks.

	You can of course acquire multiple locks in the correct order. In order
	to use this correctly, always use a SafeLock object instead of a standard
	ScopedLock.

	An exception is the MessageLock. If you're not on the message thread, you
	should lock the message lock for the shortest time possible. So whenever
	you try to obtain another lock on a different thread than the message thread
	while holding the message lock, it will throw a BadLockException.

	Another thing to know is that the internal priority of the IteratorLock and
	the SampleLock is equal, which means you can't acquire one of those while
	holding the other (there's no situation where you need both of those locks).
	*/
	enum class Type
	{
		MessageLock = 0,
		ScriptLock, ///< This lock will be held whenever a script is executed or compiled
		SampleLock, ///< This lock will be held whenever samples are added / removed.
		IteratorLock, ///< This lock will be held whenver you add / remove a processor to the chain or when the iterator is constructed.
		AudioLock, ///< This lock will be held during the audio callback.
		numLockTypes,
		unused
	};

	static Identifier getLockName(Type t)
	{
		switch (t)
		{
		case Type::MessageLock: return Identifier("MessageLock");
		case Type::ScriptLock: return Identifier("ScriptLock");
		case Type::SampleLock: return Identifier("SampleLock");
		case Type::IteratorLock: return Identifier("IteratorLock");
		case Type::AudioLock: return Identifier("AudioLock");
		default: return Identifier();;
		}
	}

	struct SafeLock
	{
		SafeLock(const MainController* mc, Type t, bool useRealLock = true);
		~SafeLock();

	private:

		MainController const* mc;
		Type type;
		bool holdsLock;
		CriticalSection const* lock;
	};

	struct BadLockException
	{
		enum class ErrorCode
		{
			LockedBySameThread,
			WhyULockMessageThread,
			MessageThreadIsLocked,
			PossibleDeadlock,
			SampleLockWhileIterating,
			numErrorCodes
		};

		String getErrorMessage() const
		{
			String m;

			m << "Error at acquiring ";

			m << getLockName(t);

			m << ": ";

			switch(c)
			{
			case ErrorCode::LockedBySameThread: m << "LockedBySameThread"; break;
			case ErrorCode::WhyULockMessageThread: m << "WhyULockMessageThread"; break;
			case ErrorCode::MessageThreadIsLocked: m << "MessageThreadIsLocked"; break;
			case ErrorCode::PossibleDeadlock: m << "PossibleDeadlock"; break;
			case ErrorCode::SampleLockWhileIterating: m << "SampleLockWhileIterating"; break;
			case ErrorCode::numErrorCodes: break;
			default: break;
			}

			return m;
		}

		BadLockException(Type t_, ErrorCode c_) :
			t(t_),
		    c(c_)
		{};

		Type t;
		ErrorCode c;
	};

	static bool freeToGo(MainController* mc);

	static bool noMessageThreadBeyondInitialisation(const MainController* mc);

	static bool isMessageThreadBeyondInitialisation(const MainController* mc);

	static bool isLockedBySameThread(const MainController* mc, Type lockToCheck);

	static void* getCurrentThreadHandleOrMessageManager();

	static bool isDuringInitialisation(const MainController* mc);

	/** Checks if the current thread (!= message thread) holds the message lock.

	On the message thread it also returns false. This method can be used to check for
	illegal locking in different threads. Call this method before you acquire one of the
	other locks to make sure there's no possible deadlock situation.
	*/
	static bool noMessageLockFromAnotherThread();

	/** Returns the lock for the given type. If it can't be acquired, it will throw a BadLockException.

	Normally, you don't call this method directly, but wrap it into BEGIN_SAFE_LOCKED_BLOCK /
	END_SAFE_LOCKED_BLOCK macros which resolve to a unchecked version if the locking is disabled.
	*/
	static const CriticalSection& getLockChecked(const MainController* mc, Type lockType);
	static const CriticalSection& getLockUnchecked(const MainController* mc, Type lockType);
};

namespace SuspendHelpers
{


class ScopedTicket
{
public:

	ScopedTicket(MainController* mc) noexcept;

	ScopedTicket(ScopedTicket&& other) noexcept;;

	ScopedTicket& operator=(ScopedTicket&& other) noexcept;

	ScopedTicket() noexcept {};

	~ScopedTicket();

	MainController* mc = nullptr;

	JUCE_DECLARE_NON_COPYABLE(ScopedTicket);

private:

	void request();
	void invalidate();

	uint16 ticket = 0;
};

struct FreeTicket
{
	FreeTicket(MainController* /*mc_*/) noexcept:
	mc(nullptr) // The (mc != nullptr) check is used to check if the engine should be suspended
	{};

	FreeTicket() noexcept:
	mc(nullptr)
	{};

	~FreeTicket()
	{};

	MainController* mc;
};

template <typename FunctionType, typename TicketType> struct Suspended
{
	Suspended() noexcept:
	pf(),
		ticket(nullptr)
	{};

	Suspended(FunctionType&& pf_, MainController* mc) noexcept:
	pf(pf_),
		ticket(TicketType(mc))
	{}

	~Suspended() noexcept {}

	Suspended(Suspended&& other) noexcept:
	pf(std::move(other.pf)),
		ticket(std::move(other.ticket))
	{};

	Suspended& operator=(Suspended&& other) noexcept
	{
		std::swap(pf, other.pf);
		std::swap(ticket, other.ticket);

		return *this;
	}

	Suspended& operator=(const Suspended& other) noexcept
	{
		pf = other.pf;
		std::swap(ticket, other.ticket);

		jassert(other.ticket.mc == nullptr);
		return *this;
	}

	Suspended(const Suspended& other) noexcept
	{
		pf = other.pf;
		std::swap(ticket, other.ticket);

		jassert(other.ticket.mc == nullptr);
	}

	Result call()
	{
		if (pf.isValid())
		{
			if (ticket.mc != nullptr)
				LockHelpers::freeToGo(ticket.mc);

			auto result = pf.callWithResult();

			return result;
		}

		return Result::fail("Invalid function call");
	}

	const FunctionType& getFunction() const noexcept { return pf; };

private:

	FunctionType pf;
	mutable TicketType ticket;
};

}


}


#endif
