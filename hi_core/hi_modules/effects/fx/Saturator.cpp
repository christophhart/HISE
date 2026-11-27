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

hise::ProcessorMetadata SaturatorEffect::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata()
		.withStandardMetadata<SaturatorEffect>()
		.withDescription("Applies waveshaping saturation with pre/post gain controls and wet/dry mix")
		.withParameter(Par(Saturation)
			.withId("Saturation")
			.withDescription("Amount of saturation applied to the signal where 0 is none and 1 is maximum")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(0.0f))
		.withParameter(Par(WetAmount)
			.withId("WetAmount")
			.withDescription("Dry/wet mix where 0 is fully dry and 1 is fully wet")
			.withSliderMode(HiSlider::NormalizedPercentage, Range())
			.withDefault(1.0f))
		.withParameter(Par(PreGain)
			.withId("PreGain")
			.withDescription("Input gain in decibels applied before saturation")
			.withSliderMode(HiSlider::Decibel, Range(0.0, 24.0))
			.withDefault(0.0f))
		.withParameter(Par(PostGain)
			.withId("PostGain")
			.withDescription("Output gain in decibels applied after saturation")
			.withSliderMode(HiSlider::Decibel, Range(-24.0, 0.0))
			.withDefault(0.0f))
		.withModulation(Mod(SaturationChain)
			.withId("Saturation Modulation")
			.withDescription("Modulates the saturation amount")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly))
		.withModulation(Mod(WetChain)
			.withId("Wet Modulation")
			.withDescription("Modulates the dry/wet mix")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
			.withModulatedParameter(WetAmount));
}

SaturatorEffect::SaturatorEffect(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid),
	saturation(0.0f),
	wet(1.0f),
	dry(0.0f),
	preGain(1.0f),
    postGain(1.0f)
{
	modChains.reserve(InternalChains::numInternalChains);

	modChains += {this, "Saturation Modulation"};
	modChains += {this, "Wet Modulation"};

	finaliseModChains();

	saturationChain = modChains[InternalChains::SaturationChain].getChain();
	wetChain = modChains[InternalChains::WetChain].getChain();

	modChains[InternalChains::SaturationChain].setExpandToAudioRate(true);
	modChains[InternalChains::SaturationChain].setAllowModificationOfVoiceValues(true);
	modChains[InternalChains::WetChain].setExpandToAudioRate(true);

	updateParameterSlots();

	editorStateIdentifiers.add("SaturationChainShown");
	editorStateIdentifiers.add("WetChainShown");

	saturator.setSaturationAmount(0.0f);

	saturationChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));
	wetChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));

	auto tmp = WeakReference<Processor>(this);

	wetChain->setTableValueConverter([tmp](float input)
	{
		if (tmp)
		{
			auto v = tmp->getAttribute(SaturatorEffect::Parameters::WetAmount) * input;
			return String(roundToInt(v * 100.0f)) + "%";
		}

		return Table::getDefaultTextValue(input);
	});
}

void SaturatorEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Saturation:
		saturation = newValue;
		saturator.setSaturationAmount(newValue);
		break;
	case WetAmount:
		dry = 1.0f - newValue;
		wet = newValue;
		break;
	case PreGain:
		preGain =  Decibels::decibelsToGain(newValue);
		break;
	case PostGain:
		postGain = Decibels::decibelsToGain(newValue);
		break;
	default:
		break;
	}

}

float SaturatorEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Saturation:
		return saturation;
		break;
	case WetAmount:
		return wet;
		break;
	case PreGain:
		return Decibels::gainToDecibels(preGain);
	case PostGain:
		return Decibels::gainToDecibels(postGain);
	default:
		break;
	}

	jassertfalse;
	return 0.0f;
}

ModulationDisplayValue::QueryFunction::Ptr SaturatorEffect::getModulationQueryFunction(int parameterIndex) const
{
	if (parameterIndex == WetAmount)
		return new ModulatorChain::GetModulationOutput<(int)InternalChains::WetChain>();

	return nullptr;
}

void SaturatorEffect::restoreFromValueTree(const ValueTree &v)
{
    MasterEffectProcessor::restoreFromValueTree(v);
    
	loadAttribute(Saturation, "Saturation");
	loadAttribute(WetAmount, "WetAmount");
	loadAttribute(PreGain, "PreGain");
	loadAttribute(PostGain, "PostGain");
}

ValueTree SaturatorEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(Saturation, "Saturation");
	saveAttribute(WetAmount, "WetAmount");
	saveAttribute(PreGain, "PreGain");
	saveAttribute(PostGain, "PostGain");

	return v;
}

ProcessorEditorBody * SaturatorEffect::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new SaturationEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

void SaturatorEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	float *l = buffer.getWritePointer(0, startSample);
	float *r = buffer.getWritePointer(1, startSample);

	auto satValues = modChains[SaturationChain].getReadPointerForVoiceValues(startSample);
	auto wetValues = modChains[WetChain].getReadPointerForVoiceValues(startSample);

	if (satValues == nullptr)
		saturator.setSaturationAmount(modChains[SaturationChain].getConstantModulationValue() * saturation);

	const float constantWet = wet * modChains[WetChain].getConstantModulationValue();

	for (int i = 0; i < numSamples; i++)
	{
		if (satValues != nullptr && (i & 7) == 0)
			saturator.setSaturationAmount(satValues[i] * saturation);

		const float thisWet = wetValues != nullptr ? wet * wetValues[i] : constantWet;
		const float thisDry = 1.0f - thisWet;

		l[i] = thisDry * l[i] + thisWet * (postGain * saturator.getSaturatedSample(preGain*l[i]));
		r[i] = thisDry * r[i] + thisWet * (postGain * saturator.getSaturatedSample(preGain*r[i]));
	}
}
} // namespace hise
