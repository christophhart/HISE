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

GainEffect::GainEffect(MainController *mc, const String &uid) :
MasterEffectProcessor(mc, uid),
gainChain(new ModulatorChain(mc, "Gain Modulation", 1, Modulation::GainMode, this)),
delayChain(new ModulatorChain(mc, "Delay Modulation", 1, Modulation::GainMode, this)),
widthChain(new ModulatorChain(mc, "Width Modulation", 1, Modulation::GainMode, this)),
balanceChain(new ModulatorChain(mc, "Pan Modulation", 1, Modulation::GainMode, this)),
gain(1.0f),
delay(0.0f),
balance(0.0f),
gainBuffer(1, 0),
delayBuffer(1, 0),
widthBuffer(1, 0),
balanceBuffer(1, 0),
smoothedGainL(1.0f),
smoothedGainR(1.0f)
{
	
	smoother.setSmoothingTime(0.2f);

	parameterNames.add("Gain");
    parameterNames.add("Delay");
    parameterNames.add("Width");
	parameterNames.add("Balance");

	editorStateIdentifiers.add("GainChainShown");
    editorStateIdentifiers.add("DelayChainShown");
    editorStateIdentifiers.add("WidthChainShown");
	editorStateIdentifiers.add("BalanceChainShown");

	gainChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));
    widthChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));
    delayChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));
	balanceChain->setFactoryType(new TimeVariantModulatorFactoryType(Modulation::GainMode, this));
    
	auto tmp = WeakReference<Processor>(this);

	auto balanceConverter = [tmp](float input)
	{
		if (tmp.get() != nullptr)
		{
			auto normalized = (input - 0.5f) * 2.0f;
			auto v = tmp->getAttribute(GainEffect::Parameters::Balance) * normalized;

			return BalanceCalculator::getBalanceAsString(roundFloatToInt(v));
		}

		return Table::getDefaultTextValue(input);
	};

	balanceChain->setTableValueConverter(balanceConverter);

	auto widthConverter = [tmp](float input)
	{
		if (tmp)
		{
			auto v = tmp->getAttribute(GainEffect::Parameters::Width) / 100.0f;
			const float thisWidth = (v - 1.0f) * input + 1.0f;
			return String(roundFloatToInt(thisWidth*100.0f)) + "%";
		}

		return Table::getDefaultTextValue(input);
	};

	widthChain->setTableValueConverter(widthConverter);

	auto gainConverter = [tmp](float input)
	{
		if (tmp)
		{
			auto v = Decibels::decibelsToGain(tmp->getAttribute(GainEffect::Parameters::Gain));
			auto dbValue = Decibels::gainToDecibels(v * input);
			return String(dbValue, 1) + " dB";
		}

		return Table::getDefaultTextValue(input);
	};

	gainChain->setTableValueConverter(gainConverter);

	auto delayConverter = [tmp](float input)
	{
		if (tmp)
		{
			auto v = Decibels::decibelsToGain(tmp->getAttribute(GainEffect::Parameters::Delay));
			auto dbValue = Decibels::gainToDecibels(v * input);
			return String(roundFloatToInt(v)) + " ms";
		}

		return Table::getDefaultTextValue(input);
	};

	delayChain->setTableValueConverter(delayConverter);



}

GainEffect::~GainEffect()
{
    
}
    
void GainEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Gain:							gain = Decibels::decibelsToGain(newValue); 
										smoothedGainL.setValue(gain);
										smoothedGainR.setValue(gain);
										break;
    case Delay:                         setDelayTime(newValue); break;
    case Width:                         msDecoder.setWidth(newValue/100.0f); break;
	case Balance:						balance = newValue; break;
	default:							jassertfalse; return;
	}
}

float GainEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Gain:							return Decibels::gainToDecibels(gain);
    case Delay:                         return delay;
    case Width:                         return msDecoder.getWidth() * 100.0f;
	case Balance:						return balance;
	default:							jassertfalse; return 1.0f;
	}
}

void GainEffect::restoreFromValueTree(const ValueTree &v)
{
	MasterEffectProcessor::restoreFromValueTree(v);

	loadAttribute(Gain, "Gain");
    loadAttribute(Delay, "Delay");
    loadAttribute(Width, "Width");
	loadAttribute(Balance, "Balance");
}

ValueTree GainEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(Gain, "Gain");
    saveAttribute(Delay, "Delay");
    saveAttribute(Width, "Width");
	saveAttribute(Balance, "Balance");

	return v;
}

ProcessorEditorBody *GainEffect::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new GainEditor(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


void GainEffect::applyEffect(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	const int samplesToCopy = numSamples;
	const int startIndex = startSample;

	float *l = buffer.getWritePointer(0, startIndex);
	float *r = buffer.getWritePointer(1, startIndex);

	if (!delayChain->isBypassed() && delayChain->getNumChildProcessors() != 0)
	{
		const float thisDelayTime = delay * delayBuffer.getSample(0, 0);

		leftDelay.setDelayTimeSeconds(thisDelayTime / 1000.0f);
		rightDelay.setDelayTimeSeconds(thisDelayTime / 1000.0f);
	}


	if (delay != 0)
	{
		leftDelay.processBlock(l, numSamples);
		smoothedGainL.applyGain(l, numSamples);

		rightDelay.processBlock(r, numSamples);
		smoothedGainR.applyGain(r, numSamples);
	}
	else
	{
		smoothedGainL.applyGain(l, numSamples);
		smoothedGainR.applyGain(r, numSamples);
	}

	if (msDecoder.getWidth() != 1.0f)
	{
		numSamples = samplesToCopy;

		l = buffer.getWritePointer(0, startIndex);
		r = buffer.getWritePointer(1, startIndex);

		if (!widthChain->isBypassed() && widthChain->getNumChildProcessors() != 0)
		{
			const float thisWidth = (msDecoder.getWidth() - 1.0f) * widthBuffer.getSample(0, 0) + 1.0f;

			msDecoder.setWidth(thisWidth);
		}

		while (numSamples > 0)
		{
			msDecoder.calculateStereoValues(l[0], r[0]);
			msDecoder.calculateStereoValues(l[1], r[1]);
			msDecoder.calculateStereoValues(l[2], r[2]);
			msDecoder.calculateStereoValues(l[3], r[3]);

			l += 4;
			r += 4;

			numSamples -= 4;
		}
	}

	if (!gainChain->isBypassed() && gainChain->getNumChildProcessors() != 0)
	{
		FloatVectorOperations::multiply(buffer.getWritePointer(0, startIndex), gainBuffer.getReadPointer(0, startIndex), samplesToCopy);
		FloatVectorOperations::multiply(buffer.getWritePointer(1, startIndex), gainBuffer.getReadPointer(0, startIndex), samplesToCopy);
	}

	if (!balanceChain->isBypassed() && balanceChain->getNumChildProcessors() != 0)
	{
		BalanceCalculator::processBuffer(buffer, balanceBuffer.getWritePointer(0, startIndex), startIndex, samplesToCopy);
	}
	else
	{
		const float smoothedBalance = balanceSmoother.smooth(balance);

		const float leftGain = BalanceCalculator::getGainFactorForBalance(smoothedBalance, true);
		const float rightGain = BalanceCalculator::getGainFactorForBalance(smoothedBalance, false);

		if(leftGain != rightGain)
		{
			FloatVectorOperations::multiply(buffer.getWritePointer(0, startIndex), leftGain, samplesToCopy);
			FloatVectorOperations::multiply(buffer.getWritePointer(1, startIndex), rightGain, samplesToCopy);
		}
	}
}


void GainEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate > 0)
	{
        gainChain->prepareToPlay(sampleRate, samplesPerBlock);
        delayChain->prepareToPlay(sampleRate, samplesPerBlock);
        widthChain->prepareToPlay(sampleRate, samplesPerBlock);
		balanceChain->prepareToPlay(sampleRate, samplesPerBlock);
        
		ProcessorHelpers::increaseBufferIfNeeded(gainBuffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(delayBuffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(widthBuffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(balanceBuffer, samplesPerBlock);

        leftDelay.prepareToPlay(sampleRate);
        rightDelay.prepareToPlay(sampleRate);
        
        leftDelay.setFadeTimeSamples(samplesPerBlock);
        rightDelay.setFadeTimeSamples(samplesPerBlock);
        
		smoother.prepareToPlay(sampleRate);
		smoother.setSmoothingTime(4.0);

		smoothedGainL.reset(44100, 0.2);
		smoothedGainR.reset(44100, 0.2);

		balanceSmoother.prepareToPlay(sampleRate / (double)samplesPerBlock);
		balanceSmoother.setSmoothingTime(1000.0f);
	}
}

ProcessorEditorBody * EmptyFX::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}
} // namespace hise
