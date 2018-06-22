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

ModulatorChain::ModulatorChain(MainController *mc, const String &uid, int numVoices, Mode m, Processor *p): 
	EnvelopeModulator(mc, uid, numVoices, m),
	Modulation(m),
	handler(this),
	parentProcessor(p),
	isVoiceStartChain(false)
{
	activeVoices.setRange(0, numVoices, false);
	setFactoryType(new ModulatorChainFactoryType(numVoices, m, p));

	FloatVectorOperations::fill(lastVoiceValues, 1.0, NUM_POLYPHONIC_VOICES);

	if (Identifier::isValidIdentifier(uid))
	{
		chainIdentifier = Identifier(uid);
	}

	setEditorState(Processor::Visible, false, dontSendNotification);
};

ModulatorChain::~ModulatorChain()
{
	handler.clear();

	newFunkyBuffer.clear();
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

	Iterator<EnvelopeModulator> iter(this);
	
	while(auto mod = iter.next())
		mod->reset(voiceIndex);

	Iterator<MonophonicEnvelope> iter2(this);

	while (auto mod = iter2.next())
		mod->reset(voiceIndex);
};

void ModulatorChain::handleHiseEvent(const HiseEvent &m)
{
	jassert(shouldBeProcessedAtAll());

	EnvelopeModulator::handleHiseEvent(m);

	Iterator<Modulator> iter(this);

	while(auto mod = iter.next())
		mod->handleHiseEvent(m);
};


void ModulatorChain::allNotesOff()
{
	if (hasVoiceModulators())
		VoiceModulation::allNotesOff();
}

float ModulatorChain::getConstantVoiceValue(int voiceIndex) const
{
	if (!hasActiveVoiceStartMods())
		return 1.0f;

	if (getMode() == Modulation::GainMode)
	{
		float value = 1.0f;

		Iterator<VoiceStartModulator> iter(this);

		while(auto mod = iter.next())
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

		Iterator<VoiceStartModulator> iter(this);

		while(auto mod = iter.next())
		{
			float modValue = mod->getVoiceStartValue(voiceIndex);

			if (mod->isBipolar())
				modValue = 2.0f * modValue - 1.0f;

			const float intensityModValue = mod->calcPitchIntensityValue(modValue);
			value += intensityModValue;
		}

		return Modulation::PitchConverters::normalisedRangeToPitchFactor(value);
	}
};

void ModulatorChain::startVoice(int voiceIndex)
{
	jassert(hasVoiceModulators());

	activeVoices.setBit(voiceIndex, true);

	polyManager.setLastStartedVoice(voiceIndex);

	Iterator<VoiceStartModulator> iter(this);

	while(auto mod = iter.next())
		mod->startVoice(voiceIndex);

	Iterator<EnvelopeModulator> iter2(this);

	while(auto mod = iter2.next())
	{
		mod->startVoice(voiceIndex);
		mod->polyManager.setLastStartedVoice(voiceIndex);
	}

	Iterator<MonophonicEnvelope> iter3(this);

	while (auto mod = iter3.next())
	{
		mod->startVoice(voiceIndex);
		mod->polyManager.setLastStartedVoice(voiceIndex);
	}

	const float startValue = getConstantVoiceValue(voiceIndex);
	lastVoiceValues[voiceIndex] = startValue;

	setOutputValue(startValue);
}



void ModulatorChain::stopVoice(int voiceIndex)
{
	jassert(hasVoiceModulators());

	activeVoices.setBit(voiceIndex, false);

	Iterator<EnvelopeModulator> iter(this);

	while(auto mod = iter.next())
		mod->stopVoice(voiceIndex);

	Iterator<MonophonicEnvelope> iter2(this);

	while (auto mod = iter2.next())
		mod->stopVoice(voiceIndex);
};

void ModulatorChain::renderAllModulatorsAsMonophonic(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	// Doesn't make sense to call this method on a polyphonic chain!
	jassert(polyManager.getVoiceAmount() == 1);
	jassert(hasActivePolyMods() || hasMonophonicTimeModulationMods());

	bool usePolyValues = false;

	if (hasActivePolyMods())
	{
		renderVoice(0, startSample, numSamples);
		usePolyValues = true;
	}

	if (hasMonophonicTimeModulationMods())
	{
		renderNextBlock(buffer, startSample, numSamples);

		if(usePolyValues)
			FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), getVoiceValues(0), numSamples);
	}
	else
	{
		FloatVectorOperations::copy(buffer.getWritePointer(0, startSample), getVoiceValues(0), numSamples);

#if ENABLE_ALL_PEAK_METERS
		if (!isVoiceStartChain)
			setOutputValue(buffer.getSample(0, startSample));
#endif

	}
};

void ModulatorChain::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);
	blockSize = samplesPerBlock;

	if(!isVoiceStartChain)
		newFunkyBuffer.setMaxSize(samplesPerBlock);
	
	for(int i = 0; i < envelopeModulators.size(); i++) envelopeModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);
	for(int i = 0; i < variantModulators.size(); i++) variantModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);

	jassert(checkModulatorStructure());
};

float ModulatorChain::calculateNewValue()
{
	jassertfalse;

	return 1.0f;
};


void ModulatorChain::setIsVoiceStartChain(bool isVoiceStartChain_)
{
	isVoiceStartChain = isVoiceStartChain_;

	if(isVoiceStartChain)
	{
		modulatorFactory = new VoiceStartModulatorFactoryType(polyManager.getVoiceAmount(), modulationMode, parentProcessor);
		
		newFunkyBuffer.clear();

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

	{
		MainController::ScopedSuspender ss(chain->getMainController());

		newModulator->setIsOnAir(true);

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

		if (JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(newModulator))
		{
			sp->compileScript();
		}

		checkActiveState();
	}

	

	if (auto ltp = dynamic_cast<LookupTableProcessor*>(newModulator))
	{
		WeakReference<Modulator> mod = newModulator;

		auto& cf = tableValueConverter;

		auto isPitch = chain->getMode() == Modulation::PitchMode;

		auto f = [mod, cf, isPitch](float input)
		{
			if (mod.get() != nullptr)
			{
				auto modulation = dynamic_cast<Modulation*>(mod.get());
				auto intensity = modulation->getIntensity();

				if (isPitch)
				{
					float normalizedInput;

					if (modulation->isBipolar())
						normalizedInput = (input-0.5f) * intensity * 2.0f;
					else
						normalizedInput = (input) * intensity;

					return String(normalizedInput * 12.0f, 1) + " st";
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

	for(int i = 0; i < getNumModulators(); ++i)
	{
		if(chain->allModulators[i] == modulatorToBeDeleted) chain->allModulators.remove(i);
	};
		
	for(int i = 0; i < chain->variantModulators.size(); ++i)
	{
		if(chain->variantModulators[i] == modulatorToBeDeleted) chain->variantModulators.remove(i, deleteMod);
	};

	for(int i = 0; i < chain->envelopeModulators.size(); ++i)
	{
		if(chain->envelopeModulators[i] == modulatorToBeDeleted) chain->envelopeModulators.remove(i, deleteMod);
	};

	for(int i = 0; i < chain->voiceStartModulators.size(); ++i) 
	{
		if(chain->voiceStartModulators[i] == modulatorToBeDeleted) chain->voiceStartModulators.remove(i, deleteMod);
	};

	jassert(chain->checkModulatorStructure());

	checkActiveState();
};


void ModulatorChain::ModulatorChainHandler::remove(Processor *processorToBeRemoved, bool deleteMod)
{
	notifyListeners(Listener::ProcessorDeleted, processorToBeRemoved);

	ScopedLock sl(chain->getMainController()->getLock());

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
}

bool ModulatorChain::isPlaying(int voiceIndex) const
{
	jassert(hasActivePolyEnvelopes());
	jassert(getMode() == GainMode);

	if (isBypassed())
		return false;

	if (!hasActivePolyEnvelopes())
		return activeVoices[voiceIndex];

	bool anyEnvelopePlaying = false;

	Iterator<EnvelopeModulator> iter(this);

	while(auto mod = iter.next())
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


void ModulatorChain::ModChainWithBuffer::calculateModulationValuesForCurrentVoice(int voiceIndex, int startSample, int numSamples)
{
	jassert(voiceIndex >= 0);

	const bool useMonophonicData = includeMonophonicValues && c->hasMonophonicTimeModulationMods();

	auto voiceData = c->newFunkyBuffer.voiceValues;
	const auto monoData = c->newFunkyBuffer.monoValues;

	if (c->hasActivePolyMods())
	{
		float thisConstantValue = useConstantValueForBuffer ? c->getConstantVoiceValue(voiceIndex) : 1.0f;
		
		const bool smoothConstantValue = useConstantValueForBuffer && 
										 (std::fabsf(currentConstantVoiceValues[voiceIndex] - thisConstantValue) > 0.01f);
		
		

		if (c->hasActivePolyEnvelopes())
		{
			if (smoothConstantValue)
			{
				const float start = currentConstantVoiceValues[voiceIndex];
				const float delta = (thisConstantValue - start) / (float)numSamples;
				int numLoop = numSamples;
				float value = start;
				float* loop_ptr = voiceData + startSample;

				while (--numLoop >= 0)
				{
					*loop_ptr++ = value;
					value += delta;
				}
			}
			else
			{
				FloatVectorOperations::fill(voiceData + startSample, thisConstantValue, numSamples);
			}

			Iterator<EnvelopeModulator> iter(c);

			while (auto mod = iter.next())
			{
				mod->render(voiceIndex, voiceData, c->newFunkyBuffer.scratchBuffer, startSample, numSamples);
			}

			if (useMonophonicData)
				FloatVectorOperations::multiply(voiceData + startSample, monoData + startSample, numSamples);
			
			currentVoiceData = voiceData;
		}
		else if (useMonophonicData)
		{
			if (useConstantValueForBuffer || !voiceValuesReadOnly)
			{
				FloatVectorOperations::fill(voiceData+startSample, thisConstantValue, numSamples);
				FloatVectorOperations::multiply(voiceData+startSample, monoData + startSample, numSamples);
				currentVoiceData = voiceData;
			}
			else
			{
				currentVoiceData = monoData;
			}
		}
		else
		{
			// Set it to nullptr, and let the module use the constant value instead...
			currentVoiceData = nullptr;
		}

		currentConstantVoiceValues[voiceIndex] = thisConstantValue;
		lastConstantVoiceValue = thisConstantValue;
	}
	else if (useMonophonicData)
	{
		currentConstantVoiceValues[voiceIndex] = 1.0f;
		lastConstantVoiceValue = 1.0f;

		if (voiceValuesReadOnly)
			currentVoiceData = monoData;
		else
		{
			FloatVectorOperations::copy(voiceData + startSample, monoData + startSample, numSamples);
			currentVoiceData = voiceData;
		}
	}
	else
	{
		currentVoiceData = nullptr;
		currentConstantVoiceValues[voiceIndex] = 1.0f;
		lastConstantVoiceValue = 1.0f;
	}

	
}

void ModulatorChain::ModChainWithBuffer::applyMonophonicModulationValues(AudioSampleBuffer& b, int startSample, int numSamples)
{
	if (c->hasMonophonicTimeModulationMods())
	{
		for (int i = 0; i < b.getNumSamples(); i++)
		{
			FloatVectorOperations::multiply(b.getWritePointer(i, startSample), c->newFunkyBuffer.monoValues, numSamples);
		}
	}
}

const float* ModulatorChain::ModChainWithBuffer::getReadPointerForVoiceValues(int startSample) const
{
	return currentVoiceData != nullptr ? currentVoiceData + startSample : nullptr;	
}

float* ModulatorChain::ModChainWithBuffer::getWritePointerForVoiceValues(int startSample)
{
	jassert(!voiceValuesReadOnly);

	return currentVoiceData != nullptr ? const_cast<float*>(currentVoiceData) + startSample : nullptr;
}

const float* ModulatorChain::ModChainWithBuffer::getMonophonicModulationValues(int startSample) const
{
	if (c->hasMonophonicTimeModulationMods())
		return c->newFunkyBuffer.monoValues + startSample;

	return nullptr;
}

float ModulatorChain::ModChainWithBuffer::getConstantModulationValue() const
{
	return lastConstantVoiceValue;
}

float ModulatorChain::ModChainWithBuffer::getOneModulationValue(int startSample) const
{
	return currentVoiceData != nullptr ? currentVoiceData[startSample] : lastConstantVoiceValue;
}

float* ModulatorChain::ModChainWithBuffer::getScratchBuffer()
{
	return c->newFunkyBuffer.scratchBuffer;
}

void ModulatorChain::renderVoice(int voiceIndex, int startSample, int numSamples)
{
	jassertfalse;
	// #WILLKILL
	return;

    ADD_GLITCH_DETECTOR(parentProcessor, DebugLogger::Location::ModulatorChainVoiceRendering);
    
	polyManager.clearCurrentVoice();
	polyManager.setCurrentVoice(voiceIndex);

	// Use the internal buffer from timeModulation as working buffer.

	//initializeBuffer(internalBuffer, startSample, numSamples);

	jassert(hasActivePolyMods());

	const int startIndex = startSample;
	const int sampleAmount = numSamples;

	if(hasActivePolyMods())
	{
		const float constantVoiceValue = getConstantVoiceValue(voiceIndex);
		const float lastVoiceValue = lastVoiceValues[voiceIndex];

		if (std::abs(constantVoiceValue - lastVoiceValue) > 0.001f)
		{
			const float stepSize = (constantVoiceValue - lastVoiceValue) / (float)numSamples;
			float* bufferPointer = internalBuffer.getWritePointer(0, startSample);
			float rampedGain = lastVoiceValue;

			for (int i = 0; i < numSamples; i++)
			{
				bufferPointer[i] = rampedGain;
				rampedGain += stepSize;
			}
		}
		else
		{
			FloatVectorOperations::fill(internalBuffer.getWritePointer(0, startSample), constantVoiceValue, numSamples);
		}

		lastVoiceValues[voiceIndex] = constantVoiceValue;

		Iterator<EnvelopeModulator> iter(this);

		while(auto mod = iter.next())
		{
			if (mod->isInMonophonicMode())
			{
				jassertfalse;
				continue;
			}

			mod->polyManager.setCurrentVoice(voiceIndex);

			float* bufferPointer = internalBuffer.getWritePointer(0, 0);

			AudioSampleBuffer b1(&bufferPointer, 1, startSample + numSamples);

			mod->renderNextBlock(b1, startSample, numSamples);
			mod->polyManager.clearCurrentVoice();
		}

		if (getMode() != Modulation::PitchMode)
			FloatVectorOperations::clip(internalBuffer.getWritePointer(0, startIndex), internalBuffer.getReadPointer(0, startIndex), (getMode() == Modulation::GainMode ? 0.0f : -1.0f), 1.0f, sampleAmount);

		if (voiceIndex == polyManager.getLastStartedVoice())
			pushPlotterValues(internalBuffer.getReadPointer(0, 0), startSample, sampleAmount);
	}

	CHECK_AND_LOG_BUFFER_DATA_WITH_ID(parentProcessor, chainIdentifier, DebugLogger::Location::ModulatorChainVoiceRendering, internalBuffer.getReadPointer(0, startIndex), true, sampleAmount);

	// Copy the result to the voice buffer
	//FloatVectorOperations::copy(internalVoiceBuffer.getWritePointer(voiceIndex, startIndex), internalBuffer.getReadPointer(0, startIndex), sampleAmount);

#if ENABLE_ALL_PEAK_METERS
	if (voiceIndex == polyManager.getLastStartedVoice())
	{
		voiceOutputValue = internalBuffer.getSample(0, startIndex);
	}
#endif
}

void ModulatorChain::renderNextBlock(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	jassertfalse;
	// #WILLKILL

	const int startIndex = startSample;
	const int sampleAmount = numSamples;

	jassert(!isVoiceStartChain);
	jassert(hasMonophonicTimeModulationMods());
	jassert(getSampleRate() > 0);

	ADD_GLITCH_DETECTOR(parentProcessor, DebugLogger::Location::ModulatorChainTimeVariantRendering);

	initializeBuffer(internalBuffer, startSample, numSamples);

	Iterator<TimeVariantModulator> iter(this);

	while (auto mod = iter.next())
		mod->renderNextBlock(internalBuffer, startSample, numSamples);

	Iterator<MonophonicEnvelope> iter2(this);

	while (auto mod = iter2.next())
		mod->renderNextBlock(internalBuffer, startSample, numSamples);

#if ENABLE_ALL_PEAK_METERS
	setOutputValue(internalBuffer.getSample(0, startSample));
#endif

	FloatVectorOperations::copy(buffer.getWritePointer(0, startIndex), internalBuffer.getReadPointer(0, startIndex), sampleAmount);


	CHECK_AND_LOG_BUFFER_DATA_WITH_ID(parentProcessor, chainIdentifier, DebugLogger::Location::ModulatorChainTimeVariantRendering, internalBuffer.getReadPointer(0, startIndex), true, sampleAmount);
    
}

void ModulatorChain::newRenderMonophonicValues(int startSample, int numSamples)
{
	jassert(!isVoiceStartChain);
	jassert(hasMonophonicTimeModulationMods());
	jassert(getSampleRate() > 0);

	if (hasMonophonicTimeModulationMods())
	{
		FloatVectorOperations::fill(newFunkyBuffer.monoValues + startSample, 1.0f, numSamples);

		Iterator<TimeVariantModulator> iter(this);

		AudioSampleBuffer monoModValues(&newFunkyBuffer.monoValues, 1, startSample + numSamples);

		while (auto mod = iter.next())
		{
			mod->render(newFunkyBuffer.monoValues, newFunkyBuffer.scratchBuffer, startSample, numSamples);
		}

		Iterator<MonophonicEnvelope> iter2(this);

		while (auto mod = iter2.next())
		{
			mod->render(0, newFunkyBuffer.monoValues, newFunkyBuffer.scratchBuffer, startSample, numSamples);
		}
	}
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

	return arraySizeCorrect && correctVoiceAmount;
}

void TimeVariantModulatorFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(LfoModulator);
	ADD_NAME_TO_TYPELIST(ControlModulator);
	ADD_NAME_TO_TYPELIST(PitchwheelModulator);
	ADD_NAME_TO_TYPELIST(MacroModulator);
    ADD_NAME_TO_TYPELIST(AudioFileEnvelope);
	ADD_NAME_TO_TYPELIST(GlobalTimeVariantModulator);
	ADD_NAME_TO_TYPELIST(CCDucker);
	ADD_NAME_TO_TYPELIST(JavascriptTimeVariantModulator);
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
	ADD_NAME_TO_TYPELIST(CCEnvelope);
	ADD_NAME_TO_TYPELIST(JavascriptEnvelopeModulator);
	ADD_NAME_TO_TYPELIST(MPEModulator);
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

void ModulatorChain::ModChainWithBuffer::calculateMonophonicModulationValues(int startSample, int numSamples)
{
	if (c->hasMonophonicTimeModulationMods())
	{
		c->newRenderMonophonicValues(startSample, numSamples);
	}
}


} // namespace hise
