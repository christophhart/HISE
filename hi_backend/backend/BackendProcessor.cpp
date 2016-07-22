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

AudioDeviceDialog::AudioDeviceDialog(BackendProcessor *ownerProcessor_) :
ownerProcessor(ownerProcessor_)
{
	setOpaque(true);

	selector = new AudioDeviceSelectorComponent(*ownerProcessor->deviceManager, 0, 0, 2, 2, true, false, true, false);

	setLookAndFeel(&alaf);

	selector->setLookAndFeel(&pplaf);

	addAndMakeVisible(cancelButton = new TextButton("Cancel"));
	addAndMakeVisible(applyAndCloseButton = new TextButton("Apply changes & close window"));

	cancelButton->addListener(this);
	applyAndCloseButton->addListener(this);

	addAndMakeVisible(selector);
}


void AudioDeviceDialog::buttonClicked(Button *b)
{
	if (b == applyAndCloseButton)
	{
		ScopedPointer<XmlElement> deviceData = ownerProcessor->deviceManager->createStateXml();

		if (deviceData != nullptr)
		{
			

#if JUCE_WINDOWS
			String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName() + "/Hart Instruments";

			File parent(parentDirectory);

			if (!parent.isDirectory())
			{
				parent.createDirectory();
			}

#else
            
#if HISE_IOS
    
            String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName();
#else
            
			String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::currentExecutableFile).getParentDirectory().getFullPathName();
#endif
#endif


			File savedDeviceData = File(parentDirectory + "/DeviceSettings.xml");

			deviceData->writeToFile(savedDeviceData, "");
		}

		ownerProcessor->initialiseAudioDriver(deviceData);
	}

	dynamic_cast<BackendProcessorEditor*>(getParentComponent())->showSettingsWindow();
}

AudioDeviceDialog::~AudioDeviceDialog()
{
	
}

BackendProcessor::BackendProcessor(AudioDeviceManager *deviceManager_/*=nullptr*/, AudioProcessorPlayer *callback_/*=nullptr*/) :
MainController(),
deviceManager(deviceManager_),
callback(callback_),
viewUndoManager(new UndoManager()),
synthChain(nullptr)
{
    synthChain = new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES, viewUndoManager);

    
	synthChain->addProcessorsWhenEmpty();

	getSampleManager().getModulatorSamplerSoundPool()->setDebugProcessor(synthChain);

	getMacroManager().setMacroChain(synthChain);

	handleEditorData(false);

	createUserPresetData();
}


BackendProcessor::~BackendProcessor()
{
	synthChain = nullptr;

	jassert(editorInformation.getType() == Identifier("editorData"));

	handleEditorData(true);
}

void BackendProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	processBlockCommon(buffer, midiMessages);
};

void BackendProcessor::handleControllersForMacroKnobs(const MidiBuffer &/*midiMessages*/)
{
	
}


void BackendProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
    
	MainController::prepareToPlay(sampleRate, samplesPerBlock);
};

AudioProcessorEditor* BackendProcessor::createEditor()
{
	return new BackendProcessorEditor(this, editorInformation);
}

void BackendProcessor::setEditorState(ValueTree &editorState)
{
	editorInformation = ValueTree(editorState);
}

