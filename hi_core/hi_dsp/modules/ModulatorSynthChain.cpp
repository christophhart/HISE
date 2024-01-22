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

ModulatorSynthChain::ModulatorSynthChain(MainController *mc, const String &id, int numVoices_) :
	MacroControlBroadcaster(this),
	ModulatorSynth(mc, id, numVoices_),
	numVoices(numVoices_),
	handler(this),
	vuValue(0.0f)
{
	finaliseModChains();

	FactoryType *t = new ModulatorSynthChainFactoryType(numVoices, this);

	getMatrix().setAllowResizing(true);

	setGain(1.0);

	editorStateIdentifiers.add("InterfaceShown");

	setFactoryType(t);

	setEditorState(Processor::EditorState::BodyShown, false);

	// Skip the pitch chain
	pitchChain->setBypassed(true);

	//gainChain->getFactoryType()->setConstrainer(new NoMidiInputConstrainer());

	constrainer = new NoMidiInputConstrainer();

	gainChain->getFactoryType()->setConstrainer(constrainer, false);

	effectChain->getFactoryType()->setConstrainer(constrainer, false);
	effectChain->setForceMonophonicProcessingOfPolyphonicEffects(true);

	updateParameterSlots();

	disableChain(PitchModulation, true);
}

ModulatorSynthChain::~ModulatorSynthChain()
{
	getHandler()->clear();

	modChains.clear();

	effectChain = nullptr;
	midiProcessorChain = nullptr;
	gainChain = nullptr;
	pitchChain = nullptr;

	constrainer = nullptr;
}


ProcessorEditorBody *ModulatorSynthChain::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

Chain::Handler* ModulatorSynthChain::getHandler()
{ return &handler; }

const Chain::Handler* ModulatorSynthChain::getHandler() const
{return &handler;}

FactoryType* ModulatorSynthChain::getFactoryType() const
{return modulatorSynthFactory;}

void ModulatorSynthChain::setFactoryType(FactoryType* newFactoryType)
{modulatorSynthFactory = newFactoryType;}

int ModulatorSynthChain::getNumChildProcessors() const
{ return ModulatorSynth::getNumChildProcessors() + handler.getNumProcessors(); }

Processor* ModulatorSynthChain::getParentProcessor()
{return nullptr;}

const Processor* ModulatorSynthChain::getParentProcessor() const
{return nullptr;}

void ModulatorSynthChain::setPackageName(const String& newPackageName)
{ packageName = newPackageName; }

String ModulatorSynthChain::getPackageName() const
{ return packageName; }

int ModulatorSynthChain::getVoiceAmount() const
{return numVoices;}

ModulatorSynthChain::ModulatorSynthChainHandler::ModulatorSynthChainHandler(ModulatorSynthChain* synthToHandle):
	synth(synthToHandle)
{

}

void ModulatorSynthChain::setActiveChannels(const HiseEvent::ChannelFilterData& newActiveChannels)
{
	activeChannels = newActiveChannels;
}

HiseEvent::ChannelFilterData* ModulatorSynthChain::getActiveChannelData()
{ return &activeChannels; }

bool ModulatorSynthChain::isUniformVoiceHandlerRoot() const
{ return ownedUniformVoiceHandler != nullptr; };


Processor * ModulatorSynthChain::getChildProcessor(int processorIndex)
{

	if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
	else													return handler.getProcessor(processorIndex - numInternalChains);
}


const Processor * ModulatorSynthChain::getChildProcessor(int processorIndex) const
{

	if (processorIndex < ModulatorSynth::numInternalChains) return ModulatorSynth::getChildProcessor(processorIndex);
	else													return handler.getProcessor(processorIndex - numInternalChains);
}

void ModulatorSynthChain::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	for (auto s: synths)
		s->prepareToPlay(newSampleRate, samplesPerBlock);

	if (ownedUniformVoiceHandler != nullptr)
        ownedUniformVoiceHandler->rebuildChildSynthList();
}

void ModulatorSynthChain::numSourceChannelsChanged()
{
    auto mc = getMainController();
    
    if(mc->getMainSynthChain() == this)
        mc->updateMultiChannelBuffer(getMatrix().getNumSourceChannels());


	for (int i = 0; i < getHandler()->getNumProcessors(); i++)
	{
		RoutableProcessor *rp = dynamic_cast<RoutableProcessor*>(getHandler()->getProcessor(i));

		jassert(rp != nullptr);

		rp->getMatrix().setNumDestinationChannels(getMatrix().getNumSourceChannels());
	}

	ModulatorSynth::numSourceChannelsChanged();

	
}

void ModulatorSynthChain::numDestinationChannelsChanged()
{
	
}


ValueTree ModulatorSynthChain::exportAsValueTree() const
{
	ValueTree v = ModulatorSynth::exportAsValueTree();

	if (this == getMainController()->getMainSynthChain())
	{
		v.setProperty("packageName", packageName, nullptr);

		MacroControlBroadcaster::saveMacrosToValueTree(v);

		v.addChild(getMainController()->getMacroManager().getMidiControlAutomationHandler()->exportAsValueTree(), -1, nullptr);

		v.addChild(getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().exportAsValueTree(), -1, nullptr);
	}
	return v;
}

void ModulatorSynthChain::addProcessorsWhenEmpty()
{
	
}

void ModulatorSynthChain::compileAllScripts()
{
	if (getMainController()->isCompilingAllScriptsOnPresetLoad())
	{
		auto scriptProcessors = ProcessorHelpers::getListOfAllProcessors<JavascriptProcessor>(this);

		for (auto& sp : scriptProcessors)
		{
			auto c = sp->getContent();

			ValueTreeUpdateWatcher::ScopedDelayer sd(c->getUpdateWatcher());
			sp->getContent()->resetContentProperties();
			sp->compileScript();
		}
	}
}

void ModulatorSynthChain::renderNextBlockWithModulators(AudioSampleBuffer &buffer, const HiseEventBuffer &inputMidiBuffer)
{
	jassert(isOnAir());

	if (isSoftBypassed()) return;

	ADD_GLITCH_DETECTOR(this, DebugLogger::Location::SynthChainRendering);

    auto isRoot = getMainController()->getMainSynthChain() == this;
    
	if (isRoot && !activeChannels.areAllChannelsEnabled())
	{
		HiseEventBuffer::Iterator it(inputMidiBuffer);

		while (HiseEvent* e = it.getNextEventPointer())
		{
			const int channelIndex = e->getChannel() - 1;
			const bool ignoreThisEvent = !activeChannels.isChannelEnabled(channelIndex);

			if (ignoreThisEvent)
				e->ignoreEvent(true);
		}
	}

	const int numSamples = buffer.getNumSamples();

	jassert(numSamples <= buffer.getNumSamples());

	initRenderCallback();

#if FRONTEND_IS_PLUGIN

#if PROCESS_SOUND_GENERATORS_IN_FX_PLUGIN
	
	internalBuffer.setSize(getMatrix().getNumSourceChannels(), numSamples, true, false, true);

	for (int i = 0; i < synths.size(); i++)
	{
		if (!synths[i]->isSoftBypassed())
			synths[i]->renderNextBlockWithModulators(internalBuffer, eventBuffer);
	}
#endif

	effectChain->renderNextBlock(buffer, 0, numSamples);
	effectChain->renderMasterEffects(buffer);

	eventBuffer.clear();

	processHiseEventBuffer(inputMidiBuffer, numSamples);

	HiseEventBuffer::Iterator eventIterator(eventBuffer);

	while (auto e = eventIterator.getNextConstEventPointer(true, false))
	{
		effectChain->handleHiseEvent(*e);
	}

#else

	processHiseEventBuffer(inputMidiBuffer, numSamples);

	if (ownedUniformVoiceHandler != nullptr)
        ownedUniformVoiceHandler->processEventBuffer(inputMidiBuffer);
		

	// Shrink the internal buffer to the output buffer size 
	internalBuffer.setSize(getMatrix().getNumSourceChannels(), numSamples, true, false, true);

#if FORCE_INPUT_CHANNELS
    if(isRoot)
    {
        for(int i = 0; i < buffer.getNumChannels(); i++)
        {
            FloatVectorOperations::copy(internalBuffer.getWritePointer(i),
                                        buffer.getReadPointer(i), numSamples);
        }

		// now clear the buffer
		buffer.clear();
    }
#endif

	ScopedAnalyser sa(getMainController(), this, internalBuffer, buffer.getNumSamples());

	// Process the Synths and add store their output in the internal buffer
	for (int i = 0; i < synths.size(); i++)
    {
		ScopedAnalyser sa(getMainController(), synths[i], internalBuffer, internalBuffer.getNumSamples());

        if (!synths[i]->isSoftBypassed())
            synths[i]->renderNextBlockWithModulators(internalBuffer, eventBuffer);
    }

	HiseEventBuffer::Iterator eventIterator(eventBuffer);

	while (auto e = eventIterator.getNextConstEventPointer(true, false))
	{
		if (!(e->isController() || e->isPitchWheel()))
		{
			continue;
		}

		handleHiseEvent(*e);
	}

	modChains[GainModulation-1].calculateMonophonicModulationValues(0, numSamples);

	postVoiceRendering(0, numSamples);

	effectChain->renderMasterEffects(internalBuffer);

	if (internalBuffer.getNumChannels() != 2 || 
		getMatrix().getConnectionForSourceChannel(0) != 0 ||
		getMatrix().getConnectionForSourceChannel(1) != 1)
	{
		jassert(internalBuffer.getNumChannels() == getMatrix().getNumSourceChannels());

		for (int i = 0; i < internalBuffer.getNumChannels(); i++)
		{
			const int sourceIndex = i;
			const int destinationIndex = getMatrix().getConnectionForSourceChannel(i);

			if (destinationIndex >= 0 && destinationIndex < buffer.getNumChannels())
			{
				FloatVectorOperations::addWithMultiply(buffer.getWritePointer(destinationIndex, 0), internalBuffer.getReadPointer(sourceIndex, 0), getGain() * getBalance(i % 2 != 0), numSamples);
			}
		}
	}
	else // save some cycles on non multichannel buffers...
	{
		FloatVectorOperations::addWithMultiply(buffer.getWritePointer(0, 0), internalBuffer.getReadPointer(0, 0), getGain() * getBalance(false), numSamples);
		FloatVectorOperations::addWithMultiply(buffer.getWritePointer(1, 0), internalBuffer.getReadPointer(1, 0), getGain() * getBalance(true), numSamples);
	}

	getMatrix().handleDisplayValues(internalBuffer, buffer, true);

	// Display the output
	handlePeakDisplay(numSamples);

#endif

	if (ownedUniformVoiceHandler != nullptr)
        ownedUniformVoiceHandler->cleanupAfterProcessing();
}


void ModulatorSynthChain::restoreFromValueTree(const ValueTree &v)
{
	packageName = v.getProperty("packageName", "");

	ModulatorSynth::restoreFromValueTree(v);

	if (!getMainController()->shouldSkipCompiling())
	{
		ValueTree autoData = v.getChildWithName("MidiAutomation");

		if (autoData.isValid())
			getMainController()->getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(autoData);
	}

	ValueTree mpeData = v.getChildWithName("MPEData");

	if (mpeData.isValid())
		getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().restoreFromValueTree(mpeData);
	else
		getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().reset();
}

void ModulatorSynthChain::reset()
{

	Processor::Iterator<Processor> iter(this, false);

#if 0
	{
		sendDeleteMessage();

		while (auto p = iter.getNextProcessor())
			p->sendDeleteMessage();
	}
#endif
	
    midiProcessorChain->getHandler()->clearAsync(midiProcessorChain);
    gainChain->getHandler()->clearAsync(gainChain);
    effectChain->getHandler()->clearAsync(effectChain);
	this->getHandler()->clearAsync(nullptr);

    getMatrix().resetToDefault();
    getMatrix().setNumSourceChannels(2);

	setIconColour(Colours::transparentBlack);

    setUseUniformVoiceHandler(false, nullptr);

#if USE_BACKEND
	setId("Master Chain");
#endif


	for (int i = 0; i < getNumInternalChains(); i++)
	{
		getChildProcessor(i)->setEditorState(getEditorStateForIndex(Processor::Visible), false, sendNotification);
	}

	for (int i = 0; i < ModulatorSynth::numModulatorSynthParameters; i++)
	{
		setAttribute(i, getDefaultValue(i), dontSendNotification);
	}
    
    clearAllMacroControls();
    
    
    for(int i = 0; i < parameterNames.size(); i++)
    {
        setAttribute(i, getDefaultValue(i), dontSendNotification);
    }
    
    sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Preset);
}

int ModulatorSynthChain::getNumActiveVoices() const
{
	int totalVoices = 0;

	for (auto synth : synths)
		totalVoices += synth->getNumActiveVoices();

	return totalVoices;
}

void ModulatorSynthChain::killAllVoices()
{
	for (auto synth : synths)
		synth->killAllVoices();

	effectChain->killMasterEffects();
}

void ModulatorSynthChain::resetAllVoices()
{
	for (auto synth : synths)
		synth->resetAllVoices();

	effectChain->resetMasterEffects();
}

bool ModulatorSynthChain::areVoicesActive() const
{
	if (isSoftBypassed())
		return false;

	for (auto synth : synths)
	{
		if (synth->areVoicesActive())
			return true;
	}
		
	return effectChain->hasTailingMasterEffects();
}


void ModulatorSynthChain::saveInterfaceValues(ValueTree &v)
{
	ValueTree interfaceData("InterfaceData");

	for (int i = 0; i < midiProcessorChain->getNumChildProcessors(); i++)
	{
		JavascriptMidiProcessor *sp = dynamic_cast<JavascriptMidiProcessor*>(midiProcessorChain->getChildProcessor(i));

		if (sp != nullptr && sp->isFront())
		{
			ValueTree spv = sp->getScriptingContent()->exportAsValueTree();

			spv.setProperty("Processor", sp->getId(), nullptr);

			interfaceData.addChild(spv, -1, nullptr);
		}
	}

	v.addChild(interfaceData, -1, nullptr);
}

void ModulatorSynthChain::restoreInterfaceValues(const ValueTree &v)
{
	for (int i = 0; i < midiProcessorChain->getNumChildProcessors(); i++)
	{
		JavascriptMidiProcessor *sp = dynamic_cast<JavascriptMidiProcessor*>(midiProcessorChain->getChildProcessor(i));

		if (sp != nullptr && sp->isFront())
		{
			for (int j = 0; j < v.getNumChildren(); j++)
			{
				const ValueTree child = v.getChild(j);

				if (child.getProperty("Processor") == sp->getId())
				{
					ScriptingApi::Content *content = sp->getScriptingContent();

					content->restoreAllControlsFromPreset(child);

					break;
				}

			}
		}
	}
}

void ModulatorSynthChain::setUseUniformVoiceHandler(bool shouldUseVoiceHandler, UniformVoiceHandler* externalVoiceHandler)
{
    if(externalVoiceHandler == nullptr)
    {
        ScopedPointer<UniformVoiceHandler> newHandler;

        if(shouldUseVoiceHandler)
            newHandler = new UniformVoiceHandler(this);

        {
            LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::AudioLock);
            newHandler.swapWith(ownedUniformVoiceHandler);
        }

        externalVoiceHandler = ownedUniformVoiceHandler.get();
        getMainController()->allNotesOff();
    }
    else
    {
        if(shouldUseVoiceHandler && ownedUniformVoiceHandler != nullptr)
        {
            debugError(this, "Can't use more than one uniform voice handler!");
        }
    }
    
    ModulatorSynth::setUseUniformVoiceHandler(shouldUseVoiceHandler, externalVoiceHandler);
    
    for(int i = 0; i < getHandler()->getNumProcessors(); i++)
    {
        auto cs = dynamic_cast<ModulatorSynth*>(getHandler()->getProcessor(i));
        cs->setUseUniformVoiceHandler(shouldUseVoiceHandler, externalVoiceHandler);
    }

#if USE_OLD_PROCESSOR_DISPATCH
    getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(this, MainController::ProcessorChangeHandler::EventType::ProcessorColourChange, false);
#endif
#if USE_NEW_PROCESSOR_DISPATCH
	dispatcher.setColour(Colours::black);
#endif
}


bool ModulatorSynthChain::hasDefinedFrontInterface() const
{   
    for (int i = 0; i < midiProcessorChain->getNumChildProcessors(); i++)
    {
        if (JavascriptMidiProcessor *sp = dynamic_cast<JavascriptMidiProcessor*>(midiProcessorChain->getChildProcessor(i)))
        {
            if (sp->isFront())
            {
                return true;
            }
        }
    }
    
    return false;
}


NoMidiInputConstrainer::NoMidiInputConstrainer()
{
	Array<FactoryType::ProcessorEntry> typeNames;

    ADD_NAME_TO_TYPELIST(HarmonicFilter);
    ADD_NAME_TO_TYPELIST(StereoEffect);
    ADD_NAME_TO_TYPELIST(PolyshapeFX);

	forbiddenModulators.addArray(typeNames);

	EnvelopeModulatorFactoryType envelopes(0, Modulation::Mode::GainMode, nullptr);

	forbiddenModulators.addArray(envelopes.getAllowedTypes());

	VoiceStartModulatorFactoryType voiceStart(0, Modulation::Mode::GainMode, nullptr);

	forbiddenModulators.addArray(voiceStart.getAllowedTypes());
}

String NoMidiInputConstrainer::getDescription() const
{ return "No voice modulators"; }

bool NoMidiInputConstrainer::allowType(const Identifier& typeName)
{
	for(int i = 0; i < forbiddenModulators.size(); i++)
	{
		if(forbiddenModulators[i].type == typeName) return false;
	}

	return true;
}

SynthGroupFXConstrainer::SynthGroupFXConstrainer() = default;

String SynthGroupFXConstrainer::getDescription() const
{ return "Only Polyphonic FX"; }

bool SynthGroupFXConstrainer::allowType(const Identifier& typeName)
{
	auto isPoly = typeName.toString().toLowerCase().contains("poly");
        
	// just trying to get through the day...
	isPoly |= (typeName == Identifier("StereoFX"));
        
	return isPoly;
}

SynthGroupConstrainer::SynthGroupConstrainer()
{
	Array<FactoryType::ProcessorEntry> typeNames;

	ADD_NAME_TO_TYPELIST(ModulatorSynthChain);
	ADD_NAME_TO_TYPELIST(GlobalModulatorContainer);
	ADD_NAME_TO_TYPELIST(ModulatorSynthGroup);

	forbiddenModulators.addArray(typeNames);
}

String SynthGroupConstrainer::getDescription() const
{ return "No container modules"; }

bool SynthGroupConstrainer::allowType(const Identifier& typeName)
{
	for (int i = 0; i < forbiddenModulators.size(); i++)
	{
		if (forbiddenModulators[i].type == typeName) return false;
	}

	return true;
}

void ModulatorSynthChain::ModulatorSynthChainHandler::add(Processor *newProcessor, Processor *siblingToInsertBefore)
{
	ModulatorSynth *ms = dynamic_cast<ModulatorSynth*>(newProcessor);

	jassert(ms != nullptr);

	const int index = siblingToInsertBefore == nullptr ? -1 : synth->synths.indexOf(dynamic_cast<ModulatorSynth*>(siblingToInsertBefore));

	ms->getMatrix().setNumDestinationChannels(synth->getMatrix().getNumSourceChannels());
	ms->getMatrix().setTargetProcessor(synth);

	auto bs = synth->getLargestBlockSize();

	if (bs > 0)
	{
		ms->prepareToPlay(synth->getSampleRate(), synth->getLargestBlockSize());
	}

	ms->setParentProcessor(synth);

	{
		LOCK_PROCESSING_CHAIN(synth);
		ms->setIsOnAir(synth->isOnAir());
		synth->synths.insert(index, ms);
	}

	notifyListeners(Listener::ProcessorAdded, newProcessor);
}

void ModulatorSynthChain::ModulatorSynthChainHandler::remove(Processor *processorToBeRemoved, bool removeSynth)
{
	notifyListeners(Listener::ProcessorDeleted, processorToBeRemoved);

	ScopedPointer<Processor> removedP = processorToBeRemoved;

	{
		LOCK_PROCESSING_CHAIN(synth);
		processorToBeRemoved->setIsOnAir(false);
		synth->synths.removeObject(dynamic_cast<ModulatorSynth*>(processorToBeRemoved), false);
	}

	if (removeSynth)
		removedP = nullptr;
	else
		removedP.release();

}

Processor * ModulatorSynthChain::ModulatorSynthChainHandler::getProcessor(int processorIndex)
{
	return synth->synths[processorIndex];
}

const Processor * ModulatorSynthChain::ModulatorSynthChainHandler::getProcessor(int processorIndex) const
{
	return synth->synths[processorIndex];
}

int ModulatorSynthChain::ModulatorSynthChainHandler::getNumProcessors() const
{
	return synth->synths.size();
}

void ModulatorSynthChain::ModulatorSynthChainHandler::clear()
{
	notifyListeners(Listener::Cleared, nullptr);

	auto root = synth->getMainController()->getMainSynthChain();

	if(root == nullptr || root == synth)
	{
		ScopedLock sl(synth->getMainController()->getLock());
		synth->synths.clear();
	}
	else
	{
		clearAsync(synth);
	}
}



} // namespace hise
