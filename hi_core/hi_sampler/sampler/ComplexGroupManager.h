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

#define DECLARE_ID(x) static const Identifier x(#x)

namespace groupIds
{
DECLARE_ID(Layers);
DECLARE_ID(Layer);

DECLARE_ID(type);
DECLARE_ID(id);
DECLARE_ID(tokens);
DECLARE_ID(ignorable);
DECLARE_ID(cached);
DECLARE_ID(purgable);
DECLARE_ID(fader);
DECLARE_ID(slotIndex);
DECLARE_ID(sourceType);

DECLARE_ID(isChromatic);


}

#undef DECLARE_ID

/** A interface for sampler sounds that store a 64 bit mask that can be queried with various filters. */
struct SynthSoundWithBitmask: public ModulatorSynthSound
{
	using Bitmask = uint64;

	/** This object can be used to filter another bitmask.
	 *
	 *  When testing a bitmask, it will apply the filter mask to the value
	 *	and then compare it against its internal value.
	 *
	 *	If this is not equal, then it will apply the ignore mask to the value
	 *	and check if it is not zero, as this indicates that the bitmask should
	 *	ignore this filter.
	 */
	struct ValueWithFilter
	{
		ValueWithFilter() = default;

		bool operator==(const ValueWithFilter& other) const;

		/** Prints the bits to the console. */
		void dump(const String message);

		/* Compares the bitmask whether it matches against the ValueWithFilter. */
		bool matches(Bitmask otherValue) const;

		Bitmask filterMask = 0; // the mask that will filter out other bits
		Bitmask ignoreMask = 0; // the mask that filter out all bits except for the ignore bit
		Bitmask value      = 0;	// the current filter value
	};

	/** This is a combination of multiple filters. */
	struct FilterList
	{
		// Currently we'll limit the amount of ignoreable filters to 4, that should
		// be enough for even the most complex mappings
		static constexpr int NumIgnorableFilters = 4;

		bool matches(Bitmask m) const;

		ValueWithFilter mainFilter;
		UnorderedStack<std::pair<uint8, ValueWithFilter>, NumIgnorableFilters> ignorableFilters;
	};

	/** This container stores a list of samples that can be started by each note number.
	 *
	 *	In the onNoteOn callback this will prefilter all notes to start which leads to a huge
	 *	performance gain!
	 */
	struct NoteContainer
	{
		struct Iterator
		{
			Iterator(const NoteContainer& c, int noteNumber_);

			const SynthSoundWithBitmask* const* begin() const;
			const SynthSoundWithBitmask* const* end() const;

		private:

			hise::SimpleReadWriteLock::ScopedReadLock sl;
			const NoteContainer& container;
			int noteNumber;
		};

		/** Iterate over all samples that are assigned to the note number. */
		Iterator createIterator(int noteNumber) const;

		void rebuild(const ReferenceCountedArray<SynthesiserSound>& soundArray);

		/** Returns the bitmask filter that was assigned to this note container. */
		ValueWithFilter getValueWithFilter() const;

		/** Assigns a bitmask filter to the note container. This will be used to prefilter
		 *  containers that contain cached sample collections.
		 *
		 *	Eg. if you have 10.000 samples, 5.000 of them are assigned to the key switch 1
		 *	and 5.000 of them are assigned to the key switch 2, setting the shouldBeCached
		 *	flag will create two note containers and then use this filter to prefilter the
		 *	key switches.
		 */
		void setValueFilter(ValueWithFilter newFilter);

	private:

		ValueWithFilter filter;

		using ContainerType = std::array<std::vector<SynthSoundWithBitmask*>, 128>;

		mutable hise::SimpleReadWriteLock lock;
		ContainerType notes;
	};

	SynthSoundWithBitmask() {}
	virtual ~SynthSoundWithBitmask() {};

	void setBitmask(Bitmask m)
	{
		mask = m;
	}

	/** Set the bitmask. Override this if you want to store it elsewhere. */
	virtual void storeBitmask(Bitmask m, bool useUndo)
	{
		ignoreUnused(useUndo);
		setBitmask(m);
	}

	/** Returns the 64 bit mask. */
	Bitmask getBitmask() const { return mask; }

	/** override this and return the note range. */
	virtual Range<int> getNoteRange() const = 0;

	void dump();

	struct Helpers
	{
		static void dumpBitmask(String msg, Bitmask m, char X='1');
	};

	bool isPurged() const noexcept{ return purged; };

	virtual void setPurged(bool shouldBePurged)
	{
		purged = shouldBePurged;
	}

private:

	Bitmask mask = 0;

	bool purged = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SynthSoundWithBitmask);
};




/** A container that will perform bit mask operations to allow multidimensional organisation of sample properties using
 *  the single RRGroup property.
 *
 */
struct ComplexGroupManager: public ModulatorSynth::SoundCollectorBase,
							public SampleMap::Listener
{
	static constexpr uint8 IgnoreFlag = 0xFF;

	using SampleType = SynthSoundWithBitmask;
	using Bitmask = SampleType::Bitmask;
	using ValueWithFilter = SampleType::ValueWithFilter;

	enum Flags
	{
		FlagIgnorable = 0x01,
		FlagCached    = 0x02,
		FlagXFade     = 0x04,
		FlagPurgable  = 0x08,
		FlagProcessHiseEvent = 0x10,
		FlagProcessPostSounds = 0x20,
		FlagProcessModulation = 0x40
	};

	enum class LogicType
	{
		Undefined,
		Custom,
		RoundRobin,
		Keyswitch,
		TableFade,
		XFade,
		LegatoInterval,
		ReleaseTrigger,
		Choke,
		numLogicTypes
	};

	struct Helpers
	{
		static uint8 getNumBitsRequired(uint8 numItems);
		static bool canBePurged(int flag);
		static bool canBeIgnored(int flag);
		static bool shouldBeCached(int flag);
		static bool isXFade(int flag);

		static int getDefaultValue(LogicType lt, Flags flag);

		static bool isProcessingHiseEvent(int flag);
		static bool isPostProcessingSounds(int flag);

		static bool isProcessingModulation(int flag);

		static StringArray getLogicTypeNames();
		static String getLogicTypeName(LogicType lt);
		
		static String getItemName(LogicType lt, const StringArray& items);
		static ValueTree getRoot(const ValueTree& v);
		static uint8 getLayerIndex(const ValueTree& layerData);
		static LogicType getLogicType(const ValueTree& layerData);

		/** Returns the token array from the groupIds::tokens property. If it's a legato layer, it will calculate the
		 *  note numbers / names depending on the LoKey/HiKey and groupIds::useNumbers property. */
		static StringArray getTokens(const ValueTree& layerData, const ModulatorSampler* sampler=nullptr);
		static Identifier getId(const ValueTree& layerData);
		static Identifier getNextFreeId(LogicType lt, const ValueTree& data);
		static VoiceBitMap<128> getKeyRange(const ValueTree& data, const ModulatorSampler* sampler=nullptr);

		static String getSampleFilename(const SynthSoundWithBitmask* fs);
		static StringArray getFileTokens(const SynthSoundWithBitmask* fs, String separator="_");
		static StringArray getFileTokens(const String& fileName, String separator="_");
	};

	ComplexGroupManager(ReferenceCountedArray<SynthesiserSound>* allSounds, PooledUIUpdater* updater=nullptr);;
	~ComplexGroupManager() override;

	PrepareSpecs lastSpecs;

	void prepare(PrepareSpecs ps);

	/** Returns the index with the given layer. */
	int getLayerIndex(const Identifier& groupId) const;

	uint8 getNumLayers() const { return (uint8)layers.size(); }

	/** Returns the ID of the layer. */
	Identifier getLayerId(uint8 layerIndex) const;

#if JUCE_DEBUG
	/** @internal Prints the bit layout to the console. */
	void dumpLayers();
#endif

	/** Queries a given sample with the layer value. */
	uint8 getLayerValue(Bitmask m, uint8 layerIndex) const;

	/** Returns the current layer value as a token string. */
	String getLayerValueAsToken(SampleType* s, uint8 layerIndex) const;

	/** Returns the token string for the given layer value. */
	String getLayerValueAsToken(uint8 layerIndex, uint8 layerValue) const;

	/** Purges all samples that match the token for the given layer. */
	void setPurged(const Identifier& id, const String& tokenValue, bool shouldBePurged);

	/** Call this to set a filter using the layer name and a token. For a faster operation in the audio thread, use the other overload. */
	void applyFilter(const Identifier& layerId, const String& tokenValue, NotificationType n);

	/** Returns a list of all filters that can be applied to a given layer. */
	Array<ValueWithFilter> getAllFiltersForLayer(uint8 layerIndex) const;

	/** Sets the filter for the given layer index to the non-zero value. This one-based value can be either any number > 0 or the special number 0xFF
	 *  which is the ignore flag.
	 */
	void applyFilter(uint8 layerIndex, uint8 value, NotificationType n);

	/** Called by HISE in the noteOn callback to figure out what sounds to play. */
	void collectSounds(const HiseEvent& m, UnorderedStack<ModulatorSynthSound*>& soundsToBeStarted) override;

	int getPredelayForVoice(const ModulatorSynthVoice* voice) const override;

	/** This function will create a bitmask from the given string and can be used as a quick and dirty filename parser.
	 *
	 *	@param filenameWithoutExtension: the sample name, eg. "C2_sus_pp_RR1.wav"
	 *	@param layerOffset: the index of the first token in the file name that will be applied to the layers in the order of definition
	 *
	 *	@returns the bitmask that has the filters setup correctly for the given sample.
	 */
	Bitmask parseBitmask(const String& filenameWithoutExtension, uint8 layerOffset) const;

	/** Writes the layer values into the sample. */
	void setSampleId(SampleType* s, const Array<std::pair<Identifier, uint8>>& layerValues, bool useUndo) const;

	/** Removes the values in the sample for the given layers. */
	void clearSampleId(SampleType* s, const Array<Identifier>& layerIds, bool useUndo);

	/** Returns a value filter with all given layer values that can be used to filter the displayed samples. */
	ValueWithFilter getDisplayFilter(const Array<std::pair<Identifier, uint8>>& layerValues) const;

	/** Returns a list of all samples that match the given value. */
	Array<WeakReference<SampleType>> getFilteredSelection(uint8 layerIndex, uint8 value, bool includeIgnoredSamples) const;

	Array<WeakReference<SampleType>> getUnassignedSamples(uint8 layerIndex) const;

	/** Returns the number of matches for each given filter. */
	int getNumFiltered(uint8 layerIndex, uint8 value, bool includeIgnoredSamples) const;

	/** Call this in the note on callback and it will handle all special behaviour of all layers. */
	void preHiseEventCallback(const HiseEvent& e) override;
	
	/** Call this whenever you want to reset the internal playing variables (eg. RR counter). */
	void resetPlayingState();

	/** Returns the x-axis value for the xfade for the given sample and XFade layer. */
	float getXFadeValue(uint8 layerIndex, SampleType* s) const;

	/** Returns the table fade index (one-based) if there is a layer with table fade enabled. */
	uint8 getTableFadeValue(Bitmask m) const;

	/** Locks the layer to the fixed value. */
	void setLockLayer(uint8 layerIndex, bool shouldBeLocked, uint8 lockValue);

	float* calculateGroupModulationValuesForVoice(const HiseEvent& e, int voiceIndex, int startSample, int numSamples, SampleType::Bitmask m);

	float getConstantGroupModulationValue(int voiceIndex, SampleType::Bitmask m);

	/** Use this to delay the finalisation process until this object goes out of scope. */
	struct ScopedUpdateDelayer
	{
		ScopedUpdateDelayer(ComplexGroupManager& gm_);

		virtual ~ScopedUpdateDelayer();

	protected:

		ComplexGroupManager& gm;
		const bool prevValue;
	};

	struct ScopedPostProcessorDeactivator
	{
		ScopedPostProcessorDeactivator(ComplexGroupManager& gm_);

		~ScopedPostProcessorDeactivator();

		ComplexGroupManager& gm;
		const bool prevValue;
	};

	ValueTree getDataTree() const { return data; }

	/** Call this whenever the sample amount changes. */
	void refreshCache();

	template <typename T, typename F> void addPlaystateListener(T& obj, uint8 layerIndex, const F& f)
	{
		if(isPositiveAndBelow(layerIndex, layers.size()))
			layers[layerIndex]->playStateBroadcaster.template addListener<T, F>(obj, f, true);
	}

	template <typename T> void removePlaystateListener(T& obj, uint8 layerIndex)
	{
		if(isPositiveAndBelow(layerIndex, layers.size()))
			layers[layerIndex]->playStateBroadcaster.template removeListener<T>(obj);
	}

	/** Returns the number of samples that are ignored by this layer. */
	std::pair<int, int> getNumUnassignedAndIgnored(uint8 layerIndex) const;

	void sampleMapWasChanged(PoolReference) override
	{
		rebuildGroups();
	};

	void sampleAmountChanged() override
	{
		rebuildGroups();
	};

	void sampleMapCleared() override
	{
		rebuildGroups();
	};

	void setSampler(ModulatorSampler* m);

	/** Sends a message { layerIndex, lockEnabled } when setLockLayer() is used. */
	LambdaBroadcaster<uint8, bool> lockBroadcaster;

	struct XFadeLayer;
	
private:

	friend class ComplexGroupManagerComponent;

	span<float, NUM_POLYPHONIC_VOICES> lastRampValues;
	heap<float> scratchBuffer;
	float constantVoiceValue = 1.0f;

	WeakReference<ModulatorSampler> sampler;
	
	struct Layer
	{
		Layer(const Identifier& g, LogicType lt_, const StringArray& tokens_, int flags);
		virtual ~Layer() {}

		/** Overwrite this method and calculate the gain values for the voices.
		 *
		 *  If there is a dynamic value within the buffer, fill the data array with the dynamic gain value and return true
		 *	otherwise set data[0] to the constant value and return false.
		 *
		 *	startSample and numSamples are using the HISE_CONTROL_RATE_DOWNSAMPLE_FACTOR for the same performance as modulation
		 */
		virtual bool calculateGroupModulationValuesForVoice(const HiseEvent& e, int voiceIndex, float* data, int startSample, int numSamples, Bitmask m)
		{
			jassertfalse;
			data[0] = 1.0f;
			return false;
		};

		int logicFlag = 0;

		virtual void handleHiseEvent(ComplexGroupManager& parent, const HiseEvent& e) {};
		virtual void postVoiceStart(ComplexGroupManager& parent, const UnorderedStack<ModulatorSynthSound*>& soundsThatWereStarted) {};
		virtual void resetPlayState(ComplexGroupManager& parent) {};

		virtual int getPredelay(const ComplexGroupManager& parent, const HiseEvent& e, uint8 layerValue, int sampleSampleRate) const { return 0; }

		virtual void prepare(PrepareSpecs ps) {};

		uint8 getUnmaskedValue(Bitmask m) const;
		float getXFadeValue(SampleType* t) const;
		void setValueFilter(ValueWithFilter& v, uint8 value) const;
		void mask(Bitmask& m, uint8 value) const;
		void clearBits(Bitmask& m) const;

		StringArray tokens;
		  
		Identifier groupId;
		LogicType lt = LogicType::Undefined;
		uint8 numItems = 0;
		uint8 bitShift = 0;

		int propertyFlags = 0;

		Bitmask ignoreFlag = 0;
		Bitmask filter;
		uint8 layerIndex = 0;

		bool isLocked = false;

		LambdaBroadcaster<uint8> playStateBroadcaster;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Layer);
	};

	struct RRLayer; struct LegatoLayer; struct TableFadeLayer; struct KeyswitchLayer;
	

	bool matchesCurrentFilter(const HiseEvent& m, SampleType* typed) const;

	void onDataChange(const ValueTree& c, bool wasAdded);

	void onRebuildPropertyChange(const ValueTree& t, const Identifier& id);

	/** Finalises the bit layout and initialises the state. Call this after you defined the layout before adding samples. */
	void finalize();

	void rebuildGroups();

	/** Adds an organisational sample layer to the group manager which will be used for the bitmask.
	 *
	 *	@param groupId: the ID of the layer
	 *	@param lt: one of the available behaviour types (eg. RR, Keyswitch, etc).
	 *  @param tokens: all tokens that are mapped to an integer value of this layer.
	 *	@param flags: a combination of different property flags. @see Flags
	 */
	void addLayer(const Identifier& groupId, LogicType lt, const StringArray& tokens, int flags = 0);
	
	/** Adds a legato layer. This is a special layer with 128 tokens and can be assigned to interval samples. */
	void addLegatoLayer(const Identifier& id);

	bool skipUpdate = false;
	bool deactivatePostProcessing = false;

	UnorderedStack<Layer*, 8> midiProcessors;
	UnorderedStack<Layer*, 8> postProcessors;
	UnorderedStack<Layer*, 8> modulationLayers;

	ValueTree data;
	valuetree::ChildListener dataListener;

	valuetree::RecursivePropertyListener groupRebuildListener;

	int8 tableFadeLayer = -1;

	// The filter that is used to filter out the cached groups
	Bitmask currentGroupFilter = 0;

	SampleType::FilterList currentFilters;
	OwnedArray<Layer> layers;
	ReferenceCountedArray<SynthesiserSound>* soundList = nullptr;
	OwnedArray<SampleType::NoteContainer> groups;

	JUCE_DECLARE_WEAK_REFERENCEABLE(ComplexGroupManager);
};


} // namespace hise
