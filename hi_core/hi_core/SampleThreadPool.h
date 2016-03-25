/*
  ==============================================================================

    SampleThreadPool.h
    Created: 7 Mar 2016 1:04:17pm
    Author:  Christoph

  ==============================================================================
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
		ScopedLock sl(lock);

		for (int i = 0; i < 1024; i++)
		{
			if (preAllocatedJobs[i] != nullptr) return preAllocatedJobs[i];
		}

		return nullptr;
	}

	int getFirstFreeSlot()
	{
		ScopedLock sl(lock);
		for (int i = 0; i < 1024; i++)
		{
			if (preAllocatedJobs[i] == nullptr) return i;
		}

		jassertfalse;
		return -1;
	}

	int getLastFreeSlot()
	{
		ScopedLock sl(lock);
		for (int i = 1024; --i >= 0;)
		{
			if (preAllocatedJobs[i] == nullptr) return i;
		}

		jassertfalse;
		return -1;
	}

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
