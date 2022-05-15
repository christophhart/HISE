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

Modulation::~Modulation()
{
	attachedPlotter = nullptr;
}

float Modulation::calcIntensityValue(float calculatedModulationValue) const noexcept
{
	switch (modulationMode)
	{
	case PitchMode:		return calcPitchIntensityValue(calculatedModulationValue);
	case GainMode:		return calcGainIntensityValue(calculatedModulationValue);
	case PanMode:		return calcPanIntensityValue(calculatedModulationValue);
	case GlobalMode:	return calcGlobalIntensityValue(calculatedModulationValue);
	default: jassertfalse; return 0.0f;
	}
}


float Modulation::calcGainIntensityValue(float calculatedModulationValue) const noexcept
{
	return (1.0f - getIntensity()) + (getIntensity() * calculatedModulationValue);
}

float Modulation::calcPitchIntensityValue(float calculatedModulationValue) const noexcept
{
	return getIntensity() * calculatedModulationValue;
}

float Modulation::calcPanIntensityValue(float calculatedModulationValue) const noexcept
{
	return getIntensity() * calculatedModulationValue;
}

float Modulation::calcGlobalIntensityValue(float calculatedModulationValue) const noexcept
{
	return getIntensity() * calculatedModulationValue;
}

void Modulation::applyModulationValue(float calculatedModulationValue, float &destinationValue) const noexcept
{
	destinationValue *= calculatedModulationValue;
}

void Modulation::setIntensity(float newIntensity) noexcept
{
	intensity = newIntensity;

	smoothedIntensity.setValue(newIntensity);
}

void Modulation::setIntensityFromSlider(float sliderValue) noexcept
{
	switch (modulationMode)
	{
	case GainMode:	setIntensity(sliderValue); break;
	case PitchMode:	setIntensity(PitchConverters::octaveRangeToSignedNormalisedRange(sliderValue)); break;
	case PanMode:	setIntensity(sliderValue / 100.0f); break;
	case GlobalMode: setIntensity(sliderValue); break;
    default: jassertfalse; break;
	}
}

bool Modulation::isBipolar() const noexcept
{
	return bipolar;
}

void Modulation::setIsBipolar(bool shouldBeBiPolar) noexcept
{
	jassert(modulationMode == PitchMode || 
			modulationMode == PanMode ||
			modulationMode == GlobalMode);

	bipolar = shouldBeBiPolar;
}

float Modulation::getInitialValue() const noexcept
{
	// Pitch mode is converted to 0.5...2.0 so it's still 1.0f;
	return getMode() == PanMode ? 0.0f : 1.0f;
}

float Modulation::getIntensity() const noexcept
{
	return intensity;
}

float Modulation::getDisplayIntensity() const noexcept
{

	switch (modulationMode)
	{
	case GainMode:			return intensity;
	case PitchMode:			return intensity * 12.0f; // return (log(intensity) / log(2.0f)) * 12.0f;
	case PanMode:			return intensity * 100.0f;
	case GlobalMode:		return intensity;
	default:				jassertfalse; return 0.0f;
	}
}

void Modulation::setPlotter(Plotter *targetPlotter)
{
	attachedPlotter = targetPlotter;

	if (attachedPlotter != nullptr)
	{
		attachedPlotter->setMode((Plotter::Mode)(int)getMode()); // ugly as f***
		
		auto modChain = dynamic_cast<ModulatorChain*>(this);

		if (modChain == nullptr)
			modChain = dynamic_cast<ModulatorChain*>(ProcessorHelpers::findParentProcessor(dynamic_cast<Modulator*>(this), false));

		if (modChain != nullptr)
		{
			attachedPlotter->setYConverter(modChain->getTableValueConverter());
		}
	}
}

bool Modulation::isPlotted() const
{
	return attachedPlotter.getComponent() != nullptr;
}

void Modulation::pushPlotterValues(const float* b, int startSample, int numSamples)
{
	if (attachedPlotter.getComponent() != nullptr && shouldUpdatePlotter())
	{
		attachedPlotter.getComponent()->pushLockFree(b, startSample, numSamples);
	}
}

Modulator::Modulator(MainController *mc, const String &id_, int numVoices) :
	Processor(mc, id_, numVoices),
	colour(Colour(0x00000000))
{		
};

Modulator::~Modulator()
{
	

	masterReference.clear();
}

void Modulator::setColour(Colour c)
{

	colour = c;

	for (int i = 0; i < getNumChildProcessors(); i++)
	{
		dynamic_cast<Modulator*>(getChildProcessor(i))->setColour(c.withMultipliedAlpha(0.8f));
	}
};;;

void TimeModulation::applyTimeModulation(float* destinationBuffer, int startIndex, int samplesToCopy)
{
	float *dest = destinationBuffer + startIndex;
	float *mod = internalBuffer.getWritePointer(0, startIndex);

	if (smoothedIntensity.isSmoothing())
	{
		float* smoothedIntensityValues = (float*)alloca(sizeof(float) * samplesToCopy);

		int numLoop = samplesToCopy;
		float* loopPtr = smoothedIntensityValues;

		while (--numLoop >= 0)
		{
			*loopPtr++ = smoothedIntensity.getNextValue();
		}

		switch (modulationMode)
		{
		case GainMode: applyGainModulation(mod, dest, 1.0f, smoothedIntensityValues, samplesToCopy); break;
		case PitchMode: applyPitchModulation(mod, dest, 1.0f, smoothedIntensityValues, samplesToCopy); break;
		case PanMode:	applyPanModulation(mod, dest, 1.0f, smoothedIntensityValues, samplesToCopy); break;
		case GlobalMode: applyGlobalModulation(mod, dest, 1.0f, smoothedIntensityValues, samplesToCopy); break;
            default: break;
		}
	}
	else
	{
		switch (modulationMode)
		{
		case GainMode:	applyGainModulation(mod, dest, getIntensity(), samplesToCopy); break;
		case PitchMode: applyPitchModulation(mod, dest, getIntensity(), samplesToCopy); break;
		case PanMode:	applyPanModulation(mod, dest, getIntensity(), samplesToCopy); break;
		case GlobalMode:	applyGlobalModulation(mod, dest, getIntensity(), samplesToCopy);
            break;
        default: break;
		}
	}

	
}

const float * TimeModulation::getCalculatedValues(int /*voiceIndex*/)
{
	return internalBuffer.getReadPointer(0);
}

#pragma warning (push)
#pragma warning (disable: 4589)

TimeModulation::TimeModulation(Mode m) :
    Modulation(m),
	internalBuffer(0, 0)
{
}

#pragma warning (pop)

void TimeModulation::prepareToModulate(double sampleRate, int /*samplesPerBlock*/)
{
	constexpr double ratio = 1.0 / (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	controlRate = sampleRate * ratio;

	smoothedIntensity.setValueAndRampTime(getIntensity(), controlRate, 0.05);

	jassert(isInitialized());
	
}

bool TimeModulation::isInitialized() { return getProcessor()->getSampleRate() != -1.0f; };

void TimeModulation::applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept
{
	const float a = 1.0f - fixedIntensity;

#if 0
	constexpr int BlockSize = 32;

	BlockDivider<BlockSize> divider;

	using SSEType = dsp::SIMDRegister<float>;

	jassert(SSEType::isSIMDAligned(calculatedModulationValues) == SSEType::isSIMDAligned(destinationValues));

	float rampValue = *calculatedModulationValues * fixedIntensity + a;

	while (numValues > 0)
	{
		bool calculateNew;
		int subBlockSize = divider.cutBlock(numValues, calculateNew, calculatedModulationValues);

		if (subBlockSize == 0)
		{
			calculatedModulationValues += BlockSize;
			const float end = *(calculatedModulationValues-1) * fixedIntensity + a;
			constexpr float ratio = 1.0f / (float)BlockSize;
			const float delta1 = (end - rampValue) * ratio;

			AlignedSSERamper<BlockSize> ramper(destinationValues);
			ramper.ramp(rampValue, delta1);

			rampValue = end;
			destinationValues += BlockSize;
		}
		else
		{
			while (--subBlockSize >= 0)
			{
				*destinationValues++ *= *calculatedModulationValues++ * fixedIntensity + a;
			}
		}
	}
#endif

	FloatVectorOperations::multiply(calculatedModulationValues, fixedIntensity, numValues);
	FloatVectorOperations::add(calculatedModulationValues, a, numValues);
	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);
}

void TimeModulation::applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, const float *intensityValues, int numValues) const noexcept
{
	while (--numValues >= 0)
	{
		const float intensityValue = fixedIntensity * *intensityValues++;
		const float a = 1.0f - intensityValue;

		*destinationValues++ *= (a + intensityValue * *calculatedModulationValues++);
	}


#if 0
	const float a = 1.0f - fixedIntensity;

	constexpr int BlockSize = 32;

	BlockDivider<BlockSize> divider;

	using SSEType = dsp::SIMDRegister<float>;

	jassert(SSEType::isSIMDAligned(calculatedModulationValues) == SSEType::isSIMDAligned(destinationValues) == SSEType::isSIMDAligned(intensityValues));

	float intensityRampValue = fixedIntensity * *intensityValues;
	float invIntensityRampValue = 1.0f - intensityRampValue;

	float rampValue = *calculatedModulationValues * intensityRampValue + invIntensityRampValue;

	while (numValues > 0)
	{
		bool calculateNew;
		int subBlockSize = divider.cutBlock(numValues, calculateNew, calculatedModulationValues);

		if (subBlockSize == 0)
		{
			calculatedModulationValues += BlockSize;
			intensityValues += BlockSize;

			intensityRampValue = fixedIntensity * *(intensityValues-1);
			invIntensityRampValue = 1.0f - intensityRampValue;


			const float end = *(calculatedModulationValues-1) * intensityRampValue + invIntensityRampValue;

			constexpr float ratio = 1.0f / (float)BlockSize;
			const float delta1 = (end - rampValue) * ratio;

			AlignedSSERamper<BlockSize> ramper(destinationValues);
			ramper.ramp(rampValue, delta1);

			rampValue = end;
			destinationValues += BlockSize;
		}
		else
		{
			while (--subBlockSize >= 0)
			{
				const float intensityValue = fixedIntensity * *intensityValues++;
				const float invIntensity = 1.0f - intensityValue;

				*destinationValues++ *= *calculatedModulationValues++ * intensityValue + invIntensity;
			}
		}
	}
#endif
}

void TimeModulation::applyPitchModulation(float* calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept
{
	// input: modValues (0 ... 1), intensity (-1...1)

	applyIntensityForPitchValues(calculatedModulationValues, fixedIntensity, numValues);
	
	Modulation::PitchConverters::normalisedRangeToPitchFactor(calculatedModulationValues, numValues);

	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);

#if 0
	const float a = fixedIntensity - 1.0f;

	FloatVectorOperations::multiply(calculatedModulationValues, a, numValues);
	FloatVectorOperations::add(calculatedModulationValues, 1.0f, numValues);
	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);
#endif
}

void TimeModulation::applyPitchModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, const float *intensityValues, int numValues) const noexcept
{
	applyIntensityForPitchValues(calculatedModulationValues, fixedIntensity, intensityValues, numValues);

	Modulation::PitchConverters::normalisedRangeToPitchFactor(calculatedModulationValues, numValues);

	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);

	
}


void TimeModulation::applyGlobalModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, float* intensityValues, int numValues) const noexcept
{
	if (isBipolar())
		FloatVectorOperations::copy(destinationValues, calculatedModValues, numValues);
	else
		applyGainModulation(calculatedModValues, destinationValues, fixedIntensity, intensityValues, numValues);
}

void TimeModulation::applyGlobalModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, int numValues) const noexcept
{
	if (isBipolar())
	{
		FloatVectorOperations::copy(destinationValues, calculatedModValues, numValues);
	}
	else
		applyGainModulation(calculatedModValues, destinationValues, fixedIntensity, numValues);
}

void TimeModulation::applyIntensityForGainValues(float* calculatedModulationValues, float fixedIntensity, int numValues) const
{
	const float a = 1.0f - fixedIntensity;

	while (--numValues >= 0)
	{
        const float modValue = *calculatedModulationValues * fixedIntensity + a;
        
		*calculatedModulationValues++ = modValue;
	}
}


void TimeModulation::applyIntensityForGainValues(float* calculatedModulationValues, float fixedIntensity, const float* intensityValues, int numValues) const
{
	while (--numValues >= 0)
	{
		const float intensityValue = fixedIntensity * *intensityValues++;
		const float invIntensity = 1.0f - intensityValue;
        const float modValue = intensityValue * *calculatedModulationValues + invIntensity;
		*calculatedModulationValues++ = modValue;
        
	}
}

void TimeModulation::applyIntensityForPitchValues(float* calculatedModulationValues, float fixedIntensity, int numValues) const
{
	float* modValues = calculatedModulationValues;
	int numLoop = numValues;

	if (isBipolar())
	{
		while (--numLoop >= 0)
		{
			const float bipolarModValue = *modValues * 2.0f - 1.0f;
			*modValues++ = bipolarModValue * fixedIntensity;
		}

		//FloatVectorOperations::multiply(calculatedModulationValues, 2.0f, numValues);
		//FloatVectorOperations::add(calculatedModulationValues, -1.0f, numValues);
		//FloatVectorOperations::multiply(calculatedModulationValues, fixedIntensity, numValues);

		// modvalues (-1 ... -intensity ... 0 ... +intensity ... 1)
	}
	else
	{
		FloatVectorOperations::multiply(calculatedModulationValues, fixedIntensity, numValues);

		// modValues ( 0 ... +-intensity ... 1 )
	}

	
}


void TimeModulation::applyIntensityForPitchValues(float* calculatedModulationValues, float fixedIntensity, const float* intensityValues, int numValues) const
{
	float* modValues = calculatedModulationValues;
	int numLoop = numValues;

	// input: modValues (0 ... 1), intensity (-1 ... 1), intensityValues (0.5 ... 2.0)

	if (isBipolar())
	{
		while (--numLoop >= 0)
		{
			const float intensityValue = *intensityValues++ * fixedIntensity;
			const float bipolarModValue = 2.0f * *modValues - 1.0f;
			*modValues++ = bipolarModValue * intensityValue;
		}

		//FloatVectorOperations::multiply(intensityValues, fixedIntensity, numValues);
		//FloatVectorOperations::multiply(calculatedModulationValues, 2.0f, numValues);
		//FloatVectorOperations::add(calculatedModulationValues, -1.0f, numValues);
		//FloatVectorOperations::multiply(calculatedModulationValues, intensityValues, numValues);
	}
	else
	{
		while (--numValues >= 0)
		{
			const float intensityValue = *intensityValues++ * fixedIntensity;
			*modValues++ *= intensityValue;
		}

		//FloatVectorOperations::multiply(intensityValues, fixedIntensity, numValues);
		//FloatVectorOperations::multiply(calculatedModulationValues, intensityValues, numValues);
	}

	
}


void TimeModulation::applyPanModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, float* intensityValues, int numValues) const noexcept
{
	// input: modValues (0 ... 1), intensity (-1 ... 1), intensityValues (0.0 ... 1.0)

	if (isBipolar())
	{
		while (--numValues >= 0)
		{

			const float bipolarModValue = 2.0f * *calculatedModValues++ - 1.0f;
			*destinationValues++ += bipolarModValue * fixedIntensity * *intensityValues++;
		}
	}
	else
	{
		while (--numValues >= 0)
		{
			const float modValue = fixedIntensity * *intensityValues++ * *calculatedModValues++;
			*destinationValues++ += modValue;
		}
	}

	

}

void TimeModulation::applyPanModulation(float * calculatedModValues, float * destinationValues, float fixedIntensity, int numValues) const noexcept
{
	// input: modValues (0 ... 1), intensity (-1 ... 1)
	// output: 0 -> 0.5

	if (isBipolar())
	{
		while (--numValues >= 0)
		{
			
			const float bipolarModValue = 2.0f * *calculatedModValues++ - 1.0f;
			*destinationValues++ += bipolarModValue * fixedIntensity;
		}
	}
	else
	{
		while (--numValues >= 0)
		{
			const float modValue = *calculatedModValues++ * fixedIntensity;
			*destinationValues++ += modValue;
		}
	}
}

#pragma warning( push )
#pragma warning( disable: 4589 )


VoiceStartModulator::VoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m) :
		VoiceModulation(numVoices, m),
		Modulator(mc, id, numVoices),
		Modulation(m),
		unsavedValue(1.0f)
{
	voiceValues.insertMultiple(0, 1.0f, numVoices);
};

EnvelopeModulator::EnvelopeModulator(MainController *mc, const String &id, int voiceAmount_, Modulation::Mode m):
	Modulator(mc, id, voiceAmount_),
	TimeModulation(m),
	Modulation(m),
	VoiceModulation(voiceAmount_, m)
{
	parameterNames.add("Monophonic");
	parameterNames.add("Retrigger");
};

#pragma warning( pop )

Processor *VoiceStartModulatorFactoryType::createProcessor(int typeIndex, const String &id)
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
		case constantModulator: return new ConstantModulator(m, id, numVoices, mode);
		case velocityModulator: return new VelocityModulator(m, id, numVoices, mode);
		case keyModulator:		return new KeyModulator(m, id, numVoices, mode);
		case randomModulator: return new RandomModulator(m, id, numVoices, mode);
		case globalVoiceStartModulator:	return new GlobalVoiceStartModulator(m, id, numVoices, mode);
		case globalStaticTimeVariantModulator: return new GlobalStaticTimeVariantModulator(m, id, numVoices, mode);
		case arrayModulator:	return new ArrayModulator(m, id, numVoices, mode);
		case scriptVoiceStartModulator:	return new JavascriptVoiceStartModulator(m, id, numVoices, mode);
		default: jassertfalse; return nullptr;
	}
};

Processor *TimeVariantModulatorFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case lfoModulator:					return new LfoModulator(m, id, mode);
	case controlModulator:				return new ControlModulator(m, id, mode);
	case pitchWheel:					return new PitchwheelModulator(m, id, mode);
	case macroModulator:				return new MacroModulator(m, id, mode);
	case globalTimeVariantModulator:	return new GlobalTimeVariantModulator(m, id, mode);
	case scriptTimeVariantModulator:	return new JavascriptTimeVariantModulator(m, id, mode);
	default: jassertfalse;				return nullptr;

	}
};

Processor *EnvelopeModulatorFactoryType::createProcessor(int typeIndex, const String &id) 
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case simpleEnvelope:	return new SimpleEnvelope(m, id, numVoices, mode);
	case ahdsrEnvelope:		return new AhdsrEnvelope(m, id, numVoices, mode);
	case tableEnvelope:		return new TableEnvelope(m, id, numVoices, mode);
	case scriptEnvelope:	return new JavascriptEnvelopeModulator(m, id, numVoices, mode);
	case mpeModulator:		return new MPEModulator(m, id, numVoices, mode);
	case voiceKillEnvelope: return new ScriptnodeVoiceKiller(m, id, numVoices);
	default: jassertfalse;	return nullptr;

	}
};

void VoiceModulation::allNotesOff()
{
	for (int i = 0; i < polyManager.getVoiceAmount(); i++) 
		stopVoice(i);
}

void VoiceModulation::PolyphonyManager::setCurrentVoice(int newCurrentVoice) noexcept
{
	jassert(currentVoice == -1);
	jassert(currentVoice < voiceAmount);

	currentVoice = newCurrentVoice;
}

void VoiceModulation::PolyphonyManager::setLastStartedVoice(int voiceIndex)
{
	lastStartedVoice = voiceIndex;
}

int VoiceModulation::PolyphonyManager::getLastStartedVoice() const
{
	return lastStartedVoice;
}

void TimeVariantModulator::render(float* monoModulationValues, float* scratchBuffer, int startSample, int numSamples)
{
	// applyTimeModulation will not work correctly if it's going to be calculated in place...
	jassert(monoModulationValues != scratchBuffer);

	setScratchBuffer(scratchBuffer, startSample + numSamples);
	calculateBlock(startSample, numSamples);

	applyTimeModulation(monoModulationValues, startSample, numSamples);
	lastConstantValue = monoModulationValues[startSample];

#if ENABLE_ALL_PEAK_METERS
	const float displayValue = internalBuffer.getSample(0, startSample);
	pushPlotterValues(internalBuffer.getReadPointer(0), startSample, numSamples);

	setOutputValue(displayValue);
#endif
}

void Modulation::PitchConverters::normalisedRangeToPitchFactor(float* rangeValues, int numValues)
{
	if (numValues > 1)
	{
#if HISE_ENABLE_FULL_CONTROL_RATE_PITCH_MOD

		bool hasDeltaSignChange = false;

		float prevValue = rangeValues[0];
		float delta = 0.0f;

		for (int i = 1; i < numValues; i++)
		{
			auto thisValue = rangeValues[i];
			auto thisDelta = thisValue - prevValue;

			hasDeltaSignChange |= (delta != 0.0f && (hmath::sign(thisDelta) != hmath::sign(delta)));
			delta = thisDelta;
			prevValue = thisValue;
		}

#else
		auto hasDeltaSignChange = false;
#endif

		if (hasDeltaSignChange)
		{
			DBG("USE RANGE");
			
			for (int i = 0; i < numValues; i++)
				rangeValues[i] = normalisedRangeToPitchFactor(rangeValues[i]);
		}
		else
		{
			float startValue = normalisedRangeToPitchFactor(rangeValues[0]);
			const float endValue = normalisedRangeToPitchFactor(rangeValues[numValues - 1]);
			float delta = (endValue - startValue);


			if (delta < 0.0003f)
			{
				FloatVectorOperations::fill(rangeValues, (startValue + endValue) * 0.5f, numValues);
			}
			else
			{
				delta /= (float)numValues;

				while (--numValues >= 0)
				{
					*rangeValues++ = startValue;
					startValue += delta;
				}
			}
		}
	}
	else if (numValues == 1)
	{
		rangeValues[0] = normalisedRangeToPitchFactor(rangeValues[0]);
	}
}

} // namespace hise
