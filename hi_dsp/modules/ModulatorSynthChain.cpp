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

ModulatorSynthChain::ModulatorSynthChain(MainController *mc, const String &id, int numVoices_, UndoManager *viewUndoManager /*= nullptr*/) :
	MacroControlBroadcaster(this),
	ModulatorSynth(mc, id, numVoices_),
#if USE_BACKEND
	ViewManager(this, viewUndoManager),
#endif
	numVoices(numVoices_),
	handler(this),
	vuValue(0.0f)
{
#if USE_BACKEND == 0
	ignoreUnused(viewUndoManager);
#endif

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

	disableChain(PitchModulation, true);
}

ModulatorSynthChain::~ModulatorSynthChain()
{
	getHandler()->clear();

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
};


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

	for (int i = 0; i < synths.size(); i++) synths[i]->prepareToPlay(newSampleRate, samplesPerBlock);
}

void ModulatorSynthChain::numSourceChannelsChanged()
{
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



#if USE_BACKEND
		ViewManager::saveViewsToValueTree(v);
#endif
		MacroControlBroadcaster::saveMacrosToValueTree(v);

		v.addChild(getMainController()->getMacroManager().getMidiControlAutomationHandler()->exportAsValueTree(), -1, nullptr);

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
		Processor::Iterator<JavascriptProcessor> it(this);

		JavascriptProcessor *sp;

		while ((sp = it.getNextProcessor()) != 0)
		{
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

	ScopedLock sl(getSynthLock());

	

	if (getMainController()->getMainSynthChain() == this && !activeChannels.areAllChannelsEnabled())
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

	const int numSamples = getBlockSize();//buffer.getNumSamples();

	jassert(numSamples <= buffer.getNumSamples());

	initRenderCallback();

#if FRONTEND_IS_PLUGIN

	effectChain->renderMasterEffects(buffer);

#else

	processHiseEventBuffer(inputMidiBuffer, numSamples);

	// Shrink the internal buffer to the output buffer size 
	internalBuffer.setSize(getMatrix().getNumSourceChannels(), numSamples, true, false, true);

	// Process the Synths and add store their output in the internal buffer
	for (int i = 0; i < synths.size(); i++) if (!synths[i]->isSoftBypassed()) synths[i]->renderNextBlockWithModulators(internalBuffer, eventBuffer);

	HiseEventBuffer::Iterator eventIterator(eventBuffer);

	while (auto e = eventIterator.getNextConstEventPointer(true, false))
	{
		if (!(e->isController() || e->isPitchWheel()))
		{
			continue;
		}

		handleHiseEvent(*e);
	}

	postVoiceRendering(0, numSamples);

	effectChain->renderMasterEffects(internalBuffer);

	if (internalBuffer.getNumChannels() != 2)
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

		if (getMatrix().isEditorShown())
		{
			float gainValues[NUM_MAX_CHANNELS];

			for (int i = 0; i < internalBuffer.getNumChannels(); i++)
			{
				gainValues[i] = FloatVectorOperations::findMaximum(internalBuffer.getReadPointer(i), numSamples);
			}

			getMatrix().setGainValues(gainValues, true);

			for (int i = 0; i < buffer.getNumChannels(); i++)
			{
				gainValues[i] = FloatVectorOperations::findMaximum(buffer.getReadPointer(i), numSamples);
			}

			getMatrix().setGainValues(gainValues, false);
		}
	}
	else // save some cycles on non multichannel buffers...
	{
		FloatVectorOperations::addWithMultiply(buffer.getWritePointer(0, 0), internalBuffer.getReadPointer(0, 0), getGain() * getBalance(false), numSamples);
		FloatVectorOperations::addWithMultiply(buffer.getWritePointer(1, 0), internalBuffer.getReadPointer(1, 0), getGain() * getBalance(true), numSamples);
	}

	// Display the output
	handlePeakDisplay(numSamples);

#endif
}


void ModulatorSynthChain::restoreFromValueTree(const ValueTree &v)
{
	packageName = v.getProperty("packageName", "");

	ModulatorSynth::restoreFromValueTree(v);


#if USE_BACKEND
	ViewManager::restoreViewsFromValueTree(v);
#endif

	ValueTree autoData = v.getChildWithName("MidiAutomation");

	if (autoData.isValid())
	{
		getMainController()->getMacroManager().getMidiControlAutomationHandler()->restoreFromValueTree(autoData);
	}
}

void ModulatorSynthChain::reset()
{

	{
		MessageManagerLock mm;

		

		sendDeleteMessage();

		Processor::Iterator<Processor> iter(this, false);

		while (auto p = iter.getNextProcessor())
		{
			p->sendDeleteMessage();
		}
	}
	

    this->getHandler()->clear();
    
    midiProcessorChain->getHandler()->clear();
    gainChain->getHandler()->clear();
    effectChain->getHandler()->clear();
    getMatrix().resetToDefault();
    getMatrix().setNumSourceChannels(2);

	setIconColour(Colours::transparentBlack);

	

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

#if USE_BACKEND
    clearAllViews();
#endif
    
    clearAllMacroControls();
    
    
    for(int i = 0; i < parameterNames.size(); i++)
    {
        setAttribute(i, getDefaultValue(i), dontSendNotification);
    }
    
    sendChangeMessage();
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
}

void ModulatorSynthChain::resetAllVoices()
{
	for (auto synth : synths)
		synth->resetAllVoices();
}

bool ModulatorSynthChain::areVoicesActive() const
{
	for (auto synth : synths)
	{
		if (synth->areVoicesActive())
			return true;
	}
		
	return false;
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

    ADD_NAME_TO_TYPELIST(PolyFilterEffect);
    ADD_NAME_TO_TYPELIST(HarmonicFilter);
    ADD_NAME_TO_TYPELIST(StereoEffect);

	forbiddenModulators.addArray(typeNames);

	EnvelopeModulatorFactoryType envelopes(0, Modulation::Mode::GainMode, nullptr);

	forbiddenModulators.addArray(envelopes.getAllowedTypes());

	VoiceStartModulatorFactoryType voiceStart(0, Modulation::Mode::GainMode, nullptr);

	forbiddenModulators.addArray(voiceStart.getAllowedTypes());
}

SynthGroupConstrainer::SynthGroupConstrainer()
{
	Array<FactoryType::ProcessorEntry> typeNames;

	ADD_NAME_TO_TYPELIST(ModulatorSynthChain);
	ADD_NAME_TO_TYPELIST(GlobalModulatorContainer);
	ADD_NAME_TO_TYPELIST(ModulatorSynthGroup);

	forbiddenModulators.addArray(typeNames);
}

void ModulatorSynthChain::ModulatorSynthChainHandler::add(Processor *newProcessor, Processor *siblingToInsertBefore)
{
	ModulatorSynth *ms = dynamic_cast<ModulatorSynth*>(newProcessor);

	jassert(ms != nullptr);

	const int index = siblingToInsertBefore == nullptr ? -1 : synth->synths.indexOf(dynamic_cast<ModulatorSynth*>(siblingToInsertBefore));

	ms->getMatrix().setNumDestinationChannels(synth->getMatrix().getNumSourceChannels());
	ms->getMatrix().setTargetProcessor(synth);

	ms->prepareToPlay(synth->getSampleRate(), synth->getBlockSize());

	{
		MainController::ScopedSuspender ss(synth->getMainController());
		ms->setIsOnAir(true);
		synth->synths.insert(index, ms);
	}

	sendChangeMessage();
}

void ModulatorSynthChain::ModulatorSynthChainHandler::remove(Processor *processorToBeRemoved, bool removeSynth)
{
	{
		auto& tmp = synth;

		auto f = [tmp, removeSynth](Processor* p) { tmp->synths.removeObject(dynamic_cast<ModulatorSynth*>(p), removeSynth); return true; };

		synth->getMainController()->getKillStateHandler().killVoicesAndCall(processorToBeRemoved, f, MainController::KillStateHandler::TargetThread::MessageThread);
		
	}

	sendChangeMessage();
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
	ScopedLock sl(synth->getMainController()->getLock());

	synth->synths.clear();

	sendChangeMessage();
}

} // namespace hise