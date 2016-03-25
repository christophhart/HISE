/*
  ==============================================================================

    PresetContainerProcessor.h
    Created: 30 Jul 2015 1:18:11pm
    Author:  Christoph

  ==============================================================================
*/

#ifndef PRESETCONTAINERPROCESSOR_H_INCLUDED
#define PRESETCONTAINERPROCESSOR_H_INCLUDED



class PresetContainerProcessor : public PluginParameterAudioProcessor
{
public:

	PresetContainerProcessor()
	{};

	void prepareToPlay(double sampleRate, int samplesPerBlock);

	void releaseResources()
	{
	};

	void processBlock(AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

	virtual void processBlockBypassed(AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
	{
		buffer.clear();
		midiMessages.clear();
	};

	void getStateInformation(MemoryBlock &destData) override;;

	void setStateInformation(const void *data, int sizeInBytes) override;


	AudioProcessorEditor* createEditor();
	bool hasEditor() const { return true; };

	bool acceptsMidi() const { return true; };
	bool producesMidi() const { return false; };
	bool silenceInProducesSilenceOut() const { return false; };
	double getTailLengthSeconds() const { return 0.0; };

	int getNumParameters() override { return 8; }

	float getParameter(int /*index*/) override
	{
		return 1.0f;
		//return synthChain->getMacroControlData(index)->getCurrentValue() / 127.0f;
	}

	void setParameter(int /*index*/, float /*newValue*/) override
	{
		//synthChain->setMacroControl(index, newValue * 127.0f, sendNotification);
	}

	const String getParameterName(int /*index*/) override
	{
		return "";;
	}

	const String getParameterText(int /*index*/) override
	{
		return "";
	}
	void addNewPreset(const File &f, ValueTree *data=nullptr);
	void removePreset(PresetProcessor * pp);
private:

	AudioPlayHead::CurrentPositionInfo lastPosInfo;

	friend class PresetContainerProcessorEditor;

	CustomKeyboardState keyboardState;

	OwnedArray<PresetProcessor> patches;

	StringArray loadedPresets;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetContainerProcessor)
};



#endif  // PRESETCONTAINERPROCESSOR_H_INCLUDED
