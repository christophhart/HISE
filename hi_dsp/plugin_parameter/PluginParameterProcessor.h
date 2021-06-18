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
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef __PLUGINPROCESSOR_H_BF259E1F__
#define __PLUGINPROCESSOR_H_BF259E1F__

namespace hise { using namespace juce;

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
		AudioProcessor(getHiseBusProperties()),
		name(name_)
	{
		
	}

#if HISE_MIDIFX_PLUGIN
	bool isMidiEffect() const override
	{
		return true;
	}
#endif

	BusesProperties getHiseBusProperties() const
	{
#if HISE_MIDIFX_PLUGIN
		return BusesProperties();
#endif

#if FRONTEND_IS_PLUGIN
#if HI_SUPPORT_MONO_CHANNEL_LAYOUT

#if HI_SUPPORT_MONO_TO_STEREO
		return BusesProperties().withInput("Input", AudioChannelSet::mono()).withOutput("Output", AudioChannelSet::stereo());
#else
		return BusesProperties().withInput("Input", AudioChannelSet::stereo()).withOutput("Output", AudioChannelSet::stereo());
#endif
#else
		return BusesProperties().withInput("Input", AudioChannelSet::stereo()).withOutput("Output", AudioChannelSet::stereo());
#endif
#else
auto busProp = BusesProperties();

		// Protools is behaving really nasty and hiding the instrument plugin if it hasn't at least one input bus...
		if (getWrapperTypeBeingCreated() == wrapperType_AAX || FORCE_INPUT_CHANNELS)
			busProp = busProp.withInput("Input", AudioChannelSet::stereo());
		
#if IS_STANDALONE_FRONTEND
		constexpr int numChannels = 2;
#else
		constexpr int numChannels = HISE_NUM_PLUGIN_CHANNELS;
#endif

		for (int i = 0; i < numChannels; i += 2)
			busProp = busProp.withOutput("Channel " + String(i + 1) + "+" + String(i + 2), AudioChannelSet::stereo());

		return busProp;

#endif
	}
    
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override
    {
#if HISE_MIDIFX_PLUGIN
		return layouts.getMainInputChannels() == 0 && layouts.getMainOutputChannels() == 0;
#endif

#if FRONTEND_IS_PLUGIN
#if HI_SUPPORT_MONO_CHANNEL_LAYOUT
#if HI_SUPPORT_MONO_TO_STEREO
		return (layouts.getMainInputChannels() == 1 && layouts.getMainOutputChannels() == 2);
#else

		return (layouts.getMainInputChannels() == 1 && layouts.getMainOutputChannels() == 1) ||
			   (layouts.getMainInputChannels() == 2 && layouts.getMainOutputChannels() == 2);
#endif
#else
		return (layouts.getMainInputChannels() == 2 && layouts.getMainOutputChannels() == 2);
#endif
#else
        bool isStereo = (layouts.getMainInputChannels() == 2) && (layouts.getMainOutputChannels() == 2);
        bool isMultiChannel = (layouts.getMainInputChannels() == HISE_NUM_PLUGIN_CHANNELS) && (layouts.getMainOutputChannels() == HISE_NUM_PLUGIN_CHANNELS);
        
        return isStereo || isMultiChannel;
#endif
    }
    
	virtual ~PluginParameterAudioProcessor() {};

	void handleLatencyInPrepareToPlay(double samplerate);

	void handleLatencyWhenBypassed(AudioSampleBuffer& buffer, MidiBuffer& );

	void setScriptedPluginParameter(Identifier id, float newValue);
	void addScriptedParameters();

    //==============================================================================
	const String getName() const {return name;};

	const String getInputChannelName (int channelIndex) const {return channelIndex == 1 ? "Right" : "Left";};
    const String getOutputChannelName (int channelIndex) const {return channelIndex == 1 ? "Right" : "Left";};
	bool isInputChannelStereoPair (int ) const {return true;};
	bool isOutputChannelStereoPair (int ) const {return true;};

	void setNonRealtime(bool isNonRealtime) noexcept override;


    //==============================================================================
	int getNumPrograms() {return 1;};
	int getCurrentProgram() {return 0;};
	void setCurrentProgram (int ) {};
	const String getProgramName (int ) {return "Default";};
	void changeProgramName (int , const String& ) {};
    
	String name;

	OwnedArray<DelayLine<32768>> bypassedLatencyDelays;
	int lastLatencySamples = 0;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginParameterAudioProcessor)
};


#pragma warning (pop)

} // namespace hise

#endif  // __PLUGINPROCESSOR_H_BF259E1F__
