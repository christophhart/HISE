
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





struct CustomAutomationParameter : public juce::AudioProcessorParameterWithID,
								   public HisePluginParameterBase
{
	using Data = MainController::UserPresetHandler::CustomAutomationData;

	CustomAutomationParameter(Data::Ptr data_, int index) :
		AudioProcessorParameterWithID(data_->id, data_->id),
		HisePluginParameterBase(data_->getMainController(), index),
	    
		data(data_)
	{

		parameterValueToSend = data->range.convertTo0to1(data->lastValue);

		data->dispatcher.addValueListener(&autoListener, false, dispatch::DispatchType::sendNotificationSync);
	};

    ~CustomAutomationParameter() = default;
    
    void cleanup() override
    {
        if(data != nullptr)
        {
            data->dispatcher.removeValueListener(&autoListener, dispatch::DispatchType::sendNotificationSync);
            
            data = nullptr;
        }

		HisePluginParameterBase::cleanup();
    }

	HisePluginParameterBase::Type getType() const override { return Type::CustomAutomation; }
	NormalisableRange<float> getNormalisableRange() const override { return data->range; }
	int getSlotIndex() const override { return data->index; }
	String getHisePluginParameterName() const override { return getName(10000); }
	float getHisePluginParameterNormalisedValue() const override { return getValue(); }
	String getHisePluginParameterGroupName() const override { return data->groupName; }

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
		return data->range.convertTo0to1(data->lastValue);
	}

	float getDefaultValue() const override
	{
		return data->range.convertTo0to1(data->defaultParameterValue);
	}

	void setValue(float newValue)
	{
		if (recursive)
			return;

		newValue = data->range.convertFrom0to1(newValue);
		data->call(newValue, dispatch::DispatchType::sendNotificationSync);
	}

	ValueToTextConverter getValueToTextConverter() const override { return data->vtc; }

	String getText(float normalisedValue, int) const override
	{
		auto normValue = data->range.convertFrom0to1(normalisedValue);

		return data->vtc.getTextForValue((double)normValue);
	}

	float getValueForText(const String& text) const override
	{
		return data->vtc.getValueForText(text);
	}

	bool isMetaParameter() const
	{
		for (auto c : data->connectionList)
		{
			if (auto mc = dynamic_cast<Data::MetaConnection*>(c))
			{
				if(mc->target->allowHost)
					return true;
			}
		}

		return false;
	}

	Data::Ptr data;

	JUCE_DECLARE_WEAK_REFERENCEABLE(CustomAutomationParameter);
};


struct MacroPluginParameter: public juce::HostedAudioProcessorParameter,
							 public HisePluginParameterBase
{
	using Data = MainController::UserPresetHandler::CustomAutomationData;

	MacroPluginParameter(MainController* mc, int macroIndex_, int parameterIndex_):
	   HisePluginParameterBase(mc, parameterIndex_),
	   parameterListener(mc->getRootDispatcher(), *this, BIND_MEMBER_FUNCTION_2(MacroPluginParameter::onParameterUpdate)),
	   macroIndex(macroIndex_)
	{
		checkMacro();
		parameterValueToSend = getValue();
	}

	~MacroPluginParameter()
	{
		
	}
    
    void cleanup() override
    {
        if(data != nullptr)
        {
            data->dispatcher.removeValueListener(&autoListener, dispatch::DispatchType::sendNotificationSync);
            
            data = nullptr;
        }

        if(connectedProcessor.get() != nullptr)
        {
            connectedProcessor->removeAttributeListener(&parameterListener);
        }

		HisePluginParameterBase::cleanup();
    }

	String getHisePluginParameterGroupName() const override { return ""; }

	ValueToTextConverter getValueToTextConverter() const override
	{
		checkMacro();

		if(md->getNumParameters() == 1)
		{
			auto mp = md->getParameter(0);
			return mp->getValueToTextConverter();
		}

		return {};
	}

	void onParameterUpdate(dispatch::library::Processor* funky, uint16 x)
	{
		if(connectedProcessor != nullptr && connectedAttribute != -1)
		{
			auto v = connectedProcessor->getAttribute(connectedAttribute);
			onUpdate(-1, v);
		}
	}

	void checkMacro() const
	{
		if(md.get() == nullptr)
		{
			md = const_cast<MacroControlBroadcaster::MacroControlData*>(getMainController()->getMainSynthChain()->getMacroControlData(macroIndex));
		}

		if(md != nullptr)
		{
			auto setDataListener = [&](Data::Ptr thisData)
			{
				auto l = const_cast<dispatch::library::CustomAutomationSource::Listener*>(&autoListener);

				if(thisData != data)
				{
					if(data != nullptr)
						data->dispatcher.removeValueListener(l, dispatch::DispatchType::sendNotificationSync);

					data = thisData;

					if(data != nullptr)
						data->dispatcher.addValueListener(l, false, dispatch::DispatchType::sendNotificationSync);
				}
			};

			auto setParameterListener = [&](Processor* p, uint16 processorIndex)
			{
				auto l = const_cast<dispatch::library::Processor::AttributeListener*>(&this->parameterListener);

				if(connectedProcessor != p || connectedAttribute != processorIndex)
				{
					if(connectedProcessor != nullptr)
						connectedProcessor->removeAttributeListener(l);

					connectedProcessor = p;
					connectedAttribute = processorIndex;

					if(connectedProcessor != nullptr)
						connectedProcessor->addAttributeListener(l, &processorIndex, 1, dispatch::sendNotificationSync);
				}
			};

			if(md->getNumParameters() > 0)
			{
				if(md->getParameter(0)->isCustomAutomation())
				{
					setParameterListener(nullptr, -1);
					auto idx = md->getParameter(0)->getParameter();
					setDataListener(getMainController()->getUserPresetHandler().getCustomAutomationData(idx));
				}
				else
				{
					auto mp = md->getParameter(0);
					setParameterListener(mp->getProcessor(), mp->getParameter());
					setDataListener(nullptr);
				}
			}
			else
			{
				setParameterListener(nullptr, -1);
				setDataListener(nullptr);
			}
		}
	}

	NormalisableRange<float> getNormalisableRange() const override
	{
		if(data != nullptr)
			return data->range;

		if(md != nullptr && md->getNumParameters() == 1)
		{
			auto nrd = md->getParameter(0)->getParameterRange();
			NormalisableRange<float> nr;

			nr.start = (float)nrd.start;
			nr.end = (float)nrd.end;
			nr.interval = (float)nrd.interval;
			nr.skew = (float)nrd.skew;
			nr.symmetricSkew = (float)nrd.symmetricSkew;

			return nr;
		}
			

		return { 0.0f, 1.0f };
	}

		
	HisePluginParameterBase::Type getType() const override { return Type::Macro; }
	int getSlotIndex() const override { return macroIndex; }

	mutable Data::Ptr data;
	mutable WeakReference<Processor> connectedProcessor;
	mutable int connectedAttribute = -1;
	
	String getParameterID() const override { return "P" + String(macroIndex + 1); }

	String getHisePluginParameterName() const override { return getName(10000); }
	float getHisePluginParameterNormalisedValue() const override { return getValue(); }

	String getName(int maximumStringLength) const override
	{
		checkMacro();

		auto n = md->getMacroName();

		if(md->getNumParameters() == 1)
		{
			return md->getParameter(0)->getParameterName();
		}

		if (isPositiveAndBelow(n.length(), maximumStringLength))
			return n;

		return n.substring(0, maximumStringLength);
	}

	String getLabel() const override { return {}; }

	

	String getText(float normalisedValue, int /*maximumStringLength*/) const override
	{
		checkMacro();

		if(md->getNumParameters() == 1)
		{
			auto mp = md->getParameter(0);

			auto vtc = mp->getValueToTextConverter();

			normalisedValue = md->getParameter(0)->getParameterRange().convertFrom0to1(normalisedValue);

			if(vtc.active)
				return vtc.getTextForValue(normalisedValue);
		}

		return String(normalisedValue, 2);
	}

	
	float getValueForText(const String& text) const override
	{
		checkMacro();

		auto fValue = text.getFloatValue();

		if(md->getNumParameters() == 1)
		{
			fValue = md->getParameter(0)->getParameterRange().convertTo0to1(fValue);
		}

		return fValue;
	}

	float getValue() const override
	{
		checkMacro();

		return md->getCurrentValue() / 127.0;
	}

	float getDefaultValue() const override
	{
		checkMacro();

		if (md->getNumParameters() == 1)
		{
			return md->getParameter(0)->getProcessor()->getDefaultValue(md->getParameter(0)->getParameter());
		}

		return 0.0f;
	}

	void setValue(float newValue) override
	{
		checkMacro();

		if (recursive)
			return;

		ScopedValueSetter<bool> svs(recursive, true);
		md->setValue(newValue * 127.0);
	}

	

	bool isMetaParameter() const { return false; }

	dispatch::library::Processor::AttributeListener parameterListener;

	const int macroIndex;
	mutable WeakReference<MacroControlBroadcaster::MacroControlData> md;
};

void PluginParameterAudioProcessor::addScriptedParameters()
{
	juce::AudioProcessorParameterGroup parameters;
	createScriptedParameters(parameters);
	setParameterTree(std::move(parameters));
}

template <typename T> static void addPluginParameterToGroup(juce::AudioProcessorParameterGroup& parameters, std::unique_ptr<T>&& np)
{
	auto hp = dynamic_cast<HisePluginParameterBase*>(np.get());

	auto groupName = hp->getHisePluginParameterGroupName();

	juce::AudioProcessorParameterGroup* groupToAddTo = &parameters;

	if(!groupName.isEmpty())
	{
		auto list = parameters.getSubgroups(false);
		bool found = false;

		for(auto& l: list)
		{
			if(l->getName() == groupName)
			{
				groupToAddTo = const_cast<juce::AudioProcessorParameterGroup*>(l);
				found = true;
				break;
			}
		}

		if(!found)
		{
			parameters.addChild(std::make_unique<juce::AudioProcessorParameterGroup>(groupName, groupName, ""));
			groupToAddTo = const_cast<juce::AudioProcessorParameterGroup*>(parameters.getSubgroups(false).getLast());
		}
	}

	groupToAddTo->addChild(std::move(np));
}

void PluginParameterAudioProcessor::createScriptedParameters(juce::AudioProcessorParameterGroup& parameters)
{
	OwnedArray<HisePluginParameterBase> flatList;

	auto mc = dynamic_cast<MainController*>(this);

	auto useMacrosAsParameter = HISE_GET_PREPROCESSOR(mc, HISE_MACROS_ARE_PLUGIN_PARAMETERS);
	auto numMacros = HISE_GET_PREPROCESSOR(mc, HISE_NUM_MACROS);
    ignoreUnused(mc);

	auto& uph = dynamic_cast<MainController*>(this)->getUserPresetHandler();
	int pIndex = 0;

	if(useMacrosAsParameter)
	{
		for(int i = 0; i < numMacros; i++)
			flatList.add(new MacroPluginParameter(dynamic_cast<MainController*>(this), i, pIndex++));
	}

	if (uph.isUsingCustomDataModel())
	{
		for (int i = 0; i < uph.getNumCustomAutomationData(); i++)
		{
			if (auto data = uph.getCustomAutomationData(i))
			{
				if (data->allowHost)
					flatList.add(new CustomAutomationParameter(data, pIndex++));
			}
		}
	}

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
					flatList.add(new ScriptedControlAudioParameter(content->getComponent(i), this, sp, pIndex++, i));
			}

			break;
		}
	}

#if 0
	else
	{
		auto& uph = dynamic_cast<MainController*>(this)->getUserPresetHandler();
		int pIndex = 0;

		

		
	}
#endif

	if(pluginParameterSortFunction)
	{
		struct Sorter
		{
			Sorter(PluginParameterAudioProcessor& p):
			  parent(p)
			{}

			PluginParameterAudioProcessor& parent;
			
			int compareElements(HisePluginParameterBase* p1, HisePluginParameterBase* p2) const
			{
				return parent.pluginParameterSortFunction(p1, p2);
			}
		};

		Sorter s(*this);
		flatList.sort(s);

		

		for(int i = 0; i < flatList.size(); i++)
			flatList[i]->parameterIndex = i;
	}

	while(!flatList.isEmpty())
	{
		std::unique_ptr<juce::AudioProcessorParameter> ptr;
		ptr.reset(dynamic_cast<AudioProcessorParameter*>(flatList.removeAndReturn(0)));
		addPluginParameterToGroup(parameters, std::move(ptr));
	}



	for(auto pp: parameterPostProcessors)
	{
		

		if(pp.get() != nullptr)
			pp->processPluginParameterTree(parameters);

		auto fl = parameters.getParameters(true);

		for(int i = 0; i < fl.size(); i++)
			dynamic_cast<HisePluginParameterBase*>(fl[i])->parameterIndex = i;
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
	pluginParameterSortFunction = HisePluginParameterBase::defaultSort;
}

AudioProcessor::BusesProperties PluginParameterAudioProcessor::getHiseBusProperties() const
{
#if HISE_MIDIFX_PLUGIN
		return BusesProperties();
#endif

#if FRONTEND_IS_PLUGIN
#if HI_SUPPORT_MONO_CHANNEL_LAYOUT

		auto m2s = BusesProperties().withInput("Input", AudioChannelSet::mono()).withOutput("Output", AudioChannelSet::stereo());
		auto s2s = BusesProperties().withInput("Input", AudioChannelSet::stereo()).withOutput("Output", AudioChannelSet::stereo());	

#if HI_SUPPORT_MONO_TO_STEREO
		// FL Studio is at it again...
		if(!PluginHostType().isFruityLoops())
			return m2s;
		else
			return s2s;
#else
		return s2s;
#endif
#else

		constexpr int numChannels = HISE_NUM_FX_PLUGIN_CHANNELS;

		auto busProp = BusesProperties();

#ifdef HISE_SIDECHAIN_CHANNEL_LAYOUT
        busProp = busProp.withInput("Input", AudioChannelSet::stereo())
            .withInput("Sidechain", AudioChannelSet::stereo())
            .withOutput("Output", AudioChannelSet::stereo());
#else
		for (int i = 0; i < numChannels; i += 2)
			busProp = busProp.withInput("Input " + String(i+1), AudioChannelSet::stereo()).withOutput("Output " + String(i+1), AudioChannelSet::stereo());
#endif

		return busProp;
		
#endif
#else
	auto busProp = BusesProperties();

	// Protools is behaving really nasty and hiding the instrument plugin if it hasn't at least one input bus...
	if (getWrapperTypeBeingCreated() == wrapperType_AAX || FORCE_INPUT_CHANNELS)
		busProp = busProp.withInput("Input", AudioChannelSet::stereo());
		
#if IS_STANDALONE_FRONTEND || IS_STANDALONE_APP
    constexpr int numChannels = HISE_NUM_STANDALONE_OUTPUTS;
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

	ignoreUnused(inputs, outputs);

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
    
#if IS_STANDALONE_FRONTEND || IS_STANDALONE_APP
    return outputs == 2 || outputs == HISE_NUM_STANDALONE_OUTPUTS;
#else
	bool isStereo = (inputs == 2 || inputs == 0) && outputs == 2;
	bool isMultiChannel = (inputs == HISE_NUM_PLUGIN_CHANNELS || inputs == 0) && (outputs == HISE_NUM_PLUGIN_CHANNELS);
	return isStereo || isMultiChannel;
#endif
#endif
}

PluginParameterAudioProcessor::~PluginParameterAudioProcessor()
{
	
}

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

#if 0
void PluginParameterAudioProcessor::setScriptedPluginParameter(Identifier id, float newValue)
{
	for (int i = 0; i < getNumParameters(); i++)
	{

#if HISE_MACROS_ARE_PLUGIN_PARAMETERS
#define CAST dynamic_cast
#else
#define CAST static_cast
#endif

		if (ScriptedControlAudioParameter * sp = CAST<ScriptedControlAudioParameter*>(getParameters().getUnchecked(i)))
		{
			if (sp->getId() == id)
			{
				sp->setParameterNotifyingHost(i, newValue);

			}
		}
	}

#undef CAST
}
#endif

} // namespace hise
