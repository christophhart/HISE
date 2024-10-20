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

SaturatorEffect::SaturatorEffect(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid),
	saturation(0.0f),
	wet(1.0f),
	dry(0.0f),
	preGain(1.0f),
    postGain(1.0f)
{
	modChains += {this, "Saturation Modulation"};

	finaliseModChains();

	saturationChain = modChains[InternalChains::SaturationChain].getChain();

	modChains[InternalChains::SaturationChain].setExpandToAudioRate(true);
	modChains[InternalChains::SaturationChain].setAllowModificationOfVoiceValues(true);

	parameterNames.add("Saturation");
	parameterNames.add("WetAmount");
	parameterNames.add("PreGain");
	parameterNames.add("PostGain");

	updateParameterSlots();

	editorStateIdentifiers.add("SaturationChainShown");

	saturator.setSaturationAmount(0.0f);

	saturationChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));
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

float SaturatorEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Saturation:
		return 0.0;
		break;
	case WetAmount:
		return 100.0;
		break;
	case PreGain:
		return 0.0;
	case PostGain:
		return 0.0;
	default:
		break;
	}

	jassertfalse;
	return 0.0f;
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

	if (auto modValues = modChains[SaturationChain].getReadPointerForVoiceValues(startSample))
	{
		for (int i = 0; i < numSamples; i++)
		{
			if (i & 7)
			{
				saturator.setSaturationAmount(modValues[i] * saturation);
			}

			l[i] = dry * l[i] + wet * (postGain * saturator.getSaturatedSample(preGain*l[i]));
			r[i] = dry * r[i] + wet * (postGain * saturator.getSaturatedSample(preGain*r[i]));
		}
	}
	else
	{
		const float modValue = modChains[SaturationChain].getConstantModulationValue();

		saturator.setSaturationAmount(modValue * saturation);

		for (int i = 0; i < numSamples; i++)
		{
			l[i] = dry * l[i] + wet * (postGain * saturator.getSaturatedSample(preGain*l[i]));
			r[i] = dry * r[i] + wet * (postGain * saturator.getSaturatedSample(preGain*r[i]));
		}
	}
}
} // namespace hise
