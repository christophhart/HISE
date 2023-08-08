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

SET_DOCUMENTATION(SineSynth)
{
	SET_DOC_NAME(SineSynth);

	addLine("A simple and lightweight sine wave generator. " \
		    "It can be used to drive a FM Synthesiser, or stacked together for Additive Synthesis or used as simple enhancement of another sound.");

	addLine("It has two operating modes for the pitch definition :");
	addLine("");
	addLine("- **Musical**: in octaves / semitones.");
	addLine("- **Harmonics**: as harmonics compared to the root frequency.Use this mode for additive synthesis(it will allow to define the harmonic structure of the resulting sound more clearly.)");
	addLine("");
	addLine("> It also has a internal Wave - Shaper effect, that allows to quickly add some harmonics to dirten up the sound.");

	ADD_PARAMETER_DOC_WITH_NAME(OctaveTranspose, "Octave Transpose",
		"If the mode is set to Musical, this defines the coarse frequency.");

	ADD_PARAMETER_DOC_WITH_NAME(SemiTones, "Semitones",
		"If the mode is set to Musical, this defines the fine frequency in semitones.");

	ADD_PARAMETER_DOC_WITH_NAME(UseFreqRatio, "Use Frequency Ratio",
		"Toggles between the two modes for the pitch definition.");

	ADD_PARAMETER_DOC_WITH_NAME(CoarseFreqRatio, "Coarse Ratio",
		"If the mode is set to Harmonics, this defines the harmonic index(1 being the root frequency).");

	ADD_PARAMETER_DOC_WITH_NAME(FineFreqRatio, "Fine Ratio",
		"If the mode is set to Harmonics, this defines the fine frequency(as factor).");

	ADD_PARAMETER_DOC_WITH_NAME(SaturationAmount, "Saturation",
		"The saturation amount for the internal wave shaper.Use this to quickly add some harmonics.");

}

    
SineSynth::SineSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices),
	octaveTranspose((int)getDefaultValue(OctaveTranspose)),
	semiTones((int)getDefaultValue(SemiTones)),
	useRatio(false),
	fineRatio(getDefaultValue(FineFreqRatio)),
	coarseRatio(getDefaultValue(CoarseFreqRatio)),
	saturationAmount(getDefaultValue(SaturationAmount))
{
	finaliseModChains();

	parameterNames.add("OctaveTranspose");
	parameterNames.add("SemiTones");
	parameterNames.add("UseFreqRatio");
	parameterNames.add("CoarseFreqRatio");
	parameterNames.add("FineFreqRatio");
	parameterNames.add("SaturationAmount");

	for (int i = 0; i < numVoices; i++) addVoice(new SineSynthVoice(this));
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
