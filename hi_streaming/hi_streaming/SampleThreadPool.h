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

#ifndef SAMPLETHREADPOOL_H_INCLUDED
#define SAMPLETHREADPOOL_H_INCLUDED

namespace hise { using namespace juce;

class SampleThreadPool : public Thread
{
public:

	SampleThreadPool();

	~SampleThreadPool();
	

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

	protected:

		void resetJob();

		Thread* getCurrentThread() { return currentThread.load(); }

	private:

		friend class SampleThreadPool;
        friend class WeakReference<Job>;
        WeakReference<Job>::Master masterReference;

		std::atomic<bool> queued;
		std::atomic<bool> running;
		std::atomic<bool> shouldStop;
		std::atomic<Thread*> currentThread;

		const String name;
	};

	double getDiskUsage() const noexcept;

	void clearPendingTasks();

	void addJob(Job* jobToAdd, bool unused);

	void run() override;

	struct Pimpl;

	
	ScopedPointer<Pimpl> pimpl;

};

typedef SampleThreadPool::Job SampleThreadPoolJob;

} // namespace hise
#endif  // SAMPLETHREADPOOL_H_INCLUDED
