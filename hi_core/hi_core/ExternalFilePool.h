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

#ifndef EXTERNALFILEPOOL_H_INCLUDED
#define EXTERNALFILEPOOL_H_INCLUDED

namespace hise { using namespace juce;

namespace MetadataIDs
{
const Identifier SampleRate("SampleRate");
const Identifier LoopEnabled("LoopEnabled");
const Identifier LoopStart("LoopStart");
const Identifier LoopEnd("LoopEnd");
}

/** A wrapper around any arbitrary data type with the same properties as the 
	other wrappers (shared instance, ref-counted, etc). */
template <class DataType> class SharedFileReference
{
public:

	SharedFileReference() :
		data(new SharedObject())
	{};

	SharedFileReference(const SharedFileReference& other) :
		data(other.data)
	{};

	bool isValid() const { return !getId().isNull(); };

	SharedFileReference& operator=(const SharedFileReference& other)
	{
		data = other.data;
		return *this;
	}

	DataType& getFile() const
	{
		return data->file;
	}

	Identifier getId() const
	{
		return data->id;
	}

	void setId(const Identifier& id)
	{
		data->id = id;
	}

private:

	struct SharedObject : public ReferenceCountedObject
	{
		DataType file;
		Identifier id;
	};

	ReferenceCountedObjectPtr<SharedObject> data;
};

using MidiFileReference = SharedFileReference<MidiFile>;
using AdditionalDataReference = SharedFileReference<String>;

class FileHandlerBase;
class PoolBase;

/** Helper functions for the file pools. 
*/
struct PoolHelpers
{
	/** If you load a file into the pool you can specify the caching behaviour with one of these. */
	enum LoadingType
	{
		LoadAndCacheWeak = 0, ///< Loads the file, but does not increase the reference count. This results in the file being unloaded if the reference to it gets destroyed. If the file is already loaded, it just returns the reference.
		LoadAndCacheStrong, ///< Loads the file and increases the reference count to extend the lifetime of the cached data until the pool is destroyed or manually cleaned.
		ForceReloadWeak, ///< This will bypass the search in the pool and reloads the file no matter if it was loaded before.
		ForceReloadStrong, ///< reloads the file and increases it's reference count.
		SkipPoolSearchWeak, ///< skips the check if the file is already pooled.
		SkipPoolSearchStrong, ///< skips the search in the pool and increases the reference count
		DontCreateNewEntry, ///< this assumes the file is already in the pool and throws an assertion if it's not.
		BypassAllCaches, ///< skips the entire caching and just returns the decoded data.
		LoadIfEmbeddedWeak, ///< loads the embedded data if it's there, but doesn't throw an error if it's not
		LoadIfEmbeddedStrong, ///< loads the embedded data if it's there, but doesn't throw an error if it's not
		numLoadingTypes
	};

	static bool isStrong(LoadingType t)
	{
		return t == LoadAndCacheStrong || t == ForceReloadStrong || t == SkipPoolSearchStrong || t == LoadIfEmbeddedStrong;
	}

	static bool throwIfNotLoaded(LoadingType t)
	{
		return t != LoadIfEmbeddedStrong && t != LoadIfEmbeddedWeak;
	}

	static bool shouldSearchInPool(LoadingType t)
	{
		return t == LoadAndCacheStrong || 
			   t == LoadAndCacheWeak || 
			   t == ForceReloadStrong || 
			   t == ForceReloadWeak || 
			   t == DontCreateNewEntry ||
			   t == LoadIfEmbeddedStrong ||
			   t == LoadIfEmbeddedWeak;
	}

	static bool shouldForceReload(LoadingType t)
	{
		return t == ForceReloadStrong || t == ForceReloadWeak;
	}

    static void sendErrorMessage(MainController* mc, const String& errorMessage);
    
	// Using an empty parameter to get the correct Subdirectory type
	static ProjectHandler::SubDirectories getSubDirectoryType(const AudioSampleBuffer& emptyData);
	static ProjectHandler::SubDirectories getSubDirectoryType(const Image& emptyImage);
	static ProjectHandler::SubDirectories getSubDirectoryType(const ValueTree& emptyTree);
	static ProjectHandler::SubDirectories getSubDirectoryType(const MidiFileReference& emptyTree);
	static ProjectHandler::SubDirectories getSubDirectoryType(const AdditionalDataReference& emptyTree);
	
	/** @internal (used by the template instantiations. */
	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, AudioSampleBuffer& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, Image& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode, ValueTree& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode,
		MidiFileReference& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void loadData(AudioFormatManager& afm, InputStream* ownedStream, int64 hashCode,
		AdditionalDataReference& data, var* additionalData);

	/** @internal (used by the template instantiations. */
	static void fillMetadata(AudioSampleBuffer& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void fillMetadata(Image& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void fillMetadata(ValueTree& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void fillMetadata(MidiFileReference& data, var* additionalData);
	/** @internal (used by the template instantiations. */
	static void fillMetadata(AdditionalDataReference& data, var* additionalData);

	/** @internal (used by the template instantiations. */
	static size_t getDataSize(const AudioSampleBuffer* buffer);
	/** @internal (used by the template instantiations. */
	static size_t getDataSize(const Image* img);
	/** @internal (used by the template instantiations. */
	static size_t getDataSize(const ValueTree* img);
	/** @internal (used by the template instantiations. */
	static size_t getDataSize(const MidiFileReference* midiFile);
	/** @internal (used by the template instantiations. */
	static size_t getDataSize(const AdditionalDataReference* midiFile);

	/** @internal (used by the template instantiations. */
	static bool isValid(const AudioSampleBuffer* buffer);
	/** @internal (used by the template instantiations. */
	static bool isValid(const Image* buffer);
	/** @internal (used by the template instantiations. */
	static bool isValid(const ValueTree* buffer);
	/** @internal (used by the template instantiations. */
	static bool isValid(const MidiFileReference* file);
	/** @internal (used by the template instantiations. */
	static bool isValid(const AdditionalDataReference* file);

	/** @internal (used by the template instantiations. */
	static Identifier getPrettyName(const AudioSampleBuffer* /*buffer*/) { RETURN_STATIC_IDENTIFIER("AudioFilePool"); }
	/** @internal (used by the template instantiations. */
	static Identifier getPrettyName(const Image* /*img*/) { RETURN_STATIC_IDENTIFIER("ImagePool"); }
	/** @internal (used by the template instantiations. */
	static Identifier getPrettyName(const ValueTree* /*img*/) { RETURN_STATIC_IDENTIFIER("SampleMapPool"); }
	static Identifier getPrettyName(const MidiFileReference* /*img*/) { RETURN_STATIC_IDENTIFIER("MidiFilePool"); }
	static Identifier getPrettyName(const AdditionalDataReference* /*img*/) { RETURN_STATIC_IDENTIFIER("AdditionalDataPool"); }

	static Image getEmptyImage(int width, int height);

	/** A lightweight object that encapsulates all different sources for a pool with a 
		hash code and a relative path system.
		
		@ingroup core
		
		Whenever you need to access external / embedded resources, this class is used to 
		resolve the path / ID to fetch the data. 
		
		Normally, you create one of these just with a String and it will figure out automatically
		whether it's a absolute path, a path relative to the project folder or an embedded resource.
	*/
	struct Reference
	{
		struct Comparator
		{
			int compareElements(const Reference& first, const Reference& second)
			{
				return first.reference.compare(second.reference);
			}
		};

		/** The data sources for the Reference. */
		enum Mode
		{
			Invalid = 0, ///< if the reference can't be resolved
			AbsolutePath, ///< the reference is an absolute path outside the HISE project folder. This needs to be avoided during development, but for compiled plugins it stores the location to a file specified by the user
			ExpansionPath, ///< if the resource is bundled in an expansion pack, it will be using this value
			ProjectPath, ///< a file within the HISE project folder. This will be transformed into a EmbeddedResource when compiling the plugin
			EmbeddedResource, ///< data that is either embedded in the plugin or shipped as compressed pool data along with the binary
			LinkToEmbeddedResource, ///< ???
			numModes_
		};

		/** Creates a Invalid reference. */
		Reference();

		/** Creates a reference using the given string. */
		Reference(const MainController* mc, const String& referenceStringOrFile, ProjectHandler::SubDirectories directoryType);

		/** Creates a reference from a drag description. */
		Reference(const var& dragDescription);

		/** Creates a reference from an embedded resource. */
		Reference(PoolBase* pool, const String& embeddedReference, ProjectHandler::SubDirectories directoryType);

		/** Returns a copy of the reference pointing to the given file handler. */
		Reference withFileHandler(FileHandlerBase* handler);

		/** This can be used to type shorter conditions. */
		explicit operator bool() const
		{
			return isValid();
		}

		bool operator ==(const Reference& other) const;
		bool operator !=(const Reference& other) const;

		/** Returns the type of the reference. */
		Mode getMode() const { return m; }

		/** Returns the String that was passed in the constructor. */
		String getReferenceString() const;

		/** Returns the Identifier as used by the pool. */
		Identifier getId() const;

		/** If this is a file based reference, it will return the file. */
		File getFile() const;

		bool isRelativeReference() const;
		bool isAbsoluteFile() const;
		bool isEmbeddedReference() const;;

		/** Tries to resolve the file reference given a root folder. */
		File resolveFile(FileHandlerBase* handler, FileHandlerBase::SubDirectories type) const;

		/** This creates an input stream for the reference. If it's file based, it will be a FileInputStream, and for embedded references you'll get a MemoryInputStream. */
	 	InputStream* createInputStream() const;

		/** Upon creation it creates a hash code that will be used by the pool to identify
			and return already cached data.
		*/
		int64 getHashCode() const;
		bool isValid() const;

		/** Returns the data type. A Reference has the data type baked in it (because you hardly want to load images into a convolution reverb). 
		*/
		ProjectHandler::SubDirectories getFileType() const;

		/** @internal. */
		var createDragDescription() const;

	private:

		void parseDragDescription(const var& v);
		void parseReferenceString(const MainController* mc, const String& input);

		String reference;
		File f;
		Identifier id;
		Mode m;

		int64 hashCode;

		PoolBase* pool;
		ProjectHandler::SubDirectories directoryType;
	};

};


class PoolCollection;

using PoolReference = PoolHelpers::Reference;


/** The base class for all resource pools.
*
*	This class handles the caching of resources and provides the data. */
class PoolBase : public ControlledObject
{
public:

	enum EventType
	{
		Added,
		Removed,
		Changed,
		Reloaded,
		numEventTypes
	};

	struct ScopedNotificationDelayer
	{
		ScopedNotificationDelayer(PoolBase& parent_, EventType type):
			parent(parent_),
			t(type)
		{
			parent.skipNotification = true;
		}

		~ScopedNotificationDelayer()
		{
			parent.skipNotification = false;
			parent.sendPoolChangeMessage(t, sendNotificationAsync);
		}

		EventType t;
		PoolBase& parent;
	};

	/** The internal data management class for embedded resources. */
	class DataProvider
	{
	public:

		class Compressor
		{
		public:

			virtual ~Compressor() {};

			virtual void write(OutputStream& output, const ValueTree& data, const File& originalFile) const;
			virtual void write(OutputStream& output, const Image& data, const File& originalFile) const;
			virtual void write(OutputStream& output, const AudioSampleBuffer& data, const File& originalFile) const;
			virtual void write(OutputStream& output, const MidiFileReference& data, const File& originalFile) const;
			virtual void write(OutputStream& output, const AdditionalDataReference& data, const File& originalFile) const;

			virtual void create(MemoryInputStream* mis, ValueTree* data) const;
			virtual void create(MemoryInputStream* mis, Image* data) const;
			virtual void create(MemoryInputStream* mis, AudioSampleBuffer* data) const;
			virtual void create(MemoryInputStream* mis, MidiFileReference* data) const;
			virtual void create(MemoryInputStream* mis, AdditionalDataReference* data) const;
		};

		DataProvider(PoolBase* pool_) :
			pool(pool_),
			metadataOffset(-1),
			compressor(new Compressor())
		{}

		virtual ~DataProvider() {}

		bool isEmbeddedResource(PoolReference r);

		PoolReference getEmbeddedReference(PoolReference other);

		virtual Result restorePool(InputStream* ownedInputStream);

		virtual MemoryInputStream* createInputStream(const String& referenceString);

		virtual Result writePool(OutputStream* ownedOutputStream, double* progress=nullptr);

		var createAdditionalData(PoolReference r);

		const Compressor* getCompressor() const { return compressor; };

		void setCompressor(Compressor* newCompressor) { compressor = newCompressor; };

		Array<PoolReference> getListOfAllEmbeddedReferences() const;

		size_t getSizeOfEmbeddedReferences() const { return embeddedSize; }

	private:

		ValueTree metadata;
		int64 metadataOffset;

		PoolBase* pool = nullptr;
		ScopedPointer<InputStream> input;
		Array<int64> hashCodes;
		size_t embeddedSize = 0;

		ScopedPointer<Compressor> compressor;
	};

	/** A interface class that will be notified about changes to the pool. 
	*
	*	If you want to reflect the state of a pool in your UI, subclass this and overwrite the callbacks.
	*	They will be asynchronously called after the state of the pool changes.
	*/
	class Listener
	{
	public:

		virtual ~Listener() {};

		/** Called whenever a pool entry was added. */
		virtual void poolEntryAdded() {};

		/** Called when a pool entry was removed. */
		virtual void poolEntryRemoved() {};

		/** If a pool entry gets changed, this will be called. */
		virtual void poolEntryChanged(PoolReference referenceThatWasChanged) {};


		virtual void poolEntryReloaded(PoolReference referenceThatWasChanged) {};

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener)
	};

	void sendPoolChangeMessage(EventType t, NotificationType notify=sendNotificationAsync, PoolReference r=PoolReference())
	{
		if (skipNotification && notify == sendNotificationAsync)
			return;

		lastType = t;
		lastReference = r;

		if (notify == sendNotificationAsync)
			notifier.triggerAsyncUpdate();
		else
			notifier.handleAsyncUpdate();
	}

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void setDataProvider(DataProvider* newDataProvider)
	{
		dataProvider = newDataProvider;
	}

	
	virtual int getNumLoadedFiles() const = 0;
	virtual PoolReference getReference(int index) const = 0;
	virtual void clearData() = 0;
	virtual var getAdditionalData(PoolReference r) const = 0;
	virtual StringArray getTextDataForId(int index) const = 0;

	virtual void writeItemToOutput(OutputStream& output, PoolReference r) = 0;

	DataProvider* getDataProvider() { return dataProvider; };
	const DataProvider* getDataProvider() const { return dataProvider; };

	FileHandlerBase::SubDirectories getFileType() const
	{
		return type;
	}

	void setUseSharedPool(bool shouldUse)
	{
		useSharedCache = shouldUse;
	}

	FileHandlerBase* getFileHandler() const { return parentHandler; }

protected:

	virtual Identifier getFileTypeName() const = 0;

	PoolBase(MainController* mc, FileHandlerBase* handler):
	  ControlledObject(mc),
	  notifier(*this),
      type(FileHandlerBase::SubDirectories::numSubDirectories),
	  dataProvider(new DataProvider(this)),
	  parentHandler(handler)
	{

	}

	FileHandlerBase::SubDirectories type;

	bool useSharedCache = false;

	FileHandlerBase* parentHandler = nullptr;

private:

	struct Notifier: public AsyncUpdater
	{
		Notifier(PoolBase& parent_) :
			parent(parent_)
		{};

		~Notifier()
		{
			cancelPendingUpdate();
		}

		void handleAsyncUpdate() override
		{
			ScopedLock sl(parent.listeners.getLock());
			
			for (auto& l : parent.listeners)
			{
				if (l != nullptr)
				{
					switch (parent.lastType)
					{
					case Added: l->poolEntryAdded(); break;
					case Removed: l->poolEntryRemoved(); break;
					case Changed: l->poolEntryChanged(parent.lastReference); break;
					case Reloaded: l->poolEntryReloaded(parent.lastReference); break;
					default:
						break;
					}
				}
			}
		}


		PoolBase& parent;

	};

	Notifier notifier;

	bool skipNotification = false;

	EventType lastType;
	PoolReference lastReference;

	Array<WeakReference<Listener>, CriticalSection> listeners;

	ScopedPointer<DataProvider> dataProvider;
	
	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PoolBase)
};


template <class DataType> class PoolEntry : public ReferenceCountedObject
{
public:

	PoolEntry(PoolReference& r) :
		ref(r),
		data(DataType())
	{};

	PoolEntry() :
		ref(PoolReference()),
		data(DataType())
	{};

	~PoolEntry()
	{
	}

	bool operator ==(const PoolEntry& other) const
	{
		return other.ref == ref;
	}

	explicit operator bool() const { return ref.isValid() && PoolHelpers::isValid(data); };

	PoolReference ref;
	DataType data;
	var additionalData;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(PoolEntry<DataType>);
};


/** This class extends the pool to a global data storage used across all instances of the plugin.
*
*	This is useful if you have a lot of read-only data (like images), which would increase the memory
*	usage when multiple instances of your plugin are used. */
template <class DataType> class SharedCache
{

public:

	SharedCache()
	{
		DataType* unused = nullptr;
		ignoreUnused(unused);
	}

	bool contains(int64 hashCode)
	{
		for (const auto& i : sharedItems)
		{
			if (i->ref.getHashCode() == hashCode)
				return true;
		}

		return false;
	}

	PoolEntry<DataType>* getSharedData(int64 hashCode)
	{
		for (const auto& i: sharedItems)
		{
			if (i->ref.getHashCode() == hashCode)
				return i;
		}

		jassertfalse;
		return {};
	}

	void store(PoolEntry<DataType>* newEntry)
	{
		if (contains(newEntry->ref.getHashCode()))
			return;

		sharedItems.add(newEntry);
	}

	~SharedCache()
	{
		DataType* unused = nullptr;
		ignoreUnused(unused);
	}

private:

	

	ReferenceCountedArray<PoolEntry<DataType>> sharedItems;
};





/** Implementations of the data pool. */
template <class DataType> class SharedPoolBase : public PoolBase, public InternalLogger
{
public:

	using PoolItem = PoolEntry<DataType>;

	/** A ManagedPtr is a wrapper around a reference in the pool. Normally you don't create them
	*	manually, but use the loadFromReference() method which creates and returns this object. 
	*/
	class ManagedPtr
	{
	public:

		ManagedPtr(SharedPoolBase<DataType>* pool_, PoolItem* object, bool refCounted) :
			pool(pool_),
			isRefCounted(refCounted),
			weak(refCounted ? nullptr : object),
			strong(refCounted ? object : nullptr)
		{
			
		}

		ManagedPtr() :
			pool(nullptr),
			weak(nullptr),
			strong(nullptr),
			isRefCounted(true)
		{

		}

		ManagedPtr& operator= (const ManagedPtr& other)
		{
			if(isRefCounted)
				clear();

			pool = other.pool;
			weak = other.weak;
			strong = other.strong;
			isRefCounted = other.isRefCounted;

			return *this;
		}

		bool operator==(const ManagedPtr& other) const
		{
			return getRef() == other.getRef();
		}

		explicit operator bool() const { return get() != nullptr;};

		PoolItem* get() { return isRefCounted ? strong.get() : weak.get(); };
		const PoolItem* get() const { return isRefCounted ? strong.get() : weak.get(); };

		operator PoolItem*() const noexcept { return get(); }

		PoolItem* operator->() noexcept { return get(); }

		const PoolItem* operator->() const noexcept { return get(); }

		~ManagedPtr()
		{
			weak = nullptr;

			if (isRefCounted)
				clear();
		}

		StringArray getTextData() const
		{
			StringArray sa;

			if (get() != nullptr)
			{
				sa.add(getRef().getReferenceString());
				auto dataSize = PoolHelpers::getDataSize(getData());

				String s = String((float)dataSize / 1024.0f, 1) + " kB";

				sa.add(s);
				sa.add(String(get()->getReferenceCount()));
			}

			return sa;
		}

		const DataType* getData() const { return *this ? &get()->data : nullptr; };
		DataType* getData() { return *this ? &get()->data : nullptr; };
		
		PoolReference getRef() const { return *this ? get()->ref : PoolReference(); };

		var getAdditionalData() const { return *this ? get()->additionalData : var(); }

		void clear()
		{
			if (pool.get() == nullptr)
				return;

			if (!getRef())
				return;

			jassert(isRefCounted);
			
			if(*this)
				pool->releaseIfUnused(*this);
		}

		void clearStrongReference()
		{
			strong = nullptr;
			isRefCounted = false;
		}

	private:

		bool isRefCounted;

		WeakReference<SharedPoolBase<DataType>> pool;
		ReferenceCountedObjectPtr<PoolEntry<DataType>> strong;
		WeakReference<PoolEntry<DataType>> weak;
	};

	SharedPoolBase(MainController* mc_, FileHandlerBase* handler) :
      PoolBase(mc_, handler)
	{
        type = PoolHelpers::getSubDirectoryType(empty);
        
		if (type == FileHandlerBase::SubDirectories::AudioFiles)
		{
			afm.registerBasicFormats();
			afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);
		}

	};


	~SharedPoolBase()
	{
		clearData();
	};

	Identifier getFileTypeName() const override
	{
		return ProjectHandler::getIdentifier(type);
	}

	/** Returns the number of loaded files. */
	int getNumLoadedFiles() const override
	{
		return weakPool.size();
	}

	/** Clears the pool. */
	void clearData() override
	{
		ScopedNotificationDelayer snd(*this, EventType::Removed);

		refCountedPool.clear();

		weakPool.clear();
		allFilesLoaded = false;
		sendPoolChangeMessage(Removed);
	}

	void refreshPoolAfterUpdate(PoolReference r=PoolReference())
	{
		if (r.isValid())
		{
			loadFromReference(r, PoolHelpers::ForceReloadStrong);
		}
		else
		{
			clearData();
			loadAllFilesFromProjectFolder();
		}

		
	}

	/** Checks if the hash code is used by the pool. Use this method with the hash code of a PoolReference. */
	bool contains(int64 hashCode) const
	{
		for (int i = 0; i < getNumLoadedFiles(); i++)
		{
			if (weakPool.getReference(i).getRef().getHashCode() == hashCode)
				return true;
		}

		return false;
	}

	/** Returns the PoolReference at the given index. */
	PoolReference getReference(int index) const override
	{
		if (index >= 0 && index < getNumLoadedFiles())
			return weakPool.getReference(index).getRef();

		return PoolReference();
	}

	int indexOf(PoolReference ref) const
	{
		for (int i = 0; i < getNumLoadedFiles(); i++)
		{
			if (weakPool.getReference(i).getRef() == ref)
				return i;
		}

		return -1;
	}

	/** Every pool item has a storage object for additional metadata (eg. the sample rate for audio files). */
	var getAdditionalData(PoolReference r) const override
	{
		auto i = indexOf(r);

		if (i >= 0)
			return weakPool.getReference(i).getAdditionalData();

		return {};
	}

	/** Creates a list of all cached data. */
	Array<PoolReference> getListOfAllReferences(bool includeEmbeddedButUnloadedReferences) const
	{
		Array<PoolReference> references;

		for (int i = 0; i < getNumLoadedFiles(); i++)
			references.add(getReference(i));

		if (includeEmbeddedButUnloadedReferences)
		{
			auto additionalRefs = getDataProvider()->getListOfAllEmbeddedReferences();

			for (const auto& r : additionalRefs)
				references.addIfNotAlreadyThere(r);
		}

		return references;
	}

	StringArray getTextDataForId(int index) const override
	{
		if (index >= 0 && index < getNumLoadedFiles())
			return weakPool.getReference(index).getTextData();

		return {};
	}

	void loadAllFilesFromDataProvider()
	{
		allFilesLoaded = true;
		ScopedNotificationDelayer snd(*this, EventType::Added);

		auto refList = getDataProvider()->getListOfAllEmbeddedReferences();

		for (auto r : refList)
			loadFromReference(r, PoolHelpers::LoadAndCacheStrong);
	}

	void loadAllFilesFromProjectFolder()
	{
		refCountedPool.clear();
		weakPool.clear();

		ScopedNotificationDelayer snd(*this, EventType::Added);

		auto fileList = parentHandler->getFileList(type, false, true);

		ScopedValueSetter<bool> svs(useSharedCache, false);

		for (auto f : fileList)
		{
			PoolReference ref(getMainController(), f.getFullPathName(), type);

			loadFromReference(ref, PoolHelpers::LoadAndCacheStrong);
		}

		allFilesLoaded = true;
	}

	bool areAllFilesLoaded() const noexcept { return allFilesLoaded; }

	/** Returns a statistic string with the size and memory usage of the pool. */
	String getStatistics() const
	{
		String s;

		s << "Size: " << weakPool.size();

		size_t dataSize = 0;

		for (const auto& d : weakPool)
		{
			if (d)
				dataSize += PoolHelpers::getDataSize(d.getData());
		}

		s << " (" << String(dataSize / 1024.0f / 1024.0f, 2) << " MB)";

		return s;
	}

	StringArray getIdList() const
	{
		StringArray sa;

		for (const auto& d : weakPool)
			sa.add(d.getRef().getReferenceString());

		return sa;
	}

	void releaseIfUnused(ManagedPtr& mptr)
	{
		jassert(mptr);
		jassert(mptr.getRef().isValid());

		PoolReference r(mptr.getRef());

		for (int i = 0; i < weakPool.size(); i++)
		{
			if (weakPool.getReference(i) == mptr)
			{
				mptr.clearStrongReference();

				if (weakPool.getReference(i).get() == nullptr)
				{
					weakPool.remove(i--);
					sendPoolChangeMessage(PoolBase::EventType::Removed, sendNotificationAsync, r);
				}
				else
					sendPoolChangeMessage(PoolBase::EventType::Changed, sendNotificationAsync, r);

				return;
			}
		}
	}

	

	void writeItemToOutput(OutputStream& output, PoolReference r)
	{
		auto d = getWeakReferenceToItem(r);

		if (d)
		{
			auto ref = d.getRef();

			File original;

			if (!ref.isEmbeddedReference())
				original = ref.getFile();

			getDataProvider()->getCompressor()->write(output, *d.getData(), original);
		}
			
	}

	ManagedPtr getWeakReferenceToItem(PoolReference r)
	{
		auto index = indexOf(r);

		if (index != -1)
			return ManagedPtr(this, weakPool.getReference(index).get(), false);

		jassertfalse;
		return ManagedPtr();
	}

	ManagedPtr createAsEmbeddedReference(PoolReference r, DataType t)
	{
		ReferenceCountedObjectPtr<PoolItem> ne = new PoolItem(r);

		ne->data = t;

		PoolHelpers::fillMetadata(ne->data, &ne->additionalData);

		refCountedPool.add(ManagedPtr(this, ne.get(), true));
		weakPool.add(ManagedPtr(this, ne.get(), false));

		return ManagedPtr(this, ne.get(), true);
	}

	/** Loads a reference with the given LoadingType. Use this whenever you need to access data,
		as it will check if the data was already cached. 
	*/
	ManagedPtr loadFromReference(PoolReference r, PoolHelpers::LoadingType loadingType)
	{
		if (getDataProvider()->isEmbeddedResource(r))
			r = getDataProvider()->getEmbeddedReference(r);

		if (useSharedCache && sharedCache->contains(r.getHashCode()))
		{
			return ManagedPtr(this, sharedCache->getSharedData(r.getHashCode()), true);
		}

		if (PoolHelpers::shouldSearchInPool(loadingType))
		{
			int index = indexOf(r);

			if (index != -1)
			{
				auto& d = weakPool.getReference(index);

				jassert(d);

				if (PoolHelpers::shouldForceReload(loadingType))
				{
					jassert(!r.isEmbeddedReference());

					if (auto inputStream = r.createInputStream())
					{
                        auto ad = d.getAdditionalData();
                        jassert(ad.isObject());
                        
						PoolHelpers::loadData(afm, inputStream, r.getHashCode(), *d.getData(), &ad);

						sendPoolChangeMessage(PoolBase::Reloaded, sendNotificationSync, r);
						return ManagedPtr(this, d.get(), true);
					}
					else
					{
						logMessage(getMainController(), r.getReferenceString() + " wasn't found.");

						jassertfalse; // This shouldn't happen...
						return ManagedPtr();
					}
				}
				else
				{
					sendPoolChangeMessage(PoolBase::Changed, sendNotificationAsync, r);
					return ManagedPtr(this, d.get(), true);
				}
			}
		}

		if (loadingType == PoolHelpers::DontCreateNewEntry)
		{
			jassertfalse;
			return ManagedPtr();
		}

		ReferenceCountedObjectPtr<PoolItem> ne = new PoolItem(r);
		
		if (r.isEmbeddedReference())
		{
			if (auto mis = getDataProvider()->createInputStream(r.getReferenceString()))
			{
				getDataProvider()->getCompressor()->create(mis, &ne->data);

				ne->additionalData = getDataProvider()->createAdditionalData(r);


				if (loadingType != PoolHelpers::BypassAllCaches)
				{
					if (useSharedCache)
					{
						sharedCache->store(ne.get());
					}
					else
					{
						weakPool.add(ManagedPtr(this, ne.get(), false));
						refCountedPool.add(ManagedPtr(this, ne.get(), true));
					}
				}

				sendPoolChangeMessage(PoolBase::Added);

				return ManagedPtr(this, ne.get(), true);
			}
			else
			{
				if (PoolHelpers::throwIfNotLoaded(loadingType))
				{
					//jassertfalse;

					//PoolHelpers::sendErrorMessage(getMainController(), "The file " + r.getReferenceString() + " is not embedded correctly.");
				}

				return ManagedPtr();
			}
		}
		else
		{
			if (auto inputStream = r.createInputStream())
			{
				PoolHelpers::loadData(afm, inputStream, r.getHashCode(), ne->data, &ne->additionalData);


				if (useSharedCache && loadingType != PoolHelpers::LoadAndCacheStrong)
				{
					sharedCache->store(ne.get());
				}
				else
				{
					weakPool.add(ManagedPtr(this, ne.get(), false));

					if (PoolHelpers::isStrong(loadingType))
						refCountedPool.add(ManagedPtr(this, ne.get(), true));
				}
				
				sendPoolChangeMessage(PoolBase::Added);

				return ManagedPtr(this, ne.get(), true);
			}
			else
			{
				logMessage(getMainController(), r.getReferenceString() + " wasn't found.");
				return ManagedPtr();
			}
		}
	}

	

private:

	bool allFilesLoaded = false;

	SharedResourcePointer<SharedCache<DataType>> sharedCache;

	DataType empty;

	Array<ManagedPtr> weakPool;
	Array<ManagedPtr> refCountedPool;


	ProjectHandler::SubDirectories type;

	AudioFormatManager afm;

	JUCE_DECLARE_WEAK_REFERENCEABLE(SharedPoolBase<DataType>);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SharedPoolBase)
};

using AudioSampleBufferPool = SharedPoolBase<AudioSampleBuffer>;
using ImagePool = SharedPoolBase<Image>;

using PooledAudioFile = AudioSampleBufferPool::ManagedPtr;
using PooledImage = ImagePool::ManagedPtr;

using SampleMapPool = SharedPoolBase<ValueTree>;
using PooledSampleMap = SampleMapPool::ManagedPtr;

using MidiFilePool = SharedPoolBase<MidiFileReference>;
using PooledMidiFile = MidiFilePool::ManagedPtr;

using AdditionalDataPool = SharedPoolBase<AdditionalDataReference>;
using PooledAdditionalData = AdditionalDataPool::ManagedPtr;

class FileHandlerBase;
class ModulatorSamplerSoundPool;

/** This provider handles the loading of pooled audio files. */
struct PooledAudioFileDataProvider : public hise::MultiChannelAudioBuffer::DataProvider,
	public ControlledObject
{
	PooledAudioFileDataProvider(MainController* mc) :
		ControlledObject(mc)
	{}

	MultiChannelAudioBuffer::SampleReference::Ptr loadFile(const String& ref) override;

	File getRootDirectory() override;

	void setRootDirectory(const File& rootDirectory) override
	{
		customDefaultFolder = rootDirectory;
	}

private:

	File customDefaultFolder;

	FileHandlerBase* getFileHandlerBase(const String& wildcard);

	FileHandlerBase* lastHandler = nullptr;
};

class PoolCollection: public ControlledObject
{
public:

	PoolCollection(MainController* mc, FileHandlerBase* handler);;

	~PoolCollection();

	void clear();

	template<class DataType> SharedPoolBase<DataType>* getPool()
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());
		jassert(dataPools[type] != nullptr);
		return static_cast<SharedPoolBase<DataType>*>(dataPools[type]);
	}

	template<class DataType> const SharedPoolBase<DataType>* getPool() const
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());
		jassert(dataPools[type] != nullptr);
		return static_cast<SharedPoolBase<DataType>*>(dataPools[type]);
	}

	const AudioSampleBufferPool& getAudioSampleBufferPool() const;
	AudioSampleBufferPool& getAudioSampleBufferPool();

	const ImagePool& getImagePool() const;
	ImagePool& getImagePool();

	const AdditionalDataPool& getAdditionalDataPool() const;
	AdditionalDataPool& getAdditionalDataPool();

	const SampleMapPool& getSampleMapPool() const;
	SampleMapPool& getSampleMapPool();

	const MidiFilePool& getMidiFilePool() const;
	MidiFilePool& getMidiFilePool();

	const ModulatorSamplerSoundPool* getSamplePool() const;
	ModulatorSamplerSoundPool* getSamplePool();

	AudioFormatManager afm;

	PoolBase* getPoolBase(FileHandlerBase::SubDirectories fileType)
	{
		return dataPools[(int)fileType];
	}

private:

	PoolBase * dataPools[(int)ProjectHandler::SubDirectories::numSubDirectories];

	FileHandlerBase* parentHandler = nullptr;

	JUCE_DECLARE_WEAK_REFERENCEABLE(PoolCollection);
};

template <class DataType> class PoolDropTarget : public DragAndDropTarget
{
protected:

	PoolDropTarget(MainController* mc_):
		mc(mc_),
		type(PoolHelpers::getSubDirectoryType(DataType()))
	{
	}

public:

	/** Overwrite this method and load the given reference. */
	virtual void poolItemWasDropped(PoolReference ref) = 0;

	bool hasHoveringPoolItem() const { return over; };

private:

	MainController * mc;

	FileHandlerBase::SubDirectories type;

	bool over = false;
};



class EncryptedCompressor : public PoolBase::DataProvider::Compressor
{
public:



	EncryptedCompressor(BlowFish* ownedKey);

	virtual ~EncryptedCompressor() {};

	void encrypt(MemoryBlock&& mb, OutputStream& output) const;


	/** SampleMaps ==================================================== */

	void write(OutputStream& output, const ValueTree& data, const File& originalFile) const override;
	void create(MemoryInputStream* mis, ValueTree* data) const override;

	/** Images ==================================================== */

	void write(OutputStream& output, const Image& data, const File& originalFile) const override;
	void create(MemoryInputStream* mis, Image* data) const override;

	/** AudioFiles ==================================================== */

	void write(OutputStream& output, const AudioSampleBuffer& data, const File& originalFile) const override;
	void create(MemoryInputStream* mis, AudioSampleBuffer* data) const override;

	/** MidiFiles ==================================================== */

	void write(OutputStream& output, const MidiFileReference& data, const File& originalFile) const override;
	void create(MemoryInputStream* mis, MidiFileReference* data) const override;

	/** AdditionalData ==================================================== */

	void write(OutputStream& output, const AdditionalDataReference& data, const File& originalFile) const override;
	void create(MemoryInputStream* mis, AdditionalDataReference* data) const override;

	ScopedPointer<BlowFish> key;
};


} // namespace hise

#endif  // EXTERNALFILEPOOL_H_INCLUDED
