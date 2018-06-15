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

MonoFilterEffect::MonoFilterEffect(MainController *mc, const String &id) :
MonophonicEffectProcessor(mc, id),
changeFlag(false),
useInternalChains(true),
freqChain(new ModulatorChain(mc, "Freq Modulation", 1, Modulation::GainMode, this)),
gainChain(new ModulatorChain(mc, "Gain Modulation", 1, Modulation::GainMode, this)),
bipolarFreqChain(new ModulatorChain(mc, "Bipolar Freq Mod", 1, Modulation::GainMode, this)),
freqBuffer(1, 0),
gainBuffer(1, 0),
bipolarFreqBuffer(1, 0),
filterCollection(1)
{
	WeakReference<Processor> t = this;

	auto f = [t](float input)
	{
		if (t != nullptr)
		{
			auto freq = t->getAttribute(MonoFilterEffect::Parameters::Frequency);

			auto v = jmap<float>(input , 20.0f, freq);
			return String(roundFloatToInt(v)) + " Hz";
		}

		return String();
	};

	freqChain->setTableValueConverter(f);
	bipolarFreqChain->setTableValueConverter(f);
	gainChain->setTableValueConverter(getTableValueAsGain);

	editorStateIdentifiers.add("FrequencyChainShown");
	editorStateIdentifiers.add("GainChainShown");
	editorStateIdentifiers.add("BipolarFreqChainShown");
    
    setRenderQuality(256);

	freqChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());
	gainChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());
	bipolarFreqChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());

	parameterNames.add("Gain");
	parameterNames.add("Frequency");
	parameterNames.add("Q");
	parameterNames.add("Mode");
    parameterNames.add("Quality");
	parameterNames.add("BipolarIntensity");

	setMode((int)getDefaultValue(MonoFilterEffect::Mode));
}



void MonoFilterEffect::setUseInternalChains(bool shouldBeUsed)
{
	useInternalChains = shouldBeUsed;
}

float MonoFilterEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Gain:		return Decibels::gainToDecibels(filterCollection.getGain());
	case Frequency:	return (float)filterCollection.getFrequency();
	case Q:			return (float)filterCollection.getQ();
	case Mode:		return (float)(int)filterCollection.getMode();
    case Quality:   return (float)getSampleAmountForRenderQuality();
	case BipolarIntensity: return bipolarIntensity;
	default:		jassertfalse; return 1.0f;
	}
}

void MonoFilterEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Gain:		filterCollection.setGain(Decibels::decibelsToGain(newValue)); break;
	case Frequency:	filterCollection.setFrequency(newValue); break;
	case Q:			filterCollection.setQ(newValue); break;
	case Mode:		setMode((int)newValue);	break;
    case Quality:   setRenderQuality((int)newValue); break;
	case BipolarIntensity: bipolarIntensity = newValue; break;
	default:		jassertfalse; return;
	}

	changeFlag = true;
}

float MonoFilterEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Gain:		return 0.0f;
	case Frequency:	return 20000.0f;
	case Q:			return 1.0f;
	case Mode:		return (float)(int)FilterBank::FilterMode::StateVariableLP;
	case Quality:   return 256.0f;
	case BipolarIntensity: return 0.0f;
	default:		jassertfalse; return 1.0f;
	}
}

void MonoFilterEffect::restoreFromValueTree(const ValueTree &v)
{
	EffectProcessor::restoreFromValueTree(v);

	loadAttribute(Gain, "Gain");
	loadAttribute(Frequency, "Frequency");
	loadAttribute(Q, "Q");
	loadAttribute(Mode, "Mode");
    loadAttribute(Quality, "RenderQuality");
	loadAttribute(BipolarIntensity, "BipolarIntensity");
}

ValueTree MonoFilterEffect::exportAsValueTree() const
{
	ValueTree v = EffectProcessor::exportAsValueTree();

	saveAttribute(Gain, "Gain");
	saveAttribute(Frequency, "Frequency");
	saveAttribute(Q, "Q");
	saveAttribute(Mode, "Mode");
    saveAttribute(Quality, "RenderQuality");
	saveAttribute(BipolarIntensity, "BipolarIntensity");
	return v;
}

void MonoFilterEffect::setMode(int filterMode)
{
	filterCollection.setMode((FilterBank::FilterMode)filterMode);
}

void MonoFilterEffect::calcCoefficients()
{
	changeFlag = false;
}

void MonoFilterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (lastSampleRate != sampleRate)
	{
		lastSampleRate = sampleRate;
		filterCollection.setSampleRate(sampleRate);
	}
}

void MonoFilterEffect::processBlockPartial(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	FilterHelpers::RenderData r(buffer, startSample, numSamples);

	r.freqModValue = (double)getCurrentModulationValue(FrequencyChain, 0, startSample);
	auto bipolarFMod = getCurrentModulationValue(BipolarFrequencyChain, 0, startSample);
	r.freqModValue += (double)bipolarIntensity * bipolarFMod;
	r.gainModValue = (double)getCurrentModulationValue(GainChain, 0, startSample);

    filterCollection.setDisplayModValues(-1, r.freqModValue, r.gainModValue);
    
	filterCollection.renderMono(r);
}

void MonoFilterEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
    const int samplesPerLoop = getSampleAmountForRenderQuality();
    
    while(numSamples - samplesPerLoop > 0)
    {
        processBlockPartial(buffer, startSample, samplesPerLoop);
        
        startSample += samplesPerLoop;
        numSamples -= samplesPerLoop;
    }
    
    processBlockPartial(buffer, startSample, numSamples);
}

Processor * MonoFilterEffect::getChildProcessor(int processorIndex)
{
	switch (processorIndex)
	{
	case FrequencyChain: return freqChain;
	case GainChain: return gainChain;
	case BipolarFrequencyChain: return bipolarFreqChain;
	}

	jassertfalse;
	return nullptr;
}

const Processor * MonoFilterEffect::getChildProcessor(int processorIndex) const
{
	switch (processorIndex)
	{
	case FrequencyChain: return freqChain;
	case GainChain: return gainChain;
	case BipolarFrequencyChain: return bipolarFreqChain;
	}

	jassertfalse;
	return nullptr;
}

AudioSampleBuffer & MonoFilterEffect::getBufferForChain(int chainIndex)
{
	switch (chainIndex)
	{
	case FrequencyChain: return freqBuffer;
	case GainChain: return gainBuffer;
	case BipolarFrequencyChain: return bipolarFreqBuffer;
	}

	jassertfalse;
	return bipolarFreqBuffer;
}

ProcessorEditorBody *MonoFilterEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new FilterEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

PolyFilterEffect::PolyFilterEffect(MainController *mc, const String &uid, int numVoices) :
VoiceEffectProcessor(mc, uid, numVoices),
freqChain(new ModulatorChain(mc, "Frequency Modulation", numVoices, Modulation::GainMode, this)),
gainChain(new ModulatorChain(mc, "Gain Modulation", numVoices, Modulation::GainMode, this)),
bipolarFreqChain(new ModulatorChain(mc, "Bipolar Freq Modulation", numVoices, Modulation::GainMode, this)),
timeVariantFreqModulatorBuffer(1, 0),
timeVariantGainModulatorBuffer(1, 0),
timeVariantBipolarFreqModulatorBuffer(1, 0),
voiceFilters(numVoices)
{
	WeakReference<Processor> t = this;

	auto f = [t](float input)
	{
		if (t != nullptr)
		{
			auto freq = t->getAttribute(MonoFilterEffect::Parameters::Frequency);

			auto v = jmap<float>(input, 20.0f, freq);
			return String(roundFloatToInt(v)) + " Hz";
		}

		return String();
	};

	freqChain->setTableValueConverter(f);
	bipolarFreqChain->setTableValueConverter(f);
	gainChain->setTableValueConverter(getTableValueAsGain);

	editorStateIdentifiers.add("FrequencyChainShown");
	editorStateIdentifiers.add("GainChainShown");
	editorStateIdentifiers.add("BipolarFreqChainShown");

	
    
    parameterNames.add("Gain");
    parameterNames.add("Frequency");
    parameterNames.add("Q");
    parameterNames.add("Mode");
    parameterNames.add("Quality");
	parameterNames.add("BipolarIntensity");

	voiceFilters.setMode((FilterBank::FilterMode)(int)getDefaultValue(MonoFilterEffect::Mode));
}

float PolyFilterEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case MonoFilterEffect::Gain:		return Decibels::gainToDecibels(voiceFilters.getGain());
	case MonoFilterEffect::Frequency:	return (float)voiceFilters.getFrequency();
	case MonoFilterEffect::Q:			return (float)voiceFilters.getQ();
	case MonoFilterEffect::Mode:		return (float)(int)voiceFilters.getMode();
    case MonoFilterEffect::Quality:		return (float)getSampleAmountForRenderQuality();
	case MonoFilterEffect::BipolarIntensity: return bipolarIntensity;
	default:							jassertfalse; return 1.0f;
	}
}

void PolyFilterEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case MonoFilterEffect::Gain:		voiceFilters.setGain(Decibels::decibelsToGain(newValue)); break;
	case MonoFilterEffect::Frequency:	voiceFilters.setFrequency(newValue); break;
	case MonoFilterEffect::Q:			voiceFilters.setQ(newValue); break;
	case MonoFilterEffect::Mode:		voiceFilters.setMode((FilterBank::FilterMode)(int)newValue); break;
    case MonoFilterEffect::Quality:		setRenderQuality((int)newValue); break;
	case MonoFilterEffect::BipolarIntensity: bipolarIntensity = jlimit<float>(-1.0f, 1.0f, newValue); break;
	default:							jassertfalse; return;
	}

	changeFlag = true;
}

float PolyFilterEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case MonoFilterEffect::Gain:		return 0.0f;
	case MonoFilterEffect::Frequency:	return 20000.0f;
	case MonoFilterEffect::Q:			return 1.0f;
	case MonoFilterEffect::Mode:		return (float)(int)FilterBank::FilterMode::StateVariableLP;
	case MonoFilterEffect::Quality:   return 256.0f;
	case MonoFilterEffect::BipolarIntensity: return 0.0f;
	default:		jassertfalse; return 1.0f;
	}
}

void PolyFilterEffect::restoreFromValueTree(const ValueTree &v)
{
	EffectProcessor::restoreFromValueTree(v);

	loadAttribute(MonoFilterEffect::Gain, "Gain");
	loadAttribute(MonoFilterEffect::Frequency, "Frequency");
	loadAttribute(MonoFilterEffect::Q, "Q");
	loadAttribute(MonoFilterEffect::Mode, "Mode");
    loadAttribute(MonoFilterEffect::Quality, "Quality");
	loadAttribute(MonoFilterEffect::BipolarIntensity, "BipolarIntensity");
}

ValueTree PolyFilterEffect::exportAsValueTree() const
{
	ValueTree v = EffectProcessor::exportAsValueTree();

	saveAttribute(MonoFilterEffect::Gain, "Gain");
	saveAttribute(MonoFilterEffect::Frequency, "Frequency");
	saveAttribute(MonoFilterEffect::Q, "Q");
	saveAttribute(MonoFilterEffect::Mode, "Mode");
    saveAttribute(MonoFilterEffect::Quality, "Quality");
	saveAttribute(MonoFilterEffect::BipolarIntensity, "BipolarIntensity");

	return v;
}

Processor * PolyFilterEffect::getChildProcessor(int processorIndex)
{
	switch (processorIndex)
	{
	case FrequencyChain: return freqChain;
	case GainChain: return gainChain;
	case BipolarFrequencyChain: return bipolarFreqChain;
	}

	jassertfalse;
	return nullptr;
}

const Processor * PolyFilterEffect::getChildProcessor(int processorIndex) const
{
	switch (processorIndex)
	{
	case FrequencyChain: return freqChain;
	case GainChain: return gainChain;
	case BipolarFrequencyChain: return bipolarFreqChain;
	}

	jassertfalse;
	return nullptr;
}

AudioSampleBuffer & PolyFilterEffect::getBufferForChain(int index)
{
	switch (index)
	{
	case FrequencyChain: return timeVariantFreqModulatorBuffer;
	case GainChain: return timeVariantGainModulatorBuffer;
	case BipolarFrequencyChain: return timeVariantBipolarFreqModulatorBuffer;
	}

	jassertfalse;
	return timeVariantFreqModulatorBuffer;
}

void PolyFilterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	voiceFilters.setSampleRate(sampleRate);
}

ProcessorEditorBody *PolyFilterEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new FilterEditor(parentEditor);
	
#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

juce::IIRCoefficients PolyFilterEffect::getCurrentCoefficients() const
{
	if (ownerSynthForCoefficients == nullptr)
	{
		ownerSynthForCoefficients = const_cast<Processor*>(ProcessorHelpers::findParentProcessor(this, true));
	}

	if (auto ownerSynth = dynamic_cast<const ModulatorSynth*>(ownerSynthForCoefficients.get()))
	{
		auto v = ownerSynth->getLastStartedVoice();

		if (v != nullptr && ownerSynth->getNumActiveVoices() != 0)
		{
			auto index = v->getVoiceIndex();

            voiceFilters.setDisplayVoiceIndex(index);
            
			return voiceFilters.getCurrentCoefficients();
		}

		return MonoFilterEffect::getDisplayCoefficients(voiceFilters.getMode(), voiceFilters.getFrequency(), voiceFilters.getQ(), voiceFilters.getGain(), getSampleRate());
	}
	else
	{
		return MonoFilterEffect::getDisplayCoefficients(voiceFilters.getMode(), voiceFilters.getFrequency(), voiceFilters.getQ(), voiceFilters.getGain(), getSampleRate());
	}
}

void PolyFilterEffect::preVoiceRendering(int voiceIndex, int startSample, int numSamples)
{
	calculateChain(PolyFilterEffect::FrequencyChain, voiceIndex, startSample, numSamples);
	calculateChain(PolyFilterEffect::GainChain, voiceIndex, startSample, numSamples);
	calculateChain(PolyFilterEffect::BipolarFrequencyChain, voiceIndex, startSample, numSamples);
}

void PolyFilterEffect::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	FilterHelpers::RenderData r(b, startSample, numSamples);
	r.voiceIndex = voiceIndex;
	r.freqModValue = (double)getCurrentModulationValue(FrequencyChain, voiceIndex, startSample);
	auto bipolarFMod = getCurrentModulationValue(BipolarFrequencyChain, voiceIndex, startSample);
	r.freqModValue += (double)(bipolarIntensity * bipolarFMod);
	r.gainModValue = getCurrentModulationValue(GainChain, voiceIndex, startSample);

    voiceFilters.setDisplayModValues(voiceIndex, r.freqModValue, r.gainModValue);
    
	voiceFilters.renderPoly(r);
}

void PolyFilterEffect::startVoice(int voiceIndex, int noteNumber)
{
	VoiceEffectProcessor::startVoice(voiceIndex, noteNumber);

	voiceFilters.reset(voiceIndex);
}

void StaticBiquadSubType::updateCoefficients(double sampleRate, double frequency, double q, double gain)
{
	switch (biquadType)
	{
	case LowPass:		currentCoefficients = IIRCoefficients::makeLowPass(sampleRate, frequency); break;
	case HighPass:		currentCoefficients = IIRCoefficients::makeHighPass(sampleRate, frequency); break;
	case LowShelf:		currentCoefficients = IIRCoefficients::makeLowShelf(sampleRate, frequency, q, (float)gain); break;
	case HighShelf:		currentCoefficients = IIRCoefficients::makeHighShelf(sampleRate, frequency, q, (float)gain); break;
	case Peak:			currentCoefficients = IIRCoefficients::makePeakFilter(sampleRate, frequency, q, (float)gain); break;
	case ResoLow:		currentCoefficients = FilterEffect::makeResoLowPass(sampleRate, frequency, q); break;
	default:							jassertfalse; break;
	}

	for (int i = 0; i < numChannels; i++)
	{
		filters[i].setCoefficients(currentCoefficients);
	}
}


} // namespace hise
