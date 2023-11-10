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
mix(1.0f)
{
	modChains += { this, "Phase Modulation" };

	finaliseModChains();

	phaseModulationChain = modChains[InternalChains::PhaseModulationChain].getChain();
	modChains[InternalChains::PhaseModulationChain].setExpandToAudioRate(true);

	WeakReference<Processor> tmp(this);

	auto f = [tmp](float input)
	{
		if (tmp != nullptr)
		{
			float freq1 = tmp->getAttribute(PhaseFX::Attributes::Frequency1);
			float freq2 = tmp->getAttribute(PhaseFX::Attributes::Frequency2);

			float v = input * (freq2 - freq1) + freq1;

			return HiSlider::getFrequencyString(v);
		}

		return Table::getDefaultTextValue(input);
	};

	phaseModulationChain->setTableValueConverter(f);

    parameterNames.add("Frequency1");
    parameterNames.add("Frequency2");
    parameterNames.add("Feedback");
    parameterNames.add("Mix");

	updateParameterSlots();

	editorStateIdentifiers.add("PhaseModulationChainShown");

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
	case Frequency1:	freq1Smoothed.setValue(value); freq1 = value; break;
	case Frequency2:	freq2Smoothed.setValue(value); freq2 = value; break;
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

	if (sampleRate > 0.0 && sampleRate != lastSampleRate)
	{
		lastSampleRate = sampleRate;

		freq1Smoothed.setValueAndRampTime(freq1, sampleRate / (double)samplesPerBlock, 0.05);
		freq2Smoothed.setValueAndRampTime(freq2, sampleRate / (double)samplesPerBlock, 0.05);

		phaserLeft.setSampleRate(sampleRate);
		phaserRight.setSampleRate(sampleRate);
	}
}



void PhaseFX::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	updateFrequencies();

	float *l = buffer.getWritePointer(0, startSample);
	float *r = buffer.getWritePointer(1, startSample);

	const float invMix = 1.0f - mix;

	if (auto modValues = modChains[InternalChains::PhaseModulationChain].getReadPointerForVoiceValues(startSample))
	{
		while (--numSamples >= 0)
		{
			*l = *l * invMix + mix * phaserLeft.getNextSample(*l, *modValues);
			*r = *r * invMix + mix * phaserRight.getNextSample(*r, *modValues);

			l++;
			r++;
			modValues++;
		}
	}
	else
	{
		const float modValue = modChains[InternalChains::PhaseModulationChain].getConstantModulationValue();
		phaserLeft.setConstDelay(modValue);
		phaserRight.setConstDelay(modValue);

		while (--numSamples >= 0)
		{
			*l = *l * invMix + mix * phaserLeft.getNextSample(*l);
			*r = *r * invMix + mix * phaserRight.getNextSample(*r);

			l++;
			r++;
		}
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


void PhaseFX::updateFrequencies()
{
	const float sf1 = freq1Smoothed.getNextValue();
	const float sf2 = freq2Smoothed.getNextValue();

	phaserLeft.setRange(sf1, sf2);
	phaserRight.setRange(sf1, sf2);
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
	setConstDelay(modValue);

	return getNextSample(input);
}

void PhaseFX::PhaseModulator::setConstDelay(float modValue)
{
	float delayThisSample = minDelay + (maxDelay - minDelay) * (modValue);

	const float delayCoefficient = AllpassDelay::getDelayCoefficient(delayThisSample);

	allpassFilters[0].setDelay(delayCoefficient);
	allpassFilters[1].setDelay(delayCoefficient);
	allpassFilters[2].setDelay(delayCoefficient);
	allpassFilters[3].setDelay(delayCoefficient);
	allpassFilters[4].setDelay(delayCoefficient);
	allpassFilters[5].setDelay(delayCoefficient);
}



float PhaseFX::PhaseModulator::getNextSample(float input)
{
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
