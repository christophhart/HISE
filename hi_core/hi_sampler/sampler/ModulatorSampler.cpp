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
		"If *Enabled*, it will unload all preload buffers and deactivate the sample playback to save memory. The **Lazy load** option unloads all preload buffers and delays the preloading of a sample until it is triggered for the first time.");

	ADD_PARAMETER_DOC(Reversed, 
		"If this is true, the samples will be fully loaded into preload buffer and reversed");

    ADD_PARAMETER_DOC(UseStaticMatrix,
        "If this is true, then the routing matrix will not be resized when you load a sample map with another mic position amount.");

	ADD_CHAIN_DOC(SampleStartModulation, "Sample Start", 
		"Allows modification of the sample start if the sound allows this. The modulation range is depending on the *SampleStartMod* value of each sample.");

	ADD_CHAIN_DOC(CrossFadeModulation, "Group Fade",
		"Fades between the RR groups. This can be used for crossfading dynamics samples.");
};


ModulatorSampler::ModulatorSampler(MainController *mc, const String &id, int numVoices) :
ModulatorSynth(mc, id, numVoices),
LookupTableProcessor(mc, 8),
preloadSize(PRELOAD_SIZE),
asyncPurger(this),
sampleMap(new SampleMap(this)),
rrGroupAmount(1),
bufferSize(4096),
preloadScaleFactor(1),
useRoundRobinCycleLogic(true),
pitchTrackingEnabled(true),
oneShotEnabled(false),
crossfadeGroups(false),
crossfadeBuffer(1, 0),
purged(false),
reversed(false),
numChannels(1),
repeatMode(RepeatMode::KillSecondOldestNote),
deactivateUIUpdate(false),
samplePreloadPending(false),
realVoiceAmount(numVoices),
temporaryVoiceBuffer(DEFAULT_BUFFER_TYPE_IS_FLOAT, 2, 0),
syncVoiceHandler(false)
{
#if USE_BACKEND || HI_ENABLE_EXPANSION_EDITING
	sampleEditHandler = new SampleEditHandler(this);
#endif

	modChains += {this, "Sample Start", ModulatorChain::ModulationType::VoiceStartOnly, Modulation::GainMode};
	modChains += {this, "Group Fade"};

	finaliseModChains();

	modChains[Chains::XFade].setAllowModificationOfVoiceValues(true);
	
	sampleStartChain = modChains[Chains::SampleStart].getChain();
	crossFadeChain = modChains[Chains::XFade].getChain();

	setGain(1.0);

	OLD_PROCESSOR_DISPATCH(enablePooledUpdate(mc->getGlobalUIUpdater()));

	//enableAllocationFreeMessages(50);

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
    parameterNames.add("UseStaticMatrix");
	parameterNames.add("LowPassEnvelopeOrder");
	parameterNames.add("Timestretching");

	updateParameterSlots();

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

	sampleStartChain->setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff5e8127)));
	crossFadeChain->setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff884b29)));

	for (int i = 0; i < 127; i++) 
		samplerDisplayValues.currentNotes[i] = 0;

	setVoiceAmount(numVoices);


	for (int i = 0; i < 8; i++)
		getTable(i)->setYTextConverterRaw(Modulation::getValueAsDecibel);

	getMatrix().setAllowResizing(true);

	PrepareSpecs ps;
	ps.voiceIndex = &syncVoiceHandler;
	syncer.state.prepare(ps);
}


ModulatorSampler::~ModulatorSampler()
{
	soundCollector = nullptr;
	sampleMap = nullptr;
	abortIteration = true;
	deleteAllSounds();
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

			SimpleReadWriteLock::ScopedReadLock sl(s->getIteratorLock());
            ModulatorSampler::SoundIterator sIter(s);

            while (auto sound = sIter.getNextSound())
            {
                sound->setReversed(shouldBeReversed);
            }

            s->refreshMemoryUsage();

            return SafeFunctionCall::OK;
        };

        killAllVoicesAndCall(f, true);
    }
}

void ModulatorSampler::updatePurgeFromAttribute(int roundedValue)
{
	if (roundedValue == 2)
	{
		purgeAllSamples(false, false);
		setPlayFromPurge(true, true);
	}
	else
	{
		auto shouldBePurged = roundedValue == 1;

		// if lazy loading was active, we need to
		// force the value to be different so that
		// purgeAllSamples will actually do something...
		if (enablePlayFromPurge)
			purged = !shouldBePurged;

		setPlayFromPurge(false, false);
		purgeAllSamples(shouldBePurged, true);
	}
}

void ModulatorSampler::purgeAllSamples(bool shouldBePurged, bool changePreloadSize)
{
	if (shouldBePurged != purged)
	{
		if (shouldBePurged)
			getMainController()->getDebugLogger().logMessage("**Purging samples** from " + getId());
		else
			getMainController()->getDebugLogger().logMessage("**Unpurging samples** from " + getId());

		if (changePreloadSize)
		{
			auto f = [shouldBePurged](Processor* p)
			{
				auto s = static_cast<ModulatorSampler*>(p);

				jassert(s->allVoicesAreKilled());

				s->purged = shouldBePurged;

				for (int i = 0; i < s->sounds.size(); i++)
				{
					ModulatorSamplerSound *sound = static_cast<ModulatorSamplerSound*>(s->getSound(i));
					sound->setPurged(shouldBePurged);
				}

				s->refreshPreloadSizes();
				s->refreshMemoryUsage();

				return SafeFunctionCall::OK;
			};

			killAllVoicesAndCall(f, true);
		}
		else
		{
			purged = shouldBePurged;

			SoundIterator iter(this);

			while (auto s = iter.getNextSound())
				s->setPurged(shouldBePurged);
		}
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
	

	voiceAmount = -1;
	setVoiceAmount(prevVoiceAmount);
	ModulatorSynth::setVoiceLimit(realVoiceAmount * getNumActiveGroups());

	if (numChannels < 1) numChannels = 1;
	if (numChannels > NUM_MIC_POSITIONS) numChannels = NUM_MIC_POSITIONS;

	for (int i = 0; i < NUM_MIC_POSITIONS; i++)
	{
		channelData[i].enabled = (channelData[i].enabled && i <= numChannels);
		channelData[i].suffix = "";
		channelData[i].level = channelData[i].enabled ? 1.0f : 0.0f;
	}

}

int ModulatorSampler::getNumActiveGroups() const
{
	if (crossfadeGroups)
		return rrGroupAmount;
	
	return jmax(1, (int)multiRRGroupState.getNumSetBits());
}

void ModulatorSampler::setNumMicPositions(StringArray &micPositions)
{
	if (micPositions.size() == 0) return;
	
	setNumChannels(micPositions.size());
	
	for (int i = 0; i < numChannels; i++)
	{
		channelData[i].suffix = micPositions[i];
	}

	sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);

}

bool ModulatorSampler::checkAndLogIsSoftBypassed(DebugLogger::Location location) const
{
	return const_cast<MainController*>(getMainController())->getDebugLogger().checkIsSoftBypassed(this, location);
}

void ModulatorSampler::refreshReleaseStartFlag()
{
#if HISE_SAMPLER_ALLOW_RELEASE_START
	ModulatorSampler::SoundIterator sIter(this);
	jassert(sIter.canIterate());

	soundsHaveReleaseStart = false;
			
	while (auto sound = sIter.getNextSound())
	{
		auto s = sound->getReferenceToSound();
		soundsHaveReleaseStart |= s->isReleaseStartEnabled();
	}
#endif
}

void ModulatorSampler::refreshCrossfadeTables()
{
	ModulatorSynth::setVoiceLimit(realVoiceAmount * getNumActiveGroups());
}

void ModulatorSampler::restoreFromValueTree(const ValueTree &v)
{
	getMainController()->getSampleManager().setCurrentPreloadMessage("Loading " + getId());

	loadAttribute(PreloadSize, "PreloadSize");
    loadAttribute(UseStaticMatrix, "UseStaticMatrix");
	
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

	auto savedMap = v.getChildWithName("samplemap");

	if (savedMap.isValid())
	{
		loadEmbeddedValueTree(savedMap, true);
	}
	else
	{
		PoolReference ref(getMainController(), v.getProperty("SampleMapID").toString(), FileHandlerBase::SampleMaps);

		if(ref.isValid())
			loadSampleMap(ref);
	}

    loadAttribute(CrossfadeGroups, "CrossfadeGroups");
    loadAttribute(RRGroupAmount, "RRGroupAmount");

	TimestretchOptions newOptions;
	newOptions.restoreFromValueTree(v.getChildWithName(TimestretchOptions::getStaticId()));

	setTimestretchOptions(newOptions);

	for (int i = 0; i < 8; i++)
		loadTable(getTableUnchecked(i), "Group" + String(i) + "Table");

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
    saveAttribute(UseStaticMatrix, "UseStaticMatrix");

	ValueTree channels("channels");

	for (int i = 0; i < numChannels; i++)
	{
		ValueTree channelDataTree = channelData[i].exportAsValueTree();

		channels.addChild(channelDataTree, -1, nullptr);
	}

	v.addChild(channels, -1, nullptr);

	if (currentTimestretchOptions)
		v.addChild(currentTimestretchOptions.exportAsValueTree(), -1, nullptr);

	for (int i = 0; i < 8; i++)
	{
		saveTable(getTableUnchecked(i), "Group" + String(i) + "Table");
	}

	if (sampleMap->isUsingUnsavedValueTree())
	{
		auto id = sampleMap->getId();

		static const Identifier cj("CustomJSON");

		if (id != cj)
		{
			debugError(const_cast<ModulatorSampler*>(this), "Saving embedded samplemaps is bad practice. Save the samplemap to a file instead.");
		}
		
		v.addChild(sampleMap->getValueTree().createCopy(), -1, nullptr);
	}
	else
	{
		if (sampleMap->hasUnsavedChanges())
		{
			debugToConsole(const_cast<ModulatorSampler*>(this), "The sample map has unsaved changes so it will be embedded into the sampler.");
			v.addChild(sampleMap->getValueTree().createCopy(), -1, nullptr);
		}
		else
		{
			v.setProperty("SampleMapID", sampleMap->getReference().getReferenceString(), nullptr);
		}

		
	}

	

	
	

	return v;
}

float ModulatorSampler::getAttribute(int parameterIndex) const
{
	if (parameterIndex == ModulatorSynth::VoiceLimit) return realVoiceAmount;

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
	case Purged:			
		if (enablePlayFromPurge) return 2.0f;
		else return purged ? 1.0f : 0.0f;
	case Reversed:			return reversed ? 1.0f : 0.0f;
    case UseStaticMatrix:   return useStaticMatrix ? 1.0f : 0.0f;
	case LowPassEnvelopeOrder: return (float)lowPassOrder * 6.0f;
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
		killAllVoicesAndCall([](Processor*p) {static_cast<ModulatorSampler*>(p)->refreshStreamingBuffers(); return SafeFunctionCall::OK; }, false);
		break;
	}
	case VoiceAmount:		setVoiceAmount((int)newValue); break;
	case RRGroupAmount:		setRRGroupAmount((int)newValue); refreshCrossfadeTables(); break;
	case SamplerRepeatMode: repeatMode = (RepeatMode)(int)newValue; break;
	case PitchTracking:		pitchTrackingEnabled = newValue > 0.5f; break;
	case OneShot:			oneShotEnabled = newValue > 0.5f; break;
	case Reversed:			setReversed(newValue > 0.5f); break;
	case CrossfadeGroups:	crossfadeGroups = newValue > 0.5f; refreshCrossfadeTables(); break;
	case Purged:			updatePurgeFromAttribute(roundToInt(newValue)); break;
	case UseStaticMatrix:   setUseStaticMatrix(newValue > 0.5f); break;
	case LowPassEnvelopeOrder: 
		lowPassOrder = roundToInt(newValue / 6);
		if (envelopeFilter != nullptr)
			envelopeFilter->setOrder(lowPassOrder);
		break;
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
    auto prevBlockSize = getLargestBlockSize();
    
	ModulatorSynth::prepareToPlay(newSampleRate, samplesPerBlock);

	if (samplesPerBlock > 0 && prevBlockSize != samplesPerBlock)
	{
        refreshMemoryUsage();

		if (envelopeFilter != nullptr)
			setEnableEnvelopeFilter();
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

void ModulatorSampler::loadCacheFromFile(File &f)
{
	FileInputStream fis(f);

	soundCache->clear();
	soundCache->readFromStream(fis);
}

void ModulatorSampler::refreshStreamingBuffers()
{
	jassert_processor_idle;

	for (int i = 0; i < getNumVoices(); i++)
	{
		SynthesiserVoice *v = getVoice(i);
		static_cast<ModulatorSamplerVoice*>(v)->resetVoice();
		static_cast<ModulatorSamplerVoice*>(v)->setLoaderBufferSize(bufferSize * preloadScaleFactor);
		static_cast<ModulatorSamplerVoice*>(v)->setEnablePlayFromPurge(enablePlayFromPurge);
	}
}

void ModulatorSampler::deleteSound(int index)
{
	if (auto s = getSound(index))
	{
		LockHelpers::freeToGo(getMainController());

		for (int i = 0; i < voices.size(); i++)
		{
			if (static_cast<ModulatorSamplerVoice*>(voices[i])->getCurrentlyPlayingSamplerSound() == s)
				static_cast<ModulatorSamplerVoice*>(voices[i])->resetVoice();
		}

		{
			LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::SampleLock);
			removeSound(index);
		}

		if (!delayUpdate)
		{
			refreshMemoryUsage();
			sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
		}
		
	}
}

void ModulatorSampler::deleteAllSounds()
{
	if (getNumSounds() == 0)
		return;

	isOnAir() && LockHelpers::freeToGo(getMainController());

	for (int i = 0; i < voices.size(); i++)
	{
		static_cast<ModulatorSamplerVoice*>(getVoice(i))->resetVoice();
	}



	{
		LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::SampleLock);

		// The lifetime could exceed this function, so we need to flag it as pending for delete
		// so that async tasks will not use this later.
		for (int i = 0; i < getNumSounds(); i++)
			static_cast<ModulatorSamplerSound*>(getSound(i))->setDeletePending();

		if (getNumSounds() != 0)
		{
			clearSounds();

			if(getSampleMap() != nullptr)
				getSampleMap()->getCurrentSamplePool()->clearUnreferencedMonoliths();
		}

		envelopeFilter = nullptr;
	}
	
	refreshMemoryUsage();
	sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom, dispatch::sendNotificationAsync);
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
		auto f = [](Processor* p)
		{
			if (static_cast<ModulatorSampler*>(p)->preloadAllSamples())
				return SafeFunctionCall::OK;

			else
				return SafeFunctionCall::cancelled;
		};

		killAllVoicesAndCall(f, true);
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

void ModulatorSampler::refreshMemoryUsage(bool fastMode)
{
	if (sampleMap == nullptr)
		return;
    
    if(getLargestBlockSize() <= 0)
        return;

	if (!fastMode)
	{
		const auto temporaryBufferIsFloatingPoint = getTemporaryVoiceBuffer()->isFloatingPoint();

#if HISE_IOS
		const auto temporaryBufferShouldBeFloatingPoint = false;
#else
		const auto temporaryBufferShouldBeFloatingPoint = !sampleMap->isMonolith();
#endif

		if (temporaryBufferIsFloatingPoint != temporaryBufferShouldBeFloatingPoint || temporaryVoiceBuffer.getNumSamples() == 0)
		{
			temporaryVoiceBuffer = hlac::HiseSampleBuffer(temporaryBufferShouldBeFloatingPoint, 2, 0);

			for (auto i = 0; i < getNumVoices(); i++)
				static_cast<ModulatorSamplerVoice*>(getVoice(i))->setStreamingBufferDataType(temporaryBufferShouldBeFloatingPoint);
		}

		StreamingSamplerVoice::initTemporaryVoiceBuffer(&temporaryVoiceBuffer, getLargestBlockSize(), (double)MAX_SAMPLER_PITCH);

		PrepareSpecs ps;
		ps.blockSize = getLargestBlockSize() * MAX_SAMPLER_PITCH;
		ps.numChannels = 2;

		DspHelpers::increaseBuffer(stretchBuffer, ps);
	}

	int64 actualPreloadSize = 0;

	{
		SoundIterator sIter(this, false);

        double maxPitch = (double)MAX_SAMPLER_PITCH;
        
		while (const auto sound = sIter.getNextSound())
		{
			for (int j = 0; j < numChannels; j++)
			{
				if (auto micS = sound->getReferenceToSound(j))
				{
					actualPreloadSize += micS->getActualPreloadSize();
                    maxPitch = jmax(sound->getMaxPitchRatio(), maxPitch);
				}
			}
		}
        
        if(!fastMode && maxPitch > (double)MAX_SAMPLER_PITCH)
        {
            StreamingSamplerVoice::initTemporaryVoiceBuffer(&temporaryVoiceBuffer, getLargestBlockSize(), maxPitch * 1.2); // give it a little more to be safe...
        }
	}

	const int64 streamBufferSizePerVoice = 2 *				// two buffers
		bufferSize *		// buffer size per buffer
		(sampleMap->isMonolith() ? 2 : 4) *  // bytes per sample
		2 * numChannels;				// number of channels

	memoryUsage = actualPreloadSize + streamBufferSizePerVoice * getNumVoices();

	sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom, dispatch::sendNotificationAsync);
	getSampleMap()->getCurrentSamplePool()->sendChangeMessage();
}

void ModulatorSampler::setVoiceAmount(int newVoiceAmount)
{
	if (isInGroup())
	{
		// Don't allow the sampler to have another voice amount than it's parent group.
		newVoiceAmount = getGroup()->getNumVoices();
	}

	if (newVoiceAmount != voiceAmount)
	{
		voiceAmount = jmin<int>(NUM_POLYPHONIC_VOICES, newVoiceAmount);

		
		if (getAttribute(ModulatorSynth::VoiceLimit) > voiceAmount)
			setAttribute(ModulatorSynth::VoiceLimit, float(voiceAmount), sendNotificationAsync);

		auto f = [](Processor*p) { static_cast<ModulatorSampler*>(p)->setVoiceAmountInternal(); return SafeFunctionCall::OK; };
		killAllVoicesAndCall(f, false);
		
	}
}

void ModulatorSampler::setVoiceAmountInternal()
{
	if(isOnAir())
		LockHelpers::freeToGo(getMainController());

	{
		deleteAllVoices();

		for (int i = 0; i < voiceAmount; i++)
		{
			if (numChannels != 1)
				addVoice(new MultiMicModulatorSamplerVoice(this, numChannels));
			else
				addVoice(new ModulatorSamplerVoice(this));

			dynamic_cast<ModulatorSamplerVoice*>(voices.getLast())->setStreamingBufferDataType(temporaryVoiceBuffer.isFloatingPoint());

			if (Processor::getSampleRate() != -1.0)
			{
				static_cast<ModulatorSamplerVoice*>(getVoice(i))->prepareToPlay(Processor::getSampleRate(), getLargestBlockSize());
			}

			static_cast<ModulatorSamplerVoice*>(getVoice(i))->setTimestretchOptions(currentTimestretchOptions);
		};
	}

	setKillFadeOutTime(int(getAttribute(ModulatorSynth::KillFadeTime)));

	refreshMemoryUsage();
	refreshStreamingBuffers();
}

bool ModulatorSampler::killAllVoicesAndCall(const ProcessorFunction& f, bool restrictToSampleLoadingThread/*=true*/)
{
	auto currentThread = getMainController()->getKillStateHandler().getCurrentThread();

	bool correctThread = (currentThread == MainController::KillStateHandler::TargetThread::SampleLoadingThread) ||
						 (!restrictToSampleLoadingThread && currentThread == MainController::KillStateHandler::TargetThread::ScriptingThread);

	bool hasSampleLock = LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::Type::SampleLock);

	if ((hasSampleLock || !isOnAir()) && correctThread)
	{
		f(this);
		return true;
	}
	else
	{
		getMainController()->getKillStateHandler().killVoicesAndCall(this, f, MainController::KillStateHandler::TargetThread::SampleLoadingThread);
		return false;
	}
}

void ModulatorSampler::setDisplayedGroup(int index, bool shouldBeVisible, ModifierKeys mods, NotificationType notifyListener)
{
#if USE_BACKEND
	auto& s = getSamplerDisplayValues().visibleGroups;
	
	if (index == -1 || !mods.isAnyModifierKeyDown())
		s.clear();
	
	if (index >= 0)
	{
		if (mods.isShiftDown())
		{
			auto startBit = s.getHighestBit();
			auto numToSet = index - startBit + 1;

			if (numToSet > 0)
				s.setRange(startBit, numToSet, true);
		}
		else
		{
			s.setBit(index, shouldBeVisible);
		}
	}

	getSampleEditHandler()->groupBroadcaster.sendMessage(notifyListener, getCurrentRRGroup(), &getSamplerDisplayValues().visibleGroups);
#endif
}

void ModulatorSampler::setSortByGroup(bool shouldSortByGroup)
{
	if (shouldSortByGroup != (soundCollector != nullptr))
	{
		LockHelpers::SafeLock sl(getMainController(), LockHelpers::Type::AudioLock);

		if (shouldSortByGroup)
			soundCollector = new GroupedRoundRobinCollector(this);
		else
			soundCollector = nullptr;
	}
}

bool ModulatorSampler::hasPendingAsyncJobs() const
{
	return getMainController()->getSampleManager().hasPendingFunction(const_cast<ModulatorSampler*>(this));
}

bool ModulatorSampler::callAsyncIfJobsPending(const ProcessorFunction& f)
{
	if (hasPendingAsyncJobs())
		return killAllVoicesAndCall(f);
	
	f(this);
	return true;
}

void ModulatorSampler::setEnableEnvelopeFilter()
{
	envelopeFilter = new CascadedEnvelopeLowPass(true);

	if (getSampleRate() > 0)
	{
		PrepareSpecs ps;
		ps.blockSize = getLargestBlockSize();
		ps.sampleRate = getSampleRate();
		ps.numChannels = 2;
		envelopeFilter->prepare(ps);
	}
}

void ModulatorSampler::setPlayFromPurge(bool shouldPlayFromPurge, bool refreshPreload)
{
	if (enablePlayFromPurge != shouldPlayFromPurge)
	{
		enablePlayFromPurge = shouldPlayFromPurge;

		if (refreshPreload)
		{
			auto pf = [](Processor* p)
			{
				auto s = static_cast<ModulatorSampler*>(p);

				// if there are some sounds loaded already, purge them all
				if (s->getNumSounds() > 0)
				{
					s->refreshPreloadSizes();
				}

				s->refreshStreamingBuffers();
				s->refreshMemoryUsage();

				return SafeFunctionCall::OK;
			};

			killAllVoicesAndCall(pf);
		}
	}
}

void ModulatorSampler::setCurrentTimestretchMode(TimestretchOptions::TimestretchMode newMode)
{
	if(currentTimestretchOptions.mode != newMode)
	{
		auto options = currentTimestretchOptions;
		options.mode = newMode;

		setTimestretchOptions(options);
	}
}

void ModulatorSampler::setTimestretchOptions(const TimestretchOptions& newOptions)
{
	currentTimestretchOptions = newOptions;

	auto f = [](Processor* p)
	{
		auto s = static_cast<ModulatorSampler*>(p);

		const auto& options = s->currentTimestretchOptions;

		auto enableSync = options.mode == TimestretchOptions::TimestretchMode::TempoSynced;

		s->syncer.setEnabled(enableSync);
		s->syncVoiceHandler.setEnabled(enableSync);

		if (enableSync)
			s->getMainController()->addTempoListener(&s->syncer);
		else
			s->getMainController()->removeTempoListener(&s->syncer);
		
		for (auto v : s->voices)
		{
			dynamic_cast<ModulatorSamplerVoice*>(v)->setTimestretchOptions(options);
		}

		return SafeFunctionCall::OK;
	};

	killAllVoicesAndCall(f, true);
}

double ModulatorSampler::getCurrentTimestretchRatio() const
{
	if (currentTimestretchOptions.mode == TimestretchOptions::TimestretchMode::Disabled)
		return 1.0;

	return syncer.getRatio(ratioToUse);
}

void ModulatorSampler::setTimestretchRatio(double newRatio)
{
	ratioToUse = jlimit(0.0625, 2.0, newRatio);
}

void ModulatorSampler::AsyncPurger::timerCallback()
{
	triggerAsyncUpdate();
	stopTimer();
}

void ModulatorSampler::AsyncPurger::handleAsyncUpdate()
{
	if (sampler->getSampleMap()->getCurrentSamplePool()->isPreloading())
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
	killAllVoicesAndCall([newPreloadSize](Processor* p) { static_cast<ModulatorSampler*>(p)->setPreloadSize(newPreloadSize); return SafeFunctionCall::OK; });
}

void ModulatorSampler::setCurrentPlayingPosition(double normalizedPosition)
{
	samplerDisplayValues.currentSamplePos = normalizedPosition;
}

void ModulatorSampler::setCrossfadeTableValue(float newValue)
{
	samplerDisplayValues.crossfadeTableValue = newValue;
}

void ModulatorSampler::resetNoteDisplay(int noteNumber)
{
	lastStartedVoice = nullptr;
	samplerDisplayValues.currentNotes[jlimit(0, 127, noteNumber)] = 0;
	samplerDisplayValues.currentSamplePos = -1.0;
	sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom, dispatch::sendNotificationAsync);
}

void ModulatorSampler::resetNotes()
{
	for (int i = 0; i < voices.size(); i++)
	{
		static_cast<ModulatorSamplerVoice*>(voices[i])->resetVoice();
	}
}

void ModulatorSampler::renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi)
{
	if (purged)
	{
		return;
	}

	if(currentTimestretchOptions.mode == TimestretchOptions::TimestretchMode::TimeVariant)
	{
		auto r = getCurrentTimestretchRatio();

		for(auto av: activeVoices)
		{
			static_cast<ModulatorSamplerVoice*>(av)->setTimestretchRatio(r);
		}
	}

	ModulatorSynth::renderNextBlockWithModulators(outputAudio, inputMidi);

	if(!eventIdsForGroupIndexes.isEmpty())
	{
		// Copy over the last state from the queue (this makes it effectively the same as calling
		// the function with an eventId of -1).
		multiRRGroupState = eventIdsForGroupIndexes[eventIdsForGroupIndexes.size()-1].second;
		eventIdsForGroupIndexes.clearQuick();
	}
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

void ModulatorSampler::preStartVoice(int voiceIndex, const HiseEvent& e)
{
	ModulatorSynth::preStartVoice(voiceIndex, e);

	const bool useSampleStartChain = sampleStartChain->shouldBeProcessedAtAll();

	float sampleStartModValue;

	sampleStartModValue = modChains[Chains::SampleStart].getConstantModulationValue();

	if (useSampleStartChain)
	{
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

	auto lv = static_cast<ModulatorSamplerVoice*>(getLastStartedVoice());

	lv->setSampleStartModValue(sampleStartModValue);

	


	if(currentTimestretchOptions.mode != TimestretchOptions::TimestretchMode::Disabled)
	{
		auto v = static_cast<ModulatorSamplerVoice*>(voices[voiceIndex]);

		if(currentTimestretchOptions.mode == TimestretchOptions::TimestretchMode::TempoSynced)
		{
			PolyHandler::ScopedVoiceSetter svs(syncVoiceHandler, voiceIndex);

			if(auto nextSound = dynamic_cast<ModulatorSamplerSound*>(soundsToBeStarted[0]))
			{
                auto nq = nextSound->getNumQuartersForTimestretch(currentTimestretchOptions.numQuarters);
                
				syncer.setSource(nextSound->getSampleRate(), nextSound->getReferenceToSound(0)->getSampleLength(), nq);
			}
				
			v->setTimestretchRatio(getCurrentTimestretchRatio());
		}
		else
		{
			v->setTimestretchRatio(getCurrentTimestretchRatio());
		}
	}
}

bool ModulatorSampler::soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity)
{
	const bool messageFits = ModulatorSynth::soundCanBePlayed(sound, midiChannel, midiNoteNumber, velocity);

	if (!messageFits) return false;
	
	
	auto soundGroup = static_cast<ModulatorSamplerSound*>(sound)->getRRGroup();

	const bool rrGroupApplies = (!multiRRGroupState && (crossfadeGroups || multiRRGroupState.getSingleGroupIndex() == soundGroup)) ||
								multiRRGroupState[soundGroup];

	if (!rrGroupApplies) return false;

	const bool preloadBufferIsNonZero = shouldPlayFromPurge() || static_cast<ModulatorSamplerSound*>(sound)->preloadBufferIsNonZero();

	if (!preloadBufferIsNonZero) return false;

	return true;
}

void ModulatorSampler::handleRetriggeredNote(ModulatorSynthVoice *voice)
{
	jassert(getMainController()->getSampleManager().isNonRealtime() || getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::TargetThread::AudioThread ||
	LockHelpers::isLockedBySameThread(getMainController(), LockHelpers::Type::AudioLock));

	switch (repeatMode)
	{
	case RepeatMode::DoNothing:		return;
	case RepeatMode::KillNote:		voice->killVoice(); break;
	case RepeatMode::NoteOff:		voice->stopNote(1.0f, true); break;
	case RepeatMode::KillSecondOldestNote:
	{
		int noteNumber = voice->getCurrentlyPlayingNote();
		auto uptime = voice->getVoiceUptime();

		for (auto v : activeVoices)
		{
			auto thisNumber = v->getCurrentlyPlayingNote();
			auto thisUptime = v->getVoiceUptime();

			if (noteNumber == thisNumber && thisUptime < uptime)
			{
				v->killVoice();
			}
		}
		break;
	}
    default: jassertfalse; break;
	}
}

void ModulatorSampler::noteOff(const HiseEvent &m)
{
	if (!oneShotEnabled)
	{
#if HISE_SAMPLER_ALLOW_RELEASE_START
		if(soundsHaveReleaseStart)
		{
			for (auto v : activeVoices)
			{
				if(v->getCurrentHiseEvent().getEventId() == m.getEventId())
				{
					auto s = static_cast<ModulatorSamplerSound*>(v->getCurrentlyPlayingSound().get());

					if(s->getReferenceToSound()->isReleaseStartEnabled())
					{
						static_cast<ModulatorSamplerVoice*>(v)->jumpToRelease();
					}
				}
			}
		}
#endif

		ModulatorSynth::noteOff(m);
	}
}

void ModulatorSampler::preHiseEventCallback(HiseEvent &m)
{
	if (m.isNoteOnOrOff())
	{
		if (m.isNoteOn())
		{
			if (useRoundRobinCycleLogic)
			{
				multiRRGroupState.bumpRoundRobin(rrGroupAmount);
			}
			else if (!eventIdsForGroupIndexes.isEmpty())
			{
				for(const auto& pending: eventIdsForGroupIndexes)
				{
					if(pending.first == m.getEventId())
					{
						memcpy(&multiRRGroupState, &pending.second, sizeof(MultiGroupState));
						break;
					}
				}
			}

#if USE_BACKEND

			getSampleEditHandler()->noteBroadcaster.sendMessage(sendNotificationAsync, m.getNoteNumber(), m.getVelocity());

			if (lockRRGroup != -1)
				multiRRGroupState.setSingleGroupIndex(lockRRGroup);

			if (lockVelocity > 0)
				m.setVelocity(lockVelocity);

			auto rrIndex = multiRRGroupState.getSingleGroupIndex();

			jassert(rrIndex == getCurrentRRGroup());

			if(isDisplayGroupFollowingRRGroup())
			{
				getSamplerDisplayValues().visibleGroups.clear();
				getSamplerDisplayValues().visibleGroups.setBit(rrIndex-1);
			}
				

			getSampleEditHandler()->groupBroadcaster.sendMessage(sendNotificationAsync, rrIndex, &getSamplerDisplayValues().visibleGroups);
#endif
		
			samplerDisplayValues.currentGroup = multiRRGroupState.getSingleGroupIndex();
		}

		if (m.isNoteOn())
		{
			samplerDisplayValues.currentNotes[m.getNoteNumber() + m.getTransposeAmount()] = m.getVelocity();
		}
		else
		{
#if USE_BACKEND
			getSampleEditHandler()->noteBroadcaster.sendMessage(sendNotificationAsync, m.getNoteNumber(), 0);
#endif

            samplerDisplayValues.currentNotes[m.getNoteNumber() + m.getTransposeAmount()] = 0;
		}
		
        sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom, dispatch::sendNotificationAsync);
	}

	if (!m.isNoteOff() || !oneShotEnabled)
	{
		ModulatorSynth::preHiseEventCallback(m);
	}
}

float* ModulatorSampler::calculateCrossfadeModulationValuesForVoice(int voiceIndex, int startSample, int numSamples, int groupIndex)
{
	// If we have set multiple groups to be active manually
	// we want to use only as much tables as there are active groups...
	if (multiRRGroupState)
		groupIndex %= multiRRGroupState.getNumSetBits();

	if (groupIndex > 8) return nullptr;

	if (auto compressedValues = modChains[Chains::XFade].getWritePointerForManualExpansion(startSample))
	{
		int numSamples_cr = numSamples / HISE_CONTROL_RATE_DOWNSAMPLING_FACTOR;

#if HISE_ENABLE_CROSSFADE_MODULATION_THRESHOLD
        auto firstValue = compressedValues[0];
        auto lastValue = compressedValues[numSamples_cr - 1];
        
		if (fabsf(firstValue - lastValue) < 0.0001f) // -80dB
		{
			// We need to manually convert the value from the table
			// and send it to the mod chain to update the ramp value.
			float modValue = firstValue;
			currentCrossfadeValue = getCrossfadeValue(groupIndex, modValue);
			modChains[Chains::XFade].setCurrentRampValueForVoice(voiceIndex, currentCrossfadeValue);
			return nullptr;
		}
#endif
		
        while (--numSamples_cr >= 0)
        {
            float value = *compressedValues;
            *compressedValues++ = getCrossfadeValue(groupIndex, value);
        }

        modChains[Chains::XFade].expandVoiceValuesToAudioRate(voiceIndex, startSample, numSamples);

        auto return_ptr = modChains[Chains::XFade].getWritePointerForVoiceValues(0);

        // It might be possible that the expansion results in a "constantification", so check again...
        if (return_ptr != nullptr)
        {
            currentCrossfadeValue = 1.0f;
            return return_ptr;
        }
        else
        {
            // Just grab the last mod value, it's already converted using the tables.
            currentCrossfadeValue = modChains[Chains::XFade].getConstantModulationValue();
            return nullptr;
        }
	}
	else
	{
		float modValue = modChains[Chains::XFade].getConstantModulationValue();
		currentCrossfadeValue = getCrossfadeValue(groupIndex, modValue);
		modChains[Chains::XFade].setCurrentRampValueForVoice(voiceIndex, currentCrossfadeValue);
		return nullptr;
	}
}

const float * ModulatorSampler::getCrossfadeModValues() const
{
	return crossfadeGroups ? modChains[Chains::XFade].getReadPointerForVoiceValues(0) : nullptr;
}

juce::ValueTree ModulatorSampler::parseMetadata(const File& sampleFile)
{
	AudioFormatManager *afm = &(getMainController()->getSampleManager().getModulatorSamplerSoundPool2()->afm);

	ScopedPointer<AudioFormatReader> reader = afm->createReaderFor(sampleFile);

	if (reader != nullptr)
	{
		auto v = getSamplePropertyTreeFromMetadata(reader->metadataValues);
		auto fileName = PoolReference(getMainController(), sampleFile.getFullPathName(), FileHandlerBase::Samples).getReferenceString();
		v.setProperty(SampleIds::FileName, fileName, nullptr);
		return v;
	}

	return {};

}

#define SET_PROPERTY_FROM_METADATA_STRING(string, prop) if (string.isNotEmpty()) sample.setProperty(prop, string.getIntValue(), nullptr);

juce::ValueTree ModulatorSampler::getSamplePropertyTreeFromMetadata(const StringPairArray& metadata)
{
	ValueTree sample("Metadata");

	const String format = metadata.getValue("MetaDataSource", "");
	String lowVel, hiVel, loKey, hiKey, root, start, end, loopEnabled, loopStart, loopEnd;

	if (format == "AIFF")
	{
		lowVel = metadata.getValue("LowVelocity", "");
		hiVel = metadata.getValue("HighVelocity", "");
		loKey = metadata.getValue("LowNote", "");
		hiKey = metadata.getValue("HighNote", "");
		root = metadata.getValue("MidiUnityNote", "");

		loopEnabled = metadata.getValue("Loop0Type", "");

		const int loopStartId = metadata.getValue("Loop0StartIdentifier", "-1").getIntValue();
		const int loopEndId = metadata.getValue("Loop0EndIdentifier", "-1").getIntValue();

		int loopStartIndex = -1;
		int loopEndIndex = -1;

		const int numCuePoints = metadata.getValue("NumCuePoints", "0").getIntValue();

		for (int i = 0; i < numCuePoints; i++)
		{
			const String idTag = "CueLabel" + String(i) + "Identifier";

			if (metadata.getValue(idTag, "-2").getIntValue() == loopStartId)
			{
				loopStartIndex = i;
				loopStart = metadata.getValue("Cue" + String(i) + "Offset", "");
			}
			else if (metadata.getValue(idTag, "-2").getIntValue() == loopEndId)
			{
				loopEndIndex = i;
				loopEnd = metadata.getValue("Cue" + String(i) + "Offset", "");
			}
		}
	}
	else if (format == "WAV")
	{
		loopStart = metadata.getValue("Loop0Start", "");
		loopEnd = metadata.getValue("Loop0End", "");
		loopEnabled = (loopStart.isNotEmpty() && loopStart != "0" && loopEnd.isNotEmpty() && loopEnd != "0") ? "1" : "";
	}

	SET_PROPERTY_FROM_METADATA_STRING(lowVel, SampleIds::LoVel);
	SET_PROPERTY_FROM_METADATA_STRING(hiVel, SampleIds::HiVel);
	SET_PROPERTY_FROM_METADATA_STRING(loKey, SampleIds::LoKey);
	SET_PROPERTY_FROM_METADATA_STRING(hiKey, SampleIds::HiKey);
	SET_PROPERTY_FROM_METADATA_STRING(root, SampleIds::Root);
	SET_PROPERTY_FROM_METADATA_STRING(start, SampleIds::SampleStart);
	SET_PROPERTY_FROM_METADATA_STRING(end, SampleIds::SampleEnd);
	SET_PROPERTY_FROM_METADATA_STRING(loopEnabled, SampleIds::LoopEnabled);
	SET_PROPERTY_FROM_METADATA_STRING(loopStart, SampleIds::LoopStart);
	SET_PROPERTY_FROM_METADATA_STRING(loopEnd, SampleIds::LoopEnd);

	return sample;
}

#undef SET_PROPERTY_FROM_METADATA_STRING

void ModulatorSampler::setVoiceLimit(int newVoiceLimit)
{
	realVoiceAmount = jmax(2, newVoiceLimit);

	ModulatorSynth::setVoiceLimit(realVoiceAmount * getNumActiveGroups());
}

float ModulatorSampler::getConstantCrossFadeModulationValue() const noexcept
{
	// Return the rr group volume if it is set
	if (!crossfadeGroups)
		return useRRGain ? rrGroupGains[multiRRGroupState.getSingleGroupIndex() -1] : 1.0f;

#if HISE_PLAY_ALL_CROSSFADE_GROUPS_WHEN_EMPTY

	// This plays all crossfade groups until there's a modulator present.
	if (!modChains[Chains::XFade].getChain()->shouldBeProcessedAtAll())
	{
		return 1.0f;
	}
#endif

	return currentCrossfadeValue;
}

float ModulatorSampler::getCrossfadeValue(int groupIndex, float modValue) const
{
	if (auto st = static_cast<const SampleLookupTable*>(getTableUnchecked(groupIndex)))
	{
		modValue = CONSTRAIN_TO_0_1(modValue);
		return st->getInterpolatedValue((double)modValue, sendNotificationAsync);
	}
	
	return 0.0f;
}

void ModulatorSampler::clearSampleMap(NotificationType n)
{
	LockHelpers::freeToGo(getMainController());

	ScopedValueSetter<bool> ia(abortIteration, true);
	SimpleReadWriteLock::ScopedWriteLock sl(getIteratorLock());

	if (sampleMap == nullptr)
		return;

	deleteAllSounds();
	sampleMap->clear(n);
}


void ModulatorSampler::reloadSampleMap()
{
	auto ref = getSampleMap()->getReference();

	if (!ref.isValid())
		return;

	auto f = [ref](Processor* p)
	{
		auto s = static_cast<ModulatorSampler*>(p);
		s->clearSampleMap(dontSendNotification);
		s->loadSampleMap(ref);
		return SafeFunctionCall::OK;
	};

	killAllVoicesAndCall(f, true);
}

void ModulatorSampler::loadSampleMap(PoolReference ref)
{
	if (getSampleMap()->getReference() == ref)
		return;

	LockHelpers::freeToGo(getMainController());

	ScopedValueSetter<bool> ia(abortIteration, true);
	SimpleReadWriteLock::ScopedWriteLock sl(getIteratorLock());

	getSampleMap()->load(ref);
}

void ModulatorSampler::loadEmbeddedValueTree(const ValueTree& v, bool /*loadAsynchronous*/ /*= false*/)
{
	debugError(this, "Loading embedded samplemaps is bad practice. Save the samplemap to a file instead.");

	auto f = [v](Processor* p)
	{
		dynamic_cast<ModulatorSampler*>(p)->getSampleMap()->loadUnsavedValueTree(v);
		return SafeFunctionCall::OK;
	};

	killAllVoicesAndCall(f, false);
}

void ModulatorSampler::updateRRGroupAmountAfterMapLoad()
{
	int maxGroup = 1;

	
	ModulatorSampler::SoundIterator sIter(this);
	jassert(sIter.canIterate());

	while (auto sound = sIter.getNextSound())
	{
		maxGroup = jmax<int>(maxGroup, sound->getSampleProperty(SampleIds::RRGroup));
	}

	setAttribute(ModulatorSampler::RRGroupAmount, (float)maxGroup, sendNotificationSync);

}

void ModulatorSampler::nonRealtimeModeChanged(bool isNonRealtime)
{
	for (auto v : voices)
	{
		dynamic_cast<ModulatorSamplerVoice*>(v)->setNonRealtime(isNonRealtime);
	}
}

bool ModulatorSampler::saveSampleMap() const
{
	return sampleMap->save();
}

bool ModulatorSampler::saveSampleMapAsReference() const
{
	return sampleMap->saveSampleMapAsReference();
}

bool ModulatorSampler::saveSampleMapAsMonolith(Component* mainEditor) const
{
	return sampleMap->saveAsMonolith(mainEditor);
}

bool ModulatorSampler::setCurrentGroupIndex(int currentIndex, int eventId)
{
	if (currentIndex <= rrGroupAmount)
	{
		if(eventId != -1)
		{
			MultiGroupState newState;
			newState.setSingleGroupIndex(currentIndex);
			eventIdsForGroupIndexes.insertWithoutSearch({ (uint16)eventId, newState});
		}
		else
			multiRRGroupState.setSingleGroupIndex(currentIndex);

		return true;
	}
	else
	{
		return false;
	}
}

void ModulatorSampler::setRRGroupVolume(int groupIndex, float gainValue)
{
	if (groupIndex == -1)
		groupIndex = multiRRGroupState.getSingleGroupIndex();

	FloatSanitizers::sanitizeFloatNumber(gainValue);

	--groupIndex;

	useRRGain = true;

	if (isPositiveAndBelow(groupIndex, rrGroupGains.size()))
		rrGroupGains.setUnchecked(groupIndex, gainValue);
}

bool ModulatorSampler::setMultiGroupState(int groupIndex, bool shouldBeEnabled, int eventId)
{
	auto& state = multiRRGroupState;

	if(eventId != -1)
	{
		eventIdsForGroupIndexes.insertWithoutSearch({(uint16)eventId, MultiGroupState()});
		state = (eventIdsForGroupIndexes.begin() + (eventIdsForGroupIndexes.size()-1))->second;
	}

	if (groupIndex == -1)
	{
		state.setAll(shouldBeEnabled);
		return true;
	}
	else
	{
		state.set(groupIndex, shouldBeEnabled);
		return (groupIndex - 1) < rrGroupAmount;
	}
}

bool ModulatorSampler::setMultiGroupState(const int* data128, int numSet, int eventId)
{
	auto& state = multiRRGroupState;

	if(eventId != -1)
	{
		eventIdsForGroupIndexes.insertWithoutSearch({(uint16)eventId, MultiGroupState()});
		state = (eventIdsForGroupIndexes.begin() + (eventIdsForGroupIndexes.size()-1))->second;
	}

	state.copyFromIntArray(data128, 128, numSet);
	return true;
}

void ModulatorSampler::setRRGroupAmount(int newGroupLimit)
{
	rrGroupAmount = jmax(1, newGroupLimit);

	allNotesOff(1, true);

	ModulatorSampler::SoundIterator sIter(this);
	jassert(sIter.canIterate());

	while (auto sound = sIter.getNextSound())
		sound->setMaxRRGroupIndex(rrGroupAmount);

	rrGroupGains.ensureStorageAllocated(rrGroupAmount);

	for (int i = rrGroupGains.size(); i < rrGroupAmount; i++)
		rrGroupGains.add(1.0f);

	// reset this, so it doesn't use a lookup as long as setRRGroupVolume isn't called
	useRRGain = false;

	ModulatorSynth::setVoiceLimit(realVoiceAmount * getNumActiveGroups());

#if USE_BACKEND
	getSampleEditHandler()->groupBroadcaster.sendMessage(sendNotificationAsync, getCurrentRRGroup(), &getSamplerDisplayValues().visibleGroups);
#endif
}


bool ModulatorSampler::isNoteNumberMapped(int noteNumber) const
{
	ModulatorSampler::SoundIterator sIter(this);
	jassert(sIter.canIterate());

	while (auto sound = sIter.getNextSound())
	{
		if (sound->appliesToNote(noteNumber))
			return true;
	}

	return false;
}

int ModulatorSampler::getMidiInputLockValue(const Identifier& id) const
{
	if (id == SampleIds::RRGroup)
		return lockRRGroup;
	if (id == SampleIds::LoVel || id == SampleIds::HiVel)
		return lockVelocity;
    
    return 0;
}

void ModulatorSampler::toggleMidiInputLock(const Identifier& id, int lockValue)
{
	if (id == SampleIds::RRGroup)
	{
		if (lockRRGroup == -1)
			lockRRGroup = lockValue;
		else
			lockRRGroup = -1;
	}
		
	if (id == SampleIds::LoVel || id == SampleIds::HiVel)
	{
		if (lockVelocity == -1)
			lockVelocity = lockValue;
		else
			lockVelocity = -1;
	}
}

bool ModulatorSampler::preloadAllSamples()
{
	int preloadSizeToUse = (int)getAttribute(ModulatorSampler::PreloadSize) * getPreloadScaleFactor();

	if (shouldPlayFromPurge())
		preloadSizeToUse = 0;

	resetNotes();
	setShouldUpdateUI(false);

	debugToConsole(this, "Changing preload size to " + String(preloadSizeToUse) + " samples");

	const bool isReversed = getAttribute(ModulatorSampler::Reversed) > 0.5f;

	ModulatorSampler::SoundIterator sIter(this);
	jassert(sIter.canIterate());

	const int numToLoad = jmax<int>(1, sounds.size() * getNumMicPositions());
	int currentIndex = 0;

	auto& progress = getMainController()->getSampleManager().getPreloadProgress();

	auto threadPool = getMainController()->getSampleManager().getGlobalSampleThreadPool();

	while (auto sound = sIter.getNextSound())
	{
		if (threadPool->threadShouldExit())
			return false;

		sound->checkFileReference();

		if (getNumMicPositions() == 1)
		{
			auto s = sound->getReferenceToSound().get();

			progress = (double)currentIndex++ / (double)numToLoad;

			if (!preloadSample(s, preloadSizeToUse))
				return false;
		}
		else
		{
			for (int j = 0; j < getNumMicPositions(); j++)
			{
				const bool isEnabled = getChannelData(j).enabled;

				progress = (double)currentIndex++ / (double)numToLoad;

				if (auto s = sound->getReferenceToSound(j))
				{
					if (isEnabled)
					{
						if (!preloadSample(s.get(), preloadSizeToUse))
							return false;
					}
					else
						s->setPurged(true);
				}
			}
		}

		sound->setReversed(isReversed);
	}

	refreshReleaseStartFlag();
	refreshMemoryUsage();
	setShouldUpdateUI(true);
	setHasPendingSampleLoad(false);
	sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);

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

ModulatorSampler::ScopedUpdateDelayer::ScopedUpdateDelayer(ModulatorSampler* s) :
	sampler(s),
	prevValue(s->delayUpdate)
{
	sampler->delayUpdate = true;
}

ModulatorSampler::ScopedUpdateDelayer::~ScopedUpdateDelayer()
{
	sampler->delayUpdate = prevValue;

	if (!prevValue)
	{
		sampler->refreshMemoryUsage();
		sampler->sendOtherChangeMessage(dispatch::library::ProcessorChangeEvent::Custom);
		sampler->getSampleMap()->sendSampleMapChangeMessage(sendNotificationAsync);
	}
}

ModulatorSampler::GroupedRoundRobinCollector::GroupedRoundRobinCollector(ModulatorSampler* s):
	sampler(s),
	ready(false)
{
	sampler->getSampleMap()->addListener(this);
	triggerAsyncUpdate();
}

ModulatorSampler::GroupedRoundRobinCollector::~GroupedRoundRobinCollector()
{
	if (sampler != nullptr)
		sampler->getSampleMap()->removeListener(this);
}

void ModulatorSampler::GroupedRoundRobinCollector::collectSounds(const HiseEvent& m, UnorderedStack<ModulatorSynthSound *>& soundsAboutToBeStarted)
{
	SimpleReadWriteLock::ScopedReadLock sl(rebuildLock);

	if (!ready)
		jassertfalse;

	auto currentGroup = sampler->getCurrentRRGroup() - 1;

	if (isPositiveAndBelow(currentGroup, groups.size()))
	{
		auto& refArray = groups.getReference(currentGroup);

		for (auto s : refArray)
		{
			if (sampler->soundCanBePlayed(s, m.getChannel(), m.getNoteNumber(), m.getFloatVelocity()))
				soundsAboutToBeStarted.insertWithoutSearch(s);
		}
	}
}

void ModulatorSampler::GroupedRoundRobinCollector::handleAsyncUpdate()
{
	Array<ReferenceCountedArray<ModulatorSynthSound>> newList;

	auto numRRGroups = (int)sampler->getAttribute(ModulatorSampler::RRGroupAmount);

	if (numRRGroups > 0)
	{
		int numToStore = sampler->getNumSounds() / numRRGroups;

		newList.ensureStorageAllocated(numRRGroups);

		for (int i = 0; i < numRRGroups; i++)
		{
			ReferenceCountedArray<ModulatorSynthSound> newArray;
			newArray.ensureStorageAllocated(numToStore);
			newList.add(newArray);
		}

		ModulatorSampler::SoundIterator it(sampler);
		jassert(it.canIterate());

		while (auto s = it.getNextSound())
		{
			auto rrIndex = (int)s->getSampleProperty(SampleIds::RRGroup) - 1;

			if (isPositiveAndBelow(rrIndex, newList.size()))
			{
				newList.getReference(rrIndex).add(static_cast<ModulatorSynthSound*>(s));
			}
		}
	}

	SimpleReadWriteLock::ScopedWriteLock sl(rebuildLock);
	std::swap(groups, newList);
	ready.store(true);
}

} // namespace hise
