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

FlexAhdsrEnvelope::FlexAhdsrEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m) :
	EnvelopeModulator(mc, id, voiceAmount, m),
	ProcessorWithSingleStaticExternalData(mc, ExternalData::DataType::DisplayBuffer, 1),
	Modulation(m),
	polyHandler(true)  
{
	obj.dragHandler.parent = this;
	obj.initialise(nullptr);

	{
		SimpleRingBuffer::ScopedPropertyCreator spc(getDisplayBuffer(0));
		SimpleReadWriteLock::ScopedWriteLock sl(getDisplayBuffer(0)->getDataLock());
		snex::ExternalData ed(getDisplayBuffer(0), 0);
		obj.setExternalData(ed, 0);
	}

	getDisplayBuffer(0)->setGlobalUIUpdater(mc->getGlobalUIUpdater());
	getDisplayBuffer(0)->setRingBufferSize(1, 9);
	

	parameterNames.add(Identifier("Attack"));
	parameterNames.add(Identifier("Hold"));
	parameterNames.add(Identifier("Decay"));
	parameterNames.add(Identifier("Sustain"));
	parameterNames.add(Identifier("Release"));
	parameterNames.add(Identifier("Mode"));
	parameterNames.add(Identifier("AttackLevel"));
	parameterNames.add(Identifier("AttackCurve"));
	parameterNames.add(Identifier("DecayCurve"));
	parameterNames.add(Identifier("ReleaseCurve"));
	updateParameterSlots();

	scriptnode::ParameterDataList list;
	obj.createParameters(list);

	for(int i = 0; i < list.size(); i++)
	{
		parameters[i] = list[i].info.defaultValue;
	}

	getDisplayBuffer(0)->getUpdater().sendContentChangeMessage(sendNotificationSync, 2);
}

void FlexAhdsrEnvelope::restoreFromValueTree(const ValueTree& v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	auto offset = getParameterOffset();

	loadAttribute(offset + (int)SpecialParameters::Attack, "Attack");
	loadAttribute(offset + (int)SpecialParameters::Hold, "Hold");
	loadAttribute(offset + (int)SpecialParameters::Decay, "Decay");
	loadAttribute(offset + (int)SpecialParameters::Sustain, "Sustain");
	loadAttribute(offset + (int)SpecialParameters::Release, "Release");
	loadAttribute(offset + (int)SpecialParameters::Mode, "Mode");
	loadAttribute(offset + (int)SpecialParameters::AttackLevel, "AttackLevel");
	loadAttribute(offset + (int)SpecialParameters::AttackCurve, "AttackCurve");
	loadAttribute(offset + (int)SpecialParameters::DecayCurve, "DecayCurve");
	loadAttribute(offset + (int)SpecialParameters::ReleaseCurve, "ReleaseCurve");
}

ValueTree FlexAhdsrEnvelope::exportAsValueTree() const
{
	auto v = EnvelopeModulator::exportAsValueTree();

	auto offset = getParameterOffset();

	saveAttribute(offset + (int)SpecialParameters::Attack, "Attack");
	saveAttribute(offset + (int)SpecialParameters::Hold, "Hold");
	saveAttribute(offset + (int)SpecialParameters::Decay, "Decay");
	saveAttribute(offset + (int)SpecialParameters::Sustain, "Sustain");
	saveAttribute(offset + (int)SpecialParameters::Release, "Release");
	saveAttribute(offset + (int)SpecialParameters::Mode, "Mode");
	saveAttribute(offset + (int)SpecialParameters::AttackLevel, "AttackLevel");
	saveAttribute(offset + (int)SpecialParameters::AttackCurve, "AttackCurve");
	saveAttribute(offset + (int)SpecialParameters::DecayCurve, "DecayCurve");
	saveAttribute(offset + (int)SpecialParameters::ReleaseCurve, "ReleaseCurve");

	return v;
}

float FlexAhdsrEnvelope::startVoice(int voiceIndex)
{
	bool restart = true;

	if(isMonophonic)
	{
		voiceIndex = 0;

		restart = shouldRetrigger || getNumPressedKeys() == 1;
	}
	
	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

	if(restart)
	{
		auto ownerSynth = static_cast<ModulatorSynth*>(getParentProcessor(true));
		auto ownerVoice = static_cast<ModulatorSynthVoice*>(ownerSynth->getVoice(voiceIndex));
		auto e = ownerVoice->getCurrentHiseEvent();

		obj.handleHiseEvent(e);
	}

	span<float, 1> data = { 1.0f };
	obj.processFrame(data);
	return data[0];
}


bool FlexAhdsrEnvelope::isPlaying(int voiceIndex) const
{
	if(isMonophonic)
		voiceIndex = 0;

	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);
	return obj.isActive();
};


void FlexAhdsrEnvelope::stopVoice(int voiceIndex)
{
	auto gateOff = true;

	if(isMonophonic)
	{
		voiceIndex = 0;

		gateOff = getNumPressedKeys() == 0;
	}


	if(gateOff)
	{
		PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

		auto ownerSynth = static_cast<ModulatorSynth*>(getParentProcessor(true));
		auto ownerVoice = static_cast<ModulatorSynthVoice*>(ownerSynth->getVoice(voiceIndex));
		auto e = ownerVoice->getCurrentHiseEvent();
		e.setType(HiseEvent::Type::NoteOff);
		obj.handleHiseEvent(e);
	}
}

void FlexAhdsrEnvelope::reset(int voiceIndex)
{
	if(isMonophonic)
	{
		if(getNumPressedKeys() > 0)
			return;

		voiceIndex = 0;
	}
	
	EnvelopeModulator::reset(voiceIndex);

	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);
	obj.reset();
}

void FlexAhdsrEnvelope::calculateBlock(int startSample, int numSamples)
{
	auto voiceIndex = polyManager.getCurrentVoice();

	

	obj.sendBallUpdate = polyManager.getLastStartedVoice() == voiceIndex;

	if(isMonophonic)
		voiceIndex = 0;

	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

	float* ptr[1] = { internalBuffer.getWritePointer(0, startSample) };
	FloatVectorOperations::fill(ptr[0], 1.0f, numSamples);
	ProcessData<1> pd(ptr, numSamples, 1);
	obj.process(pd);
}

#if USE_BACKEND
class FlexAhdsrEnvelopeEditor: public ProcessorEditorBody
{
public:

	FlexAhdsrEnvelopeEditor(ProcessorEditor* parent):
	  ProcessorEditorBody(parent)
	{
		auto mod = dynamic_cast<FlexAhdsrEnvelope*>(getProcessor());
		display.setComplexDataUIBase(mod->getDisplayBuffer(0));
		addAndMakeVisible(display);

		scriptnode::ParameterDataList list;
		mod->obj.createParameters(list);
		int i = 0;

		for(auto l: list)
		{
			auto info = l.info;
			auto id = info.getId();
			auto s = new HiSlider(id);
			s->setup(getProcessor(), i + FlexAhdsrEnvelope::getParameterOffset(), id);
			s->setRange(info.min, info.max, info.interval);

			s->setSkewFactor(info.skew);
			s->setLookAndFeel(&laf);
			s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
			auto vtc = l.getValueToTextConverter();
			s->textFromValueFunction = vtc;
			s->valueFromTextFunction = vtc;
			s->updateText();
			sliders.add(s);
			addAndMakeVisible(s);
			i++;
		}

	}

	void resized() override
	{
		auto b = getLocalBounds().withSizeKeepingCentre(5 * 138, 260);

		display.setBounds(b.removeFromTop(150).reduced(40, 0));

		auto topRow = b.removeFromTop(48);
		auto bottomRow = b.removeFromTop(48);

		for(int i = 0; i < sliders.size() / 2; i++)
		{
			sliders[i]->setBounds(topRow.removeFromLeft(128));
			sliders[i + sliders.size()/2]->setBounds(bottomRow.removeFromLeft(128));
			topRow.removeFromLeft(10);
			bottomRow.removeFromLeft(10);
		}
	}

	int getBodyHeight() const override { return 300; }

	void updateGui() override
	{
		for(auto s: sliders)
			s->updateValue();
	}

	GlobalHiseLookAndFeel laf;
	OwnedArray<HiSlider> sliders;
	flex_ahdsr_base::FlexAhdsrGraph display;
};
#endif

ProcessorEditorBody * FlexAhdsrEnvelope::createEditor(ProcessorEditor* parentEditor)
{
#if USE_BACKEND

	return new FlexAhdsrEnvelopeEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

float FlexAhdsrEnvelope::getDefaultValue(int parameterIndex) const
{
	if(parameterIndex < getParameterOffset())
		return EnvelopeModulator::getDefaultValue(parameterIndex);

	return 1.0f;
}

void FlexAhdsrEnvelope::setInternalAttribute(int parameterIndex, float newValue)
{
	if(parameterIndex < getParameterOffset())
	{
		EnvelopeModulator::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	parameterIndex -= getParameterOffset();
	parameters[parameterIndex] = newValue;

	switch(parameterIndex)
	{
		case 0: obj.template setParameter<0>(newValue); break;
		case 1: obj.template setParameter<1>(newValue); break;
		case 2: obj.template setParameter<2>(newValue); break;
		case 3: obj.template setParameter<3>(newValue); break;
		case 4: obj.template setParameter<4>(newValue); break;
		case 5: obj.template setParameter<5>(newValue); break;
		case 6: obj.template setParameter<6>(newValue); break;
		case 7: obj.template setParameter<7>(newValue); break;
		case 8: obj.template setParameter<8>(newValue); break;
		case 9: obj.template setParameter<9>(newValue); break;
	}
}

float FlexAhdsrEnvelope::getAttribute(int parameterIndex) const
{
	if(parameterIndex < getParameterOffset())
		return EnvelopeModulator::getAttribute(parameterIndex);

	parameterIndex -= getParameterOffset();
	return parameters[parameterIndex];

}

void FlexAhdsrEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	PrepareSpecs ps;
	ps.voiceIndex = &polyHandler;
	ps.sampleRate = sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	ps.blockSize = sampleRate / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	ps.numChannels = 1;

	obj.prepare(ps);
}



FlexAhdsrEnvelope::Panel::Panel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{
	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::black);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white.withAlpha(0.1f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour3, Colours::white.withAlpha(0.05f));
}

juce::Component* FlexAhdsrEnvelope::Panel::createContentComponent(int index)
{
	if (auto b = dynamic_cast<ProcessorWithExternalData*>((getProcessor())))
	{
		auto numObjects = b->getNumDataObjects(ExternalData::DataType::DisplayBuffer);

		index = jmax(index, 0);

		if(isPositiveAndBelow(index, numObjects))
		{
			auto rb = b->getDisplayBuffer(index);
			if(auto prop = rb->getPropertyObject())
			{
				if(prop->getClassIndex() == flex_ahdsr_base::Properties::PropertyIndex)
				{
					auto g = new flex_ahdsr_base::FlexAhdsrGraph();
					g->setComplexDataUIBase(rb);
					g->setColour(RingBufferComponentBase::bgColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
					g->setColour(RingBufferComponentBase::fillColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
					g->setColour(RingBufferComponentBase::lineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));
					g->setColour(HiseColourScheme::ColourIds::ComponentTextColourId, findPanelColour(FloatingTileContent::PanelColourId::textColour));

					if (g->findColour(RingBufferComponentBase::bgColour).isOpaque())
						g->setOpaque(true);

					return g;
				}
			}
		}
	}

	return nullptr;
}

void FlexAhdsrEnvelope::Panel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<ProcessorWithExternalData>(moduleList);
}


} // namespace hise
