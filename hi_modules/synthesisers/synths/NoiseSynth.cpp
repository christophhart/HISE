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

NoiseSynth::NoiseSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices)
{
	finaliseModChains();

	for (int i = 0; i < numVoices; i++) addVoice(new NoiseVoice(this));
	addSound(new NoiseSound());

#if !HI_RUN_UNIT_TESTS
	modChains[BasicChains::PitchChain].getChain()->setBypassed(true);
#endif

	signalType = DiracTrain;
}

ProcessorEditorBody* NoiseSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new EmptyProcessorEditorBody(parentEditor);
#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

void NoiseVoice::calculateBlock(int startSample, int numSamples)
{
	const int startIndex = startSample;
	const int samplesToCopy = numSamples;

#if HI_RUN_UNIT_TESTS

	auto signalType = static_cast<NoiseSynth*>(getOwnerSynth())->getTestSignal();

	switch (signalType)
	{
	case hise::NoiseSynth::Normal:
	{
		while (--numSamples >= 0)
		{
			const float currentSample = getNextValue();

			// Stereo mode assumed
			voiceBuffer.setSample(0, startSample, currentSample);

			voiceUptime += uptimeDelta;

			++startSample;
		}
		break;
	}
	case hise::NoiseSynth::DC:
	{
		FloatVectorOperations::fill(voiceBuffer.getWritePointer(0, startSample), 1.0f, samplesToCopy);
		break;
	}
		
	case hise::NoiseSynth::Ramp:
	{
		while (--numSamples >= 0)
		{
			const float delta1 = 1.0f / (float)INT16_MAX;
			const float uptimeToUse = (float)((int)voiceUptime % INT16_MAX);
			const float value = uptimeToUse * delta1;

			voiceBuffer.setSample(0, startSample, value);

			voiceUptime += uptimeDelta;
			++startSample;
		}

		break;
	}
	case hise::NoiseSynth::DiracTrain:
	{
		if (auto voicePitchValues = getOwnerSynth()->getPitchValuesForVoice())
		{
			voicePitchValues += startSample;

			while (--numSamples >= 0)
			{
				if (voiceUptime == 0.0 && lastUptime == -1.0)
					voiceBuffer.setSample(0, startSample, -1.0f);
				else if (voiceUptime >= 256.0)
				{
					voiceUptime -= 256.0;
					voiceBuffer.setSample(0, startSample, 1.0f);
				}
				else
					voiceBuffer.setSample(0, startSample, 0.0f);

				voiceUptime += uptimeDelta * *voicePitchValues++;
				lastUptime = voiceUptime;
				++startSample;
			}
		}
		else
		{
			while (--numSamples >= 0)
			{
				if (voiceUptime == 0.0)
					voiceBuffer.setSample(0, startSample, -1.0f);
				else if (voiceUptime >= 256.0)
				{
					voiceUptime -= 256.0;
					voiceBuffer.setSample(0, startSample, 1.0f);
				}
				else
					voiceBuffer.setSample(0, startSample, 0.0f);

				voiceUptime += uptimeDelta;
				lastUptime = voiceUptime;
				++startSample;
			}
		}

		

		break;
	}
	case hise::NoiseSynth::Square:
	{
		if (auto voicePitchValues = getOwnerSynth()->getPitchValuesForVoice())
		{
			voicePitchValues += startSample;


			while (--numSamples >= 0)
			{
				voiceUptime += uptimeDelta * *voicePitchValues++;

				auto prevUptime = (int)(voiceUptime / 128.0);

				if (prevUptime % 2 == 0)
				{
					voiceUptime -= 256.0;
					voiceBuffer.setSample(0, startSample, 1.0f);
				}
				else
				{
					voiceBuffer.setSample(0, startSample, -1.0f);
				}

				++startSample;
			}
		}
		else
		{
			while (--numSamples >= 0)
			{
				voiceUptime += uptimeDelta;

				auto prevUptime = (int)(voiceUptime / 128.0);

				if (prevUptime % 2 == 0)
				{
					voiceUptime -= 256.0;
					voiceBuffer.setSample(0, startSample, 1.0f);
				}
				else
				{
					voiceBuffer.setSample(0, startSample, -1.0f);
				}

				++startSample;
			}

		}


		break;
	}
		
	case hise::NoiseSynth::numTestSignals:
		break;
	default:
		break;
}

#else

	while (--numSamples >= 0)
	{
		const float currentSample = getNextValue();

		// Stereo mode assumed
		voiceBuffer.setSample(0, startSample, currentSample);

		voiceUptime += uptimeDelta;

		++startSample;
	}
	

#endif

	if (auto modValues = getOwnerSynth()->getVoiceGainValues())
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), modValues + startIndex, samplesToCopy);
	}
	else
	{
		float gainValue = getOwnerSynth()->getConstantGainModValue();
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startIndex), gainValue, samplesToCopy);
	}

	FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startIndex), voiceBuffer.getReadPointer(0, startIndex), samplesToCopy);
	
	getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startIndex, samplesToCopy);
}

SilentSynth::SilentSynth(MainController *mc, const String &id, int numVoices) :
	ModulatorSynth(mc, id, numVoices)
{
	finaliseModChains();

	modChains[BasicChains::GainChain].getChain()->setBypassed(true);
	modChains[BasicChains::PitchChain].getChain()->setBypassed(true);

	for (int i = 0; i < numVoices; i++) 
		addVoice(new SilentVoice(this));
	
	addSound(new SilentSound());

	getMatrix().setAllowResizing(true);
}

hise::ProcessorEditorBody* SilentSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new EmptyProcessorEditorBody(parentEditor);
#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

} // namespace hise
