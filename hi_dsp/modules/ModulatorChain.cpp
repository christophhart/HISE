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
		ModulatorSubType* dummy = nullptr;

		init(dummy);
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

	ModulatorSubType ** start;
	ModulatorSubType ** ende;
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

const Chain::Handler* ModulatorChain::getHandler() const
{return &handler;}

FactoryType* ModulatorChain::getFactoryType() const
{return modulatorFactory;}

Colour ModulatorChain::getColour() const
{ 
	if(Modulator::getColour() != Colours::transparentBlack) return Modulator::getColour();
	else
	{
		//return getMode() == GainMode ? Colour(0xffD9A450) : Colour(0xff628214);
		if (getMode() == GainMode)
		{
			return JUCE_LIVE_CONSTANT_OFF(Colour(0xffbe952c));
		}
		else
		{
			return JUCE_LIVE_CONSTANT_OFF(Colour(0xff7559a4));
		}
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
{ jassertfalse; return -1.0; }

float ModulatorChain::getCurrentMonophonicStartValue() const noexcept
{ return monophonicStartValue; }

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
	currentMonophonicRampValue(c->getInitialValue())
{
	FloatVectorOperations::fill(currentConstantVoiceValues, c->getInitialValue(), NUM_POLYPHONIC_VOICES);
	FloatVectorOperations::fill(currentRampValues, c->getInitialValue(), NUM_POLYPHONIC_VOICES);

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
}

void ModulatorChain::ModChainWithBuffer::handleHiseEvent(const HiseEvent& m)
{
	if (c->shouldBeProcessedAtAll())
		c->handleHiseEvent(m);
}

void ModulatorChain::ModChainWithBuffer::resetVoice(int voiceIndex)
{
	if (c->hasActiveEnvelopesAtAll())
	{
		c->reset(voiceIndex);
		currentRampValues[voiceIndex] = 0.0f;
		currentMonophonicRampValue = c->getInitialValue();
	}
}

void ModulatorChain::ModChainWithBuffer::stopVoice(int voiceIndex)
{
	if (c->hasVoiceModulators())
		c->stopVoice(voiceIndex);
}

void ModulatorChain::ModChainWithBuffer::startVoice(int voiceIndex)
{
	float firstDynamicValue = 1.0f;

	if (options.includeMonophonicValues && c->hasMonophonicTimeModulationMods())
	{
		Modulation::applyModulationValue(c->getMode(), firstDynamicValue, currentMonophonicRampValue);

	}

	if (c->hasVoiceModulators())
	{
		auto startValue = c->startVoice(voiceIndex);

		Modulation::applyModulationValue(c->getMode(), firstDynamicValue, startValue);
	}
	
	Modulation::applyModulationValue(c->getMode(), firstDynamicValue, c->getCurrentMonophonicStartValue());

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
			currentConstantValue = 1.0f;
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

		ModIterator<TimeVariantModulator> iter(c);
		
		FloatVectorOperations::fill(modBuffer.monoValues + startSample_cr, c->getInitialValue(), numSamples_cr);

		while (auto mod = iter.next())
		{
			mod->render(modBuffer.monoValues, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
		}

		ModIterator<MonophonicEnvelope> iter2(c);

		while (auto mod = iter2.next())
		{
			mod->render(0, modBuffer.monoValues, modBuffer.scratchBuffer, startSample_cr, numSamples_cr);
		}

		currentMonoValue = modBuffer.monoValues[startSample_cr];

		monoExpandChecker = false;
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

	const bool useMonophonicData = options.includeMonophonicValues && c->hasMonophonicTimeModulationMods();

	auto voiceData = modBuffer.voiceValues;
	const auto monoData = modBuffer.monoValues;

	jassert(startSample % HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR == 0);

	int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	int numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	bool constantValuesAreSmoothed = false;

	if (c->hasActivePolyMods())
	{
		const float thisConstantValue = c->getConstantVoiceValue(voiceIndex);
		const float previousConstantValue = currentConstantVoiceValues[voiceIndex];

		const bool smoothConstantValue = (std::abs(previousConstantValue - thisConstantValue) > 0.01f);

		if (smoothConstantValue)
		{
			constantValuesAreSmoothed = true;

			const float start = previousConstantValue;
			const float delta = (thisConstantValue - start) / (float)numSamples_cr;
			int numLoop = numSamples_cr;
			float value = start;
			float* loop_ptr = voiceData + startSample_cr;

			while (--numLoop >= 0)
			{
				*loop_ptr++ = value;
				value += delta;
			}
		}
		else
		{
			FloatVectorOperations::fill(voiceData + startSample_cr, thisConstantValue, numSamples_cr);
		}

		setConstantVoiceValueInternal(voiceIndex, thisConstantValue);

		if (c->hasActivePolyEnvelopes())
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

			currentVoiceData = voiceData;
			
#if JUCE_DEBUG
			polyExpandChecker = false;
#endif
		}
		else if (useMonophonicData)
		{
			applyMonophonicValuesToVoiceInternal(voiceData + startSample_cr, monoData + startSample_cr, numSamples_cr);

			
			currentVoiceData = voiceData;

#if JUCE_DEBUG
			polyExpandChecker = false;
#endif
		}
		else
		{
			// Set it to nullptr, and let the module use the constant value instead...
			currentVoiceData = nullptr;
		}
	}
	else if (useMonophonicData)
	{
		setConstantVoiceValueInternal(voiceIndex, c->getInitialValue());

#if HISE_USE_SQUARED_TIMEVARIANT_MOD_VALUES_BUG
		
		if (c->getMode() == Modulation::PanMode)
		{
			// Use the default logic for pan
			FloatVectorOperations::copy(voiceData + startSample_cr, monoData + startSample_cr, numSamples_cr);
			currentVoiceData = voiceData;
		}
		else
		{
			// Now this is extremely annoying, but the old modulation scheme
			// falsely multiplied the monophonic modulation values with each other, so we 
			// must replicate this in order to keep consistency

			int numLoop = numSamples_cr;

			auto wp = voiceData + startSample_cr;
			auto rp = monoData + startSample_cr;

			while (--numLoop >= 0)
			{
				const float value = *rp++;
				*wp++ = value * value;
			}

			currentVoiceData = voiceData;
		}

		

#else
		if (options.voiceValuesReadOnly)
			currentVoiceData = monoData;
		else
		{
			FloatVectorOperations::copy(voiceData + startSample_cr, monoData + startSample_cr, numSamples_cr);
			currentVoiceData = voiceData;
		}
#endif

#if JUCE_DEBUG
		polyExpandChecker = false;
#endif
	}
	else
	{
		currentVoiceData = nullptr;

		setConstantVoiceValueInternal(voiceIndex, 1.0f);
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
	currentConstantValue = c->getInitialValue();
}

ModulatorChain::ModulatorChain(MainController *mc, const String &uid, int numVoices, Mode m, Processor *p): 
	EnvelopeModulator(mc, uid, numVoices, m),
	Modulation(m),
	handler(this),
	parentProcessor(p),
	isVoiceStartChain(false)
{
	activeVoices.setRange(0, numVoices, false);

	setMode(m, dontSendNotification);

	

	FloatVectorOperations::fill(lastVoiceValues, 1.0, NUM_POLYPHONIC_VOICES);

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
		return getInitialValue();

	auto m = getMode();

	if(m == Modulation::GainMode)
	{
		float value = 1.0f;

		ModIterator<VoiceStartModulator> iter(this);

		while (auto mod = iter.next())
		{
			const auto modValue = mod->getVoiceStartValue(voiceIndex);
			const auto intensityModValue = mod->calcGainIntensityValue(modValue);
			value *= intensityModValue;
		}

		return value;
	}
	else
	{
		float value = 0.0f;

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

	activeVoices.setBit(voiceIndex, true);

	polyManager.setLastStartedVoice(voiceIndex);

	ModIterator<VoiceStartModulator> iter(this);

	while(auto mod = iter.next())
		mod->startVoice(voiceIndex);

	const float startValue = getConstantVoiceValue(voiceIndex);
	lastVoiceValues[voiceIndex] = startValue;

	setOutputValue(startValue);

	monophonicStartValue = 1.0f;

	auto m = getMode();

	if (m == GainMode)
	{
		float envelopeStartValue = startValue;

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
		float envelopeStartValue = 0.0f;

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
	jassert(getMode() == GainMode || getMode() == Modulation::GlobalMode);

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

	
	
	for(int i = 0; i < envelopeModulators.size(); i++) envelopeModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);
	for(int i = 0; i < variantModulators.size(); i++) variantModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);

	jassert(checkModulatorStructure());
};

void ModulatorChain::setMode(Mode newMode, NotificationType n)
{
    setFactoryType(new ModulatorChainFactoryType(polyManager.getVoiceAmount(), newMode, parentProcessor));
    
    if(getMode() != newMode)
    {
        Modulation::setMode(newMode, n);
        
        for(auto m: allModulators)
            dynamic_cast<Modulation*>(m)->setMode(newMode, n);
    }
}

void ModulatorChain::setIsVoiceStartChain(bool isVoiceStartChain_)
{
	isVoiceStartChain = isVoiceStartChain_;

	if(isVoiceStartChain)
	{
		modulatorFactory = new VoiceStartModulatorFactoryType(polyManager.getVoiceAmount(), modulationMode, parentProcessor);
		
		// This sets the initial value to 1.0f for HiSlider::getDisplayValue();
		setOutputValue(1.0f);
	}
	else
	{
		modulatorFactory = new ModulatorChainFactoryType(polyManager.getVoiceAmount(), modulationMode, parentProcessor);
	}
}

ModulatorChain::ModulatorChainHandler::ModulatorChainHandler(ModulatorChain *handledChain) : 
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

	newModulator->addBypassListener(this);

	if (chain->isInitialized())
		newModulator->prepareToPlay(chain->getSampleRate(), chain->blockSize);
	
	const int index = siblingToInsertBefore == nullptr ? -1 : chain->allModulators.indexOf(dynamic_cast<Modulator*>(siblingToInsertBefore));

	newModulator->setParentProcessor(chain);

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

	chain->sendChangeMessage();
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

void VoiceStartModulatorFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(ConstantModulator);
	ADD_NAME_TO_TYPELIST(VelocityModulator);
	ADD_NAME_TO_TYPELIST(KeyModulator);
	ADD_NAME_TO_TYPELIST(RandomModulator);
	ADD_NAME_TO_TYPELIST(GlobalVoiceStartModulator);
	ADD_NAME_TO_TYPELIST(GlobalStaticTimeVariantModulator);
	ADD_NAME_TO_TYPELIST(ArrayModulator);
	ADD_NAME_TO_TYPELIST(JavascriptVoiceStartModulator);
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

bool ModBufferExpansion::isEqual(float rampStart, const float* data, int numElements)
{
	auto range = FloatVectorOperations::findMinAndMax(data, numElements);
	return (range.contains(rampStart) || range.getEnd() == rampStart) && range.getLength() < 0.001f;
}

bool ModBufferExpansion::expand(const float* modulationData, int startSample, int numSamples, float& rampStart)
{
	const int startSample_cr = startSample / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
	const int numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

	if (isEqual(rampStart, modulationData + startSample_cr, numSamples_cr))
	{
		rampStart = modulationData[startSample_cr];
		return false;
	}
	else
	{
#if HISE_USE_CONTROLRATE_DOWNSAMPLING

		float* temp = (float*)alloca(sizeof(float) * (numSamples_cr));
		FloatVectorOperations::copy(temp, modulationData + startSample_cr, numSamples_cr);
		float* d = const_cast<float*>(modulationData + startSample);

		constexpr float ratio = 1.0f / (float)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

		for (int i = 0; i < numSamples_cr; i++)
		{
			AlignedSSERamper<HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR> ramper(d);

			const float delta1 = (temp[i] - rampStart) * ratio;
			ramper.ramp(rampStart, delta1);
			rampStart = temp[i];
			d += HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		}
#endif

		return true;
	}
}

} // namespace hise
