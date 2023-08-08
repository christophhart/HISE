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
		virtual ~Listener();;

		/** This callback is being executed synchronously when the queue has changed. */
		virtual void queueChanged(int numItemsInQueue) = 0;
		
		/** This callback is being executed synchronously when the download queue changed. */
		virtual void downloadQueueChanged(int numItemsToDownload) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	GlobalServer(MainController* mc);

	~GlobalServer();

	void setBaseURL(String url);

	uint32 startTime = 0;

    bool resendLastCallback();

	void addListener(Listener* l);

	void removeListener(Listener* l);

	void sendMessage(bool sendDownloadMessage);

	State getServerState() const;

	struct PendingCallback : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<PendingCallback>;
		using WeakPtr = WeakReference<PendingCallback>;

		PendingCallback(ProcessorWithScriptingContent* p, const var& function);

		void reset();

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

	void addPendingCallback(PendingCallback::Ptr p);

	var addDownload(ScriptingObjects::ScriptDownloadObject::Ptr newDownload);

	var getPendingDownloads();

	var getPendingCallbacks();

	int getNumPendingRequests() const;

	String getExtraHeader() const;

	PendingCallback::WeakPtr getPendingCallback(int i) const;

	void cleanFinishedDownloads();

	Result resendCallback(PendingCallback* p);

    
    
	/** Stops the execution of the request queue (pending tasks will be finished). */
	void stop();


	void setTimeoutMessageString(String newTimeoutMessage);

	void cleanup();

	/** Resumes the execution of the request queue (if it was stopped with stop()). */
	void resume();

	void setNumAllowedDownloads(int maxNumberOfParallelDownloads);

	void setHttpHeader(String newHeader);

	juce::URL getWithParameters(String subURL, var parameters);

    void setInitialised();

private:

#if USE_BACKEND
	bool initialised = true;
#else
	bool initialised = false;
#endif
    
	struct WebThread : public Thread
	{
		WebThread(GlobalServer& p);;

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
    
public:
    
    bool addTrailingSlashes = true;
};



} // namespace hise
