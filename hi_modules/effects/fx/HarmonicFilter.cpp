/*
  ==============================================================================

    HarmonicFilter.cpp
    Created: 8 Aug 2015 10:29:51pm
    Author:  Chrisboy

  ==============================================================================
*/

#include "HarmonicFilter.h"

HarmonicFilter::HarmonicFilter(MainController *mc, const String &uid, int numVoices_) :
VoiceEffectProcessor(mc, uid, numVoices_),
q(12.0f),
numVoices(numVoices_),
xFadeChain(new ModulatorChain(mc, "X-Fade Modulation", numVoices_, Modulation::GainMode, this)),
filterBandIndex(OneBand),
currentCrossfadeValue(0.5f),
semiToneTranspose(0)
{
	editorStateIdentifiers.add("XFadeChainShown");

	dataA = new SliderPackData();
	dataB = new SliderPackData();
	dataMix = new SliderPackData();

	dataA->setRange(-24.0, 24.0, 0.1);
	dataB->setRange(-24.0, 24.0, 0.1);
	dataMix->setRange(-24.0, 24.0, 0.1);

	setNumFilterBands(filterBandIndex);

	setQ(q);
}

void HarmonicFilter::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case NumFilterBands:	setNumFilterBands((int)newValue - 1); break;
	case QFactor:			setQ(newValue); break;
	case Crossfade:			setCrossfadeValue(newValue); break;
	case SemiToneTranspose: setSemitoneTranspose(newValue); break;
	default:							jassertfalse; return;
	}
}

float HarmonicFilter::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case NumFilterBands:	return (float)(filterBandIndex + 1);
	case QFactor:			return q;
	case Crossfade:			return currentCrossfadeValue;
	case SemiToneTranspose:	return (float)semiToneTranspose;
	default:				jassertfalse; return 1.0f;
	}
}

ValueTree HarmonicFilter::exportAsValueTree() const
{
	ValueTree v = EffectProcessor::exportAsValueTree();

	saveAttribute(NumFilterBands, "NumFilterBands");
	saveAttribute(QFactor, "QFactor");
	saveAttribute(SemiToneTranspose, "SemitoneTranspose");

	v.setProperty("LeftSliderPackData", dataA->toBase64(), nullptr);
	v.setProperty("RightSliderPackData", dataB->toBase64(), nullptr);

	saveAttribute(Crossfade, "CrossfadeValue");

	return v;
}

void HarmonicFilter::restoreFromValueTree(const ValueTree &v)
{
	EffectProcessor::restoreFromValueTree(v);

	loadAttribute(NumFilterBands, "NumFilterBands");
	loadAttribute(QFactor, "QFactor");
	loadAttribute(SemiToneTranspose, "SemitoneTranspose");

	dataA->fromBase64(v.getProperty("LeftSliderPackData"));
	dataB->fromBase64(v.getProperty("RightSliderPackData"));

	loadAttribute(Crossfade, "CrossfadeValue");
}

void HarmonicFilter::setQ(float newQ)
{
	q = newQ;

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		for (int j = 0; j < harmonicFilters[i]->voiceFilters.size(); j++)
		{
			harmonicFilters[i]->voiceFilters[j]->setAttribute(MonoFilterEffect::Q, newQ, dontSendNotification);
			if(getSampleRate() > 0.0) harmonicFilters[i]->voiceFilters[j]->calcCoefficients();
		}
	}
}

void HarmonicFilter::setNumFilterBands(int newFilterBandIndex)
{
	const int numBands = getNumBandForFilterBandIndex((FilterBandNumbers)newFilterBandIndex);

	filterBandIndex = newFilterBandIndex;

	harmonicFilters.clear();

	dataA->setNumSliders(numBands);
	dataB->setNumSliders(numBands);
	dataMix->setNumSliders(numBands);

	for (int i = 0; i < numBands; i++)
	{
		PolyFilterEffect *poly = new PolyFilterEffect(getMainController(), String(i), numVoices);

		poly->setAttribute(MonoFilterEffect::Q, q, dontSendNotification);
		poly->setAttribute(MonoFilterEffect::Mode, MonoFilterEffect::FilterMode::Peak, dontSendNotification);
		poly->setAttribute(MonoFilterEffect::Gain, 0.0f, dontSendNotification);

		if (getSampleRate() > 0)
		{
			poly->prepareToPlay(getSampleRate(), getBlockSize());
		}

		for (int j = 0; j < numVoices; j++)
		{
			poly->voiceFilters[j]->setUseFixedFrequency(true);
		}

		harmonicFilters.add(poly);
	}
}

void HarmonicFilter::setSemitoneTranspose(float newValue)
{
	semiToneTranspose = (int)newValue;	
}

void HarmonicFilter::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		harmonicFilters[i]->prepareToPlay(sampleRate, samplesPerBlock);
	}
}

void HarmonicFilter::preVoiceRendering(int voiceIndex, int startSample, int numSamples)
{
	calculateChain(XFadeChain, voiceIndex, startSample, numSamples);
}

void HarmonicFilter::startVoice(int voiceIndex, int noteNumber)
{
	VoiceEffectProcessor::startVoice(voiceIndex, noteNumber);

	const float freq = (float)MidiMessage::getMidiNoteInHertz(noteNumber + semiToneTranspose);

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		const float freqForThisHarmonic = freq * (float)(i + 1);

		if (freqForThisHarmonic >(getSampleRate() * 0.4)) // Spare frequencies above Nyquist
		{
			MonoFilterEffect *filter = harmonicFilters[i]->voiceFilters[voiceIndex];

			if(filter->useStateVariableFilters)
			{
				filter->stateFilterL.reset();
				filter->stateFilterR.reset();
			}
			else
			{
				filter->filterL.reset();
				filter->filterR.reset();
			}

			
			harmonicFilters[i]->voiceFilters[voiceIndex]->setBypassed(true);
		}
		else
		{
			harmonicFilters[i]->voiceFilters[voiceIndex]->setBypassed(false);

			harmonicFilters[i]->voiceFilters[voiceIndex]->setAttribute(MonoFilterEffect::Frequency, freqForThisHarmonic, dontSendNotification);

			// harmonicFilters[i]->startVoice(voiceIndex, noteNumber); // Don't need the modulator chains
			
			MonoFilterEffect *filter = harmonicFilters[i]->voiceFilters[voiceIndex];

			if(filter->useStateVariableFilters)
			{
				filter->stateFilterL.reset();
				filter->stateFilterR.reset();
			}
			else
			{
				filter->filterL.reset();
				filter->filterR.reset();
			}

		}


	}
}

void HarmonicFilter::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	double xModValue;

	if (xFadeChain->getHandler()->getNumProcessors() > 0)
	{
		xModValue = (double)getCurrentModulationValue(XFadeChain, voiceIndex, startSample);

		if (voiceIndex == xFadeChain->polyManager.getLastStartedVoice())
		{
			setCrossfadeValue(xModValue);
		}

	}
	else
	{
		xModValue = currentCrossfadeValue;
	}

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		//const float gainValue = dataMix->getValue(i);

		const float gainValue = Interpolator::interpolateLinear(dataA->getValue(i), dataB->getValue(i), (float)xModValue);

		if (gainValue == 0.0f || harmonicFilters[i]->voiceFilters[voiceIndex]->isBypassed())
		{
			continue;
		}
		else
		{
			harmonicFilters[i]->voiceFilters[voiceIndex]->setAttribute(MonoFilterEffect::Gain, gainValue, dontSendNotification);
			//harmonicFilters[i]->voiceFilters[voiceIndex]->calcCoefficients();
			harmonicFilters[i]->voiceFilters[voiceIndex]->applyEffect(b, startSample, numSamples);
		}


	}
}

ProcessorEditorBody * HarmonicFilter::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new HarmonicFilterEditor(parentEditor);
#else 
	jassertfalse;
	return nullptr;
#endif
}

SliderPackData * HarmonicFilter::getSliderPackData(int i)
{
	switch (i)
	{
	case SliderPacks::A:	return dataA;
	case SliderPacks::B:	return dataB;
	case SliderPacks::Mix:	return dataMix;
	default:				return nullptr;



	}
}

void HarmonicFilter::setCrossfadeValue(double normalizedCrossfadeValue)
{
	currentCrossfadeValue = (float)normalizedCrossfadeValue;

	for (int i = 0; i < dataA->getNumSliders(); i++)
	{
		const float aValue = dataA->getValue(i);

		const float bValue = dataB->getValue(i);

		const float mixValue = Interpolator::interpolateLinear(aValue, bValue, (float)normalizedCrossfadeValue);

		setInputValue(mixValue, dontSendNotification);

		dataMix->setValue(i, mixValue, sendNotification);

	}
}

int HarmonicFilter::getNumBandForFilterBandIndex(FilterBandNumbers number) const
{
	int numBands = 0;

	switch (number)
	{
	case OneBand:		numBands = 1; break;
	case TwoBands:		numBands = 2; break;
	case FourBands:		numBands = 4; break;
	case EightBands:	numBands = 8; break;
	case SixteenBands:	numBands = 16; break;
	default:			jassertfalse; break;
	}

	return numBands;
}
