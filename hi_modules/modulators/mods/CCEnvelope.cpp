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
CCEnvelope::CCEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m) :
EnvelopeModulator(mc, id, voiceAmount, m),
Modulation(m),
defaultValue(0),
smoothTime(200.f),
useTable(false),
controllerNumber(1),
hold(20),
decay(20),
learnMode(false),
inputValue(0.0f),
targetValue(0.0f),
fixedNoteOff(true),
startLevel(1.0f),
endLevel(1.0f),
smoothedCCValue(1.0f),
dutyVoice(INT_MAX)
{

	table = new SampleLookupTable();

	table->setLengthInSamples(512);

	parameterNames.add("UseTable");
	parameterNames.add("ControllerNumber");
	parameterNames.add("SmoothTime");
	parameterNames.add("DefaultValue");
	parameterNames.add("StartLevel");
	parameterNames.add("HoldTime");
	parameterNames.add("EndLevel");
	parameterNames.add("FixedNoteOff");
	parameterNames.add("DecayTime");

	editorStateIdentifiers.add("StartLevelChainShown");
	editorStateIdentifiers.add("HoldChainShown");
	editorStateIdentifiers.add("EndLevelChainShown");
	editorStateIdentifiers.add("DecayChainShown");

	for (int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));
	
	holdChain = new ModulatorChain(mc, "Hold Time Modulation", voiceAmount, ModulatorChain::GainMode, this);
	holdChain->setIsVoiceStartChain(true);

	startLevelChain = new ModulatorChain(mc, "Start Level Modulation", voiceAmount, ModulatorChain::GainMode, this);
	startLevelChain->setIsVoiceStartChain(true);

	endLevelChain = new ModulatorChain(mc, "End Level Modulation", voiceAmount, ModulatorChain::GainMode, this);
	endLevelChain->setIsVoiceStartChain(true);

	decayChain = new ModulatorChain(mc, "Decay Time Modulation", voiceAmount, ModulatorChain::GainMode, this);
	decayChain->setIsVoiceStartChain(true);
};

CCEnvelope::~CCEnvelope()
{
	holdChain = nullptr;
};

void CCEnvelope::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttribute(UseTable, "UseTable");
	loadAttribute(ControllerNumber, "ControllerNumber");
	loadAttribute(SmoothTime, "SmoothTime");
	loadAttribute(DefaultValue, "DefaultValue");
	loadAttribute(StartLevel, "StartLevel");
	loadAttribute(HoldTime, "HoldTime");
	loadAttribute(EndLevel, "EndLevel");
	loadAttribute(FixedNoteOff, "FixedNoteOff");
	loadAttribute(DecayTime, "DecayTime");

	if (useTable) loadTable(table, "ControllerTableData");
}




ValueTree CCEnvelope::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(UseTable, "UseTable");
	saveAttribute(ControllerNumber, "ControllerNumber");
	saveAttribute(SmoothTime, "SmoothTime");
	saveAttribute(DefaultValue, "DefaultValue");
	saveAttribute(StartLevel, "StartLevel");
	saveAttribute(HoldTime, "HoldTime");
	saveAttribute(EndLevel, "EndLevel");
	saveAttribute(FixedNoteOff, "FixedNoteOff");
	saveAttribute(DecayTime, "DecayTime");

	if (useTable) saveTable(table, "ControllerTableData");

	return v;
}



float CCEnvelope::getAttribute(int parameter_index) const
{
	switch (parameter_index)
	{
	case ControllerNumber:
		return (float)controllerNumber;
	case SmoothTime:
		return smoothTime;
	case UseTable:
		return useTable ? 1.0f : 0.0f;
	case DefaultValue:
		return defaultValue;
	case DecayTime:
		return decay;
	case StartLevel:
		return Decibels::gainToDecibels(startLevel);
	case EndLevel:
		return Decibels::gainToDecibels(endLevel);
	case HoldTime:
		return hold;
	case FixedNoteOff:
		return fixedNoteOff ? 1.0f : 0.0f;
	default:
		jassertfalse;
		return -1.0f;
	}
}


void CCEnvelope::setInternalAttribute(int parameter_index, float newValue)
{
	switch (parameter_index)
	{
	case ControllerNumber:
		controllerNumber = int(newValue); break;
	case SmoothTime:
	{
		smoothTime = newValue;
		
		smoother.setSmoothingTime(smoothTime);
		
		break;
	}
	case UseTable:
		useTable = (newValue != 0.0f); break;
	case DefaultValue:
	{
		defaultValue = newValue;
		MidiMessage m = MidiMessage::controllerEvent(1, controllerNumber, (int)defaultValue);
		handleMidiEvent(m);
		break;
	}
	case StartLevel:
		startLevel = Decibels::decibelsToGain(newValue);
		break;
	case EndLevel:
		endLevel = Decibels::decibelsToGain(newValue);
		break;
	case FixedNoteOff:
		fixedNoteOff = newValue > 0.5f;
		break;
	case DecayTime:
		decay = newValue; break;
	case HoldTime:
		hold = newValue; break;
	default:
		jassertfalse;
	}
}

int CCEnvelope::getNumInternalChains() const { return numTotalChains; }
int CCEnvelope::getNumChildProcessors() const { return numTotalChains; }

Processor * CCEnvelope::getChildProcessor(int processorIndex) 
{ 
	InternalChains c = (InternalChains)processorIndex;

	switch (c)
	{
	case CCEnvelope::StartLevelChain:
		return startLevelChain;
	case CCEnvelope::HoldChain:
		return holdChain;
	case CCEnvelope::EndLevelChain:
		return endLevelChain;
	case CCEnvelope::DecayChain:
		return decayChain;
	case CCEnvelope::numTotalChains:
	default:
        return nullptr;
	}
}
const Processor * CCEnvelope::getChildProcessor(int processorIndex) const 
{ 
	InternalChains c = (InternalChains)processorIndex;

	switch (c)
	{
	case CCEnvelope::StartLevelChain:
		return startLevelChain;
	case CCEnvelope::HoldChain:
		return holdChain;
	case CCEnvelope::EndLevelChain:
		return endLevelChain;
	case CCEnvelope::DecayChain:
		return decayChain;
	case CCEnvelope::numTotalChains:
	default:
        return nullptr;
	}
}

void CCEnvelope::startVoice(int voiceIndex)
{
	CCEnvelopeState *state = static_cast<CCEnvelopeState*>(states[voiceIndex]);

	if (voiceIndex < dutyVoice) dutyVoice = voiceIndex;

	if (state->current_state != CCEnvelopeState::IDLE)
	{
		debugMod("The Envelope was not idle at voice index " + String(voiceIndex));
	}

	if (state->current_state != CCEnvelopeState::IDLE)
	{
		reset(voiceIndex);
	}

	holdChain->startVoice(voiceIndex);
	startLevelChain->startVoice(voiceIndex);
	endLevelChain->startVoice(voiceIndex);
	decayChain->startVoice(voiceIndex);

	const float thisHoldTime = hold * holdChain->getConstantVoiceValue(voiceIndex);
	const float thisDecayTime = decay * decayChain->getConstantVoiceValue(voiceIndex);

	state->voiceStartLevel = startLevel * startLevelChain->getConstantVoiceValue(voiceIndex);
	state->voiceEndLevel = endLevel * endLevelChain->getConstantVoiceValue(voiceIndex);

	state->holdTimeInSamples = (int)(thisHoldTime / 1000.0f * (float)getSampleRate());

	state->decayTimeInSamples = (int)(thisDecayTime / 1000.0f * (float)getSampleRate());

	state->uptime = 0;

	state->current_state = CCEnvelopeState::HOLD;
	debugMod(" (Voice " + String(voiceIndex) + ": IDLE->ATTACK");
}

void CCEnvelope::stopVoice(int voiceIndex)
{
	CCEnvelopeState *state = static_cast<CCEnvelopeState*>(states[voiceIndex]);
	state->idleValue = state->actualValue;

	reset(voiceIndex);
}

void CCEnvelope::reset(int voiceIndex)
{
	EnvelopeModulator::reset(voiceIndex);

	CCEnvelopeState *state = static_cast<CCEnvelopeState*>(states[voiceIndex]);

	if (voiceIndex == dutyVoice) dutyVoice = INT_MAX;

	state->current_state = CCEnvelopeState::IDLE;
	state->actualValue = 0.0f;
	state->uptime = 0;
}

void CCEnvelope::calculateBlock(int startSample, int numSamples)
{
	if (--numSamples >= 0)
	{
		int voiceIndex = polyManager.getCurrentVoice();

		if (dutyVoice == INT_MAX) dutyVoice = voiceIndex;

		const float value = calculateNewValue();
		internalBuffer.setSample(0, startSample, value);
		++startSample;

		if (useTable) sendTableIndexChangeMessage(false, table, inputValue);

		if (voiceIndex == polyManager.getLastStartedVoice())
		{
			CCEnvelopeState *state = static_cast<CCEnvelopeState*>(states[voiceIndex]);

			currentValues.inR = jmin<float>((float)state->uptime / (float)(state->holdTimeInSamples + state->decayTimeInSamples), 1.0f);

			currentValues.outL = value;
		}
	}

	while (--numSamples >= 0)
	{
		internalBuffer.setSample(0, startSample, calculateNewValue());
		++startSample;
	}
}

void CCEnvelope::handleMidiEvent(MidiMessage const &m)
{
	holdChain->handleMidiEvent(m);
	startLevelChain->handleMidiEvent(m);
	endLevelChain->handleMidiEvent(m);
	decayChain->handleMidiEvent(m);

	if (learnMode)
	{
		if (m.isController())
		{
			controllerNumber = m.getControllerNumber();
			disableLearnMode();
		}
		else if (m.isPitchWheel())
		{
			controllerNumber = 129;
			disableLearnMode();
		}
		else if (m.isChannelPressure() || m.isAftertouch())
		{
			controllerNumber = 128;
			disableLearnMode();
		}
	}

	const bool isAftertouch = controllerNumber == 128 && (m.isAftertouch() || m.isChannelPressure());

	if (isAftertouch || m.isControllerOfType(controllerNumber) || m.isPitchWheel())
	{
		if (m.isController())
		{
			inputValue = m.getControllerValue() / 127.0f;
		}
		else if (controllerNumber == 129 && m.isPitchWheel())
		{
			inputValue = m.getPitchWheelValue() / 16383.0f;
		}
		else if (m.isChannelPressure())
		{
			inputValue = m.getChannelPressureValue() / 127.0f;
		}
		else
		{
			inputValue = m.getAfterTouchValue() / 127.0f;
		}

		float value;

		if (useTable) value = table->getInterpolatedValue(inputValue * (float)SAMPLE_LOOKUP_TABLE_SIZE);
		else value = inputValue;

		targetValue = value;

		debugMod(" New Value: " + String(value));
	}
};

void CCEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	setInternalAttribute(HoldTime, hold);
	setInternalAttribute(DecayTime, decay);
	if (holdChain != nullptr) holdChain->prepareToPlay(sampleRate, samplesPerBlock);

	decayChain->prepareToPlay(sampleRate, samplesPerBlock);
	startLevelChain->prepareToPlay(sampleRate, samplesPerBlock);
	endLevelChain->prepareToPlay(sampleRate, samplesPerBlock);

	smoother.prepareToPlay(getSampleRate());

	if (sampleRate != -1.0) setInternalAttribute(SmoothTime, smoothTime);

}

bool CCEnvelope::isPlaying(int voiceIndex) const
{
	CCEnvelopeState *state = static_cast<CCEnvelopeState*>(states[voiceIndex]);
	return state->current_state != CCEnvelopeState::IDLE;
}

float CCEnvelope::calculateNewValue()
{
	const int voiceIndex = polyManager.getCurrentVoice();
	
	jassert(dutyVoice != INT_MAX);

	if (voiceIndex == dutyVoice)
	{
		smoothedCCValue = getSmoothedCCValue(targetValue);
	}

	jassert(voiceIndex < states.size());
	 
	CCEnvelopeState *state = static_cast<CCEnvelopeState*>(states[voiceIndex]);
	 
	switch (state->current_state)
	{
		case CCEnvelopeState::HOLD:	
			if (state->uptime >= state->holdTimeInSamples)
			{
				state->current_state = CCEnvelopeState::DECAY;
			}
			else
			{
				const float alpha = (float)state->uptime / (float)state->holdTimeInSamples;

				state->actualValue = Interpolator::interpolateLinear(state->voiceStartLevel, state->voiceEndLevel, alpha);
				state->uptime++;
				break;
			}
		case CCEnvelopeState::DECAY:
		{
			if (state->uptime >= state->holdTimeInSamples + state->decayTimeInSamples)
			{
				state->current_state = CCEnvelopeState::SUSTAIN;
			}
			else
			{
				const float alpha = (float)(state->uptime - state->holdTimeInSamples) / (float)state->decayTimeInSamples;

				jassert(alpha <= 1.0f);

				state->actualValue = Interpolator::interpolateLinear(state->voiceEndLevel, smoothedCCValue, alpha);
				state->uptime++;

				break;
			}
		}
		case CCEnvelopeState::SUSTAIN:
			state->actualValue = smoothedCCValue;
			break;
		case CCEnvelopeState::IDLE:
			state->actualValue = state->idleValue;
			break;
	}
	 	 
	return state->actualValue;

}

ProcessorEditorBody *CCEnvelope::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new CCEnvelopeEditor(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif

};