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
	mode(Undefined),
	changed(false),
	currentPool(nullptr),
	sampleMapId(Identifier()),
	data("samplemap")
{
	clear(dontSendNotification);

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
		ModulatorSampler::SoundIterator soundIter(sampler);

		while (auto sound = soundIter.getNextSound())
		{
			for (int i = 0; i < sound->getNumMultiMicSamples(); i++)
			{
				auto sample = sound->getReferenceToSound(i);

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

	


	if (!allFound)
	{
		
		SystemClipboard::copyTextToClipboard(missingFileList);

		missingFileList << "This message was also copied to the clipboard";

		throw missingFileList;
	}

	return list;
}

void SampleMap::changeListenerCallback(SafeChangeBroadcaster *)
{
	if(changed==false) sampler->sendChangeMessage();
	changed = true;
};

void SampleMap::clear(NotificationType n)
{
	if (mode != Undefined)
	{
		LockHelpers::freeToGo(sampler->getMainController());

		sampleMapData.clear();

		setNewValueTree(ValueTree("samplemap"));

		mode = Undefined;

		sampleMapId = Identifier();
		changed = false;

		sampleMapData = PooledSampleMap();

		if (currentPool != nullptr)
			currentPool->removeListener(this);

		currentPool = nullptr;

		if (sampler != nullptr)
		{
			sampler->sendChangeMessage();
			sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
		}

		if (n != dontSendNotification)
			sendSampleMapChangeMessage(n);
	}
}

void SampleMap::setCurrentMonolith()
{
	if (isMonolith())
	{
		ModulatorSamplerSoundPool* pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();

		if (auto existingInfo = pool->getMonolith(sampleMapId))
		{
			currentMonolith = existingInfo;

			jassert(*currentMonolith == sampleMapId);
		}
		else
		{
			File monolithDirectory = GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::Samples);

			if (!monolithDirectory.isDirectory())
			{
				sampler->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage,
					"The sample directory does not exist");

				FRONTEND_ONLY(sampler->deleteAllSounds());

				return;
			}

			Array<File> monolithFiles;

			int numChannels = jmax<int>(1, data.getChild(0).getNumChildren());

			for (int i = 0; i < numChannels; i++)
			{
				auto path = sampleMapId.toString().replace("/", "_");

				File f = monolithDirectory.getChildFile(path + ".ch" + String(i + 1));
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
				StringArray micPositions = StringArray::fromTokens(data.getProperty("MicPositions").toString(), ";", "");

				micPositions.removeEmptyStrings(true);

				if (micPositions.size() == numChannels)
				{
					sampler->setNumMicPositions(micPositions);
				}
				else
				{
					sampler->setNumChannels(1);
				}

				currentMonolith = pool->loadMonolithicData(data, monolithFiles);

				if(currentMonolith)
					jassert(*currentMonolith == sampleMapId);
			}
		}
	}
}

void SampleMap::parseValueTree(const ValueTree &v)
{
	LockHelpers::freeToGo(sampler->getMainController());

	setNewValueTree(v);
	
	mode = (SaveMode)(int)v.getProperty("SaveMode");

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


	for (auto c : data)
	{
		valueTreeChildAdded(data, c);
	}

	sampler->updateRRGroupAmountAfterMapLoad();
	if(!sampler->isRoundRobinEnabled()) sampler->refreshRRMap();
	
	sampler->refreshMemoryUsage();
	
};

void SampleMap::saveIfNeeded()
{
	const bool unsavedChanges = sampler != nullptr && sampler->getNumSounds() != 0 && hasUnsavedChanges();

	if(unsavedChanges)
	{
		const bool result = PresetHandler::showYesNoWindow("Save sample map?", "Do you want to save the current sample map?");

		if(result) save();
	}
}

const ValueTree SampleMap::getValueTree() const
{
	return data;
}

void SampleMap::poolEntryReloaded(PoolReference referenceThatWasChanged)
{
	if (getReference() == referenceThatWasChanged)
	{
		clear(dontSendNotification);
		getSampler()->loadSampleMap(referenceThatWasChanged);
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

void SampleMap::valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property)
{
	auto i = data.indexOf(treeWhosePropertyHasChanged);

	if (i != -1)
	{
		notifier.addPropertyChange(i, property, treeWhosePropertyHasChanged.getProperty(property));
	}
}

void SampleMap::valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded)
{
	ignoreUnused(parentTree);
	jassert(parentTree == data);

	LockHelpers::freeToGo(sampler->getMainController());

	auto map = sampler->getSampleMap();

	auto newSound = new ModulatorSamplerSound(map, childWhichHasBeenAdded, map->currentMonolith);

	{
		LockHelpers::SafeLock sl(sampler->getMainController(), LockHelpers::SampleLock);
		sampler->addSound(newSound);
	}

	dynamic_cast<ModulatorSamplerSound*>(newSound)->initPreloadBuffer((int)sampler->getAttribute(ModulatorSampler::PreloadSize));

	const bool isReversed = sampler->getAttribute(ModulatorSampler::Reversed) > 0.5f;

	newSound->setReversed(isReversed);

	auto update = [](Dispatchable* obj)
	{
		auto sampler = static_cast<ModulatorSampler*>(obj);

		jassert_dispatched_message_thread(sampler->getMainController());

		ModulatorSamplerSoundPool* pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
		pool->sendChangeMessage();
		sampler->sendChangeMessage();

		sampler->getSampleMap()->notifier.sendSampleAmountChangeMessage(sendNotificationAsync);

		return Dispatchable::Status::OK;
	};

	sampler->getMainController()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(sampler, update);
}

void SampleMap::valueTreeChildRemoved(ValueTree& /*parentTree*/, ValueTree& /*childWhichHasBeenRemoved*/, int indexFromWhichChildWasRemoved)
{
	LockHelpers::freeToGo(sampler->getMainController());

	sampler->deleteSound(indexFromWhichChildWasRemoved);
	sampler->getSampleMap()->notifier.sendSampleAmountChangeMessage(sendNotificationAsync);
}

void SampleMap::save()
{
#if HI_ENABLE_EXPANSION_EDITING
	auto rootDirectory = sampler->getSampleEditHandler()->getCurrentSampleMapDirectory();

	data.setProperty("ID", sampleMapId.toString(), nullptr);
	data.setProperty("SaveMode", mode, nullptr);
	data.setProperty("RRGroupAmount", sampler->getAttribute(ModulatorSampler::Parameters::RRGroupAmount), nullptr);
	data.setProperty("MicPositions", sampler->getStringForMicPositions(), nullptr);
	
	File f;

	if (isUsingUnsavedValueTree())
	{
		FileChooser fc("Save SampleMap As", rootDirectory, "*.xml", true);

		if (fc.browseForFileToSave(true))
		{
			f = fc.getResult();

			if (!f.isAChildOf(rootDirectory))
			{
				PresetHandler::showMessageWindow("Invalid Path", "You need to store the samplemap in the samplemap directory", PresetHandler::IconType::Error);
				return;
			}
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
			{
				f = fc.getResult();
			}
			else
				return;
			
		}
	}

	ScopedPointer<XmlElement> xml = data.createXml();
	f.replaceWithText(xml->createDocument(""));

	PoolReference ref(getSampler()->getMainController(), f.getFullPathName(), FileHandlerBase::SubDirectories::SampleMaps);

	auto pool = getSampler()->getMainController()->getCurrentSampleMapPool();
	auto reloadedMap = pool->loadFromReference(ref, PoolHelpers::LoadingType::ForceReloadStrong);
	getSampler()->loadSampleMap(reloadedMap.getRef());

	
	

#endif
}


void SampleMap::saveAsMonolith(Component* mainEditor)
{
#if HI_ENABLE_EXPANSION_EDITING
	mode = SaveMode::Monolith;

	auto m = new MonolithExporter(this);

	m->setModalBaseWindowComponent(mainEditor);

	changed = false;
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

	clear(dontSendNotification);

	currentPool = getSampler()->getMainController()->getCurrentSampleMapPool();

	

	sampleMapData = currentPool->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);
	currentPool->addListener(this);

	if (sampleMapData)
	{
		auto v = sampleMapData.getData()->createCopy();

		parseValueTree(v);
		changed = false;
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
	changed = false;

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
	DialogWindowWithBackgroundThread("Exporting samples as monolith"),
	AudioFormatWriter(nullptr, "", 0.0, 0, 1),
	sampleMapDirectory(sampleMap_->getSampler()->getSampleEditHandler()->getCurrentSampleMapDirectory()),
	monolithDirectory(GET_PROJECT_HANDLER(sampleMap_->getSampler()).getSubDirectory(ProjectHandler::SubDirectories::Samples))
{
	setSampleMap(sampleMap_);

	if (!monolithDirectory.isDirectory()) monolithDirectory.createDirectory();

	StringArray sa;

	sa.add("No compression");
	sa.add("Fast Decompression");
	sa.add("Low file size (recommended)");

	File fileToUse;

	auto sampleMapId = sampleMap_->getId();

	if (sampleMapId.isValid())
	{
		auto path = sampleMapId.toString() + ".xml";

		fileToUse = sampleMapDirectory.getChildFile(path);
	}
	else
	{
		fileToUse = sampleMapDirectory;
	}



	fc = new FilenameComponent("Sample Map File", fileToUse, false, false, true, "*.xml", "", "SampleMap File");

	fc->setSize(400, 24);

	addCustomComponent(fc);

	addComboBox("compressionOptions", sa, "HLAC Compression options");

	getComboBoxComponent("compressionOptions")->setSelectedItemIndex(2, dontSendNotification);

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
	jassert(sampleMap != nullptr);

	showStatusMessage("Collecting files");

	auto& lock = sampleMap->getSampler()->getMainController()->getSampleManager().getSamplerSoundLock();

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

	updateSampleMap();

	if(exportSampleMap)
		writeSampleMapFile(overwriteExistingData);

	if (exportSamples)
	{
		for (int i = 0; i < numChannels; i++)
		{
			if (threadShouldExit())
			{
				error = "Export aborted by user";
				return;
			}

			writeFiles(i, overwriteExistingData);
		}
	}
}

void MonolithExporter::writeSampleMapFile(bool /*overwriteExistingFile*/)
{
	ScopedPointer<XmlElement> xml = v.createXml();

	sampleMapFile.getParentDirectory().createDirectory();

	xml->writeToFile(sampleMapFile, "");
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

			auto tmp = sampleMapFile;

			sampleMap->getSampler()->loadSampleMap(ref);
		}

	}
}

void MonolithExporter::checkSanity()
{
	if (filesToWrite.size() != numChannels) throw String("Channel amount mismatch");

	for (int i = 0; i < filesToWrite.size(); i++)
	{
		if (filesToWrite[i]->size() != numSamples) throw("Sample amount mismatch for Channel " + String(i + 1));
	}
}

void MonolithExporter::writeFiles(int channelIndex, bool overwriteExistingData)
{
	AudioFormatManager afm;
	afm.registerBasicFormats();
	afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

	Array<File>* channelList = filesToWrite[channelIndex];

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

		outputFile.deleteFile();
		outputFile.create();
		
		hlac::HiseLosslessAudioFormat hlac;

		FileOutputStream* hlacOutput = new FileOutputStream(outputFile);

		int cIndex = getComboBoxComponent("compressionOptions")->getSelectedItemIndex();

		hlac::HlacEncoder::CompressorOptions options = hlac::HlacEncoder::CompressorOptions::getPreset((hlac::HlacEncoder::CompressorOptions::Presets)cIndex);

		StringPairArray empty;

		ScopedPointer<AudioFormatWriter> writer = hlac.createWriterFor(hlacOutput, sampleRate, isMono ? 1 : 2, 16, empty, 5);

		dynamic_cast<hlac::HiseLosslessAudioFormatWriter*>(writer.get())->setOptions(options);

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

	const bool usePaddingForCompression = getComboBoxComponent("compressionOptions")->getSelectedItemIndex() > 0;

	for (int i = 0; i < numSamples; i++)
	{
		ValueTree s = v.getChild(i);

		if (numChannels > 0)
		{
			File sampleFile = filesToWrite.getUnchecked(0)->getUnchecked(i);

			ScopedPointer<AudioFormatReader> reader = afm.createReaderFor(sampleFile);

			if (reader != nullptr)
			{
				const int64 length = usePaddingForCompression ? (int64)hlac::CompressionHelpers::getPaddedSampleSize((int)reader->lengthInSamples) : reader->lengthInSamples;

				largestSample = jmax<int64>(largestSample, length);

				s.setProperty("MonolithOffset", offset, nullptr);
				s.setProperty("MonolithLength", length, nullptr);
				s.setProperty("SampleRate", reader->sampleRate, nullptr);

				offset += length;
			}
		}
	}
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
				static_cast<ModulatorSamplerSound*>(c.selection[i].get())->updateAsyncInternalData(c.id, c.values[i]);
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
