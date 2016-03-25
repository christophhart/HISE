/*
  ==============================================================================

    PluginPlayerProcessor.h
    Created: 24 Oct 2014 2:14:35pm
    Author:  Chrisboy

  ==============================================================================
*/

#ifndef PLUGINPLAYERPROCESSOR_H_INCLUDED
#define PLUGINPLAYERPROCESSOR_H_INCLUDED


/** A PluginPlayerProcessor dynamically loads the data that will be embedded into a compiled plugin.
*
*	In order to do this, simply drag a folder onto the editor's window with contains the following files:
*
*		- preset (no extension): the preset file
*		- interface (no extension): the interface file
*		- samplemaps (no extension) the info about the samplemaps
*		- sampledata (folder): the data with all samples
*
*	For more flexibility, it will have always 8 parameters connected to the first 8 macros.
*/
class PresetProcessor: public PluginParameterAudioProcessor,
							 public MainController,
							 public RestorableObject
{
public:
	PresetProcessor(const File &presetData);;

	~PresetProcessor();;

	
	void prepareToPlay (double sampleRate, int samplesPerBlock);
	
	void releaseResources() 
	{
	};

	ValueTree exportAsValueTree() const override;


	void restoreFromValueTree(const ValueTree &v) override;

	void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	virtual void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);;

	void handleControllersForMacroKnobs(const MidiBuffer &midiMessages);
 
	AudioProcessorEditor* createEditor();
	bool hasEditor() const {return true;};

	bool acceptsMidi() const {return true;};
	bool producesMidi() const {return false;};
	bool silenceInProducesSilenceOut() const {return false;};
	double getTailLengthSeconds() const {return 0.0;};

	ModulatorSynthChain *getMainSynthChain() override {return synthChain; };

	const ModulatorSynthChain *getMainSynthChain() const override { return synthChain; }

    int getNumParameters() override { return 8; }

    float getParameter (int /*index*/) override
	{
		return 1.0f;
		//return synthChain->getMacroControlData(index)->getCurrentValue() / 127.0f;
	}

    void setParameter (int /*index*/, float /*newValue*/) override
	{
		//synthChain->setMacroControl(index, newValue * 127.0f, sendNotification);
	}

    const String getParameterName (int index) override
	{	
		return synthChain->getMacroControlData(index)->getMacroName();
	}

    const String getParameterText (int index) override
	{
		return String(synthChain->getMacroControlData(index)->getDisplayValue(), 1);
	}

private:

	int numParameters;

	ValueTree interfaceData;

	AudioPlayHead::CurrentPositionInfo lastPosInfo;

	friend class PresetProcessorEditor;

	ScopedPointer<ModulatorSynthChain> synthChain;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetProcessor)
};




#endif  // PLUGINPLAYERPROCESSOR_H_INCLUDED
