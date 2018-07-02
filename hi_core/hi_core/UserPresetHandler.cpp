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

MainController::UserPresetHandler::UserPresetHandler(MainController* mc_) :
	mc(mc_),
	presetLoadDelayer(*this),
	presetLoadLock(MainController::KillStateHandler::Free)
{

}



MainController::UserPresetHandler::LoadLock::LoadLock(const MainController* mc) :
	parent(mc->getUserPresetHandler())
{
	auto currentThread = mc->getKillStateHandler().getCurrentThread();

#if !HI_RUN_UNIT_TESTS
	// This mechanism is not suitable for the audio thread, so don't try calling it here...
	jassert(currentThread != MainController::KillStateHandler::AudioThread);
#endif

	int freeThread = (int)MainController::KillStateHandler::Free;

	sameThreadHoldsLock = parent.presetLoadLock.load() == (int)currentThread;

	if(!sameThreadHoldsLock)
		holdsLock = parent.presetLoadLock.compare_exchange_strong(freeThread, (int)currentThread);

#if 0
	if (sameThreadHoldsLock)
		DBG("Reentrant Preset Lock in " + String((currentThread == MainController::KillStateHandler::MessageThread ? "Message Thread" : "Sample Loading Thread")));
	else if (holdsLock)
		DBG("Preset Lock acquired by " + String((currentThread == MainController::KillStateHandler::MessageThread ? "Message Thread" : "Sample Loading Thread")));
	else
		DBG("Preset Lock was denied for " + String((currentThread == MainController::KillStateHandler::MessageThread ? "Message Thread" : "Sample Loading Thread")));
#endif

}

MainController::UserPresetHandler::LoadLock::~LoadLock()
{
	if (holdsLock)
	{
		jassert(parent.presetLoadLock != MainController::KillStateHandler::TargetThread::Free);
		
#if 0
		DBG("Preset Lock was released by " + String((parent.presetLoadLock == MainController::KillStateHandler::MessageThread ? "Message Thread" : "Sample Loading Thread")));
#endif
		parent.presetLoadLock.store((int)MainController::KillStateHandler::TargetThread::Free);
	}
}

void MainController::UserPresetHandler::loadUserPreset(const ValueTree& v)
{
	auto f = [this, v](Processor*) {loadUserPresetInternal(v); return true; };

	auto synthChain = mc->getMainSynthChain();

	mc->getKillStateHandler().killVoicesAndCall(synthChain, f, KillStateHandler::TargetThread::SampleLoadingThread);
}

void MainController::UserPresetHandler::loadUserPreset(const File& f)
{
	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	if (xml != nullptr)
	{
		ValueTree v = ValueTree::fromXml(*xml);

		if (v.isValid())
		{
			loadUserPreset(v);
		}
	}
}

File MainController::UserPresetHandler::getCurrentlyLoadedFile() const
{
	return currentlyLoadedFile;
}

void MainController::UserPresetHandler::setCurrentlyLoadedFile(const File& f)
{
	currentlyLoadedFile = f;
}

void MainController::UserPresetHandler::sendRebuildMessage()
{
	ScopedLock sl(listeners.getLock());

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i] != nullptr)
		{
			listeners[i]->presetListUpdated();
		}
	}
}

void MainController::UserPresetHandler::savePreset(String presetName /*= String()*/)
{
	auto& tmp = presetName;
	auto f = [this, tmp]()->void {saveUserPresetInternal(tmp); };
	new DelayedFunctionCaller(f, 300);
}


void MainController::UserPresetHandler::saveUserPresetInternal(const String& name/*=String()*/)
{
	jassert(name.isNotEmpty());

	File currentPresetFile = getCurrentlyLoadedFile();

	if (name.isNotEmpty())
		currentPresetFile = currentPresetFile.getSiblingFile(name + ".preset");

	setCurrentlyLoadedFile(currentPresetFile);

	UserPresetHelpers::saveUserPreset(mc->getMainSynthChain(), currentPresetFile.getFullPathName());
}


void MainController::UserPresetHandler::loadUserPresetInternal(const ValueTree& userPresetToLoad)
{
	if (auto s = LoadLock(mc))
	{
#if USE_BACKEND
		if (!GET_PROJECT_HANDLER(mc->getMainSynthChain()).isActive()) return;
#endif

		jassert(userPresetToLoad.isValid());
		jassert(!mc->getMainSynthChain()->areVoicesActive());

		Processor::Iterator<JavascriptMidiProcessor> iter(mc->getMainSynthChain());

		mc->getSampleManager().setShouldSkipPreloading(true);

		while (JavascriptMidiProcessor *sp = iter.getNextProcessor())
		{
			if (!sp->isFront()) continue;

			ValueTree v;

			for (int i = 0; i < userPresetToLoad.getNumChildren(); i++)
			{
				if (userPresetToLoad.getChild(i).getProperty("Processor") == sp->getId())
				{
					v = userPresetToLoad.getChild(i);
					break;
				}
			}

			if (v.isValid())
			{
				sp->getScriptingContent()->restoreAllControlsFromPreset(v);
			}
		}

		mc->getSampleManager().preloadEverything();

		ValueTree autoData = userPresetToLoad.getChildWithName("MidiAutomation");

		if (autoData.isValid())
			mc->getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(autoData);

		ValueTree modulationData = userPresetToLoad.getChildWithName("ModulatedParameters");

		if (modulationData.isValid())
		{
			auto container = ProcessorHelpers::getFirstProcessorWithType<GlobalModulatorContainer>(mc->getMainSynthChain());

			if (container != nullptr)
			{
				container->restoreModulatedParameters(modulationData);
			}
		}

		auto mpeData = userPresetToLoad.getChildWithName("MPEData");

		if (mpeData.isValid())
		{
			mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().restoreFromValueTree(mpeData);
		}
		else
		{
			mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().reset();
		}

		auto f = [this]()->void
		{
			ScopedLock sl(this->listeners.getLock());

			for (auto l : this->listeners)
			{
				if (l != nullptr)
					l->presetChanged(currentlyLoadedFile);
			}
		};

		new DelayedFunctionCaller(f, 400);

		mc->allNotesOff(true);
	}
	else
	{
        mc->getDebugLogger().logMessage("Retry Loading of preset");
		presetLoadDelayer.retryLoading(userPresetToLoad);
	}

}


void MainController::UserPresetHandler::incPreset(bool next, bool stayInSameDirectory)
{
	Array<File> allPresets;

#if USE_BACKEND
	auto userDirectory = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
#else
	auto userDirectory = FrontendHandler::getUserPresetDirectory();
#endif

	userDirectory.findChildFiles(allPresets, File::findFiles, true, "*.preset");
	allPresets.sort();

	if (!currentlyLoadedFile.existsAsFile())
	{
		currentlyLoadedFile = allPresets.getFirst();
	}
	else
	{
		if (stayInSameDirectory)
		{
			allPresets.clear();
			currentlyLoadedFile.getParentDirectory().findChildFiles(allPresets, File::findFiles, false, "*.preset");
			allPresets.sort();
		}

		if (allPresets.size() == 1)
			return;

		const int oldIndex = allPresets.indexOf(currentlyLoadedFile);

		if (next)
		{
			const int newIndex = (oldIndex + 1) % allPresets.size();
			currentlyLoadedFile = allPresets[newIndex];
		}
		else
		{
			int newIndex = oldIndex - 1;
			if (newIndex == -1)
				newIndex = allPresets.size() - 1;

			currentlyLoadedFile = allPresets[newIndex];
		}
	}

	loadUserPreset(currentlyLoadedFile);
}


void MainController::UserPresetHandler::addListener(Listener* listener)
{
	listeners.add(listener);
}

void MainController::UserPresetHandler::removeListener(Listener* listener)
{
	listeners.removeAllInstancesOf(listener);
}

} // namespace hise
