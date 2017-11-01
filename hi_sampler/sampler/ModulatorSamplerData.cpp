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






ThumbnailHandler::ThumbnailHandler(const File &directoryToLoad, const StringArray &fileNames, ModulatorSampler *s) :
ThreadWithQuasiModalProgressWindow("Generating Audio Thumbnails for " + String(fileNames.size()) + " files.", true, true, s->getMainController()),
fileNamesToLoad(fileNames),
directory(directoryToLoad),
sampler(s),
addThumbNailsToExistingCache(true)
{
	getAlertWindow()->setLookAndFeel(&laf);
}

ThumbnailHandler::ThumbnailHandler(const File &directoryToLoad, ModulatorSampler *s) :
ThreadWithQuasiModalProgressWindow("Generating Audio Thumbnails for directory " + directoryToLoad.getFullPathName(), true, true, s->getMainController()),
directory(directoryToLoad),
sampler(s),
addThumbNailsToExistingCache(false)
{
	getAlertWindow()->setLookAndFeel(&laf);
}

File ThumbnailHandler::getThumbnailFile(ModulatorSampler *sampler)
{
	return GET_PROJECT_HANDLER(sampler).getWorkDirectory().getChildFile("thumbnails.dat");
}

void ThumbnailHandler::loadThumbnailsIntoSampler(ModulatorSampler *sampler)
{
    File thumbnailFile = getThumbnailFile(sampler);
    
	sampler->loadCacheFromFile(thumbnailFile);
}

void ThumbnailHandler::saveNewThumbNails(ModulatorSampler *sampler, const StringArray &newAudioFiles)
{
	File directory = GET_PROJECT_HANDLER(sampler).getWorkDirectory();
	
	new ThumbnailHandler(directory, newAudioFiles, sampler);
}

void ThumbnailHandler::run()
{
	AudioFormatManager &afm = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->afm;

	AudioThumbnailCache *cacheToUse = nullptr;

	if(addThumbNailsToExistingCache)
	{
		cacheToUse = &sampler->getCache();

		const int numSamplesToCache = fileNamesToLoad.size();

		jassert(numSamplesToCache > 0);

		for(int i = 0; i < numSamplesToCache; i++)
		{
			if(threadShouldExit()) return;
			
			setProgress((double)i / (double)numSamplesToCache);

			saveThumbnail(cacheToUse, afm, File(fileNamesToLoad[i]));
		}
	}
	else
	{
		const int numSamplesToCache = directory.getNumberOfChildFiles(File::TypesOfFileToFind::findFiles, "*.wav");
		writeCache = new AudioThumbnailCache(numSamplesToCache);

		cacheToUse = writeCache;

		DirectoryIterator iterator(directory, false, "*.wav", File::TypesOfFileToFind::findFiles);

		while(iterator.next())
		{
			if(threadShouldExit()) return;

			setProgress(iterator.getEstimatedProgress());

			saveThumbnail(writeCache, afm, iterator.getFile());

		}		
	}

	File outputFile = getThumbnailFile(sampler);
    
    FileOutputStream outputStream(outputFile);

	cacheToUse->writeToStream(outputStream);

	sampler->loadCacheFromFile(outputFile);
};


SampleMap::SampleMap(ModulatorSampler *sampler_):
	sampler(sampler_),
	fileOnDisk(File()),
	changed(false),
	mode(Undefined)
{
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
				StreamingSamplerSound* sample = sound->getReferenceToSound(i);

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
    mode = Undefined;
    fileOnDisk = File();
    sampleMapId = Identifier();
    changed = false;
    
    sampler->sendChangeMessage();
    sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();    
}

void SampleMap::restoreFromValueTree(const ValueTree &v)
{
	ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

	mode = (SaveMode)(int)v.getProperty("SaveMode");

	const String sampleMapName = v.getProperty("ID");
	sampleMapId = sampleMapName.isEmpty() ? Identifier::null : Identifier(sampleMapName);
    
	const int newRoundRobinAmount = v.getProperty("RRGroupAmount", 1);

	sampler->setRRGroupAmount(newRoundRobinAmount);

	if(mode == Monolith)
	{
		loadSamplesFromMonolith(v);
	}

	else
	{
		loadSamplesFromDirectory(v);
	}

	if(!sampler->isRoundRobinEnabled()) sampler->refreshRRMap();
	sampler->refreshPreloadSizes();
	sampler->refreshMemoryUsage();
	
};

void SampleMap::saveIfNeeded()
{
	const bool unsavedChanges = sampler->getNumSounds() != 0 && hasUnsavedChanges();

	if(unsavedChanges)
	{
		const bool result = PresetHandler::showYesNoWindow("Save sample map?", "Do you want to save the current sample map?");

		if(result) save();
	}
}

ValueTree SampleMap::exportAsValueTree() const
{
	// The file must be set before exporting the samplemap!
	//jassert(!saveRelativePaths && fileOnDisk != File());

	ValueTree v("samplemap");

    v.setProperty("ID", sampleMapId.toString(), nullptr);
	v.setProperty("SaveMode", mode, nullptr);
	v.setProperty("RRGroupAmount", sampler->getAttribute(ModulatorSampler::Parameters::RRGroupAmount), nullptr);
	v.setProperty("MicPositions", sampler->getStringForMicPositions(), nullptr);

	StringArray absoluteFileNames;

	ModulatorSampler::SoundIterator sIter(sampler);

	while (auto sound = sIter.getNextSound())
	{
		ValueTree soundTree = sound->exportAsValueTree();

		if (soundTree.getNumChildren() != 0)
		{
			for (int j = 0; j < soundTree.getNumChildren(); j++)
			{
				ValueTree child = soundTree.getChild(j);

				replaceFileReferences(child);
			}
		}
		else
		{
			replaceFileReferences(soundTree);
		}

		v.addChild(soundTree, -1, nullptr);
	}

	return v;
}

void SampleMap::replaceFileReferences(ValueTree &soundTree) const
{
	if (sampler->getMainController()->getSampleManager().shouldUseRelativePathToProjectFolder())
	{
		const String reference = GET_PROJECT_HANDLER(sampler).getFileReference(soundTree.getProperty("FileName", String()), ProjectHandler::SubDirectories::Samples);

		soundTree.setProperty("FileName", reference, nullptr);
	}
	else if (sampler->useGlobalFolderForSaving())
	{
		soundTree.setProperty("FileName", sampler->getGlobalReferenceForFile(soundTree.getProperty("FileName", String())), nullptr);
	}
}

void SampleMap::save()
{
	File rootDirectory = GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);

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

		name = name.replace(File::separatorString, "/");

		sampleMapId = Identifier(name);

		mode = SaveMode::MultipleFiles;

		f.deleteFile();

		ValueTree v = exportAsValueTree();
		ScopedPointer<XmlElement> xml = v.createXml();
		xml->writeToFile(f, "");

		changed = false;
	}
}


void SampleMap::saveAsMonolith(Component* mainEditor)
{
	mode = SaveMode::Monolith;

	auto m = new MonolithExporter(this);

	m->setModalBaseWindowComponent(mainEditor);

	changed = false;
}

void SampleMap::loadSamplesFromDirectory(const ValueTree &v)
{
	jassert(!v.hasProperty("Monolithic"));

	String fileName = v.getProperty("FileName", String());

	const ValueTree *treeToUse;

	ValueTree globalTree;
	ValueTree absoluteTree;

	if ((bool)v.getProperty("UseGlobalFolder", false) == true)
	{
		globalTree = v.createCopy();

		for (int i = 0; i < globalTree.getNumChildren(); i++)
		{
			const String sampleFileName = v.getChild(i).getProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), String());

			jassert(sampler->isReference(sampleFileName));

			globalTree.getChild(i).setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), sampler->getFile(sampleFileName, PresetPlayerHandler::StreamedSampleFolder).getFullPathName(), nullptr);
		}

		treeToUse = &globalTree;
	}
	else
	{
		if (fileName.isNotEmpty()) fileOnDisk = File(fileName);

		mode = (SaveMode)(int)v.getProperty("SaveMode", (int)Undefined);

		treeToUse = &v;
	}

	sampler->deleteAllSounds();

	int numChannels = jmax<int>(1, v.getChild(0).getNumChildren());
	
    StringArray micPositions = StringArray::fromTokens(v.getProperty("MicPositions").toString(), ";", "");
    
    micPositions.removeEmptyStrings(true);
    
    if (micPositions.size() != 0)
    {
        sampler->setNumMicPositions(micPositions);
    }
    else
    {
        sampler->setNumChannels(numChannels);
    }
    
    
    
    sampler->setShouldUpdateUI(false);
    ModulatorSamplerSoundPool *pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
    pool->setUpdatePool(false);
    
	{
		ScopedLock sl(sampler->getMainController()->getSampleManager().getSamplerSoundLock());

		for (int i = 0; i < treeToUse->getNumChildren(); i++)
		{
			try
			{
				sampler->addSamplerSound(treeToUse->getChild(i), i);
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

	if(fileOnDisk != File() && (treeToUse->getNumChildren() != 0))
	{
		String firstFileName = treeToUse->getChild(0).getProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), String());

		File sampleDirectory = File(GET_PROJECT_HANDLER(sampler).getFilePath(firstFileName, ProjectHandler::SubDirectories::Samples)).getParentDirectory();

		jassert(sampleDirectory.isDirectory());

		ThumbnailHandler::loadThumbnails(sampler, sampleDirectory);
	}
}

void SampleMap::loadSamplesFromMonolith(const ValueTree &v)
{
#if USE_BACKEND
	File monolithDirectory = GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::Samples);

#else    
	File monolithDirectory = dynamic_cast<FrontendDataHolder*>(sampler->getMainController())->getSampleLocation();
#endif

	if (!monolithDirectory.isDirectory())
	{
		sampler->getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage,
			"The sample directory is not existing");
#if USE_FRONTEND
		sampler->deleteAllSounds();
#endif
		return;
	}

	Array<File> monolithFiles;
	
    int numChannels = jmax<int>(1, v.getChild(0).getNumChildren());
    
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
#if USE_FRONTEND
            sampler->deleteAllSounds();
#endif
            return;
        }
	}

	if (!monolithFiles.isEmpty())
	{

		sampler->deleteAllSounds();

		StringArray micPositions = StringArray::fromTokens(v.getProperty("MicPositions").toString(), ";", "");

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

		OwnedArray<ModulatorSamplerSound> newSounds;

		pool->loadMonolithicData(v, monolithFiles, newSounds);

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			newSounds[i]->restoreFromValueTree(v.getChild(i));
		}
		
		sampler->addSamplerSounds(newSounds);

	}
    
}

void SampleMap::replaceReferencesWithGlobalFolder()
{
	
}

String SampleMap::checkReferences(ValueTree& v, const File& sampleRootFolder, Array<File>& sampleList)
{
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
		static const String wc("{PROJECT_FOLDER}");

		if (numChannels == 1)
		{
			for (int i = 0; i < v.getNumChildren(); i++)
			{
				ValueTree sample = v.getChild(i);

				const String fileReference = sample.getProperty("FileName");

				if (!fileReference.startsWith(wc))
				{
					PresetHandler::showMessageWindow("Absolute File path detected", "The sample " + fileReference + " is a absolute path which will not be resolved when using the library on another system", PresetHandler::IconType::Error);
					return fileReference;
				}

				const String strippedFileReference = fileReference.fromFirstOccurrenceOf(wc, false, false);

				File sampleLocation = sampleRootFolder.getChildFile(strippedFileReference);

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
					const String fileReference = sample.getChild(j).getProperty("FileName");

					if (!fileReference.startsWith(wc))
					{
						PresetHandler::showMessageWindow("Absolute File path detected", "The sample " + fileReference + " is a absolute path which will not be resolved when using the library on another system", PresetHandler::IconType::Error);
						return fileReference;
					}

					const String strippedFileReference = fileReference.fromFirstOccurrenceOf(wc, false, false);

					File sampleLocation = sampleRootFolder.getChildFile(strippedFileReference);

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

void SampleMap::load(const File &f)
{
#if USE_BACKEND
	if (f.hasFileExtension(".m5p"))
	{
		StringArray layerNames = MachFiveImporter::getLayerNames(f);

		if (layerNames.size() != 0)
		{
			ValueTree sampleMap = MachFiveImporter::getSampleMapValueTree(f, layerNames[0]);

			if (sampleMap.isValid())
			{
				restoreFromValueTree(sampleMap);
				changed = false;
				return;
			}
		}
	}
#endif

	ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

	File fileToUse = f;

	if(xml == nullptr)
	{
		if(NativeMessageBox::showOkCancelBox(AlertWindow::WarningIcon, "Missing Samplemap", "The samplemap " + f.getFullPathName() + " wasn't found. Click OK to search or Cancel to skip loading"))
		{
			FileChooser fc("Resolve SampleMap reference", f, "*.xml");

			if(fc.browseForFileToOpen())
			{
				fileToUse = fc.getResult();
				xml = XmlDocument::parse(fileToUse);
			}
			else
			{
				debugError(sampler, "Error loading " + f.getFullPathName());
				return;
			}
		}
		else
		{
			debugError(sampler, "Error loading " + f.getFullPathName());
			return;
		}


	}

	jassert(xml != nullptr);

	static const Identifier sm("samplemap");

	ValueTree v = ValueTree::fromXml(*xml);

#if USE_BACKEND
	if (v.getType() != sm)
	{
		PresetHandler::showMessageWindow("Invalid Samplemap", "The XML you tried to load is not a valid samplemap. Detected Type: " + v.getType().toString(), PresetHandler::IconType::Error);
		return;
	}
#endif
        
    v.setProperty("FileName", fileToUse.getFullPathName(), nullptr);

	restoreFromValueTree(v);

	changed = false;
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

MonolithExporter::MonolithExporter(SampleMap* sampleMap_) :
	DialogWindowWithBackgroundThread("Exporting samples as monolith"),
	AudioFormatWriter(nullptr, "", 0.0, 0, 1),
	sampleMapDirectory(GET_PROJECT_HANDLER(sampleMap_->getSampler()).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps)),
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

void MonolithExporter::run()
{
	sampleMapFile = fc->getCurrentFile();

	if (sampleMapFile == sampleMapDirectory)
	{
		error = "No Sample Map file specified";
		return;
	}

	auto name = sampleMapFile.getRelativePathFrom(sampleMapDirectory).upToFirstOccurrenceOf(".xml", false, true);

	name = name.replace(File::separatorString, "/");

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
	
	

	v = sampleMap->exportAsValueTree();
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
