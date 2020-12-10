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
	mc(mc_)
{
	
}

void MainController::UserPresetHandler::loadUserPreset(const ValueTree& v, bool useUndoManagerIfEnabled)
{
	if (useUndoManagerIfEnabled && useUndoForPresetLoads)
	{
		mc->getControlUndoManager()->perform(new UndoableUserPresetLoad(mc, v));
	}
	else
	{
		pendingPreset = v;

		auto f = [](Processor*p)
		{
			p->getMainController()->getUserPresetHandler().loadUserPresetInternal();
			return SafeFunctionCall::OK;
		};

		// Send a note off to stop the arpeggiator etc...
		mc->allNotesOff(false);

		mc->killAndCallOnLoadingThread(f);
	}
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
	auto f = [](Dispatchable* obj)
	{
		auto uph = static_cast<UserPresetHandler*>(obj);

		jassert_dispatched_message_thread(uph->mc);

		ScopedLock sl(uph->listeners.getLock());

		for (int i = 0; i < uph->listeners.size(); i++)
		{
			uph->mc->checkAndAbortMessageThreadOperation();

			if (uph->listeners[i] != nullptr)
				uph->listeners[i]->presetListUpdated();
		};

		return Status::OK;
	};

	mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(this, f);
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


void MainController::UserPresetHandler::loadUserPresetInternal()
{
	{
		LockHelpers::freeToGo(mc);
        
		ValueTree userPresetToLoad = pendingPreset;

#if USE_BACKEND
		if (!GET_PROJECT_HANDLER(mc->getMainSynthChain()).isActive()) return;
#endif

		jassert(userPresetToLoad.isValid());

		mc->getSampleManager().setShouldSkipPreloading(true);

		// Reload the macro connections before restoring the preset values
		// so that it will update the correct connections with `setMacroControl()` in a control callback
		if (mc->getMacroManager().isMacroEnabledOnFrontend())
			mc->getMacroManager().getMacroChain()->loadMacrosFromValueTree(userPresetToLoad, false);

#if USE_RAW_FRONTEND

		auto fp = dynamic_cast<FrontendProcessor*>(mc);
		
		fp->getRawDataHolder()->restoreFromValueTree(userPresetToLoad);

#else
		Processor::Iterator<JavascriptMidiProcessor> iter(mc->getMainSynthChain());

		try
		{
			while (JavascriptMidiProcessor *sp = iter.getNextProcessor())
			{
				if (!sp->isFront()) continue;

				ValueTree v;

				for (auto c: userPresetToLoad)
				{
					if (c.getProperty("Processor") == sp->getId())
					{
						v = c;
						break;
					}
				}

				UserPresetHelpers::restoreModuleStates(mc->getMainSynthChain(), userPresetToLoad);

				if (v.isValid())
				{
					sp->getScriptingContent()->restoreAllControlsFromPreset(v);
				}
			}
		}
		catch (String& m)
		{
			ignoreUnused(m);
			jassertfalse;
			DBG(m);
		}

#endif

		ValueTree autoData = userPresetToLoad.getChildWithName("MidiAutomation");

		if (autoData.isValid())
			mc->getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(autoData);

		auto mpeData = userPresetToLoad.getChildWithName("MPEData");

		if (mpeData.isValid())
		{
			mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().restoreFromValueTree(mpeData);
		}
		else
		{
			mc->getMacroManager().getMidiControlAutomationHandler()->getMPEData().reset();
		}

		// Now we can restore the values of the macro controls
		if (mc->getMacroManager().isMacroEnabledOnFrontend())
			mc->getMacroManager().getMacroChain()->loadMacroValuesFromValueTree(userPresetToLoad);

		auto f = [](Dispatchable* obj)
		{
			auto uph = static_cast<UserPresetHandler*>(obj);
			auto mc_ = uph->mc;
			ignoreUnused(mc_);
			jassert_dispatched_message_thread(mc_);

			ScopedLock sl(uph->listeners.getLock());

			for (auto l : uph->listeners)
			{
				uph->mc->checkAndAbortMessageThreadOperation();

				if (l != nullptr)
					l->presetChanged(uph->currentlyLoadedFile);
			}

			return Status::OK;
		};

		mc->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(this, f);
	}

	

	mc->getSampleManager().preloadEverything();
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
    PresetBrowser::DataBaseHelpers::cleanFileList(mc, allPresets);
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
            PresetBrowser::DataBaseHelpers::cleanFileList(mc, allPresets);
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

void MainController::UserPresetHandler::TagDataBase::buildDataBase(bool force /*= false*/)
{
	if (force || dirty)
	{
		buildInternal();
	}
}

void MainController::UserPresetHandler::TagDataBase::buildInternal()
{
	cachedTags.clear();

	Array<File> allPresets;

	root.findChildFiles(allPresets, File::findFiles, true, "*.preset");

	PresetBrowser::DataBaseHelpers::cleanFileList(nullptr, allPresets);

	for (auto f : allPresets)
	{
		auto sa = PresetBrowser::DataBaseHelpers::getTagsFromXml(f);

		CachedTag newTag;
		newTag.hashCode = f.hashCode64();
		for (auto t : sa)
			newTag.tags.add(Identifier(t));

		cachedTags.add(std::move(newTag));
	}

	dirty = false;
}

void MainController::UserPresetHandler::TagDataBase::setRootDirectory(const File& newRoot)
{

	if (root != newRoot)
	{
		root = newRoot;
		dirty = true;
	}
}



} // namespace hise
