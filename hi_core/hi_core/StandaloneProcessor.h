    /*
  ==============================================================================

    StandaloneProcessor.h
    Created: 18 Sep 2014 10:28:46am
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef STANDALONEPROCESSOR_H_INCLUDED
#define STANDALONEPROCESSOR_H_INCLUDED

namespace hise { using namespace juce;

class ToggleButtonList;




class GlobalSettingManager
{

public:

	class ScaleFactorListener
	{
	public:

		virtual ~ScaleFactorListener()
		{
			masterReference.clear();
		}

		virtual void scaleFactorChanged(float newScaleFactor) = 0;

	private:

		friend class WeakReference<ScaleFactorListener>;

		WeakReference<ScaleFactorListener>::Master masterReference;
	};

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

	void setVoiceAmountMultiplier(int newVoiceAmountMultiplier)
	{
		voiceAmountMultiplier = newVoiceAmountMultiplier;
	}

	void setEnabledMidiChannels(int newMidiChannelNumber)
	{
		channelData = newMidiChannelNumber;
	}

	int getChannelData() const { return channelData; };

	void setGlobalScaleFactor(double scaleFactor, NotificationType sendNotification=dontSendNotification);

	float getGlobalScaleFactor() const noexcept { return (float)scaleFactor; }

	void addScaleFactorListener(ScaleFactorListener* newListener)
	{
		listeners.addIfNotAlreadyThere(newListener);
	}

	void removeScaleFactorListener(ScaleFactorListener* listenerToRemove)
	{
		listeners.removeAllInstancesOf(listenerToRemove);
	}

	HiseSettings::Data& getSettingsObject()
	{ 
		jassert(dataObject != nullptr);
		return *dataObject;
	}

	const HiseSettings::Data& getSettingsObject() const 
	{
		jassert(dataObject != nullptr);
		return *dataObject;
	}

	HiseSettings::Data* getSettingsAsPtr()
	{
		return dataObject.get();
	}

	static File getSettingDirectory();

	static void restoreGlobalSettings(MainController* mc);

	void initData(MainController* mc)
	{
		dataObject = new HiseSettings::Data(mc);
	}

	void saveSettingsAsXml();

	int diskMode = 0;
	bool allSamplesFound = false;
	
	double globalBPM = -1.0;

	int voiceAmountMultiplier = 2;

	int channelData = 1;

#if HISE_USE_OPENGL_FOR_PLUGIN
	bool useOpenGL = (bool)HISE_DEFAULT_OPENGL_VALUE;
#else
	bool useOpenGL = false;
#endif

private:

	ScopedPointer<HiseSettings::Data> dataObject;

	double scaleFactor = 1.0;

	Array<WeakReference<ScaleFactorListener>> listeners;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GlobalSettingManager);
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

	void setAudioDeviceType(const String deviceName);

	void resetToDefault();

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

	/** Returns the state of each available MIDI input. */
	BigInteger getMidiInputState() const;
	
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

	void requestQuit();

	AudioProcessor* getCurrentProcessor() { return wrappedProcessor.get(); }
	const AudioProcessor* getCurrentProcessor() const { return wrappedProcessor.get(); }

private:

	ScopedPointer<AudioProcessor> wrappedProcessor;
	ScopedPointer<AudioDeviceManager> deviceManager;
	ScopedPointer<AudioProcessorPlayer> callback;

    ScopedPointer<MidiInput> virtualMidiPort;
    
	float scaleFactor = 1.0;
};

} // namespace hise

#endif  // STANDALONEPROCESSOR_H_INCLUDED
