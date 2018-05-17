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
	sampler(sampler_)
{
	clear();

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

void SampleMap::clear()
{
	sampleMapData.clear();
	data = ValueTree("sampleMap");

    mode = Undefined;
    fileOnDisk = File();
    sampleMapId = Identifier();
    changed = false;
    
	if (currentPool != nullptr)
		currentPool->removeListener(this);

	currentPool = nullptr;

    if(sampler != nullptr)
    {
        sampler->sendChangeMessage();
        sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
    }
}

void SampleMap::parseValueTree(const ValueTree &v)
{
	ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

	data = v;

	mode = (SaveMode)(int)v.getProperty("SaveMode");

	const String sampleMapName = v.getProperty("ID");
	sampleMapId = sampleMapName.isEmpty() ? Identifier::null : Identifier(sampleMapName);
    
	const int newRoundRobinAmount = v.getProperty("RRGroupAmount", 1);

	sampler->setRRGroupAmount(newRoundRobinAmount);

	if(mode == Monolith)
	{
		loadSamplesFromMonolith();
	}

	else
	{
		loadSamplesFromDirectory();
	}

	sampler->updateRRGroupAmountAfterMapLoad();
	if(!sampler->isRoundRobinEnabled()) sampler->refreshRRMap();
	sampler->refreshPreloadSizes();
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
		clear();
		getSampler()->loadSampleMap(referenceThatWasChanged, true);
	}
}

void SampleMap::save()
{
	data.setProperty("ID", sampleMapId.toString(), nullptr);
	data.setProperty("SaveMode", mode, nullptr);
	data.setProperty("RRGroupAmount", sampler->getAttribute(ModulatorSampler::Parameters::RRGroupAmount), nullptr);
	data.setProperty("MicPositions", sampler->getStringForMicPositions(), nullptr);

#if HI_ENABLE_EXPANSION_EDITING
	auto rootDirectory = sampler->getSampleEditHandler()->getCurrentSampleMapDirectory();

	

	File fileToUse;

	if (sampleMapId.isValid())
	{
		auto path = sampleMapId.toString() + ".xml";

		fileToUse = rootDirectory.getChildFile(path);
	}
	else
	{
		fileToUse = rootDirectory;
	}

	FileChooser fc("Save SampleMap", fileToUse, "*.xml", true);

	if (fc.browseForFileToSave(true))
	{
		File f = fc.getResult();

		auto name = f.getRelativePathFrom(rootDirectory).upToFirstOccurrenceOf(".xml", false, true);

		name = name.replace(File::getSeparatorString(), "/");

		sampleMapId = Identifier(name);

		if(mode != SaveMode::Monolith)
			mode = SaveMode::MultipleFiles;

		f.deleteFile();

		ScopedPointer<XmlElement> xml = data.createXml();
		xml->writeToFile(f, "");

		changed = false;
	}
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

void SampleMap::loadSamplesFromDirectory()
{
	jassert(!data.hasProperty("Monolithic"));

	String fileName = data.getProperty("FileName", String());

	const ValueTree *treeToUse;

	ValueTree globalTree;
	ValueTree absoluteTree;

	if (fileName.isNotEmpty()) fileOnDisk = File(fileName);

	mode = (SaveMode)(int)data.getProperty("SaveMode", (int)Undefined);

	treeToUse = &data;

	sampler->deleteAllSounds();

	int numChannels = jmax<int>(1, data.getChild(0).getNumChildren());
	
    StringArray micPositions = StringArray::fromTokens(data.getProperty("MicPositions").toString(), ";", "");
    
    micPositions.removeEmptyStrings(true);
    
    if(!sampler->isUsingStaticMatrix())
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
    
    sampler->setShouldUpdateUI(false);
    ModulatorSamplerSoundPool *pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
    pool->setUpdatePool(false);
    
	{
		ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

		for (const auto& sampleData: data)
		{
			try
			{
				sampler->addSound(new ModulatorSamplerSound(sampler->getMainController(), sampleData, nullptr));
			}
			catch (StreamingSamplerSound::LoadingError l)
			{
				String x;
				x << "Error at preloading sample " << l.fileName << ": " << l.errorDescription;
				sampler->getMainController()->getDebugLogger().logMessage(x);

#if USE_FRONTEND
				sampler->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage, x);
#else
				debugError(sampler, x);
#endif

				return;
			}

		}
	}

	

    pool->setUpdatePool(true);
    pool->sendChangeMessage();
	sampler->setShouldUpdateUI(true);
	sampler->sendChangeMessage();

}

void SampleMap::loadSamplesFromMonolith()
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

		File f = monolithDirectory.getChildFile(path + ".ch" + String(i+1));
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

		sampler->deleteAllSounds();

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

		ModulatorSamplerSoundPool* pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();

		auto hmaf = pool->loadMonolithicData(data, monolithFiles);

		for (auto sample : data)
		{


			sampler->addSound(new ModulatorSamplerSound(sampler->getMainController(), sample, hmaf));
		}

		pool->sendChangeMessage();
	}
    
}

void SampleMap::addSound(ModulatorSamplerSound* newSound)
{
	sampler->addSound(newSound);

	data.addChild(newSound->getData(), -1, nullptr);

	ModulatorSamplerSoundPool* pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
	pool->sendChangeMessage();
	sampler->sendChangeMessage();
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
	clear();

	currentPool = getSampler()->getMainController()->getCurrentSampleMapPool();

	sampleMapData = currentPool->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);
	currentPool->addListener(this);


	if (sampleMapData)
	{
		data = *sampleMapData.getData();
		parseValueTree(data);
		changed = false;
	}
	else
		jassertfalse;
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

} // namespace hise
