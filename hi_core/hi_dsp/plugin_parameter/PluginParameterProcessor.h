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

#ifndef __PLUGINPROCESSOR_H_BF259E1F__
#define __PLUGINPROCESSOR_H_BF259E1F__



#pragma warning (push)
#pragma warning (disable: 4996)


//==============================================================================
/** @class PluginParameterAudioProcessor
	@brief This AudioProcessor subclass removes some annoying virtual methods and allows PluginParameter objects to 
		   communicate with the host.

	The following methods are overwritten:

	- All Parameter methods: getNumParameters(), getParameter(), setParameter(), getParameterName(), getParameterText()
	- All Channel methods: Stereo mode assumed
	- All Program methods: functionality disabled
	
	Also a CPU meter is implemented, which can be read with getCpuUsage()

	See PluginParameterTestAudioProcessor for an example implementation of this abstract class.
*/
class PluginParameterAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    
	/** You have to create and add all PluginParameters here in order to ensure compatibility with most hosts */
	PluginParameterAudioProcessor(const String &name = "Untitled");

	virtual ~PluginParameterAudioProcessor() {};

	

	/** Add the plugin parameter modulator to the list and return the index. */
	virtual int addPluginParameter(PluginParameterModulator *existingModToBeAdded);

	/** Remove the plugin parameter from the list. */
	virtual void removePluginParameter(PluginParameterModulator *modToRemove);


    //==============================================================================
	
    virtual void prepareToPlay (double sampleRate, int samplesPerBlock) = 0;
    virtual void releaseResources() = 0;

    virtual void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages) = 0;



    //==============================================================================
    virtual AudioProcessorEditor* createEditor() = 0;
    virtual bool hasEditor() const = 0;

    //==============================================================================
	const String getName() const {return name;};

	/** @brief returns the parameter index with the supplied name*/
	int getParameterFromName(String parameter_name);

	/// @brief returns the number of PluginParameter objects, that are added in the constructor
    virtual int getNumParameters();

	/// @brief returns the PluginParameter value of the indexed PluginParameter.
    virtual float getParameter (int index);

	/** @brief sets the PluginParameter value.
	
		This method uses the 0.0-1.0 range to ensure compatibility with hosts. 
		A more user friendly but slower function for GUI handling etc. is setParameterConverted()
	*/
    virtual void setParameter (int index, float newValue);


	/// @brief returns the name of the PluginParameter
    virtual const String getParameterName (int index);

	/// @brief returns a converted and labeled string that represents the current value
    virtual const String getParameterText (int index);

	const String getInputChannelName (int channelIndex) const {return channelIndex == 1 ? "Right" : "Left";};
    const String getOutputChannelName (int channelIndex) const {return channelIndex == 1 ? "Right" : "Left";};;
	bool isInputChannelStereoPair (int ) const {return true;};
	bool isOutputChannelStereoPair (int ) const {return true;};

	

    virtual bool acceptsMidi() const = 0;
    virtual bool producesMidi() const = 0;
    virtual bool silenceInProducesSilenceOut() const = 0;
    virtual double getTailLengthSeconds() const = 0;

    //==============================================================================
	int getNumPrograms() {return 1;};
	int getCurrentProgram() {return 0;};
	void setCurrentProgram (int ) {};
	const String getProgramName (int ) {return String::empty;};
	void changeProgramName (int , const String& ) {};

    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

protected:

private:
	CriticalSection lock;

	ScopedPointer<ValueTree> data_to_save;

	String name;

	WeakReference<Processor> parameterSlots[32];

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginParameterAudioProcessor)
};


#pragma warning (pop)


#endif  // __PLUGINPROCESSOR_H_BF259E1F__
