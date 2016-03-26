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
