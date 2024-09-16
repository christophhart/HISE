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

namespace hise { using namespace juce;

struct CascadedEnvelopeLowPass
{
	static constexpr int NumMaxOrders = 5;

	CascadedEnvelopeLowPass(bool isPoly);

	using FilterType = scriptnode::filters::one_pole<NUM_POLYPHONIC_VOICES>;

	FilterType** begin()
	{
		return filters.begin();
	}

	FilterType** end()
	{
		return begin() + order;
	}

	void process(float frequency, AudioSampleBuffer& b, int startSampleInBuffer=0, int maxNumSamples=0);

	void setOrder(int newOrder)
	{
		order = jlimit(1, filters.size(), order);
	}

	void prepare(PrepareSpecs ps)
	{
		if (polyManager.isEnabled())
			ps.voiceIndex = &polyManager;

		for (auto f : filters)
			f->prepare(ps);

		reset();
	}

	void reset()
	{
		for (auto f : filters)
			f->reset();
	}

	snex::PolyHandler polyManager;

private:

	int order = 1;
	OwnedArray<scriptnode::filters::one_pole<NUM_POLYPHONIC_VOICES>> filters;
};

typedef ReferenceCountedArray<StreamingSamplerSound> StreamingSamplerSoundArray;
typedef Array<WeakReference<StreamingSamplerSound>> WeakStreamingSamplerSoundArray;

#define FOR_EVERY_SOUND(x) {for (int i = 0; i < soundArray.size(); i++) if(soundArray[i].get() != nullptr) soundArray[i]->x;}

// ====================================================================================================================

/** A simple POD structure for basic mapping data. This is used to convert between multimic / single samples. */
struct MappingData
{
	MappingData(int r, int lk, int hk, int lv, int hv, int rr);;

	MappingData() {};

	ValueTree data;

	/** This fills the information from the given sound. 
	*
	*	If the property is non-null, it will be used, so you can use sets with only one loop point
	*	per mic position.
	*/
	void fillOtherProperties(ModulatorSamplerSound* sound);
};

// ====================================================================================================================

#define DECLARE_ID(x) const juce::Identifier x(#x);

namespace SamplerKeyPresses
{
	DECLARE_ID(toggle_sample_preview);
}namespace SampleIds
{
DECLARE_ID(Unused);
DECLARE_ID(ID);
DECLARE_ID(FileName);
DECLARE_ID(Root);
DECLARE_ID(HiKey);
DECLARE_ID(LoKey);
DECLARE_ID(LoVel);
DECLARE_ID(HiVel);
DECLARE_ID(RRGroup);
DECLARE_ID(Volume);
DECLARE_ID(Pan);
DECLARE_ID(Normalized);
DECLARE_ID(NormalizedPeak);
DECLARE_ID(Pitch);
DECLARE_ID(SampleStart);
DECLARE_ID(SampleEnd);
DECLARE_ID(SampleStartMod);
DECLARE_ID(LoopStart);
DECLARE_ID(LoopEnd);
DECLARE_ID(LoopXFade);
DECLARE_ID(LoopEnabled);
DECLARE_ID(LowerVelocityXFade);
DECLARE_ID(UpperVelocityXFade);
DECLARE_ID(SampleState);
DECLARE_ID(Reversed);
DECLARE_ID(GainTable);
DECLARE_ID(PitchTable);
DECLARE_ID(LowPassTable);
DECLARE_ID(NumQuarters);

#undef DECLARE_ID

struct Helpers
{
	static Identifier getEnvelopeId(Modulation::Mode m)
	{
		switch (m)
		{
		case Modulation::Mode::GainMode:  return SampleIds::GainTable;
		case Modulation::Mode::PitchMode: return SampleIds::PitchTable;
		case Modulation::Mode::PanMode:   return SampleIds::LowPassTable;
        default:                          return {};
		}
	}
	static Modulation::Mode getEnvelopeType(const Identifier& id)
	{
		if (id == GainTable)
			return Modulation::Mode::GainMode;
		if (id == PitchTable)
			return Modulation::Mode::PitchMode;
		if (id == LowPassTable)
			return Modulation::Mode::PanMode;
        
        return Modulation::Mode::numModes;
	}

	static const Array<Identifier>& getMapIds()
	{
		static const Array<Identifier> ids = { Root , HiKey, LoKey,  HiVel,  LoVel,
			RRGroup, LowerVelocityXFade,  UpperVelocityXFade };

		return ids;
	}

	static const Array<Identifier>& getAudioIds()
	{
		static const Array<Identifier> ids = { SampleStart,  SampleEnd,  SampleStartMod,  
			LoopEnabled,  LoopStart,  LoopEnd,  LoopXFade };

		return ids;
	}

	static bool isMapProperty(const Identifier& id)
	{
		return id == Root || id == HiKey || id == LoKey || id == HiVel || id == LoVel || id == RRGroup ||
			   id == LowerVelocityXFade || id == UpperVelocityXFade;
	}

	static bool isAudioProperty(const Identifier& id)
	{
		return id == SampleStart || id == SampleEnd || id == SampleStartMod || id == LoopEnabled ||
			id == LoopStart || id == LoopEnd || id == LoopXFade;
	}

	static Array<Identifier> getAllIds()
	{
		static const Array<Identifier> ids({
			ID,
			FileName,
			Root,
			HiKey,
			LoKey,
			LoVel,
			HiVel,
			RRGroup,
			Volume,
			Pan,
			Normalized,
			Pitch,
			SampleStart,
			SampleEnd,
			SampleStartMod,
			LoopStart,
			LoopEnd,
			LoopXFade,
			LoopEnabled,
			LowerVelocityXFade,
			UpperVelocityXFade,
			SampleState,
			Reversed,
		    NumQuarters
		});
		
		return ids;
	}

};

const int numProperties = 25;
}



#undef DECLARE_ID

/** A ModulatorSamplerSound is a wrapper around a StreamingSamplerSound that allows modulation of parameters.
*	@ingroup sampler
*
*	It also contains methods that extend the properties of a StreamingSamplerSound. */
class ModulatorSamplerSound : public ModulatorSynthSound,
							  public ControlledObject
{
public:

	using Ptr = ReferenceCountedObjectPtr<ModulatorSamplerSound>;

	// ====================================================================================================================

	
	/** The extended properties of the ModulatorSamplerSound */
	enum Property
	{
		ID = 1, ///< a unique ID which corresponds to the number in the sound array
		FileName, ///< the complete filename of the audio file
		RootNote, ///< the root note
		HiKey, ///< the highest mapped key
		LoKey, ///< the lowest mapped key
		LoVel, ///< the lowest mapped velocity
		HiVel, ///< the highest mapped velocity
		RRGroup, ///< the group index for round robin / random group start behaviour
		Volume, ///< the gain in decibels. It is converted to a gain value internally
		Pan,
		Normalized, ///< enables normalization of the sample data
		Pitch, ///< the pitch factor in cents (+- 100). This is for fine tuning, for anything else, use RootNote.
		SampleStart, ///< the start sample
		SampleEnd, ///< the end sample,
		SampleStartMod, ///< the amount of samples that the sample start can be modulated.
		LoopStart, ///< the loop start in samples. This is independent from the sample start / end, but it checks the bounds.
		LoopEnd, ///< the loop end in samples. This is independent from the sample start / end, but it checks the bounds.
		LoopXFade, ///< the loop crossfade at the end of the loop (using a precalculated buffer)
		LoopEnabled, ///< true if the sample should be looped
		LowerVelocityXFade, ///< the length of the velocity crossfade (0 if there is no crossfade). If the crossfade starts at the bottom, it will have negative values.
		UpperVelocityXFade, ///< the length of the velocity crossfade (0 if there is no crossfade). If the crossfade starts at the bottom, it will have negative values.
		SampleState, ///< this property allows to set the state of samples between 'Normal', 'Disabled' and 'Purged'
		Reversed,
		GainTable,
		PitchTable,
		numProperties
	};

	
	// ====================================================================================================================

	ModulatorSamplerSound(SampleMap* parent, const ValueTree& d, HlacMonolithInfo* monolithData=nullptr);

	~ModulatorSamplerSound();

	// ====================================================================================================================

	/** Returns the name of the Property.
	*
	*	This is used to display the name in the user interface and for the tag name within the XML samplemap,
	*	so you must return a valid tag name here. */
	/** Returns true if the property should be changed asynchronously when all voices are killed. */
	static bool isAsyncProperty(const Identifier& id);

	/** Returns the min and max values for the Property.
	*
	*	This method is non-static because you can change the range dynamically eg. depending on other properties.
	*
	*	It returns the following ranges:
	*
	*	- MIDI Upper limits:	Low limits - 127
	*	- MIDI Lower limits:	0 -> Upper Limit
	*	- RRGroups:				0 -> number of RR groups
	*	- Volume:				-100dB -> +18dB
	*	- Pitch:				-100ct -> +100ct
	*	- Sample Start:			0 -> sample end or loop start or sample end - sample start mod
	*	- Sample End:			sample start + sampleStartMod -> file end
	*	- Sample Start Mod:		0 -> sample length
	*	- Loop enabled:			0 -> 1
	*	- Loop Start:			(SampleStart + Crossfade) -> (LoopEnd - Crossfade)
	*	- Loop End:				(LoopStart + Crossfade) -> SampleEnd
	*	- Loop Xfade:			0 -> (LoopStart - SampleStart) or LoopLength
	*/
	Range<int> getPropertyRange(const Identifier& p) const;;

	/** Returns a string version of the Property.
	*
	*	This is used for displaying a nicer value than the raw data value (eg. note names, or milliseconds).
	*	Do not use the return value for further processing, use getProperty() instead.
	*/
	String getPropertyAsString(const Identifier& id) const;;

	/** Toggles the boolean properties.
	*
	*	Currently supported Properties:
	*
	*	- Normalized
	*	- LoopEnabled
	*/
	void toggleBoolProperty(const Identifier& id);

	var operator[] (const Identifier& id) const { return getSampleProperty(id); }

	// ====================================================================================================================

	/** Call this whenever you want to start a new series of property modifications. */
	void startPropertyChange(const Identifier& id, int newValue);

	void startPropertyChange() {  }

	void startPropertyChange(const String& )
	{
		
	}

	/** Call this whenever you finish a series of property modifications.
	*
	*	It will group all actions since startPropertyChange and saves it to the UndoManager list
	*	with a description "CHANGING PROPERTY FROM X TO Y"
	*/
	void endPropertyChange(const Identifier& id, int startValue, int endValue);

	void endPropertyChange(const String &actionName);

	// ====================================================================================================================

	/** Closes the file handle for all samples of this sound. */
	void openFileHandle();

	/** Opens the file handle for all samples of this sound. */
	void closeFileHandle();

	

	// ====================================================================================================================

	

	/** Returns the id.
	*
	*	Can also be achieved by getProperty(ID), but this is more convenient. */
	int getId() const { return data.getParent().indexOf(data); };

	Range<int> getNoteRange() const;
	Range<int> getVelocityRange() const;

	/** Returns the gain value of the sound.
	*
	*	Unlike getProperty(Volume), it returns a value from 0.0 to 1.0, so use this for internal stuff
	*/
	float getPropertyVolume() const noexcept;

	/** Returns the pitch value of the sound.
	*
	*	Unlike getProperty(Pitch), it returns the pitch factor (from 0.5 to 2.0)
	*/
	double getPropertyPitch() const noexcept;;

	/** returns the sample rate of the sound (!= the sample rate of the current audio settings) */
	double getSampleRate() const { return firstSound->getSampleRate(); };

	/** returns the root note. */
	int getRootNote() const noexcept{ return rootNote; };

	void initPreloadBuffer(int preloadSize)
	{
		checkFileReference();

		if (noteRangeExceedsMaxPitch())
			preloadSize = -1;

		FOR_EVERY_SOUND(setPreloadSize(preloadSize, true));
	}

	bool noteRangeExceedsMaxPitch() const;

    double getMaxPitchRatio() const;
    
	void loadEntireSampleIfMaxPitch();

	// ====================================================================================================================

	void setMaxRRGroupIndex(int newGroupLimit);
	void setRRGroup(int newGroupIndex) noexcept{ rrGroup = jmin(newGroupIndex, maxRRGroup); };
	int getRRGroup() const;

	// ====================================================================================================================

	bool appliesToVelocity(int velocity) override { return velocityRange[velocity]; };
	bool appliesToNote(int midiNoteNumber) override { return !purged && allFilesExist && midiNotes[midiNoteNumber]; };
	bool appliesToChannel(int /*midiChannel*/) override { return true; };
	bool appliesToRRGroup(int group) const noexcept{ return rrGroup == group; };

	// ====================================================================================================================

	StreamingSamplerSound::Ptr getReferenceToSound() const { return firstSound.get(); };
	StreamingSamplerSound::Ptr getReferenceToSound(int multiMicIndex) const { return soundArray[multiMicIndex]; };

	PoolReference createPoolReference() const
	{
		return PoolReference(getMainController(), getSampleProperty(SampleIds::FileName).toString(), ProjectHandler::SubDirectories::Samples);
	}

	PoolReference createPoolReference(int multiMicIndex) const
	{
		if (isPositiveAndBelow(multiMicIndex, getNumMultiMicSamples()))
		{
			return PoolReference(getMainController(), getReferenceToSound(multiMicIndex)->getFileName(true), ProjectHandler::SubDirectories::Samples);
		}

		return {};
	}

	// ====================================================================================================================

	/** This sets the MIDI related properties without undo / range checks. */
	void setMappingData(MappingData newData);

	/** Calculates the gain value that must be applied to normalize the volume of the sample ( 1.0 / peakValue ).
	*
	*	It should save calculated value along with the other properties, but if a new sound is added,
	*	it will call StreamingSamplerSound::getPeakValue(), which scans the whole file.
	*/
	void calculateNormalizedPeak();;

	void removeNormalisationInfo(UndoManager* um);

	/**	Returns the gain value that must be applied to normalize the volume of the sample ( 1.0 / peakValue ). */
	float getNormalizedPeak() const;

	/** Checks if the normalization gain should be applied to the sample. */
	bool isNormalizedEnabled() const noexcept{ return isNormalized; };

	/** Returns the calculated (equal power) pan value for either the left or the right channel. */
	float getBalance(bool getRightChannelGain) const;;

	void setReversed(bool shouldBeReversed);

	// ====================================================================================================================

	void setVelocityXFade(int crossfadeLength, bool isUpperSound);
	float getGainValueForVelocityXFade(int velocity);

	// ====================================================================================================================

	int getNumMultiMicSamples() const noexcept;;
	bool isChannelPurged(int channelIndex) const;;
	void setChannelPurged(int channelIndex, bool shouldBePurged);

	bool preloadBufferIsNonZero() const noexcept;

	bool hasUnpurgedButUnloadedSounds() const;

	// ====================================================================================================================

	bool isPurged() const noexcept{ return purged; };
	void setPurged(bool shouldBePurged);
	void checkFileReference();
	bool isMissing() const noexcept
	{
		for (auto s : soundArray)
		{
			if (s == nullptr || s->isMissing())
				return true;
		}

		return false;
	};

	// ====================================================================================================================

	static void selectSoundsBasedOnRegex(const String &regexWildcard, ModulatorSampler *sampler, SelectedItemSet<ModulatorSamplerSound::Ptr> &set);

	ValueTree getData() const { return data; }

	void updateInternalData(const Identifier& id, const var& newValue);

	void updateAsyncInternalData(const Identifier& id, int newValue);

	var getDefaultValue(const Identifier& id) const;

	void setSampleProperty(const Identifier& id, const var& newValue, bool useUndo=true);

	var getSampleProperty(const Identifier& id) const;

	void setDeletePending()
	{
		deletePending = true;
	}

	bool isDeletePending() const
	{
		return deletePending;
	}
	
	void addEnvelopeProcessor(HiseAudioThumbnail& th);

	AudioFormatReader* createAudioReader(int micIndex);

	struct EnvelopeTable : public ComplexDataUIUpdaterBase::EventListener,
		public Timer
	{
		static constexpr int DownsamplingFactor = 32;

		using Type = Modulation::Mode;

		void timerCallback() override;

		EnvelopeTable(ModulatorSamplerSound& parent_, Type type_, const String& b64);

		~EnvelopeTable();

		void onComplexDataEvent(ComplexDataUIUpdaterBase::EventType t, var data) override;

		void rebuildBuffer();

		SampleLookupTable table;
		WeakReference<HiseAudioThumbnail> thumbnailToPreview;

		static void processThumbnail(EnvelopeTable& t, var left, var right);

		void processBuffer(AudioSampleBuffer& b, int srcOffset, int dstOffset);

		float getUptimeValue(double uptime) const
		{
			if (auto sl = SimpleReadWriteLock::ScopedTryReadLock(lock))
			{
				auto s = parent.getReferenceToSound(0);

				auto ls = (double)s->getLoopStart();

				if (s->isLoopEnabled() && uptime > (ls - (double)sampleRange.getStart()))
				{
					uptime -= ls;
					uptime = hmath::wrap(uptime, (double)s->getLoopLength());
					uptime += ls;
				}
				else
					uptime += (double)sampleRange.getStart();

				uptime /= (double)DownsamplingFactor;

				auto idx = jlimit(0, numElements - 1, roundToInt(uptime));
				return lookupTable[idx];
			}

			return 1.0f;
		}

		static float getFreqValueInverse(float input);

		static float getFreqValue(float input);

		static float getGainValue(float input)
		{
			return input * 2.0f;
		}

		static float getPitchValue(float input);

	private:

		

		static String getFreqencyString(float input)
		{
			input = getFreqValue(input);
			
			String s;

			if (input > 1000.0f)
			{
				s << String(input / 1000.0, 1);
				s << " kHz";
			}
			else
			{
				s << String(roundToInt(input));
				s << " Hz";
			}

			
			return s;
		}

		static String getGainString(float input)
		{
			auto v = getGainValue(input);
			
			v = Decibels::gainToDecibels(v);

			String s;
			s << String(v, 1) << "dB";
			return s;
		}

		static String getPitchString(float input)
		{
			auto v = getSemitones(input) * 100.0f;
			String s;
			s << String(roundToInt(v)) << " ct";
			return s;
		}

		static float getSemitones(float input)
		{
			input = input * 2.0f - 1.0f;
			return input;
		}

		

		Range<int> sampleRange;
		HeapBlock<float> lookupTable;
		int numElements;
		Type type;
		ModulatorSamplerSound& parent;

		mutable SimpleReadWriteLock lock;

		JUCE_DECLARE_WEAK_REFERENCEABLE(EnvelopeTable);
	};

	EnvelopeTable* getEnvelope(Modulation::Mode m)
	{
		if (m < Modulation::Mode::numModes)
			return envelopes[m];
		else
			return nullptr;
	}

    double getNumQuartersForTimestretch(double fallback) const
    {
        if(numQuartersForTimestretch == 0.0)
            return fallback;
        
        return numQuartersForTimestretch;
    }
    
private:

	void clipRangeProperties(const Identifier& id, int value, bool useUndo);

	void loadSampleFromValueTree(const ValueTree& sampleData, HlacMonolithInfo* hmaf);

	int getPropertyValueWithDefault(const Identifier& id) const;

	WeakReference<SampleMap> parentMap;
	ValueTree data;
	UndoManager *undoManager;

	ScopedPointer<EnvelopeTable> envelopes[Modulation::Mode::numModes];

	// ================================================================================================================

	friend class MultimicMergeDialogWindow;
	
	const CriticalSection& getLock() const { return firstSound.get()->getSampleLock(); };
	
	

	CriticalSection exportLock;
	
	float normalizedPeak = 1.0f;
	bool isNormalized = false;
	bool purged = false;
	bool reversed = false;
	
    double numQuartersForTimestretch = 0.0;
    
	int upperVeloXFadeValue = 0;
	int lowerVeloXFadeValue = 0;
	int rrGroup = 1;
	int rootNote;
	int maxRRGroup;
	BigInteger velocityRange;
	BigInteger midiNotes;

	std::atomic<float> gain;
	std::atomic<double> pitchFactor;

	float leftBalanceGain = 1.0f;
	float rightBalanceGain = 1.0f;

	BigInteger purgeChannels;

	bool allFilesExist;
	const bool isMultiMicSound;
	bool deletePending = false;

	StreamingSamplerSoundArray soundArray;
	WeakReference<StreamingSamplerSound> firstSound;

	

	

	bool enableAsyncPropertyChange = true;

	// ================================================================================================================

	JUCE_DECLARE_WEAK_REFERENCEABLE(ModulatorSamplerSound)

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSamplerSound)

	public:

	using WeakPtr = WeakReference<ModulatorSamplerSound>;
};

typedef Array<ModulatorSamplerSound::Ptr> SampleSelection;

/** This object acts as global pool for all samples used in an instance of the plugin
*	@ingroup sampler
*
*	It uses reference counting to decide if a sound is used. If a sound is added more than once,
*	it will not load it again, but pass a reference to the already loaded Sound.
*
*	This is achieved by isolating the actual sample data into a StreamingSamplerSound object which will be wrapped
*	by a ModulatorSamplerSound object.
*/
class ModulatorSamplerSoundPool : public StreamingSamplerSoundPool,
	public SafeChangeBroadcaster,
	public PoolBase
{
public:

	enum ColumnId
	{
		FileName = 1,
		Memory,
		State,
		References,
		numColumns
	};

	// ================================================================================================================

	ModulatorSamplerSoundPool(MainController *mc, FileHandlerBase* handler);
	~ModulatorSamplerSoundPool() {}

	// ================================================================================================================

	struct PoolEntry
	{
		PoolReference r;
		WeakReference<StreamingSamplerSound> sound;

		StreamingSamplerSound* get() const { return sound.get(); }
	};

	/** call this with any processor to enable console output. */
	void setDebugProcessor(Processor *p);;


	HlacMonolithInfo::Ptr loadMonolithicData(const ValueTree &sampleMap, const Array<File>& monolithicFiles);

	void setUpdatePool(bool shouldBeUpdated)
	{
		updatePool = shouldBeUpdated;
	}

	void setDeactivatePoolSearch(bool shouldBeDeactivated)
	{
		searchPool = !shouldBeDeactivated;
	}

	void setAllowDuplicateSamples(bool shouldAllowDuplicateSamples);

	int getNumLoadedFiles() const override { return getNumSoundsInPool(); }
	PoolReference getReference(int index) const { return pool[index].r; }
	void clearData() override { pool.clear(); }
	var getAdditionalData(PoolReference r) const override { return var(); }
	StringArray getTextDataForId(int index) const override { return { pool[index].r.getReferenceString() }; };

	void writeItemToOutput(OutputStream& /*output*/, PoolReference /*r*/)  override
	{
		jassertfalse;
	}

	Identifier getFileTypeName() const { return "Samples"; };

	// ================================================================================================================

	void getMissingSamples(StreamingSamplerSoundArray &missingSounds) const;
	void resolveMissingSamples(Component *childComponentOfMainEditor);

	// ================================================================================================================

	/** returns the number of sounds in the pool. */
	int getNumSoundsInPool() const noexcept;

	StringArray getFileNameList() const;

	/** Returns the memory usage for all sounds in the pool.
	*
	*	This is not the exact amount of memory usage of the application, because every sampler has voices which have 
	*	intermediate buffers, so the total memory footprint might be a few megabytes larger.
	*/
	size_t getMemoryUsageForAllSamples() const noexcept;;

	String getTextForPoolTable(int columnId, int indexInPool);

	// ================================================================================================================

	void increaseNumOpenFileHandles() override;

	void decreaseNumOpenFileHandles() override;

	bool isFileBeingUsed(int poolIndex);

	bool &getPreloadLockFlag()
	{
		return isCurrentlyLoading;
	}

	bool isPreloading() const { return isCurrentlyLoading; }

	void setForcePoolSearch(bool shouldBeForced) { forcePoolSearch = shouldBeForced; };

	bool isPoolSearchForced() const;

	void clearUnreferencedMonoliths();

	void clearUnreferencedSamples();

	StreamingSamplerSound* getSampleFromPool(PoolReference r) const;

	

	void addSound(const PoolEntry& newPoolEntry);

	void removeFromPool(const PoolReference& ref);

	HlacMonolithInfo* getMonolith(const Identifier& id);

private:

	

	void clearUnreferencedSamplesInternal();

	struct AsyncCleaner : public Timer
	{
		AsyncCleaner(ModulatorSamplerSoundPool& parent_) :
			parent(parent_)
		{
			flag = false;
			IF_NOT_HEADLESS(startTimer(300));
		};

        ~AsyncCleaner()
        {
            stopTimer();
        }
        
		void triggerAsyncUpdate()
		{
			flag = true;
		}

		void timerCallback() override
		{
			if (flag)
			{
				parent.clearUnreferencedSamplesInternal();
				flag = false;
			}
		}

		ModulatorSamplerSoundPool& parent;

		std::atomic<bool> flag;
	};

	// ================================================================================================================

	AsyncCleaner asyncCleaner;

	ReferenceCountedArray<HlacMonolithInfo> loadedMonoliths;

	int getSoundIndexFromPool(int64 hashCode);

	// ================================================================================================================

	
	AudioProcessor *mainAudioProcessor;
	Processor *debugProcessor;

	MainController *mc;

	Array<PoolEntry> pool;

	bool isCurrentlyLoading;
	bool forcePoolSearch;
    bool updatePool;
	bool searchPool;
    
	bool allowDuplicateSamples = true;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSamplerSoundPool)
};

struct SamplePreviewer : public ControlledObject
{
	SamplePreviewer(ModulatorSampler* sampler_);;

	static void applySampleProperty(AudioSampleBuffer& b, ModulatorSamplerSound::Ptr sound, const Identifier& id, int offset);

	void previewSample(ModulatorSamplerSound::Ptr soundToPlay, int micIndex);

	void setPreviewStart(int previewStart)
	{
		previewOffset = previewStart;
	}

	int getPreviewStart() const { return previewOffset; }

	bool isPlaying() const;

private:

	void previewSampleFromDisk(ModulatorSamplerSound::Ptr soundToPlay, int micIndex);

	void previewSampleWithMidi(ModulatorSamplerSound::Ptr soundToPlay);

	WeakReference<ModulatorSampler> sampler;
	ModulatorSamplerSound::Ptr currentlyPlayedSound;
	int previewOffset = 0;
	HiseEvent previewNote;
};

} // namespace hise
