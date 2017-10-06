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


#include "../additional_libraries/lockfree_fifo/readerwriterqueue.h"

struct NewSampleThreadPool::Pimpl
{
	Pimpl() :
		jobQueue(2048),
		currentlyExecutedJob(nullptr),
		diskUsage(0.0),
		counter(0)
	{};

	~Pimpl()
	{
		if (Job* currentJob = currentlyExecutedJob.load())
		{
			currentJob->signalJobShouldExit();
		}
	}

	Atomic<int> counter;

	std::atomic<double> diskUsage;

	int64 startTime, endTime;

	moodycamel::ReaderWriterQueue<WeakReference<Job>> jobQueue;

	std::atomic<Job*> currentlyExecutedJob;

	static const String errorMessage;
};

NewSampleThreadPool::NewSampleThreadPool() :
	Thread("Sample Loading Thread"),
	pimpl(new Pimpl())
{

	startThread(9);
}

NewSampleThreadPool::~NewSampleThreadPool()
{
	pimpl = nullptr;

	stopThread(300);
}

double NewSampleThreadPool::getDiskUsage() const noexcept
{
	return pimpl->diskUsage.load();
}

void NewSampleThreadPool::addJob(Job* jobToAdd, bool unused)
{
	++pimpl->counter;

	ignoreUnused(unused);

#if ENABLE_CONSOLE_OUTPUT
	if (jobToAdd->isQueued())
	{
		Logger::writeToLog(pimpl->errorMessage);
		Logger::writeToLog(String(pimpl->counter.get()));
	}
#endif


	jobToAdd->queued.store(true);
	pimpl->jobQueue.enqueue(jobToAdd);


	notify();
}

void NewSampleThreadPool::run()
{
	while (!threadShouldExit())
	{
		if (WeakReference<Job>* next = pimpl->jobQueue.peek())
		{
			Job* j = next->get();

#if ENABLE_CPU_MEASUREMENT

			const int64 lastEndTime = pimpl->endTime;
			pimpl->startTime = Time::getHighResolutionTicks();
#endif

			if (j != nullptr)
			{
				pimpl->currentlyExecutedJob.store(j);

				j->running.store(true);

				Job::JobStatus status = j->runJob();

				j->running.store(false);

				if (status == Job::jobHasFinished)
				{
					pimpl->jobQueue.pop();
					j->queued.store(false);
					--pimpl->counter;
				}

				pimpl->currentlyExecutedJob.store(nullptr);
			}


#if ENABLE_CPU_MEASUREMENT
			pimpl->endTime = Time::getHighResolutionTicks();

			const int64 idleTime = pimpl->startTime - lastEndTime;
			const int64 busyTime = pimpl->endTime - pimpl->startTime;

			pimpl->diskUsage.store((double)busyTime / (double)(idleTime + busyTime));
#endif
		}

#if 0 // Set this to true to enable defective threading (for debugging purposes)
		wait(500);
#else
		else
		{
			wait(500);
		}
#endif


	}
}

const String NewSampleThreadPool::Pimpl::errorMessage("HDD overflow");
