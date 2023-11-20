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


MainController::LockFreeDispatcher::LockFreeDispatcher(MainController* mc_) :
	mc(mc_),
	pendingTasks(1024)
{
#if !HISE_HEADLESS
	startTimer(30);
#endif
}

MainController::LockFreeDispatcher::~LockFreeDispatcher()
{
	clearQueueWithoutCalling();
	stopTimer();
}

void MainController::LockFreeDispatcher::clearQueueWithoutCalling()
{
	auto f = [](Job& j)
	{
		j.cancel();
		return MultithreadedQueueHelpers::OK;
	};

	pendingTasks.clear(f);
}

bool MainController::LockFreeDispatcher::isIdle() const
{
	if (!mc->isInitialised())
		return true;

	return mc->getKillStateHandler().isAudioRunning();
}

bool MainController::LockFreeDispatcher::callOnMessageThreadAfterSuspension(Dispatchable* object, const Dispatchable::Function& f)
{
	Dispatchable::Status status = Dispatchable::Status::notExecuted;

	if (isIdle())
	{
		ScopedValueSetter<bool> svs(inDispatchLoop, true);

		if (isMessageThread())
		{
			try
			{
				// Idle + Message thread, so call it directly
				status = f(object);
			}
			catch (AbortSignal&)
			{
				status = Dispatchable::Status::needsToRunAgain;
			}
		}
		else
		{
			pendingTasks.push({ object, f });
		}
	}

	if (status != Dispatchable::Status::OK)
	{
		pendingTasks.push({ object, f });
	}

	return status == Dispatchable::Status::OK;
}

void MainController::LockFreeDispatcher::timerCallback()
{
	if (isIdle())
	{
		ScopedValueSetter<bool> dvs(inDispatchLoop, true);

		auto f = [this](Job& j)
		{
			auto status = j.run();

			if (status == Dispatchable::Status::needsToRunAgain)
				return MultithreadedQueueHelpers::AbortClearing;

			return MultithreadedQueueHelpers::OK;
		};

		pendingTasks.clear(f);
	}
}

bool MainController::LockFreeDispatcher::isMessageThread() const noexcept
{
	return MessageManager::getInstance()->isThisTheMessageThread();
}

bool MainController::LockFreeDispatcher::isLoadingThread() const noexcept
{
	return mc->getKillStateHandler().getCurrentThread() == KillStateHandler::TargetThread::SampleLoadingThread;
}


MainController::LockFreeDispatcher::Job::~Job()
{
	obj = nullptr;
}

hise::Dispatchable::Status MainController::LockFreeDispatcher::Job::run()
{
	if (status == Dispatchable::Status::cancelled)
		return status;

	if (obj != nullptr)
	{
		try
		{
			status = func(obj);
		}
		catch (AbortSignal&)
		{
			status = Dispatchable::Status::needsToRunAgain;
		}
	}
	else
		status = Dispatchable::Status::OK;

	return status;
}

void MainController::LockFreeDispatcher::Job::cancel()
{
	status = Dispatchable::Status::cancelled;
}

bool MainController::LockFreeDispatcher::Job::isDone() const noexcept
{
	return status == Dispatchable::Status::OK;
}


}