/*
  ==============================================================================

    TableEnvelope.cpp
    Created: 7 Jul 2014 8:18:18pm
    Author:  Chrisboy

  ==============================================================================
*/


TableEnvelope::TableEnvelope(MainController *mc, const String &id, int voiceAmount, Modulation::Mode m, float attackTimeMs, float releaseTimeMs):
		EnvelopeModulator(mc, id, voiceAmount, m),
		Modulation(m),
		attackTable(new SampleLookupTable()),
		releaseTable(new SampleLookupTable()),
		attack(attackTimeMs),
		release(releaseTimeMs),
		attackChain(new ModulatorChain(mc, "AttackTime Modulation", voiceAmount, Modulation::GainMode, this)),
		releaseChain(new ModulatorChain(mc, "ReleaseTime Modulation", voiceAmount, Modulation::GainMode, this))
{
	parameterNames.add("Attack");
	parameterNames.add("Release");

	editorStateIdentifiers.add("AttackChainShown");
	editorStateIdentifiers.add("ReleaseChainShown");

	for(int i = 0; i < polyManager.getVoiceAmount(); i++) states.add(createSubclassedState(i));

	attackChain->setIsVoiceStartChain(true);
	releaseChain->setIsVoiceStartChain(true);

	Array<Table::GraphPoint> release;

	release.add(Table::GraphPoint(0.0f, 1.0f, 0.5));
	release.add(Table::GraphPoint(1.0f, 0.0f, 0.5));

	releaseTable->setGraphPoints(release, 2);
	releaseTable->fillLookUpTable();

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

void TableEnvelope::startVoice(int voiceIndex)
{
	
	TableEnvelopeState *state = static_cast<TableEnvelopeState*>(states[voiceIndex]);
	
	attackChain->startVoice(voiceIndex);
	releaseChain->startVoice(voiceIndex);

	state->attackModValue =  1.0f / jmax<float>(0.001f, attackChain->getConstantVoiceValue(voiceIndex));
	state->releaseModValue = 1.0f / jmax<float>(0.001f, releaseChain->getConstantVoiceValue(voiceIndex));
	
	state->uptime = 0.0f;

	state->current_state = TableEnvelopeState::ATTACK;
	debugMod(" (Voice " + String(voiceIndex) + ": IDLE->ATTACK");
}

void TableEnvelope::stopVoice(int voiceIndex)
{
	TableEnvelopeState *state = static_cast<TableEnvelopeState*>(states[voiceIndex]);
	//jassert(state->current_state == TableEnvelopeState::SUSTAIN || state->current_state == TableEnvelopeState::ATTACK);

	debugMod(" (Voice " + String(voiceIndex) + (state->current_state == TableEnvelopeState::SUSTAIN ? "SUSTAIN": "ATTACK") + "->RELEASE");
	state->current_state = TableEnvelopeState::RELEASE;

	state->releaseGain = state->current_value;

	state->uptime = 0;

}

void TableEnvelope::calculateBlock(int startSample, int numSamples)
{
	if (--numSamples >= 0)
	{
		const float value = calculateNewValue();
		internalBuffer.setSample(0, startSample, value);
		++startSample;
		if (polyManager.getCurrentVoice() == polyManager.getLastStartedVoice())
		{
			const int voiceIndex = polyManager.getCurrentVoice();

			jassert(voiceIndex < states.size());

			TableEnvelopeState *state = static_cast<TableEnvelopeState*>(states[voiceIndex]);


			// skip sustaining and idle envelopes
			if (state->current_state == TableEnvelopeState::SUSTAIN)
			{
				sendTableIndexChangeMessage(false, attackTable, 1.0f);
				sendTableIndexChangeMessage(false, releaseTable, 0.0f);
				/*
				currentValues.outL = 1.0f;

				setInputValue(1.0f);

				currentValues.inL = 1.0f;
				*/
			}
			else
			{
				SampleLookupTable *tableToUse = state->current_state == TableEnvelopeState::ATTACK ? attackTable : releaseTable;

				const float indexValue = (float)state->uptime / (float)tableToUse->getLengthInSamples();

				sendTableIndexChangeMessage(false, tableToUse, indexValue);
				

				/*
				// Uses negative values to indicate that the envelope is in release phase. Not very nice, but works.
				const float signum = (state->current_state == TableEnvelopeState::ATTACK ? 1.0f : -1.0f);

				const SampleLookupTable *tableToUse = state->current_state == TableEnvelopeState::ATTACK ? attackTable : releaseTable;

				const float value = (float)state->uptime / (float)tableToUse->getLengthInSamples();

				currentValues.outL = state->current_value;
				currentValues.inL = value;
				currentValues.inR = signum;

				setInputValue(value * signum);

				*/
			}

			setOutputValue(value);
		}
	}




	while (--numSamples >= 0)
	{
		internalBuffer.setSample(0, startSample, calculateNewValue());
		++startSample;
	}
}

void TableEnvelope::handleMidiEvent(MidiMessage const &m)
{
	attackChain->handleMidiEvent(m);
	releaseChain->handleMidiEvent(m);
};

float TableEnvelope::calculateNewValue()
{
	const int voiceIndex = polyManager.getCurrentVoice();

	jassert(voiceIndex < states.size());

	TableEnvelopeState *state = static_cast<TableEnvelopeState*>(states[voiceIndex]);

	switch(state->current_state)
	{
	case TableEnvelopeState::ATTACK:

		state->current_value = attackTable->getInterpolatedValue(state->uptime);

		state->uptime += state->attackModValue;

		if((int)state->uptime >= attackTable->getLengthInSamples() - 1)
		{
			state->uptime = 0.0f;

			if(attackTable->getLastValue() <= 0.01f)
			{
				stopVoice(voiceIndex);
				
				debugMod(" (voiceIndex = " + String(voiceIndex) + "): ATTACK->RELEASE");

			}
			else
			{
			state->current_state = TableEnvelopeState::SUSTAIN;
			debugMod(" (voiceIndex = " + String(voiceIndex) + "): ATTACK->SUSTAIN");
			}
		}
		break;
			
	case TableEnvelopeState::SUSTAIN: break;

	case TableEnvelopeState::RELEASE:

		state->uptime += state->releaseModValue;

		if((int)state->uptime >= releaseTable->getLengthInSamples() - 1)
		{
			state->current_value = 0.0f; 
			state->current_state = TableEnvelopeState::IDLE;
			debugMod(" (voiceIndex = " + String(voiceIndex) + "): RELEASE->IDLE");
		}
		else
		{
			state->current_value = state->releaseGain * releaseTable->getInterpolatedValue(state->uptime);
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
		

	setInternalAttribute(Attack, attack);
	setInternalAttribute(Release, release);
}

ProcessorEditorBody *TableEnvelope::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new TableEnvelopeEditorBody(parentEditor);

#else

	jassertfalse;

	return nullptr;

#endif

};
