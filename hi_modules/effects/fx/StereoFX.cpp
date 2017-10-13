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

void MidSideDecoder::calculateStereoValues(float &left, float &right)
{
	const float m = (left + right) * 0.5f;
	const float s = (right - left) * width * 0.5f;

	left = m - s;
	right = m + s;
}


void MidSideDecoder::setWidth(float newValue) noexcept
{
	width = newValue;
}

float MidSideDecoder::getWidth() const noexcept
{
	return width;
}

StereoEffect::StereoEffect(MainController *mc, const String &uid, int numVoices) :
	VoiceEffectProcessor(mc, uid, numVoices),
	balanceChain(new ModulatorChain(mc, "Pan Modulation", numVoices, Modulation::GainMode, this)),
	pan(getDefaultValue(Pan)),
	width(getDefaultValue(Width))
{
	panBuffer = AudioSampleBuffer(1, 0);

	parameterNames.add("Pan");
	parameterNames.add("Width");

	editorStateIdentifiers.add("PanChainShown");
}

float StereoEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Pan:							return pan * 200.0f - 100.0f;
	case Width:							return msDecoder.getWidth() * 100.0f;
	default:							jassertfalse; return 1.0f;
	}
}

void StereoEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Pan:							pan = (newValue + 100.0f) / 200.0f; ; break;
	case Width:							msDecoder.setWidth(newValue / 100.0f); break;
	default:							jassertfalse; return;
	}
}

float StereoEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Pan:							return 1.0f;
	case Width:							return 100.0f;
	default:							jassertfalse; return 1.0f;
	}
}

void StereoEffect::restoreFromValueTree(const ValueTree &v)
{
	EffectProcessor::restoreFromValueTree(v);

	loadAttribute(Pan, "Pan");
	loadAttribute(Width, "Width");
}

ValueTree StereoEffect::exportAsValueTree() const
{
	ValueTree v = EffectProcessor::exportAsValueTree();

	saveAttribute(Pan, "Pan");
	saveAttribute(Width, "Width");

	return v;
}

void StereoEffect::renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	float *l = buffer.getWritePointer(0, 0);
	float *r = buffer.getWritePointer(1, 0);

	while (--numSamples >= 0)
	{
		msDecoder.calculateStereoValues(l[startSample], r[startSample]);

		startSample++;
	}
}

ProcessorEditorBody *StereoEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new StereoEditor(parentEditor);

	
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

AudioSampleBuffer & StereoEffect::getBufferForChain(int /*index*/)
{
	return panBuffer;
}

void StereoEffect::preVoiceRendering(int voiceIndex, int startSample, int numSamples)
{
	calculateChain(BalanceChain, voiceIndex, startSample, numSamples);
}

void StereoEffect::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	if (balanceChain->shouldBeProcessed(true) || balanceChain->shouldBeProcessed(false))
	{
		float* panValues = getCurrentModulationValues(BalanceChain, voiceIndex, startSample);

		float* outL = b.getWritePointer(0, startSample);
		float* outR = b.getWritePointer(1, startSample);

		const float normalizedPan = (pan - 0.5f) * 400.0f;

		while (--numSamples >= 0)
		{
			const float scaledPanValue = (*panValues++ - 0.5f) * normalizedPan;

			*outL++ *= BalanceCalculator::getGainFactorForBalance(scaledPanValue, true);
			*outR++ *= BalanceCalculator::getGainFactorForBalance(scaledPanValue, false);
		}
	}
}

