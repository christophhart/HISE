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


#define ENABLE_LOG_PRELOAD_EVENTS 0

#if ENABLE_LOG_PRELOAD_EVENTS
#define LOG_PRELOAD_EVENTS(x) DBG(x)
#else
#define LOG_PRELOAD_EVENTS(x)
#endif



MainController::SampleManager::SampleManager(MainController *mc_) :
	mc(mc_),
	samplerLoaderThreadPool(new SampleThreadPool()),
	projectHandler(mc_),
	globalSamplerSoundPool(new ModulatorSamplerSoundPool(mc)),
	globalAudioSampleBufferPool(new AudioSampleBufferPool(mc_)),
	globalImagePool(new ImagePool(mc_)),
	sampleClipboard(ValueTree("clipboard")),
	useRelativePathsToProjectFolder(true),
	internalPreloadJob(mc_),
	preloadListenerUpdater(this)
{

}


void MainController::SampleManager::copySamplesToClipboard(const Array<WeakReference<ModulatorSamplerSound>> &soundsToCopy)
{
	sampleClipboard.removeAllChildren(nullptr);

	for (int i = 0; i < soundsToCopy.size(); i++)
	{
		if (soundsToCopy[i].get() != nullptr)
		{
			ValueTree soundTree = soundsToCopy[i]->exportAsValueTree();

			static Identifier duplicate("Duplicate");
			soundTree.setProperty(duplicate, true, nullptr);

			sampleClipboard.addChild(soundTree, -1, nullptr);
		}
	}
}

const ValueTree &MainController::SampleManager::getSamplesFromClipboard() const { return sampleClipboard; }

const ValueTree MainController::SampleManager::getLoadedSampleMap(const String &fileName) const
{
	for (int i = 0; i < sampleMaps.getNumChildren(); i++)
	{
		String childFileName = sampleMaps.getChild(i).getProperty("SampleMapIdentifier", String());
		if (childFileName == fileName) return sampleMaps.getChild(i);
	}

	return ValueTree();
}

void MainController::SampleManager::setDiskMode(DiskMode mode) noexcept
{
	mc->allNotesOff();

	hddMode = mode == DiskMode::HDD;

	const int multplier = hddMode ? 2 : 1;

	Processor::Iterator<ModulatorSampler> it(mc->getMainSynthChain());

	while (ModulatorSampler* sampler = it.getNextProcessor())
	{
		sampler->setPreloadMultiplier(multplier);
	}
}


void MainController::SampleManager::PreloadListenerUpdater::handleAsyncUpdate()
{
	for (auto p : manager->preloadListeners)
	{
		if (p != nullptr)
		{
			p->preloadStateChanged(manager->preloadFlag.load());
		}
	}
}



MainController::SampleManager::PreloadJob::PreloadJob(MainController* mc_) :
	SampleThreadPoolJob("Internal Preloading"),
	mc(mc_)
{

}

SampleThreadPool::Job::JobStatus MainController::SampleManager::PreloadJob::runJob()
{
	LOG_PRELOAD_EVENTS("Running preload thread ");

	auto &pendingFunctions = mc->getKillStateHandler().getSampleLoadingQueue();

	SafeFunctionCall c;

	while (pendingFunctions.pop(c))
	{
		if (!c.call())
			break;
	}

	mc->getSampleManager().clearPreloadFlag();

	return SampleThreadPool::Job::jobHasFinished;
}

void MainController::SampleManager::clearPreloadFlag()
{
	LOG_PRELOAD_EVENTS("Clearing Preload Pending Flag");
	mc->getKillStateHandler().setSampleLoadingPending(false);

	jassert(preloadFlag);

	preloadFlag = false;
	preloadListenerUpdater.triggerAsyncUpdate();




}

void MainController::SampleManager::setPreloadFlag()
{
	LOG_PRELOAD_EVENTS("Setting Preload Pending Flag");

	mc->getKillStateHandler().setSampleLoadingPending(true);

	if (!preloadFlag.load())
	{
		preloadFlag = true;
		preloadListenerUpdater.triggerAsyncUpdate();
	}
}

void MainController::SampleManager::triggerSamplePreloading()
{
	jassert(mc->getKillStateHandler().voicesAreKilled());

	mc->getSampleManager().setPreloadFlag();

	if (!internalPreloadJob.isRunning() && !internalPreloadJob.isQueued())
	{
		LOG_PRELOAD_EVENTS("Starting preload job");
		mc->getSampleManager().getGlobalSampleThreadPool()->addJob(&internalPreloadJob, false);
	}
	else
	{
		LOG_PRELOAD_EVENTS("Already running. Adding to queue");
	}
}

void MainController::SampleManager::addPreloadListener(PreloadListener* p)
{
	preloadListeners.addIfNotAlreadyThere(p);
}

void MainController::SampleManager::removePreloadListener(PreloadListener* p)
{
	preloadListeners.removeAllInstancesOf(p);
}

