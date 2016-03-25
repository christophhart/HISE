/*
  ==============================================================================

    Saturator.cpp
    Created: 8 Jan 2016 3:01:27pm
    Author:  Christoph

  ==============================================================================
*/

#include "Saturator.h"

SaturatorEffect::SaturatorEffect(MainController *mc, const String &uid) :
	MasterEffectProcessor(mc, uid),
	saturationChain(new ModulatorChain(mc, "Saturation Modulation", 1, Modulation::GainMode, this)),
	saturation(0.0f),
	wet(1.0f),
	dry(0.0f),
	preGain(1.0f),
    postGain(1.0f)
{
	parameterNames.add("Saturation");
	parameterNames.add("WetAmount");
	parameterNames.add("PreGain");
	parameterNames.add("PostGain");

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

ProcessorEditorBody * SaturatorEffect::createEditor(BetterProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new SaturationEditor(parentEditor);

#else 

	jassertfalse;
	return nullptr;

#endif
}

void SaturatorEffect::renderNextBlock(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
    if(getLeftSourceChannel() >= getMatrix().getNumDestinationChannels() ||
       getRightSourceChannel() >= getMatrix().getNumDestinationChannels())
        return;
    
	float *l = buffer.getWritePointer(getLeftSourceChannel(), startSample);
	float *r = buffer.getWritePointer(getRightSourceChannel(), startSample);

	float const *modValues = nullptr;

	if (!saturationChain->isBypassed() && saturationChain->getNumChildProcessors() != 0)
	{
		saturationChain->renderAllModulatorsAsMonophonic(saturationBuffer, startSample, numSamples);

		modValues = saturationBuffer.getReadPointer(0, startSample);
	}

	for (int i = 0; i < numSamples; i++)
	{
		if (modValues != nullptr && (i & 7))
		{
			saturator.setSaturationAmount(modValues[i] * saturation);
		}

		l[i] = dry * l[i] + wet * (postGain * saturator.getSaturatedSample(preGain*l[i]));
		r[i] = dry * r[i] + wet * (postGain * saturator.getSaturatedSample(preGain*r[i]));
	}
}

void SaturatorEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate > 0)
	{
		saturationBuffer = AudioSampleBuffer(1, samplesPerBlock);
	}
}
