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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef FRONTENDPROCESSOR_H_INCLUDED
#define FRONTENDPROCESSOR_H_INCLUDED

namespace hise { using namespace juce;

#if USE_COPY_PROTECTION

#if USE_SCRIPT_COPY_PROTECTION
using Unlocker = hise::ScriptUnlocker;
#else



class Unlocker : public UnlockerHandler
{
public:

	Unlocker();
	~Unlocker();

	OnlineUnlockStatus* getUnlockerObject() final override;

	var loadKeyFile();

	var isUnlocked() const;

	static RSAKey getPublicKey();

	String getEmailAdress() const;

	String getMachineId() const;

	var isValidMachine(const String& machineId) const;

	static void showActivationWindow(Component* overlay);

	static void resolveLicenseFile(Component* overlay);

	String getProductErrorMessage() const;

private:

	friend class OnlineActivator;

	class Pimpl;
	Pimpl* pimpl;
};
#endif
#endif




/** This class lets you take your exported HISE presets and wrap them into a hardcoded plugin (VST / AU, x86/x64, Win / OSX)
*
*	It is connected to a FrontendProcessorEditor, which will display all script interfaces that are brought to the front using 'Synth.addToFront(true)'.
*	It also checks for a licence file to allow minimal protection against the most stupid crackers.
*/
class FrontendProcessor: public PluginParameterAudioProcessor,
						 public AudioProcessorDriver,
						 public MainController
{
public:
	FrontendProcessor(ValueTree &synthData, AudioDeviceManager* manager, AudioProcessorPlayer* callback_, MemoryInputStream *imageData_ = nullptr, MemoryInputStream *impulseData = nullptr, MemoryInputStream* sampleMapData = nullptr, MemoryInputStream* midiData = nullptr, ValueTree *externalScriptData = nullptr, ValueTree *userPresets = nullptr);

	void createPreset(const ValueTree& synthData);

	const String getName(void) const override;

	void changeProgramName(int /*index*/, const String &/*newName*/) override {};

    ~FrontendProcessor();

	bool shouldLoadSamplesAfterSetup() {
		return GET_PROJECT_HANDLER(getMainSynthChain()).areSamplesLoadedCorrectly() && keyFileCorrectlyLoaded;
	}

	void updateUnlockedSuspendStatus()
	{

	}
    
    void restorePool(InputStream* inputStream, FileHandlerBase::SubDirectories directory, const String& fileNameToLook);
    
	void prepareToPlay (double sampleRate, int samplesPerBlock);
	void releaseResources() {};

	void loadSamplesAfterRegistration(bool reloadSamples=true)
    {
#if USE_COPY_PROTECTION
        keyFileCorrectlyLoaded = unlocker.isUnlocked();
#else
        keyFileCorrectlyLoaded = true;
#endif
        
		if (reloadSamples)
		{
			GET_PROJECT_HANDLER(getMainSynthChain()).loadSamplesAfterSetup();
		}
    }

	void getStateInformation	(MemoryBlock &destData) override;;
    
    void setStateInformation(const void *data,int sizeInBytes) override;


#if USE_RAW_FRONTEND

	

	ScopedPointer<raw::DataHolderBase> rawDataHolder;

	/** Overwrite this method and return your subclassed data model. */
	raw::DataHolderBase* createPresetRaw();

	raw::DataHolderBase* getRawDataHolder()
	{
		return rawDataHolder.get();
	}

#endif

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	virtual void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
	{
#if !FRONTEND_IS_PLUGIN
		buffer.clear();
		midiMessages.clear();
		allNotesOff();
#else
		handleLatencyWhenBypassed(buffer, midiMessages);
#endif
		
	};

	void incActiveEditors();

	void decActiveEditors();
    
    void updateSuspendState();
    
	void handleControllersForMacroKnobs(const MidiBuffer &midiMessages);
 
	AudioProcessorEditor* createEditor();
	bool hasEditor() const {return true;};

	bool acceptsMidi() const {return true;};
	bool producesMidi() const {return false;};
	
	double getTailLengthSeconds() const {return 0.0;};

	ModulatorSynthChain *getMainSynthChain() override {return synthChain; };

	const ModulatorSynthChain *getMainSynthChain() const override { return synthChain; }

	int getNumPrograms() override
	{
		return 1;// presets.getNumChildren() + 1;
	}

	const String getProgramName(int /*index*/) override
	{
		return "Default";
	}

	int getCurrentProgram() override
	{
		return 0;

		//return currentlyLoadedProgram;
	}

	void setCurrentProgram(int index) override;
	
#if USE_COPY_PROTECTION
	juce::OnlineUnlockStatus* getLicenseUnlocker() override { return unlocker.getUnlockerObject(); };
	Unlocker unlocker;
#endif

    bool deactivatedBecauseOfMemoryLimitation = false;
    
private:

    struct SuspendUpdater: private Timer
    {
        SuspendUpdater(FrontendProcessor& parent_):
          parent(parent_)
        {}
        
        void updateDelayed()
        {
            startTimer(500);
        }
        
        bool suspendState = false;
        
    private:
        
        void timerCallback() override
        {
            parent.updateSuspendState();
            stopTimer();
        }
        
        FrontendProcessor& parent;
    };
    
    
    
    SuspendUpdater updater;
    
	friend class FrontendProcessorEditor;
	friend class DefaultFrontendBar;

	

    bool keyFileCorrectlyLoaded = true;

	int numParameters;

#if FRONTEND_IS_PLUGIN && HI_SUPPORT_MONO_CHANNEL_LAYOUT
	AudioSampleBuffer stereoCopy;
#endif

	
	AudioPlayHead::CurrentPositionInfo lastPosInfo;

	friend class FrontendProcessorEditor;

	ScopedPointer<ModulatorSynthChain> synthChain;

	int currentlyLoadedProgram;
	
	int unlockCounter;

	int numActiveEditors = 0;
   
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrontendProcessor)	

};


class FrontendStandaloneApplication : public JUCEApplication
{
public:
	//==============================================================================
	FrontendStandaloneApplication() {}

	const String getApplicationName() override;
	const String getApplicationVersion() override;
	bool moreThanOneInstanceAllowed() override       { return true; }


	void initialise(const String& /*commandLine*/) override { mainWindow = new MainWindow(getApplicationName()); }
	void shutdown() override 
	{
#if JUCE_WINDOWS
		mainWindow->closeButtonPressed();
#else
		mainWindow = nullptr;
#endif
	}
	void systemRequestedQuit() override { quit(); }

	void anotherInstanceStarted(const String& /*commandLine*/) override {}

	class AudioWrapper : public Component,
						 public Timer
	{
	public:

		void init();

		void timerCallback() override
		{
			stopTimer();
			init();
		}

		AudioWrapper();

        void paint(Graphics& g) override
        {
            g.fillAll(Colours::black);
        }
    
		void requestQuit()
		{
			standaloneProcessor->requestQuit();
		}

		~AudioWrapper();

		void resized()
		{
			if (splashScreen != nullptr)
				splashScreen->setBounds(getLocalBounds());

			if(editor != nullptr)
				editor->setBounds(0, 0, editor->getWidth(), editor->getHeight());
		}

	private:

		ScopedPointer<ImageComponent> splashScreen;

		ScopedPointer<AudioProcessorEditor> editor;
		ScopedPointer<StandaloneProcessor> standaloneProcessor;
        
		#if 0
		ScopedPointer<OpenGLContext> context;
		#endif
        
	};


	class MainWindow : public DocumentWindow
	{
	public:
		MainWindow(String name);

		~MainWindow()
        {
        }

		void closeButtonPressed() override
		{
			auto audioWrapper = dynamic_cast<AudioWrapper*>(getContentComponent());

			audioWrapper->requestQuit();
		}

	private:


		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
	};


private:

	ScopedPointer<MainWindow> mainWindow;
};

struct ScopedAnalyser
{
	ScopedAnalyser(MainController*, Processor*, const AudioSampleBuffer&, int)
	{}

};

} // namespace hise

#endif  // FRONTENDPROCESSOR_H_INCLUDED
