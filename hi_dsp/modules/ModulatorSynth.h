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

#ifndef MODULATORSYNTH_H_INCLUDED
#define MODULATORSYNTH_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSynthChain;
class ModulatorSynthEditor;
class ModulatorSynthGroup;
class ModulatorSynthSound;
class ModulatorSynthVoice;

typedef HiseEventBuffer EVENT_BUFFER_TO_USE;


using VoiceStack = UnorderedStack<ModulatorSynthVoice*>;

/** The uniform voice handler will unify the voice indexes of a container so that all sound generators will use the
    same voice index (derived by the event ID of the HiseEvent that started the voice).
    
    By default you can't make assumptions about voice indexes outside of the sound generator because the note might be killed
    earlier for shorter sounds and a restarted voice might have a different voice index. This class manages the voice index
    in a way that guarantees that all voices of child sound generators that are started by the same HiseEvent have the same voice index.

    The obvious advantage of this is that allows you to reuse polyphonic modulation signals, which was a no-go before, but that comes
    with some performance impact at the voice start (because of the logic that has to determine which voice index can be used by all child synths),
    so only enable this when you need to.

    Also you must not change the MIDI events within the container you're using this (so all sound generators inside the synth are supposed to start their sound),
    so MIDI processing (eg. Arpeggiators etc) should not be used inside this (you can of course use an arpeggiator outside the container you're calling this on).
*/
struct UniformVoiceHandler
{
    UniformVoiceHandler(ModulatorSynth* parent_);

    ~UniformVoiceHandler();

    /** This is called in the prepareToPlay function and makes sure that all. */
    void rebuildChildSynthList();

    /** This will ask all child synths which voice index it should use for that event, then store it. */
    void processEventBuffer(const HiseEventBuffer& eventBuffer);

    /** This will return the uniform voice index that is used for the given event. */
    int getVoiceIndex(const HiseEvent& e);

    void incVoiceCounter(ModulatorSynth* s, int voiceIndex);
    void decVoiceCounter(ModulatorSynth* s, int voiceIndex);

    void cleanupAfterProcessing();

private:

    hise::SimpleReadWriteLock arrayLock;

    snex::span<std::tuple<HiseEvent, uint8>, NUM_POLYPHONIC_VOICES> currentEvents;

    WeakReference<ModulatorSynth> parent;
    Array<std::tuple<WeakReference<ModulatorSynth>, VoiceBitMap<NUM_POLYPHONIC_VOICES>>> childSynths;

    JUCE_DECLARE_WEAK_REFERENCEABLE(UniformVoiceHandler);
};


/** The base class for all sound generators in HISE.
	@ingroup dsp_base_classes

	It is a extension of the juce::Synthesiser class with the following additions:

	- slots for adding MIDI processing modules, modulators and effects
	- usage of the HiseEvent type instead of the MidiMessage.
	
	If you're know your way around writing a sound generator based on the juce::Synthesiser class,
	the adaption to this class should be very straight forward.
*/
class ModulatorSynth: public Synthesiser,
					  public Processor,
					  public RoutableProcessor
{
public:

	ADD_DOCUMENTATION();

	// ===================================================================================================================

	enum Parameters
	{
		Gain = 0, ///< the volume as gain factor from 0...1
		Balance, ///< the stereo balance from -100 to 100
		VoiceLimit, ///< the amount of voices
		KillFadeTime, ///< the fade time when voices are killed \endcond module_list
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

	enum BasicChains
	{
		GainChain = 0,
		PitchChain = 1
	};

	// ===================================================================================================================

	ModulatorSynth(MainController *mc, const String &id, int numVoices);
	~ModulatorSynth();

	// ===================================================================================================================

	ValueTree exportAsValueTree() const override;
	void restoreFromValueTree(const ValueTree &v) override;

	// ===================================================================================================================

	virtual float getAttribute(int parameterIndex) const override;
	virtual void setInternalAttribute(int parameterIndex, float newValue) override;;
    virtual float getDefaultValue(int parameterIndex) const override;

	// ===================================================================================================================

	Processor *getChildProcessor(int processorIndex) override;
	const Processor *getChildProcessor(int processorIndex) const override;
	int getNumChildProcessors() const override;;
	int getNumInternalChains() const override;;

	// ===================================================================================================================

	int getFreeTimerSlot();
	void synthTimerCallback(uint8 index, int numSamplesThisBlock);
	void startSynthTimer(int index, double interval, int timeStamp);
	void stopSynthTimer(int index);
	double getTimerInterval(int index) const noexcept;

	// ===================================================================================================================

	bool isLastStartedVoice(ModulatorSynthVoice *voice);;
	ModulatorSynthVoice* getLastStartedVoice() const;

	virtual int getNumActiveVoices() const;

	// ===================================================================================================================

	virtual ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	/** Call this instead of Synthesiser::renderNextBlock to let the ModulatorChains to their work. 
	*
	*	This only renders the TimeVariantModulators (like a master effect) and calculates the voice modulation, so make sure you actually apply
	*	the voice modulation in the subclassed ModulatorSynthVoice callback.
	*/
	virtual void renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi);

	/** This method is called to handle all modulatorchains just before the voice rendering. */
	virtual void preVoiceRendering(int startSample, int numThisTime);;

	/** This method is called to actually render all voices. It operates on the internal buffer of the ModulatorSynth. */
	void renderVoice(int startSample, int numThisTime);

	void calculateModulationValuesForVoice(ModulatorSynthVoice * v, int startSample, int numThisTime);;

	void clearPendingRemoveVoices();

	/** This method is called to handle all modulatorchains after the voice rendering and handles the GUI metering. It assumes stereo mode.
	*
	*	The rendered buffer is supplied as reference to be able to apply changes here after all voices are rendered (eg. gain).
	*/
	void postVoiceRendering(int startSample, int numThisTime);;

	// ===================================================================================================================

	virtual void handlePeakDisplay(int numSamplesInOutputBuffer);
	void setPeakValues(float l, float r);

	// ===================================================================================================================

	void setIconColour(Colour newIconColour);;

	Colour getIconColour() const;
	Colour getColour() const override;;

	// ===================================================================================================================

	/** Same functionality as Synthesiser::handleMidiEvent(), but sends the midi event to all Modulators in the chains. */
	//void handleHiseEvent(const HiseEvent &m) final override;

	void handleHiseEvent(const HiseEvent& e);
	void handleHostInfoHiseEvents(int numSamples);
	void handleVolumeFade(int eventId, int fadeTimeMilliseconds, float gain);
	void handlePitchFade(uint16 eventId, int fadeTimeMilliseconds, double pitchFactor);

	virtual void preHiseEventCallback(HiseEvent &e);
	virtual void preStartVoice(int voiceIndex, const HiseEvent& e);

	void preStopVoice(int voiceIndex);

	/** This sets up the synth and the ModulatorChains. 
	*
	*	Call this instead of Synthesiser::setCurrentPlaybackSampleRate(). 
	*	It also sets the ModulatorChain's voice amount, so be sure you add all SynthesiserVoices before you call this function.
	*/
	virtual void prepareToPlay(double sampleRate, int samplesPerBlock);

	// ===================================================================================================================

	void numSourceChannelsChanged() override;
	void numDestinationChannelsChanged() override;

	// ===================================================================================================================

	void setBypassed(bool shouldBeBypassed, NotificationType notifyChangeHandler=dontSendNotification) noexcept override;;

	void softBypassStateChanged(bool isBypassedNow);

	void disableChain(InternalChains chainToDisable, bool shouldBeDisabled);
	bool isChainDisabled(InternalChains chain) const;;

	// ===================================================================================================================

	
	// ===================================================================================================================

	/** Kills the note with the specified note number. 
	*
	*	This stops the note with a small fade out (instead of noteoff which can result in very long release times 
	*	if the envelope says so
	*/
	void killAllVoicesWithNoteNumber(int noteNumber);

	/** Kills the voice that is playing for the longest time. */
	int killLastVoice(bool allowTailOff=true);

	

	bool isSoftBypassed() const;;

	void deleteAllVoices();
    
	virtual void resetAllVoices();

	virtual void killAllVoices();

	/** Call this from the message thread and it'll kill all voices at the next buffer. 
	*
	*	Pass in a lambda and it will execute this asynchronously as soon as all voices are killed
	*
	*	@functionToExecuteWhenKilled: a lambda that will be executed as soon as the voices are killed
	*	@executeOnAudioThread:		  if true, the lambda will be synchronously called on the audio thread
	*
	*/
	virtual bool areVoicesActive() const;

	virtual void setVoiceLimit(int newVoiceLimit);

	void setKillFadeOutTime(double fadeTimeSeconds);

		/** Checks if the message fits the sound, but can be overriden to implement other group start logic. */
	virtual bool soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity);

	void startVoiceWithHiseEvent(ModulatorSynthVoice* voice, SynthesiserSound *sound, const HiseEvent &e);

	/** Same functionality as Synthesiser::noteOn(), but calls calculateVoiceStartValue() if a new voice is started. */
	void noteOn(const HiseEvent &m);

	void noteOn(int midiChannel, int midiNoteNumber, float velocity) final override;

	struct SoundCollectorBase
	{

		virtual ~SoundCollectorBase();;

		virtual void collectSounds(const HiseEvent& m, UnorderedStack<ModulatorSynthSound*>& soundsToBeStarted) = 0;
	};

	/** This method should go through all sounds that are playable and fill the soundsToBeStarted array. */
	virtual int collectSoundsToBeStarted(const HiseEvent &m);


	virtual void noteOff(const HiseEvent &m);

	/** Returns the voice index for the voice (the index in the internal voice array). This is needed for the ModulatorChains to know which voice is started. */
	int getVoiceIndex(const SynthesiserVoice *v) const;;

	void processHiseEventBuffer(const HiseEventBuffer &inputBuffer, int numSamples);

	/** Adds a SimpleEnvelope to a empty ModulatorSynth to prevent clicks. */
	virtual void addProcessorsWhenEmpty();

	/** Clears the internal buffer. This must be called by subclasses for every renderNextBlockWithModulators. */
	virtual void initRenderCallback();;

	/** sets the gain of the ModulatorSynth from 0.0 to 1.0. */
	void setGain(float newGain);;

	/** sets the balance from -1.0 (left) to 1.0 (right) and applies a equal power pan rule. */
	void setBalance(float newBalance);;

	/** Returns the calculated (equal power) pan value for either the left or the right channel. */
	float getBalance(bool getRightChannelGain) const;;

	/** Returns the gain of the ModulatorSynth from 0.0 to 1.0. */
	float getGain() const;;

	void setScriptGainValue(int voiceIndex, float gainValue) noexcept;

	void setScriptPitchValue(int voiceIndex, double pitchValue) noexcept;

	/** Sets the parent group. This can only be called once, since synths are not supposed to change their parents. */
    void setGroup(ModulatorSynthGroup *parent);
	

	/** Returns the ModulatorSynthGroup that this ModulatorSynth belongs to. */
	ModulatorSynthGroup *getGroup() const;;

	/** Checks if the Synth was added to a group. */
	bool isInGroup() const;;

	/** Returns the index of the child synth if it resides in a group and -1 if not. */
	int getIndexInGroup() const;

	/** Returns either itself or the group that is playing its voices. */

	ModulatorSynth* getPlayingSynth();

	const ModulatorSynth* getPlayingSynth() const;

	/** Sets the interval for the internal clock callback. */
	void setClockSpeed(ClockSpeed newClockSpeed);

	bool allowEmptyPitchValues() const;

	/** Returns the pointer to the calculated pitch buffers for the ModulatorSynthVoice's render callback. */
	const float *getConstantPitchValues() const;;

	double getSampleRate() const;

	void setKillRetriggeredNote(bool shouldBeKilled);

	/** specifies the behaviour when a note is started that is already ringing. By default, it is killed, but you can overwrite it to make something else. */
	virtual void handleRetriggeredNote(ModulatorSynthVoice *voice);

	
	/** Returns a read pointer to the calculated pitch values. */
	float *getPitchValuesForVoice() const;;

	void overwritePitchValues(const float* modDataValues, int startSample, int numSamples);

	const float* getVoiceGainValues() const;

	float getConstantPitchModValue() const;

	float getConstantGainModValue() const;


	float getConstantVoicePitchModulationValueDeleteSoon() const;


	/** Returns a read pointer to the calculated pitch values. Used by Synthgroups to render their pitch values on the voice value. */
	//float *getPitchValuesForVoice(int voiceIndex) { return pitchChain->getVoiceValues(voiceIndex);};

	ModulatorSynthVoice* getFreeVoice(SynthesiserSound* s, int midiChannel, int midiNoteNumber);

	SynthesiserVoice* findVoiceToSteal(SynthesiserSound* soundToPlay, int midiChannel, int midiNoteNumber) const override;


	int getNumFreeVoices() const;

	virtual bool handleVoiceLimit(int numSoundsToBeStarted);

protected:

	ModulatorChain::Collection modChains;

	ScopedPointer<SoundCollectorBase> soundCollector;

public:

	HiseEventBuffer eventBuffer;
	AudioSampleBuffer internalBuffer;

	UpdateMerger vuMerger;

	AudioSampleBuffer pitchBuffer;
	AudioSampleBuffer gainBuffer;

	ModulatorChain* gainChain = nullptr;
	ModulatorChain* pitchChain = nullptr;

	
	ScopedPointer<MidiProcessorChain> midiProcessorChain;
	ScopedPointer<EffectProcessorChain> effectChain;

	BigInteger disabledChains;

	float getMidiInputFlag();

	void setSoftBypass(bool shouldBeBypassed, bool bypassFXToo);

	void updateSoftBypassState();

	VoiceStack activeVoices;
	
	void flagVoiceAsRemoved(ModulatorSynthVoice* v);

	UnorderedStack<ModulatorSynthSound*> soundsToBeStarted;

	HiseEventBuffer* getEventBuffer();

	virtual void setUseUniformVoiceHandler(bool shouldUseVoiceHandler, UniformVoiceHandler* externalHandlerToUse);

	bool isUsingUniformVoiceHandler() const;

	UniformVoiceHandler* getUniformVoiceHandler() const;

private:

	VoiceStack pendingRemoveVoices;
    
    WeakReference<UniformVoiceHandler> currentUniformVoiceHandler;

protected:

	virtual bool synthNeedsEnvelope() const;;
	
	void finaliseModChains();
	

	bool finalised = false;

	bool checkTimerCallback(int timerIndex, int numSamplesThisBlock) const noexcept;;
	
	// Used to display the playing position
	ModulatorSynthVoice *lastStartedVoice;


#if JUCE_DEBUG
	// Makes sure that everything matches...
	HiseEvent eventForSoundCollection;
#endif

	ModulatorSynthVoice* getVoiceToStart(const HiseEvent& m);

private:

    void updateShouldHaveEnvelope();
	
	bool shouldHaveEnvelope = true;



	int numActiveVoices;

	

	// kills or resets all voices that have the same start event. */
	int killVoiceAndSiblings(ModulatorSynthVoice* v, bool allowTailOff);


	// ===================================================================================================================

	

	Colour iconColour;

	ClockSpeed clockSpeed;

	int lastClockCounter;

	int voiceLimit;
	int internalVoiceLimit;

	// If this is true, the script fade things have changed the pitch modulation data
	// and it must be used.
	bool useScratchBufferForArtificialPitch = false;

	

	bool shouldKillRetriggeredNote = true;

	std::atomic<double> synthTimerIntervals[4];
	std::atomic<double> nextTimerCallbackTimes[4];

	ModulatorSynthGroup *group;

	float midiInputAlpha = 0.0f;
	
	bool wasPlayingInLastBuffer;

	std::atomic<float> gain;

	std::atomic<float> balance;
	std::atomic<float> leftBalanceGain;
	std::atomic<float> rightBalanceGain;

	std::atomic<float> vuValue;

	std::atomic<float> killFadeTime;

	std::atomic<bool> bypassState;

    bool anyTimerActive = false;
    
	// ===================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSynth)
	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulatorSynth);
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

	ModulatorSynthVoice(ModulatorSynth *ownerSynth_);;

	/** If not overriden, this uses a sine generator for an example usage of this voice class. */
	virtual void renderNextBlock (AudioSampleBuffer& outputBuffer,
                                  int startSample,
                                  int numSamples) override;


	virtual void calculateBlock(int startSample, int numSamples) = 0;
	
	bool isPitchFadeActive() const noexcept;

	void applyScriptPitchFactors(float* voicePitchModulationData, int numSamples);


	/** This only checks if the sound is valid, but you can override this with the desired behaviour. */
	virtual bool canPlaySound(SynthesiserSound *s) override;;

	
	void setCurrentHiseEvent(const HiseEvent &m);

	const HiseEvent &getCurrentHiseEvent() const;

	void addToStartOffset(uint16 delta);

	/** This calculates the angle delta. For this synth, it detects the sine frequency, but you can override it to make something else. */
	virtual void startNote (int /*midiNoteNumber*/, float /*velocity*/, SynthesiserSound* , int /*currentPitchWheelPosition*/);

	bool isBeingKilled() const;

	bool isTailingOff() const;;

	virtual void prepareToPlay(double sampleRate, int samplesPerBlock);

	virtual void setInactive();;

	virtual void resetVoice();

	bool isInactive() const noexcept;;

	/** This handles the voice stop. If any envelopes are active, the voice keeps playing and repeatedly call checkRelease(), until they are finished. */
	virtual void stopNote(float velocity, bool allowTailOff) override;

	/** This kills the note with a short fade time. */
	void killVoice();

	bool const shouldBeKilled() const;

	void setKillFadeFactor(float newKillFadeFactor);


	void applyKillFadeout(int startSample, int numSamples);

	void applyEventVolumeFade(int startSample, int numSamples);

	void applyEventVolumeFactor(int startSample, int numSamples);

	/** This checks the envelopes of the gain modulation if any envelopes are tailing off. */
	virtual void checkRelease();

	virtual void pitchWheelMoved (int /*newValue*/);;

    virtual void controllerMoved (int /*controllerNumber*/, int /*newValue*/);;

	const float * getVoiceValues(int channelIndex, int startSample) const;

	double getVoiceUptime() const noexcept;

	int getVoiceIndex() const;

	void setStartUptime(double newUptime) noexcept;

	void setScriptGainValue(float newGainValue);
	void setScriptPitchValue(float newPitchValue);

	void setTransposeAmount(int value) noexcept;;
	int getTransposeAmount() const noexcept;;

	void setVolumeFade(double fadeTimeSeconds, float targetVolume);

	void setPitchFade(double fadeTimeSeconds, double targetPitch);


	void saveStartUptimeDelta();

	void setUptimeDeltaValueForBlock();

	void applyConstantPitchFactor(double pitchFactorToAdd);

	void applyGainModulation(int startSample, int numSamples, bool copyLeftChannel);

protected:

	

	/** Returns the ModulatorSynth instance that this voice belongs to.
	*
	*	You need this to get the information needed for the processing of the Modulators.
	*/
	const ModulatorSynth *getOwnerSynth() const noexcept;;

	ModulatorSynth *getOwnerSynth() noexcept;;
	
	/** The current delta value in which the uptime gets increased per calculated sample. 
	*	The unit can change from Synth to synth (eg. angle vs. sample position ), so it must be calculated for each subclass in its startNote function. 
	*/
	double uptimeDelta = 0.0;

	double startUptimeDelta = 0.0;

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
    
	float killFadeLevel;
	float killFadeFactor;
	bool killThisVoice;

private:

	
	HiseEvent currentHiseEvent;

	LinearSmoothedValue<double> pitchFader;
	LinearSmoothedValue<float> gainFader;

	friend class ModulatorSynthGroupVoice;

	

	bool isTailing;

	
	
	double startUptime;

	ModulatorSynth* const ownerSynth;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSynthVoice)

};


class ModulatorSynthSound: public SynthesiserSound
{
public:

	virtual bool appliesToVelocity(int velocity) = 0;

	bool appliesToMessage(int midiChannel, const int midiNoteNumber, const int velocity);;

};

class ModulatorSynthChainFactoryType: public FactoryType
{
public:

	enum
	{
        streamingSampler=0,
        sineSynth,
        modulatorSynthChain,
        globalModulatorContainer,
		waveSynth,
		noise,
		wavetableSynth,
		audioLooper,
		modulatorSynthGroup,
		scriptSynth,
		macroModulationSource,
		sendContainer,
		silentSynth
	};

	ModulatorSynthChainFactoryType(int numVoices_, Processor *ownerProcessor);;

	void setNumVoices(int newNumVoices);

	void fillTypeNameList();

	Processor* createProcessor	(int typeIndex, const String &id) override;
	
protected:

	const Array<ProcessorEntry>& getTypeNames() const override;;

private:

	Array<ProcessorEntry> typeNames;
	int numVoices;

};

} // namespace hise

#endif  // MODULATORSYNTH_H_INCLUDED
