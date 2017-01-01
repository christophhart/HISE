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

#ifndef MODULATORSAMPLERSOUND_H_INCLUDED
#define MODULATORSAMPLERSOUND_H_INCLUDED

typedef ReferenceCountedArray<StreamingSamplerSound> StreamingSamplerSoundArray;

#define FOR_EVERY_SOUND(x) {for (int i = 0; i < soundList.size(); i++) if(soundList[i].get() != nullptr) soundList[i]->x;}

// ====================================================================================================================

/** A simple POD structure for basic mapping data. This is used to convert between multimic / single samples. */
struct MappingData
{
	MappingData(int r, int lk, int hk, int lv, int hv, int rr) :
		rootNote(r),
		loKey(lk),
		hiKey(hk),
		loVel(lv),
		hiVel(hv),
		rrGroup(rr)
	{};

	MappingData() {};

	int rootNote;
	int loKey;
	int hiKey;
	int loVel;
	int hiVel;
	int rrGroup;
};

// ====================================================================================================================

/** A ModulatorSamplerSound is a wrapper around a StreamingSamplerSound that allows modulation of parameters.
*	@ingroup sampler
*
*	It also contains methods that extend the properties of a StreamingSamplerSound. */
class ModulatorSamplerSound : public ModulatorSynthSound,
	public SafeChangeBroadcaster,
	public RestorableObject
{
public:

	// ====================================================================================================================

	/** The extended properties of the ModulatorSamplerSound */
	enum Property
	{
		ID = 1, ///< a unique ID which corresponds to the number in the sound array
		FileName, ///< the complete filename of the audio file
		RootNote, ///< the root note
		KeyHigh, ///< the highest mapped key
		KeyLow, ///< the lowest mapped key
		VeloLow, ///< the lowest mapped velocity
		VeloHigh, ///< the highest mapped velocity
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
		numProperties
	};

	// ====================================================================================================================

	/** Creates a ModulatorSamplerSound.
	*	You only have to supply the index and the fileName, the rest is supposed to be restored with restoreFromValueTree(). */
	ModulatorSamplerSound(StreamingSamplerSound *sound, int index_);
	~ModulatorSamplerSound();

	ModulatorSamplerSound(StreamingSamplerSoundArray &soundArray, int index_);

	// ====================================================================================================================

	/** Returns the name of the Property.
	*
	*	This is used to display the name in the user interface and for the tag name within the XML samplemap,
	*	so you must return a valid tag name here. */
	static String getPropertyName(Property p);

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
	Range<int> getPropertyRange(Property p) const;;

	/** Returns a string version of the Property.
	*
	*	This is used for displaying a nicer value than the raw data value (eg. note names, or milliseconds).
	*	Do not use the return value for further processing, use getProperty() instead.
	*/
	String getPropertyAsString(Property p) const;;

	/** Returns the property as var object.
	*
	*	Use this method if you want to have access to the actual data.
	*	Internal methods of ModulatorSamplerSound access the member variables directly for more speed.
	*/
	var getProperty(Property p) const;

	/** Change a property. It also sends a change message to all registered listeners.
	*
	*	Calling this method directly can't be undone. If you want undo behaviour,
	*	call setPropertyWithUndo() instead.
	*/
	void setProperty(Property p, int newValue, NotificationType notifyEditor=sendNotification);

	/** Toggles the boolean properties.
	*
	*	Currently supported Properties:
	*
	*	- Normalized
	*	- LoopEnabled
	*/
	void toggleBoolProperty(ModulatorSamplerSound::Property p, NotificationType notifyEditor=sendNotification);;

	var operator[] (Property p) { return getProperty(p); }

	// ====================================================================================================================

	/** Exports all properties (excluding the ID) as value tree. */
	ValueTree exportAsValueTree() const override;;

	/** restores all properties (excluding filename and ID) from the value tree. */
	void restoreFromValueTree(const ValueTree &v) override;

	// ====================================================================================================================

	/** set the UndoManager that is used to save calls to setPropertyWithUndo(). */
	void setUndoManager(UndoManager *newUndoManager) { undoManager = newUndoManager; };

	/** Call this whenever you want to start a new series of property modifications. */
	void startPropertyChange(Property p, int newValue);

	void startPropertyChange() { if (undoManager != nullptr) undoManager->beginNewTransaction(); }

	/** Call this whenever you finish a series of property modifications.
	*
	*	It will group all actions since startPropertyChange and saves it to the UndoManager list
	*	with a description "CHANGING PROPERTY FROM X TO Y"
	*/
	void endPropertyChange(Property p, int startValue, int endValue);

	void endPropertyChange(const String &actionName);

	/** Call this method whenever you want to change a property with undo.
	*
	*	If you didn't supply an UndoManager, it will simply call setProperty().
	*/
	void setPropertyWithUndo(Property p, var newValue);

	// ====================================================================================================================

	/** Closes the file handle for all samples of this sound. */
	void openFileHandle();

	/** Opens the file handle for all samples of this sound. */
	void closeFileHandle();

	// ====================================================================================================================

	void loadData(int preloadSize) { wrappedSound->setPreloadSize(preloadSize, true); };

	/** Returns the id.
	*
	*	Can also be achieved by getProperty(ID), but this is more convenient. */
	int getId() const { return index; };

	Range<int> getNoteRange() const;
	Range<int> getVelocityRange() const;

    void setNewIndex(int newIndex) noexcept { index = newIndex; };
    
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
	double getSampleRate() const noexcept{ return wrappedSound->getSampleRate(); };

	/** returns the root note. */
	int getRootNote() const noexcept{ return rootNote; };

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

	StreamingSamplerSound *getReferenceToSound() const { return wrappedSound.getObject(); };
	StreamingSamplerSound *getReferenceToSound(int multiMicIndex) const { return isMultiMicSound ? soundList[multiMicIndex].get() : wrappedSound.getObject(); };

	// ====================================================================================================================

	/** This sets the MIDI related properties without undo / range checks. */
	void setMappingData(MappingData newData);

	/** Calculates the gain value that must be applied to normalize the volume of the sample ( 1.0 / peakValue ).
	*
	*	It should save calculated value along with the other properties, but if a new sound is added,
	*	it will call StreamingSamplerSound::getPeakValue(), which scans the whole file.
	*
	*	@param forceScan if true, then it will scan the file and calculate the peak value.
	*/
	void calculateNormalizedPeak(bool forceScan = false);;

	/**	Returns the gain value that must be applied to normalize the volume of the sample ( 1.0 / peakValue ). */
	float getNormalizedPeak() const;

	/** Checks if the normalization gain should be applied to the sample. */
	bool isNormalizedEnabled() const noexcept{ return isNormalized; };

	/** Returns the calculated (equal power) pan value for either the left or the right channel. */
	float getBalance(bool getRightChannelGain) const;;

	// ====================================================================================================================

	void setVelocityXFade(int crossfadeLength, bool isUpperSound);
	float getGainValueForVelocityXFade(int velocity);

	// ====================================================================================================================

	int getNumMultiMicSamples() const noexcept;;
	bool isChannelPurged(int channelIndex) const;;
	void setChannelPurged(int channelIndex, bool shouldBePurged);

	bool preloadBufferIsNonZero() const noexcept
	{
		for (int i = 0; i < soundList.size(); i++)
		{
			if (!soundList[i]->isPurged() && soundList[i]->getPreloadBuffer().getNumSamples() != 0)
			{
				return true;
			}
		}

		return false;
	}

	// ====================================================================================================================

	bool isPurged() const noexcept{ return purged; };
	void setPurged(bool shouldBePurged);
	void checkFileReference();
	bool isMissing() const noexcept{ return wrappedSound.get() ? wrappedSound->isMissing() : true; };

	// ====================================================================================================================

	static void selectSoundsBasedOnRegex(const String &regexWildcard, ModulatorSampler *sampler, SelectedItemSet<WeakReference<ModulatorSamplerSound>> &set);

private:

	// ================================================================================================================

	/** A PropertyChange is a undoable modification of one of the properties of the sound */
	class PropertyChange : public UndoableAction
	{
	public:

		/** Creates a new property change. It stores the current value as last value for the UndoManager.
		*
		*	@param p the Property that is supposed to be changed
		*	@param newValue the new value.
		*/
		PropertyChange(ModulatorSamplerSound *soundToChange, Property p, var newValue);

		/** executes the modification. */
		bool perform() override;;

		/** reverts the modification. */
		bool undo() override;;

	private:

		// ============================================================================================================

		WeakReference<ModulatorSamplerSound> sound;

		Property changedProperty;

		var currentValue;
		var lastValue;

		// ============================================================================================================

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertyChange)
	};

	// ================================================================================================================

	friend class MultimicMergeDialogWindow;
	friend class WeakReference<ModulatorSamplerSound>;
	WeakReference<ModulatorSamplerSound>::Master masterReference;

	const CriticalSection& getLock() const { return wrappedSound.get()->getSampleLock(); };

	
	float normalizedPeak;
	bool isNormalized;
	bool purged;
	bool allFilesExist;
	const bool isMultiMicSound;

	StreamingSamplerSoundArray wrappedSounds;
	StreamingSamplerSound::Ptr wrappedSound;
	Array<WeakReference<StreamingSamplerSound>> soundList;

	int upperVeloXFadeValue;
	int lowerVeloXFadeValue;

	UndoManager *undoManager;
	int rootNote;
	BigInteger velocityRange;
	BigInteger midiNotes;
	int index;

	int maxRRGroup;
	int rrGroup;

	Atomic<float> gain;
	double centPitch;
    std::atomic<double> pitchFactor;
	int pan;

	float leftBalanceGain;
	float rightBalanceGain;

	BigInteger purgeChannels;

	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSamplerSound)
};

/** This object acts as global pool for all samples used in an instance of the plugin
*	@ingroup sampler
*
*	It uses reference counting to decide if a sound is used. If a sound is added more than once,
*	it will not load it again, but pass a reference to the already loaded Sound.
*
*	This is achieved by isolating the actual sample data into a StreamingSamplerSound object which will be wrapped
*	by a ModulatorSamplerSound object.
*/
class ModulatorSamplerSoundPool : public SafeChangeBroadcaster
{
public:

	// ================================================================================================================

	ModulatorSamplerSoundPool(MainController *mc);
	~ModulatorSamplerSoundPool() {}

	// ================================================================================================================

	/** call this with any processor to enable console output. */
	void setDebugProcessor(Processor *p);;

	/** looks in the global sample pool if the file is already loaded and returns a new ModulatorSamplerSound.
	*
	*	If a copy of a existing sound is found, the following Properties will be shared among all sound instances:
	*
	*	- sample start / end
	*	- loop settings
	*	- sample start modulation
	*
	*	This is necessary because they affect the memory usage of the sound.
	*	All other properties (mapping properties and volume / pitch settings) will be unique.
	*/
	ModulatorSamplerSound *addSound(const ValueTree &soundDescription, int index, bool forceReuse = false);;

	/** Decreases the reference count of the wrapped sound in the pool and deletes it if no references are left. */
	void deleteSound(ModulatorSamplerSound *soundToDelete);;

	bool loadMonolithicData(const ValueTree &sampleMap, const Array<File>& monolithicFiles, OwnedArray<ModulatorSamplerSound> &sounds);

    void setUpdatePool(bool shouldBeUpdated)
    {
        updatePool = shouldBeUpdated;
    }
    
	void setDeactivatePoolSearch(bool shouldBeDeactivated)
	{
		searchPool = !shouldBeDeactivated;
	}

	// ================================================================================================================

	void clearUnreferencedSamples();
	void getMissingSamples(Array<StreamingSamplerSound*> &missingSounds) const;
	void deleteMissingSamples();;
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

	AudioFormatManager afm;


	void increaseNumOpenFileHandles();

	void decreaseNumOpenFileHandles();

	bool isFileBeingUsed(int poolIndex);

	bool &getPreloadLockFlag()
	{
		return isCurrentlyLoading;
	}

	bool isPreloading() const { return isCurrentlyLoading; }

	void setForcePoolSearch(bool shouldBeForced) { forcePoolSearch = shouldBeForced; };

	bool isPoolSearchForced() const;

	void clearUnreferencedMonoliths();

private:

	// ================================================================================================================

	ReferenceCountedArray<HiseMonolithAudioFormat> loadedMonoliths;

	int getSoundIndexFromPool(int64 hashCode);

	ModulatorSamplerSound *addSoundWithSingleMic(const ValueTree &soundDescription, int index, bool forceReuse = false);
	ModulatorSamplerSound *addSoundWithMultiMic(const ValueTree &soundDescription, int index, bool forceReuse = false);
	
	// ================================================================================================================

	AudioProcessor *mainAudioProcessor;
	Processor *debugProcessor;

	MainController *mc;

	ReferenceCountedArray<StreamingSamplerSound> pool;

	bool isCurrentlyLoading;
	bool forcePoolSearch;
    bool updatePool;
	bool searchPool;
    
	int numOpenFileHandles;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulatorSamplerSoundPool)
};


#endif  // MODULATORSAMPLERSOUND_H_INCLUDED
