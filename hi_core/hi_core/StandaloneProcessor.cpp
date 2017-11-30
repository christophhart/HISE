namespace hise { using namespace juce;

AudioDeviceDialog::AudioDeviceDialog(AudioProcessorDriver *ownerProcessor_) :
ownerProcessor(ownerProcessor_)
{
	setName("Audio Settings");

	setOpaque(false);

	selector = new AudioDeviceSelectorComponent(*ownerProcessor->deviceManager, 0, 0, 2, 2, true, false, true, false);

	setLookAndFeel(&alaf);

	selector->setLookAndFeel(&alaf);

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
	findParentComponentOfClass<FloatingTilePopup>()->deleteAndClose();
#endif
}

File AudioProcessorDriver::getDeviceSettingsFile()
{

	File parent = getSettingDirectory();

	File savedDeviceData = parent.getChildFile("DeviceSettings.xml");

	return savedDeviceData;
}


void AudioProcessorDriver::restoreSettings(MainController* /*mc*/)
{
	
}

void AudioProcessorDriver::saveDeviceSettingsAsXml()
{
    
	ScopedPointer<XmlElement> deviceData = deviceManager != nullptr ?
                                           deviceManager->createStateXml():
                                           nullptr;

	if (deviceData != nullptr)
	{
		deviceData->writeToFile(getDeviceSettingsFile(), "");

		
	}
}

void GlobalSettingManager::setDiskMode(int mode)
{
	diskMode = mode;

	if (MainController* mc = dynamic_cast<MainController*>(this))
	{
		mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)mode);
	}
}

AudioDeviceDialog::~AudioDeviceDialog()
{

}


StandaloneProcessor::StandaloneProcessor()
{
	LOG_START("Creating Device Manager");

	deviceManager = new AudioDeviceManager();
	callback = new AudioProcessorPlayer();

	LOG_START("Create Main Processor");

	wrappedProcessor = createProcessor();


	ScopedPointer<XmlElement> xml = AudioProcessorDriver::getSettings();

	if (xml != nullptr)
	{
		dynamic_cast<MainController*>(wrappedProcessor.get())->getDebugLogger().logMessage("Sucessfully loaded XML settings from disk");
	}

#if USE_BACKEND
	if(!CompileExporter::isExportingFromCommandLine()) 
		dynamic_cast<AudioProcessorDriver*>(wrappedProcessor.get())->initialiseAudioDriver(xml);
#else
	
    auto apd = dynamic_cast<AudioProcessorDriver*>(wrappedProcessor.get());
    
    jassert(apd != nullptr);
    
	LOG_START("Initialise Audio Driver...");

    apd->initialiseAudioDriver(xml);
    
	LOG_START("OK");

#endif
	
}


void StandaloneProcessor::requestQuit(const std::function<void(void)>& f)
{
	auto f2 = [f](Processor* ) {f(); return true; };

	auto mc = dynamic_cast<MainController*>(wrappedProcessor.get());
	
	mc->getKillStateHandler().killVoicesAndCall(mc->getMainSynthChain(), f2, MainController::KillStateHandler::TargetThread::MessageThread);
}

XmlElement * AudioProcessorDriver::getSettings()
{
	File savedDeviceData = getDeviceSettingsFile();

	return XmlDocument::parse(savedDeviceData);
}

void AudioProcessorDriver::initialiseAudioDriver(XmlElement *deviceData)
{
    jassert(deviceManager != nullptr);
    
	DebugLogger& logger = dynamic_cast<MainController*>(this)->getDebugLogger();

	if (deviceData != nullptr)
	{
		logger.logMessage("Audio Driver Initialisation with Settings:  \n\n```xml\n" + deviceData->createDocument("") + "```\n");
	}

	if (deviceData != nullptr && deviceData->hasTagName("DEVICESETUP"))
	{
		String errorMessage = deviceManager->initialise(0, 2, deviceData, true);

		if (errorMessage.isNotEmpty())
		{
			logger.logMessage("Error initialising with stored settings: " + errorMessage);

			logger.logMessage("Audio Driver Default Initialisation");

			const String error = deviceManager->initialiseWithDefaultDevices(0, 2);

			if (error.isNotEmpty())
			{
				logger.logMessage("Error initialising with default settings: " + error);
			}
			else
			{
				logger.logMessage("... audio driver successfully initialised with default settings");
			}
		}
		else
		{
			logger.logMessage("... audio driver successfully initialised");
		}
	}
	else
	{
		logger.logMessage("Audio Driver Default Initialisation");

		const String error = deviceManager->initialiseWithDefaultDevices(0, 2);

		if (error.isNotEmpty())
		{
			logger.logMessage("Error initialising with default settings: " + error);
		}
		else
		{
			logger.logMessage("... audio driver successfully initialised with default settings");
		}
	}

	callback->setProcessor(dynamic_cast<AudioProcessor*>(this));

	deviceManager->addAudioCallback(callback);
	deviceManager->addMidiInputCallback(String(), callback);
}

void GlobalSettingManager::setGlobalScaleFactor(double newScaleFactor, NotificationType notifyListeners/*=dontSendNotification*/)
{
	if (scaleFactor != newScaleFactor)
	{
		scaleFactor = newScaleFactor;

		if (notifyListeners != dontSendNotification)
		{
			for (int i = 0; i < listeners.size(); i++)
			{
				if (listeners[i].get() != nullptr)
				{
					listeners[i]->scaleFactorChanged((float)scaleFactor);
				}
			}
		}
	}
}

void GlobalSettingManager::setUseOpenGLRenderer(bool shouldUseOpenGL)
{
	useOpenGL = shouldUseOpenGL;
}

void AudioProcessorDriver::updateMidiToggleList(MainController* mc, ToggleButtonList* listToUpdate)
{
#if USE_BACKEND || IS_STANDALONE_APP
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
#else
	ignoreUnused(mc, listToUpdate);
#endif
}


GlobalSettingManager::GlobalSettingManager()
{
	ScopedPointer<XmlElement> xml = AudioProcessorDriver::getSettings();

	if (xml != nullptr)
	{
		scaleFactor = (float)xml->getDoubleAttribute("SCALE_FACTOR", 1.0);
	}
}

void GlobalSettingManager::restoreGlobalSettings(MainController* mc)
{
	LOG_START("Restoring global settings");

	File savedDeviceData = getGlobalSettingsFile();

	ScopedPointer<XmlElement> globalSettings = XmlDocument::parse(savedDeviceData);

#if USE_FRONTEND
	if (globalSettings == nullptr)
		dynamic_cast<FrontendSampleManager*>(mc)->checkAllSampleReferences();
#endif

	if (globalSettings != nullptr)
	{
		GlobalSettingManager* gm = dynamic_cast<GlobalSettingManager*>(mc);

		gm->diskMode = globalSettings->getIntAttribute("DISK_MODE");
		gm->scaleFactor = globalSettings->getDoubleAttribute("SCALE_FACTOR", 1.0);
		gm->microTuning = globalSettings->getDoubleAttribute("MICRO_TUNING", 0.0);
		gm->transposeValue = globalSettings->getIntAttribute("TRANSPOSE", 0);

#if IS_STANDALONE_APP
		// Don't save this for plugins as they are usually synced to the host
		gm->globalBPM = globalSettings->getIntAttribute("GLOBAL_BPM", -1);
#endif

		gm->channelData = globalSettings->getIntAttribute("MIDI_CHANNELS", 1);

		gm->ccSustainValue = globalSettings->getIntAttribute("SUSTAIN_CC", 64);

		gm->useOpenGL = globalSettings->getBoolAttribute("OPEN_GL", false);

		mc->setGlobalPitchFactor(gm->microTuning);

		mc->getEventHandler().setGlobalTransposeValue(gm->transposeValue);

		gm->voiceAmountMultiplier = globalSettings->getIntAttribute("VOICE_AMOUNT_MULTIPLIER", 2);

		mc->getEventHandler().addCCRemap(gm->ccSustainValue, 64);
		mc->getSampleManager().setDiskMode((MainController::SampleManager::DiskMode)gm->diskMode);
		mc->getMainSynthChain()->getActiveChannelData()->restoreFromData(gm->channelData);

#if USE_FRONTEND
		bool allSamplesThere = globalSettings->getBoolAttribute("SAMPLES_FOUND");

		if (!allSamplesThere)
		{
			LOG_START("Samples not validated. Checking references");
			dynamic_cast<FrontendSampleManager*>(mc)->checkAllSampleReferences();
		}
		else
		{
			LOG_START("Samples are validated. Skipping reference check");
			dynamic_cast<FrontendSampleManager*>(mc)->setAllSampleReferencesCorrect();
		}
#else
		ignoreUnused(mc);
#endif
	}
}

void GlobalSettingManager::saveSettingsAsXml()
{
	ScopedPointer<XmlElement> settings = new XmlElement("GLOBAL_SETTINGS");

	settings->setAttribute("DISK_MODE", diskMode);
	settings->setAttribute("SCALE_FACTOR", scaleFactor);
	settings->setAttribute("MICRO_TUNING", microTuning);
	settings->setAttribute("TRANSPOSE", transposeValue);
	settings->setAttribute("SUSTAIN_CC", ccSustainValue);
	settings->setAttribute("VOICE_AMOUNT_MULTIPLIER", voiceAmountMultiplier);

#if IS_STANDALONE_APP
	// Dont save this in plugins
	settings->setAttribute("GLOBAL_BPM", globalBPM);
#endif

	settings->setAttribute("OPEN_GL", useOpenGL);
	settings->setAttribute("MIDI_CHANNELS", channelData);

#if USE_FRONTEND
	settings->setAttribute("SAMPLES_FOUND", allSamplesFound);
#endif

	settings->writeToFile(getGlobalSettingsFile(), "");

}

File GlobalSettingManager::getSettingDirectory()
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

#if ENABLE_APPLE_SANDBOX
	String parentDirectory = ProjectHandler::Frontend::getAppDataDirectory().getChildFile("Resources/").getFullPathName();
#else
	String parentDirectory = ProjectHandler::Frontend::getAppDataDirectory().getFullPathName();
#endif



#endif
#endif

	File parent(parentDirectory);
#endif

	return parent;

}

} // namespace hise