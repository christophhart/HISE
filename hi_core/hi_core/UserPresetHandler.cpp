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


MainController::UserPresetHandler::CustomAutomationData::CustomAutomationData(MainController* mc, int index_, const var& d) :
	index(index_),
	r(Result::ok())
{
	static const Identifier id_("ID");
	static const Identifier min("min");
	static const Identifier max("max");
	static const Identifier midPos("middlePosition");
	static const Identifier step("stepSize");
	static const Identifier isMidi("allowMidiAutomation");
	static const Identifier isHost("allowHostAutomation");
	static const Identifier connections("connections");
	static const Identifier processorId("processorId");
	static const Identifier parameterId("parameterId");

	id = d[id_].toString();

	allowMidi = (bool)d[isMidi];
	allowHost = (bool)d[isHost];

	range.start = (float)d.getProperty(min, 0.0f);
	range.end = (float)d.getProperty(max, 1.0f);

	if (d.hasProperty(midPos))
		range.setSkewForCentre((float)d[midPos]);

	range.interval = (float)d.getProperty(step, 0.0f);

	auto cArray = d[connections];

	if (cArray.isArray())
	{
		for (const auto& c : *cArray.getArray())
		{
			auto pId = c[processorId].toString();
			auto paramId = c[parameterId].toString();

			if (pId.isNotEmpty() && paramId.isNotEmpty())
			{
				ProcessorConnection pc;

				if (pc.connectedProcessor = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), pId))
					pc.connectedParameterIndex = pc.connectedProcessor->getParameterIndexForIdentifier(paramId);

				if (pc)
					processorConnections.add(std::move(pc));
			}
		}
	}

	args[0] = index;
	args[1] = var(0.0f);

	if (id.toString().isEmpty())
		r = Result::fail("No ID");

	asyncListeners.enableLockFreeUpdate(mc->getGlobalUIUpdater());
}

void MainController::UserPresetHandler::CustomAutomationData::call(float newValue)
{
	FloatSanitizers::sanitizeFloatNumber(newValue);

	newValue = range.getRange().clipValue(newValue);
	newValue = range.snapToLegalValue(newValue);
	lastValue = newValue;
	args[0] = index;
	args[1] = lastValue;

	for (const auto& pc : processorConnections)
		pc.call(newValue);

	syncListeners.sendMessage(sendNotificationSync, args);
	asyncListeners.sendMessage(sendNotificationAsync, index, lastValue);
}

void MainController::UserPresetHandler::CustomAutomationData::ProcessorConnection::call(float v) const
{
	jassert(connectedProcessor != nullptr);

	if (*this)
		connectedProcessor.get()->setAttribute(connectedParameterIndex, v, sendNotification);
}



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

		preprocess(pendingPreset);

		// Send a note off to stop the arpeggiator etc...
		mc->allNotesOff(false);

		mc->killAndCallOnLoadingThread(f);
	}
}

void MainController::UserPresetHandler::preprocess(ValueTree& presetToLoad)
{
	for (auto l : listeners)
	{
		if (l != nullptr)
			presetToLoad = l.get()->prePresetLoad(presetToLoad, currentlyLoadedFile);
	}
}

void MainController::UserPresetHandler::loadUserPreset(const File& f)
{
	auto xml = XmlDocument::parse(f);

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

				if (mc->getUserPresetHandler().isUsingCustomDataModel())
				{
					mc->getUserPresetHandler().loadCustomValueTree(userPresetToLoad);
				}
				else
				{
					for (auto c : userPresetToLoad)
					{
						if (c.getProperty("Processor") == sp->getId())
						{
							v = c;
							break;
						}
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

		postPresetLoad();
	}

	mc->getSampleManager().preloadEverything();
}

void MainController::UserPresetHandler::postPresetLoad()
{
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




bool MainController::UserPresetHandler::isReadOnly(const File& f)
{
#if READ_ONLY_FACTORY_PRESETS
	return factoryPaths->contains(mc, f);
#else
	return false;
#endif
}

void MainController::UserPresetHandler::loadCustomValueTree(const ValueTree& presetData)
{
	auto v = presetData.getChildWithName("CustomJSON");
	if (v.isValid())
	{
		auto obj = JSON::parse(v["Data"].toString());

		if (obj.isObject() || obj.isArray())
		{
			for (auto l : listeners)
			{
				l->loadCustomUserPreset(obj);
			}
		}
	}
}

juce::StringArray MainController::UserPresetHandler::getCustomAutomationIds() const
{
	StringArray sa;
	for (auto l : customAutomationData)
	{
		sa.add(l->id.toString());
	}

	return sa;
}

int MainController::UserPresetHandler::getCustomAutomationIndex(const Identifier& id) const
{
	int index = 0;

	for (auto l : customAutomationData)
	{
		if (l->id == id)
			return index;

		index++;
	}
    
    return -1;
}

juce::ValueTree MainController::UserPresetHandler::createCustomValueTree(const String& presetName)
{
	jassert(isUsingCustomData);

	for (auto l : listeners)
	{
		auto obj = l->saveCustomUserPreset(presetName);

		if (obj.isObject())
		{
			ValueTree v("CustomJSON");
			auto data = JSON::toString(obj, true);
			v.setProperty("Data", data, nullptr);
			return v;
		}
	}

	return {};
}

bool MainController::UserPresetHandler::setCustomAutomationData(CustomAutomationData::List newList)
{
	if (isUsingCustomData)
	{
		customAutomationData.swapWith(newList);
		return true;
	}
	
	return false;
}

MainController::UserPresetHandler::CustomAutomationData::Ptr MainController::UserPresetHandler::getCustomAutomationData(const Identifier& id)
{
	for (auto l : customAutomationData)
	{
		if (l->id == id)
			return l;
	}

	return nullptr;
}

void MainController::UserPresetHandler::setUseCustomDataModel(bool shouldUseCustomModel, bool shouldUsePersistentObject)
{
	isUsingCustomData = shouldUseCustomModel;
	usePersistentObject = shouldUsePersistentObject;
}

#if READ_ONLY_FACTORY_PRESETS
bool MainController::UserPresetHandler::FactoryPaths::contains(MainController* mc, const File& f)
{
	if (!initialised)
		initialise(mc);

	auto path = getPath(mc, f);

	if (path.isNotEmpty())
	{
		auto ok = factoryPaths.contains(path);
		return ok;
	}


	return false;
}

void MainController::UserPresetHandler::FactoryPaths::initialise(MainController* mc)
{
#if USE_FRONTEND
	ScopedPointer<MemoryInputStream> mis = FrontendFactory::getEmbeddedData(FileHandlerBase::UserPresets);
	zstd::ZCompressor<UserPresetDictionaryProvider> decompressor;
	MemoryBlock mb(mis->getData(), mis->getDataSize());
	ValueTree presetTree;
	decompressor.expand(mb, presetTree);

	for (auto c : presetTree)
		addRecursive(c, "{PROJECT_FOLDER}");
#endif

	initialised = true;

}

String MainController::UserPresetHandler::FactoryPaths::getPath(MainController* mc, const File& f)
{
	auto factoryFolder = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::UserPresets);

	if (f.isAChildOf(factoryFolder))
		return "{PROJECT_FOLDER}" + f.withFileExtension("").getRelativePathFrom(factoryFolder).replace("\\", "/");

	for (int i = 0; i < mc->getExpansionHandler().getNumExpansions(); i++)
	{
		auto e = mc->getExpansionHandler().getExpansion(i);
		auto expFolder = e->getSubDirectory(FileHandlerBase::UserPresets);

		if (f.isAChildOf(expFolder))
			return e->getWildcard() + f.withFileExtension("").getRelativePathFrom(expFolder).replace("\\", "/");
	}

	return {};
}

void MainController::UserPresetHandler::FactoryPaths::addRecursive(const ValueTree& v, const String& path)
{
	if (v["isDirectory"])
	{
		String thisPath;

		thisPath << path << v["FileName"].toString();

		for (auto c : v)
		{
			addRecursive(c, thisPath + "/");
		}

		this->factoryPaths.addIfNotAlreadyThere(thisPath);
	}
	if (v.getType() == Identifier("PresetFile"))
	{
		String thisFile;

		thisFile << path << v["FileName"].toString();

		this->factoryPaths.addIfNotAlreadyThere(thisFile);
		return;
	}
}
#endif



} // namespace hise
