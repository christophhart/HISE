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

hise::ProcessorMetadata HarmonicFilter::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata()
		.withStandardMetadata<HarmonicFilter>()
		.withDescription("Polyphonic peak filters tuned to the root frequency and harmonics of each voice, with crossfadeable A/B slider pack configurations.")
		.withComplexDataInterface(ExternalData::DataType::SliderPack)
		.withParameter(Par(NumFilterBands)
			.withId("NumFilterBands")
			.withDescription("The number of harmonic filter bands (1, 2, 4, 8, or 16)")
			.withValueList({ "1 Filter Band", "2 Filter Bands", "4 Filter Bands", "8 Filter Bands", "16 Filter Bands" })
			.withDefault(1.0f))
		.withParameter(Par(QFactor)
			.withId("QFactor")
			.withDescription("The resonance of the harmonic peak filters")
			.withSliderMode(HiSlider::Discrete, Range(4.0, 48.0, 1.0))
			.withDefault(12.0f))
		.withParameter(Par(Crossfade)
			.withId("CrossfadeValue")
			.withDescription("Crossfade position between slider pack A and B configurations (0 = A, 0.5 = mix, 1 = B)")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(0.5f))
		.withParameter(Par(SemiToneTranspose)
			.withId("SemitoneTranspose")
			.withDescription("Transposes the harmonic filter tuning in semitones")
			.withSliderMode(HiSlider::Discrete, Range(-24.0, 24.0, 1.0))
			.withDefault(0.0f))
		.withModulation(Mod(XFadeChain)
			.withId("X-Fade Modulation")
			.withDescription("Modulates the crossfade position between A and B configurations")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
			.withModulatedParameter(Crossfade));
}

HarmonicFilter::HarmonicFilter(MainController *mc, const String &uid, int numVoices_) :
VoiceEffectProcessor(mc, uid, numVoices_),
BaseHarmonicFilter(mc),
q(12.0f),
numVoices(numVoices_),
filterBandIndex(OneBand),
currentCrossfadeValue(0.5f),
semiToneTranspose(0),
filterBanks(numVoices_)
{
	modChains += {this, "X-Fade Modulation"};

	finaliseModChains();
	updateParameterSlots();

	editorStateIdentifiers.add("XFadeChainShown");

	dataA->setRange(-24.0, 24.0, 0.1);
	dataB->setRange(-24.0, 24.0, 0.1);
	dataMix->setRange(-24.0, 24.0, 0.1);

	setNumFilterBands(filterBandIndex);

	setQ((float)q);
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
	case QFactor:			return (float)q;
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

	for (auto& filterBank : filterBanks)
	{
		filterBank.setQ(newQ);
	}
}

void HarmonicFilter::setNumFilterBands(int newFilterBandIndex)
{
	numBands = getNumBandForFilterBandIndex((FilterBandNumbers)newFilterBandIndex);

	filterBandIndex = newFilterBandIndex;

	dataA->setNumSliders(numBands);
	dataB->setNumSliders(numBands);
	dataMix->setNumSliders(numBands);

	for (auto& filterBank : filterBanks)
		filterBank.setNumBands(numBands);
}

void HarmonicFilter::setSemitoneTranspose(float newValue)
{
	semiToneTranspose = (int)newValue;	
}

void HarmonicFilter::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	for (auto& filterBank : filterBanks)
	{
		filterBank.setSampleRate(sampleRate);
	}
}

void HarmonicFilter::startVoice(int voiceIndex, const HiseEvent& e)
{
	VoiceEffectProcessor::startVoice(voiceIndex, e);

	HiseEvent copy(e);
	copy.setTransposeAmount(copy.getTransposeAmount() + semiToneTranspose);
	auto freq = copy.getFrequency();

	filterBanks[voiceIndex].reset();
	filterBanks[voiceIndex].updateBaseFrequency(freq);
}

void HarmonicFilter::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	const bool useModulation = modChains[XFadeChain].getChain()->shouldBeProcessedAtAll();

	const float xModValue = useModulation ? modChains[XFadeChain].getOneModulationValue(startSample) : currentCrossfadeValue;
	
	if (voiceIndex == modChains[XFadeChain].getChain()->polyManager.getLastStartedVoice())
		setCrossfadeValue(xModValue);

	auto& filterBank = filterBanks[voiceIndex];

	for (int i = 0; i < numBands; i++)
	{
		const float gainValue = Interpolator::interpolateLinear(dataA->getValue(i), dataB->getValue(i), xModValue);
		filterBank.updateBand(i, gainValue);
	}

	filterBank.processSamples(b, startSample, numSamples);
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
	int thisNumBands = 0;

	switch (number)
	{
	case OneBand:		thisNumBands = 1; break;
	case TwoBands:		thisNumBands = 2; break;
	case FourBands:		thisNumBands = 4; break;
	case EightBands:	thisNumBands = 8; break;
	case SixteenBands:	thisNumBands = 16; break;
	default:			jassertfalse; break;
	}

	return thisNumBands;
}



// ====================================================================================================================================================


hise::ProcessorMetadata HarmonicMonophonicFilter::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata()
		.withStandardMetadata<HarmonicMonophonicFilter>()
		.withDescription("Monophonic peak filters tuned to the root frequency and harmonics of the last played note, with crossfadeable A/B configurations.")
		.withComplexDataInterface(ExternalData::DataType::SliderPack)
		.withParameter(Par(NumFilterBands)
			.withId("NumFilterBands")
			.withDescription("The number of harmonic filter bands (1, 2, 4, 8, or 16)")
			.withValueList({ "1 Filter Band", "2 Filter Bands", "4 Filter Bands", "8 Filter Bands", "16 Filter Bands" })
			.withDefault(1.0f))
		.withParameter(Par(QFactor)
			.withId("QFactor")
			.withDescription("The resonance of the harmonic peak filters")
			.withSliderMode(HiSlider::Discrete, Range(4.0, 48.0, 1.0))
			.withDefault(12.0f))
		.withParameter(Par(Crossfade)
			.withId("CrossfadeValue")
			.withDescription("Crossfade position between slider pack A and B configurations (0 = A, 0.5 = mix, 1 = B)")
			.withSliderMode(HiSlider::NormalizedPercentage, {})
			.withDefault(0.5f))
		.withParameter(Par(SemiToneTranspose)
			.withId("SemitoneTranspose")
			.withDescription("Transposes the harmonic filter tuning in semitones")
			.withSliderMode(HiSlider::Discrete, Range(-24.0, 24.0, 1.0))
			.withDefault(0.0f))
		.withModulation(Mod(XFadeChain)
			.withId("X-Fade Modulation")
			.withDescription("Modulates the crossfade position between A and B configurations")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
			.withModulatedParameter(Crossfade));
}

HarmonicMonophonicFilter::HarmonicMonophonicFilter(MainController *mc, const String &uid) :
MonophonicEffectProcessor(mc, uid),
BaseHarmonicFilter(mc),
q(12.0f),
filterBandIndex(OneBand),
currentCrossfadeValue(0.5f),
semiToneTranspose(0),
filterBank()
{
	modChains += {this, "X-Fade Modulation"};

	finaliseModChains();

	editorStateIdentifiers.add("XFadeChainShown");

	dataA->setRange(-24.0, 24.0, 0.1);
	dataB->setRange(-24.0, 24.0, 0.1);
	dataMix->setRange(-24.0, 24.0, 0.1);

	setNumFilterBands(filterBandIndex);

	setQ((float)q);
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
	case QFactor:			return (float)q;
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
	filterBank.setQ(newQ);
	q = newQ;
}

void HarmonicMonophonicFilter::setNumFilterBands(int newFilterBandIndex)
{
	numBands = getNumBandForFilterBandIndex((FilterBandNumbers)newFilterBandIndex);

	filterBandIndex = newFilterBandIndex;

	dataA->setNumSliders(numBands);
	dataB->setNumSliders(numBands);
	dataMix->setNumSliders(numBands);
	filterBank.setNumBands(numBands);
}

void HarmonicMonophonicFilter::setSemitoneTranspose(float newValue)
{
	semiToneTranspose = (int)newValue;
}

void HarmonicMonophonicFilter::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MonophonicEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	filterBank.setSampleRate(sampleRate);
}
void HarmonicMonophonicFilter::startMonophonicVoice(const HiseEvent& e)
{
	MonophonicEffectProcessor::startMonophonicVoice(e);

	HiseEvent copy(e);
	copy.setTransposeAmount(copy.getTransposeAmount() + semiToneTranspose);
	auto freq = copy.getFrequency();

	filterBank.reset();
	filterBank.updateBaseFrequency(freq);
}

void HarmonicMonophonicFilter::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	const bool useModulation = modChains[XFadeChain].getChain()->shouldBeProcessedAtAll();

	const float xModValue = useModulation ? modChains[XFadeChain].getOneModulationValue(startSample) : currentCrossfadeValue;

	setCrossfadeValue(xModValue);
	
	for (int i = 0; i < numBands; i++)
	{
		const float gainValue = Interpolator::interpolateLinear(dataA->getValue(i), dataB->getValue(i), (float)xModValue);

		filterBank.updateBand(i, gainValue);
	}

	filterBank.processSamples(b, startSample, numSamples);
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
	int thisNumBands = 0;

	switch (number)
	{
	case OneBand:		thisNumBands = 1; break;
	case TwoBands:		thisNumBands = 2; break;
	case FourBands:		thisNumBands = 4; break;
	case EightBands:	thisNumBands = 8; break;
	case SixteenBands:	thisNumBands = 16; break;
	default:			jassertfalse; break;
	}

	return thisNumBands;
}
} // namespace hise
