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
gain(1.0f),
q(1.0),
freq(20000.0),
currentFreq(20000.0),
currentGain(1.0f),
mode(LowPass),
changeFlag(false),
useFixedFrequency(false),
useInternalChains(true),
freqChain(new ModulatorChain(mc, "Freq Modulation", 1, Modulation::GainMode, this)),
gainChain(new ModulatorChain(mc, "Gain Modulation", 1, Modulation::GainMode, this)),
bipolarFreqChain(new ModulatorChain(mc, "Bipolar Freq Mod", 1, Modulation::GainMode, this))
{
	currentFilter = &simpleFilter;

	freqBuffer = AudioSampleBuffer(1, 0);
	gainBuffer = AudioSampleBuffer(1, 0);
	bipolarFreqBuffer = AudioSampleBuffer(1, 0);

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
}

float MonoFilterEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Gain:		return Decibels::gainToDecibels(gain);
	case Frequency:	return (float)freq;
	case Q:			return (float)q;
	case Mode:		return (float)mode;
    case Quality:   return (float)getSampleAmountForRenderQuality();
	case BipolarIntensity: return bipolarIntensity;
	default:		jassertfalse; return 1.0f;
	}
}

void MonoFilterEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Gain:		gain = Decibels::decibelsToGain(newValue); 
					staticBiquadFilter.setGain(gain); 
					stateFilter.setGain(gain); break;
	case Frequency:	useFixedFrequency ? currentFreq = newValue : freq = newValue; break;
	case Q:			q = newValue; break;
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
	case Mode:		return (float)(int)FilterMode::StateVariableLP;
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
	mode = (FilterMode)filterMode;

	calculateGainModValue = false;
	
	switch (mode)
	{
	case MonoFilterEffect::OnePoleLowPass:
		simpleFilter.setType(SimpleOnePole::FilterType::LP);
		currentFilter = &simpleFilter;
		break;
	case MonoFilterEffect::OnePoleHighPass:
		simpleFilter.setType(SimpleOnePole::FilterType::HP);
		currentFilter = &simpleFilter;
		break;
	case MonoFilterEffect::LowPass:
		staticBiquadFilter.setType(StaticBiquad::LowPass);
		currentFilter = &staticBiquadFilter;
		break;
	case MonoFilterEffect::HighPass:
		staticBiquadFilter.setType(StaticBiquad::HighPass);
		currentFilter = &staticBiquadFilter;
		break;
	case MonoFilterEffect::LowShelf:
		staticBiquadFilter.setType(StaticBiquad::LowShelf);
		currentFilter = &staticBiquadFilter;
		calculateGainModValue = true;
		break;
	case MonoFilterEffect::HighShelf:
		staticBiquadFilter.setType(StaticBiquad::HighShelf);
		currentFilter = &staticBiquadFilter;
		calculateGainModValue = true;
		break;
	case MonoFilterEffect::Peak:
		staticBiquadFilter.setType(StaticBiquad::Peak);
		currentFilter = &staticBiquadFilter;
		calculateGainModValue = true;
		break;
	case MonoFilterEffect::ResoLow:
		staticBiquadFilter.setType(StaticBiquad::ResoLow);
		currentFilter = &staticBiquadFilter;
		break;
	case MonoFilterEffect::StateVariableLP:
		stateFilter.setType((int)StateVariableFilter::FilterType::LP);
		currentFilter = &stateFilter;
		break;
	case MonoFilterEffect::StateVariableHP:
		stateFilter.setType((int)StateVariableFilter::FilterType::HP);
		currentFilter = &stateFilter;
		break;
	case MonoFilterEffect::StateVariableBandPass:
		stateFilter.setType((int)StateVariableFilter::FilterType::BP);
		currentFilter = &stateFilter;
		break;
#if JORDAN_HARRIS_SVF
	case MonoFilterEffect::StateVariablePeak:
		stateFilter.setType((int)StateVariableFilter::FilterType::BPShelf);
		currentFilter = &stateFilter;
		break;
	case MonoFilterEffect::StateVariableNotch:
		stateFilter.setType((int)StateVariableFilter::FilterType::Notch);
		currentFilter = &stateFilter;
		break;
	case MonoFilterEffect::StateVariableAllpass:
		stateFilter.setType((int)StateVariableFilter::FilterType::Allpass);
		currentFilter = &stateFilter;
		break;
#endif
	case MonoFilterEffect::MoogLP:
		currentFilter = &moogFilter;
		break;
	default:
		break;
	}
}

void MonoFilterEffect::calcCoefficients()
{
	currentFilter->setFreqAndQ(currentFreq, q);
	staticBiquadFilter.setGain(currentGain);

	changeFlag = false;
}

void MonoFilterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (lastSampleRate != sampleRate)
	{
		lastSampleRate = sampleRate;

		stateFilter.setSampleRate(sampleRate);
		simpleFilter.setSampleRate(sampleRate);
		moogFilter.setSampleRate(sampleRate);
		staticBiquadFilter.setSampleRate(sampleRate);

		calcCoefficients();
	}
}

void MonoFilterEffect::processBlockPartial(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
    if (useInternalChains)
    {
        const double lastCurrentFreq = currentFreq;
        
        const float maxFreqInModBuffer = FloatVectorOperations::findMaximum(freqBuffer.getReadPointer(0, 0), numSamples);
		const double maxFreqInBipolarModBuffer = bipolarFreqChain->getNumChildProcessors() > 0 ? (double)FloatVectorOperations::findMaximum(bipolarFreqBuffer.getReadPointer(0, 0), numSamples) : 0.0;

		double bipolarDelta = 0.0;

		if (bipolarIntensity > 0.0f)
		{
			bipolarDelta = (20000.0 - freq) * maxFreqInBipolarModBuffer * (double)bipolarIntensity;
		}
		else if (bipolarIntensity < 0.0f)
		{
			bipolarDelta = (double)bipolarIntensity * maxFreqInBipolarModBuffer* freq;
		}
		
		const double freqToUse = jlimit<double>(40.0, 20000.0, freq + bipolarDelta);

		currentFreq = 0.15 * jmax<double>(40.0, (double)maxFreqInModBuffer * freqToUse) + 0.85 * lastCurrentFreq;
        
        const float lastCurrentGain = currentGain;
        
        const float modulatedDecibelValue = gainBuffer.getReadPointer(0, 0)[startSample] * Decibels::gainToDecibels(gain);
        
        currentGain = 0.3f * Decibels::decibelsToGain(modulatedDecibelValue) + 0.7f * lastCurrentGain;
        
        calcCoefficients();
    }
    else if (useFixedFrequency)
    {
        currentGain = currentGain * 0.7f + gain * 0.3f;
        calcCoefficients();
    }
    else
    {
        const bool calculateNewCoefficients = changeFlag || fabs(currentFreq - freq) > 0.01 || fabs(currentGain - gain) > 0.01;
        
        if (calculateNewCoefficients)
        {
            currentFreq = currentFreq * 0.85 + freq * 0.15;
            
            currentGain = currentGain * 0.7f + gain * 0.3f;
            
            calcCoefficients();
        }
    }
    
	currentFilter->processSamples(buffer, startSample, numSamples);
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

IIRCoefficients MonoFilterEffect::makeResoLowPass(double sampleRate, double cutoff, double q)
{
	const double c = 1.0 / (tan(double_Pi * (cutoff / sampleRate)));
	const double csq = c * c;

	q = 1 / (3 * q);

	double c1 = 1.0 / (1.0 + (q * c) + (csq));

	return IIRCoefficients(c1,
		2.0 * c1,
		c1,
		1.0,
		(2.0 * c1) * (1.0 - csq),
		c1 * (1.0 - (q * c) + csq));
}

PolyFilterEffect::PolyFilterEffect(MainController *mc, const String &uid, int numVoices) :
VoiceEffectProcessor(mc, uid, numVoices),
mode(MonoFilterEffect::LowPass),
freq(20000.0),
currentFreq(20000.0),
currentGain(1.0f),
gain(1.0f),
q(1.0),
freqChain(new ModulatorChain(mc, "Frequency Modulation", numVoices, Modulation::GainMode, this)),
gainChain(new ModulatorChain(mc, "Gain Modulation", numVoices, Modulation::GainMode, this)),
bipolarFreqChain(new ModulatorChain(mc, "Bipolar Freq Modulation", numVoices, Modulation::GainMode, this))
{
	timeVariantFreqModulatorBuffer = AudioSampleBuffer(1, 0);
	timeVariantGainModulatorBuffer = AudioSampleBuffer(1, 0);
	timeVariantBipolarFreqModulatorBuffer = AudioSampleBuffer(1, 0);

	editorStateIdentifiers.add("FrequencyChainShown");
	editorStateIdentifiers.add("GainChainShown");
	editorStateIdentifiers.add("BipolarFreqChainShown");

	for (int i = 0; i < numVoices; i++)
	{
		voiceFilters.add(new MonoFilterEffect(mc, uid + String(i)));
		voiceFilters[i]->setUseInternalChains(false);
	}
    
    parameterNames.add("Gain");
    parameterNames.add("Frequency");
    parameterNames.add("Q");
    parameterNames.add("Mode");
    parameterNames.add("Quality");
	parameterNames.add("BipolarIntensity");
}

float PolyFilterEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case MonoFilterEffect::Gain:		return Decibels::gainToDecibels(gain);
	case MonoFilterEffect::Frequency:	return (float)freq;
	case MonoFilterEffect::Q:			return (float)q;
	case MonoFilterEffect::Mode:		return (float)mode;
    case MonoFilterEffect::Quality:		return (float)getSampleAmountForRenderQuality();
	case MonoFilterEffect::BipolarIntensity: return bipolarIntensity;
	default:							jassertfalse; return 1.0f;
	}
}

void PolyFilterEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case MonoFilterEffect::Gain:		gain = Decibels::decibelsToGain(newValue); break;
	case MonoFilterEffect::Frequency:	freq = newValue; break;
	case MonoFilterEffect::Q:			q = newValue; break;
	case MonoFilterEffect::Mode:		mode = (MonoFilterEffect::FilterMode)(int)newValue;
										for (int i = 0; i < voiceFilters.size(); i++) voiceFilters[i]->setMode((int)newValue);
										break;
    case MonoFilterEffect::Quality:		setRenderQuality((int)newValue); break;
	case MonoFilterEffect::BipolarIntensity: bipolarIntensity = jlimit<float>(-1.0f, 1.0f, newValue); break;
	default:							jassertfalse; return;
	}

	changeFlag = true;
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

	for (int i = 0; i < voiceFilters.size(); i++)
	{
		voiceFilters[i]->prepareToPlay(sampleRate, samplesPerBlock);
	}
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

void PolyFilterEffect::preVoiceRendering(int voiceIndex, int startSample, int numSamples)
{
	calculateChain(PolyFilterEffect::FrequencyChain, voiceIndex, startSample, numSamples);

	calculateChain(PolyFilterEffect::GainChain, voiceIndex, startSample, numSamples);

	calculateChain(PolyFilterEffect::BipolarFrequencyChain, voiceIndex, startSample, numSamples);

	voiceFilters[voiceIndex]->q = q;
}

void PolyFilterEffect::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	if (voiceFilters[voiceIndex]->calculateGainModValue)
	{
		if (gainChain->getNumChildProcessors() > 0)
		{
			const float modulationValue = getCurrentModulationValue(GainChain, voiceIndex, startSample);

			const float modulatedDecibelValue = modulationValue * Decibels::gainToDecibels(gain);

			voiceFilters[voiceIndex]->currentGain = Decibels::decibelsToGain(modulatedDecibelValue);
		}
		else
		{
			voiceFilters[voiceIndex]->currentGain = gain;
		}
	}
	
	const double freqModValue = (double)getCurrentModulationValue(FrequencyChain, voiceIndex, startSample);
	const double bipolarFreqModValue = (double)getCurrentModulationValue(BipolarFrequencyChain, voiceIndex, startSample);

	double bipolarDelta = 0.0;

	if (bipolarIntensity > 0.0f)
		bipolarDelta = (20000.0 - freq) * bipolarFreqModValue * (double)bipolarIntensity;
	else if (bipolarIntensity < 0.0f)
		bipolarDelta = (double)bipolarIntensity * bipolarFreqModValue * freq;

	const double freqToUse = jlimit<double>(40.0, 20000.0, freq + bipolarDelta);

	const double checkFreq = jmax<double>(70.0, std::abs(freqModValue * freqToUse));

	voiceFilters[voiceIndex]->currentFreq = checkFreq;
	voiceFilters[voiceIndex]->freq = checkFreq;

	voiceFilters[voiceIndex]->calcCoefficients();

	voiceFilters[voiceIndex]->currentFilter->processSamples(b, startSample, numSamples);

	//voiceFilters[voiceIndex]->applyEffect(b, startSample, numSamples);
}

void PolyFilterEffect::startVoice(int voiceIndex, int noteNumber)
{
	VoiceEffectProcessor::startVoice(voiceIndex, noteNumber);

	voiceFilters[voiceIndex]->currentFilter->reset();
}

void StaticBiquad::updateCoefficients()
{
	FilterType mode = (FilterType)type;

	switch (mode)
	{
	case StaticBiquad::LowPass:			currentCoefficients = IIRCoefficients::makeLowPass(sampleRate, frequency); break;
	case StaticBiquad::HighPass:		currentCoefficients = IIRCoefficients::makeHighPass(sampleRate, frequency); break;
	case StaticBiquad::LowShelf:		currentCoefficients = IIRCoefficients::makeLowShelf(sampleRate, frequency, q, (float)gain); break;
	case StaticBiquad::HighShelf:		currentCoefficients = IIRCoefficients::makeHighShelf(sampleRate, frequency, q, (float)gain); break;
	case StaticBiquad::Peak:			currentCoefficients = IIRCoefficients::makePeakFilter(sampleRate, frequency, q, (float)gain); break;
	case StaticBiquad::ResoLow:			currentCoefficients = MonoFilterEffect::makeResoLowPass(sampleRate, frequency, q); break;
	default:							jassertfalse; break;
	}

	for (int i = 0; i < numChannels; i++)
	{
		filters[i].setCoefficients(currentCoefficients);
	}
}

} // namespace hise
