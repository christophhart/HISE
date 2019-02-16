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

/** A wrapper around a MIDI sequence. 

	This uses the juce::MidiMessageSequence object but extends it with these capabilities:
	
	- reference counted
	- save to / load from ValueTrees
	- unique ID (for pooling)
	- normalised tempo
	- defined length
	- a pointer to the last played note

	They act as data structure for the MIDIFilePlayer.
*/
class HiseMidiSequence : public ReferenceCountedObject,
						 public RestorableObject
{
public:

	static constexpr int TicksPerQuarter = 960;

	using Ptr = ReferenceCountedObjectPtr<HiseMidiSequence>;

	HiseMidiSequence();

	/** Saves the sequence into a ValueTree. It stores the ID and the
	    data as compressed MIDI file.
	*/
	ValueTree exportAsValueTree() const override;

	void restoreFromValueTree(const ValueTree &v) override;

	MidiMessage* getNextEvent(Range<double> rangeToLookForTicks);
	MidiMessage* getMatchingNoteOffForCurrentEvent();

	double getLength() const;
	double getLengthInQuarters();

	/** Loads a sequence from a MIDI file input stream. */
	void loadFrom(InputStream& input);
	File writeToTempFile();

	void setId(const Identifier& newId);
	Identifier getId() const noexcept { return id; }

	const juce::MidiMessageSequence* getReadPointer(int trackIndex=-1) const;
	juce::MidiMessageSequence* getWritePointer(int trackIndex=-1);

	int getNumEvents() const;
	int getNumTracks() const { return sequences.size(); }
	void setCurrentTrackIndex(int index);
	void resetPlayback();

	void setPlaybackPosition(double normalisedPosition);

	RectangleList<float> getRectangleList(Rectangle<float> targetBounds) const;

	void swapCurrentSequence(MidiMessageSequence* sequenceToSwap);

	void reset()
	{
		if (loadedFile.existsAsFile())
		{
			FileInputStream fis(loadedFile);
			loadFrom(fis);
		}
	}

private:

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

	File loadedFile;

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

		virtual void sequenceLoaded(HiseMidiSequence::Ptr newSequence) = 0;
		virtual void sequencesCleared() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(SequenceListener);
	};

	SET_PROCESSOR_NAME("MidiFilePlayer", "MIDI File Player");

	MidiFilePlayer(MainController *mc, const String &id, ModulatorSynth* ms);;
	~MidiFilePlayer();

	void tempoChanged(double newTempo) override;

	enum SpecialParameters
	{
		Stop = 0,				   ///< stops it at the provided timestamp (non-persistent)
		Play,					   ///< stops it at the given timestamp (non-persistent)
		Record,					   ///< starts recording at the current timestamp (non-persistent)
		CurrentPosition,		   ///< the current position within the current MIDI file (non-persistent)
		CurrentSequence,		   ///< the index of the currently played sequence (not zero based for combobox compatibility)
		CurrentTrack,			   ///< the index of the currently played track within a sequence.
		ClearSequences,			   ///< clears the sequences (non-persistent)
		numSpecialParameters
	};

	ValueTree exportAsValueTree() const override;;
	virtual void restoreFromValueTree(const ValueTree &v) override;

	void addSequence(HiseMidiSequence::Ptr newSequence);
	void clearSequences();

	ProcessorEditorBody *createEditor(ProcessorEditor *parentEditor)  override;

	float getAttribute(int index) const;;
	void setInternalAttribute(int index, float newAmount) override;;

	bool isProcessingWholeBuffer() const override { return true; }
	void prepareToPlay(double sampleRate_, int samplesPerBlock_);
	void preprocessBuffer(HiseEventBuffer& buffer, int numSamples) override;
	void processHiseEvent(HiseEvent &m) noexcept override;

	void addSequenceListener(SequenceListener* newListener);
	void removeSequenceListener(SequenceListener* listenerToRemove);

	int getPlayState() const { return (int)playState; };
	int getNumSequences() const { return currentSequences.size(); }
	HiseMidiSequence* getCurrentSequence() const;
	Identifier getSequenceId(int index) const;
	Identifier getCurrentSequenceId() const;
	double getPlaybackPosition() const;

	void swapCurrentSequence(MidiMessageSequence* newSequence);

private:

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
};



/** A class for editing MIDI sequences. */
class MidiSequenceEditor
{
public:

	class Action : public UndoableAction
	{
	public:

		Action(WeakReference<MidiFilePlayer> currentPlayer_, const Array<HiseEvent>& newContent, double sampleRate_, double bpm_) :
			UndoableAction(),
			currentPlayer(currentPlayer_),
			newEvents(newContent),
			sampleRate(sampleRate_),
			bpm(bpm_)
		{
			oldEvents = createBufferFromSequence(currentPlayer->getCurrentSequence(), sampleRate, bpm);
			
			if (currentPlayer == nullptr)
				return;

			if (auto seq = currentPlayer->getCurrentSequence())
				sequenceId = seq->getId();
		};

		bool perform() override;
		bool undo() override;

	private:

		void writeArrayToSequence(Array<HiseEvent>& arrayToWrite);

		
		WeakReference<MidiFilePlayer> currentPlayer;
		Array<HiseEvent> newEvents;
		Array<HiseEvent> oldEvents;
		double sampleRate;
		double bpm;
		Identifier sequenceId;
	};

	MidiSequenceEditor(MidiFilePlayer* player_) :
		player(player_)
	{}

	static Array<HiseEvent> createBufferFromSequence(HiseMidiSequence::Ptr seq, double sampleRate, double bpm);

	/** Copy the current sequence from the given MIDI sequence. */
	bool copySequencyFrom(HiseMidiSequence::Ptr sequence);

	/** Removes and returns the HiseEvent at the position i. */
	HiseEvent popHiseEvent(int index);

	HiseEvent getNoteOffForEventId(int eventId);
	HiseEvent getNoteOnForEventId(int eventId);

	/** Adds the given message. */
	void pushHiseEvent(const HiseEvent& e);

	/** Writes the processed sequence back to the original sequence. */
	void write();

	void setEvents(const Array<HiseEvent>& events);

	WeakReference<MidiFilePlayer> player;
	Array<HiseEvent> currentEvents;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MidiSequenceEditor);
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