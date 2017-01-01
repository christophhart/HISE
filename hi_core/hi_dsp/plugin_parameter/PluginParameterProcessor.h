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
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
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
	PluginParameterAudioProcessor(const String &name_ = "Untitled"):
		AudioProcessor(BusesProperties().withOutput("Output", AudioChannelSet::stereo())), 
		name(name_)
	{

	}

	virtual ~PluginParameterAudioProcessor() {};

	
    //==============================================================================
	const String getName() const {return name;};

	const String getInputChannelName (int channelIndex) const {return channelIndex == 1 ? "Right" : "Left";};
    const String getOutputChannelName (int channelIndex) const {return channelIndex == 1 ? "Right" : "Left";};;
	bool isInputChannelStereoPair (int ) const {return true;};
	bool isOutputChannelStereoPair (int ) const {return true;};


    //==============================================================================
	int getNumPrograms() {return 1;};
	int getCurrentProgram() {return 0;};
	void setCurrentProgram (int ) {};
	const String getProgramName (int ) {return "Default";};
	void changeProgramName (int , const String& ) {};
    
	String name;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginParameterAudioProcessor)
};


#pragma warning (pop)


#endif  // __PLUGINPROCESSOR_H_BF259E1F__
