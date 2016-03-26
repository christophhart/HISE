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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
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
