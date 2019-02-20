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

/** A wrapper around a MIDI file. 

	This uses the juce::MidiMessageSequence object but extends it with these capabilities:
	
	- reference counted
	- save to / load from ValueTrees
	- unique ID (for pooling)
	- normalised tempo
	- defined length
	- a pointer to the last played note

*/
class HiseMidiSequence : public ReferenceCountedObject,
						 public RestorableObject
{
public:

	/** The internal resolution (set to a sensible high default). */
	static constexpr int TicksPerQuarter = 960;

	/** This object is ref-counted so this can be used as reference pointer. */
	using Ptr = ReferenceCountedObjectPtr<HiseMidiSequence>;

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
	double getLengthInQuarters();

	/** Loads a sequence from a MIDI file input stream. */
	void loadFrom(InputStream& input);

	/** Creates a temporary MIDI file with the content of this sequence.
		
		This is used by the drag to external target functionality.
	*/
	File writeToTempFile();

	/** Sets the ID of this sequence. */
	void setId(const Identifier& newId);

	/** Returns the ID of this sequence. */
	Identifier getId() const noexcept { return id; }

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
	int getNumTracks() const { return sequences.size(); }

	/** Sets the current track. */
	void setCurrentTrackIndex(int index);

	/** Resets the playback position. */
	void resetPlayback();

	/** Sets the playback position. This will search through all events in the current track and set the index to point to the next event so this operation is not trivial. */
	void setPlaybackPosition(double normalisedPosition);

	/** Returns a rectangle list of all note events in the current track that can be used by UI elements to draw notes. It automatically scales them to the supplied targetBounds.
	*/
	RectangleList<float> getRectangleList(Rectangle<float> targetBounds) const;

	/** Returns a list of all events of the current track converted to HiseEvents. This is used by editing operations. */
	Array<HiseEvent> getEventList(double sampleRate, double bpm);

	/** Swaps the current track with the given MidiMessageSequence. */
	void swapCurrentSequence(MidiMessageSequence* sequenceToSwap);

private:

	/** @internal */
	void loadFrom(const MidiFile& file);

	/** A simple, non reentrant lock with read-write access. */
	struct SimpleReadWriteLock
	{
		struct ScopedReadLock
		{
			ScopedReadLock(SimpleReadWriteLock &lock_);
			~ScopedReadLock();
			SimpleReadWriteLock& lock;
		};

		struct ScopedWriteLock
		{
			ScopedWriteLock(SimpleReadWriteLock &lock_);
			~ScopedWriteLock();
			SimpleReadWriteLock& lock;
		};

		std::atomic<int> numReadLocks = 0;
		bool isBeingWritten = false;
	};

	mutable SimpleReadWriteLock swapLock;

	Identifier id;
	OwnedArray<MidiMessageSequence> sequences;
	int currentTrackIndex = 0;
	int lastPlayedIndex = -1;

	JUCE_DECLARE_WEAK_REFERENCEABLE(HiseMidiSequence);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HiseMidiSequence);
};



/** A MIDI File player.
*	@ingroup midiTypes
*
*	This module plays MidiFiles when its internal transport is activated.
*	It acts as common core module for multiple "overlays" (MIDI loopers, Piano roll, step sequencer, etc.)
*/
class MidiFilePlayer : public MidiProcessor,
					   public TempoListener
{
public:

	/** A Listener that will be notified when a new HiseMidiSequence was loaded. */
	struct SequenceListener
	{
		virtual ~SequenceListener() {};

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
		EditAction(WeakReference<MidiFilePlayer> currentPlayer_, const Array<HiseEvent>& newContent, double sampleRate_, double bpm_);;

		/** Applies the given event list. */
		bool perform() override;

		/** Restores the previous event list. */
		bool undo() override;

	private:

		/**@ internal */
		void writeArrayToSequence(Array<HiseEvent>& arrayToWrite);

		WeakReference<MidiFilePlayer> currentPlayer;
		Array<HiseEvent> newEvents;
		Array<HiseEvent> oldEvents;
		double sampleRate;
		double bpm;
		Identifier sequenceId;
	};

	SET_PROCESSOR_NAME("MidiFilePlayer", "MIDI File Player");

	MidiFilePlayer(MainController *mc, const String &id, ModulatorSynth* ms);;
	~MidiFilePlayer();

	/**@ internal */
	void tempoChanged(double newTempo) override;

	enum SpecialParameters
	{
		Stop = 0,				   ///< stops it at the provided timestamp (non-persistent)
		Play,					   ///< plays it at the given timestamp (non-persistent)
		Record,					   ///< starts recording at the current timestamp (non-persistent)
		CurrentPosition,		   ///< the current position within the current MIDI file (non-persistent)
		CurrentSequence,		   ///< the index of the currently played sequence (not zero based for combobox compatibility)
		CurrentTrack,			   ///< the index of the currently played track within a sequence.
		ClearSequences,			   ///< clears the sequences (non-persistent)
		numSpecialParameters
	};

	void addSequence(HiseMidiSequence::Ptr newSequence, bool select=true);
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

	/** Loads the given pooled MIDI file and adds it to the end of the list. */
	void loadMidiFile(PoolReference reference);

	/**@ internal */
	bool isProcessingWholeBuffer() const override { return true; }

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

	/** Returns the playstate as integer. */
	int getPlayState() const { return (int)playState; };

	/** Returns the number of sequences loaded into this player. */
	int getNumSequences() const { return currentSequences.size(); }

	/** Returns the currently played sequence. */
	HiseMidiSequence* getCurrentSequence() const;

	/** Returns the ID used for the given sequence. If -1 is used as index, the current sequence will be used. */
	Identifier getSequenceId(int index=-1) const;

	/** Returns the current playback position from 0...1. */
	double getPlaybackPosition() const;

	void swapCurrentSequence(MidiMessageSequence* newSequence);
	void setEnableUndoManager(bool shouldBeEnabled);

	/** Applies the list of events to the currently loaded sequence. This operation is undoable. 
	
		It locks the sequence just for a very short time so you should be able to use this from any
		thread without bothering about multithreading. */
	void flushEdit(const Array<HiseEvent>& newEvents);

	/** Returns the undo manager used for all editing operations. 
	
		This differs from the default undo manager for parameter changes because edits might
		get triggered by UI controls and it would be difficult to deinterleave parameter changes
		and MIDI edits. */
	UndoManager* getUndoManager() { return undoManager; };

	/** Resets the current sequence back to its pooled state. This operation is undoable. */
	void resetCurrentSequence();

	/** Returns the PoolReference for the given sequence. 
	
	    If -1 is passed, the current sequence index will be used. */
	PoolReference getPoolReference(int index = -1);

private:

	void sendSequenceUpdateMessage(NotificationType notification);

	ScopedPointer<UndoManager> undoManager;

	Array<PoolReference> currentlyLoadedFiles;

	Array<WeakReference<SequenceListener>> sequenceListeners;
	void changeTransportState(SpecialParameters newState);

	void play();
	void stop();
	void record();
	void updatePositionInCurrentSequence();

	ReferenceCountedArray<HiseMidiSequence> currentSequences;

	SpecialParameters playState = SpecialParameters::Stop;

	double currentPosition = -1.0;
	int currentSequenceIndex = -1;
	int currentTrackIndex = 0;

	uint16 timeStampForNextCommand = 0;

	double ticksPerSample = 0.0;
	uint16 currentTimestampInBuffer = 0;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MidiFilePlayer);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilePlayer);
};


/** Subclass this and implement your MIDI file player type. */
class MidiFilePlayerBaseType : public MidiFilePlayer::SequenceListener,
							   private SafeChangeListener
{
public:

	// "Overwrite" this with your id for the factory
	static Identifier getId() { RETURN_STATIC_IDENTIFIER("undefined"); }

	// "Overwrite" this and return a new object for the given object for the factory
	static MidiFilePlayerBaseType* create(MidiFilePlayer* player) 
	{
		ignoreUnused(player);
		return nullptr; 
	};

	virtual ~MidiFilePlayerBaseType();
	virtual int getPreferredHeight() const { return 0; }

	void setFont(Font f)
	{
		font = f;
	}

protected:

	MidiFilePlayerBaseType(MidiFilePlayer* player_);;

	MidiFilePlayer* getPlayer() { return player.get(); }
	const MidiFilePlayer* getPlayer() const { return player.get(); }

	virtual void sequenceIndexChanged() {};

	virtual void trackIndexChanged() {};
	
	

	Font getFont() const
	{
		return font;
	}

private:

	Font font;

	void changeListenerCallback(SafeChangeBroadcaster* b) override;

	int lastTrackIndex = 0;
	int lastSequenceIndex = -1;

	WeakReference<MidiFilePlayer> player;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MidiFilePlayerBaseType);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiFilePlayerBaseType);
};

#define ENABLE_OVERLAY_FACTORY(className, name) static MidiFilePlayerBaseType* create(MidiFilePlayer* player) { return new className(player); }; \
												static Identifier getId() { RETURN_STATIC_IDENTIFIER(name); };


}