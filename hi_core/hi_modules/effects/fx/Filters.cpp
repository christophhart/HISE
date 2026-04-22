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


hise::ProcessorMetadata PolyFilterEffect::createMetadata()
{
	using Par = ProcessorMetadata::ParameterMetadata;
	using Mod = ProcessorMetadata::ModulationMetadata;
	using Range = scriptnode::InvertableParameterRange;

	return ProcessorMetadata()
		.withStandardMetadata<PolyFilterEffect>()
		.withDescription("Applies monophonic or polyphonic filtering with modulatable frequency, gain, and resonance, supporting multiple filter types.")
		.withParameter(Par(Gain)
			.withId("Gain")
			.withDescription("Filter gain in decibels for shelf and peak filter types")
			.withSliderMode(HiSlider::Decibel, Range(-18.0, 18.0))
			.withDefault(0.0f))
		.withParameter(Par(Frequency)
			.withId("Frequency")
			.withDescription("The cutoff or center frequency of the filter in Hz")
			.withSliderMode(HiSlider::Frequency, Range(20.0, 20000.0).withCentreSkew(1500.0))
			.withDefault(20000.0f))
		.withParameter(Par(Q)
			.withId("Q")
			.withDescription("The resonance or bandwidth of the filter")
			.withSliderMode(HiSlider::Linear, Range(0.3, 8.0).withCentreSkew(1.0))
			.withDefault(1.0f))
		.withParameter(Par(Mode)
			.withId("Mode")
			.withDescription("Selects the filter type (low-pass, high-pass, shelf, peak, etc.)")
			.withSliderMode(HiSlider::Discrete, Range(0.0, (double)FilterBank::numFilterModes - 1.0, 1.0))
			.withDefault((float)(int)FilterBank::FilterMode::StateVariableLP))
		.withParameter(Par(Quality)
			.withId("Quality")
			.withDescription("Internal render quality as power-of-two buffer size for modulation processing")
			.withSliderMode(HiSlider::Discrete, Range(0.0, 4096.0, 1.0))
			.withDefault(512.0f))
		.withParameter(Par(BipolarIntensity)
			.withId("BipolarIntensity")
			.withDescription("Intensity of the bipolar frequency modulation (-1 to 1)")
			.withSliderMode(HiSlider::Linear, Range(-1.0, 1.0))
			.withDefault(0.0f))
		.withModulation(Mod(FrequencyChain)
			.withId("Frequency Modulation")
			.withDescription("Modulates the filter cutoff frequency")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
			.withModulatedParameter(Frequency))
		.withModulation(Mod(GainChain)
			.withId("Gain Modulation")
			.withDescription("Modulates the filter gain")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
			.withModulatedParameter(Gain))
		.withModulation(Mod(BipolarFrequencyChain)
			.withId("Bipolar Freq Modulation")
			.withDescription("Bipolar modulation of the filter frequency")
			.withMode(scriptnode::modulation::ParameterMode::AddOnly)
			.withModulatedParameter(BipolarIntensity))
		.withModulation(Mod(ResonanceChain)
			.withId("Q Modulation")
			.withDescription("Modulates the filter resonance")
			.withMode(scriptnode::modulation::ParameterMode::ScaleOnly)
			.withModulatedParameter(Q));
}

PolyFilterEffect::PolyFilterEffect(MainController *mc, const String &uid, int numVoices) :
	VoiceEffectProcessor(mc, uid, numVoices),
	FilterEffect(mc),
	metadataInitialised(updateParameterSlots()),
	voiceFilters(numVoices),
	monoFilters(1),
	frequency(getDefaultValue(PolyFilterEffect::Parameters::Frequency)),
	q(getDefaultValue(PolyFilterEffect::Parameters::Q)),
	gain(getDefaultValue(PolyFilterEffect::Parameters::Gain)),
	mode((FilterBank::FilterMode)(int)getDefaultValue(PolyFilterEffect::Parameters::Mode))
{
	getFilterData(0)->getUpdater().setUpdater(mc->getGlobalUIUpdater());

	modChains.reserve(numInternalChains);

	modChains += {this, "Frequency Modulation"};
	modChains += {this, "Gain Modulation"};
	modChains += {this, "Bipolar Freq Modulation", ModulatorChain::ModulationType::Normal, Modulation::OffsetMode};
	modChains += {this, "Q Modulation"};

	finaliseModChains();

	for (auto& mb : modChains)
		mb.getChain()->getHandler()->addPostEventListener(this);

	WeakReference<Processor> t = this;

	auto f = [t](float input)
	{
		if (t != nullptr)
		{
			auto freq = t->getAttribute(PolyFilterEffect::Parameters::Frequency);
			auto v = jmap<float>(input, 20.0f, freq);
			return ValueToTextConverter::ConverterFunctions::Frequency(v);
		}

		return Table::getDefaultTextValue(input);
	};

	modChains[InternalChains::FrequencyChain].getChain()->setTableValueConverter(f);
	modChains[InternalChains::BipolarFrequencyChain].getChain()->setTableValueConverter(f);

	auto fg = [t](float input)
	{
		if (t != nullptr)
		{
			auto g = t->getAttribute(PolyFilterEffect::Parameters::Gain);
			auto v = (input - 0.5f) * 2.0f * g;
			return String(v, 1) + " dB";
		}

		return Table::getDefaultTextValue(input);
	};

	modChains[GainChain].getChain()->setTableValueConverter(fg);

	editorStateIdentifiers.add("FrequencyChainShown");
	editorStateIdentifiers.add("GainChainShown");
	editorStateIdentifiers.add("BipolarFreqChainShown");
    
	voiceFilters.setMode((FilterBank::FilterMode)(int)getDefaultValue(PolyFilterEffect::Mode));
	monoFilters.setMode((FilterBank::FilterMode)(int)getDefaultValue(PolyFilterEffect::Mode));

	registerAtObject(getFilterData(0));
}

PolyFilterEffect::~PolyFilterEffect()
{
	deregisterAtObject(getFilterData(0));

	for (auto& mb : modChains)
		mb.getChain()->getHandler()->removePostEventListener(this);

	modChains.clear();
}

void PolyFilterEffect::processorChanged(EventType /*t*/, Processor* /*p*/)
{
	bool before = polyMode;
	polyMode = false;

	for (auto& mb : modChains)
	{
		if (mb.getChain()->hasActivePolyMods())
		{
			polyMode = true;
			break;
		}
	}

	if (polyMode != before)
	{
		setInternalAttribute(PolyFilterEffect::Parameters::Frequency, frequency);
		setInternalAttribute(PolyFilterEffect::Parameters::Q, q);
		setInternalAttribute(PolyFilterEffect::Parameters::Gain, gain);
		setInternalAttribute(PolyFilterEffect::Parameters::Mode, (float)(int)mode);
	}
}

float PolyFilterEffect::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case PolyFilterEffect::Gain:		return gain;
	case PolyFilterEffect::Frequency:	return frequency;
	case PolyFilterEffect::Q:			return q;
	case PolyFilterEffect::Mode:		return (float)(int)mode;
    case PolyFilterEffect::Quality:		return (float)getSampleAmountForRenderQuality();
	case PolyFilterEffect::BipolarIntensity: return bipolarParameterValue;
	default:							jassertfalse; return 1.0f;
	}
}

void PolyFilterEffect::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case PolyFilterEffect::Gain:		
		gain = newValue;
		monoFilters.setGain(Decibels::decibelsToGain(newValue));

		if(hasPolyMods())
			voiceFilters.setGain(Decibels::decibelsToGain(newValue));

		break;
	case PolyFilterEffect::Frequency:	
		frequency = newValue;
		monoFilters.setFrequency(newValue);

		if(hasPolyMods())
			voiceFilters.setFrequency(newValue);

		break;
	case PolyFilterEffect::Q:			
		q = newValue;
		monoFilters.setQ(newValue);

		if(hasPolyMods())
			voiceFilters.setQ(newValue);

		break;
	case PolyFilterEffect::Mode:		
		mode = (FilterBank::FilterMode)(int)newValue;
		monoFilters.setMode(mode);

		if(hasPolyMods())
			voiceFilters.setMode(mode);

		break;
    case PolyFilterEffect::Quality:		setRenderQuality((int)newValue); break;
	case PolyFilterEffect::BipolarIntensity:
	{
		bipolarParameterValue = jlimit<float>(-1.0f, 1.0f, newValue);
		bipolarIntensity.setTargetValue(bipolarParameterValue); break;
		break;
	}
		
	default:							jassertfalse; return;
	}

	changeFlag = true;

	handleFilterStatisticUpdate();
}


ModulationDisplayValue::QueryFunction::Ptr PolyFilterEffect::getModulationQueryFunction(int parameterIndex) const
{
	switch(parameterIndex)
	{
	case PolyFilterEffect::Frequency:
		return new ModulatorChain::GetModulationOutput<(int)InternalChains::FrequencyChain>();
	case PolyFilterEffect::Q:
		return new ModulatorChain::GetModulationOutput<(int)InternalChains::ResonanceChain>();
	case PolyFilterEffect::Gain:
		return new ModulatorChain::GetModulationOutput<(int)InternalChains::GainChain>();
	case PolyFilterEffect::BipolarIntensity:
		return new ModulatorChain::GetModulationOutput<(int)InternalChains::BipolarFrequencyChain>();
	}

	return VoiceEffectProcessor::getModulationQueryFunction(parameterIndex);
}

void PolyFilterEffect::restoreFromValueTree(const ValueTree &v)
{
	EffectProcessor::restoreFromValueTree(v);

	loadAttribute(PolyFilterEffect::Gain, "Gain");
	loadAttribute(PolyFilterEffect::Frequency, "Frequency");
	loadAttribute(PolyFilterEffect::Q, "Q");
	loadAttribute(PolyFilterEffect::Mode, "Mode");
    loadAttribute(PolyFilterEffect::Quality, "Quality");
	loadAttribute(PolyFilterEffect::BipolarIntensity, "BipolarIntensity");
}

ValueTree PolyFilterEffect::exportAsValueTree() const
{
	ValueTree v = EffectProcessor::exportAsValueTree();

	saveAttribute(PolyFilterEffect::Gain, "Gain");
	saveAttribute(PolyFilterEffect::Frequency, "Frequency");
	saveAttribute(PolyFilterEffect::Q, "Q");
	saveAttribute(PolyFilterEffect::Mode, "Mode");
    saveAttribute(PolyFilterEffect::Quality, "Quality");
	saveAttribute(PolyFilterEffect::BipolarIntensity, "BipolarIntensity");

	return v;
}

Processor * PolyFilterEffect::getChildProcessor(int processorIndex)
{
	return modChains[processorIndex].getChain();
}

const Processor * PolyFilterEffect::getChildProcessor(int processorIndex) const
{
	return modChains[processorIndex].getChain();
}

void PolyFilterEffect::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

	bipolarIntensity.reset(sampleRate / 64.0, 0.05);
	voiceFilters.setSampleRate(sampleRate);
	monoFilters.setSampleRate(sampleRate);
	
}

void PolyFilterEffect::renderNextBlock(AudioSampleBuffer &b, int startSample, int numSamples)
{
	if (!forceMono && (hasPolyMods() || !blockIsActive))
	{
		
		FilterHelpers::RenderData r(b, startSample, numSamples);
		r.voiceIndex = -1;
		r.freqModValue = modChains[FrequencyChain].getOneModulationValue(startSample);

		modChains[FrequencyChain].setDisplayValue(r.freqModValue);

		auto bp = bipolarIntensity.getNextValue();

		if (bp != 0.0)
		{
			auto bipolarFMod = modChains[BipolarFrequencyChain].getOneModulationValue(startSample);

			if (!modChains[BipolarFrequencyChain].getChain()->shouldBeProcessedAtAll())
				bipolarFMod = 0.0;

			modChains[BipolarFrequencyChain].setDisplayValue(bipolarFMod);

			r.bipolarDelta = (double)(bp * bipolarFMod);
		}

		r.gainModValue = (double)modChains[GainChain].getOneModulationValue(startSample);
		r.qModValue = (double)modChains[ResonanceChain].getOneModulationValue(startSample);

		monoFilters.setDisplayModValues(-1, (float)r.applyModValue(frequency), (float)r.gainModValue, (float)r.qModValue);
		updateDisplayCoefficients();

		return;
	}

	while (numSamples > 0)
	{
		bool calculateNew;
		int subBlockSize = monoDivider.cutBlock(numSamples, calculateNew, nullptr);

		if (subBlockSize == 0)
		{
			// don't care about alignment...
			subBlockSize = 64;
		}

		FilterHelpers::RenderData r(b, startSample, subBlockSize);
		r.voiceIndex = -1;
		r.freqModValue = modChains[FrequencyChain].getOneModulationValue(startSample);
		modChains[FrequencyChain].setDisplayValue(r.freqModValue);

		auto bp = bipolarIntensity.getNextValue();

		if (bp != 0.0)
		{
			auto bipolarFMod = modChains[BipolarFrequencyChain].getOneModulationValue(startSample);

			if (!modChains[BipolarFrequencyChain].getChain()->shouldBeProcessedAtAll())
				bipolarFMod = 0.0;

			modChains[BipolarFrequencyChain].setDisplayValue(bipolarFMod);

			r.bipolarDelta = (double)(bp * bipolarFMod);
		}

		auto gainMod = (double)modChains[GainChain].getOneModulationValue(startSample);
		r.gainModValue = (double)(Decibels::decibelsToGain(gain * (gainMod - 1.0)));
		r.qModValue = (double)modChains[ResonanceChain].getOneModulationValue(startSample);

		monoFilters.setDisplayModValues(-1, (float)r.applyModValue(frequency), (float)r.gainModValue, (float)r.qModValue);
		monoFilters.renderMono(r);

		updateDisplayCoefficients();

		startSample += subBlockSize;
	}

	polyWatchdog--;

	if (polyWatchdog <= 0)
	{
		polyWatchdog = 0;
		blockIsActive = false;
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

FilterDataObject::CoefficientData PolyFilterEffect::getApproximateCoefficients() const
{
	if (ownerSynthForCoefficients == nullptr)
	{
		ownerSynthForCoefficients = const_cast<Processor*>(ProcessorHelpers::findParentProcessor(this, true));
	}

	if (auto ownerSynth = dynamic_cast<const ModulatorSynth*>(ownerSynthForCoefficients.get()))
	{
		auto v = ownerSynth->getLastStartedVoice();

		if (polyMode && v != nullptr && ownerSynth->getNumActiveVoices() != 0)
		{
			auto index = v->getVoiceIndex();

            voiceFilters.setDisplayVoiceIndex(index);
            
			return voiceFilters.getCurrentCoefficients();
		}

        return monoFilters.getCurrentCoefficients();
	}
	else
	{
		return monoFilters.getCurrentCoefficients();
	}
}

bool PolyFilterEffect::hasPolyMods() const noexcept
{
	return polyMode;
}

void PolyFilterEffect::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	if (!hasPolyMods())
	{
		polyWatchdog = 32;
		return;
	}

	FilterHelpers::RenderData r(b, startSample, numSamples);
	r.voiceIndex = voiceIndex;
	r.freqModValue = modChains[FrequencyChain].getOneModulationValue(startSample);

	auto bp = bipolarIntensity.getNextValue();

	if (bp != 0.0f)
	{
		auto bipolarFMod = modChains[BipolarFrequencyChain].getOneModulationValue(startSample);

		if (!modChains[BipolarFrequencyChain].getChain()->shouldBeProcessedAtAll())
			bipolarFMod = 0.0;

		r.bipolarDelta = (double)(bp * bipolarFMod);
	}

	auto gainMod = (double)modChains[GainChain].getOneModulationValue(startSample);
  
	if(gainMod != 1.0f)
	    r.gainModValue = (double)(Decibels::decibelsToGain(gain * (gainMod - 1.0f)));

	r.qModValue = (double)modChains[ResonanceChain].getOneModulationValue(startSample);

    voiceFilters.setDisplayModValues(voiceIndex, (float)r.applyModValue(frequency), (float)r.gainModValue, (float)r.qModValue);
	voiceFilters.renderPoly(r);

	updateDisplayCoefficients();
}

void PolyFilterEffect::startVoice(int voiceIndex, const HiseEvent& e)
{
	VoiceEffectProcessor::startVoice(voiceIndex, e);

	voiceFilters.reset(voiceIndex);

	if (!polyMode && !blockIsActive)
	{
		
		monoFilters.reset();
		polyWatchdog = 32;
	}
	
	blockIsActive = true;
}




} // namespace hise
