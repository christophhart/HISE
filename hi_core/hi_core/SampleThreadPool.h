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

#ifndef SAMPLETHREADPOOL_H_INCLUDED
#define SAMPLETHREADPOOL_H_INCLUDED

class SampleThreadPool;
class SampleThreadPoolThread;

#define CHRISBOY 1

class SampleThreadPoolJob
{
public:
	explicit SampleThreadPoolJob(const String& name);

	/** Destructor. */
	virtual ~SampleThreadPoolJob();

	//==============================================================================
	
	enum JobStatus
	{
		jobHasFinished = 0, 
		jobNeedsRunningAgain
	};

	virtual JobStatus runJob() = 0;
	
	bool isRunning() const noexcept{ return isActive; }

	bool shouldExit() const noexcept{ return shouldStop; }
		
	void signalJobShouldExit();

	bool waitForJobToFinish(SampleThreadPoolJob* const otherJob, int timeOut);

	//==============================================================================
private:
	friend class SampleThreadPool;
	friend class SampleThreadPoolThread;

	SampleThreadPool* pool;
	bool shouldStop, isActive, shouldBeDeleted;

	Atomic<int> indexInPool;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleThreadPoolJob)
};


//==============================================================================

class SampleThreadPool
{
public:
	//==============================================================================

	SampleThreadPool(int numberOfThreads);
	SampleThreadPool();
	~SampleThreadPool();

	class JUCE_API  JobSelector
	{
	public:
		virtual ~JobSelector() {}

		virtual bool isJobSuitable(SampleThreadPoolJob* job) = 0;
	};
	


	void addJob(SampleThreadPoolJob* job,
		bool deleteJobWhenFinished);
	
	

	bool removeJob(SampleThreadPoolJob* job,
		bool interruptIfRunning,
		int timeOutMilliseconds);
	
	bool removeAllJobs(bool interruptRunningJobs,
		int timeOutMilliseconds,
		JobSelector* selectedJobsToRemove = nullptr);
	
	int getNumJobs() const;

	bool contains(const SampleThreadPoolJob* job) const;
	
	bool isJobRunning(const SampleThreadPoolJob* job) const;
	
	bool waitForJobToFinish(const SampleThreadPoolJob* job,
		int timeOutMilliseconds) const;
	
	bool setThreadPriorities(int newPriority);

private:
	//==============================================================================
	Array <SampleThreadPoolJob*> jobs;

	SampleThreadPoolJob* preAllocatedJobs[1024];

	SampleThreadPoolJob *getFirstJob()
	{
		ScopedLock sl(getLock());

		for (int i = 0; i < 1024; i++)
		{
			if (preAllocatedJobs[i] != nullptr) return preAllocatedJobs[i];
		}

		return nullptr;
	}

	int getFirstFreeSlot()
	{
		ScopedLock sl(getLock());
		for (int i = 0; i < 1024; i++)
		{
			if (preAllocatedJobs[i] == nullptr) return i;
		}

		jassertfalse;
		return -1;
	}

	int getLastFreeSlot()
	{
		ScopedLock sl(getLock());
		for (int i = 1024; --i >= 0;)
		{
			if (preAllocatedJobs[i] == nullptr) return i;
		}

		jassertfalse;
		return -1;
	}

	const CriticalSection& getLock() const { return lock; }

	void deleteJob(SampleThreadPoolJob *job);

	void deleteJob(const int index);

	class SampleThreadPoolThread;
	friend class SampleThreadPoolJob;
	friend class SampleThreadPoolThread;
	friend struct ContainerDeletePolicy<SampleThreadPoolThread>;
	OwnedArray<SampleThreadPoolThread> threads;

	CriticalSection lock;
	WaitableEvent jobFinishedSignal;

	bool runNextJob(SampleThreadPoolThread&);
	SampleThreadPoolJob* pickNextJobToRun();
	
	void createThreads(int numThreads);
	void stopThreads();

	void removeAllJobs(bool, int, bool);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleThreadPool)
};



#endif  // SAMPLETHREADPOOL_H_INCLUDED
