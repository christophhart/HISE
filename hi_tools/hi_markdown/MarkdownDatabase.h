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

		bool containsURL(const String& urlToLookFor) const;

		bool callForEach(const IteratorFunction& f)
		{
			if (f(*this))
				return true;

			for (auto& c : children)
			{
				if (c.callForEach(f))
					return true;
			}

			return false;
		}

		Item* findChildWithURL(const String& urlToLookFor) const;

		Item(Type t, File root, File f, const StringArray& keywords_, String description_);
		Item() {};

		int fits(String search) const;
		String generateHtml(const String& rootString, const String& activeUrl) const;
		void addToList(Array<Item>& list) const;

		ValueTree createValueTree() const;
		void loadFromValueTree(ValueTree& v);

		Array<Item> children;

		Type type = Type::Invalid;
		String tocString;
		String fileName;
		String url;
		StringArray keywords;
		String description;
		Colour c;
	};

	struct ItemGeneratorBase
	{
		ItemGeneratorBase(File rootDirectory_):
			rootDirectory(rootDirectory_)
		{}

		void setRootURL(const String& newRootURL)
		{
			rootURL = newRootURL;
		}

		File getFolderReadmeFile(const String& folderURL);

		virtual ~ItemGeneratorBase() {};
		virtual Item createRootItem(MarkdownDataBase& parent) = 0;


		File rootDirectory;
		MarkdownDataBase::Item rootItem;
		String rootURL;
	};

	struct DirectoryItemGenerator : public ItemGeneratorBase
	{
		DirectoryItemGenerator(const File& rootDirectory, Colour colour);
		Item createRootItem(MarkdownDataBase& parent) override;
		void addFileRecursive(Item& folder, File f);

		File startDirectory;
		Colour c;
	};

	MarkdownDataBase();
	~MarkdownDataBase();

	Item rootItem;
	const Array<Item>& getFlatList();

	void setRoot(const File& newRootDirectory);
	File getRoot() const { return rootDirectory; }

	String generateHtmlToc(const String& activeUrl) const;
	void buildDataBase();
	void addItemGenerator(ItemGeneratorBase* newItemGenerator);

	var getHtmlSearchDatabaseDump();

	File getDatabaseFile()
	{
		return getRoot().getChildFile("Content.dat");
	}

private:

	Array<Item> cachedFlatList;

	File rootDirectory;
	OwnedArray<ItemGeneratorBase> itemGenerators;

	

	void loadFromValueTree(ValueTree& v)
	{
		rootItem = {};
		rootItem.loadFromValueTree(v);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownDataBase);
};





}
