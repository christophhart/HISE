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

SET_DOCUMENTATION(ModulatorSynth)
{
	ADD_PARAMETER_DOC(Gain, "The volume of the synth. It is stored as gain value from `0...1` so you need to use the conversion functions when using decibel ranges");
	ADD_PARAMETER_DOC(Balance, "The stereo balance of the synth. The range is `-100...100`");
	ADD_PARAMETER_DOC(VoiceLimit, "The number of voices that this synth can play.");
	ADD_PARAMETER_DOC(KillFadeTime, "If you play more than the number of available voices this determines the fade out time of the voice that is going to be killed in ms");

	ADD_CHAIN_DOC(MidiProcessor, "MIDI",
		"Every MIDI message that is received by the sound generator will be processed by this chain. If you ignore the message here, it won't be passed to child modules");
	ADD_CHAIN_DOC(GainModulation, "Gain",
		"The volume modulation of this sound generator. The modulation range 0...1 will be used as gain value");
	ADD_CHAIN_DOC(PitchModulation, "Pitch",
		"The pitch modulation of this sound generator. The modulation range 0...1 will be converted to pitch values according to the BiPolar parameter");
	ADD_CHAIN_DOC(EffectChain, "FX",
		"the effect chain of this module");
}

ModulatorSynth::ModulatorSynth(MainController *mc, const String &id, int numVoices) :
Synthesiser(),
Processor(mc, id, numVoices),
RoutableProcessor(),
midiProcessorChain(new MidiProcessorChain(mc, "Midi Processor", this)),
effectChain(new EffectProcessorChain(this, "FX", numVoices)),
pitchBuffer(1, 0),
internalBuffer(2, 0),
gainBuffer(1, 0),
gain(0.25f),
killFadeTime(20.0f),
vuValue(0.0f),
lastStartedVoice(nullptr),
group(nullptr),
voiceLimit(-1),
iconColour(Colours::transparentBlack),
clockSpeed(ClockSpeed::Inactive),
lastClockCounter(0),
wasPlayingInLastBuffer(false),
bypassState(false)
{
	modChains += { this, "GainModulation", ModulatorChain::ModulationType::Normal, Modulation::Mode::GainMode};
	modChains += { this, "PitchModulation", ModulatorChain::ModulationType::Normal, Modulation::Mode::PitchMode};

	effectChain->setParentProcessor(this);
	midiProcessorChain->setParentProcessor(this);

	setVoiceLimit(numVoices);

	for (int i = 0; i < 4; i++)
	{
		nextTimerCallbackTimes[i] = 0.0;
		synthTimerIntervals[i] = 0.0;
	}

	getMatrix().init();

	parameterNames.add("Gain");
	parameterNames.add("Balance");
	parameterNames.add("VoiceLimit");
	parameterNames.add("KillFadeTime");

	editorStateIdentifiers.add("OverviewFolded");
	editorStateIdentifiers.add("MidiProcessorShown");
	editorStateIdentifiers.add("GainModulationShown");
	editorStateIdentifiers.add("PitchModulationShown");
	editorStateIdentifiers.add("EffectChainShown");

	setBalance(0.0f);

	
}

ModulatorSynth::~ModulatorSynth()
{
	deleteAllVoices();
	
	midiProcessorChain = nullptr;
	gainChain = nullptr;
	pitchChain = nullptr;
	effectChain = nullptr;

	modChains.clear();

}

ValueTree ModulatorSynth::exportAsValueTree() const
{
	// must be named 'v' for the macros
	ValueTree v = Processor::exportAsValueTree();

	v.addChild(getMatrix().exportAsValueTree(), -1, nullptr);

	saveAttribute(Gain, "Gain");
	saveAttribute(Balance, "Balance");
	saveAttribute(VoiceLimit, "VoiceLimit");
	saveAttribute(KillFadeTime, "KillFadeTime");

	v.setProperty("IconColour", iconColour.toString(), nullptr);

	return v;
}

void ModulatorSynth::restoreFromValueTree(const ValueTree &v)
{
	RESTORE_MATRIX();

	loadAttribute(Gain, "Gain");
	loadAttribute(Balance, "Balance");
	loadAttribute(VoiceLimit, "VoiceLimit");
	loadAttribute(KillFadeTime, "KillFadeTime");

	iconColour = Colour::fromString(v.getProperty("IconColour", Colours::transparentBlack.toString()).toString());

	Processor::restoreFromValueTree(v);
}

float ModulatorSynth::getAttribute(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Gain:			return gain;
	case Balance:		return balance;
	case VoiceLimit:	return (float)voiceLimit;
	case KillFadeTime:	return killFadeTime;
	default:			jassertfalse; return 0.0f;
	}
}

void ModulatorSynth::setInternalAttribute(int parameterIndex, float newValue)
{
	switch (parameterIndex)
	{
	case Gain:			setGain(newValue); break;
	case Balance:		setBalance(newValue); break;
	case VoiceLimit:	setVoiceLimit((int)newValue); break;
	case KillFadeTime:	setKillFadeOutTime((double)newValue); break;
	default:			jassertfalse; return;
	}
}

float ModulatorSynth::getDefaultValue(int parameterIndex) const
{
	switch (parameterIndex)
	{
	case Gain:			return 1.0;
	case Balance:		return 0.0;
	case VoiceLimit:	return (float)64;
	case KillFadeTime:	return 20;
	default:			jassertfalse; return 0.0f;
	}
}

Processor * ModulatorSynth::getChildProcessor(int processorIndex)
{
	jassert(processorIndex < numInternalChains);

	switch (processorIndex)
	{
	case GainModulation:	return gainChain;
	case PitchModulation:	return pitchChain;
	case MidiProcessor:		return midiProcessorChain;
	case EffectChain:		return effectChain;
	default:				jassertfalse; return nullptr;
	}
}

const Processor * ModulatorSynth::getChildProcessor(int processorIndex) const
{
	jassert(processorIndex < numInternalChains);

	switch (processorIndex)
	{
	case GainModulation:	return gainChain;
	case PitchModulation:	return pitchChain;
	case MidiProcessor:		return midiProcessorChain;
	case EffectChain:		return effectChain;
	default:				jassertfalse; return nullptr;
	}
}

int ModulatorSynth::getNumChildProcessors() const
{ return numInternalChains; }

int ModulatorSynth::getNumInternalChains() const
{ return numInternalChains; }

void ModulatorSynth::setIconColour(Colour newIconColour)
{ 
	iconColour = newIconColour; 
	getMainController()->getProcessorChangeHandler().sendProcessorChangeMessage(this, MainController::ProcessorChangeHandler::EventType::ProcessorColourChange, false);
}

Colour ModulatorSynth::getIconColour() const
{ return iconColour; }

Colour ModulatorSynth::getColour() const
{ return HiseColourScheme::getColour(HiseColourScheme::ModulatorSynthBackgroundColourId); }

bool ModulatorSynth::isSoftBypassed() const
{ return bypassState; }

ModulatorSynth::SoundCollectorBase::~SoundCollectorBase()
{}

void ModulatorSynth::initRenderCallback()
{
	internalBuffer.clear();
}

void ModulatorSynth::setGain(float newGain)
{
	gain = newGain;
}

void ModulatorSynth::setBalance(float newBalance)
{
	const float l = BalanceCalculator::getGainFactorForBalance((newBalance * 100.0f), true);
	const float r = BalanceCalculator::getGainFactorForBalance((newBalance * 100.0f), false);

	balance.store(newBalance);
	leftBalanceGain.store(l);
	rightBalanceGain.store(r);
}

float ModulatorSynth::getBalance(bool getRightChannelGain) const
{
	return getRightChannelGain ? rightBalanceGain : leftBalanceGain;
}

float ModulatorSynth::getGain() const
{	return gain; }

ModulatorSynthGroup* ModulatorSynth::getGroup() const
{	return group; }

bool ModulatorSynth::isInGroup() const
{ return group != nullptr;}

void ModulatorSynth::setClockSpeed(ClockSpeed newClockSpeed)
{
	clockSpeed = newClockSpeed;
}

bool ModulatorSynth::allowEmptyPitchValues() const
{
	const bool isGroup = ProcessorHelpers::is<ModulatorSynthGroup>(this);

	return !(isGroup || isInGroup());
}

const float* ModulatorSynth::getConstantPitchValues() const
{ return pitchBuffer.getReadPointer(0);	}

double ModulatorSynth::getSampleRate() const
{ return Processor::getSampleRate(); }

void ModulatorSynth::setKillRetriggeredNote(bool shouldBeKilled)
{ shouldKillRetriggeredNote = shouldBeKilled; }

float* ModulatorSynth::getPitchValuesForVoice() const
{
	if (useScratchBufferForArtificialPitch)
		return modChains[BasicChains::PitchChain].getScratchBuffer();
		
	return modChains[BasicChains::PitchChain].getWritePointerForVoiceValues(0);
}

void ModulatorSynth::overwritePitchValues(const float* modDataValues, int startSample, int numSamples)
{
	useScratchBufferForArtificialPitch = true;

	auto destination = modChains[BasicChains::PitchChain].getScratchBuffer();

	FloatVectorOperations::copy(destination + startSample, modDataValues + startSample, numSamples);
}

const float* ModulatorSynth::getVoiceGainValues() const
{
	return modChains[BasicChains::GainChain].getReadPointerForVoiceValues(0);
}

float ModulatorSynth::getConstantPitchModValue() const
{
	return modChains[BasicChains::PitchChain].getConstantModulationValue();
}

float ModulatorSynth::getConstantGainModValue() const
{
	return modChains[BasicChains::GainChain].getConstantModulationValue();
}

float ModulatorSynth::getConstantVoicePitchModulationValueDeleteSoon() const
{
	// Is applied to uptimeDelta already...
	jassertfalse;

	return 1.0f;
}

HiseEventBuffer* ModulatorSynth::getEventBuffer()
{ return &eventBuffer; }

void ModulatorSynth::setUseUniformVoiceHandler(bool shouldUseVoiceHandler, UniformVoiceHandler* externalHandlerToUse)
{
	currentUniformVoiceHandler = shouldUseVoiceHandler ? externalHandlerToUse :
		                             nullptr;
}

bool ModulatorSynth::isUsingUniformVoiceHandler() const
{ return currentUniformVoiceHandler.get() != nullptr; }

UniformVoiceHandler* ModulatorSynth::getUniformVoiceHandler() const
{
	return currentUniformVoiceHandler.get();
        
}

bool ModulatorSynth::synthNeedsEnvelope() const
{ return true; }

bool ModulatorSynth::checkTimerCallback(int timerIndex, int numSamplesThisBlock) const noexcept
{
	if(!anyTimerActive)
		return false;
        
	auto nextCallbackTime = nextTimerCallbackTimes[timerIndex].load();

	if (nextTimerCallbackTimes[timerIndex] == 0.0)
		return false;

	auto uptime = getMainController()->getUptime();
	auto timeThisBlock = (double)numSamplesThisBlock / getSampleRate();

	Range<double> rangeThisBlock(uptime, uptime + timeThisBlock);
	return uptime > nextCallbackTime || rangeThisBlock.contains(nextCallbackTime);
}

int ModulatorSynth::getFreeTimerSlot()
{
	if (synthTimerIntervals[0] == 0.0) return 0;
	if (synthTimerIntervals[1] == 0.0) return 1;
	if (synthTimerIntervals[2] == 0.0) return 2;
	if (synthTimerIntervals[3] == 0.0) return 3;

	return -1;
}

void ModulatorSynth::synthTimerCallback(uint8 index, int numSamplesThisBlock)
{
	if (index >= 0)
	{
		ADD_GLITCH_DETECTOR(this, DebugLogger::Location::TimerCallback);

		const double uptime = getMainController()->getUptime();

		// this has to be a uint32 because otherwise it could wrap around the uint16 max sample offset for bigger timer callbacks
		uint32 offsetInBuffer = (uint32)((jmax(0.0, nextTimerCallbackTimes[index] - uptime)) * getSampleRate());

		const uint32 delta = offsetInBuffer % HISE_EVENT_RASTER;
		uint32 rasteredOffset = offsetInBuffer - delta;

		while (synthTimerIntervals[index] > 0.0 && rasteredOffset < (uint32)numSamplesThisBlock)
		{
			eventBuffer.addEvent(HiseEvent::createTimerEvent(index, (uint16)rasteredOffset));
			nextTimerCallbackTimes[index].store(nextTimerCallbackTimes[index].load() + synthTimerIntervals[index].load());
			offsetInBuffer = (uint32)((nextTimerCallbackTimes[index] - uptime) * getSampleRate());
			const uint32 newDelta = offsetInBuffer % HISE_EVENT_RASTER;
			rasteredOffset = offsetInBuffer - newDelta;
		}
	}
	else jassertfalse;
}

void ModulatorSynth::startSynthTimer(int index, double interval, int timeStamp)
{
	if (interval < 0.004)
	{
		nextTimerCallbackTimes[index] = 0.0;
		jassertfalse;
		debugToConsole(this, "Go easy on the timer!");
		return;
	};

	if (index >= 0)
	{
        anyTimerActive = true;
        
		synthTimerIntervals[index] = interval;

		const double thisUptime = getMainController()->getUptime();
		const double timeStampSeconds = getSampleRate() > 0.0 ? (double)timeStamp / getSampleRate() : 0.0;

		if (interval != 0.0) nextTimerCallbackTimes[index] = thisUptime + timeStampSeconds + synthTimerIntervals[index];
	}
	else jassertfalse;
}

void ModulatorSynth::stopSynthTimer(int index)
{
	if (index >= 0)
	{
		nextTimerCallbackTimes[index] = 0.0;
		synthTimerIntervals[index] = 0.0;
	}
}

double ModulatorSynth::getTimerInterval(int index) const noexcept
{
	if (index >= 0) return synthTimerIntervals[index];

	return 0.0;
}

bool ModulatorSynth::isLastStartedVoice(ModulatorSynthVoice *voice)
{
	return voice == lastStartedVoice;
}

ModulatorSynthVoice* ModulatorSynth::getLastStartedVoice() const
{
	return lastStartedVoice;
}

int ModulatorSynth::getNumActiveVoices() const
{
	return activeVoices.size() - pendingRemoveVoices.size();
}

ProcessorEditorBody *ModulatorSynth::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new EmptyProcessorEditorBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}


void ModulatorSynth::processHiseEventBuffer(const HiseEventBuffer &inputBuffer, int numSamples)
{
	eventBuffer.copyFrom(inputBuffer);

	if (!eventBuffer.isEmpty())
		midiInputAlpha = 1.0f;
	else
		midiInputAlpha = jmax(0.0f, midiInputAlpha - 0.02f);

	if (checkTimerCallback(0, numSamples)) synthTimerCallback(0, numSamples);
	if (checkTimerCallback(1, numSamples)) synthTimerCallback(1, numSamples);
	if (checkTimerCallback(2, numSamples)) synthTimerCallback(2, numSamples);
	if (checkTimerCallback(3, numSamples)) synthTimerCallback(3, numSamples);

	if (getMainController()->getMainSynthChain() == this)
	{
		handleHostInfoHiseEvents(numSamples);
	}

	midiProcessorChain->renderNextHiseEventBuffer(eventBuffer, numSamples);

	eventBuffer.alignEventsToRaster<HISE_EVENT_RASTER>(numSamples);
}

void ModulatorSynth::addProcessorsWhenEmpty()
{
	LockHelpers::freeToGo(getMainController());

	jassert(finalised);

	

	if (dynamic_cast<ModulatorSynthChain*>(this) == nullptr)
	{
		auto envList = ProcessorHelpers::getListOfAllProcessors<EnvelopeModulator>(gainChain);
		
		if (envList.size() > 1) // the chain itself is an envelope...
			return;

		ScopedPointer<SimpleEnvelope> newEnvelope = new SimpleEnvelope(getMainController(),
			"DefaultEnvelope",
            getVoiceAmount(),
			Modulation::GainMode);

		gainChain->getHandler()->add(newEnvelope.release(), nullptr);

		setEditorState("GainModulationShown", 1, dontSendNotification);
	}
}

void ModulatorSynth::renderNextBlockWithModulators(AudioSampleBuffer& outputBuffer, const HiseEventBuffer& inputMidiBuffer)
{
	jassert(isOnAir());

    ADD_GLITCH_DETECTOR(this, DebugLogger::Location::SynthRendering);
    
	int numSamples = outputBuffer.getNumSamples();

	const int numSamplesFixed = numSamples;

	// The buffer must be initialized. Did you forget to call the base class prepareToPlay()
	jassert(numSamplesFixed <= internalBuffer.getNumSamples());

	int startSample = 0;

	
	initRenderCallback();

	processHiseEventBuffer(inputMidiBuffer, numSamplesFixed);

	

	HiseEventBuffer::Iterator eventIterator(eventBuffer);

	HiseEvent m;
	int midiEventPos;

	while (numSamples > 0)
	{
		if (!eventIterator.getNextEvent(m, midiEventPos, true, false))
		{
			preVoiceRendering(startSample, numSamples);
			renderVoice(startSample, numSamples);
			postVoiceRendering(startSample, numSamples);

			break;
		}

		const int samplesToNextMidiMessage = jmin(numSamples, midiEventPos - startSample);

		jassert(startSample % HISE_EVENT_RASTER == 0);
		jassert(midiEventPos % HISE_EVENT_RASTER == 0);
		jassert(samplesToNextMidiMessage % HISE_EVENT_RASTER == 0);

		if (samplesToNextMidiMessage > 0)
		{
			preVoiceRendering(startSample, samplesToNextMidiMessage);
			renderVoice(startSample, samplesToNextMidiMessage);
			postVoiceRendering(startSample, samplesToNextMidiMessage);
			
		}

		handleHiseEvent(m);

		startSample += samplesToNextMidiMessage;
		numSamples -= samplesToNextMidiMessage;
	}

	while (eventIterator.getNextEvent(m, midiEventPos, true, false))
		handleHiseEvent(m);

	AudioSampleBuffer thisInternalBuffer(internalBuffer.getArrayOfWritePointers(), internalBuffer.getNumChannels(), numSamplesFixed);

	if (getMainController()->getDebugLogger().isLogging())
	{
		for (int i = 0; i < thisInternalBuffer.getNumChannels(); i++)
		{
			getMainController()->getDebugLogger().checkSampleData(this, DebugLogger::Location::SynthRendering, i % 2 != 0, thisInternalBuffer.getReadPointer(i), numSamplesFixed);
		}
	}

	effectChain->renderMasterEffects(thisInternalBuffer);

	for (int i = 0; i < thisInternalBuffer.getNumChannels(); i++)
	{
		const int destinationChannel = getMatrix().getConnectionForSourceChannel(i);
		
		if (destinationChannel >= 0 && destinationChannel < outputBuffer.getNumChannels())
		{
			const float thisGain = gain.load() * (i % 2 == 0 ? leftBalanceGain : rightBalanceGain);
			FloatVectorOperations::addWithMultiply(outputBuffer.getWritePointer(destinationChannel, 0), thisInternalBuffer.getReadPointer(i, 0), thisGain, numSamplesFixed);
		}
	}

	getMatrix().handleDisplayValues(thisInternalBuffer, outputBuffer, true);

	

	handlePeakDisplay(numSamplesFixed);
}

void ModulatorSynth::preVoiceRendering(int startSample, int numThisTime)
{
	for (auto& mb : modChains)
		mb.calculateMonophonicModulationValues(startSample, numThisTime);

	effectChain->preRenderCallback(startSample, numThisTime);
}

void ModulatorSynth::renderVoice(int startSample, int numThisTime)
{
    ADD_GLITCH_DETECTOR(this, DebugLogger::Location::SynthVoiceRendering);
    
	clearPendingRemoveVoices();

	for (auto v : activeVoices)
	{
		jassert(!v->isInactive());

		calculateModulationValuesForVoice(v, startSample, numThisTime);

		v->renderNextBlock(internalBuffer, startSample, numThisTime);
	}

	clearPendingRemoveVoices();
};

	
void ModulatorSynth::calculateModulationValuesForVoice(ModulatorSynthVoice * v, int startSample, int numThisTime)
{
	auto index = v->getVoiceIndex();

	for (auto& mb : modChains)
	{
		mb.calculateModulationValuesForCurrentVoice(index, startSample, numThisTime);
		if (mb.isAudioRateModulation())
			mb.expandVoiceValuesToAudioRate(index, startSample, numThisTime);
	}
		
	v->setUptimeDeltaValueForBlock();

	v->applyConstantPitchFactor(getConstantPitchModValue());

	useScratchBufferForArtificialPitch = false;

	if (v->isPitchFadeActive())
	{
		auto bufferToUse = modChains[BasicChains::PitchChain].getWritePointerForVoiceValues(0);

		if (bufferToUse == nullptr)
		{
			bufferToUse = modChains[BasicChains::PitchChain].getScratchBuffer();
			FloatVectorOperations::fill(bufferToUse + startSample, 1.0f, numThisTime);
			useScratchBufferForArtificialPitch = true;
		}

		v->applyScriptPitchFactors(bufferToUse + startSample, numThisTime);
	}
}

void ModulatorSynth::clearPendingRemoveVoices()
{
	for (auto v : pendingRemoveVoices)
	{
		jassert(v->isInactive());
		activeVoices.remove(v);

		if (isLastStartedVoice(v) && !activeVoices.isEmpty())
			lastStartedVoice = activeVoices[activeVoices.size()-1];
	}

	pendingRemoveVoices.clearQuick();
}

void ModulatorSynth::postVoiceRendering(int startSample, int numThisTime)
{
	modChains[BasicChains::GainChain].expandMonophonicValuesToAudioRate(startSample, numThisTime);
	auto monoValues = modChains[BasicChains::GainChain].getMonophonicModulationValues(startSample);

	if (monoValues != nullptr && numThisTime > 0)
	{
		CHECK_AND_LOG_BUFFER_DATA_WITH_ID(this, getIDAsIdentifier(), DebugLogger::Location::SynthPostVoiceRenderingGainMod, gainBuffer.getReadPointer(0, startSample), true, numThisTime);

		gainChain->applyMonoOnOutputValue(monoValues[0]);

		// Apply all gain modulators to the rendered voices
		for (int i = 0; i < internalBuffer.getNumChannels(); i++)
		{
			FloatVectorOperations::multiply(internalBuffer.getWritePointer(i, startSample), monoValues, numThisTime);

			CHECK_AND_LOG_BUFFER_DATA_WITH_ID(this, getIDAsIdentifier(), DebugLogger::Location::SynthPostVoiceRendering, internalBuffer.getReadPointer(i, startSample), i % 2 != 0, numThisTime);
		}
	}
	
	if (!isChainDisabled(EffectChain)) effectChain->renderNextBlock(internalBuffer, startSample, numThisTime);
}

void ModulatorSynth::handlePeakDisplay(int numSamplesInOutputBuffer)
{
#if ENABLE_ALL_PEAK_METERS
	
	currentValues.outL = gain * internalBuffer.getMagnitude(0, 0, numSamplesInOutputBuffer) * leftBalanceGain;
	currentValues.outR = gain * internalBuffer.getMagnitude(1, 0, numSamplesInOutputBuffer) * rightBalanceGain;
	
#else

	if (this == getMainController()->getMainSynthChain())
	{
		currentValues.outL = gain * internalBuffer.getMagnitude(0, 0, numSamplesInOutputBuffer) * leftBalanceGain;
		currentValues.outR = gain * internalBuffer.getMagnitude(1, 0, numSamplesInOutputBuffer) * rightBalanceGain;
	}

#endif
}


void ModulatorSynth::setPeakValues(float l, float r)
{
	currentValues.outL = l;
	currentValues.outR = r;
}

void ModulatorSynth::handleHiseEvent(const HiseEvent& m)
{
	auto c = m;

	if (getMainController()->getKillStateHandler().voiceStartIsDisabled())
	{
		// Pass the all notes off messages through
		if (c.isAllNotesOff())
		{
			preHiseEventCallback(c);
			allNotesOff(c.getChannel(), true);
		}

		return;
	}
	
	preHiseEventCallback(c);
	
	const int channel = c.getChannel();

	if (c.isNoteOn())
	{
		noteOn(c);
	}
	else if (c.isNoteOff())
	{
		noteOff(c);
	}
	else if (c.isAllNotesOff())
	{
		allNotesOff(channel, true);
	}
	else if (c.isController())
	{
		const int controllerNumber = c.getControllerNumber();

		switch (controllerNumber)
		{
		case 0x40:  handleSustainPedal(channel, c.getControllerValue() >= 64); break;
		case 0x42:  handleSostenutoPedal(channel, c.getControllerValue() >= 64); break;
		case 0x43:  handleSoftPedal(channel, c.getControllerValue() >= 64); break;
		default:    break;
		}
	}
	else if (c.isVolumeFade())
	{
		handleVolumeFade(c.getEventId(), c.getFadeTime(), c.getGainFactor());
	}
	else if (c.isPitchFade())
	{
		handlePitchFade(c.getEventId(), c.getFadeTime(), c.getPitchFactorForEvent());
	}
}

#define DECLARE_ID(x) const juce::Identifier x(#x);

namespace HostEventIds
{
DECLARE_ID(ppqPosition);
DECLARE_ID(isPlaying);
}

#undef DECLARE_ID

void ModulatorSynth::handleHostInfoHiseEvents(int numSamples)
{
	int ppqTimeStamp = -1;

	// Check if ppqPosition should be added
	

	const bool hostIsPlaying = getMainController()->getHostInfoObject()->getProperty(HostEventIds::isPlaying);

	if (hostIsPlaying && clockSpeed != ClockSpeed::Inactive)
	{
		double ppq = getMainController()->getHostInfoObject()->getProperty(HostEventIds::ppqPosition);

		const double bufferAsMilliseconds = (double)numSamples / getSampleRate();
		const double bufferAsPPQ = bufferAsMilliseconds * (getMainController()->getBpm() / 60.0);
		const double ppqAtEndOfBuffer = ppq + bufferAsPPQ;

		const double clockSpeedMultiplier = pow(2.0, clockSpeed);

		const int clockAtStartOfBuffer = (int)((ppq)* clockSpeedMultiplier);
		const int clockAtEndOfBuffer = (int)((ppqAtEndOfBuffer)* clockSpeedMultiplier);

		if (clockAtStartOfBuffer != clockAtEndOfBuffer)
		{
			const double remaining = (double)clockAtEndOfBuffer / clockSpeedMultiplier - ppq;
			const double remainingTime = (60.0 / getMainController()->getBpm()) * remaining;
			const double remainingSamples = getSampleRate() * remainingTime;

			if (remainingSamples < numSamples)
			{
				ppqTimeStamp = (int)remainingSamples;
			}

			lastClockCounter = clockAtStartOfBuffer;
		}
	}

	
	if (hostIsPlaying != wasPlayingInLastBuffer)
	{
		HiseEvent m = hostIsPlaying ? HiseEvent(HiseEvent::Type::MidiStart, 0, 0) : HiseEvent(HiseEvent::Type::MidiStop, 0, 0);

		eventBuffer.addEvent(m);
	}

	if (ppqTimeStamp != -1)
	{
		HiseEvent pos = HiseEvent(HiseEvent::Type::SongPosition, 0, 0);

		pos.setSongPositionValue(ppqTimeStamp);
		pos.setTimeStamp(ppqTimeStamp);

		eventBuffer.addEvent(pos);
	}
}

void ModulatorSynth::handleVolumeFade(int eventId, int fadeTimeMilliseconds, float targetGain)
{
	const double fadeTimeSeconds = (double)fadeTimeMilliseconds / 1000.0;

	for (auto v : activeVoices)
	{
		if (v->getCurrentHiseEvent().getEventId() == eventId)
			v->setVolumeFade(fadeTimeSeconds, targetGain);
	}
}

void ModulatorSynth::handlePitchFade(uint16 eventId, int fadeTimeMilliseconds, double pitchFactor)
{
	const double fadeTimeSeconds = (double)fadeTimeMilliseconds / 1000.0;

	for (int i = voices.size(); --i >= 0;)
	{
		ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(voices[i]);

		if (!v->isInactive() && v->getCurrentHiseEvent().getEventId() == eventId)
		{
			v->setPitchFade(fadeTimeSeconds, pitchFactor);
		}
	}
}

void ModulatorSynth::preHiseEventCallback(HiseEvent &e)
{
	if (e.isAllNotesOff())
	{
		stopSynthTimer(0);
		stopSynthTimer(1);
		stopSynthTimer(2);
		stopSynthTimer(3);
	}

	for (auto& mb : modChains)
		mb.handleHiseEvent(e);

	effectChain->handleHiseEvent(e);
}

bool ModulatorSynth::soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity)
{
	return sound->appliesToMessage(midiChannel, midiNoteNumber, (int)(velocity * 127));
};



	
void ModulatorSynth::startVoiceWithHiseEvent(ModulatorSynthVoice* voice, SynthesiserSound *sound, const HiseEvent &e)
{
	jassert(!getMainController()->getKillStateHandler().voiceStartIsDisabled());

	if (shouldHaveEnvelope && !gainChain->hasActivePolyEnvelopes())
	{
		debugError(this, "You need at least one envelope in the gain chain");
		return;
	}

#if JUCE_DEBUG
	// If this is false, your collectSoundsToBeStarted method is wrong
	// it uses the event id because it might have another start offset in a detuned synth group
	jassert(voice->getCurrentHiseEvent().getEventId() == eventForSoundCollection.getEventId());
#endif

	pendingRemoveVoices.remove(voice);

	activeVoices.insert(voice);

	if (auto uvh = getUniformVoiceHandler())
	{
		uvh->incVoiceCounter(this, voice->getVoiceIndex());
	}

	Synthesiser::startVoice(static_cast<SynthesiserVoice*>(voice), sound, e.getChannel(), e.getNoteNumber(), e.getFloatVelocity());

	voice->saveStartUptimeDelta();
}

void ModulatorSynth::preStartVoice(int voiceIndex, const HiseEvent& e)
{
	LOG_SYNTH_EVENT("preStartVoice for " + getId() + " with index " + String(voiceIndex));

	lastStartedVoice = static_cast<ModulatorSynthVoice*>(getVoice(voiceIndex));

	for (auto& mb : modChains)
	{
		mb.startVoice(voiceIndex);
	}

	effectChain->startVoice(voiceIndex, e);
}

void ModulatorSynth::preStopVoice(int voiceIndex)
{
	for (auto& mb : modChains)
		mb.stopVoice(voiceIndex);

	effectChain->stopVoice(voiceIndex);
}

void ModulatorSynth::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	if (isOnAir())
	{
		LockHelpers::freeToGo(getMainController());

		//jassert(LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::AudioLock));
	}

	LockHelpers::SafeLock audioLock(getMainController(), LockHelpers::AudioLock, isOnAir());

	// You must call finaliseModChains() in your Constructor...
	jassert(finalised);

	if(newSampleRate != -1.0)
	{
		// Set the channel amount correctly
		internalBuffer.setSize(getMatrix().getNumSourceChannels(), internalBuffer.getNumSamples());

		ProcessorHelpers::increaseBufferIfNeeded(pitchBuffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(gainBuffer, samplesPerBlock);
		ProcessorHelpers::increaseBufferIfNeeded(internalBuffer, samplesPerBlock);
		
		for(int i = 0; i < getNumVoices(); i++)
		{
 			static_cast<ModulatorSynthVoice*>(getVoice(i))->prepareToPlay(newSampleRate, samplesPerBlock);
		}

		vuMerger.limitFromBlockSizeToFrameRate(newSampleRate, samplesPerBlock);

		Synthesiser::setCurrentPlaybackSampleRate(newSampleRate);

		Processor::prepareToPlay(newSampleRate, samplesPerBlock);
		
		midiProcessorChain->prepareToPlay(newSampleRate, samplesPerBlock);

		for (auto& mb : modChains)
			mb.prepareToPlay(newSampleRate, samplesPerBlock);

		CHECK_COPY_AND_RETURN_12(effectChain);

		effectChain->prepareToPlay(newSampleRate, samplesPerBlock);

		setKillFadeOutTime(killFadeTime);

		updateShouldHaveEnvelope();
	}
}

	
void ModulatorSynth::numSourceChannelsChanged()
{
	ScopedLock sl(getMainController()->getLock());

	if (internalBuffer.getNumSamples() != 0)
	{
		jassert(getLargestBlockSize() > 0);
		internalBuffer.setSize(getMatrix().getNumSourceChannels(), internalBuffer.getNumSamples());
	}

	for (int i = 0; i < effectChain->getNumChildProcessors(); i++)
	{
		RoutableProcessor *rp = dynamic_cast<RoutableProcessor*>(effectChain->getChildProcessor(i));

		if (rp != nullptr)
		{
			rp->getMatrix().setNumSourceChannels(getMatrix().getNumSourceChannels());
			rp->getMatrix().setNumDestinationChannels(getMatrix().getNumSourceChannels());
		}
	}
}

void ModulatorSynth::numDestinationChannelsChanged()
{
	for (int i = 0; i < effectChain->getNumChildProcessors(); i++)
	{
		RoutableProcessor *rp = dynamic_cast<RoutableProcessor*>(effectChain->getChildProcessor(i));
		
		if (rp != nullptr)
		{
			rp->getMatrix().setNumSourceChannels(getMatrix().getNumSourceChannels());
			rp->getMatrix().setNumDestinationChannels(getMatrix().getNumSourceChannels());
		}
	}
}

void ModulatorSynth::setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler) noexcept
{
	if (isBypassed() != shouldBeBypassed)
	{
		Processor::setBypassed(shouldBeBypassed, notifyChangeHandler);

		setSoftBypass(shouldBeBypassed, true);
	}
}

void ModulatorSynth::softBypassStateChanged(bool isBypassedNow)
{
	bypassState.store(isBypassedNow);

	
}

void ModulatorSynth::setSoftBypass(bool shouldBeBypassed, bool bypassFXToo)
{
	auto f = [shouldBeBypassed](Processor* p)
	{
		static_cast<ModulatorSynth*>(p)->softBypassStateChanged(shouldBeBypassed);
		return SafeFunctionCall::OK;
	};

	if (bypassFXToo && shouldBeBypassed)
		effectChain->killMasterEffects();
	else
		effectChain->updateSoftBypassState();

	getMainController()->getKillStateHandler().killVoicesAndCall(this, f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
}



void ModulatorSynth::updateSoftBypassState()
{
	setSoftBypass(isBypassed(), false);

	effectChain->updateSoftBypassState();
}

void ModulatorSynth::flagVoiceAsRemoved(ModulatorSynthVoice* v)
{
	jassert(v->isInactive());

	pendingRemoveVoices.insert(v);
}

void ModulatorSynth::finaliseModChains()
{
	modChains.finalise();

	gainChain = modChains[BasicChains::GainChain].getChain();
	pitchChain = modChains[BasicChains::PitchChain].getChain();

	modChains[BasicChains::GainChain].setIncludeMonophonicValuesInVoiceRendering(false);
	modChains[BasicChains::PitchChain].setAllowModificationOfVoiceValues(true);

	modChains[BasicChains::GainChain].setExpandToAudioRate(true);
	modChains[BasicChains::PitchChain].setExpandToAudioRate(true);

	//pitchChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());

	gainChain->setTableValueConverter(Modulation::getValueAsDecibel);
	pitchChain->setTableValueConverter(Modulation::getValueAsSemitone);

	disableChain(GainModulation, false);
	disableChain(PitchModulation, false);
	disableChain(MidiProcessor, false);
	disableChain(EffectChain, false);

	for (auto& mb : modChains)
		mb.getChain()->setParentProcessor(this);

	finalised = true;
}

void ModulatorSynth::setGroup(ModulatorSynthGroup* parent)
{
    jassert(group == nullptr);
    group = parent;
    disableChain(MidiProcessor, true);
    //disableChain(EffectChain, true);

    // only poly effects will be rendered
    dynamic_cast<Chain*>(getChildProcessor(EffectChain))->getFactoryType()->setConstrainer(new SynthGroupFXConstrainer());
    
    // This will get lost otherwise
    modChains[BasicChains::GainChain].setIncludeMonophonicValuesInVoiceRendering(true);
}

void ModulatorSynth::disableChain(InternalChains chainToDisable, bool shouldBeDisabled)
{
	disabledChains.setBit(chainToDisable, shouldBeDisabled);
}

bool ModulatorSynth::isChainDisabled(InternalChains chain) const
{
	return disabledChains[chain];
}

int ModulatorSynth::getNumFreeVoices() const
{
	auto numActive = activeVoices.size() - pendingRemoveVoices.size();
	return internalVoiceLimit - numActive;
}


int ModulatorSynth::collectSoundsToBeStarted(const HiseEvent &m)
{
	jassert(m.isNoteOn());

#if JUCE_DEBUG
	// Save this for later...
	eventForSoundCollection = m;
#endif

	const int midiChannel = m.getChannel();
	const int midiNoteNumber = m.getNoteNumber();
	const int transposedMidiNoteNumber = midiNoteNumber + m.getTransposeAmount();
	const float velocity = m.getFloatVelocity();

	soundsToBeStarted.clearQuick();

	if (soundCollector != nullptr)
	{
		soundCollector->collectSounds(m, soundsToBeStarted);
	}
	else
	{
		for (auto s : sounds)
		{
			ModulatorSynthSound *sound = static_cast<ModulatorSynthSound*>(s);

			if (soundCanBePlayed(sound, midiChannel, transposedMidiNoteNumber, velocity))
				soundsToBeStarted.insertWithoutSearch(sound);
		}
	}

	return soundsToBeStarted.size();
}


bool ModulatorSynth::handleVoiceLimit(int numSoundsToBeStarted)
{
	auto numFreeVoices = getNumFreeVoices();

	if (numSoundsToBeStarted > internalVoiceLimit)
	{
		// Whoops, you try to start more sounds with one note than the voice limit
		jassertfalse;
		numSoundsToBeStarted = internalVoiceLimit;

		soundsToBeStarted.shrink(numSoundsToBeStarted);
	}

	bool killedSomething = false;

	while (numFreeVoices <= numSoundsToBeStarted)
	{
		const bool forceKill = numFreeVoices == 0;
		const auto killedThisTime = killLastVoice(!forceKill);

		if (killedThisTime != 0)
		{
			killedSomething = true;
		}
		else
		{
			jassertfalse;
			break;
		}

		numFreeVoices += killedThisTime;
	}

	return killedSomething;

#if 0
	// if the voiceLimit is reached, kill the voice!
	if (numFreeVoices <= numSoundsToBeStarted)
	{
		const bool forceKill = numFreeVoices == 0;
		killLastVoice(!forceKill);
		return true;
	}
#endif

}



void ModulatorSynth::noteOn(const HiseEvent &m)
{
    ADD_GLITCH_DETECTOR(this, DebugLogger::Location::NoteOnCallback);
	
	const int numSoundsToStart = collectSoundsToBeStarted(m);

	if (numSoundsToStart == 0)
		return;

	// Make room for the sounds
	handleVoiceLimit(numSoundsToStart);

	

	for(auto sound: soundsToBeStarted)
	{
		auto v = getVoiceToStart(m);

		if( v != nullptr)
		{
			jassert(v->isInactive());

			const int voiceIndex = v->getVoiceIndex();

			LOG_SYNTH_EVENT("Start voice " + String(voiceIndex));

			jassert(voiceIndex != -1);

			v->setStartUptime(getMainController()->getUptime());
			v->setCurrentHiseEvent(m);

			preStartVoice(voiceIndex, m);

			startVoiceWithHiseEvent (v, sound, m);
		}
		else
		{
			// your handleVoiceLimit() function failed...
			jassertfalse;
		}
    }
	
}

void ModulatorSynth::noteOn(int midiChannel, int midiNoteNumber, float velocity)
{
	jassertfalse;

	HiseEvent m(HiseEvent::Type::NoteOn, (uint8)midiNoteNumber, (uint8)(velocity * 127.0f), (uint8)midiChannel);

	noteOn(m);
}


void ModulatorSynth::noteOff(const HiseEvent &m)
{
	float velocity = m.getFloatVelocity();
	const int midiChannel = m.getChannel();

	const uint16 eventId = m.getEventId();

	jassert(eventId != 0);

	for (int i = voices.size(); --i >= 0;)
	{
		SynthesiserVoice* const voice = voices.getUnchecked(i);

		ModulatorSynthVoice* const mvoice = static_cast<ModulatorSynthVoice*>(voices.getUnchecked(i));

		if (mvoice->getCurrentHiseEvent().getEventId() == eventId
			&& voice->isPlayingChannel(midiChannel))
		{
			if (auto sound = voice->getCurrentlyPlayingSound().get())
			{
				if (sound->appliesToChannel(midiChannel))
				{
					voice->setKeyDown(false);

					if (!(voice->isSostenutoPedalDown() || voice->isSustainPedalDown()))
						stopVoice(voice, velocity, true);
				}
			}
		}
	}
}

int ModulatorSynth::getVoiceIndex(const SynthesiserVoice *v) const
{
	return static_cast<const ModulatorSynthVoice*>(v)->getVoiceIndex();
}



void ModulatorSynth::setScriptGainValue(int voiceIndex, float gainValue) noexcept
{

	if (voiceIndex < voices.size())
	static_cast<ModulatorSynthVoice*>(voices[jmax<int>(0, voiceIndex)])->setScriptGainValue(gainValue);
}

void ModulatorSynth::setScriptPitchValue(int voiceIndex, double pitchValue) noexcept
{
	if (voiceIndex < voices.size())
	static_cast<ModulatorSynthVoice*>(voices[jmax<int>(0, voiceIndex)])->setScriptPitchValue((float)pitchValue);
}

int ModulatorSynth::getIndexInGroup() const
{
	if (group == nullptr) return -1;

	ModulatorSynthGroup::ChildSynthIterator iterator(group, ModulatorSynthGroup::ChildSynthIterator::IterateAllSynths);

	ModulatorSynth *child;

	int index = 0;

	while(iterator.getNextAllowedChild(child))
	{
		if(child == this) return index;
		index ++;
	}

	return -1;
}

ModulatorSynth* ModulatorSynth::getPlayingSynth()
{
	return isInGroup() ? static_cast<ModulatorSynth*>(getGroup()) : this;
}

const ModulatorSynth* ModulatorSynth::getPlayingSynth() const
{
	return isInGroup() ? static_cast<const ModulatorSynth*>(getGroup()) : this;
}

void ModulatorSynthVoice::resetVoice()
{
	LOG_SYNTH_EVENT("Reset Note for " + getOwnerSynth()->getId() + " with index " + String(voiceIndex));

	clearCurrentNote();

	ModulatorSynth *os = getOwnerSynth();

	ModulatorChain *g = static_cast<ModulatorChain*>(os->getChildProcessor(ModulatorSynth::GainModulation));
	ModulatorChain *p = static_cast<ModulatorChain*>(os->getChildProcessor(ModulatorSynth::PitchModulation));
	EffectProcessorChain *e = static_cast<EffectProcessorChain*>(os->getChildProcessor(ModulatorSynth::EffectChain));

	if(g->hasActiveEnvelopesAtAll())
		g->reset(voiceIndex);

	if(p->hasActiveEnvelopesAtAll())
		p->reset(voiceIndex);

	e->reset(voiceIndex);

	uptimeDelta = 0.0;
	voiceUptime = 0.0;
	startUptime = DBL_MAX;

	isTailing = false;
    isActive = false;

	killThisVoice = false;
	killFadeLevel = 1.0f;


	gainFader.setValueWithoutSmoothing(1.0f);

	pitchFader.setValueWithoutSmoothing(1.0);

	os->flagVoiceAsRemoved(this);

	currentHiseEvent = HiseEvent();

	if (auto uvh = getOwnerSynth()->getUniformVoiceHandler())
	{
		uvh->decVoiceCounter(getOwnerSynth(), getVoiceIndex());
	}
}

void ModulatorSynthVoice::checkRelease()
{
	ModulatorSynth *os = getOwnerSynth();
	
	ModulatorChain *g = static_cast<ModulatorChain*>(os->getChildProcessor(ModulatorSynth::GainModulation));

	if( killThisVoice && (killFadeLevel < 0.001f) )
	{
		resetVoice();
		return;
	}

	if(!g->hasActivePolyEnvelopes() || ! g->isPlaying(voiceIndex) )
	{
		EffectProcessorChain *e = static_cast<EffectProcessorChain*>(os->getChildProcessor(ModulatorSynth::EffectChain));
		
		if (e->hasTailingPolyEffects())
			return;

		resetVoice();
	}
}


int ModulatorSynthVoice::getVoiceIndex() const
{
	return voiceIndex;
}

void ModulatorSynthVoice::applyGainModulation(int startSample, int numSamples, bool copyLeftChannel)
{
	if (copyLeftChannel)
	{
		if (auto modValues = getOwnerSynth()->getVoiceGainValues())
		{
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startSample), modValues + startSample, numSamples);
		}
		else
		{
			const float gainMod = getOwnerSynth()->getConstantGainModValue();

			if(gainMod != 1.0f)
				FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startSample), modValues + startSample, numSamples);
		}

		FloatVectorOperations::copy(voiceBuffer.getWritePointer(1, startSample), voiceBuffer.getReadPointer(0, startSample), numSamples);
	}
	else
	{
		if (auto modValues = getOwnerSynth()->getVoiceGainValues())
		{
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startSample), modValues + startSample, numSamples);
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startSample), modValues + startSample, numSamples);
		}
		else
		{
			const float gainMod = getOwnerSynth()->getConstantGainModValue();

			if (gainMod != 1.0f)
			{
				FloatVectorOperations::multiply(voiceBuffer.getWritePointer(0, startSample), gainMod, numSamples);
				FloatVectorOperations::multiply(voiceBuffer.getWritePointer(1, startSample), gainMod, numSamples);
			}
		}
	}
}

void ModulatorSynthVoice::stopNote(float, bool)
{
	LOG_SYNTH_EVENT("Stop Note for " + getOwnerSynth()->getId() + " with index " + String(voiceIndex));
    
	ModulatorSynth *os = getOwnerSynth();
	isTailing = true;
	os->preStopVoice(voiceIndex);
	checkRelease();
};

void ModulatorSynth::handleRetriggeredNote(ModulatorSynthVoice *voice)
{
	if(shouldKillRetriggeredNote)
		voice->killVoice();
};

ModulatorSynthVoice* ModulatorSynth::getFreeVoice(SynthesiserSound* s, int midiChannel, int midiNoteNumber)
{
	auto v = findFreeVoice(s, midiChannel, midiNoteNumber, false);

	if (v != nullptr)
	{
		auto mv = static_cast<ModulatorSynthVoice*>(v);
		LOG_SYNTH_EVENT("Found free voice with index " + String(mv->getVoiceIndex()));
		return mv;
	}

	return nullptr;
}

juce::SynthesiserVoice* ModulatorSynth::findVoiceToSteal(SynthesiserSound* soundToPlay, int midiChannel, int midiNoteNumber) const
{
	for (auto v: activeVoices)
	{
		// return voices that are being killed
		if (v->isBeingKilled())
		{
			DBG("Already killing: Found voice " + String(v->getVoiceIndex()) + " to steal");
			return v;
		}
	}

	return Synthesiser::findVoiceToSteal(soundToPlay, midiChannel, midiNoteNumber);
}

float ModulatorSynth::getMidiInputFlag()
{
	return midiInputAlpha;
}




void ModulatorSynth::killAllVoicesWithNoteNumber(int noteNumber)
{
	for(int i = 0; i < voices.size(); i++)
	{	
		if(voices[i]->isPlayingChannel(1) && voices[i]->getCurrentlyPlayingNote() == noteNumber)
		{
			static_cast<ModulatorSynthVoice*>(voices[i])->killVoice();
		}
	}
};



	
int ModulatorSynth::killLastVoice(bool allowTailOff/*=true*/)
{
	ModulatorSynthVoice *oldestUnkilledMessage = nullptr;
	double oldestUptime = DBL_MAX;

	// This variable will count additional voice kills
	// that happen when a sibling voice is killed along with 
	// its to-be killed sibling...
	
	int numVoicesKilled = 0;

	for (auto v: activeVoices)
	{
		if (v->isInactive())
			continue;

		if (!v->isTailingOff())
			continue;

		// If there's a voice already being killed and we need to 
		// make room for another voice kill, force-kill it and its siblings
		if (!allowTailOff && v->isBeingKilled())
		{
			LOG_SYNTH_EVENT("Force-kill voice " + String(v->getVoiceIndex()));
			numVoicesKilled += killVoiceAndSiblings(v, false);

			if (numVoicesKilled > 0)
			{
				// job is done, return
				return numVoicesKilled;
			}
			else
			{
				allowTailOff = true;
			}
		}
		else
		{
			const double voiceUptime = v->getVoiceUptime();

			if (!v->isBeingKilled() && voiceUptime < oldestUptime)
			{
				oldestUptime = voiceUptime;
				oldestUnkilledMessage = v;
			}
		}
	}

	if (oldestUnkilledMessage != nullptr)
	{
		return killVoiceAndSiblings(oldestUnkilledMessage, allowTailOff);
	}

	oldestUnkilledMessage = nullptr;
	oldestUptime = DBL_MAX;

	// Now the same again for all notes
	for (auto v: activeVoices)
	{
		if (v->isInactive())
			continue;

		// If there's a voice already being killed and we need to 
		// make room for another voice kill, force-kill it and its siblings
		if (!allowTailOff && v->isBeingKilled())
		{
			numVoicesKilled += killVoiceAndSiblings(v, false);

			if (numVoicesKilled > 0)
			{
				// job is done, return
				return numVoicesKilled;
			}
			else
			{
				allowTailOff = true;
			}
		}
		else
		{
			const double voiceUptime = v->getVoiceUptime();

			if (!v->isBeingKilled() && voiceUptime < oldestUptime)
			{
				oldestUptime = voiceUptime;
				oldestUnkilledMessage = v;
			}
		}
	}

	if (oldestUnkilledMessage != nullptr)
	{
		return killVoiceAndSiblings(oldestUnkilledMessage, allowTailOff);
	}

	// Just forcekill the first voice that is active...
	for (auto v : activeVoices)
	{
		if (v->isBeingKilled())
		{
			return killVoiceAndSiblings(v, false);
		}
	}
	
	return 0;

#if 0
	int sizeAtCalculationStart = voices.size();

	// search all tailing notes
	for(int i = 0; i < voices.size(); i++)
	{
		// must not delete elements from the voice stack during this loop...
		jassert(sizeAtCalculationStart == voices.size());

		ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(voices[i]);



		// If there is already a note fading out, kill it and
		// let the newest tail off
		if (!allowTailOff && v->isBeingKilled())
		{
			killVoiceAndSiblings(v, false);
			allowTailOff = true;
		}

		if(v->isBeingKilled() || !v->isTailingOff()) continue;

		

		const double voiceUptime = v->getVoiceUptime();

		if(voiceUptime < oldestUptime)
		{
			oldestUptime = voiceUptime;
			oldest = v;
		}
	}
	
	
	if(oldest != nullptr)
	{
		killVoiceAndSiblings(oldest, allowTailOff);
		return;
	}

	// search all notes
	for(int i = 0; i < voices.size(); i++)
	{
		ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(voices[i]);

		// If there is already a note fading out, kill it and
		// let the newest tail off
		if (!allowTailOff && v->isBeingKilled())
		{
			killVoiceAndSiblings(v, false);
			allowTailOff = true;
		}

		if(v->isBeingKilled()) continue;

		const double voiceUptime = v->getVoiceUptime();

		if(voiceUptime < oldestUptime)
		{
			oldestUptime = voiceUptime;
			oldest = v;
		}
	}

	if(oldest != nullptr)
	{
		killVoiceAndSiblings(oldest, allowTailOff);
		return;
	}
#endif
};


int ModulatorSynth::killVoiceAndSiblings(ModulatorSynthVoice* v, bool allowTailOff)
{
	auto e = v->getCurrentHiseEvent();

	int numVoicesKilled = 0;

	for (auto av: activeVoices)
	{
		// Skip the note
		if (av == v)
			continue;

		// Let inactive notes be removed later
		if (av->isInactive())
			continue;

		if (av->getCurrentHiseEvent() == e)
		{
			if (allowTailOff)
			{
				LOG_SYNTH_EVENT("Kill sibling voice " + String(av->getVoiceIndex()));
				av->killVoice();
			}
			else
			{
				LOG_SYNTH_EVENT("Reset sibling voice " + String(av->getVoiceIndex()));
				av->resetVoice();
			}

			numVoicesKilled++;
		}
	}

	if (allowTailOff)
	{
		LOG_SYNTH_EVENT("Kill oldest voice " + String(v->getVoiceIndex()));
		v->killVoice();
	}
	else
	{
		LOG_SYNTH_EVENT("Reset oldest voice " + String(v->getVoiceIndex()));
		v->resetVoice();
	}

	numVoicesKilled++;

	return numVoicesKilled;
	
}

void ModulatorSynth::deleteAllVoices()
{
	LockHelpers::SafeLock sl(getMainController(), LockHelpers::AudioLock, isOnAir());
    
	activeVoices.clear();
	pendingRemoveVoices.clear();
	lastStartedVoice = nullptr;
	clearVoices();
}

void ModulatorSynth::resetAllVoices()
{
    {
        LockHelpers::SafeLock sl(getMainController(), LockHelpers::AudioLock, isOnAir());
        
        for (int i = 0; i < getNumVoices(); i++)
        {
            static_cast<ModulatorSynthVoice*>(getVoice(i))->resetVoice();
        }
        
        lastStartedVoice = nullptr;
        activeVoices.clearQuick();
        pendingRemoveVoices.clearQuick();
        
    }
    
	effectChain->resetMasterEffects();
}



void ModulatorSynth::killAllVoices()
{
	// Never call this directly, use 
	//jassert(MessageManager::getInstance()->isThisTheMessageThread());

	if (isInGroup())
	{
		getGroup()->killAllVoices();
	}
	else
	{
		for (auto& v : activeVoices)
		{
			v->killVoice();
		}
	}

	effectChain->killMasterEffects();
}

void ModulatorSynth::updateShouldHaveEnvelope()
{
	if (isInGroup())
		shouldHaveEnvelope = false;
	else
		shouldHaveEnvelope = synthNeedsEnvelope();
}
    
bool ModulatorSynth::areVoicesActive() const
{
	const bool active = bypassState == false;
	const bool voicesActive = !activeVoices.isEmpty();
	const bool tailActive = effectChain->hasTailingMasterEffects();

#if LOG_KILL_EVENTS
	String x;
	x << getId() << ": " << "voicesActive: " << (voicesActive ? "true" : "false") << ", " << "tailActive: " << (tailActive ? "true" : "false");
	KILL_LOG(x);
#endif

	return active && (voicesActive || tailActive);
}

void ModulatorSynth::setVoiceLimit(int newVoiceLimit)
{
	// The voice limit must be smaller than the total amount of voices!
	//jassert(voices.size() == 0 || newVoiceLimit <= voices.size());

	voiceLimit = jlimit<int>(2, NUM_POLYPHONIC_VOICES, newVoiceLimit);

	// If the voice amount is less tha
	if (voiceLimit > 8)
		internalVoiceLimit = jmax<int>(8, (int)(getMainController()->getVoiceAmountMultiplier() * (float)voiceLimit));
	else
		internalVoiceLimit = voiceLimit;
}

void ModulatorSynth::setKillFadeOutTime(double fadeTimeMilliSeconds)
{
	killFadeTime = (float)fadeTimeMilliSeconds;

	int samples = (int)(fadeTimeMilliSeconds * 0.001 * Processor::getSampleRate());

	float killTimeFactor = powf(0.001f, (1.0f / (float)samples));

	for(int i = 0; i < voices.size(); i++)
	{
		static_cast<ModulatorSynthVoice*>(voices[i])->setKillFadeFactor(killTimeFactor);
	}
}

hise::ModulatorSynthVoice* ModulatorSynth::getVoiceToStart(const HiseEvent& m)
{
    ModulatorSynthVoice* v = nullptr;
    
	if (auto uv = getUniformVoiceHandler())
	{
		if (soundsToBeStarted.size() > 1)
		{
			debugError(this, "Can't start more than one sound when uniform mode is enabled");
			return nullptr;
		}

		auto idx = uv->getVoiceIndex(m);

		if (isPositiveAndBelow(idx, voices.size()))
		{
			v = static_cast<ModulatorSynthVoice*>(voices[idx]);
			jassert(v->isInactive());
		}
			
	}

	const bool retriggerWithDifferentChannels = getMainController()->getMacroManager().getMidiControlAutomationHandler()->getMPEData().isMpeEnabled();

	for (int j = 0; j < voices.size(); j++)
	{
		ModulatorSynthVoice* const voice = static_cast<ModulatorSynthVoice*>(voices.getUnchecked(j));

		if (voice->getCurrentlyPlayingNote() == m.getNoteNumber() // Use the untransposed number for detecting repeated notes
			&& (retriggerWithDifferentChannels || voice->isPlayingChannel(m.getChannel()))
			&& !(voice->getCurrentHiseEvent() == m))
		{
			handleRetriggeredNote(voice);
		}

		if (v == nullptr && voice->isInactive())
			v = voice;
	}

	return v;
}

ModulatorSynthVoice::ModulatorSynthVoice(ModulatorSynth* ownerSynth_):
	SynthesiserVoice(),
	ownerSynth(ownerSynth_),
	voiceIndex(ownerSynth_->getNumVoices()),
	voiceUptime(0.0),
	voiceBuffer(2, 0),
	uptimeDelta(0.0),
	killThisVoice(false),
	startUptime(DBL_MAX),
	killFadeLevel(1.0f),
	killFadeFactor(0.5f),
	isTailing(false)
{
	pitchFader.setValueWithoutSmoothing(1.0);
	gainFader.setValue(1.0f);
}

bool ModulatorSynthVoice::isPitchFadeActive() const noexcept
{
	return pitchFader.isSmoothing();
}

void ModulatorSynthVoice::applyScriptPitchFactors(float* voicePitchModulationData, int numSamples)
{
	jassert(pitchFader.isSmoothing());

	float eventPitchFactorFloat = (float)eventPitchFactor;

	while (--numSamples >= 0)
	{
		eventPitchFactor = pitchFader.getNextValue();
		*voicePitchModulationData++ *= eventPitchFactorFloat;
	}
}

bool ModulatorSynthVoice::canPlaySound(SynthesiserSound* s)
{
	return s != nullptr;
}

const HiseEvent& ModulatorSynthVoice::getCurrentHiseEvent() const
{ return currentHiseEvent; }

void ModulatorSynthVoice::addToStartOffset(uint16 delta)
{
	currentHiseEvent.setStartOffset(currentHiseEvent.getStartOffset() + delta);
}

void ModulatorSynthVoice::startNote(int, float, SynthesiserSound*, int)
{
	LOG_SYNTH_EVENT("Start Note for " + getOwnerSynth()->getId() + " with index " + String(voiceIndex));

	jassert(!currentHiseEvent.isEmpty());

	killThisVoice = false;
	isTailing = false;
	voiceUptime = 0.0;
	uptimeDelta = 0.0;
	startUptimeDelta = 0.0;
	isActive = true;
}

bool ModulatorSynthVoice::isBeingKilled() const
{
	return killThisVoice;
}

bool ModulatorSynthVoice::isTailingOff() const
{
	return isTailing;
}

void ModulatorSynthVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	SynthesiserVoice::setCurrentPlaybackSampleRate(sampleRate);

	ProcessorHelpers::increaseBufferIfNeeded(voiceBuffer, samplesPerBlock);
}

void ModulatorSynthVoice::setInactive()
{
	// Call this only on non active notes!
	jassert(uptimeDelta == 0.0);

	uptimeDelta = 0.0;
	isActive = false;
		
}

bool ModulatorSynthVoice::isInactive() const noexcept
{
	return !isActive; //uptimeDelta == 0.0;
}

void ModulatorSynthVoice::killVoice()
{
	//stopNote(true);
	killThisVoice = true;	
}

bool const ModulatorSynthVoice::shouldBeKilled() const
{
	return killThisVoice;
}

void ModulatorSynthVoice::setKillFadeFactor(float newKillFadeFactor)
{
	killFadeFactor = newKillFadeFactor;
}

void ModulatorSynthVoice::applyKillFadeout(int startSample, int numSamples)
{
	while(--numSamples >= 0)
	{
		killFadeLevel *= killFadeFactor;

		for (int i = 0; i < voiceBuffer.getNumChannels(); i++)
		{
			voiceBuffer.getWritePointer(i)[startSample] *= killFadeLevel;
		}

		startSample++;
	}
}

void ModulatorSynthVoice::applyEventVolumeFade(int startSample, int numSamples)
{
	while (--numSamples >= 0)
	{
		eventGainFactor = gainFader.getNextValue();

		for (int i = 0; i < voiceBuffer.getNumChannels(); i++)
		{
			voiceBuffer.getWritePointer(i)[startSample] *= eventGainFactor;
		}

		startSample++;
	}
}

void ModulatorSynthVoice::applyEventVolumeFactor(int startSample, int numSamples)
{
	jassert(eventGainFactor >= 0.0f && eventGainFactor < 20.0f);

	if(eventGainFactor == 0.0f)
	{
		killVoice();
	}
        
	for (int i = 0; i < voiceBuffer.getNumChannels(); i++)
	{
		FloatVectorOperations::multiply(voiceBuffer.getWritePointer(i) + startSample , eventGainFactor, numSamples);
	}
}

void ModulatorSynthVoice::pitchWheelMoved(int)
{ }

void ModulatorSynthVoice::controllerMoved(int, int)
{ }

const float* ModulatorSynthVoice::getVoiceValues(int channelIndex, int startSample) const
{
	return voiceBuffer.getReadPointer(channelIndex, startSample);
}

double ModulatorSynthVoice::getVoiceUptime() const noexcept
{ return startUptime; }

void ModulatorSynthVoice::setStartUptime(double newUptime) noexcept
{ startUptime = newUptime; }

void ModulatorSynthVoice::setScriptGainValue(float newGainValue)
{ scriptGainValue = newGainValue; }

void ModulatorSynthVoice::setScriptPitchValue(float newPitchValue)
{ scriptPitchValue = newPitchValue; }

void ModulatorSynthVoice::setTransposeAmount(int value) noexcept
{ transposeAmount = value; }

int ModulatorSynthVoice::getTransposeAmount() const noexcept
{ return transposeAmount; }

void ModulatorSynthVoice::setVolumeFade(double fadeTimeSeconds, float targetVolume)
{
	if (fadeTimeSeconds == 0.0)
	{
		eventGainFactor = targetVolume;
		gainFader.setValueWithoutSmoothing(targetVolume);
	}
	else
	{
		gainFader.setValueAndRampTime(targetVolume, getSampleRate(), fadeTimeSeconds);
	}
}

void ModulatorSynthVoice::setPitchFade(double fadeTimeSeconds, double targetPitch)
{
	if (fadeTimeSeconds == 0.0)
	{
		eventPitchFactor = targetPitch;
		pitchFader.setValueWithoutSmoothing(eventPitchFactor);
	}
	else
	{
		pitchFader.setValueAndRampTime(targetPitch, getSampleRate(), fadeTimeSeconds);
	}
}

void ModulatorSynthVoice::saveStartUptimeDelta()
{
	startUptimeDelta = uptimeDelta;
}

void ModulatorSynthVoice::setUptimeDeltaValueForBlock()
{
	uptimeDelta = startUptimeDelta * (isPitchFadeActive() ? 1.0 : eventPitchFactor);
}

void ModulatorSynthVoice::applyConstantPitchFactor(double pitchFactorToAdd)
{
	uptimeDelta *= pitchFactorToAdd;
}

const ModulatorSynth* ModulatorSynthVoice::getOwnerSynth() const noexcept
{ return ownerSynth; }

ModulatorSynth* ModulatorSynthVoice::getOwnerSynth() noexcept
{ return ownerSynth; }

void ModulatorSynthVoice::renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
	if (isActive)
    { 
		calculateBlock(startSample, numSamples);

		if (gainFader.isSmoothing())
		{
			applyEventVolumeFade(startSample, numSamples);
		}
		else if (eventGainFactor != 1.0f)
		{
			applyEventVolumeFactor(startSample, numSamples);
		}

		if(killThisVoice)
		{
			applyKillFadeout(startSample, numSamples);
		}

		const int maxChannelAmount = jmin<int>(voiceBuffer.getNumChannels(), outputBuffer.getNumChannels());

		for (int i = 0; i < maxChannelAmount; i++)
		{
			FloatVectorOperations::add(outputBuffer.getWritePointer(i, startSample), voiceBuffer.getReadPointer(i, startSample), numSamples);
		}


		// checks if any envelopes are active and in their release state and calls stopNote until they are finished.
		checkRelease();
    }
}

void ModulatorSynthVoice::setCurrentHiseEvent(const HiseEvent &m)
{
	currentHiseEvent = m;

	transposeAmount = m.getTransposeAmount();
	eventGainFactor = m.getGainFactor();
	eventPitchFactor = m.getPitchFactorForEvent();
	gainFader.setValueWithoutSmoothing(eventGainFactor);
	pitchFader.setValueWithoutSmoothing(eventPitchFactor);
}


UniformVoiceHandler::UniformVoiceHandler(ModulatorSynth* parent_): parent(parent_)
{ rebuildChildSynthList(); }

UniformVoiceHandler::~UniformVoiceHandler()
{
	childSynths.clear();
	parent = nullptr;
}

void UniformVoiceHandler::rebuildChildSynthList()
{
    Processor::Iterator<ModulatorSynth> iter(parent.get());

    Array<std::tuple<WeakReference<ModulatorSynth>, VoiceBitMap<NUM_POLYPHONIC_VOICES>>> newChilds;

    while (auto s = iter.getNextProcessor())
    {
        if (s->isInGroup())
            continue;

        if (dynamic_cast<ModulatorSynthChain*>(s) != nullptr ||
            dynamic_cast<SendContainer*>(s) != nullptr)
        {
            continue;
        }
        
        newChilds.add({ s, VoiceBitMap<NUM_POLYPHONIC_VOICES>() });
    }

    {
        SimpleReadWriteLock::ScopedWriteLock sl(arrayLock);
        std::swap(newChilds, childSynths);
    }
}

void UniformVoiceHandler::processEventBuffer(const HiseEventBuffer& eventBuffer)
{
    for (const auto& e : eventBuffer)
    {
        if (e.isAllNotesOff())
        {
            for (auto& s : childSynths)
            {
                std::get<1>(s) = {};
                memset(currentEvents.begin(), 0, sizeof(currentEvents));
            }
        }

        if (e.isNoteOn())
        {
            SimpleReadWriteLock::ScopedReadLock sl(arrayLock);

            VoiceBitMap<NUM_POLYPHONIC_VOICES> voiceMap;

            for (auto& s : childSynths)
            {
                auto& bi = std::get<1>(s);
                voiceMap |= bi;
            }

            auto voiceIndex = voiceMap.getFirstFreeBit();

            if (isPositiveAndBelow(voiceIndex, NUM_POLYPHONIC_VOICES))
            {
                for (auto& cs : childSynths)
                    std::get<1>(cs).setBit(voiceIndex, true);

                currentEvents[voiceIndex] = { e, 0 };
            }
        }
    }
}

int UniformVoiceHandler::getVoiceIndex(const HiseEvent& e)
{
    int idx = 0;

    for (const auto& s : currentEvents)
    {
        if (e == std::get<0>(s))
            return idx;

        idx++;
    }

    return -1;
}

void UniformVoiceHandler::incVoiceCounter(ModulatorSynth* s, int voiceIndex)
{
    auto& num = std::get<1>(currentEvents[voiceIndex]);
    num++;
}

void UniformVoiceHandler::decVoiceCounter(ModulatorSynth* s, int voiceIndex)
{
    for (auto& cs : childSynths)
    {
        if (std::get<0>(cs) == s)
        {
            std::get<1>(cs).setBit(voiceIndex, false);
            break;
        }
    }

    auto& num = std::get<1>(currentEvents[voiceIndex]);
    num = jmax(0, num-1);
}

void UniformVoiceHandler::cleanupAfterProcessing()
{
    int voiceIndex = 0;

    for (auto& s : currentEvents)
    {
        if (!std::get<0>(s).isEmpty() && std::get<1>(s) == 0)
        {
            std::get<0>(s) = HiseEvent();

            for (auto& cs : childSynths)
                std::get<1>(cs).setBit(voiceIndex, false);
        }

        voiceIndex++;
    }
}

bool ModulatorSynthSound::appliesToMessage(int midiChannel, const int midiNoteNumber, const int velocity)
{
	return appliesToChannel(midiChannel) && appliesToNote(midiNoteNumber) && appliesToVelocity(velocity);
}

ModulatorSynthChainFactoryType::ModulatorSynthChainFactoryType(int numVoices_, Processor* ownerProcessor):
	FactoryType(ownerProcessor),
	numVoices(numVoices_)
{
	fillTypeNameList();
}

void ModulatorSynthChainFactoryType::setNumVoices(int newNumVoices)
{
	numVoices = newNumVoices;
}

const Array<FactoryType::ProcessorEntry>& ModulatorSynthChainFactoryType::getTypeNames() const
{
	return typeNames;
}

void ModulatorSynthChainFactoryType::fillTypeNameList()
{
    ADD_NAME_TO_TYPELIST(ModulatorSampler);
    ADD_NAME_TO_TYPELIST(SineSynth);
    ADD_NAME_TO_TYPELIST(ModulatorSynthChain);
    ADD_NAME_TO_TYPELIST(GlobalModulatorContainer);
	ADD_NAME_TO_TYPELIST(WaveSynth);
	ADD_NAME_TO_TYPELIST(NoiseSynth);
	ADD_NAME_TO_TYPELIST(WavetableSynth);
	ADD_NAME_TO_TYPELIST(AudioLooper);
	ADD_NAME_TO_TYPELIST(ModulatorSynthGroup);
	ADD_NAME_TO_TYPELIST(JavascriptSynthesiser);
	ADD_NAME_TO_TYPELIST(MacroModulationSource);
	ADD_NAME_TO_TYPELIST(SendContainer);
	ADD_NAME_TO_TYPELIST(SilentSynth);
}



Processor* ModulatorSynthChainFactoryType::createProcessor	(int typeIndex, const String &id)
{
	MainController *m = getOwnerProcessor()->getMainController();

	switch(typeIndex)
	{
	case sineSynth:				return new SineSynth(m, id, numVoices);
	case waveSynth:				return new WaveSynth(m, id, numVoices);
	case noise:					return new NoiseSynth(m, id, numVoices);
	case wavetableSynth:		return new WavetableSynth(m, id, numVoices);
	case audioLooper:			return new AudioLooper(m, id, numVoices);
	case streamingSampler:		return new ModulatorSampler(m, id, numVoices);
	case modulatorSynthChain:	return new ModulatorSynthChain(m, id, numVoices);
	case modulatorSynthGroup:	return new ModulatorSynthGroup(m, id, numVoices);
	case globalModulatorContainer:	return new GlobalModulatorContainer(m, id, numVoices);
	case scriptSynth:			return new JavascriptSynthesiser(m, id, numVoices);
	case macroModulationSource: return new MacroModulationSource(m, id, numVoices);
	case sendContainer:			return new SendContainer(m, id);
	case silentSynth:			return new SilentSynth(m, id, numVoices);
	default:					jassertfalse; return nullptr;
	}
};

} // namespace hise
