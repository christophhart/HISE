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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

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

	const float *voicePitchValues = getVoicePitchValues();
	const float *modValues = getVoiceGainValues(startSample, numSamples);

	float saturation = static_cast<SineSynth*>(getOwnerSynth())->saturationAmount;
	float *leftValues = voiceBuffer.getWritePointer(0, startSample);
	
	if (isPitchModulationActive())
	{
		voicePitchValues += startSample;

		while (--numSamples >= 0)
		{
			int index = (int)voiceUptime;

			float v1 = sinTable[index & 2047];
			float v2 = sinTable[(index + 1) & 2047];

			const float alpha = float(voiceUptime) - (float)index;
			const float invAlpha = 1.0f - alpha;

			const float currentSample = invAlpha * v1 + alpha * v2;

			*leftValues++ = currentSample;

			const double thisPitchValue = *voicePitchValues++;

			voiceUptime += (uptimeDelta * thisPitchValue);

		}
	}
	else
	{
		while (numSamples > 4)
		{
			for (int i = 0; i < 4; i++)
			{
				int index = (int)voiceUptime;

				float v1 = sinTable[index & 2047];
				float v2 = sinTable[(index + 1) & 2047];

				const float alpha = float(voiceUptime) - (float)index;
				const float invAlpha = 1.0f - alpha;

				const float currentSample = invAlpha * v1 + alpha * v2;

				*leftValues++ = currentSample;

				voiceUptime += uptimeDelta;
			}

			numSamples -= 4;
		}

		while (numSamples > 0)
		{
			int index = (int)voiceUptime;

			float v1 = sinTable[index & 2047];
			float v2 = sinTable[(index + 1) & 2047];

			const float alpha = float(voiceUptime) - (float)index;
			const float invAlpha = 1.0f - alpha;

			const float currentSample = invAlpha * v1 + alpha * v2;

			*leftValues++ = currentSample;

			voiceUptime += uptimeDelta;

			numSamples--;
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

	FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);

	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);

	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
	FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startIndex), modValues + startIndex, samplesToCopy);
}
