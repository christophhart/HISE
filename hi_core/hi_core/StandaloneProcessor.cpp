
AudioDeviceDialog::AudioDeviceDialog(AudioProcessorDriver *ownerProcessor_) :
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
		ownerProcessor->saveDeviceSettingsAsXml();
		ScopedPointer<XmlElement> deviceData = ownerProcessor->deviceManager->createStateXml();
		ownerProcessor->initialiseAudioDriver(deviceData);
	}

#if USE_BACKEND
	dynamic_cast<BackendProcessorEditor*>(getParentComponent())->showSettingsWindow();
#endif
}

File AudioProcessorDriver::getDeviceSettingsFile()
{

#if JUCE_WINDOWS
#if USE_BACKEND
	String parentDirectory = File(PresetHandler::getDataFolder()).getFullPathName();
#else
	String parentDirectory = ProjectHandler::Frontend::getAppDataDirectory().getFullPathName();
#endif

	File parent(parentDirectory);

	if (!parent.isDirectory())
		parent.createDirectory();

#else

#if HISE_IOS
    String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName();
#else
    
#if USE_BACKEND
	String parentDirectory = File(PresetHandler::getDataFolder()).getFullPathName();
#else
	String parentDirectory = ProjectHandler::Frontend::getAppDataDirectory().getFullPathName();
#endif
#endif

	File parent(parentDirectory);
#endif

	File savedDeviceData = parent.getChildFile("DeviceSettings.xml");

	return savedDeviceData;
}


void AudioProcessorDriver::restoreSettings(MainController* mc)
{
	ScopedPointer<XmlElement> deviceData = getSettings();

	if (deviceData != nullptr)
	{
		int diskMode = deviceData->getIntAttribute("DISK_MODE");

#if HISE_IOS
		deviceData->setAttribute("audioDeviceBufferSize", 512);
#endif

		dynamic_cast<AudioProcessorDriver*>(mc)->diskMode = diskMode;
		dynamic_cast<AudioProcessorDriver*>(mc)->scaleFactor = deviceData->getDoubleAttribute("SCALE_FACTOR", 1.0);
		double microTuning = deviceData->getDoubleAttribute("MICRO_TUNING", 0.0);
		dynamic_cast<AudioProcessorDriver*>(mc)->microTuning = microTuning;

		mc->setGlobalPitchFactor(microTuning);
		mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)diskMode);

#if USE_FRONTEND
		bool allSamplesThere = deviceData->getBoolAttribute("SAMPLES_FOUND");

		if (!allSamplesThere)
		{
			dynamic_cast<FrontendProcessor*>(mc)->checkAllSampleReferences();
		}
		else
		{
			dynamic_cast<FrontendProcessor*>(mc)->setAllSampleReferencesCorrect();
		}
#endif
	}
}

void AudioProcessorDriver::saveDeviceSettingsAsXml()
{
	ScopedPointer<XmlElement> deviceData = deviceManager->createStateXml();

	if (deviceData == nullptr)
	{
		deviceData = new XmlElement("DEVICESETUP");
	}

	deviceData->setAttribute("DISK_MODE", diskMode);
	deviceData->setAttribute("SCALE_FACTOR", scaleFactor);
	deviceData->setAttribute("MICRO_TUNING", microTuning);


#if USE_FRONTEND
	deviceData->setAttribute("SAMPLES_FOUND", allSamplesFound);
#endif

	deviceData->writeToFile(getDeviceSettingsFile(), "");
}

void AudioProcessorDriver::setDiskMode(int mode)
{
	diskMode = mode;

	if (MainController* mc = dynamic_cast<MainController*>(callback->getCurrentProcessor()))
	{
		mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)mode);
	}
}

AudioDeviceDialog::~AudioDeviceDialog()
{

}


StandaloneProcessor::StandaloneProcessor()
{
	deviceManager = new AudioDeviceManager();
	callback = new AudioProcessorPlayer();

	wrappedProcessor = createProcessor();

    ScopedPointer<XmlElement> xml = AudioProcessorDriver::getSettings();
    
	if (xml != nullptr)
	{
		scaleFactor = (float)xml->getDoubleAttribute("SCALE_FACTOR", 1.0);
	}

#if USE_BACKEND
	if(!CompileExporter::isExportingFromCommandLine()) 
		dynamic_cast<AudioProcessorDriver*>(wrappedProcessor.get())->initialiseAudioDriver(xml);
#else
	dynamic_cast<AudioProcessorDriver*>(wrappedProcessor.get())->initialiseAudioDriver(xml);
	dynamic_cast<FrontendProcessor*>(wrappedProcessor.get())->loadSamplesAfterSetup();

#endif

	
		
}


XmlElement * AudioProcessorDriver::getSettings()
{
	File savedDeviceData = getDeviceSettingsFile();

	return XmlDocument::parse(savedDeviceData);
}

void AudioProcessorDriver::initialiseAudioDriver(XmlElement *deviceData)
{
	if (deviceData != nullptr && deviceData->hasTagName("deviceType") && deviceManager->initialise(0, 2, deviceData, false) == String())
	{
		callback->setProcessor(dynamic_cast<AudioProcessor*>(this));

		deviceManager->addAudioCallback(callback);
		deviceManager->addMidiInputCallback(String(), callback);
	}
	else
	{
		deviceManager->initialiseWithDefaultDevices(0, 2);

		callback->setProcessor(dynamic_cast<AudioProcessor*>(this));

		deviceManager->addAudioCallback(callback);
		deviceManager->addMidiInputCallback(String(), callback);
	}
}


void AudioProcessorDriver::setGlobalScaleFactor(double newScaleFactor)
{
	scaleFactor = newScaleFactor;
}

void AudioProcessorDriver::updateMidiToggleList(MainController* mc, ToggleButtonList* listToUpdate)
{
    
	ScopedPointer<XmlElement> midiSourceXml = dynamic_cast<AudioProcessorDriver*>(mc)->deviceManager->createStateXml();

	StringArray midiInputs = MidiInput::getDevices();

	if (midiSourceXml != nullptr)
	{
		for (int i = 0; i < midiSourceXml->getNumChildElements(); i++)
		{
			if (midiSourceXml->getChildElement(i)->hasTagName("MIDIINPUT"))
			{
				const String activeInputName = midiSourceXml->getChildElement(i)->getStringAttribute("name");

				const int activeInputIndex = midiInputs.indexOf(activeInputName);

				if (activeInputIndex != -1)
				{
					listToUpdate->setValue(activeInputIndex, true, dontSendNotification);
				}
			}
		}
	}
}