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



namespace hise
{
using namespace juce;


HardcodedMasterFX::HardcodedMasterFX(MainController* mc, const String& uid) :
	MasterEffectProcessor(mc, uid),
	HardcodedSwappableEffect(mc, false)
{
	auto numHardcodedFXSlots = HISE_GET_PREPROCESSOR(getMainController(), NUM_HARDCODED_FX_MODS);

	for (int i = 0; i < numHardcodedFXSlots; i++)
	{
		String p;
		p << "P" << String(i + 1) << " Modulation";
		modChains += { this, p, ModulatorChain::ModulationType::Normal, Modulation::Mode::CombinedMode };
	}

	finaliseModChains();

	extraMods.init(modChains, 0);

	getMatrix().setNumAllowedConnections(NUM_MAX_CHANNELS);
	connectionChanged();
}

HardcodedMasterFX::~HardcodedMasterFX()
{
	this->shutdown();
	modChains.clear();
}



bool HardcodedMasterFX::hasTail() const
{
	return hasHardcodedTail();
}

bool HardcodedMasterFX::isSuspendedOnSilence() const
{
	if (opaqueNode != nullptr)
		return opaqueNode->isSuspendedOnSilence();

	return true;
}

bool HardcodedMasterFX::isFadeOutPending() const noexcept
{
	if(numChannelsToRender == 2)
		return MasterEffectProcessor::isFadeOutPending();
        
	return false;
}

Processor* HardcodedMasterFX::getChildProcessor(int processorIndex)
{
	if(isPositiveAndBelow(processorIndex, getNumChildProcessors()))
		return modChains[processorIndex].getChain();

	return nullptr;
}

const Processor* HardcodedMasterFX::getChildProcessor(int processorIndex) const
{
	if(isPositiveAndBelow(processorIndex, getNumChildProcessors()))
		return modChains[processorIndex].getChain();

	return nullptr;
}

int HardcodedMasterFX::getNumInternalChains() const
{
	return modChains.size();
}

int HardcodedMasterFX::getNumChildProcessors() const
{ return getNumInternalChains(); }

void HardcodedMasterFX::connectionChanged()
{
	MasterEffectProcessor::connectionChanged();
	channelCountMatches = checkHardcodedChannelCount();
}

bool HardcodedMasterFX::setEffect(const String& newEffect, bool cond)
{
		

	auto ok = HardcodedSwappableEffect::setEffect(newEffect, true);

	if(ok)
	{
		extraMods.updateModulationProperties(modProperties, 
		                                     BIND_MEMBER_FUNCTION_1(HardcodedSwappableEffect::getParameterInitData));
	}

	return ok;
}

void HardcodedMasterFX::connectToRuntimeTargets(scriptnode::OpaqueNode& on, bool shouldAdd)
{
	if(getMainController()->isBeingDeleted())
		return;

	Processor::connectToRuntimeTargets(on, shouldAdd);

	if(auto pitchChain = getPitchChain())
		pitchChain->connectToRuntimeTargets(on, shouldAdd);

	extraMods.connectToRuntimeTarget(on, shouldAdd);
}

ModulationDisplayValue::QueryFunction::Ptr HardcodedMasterFX::getModulationQueryFunction(int parameterIndex) const
{
	return extraMods.getModulationQueryFunction(modProperties, parameterIndex);
}

void HardcodedMasterFX::voicesKilled()
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if (opaqueNode != nullptr)
		opaqueNode->reset();
}

void HardcodedMasterFX::setInternalAttribute(int parameterIndex, float newValue)
{
	setHardcodedAttribute(parameterIndex, newValue);
}

juce::ValueTree HardcodedMasterFX::exportAsValueTree() const
{
	ValueTree v = MasterEffectProcessor::exportAsValueTree();

	return writeHardcodedData(v);
}

void HardcodedMasterFX::restoreFromValueTree(const ValueTree& v)
{
	LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
	MasterEffectProcessor::restoreFromValueTree(v);

	restoreHardcodedData(v);

	
}

float HardcodedMasterFX::getAttribute(int index) const
{

	return getHardcodedAttribute(index);
}

hise::ProcessorEditorBody* HardcodedMasterFX::createEditor(ProcessorEditor *parentEditor)
{
	return createHardcodedEditor(parentEditor);
}

void HardcodedMasterFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	MasterEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);

#if USE_BACKEND
    auto numSlots = HISE_GET_PREPROCESSOR(getMainController(), NUM_HARDCODED_FX_MODS);

	if(numSlots != getNumChildProcessors())
	{
		errorBroadcaster.sendMessage(sendNotificationAsync, "NUM_HARDCODED_FX_MODS has changed. Reload this effect.");
		channelCountMatches = false;
		return ;
	}
#endif

	SimpleReadWriteLock::ScopedReadLock sl(lock);

	auto ok = prepareOpaqueNode(opaqueNode.get());
	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

juce::Path HardcodedMasterFX::getSpecialSymbol() const
{
	return getHardcodedSymbol();
}

void HardcodedMasterFX::handleHiseEvent(const HiseEvent &m)
{
	MasterEffectProcessor::handleHiseEvent(m);

	// The HISE events will be processed if there are nodes in a midi_chain
	// but the default behaviour should ignore the HISE events so that it matches
	// the script FX behaviour...
#if 0
	HiseEvent copy(m);
	if (opaqueNode != nullptr)
		opaqueNode->handleHiseEvent(copy);
#endif
}

void HardcodedMasterFX::applyEffect(AudioSampleBuffer &b, int startSample, int numSamples)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);


	auto canBeSuspended = isSuspendedOnSilence();

	if(getMainController()->getSampleManager().isNonRealtime())
		canBeSuspended = false;

	if (canBeSuspended)
	{
		if (masterState.numSilentBuffers > numSilentCallbacksToWait && startSample == 0)
		{
			auto silent = ProcessDataDyn(b.getArrayOfWritePointers(), b.getNumSamples(), b.getNumChannels()).isSilent();
			
			if (silent)
			{
				getMatrix().handleDisplayValues(b, b, false);
				masterState.currentlySuspended = true;
				return;
			}
			else
			{
				masterState.numSilentBuffers = 0;
				masterState.currentlySuspended = false;
			}
		}
	}

	masterState.currentlySuspended = false;

	auto on = opaqueNode.get();

	if(on != nullptr && channelCountMatches)
	{
		using RD = ModulatorChain::ExtraModulatorRuntimeTargetSource::RenderData<OpaqueNode>;
		auto ch = static_cast<float**>(alloca(numChannelsToRender * sizeof(float*)));
		setupChannelData(ch, b, 0);
		RD rd(*on, modProperties, eventBuffer, ch, numChannelsToRender, startSample, numSamples);
		extraMods.processChunkedWithModulation(rd);
	}

	getMatrix().handleDisplayValues(b, b, false);

	if (canBeSuspended)
	{
		if (ProcessDataDyn(b.getArrayOfWritePointers(), numSamples, b.getNumChannels()).isSilent())
			masterState.numSilentBuffers++;
		else
			masterState.numSilentBuffers = 0;
	}
}

void HardcodedMasterFX::renderWholeBuffer(AudioSampleBuffer &buffer)
{
	if (numChannelsToRender == 2)
	{
		// Rewrite the channel indexes, the buffer already points to the
		// correct channels...
		channelIndexes[0] = 0;
		channelIndexes[1] = 1;
		MasterEffectProcessor::renderWholeBuffer(buffer);
	}
	else
	{
		applyEffect(buffer, 0, buffer.getNumSamples());
	}
}



HardcodedPolyphonicFX::HardcodedPolyphonicFX(MainController *mc, const String &uid, int numVoices):
	VoiceEffectProcessor(mc, uid, numVoices),
	HardcodedSwappableEffect(mc, true)
{
	polyHandler.setVoiceResetter(this);

	auto numMods = HISE_GET_PREPROCESSOR(mc, NUM_HARDCODED_POLY_FX_MODS);

	for (int i = 0; i < numMods; i++)
	{
		String p;
		p << "P" << String(i + 1) << " Modulation";
		modChains += { this, p };
	}

	finaliseModChains();

	extraModSources.init(modChains, 0);
	
	getMatrix().setNumAllowedConnections(NUM_MAX_CHANNELS);
	getMatrix().init();
	getMatrix().setOnlyEnablingAllowed(true);
	connectionChanged();
}

HardcodedPolyphonicFX::~HardcodedPolyphonicFX()
{
	this->shutdown();
	modChains.clear();
}


float HardcodedPolyphonicFX::getAttribute(int parameterIndex) const
{
	return getHardcodedAttribute(parameterIndex);
}

void HardcodedPolyphonicFX::setInternalAttribute(int parameterIndex, float newValue)
{
	setHardcodedAttribute(parameterIndex, newValue);
}

void HardcodedPolyphonicFX::restoreFromValueTree(const ValueTree &v)
{
	VoiceEffectProcessor::restoreFromValueTree(v);

	DBG(v.createXml()->createDocument(""));

	ValueTree r = v.getChildWithName("RoutingMatrix");

	if (r.isValid())
	{
		getMatrix().restoreFromValueTree(r);
	}

	restoreHardcodedData(v);
}

juce::ValueTree HardcodedPolyphonicFX::exportAsValueTree() const
{
	auto v = VoiceEffectProcessor::exportAsValueTree();

	v.addChild(getMatrix().exportAsValueTree(), -1, nullptr);

	return writeHardcodedData(v);
}

bool HardcodedPolyphonicFX::hasTail() const
{
	return hasHardcodedTail();
}

bool HardcodedPolyphonicFX::isSuspendedOnSilence() const
{
	if (opaqueNode != nullptr)
		return opaqueNode->isSuspendedOnSilence();

	return true;
}

Processor* HardcodedPolyphonicFX::getChildProcessor(int index)
{
	return modChains[index].getChain();
}

const Processor* HardcodedPolyphonicFX::getChildProcessor(int index) const
{
	return modChains[index].getChain();
}

hise::ProcessorEditorBody * HardcodedPolyphonicFX::createEditor(ProcessorEditor *parentEditor)
{
	return createHardcodedEditor(parentEditor);
}

void HardcodedPolyphonicFX::prepareToPlay(double sampleRate, int samplesPerBlock)
{
#if USE_BACKEND
	auto numMods = HISE_GET_PREPROCESSOR(getMainController(), NUM_HARDCODED_POLY_FX_MODS);

	if(numMods != modChains.size())
	{
		errorBroadcaster.sendMessage(sendNotificationAsync, "NUM_HARDCODED_POLY_FX_MODS has changed. Reload this effect");
		channelCountMatches = false;
		return;
	}
#endif

	VoiceEffectProcessor::prepareToPlay(sampleRate, samplesPerBlock);
	SimpleReadWriteLock::ScopedReadLock sl(lock);
	auto ok = prepareOpaqueNode(opaqueNode.get());

	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

void HardcodedPolyphonicFX::startVoice(int voiceIndex, const HiseEvent& e)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	VoiceEffectProcessor::startVoice(voiceIndex, e);

	if (opaqueNode != nullptr)
	{
		voiceStack.startVoice(*opaqueNode, polyHandler, voiceIndex, e);
	}
}



void HardcodedPolyphonicFX::applyEffect(int voiceIndex, AudioSampleBuffer &b, int startSample, int numSamples)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);

	auto on = opaqueNode.get();

	if(on != nullptr)
	{
		using RD = ModulatorChain::ExtraModulatorRuntimeTargetSource::RenderData<OpaqueNode>;

		auto ch = static_cast<float**>(alloca(numChannelsToRender * sizeof(float*)));

		setupChannelData(ch, b, 0);

		RD rd(*on, modProperties, nullptr, ch, numChannelsToRender, startSample, numSamples);

		if (checkPreSuspension(voiceIndex, rd.pd))
			return;

		extraModSources.processChunkedWithModulation(rd);

		checkPostSuspension(voiceIndex, rd.pd);
	}

	getMatrix().handleDisplayValues(b, b, false);

	isTailing = on != nullptr && voiceStack.containsVoiceIndex(voiceIndex);
}

void HardcodedPolyphonicFX::renderData(ProcessDataDyn& data)
{
	auto voiceIndex = polyHandler.getVoiceIndex();

	if (checkPreSuspension(voiceIndex, data))
		return;

	HardcodedSwappableEffect::renderData(data);

	checkPostSuspension(voiceIndex, data);
}

void HardcodedPolyphonicFX::renderNextBlock(AudioSampleBuffer&, int, int)
{
		
}

void HardcodedPolyphonicFX::reset(int voiceIndex)
{
	VoiceEffectProcessor::reset(voiceIndex);

	voiceStack.reset(voiceIndex);
}

void HardcodedPolyphonicFX::handleHiseEvent(const HiseEvent& m)
{
#if USE_BACKEND
	const auto useMods = modChains.size() > 0;
#else
	constexpr auto useMods = NUM_HARDCODED_POLY_FX_MODS > 0;
#endif

	if(useMods)
		VoiceEffectProcessor::handleHiseEvent(m);

	// Already handled...
	if(m.isNoteOn())
		return;
        
	if (opaqueNode != nullptr)
		voiceStack.handleHiseEvent(*opaqueNode, polyHandler, m);
}

void HardcodedPolyphonicFX::connectionChanged()
{
	channelCountMatches = checkHardcodedChannelCount();
}

void HardcodedPolyphonicFX::renderVoice(int voiceIndex, AudioSampleBuffer& b, int startSample, int numSamples)
{
#if USE_BACKEND
	const auto useMods = modChains.size() > 0;
#else
	constexpr auto useMods = NUM_HARDCODED_POLY_FX_MODS > 0;
#endif

	if(useMods)
		preVoiceRendering(voiceIndex, startSample, numSamples);

	applyEffect(voiceIndex, b, startSample, numSamples);
}

int HardcodedPolyphonicFX::getNumActiveVoices() const
{
	return voiceStack.voiceNoteOns.size();
}

bool HardcodedPolyphonicFX::isVoiceResetActive() const
{
	return hasHardcodedTail();
}

void HardcodedPolyphonicFX::onVoiceReset(bool allVoices, int voiceIndex)
{
	if (allVoices)
		voiceStack.voiceNoteOns.clear();
	else
		voiceStack.reset(voiceIndex);
}

bool HardcodedPolyphonicFX::setEffect(const String& effectName, bool cond)
{
	auto ok = HardcodedSwappableEffect::setEffect(effectName, false);

	if(ok)
	{
		jassert(opaqueNode != nullptr);
		extraModSources.updateModulationProperties(modProperties, 
		                                           BIND_MEMBER_FUNCTION_1(HardcodedSwappableEffect::getParameterInitData));
	}

	return ok;
}

void HardcodedPolyphonicFX::connectToRuntimeTargets(scriptnode::OpaqueNode& on, bool shouldAdd)
{
	if(getMainController()->isBeingDeleted())
		return;

	Processor::connectToRuntimeTargets(on, shouldAdd);

	if(auto pitchChain = getPitchChain())
		pitchChain->connectToRuntimeTargets(on, shouldAdd);
	
	extraModSources.connectToRuntimeTarget(on, shouldAdd);
}

ModulationDisplayValue::QueryFunction::Ptr HardcodedPolyphonicFX::getModulationQueryFunction(int parameterIndex) const
{
	return extraModSources.getModulationQueryFunction(modProperties, parameterIndex);
}


HardcodedTimeVariantModulator::HardcodedTimeVariantModulator(hise::MainController *mc, const String &uid, Modulation::Mode m):
  HardcodedSwappableEffect(mc, false),
  Modulation(m),
  TimeVariantModulator(mc, uid, m)
{
    numChannelsToRender = 1;
}

HardcodedTimeVariantModulator::~HardcodedTimeVariantModulator()
{
	this->shutdown();
}

void HardcodedTimeVariantModulator::calculateBlock(int startSample, int numSamples)
{
    SimpleReadWriteLock::ScopedReadLock sl(lock);

    if(opaqueNode != nullptr && channelCountMatches)
    {
		auto* modData = internalBuffer.getWritePointer(0, startSample);
        FloatVectorOperations::clear(modData, numSamples);
        
        ProcessDataDyn d(&modData, numSamples, 1);
        opaqueNode->process(d);
    }
}

void HardcodedTimeVariantModulator::handleHiseEvent(const hise::HiseEvent &m)
{
    HiseEvent copy(m);
    if (opaqueNode != nullptr)
        opaqueNode->handleHiseEvent(copy);
}

void HardcodedTimeVariantModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    TimeVariantModulator::prepareToPlay(sampleRate, samplesPerBlock);
    
    SimpleReadWriteLock::ScopedReadLock sl(lock);
    auto ok = prepareOpaqueNode(opaqueNode.get());

	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

hise::ProcessorEditorBody *HardcodedTimeVariantModulator::createEditor(hise::ProcessorEditor *parentEditor)
{
    return createHardcodedEditor(parentEditor);
}

float HardcodedTimeVariantModulator::getAttribute(int index) const
{
    return getHardcodedAttribute(index);
}

void HardcodedTimeVariantModulator::restoreFromValueTree(const juce::ValueTree &v)
{
    LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
    TimeVariantModulator::restoreFromValueTree(v);

    restoreHardcodedData(v);
}

juce::ValueTree HardcodedTimeVariantModulator::exportAsValueTree() const
{
    ValueTree v = TimeVariantModulator::exportAsValueTree();
    return writeHardcodedData(v);
}

void HardcodedTimeVariantModulator::setInternalAttribute(int index, float newValue)
{
    setHardcodedAttribute(index, newValue);
}

bool HardcodedTimeVariantModulator::checkHardcodedChannelCount()
{
    if (opaqueNode != nullptr)
    {
        return opaqueNode->numChannels == 1;
    }
    
    return false;
}

Result HardcodedTimeVariantModulator::prepareOpaqueNode(scriptnode::OpaqueNode *n)
{
    if (n != nullptr && asProcessor().getSampleRate() > 0.0 && asProcessor().getLargestBlockSize() > 0)
    {
        PrepareSpecs ps;
        ps.numChannels = 1;
        ps.blockSize = asProcessor().getLargestBlockSize() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
        ps.sampleRate = asProcessor().getSampleRate() / (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
        ps.voiceIndex = &polyHandler;
        n->prepare(ps);
        n->reset();

#if USE_BACKEND
        auto e = factory->getError();

        if (e.error != Error::OK)
        {
            return Result::fail(ScriptnodeExceptionHandler::getErrorMessage(e));
        }
#endif
    }

	return Result::ok();
}

HardcodedEnvelopeModulator::HardcodedEnvelopeModulator(MainController* mc, const String& id, int numVoices,
	Modulation::Mode m):
	EnvelopeModulator(mc, id, numVoices, m),
	HardcodedSwappableEffect(mc, true),
	Modulation(m)
{
	numChannelsToRender = 1;
	polyHandler.setVoiceResetter(this);
}

HardcodedEnvelopeModulator::~HardcodedEnvelopeModulator()
{
	this->shutdown();
}

ValueTree HardcodedEnvelopeModulator::exportAsValueTree() const
{
	ValueTree v = EnvelopeModulator::exportAsValueTree();
	return writeHardcodedData(v);
}

void HardcodedEnvelopeModulator::restoreFromValueTree(const ValueTree& v)
{
	LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
	EnvelopeModulator::restoreFromValueTree(v);
	restoreHardcodedData(v);
}

int HardcodedEnvelopeModulator::getParameterOffset() const
{ return (int)hise::EnvelopeModulator::Parameters::numParameters; }

void HardcodedEnvelopeModulator::setInternalAttribute(int index, float newValue)
{
	if (index < getParameterOffset())
		EnvelopeModulator::setInternalAttribute(index, newValue);
	else
	{
		index -= getParameterOffset();

		setHardcodedAttribute(index, newValue);
	}
}

float HardcodedEnvelopeModulator::getAttribute(int index) const
{
	if (index < getParameterOffset())
		return EnvelopeModulator::getAttribute(index);
	else
	{
		index -= getParameterOffset();
		return getHardcodedAttribute(index);
	}
}

float HardcodedEnvelopeModulator::getDefaultValue(int index) const
{
	if (index < getParameterOffset())
		return EnvelopeModulator::getDefaultValue(index);
	else
	{
		index -= getParameterOffset();
		return getHardcodedAttribute(index);
	}
}

bool HardcodedEnvelopeModulator::checkHardcodedChannelCount()
{
	if (opaqueNode != nullptr)
		return opaqueNode->numChannels == 1;
	    
	return false;
}

ProcessorEditorBody* HardcodedEnvelopeModulator::createEditor(ProcessorEditor* parentEditor)
{
	return createHardcodedEditor(parentEditor);
}

void HardcodedEnvelopeModulator::onVoiceReset(bool allVoices, int voiceIndex)
{
	if (allVoices)
	{
		for (int i = 0; i < polyManager.getVoiceAmount(); i++)
			reset(i);
	}
	else
		reset(voiceIndex);
}

bool HardcodedEnvelopeModulator::isVoiceResetActive() const
{ return true; }

int HardcodedEnvelopeModulator::getNumActiveVoices() const
{
	return voiceStack.getNumActiveVoices();
}

void HardcodedEnvelopeModulator::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	EnvelopeModulator::prepareToPlay(sampleRate, samplesPerBlock);

	SimpleReadWriteLock::ScopedReadLock sl(lock);
	auto ok = prepareOpaqueNode(opaqueNode.get());

	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

Result HardcodedEnvelopeModulator::prepareOpaqueNode(scriptnode::OpaqueNode* n)
{
	if(auto rm = dynamic_cast<scriptnode::routing::GlobalRoutingManager*>(asProcessor().getMainController()->getGlobalRoutingManager()))
		tempoSyncer.additionalEventStorage = &rm->additionalEventStorage;

	if (n != nullptr && asProcessor().getSampleRate() > 0.0 && asProcessor().getLargestBlockSize() > 0)
	{
		PrepareSpecs ps;
		ps.numChannels = 1;
		ps.blockSize = asProcessor().getLargestBlockSize() / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		ps.sampleRate = asProcessor().getSampleRate() / (double)HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;
		ps.voiceIndex = &polyHandler;
		n->prepare(ps);
		n->reset();

#if USE_BACKEND
		auto e = factory->getError();

		if (e.error != Error::OK)
		{
			//return Result::fail(ScriptnodeExceptionHandler::getErrorMessage(e));
		}
#endif
	}

	return Result::ok();
}

float HardcodedEnvelopeModulator::startVoice(int voiceIndex)
{
	if(opaqueNode != nullptr && channelCountMatches)
	{
		voiceStack.startVoice(*opaqueNode, polyHandler, voiceIndex, nextEvent);

		PolyHandler::ScopedVoiceSetter svs(polyHandler, voiceIndex);
		span<float, 1> d = {0.0f};
		opaqueNode->processFrame(d);
		return d[0];
	}

	return 0.0f;
}

void HardcodedEnvelopeModulator::stopVoice(int voiceIndex)
{
		
}

void HardcodedEnvelopeModulator::reset(int voiceIndex)
{
	voiceStack.reset(voiceIndex);
}

bool HardcodedEnvelopeModulator::isPlaying(int voiceIndex) const
{
	return voiceStack.containsVoiceIndex(voiceIndex);
}

void HardcodedEnvelopeModulator::calculateBlock(int startSample, int numSamples)
{
	SimpleReadWriteLock::ScopedReadLock sl(lock);

	if(opaqueNode != nullptr && channelCountMatches)
	{
		PolyHandler::ScopedVoiceSetter svs(polyHandler, polyManager.getCurrentVoice());

		auto* modData = internalBuffer.getWritePointer(0, startSample);
		FloatVectorOperations::clear(modData, numSamples);
        
		ProcessDataDyn d(&modData, numSamples, 1);
		opaqueNode->process(d);
	}
}

void HardcodedEnvelopeModulator::handleHiseEvent(const HiseEvent& m)
{
	nextEvent = m;
	if(opaqueNode != nullptr)
	{
		voiceStack.handleHiseEvent(*opaqueNode, polyHandler, m);
	}
}

EnvelopeModulator::ModulatorState* HardcodedEnvelopeModulator::createSubclassedState(int voiceIndex) const
{
	jassertfalse; return nullptr;
}

bool HardcodedSynthesiser::Sound::appliesToNote(int) { return true; }
bool HardcodedSynthesiser::Sound::appliesToChannel(int) { return true; }
bool HardcodedSynthesiser::Sound::appliesToVelocity(int) { return true; }

HardcodedSynthesiser::Voice::Voice(HardcodedSynthesiser* p):
	ModulatorSynthVoice(p),
	synth(p)
{}

void HardcodedSynthesiser::Voice::calculateBlock(int startSample, int numSamples)
{
	if(!synth->channelCountMatches)
		return;

	if (auto on = synth->opaqueNode.get())
	{
		if (isVoiceStart)
		{
			synth->voiceData.startVoice(*on, synth->polyHandler, getVoiceIndex(), getCurrentHiseEvent());
			isVoiceStart = false;
		}

		voiceBuffer.clear();

		using RD = ModulatorChain::ExtraModulatorRuntimeTargetSource::RenderData<OpaqueNode>;

		int numChannels = voiceBuffer.getNumChannels();

		RD rd(*on, synth->modProperties, nullptr, voiceBuffer.getArrayOfWritePointers(), numChannels, startSample, numSamples);

		PolyHandler::ScopedVoiceSetter svs(synth->polyHandler, getVoiceIndex());
		synth->extraModSources.processChunkedWithModulation(rd);

		if (auto modValues = getOwnerSynth()->getVoiceGainValues())
		{
			for(int i = 0; i < voiceBuffer.getNumChannels(); i++)
				FloatVectorOperations::multiply(voiceBuffer.getWritePointer(i, startSample), modValues + startSample, numSamples);
		}
		else
		{
			const float gainValue = getOwnerSynth()->getConstantGainModValue();

			for (int i = 0; i < voiceBuffer.getNumChannels(); i++)
				FloatVectorOperations::multiply(voiceBuffer.getWritePointer(i, startSample), gainValue, numSamples);
		}

		getOwnerSynth()->effectChain->renderVoice(voiceIndex, voiceBuffer, startSample, numSamples);
	}
}

void HardcodedSynthesiser::Voice::resetVoice()
{
	ModulatorSynthVoice::resetVoice();
	synth->voiceData.reset(getVoiceIndex());
}

void HardcodedSynthesiser::Voice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ModulatorSynthVoice::prepareToPlay(sampleRate, samplesPerBlock);

	auto numSynthChannels = synth->numChannelsToRender;

	if(numSynthChannels != voiceBuffer.getNumChannels())
		voiceBuffer.setSize(numSynthChannels, samplesPerBlock);
}

HardcodedSynthesiser::HardcodedSynthesiser(MainController* mc, const String& id, int numVoices):
	ModulatorSynth(mc, id, numVoices),
	HardcodedSwappableEffect(mc, true)
{
	auto numMods = HISE_GET_PREPROCESSOR(getMainController(), NUM_HARDCODED_SYNTH_MODS);

	for(int i = 0; i < numMods; i++)
		modChains += { this, "Extra" + String(i+1), ModulatorChain::ModulationType::Normal, Modulation::Mode::CombinedMode };

	finaliseModChains();

	extraModSources.init(modChains, 2);

	for(int i = 0; i < numMods; i++)
		modChains[i + 2].getChain()->setColour(Colour(0xFF888888));
		
	for (int i = 0; i < numVoices; i++)
		addVoice(new Voice(this));

	addSound(new Sound());

	getMatrix().setNumAllowedConnections(NUM_MAX_CHANNELS);
	getMatrix().setAllowResizing(true);
	connectionChanged();

	polyHandler.setVoiceResetter(this);
}

HardcodedSynthesiser::~HardcodedSynthesiser()
{
	this->shutdown();
	modChains.clear();
}

ProcessorEditorBody* HardcodedSynthesiser::createEditor(ProcessorEditor* parentEditor)
{
#if USE_BACKEND
	return new HardcodedMasterEditor(parentEditor);
#else
		return nullptr;
#endif
}

void HardcodedSynthesiser::connectionChanged()
{
	ModulatorSynth::connectionChanged();
	channelCountMatches = checkHardcodedChannelCount();

	if(getSampleRate() > 0 && opaqueNode != nullptr)
	{
		auto ok = prepareOpaqueNode(opaqueNode);

		errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());

		for(auto v: voices)
			static_cast<ModulatorSynthVoice*>(v)->prepareToPlay(getSampleRate(), getLargestBlockSize());
	}
}

void HardcodedSynthesiser::preHiseEventCallback(HiseEvent& e)
{
	ModulatorSynth::preHiseEventCallback(e);

	if (e.isNoteOn())
		return; // will be handled by preStartVoice

	if (auto n = opaqueNode.get())
		voiceData.handleHiseEvent(*n, polyHandler, e);
}

void HardcodedSynthesiser::preStartVoice(int voiceIndex, const HiseEvent& e)
{
	ModulatorSynth::preStartVoice(voiceIndex, e);

	if (opaqueNode != nullptr)
		static_cast<Voice*>(getVoice(voiceIndex))->setVoiceStartDataForNextRenderCallback();
}

void HardcodedSynthesiser::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(sampleRate, samplesPerBlock);

#if USE_BACKEND
	auto numMods = HISE_GET_PREPROCESSOR(getMainController(), NUM_HARDCODED_SYNTH_MODS);

	if(numMods != (modChains.size() - 2))
	{
		errorBroadcaster.sendMessage(sendNotificationAsync, "NUM_HARDCODED_SYNTH_MODS has changed. Reload this synth");
		channelCountMatches = false;
		return;
	}
#endif

	
	SimpleReadWriteLock::ScopedReadLock sl(HardcodedSwappableEffect::lock);
	auto ok = prepareOpaqueNode(opaqueNode.get());

	errorBroadcaster.sendMessage(sendNotificationAsync, ok.getErrorMessage());
}

Processor* HardcodedSynthesiser::getChildProcessor(int processorIndex)
{
	if (processorIndex < ModulatorSynth::numInternalChains)
		return ModulatorSynth::getChildProcessor(processorIndex);

	processorIndex -= ModulatorSynth::numInternalChains;
	processorIndex += 2;

	return modChains[processorIndex].getChain();
}

const Processor* HardcodedSynthesiser::getChildProcessor(int processorIndex) const
{
	return const_cast<HardcodedSynthesiser*>(this)->getChildProcessor(processorIndex);
}

int HardcodedSynthesiser::getNumInternalChains() const
{ return modChains.size() + 2; }

int HardcodedSynthesiser::getNumChildProcessors() const
{ return getNumInternalChains(); }

ValueTree HardcodedSynthesiser::exportAsValueTree() const
{
	ValueTree v = ModulatorSynth::exportAsValueTree();
	return writeHardcodedData(v);
}

void HardcodedSynthesiser::restoreFromValueTree(const ValueTree& v)
{
	LockHelpers::noMessageThreadBeyondInitialisation(getMainController());
	ModulatorSynth::restoreFromValueTree(v);
	restoreHardcodedData(v);
}

float HardcodedSynthesiser::getAttribute(int parameterIndex) const
{
	auto offset = getParameterOffset();

	if(parameterIndex < offset)
		return ModulatorSynth::getAttribute(parameterIndex);

	return getHardcodedAttribute(parameterIndex - offset);
}

void HardcodedSynthesiser::setInternalAttribute(int parameterIndex, float newValue)
{
	auto offset = getParameterOffset();

	if(parameterIndex < offset)
		ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
	else
		setHardcodedAttribute(parameterIndex - offset, newValue);
}

float HardcodedSynthesiser::getDefaultValue(int parameterIndex) const
{
	auto offset = getParameterOffset();

	if(parameterIndex < offset)
		return ModulatorSynth::getDefaultValue(parameterIndex);

	return getAttribute(parameterIndex - offset);
}

void HardcodedSynthesiser::connectToRuntimeTargets(scriptnode::OpaqueNode& opaqueNode, bool shouldAdd)
{
	if(getMainController()->isBeingDeleted())
		return;

	Processor::connectToRuntimeTargets(opaqueNode, shouldAdd);

	if(auto pitchChain = getPitchChain())
		pitchChain->connectToRuntimeTargets(opaqueNode, shouldAdd);

	extraModSources.connectToRuntimeTarget(opaqueNode, shouldAdd);
}

ModulationDisplayValue::QueryFunction::Ptr HardcodedSynthesiser::getModulationQueryFunction(int parameterIndex) const
{
	parameterIndex -= getParameterOffset();
	return extraModSources.getModulationQueryFunction(modProperties, parameterIndex);
}

bool HardcodedSynthesiser::setEffect(const String& effectName, bool cond)
{
	auto ok = HardcodedSwappableEffect::setEffect(effectName, false);

	if(ok)
	{
		jassert(opaqueNode != nullptr);
		extraModSources.updateModulationProperties(modProperties, 
		                                           BIND_MEMBER_FUNCTION_1(HardcodedSwappableEffect::getParameterInitData));
	}

	return ok;
}

void HardcodedSynthesiser::onVoiceReset(bool allVoices, int voiceIndex)
{
	if (allVoices)
	{
		for(auto v: activeVoices)
			v->resetVoice();
	}
	else
	{
		if(auto v = static_cast<ModulatorSynthVoice*>(getVoice(voiceIndex)))
			v->resetVoice();
	}
}

bool HardcodedSynthesiser::isVoiceResetActive() const
{
	return true;
}

int HardcodedSynthesiser::getNumActiveVoices() const
{
	return voiceData.getNumActiveVoices();
}
}
