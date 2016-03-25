/*
  ==============================================================================

    FrontEndProcessor.h
    Created: 16 Oct 2014 9:33:01pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef FRONTENDPROCESSOR_H_INCLUDED
#define FRONTENDPROCESSOR_H_INCLUDED


/** This class lets you take your exported HISE presets and wrap them into a hardcoded plugin (VST / AU, x86/x64, Win / OSX)
*
*	It is connected to a FrontendProcessorEditor, which will display all script interfaces that are brought to the front using 'Synth.addToFront(true)'.
*	It also checks for a licence file to allow minimal protection against the most stupid crackers.
*/
class FrontendProcessor: public PluginParameterAudioProcessor,
						 public MainController
{
public:
	FrontendProcessor(ValueTree &synthData, ValueTree *imageData_=nullptr, ValueTree *impulseData=nullptr, ValueTree *sampleData=nullptr):
		MainController(),
		synthChain(new ModulatorSynthChain(this, "Master Chain", NUM_POLYPHONIC_VOICES)),
		samplesCorrectlyLoaded(true),
		keyFileCorrectlyLoaded(true)
	{
#if USE_COPY_PROTECTION

		if(PresetHandler::loadKeyFile(unlocker))
		{
			DBG("TUT");
		}
		else
		{
			DBG("FAIL");

			keyFileCorrectlyLoaded = false;

			return;
			
		}

#endif

		loadImages(imageData_);

		if(sampleData != nullptr)
		{
			getSampleManager().setLoadedSampleMaps(*sampleData);

			ModulatorSamplerSoundPool *pool = getSampleManager().getModulatorSamplerSoundPool();
			
			samplesCorrectlyLoaded = pool->loadMonolithicData(*sampleData);

		}

		if(impulseData != nullptr)
		{
			getSampleManager().getAudioSampleBufferPool()->restoreFromValueTree(*impulseData);
		}

		numParameters = 0;

		if(samplesCorrectlyLoaded)
		{
			getMacroManager().setMacroChain(synthChain);

			synthChain->setId(synthData.getProperty("ID", String::empty));

			suspendProcessing(true);

			synthChain->restoreFromValueTree(synthData);

			

			synthChain->compileAllScripts();

			

			for(int i = 0; i < 8; i++)
			{
				if(synthChain->getMacroControlData(i)->getNumParameters() != 0)
				{
					numParameters++;
				}
			}

			if(getSampleRate() > 0)
			{
				synthChain->prepareToPlay(getSampleRate(), getBlockSize());
			}

			suspendProcessing(false);

			

		}
	};

	 inline void checkKey()
	{

#if USE_COPY_PROTECTION

		if(unlocker.isUnlocked().isUndefined())
		{
			suspendProcessing(true);
			keyFileCorrectlyLoaded = false;
		}
		else
		{
			suspendProcessing(false);
			keyFileCorrectlyLoaded = true;
		}

#endif

	}

	~FrontendProcessor()
	{
		synthChain = nullptr;
	};

	void prepareToPlay (double sampleRate, int samplesPerBlock);
	void releaseResources() 
	{
	};

	void getStateInformation	(MemoryBlock &destData) override
	{
		MemoryOutputStream output(destData, false);

		
		ValueTree v("ControlData");
		
		synthChain->saveMacrosToValueTree(v);
		
		synthChain->saveInterfaceValues(v);
		
		v.writeToStream(output);
	};

	void setStateInformation(const void *data,int sizeInBytes) override
	{
		if(samplesCorrectlyLoaded)
		{
			ValueTree v = ValueTree::readFromData(data, sizeInBytes);

			synthChain->loadMacrosFromValueTree(v);

			synthChain->restoreInterfaceValues(v);
		}
	}

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	virtual void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
	{
		buffer.clear();
		midiMessages.clear();
		allNotesOff();
	};

	void handleControllersForMacroKnobs(const MidiBuffer &midiMessages);
 
	AudioProcessorEditor* createEditor();
	bool hasEditor() const {return true;};

	bool acceptsMidi() const {return true;};
	bool producesMidi() const {return false;};
	bool silenceInProducesSilenceOut() const {return false;};
	double getTailLengthSeconds() const {return 0.0;};

	ModulatorSynthChain *getMainSynthChain() override {return synthChain; };

	const ModulatorSynthChain *getMainSynthChain() const override { return synthChain; }

	/// @brief returns the number of PluginParameter objects, that are added in the constructor
    int getNumParameters() override
	{
		return numParameters;
	}

	/// @brief returns the PluginParameter value of the indexed PluginParameter.
    float getParameter (int index) override
	{
		return synthChain->getMacroControlData(index)->getCurrentValue() / 127.0f;
	}

	/** @brief sets the PluginParameter value.
	
		This method uses the 0.0-1.0 range to ensure compatibility with hosts. 
		A more user friendly but slower function for GUI handling etc. is setParameterConverted()
	*/
    void setParameter (int index, float newValue) override
	{

		synthChain->setMacroControl(index, newValue * 127.0f, sendNotificationAsync);
	}


	/// @brief returns the name of the PluginParameter
    const String getParameterName (int index) override
	{
		
		return synthChain->getMacroControlData(index)->getMacroName();
	}

	/// @brief returns a converted and labeled string that represents the current value
    const String getParameterText (int index) override
	{
		
		return String(synthChain->getMacroControlData(index)->getDisplayValue(), 1);
	}


	

private:

	void loadImages(ValueTree *imageData)
	{
		if(imageData == nullptr) return;

		getSampleManager().getImagePool()->restoreFromValueTree(*imageData);
	}

	friend class FrontendProcessorEditor;
	friend class FrontendBar;

	bool samplesCorrectlyLoaded;

	bool keyFileCorrectlyLoaded;

	int numParameters;

	Unlocker unlocker;

	AudioPlayHead::CurrentPositionInfo lastPosInfo;

	friend class FrontendProcessorEditor;

	ScopedPointer<ModulatorSynthChain> synthChain;

	ScopedPointer<AudioSampleBufferPool> audioSampleBufferPool;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrontendProcessor)
};



#endif  // FRONTENDPROCESSOR_H_INCLUDED
