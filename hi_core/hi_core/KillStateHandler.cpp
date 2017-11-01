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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#if ENABLE_LOG_KILL_EVENTS
#define LOG_KILL_EVENTS(x) DBG(x)
#else
#define LOG_KILL_EVENTS(x)
#endif


MainController::KillStateHandler::KillStateHandler(MainController* mc_) :
	mc(mc_),
	killState(NewKillState::Clear),
	pendingStates(0),
	pendingMessageThreadFunctions(4096),
	pendingAudioThreadFunctions(4096),
	pendingSampleLoadFunctions(4096)
{
	pendingFunctions[TargetThread::AudioThread] = &pendingAudioThreadFunctions;
	pendingFunctions[TargetThread::MessageThread] = &pendingMessageThreadFunctions;
	pendingFunctions[TargetThread::SampleLoadingThread] = &pendingSampleLoadFunctions;



	threadIds[TargetThread::AudioThread] = nullptr;
	threadIds[TargetThread::SampleLoadingThread] = mc->getSampleManager().getGlobalSampleThreadPool()->getThreadId();
	threadIds[TargetThread::MessageThread] = nullptr;
}

void MainController::KillStateHandler::addFunctionToExecute(Processor* p, const ProcessorFunction& functionToCallWhenVoicesAreKilled, TargetThread targetThread, NotificationType triggerUpdate)
{
	pendingFunctions[targetThread]->push(SafeFunctionCall(p, functionToCallWhenVoicesAreKilled));

	if (triggerUpdate == sendNotification)
	{

		if (targetThread == MessageThread)
			triggerPendingMessageCallbackFunctionsUpdate();
		else if (targetThread == SampleLoadingThread)
		{
			triggerPendingSampleLoadingFunctionsUpdate();
		}
	}
}

bool MainController::KillStateHandler::voicesAreKilled() const
{
	return !mc->getMainSynthChain()->areVoicesActive();
}


void MainController::KillStateHandler::onAllVoicesAreKilled()
{
	LOG_KILL_EVENTS("  All voices killed.");

	jassert(getCurrentThread() == AudioThread);
	jassert(!isNothingPending());
	mc->getMainSynthChain()->resetAllVoices();

	executePendingAudioThreadFunctions();

	triggerPendingMessageCallbackFunctionsUpdate();
	triggerPendingSampleLoadingFunctionsUpdate();
}

void MainController::KillStateHandler::handleAsyncUpdate()
{


	jassert(getCurrentThread() == MessageThread);
	jassert(voicesAreKilled());
	jassert(isPending(WaitingForAsyncUpdate));
	jassert(pendingMessageThreadFunctions.size() > 0);



	setPendingState(AsyncMessageCallback, true);
	setPendingState(WaitingForAsyncUpdate, false);

	SafeFunctionCall f;

	while (pendingMessageThreadFunctions.pop(f))
	{
		LOG_KILL_EVENTS("  Calling async function");
		f.call();
	}

	setPendingState(AsyncMessageCallback, false);
}


void MainController::KillStateHandler::handleKillState()
{
#if ENABLE_LOG_KILL_EVENTS
	if (killState != NewKillState::Clear)
		LOG_KILL_EVENTS("      handleKillState: " + getStringForKillState(killState));
#endif

	initAudioThreadId();



	switch ((int)killState.load())
	{
	case NewKillState::Clear:
	{
		disableVoiceStartsThisCallback = false;
		executePendingAudioThreadFunctions();
		return;
	}
	case NewKillState::Pending:
	{
		disableVoiceStartsThisCallback = true;

		if (isPending(VoiceKillStart))
		{
			if (!voicesAreKilled())
			{
				setPendingState(VoiceKill, true);
				mc->getMainSynthChain()->killAllVoices();
			}
			else
			{
				onAllVoicesAreKilled();
				setPendingState(VoiceKillStart, false);
			}
		}
		else if (isPending(VoiceKill))
		{
			if (voicesAreKilled())
			{
				onAllVoicesAreKilled();
				setPendingState(VoiceKill, false);
			}
		}
		return;
	}
	case NewKillState::SetForClearance:
	{
		disableVoiceStartsThisCallback = true;

		if (isNothingPending())
		{
			setKillState(NewKillState::Clear);
		}
		else
		{

			for (int i = 0; i < PendingState::numPendingStates; i++)
			{
				if (isPending((PendingState)i))
				{
					LOG_KILL_EVENTS("    Still pending: " + getStringForPendingState((PendingState)i));
				}
			}

			setKillState(Pending);
		}
		return;
	}

	default:
		break;
	}
}


void MainController::KillStateHandler::killVoicesAndCall(Processor* p, const ProcessorFunction& functionToExecuteWhenKilled, MainController::KillStateHandler::TargetThread targetThread)
{
	if (voicesAreKilled())
	{
		auto currentThreadId = getCurrentThread();

		if (targetThread == currentThreadId)
		{
			LOG_KILL_EVENTS("  Executing synchronously...");

			setPendingState(SyncMessageCallback, true);

			jassert(p != nullptr);
			functionToExecuteWhenKilled(p);

			setPendingState(SyncMessageCallback, false);

			LOG_KILL_EVENTS("  Done Executing function OK");
			return;
		}
		else
		{
			addFunctionToExecute(p, functionToExecuteWhenKilled, targetThread, sendNotification);
		}
	}
	else
	{
		jassert(p != nullptr);

		setPendingState(VoiceKillStart, true);

		LOG_KILL_EVENTS("  Active Voices detected. Delaying function call");
		addFunctionToExecute(p, functionToExecuteWhenKilled, targetThread, dontSendNotification);
	}
}




bool MainController::KillStateHandler::voiceStartIsDisabled() const
{
	return disableVoiceStartsThisCallback;
}

void MainController::KillStateHandler::setSampleLoadingPending(bool isPending)
{
	if (isPending == false)
		jassert(voicesAreKilled());

	setPendingState(SampleLoading, isPending);
}


String MainController::KillStateHandler::getStringForKillState(NewKillState s)
{
	switch (s)
	{
	case MainController::KillStateHandler::Clear: return "Clear";
	case MainController::KillStateHandler::Pending: return "Pending";
	case MainController::KillStateHandler::SetForClearance: return "SetForClearance";
	case MainController::KillStateHandler::numNewKillStates:
		break;
	default:
		break;
	}

	return String();
}

String MainController::KillStateHandler::getStringForPendingState(PendingState state)
{
	switch (state)
	{
	case MainController::KillStateHandler::VoiceKillStart: return "VoiceKillStart";
	case MainController::KillStateHandler::VoiceKill: return "VoiceKill";
	case MainController::KillStateHandler::AudioThreadFunction: return "AudioThreadFunction";
	case MainController::KillStateHandler::SyncMessageCallback: return "SyncMessageCallback";
	case MainController::KillStateHandler::WaitingForAsyncUpdate: return "WaitingForAsyncUpdate";
	case MainController::KillStateHandler::AsyncMessageCallback: return "AsyncMessageCallback";
	case MainController::KillStateHandler::SampleLoading: return "SampleLoading";
	case MainController::KillStateHandler::numPendingStates:
		break;
	default:
		break;
	}

	return "Unknown";
}

void MainController::KillStateHandler::setPendingState(PendingState pendingState, bool isPending)
{
#if ENABLE_LOG_KILL_EVENTS
	if (isPending != pendingStates[pendingState])
		LOG_KILL_EVENTS("    Pending State Change: " + getStringForPendingState(pendingState) + ": " + String(isPending ? "true" : "false"));
#endif

	pendingStates.setBit(pendingState, isPending);

	if (isPending)
	{
		setKillState(Pending);
	}
	else
	{
		if (isNothingPending())
			setKillState(NewKillState::SetForClearance);
	}
}


void MainController::KillStateHandler::setKillState(NewKillState newKillState)
{
	if (newKillState != killState)
	{
		LOG_KILL_EVENTS("    Kill State change: " + getStringForKillState(killState) + " -> " + getStringForKillState(newKillState));
		killState = newKillState;
	}
}

bool MainController::KillStateHandler::isPending(PendingState state) const
{
	return pendingStates[state];
}

bool MainController::KillStateHandler::isNothingPending() const
{
	return pendingStates.isZero();
}


void MainController::KillStateHandler::setSampleLoadingThreadId(void* newId)
{
	jassert(newId != nullptr);
	jassert(threadIds[(int)TargetThread::SampleLoadingThread] == nullptr);

	threadIds[(int)TargetThread::SampleLoadingThread] = newId;
}

MainController::KillStateHandler::TargetThread MainController::KillStateHandler::getCurrentThread() const
{
	jassert(threadIds[(int)TargetThread::SampleLoadingThread] != nullptr);

	if (auto mm = MessageManager::getInstanceWithoutCreating())
	{
		if (mm->isThisTheMessageThread())
			return MessageThread;
	}

	auto threadId = Thread::getCurrentThreadId();

	if (threadId == threadIds[(int)TargetThread::AudioThread])
		return TargetThread::AudioThread;
	else if (threadId == threadIds[(int)TargetThread::SampleLoadingThread])
		return TargetThread::SampleLoadingThread;
	else
	{
		jassertfalse;
		return MessageThread;
	}
}

void MainController::KillStateHandler::initAudioThreadId()
{
	if (threadIds[(int)TargetThread::AudioThread] == nullptr)
	{
		auto audioThreadId = Thread::getCurrentThreadId();

		jassert(audioThreadId != nullptr);
		threadIds[(int)TargetThread::AudioThread] = audioThreadId;
	}
}

void MainController::KillStateHandler::executePendingAudioThreadFunctions()
{
	if (!pendingAudioThreadFunctions.isEmpty())
	{
		SafeFunctionCall d;

		setPendingState(AudioThreadFunction, true);

		while (pendingAudioThreadFunctions.pop(d))
		{
			if (!d.call())
			{
				jassertfalse;
				break;
			}
		}

		setPendingState(AudioThreadFunction, false);
	}
}



void MainController::KillStateHandler::triggerPendingMessageCallbackFunctionsUpdate()
{
	if (!pendingMessageThreadFunctions.isEmpty())
	{
		setPendingState(WaitingForAsyncUpdate, true);
		triggerAsyncUpdate();
	}
}


void MainController::KillStateHandler::triggerPendingSampleLoadingFunctionsUpdate()
{
	jassert(threadIds[TargetThread::SampleLoadingThread] != nullptr);

	if (!pendingSampleLoadFunctions.isEmpty())
	{
		setPendingState(SampleLoading, true);
		mc->getSampleManager().triggerSamplePreloading();
	}
}


#undef LOG_KILL_EVENTS