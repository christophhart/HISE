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

namespace hise { using namespace juce;

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
	sampleClipboard(ValueTree("clipboard")),
	internalPreloadJob(mc_),
	preloadListenerUpdater(this),
	preloadFlag(false),
	pendingFunctions(8192)
{
	
}



MainController::SampleManager::~SampleManager()
{
	preloadListeners.clear();

	internalPreloadJob.signalJobShouldExit();
	samplerLoaderThreadPool->stopThread(2000);

	pendingFunctions.clear();

	jassert(pendingFunctions.isEmpty());



	samplerLoaderThreadPool = nullptr;
}



void MainController::SampleManager::setShouldSkipPreloading(bool skip)
{
	skipPreloading = skip;
}

void MainController::SampleManager::preloadEverything()
{
    if(!skipPreloading)
        return;

	skipPreloading = false;

	LockHelpers::freeToGo(mc);

	Processor::Iterator<ModulatorSampler> it(mc->getMainSynthChain());

	Array<WeakReference<Processor>> samplersToPreload;

	while (ModulatorSampler* s = it.getNextProcessor())
	{
		if (s->hasPendingSampleLoad())
		{
			auto f = [](Processor* p)
			{
				if (static_cast<ModulatorSampler*>(p)->preloadAllSamples())
					return SafeFunctionCall::OK;
				else
					return SafeFunctionCall::cancelled;
			};

			s->killAllVoicesAndCall(f, true);
		}
	}
}

hise::ModulatorSamplerSoundPool * MainController::SampleManager::getModulatorSamplerSoundPool2() const
{

	auto pb = mc->getCurrentFileHandler().pool->getPoolBase(FileHandlerBase::Samples);

	jassert(pb != nullptr);

	return dynamic_cast<ModulatorSamplerSoundPool*>(pb);
}



void MainController::SampleManager::addDeferredFunction(Processor* p, const ProcessorFunction& f)
{
	pendingFunctions.push({ SafeFunctionCall(p, f), mc });

	// Not threadsafe, but should be OK
	pendingProcessors.add(p);

	triggerSamplePreloading();
}

double& MainController::SampleManager::getPreloadProgress()
{
	return internalPreloadJob.progress;
}

void MainController::SampleManager::cancelAllJobs()
{
	internalPreloadJob.signalJobShouldExit();
	samplerLoaderThreadPool->stopThread(5000);
}


bool MainController::SampleManager::hasPendingFunction(Processor* p) const
{
	return pendingProcessors.contains(p);

}

void MainController::SampleManager::initialiseQueue()
{
	if (!initialised)
	{
		initialised = true;

		jassert(pendingFunctions.isEmpty());

#if 0
		auto flags = MainController::KillStateHandler::QueueProducerFlags::LoadingThreadIsProducer |
			MainController::KillStateHandler::QueueProducerFlags::MessageThreadIsProducer |
			MainController::KillStateHandler::QueueProducerFlags::ScriptThreadIsProducer;

		auto tokens = mc->getKillStateHandler().createPublicTokenList(flags);

		pendingFunctions.setThreadTokens(tokens);
#endif
	}
}


void MainController::SampleManager::copySamplesToClipboard(const void* soundsToCopy_)
{
	auto soundsToCopy = *reinterpret_cast<const SampleSelection*>(soundsToCopy_);

	sampleClipboard.removeAllChildren(nullptr);

	for (int i = 0; i < soundsToCopy.size(); i++)
	{
		if (soundsToCopy[i].get() != nullptr)
		{
			ValueTree soundTree = soundsToCopy[i]->getData().createCopy();

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

NativeFileHandler& MainController::SampleManager::getProjectHandler()
{
#if USE_BACKEND
	auto assetHandler = dynamic_cast<BackendProcessor*>(mc)->assetManager.get();

	if(assetHandler != nullptr)
		return *assetHandler;


#endif
	return projectHandler;
}

void MainController::SampleManager::setDiskMode(DiskMode mode) noexcept
{
	if (hddMode != (mode == DiskMode::HDD))
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
}

const NativeFileHandler& MainController::SampleManager::getProjectHandler() const
{
#if USE_BACKEND
	auto assetHandler = dynamic_cast<BackendProcessor*>(mc)->assetManager.get();

	if(assetHandler != nullptr)
		return *assetHandler;
#endif

	return projectHandler;
}


void MainController::SampleManager::PreloadListenerUpdater::handleAsyncUpdate()
{

	Array<WeakReference<PreloadListener>> arrayToUse;

	arrayToUse.addArray(manager->preloadListeners);

	for (auto& pl: arrayToUse)
	{
		if (pl.get() != nullptr )
			pl->preloadStateChanged(manager->preloadFlag.load());
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

	auto &pFunctions = mc->getSampleManager().pendingFunctions;

	SampleFunction c;

	auto& pending = mc->getSampleManager().pendingProcessors;

	while (pFunctions.pop(c))
	{
		mc->getSampleManager().setCurrentPreloadMessage("Kill voices...");

		mc->getKillStateHandler().killVoicesAndWait();

		jassert(!mc->getKillStateHandler().isAudioRunning());

		mc->getSampleManager().setCurrentPreloadMessage("");

		auto result = c.getFunction().call();
		
		auto pToDelete = c.getFunction().p.get();
		pending.removeFirstMatchingValue(pToDelete);

		if (shouldExit())
		{
			break;
		}

		if (result == SafeFunctionCall::cancelled)
			break;
	}

	
	jassert(pending.isEmpty());
	mc->getSampleManager().clearPreloadFlag();
	mc->getSampleManager().initialiseQueue();

	return SampleThreadPool::Job::jobHasFinished;
}

void MainController::SampleManager::clearPreloadFlag()
{
	LOG_PRELOAD_EVENTS("Clearing Preload Pending Flag");
	
	jassert(preloadFlag);

	internalPreloadJob.progress = 0.0;
	preloadFlag = false;
	preloadListenerUpdater.triggerAsyncUpdate();
}

void MainController::SampleManager::setPreloadFlag()
{
	LOG_PRELOAD_EVENTS("Setting Preload Pending Flag");

	if (!preloadFlag.load())
	{
		preloadFlag = true;
		preloadListenerUpdater.triggerAsyncUpdate();
	}
}

void MainController::SampleManager::triggerSamplePreloading()
{
	mc->getSampleManager().setPreloadFlag();
	

	if (!internalPreloadJob.isRunning() && !internalPreloadJob.isQueued())
	{
		mc->getSampleManager().getGlobalSampleThreadPool()->clearPendingTasks();
		mc->getSampleManager().getGlobalSampleThreadPool()->addJob(&internalPreloadJob, false);
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

} // namespace hise
