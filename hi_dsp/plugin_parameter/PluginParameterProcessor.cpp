
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

namespace hise { using namespace juce;


struct CustomAutomationParameter : public juce::AudioProcessorParameterWithID
{
	using Data = MainController::UserPresetHandler::CustomAutomationData;

	CustomAutomationParameter(Data::Ptr data_) :
		AudioProcessorParameterWithID(data_->id, data_->id),
		data(data_)
	{
		data->syncListeners.addListener(*this, update, false);
	};

	static void update(CustomAutomationParameter& d, var* args)
	{
		auto v = (float)args[1];

		FloatSanitizers::sanitizeFloatNumber(v);


		v = d.data->range.convertTo0to1(v);

		ScopedValueSetter<bool> svs(d.recursive, true);

		d.setValueNotifyingHost(v);
	}

	float getValue() const override
	{
		return data->lastValue;
	}

	void setValue(float newValue)
	{
		if (recursive)
			return;

		newValue = data->range.convertFrom0to1(newValue);

		data->call(newValue, true);
	}

	float getValueForText(const String& text) const override
	{
		return text.getFloatValue();
	}

	float getDefaultValue() const
	{
		return 0.0f;
	}

	bool isMetaParameter() const
	{
		for (auto c : data->connectionList)
		{
			if (dynamic_cast<Data::MetaConnection*>(c) != nullptr)
				return true;
		}

		return false;
	}

	Data::Ptr data;

	bool recursive = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(CustomAutomationParameter);
};


void PluginParameterAudioProcessor::addScriptedParameters()
{
	auto& uph = dynamic_cast<MainController*>(this)->getUserPresetHandler();

	if (uph.isUsingCustomDataModel())
	{
		for (int i = 0; i < uph.getNumCustomAutomationData(); i++)
		{
			if (auto data = uph.getCustomAutomationData(i))
			{
				if (data->allowHost)
				{
					addParameter(new CustomAutomationParameter(data));
				}
			}
		}
	}
	else
	{
		ModulatorSynthChain* synthChain = dynamic_cast<MainController*>(this)->getMainSynthChain();

		jassert(synthChain != nullptr);

		Processor::Iterator<JavascriptMidiProcessor> iter(synthChain);

		while (JavascriptMidiProcessor *sp = iter.getNextProcessor())
		{
			if (sp->isFront())
			{
				ScriptingApi::Content *content = sp->getScriptingContent();

				for (int i = 0; i < content->getNumComponents(); i++)
				{
					ScriptingApi::Content::ScriptComponent *c = content->getComponent(i);

					const bool wantsAutomation = c->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::Properties::isPluginParameter);
					const bool isAutomatable = c->isAutomatable();

					if (wantsAutomation && !isAutomatable)
					{
						// You specified a parameter for a unsupported component type...
						jassertfalse;
					}

					if (wantsAutomation && isAutomatable)
					{
						ScriptedControlAudioParameter *newParameter = new ScriptedControlAudioParameter(content->getComponent(i), this, sp, i);
						addParameter(newParameter);
					}
				}
			}
		}
	}
}


void PluginParameterAudioProcessor::setNonRealtime(bool isNonRealtime) noexcept
{
	dynamic_cast<MainController*>(this)->getSampleManager().setNonRealtime(isNonRealtime);

	AudioProcessor::setNonRealtime(isNonRealtime);
}

PluginParameterAudioProcessor::PluginParameterAudioProcessor(const String& name_):
	AudioProcessor(getHiseBusProperties()),
	name(name_)
{
		
}

AudioProcessor::BusesProperties PluginParameterAudioProcessor::getHiseBusProperties() const
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

		constexpr int numChannels = HISE_NUM_FX_PLUGIN_CHANNELS;

		auto busProp = BusesProperties();

		for (int i = 0; i < numChannels; i += 2)
			busProp = busProp.withInput("Input " + String(i+1), AudioChannelSet::stereo()).withOutput("Output " + String(i+1), AudioChannelSet::stereo());

		return busProp;
		
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

bool PluginParameterAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
	auto inputs = layouts.getMainInputChannels();
	auto outputs = layouts.getMainOutputChannels();

#if HISE_MIDIFX_PLUGIN
		return inputs == 0 && outputs == 0;
#endif

#if FRONTEND_IS_PLUGIN
#if HI_SUPPORT_MONO_CHANNEL_LAYOUT
#if HI_SUPPORT_MONO_TO_STEREO
		if (outputs == 1) return false; // only mono to stereo support
		return (outputs == 2) && (inputs == 1 || inputs == 2);
#else
		return (inputs == 1 && outputs == 1) ||
			   (inputs == 2 && outputs == 2);
#endif
#else
		return inputs == 2 && outputs == 2;
#endif
#else
	bool isStereo = (inputs == 2 || inputs == 0) && outputs == 2;
	bool isMultiChannel = (inputs == HISE_NUM_PLUGIN_CHANNELS || inputs == 0) && (outputs == HISE_NUM_PLUGIN_CHANNELS);
	return isStereo || isMultiChannel;
#endif
}

PluginParameterAudioProcessor::~PluginParameterAudioProcessor()
{}

const String PluginParameterAudioProcessor::getName() const
{return name;}

const String PluginParameterAudioProcessor::getInputChannelName(int channelIndex) const
{return channelIndex == 1 ? "Right" : "Left";}

const String PluginParameterAudioProcessor::getOutputChannelName(int channelIndex) const
{return channelIndex == 1 ? "Right" : "Left";}

bool PluginParameterAudioProcessor::isInputChannelStereoPair(int i) const
{return true;}

bool PluginParameterAudioProcessor::isOutputChannelStereoPair(int i) const
{return true;}

int PluginParameterAudioProcessor::getNumPrograms()
{return 1;}

int PluginParameterAudioProcessor::getCurrentProgram()
{return 0;}

void PluginParameterAudioProcessor::setCurrentProgram(int)
{}

const String PluginParameterAudioProcessor::getProgramName(int)
{return "Default";}

void PluginParameterAudioProcessor::changeProgramName(int, const String&)
{}

void PluginParameterAudioProcessor::handleLatencyInPrepareToPlay(double samplerate)
{
	if (getLatencySamples() != lastLatencySamples && getLatencySamples() != 0)
	{
		lastLatencySamples = getLatencySamples();
		updateHostDisplay();

		int numChannels = AudioProcessor::getBusesLayout().getMainOutputChannels();

		bypassedLatencyDelays.clear();

		for (int i = 0; i < numChannels; i++)
		{
			bypassedLatencyDelays.add(new DelayLine<32768>());
			bypassedLatencyDelays.getLast()->prepareToPlay(samplerate);
			bypassedLatencyDelays.getLast()->setFadeTimeSamples(0);
			bypassedLatencyDelays.getLast()->setDelayTimeSamples(lastLatencySamples);
		}
	}
}

void PluginParameterAudioProcessor::handleLatencyWhenBypassed(AudioSampleBuffer& buffer, MidiBuffer&)
{
	if (lastLatencySamples != 0)
	{
		jassert(buffer.getNumChannels() <= bypassedLatencyDelays.size());

		auto numChannels = jmin(buffer.getNumChannels(), bypassedLatencyDelays.size());

		for (int i = 0; i < numChannels; i++)
			bypassedLatencyDelays[i]->processBlock(buffer.getWritePointer(i, 0), buffer.getNumSamples());
	}
}

void PluginParameterAudioProcessor::setScriptedPluginParameter(Identifier id, float newValue)
{
	for (int i = 0; i < getNumParameters(); i++)
	{
		if (ScriptedControlAudioParameter * sp = static_cast<ScriptedControlAudioParameter*>(getParameters().getUnchecked(i)))
		{
			if (sp->getId() == id)
			{
				sp->setParameterNotifyingHost(i, newValue);

			}
		}
	}
}

} // namespace hise
