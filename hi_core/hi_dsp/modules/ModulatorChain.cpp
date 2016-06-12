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

ModulatorChain::ModulatorChain(MainController *mc, const String &uid, int numVoices, Mode m, Processor *p): 
	EnvelopeModulator(mc, uid, numVoices, m),
	Modulation(m),
	handler(this),
	parentProcessor(p),
	isVoiceStartChain(false)
{
	activeVoices.setRange(0, numVoices, false);
	setFactoryType(new ModulatorChainFactoryType(numVoices, m, p));

	setEditorState(Processor::Visible, false, dontSendNotification);
};

ChainHandler *ModulatorChain::getHandler() {return &handler;};


bool ModulatorChain::shouldBeProcessed(bool checkPolyphonicModulators) const
{ 
	bool empty = checkPolyphonicModulators ? ( (envelopeModulators.size() + voiceStartModulators.size()) == 0 ) : 
												variantModulators.size() == 0;
	return ( (!isBypassed()) && (!empty) ); 
};

void ModulatorChain::reset(int voiceIndex)
{
	EnvelopeModulator::reset(voiceIndex);

	for(int i = 0; i < envelopeModulators.size(); ++i) envelopeModulators[i]->reset(voiceIndex);	
};

void ModulatorChain::handleMidiEvent(const MidiMessage &m)
{
	EnvelopeModulator::handleMidiEvent(m);

	for(int i = 0; i < voiceStartModulators.size(); i++) voiceStartModulators[i]->handleMidiEvent(m);

	for(int i = 0; i < envelopeModulators.size(); i++) envelopeModulators[i]->handleMidiEvent(m);

	for(int i = 0; i < variantModulators.size(); i++) variantModulators[i]->handleMidiEvent(m);
};


float ModulatorChain::getConstantVoiceValue(int voiceIndex) const
{
	float value = 1.0f;

	for (int i = 0; i < voiceStartModulators.size(); ++i)
	{
		const VoiceStartModulator *mod = voiceStartModulators[i];
		if( mod->isBypassed() ) continue;

		const float modValue = mod->getVoiceStartValue(voiceIndex);
			
		const float intensityModValue = mod->calcIntensityValue(modValue);

		applyModulationValue(intensityModValue, value);
	}

	

	return value;
};

void ModulatorChain::stopVoice(int voiceIndex)
{
	activeVoices.setBit(voiceIndex, false);

	for (int i = 0; i < envelopeModulators.size(); i++)
	{
		envelopeModulators[i]->stopVoice(voiceIndex);
	}
};

void ModulatorChain::renderAllModulatorsAsMonophonic(AudioSampleBuffer &buffer, int startSample, int numSamples)
{
	// Doesn't make sense to call this method on a polyphonic chain!
	jassert(polyManager.getVoiceAmount() == 1);
		
	renderNextBlock(buffer, startSample, numSamples);
	renderVoice(0, startSample, numSamples);
	FloatVectorOperations::multiply(buffer.getWritePointer(0, startSample), getVoiceValues(0), numSamples);
};

void ModulatorChain::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);
	blockSize = samplesPerBlock;

	internalVoiceBuffer = AudioSampleBuffer(polyManager.getVoiceAmount(), samplesPerBlock);
	envelopeTempBuffer = AudioSampleBuffer(1, samplesPerBlock);

	for(int i = 0; i < envelopeModulators.size(); i++) envelopeModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);
	for(int i = 0; i < variantModulators.size(); i++) variantModulators[i]->prepareToPlay(sampleRate, samplesPerBlock);

	jassert(checkModulatorStructure());
};

float ModulatorChain::calculateNewValue()
{

	jassertfalse;

	return 1.0f;

	/*
	if(isBypassed()) return 1.0f;

	const int voiceIndex = polyManager.getCurrentVoice();
	float value = getConstantVoiceValue(voiceIndex);

	for(int i = 0; i < envelopeModulators.size(); i++)
	{
		EnvelopeModulator *m = envelopeModulators[i];
		if(m->isBypassed() ) continue;

		m->polyManager.setCurrentVoice(voiceIndex);
		const float modValue = m->calcIntensityValue(m->calculateNewValue(bufferStart));
		m->applyModulationValue(modValue, value);
		m->polyManager.clearCurrentVoice();
	}

	for(int i = 0; i < variantModulators.size(); i++)
	{
		TimeVariantModulator *m = variantModulators[i];
		if(m->isBypassed() ) continue;

		const float modValue = m->calcIntensityValue(m->calculateNewValue(bufferStart));

		m->applyModulationValue(modValue, value);
	}

	if(bufferStart)
	{
		setOutputValue(value);
	}

	return value;
	*/
};


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

void ModulatorChain::ModulatorChainHandler::addModulator(Modulator *newModulator, Processor *siblingToInsertBefore)
{
	newModulator->setColour(chain->getColour());

	for(int i = 0; i < newModulator->getNumInternalChains(); i++)
	{
		dynamic_cast<Modulator*>(newModulator->getChildProcessor(i))->setColour(chain->getColour());
	}

	newModulator->setConstrainerForAllInternalChains(chain->getFactoryType()->getConstrainer());

	if(dynamic_cast<VoiceStartModulator*>(newModulator) != nullptr)
	{
		VoiceStartModulator *m = static_cast<VoiceStartModulator*>(newModulator);

		chain->voiceStartModulators.add(m);
		// Must be called here to get the right virtual function!
	}
	else if (dynamic_cast<EnvelopeModulator*>(newModulator) != nullptr)
	{
		EnvelopeModulator *m = static_cast<EnvelopeModulator*>(newModulator);
		chain->envelopeModulators.add(m);
		if(chain->isInitialized()) m->prepareToPlay(chain->getSampleRate(), chain->blockSize);
	}
	else if (dynamic_cast<TimeVariantModulator*>(newModulator) != nullptr)
	{
		TimeVariantModulator *m = static_cast<TimeVariantModulator*>(newModulator);
		chain->variantModulators.add(m);
		if(chain->isInitialized()) m->prepareToPlay(chain->getSampleRate(), chain->blockSize);
	}
	else jassertfalse;
		
	
	const int index = siblingToInsertBefore == nullptr ? -1 : chain->allModulators.indexOf(dynamic_cast<Modulator*>(siblingToInsertBefore));

	chain->allModulators.insert(index, newModulator);

	jassert(chain->checkModulatorStructure());

	chain->sendChangeMessage();
};


void ModulatorChain::ModulatorChainHandler::add(Processor *newProcessor, Processor *siblingToInsertBefore)
{
	jassert(dynamic_cast<Modulator*>(newProcessor) != nullptr);
	addModulator(dynamic_cast<Modulator*>(newProcessor), siblingToInsertBefore);

	const bool isPitchChain = chain->getMode() == Modulation::PitchMode;
	if (isPitchChain)
	{
		ModulatorSynth *p = dynamic_cast<ModulatorSynth*>(chain->getParentProcessor());

		if(p != nullptr) p->enablePitchModulation(true);
	}

	sendChangeMessage();
}

void ModulatorChain::ModulatorChainHandler::deleteModulator(Modulator *modulatorToBeDeleted)
{
	for(int i = 0; i < getNumModulators(); ++i)
	{
		if(chain->allModulators[i] == modulatorToBeDeleted) chain->allModulators.remove(i);
	};
		
	for(int i = 0; i < chain->variantModulators.size(); ++i)
	{
		if(chain->variantModulators[i] == modulatorToBeDeleted) chain->variantModulators.remove(i, true);
	};

	for(int i = 0; i < chain->envelopeModulators.size(); ++i)
	{
		if(chain->envelopeModulators[i] == modulatorToBeDeleted) chain->envelopeModulators.remove(i, true);
	};

	for(int i = 0; i < chain->voiceStartModulators.size(); ++i) 
	{
		if(chain->voiceStartModulators[i] == modulatorToBeDeleted) chain->voiceStartModulators.remove(i, true);
	};

	jassert(chain->checkModulatorStructure());
	chain->sendChangeMessage();
};


void ModulatorChain::ModulatorChainHandler::remove(Processor *processorToBeRemoved)
{
	jassert(dynamic_cast<Modulator*>(processorToBeRemoved) != nullptr);
	deleteModulator(dynamic_cast<Modulator*>(processorToBeRemoved));
    
	const bool isPitchChainOfNonGroup = chain->getMode() == Modulation::PitchMode;
	if (isPitchChainOfNonGroup && getNumModulators() == 0)
	{
		dynamic_cast<ModulatorSynth*>(chain->getParentProcessor())->enablePitchModulation(false);
	}

	sendChangeMessage();
}

bool ModulatorChain::isPlaying(int voiceIndex) const
{
	jassert(getMode() == GainMode);

	if(isBypassed()) return false;

	if(envelopeModulators.size() == 0)
	{
		return activeVoices[voiceIndex];
	}

	bool anyActiveEnvelopes = false;

	for (int i = 0; i < envelopeModulators.size(); i++)
	{
		if (!envelopeModulators[i]->isBypassed())
		{
			anyActiveEnvelopes = true;
			break;
		}
	}

	if (!anyActiveEnvelopes)
	{
		return activeVoices[voiceIndex];
	}

	for(int i = 0; i < envelopeModulators.size(); ++i) // only search envelope modulators for playing status!
	{
		if (! envelopeModulators[i]->isBypassed() && !envelopeModulators[i]->isPlaying(voiceIndex)) return false;
	}

	return true;

};

ProcessorEditorBody *ModulatorChain::createEditor(BetterProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
};

void ModulatorChain::renderVoice(int voiceIndex, int startSample, int numSamples)
{
    ADD_GLITCH_DETECTOR("Rendering " + getId() + " voices for " + parentProcessor->getId());
    
	// Use the internal buffer from timeModulation as working buffer.

	initializeBuffer(internalBuffer, startSample, numSamples);


	const int startIndex = startSample;
	const int sampleAmount = numSamples;

	if( shouldBeProcessed(true))
	{

		const float constantVoiceValue = getConstantVoiceValue(voiceIndex);

		FloatVectorOperations::multiply(internalBuffer.getWritePointer(0, startSample), constantVoiceValue, numSamples);

		for(int i = 0; i < envelopeModulators.size(); i++)
		{
			EnvelopeModulator *m = envelopeModulators[i];
		
			if(m->isBypassed() ) continue;

			m->polyManager.setCurrentVoice(voiceIndex);

			FloatVectorOperations::fill(envelopeTempBuffer.getWritePointer(0, startSample), 1.0f, numSamples);

			m->renderNextBlock(envelopeTempBuffer, startSample, numSamples);

			m->saveValuesForGlobalModulator(envelopeTempBuffer, startSample, numSamples, voiceIndex);

			FloatVectorOperations::multiply(internalBuffer.getWritePointer(0, startSample), envelopeTempBuffer.getReadPointer(0, startSample), numSamples);

			m->polyManager.clearCurrentVoice();
		}

	}

	// Copy the result to the voice buffer
	FloatVectorOperations::copy(internalVoiceBuffer.getWritePointer(voiceIndex, startIndex), internalBuffer.getReadPointer(0, startIndex), sampleAmount);

	if(USE_HI_DEBUG_TOOLS && voiceIndex == polyManager.getLastStartedVoice())
	{
		saveEnvelopeValueForPlotter(internalBuffer, startIndex, sampleAmount);
		
	}
	
}

void ModulatorChain::renderNextBlock(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
    ADD_GLITCH_DETECTOR("Rendering time varian modulators for " + parentProcessor->getId());
    
	jassert (getSampleRate() > 0);

	initializeBuffer(internalBuffer, startSample, numSamples);

	const int startIndex = startSample;
	const int sampleAmount = numSamples;

	if(shouldBeProcessed(false))
	{
		for(int i = 0; i < variantModulators.size(); i++)
		{
			if(variantModulators[i]->isBypassed()) continue;
			variantModulators[i]->renderNextBlock(internalBuffer, startSample, numSamples);
		}
	}
	
#if USE_HI_DEBUG_TOOLS

	updatePlotter(internalBuffer, startSample, numSamples);
	//setOutputValue(internalBuffer.getReadPointer(0, startIndex)[0] * getVoiceValues(polyManager.getLastStartedVoice())[0]);

#endif

	FloatVectorOperations::copy(buffer.getWritePointer(0, startIndex), internalBuffer.getReadPointer(0, startIndex), sampleAmount);
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
	ADD_NAME_TO_TYPELIST(PluginParameterModulator);
	ADD_NAME_TO_TYPELIST(GlobalTimeVariantModulator);
	ADD_NAME_TO_TYPELIST(GainMatcherTimeVariantModulator);
	ADD_NAME_TO_TYPELIST(CCDucker);
	
}

void VoiceStartModulatorFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(ConstantModulator);
	ADD_NAME_TO_TYPELIST(VelocityModulator);
	ADD_NAME_TO_TYPELIST(KeyModulator);
	ADD_NAME_TO_TYPELIST(RandomModulator);
	ADD_NAME_TO_TYPELIST(GlobalVoiceStartModulator);
	ADD_NAME_TO_TYPELIST(GainMatcherVoiceStartModulator);
	ADD_NAME_TO_TYPELIST(ArrayModulator);
}

void EnvelopeModulatorFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(SimpleEnvelope);
	ADD_NAME_TO_TYPELIST(AhdsrEnvelope);
	ADD_NAME_TO_TYPELIST(TableEnvelope);
	ADD_NAME_TO_TYPELIST(GlobalEnvelopeModulator);
	ADD_NAME_TO_TYPELIST(CCEnvelope);
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
