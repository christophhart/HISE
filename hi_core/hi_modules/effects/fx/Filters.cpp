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

#if HISE_INCLUDE_OLD_MONO_FILTER
MonoFilterEffect::MonoFilterEffect(MainController *mc, const String &id) :
MonophonicEffectProcessor(mc, id),
changeFlag(false),
useInternalChains(true),
filterCollection(1)
{
	modChains.reserve(numInternalChains);

	modChains += {this, "Frequency Modulation"};
	modChains += {this, "Gain Modulation"};
	modChains += {this, "Bipolar Freq Modulation"};
	

	finaliseModChains();

	freqChain = modChains[InternalChains::FrequencyChain].getChain();
	gainChain = modChains[InternalChains::GainChain].getChain();
	bipolarFreqChain = modChains[InternalChains::BipolarFrequencyChain].getChain();
	

	WeakReference<Processor> t = this;

	auto f = [t](float input)
	{
		if (t != nullptr)
		{
			auto freq = t->getAttribute(MonoFilterEffect::Parameters::Frequency);
			auto v = jmap<float>(input , 20.0f, freq);
			return HiSlider::getFrequencyString(v);
		}

		return Table::getDefaultTextValue(input);
	};

	freqChain->setTableValueConverter(f);
	bipolarFreqChain->setTableValueConverter(f);


	auto fg = [t](float input)
	{
		if (t != nullptr)
		{
			auto g = t->getAttribute(MonoFilterEffect::Parameters::Gain);
			auto v = (input - 0.5f) * 2.0f * g;
			return String(v, 1) + " dB";
		}

		return Table::getDefaultTextValue(input);
	};

	gainChain->setTableValueConverter(fg);

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

	updateParameterSlots();

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
	r.voiceIndex = -1;

	r.freqModValue = modChains[FrequencyChain].getOneModulationValue(startSample);

	auto bipolarFMod = modChains[BipolarFrequencyChain].getOneModulationValue(startSample);
	r.freqModValue += (double)(bipolarIntensity * bipolarFMod);

	auto gainMod = (double)modChains[GainChain].getOneModulationValue(startSample);
	r.gainModValue = (double)(Decibels::decibelsToGain(filterCollection.getGain() * (gainMod - 1.0)));

	filterCollection.setDisplayModValues(-1, (float)r.freqModValue, (float)r.gainModValue);
	filterCollection.renderMono(r);


#if 0




	FilterHelpers::RenderData r(buffer, startSample, numSamples);

	r.freqModValue = modChains[InternalChains::FrequencyChain].getOneModulationValue(startSample);
	auto bipolarFMod = getConstantModulationValueForChain(bipolarFreqChain, 0, startSample);
	r.freqModValue += (double)bipolarIntensity * bipolarFMod;
	r.gainModValue = (double)getConstantModulationValueForChain(gainChain, 0, startSample);

    filterCollection.setDisplayModValues(-1, r.freqModValue, r.gainModValue);
    
	filterCollection.renderMono(r);
#endif
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

#endif

PolyFilterEffect::PolyFilterEffect(MainController *mc, const String &uid, int numVoices) :
	VoiceEffectProcessor(mc, uid, numVoices),
	voiceFilters(numVoices),
	monoFilters(1),
	frequency(getDefaultValue(PolyFilterEffect::Parameters::Frequency)),
	q(getDefaultValue(PolyFilterEffect::Parameters::Q)),
	gain(getDefaultValue(PolyFilterEffect::Parameters::Gain)),
	mode((FilterBank::FilterMode)(int)getDefaultValue(PolyFilterEffect::Parameters::Mode))
{
	modChains.reserve(numInternalChains);

	modChains += {this, "Frequency Modulation"};
	modChains += {this, "Gain Modulation"};
	modChains += {this, "Bipolar Freq Modulation", ModulatorChain::ModulationType::Normal, Modulation::PanMode};
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
			return HiSlider::getFrequencyString(v);
		}

		return Table::getDefaultTextValue(input);
	};

	modChains[FrequencyChain].getChain()->setTableValueConverter(f);
	modChains[BipolarFrequencyChain].getChain()->setTableValueConverter(f);


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
    
    parameterNames.add("Gain");
    parameterNames.add("Frequency");
    parameterNames.add("Q");
    parameterNames.add("Mode");
    parameterNames.add("Quality");
	parameterNames.add("BipolarIntensity");

	updateParameterSlots();

	voiceFilters.setMode((FilterBank::FilterMode)(int)getDefaultValue(PolyFilterEffect::Mode));
	monoFilters.setMode((FilterBank::FilterMode)(int)getDefaultValue(PolyFilterEffect::Mode));
}

PolyFilterEffect::~PolyFilterEffect()
{
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
	case PolyFilterEffect::BipolarIntensity: bipolarParameterValue = jlimit<float>(-1.0f, 1.0f, newValue);
										bipolarIntensity.setTargetValue(bipolarParameterValue); break;
	default:							jassertfalse; return;
	}

	changeFlag = true;
}

float PolyFilterEffect::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case PolyFilterEffect::Gain:		return 0.0f;
	case PolyFilterEffect::Frequency:	return 20000.0f;
	case PolyFilterEffect::Q:			return 1.0f;
	case PolyFilterEffect::Mode:		return (float)(int)FilterBank::FilterMode::StateVariableLP;
	case PolyFilterEffect::Quality:   return 256.0f;
	case PolyFilterEffect::BipolarIntensity: return 0.0f;
	default:		jassertfalse; return 1.0f;
	}
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

		monoFilters.setDisplayModValues(-1, (float)r.applyModValue(frequency), (float)r.gainModValue);

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
		
		

		monoFilters.setDisplayModValues(-1, (float)r.applyModValue(frequency), (float)r.gainModValue);
		monoFilters.renderMono(r);
		
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

FilterDataObject::CoefficientData PolyFilterEffect::getCurrentCoefficients() const
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

    voiceFilters.setDisplayModValues(voiceIndex, (float)r.applyModValue(frequency), (float)r.gainModValue);
	voiceFilters.renderPoly(r);
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
