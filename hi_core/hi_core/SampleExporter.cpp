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

ReferenceSearcher::ReferenceSearcher(const File &presetFile)
{
	if(presetFile.existsAsFile())
	{
        FileInputStream fis(presetFile);
        
		preset = ValueTree::readFromStream(fis);
		if(preset.isValid()) ok = true;

	}
	else ok = false;

}

ReferenceSearcher::ReferenceSearcher(const ValueTree &preset_)
{
	preset = preset_.createCopy();
	
	ok = preset.isValid();
}


bool ReferenceSearcher::checkValidPreset()
{
	if(!preset.isValid())
	{
		std::cout << "Corrupt preset file.";
		return true;
	}
	else if(preset.getType() != Identifier("SynthChain"))
	{
		std::cout << "Preset must be a SynthChain (Type of preset: " << preset.getType().toString() << "\n";
		return true;
	}

	return false;
}


ValueTree ReferenceSearcher::getStrippedPreset()
{
	//presetFile.deleteFile();

	searchInternal(preset, true);

	return preset;
}

void SampleMapSearcher::searchInternal(ValueTree &treeToSearch, bool stripPath)
{
	if(treeToSearch.getChildWithName("samplemap").isValid())
	{
		if(stripPath)
		{
			String sampleMapPath = treeToSearch.getProperty("SampleMap");
			String id = File(sampleMapPath).getFileNameWithoutExtension();
			treeToSearch.removeProperty("SampleMap", nullptr);
			treeToSearch.setProperty("SampleMapId", id, nullptr);
		}
		else
		{
			if(treeToSearch.hasProperty("SampleMapId"))
			{
				std::cout << "Preset is already stripped!";
				return;
			}

			fileNames.add(treeToSearch.getProperty("SampleMap", String::empty));	
		}
	}

	for(int i = 0; i < treeToSearch.getNumChildren(); i++)
	{
        ValueTree child = treeToSearch.getChild(i);
        
		searchInternal(child, stripPath);
	}
};




bool SampleMapExporter::exportSamples(const String &directoryPath, const String &uniqueProductIdentifier, bool useBackgroundThread)
{
	if (exportToGlobalFolder)
	{
        ValueTree invalid = ValueTree::invalid; // Asshole OSX
        
		SampleWriter writer(this, fileNames, uniqueProductIdentifier, folderType, invalid);

		writer.runThread();
		return true;
	}

	if(!createSampleMapValueTree())
	{
		std::cout << "Aborting export";
		return false;
	}

	for(int i = 0; i < sampleMaps.getNumChildren(); i++)
	{
        ValueTree child = sampleMaps.getChild(i);
        
		String name;

		if (useOldMode())
		{
			name = File(sampleMapPaths[i]).getFileNameWithoutExtension();
		}
		else
		{
			name = child.getProperty("FileName");
		}

		if(useBackgroundThread)
		{            
			SampleMapExporter::SampleWriter writer(this, child, directoryPath, uniqueProductIdentifier, name);
			writer.runThread();
		}
		else
		{
            
			writeWaveFile(child, directoryPath, uniqueProductIdentifier, name);
		}
	}

	writeDataFile(directoryPath, uniqueProductIdentifier);

	return true;
}

void SampleMapExporter::writeWaveFile(ValueTree &sampleMapData, const String &directoryPath, const String &uniqueProductIdentifier, const String &name)
{	
	setupFilesAndFolders(sampleMapData, directoryPath, uniqueProductIdentifier, name);

	setupInputOutput(sampleMapData);

	for(int i = 0; i < sampleMapData.getNumChildren(); i++)
	{
		String message;

		writeSampleIntoMonolith(sampleMapData, i, &message);

		std::cout << message;
	}

	writer->flush();

	updateSampleMap(sampleMapData, uniqueProductIdentifier, name);

};


void SampleMapExporter::writeSampleIntoMonolith(ValueTree &sampleMapData, int sampleIndex, String *message)
{
	WavAudioFormat waf;

	String fileName = sampleMapData.getChild(sampleIndex).getProperty("FileName", String::empty);

	if(writeData.fileNames.contains(fileName))
	{
		for(int j = 0; j < sampleIndex; j++)
		{
			ValueTree prev = sampleMapData.getChild(j);

			if(prev.getProperty("FileName") == fileName)
			{
				const int64 prevStart = prev.getProperty("mono_sample_start", 0);
				const int64 prevLength = prev.getProperty("mono_sample_length", 0);

				sampleMapData.getChild(sampleIndex).setProperty("mono_sample_start", prevStart, nullptr);
				sampleMapData.getChild(sampleIndex).setProperty("mono_sample_length", prevLength, nullptr);

				break;
			}
		}

		jassert(sampleMapData.getChild(sampleIndex).hasProperty("mono_sample_start"));

		return;
	}

	writeData.fileNames.add(fileName);

	File f(writeData.samplePath + fileName);

	if(!f.existsAsFile())
	{
		std::cout << "File " << fileName << " not found!";
		return;
	}

	ScopedPointer<MemoryMappedAudioFormatReader> reader = waf.createMemoryMappedReader(f);

	reader->mapEntireFile();

	sampleMapData.getChild(sampleIndex).setProperty("mono_sample_start", writeData.pos, nullptr);

	writeData.pos += reader->lengthInSamples;

	sampleMapData.getChild(sampleIndex).setProperty("mono_sample_length", reader->lengthInSamples, nullptr);
			
	writer->writeFromAudioReader(*reader, 0, reader->lengthInSamples);

	*message << "writing " << fileName << "at " << String(writeData.pos) << " into " << writeData.monolithData.getFullPathName() << "\n";
}

void SampleMapExporter::updateSampleMap(ValueTree &sampleMapData, const String &uniqueProductIdentifier, const String &name)
{
	sampleMapData.setProperty("SampleMapIdentifier", name, nullptr);

	sampleMapData.removeProperty("FileName", nullptr);
	sampleMapData.removeProperty("sampleMapPath", nullptr);
	sampleMapData.removeProperty("RelativePath", nullptr);
	sampleMapData.removeProperty("SaveMode", nullptr);

	sampleMapData.setProperty("UniqueId", uniqueProductIdentifier, nullptr);

	sampleMapData.setProperty("Monolithic", "1", nullptr);
};

	

void SampleMapExporter::setupFilesAndFolders(const ValueTree &sampleMapData, const String &targetDirectory, const String &uniqueProductIdentifier, const String &name)
{
	writeData.sampleFolder = targetDirectory + "/" + uniqueProductIdentifier + " Samples";
	File folder(writeData.sampleFolder);
	folder.createDirectory();

	writeData.monolithData = File(writeData.sampleFolder + "/" + name + ".dat");
	writeData.monolithData.deleteFile();

	if (useOldMode())
	{
		File sampleDirectory = File(sampleMapData.getProperty("FileName", String::empty)).getParentDirectory();

		if (!sampleDirectory.exists() || !sampleDirectory.isDirectory())
		{
			std::cout << "Sample directory not found";
		}

		writeData.samplePath = sampleDirectory.getFullPathName() + "/samples/";
	}
	else
	{
		writeData.samplePath = "";
	}

	
}

void SampleMapExporter::setupInputOutput(const ValueTree &sampleMapData)
{
	writeData.pos = 0;

	String firstFile = writeData.samplePath + sampleMapData.getChild(0).getProperty("FileName", String::empty).toString();
	
	WavAudioFormat waf;
	ScopedPointer<MemoryMappedAudioFormatReader> firstReader = waf.createMemoryMappedReader(File(firstFile));

	writeData.sampleRate = (int)firstReader->sampleRate;
	writeData.bitsPerSample = firstReader->bitsPerSample;
	writeData.channels = firstReader->numChannels;

	firstReader = nullptr;
		
	FileOutputStream *output = new FileOutputStream(writeData.monolithData);

	writer = waf.createWriterFor(output, 
									writeData.sampleRate, 
									writeData.channels, 
									writeData.bitsPerSample, 
									waf.createBWAVMetadata("", "", "", Time::getCurrentTime(), 0, ""), 0);
}

bool SampleMapExporter::createSampleMapValueTree()
{
	if (!useOldMode()) return true;

	sampleMaps = ValueTree("samplemaps");

	for(int i = 0; i < sampleMapPaths.size(); i++)
	{
		ValueTree v = createValueTree(sampleMapPaths[i]);

		if(v.isValid())
		{
			sampleMaps.addChild(v, -1, nullptr);
		}
		else return false;
	}

	return true;
}

void SampleMapExporter::writeDataFile(const String &directoryPath, const String uniqueProductIdentifier)
{
	File f(directoryPath + "/samplemaps");

	sampleMaps.setProperty("ProductId", uniqueProductIdentifier, nullptr);

	f.deleteFile();

	std::cout << "Writing samplemap collection file " << f.getFullPathName() << " from collection " << uniqueProductIdentifier;


#if JUCE_WINDOWS
    
	ScopedPointer<XmlElement> xml = sampleMaps.createXml();
		
	xml->writeToFile(File("C:\\export\\samplemaps.xml"), "");
    
    FileOutputStream fos(f);

	sampleMaps.writeToStream(fos);
    
#endif
    
}

ValueTree SampleMapExporter::createValueTree(const String &sampleMapFile)
{
	ScopedPointer<XmlElement> xml = XmlDocument::parse(File(sampleMapFile));

	if(xml == nullptr)
	{
		std::cout << "SampleMap at " << sampleMapFile << " could not be found";

		return ValueTree::invalid;

	}

	ValueTree v = ValueTree::fromXml(*xml);

	v.setProperty("sampleMapPath", sampleMapFile, nullptr);

	return v;
};

void SampleMapExporter::SampleWriter::run()
{

	if (exportToGlobalFolder)
	{
		File sampleFolder = PresetPlayerHandler::getSpecialFolder(folderType, subFolderName);

		if (!sampleFolder.isDirectory())
		{
			sampleFolder.createDirectory();
		}

		AudioFormatManager afm;

		afm.registerBasicFormats();

		for (int i = 0; i < fileNames.size(); i++)
		{
			setProgress((double)i / (double)fileNames.size());
			setStatusMessage("Writing " + fileNames[i]);

			File sourceFile(fileNames[i]);


			if (sourceFile.getParentDirectory() == sampleFolder) continue;

			sourceFile.copyFileTo(sampleFolder.getFullPathName() + "/" + sourceFile.getFileName());

			DBG(fileNames[i]);
		}
	}
	else
	{
		exporter->setupFilesAndFolders(v, directoryPath, productId, sampleMapId);

		exporter->setupInputOutput(v);

		for (int i = 0; i < v.getNumChildren(); i++)
		{
			if (threadShouldExit())
			{
				break;
			}

			String message;

			exporter->writeSampleIntoMonolith(v, i, &message);

			setProgress((double)i / (double)v.getNumChildren());

			setStatusMessage(message);


		}

		exporter->writer->flush();

		exporter->updateSampleMap(v, productId, sampleMapId);
	}
	
}

ExternalResourceCollector::ExternalResourceCollector(MainController *mc_) :
ThreadWithAsyncProgressWindow("Copying external files into the project folder"),
mc(mc_)
{
	numTotalSamples = mc->getSampleManager().getModulatorSamplerSoundPool()->getNumSoundsInPool();

	numTotalImages = mc->getSampleManager().getImagePool()->getNumLoadedFiles();
	numTotalAudioFiles = mc->getSampleManager().getAudioSampleBufferPool()->getNumLoadedFiles();

	addTextBlock("Number of total samples: " + String(numTotalSamples));
	addTextBlock("Number of total images: " + String(numTotalImages));
	addTextBlock("Number of total audio files: " + String(numTotalAudioFiles));

	StringArray options;

	options.add("Copy everything (including samples)");
	options.add("Don't copy samples");

	addComboBox("options", options, "Select items to copy");

	addBasicComponents();
}

void ExternalResourceCollector::run()
{
	numSamplesCopied = 0;
	showStatusMessage("Copying samples");

	if (getComboBoxComponent("options")->getSelectedItemIndex() == 0)
	{
		ModulatorSamplerSoundPool *pool = mc->getSampleManager().getModulatorSamplerSoundPool();

		StringArray sampleList = pool->getFileNameList();

		StringArray newSampleList;

		File sampleDirectory = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Samples);

		

		String sampleDirectoryPathName = sampleDirectory.getFullPathName();

		for (int i = 0; i < sampleList.size(); i++)
		{
			setProgress((double)i / (double)(sampleList.size()-1));

			if (threadShouldExit()) return;

			if (sampleList[i].contains(sampleDirectoryPathName)) continue;

			showStatusMessage("Copying " + sampleList[i] + " to project folder");

			File f(sampleList[i]);

			if (f.existsAsFile())
			{
				File newFile = sampleDirectory.getChildFile(f.getFileName());

				f.copyFileTo(newFile);

				newSampleList.add(newFile.getFullPathName());
			}

			numSamplesCopied++;
		}
	}
}

void ExternalResourceCollector::threadFinished()
{
	PresetHandler::showMessageWindow("Collecting finished", String(numSamplesCopied) + " samples copied");
}
