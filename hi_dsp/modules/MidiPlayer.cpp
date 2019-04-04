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


namespace hise {
using namespace juce;

struct MidiPlayerHelpers
{
	static double samplesToSeconds(double samples, double sr)
	{
		return samples / sr;
	}

	static double samplesToTicks(double samples, double bpm, double sr)
	{
		auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sr, TempoSyncer::Quarter);
		return (double)HiseMidiSequence::TicksPerQuarter * samples / samplesPerQuarter;
	}

	static double secondsToTicks(double seconds, double bpm, double sr)
	{
		auto samples = secondsToSamples(seconds, sr);
		return samplesToTicks(samples, bpm, sr);
	}

	static double secondsToSamples(double seconds, double sr)
	{
		return seconds * sr;
	}

	static double ticksToSamples(double ticks, double bpm, double sr)
	{
		auto samplesPerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sr, TempoSyncer::Quarter);

		return samplesPerQuarter * ticks / HiseMidiSequence::TicksPerQuarter;
	}
};



HiseMidiSequence::SimpleReadWriteLock::ScopedReadLock::ScopedReadLock(SimpleReadWriteLock &lock_) :
	lock(lock_)
{
	for (int i = 20; --i >= 0;)
		if (!lock.isBeingWritten)
			break;

	while (lock.isBeingWritten)
		Thread::yield();

	lock.numReadLocks++;
}

HiseMidiSequence::SimpleReadWriteLock::ScopedReadLock::~ScopedReadLock()
{
	lock.numReadLocks--;
}

HiseMidiSequence::SimpleReadWriteLock::ScopedWriteLock::ScopedWriteLock(SimpleReadWriteLock &lock_) :
	lock(lock_)
{
	if (lock.isBeingWritten)
	{
		// jassertfalse;
		return;
	}

	for (int i = 100; --i >= 0;)
		if (lock.numReadLocks == 0)
			break;

	while (lock.numReadLocks > 0)
		Thread::yield();

	lock.isBeingWritten = true;
}

HiseMidiSequence::SimpleReadWriteLock::ScopedWriteLock::~ScopedWriteLock()
{
	lock.isBeingWritten = false;
}


HiseMidiSequence::HiseMidiSequence()
{

}



juce::ValueTree HiseMidiSequence::exportAsValueTree() const
{
	ValueTree v("MidiFile");
	v.setProperty("ID", id.toString(), nullptr);

	MemoryOutputStream mos;

	MidiFile currentFile;

	for (auto t : sequences)
		currentFile.addTrack(*t);

	currentFile.writeTo(mos);
	auto data = mos.getMemoryBlock();
	zstd::ZDefaultCompressor compressor;
	compressor.compressInplace(data);
	v.setProperty("Data", data.toBase64Encoding(), nullptr);

	return v;
}

void HiseMidiSequence::restoreFromValueTree(const ValueTree &v)
{
	id = v.getProperty("ID").toString();

	// This property isn't used in this class, but if you want to
	// have any kind of connection to a pooled MidiFile, you will
	// need to add this externally (see MidiPlayer::exportAsValueTree())
	jassert(v.hasProperty("FileName"));

	String encodedState = v.getProperty("Data");

	MemoryBlock mb;

	if (mb.fromBase64Encoding(encodedState))
	{
		zstd::ZDefaultCompressor compressor;
		compressor.expandInplace(mb);
		MemoryInputStream mis(mb, false);
		MidiFile mf;
		mf.readFrom(mis);
		loadFrom(mf);
	}
}


juce::MidiMessage* HiseMidiSequence::getNextEvent(Range<double> rangeToLookForTicks)
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	auto nextIndex = lastPlayedIndex + 1;

	if (auto seq = getReadPointer(currentTrackIndex))
	{
		if (nextIndex >= seq->getNumEvents())
		{
			lastPlayedIndex = -1;
			nextIndex = 0;
		}

		if (auto nextEvent = seq->getEventPointer(nextIndex))
		{
			auto timestamp = nextEvent->message.getTimeStamp();

			auto maxLength = getLength();


			if (rangeToLookForTicks.contains(timestamp))
			{
				lastPlayedIndex = nextIndex;
				return &nextEvent->message;
			}
			else if (rangeToLookForTicks.contains(maxLength))
			{
				auto rangeAtBeginning = rangeToLookForTicks.getEnd() - maxLength;

				if (timestamp < rangeAtBeginning)
				{
					lastPlayedIndex = nextIndex;
					return &nextEvent->message;
				}
			}
		}
	}

	return nullptr;
}

juce::MidiMessage* HiseMidiSequence::getMatchingNoteOffForCurrentEvent()
{
	if (auto noteOff = getReadPointer(currentTrackIndex)->getEventPointer(lastPlayedIndex)->noteOffObject)
		return &noteOff->message;

	return nullptr;
}

double HiseMidiSequence::getLength() const
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (artificialLengthInQuarters != -1.0)
		return artificialLengthInQuarters * (double)TicksPerQuarter;

	if (auto currentSequence = sequences.getFirst())
		return currentSequence->getEndTime();

	return 0.0;
}

double HiseMidiSequence::getLengthInQuarters()
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (artificialLengthInQuarters != -1.0)
		return artificialLengthInQuarters;

	if (auto currentSequence = sequences.getFirst())
		return currentSequence->getEndTime() / (double)TicksPerQuarter;

	return 0.0;
}


void HiseMidiSequence::setLengthInQuarters(double newLength)
{
	artificialLengthInQuarters = newLength;
	
}

void HiseMidiSequence::loadFrom(const MidiFile& file)
{
	OwnedArray<MidiMessageSequence> newSequences;

	MidiFile normalisedFile;

	for (int i = 0; i < file.getNumTracks(); i++)
	{
		ScopedPointer<MidiMessageSequence> newSequence = new MidiMessageSequence(*file.getTrack(i));
		newSequence->deleteSysExMessages();

		DBG("Track " + String(i + 1));

		for (int j = 0; j < newSequence->getNumEvents(); j++)
		{
			if (newSequence->getEventPointer(j)->message.isMetaEvent())
				newSequence->deleteEvent(j--, false);
		}

		if (newSequence->getNumEvents() > 0)
			normalisedFile.addTrack(*newSequence);
	}

	normalisedFile.setTicksPerQuarterNote(TicksPerQuarter);

	for (int i = 0; i < normalisedFile.getNumTracks(); i++)
	{
		ScopedPointer<MidiMessageSequence> newSequence = new MidiMessageSequence(*normalisedFile.getTrack(i));
		newSequences.add(newSequence.release());
	}

	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
		newSequences.swapWith(sequences);
	}
}

void HiseMidiSequence::createEmptyTrack()
{
	ScopedPointer<MidiMessageSequence> newTrack = new MidiMessageSequence();

	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
		sequences.add(newTrack.release());
		currentTrackIndex = sequences.size() - 1;
		lastPlayedIndex = -1;
	}
}

juce::File HiseMidiSequence::writeToTempFile()
{
	MidiFile f;

	for (int i = 0; i < sequences.size(); i++)
	{
		f.addTrack(*sequences[i]);
	}

	auto tmp = File::getSpecialLocation(File::SpecialLocationType::tempDirectory).getNonexistentChildFile(id.toString(), ".mid");
	tmp.create();

	FileOutputStream fos(tmp);
	f.writeTo(fos);
	return tmp;
}

void HiseMidiSequence::setId(const Identifier& newId)
{
	id = newId;
}

const juce::MidiMessageSequence* HiseMidiSequence::getReadPointer(int trackIndex) const
{
	if (trackIndex == -1)
		return sequences[currentTrackIndex];

	return sequences[trackIndex];
}

juce::MidiMessageSequence* HiseMidiSequence::getWritePointer(int trackIndex)
{
	if (trackIndex == -1)
		return sequences[currentTrackIndex];

	return sequences[trackIndex];
}

int HiseMidiSequence::getNumEvents() const
{
	return sequences[currentTrackIndex]->getNumEvents();
}

void HiseMidiSequence::setCurrentTrackIndex(int index)
{
	if (isPositiveAndBelow(index, sequences.size()) && index != currentTrackIndex)
	{
		double lastTimestamp = 0.0;

		SimpleReadWriteLock::ScopedReadLock sl(swapLock);

		if (lastPlayedIndex != -1)
			lastTimestamp = getReadPointer(currentTrackIndex)->getEventPointer(lastPlayedIndex)->message.getTimeStamp();

		currentTrackIndex = jlimit<int>(0, sequences.size()-1, index);

		if (lastPlayedIndex != -1)
			lastPlayedIndex = getReadPointer(currentTrackIndex)->getNextIndexAtTime(lastTimestamp);
	}
}

void HiseMidiSequence::resetPlayback()
{
	lastPlayedIndex = -1;
}

void HiseMidiSequence::setPlaybackPosition(double normalisedPosition)
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (auto s = getReadPointer(currentTrackIndex))
	{
		auto currentTimestamp = getLength() * normalisedPosition;

		lastPlayedIndex = s->getNextIndexAtTime(currentTimestamp) - 1;
	}
}

juce::RectangleList<float> HiseMidiSequence::getRectangleList(Rectangle<float> targetBounds) const
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	RectangleList<float> list;

	if (auto s = getReadPointer(currentTrackIndex))
	{
		for (auto e : *s)
		{
			if (e->message.isNoteOn() && e->noteOffObject != nullptr)
			{
				auto x = (float)(e->message.getTimeStamp() / getLength());
				auto w = (float)(e->noteOffObject->message.getTimeStamp() / getLength()) - x;
				auto y = (float)(127 - e->message.getNoteNumber());
				auto h = 1.0f;

				list.add({ x, y, w, h });
			}
		}
	}

	if (!targetBounds.isEmpty())
	{
		auto bounds = list.getBounds();
		list.offsetAll(0.0f, -bounds.getY());
		auto scaler = AffineTransform::scale(targetBounds.getWidth() / bounds.getRight(), targetBounds.getHeight() / bounds.getHeight());
		list.transformAll(scaler);
		list.offsetAll(0.0f, targetBounds.getY());
	}

	return list;
}


Array<HiseEvent> HiseMidiSequence::getEventList(double sampleRate, double bpm)
{
	Array<HiseEvent> newBuffer;
	newBuffer.ensureStorageAllocated(getNumEvents());

	Range<double> range = { 0.0, getLength() };

	auto samplePerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);

	uint16 eventIds[127];
	uint16 currentEventId = 1;
	memset(eventIds, 0, sizeof(uint16) * 127);

	auto mSeq = getReadPointer();

	for (const auto& ev : *mSeq)
	{
		auto m = ev->message;

		auto timeStamp = (int)(samplePerQuarter * m.getTimeStamp() / (double)HiseMidiSequence::TicksPerQuarter);
		HiseEvent newEvent(m);
		newEvent.setTimeStamp(timeStamp);

		if (newEvent.isNoteOn())
		{
			newEvent.setEventId(currentEventId);
			eventIds[newEvent.getNoteNumber()] = currentEventId++;
		}
		if (newEvent.isNoteOff())
			newEvent.setEventId(eventIds[newEvent.getNoteNumber()]);

		newBuffer.add(newEvent);
	}

	return newBuffer;
}



void HiseMidiSequence::swapCurrentSequence(MidiMessageSequence* sequenceToSwap)
{
	ScopedPointer<MidiMessageSequence> oldSequence = sequences[currentTrackIndex];

	{
		SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
		sequences.set(currentTrackIndex, sequenceToSwap, false);
	}
	
	oldSequence = nullptr;
}


MidiPlayer::EditAction::EditAction(WeakReference<MidiPlayer> currentPlayer_, const Array<HiseEvent>& newContent, double sampleRate_, double bpm_) :
	UndoableAction(),
	currentPlayer(currentPlayer_),
	newEvents(newContent),
	sampleRate(sampleRate_),
	bpm(bpm_)
{
	if (auto seq = currentPlayer->getCurrentSequence())
		oldEvents = seq->getEventList(sampleRate, bpm);

	if (currentPlayer == nullptr)
		return;

	if (auto seq = currentPlayer->getCurrentSequence())
		sequenceId = seq->getId();
}

bool MidiPlayer::EditAction::perform()
{
	if (currentPlayer != nullptr && currentPlayer->getSequenceId() == sequenceId)
	{
		writeArrayToSequence(currentPlayer->getCurrentSequence(),
							 newEvents,
							 currentPlayer->getMainController()->getBpm(),
							 currentPlayer->getSampleRate());

		currentPlayer->updatePositionInCurrentSequence();
		currentPlayer->sendSequenceUpdateMessage(sendNotificationAsync);
		return true;
	}
		
	return false;
}

bool MidiPlayer::EditAction::undo()
{
	if (currentPlayer != nullptr && currentPlayer->getSequenceId() == sequenceId)
	{
		writeArrayToSequence(currentPlayer->getCurrentSequence(), 
			                 oldEvents, 
			                 currentPlayer->getMainController()->getBpm(),
							 currentPlayer->getSampleRate());

		currentPlayer->updatePositionInCurrentSequence();
		currentPlayer->sendSequenceUpdateMessage(sendNotificationAsync);
		return true;
	}

	return false;
}

void MidiPlayer::EditAction::writeArrayToSequence(HiseMidiSequence::Ptr destination, const Array<HiseEvent>& arrayToWrite, double bpm, double sampleRate)
{
	ScopedPointer<MidiMessageSequence> newSeq = new MidiMessageSequence();

	auto samplePerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);

	for (const auto& e : arrayToWrite)
	{
		if (e.isEmpty())
			continue;

		auto timeStamp = ((double)e.getTimeStamp() / samplePerQuarter) * (double)HiseMidiSequence::TicksPerQuarter;
		auto m = e.toMidiMesage();
		m.setTimeStamp(timeStamp);
		newSeq->addEvent(m);
	}

	newSeq->sort();
	newSeq->updateMatchedPairs();

	destination->swapCurrentSequence(newSeq.release());
}

MidiPlayer::MidiPlayer(MainController *mc, const String &id, ModulatorSynth* ) :
	MidiProcessor(mc, id),
	undoManager(new UndoManager())
{
	addAttributeID(Stop);
	addAttributeID(Play);
	addAttributeID(Record);
	addAttributeID(CurrentPosition);
	addAttributeID(CurrentSequence);
	addAttributeID(CurrentTrack);
	addAttributeID(ClearSequences);

	mc->addTempoListener(this);

}

MidiPlayer::~MidiPlayer()
{
	getMainController()->removeTempoListener(this);
}

void MidiPlayer::tempoChanged(double newTempo)
{
	ticksPerSample = MidiPlayerHelpers::samplesToTicks(1, newTempo, getSampleRate());
}

juce::ValueTree MidiPlayer::exportAsValueTree() const
{
	ValueTree v = MidiProcessor::exportAsValueTree();

	saveID(CurrentSequence);
	saveID(CurrentTrack);
	saveID(LoopEnabled);

	ValueTree seq("MidiFiles");

	for (int i = 0; i < currentSequences.size(); i++)
	{
		auto s = currentSequences[i]->exportAsValueTree();
		s.setProperty("FileName", currentlyLoadedFiles[i].getReferenceString(), nullptr);

		seq.addChild(s, -1, nullptr);
	}
		
	v.addChild(seq, -1, nullptr);

	return v;
}

void MidiPlayer::restoreFromValueTree(const ValueTree &v)
{
	MidiProcessor::restoreFromValueTree(v);

	ValueTree seq = v.getChildWithName("MidiFiles");

	clearSequences(dontSendNotification);

	if (seq.isValid())
	{
		for (const auto& s : seq)
		{
			HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();
			newSequence->restoreFromValueTree(s);

			PoolReference ref(getMainController(), s.getProperty("FileName", ""), FileHandlerBase::MidiFiles);

			currentlyLoadedFiles.add(ref);

			addSequence(newSequence, false);
		}
	}

	loadID(CurrentSequence);
	loadID(CurrentTrack);
	loadID(LoopEnabled);
}

void MidiPlayer::addSequence(HiseMidiSequence::Ptr newSequence, bool select)
{
	currentSequences.add(newSequence);

	if (select)
	{
		currentSequenceIndex = currentSequences.size() - 1;
		sendChangeMessage();
	}

	sendSequenceUpdateMessage(sendNotificationAsync);
}

void MidiPlayer::clearSequences(NotificationType notifyListeners)
{
	if (undoManager != nullptr)
		undoManager->clearUndoHistory();

	currentSequences.clear();
	currentlyLoadedFiles.clear();

	currentSequenceIndex = -1;
	currentlyRecordedEvents.clear();
	recordState.store(RecordState::Idle);


	if (notifyListeners != dontSendNotification)
	{
		for (auto l : sequenceListeners)
		{
			if (l != nullptr)
				l->sequencesCleared();
		}
	}
}

hise::ProcessorEditorBody * MidiPlayer::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new MidiPlayerEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

float MidiPlayer::getAttribute(int index) const
{
	auto s = (SpecialParameters)index;

	switch (s)
	{
	case CurrentPosition:		return (float)getPlaybackPosition();
	case CurrentSequence:		return (float)(currentSequenceIndex + 1);
	case CurrentTrack:			return (float)(currentTrackIndex + 1);
	case LoopEnabled:			return loopEnabled ? 1.0f : 0.0f;
	default:
		break;
	}

	return 0.0f;
}

void MidiPlayer::setInternalAttribute(int index, float newAmount)
{
	auto s = (SpecialParameters)index;

	switch (s)
	{
	case CurrentPosition:		
	{
		currentPosition = jlimit<double>(0.0, 1.0, (double)newAmount); 

		updatePositionInCurrentSequence();
		break;
	}
	case CurrentSequence:		
	{
		double lastLength = 0.0f;

		if (auto seq = getCurrentSequence())
			lastLength = seq->getLengthInQuarters();

		currentSequenceIndex = jlimit<int>(-1, currentSequences.size(), (int)(newAmount - 1)); 

		currentlyRecordedEvents.clear();
		recordState.store(RecordState::Idle);

		if (auto seq = getCurrentSequence())
		{
			double newLength = seq->getLengthInQuarters();

			if (newLength > 0.0 && currentPosition >= 0.0)
			{
				double ratio = lastLength / newLength;
				currentPosition = currentPosition * ratio;
				
				updatePositionInCurrentSequence();
			}
		}

		break;
	}
	case CurrentTrack:			
	{
		currentTrackIndex = jmax<int>(0, (int)(newAmount - 1)); 

		if(auto seq = getCurrentSequence())
			seq->setCurrentTrackIndex(currentTrackIndex);

		currentlyRecordedEvents.clear();
		recordState.store(RecordState::Idle);

		break;
	}
	case LoopEnabled: loopEnabled = newAmount > 0.5f; break;
	default:
		break;
	}
}


void MidiPlayer::loadMidiFile(PoolReference reference)
{
	PooledMidiFile newContent;

#if HISE_ENABLE_EXPANSIONS

	if (auto e = getMainController()->getExpansionHandler().getExpansionForWildcardReference(reference.getReferenceString()))
		newContent = e->pool->getMidiFilePool().loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);
	else
		newContent = getMainController()->getCurrentMidiFilePool()->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);

#else

	newContent = getMainController()->getCurrentMidiFilePool()->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);

#endif

	currentlyLoadedFiles.add(reference);

	HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();
	newSequence->loadFrom(newContent->data.getFile());
	//newSequence->setId(reference.getFile().getFileNameWithoutExtension());
	addSequence(newSequence);
	
}

void MidiPlayer::prepareToPlay(double sampleRate_, int samplesPerBlock_)
{
	MidiProcessor::prepareToPlay(sampleRate_, samplesPerBlock_);
	tempoChanged(getMainController()->getBpm());
}

void MidiPlayer::preprocessBuffer(HiseEventBuffer& buffer, int numSamples)
{
	lastBlockSize = numSamples;

	if (currentSequenceIndex >= 0 && currentPosition != -1.0)
	{
		if (!loopEnabled && currentPosition > 1.0)
		{
			stop();
			return;
		}

		auto seq = getCurrentSequence();
		seq->setCurrentTrackIndex(currentTrackIndex);

		auto tickThisTime = (numSamples - timeStampForNextCommand) * ticksPerSample;
		auto lengthInTicks = seq->getLength();

		auto positionInTicks = getPlaybackPosition() * lengthInTicks;

		auto delta = tickThisTime / lengthInTicks;

		Range<double> currentRange;
		
		if(loopEnabled)
			currentRange = { positionInTicks, positionInTicks + tickThisTime };
		else
			currentRange = { positionInTicks, jmin<double>(lengthInTicks, positionInTicks + tickThisTime) };

		while (auto e = seq->getNextEvent(currentRange))
		{
			auto timeStampInThisBuffer = e->getTimeStamp() - positionInTicks;

			if (timeStampInThisBuffer < 0.0)
				timeStampInThisBuffer += lengthInTicks;

			auto timeStamp = (int)MidiPlayerHelpers::ticksToSamples(timeStampInThisBuffer, getMainController()->getBpm(), getSampleRate());
			timeStamp += timeStampForNextCommand;

			jassert(isPositiveAndBelow(timeStamp, numSamples));

			HiseEvent newEvent(*e);

			newEvent.setTimeStamp(timeStamp);
			newEvent.setArtificial();

			if (newEvent.isNoteOn())
			{
				getMainController()->getEventHandler().pushArtificialNoteOn(newEvent);

				buffer.addEvent(newEvent);

				if (auto noteOff = seq->getMatchingNoteOffForCurrentEvent())
				{
					HiseEvent newNoteOff(*noteOff);
					newNoteOff.setArtificial();

					auto noteOffTimeStampInBuffer = noteOff->getTimeStamp() - positionInTicks;
					
					if (noteOffTimeStampInBuffer < 0.0)
						noteOffTimeStampInBuffer += lengthInTicks;

					timeStamp += timeStampForNextCommand;

					auto noteOffTimeStamp = (int)MidiPlayerHelpers::ticksToSamples(noteOffTimeStampInBuffer, getMainController()->getBpm(), getSampleRate());

					auto on_id = getMainController()->getEventHandler().getEventIdForNoteOff(newNoteOff);

					jassert(newEvent.getEventId() == on_id);

					newNoteOff.setEventId(on_id);
					newNoteOff.setTimeStamp(noteOffTimeStamp);

					if (noteOffTimeStamp < numSamples)
						buffer.addEvent(newNoteOff);
					else
						addHiseEventToBuffer(newNoteOff);
				}
			}
		}

		timeStampForNextCommand = 0;
		currentPosition += delta;
	}
}

void MidiPlayer::processHiseEvent(HiseEvent &m) noexcept
{
	currentTimestampInBuffer = m.getTimeStamp();

	if (isRecording() && !m.isArtificial() && recordState == RecordState::Prepared)
	{
		if (auto seq = getCurrentSequence())
		{
			auto lengthInQuarters = seq->getLengthInQuarters() * getPlaybackPosition();
			auto ticks = lengthInQuarters * HiseMidiSequence::TicksPerQuarter;

			auto timestampSamples = (int)MidiPlayerHelpers::ticksToSamples(ticks, getMainController()->getBpm(), getSampleRate());

			// This will be processed after the position has advanced so we need to subtract the last blocksize and add the message's timestamp.
			timestampSamples -= lastBlockSize;
			timestampSamples += currentTimestampInBuffer;

			HiseEvent copy(m);
			copy.setTimeStamp(timestampSamples);

			currentlyRecordedEvents.add(copy);
		}
	}
}

void MidiPlayer::addSequenceListener(SequenceListener* newListener)
{
	sequenceListeners.addIfNotAlreadyThere(newListener);
}

void MidiPlayer::removeSequenceListener(SequenceListener* listenerToRemove)
{
	sequenceListeners.removeAllInstancesOf(listenerToRemove);
}


void MidiPlayer::sendSequenceUpdateMessage(NotificationType notification)
{
	auto update = [this]()
	{
		for (auto l : sequenceListeners)
		{
			if (l != nullptr)
				l->sequenceLoaded(getCurrentSequence());
		}
	};

	if (notification == sendNotificationAsync)
		MessageManager::callAsync(update);
	else
		update();
}

void MidiPlayer::changeTransportState(PlayState newState)
{
	switch (newState)
	{						// Supposed to be immediately...
	case PlayState::Play:	play(0);	return;
	case PlayState::Stop:	stop(0);	return;
	case PlayState::Record: record(0);	return;
	}
}

hise::HiseMidiSequence* MidiPlayer::getCurrentSequence() const
{
	return currentSequences[currentSequenceIndex].get();
}

juce::Identifier MidiPlayer::getSequenceId(int index) const
{
	if (index == -1)
		index = currentSequenceIndex;

	if (auto s = currentSequences[index])
	{
		return s->getId();
	}

	return {};
}

double MidiPlayer::getPlaybackPosition() const
{
	return fmod(currentPosition, 1.0);
}

void MidiPlayer::swapCurrentSequence(MidiMessageSequence* newSequence)
{
	getCurrentSequence()->swapCurrentSequence(newSequence);

	updatePositionInCurrentSequence();
	sendSequenceUpdateMessage(sendNotificationAsync);
}


void MidiPlayer::setEnableUndoManager(bool shouldBeEnabled)
{
	bool isEnabled = undoManager != nullptr;

	if (isEnabled != shouldBeEnabled)
	{
		undoManager = nullptr;

		if (isEnabled)
			undoManager = new UndoManager();
	}
}

void MidiPlayer::flushEdit(const Array<HiseEvent>& newEvents)
{
	ScopedPointer<EditAction> newAction = new EditAction(this, newEvents, getSampleRate(), getMainController()->getBpm());

	if (undoManager != nullptr)
	{
		undoManager->beginNewTransaction();
		undoManager->perform(newAction.release());
	}
	else
		newAction->perform();
}

void MidiPlayer::resetCurrentSequence()
{
	if (auto seq = getCurrentSequence())
	{
		auto original = getMainController()->getCurrentMidiFilePool()->loadFromReference(currentlyLoadedFiles[currentSequenceIndex], PoolHelpers::LoadAndCacheWeak);

		ScopedPointer<HiseMidiSequence> tempSeq = new HiseMidiSequence();
		tempSeq->loadFrom(original->data.getFile());
		auto l = tempSeq->getEventList(getSampleRate(), getMainController()->getBpm());

		flushEdit(l);
	}
}

hise::PoolReference MidiPlayer::getPoolReference(int index /*= -1*/)
{
	if (index == -1)
		index = currentSequenceIndex;

	return currentlyLoadedFiles[index];
}

bool MidiPlayer::play(int timestamp)
{
	sendAllocationFreeChangeMessage();

	if (auto seq = getCurrentSequence())
	{
		if (isRecording())
			finishRecording();
		else
		{
			// This allows switching from record to play while maintaining the position
			currentPosition = 0.0;
			seq->resetPlayback();
		}
			
		playState = PlayState::Play;
		timeStampForNextCommand = timestamp;
		
		return true;
	}
		
	return false;
}

bool MidiPlayer::stop(int timestamp)
{
	sendAllocationFreeChangeMessage();

	if (auto seq = getCurrentSequence())
	{
		seq->resetPlayback();
		playState = PlayState::Stop;
		timeStampForNextCommand = timestamp;
		currentPosition = -1.0;
		return true;
	}
		
	return false;
}

bool MidiPlayer::record(int timestamp)
{
	sendAllocationFreeChangeMessage();

	if (playState == PlayState::Stop)
	{
		currentPosition = 0.0;

		if(auto seq = getCurrentSequence())
			seq->resetPlayback();
	}

	playState = PlayState::Record;
	timeStampForNextCommand = timestamp;

	if (recordState == RecordState::Idle)
		prepareForRecording(true);

	return false;
}


void MidiPlayer::prepareForRecording(bool copyExistingEvents/*=true*/)
{
	auto f = [copyExistingEvents](Processor* p)
	{
		auto mp = static_cast<MidiPlayer*>(p);

		Array<HiseEvent> newEvents;

		if (auto seq = mp->getCurrentSequence())
		{
			if (copyExistingEvents)
            {
                auto newList = seq->getEventList(p->getSampleRate(), p->getMainController()->getBpm());
				newEvents.swapWith(newList);
            }
		}

		newEvents.ensureStorageAllocated(2048);

		mp->currentlyRecordedEvents.swapWith(newEvents);
		mp->recordState.store(RecordState::Prepared);

		return SafeFunctionCall::OK;
	};

	recordState.store(RecordState::PreparationPending);
	getMainController()->getSampleManager().addDeferredFunction(this, f);
}

void MidiPlayer::finishRecording()
{
	auto f = [](Processor* p)
	{
		auto mp = static_cast<MidiPlayer*>(p);

		auto& l = mp->currentlyRecordedEvents;

		int lastTimestamp = (int)MidiPlayerHelpers::ticksToSamples(mp->getCurrentSequence()->getLength(), mp->getMainController()->getBpm(), mp->getSampleRate()) - 1;

		for (int i = 0; i < l.size(); i++)
		{
			auto currentEvent = l[i];

			if (currentEvent.isNoteOn())
			{
				bool found = false;

				for (int j = 0; j < l.size(); j++)
				{
					if (l[j].isNoteOff() && l[j].getEventId() == currentEvent.getEventId())
					{
						if (l[j].getTimeStamp() < currentEvent.getTimeStamp())
						{
							l.getReference(j).setTimeStamp(lastTimestamp);
						}
						
						found = true;
						break;
					}
				}

				if (!found)
				{
					HiseEvent artificialNoteOff(HiseEvent::Type::NoteOff, (uint8)currentEvent.getNoteNumber(), 1, (uint8)currentEvent.getChannel());
					artificialNoteOff.setTimeStamp(lastTimestamp);
					artificialNoteOff.setEventId(currentEvent.getEventId());
					l.add(artificialNoteOff);
				}
			}

			if (currentEvent.isNoteOff())
			{
				bool found = false;

				for (int j = 0; j < l.size(); j++)
				{
					if (l[j].isNoteOn() && currentEvent.getEventId() == l[j].getEventId())
					{
						found = true;
						break;
					}
				}

				if (!found)
					l.remove(i--);
			}
		}

		mp->flushEdit(mp->currentlyRecordedEvents);

		// This is a shortcut because the preparation would just fill it with the current sequence.
		mp->recordState.store(RecordState::Prepared);

		return SafeFunctionCall::OK;
	};

	getMainController()->getSampleManager().addDeferredFunction(this, f);
	recordState.store(RecordState::FlushPending);
}

hise::HiseMidiSequence::Ptr MidiPlayer::getListOfCurrentlyRecordedEvents()
{
	HiseMidiSequence::Ptr recordedList = new HiseMidiSequence();
	recordedList->createEmptyTrack();
	EditAction::writeArrayToSequence(recordedList, currentlyRecordedEvents, getMainController()->getBpm(), getSampleRate());
	return recordedList;
}

void MidiPlayer::updatePositionInCurrentSequence()
{
	if (auto seq = getCurrentSequence())
	{
		seq->setPlaybackPosition(getPlaybackPosition());
	}
}

MidiPlayerBaseType::~MidiPlayerBaseType()
{
	

	if (player != nullptr)
	{
		player->removeSequenceListener(this);
		player->removeChangeListener(this);
	}
}

MidiPlayerBaseType::MidiPlayerBaseType(MidiPlayer* player_) :
	player(player_),
	font(GLOBAL_BOLD_FONT())
{
	initMidiPlayer(player);
}

void MidiPlayerBaseType::initMidiPlayer(MidiPlayer* newPlayer)
{
	player = newPlayer;

	if (player != nullptr)
	{
		player->addSequenceListener(this);
		player->addChangeListener(this);
	}
}



void MidiPlayerBaseType::changeListenerCallback(SafeChangeBroadcaster* )
{
	int thisSequence = (int)getPlayer()->getAttribute(MidiPlayer::CurrentSequence);

	if (thisSequence != lastSequenceIndex)
	{
		lastSequenceIndex = thisSequence;
		sequenceIndexChanged();
	}

	int trackIndex = (int)getPlayer()->getAttribute(MidiPlayer::CurrentTrack);

	if (trackIndex != lastTrackIndex)
	{
		lastTrackIndex = trackIndex;
		trackIndexChanged();
	}
}

}
