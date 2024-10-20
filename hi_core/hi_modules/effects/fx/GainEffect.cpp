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
gain(1.0f),
delay(0.0f),
balance(0.0f),
smoothedGainL(1.0f),
smoothedGainR(1.0f)
{
	modChains.reserve(InternalChains::numInternalChains);

	modChains += {this, "Gain Modulation"};
	modChains += {this, "Delay Modulation"};
	modChains += {this, "Width Modulation"};
	modChains += {this, "Pan Modulation", ModulatorChain::ModulationType::Normal, Modulation::PanMode};

	finaliseModChains();

	gainChain = modChains[InternalChains::GainChain].getChain();
	delayChain = modChains[InternalChains::DelayChain].getChain();
	widthChain = modChains[InternalChains::WidthChain].getChain();
	balanceChain = modChains[InternalChains::BalanceChain].getChain();

	smoother.setSmoothingTime(0.2f);

	parameterNames.add("Gain");
    parameterNames.add("Delay");
    parameterNames.add("Width");
	parameterNames.add("Balance");
	parameterNames.add("InvertPolarity");

	updateParameterSlots();

	editorStateIdentifiers.add("GainChainShown");
    editorStateIdentifiers.add("DelayChainShown");
    editorStateIdentifiers.add("WidthChainShown");
	editorStateIdentifiers.add("BalanceChainShown");

	auto tmp = WeakReference<Processor>(this);

    auto balanceConverter = [tmp](float input)
	{
		if (tmp.get() != nullptr)
		{
			auto v = tmp->getAttribute(GainEffect::Parameters::Balance) * input;

			return BalanceCalculator::getBalanceAsString(roundToInt(v));
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
			return String(roundToInt(thisWidth*100.0f)) + "%";
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
			return String(roundToInt(v)) + " ms";
		}

		return Table::getDefaultTextValue(input);
	};

	delayChain->setTableValueConverter(delayConverter);
}

GainEffect::~GainEffect()
{
	modChains.clear();
}
    
void GainEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Gain:							gain = Decibels::decibelsToGain(newValue); 
										break;
    case Delay:                         setDelayTime(newValue); break;
    case Width:                         msDecoder.setWidth(newValue/100.0f); break;
	case Balance:						balance = newValue; break;
	case InvertPolarity:				invertPolarity = newValue; break;
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
	case InvertPolarity:				return invertPolarity ? 1.0f : 0.0f;
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
	loadAttributeWithDefault(InvertPolarity);
}

ValueTree GainEffect::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	saveAttribute(Gain, "Gain");
    saveAttribute(Delay, "Delay");
    saveAttribute(Width, "Width");
	saveAttribute(Balance, "Balance");
	saveAttribute(InvertPolarity, "InvertPolarity");

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
	if (invertPolarity)
		buffer.applyGain(-1.0f);

	const int samplesToCopy = numSamples;
	const int startIndex = startSample;

	float *l = buffer.getWritePointer(0, startIndex);
	float *r = buffer.getWritePointer(1, startIndex);

	const float gainModValue = modChains[InternalChains::GainChain].getOneModulationValue(startSample);

	smoothedGainL.setValue(gain * gainModValue);
	smoothedGainR.setValue(gain * gainModValue);

	const float delayModValue = modChains[InternalChains::DelayChain].getOneModulationValue(startSample);

	if (delayModValue != 1.0f)
	{
		const float thisDelayTime = delay * delayModValue;

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

		const float widthModValue = modChains[InternalChains::WidthChain].getOneModulationValue(startSample);

		if (widthModValue != 1.0f)
		{
			const float thisWidth = (msDecoder.getWidth() - 1.0f) * widthModValue + 1.0f;
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


	auto& bc = modChains[InternalChains::BalanceChain];

	float smoothedBalance = balanceSmoother.smooth(balance);

	if (bc.getChain()->shouldBeProcessedAtAll())
		smoothedBalance *= bc.getOneModulationValue(startSample);

	const float leftGain = BalanceCalculator::getGainFactorForBalance(smoothedBalance, true);
	const float rightGain = BalanceCalculator::getGainFactorForBalance(smoothedBalance, false);

	if (leftGain != rightGain)
	{
		FloatVectorOperations::multiply(buffer.getWritePointer(0, startIndex), leftGain, samplesToCopy);
		FloatVectorOperations::multiply(buffer.getWritePointer(1, startIndex), rightGain, samplesToCopy);
	}


#if ENABLE_PEAK_METERS_FOR_GAIN_EFFECT
	currentValues.outL = buffer.getMagnitude(0, startIndex, samplesToCopy);
	currentValues.outR = buffer.getMagnitude(1, startIndex, samplesToCopy);
#endif
}


void GainEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate > 0)
	{
        leftDelay.prepareToPlay(sampleRate);
        rightDelay.prepareToPlay(sampleRate);
        
        leftDelay.setFadeTimeSamples(samplesPerBlock);
        rightDelay.setFadeTimeSamples(samplesPerBlock);
        
		smoother.prepareToPlay(sampleRate);
		smoother.setSmoothingTime(4.0);

		smoothedGainL.reset(sampleRate, 0.05);
		smoothedGainR.reset(sampleRate, 0.05);

		balanceSmoother.prepareToPlay(sampleRate / (double)samplesPerBlock);
		balanceSmoother.setSmoothingTime(1000.0f);

		smoothedGainL.setValueWithoutSmoothing(gain);
		smoothedGainR.setValueWithoutSmoothing(gain);
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

#if USE_BACKEND

class MetronomeEditorBody : public ProcessorEditorBody,
							public ComboBoxListener
{
public:

	MetronomeEditorBody(ProcessorEditor* parentEditor) :
		ProcessorEditorBody(parentEditor),
		enableButton("Enabled"),
		gainSlider("Volume"),
		noiseSlider("Noise")
	{
		addAndMakeVisible(enableButton);
		enableButton.setup(getProcessor(), MidiMetronome::Enabled, "Enabled");
		addAndMakeVisible(midiPlayerSelector);
		getProcessor()->getMainController()->skin(midiPlayerSelector);

		midiPlayerSelector.setTextWhenNothingSelected("Select MIDI Player");

		auto list = ProcessorHelpers::getListOfAllProcessors<MidiPlayer>(getProcessor()->getMainController()->getMainSynthChain());

		gainSlider.setup(getProcessor(), MidiMetronome::Volume, "Volume");
		gainSlider.setMode(HiSlider::Mode::Decibel);
		noiseSlider.setup(getProcessor(), MidiMetronome::NoiseAmount, "Noise");
		noiseSlider.setMode(HiSlider::NormalizedPercentage);

		noiseSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
		noiseSlider.setTextBoxStyle(Slider::TextBoxRight, false, 80, 20);

		gainSlider.setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
		gainSlider.setTextBoxStyle(Slider::TextBoxRight, false, 80, 20);

		addAndMakeVisible(gainSlider);
		addAndMakeVisible(noiseSlider);

		int index = 1;

		for (auto l : list)
		{
			midiPlayerSelector.addItem(l->getId(), index++);
		}

		midiPlayerSelector.addListener(this);

	}

	void comboBoxChanged(ComboBox*) override
	{
		dynamic_cast<MidiMetronome*>(getProcessor())->connectToPlayer(midiPlayerSelector.getText());
	}

	void updateGui() override
	{
		enableButton.updateValue();
		gainSlider.updateValue();
		noiseSlider.updateValue();

		midiPlayerSelector.setText(dynamic_cast<MidiMetronome*>(getProcessor())->getConnectedId(), dontSendNotification);
	}

	int getBodyHeight() const override { return 48; };

	void resized() override
	{
		auto b = getLocalBounds();
		enableButton.setBounds(b.removeFromLeft(130).reduced(8));
		b.removeFromLeft(20);
		midiPlayerSelector.setBounds(b.removeFromLeft(150).reduced(8));
		b.removeFromLeft(20);
		gainSlider.setBounds(b.removeFromLeft(120));
		b.removeFromLeft(10);
		noiseSlider.setBounds(b.removeFromLeft(120));
	}

	HiToggleButton enableButton;
	HiSlider gainSlider;
	HiSlider noiseSlider;
	ComboBox midiPlayerSelector;
};

#endif

hise::ProcessorEditorBody * MidiMetronome::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new MetronomeEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

} // namespace hise
