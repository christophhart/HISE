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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef MODULATORSAMPLER_H_INCLUDED
#define MODULATORSAMPLER_H_INCLUDED



 


class SampleEditHandler;

/** The main sampler class.
*	@ingroup sampler
*
*	A ModulatorSampler is a synthesiser which allows playback of samples.
*
*	Features:
*
*	- Disk Streaming with fast MemoryMappedFile reading
*	- Looping with crossfades & sample start modulation
*	- Round-Robin groups
*	- Resampling (using linear interpolation for now, but can be extended to a complexer algorithm)
*	- Application-wide sample pool with reference counting to ensure minimal memory usage.
*	- Different playback modes (pitch tracking / one shot, etc.)
*
*	Current limitations:
*
*	- supported file format is stereo wave.
*/
class ModulatorSampler: public ModulatorSynth,
						public ExternalFileProcessor,
						public LookupTableProcessor
{
public:

	SET_PROCESSOR_NAME("StreamingSampler", "Sampler")

	/** Special Parameters for the ModulatorSampler. */
	enum Parameters
	{
		PreloadSize = ModulatorSynth::numModulatorSynthParameters, ///< -1 ... **11000** ... | The preload size in samples for all samples that are loaded into the sampler. If the preload size is `-1`, then the whole sample will be loaded into memory.
		BufferSize, ///< 0 ... **4096** ... | The buffer size of the streaming buffers (2 per voice) in samples. The sampler uses two buffers which are swapped (one is used for reading from disk and one is used to supply the sampler with the audio data)
		VoiceAmount, ///< 0 ... **64** | The amount of voices that the sampler can play. This is not the same as voice limit.
		RRGroupAmount, ///< **0** ... x | The number of groups that are cycled in a round robin manier.
		SamplerRepeatMode, ///< **Kill Note**, Note off, Do nothing | determines how the sampler treats repeated notes.
		PitchTracking, ///< **On**, Off | Enables pitch ratio modification for different notes than the root note. Disable this for drum samples.
		OneShot, ///< On, **Off** | plays the whole sample (ignores the note off) if set to enabled.
		CrossfadeGroups, ///< On, **Off** | if enabled, the groups are played simultanously and can be crossfaded with the X-Fade Modulation Chain
		Purged, ///< If this is true, all samples of this sampler won't be loaded into memory. Turning this on will load them.
		Reversed, ///< If this is true, the samples will be fully loaded into preload buffer and reversed
		numModulatorSamplerParameters
	};

	/** Different behaviour for retriggered notes. */
	enum RepeatMode
	{
		KillNote = 0, ///< kills the note (using the supplied fade time)
		NoteOff, ///< triggers a note off event before starting the note
		DoNothing ///< do nothin (a new voice is started and the old keeps ringing).
	};

	/** Additional modulator chains. */
	enum InternalChains
	{
		SampleStartModulation = ModulatorSynth::numInternalChains, ///< allows modification of the sample start if the sound allows this.
		CrossFadeModulation,
		numInternalChains
	};

	enum EditorStates
	{
		SampleStartChainShown = ModulatorSynth::numEditorStates,
		SettingsShown,
		WaveformShown,
		MapPanelShown,
		TableShown,
		MidiSelectActive,
		CrossfadeTableShown,
		BigSampleMap,
		numEditorStates
	};


	/** Creates a new ModulatorSampler. */
	ModulatorSampler(MainController *mc, const String &id, int numVoices);;
	~ModulatorSampler();

	SET_PROCESSOR_CONNECTOR_TYPE_ID("ModulatorSampler");

	void restoreFromValueTree(const ValueTree &v) override;
	ValueTree exportAsValueTree() const override;
	
	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	int getNumMicPositions() const { return numChannels; }

	const ModulatorSamplerSound *getSound(int soundIndex) const;
	ModulatorSamplerSound *getSound(int soundIndex);

	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;
	int getNumChildProcessors() const override { return numInternalChains;	};
	int getNumInternalChains() const override {return numInternalChains; };

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	/** returns the ModulatorSamplerSound::Property for the given index. */
	var getPropertyForSound(int soundIndex, ModulatorSamplerSound::Property p);

	/** returns the thumbnailCache for the sampler. */
	AudioThumbnailCache &getCache() const noexcept { return *soundCache; };
	void loadCacheFromFile(File &f);;

	/** This resets the streaming buffer size of the voices. Call this whenever you change the voice amount. */
	void refreshStreamingBuffers();

	/** Deletes the sound from the sampler.
	*
	*	It removes the sound from the sampler and if no reference is left in the global sample pool deletes the sample and frees the storage.
	*/
	void deleteSound(ModulatorSamplerSound *s);

	/** Deletes all sounds. Call this instead of clearSounds(). */
	void deleteAllSounds();

	/** Refreshes the preload sizes for all samples.
	*
	*	This is the actual loading process, so it is put into a seperate thread with a progress window. */
	void refreshPreloadSizes();

	/** Returns the time spent reading samples from disk. */
	double getDiskUsage();

	/** Scans all sounds and voices and adds their memory usage. */
	void refreshMemoryUsage();

	int getNumActiveVoices() const override
	{
		if (purged) return 0;

		int currentVoiceAmount = ModulatorSynth::getNumActiveVoices();

        int activeChannels = 0;
        
        for(int i = 0; i < numChannels; i++)
        {
            if(channelData[i].enabled)
                activeChannels++;
        }
        
		return currentVoiceAmount * activeChannels;
	}

	/** Allows dynamically changing the voice amount.
	*
	*	This is a ModulatorSampler specific function, because all other synths can have the full voice amount with almost no overhead,
	*	but since every ModulatorSamplerVoice has two streaming buffers, it could add up wasting unnecessary memory.
	*/
	void setVoiceAmount(int newVoiceAmount);

	

    /** Sets the streaming buffer and preload buffer sizes asynchronously. */
    void setPreloadSizeAsync(int newPreloadSize);
    
	/** this sets the current playing position that will be displayed in the editor. */
	void setCurrentPlayingPosition(double normalizedPosition);

	void setCrossfadeTableValue(float newValue);

	void resetNoteDisplay(int noteNumber);
	void resetNotes();

	/** Adds a sound to the sampler.
	*
	*	
	*/
	void addSamplerSound(const ValueTree &description, int index, bool forceReuse=false);

	void addSamplerSounds(OwnedArray<ModulatorSamplerSound>& monolithicSounds);

	void renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi) override
	{
		if (purged)
		{
			return;
		}

		ModulatorSynth::renderNextBlockWithModulators(outputAudio, inputMidi);


	}

	SampleThreadPool *getBackgroundThreadPool();
	String getMemoryUsage() const;;

	bool shouldUpdateUI() const noexcept{ return !deactivateUIUpdate; };
	void setShouldUpdateUI(bool shouldUpdate) noexcept{ deactivateUIUpdate = !shouldUpdate; };

	void preStartVoice(int voiceIndex, int noteNumber) override;
	void preVoiceRendering(int startSample, int numThisTime) override;
	void soundsChanged() {};
	bool soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity) override;;
	void handleRetriggeredNote(ModulatorSynthVoice *voice) override;

	/** Overwrites the base class method and ignores the note off event if Parameters::OneShot is enabled. */
	void noteOff(const HiseEvent &m) override;;
	void preHiseEventCallback(const HiseEvent &m) override;

	bool isUsingCrossfadeGroups() const { return crossfadeGroups; }
	Table *getTable(int tableIndex) const override { return tableIndex < crossfadeTables.size() ? crossfadeTables[tableIndex] : nullptr; }

	void calculateCrossfadeModulationValuesForVoice(int voiceIndex, int startSample, int numSamples, int groupIndex);
	const float *getCrossfadeModValues(int voiceIndex) const {	return crossFadeChain->getVoiceValues(voiceIndex);	}

	UndoManager *getUndoManager() {	return getMainController()->getControlUndoManager();	};

	SampleMap *getSampleMap() {	return sampleMap; };
	void clearSampleMap();
	
	void loadSampleMap(const File &f);
	void loadSampleMap(const ValueTree &valueTreeData);
	void loadSampleMapFromIdAsync(const String& sampleMapId);
	void loadSampleMapFromId(const String& sampleMapId);

	void saveSampleMap() const;

	void saveSampleMapAs();

	void saveSampleMapAsMonolith (Component* mainEditor) const;

	/** Disables the automatic cycling and allows custom setting of the used round robin group. */
	void setUseRoundRobinLogic(bool shouldUseRoundRobinLogic) noexcept { useRoundRobinCycleLogic = shouldUseRoundRobinLogic; };
	/** Sets the current index to the group. */
	bool setCurrentGroupIndex(int currentIndex);
	bool isRoundRobinEnabled() const noexcept { return useRoundRobinCycleLogic; };
	void setRRGroupAmount(int newGroupLimit);

	bool isPitchTrackingEnabled() const {return pitchTrackingEnabled; };
	bool isOneShot() const {return oneShotEnabled; };

	CriticalSection &getSamplerLock() {	return lock; }
	
	const CriticalSection& getExportLock() const { return exportLock; }

#if USE_BACKEND
	SampleEditHandler* getSampleEditHandler() { return sampleEditHandler; }
	const SampleEditHandler* getSampleEditHandler() const { return sampleEditHandler; }
#endif

	bool useGlobalFolderForSaving() const;
	void setUseGlobalFolderForSaving() { useGlobalFolder = true; };
	void replaceReferencesWithGlobalFolder() override;

	struct SamplerDisplayValues : public Processor::DisplayValues
	{
		SamplerDisplayValues() : currentSamplePos(0.0)
		{
            memset(currentNotes, 0, 128);
		};

		double currentSamplePos;
		double currentSampleStartPos;
		float crossfadeTableValue;
		int currentGroup;

		uint8 currentNotes[128];
	};

	const SamplerDisplayValues &getSamplerDisplayValues() const { return samplerDisplayValues;	}

	SamplerDisplayValues& getSamplerDisplayValues() { return samplerDisplayValues; }

	int getRRGroupsForMessage(int noteNumber, int velocity);
	void refreshRRMap();

	void setReversed(bool shouldBeReversed)
	{
		if (reversed != shouldBeReversed)
		{
			reversed = shouldBeReversed;

			for (int i = 0; i < getNumSounds(); i++)
			{
				getSound(i)->setReversed(reversed);
			}
		}

		refreshMemoryUsage();
	}

	void purgeAllSamples(bool shouldBePurged)
	{

		if (shouldBePurged != purged)
		{
            purged = shouldBePurged;

            if(purged)
            {
                getMainController()->getDebugLogger().logMessage("**Purging samples** from " + getId());
            }
            else
            {
                getMainController()->getDebugLogger().logMessage("**Unpurging samples** from " + getId());
            }
            
			for (int i = 0; i < sounds.size(); i++)
			{
				ModulatorSamplerSound *sound = static_cast<ModulatorSamplerSound*>(getSound(i));

				sound->setPurged(shouldBePurged);
			}

			refreshPreloadSizes();
			refreshMemoryUsage();
			
		}
	}

	void setNumChannels(int numChannels);



	struct ChannelData: RestorableObject
	{
		ChannelData() :
			enabled(true),
			level(1.0f),
			suffix()
		{};

		void restoreFromValueTree(const ValueTree &v) override
		{
			enabled = v.getProperty("enabled");
			level = Decibels::decibelsToGain((float)v.getProperty("level"));
			suffix = v.getProperty("suffix");
		}

		ValueTree exportAsValueTree() const override
		{
			ValueTree v("channelData");

			v.setProperty("enabled", enabled, nullptr);
			v.setProperty("level", (float)Decibels::gainToDecibels(level), nullptr);
			v.setProperty("suffix", suffix, nullptr);
			
			return v;
		}

		bool enabled;
		float level;
		String suffix;
	};

	const ChannelData &getChannelData(int index) const
	{
		if (index >= 0 && index < getNumMicPositions())
		{
			return channelData[index];
		}
		else
		{
			jassertfalse;
			return channelData[0];
		}
		
	}

	void setMicEnabled(int channelIndex, bool channelIsEnabled) noexcept
	{
		if (channelIndex >= NUM_MIC_POSITIONS || channelIndex < 0) return;

		channelData[channelIndex].enabled = channelIsEnabled;

		asyncPurger.triggerAsyncUpdate(); // will call refreshChannelsForSound asynchronously
	}

	void refreshChannelsForSounds()
	{
		for (int i = 0; i < sounds.size(); i++)
		{
			ModulatorSamplerSound *sound = static_cast<ModulatorSamplerSound*>(sounds[i].get());

			for (int j = 0; j < sound->getNumMultiMicSamples(); j++)
			{
				const bool enabled = channelData[j].enabled;

				sound->setChannelPurged(j, !enabled);
			}
		}

		refreshPreloadSizes();
		refreshMemoryUsage();
	}
	
	void setPreloadMultiplier(int newPreloadScaleFactor)
	{
		if (newPreloadScaleFactor != preloadScaleFactor)
		{
			preloadScaleFactor = jmax<int>(1, newPreloadScaleFactor);

			if(getNumSounds() != 0) refreshPreloadSizes();
			refreshStreamingBuffers();
			refreshMemoryUsage();
		}
	}

	int getPreloadScaleFactor() const
	{
		return preloadScaleFactor;
	}

	void setNumMicPositions(StringArray &micPositions);
	
	String getStringForMicPositions() const
	{
		String saveString;

		for (int i = 0; i < getNumMicPositions(); i++)
		{
			saveString << channelData[i].suffix << ";";
		}

		return saveString;
	}

	hlac::HiseSampleBuffer* getTemporaryVoiceBuffer() { return &temporaryVoiceBuffer; }

private:

	struct AsyncPurger : public AsyncUpdater,
						 public Timer
	{
	public:

		AsyncPurger(ModulatorSampler *sampler_) :
			sampler(sampler_)
		{};

		void timerCallback()
		{
			triggerAsyncUpdate();
			stopTimer();
		}

		void handleAsyncUpdate()
		{
			if (sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->isPreloading())
			{
				startTimer(100);
				return;
			}

			sampler->refreshChannelsForSounds();
		}

	private:

		ModulatorSampler *sampler;
	};

    struct AsyncPreloader: public AsyncUpdater,
						   public Timer
    {
        AsyncPreloader(ModulatorSampler *sampler_):
        sampler(sampler_),
        preloadSize(-1)
        {};
        
		void timerCallback()
		{
			triggerAsyncUpdate();
			stopTimer();
		}

        void handleAsyncUpdate()
        {
			if (sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->isPreloading())
			{
				startTimer(100);
				return;
			}

            sampler->setPreloadSize(preloadSize);
        }
        
        void setPreloadSize(int newPreloadSize)
        {
            preloadSize = newPreloadSize;
            triggerAsyncUpdate();
        }
        
        int preloadSize;
        
        ModulatorSampler *sampler;
    };
    
	struct AsyncSampleMapLoader : public AsyncUpdater,
								  public Timer
	{
		AsyncSampleMapLoader(ModulatorSampler* s) :
			sampler(s)
		{};

		void timerCallback()
		{
			triggerAsyncUpdate();
			stopTimer();
		}

		void handleAsyncUpdate()
		{
			if (sampler->getMainController()->getSampleManager().getModulatorSamplerSoundPool()->isPreloading())
			{
				startTimer(100);
				return;
			}

			sampler->loadSampleMapFromId(sampleMapId);
		}

		void loadSampleMap(const String& newSampleMapId)
		{
			sampleMapId = newSampleMapId;
			startTimer(50);
		}

		String sampleMapId;

		ModulatorSampler *sampler;
	};

    /** Sets the streaming buffer and preload buffer sizes. */
    void setPreloadSize(int newPreloadSize);
    
	CriticalSection exportLock;

    AsyncPreloader asyncPreloader;
	AsyncPurger asyncPurger;
	AsyncSampleMapLoader asyncSampleMapLoader;

	void refreshCrossfadeTables();

	RoundRobinMap roundRobinMap;

	bool reversed = false;

	bool useGlobalFolder;
	bool pitchTrackingEnabled;
	bool oneShotEnabled;
	bool crossfadeGroups;
	bool purged;
	bool deactivateUIUpdate;
	int rrGroupAmount;
	int currentRRGroupIndex;
	bool useRoundRobinCycleLogic;
	RepeatMode repeatMode;
	int voiceAmount;
	int preloadScaleFactor;

	mutable SamplerDisplayValues samplerDisplayValues;

	File loadedMap;
	File workingDirectory;

	int preloadSize;
	int bufferSize;

	int64 memoryUsage;

	OwnedArray<SampleLookupTable> crossfadeTables;

	AudioSampleBuffer crossfadeBuffer;

	hlac::HiseSampleBuffer temporaryVoiceBuffer;

	float groupGainValues[8];

	ChannelData channelData[NUM_MIC_POSITIONS];
	int numChannels;

	ScopedPointer<SampleMap> sampleMap;
	ScopedPointer<ModulatorChain> sampleStartChain;
	ScopedPointer<ModulatorChain> crossFadeChain;
	ScopedPointer<AudioThumbnailCache> soundCache;
	
#if USE_BACKEND
	ScopedPointer<SampleEditHandler> sampleEditHandler;
#endif

};



#endif  // MODULATORSAMPLER_H_INCLUDED
