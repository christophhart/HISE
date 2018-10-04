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



class Expansion: public FileHandlerBase
{
public:

	Expansion(MainController* mc, const File& expansionFolder) :
		FileHandlerBase(mc),
		root(expansionFolder),
		v(loadValueTree()),
		name(v, "Name", nullptr, expansionFolder.getFileNameWithoutExtension()),
#if USE_BACKEND
		projectName(v, "ProjectName", nullptr, "unused"),
		projectVersion(v, "ProjectName", nullptr, "1.0.0"),
#else
		projectName(v, "ProjectName", nullptr, FrontendHandler::getProjectName()),
		projectVersion(v, "ProjectName", nullptr, FrontendHandler::getVersionString()),
#endif
		version(v, "Version", nullptr, "1.0.0"),
		blowfishKey(v, "Key", nullptr, Helpers::createBlowfishKey())
	{
		

		addMissingFolders();

		afm.registerBasicFormats();
		afm.registerFormat(new hlac::HiseLosslessAudioFormat(), false);

		
		saveExpansionInfoFile();
	}

	~Expansion()
	{
		saveExpansionInfoFile();

		
	}

	File getRootFolder() const override { return root; }

	var getSampleMapList();

	var getAudioFileList();

	var getImageList();

	PooledAudioFile loadAudioFile(const PoolReference& audioFileId);

	PooledImage loadImageFile(const PoolReference& imageId);

	String getSampleMapReference(const String& sampleMapId);

	ValueTree getSampleMap(const String& sampleMapId);

	File root;
	bool isEncrypted = false;
	ValueTree v;

	CachedValue<String> name;
	CachedValue<String> projectName;
	CachedValue<String> version;
	CachedValue<String> projectVersion;
	CachedValue<String> blowfishKey;

private:

	var getFileListInternal(ProjectHandler::SubDirectories type, const String& wildcards, bool addExtension);

	AudioFormatManager afm;

	

	void saveExpansionInfoFile()
	{
		if (isEncrypted)
			return;

		auto file = Helpers::getExpansionInfoFile(root, false);

		file.replaceWithText(v.toXmlString());
	}

	void addMissingFolders()
	{
		addFolder(ProjectHandler::SubDirectories::AudioFiles);
		addFolder(ProjectHandler::SubDirectories::Images);
		addFolder(ProjectHandler::SubDirectories::SampleMaps);
		addFolder(ProjectHandler::SubDirectories::UserPresets);
	}

	void addFolder(ProjectHandler::SubDirectories directory)
	{
		auto d = root.getChildFile(ProjectHandler::getIdentifier(directory));

		subDirectories.add({ directory, false, d });

		if (!d.isDirectory())
			d.createDirectory();
	}

	

	ValueTree loadValueTree()
	{
		auto infoFile = Helpers::getExpansionInfoFile(root, false);
		
		if (infoFile.existsAsFile())
		{
			ScopedPointer<XmlElement> xml = XmlDocument::parse(infoFile);

			if (xml != nullptr)
			{
				auto tree = ValueTree::fromXml(*xml);
				isEncrypted = false;
				return tree;
			}
		}

		auto encryptedInfoFile = Helpers::getExpansionInfoFile(root, true);

		if (encryptedInfoFile.existsAsFile())
		{
			isEncrypted = true;

			auto tree = ValueTree("ExpansionInfo");

			return tree;
		}

		isEncrypted = false;

		return ValueTree("ExpansionInfo");
	};

	struct Helpers
	{
		

		static File getExpansionInfoFile(const File& expansionRoot, bool getEncrypted)
		{
			if (getEncrypted) return expansionRoot.getChildFile("info.hxd");
			else			  return expansionRoot.getChildFile("expansion_info.xml");
		}

		static String getExpansionIdFromReference(const String& referenceId);

		static String createBlowfishKey()
		{
			MemoryBlock mb;
			mb.setSize(72, true);

			Random r;

			char* data = (char*)mb.getData();

			for (int i = 0; i < 72; i++)
			{
				data[i] = (char)r.nextInt(128);
			}

			return mb.toBase64Encoding();
		}
	};

	friend class ExpansionHandler;
	
	JUCE_DECLARE_WEAK_REFERENCEABLE(Expansion)

};

class ExpansionHandler
{
public:

	struct Reference
	{
		String referenceString;
	};

	class Listener
	{
	public:

		virtual ~Listener()
		{}

		virtual void expansionPackCreated(Expansion* newExpansion) { expansionPackLoaded(newExpansion); };
		virtual void expansionPackLoaded(Expansion* currentExpansion) = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	ExpansionHandler(MainController* mc);

	struct Helpers
	{
		static void createFrontendLayoutWithExpansionEditing(FloatingTile* root);
		static bool isValidExpansion(const File& directory);
		static Identifier getSanitizedIdentifier(const String& idToSanitize);
	};

	void createNewExpansion(const File& expansionFolder);
	File getExpansionFolder() const;
	void createAvailableExpansions();

	var getListOfAvailableExpansions() const;

	bool setCurrentExpansion(const String& expansionName);

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	PooledAudioFile loadAudioFileReference(const PoolReference& sampleId);

	double getSampleRateForFileReference(const PoolReference& sampleId);

	const var getMetadata(const PoolReference& sampleId);



	PooledImage loadImageReference(const PoolReference& imageId, PoolHelpers::LoadingType loadingType = PoolHelpers::LoadAndCacheWeak);

	PooledSampleMap loadSampleMap(const PoolReference& sampleMapId);

	bool isActive() const { return currentExpansion != nullptr; }


	Expansion* getExpansionForWildcardReference(const String& stringToTest) const;

	int getNumExpansions() const { return expansionList.size(); }

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
private:

	template <class DataType> void getPoolForReferenceString(const PoolReference& p, SharedPoolBase<DataType>** pool)
	{
		auto type = PoolHelpers::getSubDirectoryType(DataType());

		ignoreUnused(p, type);
		jassert(p.getFileType() == type);
		
        PoolCollection* poolCollection = getCurrentPoolCollection();

		*pool = poolCollection->getPool<DataType>();
	}

	struct Notifier : private AsyncUpdater
	{
	public:
		enum class EventType
		{
			ExpansionLoaded,
			ExpansionCreated,
			numModes
		};

		Notifier(ExpansionHandler& parent_) :
			parent(parent_)
		{};

		

		void sendNotification(EventType eventType, NotificationType notificationType = sendNotificationAsync)
		{
			m = eventType;

			if (notificationType == sendNotificationAsync)
			{
				triggerAsyncUpdate();
			}
			else if (notificationType == sendNotificationSync)
			{
				handleAsyncUpdate();
			}
		}

	private:

		void handleAsyncUpdate()
		{
			ScopedLock sl(parent.listeners.getLock());

			for (auto l : parent.listeners)
			{
				if (l.get() != nullptr)
				{
					if (m == EventType::ExpansionLoaded)
						l->expansionPackLoaded(parent.currentExpansion);
					else
						l->expansionPackCreated(parent.currentExpansion);
				}

			}

			return;
		}

		ExpansionHandler& parent;

		EventType m;
		WeakReference<Expansion> currentExpansion;
	};

	Notifier notifier;

	Array<WeakReference<Listener>, CriticalSection> listeners;

	OwnedArray<Expansion> expansionList;

	WeakReference<Expansion> currentExpansion;

	MainController * mc;
};


}

#endif

