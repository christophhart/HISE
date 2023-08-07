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

EffectProcessorChain::EffectProcessorChain(Processor *parentProcessor_, const String &id, int numVoices) :
		Processor(parentProcessor_->getMainController(), id, numVoices),
		handler(this),
		parentProcessor(parentProcessor_),
		killBuffer(2, 0)
{
	effectChainFactory = new EffectProcessorChainFactoryType(numVoices, parentProcessor);

	setEditorState(Processor::Visible, false, dontSendNotification);
}

EffectProcessorChain::~EffectProcessorChain()
{
	getHandler()->clearAsync(this);
}

Chain::Handler* EffectProcessorChain::getHandler()
{return &handler;}

const Chain::Handler* EffectProcessorChain::getHandler() const
{return &handler;}

Processor* EffectProcessorChain::getParentProcessor()
{ return parentProcessor; }

const Processor* EffectProcessorChain::getParentProcessor() const
{ return parentProcessor; }

FactoryType* EffectProcessorChain::getFactoryType() const
{return effectChainFactory;}

Colour EffectProcessorChain::getColour() const
{ return Colour(0xff3a6666);}

void EffectProcessorChain::setFactoryType(FactoryType* newFactoryType)
{effectChainFactory = newFactoryType;}

float EffectProcessorChain::getAttribute(int) const
{return 1.0f;	}

void EffectProcessorChain::setInternalAttribute(int, float)
{}

void EffectProcessorChain::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// Skip the effectProcessor's prepareToPlay since it assumes all child processors are ModulatorChains
	Processor::prepareToPlay(sampleRate, samplesPerBlock);

	for (auto fx : allEffects)
		fx->prepareToPlay(sampleRate, samplesPerBlock);

	ProcessorHelpers::increaseBufferIfNeeded(killBuffer, samplesPerBlock);

	resetCounterStartValue = (int)(0.12 * sampleRate);

	for (auto fx : masterEffects)
		fx->setKillBuffer(killBuffer);
}

void EffectProcessorChain::handleHiseEvent(const HiseEvent& m)
{	
	if(isBypassed()) return;
	FOR_ALL_EFFECTS(handleHiseEvent(m)); 
}

void EffectProcessorChain::renderVoice(int voiceIndex, AudioSampleBuffer& b, int startSample, int numSamples)
{ 
	if(isBypassed()) return;

	ADD_GLITCH_DETECTOR(parentProcessor, DebugLogger::Location::VoiceEffectRendering);
        
	FOR_EACH_VOICE_EFFECT(renderVoice(voiceIndex, b, startSample, numSamples)); 
}

void EffectProcessorChain::preRenderCallback(int startSample, int numSamples)
{
	FOR_EACH_VOICE_EFFECT(preRenderCallback(startSample, numSamples));
}

void EffectProcessorChain::resetMasterEffects()
{
	updateSoftBypassState();

	for (auto fx : masterEffects)
	{
		if(fx->hasTail())
			fx->voicesKilled();
	}

	resetCounter = -1;
}

void EffectProcessorChain::renderNextBlock(AudioSampleBuffer& buffer, int startSample, int numSamples)
{
	if(isBypassed()) return;

	if (renderPolyFxAsMono)
	{
		// Make sure the modulation values get calculated...
		FOR_EACH_VOICE_EFFECT(preRenderCallback(startSample, numSamples));
	}

	FOR_ALL_EFFECTS(renderNextBlock(buffer, startSample, numSamples));

}

bool EffectProcessorChain::hasTailingMasterEffects() const noexcept
{
	return resetCounter > 0;
}

bool EffectProcessorChain::hasTailingPolyEffects() const
{
	for (int i = 0; i < voiceEffects.size(); i++)
	{
		if (voiceEffects[i]->isBypassed())
			continue;

		if (!voiceEffects[i]->hasTail())
			continue;

		if (voiceEffects[i]->isTailingOff())
			return true;
	}

	return false;
}

void EffectProcessorChain::killMasterEffects()
{
	if (hasTailingMasterEffects())
		return;

	if (isBypassed())
	{
		resetCounter = -1;
		return;
	}

	bool hasActiveMasterFX = false;

	for (auto& fx : masterEffects)
	{
		if (!fx->hasTail())
			continue;

		if (!fx->isBypassed())
		{
			hasActiveMasterFX = true;
			break;
		}
	}
		
	if (!hasActiveMasterFX)
		return;

	ScopedLock sl(getMainController()->getLock());

	for (auto fx : masterEffects)
	{
		if (fx->isBypassed())
			continue;

		fx->setSoftBypass(true, true);
	}
			

	resetCounter = resetCounterStartValue;
}

void EffectProcessorChain::updateSoftBypassState()
{
	for (auto fx : masterEffects)
		fx->updateSoftBypass();

	resetCounter = -1;
}

void EffectProcessorChain::startVoice(int voiceIndex, const HiseEvent& e)
{
	if(isBypassed()) return;
	FOR_EACH_VOICE_EFFECT(startVoice(voiceIndex, e)); 
	FOR_EACH_MONO_EFFECT(startMonophonicVoice(e));
	FOR_EACH_MASTER_EFFECT(startMonophonicVoice());
}

void EffectProcessorChain::stopVoice(int voiceIndex)
{
	if(isBypassed()) return;

	FOR_EACH_VOICE_EFFECT(stopVoice(voiceIndex));
	FOR_EACH_MONO_EFFECT(stopMonophonicVoice());
	FOR_EACH_MASTER_EFFECT(stopMonophonicVoice());
}

void EffectProcessorChain::reset(int voiceIndex)
{ 
	if(isBypassed()) return;
	FOR_EACH_VOICE_EFFECT(reset(voiceIndex));	
	FOR_EACH_MONO_EFFECT(resetMonophonicVoice());
	FOR_EACH_MASTER_EFFECT(resetMonophonicVoice());
}

int EffectProcessorChain::getNumChildProcessors() const
{ return getHandler()->getNumProcessors(); }

Processor* EffectProcessorChain::getChildProcessor(int processorIndex)
{ return getHandler()->getProcessor(processorIndex); }

const Processor* EffectProcessorChain::getChildProcessor(int processorIndex) const
{ return getHandler()->getProcessor(processorIndex); }

EffectProcessorChain::EffectChainHandler::EffectChainHandler(EffectProcessorChain* handledChain): chain(handledChain)
{}

EffectProcessorChain::EffectChainHandler::~EffectChainHandler()
{}

void EffectProcessorChain::setForceMonophonicProcessingOfPolyphonicEffects(bool shouldProcessPolyFX)
{
	renderPolyFxAsMono = shouldProcessPolyFX;
}

void EffectProcessorChain::renderMasterEffects(AudioSampleBuffer &b)
{
	if (isBypassed())
		return;

	ADD_GLITCH_DETECTOR(parentProcessor, DebugLogger::Location::MasterEffectRendering);

	FOR_EACH_MASTER_EFFECT(renderWholeBuffer(b));

	const auto prev = resetCounter;

	resetCounter -= (int64)b.getNumSamples();

	const bool signChange = (prev * resetCounter) < 0;

	if (signChange)
	{
#if JUCE_DEBUG
		for (auto& fx : masterEffects)
			jassert(fx->isBypassed() || !fx->isFadeOutPending());
#endif

		resetMasterEffects();
	}

#if ENABLE_ALL_PEAK_METERS
	currentValues.outL = (b.getMagnitude(0, 0, b.getNumSamples()));
	currentValues.outR = (b.getMagnitude(1, 0, b.getNumSamples()));
#endif
}

ProcessorEditorBody *EffectProcessorChain::EffectProcessorChain::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);
	
#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


void EffectProcessorChainFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(PolyFilterEffect);
	ADD_NAME_TO_TYPELIST(HarmonicFilter);
	ADD_NAME_TO_TYPELIST(HarmonicMonophonicFilter);
	ADD_NAME_TO_TYPELIST(CurveEq);
	ADD_NAME_TO_TYPELIST(StereoEffect);
	ADD_NAME_TO_TYPELIST(SimpleReverbEffect);
	ADD_NAME_TO_TYPELIST(GainEffect);
	ADD_NAME_TO_TYPELIST(ConvolutionEffect);
	ADD_NAME_TO_TYPELIST(DelayEffect);
	ADD_NAME_TO_TYPELIST(MdaLimiterEffect);
	ADD_NAME_TO_TYPELIST(MdaDegradeEffect);
	ADD_NAME_TO_TYPELIST(ChorusEffect);
    ADD_NAME_TO_TYPELIST(PhaseFX);
	ADD_NAME_TO_TYPELIST(RouteEffect);
	ADD_NAME_TO_TYPELIST(SendEffect);
	ADD_NAME_TO_TYPELIST(SaturatorEffect);
	ADD_NAME_TO_TYPELIST(JavascriptMasterEffect);
	ADD_NAME_TO_TYPELIST(JavascriptPolyphonicEffect);
	ADD_NAME_TO_TYPELIST(SlotFX);
	ADD_NAME_TO_TYPELIST(EmptyFX);
	ADD_NAME_TO_TYPELIST(DynamicsEffect);
	ADD_NAME_TO_TYPELIST(AnalyserEffect);
	ADD_NAME_TO_TYPELIST(ShapeFX);
	ADD_NAME_TO_TYPELIST(PolyshapeFX);
	ADD_NAME_TO_TYPELIST(HardcodedMasterFX);
	ADD_NAME_TO_TYPELIST(HardcodedPolyphonicFX);
	ADD_NAME_TO_TYPELIST(MidiMetronome);
	
};

Processor* EffectProcessorChainFactoryType::createProcessor	(int typeIndex, const String &id)
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case polyphonicFilter:				return new PolyFilterEffect(m, id, numVoices);
	case harmonicFilter:				return new HarmonicFilter(m, id, numVoices);
	case polyScriptFxProcessor:			return new JavascriptPolyphonicEffect(m, id, numVoices);
	case harmonicFilterMono:			return new HarmonicMonophonicFilter(m, id);
	case curveEq:						return new CurveEq(m, id);
	case stereoEffect:					return new StereoEffect(m, id, numVoices);
	case convolution:					return new ConvolutionEffect(m, id);
	case simpleReverb:					return new SimpleReverbEffect(m, id);
	case simpleGain:					return new GainEffect(m, id);
	case delay:							return new DelayEffect(m, id);
	case limiter:						return new MdaLimiterEffect(m, id);
	case degrade:						return new MdaDegradeEffect(m, id);
	case chorus:						return new ChorusEffect(m, id);
    case phaser:                        return new PhaseFX(m, id);
	case routeFX:						return new RouteEffect(m, id);
	case sendFX:						return new SendEffect(m, id);
	case saturation:					return new SaturatorEffect(m, id);
	case scriptFxProcessor:				return new JavascriptMasterEffect(m, id);
	case slotFX:						return new SlotFX(m, id);
	case emptyFX:						return new EmptyFX(m, id);
	case dynamics:						return new DynamicsEffect(m, id);
	case analyser:						return new AnalyserEffect(m, id);
	case shapeFX:						return new ShapeFX(m, id);
	case polyshapeFx:					return new PolyshapeFX(m, id, numVoices);
	case hardcodedMasterFx:				return new HardcodedMasterFX(m, id);
	case polyHardcodedFx:				return new HardcodedPolyphonicFX(m, id, numVoices);
	case midiMetronome:					return new MidiMetronome(m, id);
	default:					jassertfalse; return nullptr;
	}
};

void EffectProcessorChain::EffectChainHandler::add(Processor *newProcessor, Processor *siblingToInsertBefore)
{
	//ScopedLock sl(chain->getMainController()->getLock());

	jassert(dynamic_cast<EffectProcessor*>(newProcessor) != nullptr);

	for (int i = 0; i < newProcessor->getNumInternalChains(); i++)
	{
		dynamic_cast<ModulatorChain*>(newProcessor->getChildProcessor(i))->setColour(newProcessor->getColour());
	}

	newProcessor->setConstrainerForAllInternalChains(chain->getFactoryType()->getConstrainer());

	newProcessor->setParentProcessor(chain);

	if (chain->getSampleRate() > 0.0 && newProcessor != nullptr)
		newProcessor->prepareToPlay(chain->getSampleRate(), chain->getLargestBlockSize());
	
	

	{
		LOCK_PROCESSING_CHAIN(chain);

		newProcessor->setIsOnAir(chain->isOnAir());

		if (VoiceEffectProcessor* vep = dynamic_cast<VoiceEffectProcessor*>(newProcessor))
		{
			const int index = chain->voiceEffects.indexOf(dynamic_cast<VoiceEffectProcessor*>(siblingToInsertBefore));
			chain->voiceEffects.insert(index, vep);
			vep->setForceMonoMode(chain->renderPolyFxAsMono);

		}
		else if (MasterEffectProcessor* mep = dynamic_cast<MasterEffectProcessor*>(newProcessor))
		{
			const int index = chain->masterEffects.indexOf(dynamic_cast<MasterEffectProcessor*>(siblingToInsertBefore));
			chain->masterEffects.insert(index, mep);
			mep->setKillBuffer(chain->killBuffer);
			mep->setEventBuffer(dynamic_cast<ModulatorSynth*>(chain->getParentProcessor())->getEventBuffer());

		}
		else if (MonophonicEffectProcessor* moep = dynamic_cast<MonophonicEffectProcessor*>(newProcessor))
		{
			const int index = chain->monoEffects.indexOf(dynamic_cast<MonophonicEffectProcessor*>(siblingToInsertBefore));
			chain->monoEffects.insert(index, moep);
		}
		else jassertfalse;

		chain->allEffects.add(dynamic_cast<EffectProcessor*>(newProcessor));

		jassert(chain->allEffects.size() == (chain->masterEffects.size() + chain->voiceEffects.size() + chain->monoEffects.size()));
	}

	if (RoutableProcessor *rp = dynamic_cast<RoutableProcessor*>(newProcessor))
	{
		RoutableProcessor *parentRouter = dynamic_cast<RoutableProcessor*>(chain->getParentProcessor());

		jassert(parentRouter != nullptr);

		rp->getMatrix().setNumSourceChannels(parentRouter->getMatrix().getNumSourceChannels());
		rp->getMatrix().setNumDestinationChannels(parentRouter->getMatrix().getNumSourceChannels());
		rp->getMatrix().setTargetProcessor(chain->getParentProcessor());
	}
	
	if (auto sp = dynamic_cast<JavascriptProcessor*>(newProcessor))
	{
		sp->compileScript();
	}

	notifyListeners(Listener::ProcessorAdded, newProcessor);
}


void EffectProcessorChain::EffectChainHandler::remove(Processor *processorToBeRemoved, bool removeEffect/*=true*/)
{
	notifyListeners(Listener::ProcessorDeleted, processorToBeRemoved);

	ScopedPointer<Processor> ownedProcessorToRemove = processorToBeRemoved;

	{
		auto mc = chain->getMainController();

		LOCK_PROCESSING_CHAIN(chain);

		LockHelpers::SafeLock sl2(mc, LockHelpers::IteratorLock);
		LockHelpers::SafeLock sl(mc, LockHelpers::AudioLock);
		
		jassert(dynamic_cast<EffectProcessor*>(processorToBeRemoved) != nullptr);

		processorToBeRemoved->setIsOnAir(false);
		chain->allEffects.removeAllInstancesOf(dynamic_cast<EffectProcessor*>(processorToBeRemoved));

		if (auto vep = dynamic_cast<VoiceEffectProcessor*>(processorToBeRemoved))		 chain->voiceEffects.removeObject(vep, false);
		else if (auto mep = dynamic_cast<MasterEffectProcessor*>(processorToBeRemoved))		 chain->masterEffects.removeObject(mep, false);
		else if (auto moep = dynamic_cast<MonophonicEffectProcessor*>(processorToBeRemoved)) chain->monoEffects.removeObject(moep, false);
		else jassertfalse;

		jassert(chain->allEffects.size() == (chain->masterEffects.size() + chain->voiceEffects.size() + chain->monoEffects.size()));
	}

	if (removeEffect)
		ownedProcessorToRemove = nullptr;
	else
		ownedProcessorToRemove.release();
}

void EffectProcessorChain::EffectChainHandler::moveProcessor(Processor *processorInChain, int delta)
{
	if (MasterEffectProcessor *mep = dynamic_cast<MasterEffectProcessor*>(processorInChain))
	{
		const int indexOfProcessor = chain->masterEffects.indexOf(mep);
		jassert(indexOfProcessor != -1);

		const int indexOfSwapProcessor = jlimit<int>(0, chain->masterEffects.size(), indexOfProcessor + delta);

		const int indexOfProcessorInAllEffects = chain->allEffects.indexOf(mep);
		const int indexOfSwapProcessorInAllEfects = jlimit<int>(0, chain->allEffects.size(), indexOfProcessorInAllEffects + delta);

		if (indexOfProcessor != indexOfSwapProcessor)
		{
			ScopedLock sl(chain->getMainController()->getLock());

			chain->masterEffects.swap(indexOfProcessor, indexOfSwapProcessor);
			chain->allEffects.swap(indexOfProcessorInAllEffects, indexOfSwapProcessorInAllEfects);
		}
	}
}

hise::Processor * EffectProcessorChain::EffectChainHandler::getProcessor(int processorIndex)
{
	if (processorIndex < chain->voiceEffects.size())
	{
		return chain->voiceEffects.getUnchecked(processorIndex);
	}

	processorIndex -= chain->voiceEffects.size();

	if (processorIndex < chain->monoEffects.size())
	{
		return chain->monoEffects.getUnchecked(processorIndex);
	}

	processorIndex -= chain->monoEffects.size();

	if (processorIndex < chain->masterEffects.size())
	{
		return chain->masterEffects.getUnchecked(processorIndex);
	}

	jassertfalse;

	return chain->allEffects.getUnchecked(processorIndex);
}

const hise::Processor * EffectProcessorChain::EffectChainHandler::getProcessor(int processorIndex) const
{


	if (processorIndex < chain->voiceEffects.size())
	{
		return chain->voiceEffects.getUnchecked(processorIndex);
	}

	processorIndex -= chain->voiceEffects.size();

	if (processorIndex < chain->monoEffects.size())
	{
		return chain->monoEffects.getUnchecked(processorIndex);
	}

	processorIndex -= chain->monoEffects.size();

	if (processorIndex < chain->masterEffects.size())
	{
		return chain->masterEffects.getUnchecked(processorIndex);
	}

	jassertfalse;

	return chain->allEffects.getUnchecked(processorIndex);
}

int EffectProcessorChain::EffectChainHandler::getNumProcessors() const
{
	return chain->allEffects.size();
}

void EffectProcessorChain::EffectChainHandler::clear()
{
	notifyListeners(Listener::Cleared, nullptr);

	chain->voiceEffects.clear();
	chain->masterEffects.clear();
	chain->monoEffects.clear();
	chain->allEffects.clear();
}

} // namespace hise