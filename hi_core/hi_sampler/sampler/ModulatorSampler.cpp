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

ModulatorSampler::ModulatorSampler(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices),
preloadSize(PRELOAD_SIZE),
undoManager(new UndoManager()),
asyncPreloader(this),
asyncPurger(this),
soundCache(new AudioThumbnailCache(512)),
sampleStartChain(new ModulatorChain(mc, "Sample Start", numVoices, Modulation::GainMode, this)),
crossFadeChain(new ModulatorChain(mc, "Group Fade", numVoices, Modulation::GainMode, this)),
sampleMap(new SampleMap(this)),
rrGroupAmount(1),
bufferSize(4096),
currentRRGroupIndex(1),
useRoundRobinCycleLogic(true),
pitchTrackingEnabled(true),
oneShotEnabled(false),
crossfadeGroups(false),
useGlobalFolder(false),
purged(false),
numChannels(1),
deactivateUIUpdate(false)
{
	setGain(1.0);

	enableAllocationFreeMessages(50);

	parameterNames.add("PreloadSize");
	parameterNames.add("BufferSize");
	parameterNames.add("VoiceAmount");
	parameterNames.add("RRGroupAmount");
	parameterNames.add("SamplerRepeatMode");
	parameterNames.add("PitchTracking");
	parameterNames.add("OneShot");
	parameterNames.add("CrossfadeGroups");
	parameterNames.add("Purged");

	editorStateIdentifiers.add("SampleStartChainShown");
	editorStateIdentifiers.add("SettingsShown");
	editorStateIdentifiers.add("WaveformShown");
	editorStateIdentifiers.add("MapPanelShown");
	editorStateIdentifiers.add("TableShown");
	editorStateIdentifiers.add("MidiSelectActive");
	editorStateIdentifiers.add("CrossfadeTableShown");
	editorStateIdentifiers.add("BigSampleMap");
	editorStateIdentifiers.add("ChannelShown");

	setEditorState(EditorStates::MapPanelShown, true);
	setEditorState(EditorStates::BigSampleMap, true);

	sampleStartChain->setFactoryType(new VoiceStartModulatorFactoryType(numVoices, Modulation::GainMode, sampleStartChain));

	sampleStartChain->setColour(Colour(0xFF413C75));

	crossFadeChain->setColour(Colour(0xFFEB8A3B));

	for (int i = 0; i < 127; i++) samplerDisplayValues.currentNotes[i] = -1;

	setVoiceAmount(numVoices);


	for (int i = 0; i < 8; i++)
	{
		crossfadeTables.add(new SampleLookupTable());
	}

	getMatrix().setAllowResizing(true);

    
    
}


ModulatorSampler::~ModulatorSampler()
{
	sampleMap = nullptr;
	deleteAllSounds();
}

bool ModulatorSampler::useGlobalFolderForSaving() const { return useGlobalFolder; }

void ModulatorSampler::replaceReferencesWithGlobalFolder()
{
	// Do nothing more, the rest will be managed by the samplemap...
	setUseGlobalFolderForSaving();
}

int ModulatorSampler::getRRGroupsForMessage(int noteNumber, int velocity)
{
	return roundRobinMap.getRRGroupsForMessage(noteNumber, velocity);
}

void ModulatorSampler::refreshRRMap()
{
	roundRobinMap.clear();
	for (int i = 0; i < sounds.size(); i++)
	{
		const ModulatorSamplerSound *sound = static_cast<const ModulatorSamplerSound*>(sounds.getUnchecked(i).get());
		roundRobinMap.addSample(sound);
	}
}

void ModulatorSampler::setNumChannels(int numNewChannels)
{
	numChannels = numNewChannels;

	getMatrix().setNumSourceChannels(numChannels * 2);

	if (getMatrix().getNumDestinationChannels() == 2)
	{
		getMatrix().loadPreset(RoutableProcessor::Presets::AllChannelsToStereo);
	}
	else
	{
		getMatrix().loadPreset(RoutableProcessor::Presets::AllChannels);
	}

	const int prevVoiceAmount = voiceAmount;
	const int prevVoiceLimit = (int)getAttribute(ModulatorSynth::VoiceLimit);

	voiceAmount = -1;
	setVoiceAmount(prevVoiceAmount);
	setVoiceLimit(prevVoiceLimit);

	if (numChannels < 1) numChannels = 1;
	if (numChannels > NUM_MIC_POSITIONS) numChannels = NUM_MIC_POSITIONS;

	for (int i = 0; i < NUM_MIC_POSITIONS; i++)
	{
		channelData[i].enabled = i <= numChannels;
		channelData[i].suffix = "";
		channelData[i].level = channelData[i].enabled ? 1.0f : 0.0f;
	}

}

void ModulatorSampler::setNumMicPositions(StringArray &micPositions)
{
	if (micPositions.size() == 0) return;
	
	setNumChannels(micPositions.size());
	
	for (int i = 0; i < numChannels; i++)
	{
		channelData[i].suffix = micPositions[i];
	}

	sendChangeMessage();

}

void ModulatorSampler::refreshCrossfadeTables()
{
	
}

void ModulatorSampler::restoreFromValueTree(const ValueTree &v)
{
	loadAttribute(PreloadSize, "PreloadSize");
	
	setInternalAttribute(BufferSize, v.getProperty("BufferSize", 4096));

	loadAttribute(PitchTracking, "PitchTracking");
	loadAttribute(OneShot, "OneShot");
	const int newNumChannels = v.getProperty("NumChannels", 1);

	if (newNumChannels != numChannels)
	{
		setNumChannels(newNumChannels);
	}

	ValueTree channels = v.getChildWithName("channels");

	if (v.getChildWithName("channels").isValid())

	for (int i = 0; i < numChannels; i++)
	{
		channelData[i].restoreFromValueTree(channels.getChild(i));
	}

	 // Macro does not support correct default value!
	setVoiceAmount(v.getProperty("VoiceAmount", voiceAmount));
	
	loadAttribute(SamplerRepeatMode, "SamplerRepeatMode");
	loadAttribute(Purged, "Purged");

    loadSampleMap(v.getChildWithName("samplemap"));
	
    loadAttribute(CrossfadeGroups, "CrossfadeGroups");
    loadAttribute(RRGroupAmount, "RRGroupAmount");

	if (crossfadeGroups)
	{
		jassert(rrGroupAmount <= crossfadeTables.size());

		for (int i = 0; i < crossfadeTables.size(); i++)
		{
			loadTable(crossfadeTables[i], "Group" + String(i) + "Table");
		}
	}

	ModulatorSynth::restoreFromValueTree(v);
};

ValueTree ModulatorSampler::exportAsValueTree() const
{
	ValueTree v = ModulatorSynth::exportAsValueTree();

	saveAttribute(PreloadSize, "PreloadSize");
	saveAttribute(BufferSize, "BufferSize");
	saveAttribute(VoiceAmount, "VoiceAmount");
	saveAttribute(SamplerRepeatMode, "SamplerRepeatMode");
	saveAttribute(RRGroupAmount, "RRGroupAmount");
	saveAttribute(PitchTracking, "PitchTracking");
	saveAttribute(OneShot, "OneShot");
	saveAttribute(CrossfadeGroups, "CrossfadeGroups");
	saveAttribute(Purged, "Purged");
	v.setProperty("NumChannels", numChannels, nullptr);

	ValueTree channels("channels");

	for (int i = 0; i < numChannels; i++)
	{
		ValueTree channelDataTree = channelData[i].exportAsValueTree();

		channels.addChild(channelDataTree, -1, nullptr);
	}

	v.addChild(channels, -1, nullptr);

	if (crossfadeGroups)
	{
		jassert(rrGroupAmount <= crossfadeTables.size());

		for (int i = 0; i < crossfadeTables.size(); i++)
		{
			saveTable(crossfadeTables[i], "Group" + String(i) + "Table");
		}
	}

	v.addChild(sampleMap->exportAsValueTree(), -1, nullptr);

	v.setProperty("SampleMap", sampleMap->getFile().getFullPathName(), nullptr);

	return v;
}

float ModulatorSampler::getAttribute(int parameterIndex) const
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters) return ModulatorSynth::getAttribute(parameterIndex);

	switch (parameterIndex)
	{
	case PreloadSize:		return (float)preloadSize;
	case BufferSize:		return (float)bufferSize;
	case VoiceAmount:		return (float)voiceAmount;
	case SamplerRepeatMode: return (float)repeatMode;
	case RRGroupAmount:		return (float)rrGroupAmount;
	case PitchTracking:		return pitchTrackingEnabled ? 1.0f : 0.0f;
	case OneShot:			return oneShotEnabled ? 1.0f : 0.0f;
	case CrossfadeGroups:	return crossfadeGroups ? 1.0f : 0.0f;
	case Purged:			return purged ? 1.0f : 0.0f;
	default:				jassertfalse; return -1.0f;
	}
}

void ModulatorSampler::setInternalAttribute(int parameterIndex, float newValue)
{
	if (parameterIndex < ModulatorSynth::numModulatorSynthParameters)
	{
		ModulatorSynth::setInternalAttribute(parameterIndex, newValue);
		return;
	}

	switch (parameterIndex)
	{
	case PreloadSize:		setPreloadSizeAsync((int)newValue); break;
	case BufferSize:		bufferSize = (int)newValue; refreshStreamingBuffers(); break;
	case VoiceAmount:		setVoiceAmount((int)newValue); break;
	case RRGroupAmount:		setRRGroupAmount((int)newValue); refreshCrossfadeTables(); break;
	case SamplerRepeatMode: repeatMode = (RepeatMode)(int)newValue; break;
	case PitchTracking:		pitchTrackingEnabled = newValue == 1.0f; break;
	case OneShot:			oneShotEnabled = newValue == 1.0f; break;
	case CrossfadeGroups:	crossfadeGroups = newValue == 1.0f; refreshCrossfadeTables(); break;
	case Purged:			purgeAllSamples(newValue == 1.0f); break;
	default:				jassertfalse; break;
	}
}

const ModulatorSamplerSound * ModulatorSampler::getSound(int soundIndex) const
{
	SynthesiserSound *s = sounds[soundIndex];
	return static_cast<const ModulatorSamplerSound*>(s);
}

ModulatorSamplerSound * ModulatorSampler::getSound(int soundIndex)
{
	SynthesiserSound *s = sounds[soundIndex];
	return static_cast<ModulatorSamplerSound*>(s);
}

Processor * ModulatorSampler::getChildProcessor(int processorIndex)
{
	jassert(processorIndex < numInternalChains);

	switch (processorIndex)
	{
	case GainModulation:	return gainChain;
	case PitchModulation:	return pitchChain;
	case SampleStartModulation:	return sampleStartChain;
	case CrossFadeModulation: return crossFadeChain;
	case MidiProcessor:		return midiProcessorChain;
	case EffectChain:		return effectChain;
	default:				jassertfalse; return nullptr;
	}
}

const Processor * ModulatorSampler::getChildProcessor(int processorIndex) const
{
	jassert(processorIndex < numInternalChains);

	switch (processorIndex)
	{
	case GainModulation:	return gainChain;
	case PitchModulation:	return pitchChain;
	case SampleStartModulation:	return sampleStartChain;
	case CrossFadeModulation: return crossFadeChain;
	case MidiProcessor:		return midiProcessorChain;
	case EffectChain:		return effectChain;
	default:				jassertfalse; return nullptr;
	}
}



void ModulatorSampler::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(sampleRate, samplesPerBlock);

	if (sampleRate != -1.0)
	{
		crossfadeBuffer = AudioSampleBuffer(1, samplesPerBlock);

		sampleStartChain->prepareToPlay(sampleRate, samplesPerBlock);
		crossFadeChain->prepareToPlay(sampleRate, samplesPerBlock);
	}
}

ProcessorEditorBody* ModulatorSampler::createEditor(ProcessorEditor *parentEditor)
{
#if USE_BACKEND

	return new SamplerBody(parentEditor);

#else 

	ignoreUnused(parentEditor);
	jassertfalse;
	return nullptr;

#endif
}

var ModulatorSampler::getPropertyForSound(int soundIndex, ModulatorSamplerSound::Property p)
{
	SynthesiserSound * s = sounds[soundIndex];

	ModulatorSamplerSound *sound = static_cast<ModulatorSamplerSound*>(s);

	if (p == ModulatorSamplerSound::ID) return String(soundIndex);

	else return sound->getProperty(p);
}

void ModulatorSampler::loadCacheFromFile(File &f)
{
	FileInputStream fis(f);

	soundCache->clear();
	soundCache->readFromStream(fis);
}

void ModulatorSampler::refreshStreamingBuffers()
{
	for (int i = 0; i < getNumVoices(); i++)
	{
		SynthesiserVoice *v = getVoice(i);
		static_cast<ModulatorSamplerVoice*>(v)->resetVoice();
		static_cast<ModulatorSamplerVoice*>(v)->setLoaderBufferSize(bufferSize);
	}
}

void ModulatorSampler::deleteSound(ModulatorSamplerSound *s)
{
	ScopedLock sl(getMainController()->getLock());

	allNotesOff(1, false);

	for (int i = 0; i < voices.size(); i++)
	{
		if (static_cast<ModulatorSamplerVoice*>(voices[i])->getCurrentlyPlayingSamplerSound() == s)
		{
			static_cast<ModulatorSamplerVoice*>(voices[i])->resetVoice();
		}
	}

	s->removeAllChangeListeners();

    const int deletedIndex = s->getProperty(ModulatorSamplerSound::ID);

	SynthesiserSound::Ptr refPointer = s;

	sounds.removeObject(s);

	getMainController()->getSampleManager().getModulatorSamplerSoundPool()->deleteSound(static_cast<ModulatorSamplerSound*>(refPointer.get()));

	refreshMemoryUsage();

    for(int i = deletedIndex; i < getNumSounds(); i++)
    {
        static_cast<ModulatorSamplerSound*>(sounds[i].get())->setNewIndex(i);
    }
    
	sendChangeMessage();
}

void ModulatorSampler::deleteAllSounds()
{
	//ReferenceCountedArray<ModulatorSamplerSound> savedSounds(sounds);

	ScopedLock sl(getMainController()->getLock());

	for (int i = 0; i < voices.size(); i++)
	{
		static_cast<ModulatorSamplerVoice*>(getVoice(i))->resetVoice();
	}


	clearSounds();

	/*
	for(int i = 0; i < savedSounds.size(); i++)
	{
	getMainController()->getModulatorSamplerSoundPool()->deleteSound(savedSounds[i]);
	}

	savedSounds.clear();

	*/

	getMainController()->getSampleManager().getModulatorSamplerSoundPool()->clearUnreferencedSamples();

	getMainController()->getSampleManager().getModulatorSamplerSoundPool()->clearUnreferencedMonoliths();

	refreshMemoryUsage();
	sendChangeMessage();
}

void ModulatorSampler::refreshPreloadSizes()
{
	if (getNumSounds() != 0)
	{
		new SoundPreloadThread(this);
	}
}

double ModulatorSampler::getDiskUsage()
{
	double diskUsage = 0.0;

	for (int i = 0; i < getNumVoices(); i++)
	{
		diskUsage += static_cast<ModulatorSamplerVoice*>(getVoice(i))->getDiskUsage();
	}

	return diskUsage * 100.0;
}

void ModulatorSampler::refreshMemoryUsage()
{
	int64 actualPreloadSize = 0;

	for (int i = 0; i < getNumSounds(); i++)
	{
		for (int j = 0; j < numChannels; j++)
		{
			actualPreloadSize += getSound(i)->getReferenceToSound(j)->getActualPreloadSize();
		}
		
	}

	for (int i = 0; i < getNumVoices(); i++)
	{
		actualPreloadSize += static_cast<ModulatorSamplerVoice*>(getVoice(i))->getStreamingBufferSize();
	}

	const int64 streamBufferSizePerVoice = 2 *				// two buffers
		getMainController()->getSampleManager().getStreamingBufferSize() *		// buffer size per buffer
		sizeof(float) *  // bytes per sample
		2 * numChannels;				// number of channels

	memoryUsage = actualPreloadSize + streamBufferSizePerVoice * getNumVoices();

	sendChangeMessage();
	getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
}

void ModulatorSampler::setVoiceAmount(int newVoiceAmount)
{
	if (newVoiceAmount != voiceAmount)
	{
		ScopedLock sl(getSynthLock());

		voiceAmount = jmin<int>(NUM_POLYPHONIC_VOICES, newVoiceAmount);

		if (getAttribute(ModulatorSynth::VoiceLimit) > voiceAmount)
		{
			setAttribute(ModulatorSynth::VoiceLimit, (float)voiceAmount, sendNotification);
		}

		allNotesOff(1, false);
		clearVoices();

		for (int i = 0; i < voiceAmount; i++)
		{
			if (numChannels != 1)
			{
				addVoice(new MultiMicModulatorSamplerVoice(this, numChannels));
			}
			else
			{
				addVoice(new ModulatorSamplerVoice(this));
			}

			if (Processor::getSampleRate() != -1.0) static_cast<ModulatorSamplerVoice*>(getVoice(i))->prepareToPlay(Processor::getSampleRate(), getBlockSize());
		};

		setKillFadeOutTime((int)getAttribute(ModulatorSynth::KillFadeTime)); 

		refreshMemoryUsage();
		refreshStreamingBuffers();
	}
}

void ModulatorSampler::setPreloadSize(int newPreloadSize)
{
	if (newPreloadSize != 0 && newPreloadSize != preloadSize)
	{
		preloadSize = newPreloadSize;

		refreshPreloadSizes();
	};

	refreshMemoryUsage();
}

void ModulatorSampler::setPreloadSizeAsync(int newPreloadSize)
{
    asyncPreloader.setPreloadSize(newPreloadSize);
}

void ModulatorSampler::setCurrentPlayingPosition(double normalizedPosition)
{
	samplerDisplayValues.currentSamplePos = normalizedPosition;
}

void ModulatorSampler::setCrossfadeTableValue(float newValue)
{
	samplerDisplayValues.crossfadeTableValue = newValue;

	const int currentlyShownTable = getEditorState(getEditorStateForIndex(ModulatorSampler::EditorStates::CrossfadeTableShown));

	if (currentlyShownTable >= 0 && currentlyShownTable < 8)
	{
		sendTableIndexChangeMessage(false, crossfadeTables[currentlyShownTable], newValue);
	}
}

void ModulatorSampler::resetNoteDisplay(int noteNumber)
{
	lastStartedVoice = nullptr;
	samplerDisplayValues.currentNotes[noteNumber] = -1;
	samplerDisplayValues.currentSamplePos = -1.0;
	sendAllocationFreeChangeMessage();
}

void ModulatorSampler::resetNotes()
{
	for (int i = 0; i < voices.size(); i++)
	{
		static_cast<ModulatorSamplerVoice*>(voices[i])->resetVoice();
	}
}

void ModulatorSampler::addSamplerSound(const ValueTree &description, int index, bool forceReuse)
{
	ScopedLock sl(getMainController()->getLock());

	jassert(sounds.size() == index);

	ModulatorSamplerSoundPool *pool = getMainController()->getSampleManager().getModulatorSamplerSoundPool();
    ModulatorSamplerSound *newSound = pool->addSound(description, index, description.hasProperty("mono_sample_start") || forceReuse);

	newSound->restoreFromValueTree(description);

	sounds.add(newSound);
	newSound->setUndoManager(undoManager);
	newSound->addChangeListener(sampleMap);

	sendChangeMessage();
}


void ModulatorSampler::addSamplerSounds(OwnedArray<ModulatorSamplerSound>& monolithicSounds)
{
	ScopedLock sl(getMainController()->getLock());

	jassert(sounds.size() == 0);

	const int numNewSounds = monolithicSounds.size();

	for (int i = 0; i < numNewSounds; i++)
	{
		ModulatorSamplerSound* newSound = monolithicSounds.removeAndReturn(0);

		sounds.add(newSound);

		newSound->setPurged(purged);

		newSound->setUndoManager(undoManager);
		newSound->addChangeListener(sampleMap);
	}

	sendChangeMessage();
}

SampleThreadPool * ModulatorSampler::getBackgroundThreadPool()
{
	return getMainController()->getSampleManager().getGlobalSampleThreadPool();
}

String ModulatorSampler::getMemoryUsage() const
{
	String memory;

	const double m = ((double)memoryUsage / 1024.0f / 1024.0f);

	memory << String(m, 2);
	memory << "MB";

	return memory;
}

void ModulatorSampler::preStartVoice(int voiceIndex, int noteNumber)
{
	ModulatorSynth::preStartVoice(voiceIndex, noteNumber);

	sampleStartChain->startVoice(voiceIndex);
	crossFadeChain->startVoice(voiceIndex);

	const float sampleStartModValue = sampleStartChain->getConstantVoiceValue(voiceIndex);

	samplerDisplayValues.currentSampleStartPos = sampleStartModValue;

	static_cast<ModulatorSamplerVoice*>(getLastStartedVoice())->setSampleStartModValue(sampleStartModValue);
}

void ModulatorSampler::preVoiceRendering(int startSample, int numThisTime)
{
	ModulatorSynth::preVoiceRendering(startSample, numThisTime);

	if (crossfadeGroups)
	{
		crossFadeChain->renderNextBlock(crossfadeBuffer, startSample, numThisTime);
	}
}

bool ModulatorSampler::soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity)
{
	const bool messageFits = ModulatorSynth::soundCanBePlayed(sound, midiChannel, midiNoteNumber, velocity);

	if (!messageFits) return false;
	
	const bool rrGroupApplies = crossfadeGroups || static_cast<ModulatorSamplerSound*>(sound)->appliesToRRGroup(currentRRGroupIndex);

	if (!rrGroupApplies) return false;

	const bool preloadBufferIsNonZero = static_cast<ModulatorSamplerSound*>(sound)->getReferenceToSound()->getPreloadBuffer().getNumSamples() != 0;

	if (!preloadBufferIsNonZero) return false;

	return true;
}

void ModulatorSampler::handleRetriggeredNote(ModulatorSynthVoice *voice)
{
	switch (repeatMode)
	{
	case RepeatMode::DoNothing:		return;
	case RepeatMode::KillNote:		voice->killVoice();
	case RepeatMode::NoteOff:		voice->stopNote(1.0f, true);

	}
}

void ModulatorSampler::noteOff(const HiseEvent &m)
{
	if (!oneShotEnabled)
	{
		ModulatorSynth::noteOff(m);
	}
}

void ModulatorSampler::preHiseEventCallback(const HiseEvent &m)
{
	crossFadeChain->handleHiseEvent(m);

	if (m.isNoteOnOrOff())
	{
		sampleStartChain->handleHiseEvent(m);
		

		if (m.isNoteOn())
		{
			if (useRoundRobinCycleLogic)
			{
				currentRRGroupIndex++;
				if (currentRRGroupIndex > rrGroupAmount) currentRRGroupIndex = 1;
			}

			samplerDisplayValues.currentGroup = currentRRGroupIndex;
		}

		if (m.isNoteOn())
		{
			samplerDisplayValues.currentNotes[m.getNoteNumber() + m.getTransposeAmount()] = m.getVelocity();
		}
		else
		{
			const HiseEvent *noteOn = getMainController()->getNoteOnEventFor(m);

			const int noteNumber = noteOn->getNoteNumber() + noteOn->getTransposeAmount();

			samplerDisplayValues.currentNotes[noteNumber] = -1;
		}

		
        sendAllocationFreeChangeMessage();
	}

	if (!m.isNoteOff() || !oneShotEnabled)
	{
		ModulatorSynth::preHiseEventCallback(m);

	}
}

void ModulatorSampler::calculateCrossfadeModulationValuesForVoice(int voiceIndex, int startSample, int numSamples, int groupIndex)
{
	if (groupIndex > 8) return;

	crossFadeChain->renderVoice(voiceIndex, startSample, numSamples);

	float *crossFadeValues = crossFadeChain->getVoiceValues(voiceIndex);

	const float* timeVariantCrossFadeValues = crossfadeBuffer.getReadPointer(0);

	FloatVectorOperations::multiply(crossFadeValues, timeVariantCrossFadeValues, startSample + numSamples);

	if (isLastStartedVoice(static_cast<ModulatorSynthVoice*>(getVoice(voiceIndex))) && numSamples > 0)
	{
		setCrossfadeTableValue(crossFadeValues[startSample]);
	}

	SampleLookupTable *table = crossfadeTables[groupIndex];

	for (int i = 0; i < numSamples; i++)
	{
		const float value = CONSTRAIN_TO_0_1(crossFadeValues[i + startSample]);
		const float tableValue = table->getInterpolatedValue((double)value * (double)SAMPLE_LOOKUP_TABLE_SIZE);

		crossFadeValues[i + startSample] = tableValue;
	}
}

void ModulatorSampler::clearSampleMap()
{
	sampleMap->saveIfNeeded();

	deleteAllSounds();
	sampleMap->clear();
}

void ModulatorSampler::loadSampleMapFromMonolith(const String &sampleMapId)
{
	setBypassed(true);

	const ValueTree loadedMap = getMainController()->getSampleManager().getLoadedSampleMap(sampleMapId);

	jassert(loadedMap.isValid());

	sampleMap->restoreFromValueTree(loadedMap);

	setBypassed(false);
}

void ModulatorSampler::loadSampleMap(const File &f)
{
	setBypassed(true);

	clearSampleMap();
	sampleMap->load(f);

	

	setBypassed(false);
}

void ModulatorSampler::loadSampleMap(const ValueTree &valueTreeData)
{
	setBypassed(true);

	clearSampleMap();
	sampleMap->restoreFromValueTree(valueTreeData);

	setBypassed(false);
}

void ModulatorSampler::saveSampleMap() const
{
	sampleMap->save();
}

void ModulatorSampler::saveSampleMapAsMonolith(Component* mainEditor) const
{
	sampleMap->saveAsMonolith(mainEditor);
}

bool ModulatorSampler::setCurrentGroupIndex(int currentIndex)
{
	if (currentIndex <= rrGroupAmount)
	{
		currentRRGroupIndex = currentIndex;
		return true;
	}
	else
	{
		return false;
	}
}

void ModulatorSampler::setRRGroupAmount(int newGroupLimit)
{
	rrGroupAmount = jmax(1, newGroupLimit);

	allNotesOff(1, true);

	for (int i = 0; i < sounds.size(); i++)
	{
		getSound(i)->setMaxRRGroupIndex(rrGroupAmount);
	};
}
