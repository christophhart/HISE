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

		virtual ~ScaleFactorListener();

		virtual void scaleFactorChanged(float newScaleFactor) = 0;

	private:

		friend class WeakReference<ScaleFactorListener>;

		WeakReference<ScaleFactorListener>::Master masterReference;
	};

	GlobalSettingManager();

	virtual ~GlobalSettingManager();


	static File getGlobalSettingsFile();

	static String getHiseVersion();

	void setDiskMode(int mode);

	void storeAllSamplesFound(bool areFound) noexcept;

	void setVoiceAmountMultiplier(int newVoiceAmountMultiplier);

	void setEnabledMidiChannels(int newMidiChannelNumber);

	int getChannelData() const;;

	void setGlobalScaleFactor(double scaleFactor, NotificationType sendNotification=dontSendNotification);

	float getGlobalScaleFactor() const noexcept;

	void addScaleFactorListener(ScaleFactorListener* newListener);

	void removeScaleFactorListener(ScaleFactorListener* listenerToRemove);

	HiseSettings::Data& getSettingsObject();

	const HiseSettings::Data& getSettingsObject() const;

	HiseSettings::Data* getSettingsAsPtr();

	static File getSettingDirectory();

	static void restoreGlobalSettings(MainController* mc, bool checkReferences=true);

	void initData(MainController* mc);

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

	AudioProcessorDriver(AudioDeviceManager* manager, AudioProcessorPlayer* callback_);;

	virtual ~AudioProcessorDriver();

	double getCurrentSampleRate();

	int getCurrentBlockSize();

	void setCurrentSampleRate(double newSampleRate);

	void setCurrentBlockSize(int newBlockSize);

	static File getDeviceSettingsFile();

	static void restoreSettings(MainController* mc);

	void saveDeviceSettingsAsXml();

	void setAudioDeviceType(const String deviceName);

	void resetToDefault();

	void setOutputChannelName(const int channelIndex);

	void setAudioDevice(const String &deviceName);

	void toggleMidiInput(const String &midiInputName, bool enableInput);

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

	~StandaloneProcessor();

	AudioProcessor* createProcessor();;

	AudioProcessorEditor *createEditor();

	float getScaleFactor() const;

	void requestQuit();

	AudioProcessor* getCurrentProcessor();
	const AudioProcessor* getCurrentProcessor() const;

private:

	ScopedPointer<AudioProcessor> wrappedProcessor;
	ScopedPointer<AudioDeviceManager> deviceManager;
	ScopedPointer<AudioProcessorPlayer> callback;

    ScopedPointer<MidiInput> virtualMidiPort;
    
	float scaleFactor = 1.0;
};

} // namespace hise

#endif  // STANDALONEPROCESSOR_H_INCLUDED
