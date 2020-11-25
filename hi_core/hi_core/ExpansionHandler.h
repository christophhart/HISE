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

#ifndef EXPANSION_HANDLER_H_INCLUDED
#define EXPANSION_HANDLER_H_INCLUDED

namespace hise {
using namespace juce;

class FloatingTile;

#define DECLARE_ID(x) static const Identifier x(#x);

namespace ExpansionIds
{
DECLARE_ID(ExpansionInfo);
DECLARE_ID(FullData);
DECLARE_ID(Preset);
DECLARE_ID(Scripts);
DECLARE_ID(Script);
DECLARE_ID(HeaderData);
DECLARE_ID(Fonts);
DECLARE_ID(Icon);
DECLARE_ID(HiseVersion);
DECLARE_ID(Credentials);
DECLARE_ID(PrivateInfo);
DECLARE_ID(Name);
DECLARE_ID(ProjectName);
DECLARE_ID(Version);
DECLARE_ID(Tags);
DECLARE_ID(Key);
DECLARE_ID(Hash);
DECLARE_ID(PoolData);
DECLARE_ID(Data);
DECLARE_ID(URL);
DECLARE_ID(UUID);
}

#undef DECLARE_ID

class Expansion: public FileHandlerBase
{
public:

	enum ExpansionType
	{
		FileBased,
		Intermediate,
		Encrypted,
		numExpansionType
	};

	Expansion(MainController* mc, const File& expansionFolder) :
		FileHandlerBase(mc),
		root(expansionFolder)
	{
		afm.registerBasicFormats();
		afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

		
	}

	struct Sorter
	{
		static int compareElements(Expansion* first, Expansion* second)
		{
			return first->getProperty(ExpansionIds::Name).compare(second->getProperty(ExpansionIds::Name));
		};
	};

	~Expansion()
	{
		saveExpansionInfoFile();
	}

	/** Override this method and initialise the expansion. You need to do at least those things:
	
		- create a Data object
	    - setup the pools
	*/
	virtual Result initialise() 
	{
		addMissingFolders();

		checkSubDirectories();

		pool->getSampleMapPool().loadAllFilesFromProjectFolder();
		pool->getMidiFilePool().loadAllFilesFromProjectFolder();

		data = new Data(root, Helpers::loadValueTreeForFileBasedExpansion(root));

		saveExpansionInfoFile();

		return Result::ok();
	};



	struct Helpers
	{
		static ValueTree loadValueTreeForFileBasedExpansion(const File& root);;

		static bool isXmlFile(const File& f);

		template <class T> static void initCachedValue(ValueTree v, const T& cachedValue)
		{
			if (!v.hasProperty(cachedValue.getPropertyID()))
			{
				v.setProperty(cachedValue.getPropertyID(), cachedValue.getDefault(), nullptr);
			}
		}

		static File getExpansionInfoFile(const File& expansionRoot, ExpansionType type);

		static String getExpansionIdFromReference(const String& referenceId);

		static String getExpansionTypeName(ExpansionType e);

	};


	virtual void encodeExpansion()
	{
		PresetHandler::showMessageWindow("Can't encode Expansion", "You haven't set a encryption key yet", PresetHandler::IconType::Error);
	}

	var getPropertyObject() const
	{
		return data->toPropertyObject();
	}

	Array<SubDirectories> getSubDirectoryIds() const override
	{
		return { AdditionalSourceCode, Images, AudioFiles, SampleMaps, MidiFiles, Samples, UserPresets };
	}

	virtual ExpansionType getExpansionType() const { return ExpansionType::FileBased; }

	static ExpansionType getExpansionTypeFromFolder(const File& f);

	File getRootFolder() const override { return root; }

	virtual PoolReference createReferenceForFile(const String& relativePath, FileHandlerBase::SubDirectories fileType);

	PooledAudioFile loadAudioFile(const PoolReference& audioFileId);

	PooledImage loadImageFile(const PoolReference& imageId);

	PooledAdditionalData loadAdditionalData(const String& relativePath);


#if 0
	ValueTree getSampleMap(const String& sampleMapId)
	{
		if (isEncrypted)
		{
			jassertfalse;
			return ValueTree();
		}
		else
		{
			Array<File> sampleMapFiles;
			getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).findChildFiles(sampleMapFiles, File::findFiles, true, "*.xml");

			String expStart = "{EXP::" + name.get() + "}";

			for (auto f : sampleMapFiles)
			{
				String thisId = expStart + f.getFileNameWithoutExtension();

				if (thisId == sampleMapId)
				{
					ScopedPointer<XmlElement> xml = XmlDocument::parse(f);

					if (xml != nullptr)
					{
						return ValueTree::fromXml(*xml);
					}
				}
			}

			throw String("!Sample Map with id " + sampleMapId.fromFirstOccurrenceOf("}", false, false) + " not found");
		}
	}
#endif

	bool isActive() const noexcept { return numActiveReferences != 0; }

	void incActiveRefCount() 
	{ 
		numActiveReferences++; 
	}
	void decActiveRefCount() 
	{
		jassert(numActiveReferences > 0);

		numActiveReferences = jmax(0, numActiveReferences - 1);
	}

	String getProperty(const Identifier& id) const
	{
		if(data != nullptr)
			return data->v.getProperty(id).toString();

		jassertfalse;
		return {};

	}

	void saveExpansionInfoFile();

	String getWildcard() const
	{
		String s;
		s << "{EXP::" << getProperty(ExpansionIds::Name) << "}";
		return s;
	}

	ValueTree getPropertyValueTree();

protected:

	File root;

	struct Data
	{
		

		Data(const File& root, ValueTree expansionInfo) :
			v(expansionInfo),
			name(v, "Name", nullptr, root.getFileNameWithoutExtension()),
#if USE_BACKEND
			projectName(v, "ProjectName", nullptr, "unused"),
			projectVersion(v, "ProjectName", nullptr, "1.0.0"),
			tags(v, "Tags", nullptr, ""),
#else
			projectName(v, "ProjectName", nullptr, FrontendHandler::getProjectName()),
			projectVersion(v, "ProjectName", nullptr, FrontendHandler::getVersionString()),
			tags(v, "Tags", nullptr, ""),
#endif
			version(v, "Version", nullptr, "1.0.0")
		{
			Helpers::initCachedValue(v, name);
			Helpers::initCachedValue(v, version);
			Helpers::initCachedValue(v, projectName);
			Helpers::initCachedValue(v, projectVersion);
			Helpers::initCachedValue(v, tags);
		}

		var toPropertyObject() const;

		virtual ~Data() {};

		ValueTree v;

		CachedValue<String> name;
		CachedValue<String> projectName;
		CachedValue<String> version;
		CachedValue<String> projectVersion;
		CachedValue<String> tags;
	};

	ScopedPointer<Data> data;

	int numActiveReferences = 0;

	AudioFormatManager afm;

	Array<SubDirectories> getListOfPooledSubDirectories()
	{
		Array<SubDirectories> sub;
		sub.add(AdditionalSourceCode);
		sub.add(AudioFiles);
		sub.add(Images);
		sub.add(MidiFiles);
		sub.add(SampleMaps);
		
		return sub;
	}

	

	void addMissingFolders()
	{
		addFolder(ProjectHandler::SubDirectories::Samples);

		if (getExpansionType() != FileBased)
			return;

		addFolder(ProjectHandler::SubDirectories::AdditionalSourceCode);
		addFolder(ProjectHandler::SubDirectories::AudioFiles);
		addFolder(ProjectHandler::SubDirectories::Images);
		addFolder(ProjectHandler::SubDirectories::SampleMaps);
		addFolder(ProjectHandler::SubDirectories::MidiFiles);
		addFolder(ProjectHandler::SubDirectories::UserPresets);
	}

	void addFolder(ProjectHandler::SubDirectories directory)
	{
		jassert(getExpansionType() == FileBased || directory == Samples);

		auto d = root.getChildFile(ProjectHandler::getIdentifier(directory));

		subDirectories.add({ directory, false, d });

		if (!d.isDirectory())
			d.createDirectory();
	}

	
	friend class ExpansionHandler;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(Expansion)

};


class ExpansionHandler: public hlac::HlacArchiver::Listener
{
public:

	struct Disabled
	{
		Disabled(MainController*, const File& ) {};

		virtual ~Disabled() {};
	};

	struct Reference
	{
		String referenceString;
	};

	class Listener
	{
	public:

		virtual ~Listener()
		{}

		/** This will be called whenever a expansion pack was created. 
		
			Normally, this will be called once at initialisation, but it might be possible that your project
			will add more expansions on runtime so this callback can be used to add the new expansion.

			Be aware that this call supersedes the expansionPackLoaded call (so if an expansion is being
			created, it will call this instead of expansionPackLoaded asynchronously.
		*/
		virtual void expansionPackCreated(Expansion* newExpansion) { expansionPackLoaded(newExpansion); };

		/** This will be called whenever an expansion pack was loaded. 
		
			Loading an expansion pack is not the only way of accessing its content, it is just telling
			that it is supposed to be the "active" expansion.
		*/
		virtual void expansionPackLoaded(Expansion* currentExpansion) 
		{
			ignoreUnused(currentExpansion);
		};

		/** Can be used to handle error messages. */
		virtual void logMessage(const String& message, bool isCritical) 
		{
			ignoreUnused(message, isCritical);
		}

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	struct InitialisationError
	{
		bool operator==(const InitialisationError& other) const
		{
			return other.e == e;
		};

		WeakReference<Expansion> e;
		Result r;
	};

#if USE_BACKEND
	struct ScopedProjectExporter: public ControlledObject
	{
		ScopedProjectExporter(MainController* mc, bool shouldDoSomething);

		~ScopedProjectExporter();

		bool doSomething = false;
		File expFolder;
		bool wasEnabled = false;
	};
#endif

	ExpansionHandler(MainController* mc);

	struct Helpers
	{
		static void createFrontendLayoutWithExpansionEditing(FloatingTile* root, bool putInTabWithMainInterface=true);
		static bool isValidExpansion(const File& directory);
		static Identifier getSanitizedIdentifier(const String& idToSanitize);

		static bool equalJSONData(var first, var second);

		static int64 getJSONHash(var obj);
	};

	void createNewExpansion(const File& expansionFolder);
	File getExpansionFolder() const;
	bool createAvailableExpansions();

	bool forceReinitialisation();

	Expansion* createExpansionForFile(const File& f);

	var getListOfAvailableExpansions() const;

	const OwnedArray<Expansion>& getListOfUnavailableExpansions() const;

	bool setCurrentExpansion(const String& expansionName);

	void setCurrentExpansion(Expansion* e, NotificationType notifyListeners = NotificationType::sendNotificationAsync);

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	bool installFromResourceFile(const File& f);

	PooledAudioFile loadAudioFileReference(const PoolReference& sampleId);

	double getSampleRateForFileReference(const PoolReference& sampleId);

	const var getMetadata(const PoolReference& sampleId);

	PooledImage loadImageReference(const PoolReference& imageId, PoolHelpers::LoadingType loadingType = PoolHelpers::LoadAndCacheWeak);

	PooledSampleMap loadSampleMap(const PoolReference& sampleMapId);

	bool isActive() const { return currentExpansion != nullptr; }


	Expansion* getExpansionForWildcardReference(const String& stringToTest) const;

	int getNumExpansions() const { return isEnabled() ? expansionList.size() : 0; }


	Expansion* getExpansionFromRootFile(const File& expansionRoot) const
	{
		for (auto& e : expansionList)
		{
			if (e->getRootFolder() == expansionRoot)
				return e;
		}

		return nullptr;
	}

	Expansion* getExpansionFromName(const String& name) const
	{
		for (auto e : expansionList)
		{
			if (e->data->name == name)
				return e;
		}

		return nullptr;
	}

	Expansion* getExpansion(int index) const
	{
		return expansionList[index];
	}

	Expansion* getCurrentExpansion() const { return currentExpansion.get(); }

	template <class DataType> SharedPoolBase<DataType>* getCurrentPool()
	{
		return getCurrentPoolCollection()->getPool<DataType>();
	}

    PoolCollection* getCurrentPoolCollection();
    
	void clearPools();

	bool& getNotifierFlag()
	{
		return notifier.enabled;
	}
    
	void setCredentials(var newCredentials)
	{
		if (!Helpers::equalJSONData(credentials, newCredentials))
		{
			credentials = newCredentials;
			forceReinitialisation();
		}
	}

	bool setErrorMessage(const String& message, bool isCritical)
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->logMessage(message, isCritical);
		}

		return false;
	}

	void setEncryptionKey(const String& newKey)
	{
		if (keyCode != newKey)
		{
			keyCode = newKey;
			forceReinitialisation();
		}
	}

	var getCredentials() const { return credentials; }

	String getEncryptionKey() const { return keyCode; }

	bool isEnabled() const noexcept { return enabled; };

	Array<Expansion::ExpansionType> getAllowedExpansionTypes() const { return allowedExpansions; };

	void setAllowedExpansions(const Array<Expansion::ExpansionType>& newAllowedExpansions)
	{
		allowedExpansions.clear();
		allowedExpansions.addArray(newAllowedExpansions);
		forceReinitialisation();
	}

	/** Call this method to set the expansion type you want to create. */
	template <class T> void setExpansionType()
	{
		if (std::is_same<T, Disabled>())
		{
			enabled = false;

			expansionCreateFunction = [](const File& f)
			{
				return nullptr;
			};
		}
		else
		{
			enabled = true;

			expansionCreateFunction = [&](const File& f)
			{
				auto e = new T(mc, f);
				return dynamic_cast<Expansion*>(e);
			};
		}
	}

#if HISE_USE_CUSTOM_EXPANSION_TYPE
	// Implement this method and return your custom C++ expansion class
	Expansion* createCustomExpansion(const File& f);
#endif
	
	Array<InitialisationError> initialisationErrors;

private:

	void logStatusMessage(const String& message);;

	void logVerboseMessage(const String&) {};

	void criticalErrorOccured(const String& message);;

	mutable File expansionFolder;

	Array<Expansion::ExpansionType> allowedExpansions;

	bool rebuildUnitialisedExpansions();

	bool enabled = true;

	String keyCode;
	var credentials;

	void checkAllowedExpansions(Result& r, Expansion* e);

	std::function<Expansion*(const File& f)> expansionCreateFunction;

	FileHandlerBase* getFileHandler(MainController* mc);
    
	template <class DataType> void getPoolForReferenceString(const PoolReference& p, SharedPoolBase<DataType>** pool)
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());

		ignoreUnused(p, type);
		jassert(p.getFileType() == type);
		
		if (auto e = getExpansionForWildcardReference(p.getReferenceString()))
		{
			*pool = e->pool->getPool<DataType>();
		}
		else
			*pool = getFileHandler(mc)->pool->getPool<DataType>();
	}

	struct Notifier : private AsyncUpdater
	{
	public:
		enum class EventType
		{
			Nothing,
			ExpansionLoaded,
			ExpansionCreated,
			numModes
		};

		Notifier(ExpansionHandler& parent_) :
			parent(parent_)
		{};

		void sendNotification(EventType eventType, NotificationType notificationType = sendNotificationAsync)
		{
			if (!enabled)
				return;

			if ((int)eventType > (int)m)
				m = eventType;

			if (notificationType == sendNotificationAsync)
			{
				IF_NOT_HEADLESS(triggerAsyncUpdate());
			}
			else if (notificationType == sendNotificationSync)
			{
				handleAsyncUpdate();
			}
		}

		bool enabled = true;

	private:

		void handleAsyncUpdate()
		{
			for (int i = 0; i < parent.listeners.size(); i++)
			{
				auto l = parent.listeners[i];

				if (l.get() != nullptr)
				{
					if (m == EventType::ExpansionLoaded)
						l->expansionPackLoaded(parent.currentExpansion);
					else
						l->expansionPackCreated(parent.currentExpansion);
				}
			}

			m = EventType::Nothing;

			return;
		}

		ExpansionHandler& parent;

		EventType m = EventType::Nothing;
	};

	Notifier notifier;

	Array<WeakReference<Listener>, CriticalSection> listeners;

	OwnedArray<Expansion> expansionList;
	OwnedArray<Expansion> uninitialisedExpansions;

	WeakReference<Expansion> currentExpansion;

	MainController * mc;
};

#define SET_CUSTOM_EXPANSION_TYPE(ExpansionClass) hise::Expansion* hise::ExpansionHandler::createCustomExpansion(const File& f) { return new ExpansionClass(mc, f); };

}

#endif

