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

#if 0

namespace hise { using namespace juce;

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

			fileNames.add(treeToSearch.getProperty("SampleMap", String()));	
		}
	}

	for(int i = 0; i < treeToSearch.getNumChildren(); i++)
	{
        ValueTree child = treeToSearch.getChild(i);
        
		searchInternal(child, stripPath);
	}
};

ExternalResourceCollector::ExternalResourceCollector(MainController *mc_) :
	DialogWindowWithBackgroundThread("Copying external files into the project folder"),
	mc(mc_)
{
	numTotalSamples = mc->getSampleManager().getModulatorSamplerSoundPool2()->getNumSoundsInPool();

	numTotalImages = mc->getCurrentImagePool()->getNumLoadedFiles();
	numTotalAudioFiles = mc->getCurrentAudioSampleBufferPool()->getNumLoadedFiles();

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
		ModulatorSamplerSoundPool *pool = mc->getSampleManager().getModulatorSamplerSoundPool2();

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
	PresetHandler::showMessageWindow("Collecting finished", String(numSamplesCopied) + " samples copied", PresetHandler::IconType::Info);
}

} // namespace hise

#endif
