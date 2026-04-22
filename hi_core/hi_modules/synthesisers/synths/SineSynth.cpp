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

hise::ProcessorMetadata SineSynth::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ModulatorSynth::createBaseMetadata()
		.withStandardMetadata<SineSynth>()
		.withDescription("A lightweight sine wave generator for FM synthesis, additive synthesis, or adding subtle harmonics to other sounds.")
		.withParameter(Par(OctaveTranspose)
			.withId("OctaveTranspose")
			.withDescription("Coarse pitch offset in octaves when using musical tuning mode")
			.withSliderMode(HiSlider::Discrete, Range(-5.0, 5.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(SemiTones)
			.withId("SemiTones")
			.withDescription("Fine pitch offset in semitones when using musical tuning mode")
			.withSliderMode(HiSlider::Discrete, Range(-12.0, 12.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(UseFreqRatio)
			.withId("UseFreqRatio")
			.withDescription("Switches between musical (octave/semitone) and harmonic ratio tuning modes")
			.asToggle()
			.withDefault(0.0f))
		.withParameter(Par(CoarseFreqRatio)
			.withId("CoarseFreqRatio")
			.withDescription("Harmonic index as a frequency multiplier where 1 is the root frequency")
			.withSliderMode(HiSlider::Discrete, Range(-5.0, 16.0, 1.0))
			.withDefault(1.0f))
		.withParameter(Par(FineFreqRatio)
			.withId("FineFreqRatio")
			.withDescription("Fine frequency offset as a fractional multiplier in harmonic tuning mode")
			.withSliderMode(HiSlider::Linear, Range(0.0, 1.0))
			.withDefault(0.0f))
		.withParameter(Par(SaturationAmount)
			.withId("SaturationAmount")
			.withDescription("Amount of waveshaping saturation applied to the sine wave to add harmonics")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(0.0f))
		;
}
    
SineSynth::SineSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	metadataInitialised(updateParameterSlots()),
	octaveTranspose((int)getDefaultValue(OctaveTranspose)),
	semiTones((int)getDefaultValue(SemiTones)),
	useRatio(false),
	fineRatio(getDefaultValue(FineFreqRatio)),
	coarseRatio(getDefaultValue(CoarseFreqRatio)),
	saturationAmount(getDefaultValue(SaturationAmount))
{
	finaliseModChains();

	for (int i = 0; i < numVoices; i++) 
		addVoice(new SineSynthVoice(this));

	addSound(new SineWaveSound());
}

ProcessorEditorBody* SineSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new SineSynthBody(parentEditor);
#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


float const * SineSynth::getSaturatedTableValues()
{
	for (int i = 0; i < 128; i++)
	{
		const float sinValue = sin((float)i / 64.0f * float_Pi);

		saturatedTableValues[i] = saturator.getSaturatedSample(sinValue);
	}

	return saturatedTableValues;
}

void SineSynthVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

	float saturation = static_cast<SineSynth*>(getOwnerSynth())->saturationAmount;
	float *leftValues = voiceBuffer.getWritePointer(0, startSample);
	const auto& sinTable = table.get();

	if (auto voicePitchValues = getOwnerSynth()->getPitchValuesForVoice())
	{
		voicePitchValues += startSample;

		while (--numSamples >= 0)
		{
			*leftValues++ = sinTable.getInterpolatedValue(voiceUptime);
			const double thisPitchValue = *voicePitchValues++;
			voiceUptime += (uptimeDelta * thisPitchValue);
		}
	}
	else
	{
		while (--numSamples >= 0)
		{
			*leftValues++ = sinTable.getInterpolatedValue(voiceUptime);
			voiceUptime += uptimeDelta;
		}
	}

	if (saturation != 0.0f)
	{
		if (saturation == 1.0f) saturation = 0.99f; // 1.0f makes it silent, so this is the best bugfix in the world...

		const float saturationAmount = 2.0f * saturation / (1.0f - saturation);

		// Once from the top...

		numSamples = samplesToCopy;
		startSample = startIndex;

		leftValues = voiceBuffer.getWritePointer(0, startSample);

		for (int i = 0; i < numSamples; i++)
		{
			const float currentSample = leftValues[i];
			const float saturatedSample = (1.0f + saturationAmount) * currentSample / (1.0f + saturationAmount * fabsf(currentSample));

			leftValues[i] = saturatedSample;
		}
	}

	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
	}
	else
	{
		const float gainValue = getOwnerSynth()->getConstantGainModValue();
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), gainValue, samplesToCopy);
	}
		
	FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);
}

} // namespace hise
