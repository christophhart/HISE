
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
	String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName() + "/Hart Instruments";
#else
	String parentDirectory = ProjectHandler::Frontend::getAppDataDirectory().getFullPathName();
#endif

	File parent(parentDirectory);

	if (!parent.isDirectory())
		parent.createDirectory();

#else

#if USE_BACKEND
#if HISE_IOS
	String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory).getFullPathName();
#else
	String parentDirectory = File::getSpecialLocation(File::SpecialLocationType::currentExecutableFile).getParentDirectory().getFullPathName();
#endif
#else
	String parentDirectory = ProjectHandler::Frontend::getAppDataDirectory().getFullPathName();
#endif

	File parent(parentDirectory);
#endif

	File savedDeviceData = parent.getChildFile("DeviceSettings.xml");

	return savedDeviceData;
}

void AudioProcessorDriver::saveDeviceSettingsAsXml()
{
	ScopedPointer<XmlElement> deviceData = deviceManager->createStateXml();

	if (deviceData != nullptr)
	{
		deviceData->writeToFile(getDeviceSettingsFile(), "");
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

	ScopedPointer<XmlElement> deviceData = dynamic_cast<AudioProcessorDriver*>(wrappedProcessor.get())->getSettings();

	dynamic_cast<AudioProcessorDriver*>(wrappedProcessor.get())->initialiseAudioDriver(deviceData);
}


XmlElement * AudioProcessorDriver::getSettings()
{
	File savedDeviceData = getDeviceSettingsFile();

	return XmlDocument::parse(savedDeviceData);
}

void AudioProcessorDriver::initialiseAudioDriver(XmlElement *deviceData)
{
	if (deviceData != nullptr && deviceManager->initialise(0, 2, deviceData, false) == String::empty)
	{
		callback->setProcessor(dynamic_cast<AudioProcessor*>(this));

		deviceManager->addAudioCallback(callback);
		deviceManager->addMidiInputCallback(String::empty, callback);
	}
	else
	{
		deviceManager->initialiseWithDefaultDevices(0, 2);

		callback->setProcessor(dynamic_cast<AudioProcessor*>(this));

		deviceManager->addAudioCallback(callback);
		deviceManager->addMidiInputCallback(String::empty, callback);
	}
}
