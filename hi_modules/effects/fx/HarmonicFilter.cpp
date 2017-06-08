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

HarmonicFilter::HarmonicFilter(MainController *mc, const String &uid, int numVoices_) :
VoiceEffectProcessor(mc, uid, numVoices_),
q(12.0f),
numVoices(numVoices_),
xFadeChain(new ModulatorChain(mc, "X-Fade Modulation", numVoices_, Modulation::GainMode, this)),
filterBandIndex(OneBand),
currentCrossfadeValue(0.5f),
semiToneTranspose(0)
{
	timeVariantFreqModulatorBuffer = AudioSampleBuffer(1, 0);

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

	ProcessorHelpers::increaseBufferIfNeeded(timeVariantFreqModulatorBuffer, samplesPerBlock);

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

			filter->currentFilter->reset();
			
			harmonicFilters[i]->voiceFilters[voiceIndex]->setBypassed(true);
		}
		else
		{
			harmonicFilters[i]->voiceFilters[voiceIndex]->setBypassed(false);

			harmonicFilters[i]->voiceFilters[voiceIndex]->setAttribute(MonoFilterEffect::Frequency, freqForThisHarmonic, dontSendNotification);

			// harmonicFilters[i]->startVoice(voiceIndex, noteNumber); // Don't need the modulator chains
			
			MonoFilterEffect *filter = harmonicFilters[i]->voiceFilters[voiceIndex];

			filter->currentFilter->reset();
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

ProcessorEditorBody * HarmonicFilter::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new HarmonicFilterEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
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



// ====================================================================================================================================================


HarmonicMonophonicFilter::HarmonicMonophonicFilter(MainController *mc, const String &uid) :
MonophonicEffectProcessor(mc, uid),
q(12.0f),
xFadeChain(new ModulatorChain(mc, "X-Fade Modulation", 1, Modulation::GainMode, this)),
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
    
    timeVariantFreqModulatorBuffer = AudioSampleBuffer(1, 0);
}

void HarmonicMonophonicFilter::setInternalAttribute(int parameterIndex, float newValue)
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

float HarmonicMonophonicFilter::getAttribute(int parameterIndex) const
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

ValueTree HarmonicMonophonicFilter::exportAsValueTree() const
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

void HarmonicMonophonicFilter::restoreFromValueTree(const ValueTree &v)
{
	EffectProcessor::restoreFromValueTree(v);

	loadAttribute(NumFilterBands, "NumFilterBands");
	loadAttribute(QFactor, "QFactor");
	loadAttribute(SemiToneTranspose, "SemitoneTranspose");

	dataA->fromBase64(v.getProperty("LeftSliderPackData"));
	dataB->fromBase64(v.getProperty("RightSliderPackData"));

	loadAttribute(Crossfade, "CrossfadeValue");
}

void HarmonicMonophonicFilter::setQ(float newQ)
{
	q = newQ;

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		harmonicFilters[i]->setAttribute(MonoFilterEffect::Q, newQ, dontSendNotification);

		if (getSampleRate() > 0.0) harmonicFilters[i]->calcCoefficients();
	}
}

void HarmonicMonophonicFilter::setNumFilterBands(int newFilterBandIndex)
{
	const int numBands = getNumBandForFilterBandIndex((FilterBandNumbers)newFilterBandIndex);

	filterBandIndex = newFilterBandIndex;

	harmonicFilters.clear();

	dataA->setNumSliders(numBands);
	dataB->setNumSliders(numBands);
	dataMix->setNumSliders(numBands);

	for (int i = 0; i < numBands; i++)
	{
		MonoFilterEffect *mono = new MonoFilterEffect(getMainController(), String(i));

		mono->setAttribute(MonoFilterEffect::Q, q, dontSendNotification);
		mono->setAttribute(MonoFilterEffect::Mode, MonoFilterEffect::FilterMode::Peak, dontSendNotification);
		mono->setAttribute(MonoFilterEffect::Gain, 0.0f, dontSendNotification);

		if (getSampleRate() > 0)
		{
			mono->prepareToPlay(getSampleRate(), getBlockSize());
		}

		mono->setUseFixedFrequency(true);
		mono->setUseInternalChains(false);
		
		harmonicFilters.add(mono);
	}
}

void HarmonicMonophonicFilter::setSemitoneTranspose(float newValue)
{
	semiToneTranspose = (int)newValue;
}

void HarmonicMonophonicFilter::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MonophonicEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		harmonicFilters[i]->prepareToPlay(sampleRate, samplesPerBlock);
	}
}
void HarmonicMonophonicFilter::startMonophonicVoice(int noteNumber)
{
	MonophonicEffectProcessor::startMonophonicVoice(noteNumber);

	const float freq = (float)MidiMessage::getMidiNoteInHertz(noteNumber + semiToneTranspose);

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		const float freqForThisHarmonic = freq * (float)(i + 1);

		if (freqForThisHarmonic >(getSampleRate() * 0.4)) // Spare frequencies above Nyquist
		{
			MonoFilterEffect *filter = harmonicFilters[i];

			filter->currentFilter->reset();

			filter->setBypassed(true);
		}
		else
		{
			MonoFilterEffect *filter = harmonicFilters[i];

			filter->setBypassed(false);

			filter->setAttribute(MonoFilterEffect::Frequency, freqForThisHarmonic, dontSendNotification);

			filter->currentFilter->reset();
		}

	}
}

void HarmonicMonophonicFilter::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	double xModValue;

	

	if (xFadeChain->getHandler()->getNumProcessors() > 0)
	{
		xFadeChain->renderAllModulatorsAsMonophonic(getBufferForChain(0), startSample, numSamples);

		xModValue = getBufferForChain(0).getSample(0, startSample);

		//xModValue = (double)getCurrentModulationValue(XFadeChain, 0, startSample);

		setCrossfadeValue(xModValue);
	}
	else
	{
		xModValue = currentCrossfadeValue;
	}

	for (int i = 0; i < harmonicFilters.size(); i++)
	{
		const float gainValue = Interpolator::interpolateLinear(dataA->getValue(i), dataB->getValue(i), (float)xModValue);

		if (gainValue == 0.0f || harmonicFilters[i]->isBypassed())
		{
			continue;
		}
		else
		{
			harmonicFilters[i]->setAttribute(MonoFilterEffect::Gain, gainValue, dontSendNotification);
			harmonicFilters[i]->applyEffect(b, startSample, numSamples);
		}
	}
}

ProcessorEditorBody * HarmonicMonophonicFilter::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
	return new HarmonicFilterEditor(parentEditor);
#else

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;
#endif
}

SliderPackData * HarmonicMonophonicFilter::getSliderPackData(int i)
{
	switch (i)
	{
	case SliderPacks::A:	return dataA;
	case SliderPacks::B:	return dataB;
	case SliderPacks::Mix:	return dataMix;
	default:				return nullptr;



	}
}

void HarmonicMonophonicFilter::setCrossfadeValue(double normalizedCrossfadeValue)
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

int HarmonicMonophonicFilter::getNumBandForFilterBandIndex(FilterBandNumbers number) const
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
