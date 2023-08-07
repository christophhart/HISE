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


void HiseMidiSequence::TimeSignature::setLoopEnd(double normalisedEnd)
{
	jassert(numBars > 0.0);
	jassert(denominator != 0.0);
	auto beatLengthNormalised = 1.0 / (numBars * denominator);
	normalisedEnd = jmax(normalisedLoopRange.getStart() + beatLengthNormalised, normalisedEnd);
	normalisedLoopRange.setEnd(normalisedEnd);
}

void HiseMidiSequence::TimeSignature::setLoopStart(double normalisedStart)
{
	jassert(numBars > 0.0);
	jassert(denominator != 0.0);
	auto beatLengthNormalised = 1.0 / (numBars * denominator);

	normalisedStart = jmin(normalisedStart, normalisedLoopRange.getEnd() - beatLengthNormalised);
	normalisedLoopRange.setStart(normalisedStart);
}

void HiseMidiSequence::TimeSignature::calculateNumBars(double lengthInQuarters, bool roundToQuarter)
{
	if (roundToQuarter)
		lengthInQuarters = hmath::ceil(lengthInQuarters);

	numBars = lengthInQuarters * denominator / 4.0 / nominator;
}

String HiseMidiSequence::TimeSignature::toString() const
{
	String s;
	s << roundToInt(numBars) << " of ";
	s << roundToInt(nominator) << "/";
	s << roundToInt(denominator);
	return s;
}

double HiseMidiSequence::TimeSignature::getNumQuarters() const
{
	return numBars / denominator * 4.0 * nominator;
}

ValueTree HiseMidiSequence::TimeSignature::exportAsValueTree() const
{
	ValueTree v("TimeSignature");
	v.setProperty(TimeSigIds::NumBars, numBars, nullptr);
	v.setProperty(TimeSigIds::Nominator, nominator, nullptr);
	v.setProperty(TimeSigIds::Denominator, denominator, nullptr);
	v.setProperty(TimeSigIds::LoopStart, normalisedLoopRange.getStart(), nullptr);
	v.setProperty(TimeSigIds::LoopEnd, normalisedLoopRange.getEnd(), nullptr);
	v.setProperty(TimeSigIds::Tempo, bpm, nullptr);

	return v;
}

var HiseMidiSequence::TimeSignature::getAsJSON() const
{
	DynamicObject::Ptr obj = new DynamicObject();
			
	auto v = exportAsValueTree();

	for (int i = 0; i < v.getNumProperties(); i++)
	{
		auto id = v.getPropertyName(i);
		obj->setProperty(id, v[id]);
	}
			
	return var(obj.get());
}

void HiseMidiSequence::TimeSignature::restoreFromValueTree(const ValueTree& v)
{
	numBars =					 v.getProperty(TimeSigIds::NumBars, 0.0);
	nominator =					 v.getProperty(TimeSigIds::Nominator, 4.0);
	denominator =				 v.getProperty(TimeSigIds::Denominator, 4.0);
	normalisedLoopRange.setStart(v.getProperty(TimeSigIds::LoopStart, 0.0));
	normalisedLoopRange.setEnd(	 v.getProperty(TimeSigIds::LoopEnd, 1.0));
	bpm =						 v.getProperty(TimeSigIds::Tempo, 120.0);
}

HiseMidiSequence::Ptr HiseMidiSequence::clone() const
{
	HiseMidiSequence::Ptr newSeq = new HiseMidiSequence();
	newSeq->restoreFromValueTree(exportAsValueTree());
	return newSeq;
}

HiseMidiSequence::TimeSignature HiseMidiSequence::getTimeSignature() const
{ return signature; }

HiseMidiSequence::TimeSignature* HiseMidiSequence::getTimeSignaturePtr()
{ return &signature; }

Identifier HiseMidiSequence::getId() const noexcept
{ return id; }

int HiseMidiSequence::getNumTracks() const
{ return sequences.size(); }

HiseMidiSequence::TimestampEditFormat HiseMidiSequence::getTimestampEditFormat() const
{ return timestampFormat; }

void HiseMidiSequence::setTimeStampEditFormat(TimestampEditFormat formatToUse)
{
	timestampFormat = formatToUse;
}

HiseMidiSequence::HiseMidiSequence()
{

}



juce::ValueTree HiseMidiSequence::exportAsValueTree() const
{
	ValueTree v("MidiFile");
	v.setProperty("ID", id.toString(), nullptr);

	v.addChild(signature.exportAsValueTree(), -1, nullptr);

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
	auto id_ = v.getProperty("ID").toString();

	if (id_.isNotEmpty())
		id = Identifier(id_);

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

		auto tsData = v.getChildWithName("TimeSignature");

		if (tsData.isValid())
		{
			signature.restoreFromValueTree(tsData);
			setLengthFromTimeSignature(signature);
		}
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

		auto loopEndTicks = getLength() * signature.normalisedLoopRange.getEnd();
		

		auto wrapAroundLoop = rangeToLookForTicks.contains(loopEndTicks);

		if (wrapAroundLoop)
		{
			auto loopStartTicks = getLength() * signature.normalisedLoopRange.getStart();
			auto rangeEndAfterWrap = rangeToLookForTicks.getEnd() - loopEndTicks + loopStartTicks;

			Range<double> beforeWrap = { rangeToLookForTicks.getStart(), loopEndTicks };
			Range<double> afterWrap = { loopStartTicks, rangeEndAfterWrap };

			if (auto nextEvent = seq->getEventPointer(nextIndex))
			{
				auto ts = nextEvent->message.getTimeStamp();

				if (beforeWrap.contains(ts))
				{
					lastPlayedIndex = nextIndex;
					return &nextEvent->message;
				}
				if (afterWrap.contains(ts))
				{
					lastPlayedIndex = nextIndex;
					return &nextEvent->message;
				}

				// We don't want to wrap around notes that lie within the loop range.
				if (ts < loopEndTicks)
					return nullptr;
			}

			auto indexAfterWrap = seq->getNextIndexAtTime(loopStartTicks);

			if (auto afterEvent = seq->getEventPointer(indexAfterWrap))
			{
				while (afterEvent != nullptr && afterEvent->message.isNoteOff())
				{
					indexAfterWrap++;

					afterEvent = seq->getEventPointer(indexAfterWrap);
				}

				if (afterEvent != nullptr && !afterEvent->message.isNoteOff())
				{
					auto ts = afterEvent->message.getTimeStamp();

					if (afterWrap.contains(ts))
					{
						lastPlayedIndex = indexAfterWrap;

						return &afterEvent->message;
					}
				}
			}
		}
		else
		{
			if (auto nextEvent = seq->getEventPointer(nextIndex))
			{
				if (rangeToLookForTicks.contains(nextEvent->message.getTimeStamp()))
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

	if (signature.numBars != 0.0)
		return signature.getNumQuarters() * (double)TicksPerQuarter;

	double maxLength = 0.0;

	for (auto seq : sequences)
		maxLength = jmax(maxLength, seq->getEndTime());

	return maxLength;
}

double HiseMidiSequence::getLengthInQuarters() const
{
	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (artificialLengthInQuarters != -1.0)
		return artificialLengthInQuarters;

	if (signature.numBars != 0.0)
		return signature.getNumQuarters();

	if (auto currentSequence = sequences.getFirst())
		return currentSequence->getEndTime() / (double)TicksPerQuarter;

	return 0.0;
}


double HiseMidiSequence::getLengthInSeconds(double bpm)
{
	return getLengthInQuarters() * 60.0 / bpm;
}

double HiseMidiSequence::getLastPlayedNotePosition() const
{
	if (auto seq = getReadPointer(currentTrackIndex))
	{
		if (auto h = seq->getEventPointer(lastPlayedIndex))
		{
			auto lastTimestamp = h->message.getTimeStamp();

			auto lengthInTicks = getLengthInQuarters() * TicksPerQuarter;

			return lastTimestamp / lengthInTicks;
		}
	}

	return 0.0;
}

void HiseMidiSequence::setLengthInQuarters(double newLength)
{
	artificialLengthInQuarters = newLength;
	signature.calculateNumBars(artificialLengthInQuarters, false);
}

void HiseMidiSequence::setLengthFromTimeSignature(TimeSignature s)
{
	signature = s;
	setLengthInQuarters(signature.getNumQuarters());
}

void HiseMidiSequence::loadFrom(const MidiFile& file)
{
	OwnedArray<MidiMessageSequence> newSequences;

	MidiFile normalisedFile;

	MidiMessageSequence times;
	MidiMessageSequence tempos;

	file.findAllTimeSigEvents(times);

	file.findAllTempoEvents(tempos);
	

	for (auto te : tempos)
		signature.bpm = jlimit(1.0, 1000.0, 60.0 / jmax(0.0001, te->message.getTempoSecondsPerQuarterNote()));

	int nom = 4;
	int denom = 4;

	for (auto te : times)
		te->message.getTimeSignatureInfo(nom, denom);

	signature.nominator = (double)nom;
	signature.denominator = (double)denom;

	auto fileTicks = (int)file.getTimeFormat();

	double timeFactor = fileTicks > 0 ? (double)TicksPerQuarter / (double)fileTicks : 1.0;

	for (int i = 0; i < file.getNumTracks(); i++)
	{
		ScopedPointer<MidiMessageSequence> newSequence = new MidiMessageSequence(*file.getTrack(i));
		newSequence->deleteSysExMessages();

		for (int j = 0; j < newSequence->getNumEvents(); j++)
		{
			auto e = newSequence->getEventPointer(j);

			if (e->message.isMetaEvent())
			{
				if (e->message.isEndOfTrackMetaEvent())
				{
					auto endStamp = e->message.getTimeStamp() * timeFactor;
					auto numQuarters = endStamp / TicksPerQuarter;
					
					signature.calculateNumBars(numQuarters, true);
				}

				newSequence->deleteEvent(j--, false);
			}
			else if (timeFactor != 1.0)
				e->message.setTimeStamp(e->message.getTimeStamp() * timeFactor);
		}

		if(newSequence->getNumEvents() > 0)
			normalisedFile.addTrack(*newSequence);
	}

	normalisedFile.setTicksPerQuarterNote(TicksPerQuarter);

	if(signature.numBars == 0.0)
		signature.calculateNumBars(normalisedFile.getLastTimestamp() / TicksPerQuarter, true);

	

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

	f.setTicksPerQuarterNote(HiseMidiSequence::TicksPerQuarter);

	auto endTimestamp = signature.getNumQuarters() * (double)HiseMidiSequence::TicksPerQuarter;



	for (int i = 0; i < sequences.size(); i++)
	{
		auto copy = MidiMessageSequence(*sequences[i]);
		copy.addEvent(MidiMessage::endOfTrack(), endTimestamp);
		f.addTrack(copy);
	}

	auto name = id.toString();

	if (name.isEmpty())
		name = "temp";

	auto tmp = File::getSpecialLocation(File::SpecialLocationType::tempDirectory).getNonexistentChildFile(name, ".mid");
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
	if(isPositiveAndBelow(currentTrackIndex, sequences.size()))
		return sequences[currentTrackIndex]->getNumEvents();

	return 0;
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

void HiseMidiSequence::trimInactiveTracks()
{
	SimpleReadWriteLock::ScopedWriteLock sl(swapLock);

	auto seqToKeep = sequences.removeAndReturn(currentTrackIndex);

	sequences.clear(true);
	sequences.add(seqToKeep);
	currentTrackIndex = 0;
	resetPlayback();
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
	if (getLength() == 0.0)
		return {};

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

				if (x >= 1.0)
					break;

				auto y = (float)(127 - e->message.getNoteNumber()) / 128.0f;
				auto h = 1.0f / 128.0f;

				list.add({ x, y, w, h });
			}
		}
	}
	
	if (!targetBounds.isEmpty())
	{
		auto scaler = AffineTransform::scale(targetBounds.getWidth(), targetBounds.getHeight());
		list.transformAll(scaler);
	}

	return list;
}


Array<HiseEvent> HiseMidiSequence::getEventList(double sampleRate, double bpm, HiseMidiSequence::TimestampEditFormat formatToUse)
{
	Array<HiseEvent> newBuffer;
	newBuffer.ensureStorageAllocated(getNumEvents());

	Range<double> range = { 0.0, getLength() };

	auto samplePerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);

	int16 currentEventId = 0;

	SimpleReadWriteLock::ScopedReadLock sl(swapLock);

	if (auto mSeq = getReadPointer())
	{
		for (const auto& ev : *mSeq)
		{
			if (ev->message.isNoteOn() && ev->noteOffObject != nullptr)
			{
				HiseEvent on = HiseEvent(ev->message);
				on.setEventId(currentEventId);

				HiseEvent off = HiseEvent(ev->noteOffObject->message);
				off.setEventId(currentEventId);

				currentEventId++;

				auto onTsTicks = jmin(getLength() - 1.0, ev->message.getTimeStamp());
				auto offTsTicks = jmin(getLength() - 1.0, ev->noteOffObject->message.getTimeStamp());

				if (onTsTicks == offTsTicks) // note on lies after the end of the sequence
					continue;

				auto fToUse = formatToUse != TimestampEditFormat::numTimestampFormats ? formatToUse : timestampFormat;

				if (fToUse == TimestampEditFormat::Samples)
				{
					auto onTs = (int)(samplePerQuarter * onTsTicks / (double)HiseMidiSequence::TicksPerQuarter);
					auto offTs = (int)(samplePerQuarter * offTsTicks / (double)HiseMidiSequence::TicksPerQuarter);

					on.setTimeStamp(onTs);
					off.setTimeStamp(offTs);
				}
				else
				{
					on.setTimeStamp(onTsTicks);
					off.setTimeStamp(offTsTicks);
				}

				newBuffer.add(on);
				newBuffer.add(off);
			}
			else if (ev->message.isController() || ev->message.isPitchWheel())
			{
				HiseEvent cc(ev->message);

				auto ccTsTicks = jmin(getLength() - 1.0, ev->message.getTimeStamp());
				
				auto fToUse = formatToUse != TimestampEditFormat::numTimestampFormats ? formatToUse : timestampFormat;

				if (fToUse == TimestampEditFormat::Samples)
				{
					auto onTs = (int)(samplePerQuarter * ccTsTicks / (double)HiseMidiSequence::TicksPerQuarter);
					cc.setTimeStamp(onTs);
				}
				else
					cc.setTimeStamp(ccTsTicks);

				newBuffer.add(cc);
			}
		}
	}

	struct NoteOnSorter
	{
		static int compareElements(const HiseEvent& first, const HiseEvent& second)
		{
			auto ft = first.getTimeStamp();
			auto st = second.getTimeStamp();

			if (ft < st)
				return -1;
			else if (ft > st)
				return 1;
			else return 0;
		}
	} sorter;

	newBuffer.sort(sorter);

	return newBuffer;
}



void HiseMidiSequence::swapCurrentSequence(MidiMessageSequence* sequenceToSwap)
{
	SimpleReadWriteLock::ScopedWriteLock sl(swapLock);
	sequences.set(currentTrackIndex, sequenceToSwap, true);
}


MidiPlayer::PlaybackListener::~PlaybackListener()
{}

MidiPlayer::EventRecordProcessor::~EventRecordProcessor()
{}

MidiPlayer::SequenceListener::~SequenceListener()
{}

MidiPlayer::SequenceListAction::SequenceListAction(MidiPlayer* p, HiseMidiSequence::List newList_, int newSeqIndex):
	player(p),
	oldList(p->createListOfCurrentSequences()),
	newList(newList_),
	newIndex(newSeqIndex)
{
	oldIndex = oldList.indexOf(p->getCurrentSequence());
}

bool MidiPlayer::SequenceListAction::perform()
{
	if (player == nullptr)
		return false;

	player->swapSequenceListWithIndex(newList, newIndex);
	return true;
}

bool MidiPlayer::SequenceListAction::undo()
{
	if (player == nullptr)
		return false;

	player->swapSequenceListWithIndex(oldList, oldIndex);
	return true;
}

MidiPlayer::TimesigUndo::TimesigUndo(MidiPlayer* player_, HiseMidiSequence::TimeSignature newSig_):
	player(player_),
	newSig(newSig_)
{
	if (auto seq = player->getCurrentSequence())
		oldSig = seq->getTimeSignature();
}

bool MidiPlayer::TimesigUndo::perform()
{
	if (player == nullptr)
		return false;

	player->setLength(newSig, false);
	return true;
}

bool MidiPlayer::TimesigUndo::undo()
{
	if (player == nullptr)
		return false;

	player->setLength(oldSig, false);
	return true;
}

bool MidiPlayer::isProcessingWholeBuffer() const
{ return true; }

MidiPlayer::PlayState MidiPlayer::getPlayState() const
{ return playState; }

int MidiPlayer::getNumSequences() const
{ return currentSequences.size(); }

void MidiPlayer::setMidiControlAutomationHandlerConsumesControllerEvents(bool shouldBeEnabled)
{
	globalMidiHandlerConsumesCC = shouldBeEnabled;
}

UndoManager* MidiPlayer::getUndoManager()
{ return undoManager; }

void MidiPlayer::setUseExternalUndoManager(UndoManager* externalUndoManagerToUse)
{
	if (externalUndoManagerToUse == nullptr)
		undoManager = ownedUndoManager.get();
	else
		undoManager = externalUndoManagerToUse;
}

void MidiPlayer::setFlushRecordingOnStop(bool shouldFlushRecording)
{
	flushRecordedEvents = shouldFlushRecording;
}

double MidiPlayer::getCurrentTicksSincePlaybackStart() const
{
	return ticksSincePlaybackStart;
}

void MidiPlayer::setPositionWithTicksFromPlaybackStart(double newPos)
{
	ticksSincePlaybackStart = newPos;
	updatePositionInCurrentSequence();
}

void MidiPlayer::onResync(double ppqPos)
{
	setPositionWithTicksFromPlaybackStart(ppqPos * HiseMidiSequence::TicksPerQuarter);
}

double MidiPlayer::getTicksPerSample() const
{ return ticksPerSample * getPlaybackSpeed(); }

double MidiPlayer::getPlaybackSpeed() const
{ return playbackSpeed * getMainController()->getGlobalPlaybackSpeed(); }

const Array<HiseEvent>& MidiPlayer::getListOfCurrentlyRecordedEventsRaw() const
{ return currentlyRecordedEvents; }

void MidiPlayer::addPlaybackListener(PlaybackListener* l)
{
	playbackListeners.addIfNotAlreadyThere(l);
}

void MidiPlayer::removePlaybackListener(PlaybackListener* l)
{
	playbackListeners.removeAllInstancesOf(l);
}

void MidiPlayer::setNoteOffAtStop(bool shouldMoveNotesOfToStop)
{
	noteOffAtStop = shouldMoveNotesOfToStop;
}

void MidiPlayer::setReferenceWithoutLoading(PoolReference r)
{
	forcedReference = r;
}

void MidiPlayer::addEventRecordProcessor(EventRecordProcessor* newProcessor)
{
	eventRecordProcessors.addIfNotAlreadyThere(newProcessor);
}

void MidiPlayer::removeEventRecordProcessor(EventRecordProcessor* processorToRemove)
{
	eventRecordProcessors.removeAllInstancesOf(processorToRemove);
}

bool MidiPlayer::processRecordedEvent(HiseEvent& m)
{
	for (auto& p : eventRecordProcessors)
	{
		jassert(p != nullptr);

		if (p != nullptr)
			p->processRecordedEvent(m);
	}

	return !m.isIgnored();
}

bool MidiPlayer::NotePair::operator==(const NotePair& other) const
{ return on == other.on && off == other.off; }

MidiPlayer::OverdubUpdater::OverdubUpdater(MidiPlayer& mp):
	SimpleTimer(mp.getMainController()->getGlobalUIUpdater()),
	parent(mp)
{}

void MidiPlayer::OverdubUpdater::timerCallback()
{
	if (dirty)
	{
		parent.flushOverdubNotes(lastTimestamp);
		lastTimestamp = -1.0;
		dirty.store(false);
	}
}

void MidiPlayer::OverdubUpdater::setDirty(double activeNoteTimestamp)
{
	if (activeNoteTimestamp != lastTimestamp)
	{
		lastTimestamp = activeNoteTimestamp;
	}

	dirty.store(true);
}

bool MidiPlayer::isRecording() const noexcept
{ return getPlayState() == PlayState::Record; }

MidiPlayer::EditAction::EditAction(WeakReference<MidiPlayer> currentPlayer_, const Array<HiseEvent>& newContent, double sampleRate_, double bpm_, HiseMidiSequence::TimestampEditFormat formatToUse_) :
	UndoableAction(),
	currentPlayer(currentPlayer_),
	newEvents(newContent),
	sampleRate(sampleRate_),
	bpm(bpm_),
	formatToUse(formatToUse_)
{
	if (auto seq = currentPlayer->getCurrentSequence())
	{
		oldEvents = seq->getEventList(sampleRate, bpm, formatToUse);
		oldSig = seq->getTimeSignature();
	}
		
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
							 bpm,
							 sampleRate,
							 formatToUse);

		if (auto seq = currentPlayer->getCurrentSequence())
		{
			currentPlayer->getCurrentSequence()->setLengthFromTimeSignature(oldSig);
			currentPlayer->updatePositionInCurrentSequence();
			currentPlayer->sendSequenceUpdateMessage(sendNotificationAsync);

			return true;
		}
	}
		
	return false;
}

bool MidiPlayer::EditAction::undo()
{
	if (currentPlayer != nullptr && currentPlayer->getSequenceId() == sequenceId)
	{
		writeArrayToSequence(currentPlayer->getCurrentSequence(), 
			                 oldEvents, 
			                 bpm,
							 sampleRate,
							 formatToUse);

		currentPlayer->getCurrentSequence()->setLengthFromTimeSignature(oldSig);
		currentPlayer->updatePositionInCurrentSequence();
		currentPlayer->sendSequenceUpdateMessage(sendNotificationAsync);
		return true;
	}

	return false;
}

void MidiPlayer::EditAction::writeArrayToSequence(HiseMidiSequence::Ptr destination, Array<HiseEvent>& arrayToWrite, double bpm, double sampleRate, HiseMidiSequence::TimestampEditFormat formatToUse)
{
	if (destination == nullptr)
		return;

	auto format = destination->getTimestampEditFormat();

	if (formatToUse != HiseMidiSequence::TimestampEditFormat::numTimestampFormats)
		format = formatToUse;

	ScopedPointer<MidiMessageSequence> newSeq = new MidiMessageSequence();

	auto samplePerQuarter = (double)TempoSyncer::getTempoInSamples(bpm, sampleRate, TempoSyncer::Quarter);

	auto maxLength = destination->getLength();

	for (auto& e : arrayToWrite)
	{
		if (e.isEmpty())
			continue;

		double timeStamp;

		if (format == HiseMidiSequence::TimestampEditFormat::Samples)
			timeStamp = ((double)e.getTimeStamp() / samplePerQuarter) * (double)HiseMidiSequence::TicksPerQuarter;
		else
			timeStamp = e.getTimeStamp();

		if (maxLength != 0.0)
			timeStamp = jmin(maxLength, timeStamp);

		if (e.getChannel() == 0)
			e.setChannel(1);

		if (e.isNoteOn() && e.getTransposeAmount() != 0)
		{
			// We need to write the tranpose amount into the matching note-off
			// or it can't resolve it as matching pair

			for (auto& no : arrayToWrite)
			{
				if (no.isNoteOff() && no.getEventId() == e.getEventId())
				{
					no.setTransposeAmount(e.getTransposeAmount());

					jassert(no.getNoteNumberIncludingTransposeAmount() == e.getNoteNumberIncludingTransposeAmount());

					break;
				}
			}
		}

		auto m = e.toMidiMesage();
		m.setTimeStamp(timeStamp);
		newSeq->addEvent(m);
	}

	newSeq->sort();
	newSeq->updateMatchedPairs();
	destination->swapCurrentSequence(newSeq.release());
}

MidiPlayer::MidiPlayer(MainController *mc, const String &id, ModulatorSynth*) :
	MidiProcessor(mc, id),
	ownedUndoManager(new UndoManager()),
	updater(*this),
	overdubUpdater(*this)
{
	addAttributeID(CurrentPosition);
	addAttributeID(CurrentSequence);
	addAttributeID(CurrentTrack);
	addAttributeID(LoopEnabled);
	addAttributeID(LoopStart);
	addAttributeID(LoopEnd);
	addAttributeID(PlaybackSpeed);

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

void MidiPlayer::onGridChange(int gridIndex, uint16 timestamp, bool firstGridEventInPlayback)
{
	if (syncToMasterClock && firstGridEventInPlayback)
	{
		if (playState == PlayState::Stop)
		{
			if (recordOnNextPlaybackStart)
				recordInternal(timestamp);
			else
				startInternal(timestamp);
		}
		
		if (gridIndex != 0)
		{
			auto t = getMainController()->getMasterClock().getCurrentClockGrid();
			auto quarterPos = (double)gridIndex * TempoSyncer::getTempoFactor(t);
			auto tickPos = quarterPos * (double)HiseMidiSequence::TicksPerQuarter;
			setPositionWithTicksFromPlaybackStart(tickPos);
		}
	}
}

void MidiPlayer::onTransportChange(bool isPlaying, double ppqPosition)
{
	if (syncToMasterClock && !isPlaying)
	{
		stopInternal(0);
	}
}

juce::ValueTree MidiPlayer::exportAsValueTree() const
{
	ValueTree v = MidiProcessor::exportAsValueTree();

	saveID(CurrentSequence);
	saveID(CurrentTrack);
	saveID(LoopEnabled);
	saveID(PlaybackSpeed);

	SimpleReadWriteLock::ScopedReadLock sl(sequenceLock);

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

	if (v.hasProperty("PlaybackSpeed"))
	{
		loadID(PlaybackSpeed);
	}
	else
		setInternalAttribute(PlaybackSpeed, 1.0f);
}

void MidiPlayer::addSequence(HiseMidiSequence::Ptr newSequence, bool select)
{
	{
		SimpleReadWriteLock::ScopedWriteLock sl(sequenceLock);
		currentSequences.add(newSequence);
	}

	if (select)
	{
		currentSequenceIndex = currentSequences.size() - 1;
		sendChangeMessage();
	}

	sendSequenceUpdateMessage(sendNotificationAsync);
}

void MidiPlayer::clearSequences(NotificationType notifyListeners)
{
	if(undoManager == ownedUndoManager && undoManager != nullptr)
		undoManager->clearUndoHistory();

	{
		SimpleReadWriteLock::ScopedWriteLock sl(sequenceLock);
		currentSequences.clear();
		currentSequenceIndex = -1;
	}
	
	currentlyLoadedFiles.clear();
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
	case LoopStart:				return (float)getLoopStart();
	case LoopEnd:				return (float)getLoopEnd();
	case PlaybackSpeed:			return (float)playbackSpeed;
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
		if (auto seq = getCurrentSequence())
		{
			newAmount = jlimit<float>((float)getLoopStart(), (float)getLoopEnd(), newAmount);

			auto lengthInQuarters = seq->getLengthInQuarters();
			auto ticks = lengthInQuarters * HiseMidiSequence::TicksPerQuarter;
			ticksSincePlaybackStart = ticks * newAmount;

			updatePositionInCurrentSequence();
		}

		break;
	}
	case LoopStart:
	{
		auto loopStart = jlimit(0.0, 1.0, (double)newAmount);

		if (auto seq = getCurrentSequence())
			seq->getTimeSignaturePtr()->setLoopStart(loopStart);

		updatePositionInCurrentSequence();
		break;
	}
	case LoopEnd:
	{
		auto loopEnd = jlimit(0.0, 1.0, (double)newAmount);

		if (auto seq = getCurrentSequence())
			seq->getTimeSignaturePtr()->setLoopEnd(loopEnd);

		updatePositionInCurrentSequence();
		break;
	}
	case CurrentSequence:		
	{
		currentSequenceIndex = jlimit<int>(-1, currentSequences.size()-1, (int)(newAmount - 1)); 

		currentlyRecordedEvents.clear();
		recordState.store(RecordState::Idle);

		updatePositionInCurrentSequence();

		sendSequenceUpdateMessage(sendNotificationAsync);

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
	case PlaybackSpeed: 
		if (playbackSpeed != (double)newAmount)
		{
			playbackSpeed = jlimit(0.01, 16.0, (double)newAmount);
			//getMainController()->allNotesOff();
		}
		break;
	case LoopEnabled: loopEnabled = newAmount > 0.5f; break;
	default:
		break;
	}
}


void MidiPlayer::loadMidiFile(PoolReference reference)
{
	PooledMidiFile newContent;

	if (auto e = getMainController()->getExpansionHandler().getExpansionForWildcardReference(reference.getReferenceString()))
		newContent = e->pool->getMidiFilePool().loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);
	else
		newContent = getMainController()->getCurrentMidiFilePool()->loadFromReference(reference, PoolHelpers::LoadAndCacheWeak);

	if (newContent.get() != nullptr)
	{
		currentlyLoadedFiles.add(reference);

		HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();
		newSequence->loadFrom(newContent->data.getFile());
		//newSequence->setId(reference.getFile().getFileNameWithoutExtension());
		addSequence(newSequence);
	}
	else
		jassertfalse;
		
}

void MidiPlayer::prepareToPlay(double sampleRate_, int samplesPerBlock_)
{
	MidiProcessor::prepareToPlay(sampleRate_, samplesPerBlock_);
	tempoChanged(getMainController()->getBpm());
}

void MidiPlayer::preprocessBuffer(HiseEventBuffer& buffer, int numSamples)
{
	lastBlockSize = numSamples;

	if (std::isnan(currentPosition))
		currentPosition = -1.0;

	if (currentSequenceIndex >= 0 && currentPosition != -1.0)
	{
		auto loopStart = getLoopStart();
		auto loopEnd = getLoopEnd();

		if (currentPosition > loopEnd && (!loopEnabled || (isRecording())))
		{
			if (isRecording() && overdubMode)
			{
				if (!overdubNoteOns.isEmpty())
				{
					auto lengthInQuarters = getCurrentSequence()->getLengthInQuarters() * loopEnd;
					auto ts = lengthInQuarters * (double)HiseMidiSequence::TicksPerQuarter - 1.0;

					overdubUpdater.setDirty(ts);
				}
			}
			else
			{
				if (isRecording())
					finishRecording();

				stop();
				return;
			}
		}

#if 0
		if (isRecording() && (currentPosition - recordStart) > 1.0)
		{
			//finishRecording();
			playState = PlayState::Play;
			sendAllocationFreeChangeMessage();
		}
#endif

		if (auto seq = getCurrentSequence())
		{
			if (playState == PlayState::Stop)
			{
				seq->resetPlayback();
				playState = PlayState::Stop;
				timeStampForNextCommand = 0;
				currentPosition = -1.0;
				return;
			}

			seq->setCurrentTrackIndex(currentTrackIndex);

			if (currentPosition < loopStart)
			{
				updatePositionInCurrentSequence();
			}
			else if (currentPosition > loopEnd)
			{
				updatePositionInCurrentSequence();
			}

			auto tickThisTime = (numSamples - timeStampForNextCommand) * getTicksPerSample();
			auto lengthInTicks = seq->getLength();

			if (lengthInTicks == 0.0)
				return;

			auto positionInTicks = getPlaybackPosition() * lengthInTicks;

			auto delta = tickThisTime / lengthInTicks;

			Range<double> currentRange;

			if (loopEnabled)
				currentRange = { positionInTicks, positionInTicks + tickThisTime };
			else
				currentRange = { positionInTicks, jmin<double>(lengthInTicks, positionInTicks + tickThisTime) };

			MidiMessage* eventsInThisCallback[16];
			memset(eventsInThisCallback, 0, sizeof(MidiMessage*) * 16);



			while (auto e = seq->getNextEvent(currentRange))
			{
				bool found = false;

				for (int i = 0; i < 16; i++)
				{
					if (eventsInThisCallback[i] == e)
					{
						found = true;
						break;
					}

					if (eventsInThisCallback[i] == nullptr)
					{
						eventsInThisCallback[i] = e;
						break;
					}
				}

				if (found)
					break;

				auto timeStampInThisBuffer = e->getTimeStamp() - positionInTicks;

				if (timeStampInThisBuffer < 0.0)
					timeStampInThisBuffer += getCurrentSequence()->getTimeSignature().normalisedLoopRange.getLength() * lengthInTicks;

				auto timeStamp = (int)MidiPlayerHelpers::ticksToSamples(timeStampInThisBuffer / getPlaybackSpeed(), getMainController()->getBpm(), getSampleRate());
				timeStamp += timeStampForNextCommand;

				jassert(isPositiveAndBelow(timeStamp, numSamples));

				HiseEvent newEvent(*e);

				newEvent.setTimeStamp(timeStamp);
				newEvent.setArtificial();
				newEvent.setChannel(currentTrackIndex + 1);

				if (newEvent.isController())
				{
					bool consumed = false;

					if (globalMidiHandlerConsumesCC)
					{
						auto handler = getMainController()->getMacroManager().getMidiControlAutomationHandler();
						consumed = handler->handleControllerMessage(newEvent);
					}

					if (!consumed)
						buffer.addEvent(newEvent);
				}
				else if (newEvent.isPitchWheel())
				{
					buffer.addEvent(newEvent);
				}
				else if (newEvent.isNoteOn() && !isBypassed())
				{
					getMainController()->getEventHandler().pushArtificialNoteOn(newEvent);

					buffer.addEvent(newEvent);

					if (auto noteOff = seq->getMatchingNoteOffForCurrentEvent())
					{
						HiseEvent newNoteOff(*noteOff);
						newNoteOff.setArtificial();

						auto noteOffTimeStampInBuffer = noteOff->getTimeStamp() - positionInTicks;

						if (noteOffTimeStampInBuffer < 0.0)
							noteOffTimeStampInBuffer += getCurrentSequence()->getTimeSignature().normalisedLoopRange.getLength() * lengthInTicks;

						timeStamp += timeStampForNextCommand;

						auto noteOffTimeStamp = (int)MidiPlayerHelpers::ticksToSamples(noteOffTimeStampInBuffer, getMainController()->getBpm(), getSampleRate());

						newNoteOff.setChannel(currentTrackIndex + 1);

						auto on_id = getMainController()->getEventHandler().getEventIdForNoteOff(newNoteOff);

						jassert(newEvent.getEventId() == on_id);

						newNoteOff.setEventId(on_id);
						newNoteOff.setTimeStamp(noteOffTimeStamp);
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
			ticksSincePlaybackStart += tickThisTime;

			if (isRecording() && overdubMode)
			{
				overdubUpdater.setDirty();
			}
		}
	}
}

void MidiPlayer::processHiseEvent(HiseEvent &m) noexcept
{
	currentTimestampInBuffer = m.getTimeStamp();

	if (isBypassed())
		return;

	if (m.isAllNotesOff())
	{
		stop(m.getTimeStamp());
	}

	bool artificial = m.isArtificial();

	if (isRecording() && !artificial && (recordState == RecordState::Prepared || overdubMode) && !m.isIgnored())
	{
		if (auto seq = getCurrentSequence())
		{
			if (useNextNoteAsRecordStartPos)
			{
				recordStart = currentPosition;
				useNextNoteAsRecordStartPos = false;
			}

			

			auto lengthInQuarters = seq->getLengthInQuarters() * getPlaybackPosition();
			auto ticks = lengthInQuarters * HiseMidiSequence::TicksPerQuarter;

			if (overdubMode)
			{
				HiseEvent copy(m);

				copy.setChannel(currentTrackIndex + 1);

				auto ticksInBuffer = MidiPlayerHelpers::samplesToTicks((double)copy.getTimeStamp(), getMainController()->getBpm(), getSampleRate());

				copy.setTimeStamp(ticks + ticksInBuffer);

				if (copy.isNoteOn())
				{
					if (processRecordedEvent(copy))
					{
						NotePair newPair;
						newPair.on = copy;

						SimpleReadWriteLock::ScopedWriteLock sl(overdubLock);
						overdubNoteOns.insert(newPair);
					}
				}
				else if (copy.isNoteOff())
				{
					// We don't need to ignore note-off events - if they have a matching note-on, they should
					// be used...
					processRecordedEvent(copy);
					copy.ignoreEvent(false);

					{
						SimpleReadWriteLock::ScopedReadLock sl(overdubLock);

						for (auto& e : overdubNoteOns)
						{
							if (e.on.getEventId() == copy.getEventId())
							{
								e.off = copy;
								break;
							}
						}
					}
				}
				else
				{
					if (processRecordedEvent(copy))
						controllerEvents.insertWithoutSearch(copy);
				}

				return;
			}

			auto timestampSamples = (int)MidiPlayerHelpers::ticksToSamples(ticks, getMainController()->getBpm(), getSampleRate());

			// This will be processed after the position has advanced so we need to subtract the last blocksize and add the message's timestamp.
			timestampSamples = jmax(0, timestampSamples - lastBlockSize);
			timestampSamples += currentTimestampInBuffer;

			HiseEvent copy(m);
			copy.setChannel(currentTrackIndex + 1);
			copy.setTimeStamp(timestampSamples);

			if(processRecordedEvent(copy))
				currentlyRecordedEvents.add(copy);
		}
	}
}

void MidiPlayer::addSequenceListener(SequenceListener* newListener)
{
	SimpleReadWriteLock::ScopedWriteLock sl(listenerLock);
	sequenceListeners.addIfNotAlreadyThere(newListener);
}

void MidiPlayer::removeSequenceListener(SequenceListener* listenerToRemove)
{
	SimpleReadWriteLock::ScopedWriteLock sl(listenerLock);
	sequenceListeners.removeAllInstancesOf(listenerToRemove);
}

double MidiPlayer::getLoopStart() const
{
	if (auto seq = getCurrentSequence())
		return seq->getTimeSignature().normalisedLoopRange.getStart();

	return 0.0;
}

double MidiPlayer::getLoopEnd() const
{
	if (auto seq = getCurrentSequence())
		return seq->getTimeSignature().normalisedLoopRange.getEnd();

	return 1.0;
}

juce::ReferenceCountedArray<hise::HiseMidiSequence> MidiPlayer::createListOfCurrentSequences()
{
	ReferenceCountedArray<HiseMidiSequence> newList;

	for (auto s : currentSequences)
		newList.add(s);

	return newList;
}

void MidiPlayer::swapSequenceListWithIndex(HiseMidiSequence::List listToSwapWith, int newSequenceIndex)
{
	{
		SimpleReadWriteLock::ScopedWriteLock sl(sequenceLock);
        std::swap(currentSequences, listToSwapWith);
	}

	for (auto s : currentSequences)
		s->setCurrentTrackIndex(currentTrackIndex);

	setAttribute(CurrentSequence, (float)newSequenceIndex + 1.0f, sendNotification);
	sendSequenceUpdateMessage(sendNotificationAsync);
}

void MidiPlayer::setSyncToMasterClock(bool shouldSyncToMasterClock)
{
	// You can't sync to the clock if the grid is not enabled...
	jassert(!shouldSyncToMasterClock || getMainController()->getMasterClock().isGridEnabled());

	if (syncToMasterClock != shouldSyncToMasterClock)
	{
		syncToMasterClock = shouldSyncToMasterClock;

		if (syncToMasterClock)
			getMainController()->addMusicalUpdateListener(this);
		else
			getMainController()->removeMusicalUpdateListener(this);
	}

	stopInternal(0);
}

void MidiPlayer::sendPlaybackChangeMessage(int timestamp)
{
	if (!playbackListeners.isEmpty())
	{
		for (auto pl : playbackListeners)
			pl->playbackChanged(timestamp, playState);
	}
}

void MidiPlayer::sendSequenceUpdateMessage(NotificationType notification)
{
	updater.handleUpdate(getCurrentSequence(), notification);
}

void MidiPlayer::changeTransportState(PlayState newState)
{
	switch (newState)
	{						// Supposed to be immediately...
	case PlayState::Play:	play(0);	return;
	case PlayState::Stop:	stop(0);	return;
	case PlayState::Record: record(0);	return;
        default: return;
	}
}

double MidiPlayer::getPlaybackPositionFromTicksSinceStart() const
{
	if (playState == PlayState::Stop)
		return 0.0;

	if (auto seq = getCurrentSequence())
	{
		auto range = seq->getTimeSignature().normalisedLoopRange;
		auto totalTicks = seq->getLength();

		auto loopLengthInTicks = range.getLength() * totalTicks;
		auto loopStartInTicks = range.getStart() * totalTicks;

		if (loopLengthInTicks > 0.0)
		{
			auto loopPositionTicks = fmod(ticksSincePlaybackStart, loopLengthInTicks) + loopStartInTicks;
			return loopPositionTicks / totalTicks;
		}
	}

	return 0.0;
}

hise::HiseMidiSequence::Ptr MidiPlayer::getCurrentSequence() const
{
	SimpleReadWriteLock::ScopedReadLock sl(sequenceLock);

	if(currentSequenceIndex != -1)
        return currentSequences[currentSequenceIndex].get();
    else
        return nullptr;
}

juce::Identifier MidiPlayer::getSequenceId(int index) const
{
	if (index == -1)
		index = currentSequenceIndex;

    if(index == -1)
        return {};
    
	if (auto s = currentSequences[index])
	{
		return s->getId();
	}

	return {};
}

double MidiPlayer::getPlaybackPosition() const
{
	if (std::isnan(currentPosition))
		return 0.0;

	return fmod(currentPosition, 1.0);
}

double MidiPlayer::getPlayPackPositionInLoop() const
{
	auto pos = getPlaybackPosition();

	if (pos < getLoopStart())
		return getLoopStart();
	if (pos > getLoopEnd())
		return getLoopStart();
	return pos;
}

void MidiPlayer::swapCurrentSequence(MidiMessageSequence* newSequence)
{
	getCurrentSequence()->swapCurrentSequence(newSequence);

	updatePositionInCurrentSequence();
	sendSequenceUpdateMessage(sendNotificationAsync);
}


void MidiPlayer::enableInternalUndoManager(bool shouldBeEnabled)
{
	bool isEnabled = ownedUndoManager != nullptr;

	if (isEnabled != shouldBeEnabled)
	{
		undoManager = nullptr;

		if (isEnabled)
			undoManager = new UndoManager();
	}
}

void MidiPlayer::setExternalUndoManager(UndoManager* externalUndoManager)
{
	ownedUndoManager = nullptr;
	undoManager = externalUndoManager;
}

void MidiPlayer::flushEdit(const Array<HiseEvent>& newEvents, HiseMidiSequence::TimestampEditFormat formatToUse)
{
	ScopedPointer<EditAction> newAction = new EditAction(this, newEvents, getSampleRate(), getMainController()->getBpm(), formatToUse);

	if (undoManager != nullptr)
	{
		// The internal undomanager will keep each action separate, 
		// but if you use an external undo manager, you might want to coallescate
		// them in a timer callback...
		if (ownedUndoManager != nullptr)
			ownedUndoManager->beginNewTransaction();

		undoManager->perform(newAction.release());
	}
	else
		newAction->perform();
}

void MidiPlayer::clearCurrentSequence()
{
	currentlyRecordedEvents.clear();
	overdubNoteOns.clearQuick();
	flushEdit({});
}

void MidiPlayer::removeSequence(int sequenceIndex)
{
	HiseMidiSequence::Ptr seqToRemove;

	if (isPositiveAndBelow(sequenceIndex, getNumSequences()))
	{
		SimpleReadWriteLock::ScopedWriteLock sl(sequenceLock);
		seqToRemove = currentSequences.removeAndReturn(sequenceIndex);
	}

	setAttribute(MidiPlayer::CurrentSequence, currentSequenceIndex + 1.0f, sendNotification);
	sendSequenceUpdateMessage(sendNotification);
}

void MidiPlayer::setLength(HiseMidiSequence::TimeSignature sig, bool useUndoManager)
{
	if (auto seq = getCurrentSequence())
	{
		if (useUndoManager && getUndoManager() != nullptr)
		{
			getUndoManager()->perform(new TimesigUndo(this, sig));
		}
		else
		{
			seq->setLengthFromTimeSignature(sig);
			updatePositionInCurrentSequence();
			sendSequenceUpdateMessage(sendNotification);
		}
	}
}

void MidiPlayer::resetCurrentSequence()
{
	if (auto seq = getCurrentSequence())
	{
		if (auto original = getMainController()->getCurrentMidiFilePool()->loadFromReference(currentlyLoadedFiles[currentSequenceIndex], PoolHelpers::LoadAndCacheWeak))
		{
			ScopedPointer<HiseMidiSequence> tempSeq = new HiseMidiSequence();
			tempSeq->loadFrom(original->data.getFile());
			auto l = tempSeq->getEventList(getSampleRate(), getMainController()->getBpm());

			flushEdit(l);
		}
	}
}

hise::PoolReference MidiPlayer::getPoolReference(int index /*= -1*/)
{
	if (forcedReference.isValid())
		return forcedReference;

	if (index == -1)
		index = currentSequenceIndex;

	return currentlyLoadedFiles[index];
}

void MidiPlayer::flushOverdubNotes(double timestampForActiveNotes/*=-1.0*/)
{
	if (overdubNoteOns.isEmpty() && controllerEvents.isEmpty())
		return;

	auto l = getCurrentSequence()->getEventList(44100.0, 120.0, HiseMidiSequence::TimestampEditFormat::Ticks);

	bool didSomething = false;

	for (const auto& c : controllerEvents)
	{
		l.add(c);
		didSomething = true;
	}
	
	controllerEvents.clearQuick();

	for (auto& np : overdubNoteOns)
	{
		if (timestampForActiveNotes != -1.0 && np.off.isEmpty())
		{
			auto delta = timestampForActiveNotes - np.on.getTimeStamp();

			if (delta < JUCE_LIVE_CONSTANT_OFF(192.0))
			{
				np.on.setTimeStamp(0);
			}
			else
			{
				np.off = HiseEvent(HiseEvent::Type::NoteOff, np.on.getNoteNumber(), 0, np.on.getChannel());
				np.off.setEventId(np.on.getEventId());
				np.off.setTransposeAmount(np.on.getTransposeAmount());
				np.off.setTimeStamp(timestampForActiveNotes);
			}
		}

		if (!np.off.isEmpty())
		{
			didSomething = true;
			l.add(np.on);
			l.add(np.off);
		}
	}

	for (int i = 0; i < overdubNoteOns.size(); i++)
	{
		if (overdubNoteOns[i].off.isNoteOff())
			overdubNoteOns.removeElement(i--);
	}

	if(didSomething)
		flushEdit(l, HiseMidiSequence::TimestampEditFormat::Ticks);
}

bool MidiPlayer::stopInternal(int timestamp)
{
	sendAllocationFreeChangeMessage();

	overdubUpdater.stop();

	if (auto seq = getCurrentSequence())
	{
		if (isRecording())
			finishRecording();

		if (noteOffAtStop)
		{
			addNoteOffsToPendingNoteOns();
		}

		seq->resetPlayback();
		playState = PlayState::Stop;

		timeStampForNextCommand = timestamp;
		currentPosition = -1.0;

		sendPlaybackChangeMessage(timestamp);

		return true;
	}

	return false;
}

bool MidiPlayer::startInternal(int timestamp)
{
	sendAllocationFreeChangeMessage();

	if (auto seq = getCurrentSequence())
	{
		if (isRecording())
		{
			if (overdubMode)
			{
				playState = PlayState::Play;
				sendPlaybackChangeMessage(timestamp);
				return true;
			}
			else
				finishRecording();
		}
		else
		{
			// This allows switching from record to play while maintaining the position
			currentPosition = 0.0;
			seq->resetPlayback();
		}

		playState = PlayState::Play;
		timeStampForNextCommand = timestamp;

		sendPlaybackChangeMessage(timestamp);

		ticksSincePlaybackStart = 0.0;

		return true;
	}

	return false;
}

bool MidiPlayer::recordInternal(int timestamp)
{
	sendAllocationFreeChangeMessage();

	

	if (overdubMode)
	{
		overdubUpdater.start();
	}

	if (playState == PlayState::Stop)
	{
		currentPosition = 0.0;
		ticksSincePlaybackStart = 0.0;

		if (auto seq = getCurrentSequence())
			seq->resetPlayback();
	}

	playState = PlayState::Record;
	sendPlaybackChangeMessage(timestamp);

	timeStampForNextCommand = timestamp;

	updatePositionInCurrentSequence();

	useNextNoteAsRecordStartPos = true;



	if (recordState == RecordState::Idle)
		prepareForRecording(true);

	return false;
}

bool MidiPlayer::play(int timestamp)
{
	if (syncToMasterClock && !isRecording())
	{
		return false;
	}
		

	return startInternal(timestamp);
}

bool MidiPlayer::stop(int timestamp)
{
	if (syncToMasterClock)
	{
		recordOnNextPlaybackStart = false;
		return false;
	}
	
	return stopInternal(timestamp);
}

bool MidiPlayer::record(int timestamp)
{
	if (syncToMasterClock && getPlayState() == PlayState::Stop)
	{
		recordOnNextPlaybackStart = true;
		return false;
	}
		
	return recordInternal(timestamp);
}


void MidiPlayer::prepareForRecording(bool copyExistingEvents/*=true*/)
{
	if (overdubMode)
		return;

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

				if (seq->getTimestampEditFormat() == HiseMidiSequence::TimestampEditFormat::Ticks)
				{
					auto bpm = mp->getMainController()->getBpm();
					auto sr = mp->getSampleRate();

					for (auto& e : newList)
					{
						auto tsTicks = e.getTimeStamp();
						auto tsSamples = MidiPlayerHelpers::ticksToSamples(tsTicks, bpm, sr);
						e.setTimeStamp(tsSamples);
					}
				}
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
	if (currentlyRecordedEvents.isEmpty() || overdubMode)
		return;

	auto finishPos = currentPosition;

	auto f = [finishPos](Processor* p)
	{
		auto mp = static_cast<MidiPlayer*>(p);

		auto l = &mp->currentlyRecordedEvents;

		int lastTimestamp = (int)MidiPlayerHelpers::ticksToSamples(mp->getCurrentSequence()->getLength() * finishPos, mp->getMainController()->getBpm(), mp->getSampleRate()) - 1;

		for (int i = 0; i < l->size(); i++)
		{
			auto& currentEvent = l->getReference(i);

			if (currentEvent.isNoteOn())
			{
				bool found = false;

				for (int j = 0; j < l->size(); j++)
				{
					if (l->getUnchecked(j).isNoteOff() && l->getUnchecked(j).getEventId() == currentEvent.getEventId())
					{
						if (l->getUnchecked(j).getTimeStamp() < currentEvent.getTimeStamp())
						{
							l->getReference(j).setTimeStamp(lastTimestamp);
						}
						
						if (currentEvent.getTransposeAmount() != 0)
						{
							int numberWithTranspose = currentEvent.getNoteNumber() + currentEvent.getTransposeAmount();

							currentEvent.setNoteNumber(numberWithTranspose);
							currentEvent.setTransposeAmount(0);
							l->getReference(j).setNoteNumber(numberWithTranspose);
						}

						found = true;
						break;
					}
				}

				if (!found)
				{
					

					HiseEvent artificialNoteOff(HiseEvent::Type::NoteOff, (uint8)(currentEvent.getNoteNumber() + currentEvent.getTransposeAmount()), 1, (uint8)currentEvent.getChannel());

					currentEvent.setNoteNumber(artificialNoteOff.getNoteNumber());
					currentEvent.setTransposeAmount(0);

					artificialNoteOff.setTimeStamp(lastTimestamp);
					artificialNoteOff.setEventId(currentEvent.getEventId());
					l->add(artificialNoteOff);
				}
			}
			else if (currentEvent.isNoteOff())
			{
				bool found = false;

				for (int j = 0; j < l->size(); j++)
				{
					if (l->getUnchecked(j).isNoteOn() && currentEvent.getEventId() == l->getUnchecked(j).getEventId())
					{
						found = true;
						break;
					}
				}

				if (!found)
					l->remove(i--);
			}
		}

		if (mp->getCurrentSequence()->getTimestampEditFormat() == HiseMidiSequence::TimestampEditFormat::Ticks)
		{
			auto bpm = mp->getMainController()->getBpm();
			auto sr = mp->getSampleRate();

			for (auto& e : mp->currentlyRecordedEvents)
			{
				auto tsSamples = e.getTimeStamp();
				auto tsTicks = MidiPlayerHelpers::samplesToTicks(tsSamples, bpm, sr);
				e.setTimeStamp(tsTicks);
			}
		}

		if (mp->flushRecordedEvents)
			mp->flushEdit(mp->currentlyRecordedEvents);
		
		// This is a shortcut because the preparation would just fill it with the current sequence.
		mp->recordState.store(RecordState::Idle);

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

bool MidiPlayer::saveAsMidiFile(const String& fileName, int trackIndex)
{
	//trackIndex -= 1;

	if (getCurrentSequence() == nullptr)
		return false;

	if (auto track = getCurrentSequence()->getReadPointer(currentTrackIndex))
	{
		MidiMessageSequence trackCopy(*track);

		auto sig = getCurrentSequence()->getTimeSignature();

		auto timeSigMessage = MidiMessage::timeSignatureMetaEvent((int)sig.nominator, (int)sig.denominator);
		
		timeSigMessage.setTimeStamp(0);

		

		auto endMessage = MidiMessage::endOfTrack();
		endMessage.setTimeStamp(getCurrentSequence()->getLength());

		trackCopy.addEvent(timeSigMessage);
		trackCopy.addEvent(endMessage);
		trackCopy.sort();

		PoolReference r(getMainController(), fileName, FileHandlerBase::MidiFiles);

		auto pool = getMainController()->getCurrentMidiFilePool();

		if (r.getMode() == PoolReference::Mode::ExpansionPath)
		{
			if (auto exp = getMainController()->getExpansionHandler().getExpansionForWildcardReference(r.getReferenceString()))
			{
				pool = &exp->pool->getMidiFilePool();
			}
			else
				jassertfalse;
		}

		if (r.getFile().existsAsFile())
		{
			if (auto mf = pool->loadFromReference(r, PoolHelpers::ForceReloadStrong))
			{
				MidiFile& existingFile = mf->data.getFile();

				if (trackIndex < existingFile.getNumTracks())
				{
					MidiFile copy;

					for (int i = 0; i < existingFile.getNumTracks(); i++)
					{
						if (i == trackIndex)
							copy.addTrack(trackCopy);
						else
							copy.addTrack(*existingFile.getTrack(i));
					}

					auto file = r.getFile();

					file.deleteFile();
					file.create();
					FileOutputStream fos(file);
					bool ok = copy.writeTo(fos);

					if (ok)
						debugToConsole(this, "Written MIDI content to " + r.getFile().getFullPathName());

					pool->clearData();
					pool->loadAllFilesFromProjectFolder();
					pool->loadFromReference(r, PoolHelpers::ForceReloadStrong);
					return ok;
				}
				else
				{
					for (int i = existingFile.getNumTracks(); i < trackIndex; i++)
					{
						MidiMessageSequence empty;
						empty.addEvent(MidiMessage::pitchWheel(1, 8192));
						empty.addEvent(timeSigMessage);
						empty.addEvent(endMessage);
						existingFile.addTrack(empty);
					}

					existingFile.addTrack(trackCopy);

					r.getFile().deleteFile();
					r.getFile().create();
					FileOutputStream fos(r.getFile());
					
					bool ok = existingFile.writeTo(fos);

					if (ok)
						debugToConsole(this, "Written MIDI content to " + r.getFile().getFullPathName());

					pool->loadFromReference(r, PoolHelpers::ForceReloadStrong);
					return ok;
				}
			}
		}
		else
		{
			MidiFile newFile;
			newFile.setTicksPerQuarterNote(HiseMidiSequence::TicksPerQuarter);

			for (int i = 0; i < trackIndex; i++)
			{
				MidiMessageSequence empty;
				empty.addEvent(MidiMessage::tempoMetaEvent(HiseMidiSequence::TicksPerQuarter));
				newFile.addTrack(empty);
			}

			newFile.addTrack(trackCopy);

			r.getFile().create();
			FileOutputStream fos(r.getFile());

			bool ok = newFile.writeTo(fos);

			if (ok)
				debugToConsole(this, "Written MIDI content to " + r.getFile().getFullPathName());

			pool->loadFromReference(r, PoolHelpers::ForceReloadStrong);
			return ok;
		}
	}

	return false;
}

void MidiPlayer::updatePositionInCurrentSequence(bool ignorePlaybackspeed)
{
	if (auto seq = getCurrentSequence())
	{
		auto newPos = getPlaybackPositionFromTicksSinceStart();
		currentPosition = newPos;
		seq->setPlaybackPosition(currentPosition);
	}
}

void MidiPlayer::addNoteOffsToPendingNoteOns()
{
	auto midiChain = getOwnerSynth()->midiProcessorChain.get();

	bool sortAfterOp = false;

	LockHelpers::SafeLock sl(getMainController(), LockHelpers::AudioLock);

	for (auto& futureEvent : midiChain->artificialEvents)
	{
		if (futureEvent.isNoteOff())
		{
			auto channel = futureEvent.getChannel();
			jassert(channel == currentTrackIndex + 1);
            ignoreUnused(channel);

			futureEvent.setTimeStamp(getLargestBlockSize() - 2);
			sortAfterOp = true;
		}
		if (futureEvent.isNoteOn())
			futureEvent.ignoreEvent(true);
	}

	if (sortAfterOp)
		midiChain->artificialEvents.sortTimestamps();
}

String MidiPlayerBaseType::TransportPaths::getId() const
{ return "MIDI Transport"; }

Identifier MidiPlayerBaseType::getId()
{ RETURN_STATIC_IDENTIFIER("undefined"); }

MidiPlayerBaseType* MidiPlayerBaseType::create(MidiPlayer* player)
{
	ignoreUnused(player);
	return nullptr; 
}

int MidiPlayerBaseType::getPreferredHeight() const
{ return 0; }

void MidiPlayerBaseType::setFont(Font f)
{
	font = f;
}

void MidiPlayerBaseType::cancelUpdates()
{
	if (player != nullptr)
	{
		player->removeSequenceListener(this);
	}
}

MidiPlayer* MidiPlayerBaseType::getPlayer()
{ return player.get(); }

const MidiPlayer* MidiPlayerBaseType::getPlayer() const
{ return player.get(); }

Font MidiPlayerBaseType::getFont() const
{
	return font;
}

juce::Path MidiPlayerBaseType::TransportPaths::createPath(const String& name) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(name);

	ids.addIfNotAlreadyThere(url);

	if (url == "start")
	{
		Path p;
		p.addTriangle({ 0.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 0.5f });
		return p;
	}
	if (url == "stop")
	{
		Path p;
		p.addRectangle<float>({ 0.0f, 0.0f, 1.0f, 1.0f });
		return p;
	}
	if (url == "record")
	{
		Path p;
		p.addEllipse({ 0.0f, 0.0f, 1.0f, 1.0f });
		return p;
	}

	return {};
}

MidiPlayerBaseType::~MidiPlayerBaseType()
{
	cancelUpdates();
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
	}
}



MidiPlayer::Updater::Updater(MidiPlayer& mp) :
	SimpleTimer(mp.getMainController()->getGlobalUIUpdater()),
	parent(mp)
{
	start();
}

void MidiPlayer::Updater::timerCallback()
{
	if (dirty)
	{
		if (handleUpdate(sequenceToUpdate, sendNotificationSync))
		{
			dirty = false;
			sequenceToUpdate = nullptr;
		}
	}
}

bool MidiPlayer::Updater::handleUpdate(HiseMidiSequence::Ptr seq, NotificationType n)
{
	if (n != sendNotificationAsync)
	{
		if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(parent.listenerLock))
		{
			if (seq != nullptr)
			{
				for (auto l : parent.sequenceListeners)
				{
					if (l != nullptr)
						l->sequenceLoaded(seq);
				}
			}
			else
			{
				for (auto l : parent.sequenceListeners)
				{
					if (l != nullptr)
						l->sequencesCleared();
				}
			}

			return true;
		}
		else
			return false;
	}
	else
	{
		sequenceToUpdate = seq;
		dirty = true;
		return true;
	}
}

}
