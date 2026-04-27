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

hise::ProcessorMetadata FlexAhdsrEnvelope::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	constexpr int o = (int)EnvelopeModulator::Parameters::numParameters;

	return EnvelopeModulator::createBaseMetadata()
		.withStandardMetadata<FlexAhdsrEnvelope>()
		.withDescription("A more complex AHDSR envelope with draggable curves and multiple playback modes")
		.withComplexDataInterface(ExternalData::DataType::DisplayBuffer)
		.withParameter(Par(o + (int)SpecialParameters::Attack)
			.withId("Attack")
			.withDescription("Controls how long the envelope takes to reach full level after a note-on")
			.withSliderMode(HiSlider::Time, Range(0.0, 30000.0, 0.0).withCentreSkew(2000.0))
			.withDefault(5.0f))
		.withParameter(Par(o + (int)SpecialParameters::Hold)
			.withId("Hold")
			.withDescription("The time the envelope stays at the attack level before decaying")
			.withSliderMode(HiSlider::Time, Range(0.0, 30000.0, 0.0).withCentreSkew(2000.0))
			.withDefault(0.0f))
		.withParameter(Par(o + (int)SpecialParameters::Decay)
			.withId("Decay")
			.withDescription("The time the envelope takes to fall from the attack level to the sustain level")
			.withSliderMode(HiSlider::Time, Range(0.0, 30000.0, 0.0).withCentreSkew(2000.0))
			.withDefault(100.0f))
		.withParameter(Par(o + (int)SpecialParameters::Sustain)
			.withId("Sustain")
			.withDescription("The level maintained while the note is held")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(0.5f))
		.withParameter(Par(o + (int)SpecialParameters::Release)
			.withId("Release")
			.withDescription("The time the envelope takes to fall to zero after note-off")
			.withSliderMode(HiSlider::Time, Range(0.0, 30000.0, 0.0).withCentreSkew(2000.0))
			.withDefault(300.0f))
		.withParameter(Par(o + (int)SpecialParameters::Mode)
			.withId("Mode")
			.withDescription("The envelope playback mode")
			.withValueList({ "Trigger", "Note", "Loop" }, 0)
			.withDefault(1.0f))
		.withParameter(Par(o + (int)SpecialParameters::AttackLevel)
			.withId("AttackLevel")
			.withDescription("The peak level reached at the end of the attack phase")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(1.0f))
		.withParameter(Par(o + (int)SpecialParameters::AttackCurve)
			.withId("AttackCurve")
			.withDescription("Controls the curvature of the attack phase")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(0.5f))
		.withParameter(Par(o + (int)SpecialParameters::DecayCurve)
			.withId("DecayCurve")
			.withDescription("Controls the curvature of the decay phase")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(0.5f))
		.withParameter(Par(o + (int)SpecialParameters::ReleaseCurve)
			.withId("ReleaseCurve")
			.withDescription("Controls the curvature of the release phase")
			.withSliderMode(HiSlider::NormalizedPercentage, Range(0.0, 1.0, 0.0))
			.withDefault(0.5f))
		.withModulation(Mod(InternalChains::AttackTimeChain)
			.withId("AttackTimeModulation")
			.withDescription("Modulates the attack time per voice")
			.withConstrainer<VoiceStartModulatorFactoryType::Constrainer>()
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly))
		.withModulation(Mod(InternalChains::AttackLevelChain)
			.withId("AttackLevelModulation")
			.withDescription("Modulates the attack level per voice")
			.withConstrainer<VoiceStartModulatorFactoryType::Constrainer>()
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly))
		.withModulation(Mod(InternalChains::DecayTimeChain)
			.withId("DecayTimeModulation")
			.withDescription("Modulates the decay time per voice")
			.withConstrainer<VoiceStartModulatorFactoryType::Constrainer>()
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly))
		.withModulation(Mod(InternalChains::SustainLevelChain)
			.withId("SustainLevelModulation")
			.withDescription("Modulates the sustain level per voice")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly))
		.withModulation(Mod(InternalChains::ReleaseTimeChain)
			.withId("ReleaseTimeModulation")
			.withDescription("Modulates the release time per voice")
			.withConstrainer<VoiceStartModulatorFactoryType::Constrainer>()
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly));
}

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
	
updateParameterSlots();

	internalChains.reserve(numInternalChains);

	internalChains += {this, "Attack Time", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Attack Level", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Decay Time", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Sustain Level", ModulatorChain::ModulationType::Normal };
	internalChains += {this, "Release Time", ModulatorChain::ModulationType::VoiceStartOnly };

	internalChains.finalise();

	for (auto& mb : internalChains)
		mb.getChain()->setParentProcessor(this);

	scriptnode::ParameterDataList list;
	obj.createParameters(list);

	auto md = getMetadata();

	for(int i = getParameterOffset(); i < md.parameters.size(); i++)
		parameters[i - getParameterOffset()] = md[i].defaultValue;

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

hise::Processor* FlexAhdsrEnvelope::getChildProcessor(int processorIndex)
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex].getChain();
}

const hise::Processor* FlexAhdsrEnvelope::getChildProcessor(int processorIndex) const
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex].getChain();
}

float FlexAhdsrEnvelope::startVoice(int voiceIndex)
{
	EnvelopeModulator::startVoice(voiceIndex);

	bool restart = true;

	if(isMonophonic)
	{
		voiceIndex = 0;

		restart = shouldRetrigger || getNumPressedKeys() == 1;
	}
	
	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

	std::array<float, (int)InternalChains::numInternalChains> modValues;

	int idx = 0;

	for (auto& mb : internalChains)
	{
		mb.startVoice(voiceIndex);
		modValues[idx++] = mb.getChain()->getConstantVoiceValue(voiceIndex);
	}
		

	if(restart)
	{
		using STATE = flex_ahdsr_base::State;

		constexpr auto TIME = hise::flex_ahdsr_base::ParameterType::Time;
		constexpr auto LEVEL = hise::flex_ahdsr_base::ParameterType::Level;

		auto& s = obj.state.get();

		s.template setModulationValue<STATE::ATTACK, TIME>(modValues[InternalChains::AttackTimeChain]);
		s.template setModulationValue<STATE::ATTACK, LEVEL>(modValues[InternalChains::AttackLevelChain]);
		s.template setModulationValue<STATE::HOLD, LEVEL>(modValues[InternalChains::AttackLevelChain]); 
		s.template setModulationValue<STATE::DECAY, TIME>(modValues[InternalChains::DecayTimeChain]); 
		s.template setModulationValue<STATE::RELEASE, TIME>(modValues[InternalChains::ReleaseTimeChain]);

		if(!internalChains[InternalChains::SustainLevelChain].getChain()->hasTimeModulationMods())
		{
			s.template setModulationValue<STATE::DECAY, LEVEL>(modValues[InternalChains::SustainLevelChain]);
			s.template setModulationValue<STATE::SUSTAIN, LEVEL>(modValues[InternalChains::SustainLevelChain]);
		}

		auto ownerSynth = static_cast<ModulatorSynth*>(getParentProcessor(true));
		auto ownerVoice = static_cast<ModulatorSynthVoice*>(ownerSynth->getVoice(voiceIndex));
		auto e = ownerVoice->getCurrentHiseEvent();

		// reset the smoothers
		obj.reset();
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
	EnvelopeModulator::stopVoice(voiceIndex);

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

void FlexAhdsrEnvelope::handleHiseEvent(const HiseEvent& e)
{
	EnvelopeModulator::handleHiseEvent(e);

	for (auto& mb : internalChains)
		mb.handleHiseEvent(e);
}

void FlexAhdsrEnvelope::calculateBlock(int startSample, int numSamples)
{
	auto voiceIndex = polyManager.getCurrentVoice();

	

	obj.sendBallUpdate = polyManager.getLastStartedVoice() == voiceIndex;

	if(isMonophonic)
		voiceIndex = 0;

	

	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

	auto& schain = internalChains[InternalChains::SustainLevelChain];

	

	if (schain.getChain()->hasTimeModulationMods())
	{
		auto os = static_cast<ModulatorSynth*>(getParentProcessor(true, true));
		auto msv = static_cast<ModulatorSynthVoice*>(os->getVoice(voiceIndex));

		const int pseudoOffset = startSample * HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		const int pseudoSize = numSamples * HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		if (msv->isFirstRenderedVoice())
			schain.calculateMonophonicModulationValues(pseudoOffset, pseudoSize);

		schain.calculateModulationValuesForCurrentVoice(voiceIndex, pseudoOffset, pseudoSize);

		auto sv = schain.getWritePointerForManualExpansion(pseudoOffset);
		
		span<float, 1> data;

		auto& s = obj.state.get();

		for(int i = 0; i < numSamples; i++)
		{
			data[0] = 1.0f;//internalBuffer.getSample(0, startSample + i);
			
			auto mv = sv[i];

			s.template setModulationValue<flex_ahdsr_base::State::DECAY, flex_ahdsr_base::ParameterType::Level>(mv);
			s.template setModulationValue<flex_ahdsr_base::State::SUSTAIN, flex_ahdsr_base::ParameterType::Level>(mv);
			obj.processFrame(data);
			internalBuffer.setSample(0, i + startSample, data[0]);
		}

		return;
	}

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

		auto md = getProcessor()->getMetadata();

		for(int i = FlexAhdsrEnvelope::getParameterOffset(); i < md.parameters.size(); i++)
		{
			auto s = new HiSlider(md[i].id.toString());
			md.setup(*s, getProcessor(), i);

			s->setLookAndFeel(&laf);
			s->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
			s->updateText();
			sliders.add(s);
			addAndMakeVisible(s);
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

void FlexAhdsrEnvelope::setInternalAttribute(int parameterIndex, float newValue)
{
	if(parameterIndex < getParameterOffset())
	{
		EnvelopeModulator::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	parameterIndex -= getParameterOffset();
	parameters[parameterIndex] = newValue;

	PolyHandler::ScopedAllVoiceSetter avs(polyHandler);

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

	for (auto& mb : internalChains)
		mb.prepareToPlay(sampleRate, samplesPerBlock);

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
