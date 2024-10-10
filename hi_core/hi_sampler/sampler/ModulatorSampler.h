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

#ifndef MODULATORSAMPLER_H_INCLUDED
#define MODULATORSAMPLER_H_INCLUDED

namespace hise { using namespace juce;
using namespace scriptnode;



class SampleEditHandler;

/** The main sampler class.
*	@ingroup sampler
*
*	Features:
*
*	- Disk Streaming with fast MemoryMappedFile reading
*	- Looping with crossfades & sample start modulation
*	- Round-Robin groups
*	- Resampling (using linear interpolation for now, but can be extended to a complexer algorithm)
*	- Application-wide sample pool with reference counting to ensure minimal memory usage.
*	- Different playback modes (pitch tracking / one shot, etc.)
*/
class ModulatorSampler: public ModulatorSynth,
						public LookupTableProcessor,
						public SuspendableTimer::Manager,
						public NonRealtimeProcessor
{
public:



	/** If you add or delete multiple samples at once (but not the entire sample set), it will
	    fire an UI update for each sample drastically slowing down the UI responsiveness.
		
		In this case, just create a ScopedUpdateDelayer object and it will cancel all UI updates
		until it goes out of scope (and in this case, it will fire a update regardless if it's
		necessary or not.
	*/
	class ScopedUpdateDelayer
	{
	public:

		ScopedUpdateDelayer(ModulatorSampler* s);;

		~ScopedUpdateDelayer();


	private:

		WeakReference<ModulatorSampler> sampler;
		bool prevValue;
	};

	class GroupedRoundRobinCollector : public ModulatorSynth::SoundCollectorBase,
									   public SampleMap::Listener,
									   public AsyncUpdater
	{
	public:

		GroupedRoundRobinCollector(ModulatorSampler* s);

		~GroupedRoundRobinCollector();

		void collectSounds(const HiseEvent& m, UnorderedStack<ModulatorSynthSound *>& soundsToBeStarted) override;

		void sampleMapWasChanged(PoolReference newSampleMap)
		{
			triggerAsyncUpdate();
		}

		void samplePropertyWasChanged(ModulatorSamplerSound* , const Identifier& sampleId, const var& )
		{
			if(sampleId == SampleIds::RRGroup)
				triggerAsyncUpdate();
		};

		virtual void sampleAmountChanged() 
		{
			triggerAsyncUpdate();
		};

		virtual void sampleMapCleared()
		{
			triggerAsyncUpdate();
		};

	private:

		SimpleReadWriteLock rebuildLock;

		WeakReference<ModulatorSampler> sampler;

		void handleAsyncUpdate() override;

		std::atomic<bool> ready;

		Array<ReferenceCountedArray<ModulatorSynthSound>> groups;
	};

	/** A small helper tool that iterates over the sound array in a thread-safe way.
	*
	*/
	class SoundIterator
	{
	public:

		using SharedPointer = WeakReference<ModulatorSamplerSound>;

		/** This iterates over all sounds and locks the sound lock if desired. */
		SoundIterator(const ModulatorSampler* s_, bool /*lock_*/=true):
			s(const_cast<ModulatorSampler*>(s_)),
			lock(s->getIteratorLock())
		{
		}

		SharedPointer getNextSound()
		{
			if (!lock)
				return nullptr;

			while (auto sound = getSoundInternal())
				return sound;

			return nullptr;
		}

		~SoundIterator()
		{
		}

		int size() const
		{
			return s->getNumSounds();
		}

		void reset()
		{
			index = 0;
		}

		bool canIterate() const
		{
			return lock;
		}

	private:

		SharedPointer getSoundInternal()
		{
			if (index >= s->getNumSounds())
				return nullptr;

			if (s->shouldAbortIteration())
			{
				lock.unlock();
				return nullptr;
			}

			return dynamic_cast<ModulatorSamplerSound*>(s->getSound(index++));
		}

		

		int index = 0;
		WeakReference<ModulatorSampler> s;
		SimpleReadWriteLock::ScopedTryReadLock lock;
	};

	void suspendStateChanged(bool shouldBeSuspended) override
	{
		getSampleMap()->suspendInternalTimers(shouldBeSuspended);
	}

	SET_PROCESSOR_NAME("StreamingSampler", "Sampler", "The main sampler class of HISE.");

	/** Special Parameters for the ModulatorSampler. */
	enum Parameters
	{
		PreloadSize = ModulatorSynth::numModulatorSynthParameters, 
		BufferSize, 
		VoiceAmount, 
		RRGroupAmount, 
		SamplerRepeatMode, 
		PitchTracking,
		OneShot,
		CrossfadeGroups, 
		Purged, 
		Reversed,
        UseStaticMatrix,
		LowPassEnvelopeOrder,
		numModulatorSamplerParameters
	};

	/** Different behaviour for retriggered notes. */
	enum RepeatMode
	{
		KillNote = 0, ///< kills the note (using the supplied fade time)
		NoteOff, ///< triggers a note off event before starting the note
		DoNothing, ///< do nothin (a new voice is started and the old keeps ringing).
		KillSecondOldestNote, // allow one note to retrigger, but then kill the notes
		KillThirdOldestNote
	};

	enum Chains
	{
		SampleStart = 2,
		XFade
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

	struct TimestretchOptions: public RestorableObject
	{
		static Identifier getStaticId() { RETURN_STATIC_IDENTIFIER("TimestretchOptions"); }

		enum class TimestretchMode
		{
			Disabled,	 ///< no timestretching
			VoiceStart,	 ///< currently active voices will keep their ratio and new voices will use the current ratio
			TimeVariant, ///< all currently active voices will use the same ratio
			TempoSynced,  ///< calculate the stretch ratio based on the length of the sample and the current tempo,
			numTimestretchModes
		};

		TimestretchMode mode = TimestretchMode::Disabled;
		double tonality = 0.0;
		bool synchronousSkip = false;
		double numQuarters = 0.0;
		Identifier engineId;

		void reset()
		{
			mode = TimestretchMode::Disabled;
			tonality = 0.0;
			synchronousSkip = false;
			numQuarters = 0.0;
			engineId = {};
		}

		var toJSON() const
		{
			static const StringArray modes = { "Disabled", "VoiceStart", "TimeVariant", "TempoSynced" };

			const DynamicObject::Ptr obj = new DynamicObject();
			obj->setProperty("Tonality", tonality);
			obj->setProperty("SkipLatency", synchronousSkip);
			obj->setProperty("Mode", modes[static_cast<int>(mode)]);
			obj->setProperty("NumQuarters", numQuarters);
			obj->setProperty("PreferredEngine", engineId.toString());

			return {obj.get()};
		}

		void fromJSON(const var& json)
		{
			static const StringArray modes = { "Disabled", "VoiceStart", "TimeVariant", "TempoSynced" };

			tonality = jlimit(0.0, 1.0, static_cast<double>(json.getProperty("Tonality", 0.0)));
			synchronousSkip = json.getProperty("SkipLatency", false);
			mode = static_cast<TimestretchMode>(modes.indexOf(json.getProperty("Mode", "Disabled").toString()));
			numQuarters = json.getProperty("NumQuarters", 0.0);

			auto id = json.getProperty("PreferredEngine", "").toString();

			if (id.isEmpty())
				engineId = {};
			else
				engineId = Identifier(id);
		}

		void restoreFromValueTree(const ValueTree& v) override
		{
			if(v.getType() == getStaticId())
			{
				auto obj = valuetree::Helpers::valueTreeToJSON(v);
				fromJSON(obj);
			}
			else
			{
				reset();
			}
		}

		ValueTree exportAsValueTree() const override
		{
			return valuetree::Helpers::jsonToValueTree(toJSON(), getStaticId(), false);
		}

		operator bool() const { return mode != TimestretchMode::Disabled; }
	};

	ADD_DOCUMENTATION_WITH_BASECLASS(ModulatorSynth);

	/** Creates a new ModulatorSampler. */
	ModulatorSampler(MainController *mc, const String &id, int numVoices);;
	~ModulatorSampler() override;

	SET_PROCESSOR_CONNECTOR_TYPE_ID("StreamingSampler");

	void restoreFromValueTree(const ValueTree &v) override;
	ValueTree exportAsValueTree() const override;
	
	float getAttribute(int parameterIndex) const override;;
	void setInternalAttribute(int parameterIndex, float newValue) override;;

	int getNumMicPositions() const { return numChannels; }

	Processor *getChildProcessor(int processorIndex) override;;
	const Processor *getChildProcessor(int processorIndex) const override;;
	int getNumChildProcessors() const override { return numInternalChains;	};
	int getNumInternalChains() const override {return numInternalChains; };

	void prepareToPlay(double sampleRate, int samplesPerBlock) override;;

	ProcessorEditorBody* createEditor(ProcessorEditor *parentEditor) override;

	void loadCacheFromFile(File &f);;

	/** This resets the streaming buffer size of the voices. Call this whenever you change the voice amount. */
	void refreshStreamingBuffers();

	/** Deletes the sound from the sampler.
	*
	*	It removes the sound from the sampler and if no reference is left in the global sample pool deletes the sample and frees the storage.
	*/
	void deleteSound(int index);

	/** Deletes all sounds. Call this instead of clearSounds(). */
	void deleteAllSounds();

	/** Refreshes the preload sizes for all samples.
	*
	*	This is the actual loading process, so it is put into a seperate thread with a progress window. */
	void refreshPreloadSizes();

	/** Returns the time spent reading samples from disk. */
	double getDiskUsage();

	/** Scans all sounds and voices and adds their memory usage. */
	void refreshMemoryUsage(bool fastMode=false);

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

	void setVoiceAmountInternal();
	

    /** Sets the streaming buffer and preload buffer sizes asynchronously. */
    void setPreloadSizeAsync(int newPreloadSize);
    
	/** this sets the current playing position that will be displayed in the editor. */
	void setCurrentPlayingPosition(double normalizedPosition);

	void setCrossfadeTableValue(float newValue);

	void resetNoteDisplay(int noteNumber);
	void resetNotes();

	void renderNextBlockWithModulators(AudioSampleBuffer& outputAudio, const HiseEventBuffer& inputMidi) override;

	SampleThreadPool *getBackgroundThreadPool();
	String getMemoryUsage() const;;

	bool shouldUpdateUI() const noexcept{ return !deactivateUIUpdate; };
	void setShouldUpdateUI(bool shouldUpdate) noexcept{ deactivateUIUpdate = !shouldUpdate; };

	void preStartVoice(int voiceIndex, const HiseEvent& e) override;
	void soundsChanged() {};
	bool soundCanBePlayed(ModulatorSynthSound *sound, int midiChannel, int midiNoteNumber, float velocity) override;;
	void handleRetriggeredNote(ModulatorSynthVoice *voice) override;

	/** Overwrites the base class method and ignores the note off event if Parameters::OneShot is enabled. */
	void noteOff(const HiseEvent &m) override;;
	void preHiseEventCallback(HiseEvent &m) override;

	bool isUsingCrossfadeGroups() const { return crossfadeGroups; }
	float* calculateCrossfadeModulationValuesForVoice(int voiceIndex, int startSample, int numSamples, int groupIndex);
	const float *getCrossfadeModValues() const;

	ValueTree parseMetadata(const File& sampleFile);

	static ValueTree getSamplePropertyTreeFromMetadata(const StringPairArray& metadata);

	void setVoiceLimit(int newVoiceLimit) override;

	float getConstantCrossFadeModulationValue() const noexcept;

	float getCrossfadeValue(int groupIndex, float inputValue) const;

	UndoManager *getUndoManager() {	return getMainController()->getControlUndoManager();	};

	SampleMap *getSampleMap() {	return sampleMap; };
	void clearSampleMap(NotificationType n);

	void reloadSampleMap();

	void loadSampleMap(PoolReference ref);

	void loadEmbeddedValueTree(const ValueTree& v, bool loadAsynchronous = false);

	void updateRRGroupAmountAfterMapLoad();

	void nonRealtimeModeChanged(bool isNonRealtime) override;

#if 0
	void loadSampleMapSync(const File &f);
	void loadSampleMapSync(const ValueTree &valueTreeData);
	void loadSampleMapFromIdAsync(const String& sampleMapId);
	void loadSampleMapFromId(const String& sampleMapId);
#endif

	/** This function will be called on a background thread and preloads all samples. */
	bool preloadAllSamples();

	bool preloadSample(StreamingSamplerSound * s, const int preloadSizeToUse);

	bool saveSampleMap() const;

	bool saveSampleMapAsReference() const;

	bool saveSampleMapAsMonolith (Component* mainEditor) const;

	bool shouldUseRoundRobinLogic() const { return useRoundRobinCycleLogic; }

	/** Disables the automatic cycling and allows custom setting of the used round robin group. */
	void setUseRoundRobinLogic(bool shouldUseRoundRobinLogic) noexcept { useRoundRobinCycleLogic = shouldUseRoundRobinLogic; };
	/** Sets the current index to the group. */
	bool setCurrentGroupIndex(int currentIndex, int eventId=-1);

	

	void setRRGroupVolume(int groupIndex, float gainValue);

	bool setMultiGroupState(int groupIndex, bool shouldBeEnabled, int eventId=-1);
	
	bool setMultiGroupState(const int* data128, int numSet, int eventId=-1);

	bool isRoundRobinEnabled() const noexcept { return useRoundRobinCycleLogic; };
	void setRRGroupAmount(int newGroupLimit);

	bool isPitchTrackingEnabled() const {return pitchTrackingEnabled; };
	bool isOneShot() const {return oneShotEnabled; };

	bool isNoteNumberMapped(int noteNumber) const;

	int getMidiInputLockValue(const Identifier& id) const;
	void toggleMidiInputLock(const Identifier& propertyId, int lockValue);

	void setDisplayGroupFollowsRRGroup(bool shouldFollow)
	{
		if(shouldFollow != displayGroupFollowsRRGroup)
		{
			displayGroupFollowsRRGroup = shouldFollow;

			if(shouldFollow)
				setDisplayedGroup(getCurrentRRGroup()-1, true, {}, sendNotificationAsync);
		}
	}

	bool isDisplayGroupFollowingRRGroup() const { return displayGroupFollowsRRGroup; }

	bool displayGroupFollowsRRGroup = false;

	CriticalSection &getSamplerLock() {	return lock; }

	SimpleReadWriteLock& getIteratorLock() { return iteratorLock; };
	
	const CriticalSection& getExportLock() const { return exportLock; }

#if USE_BACKEND || HI_ENABLE_EXPANSION_EDITING
	SampleEditHandler* getSampleEditHandler() { return sampleEditHandler; }
	const SampleEditHandler* getSampleEditHandler() const { return sampleEditHandler; }
#endif

	struct SamplerDisplayValues
	{
		SamplerDisplayValues() : currentSamplePos(0.0)
		{
            memset(currentNotes, 0, 128);
		};

		double currentSamplePos = 0.0f;
		double currentSampleStartPos = 0.0f;
		float crossfadeTableValue = 0.0f;
		int currentGroup = 1;
		BigInteger visibleGroups;

		uint8 currentNotes[128];
	};

	const SamplerDisplayValues &getSamplerDisplayValues() const { return samplerDisplayValues;	}

	SamplerDisplayValues& getSamplerDisplayValues() { return samplerDisplayValues; }

	int getRRGroupsForMessage(int noteNumber, int velocity);
	void refreshRRMap();

    void setReversed(bool shouldBeReversed);

	void updatePurgeFromAttribute(int roundedValue);

	void purgeAllSamples(bool shouldBePurged, bool changePreloadSize=true);

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

        if(channelData[channelIndex].enabled != channelIsEnabled)
        {
            channelData[channelIndex].enabled = channelIsEnabled;
            asyncPurger.triggerAsyncUpdate(); // will call refreshChannelsForSound asynchronously
        }
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

	int getCurrentRRGroup(int eventId=-1) const noexcept
	{
		if(eventId == -1)
			return multiRRGroupState.getSingleGroupIndex();

		for(const auto& g: eventIdsForGroupIndexes)
		{
			if((uint16)eventId == g.first)
				return g.second.getSingleGroupIndex();
		}

		return multiRRGroupState.getSingleGroupIndex();;
	}

	int getNumActiveGroups() const;

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

	AudioSampleBuffer* getTemporaryStretchBuffer() { return &stretchBuffer; }

	hlac::HiseSampleBuffer* getTemporaryVoiceBuffer() { return &temporaryVoiceBuffer; }

	bool checkAndLogIsSoftBypassed(DebugLogger::Location location) const;

	void setHasPendingSampleLoad(bool hasSamplesPending)
	{
		samplePreloadPending = hasSamplesPending;
	}

	bool hasPendingSampleLoad() const { return samplePreloadPending; }

	bool killAllVoicesAndCall(const ProcessorFunction& f, bool restrictToSampleLoadingThread=true);

	void setUseStaticMatrix(bool shouldUseStaticMatrix)
	{
		useStaticMatrix = shouldUseStaticMatrix;
	}
    
    bool isUsingStaticMatrix() const noexcept { return useStaticMatrix; };

	void setDisplayedGroup(int index, bool shouldBeVisible, ModifierKeys mods, NotificationType notifyListener);
	
	void setSortByGroup(bool shouldSortByGroup);

	bool shouldDelayUpdate() const noexcept { return delayUpdate; }

	/** Checks the global queue if there are any jobs that will be executed sometime in the future. 
	
		You can use this to query whether to defer the thing you need to do or run it synchronously.
	*/
	bool hasPendingAsyncJobs() const;

	/** This checks whether there is a async function waiting to be executed in the global queue.
	
		If there is a function, it will defer the function and return false, otherwise it will 
		call it asynchronously and return true. 
	*/
	bool callAsyncIfJobsPending(const ProcessorFunction& f);

	bool shouldAbortIteration() const noexcept { return false; }

	bool& getIterationFlag() { return abortIteration; };

	CascadedEnvelopeLowPass* getEnvelopeFilter() { return envelopeFilter.get(); }

	void setEnableEnvelopeFilter();

	void setPlayFromPurge(bool shouldPlayFromPurge, bool refreshPreloadSize);

	bool shouldPlayFromPurge() const { return enablePlayFromPurge; }

	TimestretchOptions::TimestretchMode getTimestretchMode() const { return currentTimestretchOptions.mode; }

	void setCurrentTimestretchMode(TimestretchOptions::TimestretchMode newMode);
	
	void setTimestretchRatio(double newRatio);

	

	TimestretchOptions getTimestretchOptions() const { return currentTimestretchOptions; }

	void setTimestretchOptions(const TimestretchOptions& newOptions);

	double getCurrentTimestretchRatio() const;

	PolyHandler& getSyncVoiceHandler() { return syncVoiceHandler; }

	void refreshReleaseStartFlag();

private:

	scriptnode::PolyHandler syncVoiceHandler;
	scriptnode::core::stretch_player<NUM_POLYPHONIC_VOICES>::tempo_syncer syncer;

	TimestretchOptions currentTimestretchOptions;

	double ratioToUse = 1.0;

	TimestretchOptions timestretchOptions;

	int lockVelocity = -1;
	int lockRRGroup = -1;

	int realVoiceAmount = NUM_POLYPHONIC_VOICES;

	SimpleReadWriteLock iteratorLock;

	bool abortIteration = false;
	
	

	bool isOnSampleLoadingThread() const
	{
		return getMainController()->getKillStateHandler().getCurrentThread() == MainController::KillStateHandler::TargetThread::SampleLoadingThread;
	}

	bool allVoicesAreKilled() const
	{
		return !getMainController()->getKillStateHandler().isAudioRunning();
	}

	


	struct AsyncPurger : public AsyncUpdater,
						 public Timer
	{
	public:

		AsyncPurger(ModulatorSampler *sampler_) :
			sampler(sampler_)
		{};

		void timerCallback();

		void handleAsyncUpdate() override;

	private:

		ModulatorSampler *sampler;
	};

	
    /** Sets the streaming buffer and preload buffer sizes. */
    void setPreloadSize(int newPreloadSize);
    
	CriticalSection exportLock;

	AsyncPurger asyncPurger;

	void refreshCrossfadeTables();

	RoundRobinMap roundRobinMap;

	bool reversed = false;

	bool pitchTrackingEnabled;
	bool oneShotEnabled;
	bool crossfadeGroups;
	bool purged;
	bool deactivateUIUpdate;
	bool soundsHaveReleaseStart = false;
	int rrGroupAmount;
	//int currentRRGroupIndex;

	static constexpr int MaxMultiGroupIndex = 64;

	struct MultiGroupState
	{
		MultiGroupState()
		{
			numSet = 0;
		}

		bool operator[](int index) const
		{
			return data[index];
		}

		void copyFromIntArray(const int* values, int numToCopy, int numSetValues)
		{
			for (int i = 0; i < numToCopy; i++)
				data.setBit(i, (uint8)(values[i] != -1));

			numSet = numSetValues;
		}

		void setAll(bool enabled)
		{
			data.setAll(enabled);
			numSet = enabled * 256;
		}

		void set(int index, bool enabled)
		{
			data.setBit(index, enabled);
			numSet = jmax(0, numSet + ((int)enabled) * 2 - 1);
		}

		operator bool() const { return !data.isEmpty();}

		uint8 getNumSetBits() const noexcept { return numSet; }

		void setSingleGroupIndex(uint8 g)
		{
			currentRRGroup = g;
		}

		int getSingleGroupIndex() const noexcept { return currentRRGroup; }

		void bumpRoundRobin(int upperLimit)
		{
			currentRRGroup++;
				if (currentRRGroup > upperLimit) currentRRGroup = 1;
		}

	private:

		hise::VoiceBitMap<MaxMultiGroupIndex, uint16> data;
		uint8 numSet = 0;
		uint8 currentRRGroup = 1;
	};

	using GroupQueue = hise::UnorderedStack<std::pair<uint16, MultiGroupState>, NUM_POLYPHONIC_VOICES/4>;
	GroupQueue eventIdsForGroupIndexes;
	bool enablePlayFromPurge = false;

	Array<float> rrGroupGains;
	bool useRRGain = false;

	MultiGroupState multiRRGroupState;

	bool useRoundRobinCycleLogic;
	RepeatMode repeatMode;
	int voiceAmount;
	int preloadScaleFactor = 1;

	mutable SamplerDisplayValues samplerDisplayValues;

	File loadedMap;
	File workingDirectory;

	int preloadSize;
	int bufferSize;

	bool useStaticMatrix = false;

	int64 memoryUsage;

	AudioSampleBuffer crossfadeBuffer;

	hlac::HiseSampleBuffer temporaryVoiceBuffer;
	AudioSampleBuffer stretchBuffer;

	bool delayUpdate = false;
	int lowPassOrder = 0;

	float groupGainValues[8];
	float currentCrossfadeValue;

	ChannelData channelData[NUM_MIC_POSITIONS];
	int numChannels;

	ScopedPointer<SampleMap> sampleMap;
	ModulatorChain* sampleStartChain = nullptr;
	ModulatorChain* crossFadeChain = nullptr;
	ScopedPointer<AudioThumbnailCache> soundCache;
	
	ScopedPointer<CascadedEnvelopeLowPass> envelopeFilter;

#if USE_BACKEND || HI_ENABLE_EXPANSION_EDITING
	ScopedPointer<SampleEditHandler> sampleEditHandler;
#endif

    std::atomic<bool> samplePreloadPending;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulatorSampler);
};


} // namespace hise
#endif  // MODULATORSAMPLER_H_INCLUDED
