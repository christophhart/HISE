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

namespace hise { using namespace juce;


MainController::KillStateHandler::KillStateHandler(MainController* mc_) :
	mc(mc_),
	currentState(State::WaitingForInitialisation)
{
	threadIds[(int)TargetThread::AudioThread] = nullptr;
	threadIds[(int)TargetThread::SampleLoadingThread] = mc->getSampleManager().getGlobalSampleThreadPool()->getThreadId();
	threadIds[(int)TargetThread::ScriptingThread] = nullptr;
	threadIds[(int)TargetThread::MessageThread] = nullptr;
	jassert(threadIds[(int)TargetThread::SampleLoadingThread] != nullptr);

	setCurrentExportThread(nullptr);
}



bool MainController::KillStateHandler::handleKillState()
{
#if ENABLE_LOG_KILL_EVENTS
	if (killState != NewKillState::Clear)
		LOG_KILL_EVENTS("      handleKillState: " + getStringForKillState(killState));
#endif

	initAudioThreadId();

	SimpleReadWriteLock::ScopedTryReadLock sl(ticketLock);

	if(!sl)
	{
		jassertfalse;
	}

	switch (currentState.load())
	{
	case State::Clear:
	{
		if (!checkForClearance())
		{
#if FRONTEND_IS_PLUGIN

			// no kill voice time for fx plugins needed.
			currentState = State::VoiceKill;
			return true;
#else
			currentState = VoiceKill;

			mc->getMainSynthChain()->killAllVoices();

			if (voicesAreKilled())
			{
				currentState = State::Suspended;
				return false;
			}
			else
			{
				return true;
			}
#endif
		}
		return true;
	}
	case State::VoiceKill:
	{
#if FRONTEND_IS_PLUGIN
		currentState = State::Suspended;
		return true;
#else
		if (voicesAreKilled())
		{
			currentState = State::Suspended;
			return false;
		}

		return true;
#endif
	}
	case State::WaitingForClearance:
		currentState = State::Clear;
		return true;
	case State::Suspended:
	{
		if (checkForClearance())
		{
#if FRONTEND_IS_PLUGIN
			currentState = State::WaitingForClearance;
			return true;
#else
			{
				DBG_WITH_AUDIO_GUARD("All tickets cleared. Resume processing");
			}
			
			mc->getMainSynthChain()->resetAllVoices();
			currentState = State::Clear;
			return true;
#endif
		}

		// Let the audio export thread through...
		if (threadIds[(int)TargetThread::AudioExportThread] == Thread::getCurrentThreadId())
			return true;

		return false;
	}
	case State::ShutdownSignalReceived:
	{
		currentState = PendingShutdown;
		mc->getMainSynthChain()->killAllVoices();

		if (voicesAreKilled())
		{
			quit();
			return false;
		}
		else
		{
			return true;
		}
	}
	case State::PendingShutdown:
	{
		jassert(allowGracefulExit());

		if (voicesAreKilled())
		{
			quit();
			return false;
		}

		return true;
	}
	case State::ShutdownComplete:
		return false;
	default:
		jassertfalse;
        return false;
	}

}



bool MainController::KillStateHandler::handleBufferDuringSuspension(AudioSampleBuffer& b)
{
	if (currentState == Clear)
		return true;

	if (currentState == VoiceKill)
	{
		b.applyGainRamp(0, b.getNumSamples(), 1.0f, 0.0f);
		return true;
	}

	if (currentState == WaitingForClearance)
	{
		b.applyGainRamp(0, b.getNumSamples(), 0.0f, 1.0f);
		return true;
	}

	if (currentState == Suspended)
	{
		b.clear();
		return false;
	}

	return true;
}

bool MainController::KillStateHandler::hasRequestedQuit() const
{
	return currentState >= State::ShutdownSignalReceived;
}



void MainController::KillStateHandler::requestQuit()
{
	if (allowGracefulExit())
	{
		currentState = State::ShutdownSignalReceived;
	}
	else
	{

		JUCEApplication::quit();
	}
}

bool MainController::KillStateHandler::allowGracefulExit() const noexcept
{
	if(currentState == State::InitialisedButNotActivated)
		return false;

#if JUCE_WINDOWS && IS_STANDALONE_APP
	const bool formatAllowsGracefulExit = true;
#else
	const bool formatAllowsGracefulExit = false;
#endif

	return initialised() && formatAllowsGracefulExit;
}

void MainController::KillStateHandler::quit()
{
	LockHelpers::SafeLock sl(mc, LockHelpers::Type::AudioLock);

	mc->getJavascriptThreadPool().deactivateSleepUntilCompilation();
	mc->getMainSynthChain()->resetAllVoices();
	currentState = ShutdownComplete;

	auto f = [](Dispatchable* )
	{
		JUCEApplication::quit();
		return Dispatchable::Status::OK;
	};

	mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(mc->getMainSynthChain(), f);
}



void MainController::KillStateHandler::deferToThread(Processor* p, const ProcessorFunction& f, TargetThread t)
{
	switch (t)
	{
	case TargetThread::SampleLoadingThread:
	{
		mc->getSampleManager().addDeferredFunction(p, f);
		break;
	}
	
	case TargetThread::ScriptingThread:
	{
		auto sf = [f](JavascriptProcessor* jp)
		{
			auto p = dynamic_cast<Processor*>(jp);
			f(p);
			return jp->getLastErrorMessage();
		};

		mc->getJavascriptThreadPool().addJob(JavascriptThreadPool::Task::Compilation, dynamic_cast<JavascriptProcessor*>(p), sf);

		break;
	}
	case TargetThread::AudioThread:
	{
        jassertfalse;
        break;
	}
	case TargetThread::MessageThread:
	default:
	{
		jassertfalse;
		break;
	}
	}
}

bool MainController::KillStateHandler::isAudioRunning() const noexcept
{
	if(currentState == State::InitialisedButNotActivated)
		return false;

	return initialised() && currentState < State::Suspended;
}

void MainController::KillStateHandler::setLockForCurrentThread(LockHelpers::Type t, bool lock) const
{
	auto id = lock ? getCurrentThread() : TargetThread::Free;
	lockStates.threadsForLock[(int)t].store(id);
}

bool MainController::KillStateHandler::currentThreadHoldsLock(LockHelpers::Type t) const noexcept
{
	return getCurrentThread() == lockStates.threadsForLock[(int)t];
}

bool MainController::KillStateHandler::initialised() const noexcept
{
	return init && !stateIsLoading && currentState != ShutdownComplete;
}

void MainController::KillStateHandler::deinitialise()
{
	currentState = State::PendingShutdown;
}

bool MainController::KillStateHandler::invalidateTicket(uint16 ticket)
{
	jassert(mc->isBeingDeleted() || 
			currentState == Suspended || 
			currentState == InitialisedButNotActivated);

	{
		SimpleReadWriteLock::ScopedWriteLock sl(ticketLock);
		
		// You tried to invalidate an invalid ticket!
		jassert(ticket != 0);

		auto index = pendingTickets.indexOf(ticket);
		pendingTickets.removeElement(index);
		return index != -1;
	}
}

static uint32 lastTime = 0;

juce::uint16 MainController::KillStateHandler::requestNewTicket()
{
	jassert(currentState != WaitingForInitialisation);

	auto thisTime = Time::getMillisecondCounter();

	auto delta = thisTime - lastTime;
	lastTime = thisTime;

	


	uint16 newTicket;

	ignoreUnused(delta);
	//DBG(String(delta) + "ms: Request Ticket " + String(ticketCounter + 1));

	{
		SimpleReadWriteLock::ScopedWriteLock sl(ticketLock);

		ticketCounter = jmax<uint16>(1, ticketCounter + 1);
		newTicket = ticketCounter;
		pendingTickets.insertWithoutSearch(newTicket);
	}
	
	//stackTraces.insertWithoutSearch(StackTrace<3, 6>(newTicket));

	return newTicket;
}

bool MainController::KillStateHandler::checkForClearance() const noexcept
{
	SimpleReadWriteLock::ScopedTryReadLock sl(ticketLock);
	
	if (sl)
		return pendingTickets.isEmpty();

	// If we didn't get the lock, there's certainly going on something, 
	// so we can't unsuspend the audio processing now.
	return false;
}

bool MainController::KillStateHandler::isSuspendableThread() const noexcept
{
	auto c = getCurrentThread();
	return c == TargetThread::ScriptingThread || c == TargetThread::SampleLoadingThread;
}

bool MainController::KillStateHandler::voicesAreKilled() const
{
	// This method should only be called during the voice killing state
	jassert(currentState == State::VoiceKill || currentState == PendingShutdown);

	return !mc->getMainSynthChain()->areVoicesActive();
}


bool MainController::KillStateHandler::killVoicesAndCall(Processor* p, const ProcessorFunction& functionToExecuteWhenKilled, MainController::KillStateHandler::TargetThread targetThread)
{
	WARN_IF_AUDIO_THREAD(true, IllegalOps::GlobalLocking);

#if USE_BACKEND
	if (CompileExporter::isExportingFromCommandLine())
	{
		functionToExecuteWhenKilled(p);
		return true;
	}
#endif

    if (!initialised())
	{
		jassert(currentState == State::WaitingForInitialisation || 
		        currentState == State::ShutdownComplete ||
                stateIsLoading);

		functionToExecuteWhenKilled(p);
		return true;
	}

	jassert(targetThread != TargetThread::MessageThread);

	const bool sameThread = getCurrentThread() == targetThread;

	if (inUnitTestMode() ||
        ( !isAudioRunning() && sameThread ) ||
        p->getMainController()->isFlakyThreadingAllowed())
	{
		// We either don't care about threading (unit-test) or the engine is idle
		// and the target thread is active, so we can call the function without
		// further actions
		functionToExecuteWhenKilled(p);
		return true;
	}
	else
	{
		if (isSuspendableThread() && sameThread)
		{
			// We are on a suspendable thread and the function
			// should be called here.

			if (isAudioRunning())
			{
				auto newTicket = requestNewTicket();

				if (killVoicesAndWait())
				{
					jassert(!isAudioRunning());

					functionToExecuteWhenKilled(p);
					
					jassert(currentState = State::Suspended);

					invalidateTicket(newTicket);

					return true;
				}
				else
				{
					// Somehow the voices could not been killed.
					jassertfalse;
					invalidateTicket(newTicket);
					return true;
				}
			}
		}
		else
		{
			deferToThread(p, functionToExecuteWhenKilled, targetThread);
			return false;
		}
	}

	jassertfalse;
	return false;
}


bool MainController::KillStateHandler::killVoicesAndWait(int* timeoutMilliseconds)
{
	if (!isSuspendableThread())
	{
		jassertfalse;
		return false;
	}

	if (currentState == Suspended)
	{
		if (timeoutMilliseconds != nullptr)
			*timeoutMilliseconds = 0;
		return true;
	}

	// If you call this method you need to have requested a ticket!
	jassert(!checkForClearance());

	int safeguard = 0;
	int timeout = timeoutMilliseconds != nullptr ? *timeoutMilliseconds : 1000;
	int numRetries = timeout / 20 + 10;

	while (isAudioRunning() && safeguard < numRetries)
	{
		Thread::sleep(20);
		safeguard++;
	}

	if (isAudioRunning())
	{
		jassertfalse;
		return false;
	}

	if(timeoutMilliseconds != nullptr)
		*timeoutMilliseconds = safeguard * numRetries;

	return true;
}

bool MainController::KillStateHandler::test() const noexcept
{
	if (mc->getMainSynthChain() == nullptr)
		return false;

	if (!mc->getMainSynthChain()->isOnAir())
		return false;

	if (audioThreads.isEmpty())
		return false;

	return true;
}


juce::Array<MultithreadedQueueHelpers::PublicToken> MainController::KillStateHandler::createPublicTokenList(int producerFlags /*= AllProducers*/)
{
	IF_NOT_HEADLESS(jassert(mc->isFlakyThreadingAllowed() || initialised()));

	WARN_IF_AUDIO_THREAD(true, IllegalOps::Compilation);

	/** You can't call this before initialising the audio threads. */
	IF_NOT_HEADLESS(jassert(mc->isFlakyThreadingAllowed() || !audioThreads.isEmpty()));

	MultithreadedQueueHelpers::PublicToken audioThreadToken;
	audioThreadToken.canBeProducer = producerFlags & QueueProducerFlags::AudioThreadIsProducer;
	audioThreadToken.threadIds.insertArray(0, audioThreads.begin(), audioThreads.size());
	audioThreadToken.threadName = "AudioThread";

	MultithreadedQueueHelpers::PublicToken messageThreadToken;
	messageThreadToken.canBeProducer = producerFlags & QueueProducerFlags::MessageThreadIsProducer;
	messageThreadToken.threadIds.insert(-1, MessageManager::getInstance()->getCurrentMessageThread());
	messageThreadToken.threadName = "Message Thread";

	MultithreadedQueueHelpers::PublicToken sampleLoadingThreadToken;
	sampleLoadingThreadToken.canBeProducer = producerFlags & QueueProducerFlags::LoadingThreadIsProducer;
	sampleLoadingThreadToken.threadIds.insert(-1, mc->getSampleManager().getGlobalSampleThreadPool()->getThreadId());
	sampleLoadingThreadToken.threadName = "Loading Thread";

	MultithreadedQueueHelpers::PublicToken scriptThreadToken;
	scriptThreadToken.canBeProducer = producerFlags & QueueProducerFlags::ScriptThreadIsProducer;
	scriptThreadToken.threadIds.insert(-1, mc->javascriptThreadPool->getThreadId());
	scriptThreadToken.threadName = "Scripting Thread";

	return { audioThreadToken, messageThreadToken, sampleLoadingThreadToken, scriptThreadToken };
}



bool MainController::KillStateHandler::voiceStartIsDisabled() const
{
#if HI_RUN_UNIT_TESTS
	return false;
#else
	return currentState != State::Clear && !isCurrentlyExporting();
#endif
}




void MainController::KillStateHandler::setSampleLoadingThreadId(void* newId)
{
	jassert(newId != nullptr);
	jassert(threadIds[(int)TargetThread::SampleLoadingThread] == nullptr);

	threadIds[(int)TargetThread::SampleLoadingThread] = newId;
}

MainController::KillStateHandler::TargetThread MainController::KillStateHandler::getCurrentThread() const
{
	// These must be set during the constructor of MainController
	jassert(threadIds[(int)TargetThread::SampleLoadingThread] != nullptr);
	jassert(threadIds[(int)TargetThread::ScriptingThread] != nullptr);

	auto threadId = Thread::getCurrentThreadId();

	if (audioThreads.contains(threadId))
		return TargetThread::AudioThread;
	else if (threadId == threadIds[(int)TargetThread::SampleLoadingThread].load())
		return TargetThread::SampleLoadingThread;
	else if (threadId == threadIds[(int)TargetThread::ScriptingThread].load())
		return TargetThread::ScriptingThread;

	if (auto mm = MessageManager::getInstanceWithoutCreating())
	{
		if (mm->isThisTheMessageThread())
			return TargetThread::MessageThread;
	}

#if JUCE_LINUX && !IS_STANDALONE_APP
	// Linux appears to be using multiple message threads, so this is the best bet...
	return TargetThread::MessageThread;
#else
	return TargetThread::UnknownThread;
#endif
}


void MainController::KillStateHandler::setCurrentExportThread(void* exportThread)
{
    auto& currentExportThread = threadIds[(int)TargetThread::AudioExportThread];
    
    if(currentExportThread != exportThread)
    {
        if(currentExportThread != nullptr)
            audioThreads.remove(currentExportThread);
        
        currentExportThread = exportThread;
        
        if(currentExportThread != nullptr)
            audioThreads.insert(currentExportThread);
    }
}

LockHelpers::Type MainController::KillStateHandler::getLockTypeForThread(TargetThread t)
{
	switch(t)
	{
	case TargetThread::MessageThread: 		return LockHelpers::Type::MessageLock;
	case TargetThread::ScriptingThread: 	return LockHelpers::Type::ScriptLock;
	case TargetThread::SampleLoadingThread: return LockHelpers::Type::SampleLock;
	case TargetThread::AudioThread: 		return LockHelpers::Type::AudioLock;
	case TargetThread::AudioExportThread: 	return LockHelpers::Type::AudioLock;
	case TargetThread::UnknownThread:		return LockHelpers::Type::numLockTypes;
	case TargetThread::Free:				return LockHelpers::Type::unused;
	default:								jassertfalse; return LockHelpers::Type::numLockTypes;
	}
}

MainController::KillStateHandler::TargetThread MainController::KillStateHandler::getThreadForLockType(
	LockHelpers::Type t)
{
	switch(t)
	{
	case LockHelpers::Type::MessageLock:				return TargetThread::MessageThread;
	case LockHelpers::Type::ScriptLock:					return TargetThread::ScriptingThread;
	case LockHelpers::Type::SampleLock:					return TargetThread::SampleLoadingThread;
	case LockHelpers::Type::IteratorLock: jassertfalse; return TargetThread::UnknownThread;
	case LockHelpers::Type::AudioLock:					return TargetThread::AudioThread;
	case LockHelpers::Type::numLockTypes: jassertfalse; return TargetThread::UnknownThread;
	case LockHelpers::Type::unused:						return TargetThread::Free;
	default:											return TargetThread::UnknownThread;
	}
}

void MainController::KillStateHandler::addThreadIdToAudioThreadList()
{
    if(MessageManager::getInstance()->isThisTheMessageThread())
        return;
    
	auto threadId = Thread::getCurrentThreadId();

	audioThreads.insert(threadId);
}

void MainController::KillStateHandler::removeThreadIdFromAudioThreadList()
{
	if (MessageManager::getInstance()->isThisTheMessageThread())
		return;

	auto threadId = Thread::getCurrentThreadId();

	audioThreads.remove(threadId);
}

void MainController::KillStateHandler::initAudioThreadId()
{
	addThreadIdToAudioThreadList();

	if (currentState == State::InitialisedButNotActivated)
	{
		currentState = State::Clear;
	}

	if (!init)
	{
		if (currentState == WaitingForInitialisation)
		{
			currentState = State::Clear;
		}
		
		init = true;

		BACKEND_ONLY(mc->getConsoleHandler().initialise());
	}


	
}


void MainController::KillStateHandler::warn(int operationType)
{
	if (guardEnabled)
	{
		String errorMessage = "Illegal call in audio thread detected: \n";

		errorMessage << getOperationName(operationType);

		DBG(errorMessage);

		//errorMessage << SystemStats::getStackBacktrace();

		debugError(mc->getMainSynthChain(), errorMessage);
	}
}

juce::String MainController::KillStateHandler::getOperationName(int operationType)
{
	auto t = (IllegalOps)operationType;

	switch (t)
	{
	case IllegalOps::ProcessorInsertion:	return "Processor insertion";
	case IllegalOps::IteratorCreation:		return "Iterator creation";
	case IllegalOps::Compilation:			return "Script compilation";
	case IllegalOps::SampleCreation:		return "Sample creation";
	case IllegalOps::SampleDestructor:		return "Sample deletion";
	case IllegalOps::ValueTreeOperation:	return "ValueTree operation";
	case IllegalOps::ProcessorDestructor:	return "Processor destructor";
	default:
		break;
	}

	return Handler::getOperationName(operationType);
}

#undef LOG_KILL_EVENTS

} // namespace hise
