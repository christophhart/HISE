    /*
  ==============================================================================

    StandaloneProcessor.h
    Created: 18 Sep 2014 10:28:46am
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef STANDALONEPROCESSOR_H_INCLUDED
#define STANDALONEPROCESSOR_H_INCLUDED

class ToggleButtonList;


class GlobalSettingManager
{

public:

	GlobalSettingManager();

	virtual ~GlobalSettingManager()
	{
		saveSettingsAsXml();
	}

	

	static File getGlobalSettingsFile()
	{
		return getSettingDirectory().getChildFile("GeneralSettings.xml");
	}

	void setDiskMode(int mode);

	void storeAllSamplesFound(bool areFound) noexcept
	{
		allSamplesFound = areFound;
	}

	void setGlobalScaleFactor(double scaleFactor);

	static File getSettingDirectory();

	static void restoreGlobalSettings(MainController* mc);

	void saveSettingsAsXml();

	int diskMode = 0;
	bool allSamplesFound = false;
	double scaleFactor = 1.0;
	double microTuning = 0.0;
	int transposeValue = 0;
	int ccSustainValue = 64;

};

class AudioProcessorDriver: public GlobalSettingManager
{
public:

	AudioProcessorDriver(AudioDeviceManager* manager, AudioProcessorPlayer* callback_) :
		GlobalSettingManager(),
		callback(callback_),
		deviceManager(manager)
	{};

	virtual ~AudioProcessorDriver()
	{
        saveDeviceSettingsAsXml();
        

		deviceManager = nullptr;
		callback = nullptr;
	}

	double getCurrentSampleRate()
	{
		return callback->getCurrentProcessor()->getSampleRate();
	}

	int getCurrentBlockSize()
	{
		return callback->getCurrentProcessor()->getBlockSize();
	}

	void setCurrentSampleRate(double newSampleRate)
	{
		AudioDeviceManager::AudioDeviceSetup currentSetup;
		
		deviceManager->getAudioDeviceSetup(currentSetup);
		currentSetup.sampleRate = newSampleRate;
		deviceManager->setAudioDeviceSetup(currentSetup, true);
	}

	void setCurrentBlockSize(int newBlockSize)
	{
		AudioDeviceManager::AudioDeviceSetup currentSetup;

		deviceManager->getAudioDeviceSetup(currentSetup);
		currentSetup.bufferSize = newBlockSize;
		deviceManager->setAudioDeviceSetup(currentSetup, true);
	}

	static File getDeviceSettingsFile();

	static void restoreSettings(MainController* mc);

	void saveDeviceSettingsAsXml();

	void setAudioDeviceType(const String deviceName)
	{
		deviceManager->setCurrentAudioDeviceType(deviceName, true);
	}

	

	void setOutputChannelName(const int channelIndex)
	{
		AudioDeviceManager::AudioDeviceSetup currentSetup;

		deviceManager->getAudioDeviceSetup(currentSetup);
		
		BigInteger thisChannels = 0;
		thisChannels.setBit(channelIndex);
		currentSetup.outputChannels = thisChannels;

		deviceManager->setAudioDeviceSetup(currentSetup, true);
	}

	void setAudioDevice(const String &deviceName)
	{
		AudioDeviceManager::AudioDeviceSetup currentSetup;

		deviceManager->getAudioDeviceSetup(currentSetup);
		currentSetup.outputDeviceName = deviceName;
		deviceManager->setAudioDeviceSetup(currentSetup, true);
	}

	void toggleMidiInput(const String &midiInputName, bool enableInput)
	{
		if (midiInputName.isNotEmpty())
		{
			deviceManager->setMidiInputEnabled(midiInputName, enableInput);
		}
	}

	

	static void updateMidiToggleList(MainController* mc, ToggleButtonList* listToUpdate);

	
	static XmlElement *getSettings();

	void initialiseAudioDriver(XmlElement *deviceData);

	AudioDeviceManager *deviceManager;
	AudioProcessorPlayer *callback;
	
};

class AudioDeviceDialog : public Component,
	public ButtonListener
{
public:

	AudioDeviceDialog(AudioProcessorDriver *ownerProcessor_);

	void resized() override
	{
		selector->setBounds(0, 0, getWidth(), getHeight() - 36);
		cancelButton->setBounds(getWidth() - 100, getHeight() - 36, 80, 32);
		applyAndCloseButton->setBounds(20, getHeight() - 36, 200, 32);
	}

	void buttonClicked(Button *b);

	

	void paint(Graphics &g) override
	{
		g.fillAll(Colour(0xFF444444));
	}

	~AudioDeviceDialog();


private:

	ScopedPointer<AudioDeviceSelectorComponent> selector;

	ScopedPointer<TextButton> applyAndCloseButton;
	ScopedPointer<TextButton> cancelButton;

	AudioProcessorDriver *ownerProcessor;

	HiPropertyPanelLookAndFeel pplaf;
	AlertWindowLookAndFeel alaf;

};



class StandaloneProcessor
{
public:

	StandaloneProcessor();

	~StandaloneProcessor()
	{
		
		deviceManager->removeAudioCallback(callback);
		deviceManager->removeMidiInputCallback(String(), callback);
        deviceManager->closeAudioDevice();
        
		callback = nullptr;
		wrappedProcessor = nullptr;
		deviceManager = nullptr;
	}

	AudioProcessor* createProcessor();;

	AudioProcessorEditor *createEditor()
	{
		return wrappedProcessor->createEditor();
	}

	float getScaleFactor() const 
	{ 
#if USE_BACKEND
		return 1.0;
#else
		return scaleFactor; 
#endif
	}

private:

	ScopedPointer<AudioProcessor> wrappedProcessor;
	ScopedPointer<AudioDeviceManager> deviceManager;
	ScopedPointer<AudioProcessorPlayer> callback;
	
	float scaleFactor = 1.0;

};



#endif  // STANDALONEPROCESSOR_H_INCLUDED
