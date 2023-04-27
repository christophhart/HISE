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


bool EffectProcessor::isSilent(AudioSampleBuffer& b, int startSample, int numSamples)
{
	float* stereo[2] = { b.getWritePointer(0, startSample), b.getWritePointer(jmin(1, b.getNumChannels()-1), startSample) };

	return ProcessData<2>(stereo, numSamples).isSilent();
}

void EffectProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	jassert(finalised);

	Processor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate >= 0.0)
	{
		auto callbackDurationMs = jmax(1.0, (double)samplesPerBlock / sampleRate * 1000.0);
		numSilentCallbacksToWait = roundToInt((double)HISE_SUSPENSION_TAIL_MS / callbackDurationMs);
	}

	isInSend = dynamic_cast<SendContainer*>(getParentProcessor(true, false)) != nullptr;

	for (auto& mc : modChains)
		mc.prepareToPlay(sampleRate, samplesPerBlock);
}

void EffectProcessor::finaliseModChains()
{
	modChains.finalise();

	for (auto& mb : modChains)
		mb.getChain()->setParentProcessor(this);

	finalised = true;
}

juce::Path VoiceEffectProcessor::getSpecialSymbol() const
{
	Path path;

	path.loadPathFromData(ProcessorIcons::polyFX, sizeof(ProcessorIcons::polyFX));
	return path;
}

bool MasterEffectProcessor::isFadeOutPending() const noexcept
{
	return softBypassState == Pending && softBypassRamper.getTargetValue() < 0.5f;
}

void MasterEffectProcessor::setSoftBypass(bool shouldBeSoftBypassed, bool useRamp/*=true*/)
{
	masterState.reset();

	if (useRamp)
	{
		softBypassRamper.setValue(shouldBeSoftBypassed ? 0.0f : 1.0f);

		if ((shouldBeSoftBypassed && softBypassState != Bypassed) ||
			(!shouldBeSoftBypassed && softBypassState != Inactive))
		{


			softBypassState = Pending;
		}

		if (softBypassRamper.getCurrentValue() == softBypassRamper.getTargetValue())
		{
			softBypassState = shouldBeSoftBypassed ? Bypassed : Inactive;
		}
	}
	else
	{
		softBypassState = shouldBeSoftBypassed ? Bypassed : Inactive;
		softBypassRamper.setValueWithoutSmoothing(shouldBeSoftBypassed ? 0.0f : 1.0f);
	}
}

} // namespace hise
