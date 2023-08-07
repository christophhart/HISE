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

namespace hise {
using namespace juce;


class MarkdownDataBase
{
public:

	struct Item
	{
		using IteratorFunction = std::function<bool(Item&)>;

		struct Sorter
		{
			static int compareElements(Item& first, Item& second);
		};

		struct PrioritySorter
		{
			struct PSorter
			{
				PSorter(const String& s_) : s(s_) {};

				int compareElements(const Item& first, const Item& second) const;

				String s;
			};

			PrioritySorter(const String& searchString_) :
				searchString(searchString_)
			{};

			Array<Item> sortItems(Array<Item>& arrayToBeSorted);

			String searchString;
		};

		enum Type
		{
			Invalid = 0,
			Root,
			Folder,
			Keyword,
			Filename,
			Headline,
			numTypes
		};

		bool callForEach(const IteratorFunction& f);

		bool swapChildWithName(Item& itemToSwap, const String& name);

		var toJSONObject() const;

		Item getChildWithName(const String& name) const;

		explicit operator bool() const
		{
			return url.isValid();
		}

		Item createChildItem(const String& subPath) const;

		Item(File root, File f, const StringArray& keywords_, String description_);
		Item() {};

		Item(const Item& other);

		Item& operator=(const Item& other);

		Item(const MarkdownLink& link);

		int fits(String search) const;
		String generateHtml(const String& rootString, const String& activeUrl) const;
		void addToList(Array<Item>& list) const;

		void addTocChildren(File root);

		ValueTree createValueTree() const;
		void loadFromValueTree(ValueTree& v);

		void setDefaultColour(Colour newColour);

		void fillMetadataFromURL();

		String tocString;
		
		MarkdownLink url;
		StringArray keywords;
		String description;
		bool isAlwaysOpen = false;
		Colour c;
		String icon;

		void addChild(Item&& item);

		void sortChildren();

		void removeChild(int childIndex);

		void swapChildren(Array<Item>& other);

		int getNumChildren() const { return children.size(); }

		Item& operator[](int childIndex) { return children.getReference(childIndex); };

		bool hasChildren() const { return !children.isEmpty(); }

		Item* begin() const
		{
			return const_cast<Item*>(children.begin());
		}

		Item* end() const
		{
			return const_cast<Item*>(children.end());
		}

		Item* getParentItem() const { return parent; }

		int index = -1;
		
		int getWeight() const;

		void setAutoweight(int newAutoWeight);

		void applyWeightFromHeader(const MarkdownHeader& h);

		void setIndexFromHeader(const MarkdownHeader& h);

	private:

		void applyWeightString(const String& weightString);

		int deltaWeight = 0;
		int absoluteWeight = -1;
		int autoWeight = 100;

		Item* parent = nullptr;

		Array<Item> children;

		
	};

	struct ItemGeneratorBase
	{
		ItemGeneratorBase(File rootDirectory_):
			rootDirectory(rootDirectory_)
		{}

		File getFolderReadmeFile(const String& folderURL);
		void setColour(Colour c) { colour = c; };

		virtual ~ItemGeneratorBase() {};
		virtual Item createRootItem(MarkdownDataBase& parent) = 0;

		Colour colour;
		File rootDirectory;
		MarkdownDataBase::Item rootItem;
	};

	struct DirectoryItemGenerator : public ItemGeneratorBase
	{
		DirectoryItemGenerator(const File& rootDirectory, Colour colour);
		Item createRootItem(MarkdownDataBase& parent) override;
		void addFileRecursive(Item& folder, File f);

		File startDirectory;
	};

	

	MarkdownDataBase();
	~MarkdownDataBase();

	Item rootItem;
	const Array<Item>& getFlatList();

	void setRoot(const File& newRootDirectory);
	File getRoot() const { return rootDirectory; }

	String generateHtmlToc(const String& activeUrl) const;
	
	void addItemGenerator(ItemGeneratorBase* newItemGenerator);

	var getHtmlSearchDatabaseDump();

	var getJSONObjectForToc()
	{
		return rootItem.toJSONObject();
	}

	File getDatabaseFile()
	{
		return getRoot().getChildFile("content.dat");
	}

	void clear()
	{
		//discussions.clear();
		itemGenerators.clear();
		cachedFlatList.clear();
        rootDirectory = File();
		rootItem = {};
	}

	struct ForumDiscussionLink
	{
		MarkdownLink contentLink;
		MarkdownLink forumLink;
	};

	void addForumDiscussion(const ForumDiscussionLink& link);


	MarkdownLink getForumDiscussion(const MarkdownLink& contentLink) const;

	void setProgressCounter(double* newProgressCounter)
	{
		progressCounter = newProgressCounter;
	}

	MarkdownLink getLink(const String& link);

	bool createFooter = true;

private:

	Array<ForumDiscussionLink> discussions;

	friend class MarkdownDatabaseHolder;

	void buildDataBase(bool useCache);

	Array<Item> cachedFlatList;

	File rootDirectory;
	OwnedArray<ItemGeneratorBase> itemGenerators;

	double* progressCounter = nullptr;

	void loadFromValueTree(ValueTree& v)
	{
		rootItem = {};
		rootItem.loadFromValueTree(v);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownDataBase);
};

class MarkdownContentProcessor;

class MarkdownDatabaseHolder
{
public:

	struct DatabaseListener
	{
		virtual ~DatabaseListener() {};

		virtual void databaseWasRebuild() = 0;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(DatabaseListener);
	};

	struct ServerUpdateListener
	{
		virtual ~ServerUpdateListener() {};

		virtual void serverUpdateStateStarted() {};

		virtual void serverUpdateFinished(bool /*successful*/) {};

		JUCE_DECLARE_WEAK_REFERENCEABLE(ServerUpdateListener);
	};

    virtual ~MarkdownDatabaseHolder() {};
    
	const MarkdownDataBase& getDatabase() const
	{
		return db;
	}

	MarkdownDataBase& getDatabase()
	{
		return db;
	}

	virtual Component* getRootComponent() { return nullptr; };

	virtual URL getBaseURL() const;

	virtual void registerContentProcessor(MarkdownContentProcessor* processor) = 0;
	virtual void registerItemGenerators() = 0;
	virtual File getCachedDocFolder() const = 0;
	virtual File getDatabaseRootDirectory() const = 0;

	virtual bool shouldUseCachedData() const;

	void setForceCachedDataUse(bool shouldUseCachedData, bool rebuild=true);


	File rootFile;
	MarkdownDataBase db;

	void addDatabaseListener(DatabaseListener* l) { listeners.addIfNotAlreadyThere(l); }
	void removeDatabaseListener(DatabaseListener* l) { listeners.removeAllInstancesOf(l); }

	virtual bool databaseDirectoryInitialised() const
	{
		return getDatabaseRootDirectory().isDirectory();
	}


	bool shouldAbort() const;

	void rebuildDatabase();

	void addContentProcessor(MarkdownContentProcessor* contentProcessor);

	void removeContentProcessor(MarkdownContentProcessor* contentToRemove);

	void addItemGenerator(MarkdownDataBase::ItemGeneratorBase* newItemGenerator);

	void setProgressCounter(double* p)
	{
		progressCounter = p;
		db.setProgressCounter(p);
	}

	bool nothingInHere() const { return nothingToShow; }

	void addServerUpdateListener(ServerUpdateListener* l);

	void removeServerUpdateListener(ServerUpdateListener* l);

	void sendServerUpdateMessage(bool started, bool successful);

private:

	bool nothingToShow = false;

	double* progressCounter = nullptr;

	
	Array<WeakReference<ServerUpdateListener>> serverUpdateListeners;
	Array<WeakReference<MarkdownContentProcessor>> contentProcessors;

	bool forceUseCachedData = true;

	Array<WeakReference<DatabaseListener>> listeners;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MarkdownDatabaseHolder)
};



}
