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

static constexpr int NumMaxThreadsPerType = 8;

enum class ThreadType
{
	Unknown = 0,
	AudioThread,
	MessageThread,
	ScriptingThread,
	SampleThread,
	WorkerThreadOffset
};

struct ThreadInfo
{
	ThreadInfo()
	{
		memset(threadIds, 0, sizeof(void*) * NumMaxThreadsPerType);
	}
	void* threadIds[NumMaxThreadsPerType];
	uint8 threadType = 0;

	bool canHaveMultipleThreads() const noexcept { return threadType == (uint8)ThreadType::AudioThread; };
	bool isMessageThread()		  const noexcept { return threadType == (uint8)ThreadType::MessageThread; }
	bool isRealtime()			  const noexcept { return threadType == (uint8)ThreadType::AudioThread; }
	bool isAudioThread()		  const noexcept { return threadType == (uint8)ThreadType::AudioThread; }
	bool isScriptingThread()      const noexcept { return threadType == (uint8)ThreadType::ScriptingThread; }
	bool isLoadingThread()		  const noexcept { return threadType == (uint8)ThreadType::SampleThread; }
	bool isSpecialThread()		  const noexcept { return isScriptingThread() || isLoadingThread(); }
	bool canWait()				  const noexcept { return !isMessageThread() && !isRealtime(); }
	bool isWorkerThread()		  const noexcept { return threadType >= (uint8)ThreadType::WorkerThreadOffset; }
	bool isInitialised()		  const noexcept { return threadIds[0] != nullptr; }

	int getNumThreads()			  const noexcept
	{
		int numThreads = 0;
		while(isPositiveAndBelow(numThreads, NumMaxThreadsPerType) && threadIds[numThreads] != nullptr)
			numThreads++;
		return numThreads;
	}

	bool registerThread(void* newThread)
	{
		int numUsed = getNumThreads();

		if(isPositiveAndBelow(numUsed, NumMaxThreadsPerType))
		{
			threadIds[numUsed] = newThread;
			return true;
		}

		return false;
	}

	bool isMoreImportantThan(const ThreadType& otherType) const noexcept
	{
		if(threadType == (uint8)otherType)
			return false;

		jassertfalse; // soon
	}

};

// contains information about the properties and permissions of each thread

// keeps track of registered thread IDs
// TODO: Copy from KillStateHandler	
struct ThreadManager
{
	ThreadManager()
	{
		auto ok = registerThread((uint8)ThreadType::MessageThread, MessageManager::getInstance()->getCurrentMessageThread());
		jassert(ok);
	}

	std::array<ThreadInfo, 128> threadInfo;

	bool allSpecialThreadsInitialised() const
	{
		for(int i = 1; i < (int)ThreadType::WorkerThreadOffset; i++)
		{
			if(threadInfo[i].getNumThreads() == 0)
				return false;
		}

		return true;
	}

	uint8 getThreadIndex(void* threadId) const noexcept
	{
		jassert(allSpecialThreadsInitialised());

		for(uint8 i = 0; i < 128; i++)
		{
			for(int j = 0; j < threadInfo[i].getNumThreads(); j++)
			{
				if(threadInfo[i].threadIds[j] == threadId)
					return i;
			}
		}

		return 0;
	}

	uint8 getCurrentThreadIndex() const
	{
		return getThreadIndex(Thread::getCurrentThreadId());
	}

	bool deregisterThread(void* t)
	{
		// soon...
		jassertfalse;
	}

	bool registerThread(uint8 t, void* threadId)
	{
		if(isPositiveAndBelow(t, 128))
			return threadInfo[t].registerThread(threadId);

		return false;
	}

	bool registerCurrentThread(uint8 t)
	{
		registerThread(t, Thread::getCurrentThreadId());
	}
};



} // dispatch
} // hise