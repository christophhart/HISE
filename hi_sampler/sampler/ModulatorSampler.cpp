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

namespace hise { using namespace juce;





SET_DOCUMENTATION(ModulatorSampler)
{
	SET_DOC_NAME(ModulatorSampler);

	addLine("A Sampler is a synthesiser which allows playback of samples.");
	addLine("Features:");
	addLine("- Disk Streaming with fast MemoryMappedFile reading");
	addLine("- Looping with crossfades & sample start modulation");
	addLine("- Round - Robin groups");
	addLine("- Application - wide sample pool with reference counting to ensure minimal memory usage.");
	addLine("- Different playback modes(pitch tracking / one shot, etc.)");

	ADD_PARAMETER_DOC_WITH_NAME(PreloadSize, "Preload Size",
		"The preload size in samples for all samples that are loaded into the sampler. " \
		"If the preload size is `-1`, then the whole sample will be loaded into memory.");

	ADD_PARAMETER_DOC_WITH_NAME(BufferSize, "Buffer Size",
		"The buffer size of the streaming buffers (2 per voice) in samples.  " \
		"The sampler uses two buffers which are swapped (one is used for reading from disk and one is used to supply the sampler with the audio data)");

	ADD_PARAMETER_DOC_WITH_NAME(VoiceAmount, "Soft Limit", 
		"The amount of voices that the sampler can play. ");

	ADD_PARAMETER_DOC_WITH_NAME(RRGroupAmount, "RR Groups", 
		"The number of groups that are cycled in a round robin manier. "\
		"This is effectively just another dimension for mapping samples and " \
		"can be used for many different purposes (handling round robins is just the default).");

	ADD_PARAMETER_DOC_WITH_NAME(SamplerRepeatMode, "Retrigger",
		"Determines how the sampler treats repeated notes.  "); 

	ADD_PARAMETER_DOC(PitchTracking, 
		"Enables pitch ratio modification for different notes than the root note. Disable this for drum samples.");

	ADD_PARAMETER_DOC(OneShot, 
		"Plays the whole sample (ignores the note off) if set to enabled.");

	ADD_PARAMETER_DOC_WITH_NAME(CrossfadeGroups, "Group XF", 
		"If enabled, the groups are played simultanously and can be crossfaded with the Group-Fade Modulation Chain.");

	ADD_PARAMETER_DOC(Purged, 
		"If this is true, all samples of this sampler won't be loaded into memory. Turning this on will load them.");

	ADD_PARAMETER_DOC(Reversed, 
		"If this is true, the samples will be fully loaded into preload buffer and reversed");

	ADD_CHAIN_DOC(SampleStartModulation, "Sample Start", 
		"Allows modification of the sample start if the sound allows this. The modulation range is depending on the *SampleStartMod* value of each sample.");

	ADD_CHAIN_DOC(CrossFadeModulation, "Group Fade",
		"Fades between the RR groups. This can be used for crossfading dynamics samples.");
};


ModulatorSampler::ModulatorSampler(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices),
preloadSize(PRELOAD_SIZE),
asyncPurger(this),
soundCache(new AudioThumbnailCache(512)),
sampleStartChain(new ModulatorChain(mc, "Sample Start", numVoices, Modulation::GainMode, this)),
crossFadeChain(new ModulatorChain(mc, "Group Fade", numVoices, Modulation::GainMode, this)),
sampleMap(new SampleMap(this)),
rrGroupAmount(1),
bufferSize(4096),
preloadScaleFactor(1),
currentRRGroupIndex(1),
useRoundRobinCycleLogic(true),
pitchTrackingEnabled(true),
oneShotEnabled(false),
crossfadeGroups(false),
crossfadeBuffer(1, 0),
useGlobalFolder(false),
purged(false),
reversed(false),
numChannels(1),
deactivateUIUpdate(false),
samplePreloadPending(false),
temporaryVoiceBuffer(true, 2, 0),
samplePropertyUpdater(this)
{
#if USE_BACKEND
	sampleEditHandler = new SampleEditHandler(this);
#endif

	
	

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
	parameterNames.add("Reversed");

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

    
    
    
	sampleStartChain->setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff5e8127)));

	crossFadeChain->setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff884b29)));

	for (int i = 0; i < 127; i++) samplerDisplayValues.currentNotes[i] = 0;

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

void ModulatorSampler::setReversed(bool shouldBeReversed)
{
    if (reversed != shouldBeReversed)
    {
        auto f = [shouldBeReversed](Processor* p)
        {
            auto s = static_cast<ModulatorSampler*>(p);

            s->reversed = shouldBeReversed;

            ModulatorSampler::SoundIterator sIter(s);

            while (auto sound = sIter.getNextSound())
            {
                sound->setReversed(shouldBeReversed);
            }

            s->refreshMemoryUsage();

            return true;
        };

        killAllVoicesAndCall(f);
    }
}

void ModulatorSampler::setNumChannels(int numNewChannels)
{


	jassert(numNewChannels <= (NUM_MAX_CHANNELS / 2));

	numChannels = jmin<int>(NUM_MAX_CHANNELS/2, numNewChannels);

	if (!useStaticMatrix)
	{
		getMatrix().setNumSourceChannels(numChannels * 2);

		if (getMatrix().getNumDestinationChannels() == 2)
		{
			getMatrix().loadPreset(RoutableProcessor::Presets::AllChannelsToStereo);
		}
		else
		{
			getMatrix().loadPreset(RoutableProcessor::Presets::AllChannels);
		}
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
		channelData[i].enabled = (channelData[i].enabled && i <= numChannels);
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

bool ModulatorSampler::checkAndLogIsSoftBypassed(DebugLogger::Location location) const
{
	return const_cast<MainController*>(getMainController())->getDebugLogger().checkIsSoftBypassed(this, location);
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
	
	loadAttribute(Reversed, "Reversed");

	loadAttribute(SamplerRepeatMode, "SamplerRepeatMode");
	loadAttribute(Purged, "Purged");

	killAllVoicesAndCall([v](Processor* p) { static_cast<ModulatorSampler*>(p)->loadSampleMapSync(v.getChildWithName("samplemap")); return true; });

    loadAttribute(CrossfadeGroups, "CrossfadeGroups");
    loadAttribute(RRGroupAmount, "RRGroupAmount");

	for (int i = 0; i < crossfadeTables.size(); i++)
	{
		loadTable(crossfadeTables[i], "Group" + String(i) + "Table");
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
	saveAttribute(Reversed, "Reversed");
	v.setProperty("NumChannels", numChannels, nullptr);

	ValueTree channels("channels");

	for (int i = 0; i < numChannels; i++)
	{
		ValueTree channelDataTree = channelData[i].exportAsValueTree();

		channels.addChild(channelDataTree, -1, nullptr);
	}

	v.addChild(channels, -1, nullptr);

	for (int i = 0; i < crossfadeTables.size(); i++)
	{
		saveTable(crossfadeTables[i], "Group" + String(i) + "Table");
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
	case Reversed:			return reversed ? 1.0f : 0.0f;
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
	case BufferSize:		
	{
		bufferSize = (int)newValue; 
		killAllVoicesAndCall([](Processor*p) {static_cast<ModulatorSampler*>(p)->refreshStreamingBuffers(); return true; });
		break;
	}
	case VoiceAmount:		setVoiceAmount((int)newValue); break;
	case RRGroupAmount:		setRRGroupAmount((int)newValue); refreshCrossfadeTables(); break;
	case SamplerRepeatMode: repeatMode = (RepeatMode)(int)newValue; break;
	case PitchTracking:		pitchTrackingEnabled = newValue == 1.0f; break;
	case OneShot:			oneShotEnabled = newValue == 1.0f; break;
	case Reversed:			setReversed(newValue > 0.5f); break;
	case CrossfadeGroups:	crossfadeGroups = newValue == 1.0f; refreshCrossfadeTables(); break;
	case Purged:			purgeAllSamples(newValue == 1.0f); break;
	default:				jassertfalse; break;
	}
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



void ModulatorSampler::prepareToPlay(double newSampleRate, int samplesPerBlock)
{
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	if (newSampleRate != -1.0)
	{
		ProcessorHelpers::increaseBufferIfNeeded(crossfadeBuffer, samplesPerBlock);

		StreamingSamplerVoice::initTemporaryVoiceBuffer(&temporaryVoiceBuffer, samplesPerBlock);

		sampleStartChain->prepareToPlay(newSampleRate, samplesPerBlock);
		crossFadeChain->prepareToPlay(newSampleRate, samplesPerBlock);
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
	jassert(getMainController()->getKillStateHandler().voicesAreKilled());

	for (int i = 0; i < getNumVoices(); i++)
	{
		SynthesiserVoice *v = getVoice(i);
		static_cast<ModulatorSamplerVoice*>(v)->resetVoice();
		static_cast<ModulatorSamplerVoice*>(v)->setLoaderBufferSize(bufferSize * preloadScaleFactor);
	}
}

void ModulatorSampler::deleteSound(ModulatorSamplerSound *s)
{
	//ScopedLock sl(getMainController()->getLock());

	ScopedLock sl(getMainController()->getSampleManager().getSamplerSoundLock());

	jassert(getMainController()->getKillStateHandler().voicesAreKilled());

	checkAndLogIsSoftBypassed(DebugLogger::Location::DeleteOneSample);

	

	for (int i = 0; i < voices.size(); i++)
	{
		if (static_cast<ModulatorSamplerVoice*>(voices[i])->getCurrentlyPlayingSamplerSound() == s)
		{
			static_cast<ModulatorSamplerVoice*>(voices[i])->resetVoice();
		}
	}

	s->removeAllChangeListeners();

	const int deletedIndex = s->getProperty(ModulatorSamplerSound::ID);

	sounds.removeObject(s);

	refreshMemoryUsage();

    for(int i = deletedIndex; i < getNumSounds(); i++)
    {
        static_cast<ModulatorSamplerSound*>(sounds[i].get())->setNewIndex(i);
    }
    
	sendChangeMessage();
}

void ModulatorSampler::deleteAllSounds()
{
	ScopedLock sl(getMainController()->getSampleManager().getSamplerSoundLock());

	jassert(getMainController()->getKillStateHandler().voicesAreKilled());

	for (int i = 0; i < voices.size(); i++)
	{
		static_cast<ModulatorSamplerVoice*>(getVoice(i))->resetVoice();
	}

	if(getNumSounds() != 0)
	{
		clearSounds();

		getMainController()->getSampleManager().getModulatorSamplerSoundPool()->clearUnreferencedMonoliths();
	}
	
	refreshMemoryUsage();
	sendChangeMessage();
}

void ModulatorSampler::refreshPreloadSizes()
{
	if (getMainController()->getSampleManager().shouldSkipPreloading() && getNumSounds() != 0)
	{
		// will be loaded later
		samplePreloadPending = true;
		return;
	}
	
	if (!getMainController()->getSampleManager().shouldSkipPreloading() &&  getNumSounds() != 0)
	{
		auto f = [](Processor* p)->bool
		{
			return static_cast<ModulatorSampler*>(p)->preloadAllSamples();
		};

		killAllVoicesAndCall(f);
	}
	
}

double ModulatorSampler::getDiskUsage()
{
    double diskUsage = 0.0;
    
	for (int i = 0; i < getNumVoices(); i++)
	{
		if (auto v = getVoice(i))
		{
			diskUsage += static_cast<ModulatorSamplerVoice*>(v)->getDiskUsage();
		}
	}

	return diskUsage * 100.0;
}

void ModulatorSampler::refreshMemoryUsage()
{
	if (sampleMap == nullptr)
		return;

	const auto temporaryBufferIsFloatingPoint = getTemporaryVoiceBuffer()->isFloatingPoint();
	const auto temporaryBufferShouldBeFloatingPoint = !sampleMap->isMonolith();

	if (temporaryBufferIsFloatingPoint != temporaryBufferShouldBeFloatingPoint)
	{
		temporaryVoiceBuffer = hlac::HiseSampleBuffer(temporaryBufferShouldBeFloatingPoint, 2, 0);

		StreamingSamplerVoice::initTemporaryVoiceBuffer(&temporaryVoiceBuffer, getBlockSize());

		for (auto i = 0; i < getNumVoices(); i++)
		{
			static_cast<ModulatorSamplerVoice*>(getVoice(i))->setStreamingBufferDataType(temporaryBufferShouldBeFloatingPoint);
		}
	}

	int64 actualPreloadSize = 0;

	{
		SoundIterator sIter(this, false);

		while (const auto sound = sIter.getNextSound())
		{
			for (int j = 0; j < numChannels; j++)
			{
				actualPreloadSize += sound->getReferenceToSound(j)->getActualPreloadSize();
			}
		}
	}

	for (int i = 0; i < getNumVoices(); i++)
	{
		//actualPreloadSize += static_cast<ModulatorSamplerVoice*>(getVoice(i))->getStreamingBufferSize();
	}

	const int64 streamBufferSizePerVoice = 2 *				// two buffers
		bufferSize *		// buffer size per buffer
		(sampleMap->isMonolith() ? 2 : 4) *  // bytes per sample
		2 * numChannels;				// number of channels

	memoryUsage = actualPreloadSize + streamBufferSizePerVoice * getNumVoices();

	sendChangeMessage();
	getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
}

void ModulatorSampler::setVoiceAmount(int newVoiceAmount)
{
	

	if (newVoiceAmount != voiceAmount)
	{
		voiceAmount = jmin<int>(NUM_POLYPHONIC_VOICES, newVoiceAmount);

		
		if (getAttribute(ModulatorSynth::VoiceLimit) > voiceAmount)
			setAttribute(ModulatorSynth::VoiceLimit, float(voiceAmount), sendNotification);

		auto f = [](Processor*p) { static_cast<ModulatorSampler*>(p)->setVoiceAmountInternal(); return true; };

		killAllVoicesAndCall(f);
	}
}

void ModulatorSampler::setVoiceAmountInternal()
{
	jassert(allVoicesAreKilled());

	{
		ScopedLock sl(getMainController()->getLock());

		deleteAllVoices();

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

			dynamic_cast<ModulatorSamplerVoice*>(voices.getLast())->setStreamingBufferDataType(temporaryVoiceBuffer.isFloatingPoint());

			if (Processor::getSampleRate() != -1.0)
			{
				static_cast<ModulatorSamplerVoice*>(getVoice(i))->prepareToPlay(Processor::getSampleRate(), getBlockSize());
			}
		};
	}

	setKillFadeOutTime(int(getAttribute(ModulatorSynth::KillFadeTime)));

	refreshMemoryUsage();
	refreshStreamingBuffers();
}

void ModulatorSampler::killAllVoicesAndCall(const ProcessorFunction& f)
{
	if (!isOnAir())
	{
		f(this);
	}
	else
	{
		getMainController()->getKillStateHandler().killVoicesAndCall(this, f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
	}
}

void ModulatorSampler::setSoundPropertyAsync(ModulatorSamplerSound* s, int index, int newValue)
{
    samplePropertyUpdater.addNewPropertyChange(s, index, newValue, false);
}

void ModulatorSampler::setSoundPropertyAsyncForAllSamples(int index, int newValue)
{
    samplePropertyUpdater.addNewPropertyChange(nullptr, index, newValue, true);
}

void ModulatorSampler::SamplePropertyUpdater::handlePendingChanges()
{
    Array<PropertyChange> thisTime;

	{
		ScopedLock sl(arrayLock);
		thisTime.swapWith(pendingChanges);
	}

	for (auto c : thisTime)
	{
		if (c.allSamples)
		{
			jassert(c.sound == nullptr);

			ModulatorSampler::SoundIterator iter(sampler, false);

			while (auto s = iter.getNextSound())
			{
				s->setProperty(ModulatorSamplerSound::Property(c.index), c.newValue, dontSendNotification);
			}
		}
		else
		{
			if (c.sound != nullptr)
			{
				dynamic_cast<ModulatorSamplerSound*>(c.sound.get())->setProperty(ModulatorSamplerSound::Property(c.index),
				                                                                 c.newValue, dontSendNotification);
			}
		}
	}

	stopTimer();
}

void ModulatorSampler::SamplePropertyUpdater::addNewPropertyChange(ModulatorSamplerSound* sound, int index,
                                                                   int newValue, bool allSamples)
{
	ScopedLock sl(arrayLock);

	for (auto& c : pendingChanges)
	{
		if (c.sound == sound && c.index == index && c.allSamples == allSamples)
		{
			c.newValue = newValue;
			return;
		}
	}

	pendingChanges.add(PropertyChange(sound, index, newValue, allSamples));

	if (!sampler->sampleMapLoadingPending)
	{
		startTimer(200);
	}
}

void ModulatorSampler::AsyncPurger::timerCallback()
{
	triggerAsyncUpdate();
	stopTimer();
}

void ModulatorSampler::AsyncPurger::handleAsyncUpdate()
{
	if (sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->isPreloading())
	{
		startTimer(100);
		return;
	}

	sampler->refreshChannelsForSounds();
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
	killAllVoicesAndCall([newPreloadSize](Processor* p) { static_cast<ModulatorSampler*>(p)->setPreloadSize(newPreloadSize); return true; });
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
	samplerDisplayValues.currentNotes[noteNumber] = 0;
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

ModulatorSamplerSound* ModulatorSampler::addSamplerSound(const ValueTree &description, int index, bool forceReuse/*=false*/)
{
	//ScopedLock sl(getMainController()->getLock());
	checkAndLogIsSoftBypassed(DebugLogger::Location::AddOneSample);

	jassert(sounds.size() == index);

	ModulatorSamplerSoundPool *pool = getMainController()->getSampleManager().getModulatorSamplerSoundPool();
    ModulatorSamplerSound *newSound = pool->addSound(description, index, description.hasProperty("mono_sample_start") || forceReuse);

	if (newSound != nullptr)
	{
		newSound->restoreFromValueTree(description);

		sounds.add(newSound);
		newSound->setUndoManager(getMainController()->getControlUndoManager());
		newSound->addChangeListener(sampleMap);
		newSound->setMaxRRGroupIndex(rrGroupAmount);

		sendChangeMessage();

		
	}

	return newSound;
	
}


void ModulatorSampler::addSamplerSounds(OwnedArray<ModulatorSamplerSound>& monolithicSounds)
{
	//ScopedLock sl(getMainController()->getLock());
	checkAndLogIsSoftBypassed(DebugLogger::Location::AddMultipleSamples);

	jassert(sounds.size() == 0);

	const int numNewSounds = monolithicSounds.size();

	for (int i = 0; i < numNewSounds; i++)
	{
		ModulatorSamplerSound* newSound = monolithicSounds.removeAndReturn(0);

		

		sounds.add(newSound);

		newSound->setPurged(purged);
		newSound->setMaxRRGroupIndex(rrGroupAmount);

		newSound->setUndoManager(getMainController()->getControlUndoManager());
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

	const bool useSampleStartChain = sampleStartChain->getNumChildProcessors() != 0;

	float sampleStartModValue;

	if (useSampleStartChain)
	{
		sampleStartChain->startVoice(voiceIndex);
		sampleStartModValue = sampleStartChain->getConstantVoiceValue(voiceIndex);

		// The display value can already be determined here.
		BACKEND_ONLY(samplerDisplayValues.currentSampleStartPos = jlimit<float>(0.0f, 1.0f, sampleStartModValue));
	}
	else
	{
		// hack: the sample start value is normalized so we need to just flip the sign in order to tell startVoice to use a direct sample value
		sampleStartModValue = -1.0f * static_cast<ModulatorSynthVoice*>(getVoice(voiceIndex))->getCurrentHiseEvent().getStartOffset();
		samplerDisplayValues.currentSampleStartPos = 0.0f;
	}

	crossFadeChain->startVoice(voiceIndex);

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

	const bool preloadBufferIsNonZero = static_cast<ModulatorSamplerSound*>(sound)->preloadBufferIsNonZero();

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
            samplerDisplayValues.currentNotes[m.getNoteNumber() + m.getTransposeAmount()] = 0;
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
	jassert(isOnSampleLoadingThread());

	ScopedLock sl(getMainController()->getSampleManager().getSamplerSoundLock());

	sampleMap->saveIfNeeded();

	deleteAllSounds();
	sampleMap->clear();
}


void ModulatorSampler::loadSampleMapSync(const File &f)
{
	jassert(isOnSampleLoadingThread());

	clearSampleMap();
	getSampleMap()->load(f);
}

void ModulatorSampler::loadSampleMapSync(const ValueTree &valueTreeData)
{
	jassert(isOnSampleLoadingThread());

	clearSampleMap();
	getSampleMap()->restoreFromValueTree(valueTreeData);
}



void ModulatorSampler::loadSampleMapFromIdAsync(const String& sampleMapId)
{
	if (getSampleMap()->getId().toString() == sampleMapId)
		return;

    
	getMainController()->getDebugLogger().logMessage("**Loading samplemap** " + sampleMapId);
    
	sampleMapLoadingPending = true;

	auto f = [sampleMapId](Processor* p)
	{ 
		dynamic_cast<ModulatorSampler*>(p)->loadSampleMapFromId(sampleMapId);
		return true; 
	};

	killAllVoicesAndCall(f);
}

void ModulatorSampler::loadSampleMapFromId(const String& sampleMapId)
{
	jassert(isOnSampleLoadingThread());

	//ScopedLock sl(getMainController()->getLock());

#if USE_BACKEND || DONT_EMBED_FILES_IN_FRONTEND

#if USE_BACKEND
	File f = GET_PROJECT_HANDLER(this).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).getChildFile(sampleMapId + ".xml");
#else
    
#if HISE_IOS
    File f = ProjectHandler::Frontend::getResourcesFolder().getChildFile("SampleMaps/").getChildFile(sampleMapId + ".xml");
#else
    
	File f = ProjectHandler::Frontend::getAppDataDirectory().getChildFile("SampleMaps/").getChildFile(sampleMapId + ".xml");
#endif

	jassert(f.existsAsFile());
#endif

	if (!f.existsAsFile())
	{
		Logger::writeToLog("!Samplemap " + f.getFileName() + " not found.");
		return;
	}

	XmlDocument doc(f);

	ScopedPointer<XmlElement> xml = doc.getDocumentElement();

	if (xml != nullptr)
	{
		ValueTree v = ValueTree::fromXml(*xml);

		static const Identifier unused = Identifier("unused");

		const Identifier oldId = getSampleMap()->getId();
		const Identifier newId = Identifier(v.getProperty("ID", "unused").toString());

		if (newId != unused && newId != oldId)
		{
			loadSampleMapSync(v);
			sendChangeMessage();
			getMainController()->getSampleManager().getModulatorSamplerSoundPool()->sendChangeMessage();
		}

	}
	else
	{
		Logger::writeToLog("!Error when loading sample map: " + doc.getLastParseError());
		return;
	}

#else

	ValueTree v = dynamic_cast<FrontendDataHolder*>(getMainController())->getSampleMap(sampleMapId);

	if (v.isValid())
	{
		static const Identifier unused = Identifier("unused");
		const Identifier oldId = getSampleMap()->getId();
		const Identifier newId = Identifier(v.getProperty("ID", "unused").toString());

		if (newId != unused && newId != oldId)
		{
			loadSampleMapSync(v);
		}
	}
	else
	{
		Logger::writeToLog("!Error when loading sample map: " + sampleMapId);
		return;
	}

#endif


	int maxGroup = 1;

	ModulatorSampler::SoundIterator sIter(this);

	while (auto sound = sIter.getNextSound())
	{
		maxGroup = jmax<int>(maxGroup, sound->getProperty(ModulatorSamplerSound::RRGroup));
	}

	setAttribute(ModulatorSampler::RRGroupAmount, (float)maxGroup, sendNotification);

	sampleMapLoadingPending = false;

	samplePropertyUpdater.handlePendingChanges();
}

void ModulatorSampler::saveSampleMap() const
{
	sampleMap->save();
}

void ModulatorSampler::saveSampleMapAs()
{
	auto newName = PresetHandler::getCustomName("SampleMap");
	
	if (newName.isNotEmpty())
	{
		sampleMap->setId(newName);
		sampleMap->save();
	}
	
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

	ModulatorSampler::SoundIterator sIter(this);

	while (auto sound = sIter.getNextSound())
		sound->setMaxRRGroupIndex(rrGroupAmount);
}


bool ModulatorSampler::preloadAllSamples()
{
	const int preloadSizeToUse = (int)getAttribute(ModulatorSampler::PreloadSize) * getPreloadScaleFactor();

	resetNotes();
	setShouldUpdateUI(false);

	debugToConsole(this, "Changing preload size to " + String(preloadSizeToUse) + " samples");

	const bool isReversed = getAttribute(ModulatorSampler::Reversed) > 0.5f;

	ModulatorSampler::SoundIterator sIter(this);

	const int numToLoad = jmax<int>(1, sounds.size() * getNumMicPositions());
	int currentIndex = 0;

	auto& progress = getMainController()->getSampleManager().getPreloadProgress();

	while (auto sound = sIter.getNextSound())
	{
		sound->checkFileReference();

		if (getNumMicPositions() == 1)
		{
			auto s = sound->getReferenceToSound();

			progress = (double)currentIndex++ / (double)numToLoad;

			if (!preloadSample(s, preloadSizeToUse))
				return false;
		}
		else
		{
			for (int j = 0; j < getNumMicPositions(); j++)
			{
				const bool isEnabled = getChannelData(j).enabled;

				auto s = sound->getReferenceToSound(j);

				progress = (double)currentIndex++ / (double)numToLoad;

				if (s != nullptr)
				{
					if (isEnabled)
					{
						if (!preloadSample(s, preloadSizeToUse))
							return false;
					}
					else
						s->setPurged(true);
				}
			}
		}

		sound->setReversed(isReversed);
	}

	refreshMemoryUsage();
	setShouldUpdateUI(true);
	setHasPendingSampleLoad(false);
	sendChangeMessage();

	return true;
}


bool ModulatorSampler::preloadSample(StreamingSamplerSound * s, const int preloadSizeToUse)
{
	jassert(s != nullptr);

	String fileName = s->getFileName(false);

	try
	{
		s->setPreloadSize(s->hasActiveState() ? preloadSizeToUse : 0, true);
		s->closeFileHandle();
		return true;
	}
	catch (StreamingSamplerSound::LoadingError l)
	{
		String x;
		x << "Error at preloading sample " << l.fileName << ": " << l.errorDescription;
		getMainController()->getDebugLogger().logMessage(x);

#if USE_FRONTEND
		getMainController()->sendOverlayMessage(DeactiveOverlay::State::CustomErrorMessage, x);
#else
		debugError(this, x);
#endif

		return false;
	}
}

} // namespace hise
