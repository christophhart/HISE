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




SampleMap::SampleMap(ModulatorSampler *sampler_):
	sampler(sampler_),
	notifier(*this),
	currentPool(nullptr),
	sampleMapId(Identifier()),
	data("samplemap"),
	mode(data, Identifier("SaveMode"), nullptr, 0)
{
	data.addListener(this);

	changeWatcher = new ChangeWatcher(data);

	// You have to clear the sound array before you set a new SampleMap!
	jassert(sampler->getNumSounds() == 0);
}

SampleMap::FileList SampleMap::createFileList()
{
	FileList list;

	bool allFound = true;

	String missingFileList = "Missing files:\n";

	for (int i = 0; i < sampler->getNumMicPositions(); i++)
	{
		list.add(new Array<File>());
	}

	{
		SimpleReadWriteLock::ScopedReadLock sl(sampler->getIteratorLock());
		ModulatorSampler::SoundIterator soundIter(sampler);
		jassert(soundIter.canIterate());

		while (auto sound = soundIter.getNextSound())
		{
			for (int i = 0; i < sound->getNumMultiMicSamples(); i++)
			{
				if (auto sample = sound->getReferenceToSound(i))
				{
					File file = sample->getFileName(true);

					if (!file.existsAsFile())
					{
						allFound = false;
						missingFileList << file.getFullPathName() << "\n";
					}

					list[i]->add(file);
				}
			}
		}
	}

	


	if (!allFound)
	{
		
		SystemClipboard::copyTextToClipboard(missingFileList);

		missingFileList << "This message was also copied to the clipboard";

		throw missingFileList;
	}

	return list;
}

SampleMap::~SampleMap()
{
	getCurrentSamplePool()->clearUnreferencedMonoliths();
}

void SampleMap::changeListenerCallback(SafeChangeBroadcaster *)
{
	jassertfalse;
};

void SampleMap::clear(NotificationType n)
{
	LockHelpers::freeToGo(sampler->getMainController());


	ScopedValueSetter<bool> iterationAborter(sampler->getIterationFlag(), true);
	SimpleReadWriteLock::ScopedWriteLock sl(sampler->getIteratorLock());

	ScopedNotificationDelayer snd(*this);

	sampleMapData.clear();

	setNewValueTree(ValueTree("samplemap"));

	mode = Undefined;

	sampleMapId = Identifier();
	changeWatcher = new ChangeWatcher(data);

	sampleMapData = PooledSampleMap();

	if (currentPool != nullptr)
		currentPool->removeListener(this);

	currentPool = nullptr;
	currentMonolith = nullptr;

	if (sampler != nullptr)
	{
		sampler->sendChangeMessage();
		getCurrentSamplePool()->sendChangeMessage();
	}

	if (n != dontSendNotification)
		sendSampleMapChangeMessage(n);
}

juce::String SampleMap::getMonolithID() const
{
	return data.getProperty("MonolithReference", sampleMapId.toString()).toString();
}

hise::FileHandlerBase* SampleMap::getCurrentFileHandler() const
{
	FileHandlerBase* handler = &GET_PROJECT_HANDLER(sampler);

	if (handler->getMainController()->getExpansionHandler().isEnabled() && currentPool != nullptr)
		handler = currentPool->getFileHandler();

	return handler;
}

hise::ModulatorSamplerSoundPool* SampleMap::getCurrentSamplePool() const
{
	return getCurrentFileHandler()->pool->getSamplePool();
}

void SampleMap::setCurrentMonolith()
{
	if (isMonolith())
	{
		ModulatorSamplerSoundPool* pool = getCurrentFileHandler()->pool->getSamplePool();

		if (auto existingInfo = pool->getMonolith(getMonolithID()))
		{
			currentMonolith = existingInfo;

			jassert(*currentMonolith == getMonolithID());
		}
		else
		{
			File monolithDirectories[2];

			// First check in the expansion folder, then in the global sample folder...
			monolithDirectories[0] = getCurrentFileHandler()->getSubDirectory(ProjectHandler::SubDirectories::Samples);
			monolithDirectories[1] = sampler->getMainController()->getCurrentFileHandler().getSubDirectory(ProjectHandler::SubDirectories::Samples);


			if (!monolithDirectories[1].isDirectory())
			{
				sampler->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage,
					"The sample directory does not exist");

				FRONTEND_ONLY(sampler->deleteAllSounds());

				return;
			}

			Array<File> monolithFiles;

			int numSingleChannelSplits = (int)data.getProperty("MonolithSplitAmount", 0);

			int numChannels = jmax<int>(1, data.getChild(0).getNumChildren());

			if (numSingleChannelSplits > 0 && numChannels != 1)
			{
				jassertfalse;
			}

			int numMonolithsToLoad = jmax(numChannels, numSingleChannelSplits);

			for (int i = 0; i < numMonolithsToLoad; i++)
			{
				auto path = getMonolithID().replace("/", "_");

				File f = monolithDirectories[0].getChildFile(path + ".ch" + String(i + 1));

				if(!f.existsAsFile() && monolithDirectories[0] != monolithDirectories[1])
					f = monolithDirectories[1].getChildFile(path + ".ch" + String(i + 1));

				if (f.existsAsFile())
				{
					monolithFiles.add(f);
				}
				else
				{
					sampler->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage,
						"The sample " + f.getFileName() + " wasn't found");

					FRONTEND_ONLY(sampler->deleteAllSounds());

					return;
				}
			}

			if (!monolithFiles.isEmpty())
			{
				if (numChannels != 1)
				{
					StringArray micPositions = StringArray::fromTokens(data.getProperty("MicPositions").toString(), ";", "");

					micPositions.removeEmptyStrings(true);

					if (micPositions.size() == numChannels)
						sampler->setNumMicPositions(micPositions);
					else
						sampler->setNumChannels(1);
				}
				else
				{
					sampler->setNumChannels(1);
				}

				currentMonolith = pool->loadMonolithicData(data, monolithFiles);

				if(currentMonolith)
					jassert(*currentMonolith == getMonolithID());
			}
		}
	}
}

void SampleMap::parseValueTree(const ValueTree &v)
{
	LockHelpers::freeToGo(sampler->getMainController());
	ScopedValueSetter<bool> iterationAborter(sampler->getIterationFlag(), true);
	SimpleReadWriteLock::ScopedWriteLock sl(sampler->getIteratorLock());

	setNewValueTree(v);
	
	mode.referTo(data, "SaveMode", nullptr);

	const String sampleMapName = v.getProperty("ID");
	sampleMapId = sampleMapName.isEmpty() ? Identifier::null : Identifier(sampleMapName);

	setCurrentMonolith();

	const int newRoundRobinAmount = v.getProperty("RRGroupAmount", 1);

	sampler->setRRGroupAmount(newRoundRobinAmount);

	int numChannels = jmax<int>(1, data.getChild(0).getNumChildren());

	StringArray micPositions = StringArray::fromTokens(data.getProperty("MicPositions").toString(), ";", "");

	micPositions.removeEmptyStrings(true);

	if (!sampler->isUsingStaticMatrix())
	{
		if (micPositions.size() != 0)
		{
			sampler->setNumMicPositions(micPositions);
		}
		else
		{
			sampler->setNumChannels(numChannels);
		}
	}

	auto& progress = getSampler()->getMainController()->getSampleManager().getPreloadProgress();

	auto numSamples = (double)(jmax<int>(1, data.getNumChildren()));
	auto sampleIndex = 0.0;

	ScopedNotificationDelayer dnd(*this);

	for (auto c : data)
	{
		progress = sampleIndex / numSamples;
		sampleIndex += 1.0;

		valueTreeChildAdded(data, c);
	}

	sampler->updateRRGroupAmountAfterMapLoad();
	if(!sampler->isRoundRobinEnabled()) sampler->refreshRRMap();
	
	sampler->refreshMemoryUsage();
	
};

const ValueTree SampleMap::getValueTree() const
{
	return data;
}

void SampleMap::poolEntryReloaded(PoolReference referenceThatWasChanged)
{
	if (getReference() == referenceThatWasChanged)
	{
		auto f = [referenceThatWasChanged](Processor* p)
		{
			auto s = static_cast<ModulatorSampler*>(p);

			s->getSampleMap()->clear(dontSendNotification);
			s->loadSampleMap(referenceThatWasChanged);

			return SafeFunctionCall::OK;
		};

		sampler->killAllVoicesAndCall(f);
	}
}



hise::ModulatorSamplerSound* SampleMap::getSound(int index)
{
	auto s = sampler->getSound(index);

	if (s != nullptr)
		return static_cast<ModulatorSamplerSound*>(s);

	return nullptr;
}

const hise::ModulatorSamplerSound* SampleMap::getSound(int index) const
{
	auto s = sampler->getSound(index);

	if (s != nullptr)
		return static_cast<ModulatorSamplerSound*>(s);

	return nullptr;
}

int SampleMap::getNumRRGroups() const
{
	return (int)getSampler()->getAttribute(ModulatorSampler::RRGroupAmount);
}

void SampleMap::discardChanges()
{
	auto pool = sampler->getMainController()->getCurrentSampleMapPool();
	pool->loadFromReference(getReference(), PoolHelpers::ForceReloadStrong);
}

void SampleMap::saveAndReloadMap()
{
	auto f = getReference().getFile();

	ScopedPointer<XmlElement> xml = data.createXml();
	xml->writeToFile(f, "");

	auto pool = sampler->getMainController()->getCurrentSampleMapPool();
	pool->removeListener(this);
	pool->loadFromReference(getReference(), PoolHelpers::ForceReloadStrong);
	pool->addListener(this);

	changeWatcher = new ChangeWatcher(data);
}

void SampleMap::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
	if (treeWhosePropertyHasChanged == data)
		return;

	auto i = data.indexOf(treeWhosePropertyHasChanged);

	if (i != -1)
	{
		notifier.addPropertyChange(i, property, treeWhosePropertyHasChanged.getProperty(property));
	}
}

void SampleMap::valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
{
	static const Identifier sa("sample");

	if (parentTree.getType() == sa)
	{
		return;
	}
		

	ignoreUnused(parentTree);
	jassert(parentTree == data);

	ValueTree child = childWhichHasBeenAdded;

	auto f = [child](Processor* p)
	{
		static_cast<ModulatorSampler*>(p)->getSampleMap()->addSampleFromValueTree(child);
		return SafeFunctionCall::OK;
	};

	if (syncEditMode)
		f(sampler);
	else
		sampler->killAllVoicesAndCall(f);
}

void SampleMap::addSampleFromValueTree(ValueTree childWhichHasBeenAdded)
{
	auto map = sampler->getSampleMap();

	auto newSound = new ModulatorSamplerSound(map, childWhichHasBeenAdded, map->currentMonolith);

	{
		LockHelpers::SafeLock sl(sampler->getMainController(), LockHelpers::SampleLock);
		sampler->addSound(newSound);
	}

	dynamic_cast<ModulatorSamplerSound*>(newSound)->initPreloadBuffer((int)sampler->getAttribute(ModulatorSampler::PreloadSize));

	const bool isReversed = sampler->getAttribute(ModulatorSampler::Reversed) > 0.5f;

	newSound->setReversed(isReversed);

	sendSampleAddedMessage();
	return;
}

void SampleMap::sendSampleAddedMessage()
{
	auto update = [](Dispatchable* obj)
	{
		auto sampler = static_cast<ModulatorSampler*>(obj);

		jassert_dispatched_message_thread(sampler->getMainController());

		ModulatorSamplerSoundPool* pool = sampler->getSampleMap()->getCurrentSamplePool();
		pool->sendChangeMessage();
		sampler->sendChangeMessage();

		sampler->getSampleMap()->notifier.sendSampleAmountChangeMessage(sendNotificationAsync);

		return Dispatchable::Status::OK;
	};

	if (delayNotifications)
		notificationPending = true;
	else
		sampler->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(sampler, update);
}

void SampleMap::valueTreeChildRemoved(ValueTree& /*parentTree*/, ValueTree& child, int /*indexFromWhichChildWasRemoved*/)
{
	auto f = [child](Processor* s)
	{
		auto sampler = static_cast<ModulatorSampler*>(s);
		LockHelpers::freeToGo(sampler->getMainController());

		for (int i = 0; i < sampler->getNumSounds(); i++)
		{
			if (static_cast<ModulatorSamplerSound*>(sampler->getSound(i))->getData() == child)
			{
				sampler->deleteSound(i);
				break;
			}
		}

		if (!sampler->shouldDelayUpdate())
		{
			sampler->getSampleMap()->sendSampleDeletedMessage(sampler);
		}

		return SafeFunctionCall::OK;
	};

	if (syncEditMode)
		f(sampler);
	else
		sampler->killAllVoicesAndCall(f);
}

void SampleMap::sendSampleDeletedMessage(ModulatorSampler * s)
{
	auto update = [](Dispatchable* obj)
	{
		auto sampler = static_cast<ModulatorSampler*>(obj);

		sampler->getSampleMap()->notifier.sendSampleAmountChangeMessage(sendNotificationAsync);

		return Dispatchable::Status::OK;
	};

	if (delayNotifications)
		notificationPending = true;
	else
		s->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(s, update);
}

bool SampleMap::save(const File& fileToUse)
{
#if HI_ENABLE_EXPANSION_EDITING
	auto rootDirectory = sampler->getSampleEditHandler()->getCurrentSampleMapDirectory();

	if (fileToUse.existsAsFile())
	{
		auto id = fileToUse.getRelativePathFrom(rootDirectory).upToFirstOccurrenceOf(".xml", false, false);
		setId(Identifier(id));
	}

	data.setProperty("ID", sampleMapId.toString(), nullptr);
	data.setProperty("RRGroupAmount", sampler->getAttribute(ModulatorSampler::Parameters::RRGroupAmount), nullptr);
	data.setProperty("MicPositions", sampler->getStringForMicPositions(), nullptr);
	
	File f;

	if (fileToUse.existsAsFile())
	{
		f = fileToUse;
	}
	else
	{
		if (isUsingUnsavedValueTree())
		{
			FileChooser fc("Save SampleMap As", rootDirectory, "*.xml", true);

			if (fc.browseForFileToSave(true))
			{
				f = fc.getResult();

				if (!f.isAChildOf(rootDirectory))
				{
					PresetHandler::showMessageWindow("Invalid Path", "You need to store the samplemap in the samplemap directory", PresetHandler::IconType::Error);
					return false;
				}

				auto id = f.getRelativePathFrom(rootDirectory).upToFirstOccurrenceOf(".xml", false, false);
				setId(Identifier(id));
			}
		}
		else
		{
			f = sampleMapData.getRef().getFile();

			jassert(f.existsAsFile());

			if (!PresetHandler::showYesNoWindow("Overwrite SampleMap", "Press OK to overwrite the current samplemap or cancel to select another file"))
			{

				FileChooser fc("Save SampleMap As", f, "*.xml", true);

				if (fc.browseForFileToSave(true))
					f = fc.getResult();
				else
					return false;
			}
		}

	}

	ScopedPointer<XmlElement> xml = data.createXml();
	f.replaceWithText(xml->createDocument(""));

	PoolReference ref(getSampler()->getMainController(), f.getFullPathName(), FileHandlerBase::SubDirectories::SampleMaps);

	auto pool = getSampler()->getMainController()->getCurrentSampleMapPool();

	auto reloadedMap = pool->loadFromReference(ref, PoolHelpers::LoadingType::ForceReloadStrong);

	auto tmp = reloadedMap.getRef();

	auto func = [tmp](Processor* p)
	{
		static_cast<ModulatorSampler*>(p)->loadSampleMap(tmp);

		return SafeFunctionCall::OK;
	};

	getSampler()->killAllVoicesAndCall(func, true);

	return true;

#else
    return false;
#endif
}


void SampleMap::saveSampleMapAsReference()
{
	sampleMapData = {};
	data.setProperty("MonolithReference", data.getProperty("ID"), nullptr);
	save();
}

void SampleMap::saveAsMonolith(Component* mainEditor)
{
#if HI_ENABLE_EXPANSION_EDITING
	mode = SaveMode::Monolith;

	auto m = new MonolithExporter(this);

	m->setModalBaseWindowComponent(mainEditor);

	changeWatcher = new ChangeWatcher(data);
#else
	ignoreUnused(mainEditor);
#endif
}

void SampleMap::setNewValueTree(const ValueTree& v)
{
	LockHelpers::freeToGo(sampler->getMainController());

	data.removeListener(this);

	sampler->deleteAllSounds();
	notifier.sendSampleAmountChangeMessage(sendNotificationAsync);

	data = v;
	data.addListener(this);
}

void SampleMap::addSound(ValueTree& newSoundData)
{
	LockHelpers::freeToGo(sampler->getMainController());

	data.addChild(newSoundData, -1, sampler->getUndoManager());
}


void SampleMap::removeSound(ModulatorSamplerSound* s)
{
	LockHelpers::freeToGo(sampler->getMainController());

	data.removeChild(s->getData(), getSampler()->getUndoManager());
}

juce::String SampleMap::checkReferences(MainController* mc, ValueTree& v, const File& sampleRootFolder, Array<File>& sampleList)
{
	if (v.getNumChildren() == 0)
		return String();

	if (!sampleRootFolder.isDirectory())
		return "Sample Root folder does not exist";

	const bool isMonolith = (int)v.getProperty("SaveMode") == (int)SaveMode::Monolith;

	const std::string channelNames = v.getProperty("MicPositions").toString().toStdString();
	const size_t numChannels = std::count(channelNames.begin(), channelNames.end(), ';');

	const String sampleMapName = v.getProperty("ID").toString().replace("/", "_");

	if (isMonolith)
	{
		for (size_t i = 0; i < numChannels; i++)
		{
			const String fileName = sampleMapName + ".ch" + String(i + 1);

			File f = sampleRootFolder.getChildFile(fileName);


			if (!f.existsAsFile())
			{
				return f.getFullPathName();
			}
		}
	}
	else
	{
		if (numChannels == 1)
		{
			for (int i = 0; i < v.getNumChildren(); i++)
			{
				ValueTree sample = v.getChild(i);

				PoolReference ref(mc, sample.getProperty("FileName"), ProjectHandler::SubDirectories::Samples);

#if USE_BACKEND
				if (ref.isAbsoluteFile())
				{
					PresetHandler::showMessageWindow("Absolute File path detected", "The sample " + ref.getReferenceString() + " is a absolute path which will not be resolved when using the library on another system", PresetHandler::IconType::Error);
					return ref.getReferenceString();
				}
#else
				if (ref.isAbsoluteFile())
				{
					// You have managed to get an absolute file path in your plugin. This will prevent execution on other computers than your own, 
					// but apart from that restriction, everything is fine...
					jassertfalse;
					return ref.getReferenceString();
				}
#endif


				File sampleLocation = ref.getFile();

				if (!sampleList.contains(sampleLocation))
				{
					return sampleLocation.getFullPathName();
				}
			}
		}
		else
		{
			for (int i = 0; i < v.getNumChildren(); i++)
			{
				ValueTree sample = v.getChild(i);

				for (int j = 0; j < sample.getNumChildren(); j++)
				{
					PoolReference ref(mc, sample.getChild(j).getProperty("FileName"), ProjectHandler::SubDirectories::Samples);

					if (ref.isAbsoluteFile())
					{
						PresetHandler::showMessageWindow("Absolute File path detected", "The sample " + ref.getReferenceString() + " is a absolute path which will not be resolved when using the library on another system", PresetHandler::IconType::Error);
						return ref.getReferenceString();
					}

					File sampleLocation = ref.getFile();

					if (!sampleList.contains(sampleLocation))
					{
						return sampleLocation.getFullPathName();
					}
				}
			}
		}
	}

	return String();
}


void SampleMap::load(const PoolReference& reference)
{
	LockHelpers::freeToGo(sampler->getMainController());

	ScopedValueSetter<bool> iterationAborter(sampler->getIterationFlag(), true);
	SimpleReadWriteLock::ScopedWriteLock sl(sampler->getIteratorLock());
	clear(dontSendNotification);

	currentPool = getSampler()->getMainController()->getCurrentSampleMapPool();

	if (auto expansion = getSampler()->getMainController()->getExpansionHandler().getExpansionForWildcardReference(reference.getReferenceString()))
	{
		currentPool = &expansion->pool->getSampleMapPool();
	}

	sampleMapData = currentPool->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);
	currentPool->addListener(this);

	if (sampleMapData)
	{
		auto v = sampleMapData.getData()->createCopy();

		parseValueTree(v);
		changeWatcher = new ChangeWatcher(data);
	}
	else
		jassertfalse;

	sendSampleMapChangeMessage();

}

void SampleMap::loadUnsavedValueTree(const ValueTree& v)
{
	LockHelpers::freeToGo(sampler->getMainController());

	clear(dontSendNotification);

	parseValueTree(v);

	currentPool = nullptr;
	sampleMapData = PooledSampleMap();
	
	parseValueTree(data);
	changeWatcher = new ChangeWatcher(data);


	sendSampleMapChangeMessage();
}

RoundRobinMap::RoundRobinMap()
{
	clear();
}

void RoundRobinMap::clear()
{
	for (int i = 0; i < 128; i++)
	{
		for (int j = 0; j < 128; j++)
		{
			internalData[i][j] = 0;
		}
	}
}

void RoundRobinMap::addSample(const ModulatorSamplerSound *sample)
{
	if (sample->isMissing() || sample->isPurged()) return; 

	Range<int> veloRange = sample->getVelocityRange();
	Range<int> noteRange = sample->getNoteRange();

	char thisGroup = (char)sample->getRRGroup();

	for (int i = noteRange.getStart(); i < noteRange.getEnd(); i++)
	{
		for (int j = veloRange.getStart(); j < veloRange.getEnd(); j++)
		{
			char maxGroup = internalData[i][j];

			if (thisGroup > maxGroup)
			{
				internalData[i][j] = thisGroup;
			}
		}
	}
}

int RoundRobinMap::getRRGroupsForMessage(int noteNumber, int velocity)
{
	if (noteNumber < 128 && noteNumber >= 0 && velocity >= 0 && velocity < 128)
	{
		return (int)internalData[noteNumber][velocity];
	}
	else return -1;
	
}

#if HI_ENABLE_EXPANSION_EDITING
MonolithExporter::MonolithExporter(SampleMap* sampleMap_) :
	DialogWindowWithBackgroundThread("Convert SampleMap to HLAC monolith"),
	AudioFormatWriter(nullptr, "", 0.0, 0, 1),
	sampleMapDirectory(sampleMap_->getSampler()->getSampleEditHandler()->getCurrentSampleMapDirectory())
{
	setSampleMap(sampleMap_);

	monolithDirectory = sampleMap->getCurrentFileHandler()->getSubDirectory(FileHandlerBase::Samples);
	
	if (!monolithDirectory.isDirectory()) monolithDirectory.createDirectory();

	File fileToUse;
	auto ref = sampleMap_->getReference();
	
	if(ref.isValid())
		fileToUse = ref.getFile();
	else
		fileToUse = sampleMapDirectory;

	fc = new FilenameComponent("Sample Map File", fileToUse, false, false, true, "*.xml", "", "SampleMap File");

	fc->setSize(400, 24);

	addCustomComponent(fc);

	StringArray sa2;

	sa2.add("No normalisation");
	sa2.add("Normalise every sample");
	sa2.add("Full Dynamics");

	addComboBox("normalise", sa2, "Normalization");

	if (GET_HISE_SETTING(sampleMap->getSampler(), HiseSettings::Project::SupportFullDynamicsHLAC))
		getComboBoxComponent("normalise")->setSelectedItemIndex(2, dontSendNotification);

	addBasicComponents(true);
}


MonolithExporter::MonolithExporter(const String &name, ModulatorSynthChain* chain) :
	DialogWindowWithBackgroundThread(name),
	AudioFormatWriter(nullptr, "", 0.0, 0, 1),
	
	monolithDirectory(GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::Samples)),
	sampleMap(nullptr)
{
	if (auto firstSampler = ProcessorHelpers::getFirstProcessorWithType<ModulatorSampler>(chain))
	{
		sampleMapDirectory = firstSampler->getSampleEditHandler()->getCurrentSampleMapDirectory();
	}
	else
	{
		sampleMapDirectory = GET_PROJECT_HANDLER(chain).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);
	}
}


void MonolithExporter::run()
{
	sampleMapFile = fc->getCurrentFile();

	if (sampleMapFile == sampleMapDirectory)
	{
		error = "No Sample Map file specified";
		return;
	}

	auto name = sampleMapFile.getRelativePathFrom(sampleMapDirectory).upToFirstOccurrenceOf(".xml", false, true);

	name = name.replace(File::getSeparatorString(), "/");

	sampleMap->setId(name);

	exportCurrentSampleMap(true, true, true);
}

void MonolithExporter::exportCurrentSampleMap(bool overwriteExistingData, bool exportSamples, bool exportSampleMap)
{
	sampleMap->getCurrentSamplePool()->clearUnreferencedMonoliths();

	jassert(sampleMap != nullptr);

	showStatusMessage("Collecting files for samplemap " + sampleMap->getId());

	auto& lock = sampleMap->getSampler()->getMainController()->getSampleManager().getSampleLock();

	try
	{
		MessageManagerLock mm;
		ScopedLock sl(lock);

		filesToWrite = sampleMap->createFileList();
	}
	catch (String errorMessage)
	{
		error = errorMessage;
		return;
	}

	v = sampleMap->getValueTree();
	numSamples = v.getNumChildren();
	numChannels = jmax<int>(1, v.getChild(0).getNumChildren());

	try
	{
		checkSanity();
	}
	catch (String errorMessage)
	{
		error = errorMessage;
		return;
	}

	if (exportSamples)
	{
		for (int i = 0; i < numChannels; i++)
		{
			if (threadShouldExit())
			{
				error = "Export aborted by user";
				return;
			}

			showStatusMessage("Writing Monolith for Channel " + String(i + 1) + "/" + String(numChannels));

			writeFiles(i, overwriteExistingData);
		}
	}
	else
	{
		showStatusMessage("Skipping HLAC Reencoding");
	}

	updateSampleMap();

	if (exportSampleMap)
		writeSampleMapFile(overwriteExistingData);
}

void MonolithExporter::writeSampleMapFile(bool /*overwriteExistingFile*/)
{
	showStatusMessage("Saving Samplemap file");
	
	ScopedPointer<XmlElement> xml = v.createXml();

	sampleMapFile.getParentDirectory().createDirectory();

	xml->writeToFile(sampleMapFile, "");

	auto pool = &sampleMap->getCurrentFileHandler()->pool->getSampleMapPool();

	PoolReference ref(sampleMap->getSampler()->getMainController(), sampleMapFile.getFullPathName(), FileHandlerBase::SampleMaps);

	pool->loadFromReference(ref, PoolHelpers::ForceReloadStrong);
}

void MonolithExporter::threadFinished()
{
	if (error.isNotEmpty())
	{
		PresetHandler::showMessageWindow("Error at exporting", error, PresetHandler::IconType::Error);
	}
	else
	{
		PresetHandler::showMessageWindow("Exporting successful", "All samples were successfully written as monolithic file.", PresetHandler::IconType::Info);

		if (sampleMapFile.existsAsFile())
		{
			PoolReference ref(sampleMap->getSampler()->getMainController(), sampleMapFile.getFullPathName(), FileHandlerBase::SampleMaps);

			sampleMap->getSampler()->getMainController()->getCurrentSampleMapPool()->loadFromReference(ref, PoolHelpers::ForceReloadStrong);

			auto tmp = sampleMapFile;

			auto f = [ref](Processor* p)
			{
				static_cast<ModulatorSampler*>(p)->loadSampleMap(ref);

				return SafeFunctionCall::OK;
			};

			sampleMap->getSampler()->killAllVoicesAndCall(f);
		}

	}
}


juce::AudioFormatWriter* MonolithExporter::createWriter(hlac::HiseLosslessAudioFormat& hlac, const File& outputFile, bool isMono)
{
	bool ok = outputFile.deleteFile();

	jassert(ok); ignoreUnused(ok);

	outputFile.create();

	FileOutputStream* hlacOutput = new FileOutputStream(outputFile);

	auto options = hlac::HlacEncoder::CompressorOptions::getPreset(hlac::HlacEncoder::CompressorOptions::Presets::Diff);

	options.applyDithering = false;
	options.normalisationMode = (uint8)getComboBoxComponent("normalise")->getSelectedItemIndex();

	StringPairArray empty;

	ScopedPointer<AudioFormatWriter> writer = hlac.createWriterFor(hlacOutput, sampleRate, isMono ? 1 : 2, 16, empty, 5);

	dynamic_cast<hlac::HiseLosslessAudioFormatWriter*>(writer.get())->setOptions(options);

	return writer.release();
}

void MonolithExporter::checkSanity()
{
	if (filesToWrite.size() != numChannels)
        throw String("Channel amount mismatch");

	for (int i = 0; i < filesToWrite.size(); i++)
	{
		if (filesToWrite[i]->size() != numSamples)
            throw("Sample amount mismatch for Channel " + String(i + 1));
	}

	ModulatorSampler::SoundIterator iter(sampleMap->getSampler());

	Array<double> legitSampleRates({ 44100.0, 48000.0, 88200.0, 96000.0 });

	while (auto sound = iter.getNextSound())
	{
		auto sr = sound->getSampleRate();

		if (!legitSampleRates.contains(sr))
			throw String("Sample " + sound->getReferenceToSound()->getFileName(true) + " has unsupported samplerate: " + String(sr));
	}
}

File getNextMonolith(const File& f)
{
	auto p = f.getParentDirectory();
	auto n = f.getFileName();

	int index = n.getTrailingIntValue();
	int nextIndex = index + 1;

	auto newFileName = n.replace(".ch" + String(index), ".ch" + String(nextIndex));
	return p.getChildFile(newFileName);
}

void MonolithExporter::writeFiles(int channelIndex, bool overwriteExistingData)
{
	AudioFormatManager afm;
	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

	Array<File>* channelList = filesToWrite[channelIndex];
	splitData.clear();

	bool isMono = false;

	if (channelList->size() > 0)
	{
		ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(channelList->getUnchecked(0));

		isMono = reader->numChannels == 1;
		sampleRate = reader->sampleRate;
	}

	String channelFileName = sampleMap->getId().toString().replace("/", "_") + ".ch" + String(channelIndex + 1);

	File outputFile = monolithDirectory.getChildFile(channelFileName);

	if (!outputFile.existsAsFile() || overwriteExistingData)
	{
		hlac::HiseLosslessAudioFormat hlac;
		ScopedPointer<AudioFormatWriter> writer = createWriter(hlac, outputFile, isMono);

		int currentSplitIndex = 0;

		for (int i = 0; i < channelList->size(); i++)
		{
			setProgress((double)i / (double)numSamples);

            if(threadShouldExit())
                return;
            
			ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(channelList->getUnchecked(i));

			if (reader != nullptr)
			{
				writer->writeFromAudioReader(*reader, 0, -1);
				
			}
			else
			{
				error = "Could not read the source file " + channelList->getUnchecked(i).getFullPathName();
				writer->flush();
				writer = nullptr;

				return;
			}

			if (dynamic_cast<hlac::HiseLosslessAudioFormatWriter*>(writer.get())->getNumBytesWritten() > maxMonolithSize)
			{
				// This only works with non-multimic samples
				jassert(numChannels == 1);

				writer->flush();
				writer = nullptr;

				splitData.add({ currentSplitIndex++, channelList->getUnchecked(i) });

				outputFile = getNextMonolith(outputFile);

				writer = createWriter(hlac, outputFile, isMono);
			}
		}

		writer->flush();
		writer = nullptr;

		return;
	}
}

void MonolithExporter::updateSampleMap()
{
	checkSanity();

	sampleMap->setIsMonolith();

	AudioFormatManager afm;

	afm.registerBasicFormats();

	largestSample = 0;
	int64 offset = 0;

	bool usesSingleChannelSplit = !splitData.isEmpty();
	int currentSplitIndex = 0;

	if (usesSingleChannelSplit)
	{
		v.setProperty("MonolithSplitAmount", splitData.size() + 1, nullptr);
	}

	for (int i = 0; i < numSamples; i++)
	{
		ValueTree s = v.getChild(i);

		if (numChannels > 0)
		{
			File sampleFile = filesToWrite.getUnchecked(0)->getUnchecked(i);

			

			ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(sampleFile);



			if (reader != nullptr)
			{
				const auto length = (int64)hlac::CompressionHelpers::getPaddedSampleSize((int)reader->lengthInSamples);
				auto sampleEnd = (int64)s.getProperty(SampleIds::SampleEnd, 0);

				if (reader->lengthInSamples < sampleEnd)
				{
					if (logData.logFunction)
						logData.logFunction("Truncated sample end for sample " + s.getProperty(SampleIds::FileName).toString());
					s.setProperty(SampleIds::SampleEnd, reader->lengthInSamples, nullptr);
				}
				
				largestSample = jmax<int64>(largestSample, length);

				s.setProperty("MonolithOffset", offset, nullptr);
				s.setProperty("MonolithLength", length, nullptr);
				s.setProperty("SampleRate", reader->sampleRate, nullptr);

				if (usesSingleChannelSplit)
					s.setProperty("MonolithSplitIndex", currentSplitIndex, nullptr);

				offset += length;
			}

			if (usesSingleChannelSplit && splitData[currentSplitIndex].lastSample == sampleFile)
			{
				currentSplitIndex++;
				offset = 0;
			}
		}
	}
}

BatchReencoder::BatchReencoder(ModulatorSampler* s) :
	MonolithExporter(String("Batch reencode all sample maps"), s->getMainController()->getMainSynthChain()),
	ControlledObject(s->getMainController()),
	sampler(s)
{
	StringArray sa;

	sa.add("Yes");
	sa.add("No");

	addComboBox("checkSamplemaps", sa, "Validate Samplemap IDs");

	StringArray sa2;


	

	sa2.add("No normalisation");
	sa2.add("Normalise every sample");
	sa2.add("Full Dynamics");

	addComboBox("normalise", sa2, "Normalization");

#if USE_FRONTEND && HI_SUPPORT_FULL_DYNAMICS_HLAC
	getComboBoxComponent("normalise")->setSelectedItemIndex(2, dontSendNotification);
#endif

	if (GET_HISE_SETTING(s, HiseSettings::Project::SupportFullDynamicsHLAC))
		getComboBoxComponent("normalise")->setSelectedItemIndex(2, dontSendNotification);

	addProgressBarComponent(wholeProgress);

	addBasicComponents(true);
}

void BatchReencoder::run()
{
	if (getComboBoxComponent("checkSamplemaps")->getSelectedItemIndex() == 0)
	{
		auto result = sampler->getMainController()->getActiveFileHandler()->updateSampleMapIds(true);

		if (!result.wasOk())
		{
			setError(result.getErrorMessage());
			return;
		}
	}

#if USE_FRONTEND

	auto exp = sampler->getMainController()->getExpansionHandler().getCurrentExpansion();

	if (exp == nullptr)
	{
		setError("You need to select a expansion for the batch reencoding");
		return;
	}

	monolithDirectory = exp->getSubDirectory(FileHandlerBase::Samples);

	auto pool = &exp->pool->getSampleMapPool();

#else

	auto pool = sampler->getMainController()->getCurrentSampleMapPool();

	if (auto currentExp = sampler->getMainController()->getExpansionHandler().getCurrentExpansion())
	{
		pool = &currentExp->pool->getSampleMapPool();
	}

#endif

	setSampleMap(sampler->getSampleMap());

	auto list = pool->getListOfAllReferences(true);

	



	for (int i = 0; i < list.size(); i++)
	{
		reencode(list[i]);

		if (threadShouldExit())
			return;

		wholeProgress = (double)i / double(list.size());
	}
}

void BatchReencoder::reencode(PoolReference r)
{
	auto map = sampler->getSampleMap();
	bool done = false;
	bool* ptr = &done;

	auto f = [map, r, ptr](Processor* )
	{
		map->load(r);

		if (map->isMonolith())
		{
			auto tree = map->getValueTree().createCopy();

			tree.setProperty("SaveMode", 0, nullptr);

			for (auto s : tree)
			{
				s.removeProperty("MonolithOffset", nullptr);
				s.removeProperty("MonolithLength", nullptr);
			}

			map->loadUnsavedValueTree(tree);
		}

		*ptr = true;
		return SafeFunctionCall::OK;
	};

	showStatusMessage("Resaving samplemap");
	
	sampler->killAllVoicesAndCall(f);

	while (!done)
	{
		Thread::sleep(300);

		if (threadShouldExit())
			return;
	}

	sampleMapFile = r.getFile();

	exportCurrentSampleMap(true, reencodeSamples, true);
}



#endif

SampleMap::Notifier::Notifier(SampleMap& parent_) :
	asyncUpdateCollector(*this),
	parent(parent_)
{

}

void SampleMap::Notifier::sendMapChangeMessage(NotificationType n)
{
	{
		ScopedLock sl(pendingChanges.getLock());
		sampleAmountWasChanged = false;
		mapWasChanged = true;
	}


	if (n == sendNotificationAsync)
		triggerLightWeightUpdate();
	else
		handleLightweightPropertyChanges();
}

void SampleMap::Notifier::addPropertyChange(int index, const Identifier& id, const var& newValue)
{
	if (auto sound = parent.getSound(index))
	{
		if (!ModulatorSamplerSound::isAsyncProperty(id))
		{
			sound->updateInternalData(id, newValue);

			ScopedLock sl(pendingChanges.getLock());

			bool found = false;

			for (auto p : pendingChanges)
			{
				if (*p == index)
				{
					p->set(id, newValue);
					found = true;
					break;
				}
			}

			if (!found)
			{
				ScopedPointer<PropertyChange> newChange = new PropertyChange();
				newChange->index = index;
				newChange->set(id, newValue);

				pendingChanges.add(newChange.release());
			}

			triggerLightWeightUpdate();
		}
		else
		{
			{
				bool found = false;

				for (auto& s : asyncPendingChanges)
				{
					if (s == id)
					{
						s.addPropertyChange(sound, newValue);
						found = true;
						break;
					}
				}

				if (!found)
				{
					asyncPendingChanges.add(AsyncPropertyChange(sound, id, newValue));
				}

				triggerHeavyweightUpdate();
			}

			
		}

		
	}
}

void SampleMap::Notifier::sendSampleAmountChangeMessage(NotificationType n)
{
	{
		ScopedLock sl(pendingChanges.getLock());
		sampleAmountWasChanged = true;
	}


	if (n == sendNotificationAsync)
		triggerLightWeightUpdate();
	else
		handleLightweightPropertyChanges();
}

void SampleMap::Notifier::handleHeavyweightPropertyChangesIdle(const Array<AsyncPropertyChange, CriticalSection>& changesThisTime)
{
	jassert_sample_loading_thread(parent.getSampler()->getMainController());
	LockHelpers::freeToGo(parent.getSampler()->getMainController());

	for (auto& c : changesThisTime)
	{
		jassert(c.selection.size() == c.values.size());

		for (int i = 0; i < c.selection.size(); i++)
		{
			if (c.selection[i] != nullptr)
			{
				auto sound = static_cast<ModulatorSamplerSound*>(c.selection[i].get());

				if (sound->isDeletePending())
				{
					jassertfalse;
					continue;
				}

				sound->updateAsyncInternalData(c.id, c.values[i]);
			}
				
		}
	}
}

void SampleMap::Notifier::handleHeavyweightPropertyChanges()
{
	auto f = [this](Processor* /*p*/)
	{
		LockHelpers::SafeLock sl(parent.getSampler()->getMainController(), LockHelpers::SampleLock);

		Array<AsyncPropertyChange, CriticalSection> changesThisTime;

		changesThisTime.swapWith(asyncPendingChanges);

		handleHeavyweightPropertyChangesIdle(changesThisTime);

		return SafeFunctionCall::OK;
	};

	parent.getSampler()->killAllVoicesAndCall(f);
	
}

void SampleMap::Notifier::handleLightweightPropertyChanges()
{
	auto mc = parent.getSampler()->getMainController();

	jassert_dispatched_message_thread(mc);

	if (mapWasChanged)
	{
		ScopedLock sl(parent.listeners.getLock());

		auto r = parent.sampleMapData.getRef();

		for (auto l : parent.listeners)
		{
			mc->checkAndAbortMessageThreadOperation();

			if (l != nullptr)
				l->sampleMapWasChanged(r);
		}

		mapWasChanged = false;
		sampleAmountWasChanged = false;
	}
	else if (sampleAmountWasChanged)
	{
		ScopedLock sl(parent.listeners.getLock());

		for (auto l : parent.listeners)
		{
			mc->checkAndAbortMessageThreadOperation();

			if(l != nullptr)
				l->sampleAmountChanged();
		}

		sampleAmountWasChanged = false;
	}
	else
	{
		while (!pendingChanges.isEmpty())
		{
			ScopedPointer<PropertyChange> c = pendingChanges.removeAndReturn(0);

			if (auto sound = parent.getSound(c->index))
			{
				ScopedLock sl(parent.listeners.getLock());

				for (auto l : parent.listeners)
				{
					mc->checkAndAbortMessageThreadOperation();

					if (l)
					{
						for (const auto& prop : c->propertyChanges)
							l->samplePropertyWasChanged(sound, prop.name, prop.value);
					}
				}
			}
		}
	}
}

void SampleMap::Notifier::triggerHeavyweightUpdate()
{
	asyncUpdateCollector.triggerAsyncUpdate();
}

void SampleMap::Notifier::triggerLightWeightUpdate()
{
	if (lightWeightUpdatePending)
		return;

	auto f = [](Dispatchable* obj)
	{
		auto n = static_cast<Notifier*>(obj);
		
		n->handleLightweightPropertyChanges();
		n->lightWeightUpdatePending = false;
		return Status::OK;
	};

	lightWeightUpdatePending = true;
	
	parent.getSampler()->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(this, f);
}

SampleMap::Notifier::AsyncPropertyChange::AsyncPropertyChange(ModulatorSamplerSound* sound, const Identifier& id_, const var& newValue):
	id(id_)
{
	jassert(ModulatorSamplerSound::isAsyncProperty(id));

	addPropertyChange(sound, newValue);
}

void SampleMap::Notifier::AsyncPropertyChange::addPropertyChange(ModulatorSamplerSound* sound, const var& newValue)
{
	int index = selection.indexOf(sound);

	if (index == -1)
	{
		selection.add(sound);
		values.add(newValue);
	}
	else
	{
		values.set(index, newValue);
	}
}

void SampleMap::Notifier::PropertyChange::set(const Identifier& id, const var& newValue)
{
	jassert(!ModulatorSamplerSound::isAsyncProperty(id));

	propertyChanges.set(id, newValue);
}

void SampleMap::Notifier::Collector::handleAsyncUpdate()
{
	parent.handleHeavyweightPropertyChanges();
}

} // namespace hise
