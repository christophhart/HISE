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
	case PitchMode: return calcPitchIntensityValue(calculatedModulationValue);
	case GainMode: return calcGainIntensityValue(calculatedModulationValue);
	default: jassertfalse; return -1.0f;
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
	if (modulationMode == GainMode)
	{
		setIntensity(sliderValue);

	}
	else
	{
		const float thisIntensity = PitchConverters::octaveRangeToSignedNormalisedRange(sliderValue);

		setIntensity(thisIntensity);
	}
}

bool Modulation::isBipolar() const noexcept
{
	return bipolar;
}

void Modulation::setIsBipolar(bool shouldBeBiPolar) noexcept
{
	jassert(modulationMode == PitchMode);

	bipolar = shouldBeBiPolar;
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
	default:				jassertfalse; return 0.0f;
	}
}

void Modulation::setPlotter(Plotter *targetPlotter)
{
	attachedPlotter = targetPlotter;
}

bool Modulation::isPlotted() const
{
	return attachedPlotter.getComponent() != nullptr;
}

void Modulation::pushPlotterValues(const float* b, int startSample, int numSamples)
{
	if (attachedPlotter.getComponent() != nullptr && shouldUpdatePlotter())
	{
		attachedPlotter.getComponent()->addValues(b, startSample, numSamples);
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
		}
	}
	else
	{
		switch (modulationMode)
		{
		case GainMode: applyGainModulation(mod, dest, getIntensity(), samplesToCopy); break;
		case PitchMode: applyPitchModulation(mod, dest, getIntensity(), samplesToCopy); break;
		}
	}

	
}

const float * TimeModulation::getCalculatedValues(int /*voiceIndex*/)
{
	return internalBuffer.getReadPointer(0);
}

void TimeModulation::prepareToModulate(double sampleRate, int samplesPerBlock)
{
	constexpr double ratio = 1.0 / (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	controlRate = sampleRate * ratio;

	smoothedIntensity.setValueAndRampTime(getIntensity(), controlRate, 0.1);
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


void TimeModulation::applyIntensityForGainValues(float* calculatedModulationValues, float fixedIntensity, int numValues) const
{
	const float a = 1.0f - fixedIntensity;

	while (--numValues >= 0)
	{
		*calculatedModulationValues++ = *calculatedModulationValues * fixedIntensity + a;
	}
}


void TimeModulation::applyIntensityForGainValues(float* calculatedModulationValues, float fixedIntensity, const float* intensityValues, int numValues) const
{
	while (--numValues >= 0)
	{
		const float intensityValue = fixedIntensity * *intensityValues++;
		const float invIntensity = 1.0f - intensityValue;
		*calculatedModulationValues++ = intensityValue * *calculatedModulationValues + invIntensity;
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
	Modulation(m),
	TimeModulation(m),
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
	case audioFileEnvelope:				return new AudioFileEnvelope(m, id, mode);
	case globalTimeVariantModulator:	return new GlobalTimeVariantModulator(m, id, mode);
	case ccDucker:						return new CCDucker(m, id, mode);
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
	case ccEnvelope:		return new CCEnvelope(m, id, numVoices, mode);
	case scriptEnvelope:	return new JavascriptEnvelopeModulator(m, id, numVoices, mode);
	case mpeModulator:		return new MPEModulator(m, id, numVoices, mode);
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

} // namespace hise