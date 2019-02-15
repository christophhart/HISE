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

	String encodedState = v.getProperty("Data");

	MemoryBlock mb;

	if (mb.fromBase64Encoding(encodedState))
	{
		zstd::ZDefaultCompressor compressor;
		compressor.expandInplace(mb);
		MemoryInputStream mis(mb, false);
		loadFrom(mis);
	}
}


juce::MidiMessage* HiseMidiSequence::getNextEvent(Range<double> rangeToLookForTicks)
{
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

	return false;
}

juce::MidiMessage* HiseMidiSequence::getMatchingNoteOffForCurrentEvent()
{
	if (auto noteOff = getReadPointer(currentTrackIndex)->getEventPointer(lastPlayedIndex)->noteOffObject)
		return &noteOff->message;

	return nullptr;
}

double HiseMidiSequence::getLength() const
{
	if (auto currentSequence = sequences.getFirst())
		return currentSequence->getEndTime();

	return 0.0;
}

double HiseMidiSequence::getLengthInQuarters()
{
	if (auto currentSequence = sequences.getFirst())
		return currentSequence->getEndTime() / (double)TicksPerQuarter;

	return 0.0;
}

void HiseMidiSequence::loadFrom(InputStream& input)
{
	MidiFile currentFile;
	MidiFile normalisedFile;


	currentFile.readFrom(input);

	for (int i = 0; i < currentFile.getNumTracks(); i++)
	{
		ScopedPointer<MidiMessageSequence> newSequence = new MidiMessageSequence(*currentFile.getTrack(i));
		newSequence->deleteSysExMessages();

		DBG("Track " + String(i + 1));

		for (int i = 0; i < newSequence->getNumEvents(); i++)
		{
			if (newSequence->getEventPointer(i)->message.isMetaEvent())
				newSequence->deleteEvent(i--, false);
		}

		if(newSequence->getNumEvents() > 0)
			normalisedFile.addTrack(*newSequence);
	}

	normalisedFile.setTicksPerQuarterNote(TicksPerQuarter);

	for (int i = 0; i < normalisedFile.getNumTracks(); i++)
	{
		ScopedPointer<MidiMessageSequence> newSequence = new MidiMessageSequence(*normalisedFile.getTrack(i));
		sequences.add(newSequence.release());
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
	return sequences[trackIndex];
}

juce::MidiMessageSequence* HiseMidiSequence::getWritePointer(int trackIndex)
{
	return sequences[trackIndex];
}

void HiseMidiSequence::setCurrentTrackIndex(int index)
{
	if (index != currentTrackIndex)
	{
		double lastTimestamp = 0.0;

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
	if (auto s = getReadPointer(currentTrackIndex))
	{
		auto currentTimestamp = getLength() * normalisedPosition;

		lastPlayedIndex = s->getNextIndexAtTime(currentTimestamp) - 1;
	}
}

juce::RectangleList<float> HiseMidiSequence::getRectangleList(Rectangle<float> targetBounds) const
{
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
	}

	return list;
}

MidiFilePlayer::MidiFilePlayer(MainController *mc, const String &id, ModulatorSynth* ms) :
	MidiProcessor(mc, id)
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

MidiFilePlayer::~MidiFilePlayer()
{
	getMainController()->removeTempoListener(this);
}

void MidiFilePlayer::tempoChanged(double newTempo)
{
	ticksPerSample = MidiPlayerHelpers::samplesToTicks(1, newTempo, getSampleRate());
}

juce::ValueTree MidiFilePlayer::exportAsValueTree() const
{
	ValueTree v = MidiProcessor::exportAsValueTree();

	saveID(CurrentSequence);
	saveID(CurrentTrack);

	ValueTree seq("MidiFiles");

	for (int i = 0; i < currentSequences.size(); i++)
		seq.addChild(currentSequences[i]->exportAsValueTree(), -1, nullptr);

	v.addChild(seq, -1, nullptr);

	return v;
}

void MidiFilePlayer::restoreFromValueTree(const ValueTree &v)
{
	MidiProcessor::restoreFromValueTree(v);

	ValueTree seq = v.getChildWithName("MidiFiles");

	if (seq.isValid())
	{
		for (const auto& s : seq)
		{
			HiseMidiSequence::Ptr newSequence = new HiseMidiSequence();
			newSequence->restoreFromValueTree(s);
			addSequence(newSequence);
		}
	}

	loadID(CurrentSequence);
	loadID(CurrentTrack);
}

void MidiFilePlayer::addSequence(HiseMidiSequence::Ptr newSequence)
{
	currentSequences.add(newSequence);

	for (auto l : sequenceListeners)
	{
		if (l != nullptr)
			l->sequenceLoaded(newSequence);
	}
}

void MidiFilePlayer::clearSequences()
{
	currentSequences.clear();

	currentSequenceIndex = -1;

	for (auto l : sequenceListeners)
	{
		if (l != nullptr)
			l->sequencesCleared();
	}
}

hise::ProcessorEditorBody * MidiFilePlayer::createEditor(ProcessorEditor *parentEditor)
{

#if USE_BACKEND

	return new MidiFilePlayerEditor(parentEditor);

#else

	ignoreUnused(parentEditor);
	jassertfalse;

	return nullptr;

#endif
}

float MidiFilePlayer::getAttribute(int index) const
{
	auto s = (SpecialParameters)index;

	switch (s)
	{
	case Play:
	case Stop:
	case Record:				return (float)(playState == index);
	case CurrentPosition:		return (float)getPlaybackPosition();
	case CurrentSequence:		return (float)(currentSequenceIndex + 1);
	case CurrentTrack:			return (float)(currentTrackIndex + 1);
	default:
		break;
	}
}

void MidiFilePlayer::setInternalAttribute(int index, float newAmount)
{
	auto s = (SpecialParameters)index;

	switch (s)
	{
	case Play:
	case Stop:
	case Record:				timeStampForNextCommand = (uint16)newAmount; changeTransportState(s); break;
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
		getCurrentSequence()->setCurrentTrackIndex(currentTrackIndex);
		break;
	}
	default:
		break;
	}
}

void MidiFilePlayer::prepareToPlay(double sampleRate_, int samplesPerBlock_)
{
	MidiProcessor::prepareToPlay(sampleRate_, samplesPerBlock_);
	tempoChanged(getMainController()->getBpm());
}

void MidiFilePlayer::preprocessBuffer(HiseEventBuffer& buffer, int numSamples)
{
	if (currentSequenceIndex >= 0 && currentPosition != -1.0)
	{
		auto seq = getCurrentSequence();
		seq->setCurrentTrackIndex(currentTrackIndex);

		auto tickThisTime = (numSamples - timeStampForNextCommand) * ticksPerSample;
		auto lengthInTicks = seq->getLength();

		auto positionInTicks = getPlaybackPosition() * lengthInTicks;

		auto delta = tickThisTime / lengthInTicks;

		Range<double> currentRange(positionInTicks, positionInTicks + tickThisTime);

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

					auto id = getMainController()->getEventHandler().getEventIdForNoteOff(newNoteOff);

					jassert(newEvent.getEventId() == id);

					newNoteOff.setEventId(id);
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

void MidiFilePlayer::processHiseEvent(HiseEvent &m) noexcept
{
	currentTimestampInBuffer = m.getTimeStamp();
}

void MidiFilePlayer::addSequenceListener(SequenceListener* newListener)
{
	sequenceListeners.addIfNotAlreadyThere(newListener);
}

void MidiFilePlayer::removeSequenceListener(SequenceListener* listenerToRemove)
{
	sequenceListeners.removeAllInstancesOf(listenerToRemove);
}

void MidiFilePlayer::changeTransportState(SpecialParameters newState)
{
	playState = newState;

	switch (newState)
	{
	case Play:	play(); return;
	case Stop:	stop();	return;
	case Record: record(); return;
	}
}

hise::HiseMidiSequence* MidiFilePlayer::getCurrentSequence() const
{
	return currentSequences[currentSequenceIndex].get();
}

juce::Identifier MidiFilePlayer::getSequenceId(int index) const
{
	if (auto s = currentSequences[index])
	{
		return s->getId();
	}

	return {};
}

double MidiFilePlayer::getPlaybackPosition() const
{
	return fmod(currentPosition, 1.0);
}

void MidiFilePlayer::play()
{
	currentPosition = 0.0;

	if (auto seq = getCurrentSequence())
		seq->resetPlayback();
}

void MidiFilePlayer::stop()
{
	currentPosition = -1.0;
	if (auto seq = getCurrentSequence())
		seq->resetPlayback();
}

void MidiFilePlayer::record()
{
	// Not yet implemented
	jassertfalse;
}


void MidiFilePlayer::updatePositionInCurrentSequence()
{
	if (auto seq = getCurrentSequence())
	{
		seq->setPlaybackPosition(getPlaybackPosition());
	}
}

MidiFilePlayerBaseType::~MidiFilePlayerBaseType()
{
	

	if (player != nullptr)
	{
		player->removeSequenceListener(this);
		player->removeChangeListener(this);
	}
}

MidiFilePlayerBaseType::MidiFilePlayerBaseType(MidiFilePlayer* player_) :
	player(player_),
	font(GLOBAL_BOLD_FONT())
{
	player->addSequenceListener(this);
	player->addChangeListener(this);
}

void MidiFilePlayerBaseType::changeListenerCallback(SafeChangeBroadcaster* b)
{
	int thisSequence = (int)getPlayer()->getAttribute(MidiFilePlayer::CurrentSequence);

	if (thisSequence != lastSequenceIndex)
	{
		lastSequenceIndex = thisSequence;
		sequenceIndexChanged();
	}

	int trackIndex = (int)getPlayer()->getAttribute(MidiFilePlayer::CurrentTrack);

	if (trackIndex != lastTrackIndex)
	{
		lastTrackIndex = trackIndex;
		trackIndexChanged();
	}
}


}