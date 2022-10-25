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

		bool callForEach(const IteratorFunction& f)
		{
			if (f(*this))
				return true;

			for (auto& child : children)
			{
				if (child.callForEach(f))
					return true;
			}

			return false;
		}

		bool swapChildWithName(Item& itemToSwap, const String& name)
		{
			for (auto& i : children)
			{
				if (i.url.toString(MarkdownLink::UrlSubPath) == name)
				{
					std::swap(i, itemToSwap);
					return true;
				}
			}

			return false;
		}

		var toJSONObject() const
		{
			jassert(url.getType() == MarkdownLink::Folder || url.getType() == MarkdownLink::MarkdownFile);

			DynamicObject::Ptr newObject = new DynamicObject();
			newObject->setProperty("URL", url.toString(MarkdownLink::FormattedLinkHtml));
			newObject->setProperty("Title", tocString);
			newObject->setProperty("Colour", "#" + c.toDisplayString(false));

			Array<var> childrenArray;

			for (const auto& child : children)
				childrenArray.add(child.toJSONObject());

			newObject->setProperty("Children", childrenArray);

			return var(newObject.get());
		}
		
		Item getChildWithName(const String& name) const
		{
			if (url.toString(MarkdownLink::UrlSubPath) == name)
				return *this;

			for (const auto& child : children)
			{
				auto i = child.getChildWithName(name);

				if (i.url.isValid())
					return i;
			}

			return {};
		}

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

		void setDefaultColour(Colour newColour)
		{
			if (c.isTransparent())
				c = newColour;

			for (auto& child : children)
				child.setDefaultColour(c);
		}

		void fillMetadataFromURL();

		String tocString;
		
		MarkdownLink url;
		StringArray keywords;
		String description;
		bool isAlwaysOpen = false;
		Colour c;
		String icon;

		void addChild(Item&& item)
		{
			item.parent = this;
			item.setAutoweight(getWeight() - 10);

			if (item.url.getType() == MarkdownLink::Type::MarkdownFileOrFolder)
			{
				jassert(item.url.hasAnchor());
				item.url.setType(url.getType());
			}

			children.add(item);
		}

		void sortChildren()
		{
			MarkdownDataBase::Item::Sorter sorter;
			children.sort(sorter);
		}

		void removeChild(int childIndex)
		{
			children.remove(childIndex);
		}

		void swapChildren(Array<Item>& other)
		{
			children.swapWith(other);
		}

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
		
		int getWeight() const
		{
			if (absoluteWeight != -1)
				return absoluteWeight + deltaWeight;
			else
				return autoWeight + deltaWeight;
		}

		void setAutoweight(int newAutoWeight)
		{
			autoWeight = newAutoWeight;

			for (auto& child : children)
				child.setAutoweight(getWeight() - 10);
		}

		void applyWeightFromHeader(const MarkdownHeader& h)
		{
			auto weightString = h.getKeyValue("weight");

			if (weightString.isNotEmpty())
				applyWeightString(weightString);
		}

		void setIndexFromHeader(const MarkdownHeader& h)
		{
			auto indexString = h.getKeyValue("index");

			if (indexString.isNotEmpty())
				index = indexString.getIntValue();
		}

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

	void addForumDiscussion(const ForumDiscussionLink& link)
	{
		discussions.add(link);
	}


	MarkdownLink getForumDiscussion(const MarkdownLink& contentLink) const
	{
		for (auto d : discussions)
		{
			if (d.contentLink == contentLink)
				return d.forumLink;
		}

		return {};
	}
	
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

	void setForceCachedDataUse(bool shouldUseCachedData, bool rebuild=true)
	{
		if (forceUseCachedData != shouldUseCachedData)
		{
			forceUseCachedData = shouldUseCachedData;

			if (rebuild)
			{
				rebuildDatabase();
			}
		}
	}

	

	File rootFile;
	MarkdownDataBase db;

	void addDatabaseListener(DatabaseListener* l) { listeners.addIfNotAlreadyThere(l); }
	void removeDatabaseListener(DatabaseListener* l) { listeners.removeAllInstancesOf(l); }

	virtual bool databaseDirectoryInitialised() const
	{
		return getDatabaseRootDirectory().isDirectory();
	}


	bool shouldAbort() const
	{
		if (!MessageManager::getInstance()->isThisTheMessageThread() &&
			Thread::getCurrentThread()->threadShouldExit())
		{
			return true;
		}

		return false;
	}

	void rebuildDatabase();

	void addContentProcessor(MarkdownContentProcessor* contentProcessor);

	void removeContentProcessor(MarkdownContentProcessor* contentToRemove)
	{
		contentProcessors.removeAllInstancesOf(contentToRemove);
	}

	void addItemGenerator(MarkdownDataBase::ItemGeneratorBase* newItemGenerator)
	{
		db.addItemGenerator(newItemGenerator);
	}

	void setProgressCounter(double* p)
	{
		progressCounter = p;
		db.setProgressCounter(p);
	}

	bool nothingInHere() const { return nothingToShow; }

	void addServerUpdateListener(ServerUpdateListener* l)
	{
		serverUpdateListeners.addIfNotAlreadyThere(l);
	}

	void removeServerUpdateListener(ServerUpdateListener* l)
	{
		serverUpdateListeners.removeAllInstancesOf(l);
	}
	
	void sendServerUpdateMessage(bool started, bool successful)
	{
		for (auto l : serverUpdateListeners)
		{
			if (l == nullptr)
				continue;

			if (started)
				l->serverUpdateStateStarted();
			else
				l->serverUpdateFinished(successful);
		}
	}

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
