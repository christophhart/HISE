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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

class SampleThreadPool::SampleThreadPoolThread : public Thread
{
public:
	SampleThreadPoolThread(SampleThreadPool& p)
		: Thread("Pool"), currentJob(nullptr), pool(p)
	{
		p.setThreadPriorities(9);
	}

	void run() override
	{
		while (!threadShouldExit())
			if (!pool.runNextJob(*this))
				wait(500);
	}

	SampleThreadPoolJob* volatile currentJob;
	SampleThreadPool& pool;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleThreadPoolThread)
};

//==============================================================================
SampleThreadPoolJob::SampleThreadPoolJob(const String& /*name*/)
	: pool(nullptr),
	shouldStop(false), isActive(false), shouldBeDeleted(false), indexInPool(-1)
{
}

SampleThreadPoolJob::~SampleThreadPoolJob()
{
	// you mustn't delete a job while it's still in a pool! Use SampleThreadPool::removeJob()
	// to remove it first!
	jassert(pool == nullptr || !pool->contains(this));
}

void SampleThreadPoolJob::signalJobShouldExit()
{
	shouldStop = true;
}

bool SampleThreadPoolJob::waitForJobToFinish(SampleThreadPoolJob* const otherJob, int timeOut)
{
	return pool->waitForJobToFinish(otherJob, timeOut);
}

//==============================================================================
SampleThreadPool::SampleThreadPool(const int numThreads)
{
	for (int i = 0; i < 1024; i++)
	{
		preAllocatedJobs[i] = nullptr;
	}

	jassert(numThreads > 0); // not much point having a pool without any threads!

	createThreads(numThreads);
}

SampleThreadPool::SampleThreadPool()
{
	for (int i = 0; i < 1024; i++)
	{
		preAllocatedJobs[i] = nullptr;
	}

	createThreads(SystemStats::getNumCpus());
}

SampleThreadPool::~SampleThreadPool()
{
	removeAllJobs(true, 5000);
	stopThreads();
}

void SampleThreadPool::createThreads(int numThreads)
{
	for (int i = jmax(1, numThreads); --i >= 0;)
		threads.add(new SampleThreadPoolThread(*this));

	for (int i = threads.size(); --i >= 0;)
		threads.getUnchecked(i)->startThread();
}

void SampleThreadPool::stopThreads()
{
	for (int i = threads.size(); --i >= 0;)
		threads.getUnchecked(i)->signalThreadShouldExit();

	for (int i = threads.size(); --i >= 0;)
		threads.getUnchecked(i)->stopThread(500);
}

void SampleThreadPool::addJob(SampleThreadPoolJob* const job, const bool /*deleteJobWhenFinished*/)
{
	if (job->pool == nullptr)
	{
		job->pool = this;
		job->shouldStop = false;
		job->isActive = false;

		{
			const ScopedLock sl(lock);

			const int index = getFirstFreeSlot();

			if (index != -1)
			{
				preAllocatedJobs[index] = job;
				job->indexInPool = index;
			}
		}

		for (int i = threads.size(); --i >= 0;)
			threads.getUnchecked(i)->notify();
	}
}

int SampleThreadPool::getNumJobs() const
{
	return jobs.size();
}

bool SampleThreadPool::contains(const SampleThreadPoolJob* const job) const
{
	return job->indexInPool.get() != -1;
}

bool SampleThreadPool::isJobRunning(const SampleThreadPoolJob* const job) const
{
	const ScopedLock sl(lock);
	return job->isActive && contains(job);
}

bool SampleThreadPool::waitForJobToFinish(const SampleThreadPoolJob* const job, const int timeOutMs) const
{
	if (job != nullptr)
	{
		const uint32 start = Time::getMillisecondCounter();

		while (contains(job))
		{
			if (timeOutMs >= 0 && Time::getMillisecondCounter() >= start + (uint32)timeOutMs)
				return false;

			jobFinishedSignal.wait(2);
		}
	}

	return true;
}

bool SampleThreadPool::removeJob(SampleThreadPoolJob* const job,
	const bool interruptIfRunning,
	const int timeOutMs)
{
	bool dontWait = true;

	if (job != nullptr)
	{
		if (job->isActive)
		{
			if (interruptIfRunning)
				job->signalJobShouldExit();

			dontWait = false;
		}
		else
		{
			deleteJob(job);
		}
	}

	return dontWait || waitForJobToFinish(job, timeOutMs);
}

bool SampleThreadPool::removeAllJobs(const bool interruptRunningJobs, const int timeOutMs,
	SampleThreadPool::JobSelector* const /*selectedJobsToRemove*/)
{
	Array <SampleThreadPoolJob*> jobsToWaitFor;
	jobsToWaitFor.ensureStorageAllocated(1024);

	const ScopedLock sl(lock);

	for (int i = 1024; --i >= 0;)
	{
		SampleThreadPoolJob* const job = preAllocatedJobs[i];

		if (job == nullptr) continue;

		if (job->isActive)
		{
			jobsToWaitFor.add(job);
			if (interruptRunningJobs)
				job->signalJobShouldExit();
		}
		else
		{
			deleteJob(i);
		}
	}

	const uint32 start = Time::getMillisecondCounter();

	for (;;)
	{
		for (int i = jobsToWaitFor.size(); --i >= 0;)
		{
			SampleThreadPoolJob* const job = jobsToWaitFor.getUnchecked(i);

			if (!isJobRunning(job))
				jobsToWaitFor.remove(i);
		}

		if (jobsToWaitFor.size() == 0)
			break;

		if (timeOutMs >= 0 && Time::getMillisecondCounter() >= start + (uint32)timeOutMs)
			return false;

		jobFinishedSignal.wait(20);
	}

	return true;
}



bool SampleThreadPool::setThreadPriorities(const int newPriority)
{
	bool ok = true;

	for (int i = threads.size(); --i >= 0;)
		if (!threads.getUnchecked(i)->setPriority(newPriority))
			ok = false;

	return ok;
}

SampleThreadPoolJob* SampleThreadPool::pickNextJobToRun()
{
	const ScopedLock sl(lock);

	for (int i = 0; i < 1024; ++i)
	{
		SampleThreadPoolJob* job = preAllocatedJobs[i];

		if (job != nullptr && !job->isActive)
		{
			if (job->shouldStop)
			{
				deleteJob(i);
				continue;
			}

			job->isActive = true;
			return job;
		}
	}
	
	return nullptr;
}

bool SampleThreadPool::runNextJob(SampleThreadPoolThread& thread)
{
	if (SampleThreadPoolJob* const job = pickNextJobToRun())
	{
		SampleThreadPoolJob::JobStatus result = SampleThreadPoolJob::jobHasFinished;
		thread.currentJob = job;

		try
		{
			result = job->runJob();
		}
		catch (...)
		{
			jassertfalse; // Your runJob() method mustn't throw any exceptions!
		}

		thread.currentJob = nullptr;

		const ScopedLock sl(lock);
		
		const int index = job->indexInPool.get();

		if (preAllocatedJobs[index] == job)
		{
			job->isActive = false;

			if (result != SampleThreadPoolJob::jobNeedsRunningAgain || job->shouldStop)
			{
				deleteJob(index);

				jobFinishedSignal.signal();
			}
			else
			{
				// move the job to the end of the queue if it wants another go
				const int lastFreeIndex = getLastFreeSlot();

				if (lastFreeIndex != -1)
				{
					preAllocatedJobs[index] = nullptr;
					preAllocatedJobs[lastFreeIndex] = job;
				}
			}
		}

		return true;
	}

	return false;
}

void SampleThreadPool::deleteJob(SampleThreadPoolJob* const job)
{
	const int indexInPool = job->indexInPool.get();

	if (indexInPool != -1)
	{
		deleteJob(indexInPool);
	}
}

void SampleThreadPool::deleteJob(const int index)
{
	SampleThreadPoolJob *job;

	if (index >= 0 && index < 1024)
	{
		const ScopedLock sl(lock);

		job = preAllocatedJobs[index];

		preAllocatedJobs[index] = nullptr;
		
		if (job != nullptr)
		{
			job->shouldStop = true;
			job->pool = nullptr;
			job->indexInPool = -1;
		}
	}
	else
	{
		job = nullptr;
	}	
}
