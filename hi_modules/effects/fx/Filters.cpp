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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

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
useStateVariableFilters(false),
freqChain(new ModulatorChain(mc, "Freq Modulation", 1, Modulation::GainMode, this)),
gainChain(new ModulatorChain(mc, "Gain Modulation", 1, Modulation::GainMode, this))
{
	editorStateIdentifiers.add("FrequencyChainShown");
	editorStateIdentifiers.add("GainChainShown");
    
    setRenderQuality(256);

	freqChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());
	gainChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());

	parameterNames.add("Gain");
	parameterNames.add("Frequency");
	parameterNames.add("Q");
	parameterNames.add("Mode");
    parameterNames.add("Quality");
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
	default:		jassertfalse; return 1.0f;
	}
}

void MonoFilterEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Gain:		gain = Decibels::decibelsToGain(newValue); break;
	case Frequency:	useFixedFrequency ? currentFreq = newValue : freq = newValue; break;
	case Q:			q = newValue; break;
	case Mode:		setMode((int)newValue);	break;
    case Quality:   setRenderQuality((int)newValue); break;
	default:		jassertfalse; return;
	}

	changeFlag = true;
}

void MonoFilterEffect::restoreFromValueTree(const ValueTree &v)
{
	EffectProcessor::restoreFromValueTree(v);

	loadAttribute(Gain, "Gain");
	loadAttribute(Frequency, "Frequency");
	loadAttribute(Q, "Q");
	loadAttribute(Mode, "Mode");
    loadAttribute(Quality, "RenderQuality");
}

ValueTree MonoFilterEffect::exportAsValueTree() const
{
	ValueTree v = EffectProcessor::exportAsValueTree();

	saveAttribute(Gain, "Gain");
	saveAttribute(Frequency, "Frequency");
	saveAttribute(Q, "Q");
	saveAttribute(Mode, "Mode");
    saveAttribute(Quality, "RenderQuality");
	return v;
}

void MonoFilterEffect::setMode(int filterMode)
{
	mode = (FilterMode)filterMode;
	useStateVariableFilters = (mode == StateVariableHP || mode == StateVariableLP);
	if (useStateVariableFilters)
	{
		stateFilterL.setType(mode == StateVariableHP ? StateVariableFilter::HP : StateVariableFilter::LP);
		stateFilterR.setType(mode == StateVariableHP ? StateVariableFilter::HP : StateVariableFilter::LP);
	}
}

void MonoFilterEffect::calcCoefficients()
{
	if (useStateVariableFilters)
	{
		stateFilterL.setCoefficients((float)currentFreq, (float)q / 10.0f);
		stateFilterR.setCoefficients((float)currentFreq, (float)q / 10.0f);
	}
	else
	{
		if (mode == MoogLP)
		{
			moogL.setCoefficients((float)currentFreq, (float)q);
			moogR.setCoefficients((float)currentFreq, (float)q);
		}
		else
		{
			switch (mode)
			{
			case LowPass:		currentCoefficients = IIRCoefficients::makeLowPass(getSampleRate(), currentFreq); break;
			case HighPass:		currentCoefficients = IIRCoefficients::makeHighPass(getSampleRate(), currentFreq); break;
			case LowShelf:		currentCoefficients = IIRCoefficients::makeLowShelf(getSampleRate(), currentFreq, q, currentGain); break;
			case HighShelf:		currentCoefficients = IIRCoefficients::makeHighShelf(getSampleRate(), currentFreq, q, currentGain); break;
			case Peak:			currentCoefficients = IIRCoefficients::makePeakFilter(getSampleRate(), currentFreq, q, currentGain); break;
			case ResoLow:		currentCoefficients = makeResoLowPass(getSampleRate(), currentFreq, q); break;
			default:			jassertfalse; return;
			}

			filterL.setCoefficients(currentCoefficients);
			filterR.setCoefficients(currentCoefficients);
		}
	}

	changeFlag = false;
}

void MonoFilterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	stateFilterL.setSamplerate((float)sampleRate);
	stateFilterR.setSamplerate((float)sampleRate);

	stateFilterL.reset();
	stateFilterR.reset();

	moogL.reset();
	moogR.reset();

	moogL.setSampleRate((float)sampleRate);
	moogR.setSampleRate((float)sampleRate);

	currentCoefficients = IIRCoefficients::makeLowPass(sampleRate, sampleRate / 2.0);
	
	calcCoefficients();
}

void MonoFilterEffect::processBlockPartial(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
    if (useInternalChains)
    {
        const double lastCurrentFreq = currentFreq;
        
        const float maxFreqInModBuffer = FloatVectorOperations::findMaximum(freqBuffer.getReadPointer(0, 0), numSamples);
        
        currentFreq = 0.15 * jmax<double>(40.0, (double)maxFreqInModBuffer * freq)
        + 0.85 * lastCurrentFreq;
        
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
    
    if (useStateVariableFilters)
    {
        stateFilterL.processSamples(buffer.getWritePointer(0, startSample), numSamples);
        stateFilterR.processSamples(buffer.getWritePointer(1, startSample), numSamples);
    }
    else
    {
        if (mode == MoogLP)
        {
            moogL.processSamples(buffer.getWritePointer(0, startSample), numSamples);
            moogR.processSamples(buffer.getWritePointer(1, startSample), numSamples);
        }
        else
        {
            filterL.processSamples(buffer.getWritePointer(0, startSample), numSamples);
            filterR.processSamples(buffer.getWritePointer(1, startSample), numSamples);
        }
    }
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
gainChain(new ModulatorChain(mc, "Gain Modulation", numVoices, Modulation::GainMode, this))
{
	editorStateIdentifiers.add("FrequencyChainShown");
	editorStateIdentifiers.add("GainChainShown");

	for (int i = 0; i < numVoices; i++)
	{
		voiceFilters.add(new MonoFilterEffect(mc, uid + String(i)));
		voiceFilters[i]->setUseInternalChains(false);
	}
}

float PolyFilterEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case MonoFilterEffect::Gain:		return gain;
	case MonoFilterEffect::Frequency:	return (float)freq;
	case MonoFilterEffect::Q:			return (float)q;
	case MonoFilterEffect::Mode:		return (float)mode;
    case MonoFilterEffect::Quality:		return (float)getSampleAmountForRenderQuality();
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
        case MonoFilterEffect::Quality: setRenderQuality((int)newValue); break;
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
}

ValueTree PolyFilterEffect::exportAsValueTree() const
{
	ValueTree v = EffectProcessor::exportAsValueTree();

	saveAttribute(MonoFilterEffect::Gain, "Gain");
	saveAttribute(MonoFilterEffect::Frequency, "Frequency");
	saveAttribute(MonoFilterEffect::Q, "Q");
	saveAttribute(MonoFilterEffect::Mode, "Mode");
    saveAttribute(MonoFilterEffect::Quality, "Quality");

	return v;
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

	voiceFilters[voiceIndex]->q = q;
}

void PolyFilterEffect::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	const double freqModValue = (double)getCurrentModulationValue(FrequencyChain, voiceIndex, startSample);
	const double checkFreq = std::abs(freqModValue * freq);

    if (mode != MonoFilterEffect::FilterMode::LowPass && mode != MonoFilterEffect::FilterMode::HighPass && gainChain->getNumChildProcessors() > 0)
	{
		const float modulationValue = getCurrentModulationValue(GainChain, voiceIndex, startSample);

		const float modulatedDecibelValue = modulationValue * Decibels::gainToDecibels(gain);

		voiceFilters[voiceIndex]->gain = Decibels::decibelsToGain(modulatedDecibelValue);

	}
	else
	{
		voiceFilters[voiceIndex]->gain = gain;
	}

	voiceFilters[voiceIndex]->currentFreq = checkFreq < 70.0 ? 70.0 : checkFreq;
	voiceFilters[voiceIndex]->freq = checkFreq;
	voiceFilters[voiceIndex]->calcCoefficients();
	voiceFilters[voiceIndex]->applyEffect(b, startSample, numSamples);
}

void PolyFilterEffect::startVoice(int voiceIndex, int noteNumber)
{
	VoiceEffectProcessor::startVoice(voiceIndex, noteNumber);

	if (voiceFilters[voiceIndex]->useStateVariableFilters)
	{
		voiceFilters[voiceIndex]->stateFilterL.reset();
		voiceFilters[voiceIndex]->stateFilterR.reset();

	}
	else
	{
		if (mode == MonoFilterEffect::MoogLP)
		{
			voiceFilters[voiceIndex]->moogL.reset();
			voiceFilters[voiceIndex]->moogR.reset();
		}
		else
		{
			voiceFilters[voiceIndex]->filterL.reset();
			voiceFilters[voiceIndex]->filterR.reset();
		}

		
	}
}

