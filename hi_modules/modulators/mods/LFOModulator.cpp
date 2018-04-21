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
	intensityChain(new ModulatorChain(mc, "LFO Intensity Mod", 1, Modulation::GainMode, this)),
    intensityBuffer(1, 0),
    frequencyBuffer(1, 0),
	customTable(new SampleLookupTable()),
	data(new SliderPackData(mc->getControlUndoManager())),
	currentWaveform((Waveform)(int)getDefaultValue(WaveFormType)),
	currentTable(nullptr),
	currentRandomValue(1.0f),
	currentTempo(TempoSyncer::Eighth),
	legato(getDefaultValue(Legato) >= 0.5f),
	loopEnabled(getDefaultValue(LoopEnabled) >= 0.5f),
	tempoSync(getDefaultValue(TempoSync) >= 0.5f),
	smoothingTime(getDefaultValue(SmoothingTime)),
	updater(*this)
{
	
	scaleFunction = [](float input) { return input * 2.0f - 1.0f; };

	editorStateIdentifiers.add("IntensityChainShown");
	editorStateIdentifiers.add("FrequencyChainShown");

	parameterNames.add(Identifier("Frequency"));
	parameterNames.add(Identifier("FadeIn")); 
	parameterNames.add(Identifier("WaveFormType"));
	parameterNames.add(Identifier("Legato"));
	parameterNames.add(Identifier("TempoSync"));
	parameterNames.add(Identifier("SmoothingTime"));
	parameterNames.add(Identifier("NumSteps"));
	parameterNames.add(Identifier("LoopEnabled"));

	frequencyUpdater.setManualCountLimit(4096);

	randomGenerator.setSeedRandomly();

	getMainController()->addTempoListener(this);

	data->setNumSliders(16);

	frequencyChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());
	intensityChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());

	WaveformLookupTables::init();

	setCurrentWaveform();

	CHECK_COPY_AND_RETURN_10(this);

	customTable->addChangeListener(&updater);
	data->addChangeListener(&updater);

	setTargetRatioA(0.3f);
	
};

LfoModulator::~LfoModulator()
{
	customTable->removeAllChangeListeners();
	data->removeAllChangeListeners();
	customTable = nullptr;

	getMainController()->removeTempoListener(this);
};


ProcessorEditorBody *LfoModulator::createEditor(ProcessorEditor *parentEditor)
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
	case Parameters::NumSteps:
		return 16.0f;
	case Parameters::LoopEnabled:
		return true;
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
	case Parameters::NumSteps:
		return (float)data->getNumSliders();
	case Parameters::LoopEnabled:
		return loopEnabled ? 1.0f : 0.0f;
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
	//FloatVectorOperations::multiply(intens, getIntensity(), samplesToCopy);

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
		CHECK_COPY_AND_RETURN_15(this);
		break;
	case Parameters::SmoothingTime:
		smoothingTime = newValue;
		smoother.setSmoothingTime(smoothingTime);
		break;
	case Parameters::NumSteps:
		data->setNumSliders(jmax<int>(1, (int)newValue));
		break;
	case Parameters::LoopEnabled:
		loopEnabled = newValue > 0.5f;
		break;
	default: 
		jassertfalse;
	}
};


void LfoModulator::getWaveformTableValues(int /*displayIndex*/, float const** tableValues, int& numValues, float& normalizeValue)
{
	if (currentWaveform == Random)
	{
		*tableValues = WaveformLookupTables::randomTable;
		numValues = SAMPLE_LOOKUP_TABLE_SIZE;
		normalizeValue = 1.0f;
		interpolationMode = WaveformComponent::Truncate;
	}
	else if (currentWaveform == Steps)
	{
		*tableValues = data->getCachedData();
		numValues = data->getNumSliders();
		normalizeValue = 1.0f;

		interpolationMode = WaveformComponent::Truncate;
	}
	else 
	{
		*tableValues = currentTable;
		numValues = SAMPLE_LOOKUP_TABLE_SIZE;
		normalizeValue = 1.0f;

		interpolationMode = WaveformComponent::LinearInterpolation;
	}
}

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
	else if (currentWaveform == Waveform::Steps)
	{

		if (lastSwapIndex != index && nextIndex - firstIndex != 1)
		{
			lastSwapIndex = index;

			if (!loopEnabled && (currentSliderIndex+1) == data->getNumSliders())
			{
				if (loopEndValue == -1.0f)
					loopEndValue = 1.0f - data->getValue(data->getNumSliders() - 1);

				currentSliderValue = loopEndValue;
			}
			else
			{
				currentSliderIndex = (currentSliderIndex + 1) % data->getNumSliders();

				data->setDisplayedIndex(currentSliderIndex);

				currentSliderValue = 1.0f - data->getValue(currentSliderIndex);
			}

			
		}

		newValue = currentSliderValue;
	}
	else
	{
		if (!loopEnabled && currentWaveform == Custom && uptime > (double)(SAMPLE_LOOKUP_TABLE_SIZE-1))
		{
			if (loopEndValue == -1.0f)
				loopEndValue = currentTable[SAMPLE_LOOKUP_TABLE_SIZE - 1];

			newValue = 1.0f - loopEndValue;
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
		CHECK_COPY_AND_RETURN_5(this);

		ProcessorHelpers::increaseBufferIfNeeded(intensityBuffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(frequencyBuffer, samplesPerBlock);

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

void LfoModulator::handleHiseEvent(const HiseEvent &m)
{
	intensityChain->handleHiseEvent(m);
	frequencyChain->handleHiseEvent(m);

	if (m.isAllNotesOff())
	{
		keysPressed = 0;
	}
	if(m.isNoteOn())
	{
		if(legato == false || keysPressed == 0)
		{
			uptime = 0.0;
			
			loopEndValue = -1.0f;

			if (currentWaveform == Steps)
			{
				currentSliderIndex = 0;
				currentSliderValue = 1.0f - data->getValue(0);
				data->setDisplayedIndex(0);
				lastSwapIndex = -1;
			}

			
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



void LfoModulator::calcAngleDelta()
{
	const double sr = getSampleRate();

	const float frequencyToUse = tempoSync ? TempoSyncer::getTempoInHertz(getMainController()->getBpm(), currentTempo) :
		frequency;

	const float cyclesPerSecond = frequencyToUse * frequencyModulationValue;
	const double cyclesPerSample = (double)cyclesPerSecond / sr;

	angleDelta = cyclesPerSample * (double)SAMPLE_LOOKUP_TABLE_SIZE;
}

float WaveformLookupTables::sineTable[SAMPLE_LOOKUP_TABLE_SIZE];
float WaveformLookupTables::triangleTable[SAMPLE_LOOKUP_TABLE_SIZE];
float WaveformLookupTables::sawTable[SAMPLE_LOOKUP_TABLE_SIZE];
float WaveformLookupTables::squareTable[SAMPLE_LOOKUP_TABLE_SIZE];
float WaveformLookupTables::randomTable[SAMPLE_LOOKUP_TABLE_SIZE];
bool WaveformLookupTables::initialised = false;

void WaveformLookupTables::init()
{
	if (initialised)
		return;

	const float max = (float)SAMPLE_LOOKUP_TABLE_SIZE;
	const float half = SAMPLE_LOOKUP_TABLE_SIZE / 2;

	juce::Random r;

	float lastRandomValue = r.nextFloat();

	for (int i = 0; i < SAMPLE_LOOKUP_TABLE_SIZE; i++)
	{
		sineTable[i] = 0.5f *cosf(i * float_Pi / half) + 0.5f;

		triangleTable[i] = i >= half ? (float)(2.0 * i) / max - 1.0f :
			(float)(-2.0 * i) / max + 1.0f;

		sawTable[i] = (float)(1.0f * i) / max;

		squareTable[i] = i >= half ? 0.0f : 1.0f;

		if (i % 32 == 0)
			lastRandomValue = r.nextFloat();

		randomTable[i] = lastRandomValue;

	}

	initialised = true;
}

} // namespace hise
