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

TableEnvelope::TableEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m, float attackTimeMs, float releaseTimeMs):
		EnvelopeModulator(mc, id, voiceAmount, m),
		Modulation(m),
		LookupTableProcessor(mc, 2),
		attack(attackTimeMs),
		release(releaseTimeMs),
		attackChain(new ModulatorChain(mc, "AttackTime Modulation", voiceAmount, Modulation::GainMode, this)),
		releaseChain(new ModulatorChain(mc, "ReleaseTime Modulation", voiceAmount, Modulation::GainMode, this))
{
	attackTable = dynamic_cast<SampleLookupTable*>(getTableUnchecked(0));
	releaseTable = dynamic_cast<SampleLookupTable*>(getTableUnchecked(1));

	parameterNames.add("Attack");
	parameterNames.add("Release");

	editorStateIdentifiers.add("AttackChainShown");
	editorStateIdentifiers.add("ReleaseChainShown");

	for(int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	monophonicState = createSubclassedState(-1);

	WeakReference<Processor> t = this;

	auto attackConverter = [t](float input)
	{
		if (t != nullptr)
		{
			auto time = t->getAttribute(TableEnvelope::SpecialParameters::Attack);
			return String(roundToInt(input * time)) + " ms";
		}

		return String();
	};

	auto releaseConverter = [t](float input)
	{
		if (t != nullptr)
		{
			auto time = t->getAttribute(TableEnvelope::SpecialParameters::Release);
			return String(roundToInt(input * time)) + " ms";
		}

		return String();
	};

	attackTable->setXTextConverter(attackConverter);
	releaseTable->setXTextConverter(releaseConverter);

	attackChain->setIsVoiceStartChain(true);
	releaseChain->setIsVoiceStartChain(true);

	Array<Table::GraphPoint> releasePoints;

	releasePoints.add(Table::GraphPoint(0.0f, 1.0f, 0.5));
	releasePoints.add(Table::GraphPoint(1.0f, 0.0f, 0.5));

	releaseTable->setGraphPoints(releasePoints, 2, true);

	attackChain->setParentProcessor(this);
	releaseChain->setParentProcessor(this);

	setInternalAttribute(SpecialParameters::Attack, attackTimeMs);
	setInternalAttribute(SpecialParameters::Release, releaseTimeMs);
};

TableEnvelope::~TableEnvelope()
{
};


void TableEnvelope::restoreFromValueTree(const ValueTree &v)
{
	EnvelopeModulator::restoreFromValueTree(v);

	loadAttribute(Attack, "Attack");
	loadAttribute(Release, "Release");

	loadTable(attackTable, "AttackTableData");
	loadTable(releaseTable, "ReleaseTableData");
}

ValueTree TableEnvelope::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();

	saveAttribute(Attack, "Attack");
	saveAttribute(Release, "Release");

	saveTable(attackTable, "AttackTableData");
	saveTable(releaseTable, "ReleaseTableData");

	return v;
}

float TableEnvelope::startVoice(int voiceIndex)
{
	if (isMonophonic)
	{
		EnvelopeModulator::startVoice(voiceIndex);

		const bool isFirstKey = getNumPressedKeys() == 1;
		const bool restartEnvelope = shouldRetrigger || isFirstKey;
		
		if (restartEnvelope)
		{
			auto monoState = static_cast<TableEnvelopeState*>(monophonicState.get());

			if(attackChain->shouldBeProcessedAtAll())
				attackChain->startVoice(voiceIndex);

			if(releaseChain->shouldBeProcessedAtAll())
				releaseChain->startVoice(voiceIndex);

			monoState->attackModValue = 1.0f / jmax<float>(0.001f, attackChain->getConstantVoiceValue(voiceIndex));
			monoState->releaseModValue = 1.0f / jmax<float>(0.001f, attackChain->getConstantVoiceValue(voiceIndex));

			monoState->uptime = 0.0f;

			if (attack == 0.0f || monoState->attackModValue > 998.0)
			{
				monoState->current_value = 1.0f;
				monoState->current_state = TableEnvelopeState::SUSTAIN;
			}
			else
			{
				monoState->current_state = isFirstKey ?	TableEnvelopeState::ATTACK : TableEnvelopeState::RETRIGGER;
			}
		}
	}
	else
	{
		auto state = static_cast<TableEnvelopeState*>(states[voiceIndex]);

		if(attackChain->shouldBeProcessedAtAll())
			attackChain->startVoice(voiceIndex);

		if(releaseChain->shouldBeProcessedAtAll())
			releaseChain->startVoice(voiceIndex);

		state->attackModValue = 1.0f / jmax<float>(0.001f, attackChain->getConstantVoiceValue(voiceIndex));
		state->releaseModValue = 1.0f / jmax<float>(0.001f, releaseChain->getConstantVoiceValue(voiceIndex));

		state->uptime = 0.0f;

		if (attack == 0.0f || state->attackModValue > 998.0)
		{
			state->current_value = 1.0f;
			state->current_state = TableEnvelopeState::SUSTAIN;
		}
		else
		{
			state->current_state = TableEnvelopeState::ATTACK;
		}
	}

	return calculateNewValue(voiceIndex);
}

void TableEnvelope::stopVoice(int voiceIndex)
{
	if (isMonophonic)
	{
		EnvelopeModulator::stopVoice(voiceIndex);

		if (getNumPressedKeys() == 0)
		{
			auto monoState = static_cast<TableEnvelopeState*>(monophonicState.get());
			monoState->current_state = TableEnvelopeState::RELEASE;

			monoState->releaseGain = monoState->current_value;
			monoState->uptime = 0;
		}
	}
	else
	{
		auto state = static_cast<TableEnvelopeState*>(states[voiceIndex]);

		state->current_state = TableEnvelopeState::RELEASE;
		state->releaseGain = state->current_value;
		state->uptime = 0;
	}
}

void TableEnvelope::calculateBlock(int startSample, int numSamples)
{
	const int voiceIndex = isMonophonic ? -1 : polyManager.getCurrentVoice();

	auto state = static_cast<TableEnvelopeState*>(isMonophonic ? monophonicState.get() : states[voiceIndex]);

	while (--numSamples >= 0)
	{
		internalBuffer.setSample(0, startSample, calculateNewValue(voiceIndex));
		++startSample;
	}

	if (polyManager.getLastStartedVoice() == voiceIndex && uiUpdater.shouldUpdate())
	{
		float normalisedDisplayValue = (float)state->uptime / (float)SAMPLE_LOOKUP_TABLE_SIZE;

		switch (state->current_state)
		{
		case TableEnvelopeState::ATTACK:
			attackTable->sendDisplayIndexMessage(normalisedDisplayValue);
			releaseTable->sendDisplayIndexMessage(0.0f);
			break;
		case TableEnvelopeState::SUSTAIN:
			attackTable->sendDisplayIndexMessage(1.0f);
			releaseTable->sendDisplayIndexMessage(0.0f);
			break;
		case TableEnvelopeState::RELEASE:
			attackTable->sendDisplayIndexMessage(1.0f);
			releaseTable->sendDisplayIndexMessage(normalisedDisplayValue);
			break;
		case TableEnvelopeState::RETRIGGER:
			attackTable->sendDisplayIndexMessage(normalisedDisplayValue);
			releaseTable->sendDisplayIndexMessage(0.0f);
			break;
		case TableEnvelopeState::IDLE:
			attackTable->sendDisplayIndexMessage(0.0f);
			releaseTable->sendDisplayIndexMessage(0.0f);
			break;
		}
	}
}

void TableEnvelope::reset(int voiceIndex)
{
	if (isMonophonic)
	{
		return;
	}
	else
	{
		EnvelopeModulator::reset(voiceIndex);

		TableEnvelopeState *state = static_cast<TableEnvelopeState*>(states[voiceIndex]);

		state->current_state = TableEnvelopeState::IDLE;
		state->current_value = 0.0f;

		setInputValue(0.0f);
		currentValues.inL = 0.0f;
		currentValues.outR = 0.0f;
	}
}

void TableEnvelope::handleHiseEvent(const HiseEvent& m)
{
	EnvelopeModulator::handleHiseEvent(m);
	
	if(attackChain->shouldBeProcessedAtAll())
		attackChain->handleHiseEvent(m);

	if(releaseChain->shouldBeProcessedAtAll())
		releaseChain->handleHiseEvent(m);
};

float TableEnvelope::calculateNewValue(int voiceIndex)
{
	jassert(voiceIndex < states.size());

	TableEnvelopeState *state = static_cast<TableEnvelopeState*>(isMonophonic ? monophonicState.get() : states[voiceIndex]);

	switch(state->current_state)
	{
	case TableEnvelopeState::ATTACK:
	{
		state->current_value = attackTable->getInterpolatedValue(state->uptime / (double)SAMPLE_LOOKUP_TABLE_SIZE, dontSendNotification);

		state->uptime += attackUptimeDelta * state->attackModValue;

		if ((int)state->uptime >= SAMPLE_LOOKUP_TABLE_SIZE)
		{
			state->uptime = 0.0f;

			if (!isMonophonic && attackTable->getLastValue() <= 0.01f)
			{
				stopVoice(voiceIndex);

			}
			else
			{
				state->current_state = TableEnvelopeState::SUSTAIN;
				state->current_value = 1.0f;
			}
		}
		break;
	}
	case TableEnvelopeState::SUSTAIN: break;

	case TableEnvelopeState::RETRIGGER:
	{
		const float targetValue = attackTable->getInterpolatedValue(0.0, dontSendNotification);
		const float currentValue = state->current_value;

		const bool down = currentValue > targetValue;

		if (down)
		{
			state->current_value -= 0.005f;

			if (state->current_value <= jmax<float>(0.0f, targetValue))
			{
				state->current_value = targetValue;
				state->current_state = TableEnvelopeState::ATTACK;
			}
		}
		else
		{
			state->current_value += 0.005f;

			if (state->current_value >= jmin<float>(1.0f, targetValue))
			{
				state->current_value = targetValue;
				state->current_state = TableEnvelopeState::ATTACK;
			}
		}

		break;
	}

	case TableEnvelopeState::RELEASE:

		state->uptime += releaseUptimeDelta * state->releaseModValue;

		if((int)state->uptime >= SAMPLE_LOOKUP_TABLE_SIZE)
		{
			state->current_value = 0.0f; 
			state->current_state = TableEnvelopeState::IDLE;
		}
		else
		{
			state->current_value = state->releaseGain * releaseTable->getInterpolatedValue(state->uptime / (double)SAMPLE_LOOKUP_TABLE_SIZE, dontSendNotification);
		}

		break;

	case TableEnvelopeState::IDLE: break;

	default:					    jassertfalse; break;
	}

	
	return state->current_value;
};

void TableEnvelope::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	uiUpdater.limitFromBlockSizeToFrameRate(getControlRate(), samplesPerBlock);
		

	setInternalAttribute(Attack, attack);
	setInternalAttribute(Release, release);
}

bool TableEnvelope::isPlaying(int voiceIndex) const
{
	if (isMonophonic)
	{
		return true;
	}
	else
	{
		TableEnvelopeState *state = static_cast<TableEnvelopeState*>(states[voiceIndex]);
		return state->current_state != TableEnvelopeState::IDLE;
	}
}

ProcessorEditorBody *TableEnvelope::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new TableEnvelopeEditorBody(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif

};

} // namespace hise
