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
	notifier(*this)
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
	
	sampleMapData.clear();
	
	setNewValueTree(ValueTree("sampleMap"));

    mode = Undefined;
    
    sampleMapId = Identifier();
    changed = false;
    
	sampleMapData = PooledSampleMap();

	if (currentPool != nullptr)
		currentPool->removeListener(this);

	currentPool = nullptr;

    if(sampler != nullptr)
    {
        sampler->sendChangeMessage();
        sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
    }

	if (n != dontSendNotification)
		sendSampleMapChangeMessage(n);
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
	ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

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
		getSampler()->loadSampleMap(referenceThatWasChanged, true);
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
	jassert(parentTree == data);

	auto f = [childWhichHasBeenAdded](Processor* p)
	{
		if (auto s = dynamic_cast<ModulatorSampler*>(p))
		{
			auto map = s->getSampleMap();

			auto newSound = new ModulatorSamplerSound(map, childWhichHasBeenAdded, map->currentMonolith);
			s->addSound(newSound);
			dynamic_cast<ModulatorSamplerSound*>(newSound)->initPreloadBuffer(s->getAttribute(ModulatorSampler::PreloadSize));

			ModulatorSamplerSoundPool* pool = s->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
			pool->sendChangeMessage();
			s->sendChangeMessage();

			s->getSampleMap()->notifier.sendSampleAmountChangeMessage(sendNotificationAsync);

			return true;
		}

		return false;
	};

	getSampler()->killAllVoicesAndCall(f);	
}

void SampleMap::valueTreeChildRemoved(ValueTree& /*parentTree*/, ValueTree& /*childWhichHasBeenRemoved*/, int indexFromWhichChildWasRemoved)
{
	auto f = [indexFromWhichChildWasRemoved](Processor* p)
	{
		if (auto sampler = dynamic_cast<ModulatorSampler*>(p))
		{
			sampler->deleteSound(indexFromWhichChildWasRemoved);
			sampler->getSampleMap()->notifier.sendSampleAmountChangeMessage(sendNotificationAsync);
		}

		return true;
	};
	
	sampler->killAllVoicesAndCall(f);
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
	getSampler()->loadSampleMap(reloadedMap.getRef(), true);

	
	

#endif
}


void SampleMap::saveAsMonolith(Component* mainEditor)
{
#if HI_ENABLE_EXPANSION_EDITING
	mode = SaveMode::Monolith;

	auto m = new MonolithExporter(this);

	m->setModalBaseWindowComponent(mainEditor);

	changed = false;
#endif
}

void SampleMap::setNewValueTree(const ValueTree& v)
{
	

	data.removeListener(this);
	
	sampler->deleteAllSounds();
	notifier.sendSampleAmountChangeMessage(sendNotificationAsync);

	data = v;
	data.addListener(this);
}

void SampleMap::addSound(ValueTree& newSoundData)
{
	data.addChild(newSoundData, -1, sampler->getUndoManager());
}


void SampleMap::removeSound(ModulatorSamplerSound* s)
{
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
	clear(dontSendNotification);

	currentPool = getSampler()->getMainController()->getCurrentSampleMapPool();
	sampleMapData = currentPool->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);
	currentPool->addListener(this);

	if (sampleMapData)
	{
		
		parseValueTree(*sampleMapData.getData());
		changed = false;
	}
	else
		jassertfalse;

	sendSampleMapChangeMessage();
}

void SampleMap::loadUnsavedValueTree(const ValueTree& v)
{
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

	try
	{
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

			sampleMap->getSampler()->loadSampleMap(ref, true);
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

void SampleMap::Notifier::addPropertyChange(int index, const Identifier& id, const var& newValue)
{
	if (auto sound = parent.getSound(index))
	{
		if (!ModulatorSamplerSound::isAsyncProperty(id))
		{
			sound->updateInternalData(id, newValue);
		}
		else
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

			startTimer(100);
		}

		

		bool found = false;

		for (auto& p : pendingChanges)
		{
			if (p == index)
			{
				p.set(id, newValue);
				found = true;
				break;
			}
		}

		if (!found)
		{
			PropertyChange newChange;
			newChange.index = index;
			newChange.set(id, newValue);

			pendingChanges.add(newChange);
		}
			
		triggerAsyncUpdate();
	}
}

void SampleMap::Notifier::handleHeavyweightPropertyChanges()
{
	Array<AsyncPropertyChange> newChanges;

	newChanges.swapWith(asyncPendingChanges);

	auto f = [newChanges](Processor* p)
	{
		for (const auto& c : newChanges)
		{
			jassert(c.selection.size() == c.values.size());

			for (int i = 0; i < c.selection.size(); i++)
			{
				if (c.selection[i] != nullptr)
				{
					static_cast<ModulatorSamplerSound*>(c.selection[i].get())->updateAsyncInternalData(c.id, c.values[i]);
				}
			}
		}

		return true;
	};

	parent.sampler->killAllVoicesAndCall(f);
}

void SampleMap::Notifier::handleLightweightPropertyChanges()
{
	if (mapWasChanged)
	{
		auto r = parent.sampleMapData.getRef();

		for (auto l : parent.listeners)
		{
			if (auto l_ = l.get())
			{
				l_->sampleMapWasChanged(r);
			}
		}

		mapWasChanged = false;
		sampleAmountWasChanged = false;
	}
	else if (sampleAmountWasChanged)
	{
		for (auto l : parent.listeners)
		{
			l->sampleAmountChanged();
		}

		sampleAmountWasChanged = false;
	}
	else
	{
		Array<PropertyChange> thisChanges;

		thisChanges.swapWith(pendingChanges);

		for (auto c : thisChanges)
		{
			if (auto sound = parent.getSound(c.index))
			{
				for (auto l : parent.listeners)
				{
					if (l)
					{
						for (const auto& prop : c.propertyChanges)
							l->samplePropertyWasChanged(sound, prop.name, prop.value);
					}
				}
			}
		}
	}
}

void SampleMap::Notifier::handleAsyncUpdate()
{
	handleLightweightPropertyChanges();
}

void SampleMap::Notifier::timerCallback()
{
	handleHeavyweightPropertyChanges();

	

	stopTimer();
}

SampleMap::Notifier::AsyncPropertyChange::AsyncPropertyChange(ModulatorSamplerSound* sound, const Identifier& id_, const var& newValue):
	id(id_)
{
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

} // namespace hise
