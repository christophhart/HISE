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

#ifndef MODULATORSAMPLERDATA_H_INCLUDED
#define MODULATORSAMPLERDATA_H_INCLUDED

namespace hise { using namespace juce;

class ModulatorSampler;
class ModulatorSamplerSound;



struct SampleMapData
{
	ValueTree data;
};

/** A SampleMap is a data structure that encapsulates all data loaded into an ModulatorSampler. 
*	@ingroup sampler
*
*	It saves / loads all sampler data (modulators, effects) as well as all loaded sound files.
*
*	It supports two saving modes (monolithic and file-system based).
*	It only accesses the sampler data when saved or loaded, and uses a ChangeListener to check if a sound has changed.
*/
class SampleMap: public SafeChangeListener,
				 public PoolBase::Listener,
				 public ValueTree::Listener
{
public:

	class Listener
	{
	public:

		virtual ~Listener() {};

		virtual void sampleMapWasChanged(PoolReference newSampleMap) = 0;

		virtual void samplePropertyWasChanged(ModulatorSamplerSound* s, const Identifier& id, const var& newValue) 
		{
			ignoreUnused(s, id, newValue);
		};

		virtual void sampleAmountChanged() {};

		virtual void sampleMapCleared() {};

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	/** A SamplerMap can be saved in multiple modes. */
	enum SaveMode
	{
		/** The default mode, until the map gets saved. */
		Undefined = 0,
		/** Saves all data using this file structure:
		*
		*	- the sample map will be saved as .xml file
		*	- the thumbnail data will be saved as thumbnail.dat
		*	- the samples will be saved into a '/samples' subfolder and replaced by relative file references.
		*	- the sampler data (modulators) will be stored as preset file (*.hip) containing a reference to the samplerMap
		*/
		MultipleFiles,
		/** Saves everything into a big file which contains all data. */
		Monolith,
		/** Saves everything into a big file and encrypts the header data using a RSA Key 
		*	which can be used to handle serial numbers
		*/
		MonolithEncrypted,
		numSaveModes
	};
	
	SampleMap(ModulatorSampler *sampler_);

	using FileList = OwnedArray < Array<File> >;

	FileList createFileList();

	~SampleMap();;

	/** Checks if the samplemap was changed and deletes it. */
	void changeListenerCallback(SafeChangeBroadcaster *b);

	/** Checks if any ModulatorSamplerSound was changed since the last save. 
	*
	*	This feature is currently disabled.
	*	It does not check if any other ModulatorSampler Properties were changed.
	*/
	bool hasUnsavedChanges() const
	{
		return changeWatcher != nullptr && changeWatcher->wasChanged();
	}

	void load(const PoolReference& reference);

	void loadUnsavedValueTree(const ValueTree& v);

	/** Saves all data with the mode depending on the file extension. */
	bool save(const File& fileToUse = File());

	bool saveSampleMapAsReference();

	bool saveAsMonolith(Component* mainEditor);

	void setIsMonolith() noexcept { mode = SaveMode::Monolith; }

	bool isMonolith() const noexcept { return mode == SaveMode::Monolith; };

	/** Clears the sample map. */
    void clear(NotificationType n);
	
	ModulatorSampler* getSampler() const { return sampler; }
	
	String getMonolithID() const;

	void setId(Identifier newIdentifier)
    {
        sampleMapId = newIdentifier.toString().replaceCharacter('\\', '/');

		data.setProperty("ID", sampleMapId.toString(), nullptr);
    }
    
	void updateCrossfades(Identifier id, var newValue);
	

	FileHandlerBase* getCurrentFileHandler() const;

	ModulatorSamplerSoundPool* getCurrentSamplePool() const;

    Identifier getId() const { return sampleMapId; };
    
	static String checkReferences(MainController* mc, ValueTree& v, const File& sampleRootFolder, Array<File>& sampleList);

	void addSound(ValueTree& newSoundData);

	void removeSound(ModulatorSamplerSound* s);

	
	const ValueTree getValueTree() const;

	ValueTree getValueTree() { return data; }

	float getCrossfadeGammaValue() const;

	PoolReference getReference() const
	{
		return sampleMapData.getRef();
	}

	void poolEntryReloaded(PoolReference referenceThatWasChanged) override;

	bool isUsingUnsavedValueTree() const
	{
		return !sampleMapData && data.getNumChildren() != 0;
	}

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void sendSampleMapChangeMessage(NotificationType n=sendNotificationAsync)
	{
		notifier.sendMapChangeMessage(n);
	}

	void valueTreePropertyChanged(ValueTree& /*treeWhosePropertyHasChanged*/,
		const Identifier& /*property*/);

	void valueTreeChildAdded(ValueTree& parentTree,
		ValueTree& childWhichHasBeenAdded) override;;

	void addSampleFromValueTree(ValueTree childWhichHasBeenAdded);

	void sendSampleAddedMessage();

	void valueTreeChildRemoved(ValueTree& parentTree,
		ValueTree& childWhichHasBeenRemoved,
		int indexFromWhichChildWasRemoved) override;;

	void sendSampleDeletedMessage(ModulatorSampler * sampler);

	void valueTreeChildOrderChanged(ValueTree& /*parentTreeWhoseChildrenHaveMoved*/,
		int /*oldIndex*/, int /*newIndex*/) override {};

	void valueTreeParentChanged(ValueTree& /*treeWhoseParentHasChanged*/) override {};

	void valueTreeRedirected(ValueTree& /*treeWhichHasBeenChanged*/) override {};

	ModulatorSamplerSound* getSound(int index);
	const ModulatorSamplerSound* getSound(int index) const;

	int getNumRRGroups() const;

	void discardChanges();

	void saveAndReloadMap();

	void suspendInternalTimers(bool shouldBeSuspended);

	bool& getSyncEditModeFlag() { return syncEditMode; }

	struct ScopedNotificationDelayer
	{
		ScopedNotificationDelayer(SampleMap& parent_) :
			parent(parent_)
		{
			parent.delayNotifications = true;
		};

		~ScopedNotificationDelayer()
		{
			parent.delayNotifications = false;

			if (parent.notificationPending)
				parent.sendSampleAddedMessage();

		}

		SampleMap& parent;
	};

#if HISE_SAMPLER_ALLOW_RELEASE_START
	void setReleaseStartOptions(StreamingHelpers::ReleaseStartOptions::Ptr newOptions);

	StreamingHelpers::ReleaseStartOptions::Ptr getReleaseStartOptions() { return releaseStartOptions; }
#endif

private:

	struct ChangeWatcher : private ValueTree::Listener
	{
	public:

		ChangeWatcher(ValueTree& v) :
			d(v)
		{
			d.addListener(this);
		}

		~ChangeWatcher()
		{
			d.removeListener(this);
		}

		bool wasChanged() const
		{
			return changed;
		}

	private:

		ValueTree d;

		void valueTreePropertyChanged(ValueTree& /*treeWhosePropertyHasChanged*/,
			const Identifier& /*property*/)
		{
			changed = true;
		}

		void valueTreeChildAdded(ValueTree& /*parentTree*/,
			ValueTree& /*childWhichHasBeenAdded*/) override
		{
			changed = true;
		}

		void valueTreeChildRemoved(ValueTree& /*parentTree*/,
			ValueTree& /*childWhichHasBeenRemoved*/,
			int /*indexFromWhichChildWasRemoved*/) override
		{
			changed = true;
		}

		void sendSampleDeletedMessage(ModulatorSampler * sampler);

		void valueTreeChildOrderChanged(ValueTree& /*parentTreeWhoseChildrenHaveMoved*/,
			int /*oldIndex*/, int /*newIndex*/) override {};

		void valueTreeParentChanged(ValueTree& /*treeWhoseParentHasChanged*/) override {};

		void valueTreeRedirected(ValueTree& /*treeWhichHasBeenChanged*/) override {};

		bool changed = false;


		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChangeWatcher);
	};

	void setCurrentMonolith();

	

	bool delayNotifications = false;
	bool notificationPending = false;
	
	bool syncEditMode = false;

	valuetree::PropertyListener crossfadeListener;

	struct Notifier: public Dispatchable
	{
		Notifier(SampleMap& parent_);

		void sendMapChangeMessage(NotificationType n);

		void sendMapClearMessage(NotificationType n);

		void addPropertyChange(int index, const Identifier& id, const var& newValue);
		void sendSampleAmountChangeMessage(NotificationType n);

		struct Collector : public LockfreeAsyncUpdater
		{
			Collector(Notifier& parent_) :
				parent(parent_)
			{};

			void handleAsyncUpdate() override;

		private:

			Notifier& parent;
		};

		Collector asyncUpdateCollector;

	private:

		struct AsyncPropertyChange
		{
			AsyncPropertyChange():
				id({})
			{
			}

			AsyncPropertyChange(ModulatorSamplerSound* sound, const Identifier& id, const var& newValue);

			bool operator==(const Identifier& id_) const
			{
				return id == id_;
			}

			Array<SynthesiserSound::Ptr> selection;
			Array<var> values;

			Identifier id;

			void addPropertyChange(ModulatorSamplerSound* sound, const var& newValue);

		};

		struct PropertyChange
		{
			PropertyChange():
				index(-1)
			{};

			bool operator==(int indexToCompare) const
			{
				return indexToCompare == index;
			}

			void set(const Identifier& id, const var& newValue);

			int index;

			NamedValueSet propertyChanges;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PropertyChange);
		};

		void handleHeavyweightPropertyChangesIdle(Array<AsyncPropertyChange, CriticalSection> thisTime);

		void handleHeavyweightPropertyChanges();
		void handleLightweightPropertyChanges();

		void triggerHeavyweightUpdate();
		void triggerLightWeightUpdate();

		ScopedPointer<ChangeWatcher> changeWatcher;

		

		OwnedArray<PropertyChange, CriticalSection> pendingChanges;
		Array<AsyncPropertyChange, CriticalSection> asyncPendingChanges;

		bool lightWeightUpdatePending = false;
		bool heavyWeightUpdatePending = false;

		bool mapWasChanged = false;
		bool sampleAmountWasChanged = false;
		SampleMap& parent;
	};

	ScopedPointer<ChangeWatcher> changeWatcher;

	Notifier notifier;

	/** Restores the samplemap from the ValueTree.
	*
	*	If the files are saved in relative mode, the references are replaced
	*	using the parent directory of the sample map before they are loaded.
	*	If the files are saved as monolith, it assumes the files are already loaded and simply adds references to this samplemap.
	*/
	void parseValueTree(const ValueTree &v);

	PooledSampleMap sampleMapData;

#if HISE_SAMPLER_ALLOW_RELEASE_START
	StreamingHelpers::ReleaseStartOptions::Ptr releaseStartOptions;
#endif

	ValueTree data;

	void setNewValueTree(const ValueTree& v);

	ModulatorSampler *sampler;

	CachedValue<int> mode;

	WeakReference<SampleMapPool> currentPool;

	Array<WeakReference<Listener>, CriticalSection> listeners;

	HlacMonolithInfo::Ptr currentMonolith;

    Identifier sampleMapId;
    
	JUCE_DECLARE_WEAK_REFERENCEABLE(SampleMap);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleMap)
		
};

/** A data container which stores information about the amount of round robin groups for each notenumber / velocity combination.
*
*	The information is precalculated so that the query is a very fast look up operation (O(1)). In order to use it, create one, and
*	call addSample() for every ModulatorSamplerSound you need.
*	You can query the rr group later with getRRGroupsForMessage().
*/
class RoundRobinMap
{
public:

	RoundRobinMap();

	/** Clears the map */
	void clear();

	/** adds the information of the sample to the map. It checks for every notenumber / velocity combination if it is the biggest group. */
	void addSample(const ModulatorSamplerSound *sample);

	/** returns the biggest group index for the given MIDI information. This is very fast. */
	int getRRGroupsForMessage(int noteNumber, int velocity);

private:

	char internalData[128][128];

};

#if HI_ENABLE_EXPANSION_EDITING
class MonolithExporter : public DialogWindowWithBackgroundThread,
						 public AudioFormatWriter
{
public:

	MonolithExporter(SampleMap* sampleMap_);

	MonolithExporter(const String &name, ModulatorSynthChain* chain);

	~MonolithExporter()
	{
		fc = nullptr;
	}

	static void collectFiles()
	{

	}

	void run() override;

	void exportCurrentSampleMap(bool overwriteExistingData, bool exportSamples, bool exportSampleMap);

	void setSampleMap(SampleMap* samplemapToExport)
	{
		sampleMap = samplemapToExport;
	}

	void writeSampleMapFile(bool overwriteExistingFile);

	void threadFinished() override;;

	bool write(const int** /*data*/, int /*numSamples*/) override
	{
		jassertfalse;
		return false;
	}
    
	void setSilentMode(bool shouldShowMessage)
	{
		silentMode = shouldShowMessage;
	}

protected:
    
	void setError(const String& errorMessage)
	{
		error = errorMessage;
	}

    File sampleMapFile;

	File monolithDirectory;

private:

	bool silentMode = false;

	Array<int> splitIndexes;

	AudioFormatWriter* createWriter(hlac::HiseLosslessAudioFormat& hlaf, const File& f, bool isMono);

	/** The max monolith size is 2GB - 60MB (to guarantee to stay below 2GB for FAT32. */
	//constexpr static int maxMonolithSize = 2084569088;

	int64 getNumBytesForSplitSize() const;

	void checkSanity();

	File getNextMonolith(const File& f) const;

	/** Writes the files and updates the samplemap with the information. */
	void writeFiles(int channelIndex, bool overwriteExistingData);

	/** Checks whether the monolith needs to be split up. */
	bool shouldSplit(int channelIndex, int64 numBytesWritten, int sampleIndex) const;

	void updateSampleMap();

	int64 largestSample;

	ScopedPointer<FilenameComponent> fc;

	ValueTree v;
	SampleMap* sampleMap;
	SampleMap::FileList filesToWrite;
	int numChannels;
	int numSamples;
	File sampleMapDirectory;
	
	ScopedPointer<MonolithFileReference> monolithFileReference;


	int numMonolithSplitParts = -1;

	String error;
};



class BatchReencoder : public MonolithExporter,
	public ControlledObject
{
public:

	BatchReencoder(ModulatorSampler* s);;

	void run() override;

	void reencode(PoolReference r);

	void setReencodeSamples(bool shouldReencodeSamples)
	{
		reencodeSamples = shouldReencodeSamples;
	}

private:

	bool reencodeSamples = true;

	double wholeProgress = 0.0;

	WeakReference<ModulatorSampler> sampler;

};

#endif

} // namespace hise
#endif  // MODULATORSAMPLERDATA_H_INCLUDED
