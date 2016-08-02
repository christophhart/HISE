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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


SoundPreloadThread::SoundPreloadThread(ModulatorSampler *s) :
ThreadWithQuasiModalProgressWindow("Loading Sample Data", true, true, s->getMainController(), 10000, "Abort loading"),
sampler(s)
{
	getAlertWindow()->setLookAndFeel(&laf);
}


SoundPreloadThread::SoundPreloadThread(ModulatorSampler *s, Array<ModulatorSamplerSound*> soundsToPreload_) :
ThreadWithQuasiModalProgressWindow("Preloading Sample Data", true, true, s->getMainController()),
sampler(s),
soundsToPreload(soundsToPreload_)
{
	getAlertWindow()->setLookAndFeel(&laf);
}

void SoundPreloadThread::run()
{
    if(sampler == nullptr)
    {
        jassertfalse;
        
        return;
    }
    
	ModulatorSamplerSoundPool *pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();

	ScopedValueSetter<bool> preloadLock(pool->getPreloadLockFlag(), true);

    ScopedLock(sampler->getLock());
    
	const int numSoundsToPreload = sampler->getNumSounds();

	const int preloadSize = (int)sampler->getAttribute(ModulatorSampler::PreloadSize);

	sampler->resetNotes();

	sampler->setBypassed(true);

	sampler->setShouldUpdateUI(false);
	pool->setUpdatePool(false);

	debugToConsole(sampler, "Changing preload size to " + String(preloadSize) + " samples");

	for(int i = 0; i < numSoundsToPreload; ++i)
	{
        if(threadShouldExit()) break;

		setProgress(i / (double)numSoundsToPreload);

		sampler->getSound(i)->checkFileReference();

		if (sampler->getNumMicPositions() == 1)
		{
			StreamingSamplerSound *s = sampler->getSound(i)->getReferenceToSound();
			preloadSample(s, preloadSize);

		}
		else
		{
			for (int j = 0; j < sampler->getNumMicPositions(); j++)
			{
                ModulatorSamplerSound *sound = sampler->getSound(i);
                
                if(sound != nullptr)
                {
                    StreamingSamplerSound *s = sound->getReferenceToSound(j);
                    
                    if(s != nullptr) preloadSample(s, preloadSize);
                }
			}
		}

	}

	sampler->setBypassed(false);
	sampler->setShouldUpdateUI(true);
	sampler->sendChangeMessage();
	sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->setUpdatePool(true);
	sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
};

void SoundPreloadThread::preloadSample(StreamingSamplerSound * s, const int preloadSize)
{
	jassert(s != nullptr);

	String fileName = s->getFileName(false);

	setStatusMessage(fileName);

	try
	{
		s->setPreloadSize(s->hasActiveState() ? preloadSize : 0, true);
		s->closeFileHandle();
	}
	catch (StreamingSamplerSound::LoadingError l)
	{
		debugError(sampler, "Error at preloading " + l.fileName + ": " + l.errorDescription);

		//sampler->deleteAllSounds();

		//sampler->clearSounds();
		sampler->setBypassed(false);
        
		signalThreadShouldExit();


	}
}

ThumbnailHandler::ThumbnailHandler(const File &directoryToLoad, const StringArray &fileNames, ModulatorSampler *s) :
ThreadWithQuasiModalProgressWindow("Generating Audio Thumbnails for " + String(fileNames.size()) + " files.", true, true, s->getMainController()),
fileNamesToLoad(fileNames),
directory(directoryToLoad),
sampler(s),
writeCache(nullptr),
addThumbNailsToExistingCache(true)
{
	getAlertWindow()->setLookAndFeel(&laf);
}

ThumbnailHandler::ThumbnailHandler(const File &directoryToLoad, ModulatorSampler *s) :
ThreadWithQuasiModalProgressWindow("Generating Audio Thumbnails for directory " + directoryToLoad.getFullPathName(), true, true, s->getMainController()),
directory(directoryToLoad),
sampler(s),
writeCache(nullptr),
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


SampleWriter::SampleWriter(ModulatorSampler *sampler_, const StringArray &fileNames_):
	ThreadWithProgressWindow("Saving samples", true, true),
	sampler(sampler_),
	fileNames(fileNames_)
{


};

void SampleWriter::run()
{
	if(sampler->getNumSounds() == fileNames.size())
	{

		if(fileNames.size() != 0 && !File(fileNames[0]).getParentDirectory().exists())
		{
			File(fileNames[0]).getParentDirectory().createDirectory();
		}

		AudioFormatManager afm;
		afm.registerBasicFormats();

		for(int i = 0; i < fileNames.size(); i++)
		{
			setProgress((double)i / (double)fileNames.size());

			if(threadShouldExit()) return;

			const String originalFileName = sampler->getPropertyForSound(i, ModulatorSamplerSound::FileName);

			if(fileNames[i] != originalFileName)
			{
				File originalFile(originalFileName);

				originalFile.copyFileTo(File(fileNames[i]));
			}
		}
	}
}

SampleMap::SampleMap(ModulatorSampler *sampler_):
	sampler(sampler_),
	fileOnDisk(File::nonexistent),
	changed(false),
	mode(Undefined)
{
	// You have to clear the sound array before you set a new SampleMap!
	jassert(sampler->getNumSounds() == 0);
}

SampleMap::FileList SampleMap::createFileList()
{
	FileList list;

	for (int i = 0; i < sampler->getNumMicPositions(); i++)
	{
		list.add(new Array<File>());
	}

	for (int i = 0; i < sampler->getNumSounds(); i++)
	{
		ModulatorSamplerSound* sound = sampler->getSound(i);

		for (int j = 0; j < sound->getNumMultiMicSamples(); j++)
		{
			StreamingSamplerSound* sample = sound->getReferenceToSound(j);

			File file = sample->getFileName(true);
			jassert(file.existsAsFile());

			list[j]->add(file);
		}
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
    fileOnDisk = File::nonexistent;
    sampleMapId = Identifier("unused");
    changed = false;
    
    sampler->sendChangeMessage();
    sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();    
}

void SampleMap::restoreFromValueTree(const ValueTree &v)
{
	mode = (SaveMode)(int)v.getProperty("SaveMode");

    sampleMapId = Identifier(v.getProperty("ID", "unused").toString());
    
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
	//jassert(!saveRelativePaths && fileOnDisk != File::nonexistent);

	ValueTree v("samplemap");

    v.setProperty("ID", sampleMapId.toString(), nullptr);
	v.setProperty("SaveMode", mode, nullptr);

	StringArray absoluteFileNames;

	for(int i = 0; i < sampler->getNumSounds(); i++)
	{
		ValueTree soundTree = sampler->getSound(i)->exportAsValueTree();

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

		v.addChild(soundTree, i, nullptr);
	}

	return v;
}

void SampleMap::replaceFileReferences(ValueTree &soundTree) const
{
	if (sampler->getMainController()->getSampleManager().shouldUseRelativePathToProjectFolder())
	{
		const String reference = GET_PROJECT_HANDLER(sampler).getFileReference(soundTree.getProperty("FileName", String::empty), ProjectHandler::SubDirectories::Samples);

		soundTree.setProperty("FileName", reference, nullptr);
	}
	else if (sampler->useGlobalFolderForSaving())
	{
		soundTree.setProperty("FileName", sampler->getGlobalReferenceForFile(soundTree.getProperty("FileName", String::empty)), nullptr);
	}
}

void SampleMap::save()
{
	const String name = PresetHandler::getCustomName("Sample Map");

    sampleMapId = Identifier(name);
    
	File sampleMapDirectory = GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps);

	File sampleMapFile = sampleMapDirectory.getChildFile(name + ".xml");

	mode = SaveMode::MultipleFiles;

	ValueTree v = exportAsValueTree();
	ScopedPointer<XmlElement> xml = v.createXml();
	xml->writeToFile(sampleMapFile, "");

	changed = false;
}

class MonolithExporter : public ThreadWithAsyncProgressWindow,
						 public AudioFormatWriter
{
public:

	MonolithExporter(SampleMap* sampleMap_) :
		ThreadWithAsyncProgressWindow("Exporting samples as monolith"),
		AudioFormatWriter(nullptr, "", 0.0, 0, 1),
		sampleMap(sampleMap_),
		sampleMapDirectory(GET_PROJECT_HANDLER(sampleMap->getSampler()).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps)),
		monolithDirectory(sampleMapDirectory.getChildFile("MonolithSamplePacks"))
	{
		if (!monolithDirectory.isDirectory()) monolithDirectory.createDirectory();

		addTextEditor("id", sampleMap->getId().toString(), "Sample Map Name");

		addBasicComponents(true);
	}

	void run() override
	{
		sampleMap->setId(getTextEditorContents("id"));
	
		showStatusMessage("Collecting files");

		filesToWrite = sampleMap->createFileList();

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

		writeSampleMapFile();

		for (int i = 0; i < numChannels; i++)
		{
			if (threadShouldExit())
			{
				error = "Export aborted by user";
				return;
			}
			writeFiles(i);
		}
		
	}


	void writeSampleMapFile()
	{
		File sampleMapFile = sampleMapDirectory.getChildFile(sampleMap->getId().toString() + ".xml");
		ScopedPointer<XmlElement> xml = v.createXml();
		xml->writeToFile(sampleMapFile, "");
	}

	void threadFinished() override
	{
		if (error.isEmpty())
		{
			PresetHandler::showMessageWindow("Error at exporting", error, PresetHandler::IconType::Error);
		}
		else
		{
			PresetHandler::showMessageWindow("Exporting successful", "All samples were successfully written as monolithic file.", PresetHandler::IconType::Info);
		}
	};


	bool write(const int** data, int numSamples) override
	{
		jassertfalse;
		return false;
	}

private:

	bool checkSanity()
	{
		if (filesToWrite.size() != numChannels) throw String("Channel amount mismatch");

		for (int i = 0; i < filesToWrite.size(); i++)
		{
			if (filesToWrite[i]->size() != numSamples) throw("Sample amount mismatch for Channel " + String(i + 1));
		}
	}


	/** Writes the files and updates the samplemap with the information. */
	void writeFiles(int channelIndex)
	{
		String channelFileName = sampleMap->getId().toString() + ".ch" + String(channelIndex+1);

		File outputFile = monolithDirectory.getChildFile(channelFileName);

		outputFile.deleteFile();
		outputFile.create();

		Array<File>* channelList = filesToWrite[channelIndex];

		showStatusMessage("Exporting Channel " + String(channelIndex+1));

		AudioSampleBuffer buffer(2, largestSample);
		MemoryBlock tempBlock;
		FileOutputStream fos(outputFile);
		AudioFormatManager afm;

		tempBlock.setSize(2 * 2 * largestSample);
		afm.registerBasicFormats();

		for (int i = 0; i < channelList->size(); i++)
		{
			setProgress((double)i / (double)numSamples);

			AudioFormatReader* reader = afm.createReaderFor(channelList->getUnchecked(i));
			reader->read(&buffer, 0, reader->lengthInSamples, 0, true, true);
			size_t bytesUsed = reader->lengthInSamples * 4;

			AudioFormatWriter::WriteHelper<AudioData::Int16, AudioData::Float32, AudioData::LittleEndian>::write(
				tempBlock.getData(), 2, (const int* const *)buffer.getArrayOfReadPointers(), reader->lengthInSamples);

			fos.write(tempBlock.getData(), bytesUsed);	
		}

		fos.flush();
	}

	void updateSampleMap()
	{
		checkSanity();

		AudioFormatManager afm;

		afm.registerBasicFormats();

		largestSample = 0;
		int64 offset = 0;

		for (int i = 0; i < numSamples; i++)
		{
			ValueTree s = v.getChild(i);

			if(numChannels > 0)
			{
				File sampleFile = filesToWrite.getUnchecked(0)->getUnchecked(i);
				
				AudioFormatReader* reader = afm.createReaderFor(sampleFile);

				if (reader != nullptr)
				{
					const int64 length = reader->lengthInSamples;

					largestSample = jmax<int64>(largestSample, length);

					s.setProperty("MonolithOffset", offset, nullptr);
					s.setProperty("MonolithLength", length, nullptr);
					s.setProperty("SampleRate", reader->sampleRate, nullptr);

					offset += length;
				}
			}
		}
	}

	int64 largestSample;

	ValueTree v;
	SampleMap* sampleMap;
	SampleMap::FileList filesToWrite;
	int numChannels;
	int numSamples;
	File sampleMapDirectory;
	const File monolithDirectory;

	String error;
};


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

	String fileName = v.getProperty("FileName", String::empty);

	const ValueTree *treeToUse;

	ValueTree globalTree;
	ValueTree absoluteTree;

	if ((bool)v.getProperty("UseGlobalFolder", false) == true)
	{
		globalTree = v.createCopy();

		for (int i = 0; i < globalTree.getNumChildren(); i++)
		{
			const String fileName = v.getChild(i).getProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), String::empty);

			jassert(sampler->isReference(fileName));

			globalTree.getChild(i).setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), sampler->getFile(fileName, PresetPlayerHandler::StreamedSampleFolder).getFullPathName(), nullptr);
		}

		treeToUse = &globalTree;
	}
	else
	{
		if (fileName.isNotEmpty()) fileOnDisk = PresetHandler::checkFile(fileName);

		mode = (SaveMode)(int)v.getProperty("SaveMode", (int)Undefined);

		treeToUse = &v;
	}

	sampler->deleteAllSounds();

	sampler->setShouldUpdateUI(false);
    ModulatorSamplerSoundPool *pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();
    pool->setUpdatePool(false);
    
	for(int i = 0; i < treeToUse->getNumChildren(); i++)
	{
		try
		{
			sampler->addSamplerSound(treeToUse->getChild(i), i);
		}
		catch(StreamingSamplerSound::LoadingError l)
		{
			debugError(sampler, "Error loading file " + l.fileName + ": " + l.errorDescription);
			debugError(sampler, "Loading cancelled due to Error.");
			return;
		}
		
	}

    pool->setUpdatePool(true);
    pool->sendChangeMessage();
	sampler->setShouldUpdateUI(true);
	sampler->sendChangeMessage();

	if(fileOnDisk != File::nonexistent && (treeToUse->getNumChildren() != 0))
	{
		String firstFileName = treeToUse->getChild(0).getProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), String::empty);

		File sampleDirectory = File(GET_PROJECT_HANDLER(sampler).getFilePath(firstFileName, ProjectHandler::SubDirectories::Samples)).getParentDirectory();

		jassert(sampleDirectory.isDirectory());

		ThumbnailHandler::loadThumbnails(sampler, sampleDirectory);
	}
}

void SampleMap::loadSamplesFromMonolith(const ValueTree &v)
{
	File monolithDirectory = GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).getChildFile("MonolithSamplePacks");

	File monolithFile = monolithDirectory.getChildFile(sampleMapId.toString() + ".ch1");

	if (monolithFile.existsAsFile())
	{
		sampler->deleteAllSounds();

		ModulatorSamplerSoundPool* pool = sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool();

		OwnedArray<ModulatorSamplerSound> newSounds;

		pool->loadMonolithicData(v, monolithFile, newSounds);

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

void SampleMap::load(const File &f)
{
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

	
	ValueTree v = ValueTree::fromXml(*xml);
        
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
