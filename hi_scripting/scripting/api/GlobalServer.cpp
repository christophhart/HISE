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

namespace hise { using namespace juce;

GlobalServer::~GlobalServer()
{
	internalThread.stopThread(HISE_SCRIPT_SERVER_TIMEOUT);
}

var GlobalServer::addDownload(ScriptingObjects::ScriptDownloadObject::Ptr newDownload)
{
	ScopedLock sl(internalThread.queueLock);

	for (auto ep : internalThread.pendingDownloads)
	{
		if (*newDownload == *ep)
		{
			ep->copyCallBackFrom(newDownload.get());
			return var(ep);
		}
	}

	

	internalThread.pendingDownloads.add(newDownload);
	internalThread.notify();
	sendMessage(true);
	return var(newDownload.get());
}

var GlobalServer::getPendingDownloads()
{
	Array<var> list;

	for (auto p : internalThread.pendingDownloads)
		list.add(var(p));

	return list;
}

var GlobalServer::getPendingCallbacks()
{
	Array<var> list;

	for (auto p : internalThread.pendingCallbacks)
		list.add(var(p));

	return list;
}

Result GlobalServer::resendCallback(PendingCallback* p)
{
	if (p != nullptr)
	{
		if (p->f)
		{
			p->reset();
			internalThread.pendingCallbacks.add(p);
			internalThread.notify();
			return Result::ok();
		}
		else
		{
			return Result::fail("Callback was from previous compilation");
		}
	}
	else
		return Result::fail("Callback was deleted");

}

juce::URL GlobalServer::getWithParameters(String subURL, var parameters)
{
	auto url = baseURL.getChildURL(subURL);

	if (auto d = parameters.getDynamicObject())
	{
		for (auto& p : d->getProperties())
			url = url.withParameter(p.name.toString(), p.value.toString());
	}
    else if (parameters.isString())
    {
        url = url.withPOSTData(parameters.toString());
    }

	return url;
}

void GlobalServer::WebThread::run()
{
	while (!threadShouldExit())
	{
		if (parent.initialised)
		{
			{
				ReferenceCountedArray<ScriptingObjects::ScriptDownloadObject> thisList;

				{
					ScopedLock sl(queueLock);
					thisList.addArray(pendingDownloads);
				}
				

				int numActiveDownloads = 0;

				bool fireCallback = false;

				for (int i = 0; i < thisList.size(); i++)
				{
					auto d = thisList[i];

					if (d->isWaitingForStart && numActiveDownloads < numMaxDownloads)
					{
						fireCallback = true;
						d->start();
					}
						
					if (threadShouldExit())
						return;

					if (d->isWaitingForStop)
						d->stopInternal();

					if (threadShouldExit())
						return;

					if (d->isRunning())
					{
						if (numActiveDownloads >= numMaxDownloads)
						{
							d->stop();
						}
						else
							numActiveDownloads++;
					}

					if (d->isFinished)
					{
						d->flushTemporaryFile();
					}

					if (threadShouldExit())
						return;

					if (cleanDownloads && d->isFinished)
					{
						ScopedLock sl(queueLock);
						pendingDownloads.removeObject(d);
						fireCallback = true;
					}
				}

				cleanDownloads = false;

				if(fireCallback)
					parent.sendMessage(true);
			}

			bool shouldFireServerCallback = false;

			if (running)
			{
				while (auto job = pendingCallbacks.removeAndReturn(0))
				{
					if (!job->f)
						continue;

					if (threadShouldExit())
						return;

					ScopedPointer<WebInputStream> wis;

					job->requestTimeMs = Time::getMillisecondCounter();

					wis = dynamic_cast<WebInputStream*>(job->url.createInputStream(job->isPost, nullptr, nullptr, job->extraHeader, HISE_SCRIPT_SERVER_TIMEOUT, nullptr, &job->status).release());

					if (threadShouldExit())
						return;

					auto response = wis != nullptr ? wis->readEntireStreamAsString() : "{}";

					if (!job->f)
						continue;

					std::array<var, 2> args;

					if (threadShouldExit())
						return;

					args[0] = job->status;

					job->completionTimeMs = Time::getMillisecondCounter();

					auto r = JSON::parse(response, args[1]);

					if (!r.wasOk() || response.isEmpty())
					{
						args[1] = response;
						
					}
                    
                    job->responseObj = args[1];

					job->f.call(args);

					shouldFireServerCallback = true;

					if (!running)
						break;
				}
			}

			if (shouldFireServerCallback)
				parent.sendMessage(false);

			Thread::wait(500);
		}
		else
		{
			// We postpone each server call until the thingie is loaded...
			Thread::wait(200);
		}
	}
}

} 
