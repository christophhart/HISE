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

	s->checkFileReference();

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

File ThumbnailHandler::getThumbnailFile(ModulatorSampler *sampler)
{
	return GET_PROJECT_HANDLER(sampler).getWorkDirectory().getChildFile("/thumbnails.dat");
}

void ThumbnailHandler::loadThumbnailsIntoSampler(ModulatorSampler *sampler)
{
    File thumbnailFile = getThumbnailFile(sampler);
    
	sampler->loadCacheFromFile(thumbnailFile);
}

void ThumbnailHandler::saveNewThumbNails(ModulatorSampler *sampler, const StringArray &newAudioFiles)
{
	File directory = GET_PROJECT_HANDLER(sampler).getWorkDirectory();
	
	ThumbnailHandler th(directory, newAudioFiles, sampler);
	th.runThread();
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
	saveRelativePaths(true),
	mode(Undefined)
{
	// You have to clear the sound array before you set a new SampleMap!
	jassert(sampler->getNumSounds() == 0);
}

void SampleMap::changeListenerCallback(SafeChangeBroadcaster *)
{
	if(changed==false) sampler->sendChangeMessage();
	changed = true;
		
};

void SampleMap::restoreFromValueTree(const ValueTree &v)
{

	monolith = v.hasProperty("Monolithic");

	if (v.getProperty("UseGlobalFolder", false))
	{
		sampler->setUseGlobalFolderForSaving();
	}
	

	if(monolith)
	{

		loadSamplesFromMonolith(v);

	}

	else
	{

		loadSamplesFromDirectory(v);

		
	}

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

	v.setProperty("FileName", fileOnDisk.getFullPathName(), nullptr);
	v.setProperty("SaveMode", mode, nullptr);
	v.setProperty("RelativePath", saveRelativePaths, nullptr);
	v.setProperty("UseGlobalFolder", sampler->useGlobalFolderForSaving(), nullptr);

	StringArray absoluteFileNames;

	bool warnIfUseAbsolutePathsInProjects = true;

	for(int i = 0; i < sampler->getNumSounds(); i++)
	{
		ValueTree soundTree = sampler->getSound(i)->exportAsValueTree();

		if(saveRelativePaths)
		{
			File soundFile = File(soundTree.getProperty("FileName", String::empty));

			if (warnIfUseAbsolutePathsInProjects)
			{
				warnIfUseAbsolutePathsInProjects = !soundFile.isAChildOf(GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::Samples));
			}

#if USE_BACKEND
            
			if (warnIfUseAbsolutePathsInProjects)
			{
				if (PresetHandler::showYesNoWindow("Absolute file paths when using project", "You are using external samples without redirecting the sample folder (or outside the redirecting location). The preset can't be loaded on another machine. Press OK to choose a root sample folder or Cancel to ignore this.", PresetHandler::IconType::Warning))
				{
					sampler->getMainController()->getCommandManager()->invokeDirectly(BackendCommandTarget::MenuToolsRedirectSampleFolder, false);
				}
			}
            
#endif

			absoluteFileNames.add(getSampleDirectory() + soundFile.getFileName());
			
			soundTree.setProperty("FileName", soundFile.getFileName(), nullptr);
		}

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

	if(saveRelativePaths)
	{
		SampleWriter sw(sampler, absoluteFileNames);

		sw.runThread();
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

void SampleMap::save(SaveMode m)
{
	const String extension = m == Monolith ? "him" : "xml";

	if(m != Undefined) // Choose a new file
	{
		FileChooser fc("Save SampleMap", GET_PROJECT_HANDLER(sampler).getWorkDirectory(), extension, true);

		if(fc.browseForFileToSave(true))
		{
			fileOnDisk = fc.getResult();
		}
		else
		{
			return;
		}
	}

	if(extension == "xml")
	{
		mode = SaveMode::MultipleFiles;

		ValueTree v = exportAsValueTree();
		ScopedPointer<XmlElement> xml = v.createXml();
		xml->writeToFile(fileOnDisk, "");

		File thumbnailFile(getSampleDirectory() + "/thumbnails.dat");
        
        FileOutputStream outputStream(thumbnailFile);

		sampler->getCache().writeToStream(outputStream);

		PresetHandler::saveProcessorAsPreset(sampler);
	}

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
		saveRelativePaths = v.getProperty("RelativePath", false);

		if (saveRelativePaths)
		{
			// Copy the tree, since you need to change the references.
			absoluteTree = v.createCopy();

			const String directory = fileOnDisk.getParentDirectory().getFullPathName() + "/samples/";

			for (int i = 0; i < absoluteTree.getNumChildren(); i++)
			{
				const String fileName = absoluteTree.getChild(i).getProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), String::empty);

				String absoluteFileName = directory + fileName;

				jassert(File(absoluteFileName).existsAsFile());

				absoluteTree.getChild(i).setProperty(ModulatorSamplerSound::getPropertyName(ModulatorSamplerSound::FileName), directory + fileName, nullptr);
			}

			treeToUse = &absoluteTree;
		}
		else
		{
			treeToUse = &v;
		}
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

		File sampleDirectory = File(firstFileName).getParentDirectory();

		jassert(sampleDirectory.isDirectory());

		ThumbnailHandler::loadThumbnails(sampler, sampleDirectory);
	}
}

void SampleMap::loadSamplesFromMonolith(const ValueTree &v)
{

	//sampler->deleteAllSounds();

	for(int i = 0; i < v.getNumChildren(); i++)
	{
		jassert(v.getChild(i).hasProperty("mono_sample_start"));

		sampler->addSamplerSound(v.getChild(i), i);
	}

	return;
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
