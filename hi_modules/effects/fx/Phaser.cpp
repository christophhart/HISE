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

PhaseFX::PhaseFX(MainController *mc, const String &id) :
MasterEffectProcessor(mc, id),
freq1(400.0f),
freq2(1600.0f),
feedback(0.7f),
mix(1.0f),
phaseModulationChain(new ModulatorChain(mc, "Phase Modulation", 1, Modulation::GainMode, this)),
phaseModulationBuffer(1, 0)
{
	

    parameterNames.add("Frequency1");
    parameterNames.add("Frequency2");
    parameterNames.add("Feedback");
    parameterNames.add("Mix");
    
	editorStateIdentifiers.add("PhaseModulationChainShown");

	updateFrequencies();
}

float PhaseFX::getAttribute(int parameterIndex) const
{
    switch (parameterIndex)
    {
        case Frequency1:			return freq1;
        case Frequency2:			return freq2;
        case Feedback:		return feedback;
        case Mix:			return mix;
        default:			jassertfalse; return 1.0f;
    }
}

void PhaseFX::setInternalAttribute(int parameterIndex, float value)
{
    switch (parameterIndex)
    {
	case Frequency1:	freq1 = value; updateFrequencies(); break;
	case Frequency2:	freq2 = value; updateFrequencies(); break;
	case Feedback:		feedback = value; 
						phaserLeft.setFeedback(value);
						phaserRight.setFeedback(value);
						break;
	case Mix:			mix = value; break;
       default:			jassertfalse; break;
    }
}


void PhaseFX::restoreFromValueTree(const ValueTree &v)
{
    MasterEffectProcessor::restoreFromValueTree(v);
    
    loadAttribute(Frequency1, "Frequency1");
    loadAttribute(Frequency2, "Frequency2");
    loadAttribute(Feedback, "Feedback");
    loadAttribute(Mix, "Mix");
}

ValueTree PhaseFX::exportAsValueTree() const
{
    ValueTree v = MasterEffectProcessor::exportAsValueTree();
    
    saveAttribute(Frequency1, "Frequency1");
    saveAttribute(Frequency2, "Frequency2");
    saveAttribute(Feedback, "Feedback");
    saveAttribute(Mix, "Mix");

    return v;
}

void PhaseFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	ProcessorHelpers::increaseBufferIfNeeded(phaseModulationBuffer, samplesPerBlock);

	phaseModulationChain->prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate > 0.0 && sampleRate != lastSampleRate)
	{
		lastSampleRate = sampleRate;

		phaserLeft.setSampleRate(sampleRate);
		phaserRight.setSampleRate(sampleRate);

		updateFrequencies();
	}
}

void PhaseFX::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	const float *modValues = phaseModulationBuffer.getReadPointer(0, startSample);

	float *l = buffer.getWritePointer(0, startSample);
	float *r = buffer.getWritePointer(1, startSample);

	while (--numSamples >= 0)
	{
		

		*l = *l * (1.0f - mix) + mix * phaserLeft.getNextSample(*l, *modValues);
		*r = *r * (1.0f-mix) + mix * phaserRight.getNextSample(*r, *modValues);
		
		l++;
		r++;
		modValues++;
	}
}

ProcessorEditorBody *PhaseFX::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND
    
    return new PhaserEditor(parentEditor);
    
#else 
    
    ignoreUnused(parentEditor);
    jassertfalse;
    return nullptr;
    
#endif
}


PhaseFX::PhaseModulator::PhaseModulator() :
feedback(.7f),
currentValue(0.f),
sampleRate(-1.0f)
{
	setRange(440.f, 1600.f);
}

void PhaseFX::PhaseModulator::setRange(float freq1, float freq2)
{
	fMin = jmin<float>(freq1, freq2);
	fMax = jmax<float>(freq1, freq2);

	if (sampleRate > 0.0f)
	{
		minDelay = fMin / (sampleRate / 2.f);
		maxDelay = fMax / (sampleRate / 2.f);
	}
}

void PhaseFX::PhaseModulator::setSampleRate(double newSampleRate)
{
	sampleRate = (float)newSampleRate;
	setRange(fMin, fMax);
}

float PhaseFX::PhaseModulator::getNextSample(float input, float modValue)
{
	float delayThisSample = minDelay + (maxDelay - minDelay) * (modValue);

	const float delayCoefficient = AllpassDelay::getDelayCoefficient(delayThisSample);

	allpassFilters[0].setDelay(delayCoefficient);
	allpassFilters[1].setDelay(delayCoefficient);
	allpassFilters[2].setDelay(delayCoefficient);
	allpassFilters[3].setDelay(delayCoefficient);
	allpassFilters[4].setDelay(delayCoefficient);
	allpassFilters[5].setDelay(delayCoefficient);

	float output = allpassFilters[0].getNextSample(
		allpassFilters[1].getNextSample(
		allpassFilters[2].getNextSample(
		allpassFilters[3].getNextSample(
		allpassFilters[4].getNextSample(
		allpassFilters[5].getNextSample(input + currentValue * feedback))))));

	currentValue = output;

	return input + output;
}

} // namespace hise
