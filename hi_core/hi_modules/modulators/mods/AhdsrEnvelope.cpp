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

Processor * AhdsrEnvelope::getChildProcessor(int processorIndex)
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex].getChain();
}

const Processor * AhdsrEnvelope::getChildProcessor(int processorIndex) const
{
	jassert(processorIndex < internalChains.size());
	return internalChains[processorIndex].getChain();
}

AhdsrEnvelope::AhdsrEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m) :
	EnvelopeModulator(mc, id, voiceAmount, m),
	Modulation(m)
{
	parameterNames.add("Attack");
	parameterNames.add("AttackLevel");
	parameterNames.add("Hold");
	parameterNames.add("Decay");
	parameterNames.add("Sustain");
	parameterNames.add("Release");
	parameterNames.add("AttackCurve");
	parameterNames.add("DecayCurve");
	parameterNames.add("EcoMode");

	displayBuffer = new SimpleRingBuffer();
	displayBuffer->setGlobalUIUpdater(mc->getGlobalUIUpdater());

	displayBuffer->setPropertyObject(new AhdsrEnvelope::AhdsrRingBufferProperties(this));

	auto s = displayBuffer->getReadBuffer().getNumSamples();

	for(int i = 0; i < s; i++)
	{
		auto newValue = getAttribute(i + SpecialParameters::Attack);
		setDisplayValue(i, newValue, false);
	}

	SimpleReadWriteLock::ScopedWriteLock sl(displayBuffer->getDataLock());
	setExternalData(snex::ExternalData(displayBuffer.get(), 0), 0);

	editorStateIdentifiers.add("AttackTimeChainShown");
	editorStateIdentifiers.add("AttackLevelChainShown");
	editorStateIdentifiers.add("DecayTimeChainShown");
	editorStateIdentifiers.add("SustainLevelChainShown");
	editorStateIdentifiers.add("ReleaseTimeChainShown");

	for(int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	monophonicState = createSubclassedState(-1);

	internalChains.reserve(numInternalChains);

	internalChains += {this, "Attack Time", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Attack Level", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Decay Time", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Sustain Level", ModulatorChain::ModulationType::VoiceStartOnly };
	internalChains += {this, "Release Time", ModulatorChain::ModulationType::VoiceStartOnly };

	internalChains.finalise();

	for (auto& mb : internalChains)
		mb.getChain()->setParentProcessor(this);

    setTargetRatioDR(0.0001f);

	setAttackCurve(0.0f);
	setDecayCurve(0.0f);
}

void AhdsrEnvelope::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttributeWithDefault(AttackCurve);
	loadAttributeWithDefault(DecayCurve);
	loadAttribute(Attack, "Attack");
	loadAttribute(AttackLevel, "AttackLevel");
	loadAttribute(Hold, "Hold");
	loadAttribute(Decay, "Decay");
	loadAttribute(Sustain, "Sustain");
	loadAttribute(Release, "Release");
	loadAttribute(EcoMode, "EcoMode");
}

ValueTree AhdsrEnvelope::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(AttackCurve, "AttackCurve");
	saveAttribute(DecayCurve, "DecayCurve");
	saveAttribute(Attack, "Attack");
	saveAttribute(AttackLevel, "AttackLevel");
	saveAttribute(Hold, "Hold");
	saveAttribute(Decay, "Decay");
	saveAttribute(Sustain, "Sustain");
	saveAttribute(Release, "Release");
	saveAttribute(EcoMode, "EcoMode");

	return v;
}

float AhdsrEnvelope::startVoice(int voiceIndex)
{
	stateInfo.state = AhdsrEnvelopeState::ATTACK;
	stateInfo.changeTime = getMainController()->getUptime();

	if (isMonophonic)
	{
		state = static_cast<AhdsrEnvelopeState*>(monophonicState.get());

		EnvelopeModulator::startVoice(voiceIndex);

		const bool restartEnvelope = shouldRetrigger || getNumPressedKeys() == 1;

		if (restartEnvelope)
		{
			
			for (auto& mb : internalChains)
				mb.startVoice(voiceIndex);
			
			state->modValues[AttackTimeChain] = internalChains[AttackTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[AttackLevelChain] = internalChains[AttackLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[DecayTimeChain] = internalChains[DecayTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[SustainLevelChain] = internalChains[SustainLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
			state->modValues[ReleaseTimeChain] = internalChains[ReleaseTimeChain].getChain()->getConstantVoiceValue(voiceIndex);

			// Don't reset the envelope for tailing releases
			if (shouldRetrigger && state->current_state != AhdsrEnvelopeState::IDLE)
			{
				state->current_state = AhdsrEnvelopeState::RETRIGGER;
			}
			else
			{
				state->current_state = AhdsrEnvelopeState::ATTACK;
				state->current_value = 0.0f;
			}

			state->attackLevel = attackLevel * state->modValues[AttackLevelChain];
			state->setAttackRate(attack);
			state->setDecayRate(decay);
			state->setReleaseRate(release);

			state->lastSustainValue = sustain * state->modValues[SustainLevelChain];
		}
	}
	else
	{
		state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);

		if (state->current_state != AhdsrEnvelopeState::IDLE)
		{
			reset(voiceIndex);
		}

		for (auto& mb : internalChains)
			mb.startVoice(voiceIndex);

		state->modValues[AttackTimeChain] = internalChains[AttackTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[AttackLevelChain] = internalChains[AttackLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[DecayTimeChain] = internalChains[DecayTimeChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[SustainLevelChain] = internalChains[SustainLevelChain].getChain()->getConstantVoiceValue(voiceIndex);
		state->modValues[ReleaseTimeChain] = internalChains[ReleaseTimeChain].getChain()->getConstantVoiceValue(voiceIndex);

		state->attackLevel = attackLevel * state->modValues[AttackLevelChain];
		state->setAttackRate(attack);
		state->setDecayRate(decay);
		state->setReleaseRate(release);

		state->current_state = AhdsrEnvelopeState::ATTACK;

		state->current_value = 0.0f;
		state->leftOverSamplesFromLastBuffer = 0;

		state->lastSustainValue = sustain * state->modValues[SustainLevelChain];
	}

	return calculateNewValue(voiceIndex);
}

void AhdsrEnvelope::stopVoice(int voiceIndex)
{
	if (isMonophonic)
	{
		EnvelopeModulator::stopVoice(voiceIndex);

		
	}
	else
	{
		static_cast<AhdsrEnvelopeState*>(states[voiceIndex])->current_state = AhdsrEnvelopeState::RELEASE;
	}
}

void AhdsrEnvelope::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = isMonophonic ? -1 : polyManager.getCurrentVoice();

	jassert(voiceIndex < states.size());

	if (isMonophonic)
		state = static_cast<AhdsrEnvelopeState*>(monophonicState.get());
	else
		state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);

	const bool isSustain = static_cast<AhdsrEnvelopeState*>(state)->current_state == AhdsrEnvelopeState::SUSTAIN;

	if (isSustain)
	{
		const float thisSustainValue = sustain * state->modValues[SustainLevelChain];
		const float lastSustainValue = state->lastSustainValue;
		
		if (std::abs(thisSustainValue - lastSustainValue) > 0.001f)
		{
			const float stepSize = (thisSustainValue - lastSustainValue) / (float)numSamples;
			float* bufferPointer = internalBuffer.getWritePointer(0, startSample);
			float rampedGain = lastSustainValue;

			for (int i = 0; i < numSamples; i++)
			{
				bufferPointer[i] = rampedGain;
				rampedGain += stepSize;
				startSample++;
			}
		}
		else
		{
			FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), thisSustainValue, numSamples);
			startSample += numSamples;
		}

		state->lastSustainValue = thisSustainValue;
		state->current_value = thisSustainValue;
	}
	else
	{
		while (numSamples > 0)
		{
			internalBuffer.setSample(0, startSample, calculateNewValue(voiceIndex));
			++startSample;
			numSamples--;
		}

		
	}

	const bool isActiveVoice = polyManager.getCurrentVoice() == polyManager.getLastStartedVoice();

	if (isMonophonic || isActiveVoice)
	{
		auto uptime = getMainController()->getUptime();

		if (state->current_state != stateInfo.state)
		{
			stateInfo.state = state->current_state;
			stateInfo.changeTime = uptime;
		}

		if (ballUpdater.shouldUpdate())
		{
			auto pos = state->getUIPosition((uptime - stateInfo.changeTime) * 1000.0);
			displayBuffer->sendDisplayIndexMessage(pos);
		}
	}
}

void AhdsrEnvelope::reset(int voiceIndex)
{
	if (isMonophonic)
	{
		stateInfo.state = AhdsrEnvelopeState::IDLE;
		return;
	}
	else
	{
		EnvelopeModulator::reset(voiceIndex);

		if (voiceIndex == polyManager.getLastStartedVoice())
			stateInfo.state = AhdsrEnvelopeState::IDLE;

		state = static_cast<AhdsrEnvelopeState*>(states[voiceIndex]);
		state->current_state = AhdsrEnvelopeState::IDLE;
		state->current_value = 0.0f;
	}
}

void AhdsrEnvelope::handleHiseEvent(const HiseEvent &e)
{
	EnvelopeModulator::handleHiseEvent(e);

	if (isInMonophonicMode() && getNumPressedKeys() == 0)
	{
		auto monoState = static_cast<AhdsrEnvelopeState*>(monophonicState.get());
		monoState->current_state = AhdsrEnvelopeState::RELEASE;
	}

	for (auto& mb : internalChains)
		mb.handleHiseEvent(e);
};

float AhdsrEnvelope::getDefaultValue(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getDefaultValue(parameterIndex);
	}

	switch (parameterIndex)
	{
	case Attack:		return 20.0f;
	case AttackLevel:	return Decibels::gainToDecibels(1.0f);
	case Hold:			return 10.0f;
	case Decay:			return 300.0f;
	case Sustain:		return Decibels::gainToDecibels(1.0f);
	case Release:		return 20.0f;
	case AttackCurve:	return 1.0f;
	case DecayCurve:	return 1.0f;
	case EcoMode:		return 1.0f;
	default:		jassertfalse; return -1;
	}
}

void AhdsrEnvelope::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex >= SpecialParameters::Attack)
	{
		if (auto s = SimpleReadWriteLock::ScopedTryReadLock(displayBuffer->getDataLock()))
		{
			setDisplayValue(parameterIndex - SpecialParameters::Attack, newValue, false);
		}
	}

	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		EnvelopeModulator::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	switch (parameterIndex)
	{
	case Attack:		setAttackRate(newValue); break;
	case AttackLevel:	attackLevel = Decibels::decibelsToGain(newValue); break;
	case Hold:			setHoldTime(newValue); break;
	case Decay:			setDecayRate(newValue); break;
	case Sustain:		setSustainLevel(Decibels::decibelsToGain(newValue)); break;
	case Release:		setReleaseRate(newValue); break;
	case AttackCurve:	setAttackCurve(newValue); break;
	case DecayCurve:	setDecayCurve(newValue); break;
	case EcoMode:		break; // not needed anymore...
	default:			jassertfalse;
	}
}

float AhdsrEnvelope::getAttribute(int parameterIndex) const
{
	if (parameterIndex < EnvelopeModulator::Parameters::numParameters)
	{
		return EnvelopeModulator::getAttribute(parameterIndex);
	}

	switch (parameterIndex)
	{
	case Attack:		return attack;
	case AttackLevel:	return Decibels::gainToDecibels(attackLevel);
	case Hold:			return hold;
	case Decay:			return decay;
	case Sustain:		return Decibels::gainToDecibels(sustain);
	case Release:		return release;
	case AttackCurve:	return attackCurve;
	case DecayCurve:	return decayCurve;
	case EcoMode:		return 1.0f; // not needed anymore...
	default:		jassertfalse; return -1;
	}
}

void AhdsrEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	for (auto& mb : internalChains)
		mb.prepareToPlay(sampleRate, samplesPerBlock);

	setBaseSampleRate(getControlRate());

	ballUpdater.limitFromBlockSizeToFrameRate(sampleRate, samplesPerBlock);

	setAttackRate(attack);
	setDecayRate(decay);
	setReleaseRate(release);
	setSustainLevel(sustain);
}

bool AhdsrEnvelope::isPlaying(int voiceIndex) const
{
	if (isMonophonic)
		return true;

	return static_cast<AhdsrEnvelopeState*>(states[voiceIndex])->current_state != AhdsrEnvelopeState::IDLE;
}

EnvelopeModulator::ModulatorState * AhdsrEnvelope::createSubclassedState(int voiceIndex) const
{
	return new AhdsrEnvelopeState(voiceIndex, this);
}

float AhdsrEnvelope::calculateNewValue(int /*voiceIndex*/)
{
	return state->tick();
}


ProcessorEditorBody * AhdsrEnvelope::createEditor(ProcessorEditor* parentEditor)
{
#if USE_BACKEND

	return new AhdsrEnvelopeEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};


AhdsrEnvelope::Panel::Panel(FloatingTile* parent) :
	PanelWithProcessorConnection(parent)
{
	setDefaultPanelColour(FloatingTileContent::PanelColourId::bgColour, Colours::transparentBlack);
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::white.withAlpha(0.1f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour2, Colours::white.withAlpha(0.5f));
	setDefaultPanelColour(FloatingTileContent::PanelColourId::itemColour3, Colours::white.withAlpha(0.05f));
}

juce::Component* AhdsrEnvelope::Panel::createContentComponent(int /*index*/)
{
	if (auto b = dynamic_cast<scriptnode::data::base*>(getProcessor()))
	{
		if (auto rb = dynamic_cast<SimpleRingBuffer*>(b->externalData.obj))
		{
			auto g = new AhdsrGraph();
			g->setComplexDataUIBase(rb);
			g->setUseFlatDesign(true);

			g->setColour(AhdsrGraph::bgColour, findPanelColour(FloatingTileContent::PanelColourId::bgColour));
			g->setColour(AhdsrGraph::fillColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour1));
			g->setColour(AhdsrGraph::lineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour2));
			g->setColour(AhdsrGraph::outlineColour, findPanelColour(FloatingTileContent::PanelColourId::itemColour3));

			if (g->findColour(AhdsrGraph::bgColour).isOpaque())
				g->setOpaque(true);

			if (getProcessor()->getMainController()->getCurrentScriptLookAndFeel() != nullptr)
			{
				ScopedPointer<LookAndFeel> scriptlaf = HiseColourScheme::createAlertWindowLookAndFeel(getProcessor()->getMainController());

				if (auto s = dynamic_cast<AhdsrGraph::LookAndFeelMethods*>(scriptlaf.get()))
					g->setSpecialLookAndFeel(scriptlaf.release(), true);
			}

			return g;
		}
	}

	return nullptr;
}

void AhdsrEnvelope::Panel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<AhdsrEnvelope>(moduleList);
}


} // namespace hise
