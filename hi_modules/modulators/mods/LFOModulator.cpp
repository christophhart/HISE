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

LfoModulator::LfoModulator(MainController *mc, const String &id, Modulation::Mode m):
	TimeVariantModulator(mc, id, m),
	Modulation(m),
	frequency(getDefaultValue(Frequency)),
	run(false),
	currentValue(1.0f),
	angleDelta(0.0),
	attack(getDefaultValue(FadeIn)),
	attackBase(0.0),
	attackCoef(0.0),
	attackValue(0.0f),
	uptime(0.0),
	keysPressed(0),
	intensityModulationValue(1.0f),
	frequencyModulationValue(1.0f),
	frequencyChain(new ModulatorChain(mc, "LFO Frequency Mod", 1, Modulation::GainMode, this)),
	intensityChain(new ModulatorChain(mc, "LFO Intensity Mod", 1, m, this)),
	customTable(new SampleLookupTable()),
	currentWaveform((Waveform)(int)getDefaultValue(WaveFormType)),
	currentTable(nullptr),
	currentRandomValue(1.0f),
	currentTempo(TempoSyncer::Eighth),
	legato(getDefaultValue(Legato) >= 0.5f),
	tempoSync(getDefaultValue(TempoSync) >= 0.5f),
	smoothingTime(getDefaultValue(SmoothingTime))
{
	editorStateIdentifiers.add("IntensityChainShown");
	editorStateIdentifiers.add("FrequencyChainShown");

	parameterNames.add(Identifier("Frequency"));
	parameterNames.add(Identifier("FadeIn")); 
	parameterNames.add(Identifier("WaveFormType"));
	parameterNames.add(Identifier("Legato"));
	parameterNames.add(Identifier("TempoSync"));
	parameterNames.add(Identifier("SmoothingTime"));

	frequencyUpdater.setManualCountLimit(4096);

	randomGenerator.setSeedRandomly();

	getMainController()->addTempoListener(this);

	frequencyChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());
	intensityChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());

	initSampleTables();

	setCurrentWaveform();

	CHECK_KEY(getMainController());

	setTargetRatioA(0.3f);
	
};

LfoModulator::~LfoModulator()
{
	getMainController()->removeTempoListener(this);
};


ProcessorEditorBody *LfoModulator::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new LfoEditorBody(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
};


float LfoModulator::getDefaultValue(int parameterIndex) const
{
	

	switch (parameterIndex)
	{
	case Parameters::Frequency:
		return tempoSync ? (float)TempoSyncer::Eighth : 3.0f;
	case Parameters::FadeIn: return 1000.0;
	case Parameters::WaveFormType:
		return (float)Waveform::Sine;
	case Parameters::Legato:
		return true;
	case Parameters::TempoSync:
		return false;
	case Parameters::SmoothingTime:
		return 5.0f;
	default:
		jassertfalse;
		return -1.0f;
	}
}

float LfoModulator::getAttribute(int parameter_index) const
{
	switch(parameter_index)
	{
	case Parameters::Frequency:
		return tempoSync ? (float)currentTempo : frequency;
	case Parameters::FadeIn:
		return attack;
	case Parameters::WaveFormType:
		return (float)currentWaveform;
	case Parameters::Legato:
		return legato ? 1.0f : 0.0f;
	case Parameters::TempoSync:
		return tempoSync ? 1.0f : 0.0f;
	case Parameters::SmoothingTime:
		return smoother.getSmoothingTime();
	default: 
		jassertfalse;
		return -1.0f;
	}

};


void LfoModulator::applyTimeModulation(AudioSampleBuffer &buffer, int startIndex, int samplesToCopy)
{
	float *dest = buffer.getWritePointer(0, startIndex);
	float *mod = internalBuffer.getWritePointer(0, startIndex);

	intensityChain->renderAllModulatorsAsMonophonic(intensityBuffer, startIndex, samplesToCopy);
	
	frequencyChain->renderAllModulatorsAsMonophonic(frequencyBuffer, startIndex, samplesToCopy);

	if(frequencyUpdater.shouldUpdate(samplesToCopy))
	{
		frequencyModulationValue = frequencyBuffer.getReadPointer(0, startIndex)[0];
		calcAngleDelta();
	}

	float *intens = intensityBuffer.getWritePointer(0, startIndex);
	FloatVectorOperations::multiply(intens, getIntensity(), samplesToCopy);

	if(getMode() == GainMode)		TimeModulation::applyGainModulation( mod, dest, getIntensity(), intens, samplesToCopy);
	else if(getMode() == PitchMode) TimeModulation::applyPitchModulation(mod, dest, getIntensity(), intens, samplesToCopy);

};

void LfoModulator::setInternalAttribute (int parameter_index, float newValue)
{
	switch(parameter_index)
	{
	case Parameters::Frequency:
		if(tempoSync)
		{
			currentTempo = (TempoSyncer::Tempo)(int)(newValue);
		}
		else
		{
			frequency = newValue;
		}
		
		calcAngleDelta();
		break;
	case Parameters::FadeIn:
		setFadeInTime(newValue);
		break;
	case Parameters::WaveFormType:	
		currentWaveform = (Waveform)(int) newValue;
		setCurrentWaveform();
		break;
	case Parameters::Legato:
		legato = newValue >= 0.5f;
		break;
	case Parameters::TempoSync:
		tempoSync = newValue >= 0.5f;
		CHECK_KEY(getMainController());
		break;
	case Parameters::SmoothingTime:
		smoothingTime = newValue;
		smoother.setSmoothingTime(smoothingTime);
		break;
	default: 
		jassertfalse;
	}
};


float LfoModulator::calculateNewValue ()
{
	//const float newValue = (cosf (uptime)) * 0.5f + 0.5f;	

	int index = (int)uptime;

	const int firstIndex = index & (SAMPLE_LOOKUP_TABLE_SIZE - 1);
	const int nextIndex = (index +1) & (SAMPLE_LOOKUP_TABLE_SIZE - 1);

	float newValue;

	if(currentWaveform == Waveform::Random)
	{
		jassert(currentTable == nullptr);

		if(nextIndex - firstIndex != 1)
		{
			currentRandomValue = randomGenerator.nextFloat();
		}

		newValue = currentRandomValue;

	}
	else
	{
		jassert(currentTable != nullptr);

		float v1 = currentTable[firstIndex];
		float v2 = currentTable[nextIndex];

		const float alpha = float(uptime) - (float)index;
		const float invAlpha = 1.0f - alpha;

		newValue = 1.0f - (invAlpha * v1 + alpha * v2);
	}

	if(attack != 0.0f || attackValue < 1.0f) attackValue = attackBase + attackValue * attackCoef;
	else attackValue = 1.0f;

	attackValue = CONSTRAIN_TO_0_1(attackValue);

	jassert(attackValue >= 0.0f);

	// Apply a little smoothing to filter hard edges
	currentValue = smoother.smooth(1.0f - newValue * attackValue);

	uptime += (angleDelta);
	
	return currentValue;
}

void LfoModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	Processor::prepareToPlay(sampleRate, samplesPerBlock);

	TimeModulation::prepareToModulate(sampleRate, samplesPerBlock);
	
	if(sampleRate != -1.0)
	{
		CHECK_KEY(getMainController());

		intensityBuffer = AudioSampleBuffer(1, samplesPerBlock *2);

		frequencyBuffer = AudioSampleBuffer(1, samplesPerBlock *2);

		intensityChain->prepareToPlay(sampleRate, samplesPerBlock);
		frequencyChain->prepareToPlay(sampleRate, samplesPerBlock);

		setAttackRate(attack);

		calcAngleDelta();
		smoother.prepareToPlay(sampleRate);
		
		smoother.setSmoothingTime(smoothingTime);

		inputMerger.setManualCountLimit(10);

		randomGenerator.setSeedRandomly();
	}

	// Use the block size to ramp the blocks.
	intensityInterpolator.setStepAmount(samplesPerBlock);
};

void LfoModulator::handleMidiEvent (const MidiMessage &m)
{
	intensityChain->handleMidiEvent(m);
	frequencyChain->handleMidiEvent(m);

	if (m.isAllNotesOff())
	{
		keysPressed = 0;
	}
	if(m.isNoteOn())
	{
		if(legato == false || keysPressed == 0)
		{
			uptime = 0.0;
			intensityChain->startVoice(0);
			//intensityInterpolator.setValue(intensityChain->getConstantVoiceValue(0));
			frequencyChain->startVoice(0);
			resetFadeIn();
			frequencyModulationValue = frequencyChain->getConstantVoiceValue(0);
			calcAngleDelta();
		}

		keysPressed++;

	}

	if(m.isNoteOff())
	{
		keysPressed--;

		if(keysPressed < 0) keysPressed = 0;

		if(legato == false || keysPressed == 0)
		{
			intensityChain->stopVoice(0);
			frequencyChain->stopVoice(0);
		}

	}

}



float LfoModulator::sineTable[SAMPLE_LOOKUP_TABLE_SIZE];
float LfoModulator::triangleTable[SAMPLE_LOOKUP_TABLE_SIZE];
float LfoModulator::sawTable[SAMPLE_LOOKUP_TABLE_SIZE];
float LfoModulator::squareTable[SAMPLE_LOOKUP_TABLE_SIZE];
