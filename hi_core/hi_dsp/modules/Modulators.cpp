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

Modulator::Modulator(MainController *mc, const String &id_) :
	Processor(mc, id_),
	attachedPlotter(nullptr),
	colour(Colour(0x00000000))
{		
};

Modulator::~Modulator()
{
	if (attachedPlotter != nullptr)
	{
		attachedPlotter->removePlottedModulator(this);
	}

	masterReference.clear();
}

void Modulator::setColour(Colour c)
{

	colour = c;

	for (int i = 0; i < getNumChildProcessors(); i++)
	{
		dynamic_cast<Modulator*>(getChildProcessor(i))->setColour(c.withMultipliedAlpha(0.8f));
	}
}

void Modulator::setPlotter(Plotter *targetPlotter)
{
	attachedPlotter = targetPlotter;
	
};

bool Modulator::isPlotted() const 
{
	return attachedPlotter.getComponent() != nullptr; 
};

void Modulator::addValueToPlotter(float v) const
{
	if(attachedPlotter.getComponent() != nullptr) 
	{
		attachedPlotter.getComponent()->addValue(this, v);
	}
};

void TimeModulation::renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	// Save the values for later
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	calculateBlock(startSample, numSamples);

	if (shouldUpdatePlotter()) updatePlotter(internalBuffer, startIndex, samplesToCopy);

	applyTimeModulation(buffer, startIndex, samplesToCopy);
}

void TimeModulation::applyTimeModulation(AudioSampleBuffer &buffer, int startIndex, int samplesToCopy)
{
	float *dest = buffer.getWritePointer(0, startIndex);
	float *mod = internalBuffer.getWritePointer(0, startIndex);

	FloatVectorOperations::clip(mod, mod, 0.0f, 1.0f, samplesToCopy);

	switch (modulationMode)
	{
	case GainMode: applyGainModulation(mod, dest, getIntensity(), samplesToCopy); break;
	case PitchMode: applyPitchModulation(mod, dest, getIntensity(), samplesToCopy); break;
	}
}

const float * TimeModulation::getCalculatedValues(int /*voiceIndex*/)
{
	return internalBuffer.getReadPointer(0);
}

void TimeModulation::prepareToModulate(double /*sampleRate*/, int samplesPerBlock)
{
	ProcessorHelpers::increaseBufferIfNeeded(internalBuffer, samplesPerBlock);

	jassert(isInitialized());
}

bool TimeModulation::isInitialized() { return getProcessor()->getSampleRate() != -1.0f; };

void TimeModulation::applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept
{
	const float a = 1.0f - fixedIntensity;

	FloatVectorOperations::multiply(calculatedModulationValues, fixedIntensity, numValues);
	FloatVectorOperations::add(calculatedModulationValues, a, numValues);
	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);
}

void TimeModulation::applyGainModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, float *intensityValues, int numValues) const noexcept
{
	FloatVectorOperations::multiply(intensityValues, fixedIntensity, numValues);
	FloatVectorOperations::multiply(calculatedModulationValues, intensityValues, numValues);
	FloatVectorOperations::multiply(intensityValues, -1.0f, numValues);
	FloatVectorOperations::add(intensityValues, 1.0f, numValues);
	FloatVectorOperations::add(calculatedModulationValues, intensityValues, numValues);
	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);
}

void TimeModulation::applyPitchModulation(float* calculatedModulationValues, float *destinationValues, float fixedIntensity, int numValues) const noexcept
{
	// input: modValues (0 ... 1), intensity (-1...1)

	if (isBipolar())
	{
		FloatVectorOperations::multiply(calculatedModulationValues, 2.0f, numValues);
		FloatVectorOperations::add(calculatedModulationValues, -1.0f, numValues);
		FloatVectorOperations::multiply(calculatedModulationValues, fixedIntensity, numValues);

		// modvalues (-1 ... -intensity ... 0 ... +intensity ... 1)
	}
	else
	{
		FloatVectorOperations::multiply(calculatedModulationValues, fixedIntensity, numValues);

		// modValues ( 0 ... +-intensity ... 1 )
	}

	Modulation::PitchConverters::normalisedRangeToPitchFactor(calculatedModulationValues, numValues);
	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);

#if 0
	const float a = fixedIntensity - 1.0f;

	FloatVectorOperations::multiply(calculatedModulationValues, a, numValues);
	FloatVectorOperations::add(calculatedModulationValues, 1.0f, numValues);
	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);
#endif
}

void TimeModulation::applyPitchModulation(float *calculatedModulationValues, float *destinationValues, float fixedIntensity, float *intensityValues, int numValues) const noexcept
{
	// input: modValues (0 ... 1), intensity (-1 ... 1), intensityValues (0.5 ... 2.0)

	if (isBipolar())
	{
		FloatVectorOperations::multiply(intensityValues, fixedIntensity, numValues);
		FloatVectorOperations::multiply(calculatedModulationValues, 2.0f, numValues);
		FloatVectorOperations::add(calculatedModulationValues, -1.0f, numValues);
		FloatVectorOperations::multiply(calculatedModulationValues, intensityValues, numValues);
	}
	else
	{
		FloatVectorOperations::multiply(intensityValues, fixedIntensity, numValues);
		FloatVectorOperations::multiply(calculatedModulationValues, intensityValues, numValues);
	}

	Modulation::PitchConverters::normalisedRangeToPitchFactor(calculatedModulationValues, numValues);
	FloatVectorOperations::multiply(destinationValues, calculatedModulationValues, numValues);
}

void TimeModulation::initializeBuffer(AudioSampleBuffer &bufferToBeInitialized, int startSample, int numSamples)
{
	jassert(bufferToBeInitialized.getNumChannels() == 1);
	jassert(bufferToBeInitialized.getNumSamples() >= startSample + numSamples);

	float *writePointer = bufferToBeInitialized.getWritePointer(0, startSample);

	FloatVectorOperations::fill(writePointer, 1.0f, numSamples);
}

#pragma warning( push )
#pragma warning( disable: 4589 )


VoiceStartModulator::VoiceStartModulator(MainController *mc, const String &id, int numVoices, Modulation::Mode m) :
		VoiceModulation(numVoices, m),
		Modulator(mc, id),
		Modulation(m),
		unsavedValue(1.0f)
{
	voiceValues.insertMultiple(0, 1.0f, numVoices);
};

EnvelopeModulator::EnvelopeModulator(MainController *mc, const String &id, int voiceAmount_, Modulation::Mode m):
	Modulator(mc, id),
	Modulation(m),
	TimeModulation(m),
	VoiceModulation(voiceAmount_, m)
{};

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
	default: jassertfalse;	return nullptr;

	}
};

void VoiceModulation::allNotesOff()
{
	for (int i = 0; i < polyManager.getVoiceAmount(); i++) stopVoice(i);
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
