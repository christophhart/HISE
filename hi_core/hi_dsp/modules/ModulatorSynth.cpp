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


ModulatorSynth::ModulatorSynth(MainController *mc, const String &id, int numVoices) :
Synthesiser(),
Processor(mc, id),
RoutableProcessor(),
gainChain(new ModulatorChain(mc, "GainModulation", numVoices, TimeModulation::GainMode, this)),
pitchChain(new ModulatorChain(mc, "PitchModulation", numVoices, TimeModulation::PitchMode, this)),
midiProcessorChain(new MidiProcessorChain(mc, "Midi Processor", this)),
effectChain(new EffectProcessorChain(this, "FX", numVoices)),
gain(0.25f),
killFadeTime(20.0f),
vuValue(0.0f),
synthUptime(0.0),
nextTimerCallbackTime(0.0),
lastStartedVoice(nullptr),
group(nullptr),
voiceLimit(numVoices),
iconColour(Colours::transparentBlack),
clockSpeed(ClockSpeed::Inactive),
lastClockCounter(0),
wasPlayingInLastBuffer(false),
pitchModulationActive(false)
{
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

	generatedMessages.ensureSize(2048);
	outputMidiBuffer.ensureSize(2048);

	setBalance(0.0f);

	pitchChain->getFactoryType()->setConstrainer(new NoGlobalEnvelopeConstrainer());

	disableChain(GainModulation, false);
	disableChain(PitchModulation, false);
	disableChain(MidiProcessor, false);
	disableChain(EffectChain, false);
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

	if (getMainController()->getMainSynthChain() == this)
	{
		handleHostInfoHiseEvents();
	}

	midiProcessorChain->renderNextHiseEventBuffer(eventBuffer, numSamples);
}

void ModulatorSynth::addProcessorsWhenEmpty()
{
	
	if (dynamic_cast<ModulatorSynthChain*>(this) == nullptr)
	{
		gainChain->getHandler()->add(new SimpleEnvelope(getMainController(),
			"DefaultEnvelope",
			voices.size(),
			Modulation::GainMode),
			nullptr);

		setEditorState("GainModulationShown", 1, dontSendNotification);
	}
}

void ModulatorSynth::renderNextBlockWithModulators(AudioSampleBuffer& outputBuffer, const HiseEventBuffer& inputMidiBuffer)
{
    ADD_GLITCH_DETECTOR("Rendering " + getId());
    
	const ScopedLock sl (getSynthLock());

	int numSamples = outputBuffer.getNumSamples();

	// The buffer must be initialized. Did you forget to call the base class prepareToPlay()
	jassert(numSamples <= internalBuffer.getNumSamples());

	int startSample = 0;

	if(checkTimerCallback())
	{
		synthTimerCallback();
	};

	initRenderCallback();

	processHiseEventBuffer(inputMidiBuffer, numSamples);

	midiInputFlag = !eventBuffer.isEmpty();

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

		const int samplesToNextMidiMessage = midiEventPos - startSample;

		const int delta = (samplesToNextMidiMessage % 8);

		const int rastered = samplesToNextMidiMessage - delta;

		if (rastered >= numSamples)
		{
			preVoiceRendering(startSample, numSamples);
			renderVoice(startSample, numSamples);
			postVoiceRendering(startSample, numSamples);
			handleHiseEvent(m);
			break;
		}

		if (rastered < 32)
		{
			handleHiseEvent(m);
			continue;
		}

		preVoiceRendering(startSample, rastered);
		renderVoice(startSample, rastered);
		postVoiceRendering(startSample, rastered);

		handleHiseEvent(m);
		startSample += rastered;
		numSamples -= rastered;
	}

	while (eventIterator.getNextEvent(m, midiEventPos, true, false))
		handleHiseEvent(m);

	effectChain->renderMasterEffects(internalBuffer);

	for (int i = 0; i < internalBuffer.getNumChannels(); i++)
	{
		const int destinationChannel = getMatrix().getConnectionForSourceChannel(i);
		
		if (destinationChannel >= 0 && destinationChannel < outputBuffer.getNumChannels())
		{
			const float thisGain = gain * (i % 2 == 0 ? leftBalanceGain : rightBalanceGain);
			FloatVectorOperations::addWithMultiply(outputBuffer.getWritePointer(destinationChannel, 0), internalBuffer.getReadPointer(i, 0), thisGain, outputBuffer.getNumSamples());
		}

		if (getMatrix().isEditorShown())
		{
			float gainValues[NUM_MAX_CHANNELS];

			for (int i = 0; i < internalBuffer.getNumChannels(); i++)
			{
				gainValues[i] = internalBuffer.getMagnitude(i, 0, internalBuffer.getNumSamples());
			}

			getMatrix().setGainValues(gainValues, true);

			for (int i = 0; i < outputBuffer.getNumChannels(); i++)
			{
				gainValues[i] = outputBuffer.getMagnitude(i, 0, outputBuffer.getNumSamples());
			}

			getMatrix().setGainValues(gainValues, false);
		}
	}

	handlePeakDisplay(outputBuffer.getNumSamples());
}

void ModulatorSynth::renderVoice(int startSample, int numThisTime)
{
    ADD_GLITCH_DETECTOR("Rendering voices for " + getId());
    
	for (int i = voices.size(); --i >= 0;)
	{
		ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(voices[i]);

		if (!v->isInactive())
		{
			v->renderNextBlock(internalBuffer, startSample, numThisTime);
		}
	}

};

	
void ModulatorSynth::handlePeakDisplay(int numSamplesInOutputBuffer)
{
#if ENABLE_ALL_PEAK_METERS
	
	currentValues.outL = gain * internalBuffer.getMagnitude(0, 0, numSamplesInOutputBuffer) * leftBalanceGain;
	currentValues.outR = gain * internalBuffer.getMagnitude(1, 0, numSamplesInOutputBuffer) * rightBalanceGain;
	
#else

	if (this != getMainController()->getMainSynthChain())
	{
		currentValues.outL = gain * internalBuffer.getMagnitude(0, 0, numSamplesInOutputBuffer) * leftBalanceGain;
		currentValues.outR = gain * internalBuffer.getMagnitude(1, 0, numSamplesInOutputBuffer) * rightBalanceGain;
	}

#endif
}


void ModulatorSynth::handleHiseEvent(const HiseEvent& m)
{
	preHiseEventCallback(m);
	
	const int channel = 1;

	if (m.isNoteOn())
	{
		noteOn(m);
	}
	else if (m.isNoteOff())
	{
		noteOff(m);
	}
	else if (m.isAllNotesOff())
	{
		allNotesOff(channel, true);
	}
	else if (m.isPitchWheel())
	{
		const int wheelPos = m.getPitchWheelValue();
		lastPitchWheelValues[channel - 1] = wheelPos;
		handlePitchWheel(channel, wheelPos);
	}
	else if (m.isAftertouch())
	{
		handleAftertouch(channel, m.getNoteNumber(), m.getAfterTouchValue());
	}
	else if (m.isChannelPressure())
	{
		handleChannelPressure(channel, m.getChannelPressureValue());
	}
	else if (m.isController())
	{
		handleController(channel, m.getControllerNumber(), m.getControllerValue());
	}
	else if (m.isVolumeFade())
	{
		handleVolumeFade(m.getEventId(), m.getFadeTime(), m.getGainFactor());
	}
	else if (m.isPitchFade())
	{
		handlePitchFade(m.getEventId(), m.getFadeTime(), m.getPitchFactorForEvent());
	}
}

void ModulatorSynth::handleHostInfoHiseEvents()
{
	int ppqTimeStamp = -1;

	// Check if ppqPosition should be added
	static const Identifier ppqPosition("ppqPosition");
	static const Identifier isPlaying("isPlaying");

	const bool hostIsPlaying = getMainController()->getHostInfoObject()->getProperty(isPlaying);

	if (hostIsPlaying && clockSpeed != ClockSpeed::Inactive)
	{
		double ppq = getMainController()->getHostInfoObject()->getProperty(ppqPosition);

		const double bufferAsMilliseconds = getBlockSize() / getSampleRate();
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

			if (remainingSamples < getBlockSize())
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
		pos.setTimeStamp((uint16)ppqTimeStamp);

		eventBuffer.addEvent(pos);
	}
}

void ModulatorSynth::handleVolumeFade(int eventId, int fadeTimeMilliseconds, float gain)
{
	const double fadeTimeSeconds = (double)fadeTimeMilliseconds / 1000.0;

	for (int i = voices.size(); --i >= 0;)
	{
		ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(voices[i]);

		if (!v->isInactive() && v->getCurrentHiseEvent().getEventId() == eventId)
		{
			v->setVolumeFade(fadeTimeSeconds, gain);
		}
	}
}

void ModulatorSynth::handlePitchFade(int eventId, int fadeTimeMilliseconds, double pitchFactor)
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

void ModulatorSynth::preHiseEventCallback(const HiseEvent &e)
{
	if (e.isAllNotesOff()) stopSynthTimer();

	gainChain->handleHiseEvent(e);
	pitchChain->handleHiseEvent(e);
	effectChain->handleHiseEvent(e);

}

bool ModulatorSynth::soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity)
{
	return sound->appliesToMessage(midiChannel, midiNoteNumber, (int)(velocity * 127));
};



	
void ModulatorSynth::startVoiceWithHiseEvent(ModulatorSynthVoice* voice, SynthesiserSound *sound, const HiseEvent &e)
{
	
	voice->setCurrentHiseEvent(e);

	Synthesiser::startVoice(static_cast<SynthesiserVoice*>(voice), sound, e.getChannel(), e.getNoteNumber(), e.getFloatVelocity());
}

void ModulatorSynth::preStartVoice(int voiceIndex, int noteNumber)
{
	lastStartedVoice = static_cast<ModulatorSynthVoice*>(getVoice(voiceIndex));



	gainChain->startVoice(voiceIndex);
	pitchChain->startVoice(voiceIndex);
	effectChain->startVoice(voiceIndex, noteNumber);
}

void ModulatorSynth::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ScopedLock sl(getSynthLock());

	if(sampleRate != -1.0)
	{
		pitchBuffer = AudioSampleBuffer(1, samplesPerBlock); // should be enough
		internalBuffer = AudioSampleBuffer(getMatrix().getNumSourceChannels(), samplesPerBlock);
		gainBuffer = AudioSampleBuffer(1, samplesPerBlock);
		

		for(int i = 0; i < getNumVoices(); i++)
		{
			static_cast<ModulatorSynthVoice*>(getVoice(i))->prepareToPlay(sampleRate, samplesPerBlock);
		}

		vuMerger.limitFromBlockSizeToFrameRate(sampleRate, samplesPerBlock);

		Synthesiser::setCurrentPlaybackSampleRate(sampleRate);

		Processor::prepareToPlay(sampleRate, samplesPerBlock);
		
		midiProcessorChain->prepareToPlay(sampleRate, samplesPerBlock);
		gainChain->prepareToPlay(sampleRate, samplesPerBlock);
		pitchChain->prepareToPlay(sampleRate, samplesPerBlock);

		CHECK_COPY_AND_RETURN_12(effectChain);

		effectChain->prepareToPlay(sampleRate, samplesPerBlock);

		setKillFadeOutTime(killFadeTime);
	}
}

	
void ModulatorSynth::numSourceChannelsChanged()
{
	ScopedLock sl(getSynthLock());

	if (internalBuffer.getNumSamples() != 0)
	{
		jassert(getBlockSize() > 0);
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

void ModulatorSynth::noteOn(const HiseEvent &m)
{
    ADD_GLITCH_DETECTOR("Note on callback for " + getId());
    
    const ScopedLock sl (getSynthLock());

	jassert(m.isNoteOn());

	const int midiChannel = m.getChannel();
	const int midiNoteNumber = m.getNoteNumber();
	const int transposedMidiNoteNumber = midiNoteNumber + m.getTransposeAmount();
	const float velocity = m.getFloatVelocity();

    for (int i = sounds.size(); --i >= 0;)
    {
		SynthesiserSound *s = sounds.getUnchecked(i);
        ModulatorSynthSound *sound = static_cast<ModulatorSynthSound*>(s);

		if (soundCanBePlayed(sound, midiChannel, transposedMidiNoteNumber, velocity))
        {
            // If hitting a note that's still ringing, stop it first (it could be
            // still playing because of the sustain or sostenuto pedal).
            for (int j = voices.size(); --j >= 0;)
            {
                ModulatorSynthVoice* const voice = static_cast<ModulatorSynthVoice*>(voices.getUnchecked (j));

				const bool voiceIsActive = voice->isPlayingChannel(midiChannel) && !voice->isBeingKilled();

				// if the voiceLimit is reached, kill the voice!

				if(voiceIsActive && j >= (voiceLimit - 1)) 
				{
					killLastVoice();
				}

                else if (voice->getCurrentlyPlayingNote() == midiNoteNumber // Use the untransposed number for detecting repeated notes
                     && voice->isPlayingChannel (midiChannel))
				{
					handleRetriggeredNote(voice);
				}
            }

			ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(findFreeVoice (sound, midiChannel, midiNoteNumber, isNoteStealingEnabled()));

			if( v != nullptr)
			{
				const int voiceIndex = v->getVoiceIndex();

				jassert(voiceIndex != -1);

				v->setStartUptime(getMainController()->getUptime());

				preStartVoice(voiceIndex, transposedMidiNoteNumber);

				startVoiceWithHiseEvent (v, sound, m);
			}

			// Deactivates starting of more than one voice per synth
			//break;
        }
	}
}

void ModulatorSynth::noteOff(const HiseEvent &m)
{
	const ScopedLock sl(getSynthLock());

	const int midiNoteNumber = m.getNoteNumber();
	
	float velocity = m.getFloatVelocity();
	const int midiChannel = m.getChannel();

	for (int i = voices.size(); --i >= 0;)
	{
		SynthesiserVoice* const voice = voices.getUnchecked(i);

		if (voice->getCurrentlyPlayingNote() == midiNoteNumber
			&& voice->isPlayingChannel(midiChannel))
		{
			if (SynthesiserSound* const sound = voice->getCurrentlyPlayingSound())
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


void ModulatorSynth::enablePitchModulation(bool shouldBeEnabled)
{
	pitchModulationActive = shouldBeEnabled;

	if (allowEmptyPitchValues() || shouldBeEnabled)
	{
		for (int i = 0; i < voices.size(); i++)
		{
			static_cast<ModulatorSynthVoice*>(getVoice(i))->enablePitchModulation(shouldBeEnabled);
		}
	}
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

void ModulatorSynthVoice::resetVoice()
{
	clearCurrentNote();

	ModulatorSynth *os = getOwnerSynth();

	ModulatorChain *g = static_cast<ModulatorChain*>(os->getChildProcessor(ModulatorSynth::GainModulation));
	ModulatorChain *p = static_cast<ModulatorChain*>(os->getChildProcessor(ModulatorSynth::PitchModulation));
	EffectProcessorChain *e = static_cast<EffectProcessorChain*>(os->getChildProcessor(ModulatorSynth::EffectChain));

	g->reset(voiceIndex);
	p->reset(voiceIndex);
	e->reset(voiceIndex);

	uptimeDelta = 0.0;
	voiceUptime = 0.0;
	startUptime = DBL_MAX;

	isTailing = false;
    isActive = false;

	killThisVoice = false;
	killFadeLevel = 1.0f;

	currentHiseEvent = HiseEvent();

	getOwnerSynth()->getMainController()->decreaseVoiceCounter();
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

	if( ! g->isPlaying(voiceIndex) )
	{
		EffectProcessorChain *e = static_cast<EffectProcessorChain*>(os->getChildProcessor(ModulatorSynth::EffectChain));
		
		// Skip killing if effect is ringing off
		if ((e->hasTail() && e->isTailingOff())) return;

		resetVoice();
	}
}


int ModulatorSynthVoice::getVoiceIndex() const
{
	return voiceIndex;
}

void ModulatorSynthVoice::stopNote(float, bool)
{
    
		ModulatorSynth *os = getOwnerSynth();
		
		ModulatorChain *c = static_cast<ModulatorChain*>(os->getChildProcessor(ModulatorSynth::GainModulation));
		ModulatorChain *p = static_cast<ModulatorChain*>(os->getChildProcessor(ModulatorSynth::PitchModulation));
		EffectProcessorChain *e = static_cast<EffectProcessorChain*>(os->getChildProcessor(ModulatorSynth::EffectChain));

		isTailing = true;

		c->stopVoice(voiceIndex);
		p->stopVoice(voiceIndex);
		e->stopVoice(voiceIndex);

		checkRelease();
};

void ModulatorSynth::handleRetriggeredNote(ModulatorSynthVoice *voice)
{
	voice->killVoice();
};

bool ModulatorSynth::getMidiInputFlag()
{
	if (midiInputFlag)
	{
		midiInputFlag = false;
		return true;
	}

	return false;
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



	
void ModulatorSynth::killLastVoice()
{
	ModulatorSynthVoice *oldest = nullptr;
	double oldestUptime = DBL_MAX;

	// search all tailing notes
	for(int i = 0; i < voices.size(); i++)
	{
		ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(voices[i]);

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
		oldest->killVoice();
		return;
	}

	// search all notes
	for(int i = 0; i < voices.size(); i++)
	{
		ModulatorSynthVoice *v = static_cast<ModulatorSynthVoice*>(voices[i]);

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
		oldest->killVoice();
		return;
	}
};

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

void ModulatorSynthVoice::renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples)
{
	if (isActive)
    { 
		if(isPitchModulationActive()) calculateVoicePitchValues(startSample, numSamples);

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
	scriptPitchActive = eventPitchFactor != 1.0;
}

void ModulatorSynthChainFactoryType::fillTypeNameList()
{
	ADD_NAME_TO_TYPELIST(SineSynth);
	ADD_NAME_TO_TYPELIST(WaveSynth);
	ADD_NAME_TO_TYPELIST(NoiseSynth);
	ADD_NAME_TO_TYPELIST(WavetableSynth);
	ADD_NAME_TO_TYPELIST(AudioLooper);
	ADD_NAME_TO_TYPELIST(ModulatorSampler);
	ADD_NAME_TO_TYPELIST(ModulatorSynthChain);
	ADD_NAME_TO_TYPELIST(ModulatorSynthGroup);
	ADD_NAME_TO_TYPELIST(GlobalModulatorContainer);
	ADD_NAME_TO_TYPELIST(JavascriptModulatorSynth);
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
	case scriptSynth:			return new JavascriptModulatorSynth(m, id, numVoices);
	default:					jassertfalse; return nullptr;
	}
};
