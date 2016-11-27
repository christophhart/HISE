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

EffectProcessorChain::EffectProcessorChain(Processor *parentProcessor_, const String &id, int numVoices) :
		VoiceEffectProcessor(parentProcessor_->getMainController(), id, numVoices),
		handler(this),
		parentProcessor(parentProcessor_)
{
	effectChainFactory = new EffectProcessorChainFactoryType(numVoices, parentProcessor);

	setEditorState(Processor::Visible, false, dontSendNotification);
}

ProcessorEditorBody *::EffectProcessorChain::createEditor(ProcessorEditor *parentEditor)
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
	ADD_NAME_TO_TYPELIST(MonoFilterEffect);
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
	ADD_NAME_TO_TYPELIST(GainCollector);
	ADD_NAME_TO_TYPELIST(RouteEffect);
	ADD_NAME_TO_TYPELIST(SaturatorEffect);
	ADD_NAME_TO_TYPELIST(AudioProcessorWrapper);
	ADD_NAME_TO_TYPELIST(JavascriptMasterEffect);

#if INCLUDE_PROTOPLUG
	ADD_NAME_TO_TYPELIST(ProtoplugEffectProcessor);
#endif
};

Processor* EffectProcessorChainFactoryType::createProcessor	(int typeIndex, const String &id)
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case monophonicFilter:				return new MonoFilterEffect(m, id);
	case polyphonicFilter:				return new PolyFilterEffect(m, id, numVoices);
	case harmonicFilter:				return new HarmonicFilter(m, id, numVoices);
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
	case gainCollector:					return new GainCollector(m, id);
	case routeFX:						return new RouteEffect(m, id);
	case saturation:					return new SaturatorEffect(m, id);
	case audioProcessorWrapper:			return new AudioProcessorWrapper(m, id);
	case scriptFxProcessor:				return new JavascriptMasterEffect(m, id);
#if INCLUDE_PROTOPLUG
	case protoPlugEffect:				return new ProtoplugEffectProcessor(m, id);
#endif
	default:					jassertfalse; return nullptr;
	}
};

void EffectProcessorChain::EffectChainHandler::add(Processor *newProcessor, Processor *siblingToInsertBefore)
{
	ScopedLock sl(chain->getMainController()->getLock());

	jassert(dynamic_cast<EffectProcessor*>(newProcessor) != nullptr);

	for (int i = 0; i < newProcessor->getNumInternalChains(); i++)
	{
		dynamic_cast<ModulatorChain*>(newProcessor->getChildProcessor(i))->setColour(newProcessor->getColour());
	}

	newProcessor->setConstrainerForAllInternalChains(chain->getFactoryType()->getConstrainer());

	if (VoiceEffectProcessor* vep = dynamic_cast<VoiceEffectProcessor*>(newProcessor))
	{
		const int index = chain->voiceEffects.indexOf(dynamic_cast<VoiceEffectProcessor*>(siblingToInsertBefore));

		chain->voiceEffects.insert(index, vep);
	}
	else if (MasterEffectProcessor* mep = dynamic_cast<MasterEffectProcessor*>(newProcessor))
	{
		const int index = chain->masterEffects.indexOf(dynamic_cast<MasterEffectProcessor*>(siblingToInsertBefore));
		chain->masterEffects.insert(index, mep);
	}
	else if (MonophonicEffectProcessor* moep = dynamic_cast<MonophonicEffectProcessor*>(newProcessor))
	{
		const int index = chain->monoEffects.indexOf(dynamic_cast<MonophonicEffectProcessor*>(siblingToInsertBefore));
		chain->monoEffects.insert(index, moep);

	}
	else jassertfalse;

	if (RoutableProcessor *rp = dynamic_cast<RoutableProcessor*>(newProcessor))
	{
		RoutableProcessor *parentRouter = dynamic_cast<RoutableProcessor*>(chain->getParentProcessor());

		jassert(parentRouter != nullptr);

		rp->getMatrix().setNumSourceChannels(parentRouter->getMatrix().getNumSourceChannels());
		rp->getMatrix().setNumDestinationChannels(parentRouter->getMatrix().getNumSourceChannels());
		rp->getMatrix().setTargetProcessor(chain->getParentProcessor());
	}

	chain->allEffects.add(dynamic_cast<EffectProcessor*>(newProcessor));

	jassert(chain->allEffects.size() == (chain->masterEffects.size() + chain->voiceEffects.size() + chain->monoEffects.size()));

	if (chain->getSampleRate() > 0.0 && newProcessor != nullptr)
	{
		newProcessor->prepareToPlay(chain->getSampleRate(), chain->getBlockSize());
	}
	else
	{
		debugError(chain, "Trying to add a processor to a uninitialized effect chain (internal engine error).");
	}


	if (JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(newProcessor))
	{
		sp->compileScript();
	}

	sendChangeMessage();
}
