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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#pragma once

namespace hise { using namespace juce;

/** This object will surpass the lifetime of a server API object. */
struct GlobalServer: public ControlledObject
{
	enum class State
	{
		Inactive,
		Pause,
		Idle,
		WaitingForResponse,
		numStates
	};

	struct Listener
	{
		virtual ~Listener() {};

		/** This callback is being executed synchronously when the queue has changed. */
		virtual void queueChanged(int numItemsInQueue) = 0;
		
		/** This callback is being executed synchronously when the download queue changed. */
		virtual void downloadQueueChanged(int numItemsToDownload) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	GlobalServer(MainController* mc):
		ControlledObject(mc),
		internalThread(*this)
	{}

	~GlobalServer();

	void setBaseURL(String url)
	{
		startTime = Time::getMillisecondCounter();
		baseURL = URL(url);
		internalThread.startThread();
	}

	uint32 startTime = 0;

    bool resendLastCallback()
    {
        if(lastCall != nullptr)
        {
            auto r = resendCallback(lastCall.get());
            
            return r.wasOk();
        }
        
        return false;
    }
    
	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void sendMessage(bool sendDownloadMessage)
	{
		int numThisTime = sendDownloadMessage ? internalThread.pendingDownloads.size() : internalThread.pendingCallbacks.size();

		for (auto l : listeners)
		{
			if (l != nullptr)
			{
				if (sendDownloadMessage)
					l.get()->downloadQueueChanged(numThisTime);
				else
					l.get()->queueChanged(numThisTime);
			}
		}
	}

	State getServerState() const
	{
		if (!internalThread.isThreadRunning())
			return State::Inactive;

		if (!internalThread.running)
			return State::Pause;

		if (internalThread.pendingCallbacks.isEmpty())
			return State::Idle;

		return State::WaitingForResponse;
	}

	struct PendingCallback : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<PendingCallback>;
		using WeakPtr = WeakReference<PendingCallback>;

		PendingCallback(ProcessorWithScriptingContent* p, const var& function) :
			f(p, nullptr, function, 2),
			creationTimeMs(Time::getMillisecondCounter())
		{
			f.setHighPriority();
			f.incRefCount();
		}

		void reset()
		{
			requestTimeMs = 0;
			completionTimeMs = 0;
			responseObj = var();
			status = 0;
		}

		WeakCallbackHolder f;

		URL url;
		String extraHeader;
		bool isPost;
		int status = 0;
		const uint32 creationTimeMs;
		uint32 requestTimeMs = 0;
		uint32 completionTimeMs = 0;
		var responseObj;

		JUCE_DECLARE_WEAK_REFERENCEABLE(PendingCallback);
	};

	void addPendingCallback(PendingCallback::Ptr p)
	{
		p->extraHeader = extraHeader;
		internalThread.pendingCallbacks.add(p);
		internalThread.notify();
        lastCall = p;
		sendMessage(false);
	}

	var addDownload(ScriptingObjects::ScriptDownloadObject::Ptr newDownload);

	var getPendingDownloads();

	var getPendingCallbacks();

	int getNumPendingRequests() const { return internalThread.pendingCallbacks.size(); }

	String getExtraHeader() const { return extraHeader; }

	PendingCallback::WeakPtr getPendingCallback(int i) const
	{
		return internalThread.pendingCallbacks[i].get();
	}

	void cleanFinishedDownloads()
	{
		internalThread.cleanDownloads = true;
	}

	Result resendCallback(PendingCallback* p);

    
    
	/** Stops the execution of the request queue (pending tasks will be finished). */
	void stop()
	{
		internalThread.running.store(false);
	}

    
    void setTimeoutMessageString(String newTimeoutMessage)
    {
        internalThread.timeoutMessage = var(newTimeoutMessage);
    }
    
	void cleanup()
	{
        lastCall = nullptr;
		internalThread.stopThread(HISE_SCRIPT_SERVER_TIMEOUT);
	}

	/** Resumes the execution of the request queue (if it was stopped with stop()). */
	void resume()
	{
		internalThread.running.store(true);
	}

	void setNumAllowedDownloads(int maxNumberOfParallelDownloads)
	{
		internalThread.numMaxDownloads = maxNumberOfParallelDownloads;
	}

	void setHttpHeader(String newHeader)
	{
		extraHeader = newHeader;
	}

	juce::URL getWithParameters(String subURL, var parameters);

    void setInitialised()
    {
        initialised = true;
    }
    
private:

#if USE_BACKEND
	bool initialised = true;
#else
	bool initialised = false;
#endif
    
	struct WebThread : public Thread
	{
		WebThread(GlobalServer& p) :
			Thread("Server Thread"),
			parent(p),
            timeoutMessage("{}")
		{};

		GlobalServer& parent;

		void run() override;

		CriticalSection queueLock;

		std::atomic<bool> cleanDownloads = { false };

		std::atomic<bool> running = { true };

		int numMaxDownloads = 1;
		ReferenceCountedArray<PendingCallback> pendingCallbacks;
		ReferenceCountedArray<ScriptingObjects::ScriptDownloadObject> pendingDownloads;

        var timeoutMessage;
        
	} internalThread;

    PendingCallback::Ptr lastCall;

    
    
	URL baseURL;
	String extraHeader;

	Array<WeakReference<Listener>> listeners;
};



} // namespace hise
