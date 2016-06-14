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

#ifndef FRONTENDPROCESSOR_H_INCLUDED
#define FRONTENDPROCESSOR_H_INCLUDED


/** This class lets you take your exported HISE presets and wrap them into a hardcoded plugin (VST / AU, x86/x64, Win / OSX)
*
*	It is connected to a FrontendProcessorEditor, which will display all script interfaces that are brought to the front using 'Synth.addToFront(true)'.
*	It also checks for a licence file to allow minimal protection against the most stupid crackers.
*/
class FrontendProcessor: public AudioProcessorWithScriptedParameters,
						 public MainController
{
public:
	FrontendProcessor(ValueTree &synthData, ValueTree *imageData_=nullptr, ValueTree *impulseData=nullptr, ValueTree *externalScriptData=nullptr, ValueTree *userPresets=nullptr);

	const String getName(void) const override
	{
		return JucePlugin_Name;
	}

	void changeProgramName(int index, const String &newName) override {};

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
		
		synthChain->saveMacroValuesToValueTree(v);
		
		v.addChild(getMacroManager().getMidiControlAutomationHandler()->exportAsValueTree(), -1, nullptr);

		synthChain->saveInterfaceValues(v);
		
		v.setProperty("Program", currentlyLoadedProgram, nullptr);

		v.writeToStream(output);
	};


	void setStateInformation(const void *data,int sizeInBytes) override
	{
		if(samplesCorrectlyLoaded)
		{
			ValueTree v = ValueTree::readFromData(data, sizeInBytes);

			currentlyLoadedProgram = v.getProperty("Program");

			getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(v.getChildWithName("MidiAutomation"));

			synthChain->loadMacroValuesFromValueTree(v);

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

	int getNumPrograms() override
	{
		return presets.getNumChildren() + 1;
	}

	const String getProgramName(int index) override
	{
		if (index == 0) return "Default";

		return presets.getChild(index - 1).getProperty("FileName");
	}

	int getCurrentProgram() override
	{
		return currentlyLoadedProgram;
	}

	void setCurrentProgram(int index) override;
	
    const ValueTree &getPresetData() const { return presets; };
    
#if USE_COPY_PROTECTION
	Unlocker unlocker;
#endif

private:

	void loadImages(ValueTree *imageData)
	{
		if(imageData == nullptr) return;

		getSampleManager().getImagePool()->restoreFromValueTree(*imageData);
	}
	void addScriptedParameters();

	friend class FrontendProcessorEditor;
	friend class FrontendBar;

	bool samplesCorrectlyLoaded;

	bool keyFileCorrectlyLoaded;

	int numParameters;

	const ValueTree presets;

	AudioPlayHead::CurrentPositionInfo lastPosInfo;

	friend class FrontendProcessorEditor;

	ScopedPointer<ModulatorSynthChain> synthChain;

	ScopedPointer<AudioSampleBufferPool> audioSampleBufferPool;

	int currentlyLoadedProgram;
	
	int unlockCounter;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrontendProcessor)
};



#endif  // FRONTENDPROCESSOR_H_INCLUDED
