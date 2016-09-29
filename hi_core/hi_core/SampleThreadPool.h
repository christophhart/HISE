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

#define NEW_THREAD_POOL_IMPLEMENTATION 1

#include "../additional_libraries/lockfree_fifo/readerwriterqueue.h"

#if NEW_THREAD_POOL_IMPLEMENTATION

class NewSampleThreadPool : public Thread
{
public:

	NewSampleThreadPool():
		Thread("Sample Loading Thread"),
		jobQueue(2048),
		currentlyExecutedJob(nullptr),
		diskUsage(0.0),
		counter(0)
	{
		startThread(9);
		
	}

	~NewSampleThreadPool()
	{
		if (Job* currentJob = currentlyExecutedJob.load())
		{
			currentJob->signalJobShouldExit();
		}

		stopThread(300);
	}
	

	class Job
	{
	public:

		Job(const String &name_) : 
			name(name_),
			queued(false),
			running(false),
			shouldStop(false)
		{};
        
        virtual ~Job() { masterReference.clear(); }

		enum JobStatus
		{
			jobHasFinished = 0,
			jobNeedsRunningAgain
		};

		virtual JobStatus runJob() = 0;

		bool shouldExit() const noexcept{ return shouldStop.load(); }

		void signalJobShouldExit() { shouldStop.store(true); }

		bool isRunning() const noexcept{ return running.load(); };

		bool isQueued() const noexcept{ return queued.load(); };

	private:

		friend class NewSampleThreadPool;
        
        friend class WeakReference<Job>;
        WeakReference<Job>::Master masterReference;

		std::atomic<bool> queued;

		std::atomic<bool> running;

		std::atomic<bool> shouldStop;

		const String name;
	};

	double getDiskUsage() const noexcept
	{
		return diskUsage.load();
	}

	void addJob(Job* jobToAdd, bool unused)
	{
		++counter;
        
		ignoreUnused(unused);

#if ENABLE_CONSOLE_OUTPUT
		if (jobToAdd->isQueued())
		{
			Logger::writeToLog(errorMessage);
			Logger::writeToLog(String(counter.get()));
		}
#endif


		jobToAdd->queued.store(true);
		jobQueue.enqueue(jobToAdd);
		

		notify();
	}

	void run() override
	{
		while (!threadShouldExit())
		{
			if (WeakReference<Job>* next = jobQueue.peek())
			{
                Job* j = next->get();

#if ENABLE_CPU_MEASUREMENT

				const int64 lastEndTime = endTime;
				startTime = Time::getHighResolutionTicks();
#endif
                
                if(j != nullptr)
                {
                    currentlyExecutedJob.store(j);
                    
                    j->running.store(true);
                    
                    Job::JobStatus status = j->runJob();
                    
                    j->running.store(false);
                    
                    if (status == Job::jobHasFinished)
                    {
                        jobQueue.pop();
                        j->queued.store(false);
                        --counter;
                    }
                    
                    currentlyExecutedJob.store(nullptr);
                }
                
                
#if ENABLE_CPU_MEASUREMENT
				endTime = Time::getHighResolutionTicks();

				const int64 idleTime = startTime - lastEndTime;
				const int64 busyTime = endTime - startTime;

				diskUsage.store((double)busyTime / (double)(idleTime + busyTime));
#endif
			}

#if 1 // Set this to true to enable defective threading (for debugging purposes)
			wait(500);
#else
            else
            {
                wait(500);
            }
#endif
            
            
		}
			
	}

    Atomic<int> counter;
    
	std::atomic<double> diskUsage;

	int64 startTime, endTime;

	moodycamel::ReaderWriterQueue<WeakReference<Job>> jobQueue;

	std::atomic<Job*> currentlyExecutedJob;

	static const String errorMessage;
};



typedef NewSampleThreadPool SampleThreadPool;
typedef NewSampleThreadPool::Job SampleThreadPoolJob;

#else

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

#endif

#endif  // SAMPLETHREADPOOL_H_INCLUDED
