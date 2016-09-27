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

#ifndef MODULATORSYNTH_H_INCLUDED
#define MODULATORSYNTH_H_INCLUDED


class ModulatorSynthChain;
class ModulatorSynthEditor;
class ModulatorSynthGroup;
class ModulatorSynthSound;
class ModulatorSynthVoice;

typedef HiseEventBuffer EVENT_BUFFER_TO_USE;

/** A ModulatorSynth is a synthesiser with a ModulatorChain for volume and pitch that allows
*	modulation of these parameters.
*
*	Instead of renderNextBlock(), simply call renderNextBlockWithModulators() and it processes the chains.
*	Also you have to subclass the used voice type from ModulatorSynthVoice and make sure it uses the voice modulation values in its renderNextBlock().
*/
class ModulatorSynth: public Synthesiser,
					  public Processor,
					  public RoutableProcessor
{
public:

	enum Parameters
	{
		Gain = 0,
		Balance,
		VoiceLimit,
		KillFadeTime,
		numModulatorSynthParameters
	};

	/** These are the three Chains that the ModulatorSynth uses. */
	enum InternalChains
	{
		MidiProcessor = 0,
		GainModulation,
		PitchModulation,
		
		EffectChain,
		numInternalChains
	};

	enum EditorState
	{
		OverviewFolded = Processor::numEditorStates,
		MidiProcessorShown,
		GainModulationShown,
		PitchModulationShown,
		
		EffectChainShown,
		numEditorStates
	};

	enum ClockSpeed
	{
		Inactive = 0xFFF,
		ThirtyTwos = 3,
		Sixteens = 2,
		Eighths = 1,
		Quarters = 0,
		Half = -1,
		Bar = -2
	};

	ModulatorSynth(MainController *mc, const String &id, int numVoices);;

	virtual ~ModulatorSynth()
	{
		midiProcessorChain = nullptr;
		gainChain = nullptr;
		pitchChain = nullptr;
		effectChain = nullptr;
	};

	virtual void restoreFromValueTree(const ValueTree &v) override
	{
		

		RESTORE_MATRIX();

		loadAttribute(Gain, "Gain");
		loadAttribute(Balance, "Balance");
		loadAttribute(VoiceLimit, "VoiceLimit");
		loadAttribute(KillFadeTime, "KillFadeTime");

		iconColour = Colour::fromString(v.getProperty("IconColour", Colours::transparentBlack.toString()).toString());

		Processor::restoreFromValueTree(v);
	};

	

	virtual ValueTree exportAsValueTree() const override
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
	};

	virtual float getAttribute(int parameterIndex) const override
	{
		switch(parameterIndex)
		{
		case Gain:			return gain;
		case Balance:		return balance;
		case VoiceLimit:	return (float)voiceLimit;
		case KillFadeTime:	return killFadeTime;
		default:			jassertfalse; return 0.0f;
		}
	}

	virtual void setInternalAttribute(int parameterIndex, float newValue) override
	{
		switch(parameterIndex)
		{
		case Gain:			setGain(newValue); break;
		case Balance:		setBalance(newValue); break;
		case VoiceLimit:	setVoiceLimit((int)newValue); break;
		case KillFadeTime:	setKillFadeOutTime((double)newValue); break;
		default:			jassertfalse; return;
		}
	};
    
    virtual float getDefaultValue(int parameterIndex) const override
    {
        switch(parameterIndex)
        {
            case Gain:			return 1.0;
            case Balance:		return 0.0;
            case VoiceLimit:	return (float)64;
            case KillFadeTime:	return 20;
            default:			jassertfalse; return 0.0f;
        }
    }

	virtual void synthTimerCallback()
	{
        ADD_GLITCH_DETECTOR(getId() + " timer callback");

		const int offsetInBuffer = (int)((getMainController()->getUptime() - nextTimerCallbackTime) * getSampleRate());

		midiProcessorChain->synthTimerCallback(offsetInBuffer);

		if(nextTimerCallbackTime != 0.0) startSynthTimer(synthTimerInterval);
	}

	virtual void startSynthTimer(double interval)
	{
		
		if(interval < 0.04)
		{
			nextTimerCallbackTime = 0.0;
			jassertfalse;
			debugToConsole(this, "Go easy on the timer!");
			return;
		};

		synthTimerInterval = interval;

		if(interval != 0.0) nextTimerCallbackTime = getMainController()->getUptime() + synthTimerInterval;
	};

	virtual void stopSynthTimer()
	{
		nextTimerCallbackTime = 0.0;
	}

	bool isLastStartedVoice(ModulatorSynthVoice *voice)
	{
		return voice == lastStartedVoice;
	};


	ModulatorSynthVoice* getLastStartedVoice() const
	{
		return lastStartedVoice;
	}

	virtual ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	/** Call this instead of Synthesiser::renderNextBlock to let the ModulatorChains to their work. 
	*
	*	This only renders the TimeVariantModulators (like a master effect) and calculates the voice modulation, so make sure you actually apply
	*	the voice modulation in the subclassed ModulatorSynthVoice callback.
	*/
	virtual void renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi);

	/** This method is called to handle all modulatorchains just before the voice rendering. */
	virtual void preVoiceRendering(int startSample, int numThisTime)
	{
		// calculate the variant pitch values before the voices are rendered.
		pitchChain->renderNextBlock(pitchBuffer, startSample, numThisTime);

		if( !isChainDisabled(EffectChain)) effectChain->preRenderCallback(startSample, numThisTime);
	};

	void setIconColour(Colour newIconColour)
	{
		iconColour = newIconColour;
	};

	Colour getIconColour() const
	{
		return iconColour;
	}

	double getSampleRate() const
	{
		return Processor::getSampleRate();
	}

	/** This method is called to actually render all voices. It operates on the internal buffer of the ModulatorSynth. */
	virtual void renderVoice(int startSample, int numThisTime);

	/** This method is called to handle all modulatorchains after the voice rendering and handles the GUI metering. It assumes stereo mode.
	*
	*	The rendered buffer is supplied as reference to be able to apply changes here after all voices are rendered (eg. gain).
	*/
	virtual void postVoiceRendering(int startSample, int numThisTime)
	{

		// Calculate the timeVariant modulators
		gainChain->renderNextBlock(gainBuffer, startSample, numThisTime);

		// Apply all gain modulators to the rendered voices
		for (int i = 0; i < internalBuffer.getNumChannels(); i++)
		{
			FloatVectorOperations::multiply(internalBuffer.getWritePointer(i, startSample), gainBuffer.getReadPointer(0, startSample), numThisTime);
		}

		if( !isChainDisabled(EffectChain) ) effectChain->renderNextBlock(internalBuffer, startSample, numThisTime);		
	};

	virtual void handlePeakDisplay(int numSamplesInOutputBuffer);

	void setPeakValues(float l, float r)
	{
		currentValues.outL = l;
		currentValues.outR = r;
	}



	virtual Colour getColour() const override {	return Colours::black.withBrightness(0.45f); };

	

	/** Same functionality as Synthesiser::handleMidiEvent(), but sends the midi event to all Modulators in the chains. */
	//void handleHiseEvent(const HiseEvent &m) final override;

	void handleHiseEvent(const HiseEvent& e);

	void handleHostInfoHiseEvents();

	void handleVolumeFade(int eventId, int fadeTimeMilliseconds, float gain);
	
	void handlePitchFade(int eventId, int fadeTimeMilliseconds, double pitchFactor);

	virtual void preHiseEventCallback(const HiseEvent &e);

	/** Use this to do any special handling before the voice gets started. */
	virtual void preStartVoice(int voiceIndex, int noteNumber);

	/** This sets up the synth and the ModulatorChains. 
	*
	*	Call this instead of Synthesiser::setCurrentPlaybackSampleRate(). 
	*	It also sets the ModulatorChain's voice amount, so be sure you add all SynthesiserVoices before you call this function.
	*/
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock);

	void numSourceChannelsChanged() override;

	void numDestinationChannelsChanged() override;

	void setBypassed(bool shouldBeBypassed) noexcept override
	{
		ScopedLock sl(getSynthLock());

		Processor::setBypassed(shouldBeBypassed);
		
		midiProcessorChain->sendAllNoteOffEvent();
		
		for (int i = 0; i < getNumInternalChains(); i++)
		{
			ModulatorChain *chain = dynamic_cast<ModulatorChain*>(getChildProcessor(i));

			if (chain != nullptr)
			{
				chain->handleHiseEvent(HiseEvent(HiseEvent::Type::AllNotesOff, 0, 0, 1));
			}
		}

		allNotesOff(1, false);
	};

	void disableChain(InternalChains chainToDisable, bool shouldBeDisabled)
	{
		disabledChains.setBit(chainToDisable, shouldBeDisabled);
	}

	bool isChainDisabled(InternalChains chain) const
	{
		return disabledChains[chain];
	};

	void enablePitchModulation(bool shouldBeEnabled);

	bool isPitchModulationActive() const noexcept{ return pitchModulationActive; }

	/** Kills the note with the specified note number. 
	*
	*	This stops the note with a small fade out (instead of noteoff which can result in very long release times 
	*	if the envelope says so
	*/
	void killAllVoicesWithNoteNumber(int noteNumber);

	/** Kills the voice that is playing for the longest time. */
	void killLastVoice();

	void setVoiceLimit(int newVoiceLimit)
	{
		// The voice limit must be smaller than the total amount of voices!
		//jassert(voices.size() == 0 || newVoiceLimit <= voices.size());

		voiceLimit = jmin<int>(newVoiceLimit, NUM_POLYPHONIC_VOICES);
	}

	void setKillFadeOutTime(double fadeTimeSeconds);

	/** Checks if the message fits the sound, but can be overriden to implement other group start logic. */
	virtual bool soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity);

	void startVoiceWithHiseEvent(ModulatorSynthVoice* voice, SynthesiserSound *sound, const HiseEvent &e);

	/** Same functionality as Synthesiser::noteOn(), but calls calculateVoiceStartValue() if a new voice is started. */
	void noteOn(const HiseEvent &m);

	void noteOn(int midiChannel, int midiNoteNumber, float velocity) override
	{
		jassertfalse;

		HiseEvent m(HiseEvent::Type::NoteOn, (uint8)midiNoteNumber, (uint8)(velocity * 127.0f), (uint8)midiChannel);

		noteOn(m);
	}

	virtual void noteOff(const HiseEvent &m);

	/** Returns the voice index for the voice (the index in the internal voice array). This is needed for the ModulatorChains to know which voice is started. */
	int getVoiceIndex(const SynthesiserVoice *v) const;;

	

	void processHiseEventBuffer(const HiseEventBuffer &inputBuffer, int numSamples);


	/** This adds a midi message to a internal buffer which will be injected at the next buffer (at the end). */
	void addGeneratedMidiMessageToNextBlock(const MidiMessage &m)
	{
		generatedMessages.clear();
		generatedMessages.addEvent(m, 0);
	}

	/** Adds a SimpleEnvelope to a empty ModulatorSynth to prevent clicks. */
	virtual void addProcessorsWhenEmpty();

	/** Clears the internal buffer. This must be called by subclasses for every renderNextBlockWithModulators. */
	virtual void initRenderCallback()
	{
		internalBuffer.clear();
	};

	/** sets the gain of the ModulatorSynth from 0.0 to 1.0. */
	void setGain(float newGain)
	{
		gain = newGain;
	};

	/** sets the balance from -1.0 (left) to 1.0 (right) and applies a equal power pan rule. */
	void setBalance(float newBalance)
	{
		ScopedLock sl(getSynthLock());

		balance = newBalance;

		leftBalanceGain = BalanceCalculator::getGainFactorForBalance((balance * 100.0f), true);
		rightBalanceGain = BalanceCalculator::getGainFactorForBalance((balance * 100.0f), false);
	};

	/** Returns the calculated (equal power) pan value for either the left or the right channel. */
	float getBalance(bool getRightChannelGain) const 
	{
		return getRightChannelGain ? rightBalanceGain : leftBalanceGain;
	};

	/** Returns the gain of the ModulatorSynth from 0.0 to 1.0. */
	float getGain() const {	return gain; };

	void setScriptGainValue(int voiceIndex, float gainValue) noexcept;

	void setScriptPitchValue(int voiceIndex, double pitchValue) noexcept;

	/** Sets the parent group. This can only be called once, since synths are not supposed to change their parents. */
	void setGroup(ModulatorSynthGroup *parent)
	{
		jassert(group == nullptr);
		group = parent;
		disableChain(MidiProcessor, true);
		disableChain(EffectChain, true);
	};

	/** Returns the ModulatorSynthGroup that this ModulatorSynth belongs to. */
	ModulatorSynthGroup *getGroup() const {	return group; };

	/** Checks if the Synth was added to a group. */
	bool isInGroup() const { return group != nullptr;};

	/** Returns the index of the child synth if it resides in a group and -1 if not. */
	int getIndexInGroup() const;

	/** Sets the interval for the internal clock callback. */
	void setClockSpeed(ClockSpeed newClockSpeed)
	{
		clockSpeed = newClockSpeed;
	}

	bool allowEmptyPitchValues() const
	{
		const bool isGroup = ProcessorHelpers::is<ModulatorSynthGroup>(this);

		return !(isGroup || isInGroup());
	}

	/** Returns the specified chain. You have to static_cast it to the subclass in most cases, since it can be both MidiProcessorChains or ModulatorChains. */
	virtual Processor *getChildProcessor(int processorIndex) override
	{
		jassert(processorIndex < numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	/** Returns the specified chain. You have to static_cast it to the subclass in most cases, since it can be both MidiProcessorChains or ModulatorChains. */
	virtual const Processor *getChildProcessor(int processorIndex) const override
	{
		jassert(processorIndex < numInternalChains);

		switch(processorIndex)
		{
		case GainModulation:	return gainChain;
		case PitchModulation:	return pitchChain;
		case MidiProcessor:		return midiProcessorChain;
		case EffectChain:		return effectChain;
		default:				jassertfalse; return nullptr;
		}
	};

	/** Returns 4 (the number of internal chains), but can be overriden. */
	virtual int getNumChildProcessors() const override { return numInternalChains;	};

	virtual int getNumInternalChains() const override {return numInternalChains; };

	/** Returns the pointer to the calculated pitch buffers for the ModulatorSynthVoice's render callback. */
	const float *getConstantPitchValues() const { return pitchBuffer.getReadPointer(0);	};

	/** returns the lock the synth is using. */
	const CriticalSection &getSynthLock() const
	{
		return lock;
		//return getMainController()->getLock();
	};

	/** specifies the behaviour when a note is started that is already ringing. By default, it is killed, but you can overwrite it to make something else. */
	virtual void handleRetriggeredNote(ModulatorSynthVoice *voice);

	/** Calculates the voice values with the GainModulationChain and returns a read pointer to the values. */
	const float *calculateGainValuesForVoice(int voiceIndex, float scriptGainValue, int startSample, int numSamples)
	{
		gainChain->renderVoice(voiceIndex, startSample, numSamples);
		float *data = gainChain->getVoiceValues(voiceIndex);
		if (scriptGainValue != 1.0f) FloatVectorOperations::multiply(data + startSample, scriptGainValue, numSamples);

		return data;
	};

	/** calculates the voice pitch values. You can get the values with getPitchValues for voice. */
	void calculatePitchValuesForVoice(int voiceIndex, float scriptPitchValue, int startSample, int numSamples)
	{
        pitchChain->renderVoice(voiceIndex, startSample, numSamples);
		float *voicePitchValues = pitchChain->getVoiceValues(voiceIndex);
		const float *timeVariantPitchValues = getConstantPitchValues();
		FloatVectorOperations::multiply(voicePitchValues, timeVariantPitchValues, startSample + numSamples);
		if (scriptPitchValue != 1.0f) FloatVectorOperations::multiply(voicePitchValues, scriptPitchValue, startSample + numSamples);
	}

	/** Returns a read pointer to the calculated pitch values. */
	const float *getPitchValuesForVoice(int voiceIndex) const { return pitchChain->getVoiceValues(voiceIndex);};

	/** Returns a read pointer to the calculated pitch values. Used by Synthgroups to render their pitch values on the voice value. */
	float *getPitchValuesForVoice(int voiceIndex) { return pitchChain->getVoiceValues(voiceIndex);};

	
	HiseEventBuffer eventBuffer;


	AudioSampleBuffer internalBuffer;

	MidiBuffer generatedMessages;
	MidiBuffer outputMidiBuffer;

	UpdateMerger vuMerger;

	AudioSampleBuffer pitchBuffer;
	AudioSampleBuffer gainBuffer;

	ScopedPointer<ModulatorChain> gainChain;
	ScopedPointer<ModulatorChain> pitchChain;
	ScopedPointer<MidiProcessorChain> midiProcessorChain;
	ScopedPointer<EffectProcessorChain> effectChain;

	BigInteger disabledChains;

	bool getMidiInputFlag();

protected:

	bool checkTimerCallback() const noexcept
	{
		return nextTimerCallbackTime != 0.0 && getMainController()->getUptime() > nextTimerCallbackTime;
	};
	
	// Used to display the playing position
	ModulatorSynthVoice *lastStartedVoice;

private:

	Colour iconColour;

	ClockSpeed clockSpeed;

	int lastClockCounter;

	int voiceLimit;

	double synthUptime;
	double synthTimerInterval;
	double nextTimerCallbackTime;

	ModulatorSynthGroup *group;

	bool midiInputFlag;

	bool wasPlayingInLastBuffer;
	bool pitchModulationActive;

	float gain;

	float balance;
	float leftBalanceGain;
	float rightBalanceGain;

	float vuValue;

	float killFadeTime;
	

	// this stores the variant pitch values to be retrieved within ModulatorSynthVoice::renderNextBlock()
	
};

/** This voice calculates the ModulatorChains of the ModulatorSynth it belongs to.
*
*	Since the pitch information and the gain information is processed differently for each voice type,
*	you need this base class to get the data calculated by the owner ModulatorSynth.
*	This class isn't purely virtual as the base class SynthesiserVoice is. Instead, it acts as sine generator.
*	Just copy the behaviour in the subclass and do what you like.
*
*/
class ModulatorSynthVoice: public SynthesiserVoice
{
public:

	ModulatorSynthVoice(ModulatorSynth *ownerSynth_):
		SynthesiserVoice(),
		ownerSynth(ownerSynth_),
		voiceIndex(ownerSynth_->getNumVoices()),
		pitchModulationActive(ownerSynth_->isPitchModulationActive()),
		voiceUptime(0.0),
		uptimeDelta(0.0),
		killThisVoice(false),
		startUptime(DBL_MAX),
		killFadeLevel(1.0f),
		killFadeFactor(0.5f),
		isTailing(false)
	{};

	/** If not overriden, this uses a sine generator for an example usage of this voice class. */
	virtual void renderNextBlock (AudioSampleBuffer& outputBuffer,
                                  int startSample,
                                  int numSamples) override;


	virtual void calculateBlock(int startSample, int numSamples) = 0;
	
	void calculateVoicePitchValues(int startSample, int numSamples)
	{
		getOwnerSynth()->calculatePitchValuesForVoice(voiceIndex, (float)scriptPitchValue, startSample, numSamples);

		if (pitchFader.isSmoothing())
		{
			float* pitchValues = getVoicePitchValues() + startSample;

			while (--numSamples >= 0)
			{
				eventPitchFactor = pitchFader.getNextValue();
				*pitchValues++ *= eventPitchFactor;
			}
		}
		else if (scriptPitchActive)
		{
			float* pitchValues = getVoicePitchValues() + startSample;

			FloatVectorOperations::multiply(pitchValues, eventPitchFactor, numSamples);
		}
	}

	

	float *getVoicePitchValues() 
	{
		return isPitchModulationActive() ? getOwnerSynth()->getPitchValuesForVoice(voiceIndex) : nullptr;
	}

	const float *getVoiceGainValues(int startSample, int numSamples)
	{
		return getOwnerSynth()->calculateGainValuesForVoice(voiceIndex, scriptGainValue, startSample, numSamples);
	}

	/** This only checks if the sound is valid, but you can override this with the desired behaviour. */
	virtual bool canPlaySound(SynthesiserSound *s) override
	{
		return s != nullptr;
	};

	
	void setCurrentHiseEvent(const HiseEvent &m);

	const HiseEvent &getCurrentHiseEvent() const { return currentHiseEvent; }

	/** This calculates the angle delta. For this synth, it detects the sine frequency, but you can override it to make something else. */
	virtual void startNote (int /*midiNoteNumber*/, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/)
	{
		jassert(!currentHiseEvent.isEmpty());

		getOwnerSynth()->getMainController()->increaseVoiceCounter();

		killThisVoice = false;
		isTailing = false;
		voiceUptime = 0.0;
		uptimeDelta = 0.0;
        isActive = true;
	}
   
	bool isBeingKilled() const
	{
		return killThisVoice;
	}

	bool isTailingOff() const
	{
		return isTailing;
	};

	virtual void prepareToPlay(double sampleRate, int samplesPerBlock)
	{
		SynthesiserVoice::setCurrentPlaybackSampleRate(sampleRate);

		voiceBuffer = AudioSampleBuffer(2, samplesPerBlock);
	}

	virtual void setInactive()
	{
		// Call this only on non active notes!
		jassert(uptimeDelta == 0.0);

		uptimeDelta = 0.0;
        isActive = false;
		
	};

	virtual void resetVoice();

	bool isInactive() const noexcept
	{
        return !isActive; //uptimeDelta == 0.0;
	};

	/** This handles the voice stop. If any envelopes are active, the voice keeps playing and repeatedly call checkRelease(), until they are finished. */
	virtual void stopNote(float velocity, bool allowTailOff) override;

	/** This kills the note with a short fade time. */
	void killVoice()
	{
		//stopNote(true);
		killThisVoice = true;	
	}

	void setKillFadeFactor(float newKillFadeFactor)
	{
		killFadeFactor = newKillFadeFactor;
	}

	void applyKillFadeout(int startSample, int numSamples)
	{
		//float *l = voiceBuffer.getWritePointer(0, 0);
		//float *r = voiceBuffer.getWritePointer(1, 0);

		while(--numSamples >= 0)
		{
			killFadeLevel *= killFadeFactor;

			for (int i = 0; i < voiceBuffer.getNumChannels(); i++)
			{
				voiceBuffer.getWritePointer(i)[startSample] *= killFadeLevel;
			}

			//l[startSample] *= killFadeLevel;
			//r[startSample] *= killFadeLevel;

			startSample++;
		}
	}

	void applyEventVolumeFade(int startSample, int numSamples)
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

	void applyEventVolumeFactor(int startSample, int numSamples)
	{
		jassert(eventGainFactor >= 0.0f && eventGainFactor < 20.0f);

		for (int i = 0; i < voiceBuffer.getNumChannels(); i++)
		{
			FloatVectorOperations::multiply(voiceBuffer.getWritePointer(i) + startSample , eventGainFactor, numSamples);
		}
	}

	/** This checks the envelopes of the gain modulation if any envelopes are tailing off. */
	virtual void checkRelease();

	virtual void pitchWheelMoved (int /*newValue*/) { };

    virtual void controllerMoved (int /*controllerNumber*/, int /*newValue*/) { };

	const float * getVoiceValues(int channelIndex, int startSample) const
	{
		return voiceBuffer.getReadPointer(channelIndex, startSample);
	}

	double getVoiceUptime() const noexcept { return startUptime; }

	int getVoiceIndex() const;

	void setStartUptime(double newUptime) noexcept { startUptime = newUptime; }

	void enablePitchModulation(bool shouldBeEnabled) noexcept{ pitchModulationActive = shouldBeEnabled; }

	bool isPitchModulationActive() const noexcept{ return pitchModulationActive || scriptPitchActive; }

	void setScriptGainValue(float newGainValue) { scriptGainValue = newGainValue; }
	void setScriptPitchValue(float newPitchValue) { scriptPitchValue = newPitchValue; }

	void setTransposeAmount(int value) noexcept{ transposeAmount = value; };
	int getTransposeAmount() const noexcept { return transposeAmount; };

	void setVolumeFade(double fadeTimeSeconds, float targetVolume)
	{
		gainFader.setValue(eventGainFactor);
		gainFader.reset(getSampleRate(), fadeTimeSeconds);
		gainFader.setValue(targetVolume);
	}

	void setPitchFade(double fadeTimeSeconds, double targetPitch)
	{
		scriptPitchActive = true;
		pitchFader.setValue(eventPitchFactor);
		pitchFader.reset(getSampleRate(), fadeTimeSeconds);
		pitchFader.setValue(targetPitch);
	}

protected:


	/** Returns the ModulatorSynth instance that this voice belongs to.
	*
	*	You need this to get the information needed for the processing of the Modulators.
	*/
	const ModulatorSynth *getOwnerSynth() const noexcept { return ownerSynth; };

	ModulatorSynth *getOwnerSynth() noexcept { return ownerSynth; };
	
	/** The current delta value in which the uptime gets increased per calculated sample. 
	*	The unit can change from Synth to synth (eg. angle vs. sample position ), so it must be calculated for each subclass in its startNote function. 
	*/
	double uptimeDelta = 0.0;

	/** The total voice uptime. If you want to stop the rendering, set this to 0.0. */
	double voiceUptime;

	AudioSampleBuffer voiceBuffer;

	const int voiceIndex;

	int transposeAmount = 0;

	float scriptGainValue = 1.0f;
	double scriptPitchValue = 1.0;

	double eventPitchFactor = 1.0;
	float eventGainFactor = 1.0f;

    bool isActive = false;
    
private:

	HiseEvent currentHiseEvent;

	LinearSmoothedValue<double> pitchFader;
	LinearSmoothedValue<float> gainFader;

	bool pitchModulationActive = false;
	bool scriptPitchActive = false;

	friend class ModulatorSynthGroupVoice;

	bool killThisVoice;

	bool isTailing;

    
    
	float killFadeLevel;
	float killFadeFactor;
	
	double startUptime;

	

	ModulatorSynth* const ownerSynth;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSynthVoice)

};


class ModulatorSynthSound: public SynthesiserSound
{
public:

	virtual bool appliesToVelocity(int velocity) = 0;

	bool appliesToMessage(int midiChannel, const int midiNoteNumber, const int velocity) 
	{
		return appliesToChannel(midiChannel) && appliesToNote(midiNoteNumber) && appliesToVelocity(velocity);
	};

};

class ModulatorSynthChainFactoryType: public FactoryType
{
public:

	enum
	{
		sineSynth = 0,
		waveSynth,
		noise,
		wavetableSynth,
		audioLooper,
		streamingSampler,
		modulatorSynthChain,
		modulatorSynthGroup,
		globalModulatorContainer,
		scriptSynth
	};

	ModulatorSynthChainFactoryType(int numVoices_, Processor *ownerProcessor):
		FactoryType(ownerProcessor),
		numVoices(numVoices_)
	{
		fillTypeNameList();
	};

	void fillTypeNameList();

	Processor* createProcessor	(int typeIndex, const String &id) override;
	
protected:

	const Array<ProcessorEntry>& getTypeNames() const override
	{
		return typeNames;
	};

private:

	Array<ProcessorEntry> typeNames;
	int numVoices;

};

#endif  // MODULATORSYNTH_H_INCLUDED
