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
	pan(getDefaultValue(Pan)/100.0f)
{
	modChains += {this, "Pan Modulation", ModulatorChain::ModulationType::Normal, Modulation::PanMode};
	
	finaliseModChains();

	modChains[InternalChains::BalanceChain].setExpandToAudioRate(true);
	modChains[InternalChains::BalanceChain].setIncludeMonophonicValuesInVoiceRendering(true);
	modChains[InternalChains::BalanceChain].setAllowModificationOfVoiceValues(true);

	parameterNames.add("Pan");
	parameterNames.add("Width");

	auto tmp = WeakReference<Processor>(this);

	auto f = [tmp](float input)
	{
		if (tmp.get() != nullptr)
		{
			auto v = tmp->getAttribute(StereoEffect::Pan) * input;

			return BalanceCalculator::getBalanceAsString(roundToInt(v));
		}

		return Table::getDefaultTextValue(input);
	};

	modChains[BalanceChain].getChain()->setTableValueConverter(f);

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
	case Pan:							return 100.0f;
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

	if (msDecoder.getWidth() != 1.0f)
	{
		while (--numSamples >= 0)
		{
			msDecoder.calculateStereoValues(l[startSample], r[startSample]);
			startSample++;
		}
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

void StereoEffect::applyEffect(int /*voiceIndex*/, AudioSampleBuffer &b, int startSample, int numSamples)
{
	auto& balanceChain = modChains[BalanceChain];

	if (!balanceChain.getChain()->shouldBeProcessedAtAll())
		return;

	if (auto panValues = balanceChain.getReadPointerForVoiceValues(startSample))
	{
		float* outL = b.getWritePointer(0, startSample);
		float* outR = b.getWritePointer(1, startSample);

		const float normalizedPan = (pan - 0.5f) * 200.0f;

		while (--numSamples >= 0)
		{
			const float scaledPanValue = *panValues++ * normalizedPan;

			*outL++ *= BalanceCalculator::getGainFactorForBalance(scaledPanValue, true);
			*outR++ *= BalanceCalculator::getGainFactorForBalance(scaledPanValue, false);
		}
	}
	else
	{
		auto modValue = balanceChain.getConstantModulationValue();

		float* outL = b.getWritePointer(0, startSample);
		float* outR = b.getWritePointer(1, startSample);

		const float normalizedPan = (pan - 0.5f) * 200.0f;
		const float scaledPanValue = (modValue) * normalizedPan;

		float gainL = BalanceCalculator::getGainFactorForBalance(scaledPanValue, true);
		float gainR = BalanceCalculator::getGainFactorForBalance(scaledPanValue, false);

		FloatVectorOperations::multiply(outL, gainL, numSamples);
		FloatVectorOperations::multiply(outR, gainR, numSamples);
	}
}

} // namespace hise
