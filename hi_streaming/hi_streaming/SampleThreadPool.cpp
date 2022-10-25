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


struct SampleThreadPool::Pimpl
{
	Pimpl() :
		jobQueue(8192),
		currentlyExecutedJob(nullptr),
		diskUsage(0.0)
	{};

	~Pimpl()
	{
		if (auto currentJob = currentlyExecutedJob.load())
		{
			currentJob->signalJobShouldExit();
		}
	}

	CriticalSection clearLock;

	std::atomic<double> diskUsage;
	int64 startTime, endTime;
	moodycamel::ReaderWriterQueue<WeakReference<Job>> jobQueue;
	std::atomic<Job*> currentlyExecutedJob;
	static const String errorMessage;
};

SampleThreadPool::SampleThreadPool() :
	Thread("Sample Loading Thread", HISE_DEFAULT_STACK_SIZE),
	pimpl(new Pimpl())
{

	startThread(9);
	
}

SampleThreadPool::~SampleThreadPool()
{
	stopThread(1000);
	pimpl = nullptr;
}

double SampleThreadPool::getDiskUsage() const noexcept
{
	return pimpl->diskUsage.load();
}

void SampleThreadPool::clearPendingTasks()
{
	ScopedLock sl(pimpl->clearLock);
		
	WeakReference<Job> next;

	while (pimpl->jobQueue.try_dequeue(next))
	{
		next->queued.store(false);
		next->signalJobShouldExit();
	}
}

void SampleThreadPool::addJob(Job* jobToAdd, bool unused)
{
	ignoreUnused(unused);

#if ENABLE_CONSOLE_OUTPUT
	if (jobToAdd->isQueued())
	{
		Logger::writeToLog(pimpl->errorMessage);
	}
#endif


	jobToAdd->queued.store(true);
	pimpl->jobQueue.enqueue(jobToAdd);

	notify();
}

void SampleThreadPool::run()
{
	while (!threadShouldExit())
	{
		WeakReference<Job> next;

		if (pimpl->jobQueue.try_dequeue(next))
		{
			ScopedLock sl(pimpl->clearLock);

			Job* j = next.get();

#if ENABLE_CPU_MEASUREMENT

			const int64 lastEndTime = pimpl->endTime;
			pimpl->startTime = Time::getHighResolutionTicks();
#endif

			if (j != nullptr)
			{
				pimpl->currentlyExecutedJob.store(j);

				j->currentThread.store(this);

				j->running.store(true);
				
				Job::JobStatus status = j->runJob();

				j->running.store(false);

				if (status == Job::jobHasFinished)
				{
					j->queued.store(false);
				}
				else if (status == Job::jobNeedsRunningAgain)
				{
					pimpl->jobQueue.enqueue(next);
				}

				pimpl->currentlyExecutedJob.store(nullptr);
			}
			else
			{
				pimpl->jobQueue.pop();
			}

#if ENABLE_CPU_MEASUREMENT
			pimpl->endTime = Time::getHighResolutionTicks();

			const int64 idleTime = pimpl->startTime - lastEndTime;
			const int64 busyTime = pimpl->endTime - pimpl->startTime;

			pimpl->diskUsage.store((double)busyTime / (double)(idleTime + busyTime));
#endif
		}

#if 0 // Set this to true to enable defective threading (for debugging purposes)
		wait(2500);
#else
		else
		{
			wait(500);
		}
#endif


	}
}

const String SampleThreadPool::Pimpl::errorMessage("HDD overflow");


void SampleThreadPool::Job::resetJob()
{
	queued.store(false);
	running.store(false);
	shouldStop.store(false);
	currentThread.store(nullptr);
}

} // namespace hise
