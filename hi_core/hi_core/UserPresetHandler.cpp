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

struct MainController::UserPresetHandler::CustomAutomationData::CableConnection: 
	public MainController::UserPresetHandler::CustomAutomationData::ConnectionBase,
	public scriptnode::routing::GlobalRoutingManager::CableTargetBase
																	
{
	CableConnection(scriptnode::routing::GlobalRoutingManager::SlotBase::Ptr c) :
		cable(c)
	{
		if (auto typed = dynamic_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(cable.get()))
		{
			typed->addTarget(this);
		}
	}

	~CableConnection()
	{
		if (auto typed = dynamic_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(cable.get()))
		{
			typed->removeTarget(this);
		}
	}

	bool isValid() const final override
	{
		return cable != nullptr;
	}

	void sendValue(double v) override
	{
		if (parent != nullptr)
		{
			v = parent->range.convertFrom0to1((float)v);

			ScopedValueSetter<bool> svs(recursive, true);
			parent->call(v, dispatch::DispatchType::sendNotificationSync);
		}
	}

	Path getTargetIcon() const override
	{
		return {};
	}

	void selectCallback(Component* rootEditor) override
	{

	}

	String getTargetId() const override
	{
		return "Automation";
	}

	void call(float v, dispatch::DispatchType) const final override
	{
		if (isValid() && !recursive)
		{
			auto unconst = const_cast<CableConnection*>(this);
			v = r.convertTo0to1(v);
			static_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(cable.get())->sendValue(unconst, (double)v);
		}
	}

	String getDisplayString() const final override
	{
		if (isValid())
			return "Cable: " + static_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(cable.get())->id;

		return "Unknown cable";
	}

	float getLastValue() const final override
	{
		if (cable != nullptr)
			return (float)static_cast<scriptnode::routing::GlobalRoutingManager::Cable*>(cable.get())->lastValue;

		return 0.0f;
	}

	NormalisableRange<float> r;
	scriptnode::routing::GlobalRoutingManager::SlotBase::Ptr cable;

	CustomAutomationData::WeakPtr parent;
	bool recursive = false;
};

MainController::UserPresetHandler::CustomAutomationData::CustomAutomationData(CustomAutomationData::List newList, MainController* mc, int index_, const var& d) :
	ControlledObject(mc),
	index(index_),
	NEW_AUTOMATION_WITH_COMMA(dispatcher(mc->getCustomAutomationSourceManager(), *this, index, dispatch::HashedCharPtr(d["ID"].toString())))
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
	

	id = d[id_].toString();

	allowMidi = (bool)d.getProperty(isMidi, true);
	allowHost = (bool)d.getProperty(isHost, true);

	range.start = (float)d.getProperty(min, 0.0f);
	range.end = (float)d.getProperty(max, 1.0f);

	if (d.hasProperty(midPos))
		range.setSkewForCentre((float)d[midPos]);

	range.interval = (float)d.getProperty(step, 0.0f);

	auto cArray = d[connections];

	if (cArray.isArray())
	{
		try
		{
			for (const auto& c : *cArray.getArray())
				connectionList.add(parse(newList, mc, c));

			for (auto c : connectionList)
			{
				if (auto cc = dynamic_cast<CableConnection*>(c))
					cc->parent = this;
			}
		}
		catch (String& error)
		{
			r = Result::fail(error);
		}
	}
	else
	{
		r = Result::fail("No connections");
	}

	args[0] = index;
	args[1] = var(lastValue);

    if (id.isEmpty())
		r = Result::fail("No ID");

#if USE_OLD_AUTOMATION_DISPATCH
	asyncListeners.enableLockFreeUpdate(mc->getGlobalUIUpdater());
    syncListeners.setLockListenersDuringMessage(true);
    asyncListeners.setLockListenersDuringMessage(true);
	asyncListeners.sendMessage(dontSendNotification, index, lastValue);
	syncListeners.sendMessage(dontSendNotification, args);
#endif

	dispatcher.setValue(lastValue, dispatch::DispatchType::dontSendNotification);
}

hise::MainController::UserPresetHandler::CustomAutomationData::ConnectionBase::Ptr MainController::UserPresetHandler::CustomAutomationData::parse(CustomAutomationData::List newList, MainController* mc, const var& c)
{
	static const Identifier processorId("processorId");
	static const Identifier parameterId("parameterId");
	static const Identifier automationId("automationId");
	static const Identifier cableId("cableId");

	auto pId = c[processorId].toString();
	auto paramId = c[parameterId].toString();
	
	

	if (pId.isNotEmpty() && paramId.isNotEmpty())
	{
		auto pc = new ProcessorConnection();

		if ((pc->connectedProcessor = ProcessorHelpers::getFirstProcessorWithName(mc->getMainSynthChain(), pId)))
			pc->connectedParameterIndex = pc->connectedProcessor->getParameterIndexForIdentifier(paramId);

		if (pc->isValid())
		{
			lastValue = pc->connectedProcessor->getAttribute(pc->connectedParameterIndex);
			return pc;
		}
		else
		{
			throw String("Can't find processor / parameter with ID " + pId + "." + paramId);
		}
	}
    
    auto automId = c[automationId].toString();
    
    if (automId.isNotEmpty())
	{
		for (auto l : newList)
		{
			if (l->id == automId)
			{
				auto p = new MetaConnection();
				p->target = l;
				return p;
			}
		}
		
		throw String("Can't find automation ID for meta automation: " + automId);
	}
    
    auto cId = c[cableId].toString();
    
    if (cId.isNotEmpty())
	{
		if (auto m = scriptnode::routing::GlobalRoutingManager::Helpers::getOrCreate(mc))
		{
			for (auto c : m->cables)
			{
				if (c->id == cId)
				{
					auto cc = new CableConnection(c);
					cc->cable = c;
					
					return cc;
				}
			}

			auto c = m->getSlotBase(cId, scriptnode::routing::GlobalRoutingManager::SlotBase::SlotType::Cable);

			auto cc = new CableConnection(c);
			cc->cable = c;
			return cc;
		}
	}

	throw String("unknown target type: " + JSON::toString(c, true));
}

void MainController::UserPresetHandler::CustomAutomationData::updateFromConnectionValue(int preferredIndex)
{
	preferredIndex = jlimit(0, connectionList.size() - 1, preferredIndex);

	if (auto c = connectionList[preferredIndex])
	{
		auto newValue = c->getLastValue();

		FloatSanitizers::sanitizeFloatNumber(newValue);

		lastValue = newValue;
		args[0] = index;
		args[1] = newValue;

#if USE_OLD_AUTOMATION_DISPATCH
		syncListeners.sendMessage(sendNotificationSync, args);
		asyncListeners.sendMessage(sendNotificationAsync, index, newValue);
#endif

		IF_NEW_AUTOMATION_DISPATCH(dispatcher.setValue(newValue, dispatch::DispatchType::sendNotificationSync));

	}
}

bool MainController::UserPresetHandler::CustomAutomationData::isConnectedToMidi() const
{
	if (!allowMidi)
		return false;

	auto handler = getMainController()->getMacroManager().getMidiControlAutomationHandler();

	for (int i = 0; i < handler->getNumActiveConnections(); i++)
	{
		auto d = handler->getDataFromIndex(i);

		if (d.used && d.attribute == index)
			return true;
	}

	return false;
}

bool MainController::UserPresetHandler::CustomAutomationData::isConnectedToComponent() const
{
#if USE_NEW_AUTOMATION_DISPATCH
	return false;//return dispatcher.getNumListenersWithClass<ScriptingApi::Content::ScriptComponent>() != 0;
#elif USE_OLD_AUTOMATION_DISPATCH
	return asyncListeners.template getNumListenersWithClass<ScriptingApi::Content::ScriptComponent>() != 0;
#else
	return false;
#endif


	
}

void MainController::UserPresetHandler::CustomAutomationData::call(float newValue, dispatch::DispatchType n, const std::function<bool(ConnectionBase*)>& connectionFilter)
{
	bool sendToListeners = n != dispatch::DispatchType::dontSendNotification;

	FloatSanitizers::sanitizeFloatNumber(newValue);

	newValue = range.getRange().clipValue(newValue);
	newValue = range.snapToLegalValue(newValue);
	lastValue = newValue;
	args[0] = index;
	args[1] = lastValue;

	if (sendToListeners)
	{
		for (auto pc : connectionList)
		{
			if(!connectionFilter || connectionFilter(pc))
				pc->call(newValue, n);
		}
	}	

	IF_OLD_AUTOMATION_DISPATCH(syncListeners.sendMessage(sendNotificationSync, args));
	IF_OLD_AUTOMATION_DISPATCH(asyncListeners.sendMessage(sendNotificationAsync, index, lastValue));
	IF_NEW_AUTOMATION_DISPATCH(dispatcher.setValue(lastValue, n));
}


void MainController::UserPresetHandler::CustomAutomationData::ProcessorConnection::call(float v, dispatch::DispatchType n) const
{
	jassert(connectedProcessor != nullptr);

	if (*this)
		connectedProcessor.get()->setAttribute(connectedParameterIndex, v, n);
}

String MainController::UserPresetHandler::CustomAutomationData::ProcessorConnection::getDisplayString() const
{
	String id;

	if (connectedProcessor != nullptr)
	{
		id << connectedProcessor->getId() << "::" << connectedProcessor->getIdentifierForParameterIndex(connectedParameterIndex).toString();
	}
	else
		id << "Dangling connection";

	return id;
}

float MainController::UserPresetHandler::CustomAutomationData::ProcessorConnection::getLastValue() const
{
	if (isValid())
	{
		return connectedProcessor->getAttribute(connectedParameterIndex);
	}

	return 0.0f;
}


MainController::UserPresetHandler::UserPresetHandler(MainController* mc_) :
	mc(mc_)
{
	timeOfLastPresetLoad = Time::getMillisecondCounter();
}

void MainController::UserPresetHandler::loadUserPresetFromValueTree(const ValueTree& v, const File& oldFile, const File& newFile, bool useUndoManagerIfEnabled)
{
	if (useUndoManagerIfEnabled && useUndoForPresetLoads)
	{
        mc->getControlUndoManager()->beginNewTransaction();
		mc->getControlUndoManager()->perform(new UndoableUserPresetLoad(mc, oldFile, newFile, v));
	}
	else
	{
		currentlyLoadedFile = newFile;
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

void MainController::UserPresetHandler::loadUserPreset(const File& f, bool useUndoManagerIfEnabled)
{
	auto xml = XmlDocument::parse(f);

	if (xml != nullptr)
	{
		ValueTree v = ValueTree::fromXml(*xml);

		if (v.isValid())
		{
			loadUserPresetFromValueTree(v, currentlyLoadedFile, f, useUndoManagerIfEnabled);
		}
	}
}

File MainController::UserPresetHandler::getCurrentlyLoadedFile() const
{
	return currentlyLoadedFile;
}

	/*
void MainController::UserPresetHandler::setCurrentlyLoadedFile(const File& f)
{
	currentlyLoadedFile = f;
}
*/

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
	
	UserPresetHelpers::saveUserPreset(mc->getMainSynthChain(), currentPresetFile.getFullPathName());
}

void MainController::UserPresetHandler::loadUserPresetInternal()
{
	ScopedValueSetter<void*> svs(currentThreadThatIsLoadingPreset, LockHelpers::getCurrentThreadHandleOrMessageManager());

	{
		LockHelpers::freeToGo(mc);

		timeOfLastPresetLoad = Time::getMillisecondCounter();

		ValueTree userPresetToLoad = pendingPreset;

#if USE_BACKEND
		if (!GET_PROJECT_HANDLER(mc->getMainSynthChain()).isActive()) return;
#endif

		jassert(userPresetToLoad.isValid());

		mc->getSampleManager().setShouldSkipPreloading(true);

		// Reload the macro connections before restoring the preset values
		// so that it will update the correct connections with `setMacroControl()` in a control callback
		if (mc->getMacroManager().isMacroEnabledOnFrontend())
		{
			// If we're in exclusive mode and using the macros as plugin parameter, we will only restore them in internal presets
			if (!HISE_MACROS_ARE_PLUGIN_PARAMETERS || isInternalPresetLoad() || !mc->getMacroManager().isExclusive())
				mc->getMacroManager().getMacroChain()->loadMacrosFromValueTree(userPresetToLoad, false);
		}
			

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

				restoreStateManager(userPresetToLoad, UserPresetIds::Modules);

				if (mc->getUserPresetHandler().isUsingCustomDataModel())
				{
					restoreStateManager(userPresetToLoad, UserPresetIds::CustomJSON);
				}
				else
				{
					ValueTree v;

					for (auto c : userPresetToLoad)
					{
						if (c.getProperty("Processor") == sp->getId())
						{
							v = c;
							break;
						}
					}

					if (v.isValid())
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

		restoreStateManager(userPresetToLoad, UserPresetIds::MidiAutomation);
		restoreStateManager(userPresetToLoad, UserPresetIds::MPEData);

		// Now we can restore the values of the macro controls
		if (mc->getMacroManager().isMacroEnabledOnFrontend())
		{
			if(!HISE_MACROS_ARE_PLUGIN_PARAMETERS || isInternalPresetLoad() || !mc->getMacroManager().isExclusive())
			{
				mc->getMacroManager().getMacroChain()->loadMacroValuesFromValueTree(userPresetToLoad);
			}
		}

		// restore the remaining state managers...
		restoreStateManager(userPresetToLoad, UserPresetIds::AdditionalStates);

		postPresetLoad();
	}

	mc->getSampleManager().preloadEverything();
}

void MainController::UserPresetHandler::postPresetSave()
{
	// Already on the message thread
	jassert(MessageManager::getInstance()->isThisTheMessageThread());

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->presetSaved(currentlyLoadedFile);
	}
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

	if (auto e = FullInstrumentExpansion::getCurrentFullExpansion(mc))
		userDirectory = e->getSubDirectory(FileHandlerBase::UserPresets);

	userDirectory.findChildFiles(allPresets, File::findFiles, true, "*.preset");
    PresetBrowser::DataBaseHelpers::cleanFileList(mc, allPresets);
	allPresets.sort();

	auto expFolder = mc->getExpansionHandler().getExpansionFolder();

	auto wasExpansionPreset = currentlyLoadedFile.isAChildOf(expFolder);

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
		else if(mc->getExpansionHandler().getNumExpansions() > 0)
		{
			for(int i = 0; i < mc->getExpansionHandler().getNumExpansions(); i++)
			{
				auto expansionPresetFolder = mc->getExpansionHandler().getExpansion(i)->getSubDirectory(FileHandlerBase::UserPresets);
				auto thisList = expansionPresetFolder.findChildFiles(File::findFiles, true, "*.preset");
				PresetBrowser::DataBaseHelpers::cleanFileList(mc, thisList);
				thisList.sort();
				allPresets.addArray(thisList);
			}
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

	if(currentlyLoadedFile.isAChildOf(expFolder))
	{
		for(int i = 0; i < mc->getExpansionHandler().getNumExpansions(); i++)
		{
			auto e = mc->getExpansionHandler().getExpansion(i);
			
			if(currentlyLoadedFile.isAChildOf(e->getRootFolder()))
			{
				mc->getExpansionHandler().setCurrentExpansion(e, sendNotificationAsync);
				break;
			}
		}
	}
	else if (wasExpansionPreset)
	{
		mc->getExpansionHandler().setCurrentExpansion(nullptr, sendNotificationAsync);
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

juce::StringArray MainController::UserPresetHandler::getCustomAutomationIds() const
{
	StringArray sa;
	for (auto l : customAutomationData)
	{
		sa.add(l->id);
	}

	return sa;
}

int MainController::UserPresetHandler::getCustomAutomationIndex(const Identifier& id) const
{
	int index = 0;

	for (auto l : customAutomationData)
	{
		if (l->id == id.toString())
			return index;

		index++;
	}
    
    return -1;
}

bool MainController::UserPresetHandler::setCustomAutomationData(CustomAutomationData::List newList)
{
	if (isUsingCustomDataModel())
	{
		customAutomationData.swapWith(newList);

		deferredAutomationListener.sendMessage(sendNotificationSync, true);
		deferredAutomationListener.removeAllListeners();

		return true;
	}
	
	return false;
}

MainController::UserPresetHandler::CustomAutomationData::Ptr MainController::UserPresetHandler::getCustomAutomationData(const Identifier& id) const
{
	for (auto l : customAutomationData)
	{
		if (l->id == id.toString())
			return l;
	}

	return nullptr;
}

void MainController::UserPresetHandler::setUseCustomDataModel(bool shouldUseCustomModel, bool shouldUsePersistentObject)
{
	if (shouldUseCustomModel != isUsingCustomDataModel())
	{
		if (shouldUseCustomModel)
			customStateManager = new CustomStateManager(*this);
		else
		{
			removeStateManager(customStateManager);
			customStateManager = nullptr;
		}
	}

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
