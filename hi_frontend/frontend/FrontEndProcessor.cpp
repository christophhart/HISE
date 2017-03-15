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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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


void FrontendProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
#if USE_COPY_PROTECTION || USE_TURBO_ACTIVATE
	if (((unlockCounter++ & 1023) == 0) && !unlocker.isUnlocked()) return;
#endif

	getDelayedRenderer().processWrapped(buffer, midiMessages);
};

void FrontendProcessor::handleControllersForMacroKnobs(const MidiBuffer &midiMessages)
{
	if(!getMacroManager().macroControlMidiLearnModeActive() && !getMacroManager().midiMacroControlActive()) return;

	MidiBuffer::Iterator it(midiMessages);

	int samplePos;
	MidiMessage message;

	while(it.getNextEvent(message, samplePos))
	{
		if(message.isController())
		{
			const int controllerNumber = message.getControllerNumber();

			if(getMacroManager().macroControlMidiLearnModeActive())
			{
				getMacroManager().setMidiControllerForMacro(controllerNumber);
			}

			const int macroNumber = getMacroManager().getMacroControlForMidiController(controllerNumber);

			if(macroNumber != -1)
			{
				getMacroManager().getMacroChain()->setMacroControl(macroNumber, (float)message.getControllerValue(), sendNotification);
			}
		}
	}

}

FrontendProcessor::FrontendProcessor(ValueTree &synthData, AudioDeviceManager* manager, AudioProcessorPlayer* callback_, ValueTree *imageData_/*=nullptr*/, ValueTree *impulseData/*=nullptr*/, ValueTree *externalFiles/*=nullptr*/, ValueTree *userPresets) :
MainController(),
PluginParameterAudioProcessor(ProjectHandler::Frontend::getProjectName()),
AudioProcessorDriver(manager, callback_),
synthChain(new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES)),
keyFileCorrectlyLoaded(true),
presets(*userPresets),
currentlyLoadedProgram(0),

#if USE_TURBO_ACTIVATE
unlockCounter(0),
#if JUCE_WINDOWS
unlocker(ProjectHandler::Frontend::getAppDataDirectory().getFullPathName().toUTF16().getAddress())
#else
unlocker(ProjectHandler::Frontend::getAppDataDirectory().getFullPathName().toUTF8().getAddress())
#endif
#else
unlockCounter(0)
#endif
{
#if USE_COPY_PROTECTION
	if (PresetHandler::loadKeyFile(unlocker))
	{
	}
	else
	{
		keyFileCorrectlyLoaded = false;
	}
#elif USE_TURBO_ACTIVATE
	
	keyFileCorrectlyLoaded = unlocker.isUnlocked();

#endif
    

	loadImages(imageData_);

	if (externalFiles != nullptr)
	{
		setExternalScriptData(externalFiles->getChildWithName("ExternalScripts"));
		restoreCustomFontValueTree(externalFiles->getChildWithName("CustomFonts"));

		sampleMaps = externalFiles->getChildWithName("SampleMaps");
	}

	if (impulseData != nullptr)
	{
		getSampleManager().getAudioSampleBufferPool()->restoreFromValueTree(*impulseData);
	}
	else
	{
		File audioResourceFile(ProjectHandler::Frontend::getAppDataDirectory().getChildFile("AudioResources.dat"));

		if (audioResourceFile.existsAsFile())
		{
			FileInputStream fis(audioResourceFile);

			ValueTree impulseDataFile = ValueTree::readFromStream(fis);

			if (impulseDataFile.isValid())
			{
				getSampleManager().getAudioSampleBufferPool()->restoreFromValueTree(impulseDataFile);
			}
		}
	}

	numParameters = 0;

	getMacroManager().setMacroChain(synthChain);

	synthChain->setId(synthData.getProperty("ID", String()));

	suspendProcessing(true);

	getSampleManager().setShouldSkipPreloading(true);

	synthChain->restoreFromValueTree(synthData);

    
#if ENABLE_FILE_LOGGING
    
    const String name = ProjectHandler::Frontend::getCompanyName() + " " + ProjectHandler::Frontend::getProjectName();
    const String version = " Version " + ProjectHandler::Frontend::getVersionString();
    
    Logger::setCurrentLogger(new FileLogger(ProjectHandler::Frontend::getAppDataDirectory().getChildFile("log.txt"), "Log file for " + name + version));
    
#endif

    
	synthChain->compileAllScripts();

	synthChain->loadMacrosFromValueTree(synthData);

	addScriptedParameters();

	CHECK_COPY_AND_RETURN_6(synthChain);

	if (getSampleRate() > 0)
	{
		synthChain->prepareToPlay(getSampleRate(), getBlockSize());
	}

	suspendProcessing(false);

	createUserPresetData();
}

const String FrontendProcessor::getName(void) const
{
	return ProjectHandler::Frontend::getProjectName();
}

void FrontendProcessor::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	suspendProcessing(true);

	CHECK_COPY_AND_RETURN_1(synthChain);
		
	getDelayedRenderer().prepareToPlayWrapped(newSampleRate, samplesPerBlock);

	suspendProcessing(false);
};

AudioProcessorEditor* FrontendProcessor::createEditor()
{
	return new FrontendProcessorEditor(this);
}

void FrontendProcessor::setCurrentProgram(int /*index*/)
{
	return;
}

const String FrontendStandaloneApplication::getApplicationName()
{
	return ProjectInfo::projectName;
}

const String FrontendStandaloneApplication::getApplicationVersion()
{
	return ProjectInfo::versionString;
}
