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



template <class ModulatorSubType> struct ModIterator
{
	ModIterator(const ModulatorChain* modChain) :
		chain(modChain)
	{
		if(chain != nullptr)
		{
			ModulatorSubType* dummy = nullptr;
			init(dummy);
		}
	}

	ModulatorSubType* next()
	{
		return (start != ende) ? *start++ : nullptr;
	}

private:

	void init(VoiceStartModulator* v)
	{
		ignoreUnused(v);

		auto handler = static_cast<const ModulatorChain::ModulatorChainHandler*>(chain->getHandler());

		start = static_cast<ModulatorSubType**>(handler->activeVoiceStartList.begin());
		ende = static_cast<ModulatorSubType**>(handler->activeVoiceStartList.end());
	}

	void init(TimeVariantModulator* v)
	{
		ignoreUnused(v);

		auto handler = static_cast<const ModulatorChain::ModulatorChainHandler*>(chain->getHandler());

		start = static_cast<ModulatorSubType**>(handler->activeTimeVariantsList.begin());
		ende = static_cast<ModulatorSubType**>(handler->activeTimeVariantsList.end());
	}

	void init(EnvelopeModulator* m)
	{
		ignoreUnused(m);

		auto handler = static_cast<const ModulatorChain::ModulatorChainHandler*>(chain->getHandler());

		start = static_cast<ModulatorSubType**>(handler->activeEnvelopesList.begin());
		ende = static_cast<ModulatorSubType**>(handler->activeEnvelopesList.end());
	}

	void init(MonophonicEnvelope* m)
	{
		ignoreUnused(m);

		auto handler = static_cast<const ModulatorChain::ModulatorChainHandler*>(chain->getHandler());

		start = static_cast<ModulatorSubType**>(handler->activeMonophonicEnvelopesList.begin());
		ende = static_cast<ModulatorSubType**>(handler->activeMonophonicEnvelopesList.end());
	}

	void init(Modulator* a)
	{
		ignoreUnused(a);

		auto handler = static_cast<const ModulatorChain::ModulatorChainHandler*>(chain->getHandler());

		start = static_cast<ModulatorSubType**>(handler->activeAllList.begin());
		ende = static_cast<ModulatorSubType**>(handler->activeAllList.end());
	}

	const ModulatorChain* chain;

	ModulatorSubType ** start = nullptr;
	ModulatorSubType ** ende = nullptr;
};


template <typename ModulatorSubType> struct CombinedModIterator
{
	CombinedModIterator(const ModulatorChain* mc):
	  isCombined(mc->getMode() == Modulation::Mode::CombinedMode),
	  gain(mc),
	  offset(isCombined ? mc : nullptr)
	{}

	template <Modulation::Mode M> ModulatorSubType* nextWithMode()
	{
		if(!isCombined)
		{
			if constexpr (Modulation::Mode::GainMode == M)
				return gain.next();
			else
				return nullptr;
		}

		auto& iter = M == Modulation::GainMode ? gain : offset;

		while(auto n = iter.next())
		{
			if constexpr (std::is_base_of<Modulation, ModulatorSubType>())
			{
				if(n->getMode() == M)
					return n;
			}
			else
			{
				if(dynamic_cast<Modulation*>(n)->getMode() == M)
					return n;
			}
		}

		return nullptr;
	}

private:

	const bool isCombined = false;

	ModIterator<ModulatorSubType> gain;
	ModIterator<ModulatorSubType> offset;
};

ModulatorChain::ModChainWithBuffer::ConstructionData::ConstructionData(Processor* parent_, const String& id_, Type t_,
	Mode m_):
	parent(parent_),
	id(id_),
	t(t_),
	m(m_)
{}

ModulatorChain::ModChainWithBuffer::Buffer::Buffer()
{}

ModulatorChain* ModulatorChain::ModChainWithBuffer::getChain() noexcept
{ return c.get(); }

bool ModulatorChain::ModChainWithBuffer::isAudioRateModulation() const noexcept
{ return options.expandToAudioRate; }

void ModulatorChain::ModChainWithBuffer::setScratchBufferFunction(
	const std::function<void(int, Modulator* m, float*, int, int)>& f)
{
	scratchBufferFunction = f;
}

void ModulatorChain::ModChainWithBuffer::setConstantVoiceValueInternal(int voiceIndex, float newValue)
{
	lastConstantVoiceValue = newValue;
	currentConstantVoiceValues[voiceIndex] = newValue;
	currentConstantValue = newValue;
}


bool ModulatorChain::SpecialQueryFunctions::GainModulation::onScaleDrag(Processor* p, bool isDown, float delta)
{
	jassert(dynamic_cast<ModulatorSynth*>(p) != nullptr);
	auto modChain = dynamic_cast<ModulatorChain*>(p->getChildProcessor(ModulatorSynth::InternalChains::GainModulation));
	return modChain->onIntensityDrag(isDown, delta);
}

ModulationDisplayValue ModulatorChain::SpecialQueryFunctions::GainModulation::getDisplayValue(Processor* p, double v,
	NormalisableRange<double> nr, int ) const
{
	jassert(dynamic_cast<ModulatorSynth*>(p) != nullptr);
	auto modChain = dynamic_cast<ModulatorChain*>(p->getChildProcessor(ModulatorSynth::InternalChains::GainModulation)); 

	ModulationDisplayValue mv;

	mv.normalisedValue = nr.convertTo0to1(v);
	mv.modulationActive = modChain->shouldBeProcessedAtAll();

	if(mv.modulationActive && mv.normalisedValue > 0.0)
	{
		auto modValue = modChain->getOutputValue();
		//auto scaledValue = modValue * gainValue;
		auto scaledDb = Decibels::gainToDecibels(modValue);
		scaledDb = nr.snapToLegalValue(scaledDb);
		auto scaledRange = nr.convertTo0to1(scaledDb);
		auto realScaledValue = scaledRange * mv.normalisedValue;
		mv.scaledValue = realScaledValue;
	}

	return mv;
}

bool ModulatorChain::SpecialQueryFunctions::PitchModulation::onScaleDrag(Processor* p, bool isDown, float delta)
{
	jassert(dynamic_cast<ModulatorSynth*>(p) != nullptr);
	auto modChain = dynamic_cast<ModulatorChain*>(p->getChildProcessor(ModulatorSynth::InternalChains::PitchModulation));
	return modChain->onIntensityDrag(isDown, delta);
}

ModulationDisplayValue ModulatorChain::SpecialQueryFunctions::PitchModulation::getDisplayValue(Processor* p, double v,
	NormalisableRange<double> nr, int) const
{
	jassert(dynamic_cast<ModulatorSynth*>(p) != nullptr);
	auto modChain = dynamic_cast<ModulatorChain*>(p->getChildProcessor(ModulatorSynth::InternalChains::PitchModulation));

	ModulationDisplayValue mv;

	auto normValue = nr.convertTo0to1(v);
	auto anyPlaying = dynamic_cast<ModulatorSynth*>(p)->getNumActiveVoices() != 0;

	mv.normalisedValue = normValue;
	mv.modulationActive = modChain->shouldBeProcessedAtAll();
	mv.scaledValue = normValue;

	ModIterator<Modulator> iter(modChain);

	double minValue = normValue;
	double maxValue = normValue;

	double intensityFactor = 12.0 / nr.getRange().getLength();
				
	while(auto m = iter.next())
	{
		if(!m->isBypassed())
		{
			auto bipolar = dynamic_cast<Modulation*>(m)->isBipolar();
			auto intensity = dynamic_cast<Modulation*>(m)->getIntensity();

			intensity *= intensityFactor;

			if(bipolar)
			{
				maxValue += hmath::abs(intensity);
				minValue -= hmath::abs(intensity);
			}
			else
			{
				if(intensity > 0.0)
					maxValue += intensity;
				else
					minValue += intensity;
			}
							
		}
	}

	mv.modulationRange.setStart(jlimit(0.0, 1.0, minValue));
	mv.modulationRange.setEnd(jlimit(0.0, 1.0, maxValue));

	if(!anyPlaying)
	{
		mv.addValue = 0.0;
		mv.modulationActive = false;
	}
	else
	{
		auto displayValue = modChain->getOutputValue();
		auto displayValueSemitones = scriptnode::conversion_logic::pitch2st().getValue(displayValue);
		displayValueSemitones = nr.snapToLegalValue(displayValueSemitones);
		auto displayValueNormalised = nr.convertTo0to1(displayValueSemitones);
		mv.addValue = displayValueNormalised - normValue;
	}

	return mv;
}

ModulationDisplayValue ModulatorChain::getModulationDisplayValue(double pValue, NormalisableRange<double> nr) const
{
	auto normValue = nr.convertTo0to1(pValue);

	ModulationDisplayValue mv;

	mv.normalisedValue = normValue;
	mv.modulationActive = shouldBeProcessedAtAll();

	if(mv.modulationActive)
	{
		switch(getMode())
		{
		case Modulation::GainMode:
		case Modulation::GlobalMode:
		{
			mv.scaledValue = getOutputValue();

			ModIterator<Modulator> iter(this);

			auto maxValue = normValue;
			auto minValue = normValue;

			while(auto m = iter.next())
			{
				auto iv = (double)dynamic_cast<Modulation*>(m)->getIntensity();
				minValue = jmin(minValue, (1.0 - iv) * maxValue);
			}

			mv.modulationRange = { minValue, maxValue };
			break;
		}
		case Modulation::PitchMode:
		{
			return SpecialQueryFunctions::PitchModulation().getDisplayValue(const_cast<Processor*>(getParentProcessor()), pValue, nr, -1);
		}
		case Modulation::PanMode:
		case Modulation::OffsetMode:
		{
			mv.scaledValue = getOutputValue();

			ModIterator<Modulator> iter(this);

			auto maxValue = normValue;
			auto minValue = normValue;

			while(auto m = iter.next())
			{
				auto iv = (double)dynamic_cast<Modulation*>(m)->getIntensity();

				maxValue += iv;

				if(dynamic_cast<Modulation*>(m)->isBipolar())
					minValue -= iv;

				if(minValue > maxValue)
					std::swap(minValue, maxValue);
			}

			mv.modulationRange = { minValue, maxValue };
			mv.clipTo0To1();

			break;
		}
			
			
		case Modulation::Mode::CombinedMode:
		{
			auto rv = getCombinedOutputValues();
			auto x = rv.first;

			mv.scaledValue = jlimit(0.0, 1.0, (double)x.first);
			mv.addValue = jlimit(0.0, 1.0, mv.scaledValue + x.second) - mv.scaledValue;

			double l = mv.scaledValue;
			double u = normValue;

			u += mv.addValue;

			if(isBipolar())
				l -= mv.addValue;

			mv.modulationRange = rv.second;
			break;
		}
		default:
			break;
		}
	}

	return mv;
}

const Chain::Handler* ModulatorChain::getHandler() const
{return &handler;}

FactoryType* ModulatorChain::getFactoryType() const
{return modulatorFactory;}

Colour ModulatorChain::getColour() const
{ 
	if(Modulator::getColour() != Colours::transparentBlack) return Modulator::getColour();
	else
	{
		auto m = getMode();

		if (m == GainMode || m == OffsetMode)
			return JUCE_LIVE_CONSTANT_OFF(Colour(0xffbe952c));
		else
			return JUCE_LIVE_CONSTANT_OFF(Colour(0xff7559a4));
	}
}

void ModulatorChain::setFactoryType(FactoryType* newFactoryType)
{modulatorFactory = newFactoryType;}

int ModulatorChain::getNumChildProcessors() const
{ return handler.getNumProcessors();	}

Processor* ModulatorChain::getChildProcessor(int processorIndex)
{ return handler.getProcessor(processorIndex);	}

Processor* ModulatorChain::getParentProcessor()
{ return parentProcessor; }

const Processor* ModulatorChain::getParentProcessor() const
{ return parentProcessor; }

const Processor* ModulatorChain::getChildProcessor(int processorIndex) const
{ return handler.getProcessor(processorIndex);	}

void ModulatorChain::setInternalAttribute(int i, float x)
{ jassertfalse; }

float ModulatorChain::getAttribute(int i) const
{ return -1.0; }

float ModulatorChain::getCurrentMonophonicStartValue() const noexcept
{
	return monophonicStartValue;
}

EnvelopeModulator::ModulatorState* ModulatorChain::createSubclassedState(int) const
{ jassertfalse; return nullptr; }

void ModulatorChain::calculateBlock(int, int)
{
}

Table::ValueTextConverter ModulatorChain::getTableValueConverter() const
{
	return handler.tableValueConverter;
}

void ModulatorChain::setPostEventFunction(const std::function<void(Modulator*, const HiseEvent&)>& pf)
{
	postEventFunction = pf;
}

void ModulatorChain::applyMonoOnOutputValue(float monoValue)
{
	ignoreUnused(monoValue);

#if ENABLE_ALL_PEAK_METERS
	setOutputValue(getOutputValue() * monoValue);
#endif
}

void ModulatorChain::updateInitialValueFromChildMods()
{
	auto iv = getInitialValue();

	for(auto m: allModulators)
	{
		if(m->isBypassed())
		{
			auto mod = dynamic_cast<Modulation*>(m);
			auto inactiveValue = m->getInactiveModValue();
			m->setOutputValue(inactiveValue);
			mod->applyModulationValue(inactiveValue, iv);
		}
	}

	setInitialValue(iv);
}

void ModulatorChain::setInitialValue(float newInitialValue)
{
	if(getInitialValue() == newInitialValue)
	{
		initialValue = { false, newInitialValue };
		return;
	}

	switch(getMode())
	{
	case GainMode:
	case GlobalMode:
	case CombinedMode:
	case OffsetMode:
		initialValue = { true, newInitialValue };
		break;
	case PanMode:
		initialValue = { true, newInitialValue };
		break;
	case PitchMode:
		initialValue = { true, newInitialValue };
		break;
	}
}

void ModulatorChain::setTableValueConverter(const Table::ValueTextConverter& converter)
{
	handler.tableValueConverter = converter;
}

ModulatorChain::ModulatorChainHandler::ModSorter::ModSorter(ModulatorChainHandler& parent_):
	parent(parent_)
{}

bool ModulatorChain::ModulatorChainHandler::ModSorter::operator()(Modulator* f, Modulator* s) const
{
	auto fi = parent.chain->allModulators.indexOf(f);
	auto si = parent.chain->allModulators.indexOf(s);
                
	return si > fi;
}

ModulatorChain::ModulatorChainHandler::~ModulatorChainHandler()
{}

Modulator* ModulatorChain::ModulatorChainHandler::getModulator(int modIndex) const
{
	jassert(modIndex < getNumModulators());
	return chain->allModulators[modIndex];
}

int ModulatorChain::ModulatorChainHandler::getNumModulators() const
{ return chain->allModulators.size(); }

Processor* ModulatorChain::ModulatorChainHandler::getProcessor(int processorIndex)
{
	return chain->allModulators[processorIndex];
}

const Processor* ModulatorChain::ModulatorChainHandler::getProcessor(int processorIndex) const
{
	return chain->allModulators[processorIndex];
}

int ModulatorChain::ModulatorChainHandler::getNumProcessors() const
{
	return getNumModulators();
}

void ModulatorChain::ModulatorChainHandler::clear()
{
	notifyListeners(Listener::Cleared, nullptr);

	activeEnvelopes = false;
	activeTimeVariants = false;
	activeVoiceStarts = false;

	chain->envelopeModulators.clear();
	chain->variantModulators.clear();
	chain->voiceStartModulators.clear();
	chain->allModulators.clear();
}

bool ModulatorChain::ModulatorChainHandler::hasActiveEnvelopes() const noexcept
{ return activeEnvelopes; }

bool ModulatorChain::ModulatorChainHandler::hasActiveTimeVariantMods() const noexcept
{ return activeTimeVariants; }

bool ModulatorChain::ModulatorChainHandler::hasActiveVoiceStartMods() const noexcept
{ return activeVoiceStarts; }

bool ModulatorChain::ModulatorChainHandler::hasActiveMonophoicEnvelopes() const noexcept
{ return activeMonophonicEnvelopes; }

bool ModulatorChain::ModulatorChainHandler::hasActiveMods() const noexcept
{ return anyActive; }

void ModulatorChain::ModChainWithBuffer::Buffer::setMaxSize(int maxSamplesPerBlock_)
{
	int requiredSize = (dsp::SIMDRegister<float>::SIMDRegisterSize + maxSamplesPerBlock_) * 3;

	if (requiredSize > allocated)
	{
		maxSamplesPerBlock = maxSamplesPerBlock_;
		data.realloc(requiredSize, sizeof(float));

		allocated = requiredSize;

		data.clear(requiredSize);
	}

	updatePointers();
}




bool ModulatorChain::ModChainWithBuffer::Buffer::isInitialised() const noexcept
{
	return allocated != 0;
}

void ModulatorChain::ModChainWithBuffer::Buffer::clear()
{
	voiceValues = nullptr;
	monoValues = nullptr;
	scratchBuffer = nullptr;
	data.free();
}

void ModulatorChain::ModChainWithBuffer::Buffer::updatePointers()
{
	voiceValues = dsp::SIMDRegister<float>::getNextSIMDAlignedPtr(data);
	monoValues = dsp::SIMDRegister<float>::getNextSIMDAlignedPtr(voiceValues + maxSamplesPerBlock);
	scratchBuffer = dsp::SIMDRegister<float>::getNextSIMDAlignedPtr(monoValues + maxSamplesPerBlock);
}

void ModulatorChain::ModChainWithBuffer::applyMonophonicValuesToVoiceInternal(float* voiceBuffer, float* monoBuffer, int numSamples)
{
	if (c->getMode() == Modulation::PanMode)
	{
		FloatVectorOperations::add(voiceBuffer, monoBuffer, numSamples);
	}
	else
	{
		FloatVectorOperations::multiply(voiceBuffer, monoBuffer, numSamples);
	}
}


void ModulatorChain::ModChainWithBuffer::setDisplayValueInternal(int voiceIndex, int startSample, int numSamples)
{
	if (c->polyManager.getLastStartedVoice() == voiceIndex)
	{
		float displayValue;

		if (currentVoiceData == nullptr)
			displayValue = getConstantModulationValue();
		else
			displayValue = currentVoiceData[startSample];

		if (c->getMode() == Modulation::PanMode)
		{
			displayValue = displayValue * 0.5f + 0.5f;
		}


		c->setOutputValue(displayValue);

		if(currentVoiceData != nullptr)
			c->pushPlotterValues(currentVoiceData, startSample, numSamples);
	}
}

ModulatorChain::ModChainWithBuffer::ModChainWithBuffer(ConstructionData data) :
	c(new ModulatorChain(data.parent->getMainController(),
		data.id,
		data.parent->getVoiceAmount(),
		data.m,
		data.parent)),
	type(data.t),
	currentMonophonicRampValue(c->getInitialValueInternal())
{
	FloatVectorOperations::fill(currentConstantVoiceValues, c->getInitialValueInternal(), NUM_POLYPHONIC_VOICES);
	FloatVectorOperations::fill(currentRampValues, c->getInitialValueInternal(), NUM_POLYPHONIC_VOICES);

	if (data.t == Type::VoiceStartOnly)
		c->setIsVoiceStartChain(true);
}



ModulatorChain::ModChainWithBuffer::~ModChainWithBuffer()
{
	c = nullptr;

	modBuffer.clear();
}

void ModulatorChain::ModChainWithBuffer::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	c->prepareToPlay(sampleRate, samplesPerBlock);

	if (type == Type::Normal)
		modBuffer.setMaxSize(samplesPerBlock);

	jassert(numActiveVoices == 0);
}

void ModulatorChain::ModChainWithBuffer::handleHiseEvent(const HiseEvent& m)
{
	if (c->shouldBeProcessedAtAll())
		c->handleHiseEvent(m);

	if(m.isAllNotesOff())
		numActiveVoices = 0;
}

void ModulatorChain::ModChainWithBuffer::resetVoice(int voiceIndex)
{
	numActiveVoices = jmax(numActiveVoices-1, 0);

	if (c->hasActiveEnvelopesAtAll())
	{
		c->reset(voiceIndex);
		currentRampValues[voiceIndex] = 0.0f;
		currentMonophonicRampValue = c->getInitialValueInternal();
	}
}

void ModulatorChain::ModChainWithBuffer::stopVoice(int voiceIndex)
{
	if (c->hasVoiceModulators())
		c->stopVoice(voiceIndex);
}

void ModulatorChain::ModChainWithBuffer::startVoice(int voiceIndex)
{
	numActiveVoices++;

	float firstDynamicValue = c->getInitialValueInternal();

	

	if (options.includeMonophonicValues && c->hasMonophonicTimeModulationMods())
	{
		Modulation::applyModulationValue(c->getMode(), firstDynamicValue, currentMonophonicRampValue);
	}

	auto m = c->getMode();

	if (c->hasVoiceModulators())
	{
		auto startValue = c->startVoice(voiceIndex);

		if(m == Modulation::CombinedMode)
		{
			// we've calculated it directly here...
			firstDynamicValue = startValue;
		}
		else
		{
			Modulation::applyModulationValue(m, firstDynamicValue, startValue);	
		}
	}
	else
	{
		c->polyManager.setLastStartedVoice(voiceIndex);
	}

	if(m != Modulation::CombinedMode)
	{
		Modulation::applyModulationValue(m, firstDynamicValue, c->getCurrentMonophonicStartValue());
	}
	else
	{
		firstDynamicValue += c->addBufferData->currentTimeVariantAddValue;
	}

	if(options.clampTo0to1)
	{
		firstDynamicValue = jlimit(0.0f, 1.0f, firstDynamicValue);
	}

	setConstantVoiceValueInternal(voiceIndex, c->getConstantVoiceValue(voiceIndex));

	currentRampValues[voiceIndex] = firstDynamicValue;


	//currentMonophonicRampValue = firstDynamicValue;
}


void ModulatorChain::ModChainWithBuffer::expandVoiceValuesToAudioRate(int voiceIndex, int startSample, int numSamples)
{
	if (currentVoiceData != nullptr)
	{
		polyExpandChecker = true;

		if (!ModBufferExpansion::expand(currentVoiceData, startSample, numSamples, currentRampValues[voiceIndex]))
		{
			// Don't use the dynamic data for further processing...

			currentConstantValue = currentRampValues[voiceIndex];

			currentVoiceData = nullptr;
		}
		else
		{
			currentConstantValue = c->getInitialValue();
		}
	}
}

void ModulatorChain::ModChainWithBuffer::expandMonophonicValuesToAudioRate(int startSample, int numSamples)
{
#if JUCE_DEBUG
	monoExpandChecker = true;
#endif

	if (auto data = getMonophonicModulationValues(startSample))
	{
		if (!ModBufferExpansion::expand(getMonophonicModulationValues(0), startSample, numSamples, currentMonophonicRampValue))
		{
			FloatVectorOperations::fill(const_cast<float*>(data + startSample), currentMonophonicRampValue, numSamples);
		}
	}
}

void ModulatorChain::ModChainWithBuffer::setCurrentRampValueForVoice(int voiceIndex, float value) noexcept
{
	if (voiceIndex >= 0 && voiceIndex < NUM_POLYPHONIC_VOICES)
		currentRampValues[voiceIndex] = value;
}

void ModulatorChain::ModChainWithBuffer::setDisplayValue(float v)
{
	c->setOutputValue(v);
}

void ModulatorChain::ModChainWithBuffer::setExpandToAudioRate(bool shouldExpandAfterRendering)
{
	options.expandToAudioRate = shouldExpandAfterRendering;
}


void ModulatorChain::ModChainWithBuffer::calculateMonophonicModulationValues(int startSample, int numSamples)
{
	if (c->isVoiceStartChain)
		return;

	if (c->hasMonophonicTimeModulationMods())
	{
		int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		int numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		jassert(type == Type::Normal);
		jassert(c->hasMonophonicTimeModulationMods());
		jassert(c->getSampleRate() > 0);

		

		float iv;

		// use the standard initial value or it will be applied twice

		if(options.includeMonophonicValues && !c->hasActivePolyMods())
			iv = c->getInitialValueInternal();
		else
			iv = c->getInitialValue();

		smoothConstantInitialValue(modBuffer.monoValues + startSample_cr, numSamples_cr, iv, lastInitialMonoValue);

		lastInitialMonoValue = iv;

		if(c->getMode() == Modulation::Mode::CombinedMode)
		{
			CombinedModIterator<TimeVariantModulator> iter(c);

			while (auto mod = iter.nextWithMode<Modulation::Mode::GainMode>())
			{
				mod->render(modBuffer.monoValues, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
			}

			auto ptr = c->addBufferData->getWritePointer(0, false);
			FloatVectorOperations::clear(ptr + startSample_cr, numSamples_cr);

			while (auto mod = iter.nextWithMode<Modulation::Mode::OffsetMode>())
				mod->render(ptr, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);

			CombinedModIterator<MonophonicEnvelope> iter2(c);

			while (auto mod = iter2.nextWithMode<Modulation::Mode::GainMode>())
			{
				mod->render(0, modBuffer.monoValues, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
			}

			while (auto mod = iter2.nextWithMode<Modulation::Mode::OffsetMode>())
			{
				mod->render(0, ptr, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
			}

			currentMonoValue = modBuffer.monoValues[startSample_cr];

			auto v = ptr[startSample_cr];

			c->addBufferData->currentTimeVariantAddValue = v;
			currentMonoValue += v;
		}
		else
		{
			ModIterator<TimeVariantModulator> iter(c);

			while (auto mod = iter.next())
			{
				mod->render(modBuffer.monoValues, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
			}

			ModIterator<MonophonicEnvelope> iter2(c);

			while (auto mod = iter2.next())
			{
				mod->render(0, modBuffer.monoValues, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
			}

			while (auto mod = iter2.next())
			{
				mod->render(0, modBuffer.monoValues, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
			}

			currentMonoValue = modBuffer.monoValues[startSample_cr];
		}

		

		monoExpandChecker = false;
	}
	else if(options.forceProcessing && options.includeMonophonicValues)
	{
		int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		int numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		if(!c->hasActivePolyMods())
		{
			auto iv = c->getInitialValueInternal();
			smoothConstantInitialValue(modBuffer.monoValues + startSample_cr, numSamples_cr, iv, lastInitialMonoValue);

			lastInitialMonoValue = iv;
		}
		else
		{
			jassert(c->getMode() == Mode::CombinedMode ||
					c->getMode() == Mode::GainMode ||
					c->getMode() == Mode::PitchMode);
			FloatVectorOperations::fill(modBuffer.monoValues + startSample_cr, 1.0f, numSamples_cr);
		}
	}

	if(c->runtimeTargetSource.isEnabled() && !c->polyManager.isAnyVoicePlaying())
	{
		int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		int numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		auto useSignal = c->hasActiveTimeVariantMods() || c->hasActiveMonoEnvelopes();
		auto staticSignal = lastInitialMonoValue;

		auto tf = c->getMainController()->getBufferSizeForCurrentBlock() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		c->runtimeTargetSource.updateThisBlockSize(tf);
		c->runtimeTargetSource.copyModulationValues(useSignal ? modBuffer.monoValues : nullptr, staticSignal, startSample_cr, numSamples_cr);
	}
}



void ModulatorChain::ModChainWithBuffer::calculateModulationValuesForCurrentVoice(int voiceIndex, int startSample, int numSamples)
{
	if (c->isVoiceStartChain)
	{
		return;
	}

	jassert(voiceIndex >= 0);
	jassert(modBuffer.isInitialised());

	c->polyManager.setCurrentVoice(voiceIndex);

	const bool useMonophonicData = options.includeMonophonicValues && (c->hasMonophonicTimeModulationMods() || options.forceProcessing);

	auto voiceData = modBuffer.voiceValues;
	const auto monoData = modBuffer.monoValues;

	jassert(startSample % HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR == 0);

	int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
 	int numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	if (c->hasActivePolyMods())
	{
		float thisConstantValue = c->getConstantVoiceValue(voiceIndex);

		if(options.clampTo0to1)
			thisConstantValue = jlimit(0.0f, 1.0f, thisConstantValue);

		const float previousConstantValue = currentConstantVoiceValues[voiceIndex];

		smoothConstantInitialValue(voiceData + startSample_cr, numSamples_cr, thisConstantValue, previousConstantValue);


		setConstantVoiceValueInternal(voiceIndex, thisConstantValue);

		if (c->hasActivePolyEnvelopes())
		{
			if(c->getMode() == Modulation::Mode::CombinedMode)
			{
				CombinedModIterator<EnvelopeModulator> iter(c);

				while (auto mod = iter.nextWithMode<Modulation::Mode::GainMode>())
				{
					mod->render(voiceIndex, voiceData, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
					jassert(!scratchBufferFunction);
				}

				if (useMonophonicData)
				{
					applyMonophonicValuesToVoiceInternal(voiceData + startSample_cr, monoData + startSample_cr, numSamples_cr);
				}

				auto ptr = c->addBufferData->getWritePointer(0, true);

				FloatVectorOperations::fill(ptr + startSample_cr, c->addBufferData->addStartValue, numSamples_cr);

				while (auto mod = iter.nextWithMode<Modulation::Mode::OffsetMode>())
				{
					mod->render(voiceIndex, ptr, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
				}

				if (useMonophonicData)
				{
					auto mv = c->addBufferData->getReadPointer(0, false);
					FloatVectorOperations::add(ptr + startSample_cr, mv + startSample_cr, numSamples_cr);
					
				}

				FloatVectorOperations::add(voiceData + startSample_cr, ptr + startSample_cr, numSamples_cr);
			}
			else
			{
				ModIterator<EnvelopeModulator> iter(c);

				while (auto mod = iter.next())
				{
					mod->render(voiceIndex, voiceData, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);

					if (scratchBufferFunction)
						scratchBufferFunction(voiceIndex, mod, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
				}

				if (useMonophonicData)
				{
					applyMonophonicValuesToVoiceInternal(voiceData + startSample_cr, monoData + startSample_cr, numSamples_cr);
				}
			}

			if(c->checkRelease)
			{
				auto active = c->isPlaying(voiceIndex);

				if(!active)
				{
					auto eventId = c->eventIdToVoiceIndexMap[voiceIndex];

					if (isPositiveAndBelow(eventId, NUM_POLYPHONIC_VOICES))
					{
						c->eventIdToVoiceIndexMap[voiceIndex] = -1;
						c->runtimeTargetSource.resetFlag[eventId] = scriptnode::modulation::ClearState::Reset;
					}
				}
			}

			currentVoiceData = voiceData;

			DEBUG_ONLY(polyExpandChecker = false);
		}
		else if (useMonophonicData)
		{
 			applyMonophonicValuesToVoiceInternal(voiceData + startSample_cr, monoData + startSample_cr, numSamples_cr);

			if(c->getMode() == Modulation::Mode::CombinedMode)
			{
				auto ptr = c->addBufferData->getReadPointer(startSample_cr, false);

				FloatVectorOperations::add(voiceData + startSample_cr, ptr, numSamples_cr);
				FloatVectorOperations::add(voiceData + startSample_cr, c->addBufferData->addStartValue, numSamples_cr);
			}

			currentVoiceData = voiceData;
			DEBUG_ONLY(polyExpandChecker = false);
		}
		else
		{
			// Set it to nullptr, and let the module use the constant value instead...
			currentVoiceData = nullptr;

			if(c->getMode() == Modulation::Mode::CombinedMode)
			{
				auto addV = c->addBufferData->addStartValue;
				thisConstantValue += addV;

				if(options.clampTo0to1)
					thisConstantValue = jlimit(0.0f, 1.0f, thisConstantValue);

				setConstantVoiceValueInternal(voiceIndex, thisConstantValue);
			}
		}
	}
	else if (useMonophonicData)
	{
		setConstantVoiceValueInternal(voiceIndex, c->getInitialValue());

		if (options.voiceValuesReadOnly)
		{
			jassert(c->getMode() != Modulation::Mode::CombinedMode);
			currentVoiceData = monoData;
		}
		else
		{
			FloatVectorOperations::copy(voiceData + startSample_cr, monoData + startSample_cr, numSamples_cr);

			

			if(c->getMode() == Modulation::Mode::CombinedMode)
			{
				auto src = c->addBufferData->getReadPointer(startSample_cr, false);
				FloatVectorOperations::add(voiceData + startSample_cr, src, numSamples_cr);
			}

			currentVoiceData = voiceData;
		}

		DEBUG_ONLY(polyExpandChecker = false);
	}
	else
	{
		currentVoiceData = nullptr;
		setConstantVoiceValueInternal(voiceIndex, c->getInitialValueInternal());
	}

	auto ptrToClip = options.clampTo0to1 ? const_cast<float*>(currentVoiceData) : nullptr;

	if(ptrToClip != nullptr)
	{
		FloatVectorOperations::clip(ptrToClip + startSample_cr, ptrToClip + startSample_cr, 0.0f, 1.0f, numSamples_cr);
	}


	if(c->runtimeTargetSource.isEnabled())
	{
		c->runtimeTargetSource.copyModulationValues(currentVoiceData, getConstantModulationValue(), startSample_cr, numSamples_cr);
	}

	setDisplayValueInternal(voiceIndex, startSample_cr, numSamples_cr);

	c->polyManager.clearCurrentVoice();
}

void ModulatorChain::ModChainWithBuffer::applyMonophonicModulationValues(AudioSampleBuffer& b, int startSample, int numSamples)
{
	if (c->hasMonophonicTimeModulationMods())
	{
		for (int i = 0; i < b.getNumSamples(); i++)
		{
			FloatVectorOperations::multiply(b.getWritePointer(i, startSample), modBuffer.monoValues, numSamples);
		}
	}
}

const float* ModulatorChain::ModChainWithBuffer::getReadPointerForVoiceValues(int startSample) const
{
	// You need to expand the modulation values to audio rate before calling this method.
	// Either call setExpandAudioRate(true) in the constructor, or manually expand them
	jassert(currentVoiceData == nullptr || polyExpandChecker);

	return currentVoiceData != nullptr ? currentVoiceData + startSample : nullptr;
}

float* ModulatorChain::ModChainWithBuffer::getWritePointerForVoiceValues(int startSample)
{
	jassert(!options.voiceValuesReadOnly);

	// You need to expand the modulation values to audio rate before calling this method.
	// Either call setExpandAudioRate(true) in the constructor, or manually expand them
	jassert(currentVoiceData == nullptr || polyExpandChecker);

	return currentVoiceData != nullptr ? const_cast<float*>(currentVoiceData) + startSample : nullptr;
}

float* ModulatorChain::ModChainWithBuffer::getWritePointerForManualExpansion(int startSample)
{
	// You have already expanded the values...
	//jassert(currentVoiceData != nullptr || !polyExpandChecker);

	// Have you already downsampled the startOffsetValue? If not, this is really bad...
	jassert(startSample % HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR == 0);

	int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	manualExpansionPending = true;

	return currentVoiceData != nullptr ? const_cast<float*>(currentVoiceData) + startSample_cr : nullptr;
}

const float* ModulatorChain::ModChainWithBuffer::getMonophonicModulationValues(int startSample) const
{
	// If you include the monophonic modulation values in the voice modulation, there's no need for this method
	//jassert(!options.includeMonophonicValues);

	if (c->hasMonophonicTimeModulationMods())
	{
		return modBuffer.monoValues + startSample;
	}

	return nullptr;
}

float ModulatorChain::ModChainWithBuffer::getConstantModulationValue() const
{
	return currentConstantValue;
}

float ModulatorChain::ModChainWithBuffer::getModValueForVoiceWithOffset(int startSample) const
{
	return currentVoiceData != nullptr ? currentVoiceData[startSample] : currentConstantValue;
}

float ModulatorChain::ModChainWithBuffer::getOneModulationValue(int startSample) const
{
	if(numActiveVoices == 0)
	{
		ModIterator<Modulator> iter(c);

		float m = 1.0f;

		while(auto mod = iter.next())
		{
			auto mv = mod->getInactiveModValue();
			m *= mv;
		}

		return m;
	}
		

	// If you set this, you probably don't need this method...
	jassert(!options.expandToAudioRate);

	if (currentVoiceData == nullptr)
		return getConstantModulationValue();

	const int downsampledOffset = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	return currentVoiceData[downsampledOffset];
}

float* ModulatorChain::ModChainWithBuffer::getScratchBuffer()
{
	return modBuffer.scratchBuffer;
}

void ModulatorChain::ModChainWithBuffer::setAllowModificationOfVoiceValues(bool mightBeOverwritten)
{
	options.voiceValuesReadOnly = !mightBeOverwritten;
}

void ModulatorChain::ModChainWithBuffer::setIncludeMonophonicValuesInVoiceRendering(bool shouldInclude)
{
	options.includeMonophonicValues = shouldInclude;
}


void ModulatorChain::ModChainWithBuffer::clear()
{
	currentVoiceData = nullptr;
	currentConstantValue = c->getInitialValueInternal();
}

ModulatorChain::ModulatorChain(MainController *mc, const String &uid, int numVoices, Mode m, Processor *p): 
	EnvelopeModulator(mc, uid, numVoices, m),
	Modulation(m),
	handler(this),
	parentProcessor(p),
	isVoiceStartChain(false),
	runtimeTargetSource(*this)
{
	for(auto& s: eventIdToVoiceIndexMap)
		s = -1;

	activeVoices.setRange(0, numVoices, false);

	setMode(m, dontSendNotification);

	FloatVectorOperations::fill(lastVoiceValues, getInitialValueInternal(), NUM_POLYPHONIC_VOICES);

	if (Identifier::isValidIdentifier(uid))
	{
		chainIdentifier = Identifier(uid);
	}

	setEditorState(Processor::Visible, false, dontSendNotification);
};

ModulatorChain::~ModulatorChain()
{
	handler.clearAsync(this);
}

Chain::Handler *ModulatorChain::getHandler() {return &handler;};;

bool ModulatorChain::hasActivePolyMods() const noexcept
{
	return !isBypassed() && (handler.hasActiveEnvelopes() || handler.hasActiveVoiceStartMods());
}

bool ModulatorChain::hasActiveVoiceStartMods() const noexcept
{
	return !isBypassed() && handler.hasActiveVoiceStartMods();
}

bool ModulatorChain::hasActiveTimeVariantMods() const noexcept
{
	return !isBypassed() && handler.hasActiveTimeVariantMods(); 
}

bool ModulatorChain::hasActivePolyEnvelopes() const noexcept
{
	return !isBypassed() && handler.hasActiveEnvelopes();
}

bool ModulatorChain::hasActiveMonoEnvelopes() const noexcept
{
	return !isBypassed() && handler.hasActiveMonophoicEnvelopes();
}

bool ModulatorChain::hasActiveEnvelopesAtAll() const noexcept
{
	return !isBypassed() && (handler.hasActiveMonophoicEnvelopes() || handler.hasActiveEnvelopes());
}

bool ModulatorChain::hasOnlyVoiceStartMods() const noexcept
{
	return !isBypassed() && !(handler.hasActiveEnvelopes() || handler.hasActiveTimeVariantMods() || handler.hasActiveMonophoicEnvelopes()) && handler.hasActiveVoiceStartMods();
}

bool ModulatorChain::hasTimeModulationMods() const noexcept
{
	return !isBypassed() && (handler.hasActiveTimeVariantMods() || handler.hasActiveEnvelopes() || handler.hasActiveMonophoicEnvelopes());
}

bool ModulatorChain::hasMonophonicTimeModulationMods() const noexcept
{
	return !isBypassed() && (handler.hasActiveTimeVariantMods() || handler.hasActiveMonophoicEnvelopes());
}

bool ModulatorChain::hasVoiceModulators() const noexcept
{
	return !isBypassed() &&  (handler.hasActiveVoiceStartMods() || handler.hasActiveEnvelopes() || handler.hasActiveMonophoicEnvelopes());
}

bool ModulatorChain::shouldBeProcessedAtAll() const noexcept
{
	return !isBypassed() && handler.hasActiveMods();
}

void ModulatorChain::syncAfterDelayStart(bool waitForDelay, int voiceIndex)
{
	// We just want the envelopes to recalculate their value,
	// constant modulators are constant (noice) and
	// time variant modulators will have a continuous signal that can't be suspended
	if(hasActiveEnvelopesAtAll())
	{
		ModIterator<EnvelopeModulator> iter2(this);

		while (auto mod = iter2.next())
			mod->syncAfterDelayStart(waitForDelay, voiceIndex);

		ModIterator<MonophonicEnvelope> iter3(this);

		while (auto mod = iter3.next())
			mod->syncAfterDelayStart(waitForDelay, voiceIndex);
	}
}

void ModulatorChain::reset(int voiceIndex)
{
	jassert(hasActiveEnvelopesAtAll());

	EnvelopeModulator::reset(voiceIndex);

	ModIterator<EnvelopeModulator> iter(this);
	
	while(auto mod = iter.next())
		mod->reset(voiceIndex);

	ModIterator<MonophonicEnvelope> iter2(this);

	while (auto mod = iter2.next())
		mod->reset(voiceIndex);
};

void ModulatorChain::handleHiseEvent(const HiseEvent &m)
{
	jassert(shouldBeProcessedAtAll());

	if(m.isNoteOn())
		unsavedEventId = m.getEventId() % NUM_POLYPHONIC_VOICES;

	EnvelopeModulator::handleHiseEvent(m);

	ModIterator<Modulator> iter(this);

    if(postEventFunction)
    {
        while(auto mod = iter.next())
        {
            mod->handleHiseEvent(m);
            postEventFunction(mod, m);
        }
    }
    else
    {
        while(auto mod = iter.next())
            mod->handleHiseEvent(m);
    }
    
	
};


void ModulatorChain::allNotesOff()
{
	if (hasVoiceModulators())
		VoiceModulation::allNotesOff();
}

float ModulatorChain::getConstantVoiceValue(int voiceIndex) const
{
	if (!hasActiveVoiceStartMods())
		return getInitialValueInternal();

	auto m = getMode();

	if(m == Modulation::CombinedMode)
	{
		float scaleValue = getInitialValueInternal();
		float addValue = 0.0f;

		CombinedModIterator<VoiceStartModulator> iter(this);

		while (auto mod = iter.nextWithMode<Modulation::Mode::GainMode>())
		{
			const auto modValue = mod->getVoiceStartValue(voiceIndex);
			const auto intensityModValue = mod->calcGainIntensityValue(modValue);
			scaleValue -= zeroPosition;
			scaleValue *= intensityModValue;
			scaleValue += zeroPosition;
		}

		while (auto mod = iter.nextWithMode<Modulation::Mode::OffsetMode>())
		{
			const auto modValue = mod->getVoiceStartValue(voiceIndex);
			const auto intensityModValue = mod->calcOffsetIntensityValue(modValue);
			addValue += intensityModValue;
		}

		addBufferData->addStartValue = addValue;

		return scaleValue;
	}
	if(m == Modulation::GainMode)
	{
		float value = getInitialValueInternal();

		ModIterator<VoiceStartModulator> iter(this);

		while (auto mod = iter.next())
		{
			const auto modValue = mod->getVoiceStartValue(voiceIndex);
			const auto intensityModValue = mod->calcGainIntensityValue(modValue);
			value *= intensityModValue;
		}

		return value;
	}
	if(m == Modulation::OffsetMode)
	{
		float value = getInitialValueInternal();

		ModIterator<VoiceStartModulator> iter(this);

		while (auto mod = iter.next())
		{
			const auto modValue = mod->getVoiceStartValue(voiceIndex);
			const auto intensityModValue = mod->calcOffsetIntensityValue(modValue);
			value += intensityModValue;
		}

		return value;

	}
	else
	{
		float value = getInitialValueInternal();

		if(m == PitchMode)
			value = PitchConverters::pitchFactorToNormalisedRange(value);

		ModIterator<VoiceStartModulator> iter(this);

		while (auto mod = iter.next())
		{
			float modValue = mod->getVoiceStartValue(voiceIndex);

			if (mod->isBipolar())
				modValue = 2.0f * modValue - 1.0f;

			const float intensityModValue = mod->calcPitchIntensityValue(modValue);
			value += intensityModValue;
		}

		return m == Mode::PanMode ? value : Modulation::PitchConverters::normalisedRangeToPitchFactor(value);
	}
};

float ModulatorChain::startVoice(int voiceIndex)
{
	jassert(hasVoiceModulators());

	eventIdToVoiceIndexMap[voiceIndex] = unsavedEventId;
	runtimeTargetSource.resetFlag[unsavedEventId] = scriptnode::modulation::ClearState::Playing;

	activeVoices.setBit(voiceIndex, true);

	polyManager.setLastStartedVoice(voiceIndex);

	ModIterator<VoiceStartModulator> iter(this);

	while(auto mod = iter.next())
		mod->startVoice(voiceIndex);

	const float startValue = getConstantVoiceValue(voiceIndex);
	lastVoiceValues[voiceIndex] = startValue;

	setOutputValue(startValue);

	monophonicStartValue = getInitialValue();

	auto m = getMode();

	if(m == CombinedMode)
	{
		// will not be used anyways...
		monophonicStartValue = 0.0f;

		// do not use the custom initial value here or it will be applied twice
		float scaleValue = getInitialValue();
		float addValue = 0.0f;

		CombinedModIterator<EnvelopeModulator> iter2(this);

		while (auto mod = iter2.template nextWithMode<Modulation::Mode::GainMode>())
		{
			const auto modValue = mod->startVoice(voiceIndex);
			const auto intensityModValue = mod->calcGainIntensityValue(modValue);
			scaleValue *= intensityModValue;
			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		while (auto mod = iter2.template nextWithMode<Modulation::Mode::OffsetMode>())
		{
			const auto modValue = mod->startVoice(voiceIndex);
			const auto intensityModValue = mod->calcOffsetIntensityValue(modValue);
			addValue += intensityModValue;
			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		CombinedModIterator<MonophonicEnvelope> iter3(this);

		while (auto mod = iter3.nextWithMode<Modulation::Mode::GainMode>())
		{
			// Don't use the start value for monophonic envelopes...
			const auto modValue = mod->startVoice(voiceIndex);
			const auto intensityModValue = mod->calcGainIntensityValue(modValue);
			monophonicStartValue *= intensityModValue;
			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		while (auto mod = iter3.nextWithMode<Modulation::Mode::OffsetMode>())
		{
			// Don't use the start value for monophonic envelopes...
			const auto modValue = mod->startVoice(voiceIndex);
			const auto intensityModValue = mod->calcOffsetIntensityValue(modValue);
			addValue += intensityModValue;
			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		jassert(addBufferData != nullptr);

		//lastVoiceValues[voiceIndex] += addValue;

		//addBufferData->addStartValue = addValue;

		addValue += addBufferData->addStartValue;

		lastVoiceValues[voiceIndex] += addValue;

		auto v = scaleValue + addValue;

		return v;
	}
	if (m == GainMode)
	{
		// do not use the custom initial value here or it will be applied twice
		float envelopeStartValue = getInitialValue();

		ModIterator<EnvelopeModulator> iter2(this);

		while (auto mod = iter2.next())
		{
			const auto modValue = mod->startVoice(voiceIndex);
			const auto intensityModValue = mod->calcGainIntensityValue(modValue);
			envelopeStartValue *= intensityModValue;
			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		

		ModIterator<MonophonicEnvelope> iter3(this);

		while (auto mod = iter3.next())
		{
			// Don't use the start value for monophonic envelopes...
			const auto modValue = mod->startVoice(voiceIndex);
			const auto intensityModValue = mod->calcGainIntensityValue(modValue);
			monophonicStartValue *= intensityModValue;
			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		return envelopeStartValue;
	}
	else // Pitch / Pan Mode
	{
		float envelopeStartValue = getInitialValueInternal();

		if(m == PitchMode)
			envelopeStartValue = PitchConverters::pitchFactorToNormalisedRange(envelopeStartValue);

		ModIterator<EnvelopeModulator> iter2(this);

		while (auto mod = iter2.next())
		{
			auto modValue = mod->startVoice(voiceIndex);

			if (mod->isBipolar())
				modValue = 2.0f * modValue - 1.0f;

			const float intensityModValue = mod->calcPitchIntensityValue(modValue);
			envelopeStartValue += intensityModValue;

			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		ModIterator<MonophonicEnvelope> iter3(this);

		while (auto mod = iter3.next())
		{
			auto modValue = mod->startVoice(voiceIndex);

			if (mod->isBipolar())
				modValue = 2.0f * modValue - 1.0f;

			const float intensityModValue = mod->calcPitchIntensityValue(modValue);
			monophonicStartValue += intensityModValue;

			mod->polyManager.setLastStartedVoice(voiceIndex);
		}

		return m == PanMode ? envelopeStartValue : Modulation::PitchConverters::normalisedRangeToPitchFactor(envelopeStartValue);
	}
}


bool ModulatorChain::isPlaying(int voiceIndex) const
{
	jassert(hasActivePolyEnvelopes());
	jassert(getMode() == GainMode || getMode() == Modulation::GlobalMode || getMode() == Modulation::OffsetMode);

	if (isBypassed())
		return false;

	if (!hasActivePolyEnvelopes())
		return activeVoices[voiceIndex];

	ModIterator<EnvelopeModulator> iter(this);

	while (auto mod = iter.next())
		if (!mod->isPlaying(voiceIndex))
			return false;

	return true;
};

ProcessorEditorBody *ModulatorChain::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
};


void ModulatorChain::stopVoice(int voiceIndex)
{
	jassert(hasVoiceModulators());

	activeVoices.setBit(voiceIndex, false);

	ModIterator<EnvelopeModulator> iter(this);

	while(auto mod = iter.next())
		mod->stopVoice(voiceIndex);

	ModIterator<MonophonicEnvelope> iter2(this);

	while (auto mod = iter2.next())
		mod->stopVoice(voiceIndex);
};;

void ModulatorChain::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);
	blockSize = samplesPerBlock;

	if(getMode() == CombinedMode)
	{
		if(addBufferData == nullptr)
			addBufferData = new AddBufferData();

		addBufferData->prepare(sampleRate, samplesPerBlock);
	}
	
	for(int i = 0; i < envelopeModulators.size(); i++) envelopeModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);
	for(int i = 0; i < variantModulators.size(); i++) variantModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);

	jassert(checkModulatorStructure());
};

void ModulatorChain::setMode(Mode newMode, NotificationType n)
{
	// Use Gain mode as default mode for the combined mode.
	auto fm = newMode == Modulation::Mode::CombinedMode ? Modulation::Mode::GainMode : newMode;

    setFactoryType(new ModulatorChainFactoryType(polyManager.getVoiceAmount(), fm, parentProcessor));
    
    if(getMode() != newMode)
    {
        Modulation::setMode(newMode, n);

		if(newMode == Modulation::Mode::CombinedMode)
		{
			// make sure that it creates the addBufferData object
			if(getSampleRate() > 0.0)
				prepareToPlay(getSampleRate(), getLargestBlockSize());
		}
		else
		{
			// set all child modulators to the mode
			for(auto m: allModulators)
				dynamic_cast<Modulation*>(m)->setMode(newMode, n);
		}
    }
}

void ModulatorChain::setIsVoiceStartChain(bool isVoiceStartChain_)
{
	isVoiceStartChain = isVoiceStartChain_;

	if(isVoiceStartChain)
	{
		modulatorFactory = new VoiceStartModulatorFactoryType(polyManager.getVoiceAmount(), modulationMode, parentProcessor);
		
		// This sets the initial value to 1.0f for HiSlider::getDisplayValue();
		setOutputValue(getInitialValueInternal());
	}
	else
	{
		modulatorFactory = new ModulatorChainFactoryType(polyManager.getVoiceAmount(), modulationMode, parentProcessor);
	}
}

ModulatorChain::ModulatorChainHandler::ModulatorChainHandler(ModulatorChain *handledChain) :
	BypassListener(handledChain->getMainController()->getRootDispatcher()),
	chain(handledChain),
	tableValueConverter(Table::getDefaultTextValue)
{}

void ModulatorChain::ModulatorChainHandler::bypassStateChanged(Processor* p, bool bypassState)
{
	jassert(dynamic_cast<Modulator*>(p) != nullptr);

	auto mod = dynamic_cast<Modulator*>(p);

	if (!bypassState)
	{
		activeAllList.insert(mod);

		if (auto env = dynamic_cast<EnvelopeModulator*>(mod))
		{
			chain->getMainController()->allNotesOff();

			if (env->isInMonophonicMode())
			{
				activeMonophonicEnvelopesList.insert(static_cast<MonophonicEnvelope*>(env));
				activeEnvelopesList.remove(env);
			}
			else
			{
				activeMonophonicEnvelopesList.remove(static_cast<MonophonicEnvelope*>(env));
				activeEnvelopesList.insert(env);
			}
		}
		else if (auto tv = dynamic_cast<TimeVariantModulator*>(mod))
		{
			activeTimeVariantsList.insert(tv);
		}
		else if (auto vs = dynamic_cast<VoiceStartModulator*>(mod))
		{
			activeVoiceStartList.insert(vs);
		}
	}
	else
	{
		activeAllList.remove(mod);

		if (auto env = dynamic_cast<EnvelopeModulator*>(mod))
		{
			chain->getMainController()->allNotesOff();
			activeEnvelopesList.remove(env);
			activeMonophonicEnvelopesList.remove(static_cast<MonophonicEnvelope*>(env));
		}
		else if (auto tv = dynamic_cast<TimeVariantModulator*>(mod))
		{
			activeTimeVariantsList.remove(tv);
		}
		else if (auto vs = dynamic_cast<VoiceStartModulator*>(mod))
		{
			activeVoiceStartList.remove(vs);
		}
	}

	checkActiveState();

	notifyListeners(Chain::Handler::Listener::EventType::ProcessorOrderChanged, p);
	notifyPostEventListeners(Chain::Handler::Listener::EventType::ProcessorOrderChanged, p);
}

void ModulatorChain::ModulatorChainHandler::addModulator(Modulator *newModulator, Processor *siblingToInsertBefore)
{
	newModulator->setColour(chain->getColour());

	for(int i = 0; i < newModulator->getNumInternalChains(); i++)
	{
		dynamic_cast<Modulator*>(newModulator->getChildProcessor(i))->setColour(chain->getColour());
	}

	newModulator->setConstrainerForAllInternalChains(chain->getFactoryType()->getConstrainer());

	newModulator->addBypassListener(this, dispatch::sendNotificationSync);

	if (chain->isInitialized())
		newModulator->prepareToPlay(chain->getSampleRate(), chain->blockSize);
	
	const int index = siblingToInsertBefore == nullptr ? -1 : chain->allModulators.indexOf(dynamic_cast<Modulator*>(siblingToInsertBefore));

	newModulator->setParentProcessor(chain);

	dynamic_cast<Modulation*>(newModulator)->setZeroPosition(chain->zeroPosition);

	{
		LOCK_PROCESSING_CHAIN(chain);

		newModulator->setIsOnAir(chain->isOnAir());

		if (dynamic_cast<VoiceStartModulator*>(newModulator) != nullptr)
		{
			VoiceStartModulator *m = static_cast<VoiceStartModulator*>(newModulator);
			chain->voiceStartModulators.add(m);

			activeVoiceStartList.insert(m);
		}
		else if (dynamic_cast<EnvelopeModulator*>(newModulator) != nullptr)
		{
			EnvelopeModulator *m = static_cast<EnvelopeModulator*>(newModulator);
			chain->envelopeModulators.add(m);

			if (m->isInMonophonicMode())
				activeMonophonicEnvelopesList.insert(static_cast<MonophonicEnvelope*>(m));
			else
				activeEnvelopesList.insert(m);
		}
		else if (dynamic_cast<TimeVariantModulator*>(newModulator) != nullptr)
		{
			TimeVariantModulator *m = static_cast<TimeVariantModulator*>(newModulator);
			chain->variantModulators.add(m);

			activeTimeVariantsList.insert(m);
		}
		else jassertfalse;

		activeAllList.insert(newModulator);
		chain->allModulators.insert(index, newModulator);
		jassert(chain->checkModulatorStructure());

		checkActiveState();
	}

	if (JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(newModulator))
	{
		sp->compileScript();
	}


	if (auto ltp = dynamic_cast<LookupTableProcessor*>(newModulator))
	{
		WeakReference<Modulator> mod = newModulator;

		auto& cf = tableValueConverter;

		auto isBipolarModulation = chain->getMode() == Modulation::PitchMode || chain->getMode() == Modulation::PanMode;

		auto f = [mod, cf, isBipolarModulation](float input)
		{
			if (mod.get() != nullptr)
			{
				auto modulation = dynamic_cast<Modulation*>(mod.get());
				auto intensity = modulation->getIntensity();

				if (isBipolarModulation)
				{
					float normalizedInput;

					if (modulation->isBipolar())
						normalizedInput = (input-0.5f) * intensity * 2.0f;
					else
						normalizedInput = (input) * intensity;

					return cf(normalizedInput);
				}
				else
				{
					auto v = jmap<float>(input, 1.0f - intensity, 1.0f);
					return cf(v);
				}
			}

			return Table::getDefaultTextValue(input);
		};

		ltp->addYValueConverter(f, newModulator);
	}

	chain->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Children);
};


void ModulatorChain::ModulatorChainHandler::add(Processor *newProcessor, Processor *siblingToInsertBefore)
{
	//ScopedLock sl(chain->getMainController()->getLock());

	jassert(dynamic_cast<Modulator*>(newProcessor) != nullptr);

	dynamic_cast<AudioProcessor*>(chain->getMainController())->suspendProcessing(true);

	addModulator(dynamic_cast<Modulator*>(newProcessor), siblingToInsertBefore);

	dynamic_cast<AudioProcessor*>(chain->getMainController())->suspendProcessing(false);

	notifyListeners(Listener::ProcessorAdded, newProcessor);
	notifyPostEventListeners(Listener::ProcessorAdded, newProcessor);
}

void ModulatorChain::ModulatorChainHandler::deleteModulator(Modulator *modulatorToBeDeleted, bool deleteMod)
{
	notifyListeners(Listener::ProcessorDeleted, modulatorToBeDeleted);

	ScopedPointer<Modulator> mod = modulatorToBeDeleted;

	{
		LOCK_PROCESSING_CHAIN(chain);

		modulatorToBeDeleted->setIsOnAir(false);
		modulatorToBeDeleted->removeBypassListener(this);

		activeAllList.remove(modulatorToBeDeleted);

		if (auto env = dynamic_cast<EnvelopeModulator*>(modulatorToBeDeleted))
		{
			activeEnvelopesList.remove(env);
			activeMonophonicEnvelopesList.remove(static_cast<MonophonicEnvelope*>(env));
		}
		else if (auto vs = dynamic_cast<VoiceStartModulator*>(modulatorToBeDeleted))
			activeVoiceStartList.remove(vs);
		else if (auto tv = dynamic_cast<TimeVariantModulator*>(modulatorToBeDeleted))
			activeTimeVariantsList.remove(tv);

		for (int i = 0; i < getNumModulators(); ++i)
		{
			if (chain->allModulators[i] == modulatorToBeDeleted) chain->allModulators.remove(i);
		};

		for (int i = 0; i < chain->variantModulators.size(); ++i)
		{
			if (chain->variantModulators[i] == modulatorToBeDeleted) chain->variantModulators.remove(i, false);
		};

		for (int i = 0; i < chain->envelopeModulators.size(); ++i)
		{
			if (chain->envelopeModulators[i] == modulatorToBeDeleted) chain->envelopeModulators.remove(i, false);
		};

		for (int i = 0; i < chain->voiceStartModulators.size(); ++i)
		{
			if (chain->voiceStartModulators[i] == modulatorToBeDeleted) chain->voiceStartModulators.remove(i, false);
		};

		jassert(chain->checkModulatorStructure());

		checkActiveState();
	}

	if (deleteMod)
	{
		mod = nullptr;
	}
	else
		mod.release();
};


void ModulatorChain::ModulatorChainHandler::remove(Processor *processorToBeRemoved, bool deleteMod)
{
	notifyListeners(Listener::ProcessorDeleted, processorToBeRemoved);

	jassert(dynamic_cast<Modulator*>(processorToBeRemoved) != nullptr);
	deleteModulator(dynamic_cast<Modulator*>(processorToBeRemoved), deleteMod);

	notifyPostEventListeners(Listener::ProcessorDeleted, nullptr);
}

void ModulatorChain::ModulatorChainHandler::checkActiveState()
{
	activeEnvelopes = !activeEnvelopesList.isEmpty();
	activeTimeVariants = !activeTimeVariantsList.isEmpty();
	activeVoiceStarts = !activeVoiceStartList.isEmpty();
	activeMonophonicEnvelopes = !activeMonophonicEnvelopesList.isEmpty();
	anyActive = !activeAllList.isEmpty();
    
    std::sort(activeVoiceStartList.begin(), activeVoiceStartList.end(), ModSorter(*this));
    std::sort(activeTimeVariantsList.begin(), activeTimeVariantsList.end(), ModSorter(*this));
    std::sort(activeEnvelopesList.begin(), activeEnvelopesList.end(), ModSorter(*this));
    std::sort(activeAllList.begin(), activeAllList.end(), ModSorter(*this));

	if(chain->addBufferData != nullptr)
		chain->addBufferData->checkActiveState(chain);
}

int ModulatorChain::RuntimeTargetSource::getRuntimeHash() const
{
	auto p = parent.getParentProcessor();
	auto parentAsSynth = dynamic_cast<ModulatorSynth*>(p);

	for(int i = 0; i < p->getNumChildProcessors(); i++)
	{
		if(p->getChildProcessor(i) == &parent)
		{
			if(parentAsSynth != nullptr)
			{
				if(i == ModulatorSynth::InternalChains::GainModulation)
					return modulation::config::GainModulation;
				if(i == ModulatorSynth::InternalChains::PitchModulation)
					return modulation::config::PitchModulation;
			}

			return modulation::config::CustomOffset;
		}
	}

	jassertfalse;
	return -1;
}

void ModulatorChain::RuntimeTargetSource::copyModulationValues(const float* modValues, float constantValue,
	int startSample_cr, int numSamples_cr)
{
	if(isEnabled() && isPositiveAndBelow(startSample_cr + numSamples_cr, signalData.getNumSamples() + 1))
	{
		if (modValues != nullptr)
			FloatVectorOperations::copy(signalData.getWritePointer(0, startSample_cr), modValues + startSample_cr, numSamples_cr);
		else
			FloatVectorOperations::fill(signalData.getWritePointer(0, startSample_cr), constantValue, numSamples_cr);
	}
}

int ModulatorChain::ExtraModulatorRuntimeTargetSource::getRuntimeHash() const
{ return scriptnode::modulation::config::CustomOffset; }

runtime_target::RuntimeTarget ModulatorChain::ExtraModulatorRuntimeTargetSource::getType() const
{ return runtime_target::RuntimeTarget::ExternalModulatorChain; }

runtime_target::connection ModulatorChain::ExtraModulatorRuntimeTargetSource::createConnection() const
{
	auto c = source_base::createConnection();
	c.connectFunction = connectStatic<true>;
	c.disconnectFunction = connectStatic<false>;
	return c;
}

void ModulatorChain::ExtraModulatorRuntimeTargetSource::init(Collection& modChains, int offset)
{
	extraOffset = offset;

	for(int i = offset; i < modChains.size(); i++)
	{
		auto& mb = modChains[i];
		mb.setAllowModificationOfVoiceValues(true);
		mb.setExpandToAudioRate(false);
		mb.setIncludeMonophonicValuesInVoiceRendering(true);
		mb.setClampTo0To1(true);
		mb.setForceProcessing(true);

		extraMods.add(&mb);
	}
}

void ModulatorChain::ExtraModulatorRuntimeTargetSource::connectToRuntimeTarget(scriptnode::OpaqueNode& on,
	bool shouldAdd)
{
	auto c = createConnection();
	on.connectToRuntimeTarget(shouldAdd, c);
}

ModulationDisplayValue::QueryFunction::Ptr ModulatorChain::ExtraModulatorRuntimeTargetSource::
getModulationQueryFunction(const ParameterProperties& pp, int parameterIndex) const
{
	auto mi = pp.getModulationChainIndex(parameterIndex);

	if(isPositiveAndBelow(mi, extraMods.size()))
	{
		auto mc = extraMods[mi]->getChain();
		return new ModulatorChain::SelfQueryFunction(mc);
	}

	return nullptr;
}




bool ModulatorChain::ExtraModulatorRuntimeTargetSource::addConnection(bool shouldBeAdded, TargetType* target)
{
	if(extraMods.isEmpty())
		return false;

	scriptnode::modulation::SignalSource signal;

	auto& first = extraMods.getFirst()->getChain()->runtimeTargetSource;

	if(shouldBeAdded)
	{
		signal.sampleRate_cr = first.parent.getSampleRate() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		signal.numSamples_cr = first.parent.getLargestBlockSize() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		for(int i = 0; i < extraMods.size(); i++)
		{
			auto rt = &extraMods[i]->getChain()->runtimeTargetSource;
			signal.modValueFunctions[i] = { rt, RuntimeTargetSource::getEventData };
		}
	}

	for(auto mc: extraMods)
		mc->getChain()->runtimeTargetSource.handleConnection(shouldBeAdded, signal.numSamples_cr);

	target->onValue(signal);
	return true;
}

void ModulatorChain::ExtraModulatorRuntimeTargetSource::updateModulationChainIdAndColour(Processor* parentProcessor, const scriptnode::modulation::ParameterProperties& data, const std::function<String(int)>& parameterIdFunction)
{
#if USE_BACKEND
	auto numExtra = getNumExtraMods();
	auto numUsed = data.getNumUsed(numExtra);

	for (int i = 0; i < numExtra; i++)
	{
		auto pIndex = data.getParameterIndex(i);
		auto modChain = getModulatorChain(i);

		String newId;
		Colour newColour;

		if (pIndex != -1 && i < numUsed)
		{
			newId = parameterIdFunction(pIndex);

			if(!newId.isEmpty())
				newId += " Mod";
			else
				newId = "Extra " + String(i+1);

			newColour = data.getModulationColour(pIndex);
		}
		else
		{
			newId = "Extra " + String(i + 1);
			newColour = HiseModulationColours().getColour(HiseModulationColours::ColourId::ExtraMod);
		}

		auto prevId = modChain->getId();

		if (prevId != newId)
			modChain->setDisplayName(newId);

		if(modChain->getColour() != newColour)
			modChain->setColour(newColour);
	}
#endif
}

void ModulatorChain::ExtraModulatorRuntimeTargetSource::updateModulationProperties(const ParameterProperties& pp, const ParameterInitData::QueryFunction& pf)
{
	int idx = 0;

	for (auto& mb : extraMods)
	{
		auto isUsed = pp.isUsed(idx);

		auto parent = mb->getChain()->getParentProcessor();
		auto parameterIndex = pp.getParameterIndex(idx);
		auto mode = Modulation::getModeFromModProperties(pp, parameterIndex);

		if (auto ms = dynamic_cast<ModulatorSynth*>(parent))
		{
			mb->getChain()->checkRelease = (mode == Modulation::Mode::GainMode);
		}

		mb->getChain()->setBypassed(!isUsed);
		mb->setForceProcessing(isUsed);

		if (isUsed)
		{
			if (parameterIndex != -1)
			{
				if(mode == Modulation::Mode::PitchMode)
				{
					mb->getChain()->setZeroPosition(0.5f);
					mb->getChain()->setMode(mode, sendNotificationAsync);
					mb->getChain()->setInitialValue(mb->getChain()->getInitialValue());
					mb->getChain()->setTableValueConverter(Modulation::getValueAsSemitone);
					mb->setClampTo0To1(false);

					

				}
				else
				{
					mb->setClampTo0To1(true);

					auto zp = mode == Modulation::Mode::PanMode ? 0.5f : 0.0f;
					mb->getChain()->setZeroPosition(zp);

					// For some reason we'll force the CombinedMode....
					auto modeToUse = mb->getChain()->checkRelease ? Modulation::Mode::GainMode :
						Modulation::Mode::CombinedMode;

					mb->getChain()->setMode(modeToUse, sendNotificationAsync);

					auto pd = pf(parameterIndex);

					auto initialValue = pd.ir.convertTo0to1(pd.initValue, false);
					mb->getChain()->setInitialValue(initialValue);

					mb->getChain()->setTableValueConverter([pd](float v)
					{
						v = pd.ir.convertFrom0to1(v, false);

						if (pd.vtc.active)
							return pd.vtc.getTextForValue(v);
						else
							return String(v);
					});
				}
			}
		}

		idx++;
	}
}


bool ModulatorChain::onIntensityDrag(bool isMouseDown, float delta)
{
	if(!shouldBeProcessedAtAll())
		return false;

	if(isMouseDown)
	{
		intensityValues.clear();

		ModIterator<Modulator> iter(this);

		while(auto m = iter.next())
		{
			auto mod = dynamic_cast<Modulation*>(m);
			intensityValues.push_back(mod->getIntensity());
		}
	}
	else
	{
		ModIterator<Modulator> iter(this);

		int idx = 0;

		while(auto m = iter.next())
		{
			auto mod = dynamic_cast<Modulation*>(m);

			jassert(isPositiveAndBelow(idx, intensityValues.size()));

			auto intensity = intensityValues[idx++];

			intensity += delta * JUCE_LIVE_CONSTANT_OFF(0.333f);

			auto mv = mod->getMode() == Modulation::GainMode? 0.0f : -1.0f;
			intensity = jlimit(mv, 1.0f, intensity);
			mod->setIntensity(intensity);
		}
	}

	return true;
}

void ModulatorChain::AddBufferData::prepare(double sampleRate, int blockSize)
{
	auto numToAllocate = blockSize / HISE_EVENT_RASTER;

	timeVariantValues.setSize(numToAllocate);
	envelopeValues.setSize(numToAllocate);

	FloatVectorOperations::clear(timeVariantValues.begin(), numToAllocate);
	FloatVectorOperations::clear(envelopeValues.begin(), numToAllocate);
}

void ModulatorChain::AddBufferData::checkActiveState(ModulatorChain* parent)
{
	CombinedModIterator<TimeVariantModulator> tv(parent);
	CombinedModIterator<VoiceStartModulator> vs(parent);
	CombinedModIterator<EnvelopeModulator> ev(parent);
	CombinedModIterator<MonophonicEnvelope> me(parent);

	useVoiceStartValue = vs.nextWithMode<Modulation::Mode::OffsetMode>() != nullptr;
	useTimeVariantBuffer = tv.nextWithMode<Modulation::Mode::OffsetMode>() != nullptr;
	useTimeVariantBuffer |= me.nextWithMode<Modulation::Mode::OffsetMode>() != nullptr;
	useEnvelopeValues = ev.nextWithMode<Modulation::Mode::OffsetMode>() != nullptr;

	clearModValues();
}


std::pair<std::pair<float, float>, Range<double>> ModulatorChain::getCombinedOutputValues() const
{
	jassert(getMode() == Modulation::Mode::CombinedMode);

	CombinedModIterator<Modulator> iter(this);

	float scaleValue = getInitialValueInternal();
	float addValue = 0.0f;

	double maxValue = scaleValue;
	double minValue = scaleValue;

	if(hasActivePolyEnvelopes())
		addValue += addBufferData->envelopeValues[0];
	else
	{
		if(hasActiveTimeVariantMods() || hasActiveMonoEnvelopes())
			addValue += addBufferData->currentTimeVariantAddValue;

		if(hasActiveVoiceStartMods())
			addValue += addBufferData->addStartValue;
	}

	while(auto m = iter.nextWithMode<Modulation::Mode::GainMode>())
	{
		auto mv = m->getOutputValue();
		auto iv = dynamic_cast<Modulation*>(m)->calcGainIntensityValue(mv);

		scaleValue -= zeroPosition;
		scaleValue *= iv;
		scaleValue += zeroPosition;

		auto low = dynamic_cast<Modulation*>(m)->calcGainIntensityValue(0.0f);

		minValue -= zeroPosition;
		minValue *= low;
		minValue += zeroPosition;
	}

	while(auto m = iter.nextWithMode<Modulation::Mode::OffsetMode>())
	{
		auto bipolar = dynamic_cast<Modulation*>(m)->isBipolar();
		auto intensity = dynamic_cast<Modulation*>(m)->getIntensity();

		if(bipolar)
		{
			minValue -= hmath::abs(intensity);
			maxValue += hmath::abs(intensity);
		}
		else
		{
			if(intensity < 0.0f)
				minValue += intensity;
			else
				maxValue += intensity;
		}
	}

	minValue = jlimit(0.0, 1.0, minValue);
	maxValue = jlimit(0.0, 1.0, maxValue);

	Range<double> nr(minValue, maxValue);

	return { { scaleValue, addValue }, nr };
}

bool ModulatorChain::checkModulatorStructure()
{
	
	// Check the array size
	const bool arraySizeCorrect = allModulators.size() == (voiceStartModulators.size() + envelopeModulators.size() + variantModulators.size());
		
	// Check the correct voice size
	bool correctVoiceAmount = true;
	for(int i = 0; i < envelopeModulators.size(); ++i)
	{
		if(envelopeModulators[i]->polyManager.getVoiceAmount() != polyManager.getVoiceAmount()) correctVoiceAmount = false;
	};
    
    jassert(arraySizeCorrect);
    jassert(correctVoiceAmount);

	return arraySizeCorrect && correctVoiceAmount;
}

void TimeVariantModulatorFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(LfoModulator);
	ADD_NAME_TO_TYPELIST(ControlModulator);
	ADD_NAME_TO_TYPELIST(PitchwheelModulator);
	ADD_NAME_TO_TYPELIST(MacroModulator);
	ADD_NAME_TO_TYPELIST(GlobalTimeVariantModulator);
	ADD_NAME_TO_TYPELIST(JavascriptTimeVariantModulator);
    ADD_NAME_TO_TYPELIST(HardcodedTimeVariantModulator);
}

void VoiceStartModulatorFactoryType::fillArrayWithTypes(Array<ProcessorEntry>& typeNames)
{
	ADD_NAME_TO_TYPELIST(ConstantModulator);
	ADD_NAME_TO_TYPELIST(VelocityModulator);
	ADD_NAME_TO_TYPELIST(KeyModulator);
	ADD_NAME_TO_TYPELIST(RandomModulator);
	ADD_NAME_TO_TYPELIST(GlobalVoiceStartModulator);
	ADD_NAME_TO_TYPELIST(GlobalStaticTimeVariantModulator);
	ADD_NAME_TO_TYPELIST(ArrayModulator);
	ADD_NAME_TO_TYPELIST(JavascriptVoiceStartModulator);
	ADD_NAME_TO_TYPELIST(EventDataModulator);
}

void VoiceStartModulatorFactoryType::fillTypeNameList()
{
	fillArrayWithTypes(typeNames);
}

void EnvelopeModulatorFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(SimpleEnvelope);
	ADD_NAME_TO_TYPELIST(AhdsrEnvelope);
	ADD_NAME_TO_TYPELIST(TableEnvelope);
	ADD_NAME_TO_TYPELIST(JavascriptEnvelopeModulator);
	ADD_NAME_TO_TYPELIST(MPEModulator);
	ADD_NAME_TO_TYPELIST(ScriptnodeVoiceKiller);
	ADD_NAME_TO_TYPELIST(GlobalEnvelopeModulator);
	ADD_NAME_TO_TYPELIST(EventDataEnvelope);
	ADD_NAME_TO_TYPELIST(HardcodedEnvelopeModulator);
	ADD_NAME_TO_TYPELIST(MatrixModulator);
	ADD_NAME_TO_TYPELIST(FlexAhdsrEnvelope);
}



Processor *ModulatorChainFactoryType::createProcessor(int typeIndex, const String &id)
{
	Identifier s = typeNames[typeIndex].type;

	FactoryType *factory;

	if	   (voiceStartFactory->getProcessorTypeIndex(s) != -1)	factory = voiceStartFactory; 
	else if(timeVariantFactory->getProcessorTypeIndex(s) != -1) factory = timeVariantFactory; 
	else if(envelopeFactory->getProcessorTypeIndex(s) != -1)	factory = envelopeFactory; 
	else {														jassertfalse; return nullptr;};
		
	return MainController::createProcessor(factory, s, id);
};





int ModulatorChainFactoryType::fillPopupMenu(PopupMenu& menu, int startIndex)
{
	int index = startIndex;

	PopupMenu voiceMenu;
	index = voiceStartFactory->fillPopupMenu(voiceMenu, index);
	menu.addSubMenu("VoiceStart", voiceMenu);

	PopupMenu timeMenu;
	index = timeVariantFactory->fillPopupMenu(timeMenu, index);
	menu.addSubMenu("TimeVariant", timeMenu);

	PopupMenu envelopes;
	index = envelopeFactory->fillPopupMenu(envelopes, index);
	menu.addSubMenu("Envelopes", envelopes);

	return index;
}
} // namespace hise
