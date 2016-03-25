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
			String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::currentExecutableFile).getParentDirectory().getFullPathName();
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
synthChain(new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES, viewUndoManager))
{
	synthChain->addProcessorsWhenEmpty();

	getSampleManager().getModulatorSamplerSoundPool()->setDebugProcessor(synthChain);

	getMacroManager().setMacroChain(synthChain);

	handleEditorData(false);
}


BackendProcessor::~BackendProcessor()
{
	synthChain = nullptr;

	jassert(editorInformation.getType() == Identifier("editorData"));

	handleEditorData(true);
}

void BackendProcessor::processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
    AudioPlayHead::CurrentPositionInfo newTime;

    if(buffer.getNumSamples() != getBlockSize())
    {
        debugError(synthChain, "Block size mismatch (old: " + String(getBlockSize()) + ", new: " + String(buffer.getNumSamples()));
        //prepareToPlay(getSampleRate(), buffer.getNumSamples());
    }
    
	ScopedNoDenormals snd;

    if (getPlayHead() != nullptr && getPlayHead()->getCurrentPosition (newTime))
    {
        lastPosInfo = newTime;
    }
    else
    {
        lastPosInfo.resetToDefault();
    };

	storePlayheadIntoDynamicObject(lastPosInfo);

	setBpm(lastPosInfo.bpm);

#if USE_HI_DEBUG_TOOLS 
	startCpuBenchmark(buffer.getNumSamples()); 
#endif

	if(isSuspended())
	{
		buffer.clear();
	}

	checkAllNotesOff(midiMessages);

#if USE_MIDI_CONTROLLERS_FOR_MACROS

	handleControllersForMacroKnobs(midiMessages);

#endif

	keyboardState.processNextMidiBuffer (midiMessages, 0, buffer.getNumSamples(), true);

	
	multiChannelBuffer.clear();

	synthChain->renderNextBlockWithModulators(multiChannelBuffer, midiMessages);

	FloatVectorOperations::copy(buffer.getWritePointer(0), multiChannelBuffer.getReadPointer(0), buffer.getNumSamples());
	FloatVectorOperations::copy(buffer.getWritePointer(1), multiChannelBuffer.getReadPointer(1), buffer.getNumSamples());

	for (int i = 0; i < buffer.getNumChannels(); i++)
	{
		FloatVectorOperations::clip(buffer.getWritePointer(i, 0), buffer.getReadPointer(i, 0), -1.0f, 1.0f, buffer.getNumSamples());
	}

	
	midiMessages.clear();

#if USE_HI_DEBUG_TOOLS
	stopCpuBenchmark();
#endif

	uptime += double(buffer.getNumSamples()) / getSampleRate();

};

void BackendProcessor::handleControllersForMacroKnobs(const MidiBuffer &midiMessages)
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


void BackendProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    setRateAndBufferSizeDetails(sampleRate, samplesPerBlock);
    
	MainController::prepareToPlay(sampleRate, samplesPerBlock);

	synthChain->prepareToPlay(sampleRate, samplesPerBlock);

	multiChannelBuffer.setSize(synthChain->getMatrix().getNumDestinationChannels(), samplesPerBlock);
	
};

// RENAME BACK!

AudioProcessorEditor* BackendProcessor::createEditor()
{
	return new BackendProcessorEditor(this, editorInformation);
}

void BackendProcessor::setEditorState(ValueTree &editorState)
{
	editorInformation = ValueTree(editorState);
}

