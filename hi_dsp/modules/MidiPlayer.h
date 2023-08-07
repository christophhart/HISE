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


#pragma once

namespace hise {
using namespace juce;

#define DECLARE_ID(x) static Identifier x(#x);
namespace TimeSigIds
{
	DECLARE_ID(Nominator);
	DECLARE_ID(Denominator);
	DECLARE_ID(NumBars);
	DECLARE_ID(LoopStart);
	DECLARE_ID(LoopEnd);
	DECLARE_ID(Tempo);
}
#undef DECLARE_ID

/** A wrapper around a MIDI file. 

	This uses the juce::MidiMessageSequence object but extends it with these capabilities:
	
	- reference counted
	- save to / load from ValueTrees
	- unique ID
	- normalised tempo
	- defined length
	- temporary copy of pooled read-only content
	- operations with undo support

	This object is used by the MidiFilePlayer for the playback logic. It is considered to be a temporary, non-writable
	object that is constructed from pooled MIDI files and have some operations applied to it. Currently the only way
	to get the content of a MIDI sequence to the outside world is dragging it to another location (with writeToTempFile()) . 
*/
class HiseMidiSequence : public ReferenceCountedObject,
						 public RestorableObject
{
public:

	enum class TimestampEditFormat
	{
		Samples,
		Ticks,
		numTimestampFormats
	};

	struct TimeSignature: public RestorableObject
	{
		double numBars = 0.0;
		double nominator = 4.0;
		double denominator = 4.0;
		double bpm = 120.0;
		Range<double> normalisedLoopRange = { 0.0, 1.0 };

		void setLoopEnd(double normalisedEnd);

		void setLoopStart(double normalisedStart);

		void calculateNumBars(double lengthInQuarters, bool roundToQuarter);

		String toString() const;

		double getNumQuarters() const;

		ValueTree exportAsValueTree() const override;

		var getAsJSON() const;

		void restoreFromValueTree(const ValueTree &v) override;
	};

	/** The internal resolution (set to a sensible high default). */
	static constexpr int TicksPerQuarter = 960;

	/** This object is ref-counted so this can be used as reference pointer. */
	using Ptr = ReferenceCountedObjectPtr<HiseMidiSequence>;

	using List = ReferenceCountedArray<HiseMidiSequence>;

	/** Creates an empty new sequence object. */
	HiseMidiSequence();

	/** Saves the sequence into a ValueTree. It stores the ID and the
	    data as compressed MIDI file.
	*/
	ValueTree exportAsValueTree() const override;

	/** Loads the sequence from the value tree. */
	void restoreFromValueTree(const ValueTree &v) override;

	/** Gets the next event of the current track in the given range. This also advances the playback pointer
		so you should only use it in the audio thread for playback. 
	*/
	MidiMessage* getNextEvent(Range<double> rangeToLookForTicks);

	/** Returns the MIDI note off message for the current note on message. */
	MidiMessage* getMatchingNoteOffForCurrentEvent();

	/** Returns the length in ticks (as defined with TicksPerQuarter). */
	double getLength() const;

	/** Returns the length of the MIDI sequence in quarter beats. */
	double getLengthInQuarters() const;

	double getLengthInSeconds(double bpm);

	double getLastPlayedNotePosition() const;

	/** Forces the length of the sequence to this value. If you want to use the original length, pass in -1.0. */
	void setLengthInQuarters(double newLength);

	/** Uses a time signature object to set the length. */
	void setLengthFromTimeSignature(TimeSignature s);

	/** Clones this sequence into a new object. */
	HiseMidiSequence::Ptr clone() const;

	/** Creates a temporary MIDI file with the content of this sequence.
		
		This is used by the drag to external target functionality.
	*/
	File writeToTempFile();

	TimeSignature getTimeSignature() const;

	TimeSignature* getTimeSignaturePtr();;

	/** Sets the ID of this sequence. */
	void setId(const Identifier& newId);

	/** Returns the ID of this sequence. */
	Identifier getId() const noexcept;

	/** Returns a read only pointer to the given track.
	
		If the argument is omitted, it will return the current track. 
	*/
	const juce::MidiMessageSequence* getReadPointer(int trackIndex=-1) const;

	/** Returns a write pointer to the given track.

	If the argument is omitted, it will return the current track. 
	*/
	juce::MidiMessageSequence* getWritePointer(int trackIndex=-1);

	/** Get the number of events in the current track. */
	int getNumEvents() const;

	/** Get the number of tracks in this MIDI sequence. */
	int getNumTracks() const;

	/** Sets the current track. */
	void setCurrentTrackIndex(int index);

	/** Removes all inactive tracks and keeps only the currently active one. */
	void trimInactiveTracks();

	/** Resets the playback position. */
	void resetPlayback();

	/** Sets the playback position. This will search through all events in the current track and set the index to point to the next event so this operation is not trivial. */
	void setPlaybackPosition(double normalisedPosition);

	/** Returns a rectangle list of all note events in the current track that can be used by UI elements to draw notes. It automatically scales them to the supplied targetBounds.
	*/
	RectangleList<float> getRectangleList(Rectangle<float> targetBounds) const;

	/** Returns a list of all events of the current track converted to HiseEvents. This is used by editing operations.
	
		
	*/
	Array<HiseEvent> getEventList(double sampleRate, double bpm, HiseMidiSequence::TimestampEditFormat formatToUse=HiseMidiSequence::TimestampEditFormat::numTimestampFormats);

	/** Swaps the current track with the given MidiMessageSequence. */
	void swapCurrentSequence(MidiMessageSequence* sequenceToSwap);

	/** Loads from a MidiFile. */
	void loadFrom(const MidiFile& file);

	/** Creates an empty track and selects it. */
	void createEmptyTrack();

	TimestampEditFormat getTimestampEditFormat() const;

	void setTimeStampEditFormat(TimestampEditFormat formatToUse);

private:

	TimestampEditFormat timestampFormat = TimestampEditFormat::Samples;

	TimeSignature signature;

	

	mutable SimpleReadWriteLock swapLock;

	Identifier id;
	OwnedArray<MidiMessageSequence> sequences;
	int currentTrackIndex = 0;
	int lastPlayedIndex = -1;

	double artificialLengthInQuarters = -1.0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseMidiSequence);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseMidiSequence);
};



/** A player for MIDI sequences.
*	@ingroup midiTypes
*
*	This module plays MidiFiles when its internal transport is activated.
*	It acts as common core module for multiple "overlays" (MIDI loopers, Piano roll, step sequencer, etc.)
*/
class MidiPlayer : public MidiProcessor,
				   public TempoListener
{
public:

	enum class PlayState
	{
		Stop,
		Play,
		Record,
		numPlayStates
	};

	struct PlaybackListener
	{
		virtual ~PlaybackListener();

		virtual void playbackChanged(int timestamp, PlayState newState) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(PlaybackListener);
	};

	struct EventRecordProcessor
	{
		virtual ~EventRecordProcessor();;

		virtual void processRecordedEvent(HiseEvent& e) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(EventRecordProcessor);
	};

	/** A Listener that will be notified when a new HiseMidiSequence was loaded. */
	struct SequenceListener
	{
		virtual ~SequenceListener();;

		/** Will be called whenever a new sequence is loaded or the current sequence is changed. 
		
			This will always happen on the message thread.
		*/
		virtual void sequenceLoaded(HiseMidiSequence::Ptr newSequence) = 0;

		/** Will be called whenever the sequences are cleared. 
		
			This will always happen on the message thread.
		*/
		virtual void sequencesCleared() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SequenceListener);
	};

	/** A undoable edit action that takes a list of events and overwrites the events in the current sequence.

		You need to supply the samplerate and BPM because the domain of the timestamps in the event list are
		samples (to stay consistent with the scripting API).
	*/
	class EditAction : public UndoableAction
	{
	public:

		/** Creates a new action. 
		
		    Upon construction, it will create a list of events from the current sequence that
			will be used for undo operations. */
		EditAction(WeakReference<MidiPlayer> currentPlayer_, const Array<HiseEvent>& newContent, double sampleRate_, double bpm_, HiseMidiSequence::TimestampEditFormat formatToUse);;

		/** Applies the given event list. */
		bool perform() override;

		/** Restores the previous event list. */
		bool undo() override;

		static void writeArrayToSequence(HiseMidiSequence::Ptr destination, Array<HiseEvent>& arrayToWrite, double bpm, double sampleRate, HiseMidiSequence::TimestampEditFormat formatToUse=HiseMidiSequence::TimestampEditFormat::numTimestampFormats);

	private:

		HiseMidiSequence::TimeSignature oldSig;

		WeakReference<MidiPlayer> currentPlayer;
		Array<HiseEvent> newEvents;
		Array<HiseEvent> oldEvents;
		double sampleRate;
		double bpm;
		Identifier sequenceId;
		HiseMidiSequence::TimestampEditFormat formatToUse;
	};

	/** An undoable operation that exchanges the entire sequence set. */
	class SequenceListAction : public UndoableAction
	{
	public:

		SequenceListAction(MidiPlayer* p, HiseMidiSequence::List newList_, int newSeqIndex);

		bool perform() override;

		bool undo() override;

	private:

		WeakReference<MidiPlayer> player;

		HiseMidiSequence::List oldList;
		HiseMidiSequence::List newList;

		int oldIndex = -1;
		int newIndex;
	};


	struct TimesigUndo : public UndoableAction
	{
		TimesigUndo(MidiPlayer* player_, HiseMidiSequence::TimeSignature newSig_);

		bool perform() override;

		bool undo() override;

		WeakReference<MidiPlayer> player;
		HiseMidiSequence::TimeSignature oldSig;
		HiseMidiSequence::TimeSignature newSig;
	};


	SET_PROCESSOR_NAME("MidiPlayer", "MIDI Player", "A player for MIDI sequences.");

	MidiPlayer(MainController *mc, const String &id, ModulatorSynth* ms);;
	~MidiPlayer();

	/**@ internal */
	void tempoChanged(double newTempo) override;

	void onGridChange(int gridIndex, uint16 timestamp, bool firstGridEventInPlayback) override;

	void onTransportChange(bool isPlaying, double ppqPosition) override;
	
	enum class RecordState
	{
		Idle,
		PreparationPending,
		Prepared,
		FlushPending,
		numRecordStates
	};

	enum SpecialParameters
	{
		CurrentPosition,		   ///< the current position within the current MIDI file (non-persistent)
		CurrentSequence,		   ///< the index of the currently played sequence (not zero based for combobox compatibility)
		CurrentTrack,			   ///< the index of the currently played track within a sequence.
		LoopEnabled,			   ///< toggles between oneshot and loop playback
		LoopStart,				   ///< start of the (loop) playback
		LoopEnd,				   ///< end of the (loop) playback
		PlaybackSpeed,			   ///< the playback speed of the MidiPlayer
		numSpecialParameters
	};

	void addSequence(HiseMidiSequence::Ptr newSequence, bool select=true);

	/** Clears all sequences. This also clears the undo history. */
	void clearSequences(NotificationType notifyListeners=sendNotification);

	// ======================================================================== Processor methods

	/**@ internal */
	ValueTree exportAsValueTree() const override;;

	/**@ internal */
	virtual void restoreFromValueTree(const ValueTree &v) override;

	/**@ internal */
	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	/**@ internal */
	float getAttribute(int index) const;;
	/**@ internal */
	void setInternalAttribute(int index, float newAmount) override;;

	/** Loads the given pooled MIDI file and adds it to the end of the list. 
	
		If trackIndexToKeep is not -1, it will discard all tracks other than the given track index (zero-based!) and set the current track index to 0. */
	void loadMidiFile(PoolReference reference);

	/**@ internal */
	bool isProcessingWholeBuffer() const override;

	/**@ internal */
	void prepareToPlay(double sampleRate_, int samplesPerBlock_);

	/**@ internal */
	void preprocessBuffer(HiseEventBuffer& buffer, int numSamples) override;

	/**@ internal */
	void processHiseEvent(HiseEvent &m) noexcept override;

	/** Adds a sequence listener that will be notified about changes to the sequences. */
	void addSequenceListener(SequenceListener* newListener);

	/** Removes a sequence listener that was registered to this player. */
	void removeSequenceListener(SequenceListener* listenerToRemove);

	/** Returns the play state as integer. */
	PlayState getPlayState() const;;

	/** Returns the number of sequences loaded into this player. */
	int getNumSequences() const;

	/** Returns the currently played sequence. */
	HiseMidiSequence::Ptr getCurrentSequence() const;

	/** Returns the ID used for the given sequence. If -1 is used as index, the current sequence will be used. */
	Identifier getSequenceId(int index=-1) const;

	/** Returns the current playback position from 0...1. */
	double getPlaybackPosition() const;

	/** Returns the normalised playback position inside the current loop. This will never be outside the bounds of the loop. */
	double getPlayPackPositionInLoop() const;

	/** This will send any CC messages from the MIDI file to the global MIDI handler. */
	void setMidiControlAutomationHandlerConsumesControllerEvents(bool shouldBeEnabled);

	void swapCurrentSequence(MidiMessageSequence* newSequence);
	void enableInternalUndoManager(bool shouldBeEnabled);

	void setExternalUndoManager(UndoManager* externalUndoManager);

	/** Applies the list of events to the currently loaded sequence. This operation is undo-able. 
	
		It locks the sequence just for a very short time so you should be able to use this from any
		thread without bothering about multi-threading. */
	void flushEdit(const Array<HiseEvent>& newEvents, HiseMidiSequence::TimestampEditFormat formatToUse=HiseMidiSequence::TimestampEditFormat::numTimestampFormats);

	/** Clears the current sequence and any recorded events. */
	void clearCurrentSequence();

	void removeSequence(int sequenceIndex);

	/** Returns the undo manager used for all editing operations. 
	
		This differs from the default undo manager for parameter changes because edits might
		get triggered by UI controls and it would be difficult to deinterleave parameter changes
		and MIDI edits. */
	UndoManager* getUndoManager();;

	void setUseExternalUndoManager(UndoManager* externalUndoManagerToUse);

	/** If set to false, the recording will not be flushed and you can preprocess it. */
	void setFlushRecordingOnStop(bool shouldFlushRecording);

	void setLength(HiseMidiSequence::TimeSignature sig, bool useUndoManager=true);

	double getCurrentTicksSincePlaybackStart() const;

	void setPositionWithTicksFromPlaybackStart(double newPos);

	void onResync(double ppqPos);

	/** Resets the current sequence back to its pooled state. This operation is undo-able. */
	void resetCurrentSequence();

	/** Returns the PoolReference for the given sequence. 
	
	    If -1 is passed, the current sequence index will be used. */
	PoolReference getPoolReference(int index = -1);

	/** Starts playing the sequence from the beginning. 
	
		You can supply a timestamp that delays this operation - this can also be used for sample accurate triggering
		by passing in the timestamp of the current event within the buffer. */
	bool play(int timestampInBuffer=0);

	/** Stops the playback and resets the position. 
	
		You can supply a timestamp that delays this operation - this can also be used for sample accurate triggering
		by passing in the timestamp of the current event within the buffer. */
	bool stop(int timestampInBuffer=0);

	double getTicksPerSample() const;

	double getPlaybackSpeed() const;

	/** Starts recording. If the sequence is already playing, it switches into overdub mode, otherwise it also starts playing. */
	bool record(int timestampInBuffer=0);

	/** This prepares the internal event queue for recording. Can be called on any thread including the audio thread. */
	void prepareForRecording(bool copyExistingEvents=true);

	/** Finishes the recording. Can be called on any thread including the audio thread. */
	void finishRecording();

	/** Creates a temporary sequence containing all the events from the currently recorded event list. */
	HiseMidiSequence::Ptr getListOfCurrentlyRecordedEvents();

	/** Returns the array of HiseEvents without conversion to a HiseMidiSequence. */
	const Array<HiseEvent>& getListOfCurrentlyRecordedEventsRaw() const;

	bool saveAsMidiFile(const String& fileName, int trackIndex);

	void addPlaybackListener(PlaybackListener* l);

	void removePlaybackListener(PlaybackListener* l);

	void sendSequenceUpdateMessage(NotificationType notification);

	void setNoteOffAtStop(bool shouldMoveNotesOfToStop);

	double getLoopStart() const;

	double getLoopEnd() const;

	HiseMidiSequence::List createListOfCurrentSequences();

	void swapSequenceListWithIndex(HiseMidiSequence::List listToSwapWith, int newSequenceIndex);

	/** @internal. */
	void setReferenceWithoutLoading(PoolReference r);

	void setSyncToMasterClock(bool shouldSyncToMasterClock);

	void addEventRecordProcessor(EventRecordProcessor* newProcessor);

	void removeEventRecordProcessor(EventRecordProcessor* processorToRemove);

private:

	bool processRecordedEvent(HiseEvent& m);

	Array<WeakReference<EventRecordProcessor>> eventRecordProcessors;

	struct NotePair
	{
		bool operator==(const NotePair& other) const;
		HiseEvent on;
		HiseEvent off;
	};

	struct OverdubUpdater : public PooledUIUpdater::SimpleTimer
	{
		OverdubUpdater(MidiPlayer& mp);;

		void timerCallback() override;

		void setDirty(double activeNoteTimestamp=-1.0);

		double lastTimestamp = -1.0;
		std::atomic<bool> dirty = { false };

		MidiPlayer& parent;
	} overdubUpdater;

	void flushOverdubNotes(double timestampForActiveNotes=-1.0);

	
	bool recordOnNextPlaybackStart = false;

	bool overdubMode = true;
	hise::UnorderedStack<NotePair> overdubNoteOns;
	hise::UnorderedStack<HiseEvent> controllerEvents;
	SimpleReadWriteLock overdubLock;

	struct Updater : private PooledUIUpdater::SimpleTimer
	{
		Updater(MidiPlayer& mp);;

		void timerCallback() override;

		bool handleUpdate(HiseMidiSequence::Ptr seq, NotificationType n);

		bool dirty = false;

		HiseMidiSequence::Ptr sequenceToUpdate;

		MidiPlayer& parent;
	} updater;

	bool stopInternal(int timestamp);

	bool startInternal(int timestamp);

	bool recordInternal(int timestamp);

	bool syncToMasterClock = false;

	PoolReference forcedReference;

	mutable SimpleReadWriteLock sequenceLock;

	

	void sendPlaybackChangeMessage(int timestamp);

	Array<WeakReference<PlaybackListener>> playbackListeners;

	Array<HiseEvent> currentlyRecordedEvents;

	std::atomic<RecordState> recordState{ RecordState::Idle};

	bool noteOffAtStop = false;


	bool isRecording() const noexcept;

	bool globalMidiHandlerConsumesCC = false;

	ScopedPointer<UndoManager> ownedUndoManager;
    UndoManager* undoManager = nullptr;

	Array<PoolReference> currentlyLoadedFiles;

	SimpleReadWriteLock listenerLock;
	Array<WeakReference<SequenceListener>> sequenceListeners;
	void changeTransportState(PlayState newState);

	double getPlaybackPositionFromTicksSinceStart() const;

	void updatePositionInCurrentSequence(bool ignoreSpeed=false);

	int lastBlockSize = -1;

	HiseMidiSequence::List currentSequences;

	PlayState playState = PlayState::Stop;

	double ticksSincePlaybackStart = 0.0;

	void addNoteOffsToPendingNoteOns();

	bool flushRecordedEvents = true;
	double currentPosition = -1.0;
	int currentSequenceIndex = -1;
	int currentTrackIndex = 0;
	bool loopEnabled = true;

	int timeStampForNextCommand = 0;

	double ticksPerSample = 0.0;
	int currentTimestampInBuffer = 0;

	bool useNextNoteAsRecordStartPos = false;
	double recordStart = 0.0;
	double playbackSpeed = 1.0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MidiPlayer);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPlayer);
};


/** Subclass this and implement your MIDI file player type. */
class MidiPlayerBaseType : public MidiPlayer::SequenceListener
{
public:

	class TransportPaths : public PathFactory
	{
	public:

		String getId() const override;

		Path createPath(const String& name) const override;
	};

	// "Overwrite" this with your id for the factory
	static Identifier getId();

	// "Overwrite" this and return a new object for the given object for the factory
	static MidiPlayerBaseType* create(MidiPlayer* player);;

	virtual ~MidiPlayerBaseType();
	virtual int getPreferredHeight() const;

	void setFont(Font f);

	void initMidiPlayer(MidiPlayer* player);

protected:

	void cancelUpdates();

	MidiPlayerBaseType(MidiPlayer* player_);;

	

	MidiPlayer* getPlayer();
	const MidiPlayer* getPlayer() const;

	Font getFont() const;

private:

	Font font;

	int lastTrackIndex = 0;
	int lastSequenceIndex = -1;

	WeakReference<MidiPlayer> player;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MidiPlayerBaseType);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiPlayerBaseType);
};

#define ENABLE_OVERLAY_FACTORY(className, name) static MidiPlayerBaseType* create(MidiPlayer* player) { return new className(player); }; \
												static Identifier getId() { RETURN_STATIC_IDENTIFIER(name); };


}
