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
		struct Sorter
		{
			static int compareElements(Item& first, Item& second)
			{
				return first.tocString.compareNatural(second.tocString);
			}
		};

		struct PrioritySorter
		{
			PrioritySorter(const String& searchString_) :
				searchString(searchString_)
			{};

			Array<Item> sortItems(Array<Item>& arrayToBeSorted)
			{
				Array<Item> s;
				s.ensureStorageAllocated(arrayToBeSorted.size());

				Sorter sorter;
				arrayToBeSorted.sort(sorter);
				
				// Direct keyword matches first
				for (int i = 0; i < arrayToBeSorted.size(); i++)
				{
					auto item = arrayToBeSorted[i];

					if (item.type != Keyword)
						continue;

					if (item.keywords.contains(searchString))
						s.add(arrayToBeSorted.removeAndReturn(i--));
				}

				for (int i = 0; i < arrayToBeSorted.size(); i++)
				{
					auto item = arrayToBeSorted[i];

					if (item.keywords.contains(searchString))
						s.add(arrayToBeSorted.removeAndReturn(i--));
				}

				// Keyword items first
				for (int i = 0; i < arrayToBeSorted.size(); i++)
				{
					auto item = arrayToBeSorted[i];

					if (item.type == Keyword)
						s.add(arrayToBeSorted.removeAndReturn(i--));
				}
				
				s.addArray(arrayToBeSorted);

				return s;
			}

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

		int fits(String search) const
		{
			search = search.toLowerCase().removeCharacters("\\/[]()`* ").substring(0, 64);

			StringArray sa;

			sa.addArray(keywords);
			sa.add(description);
			sa.add(fileName);
			sa.add(tocString);

			for (auto& s : sa)
			{
				s = s.toLowerCase().removeCharacters("\\/[]()`* ").substring(0, 64);
				if (s.contains(search))
					return 1;
			}

			if (!FuzzySearcher::searchForIndexes(search, sa, 0.3f).isEmpty())
				return 2;

			return 0;
		}

		String generateHtml(const String& rootString, const String& activeUrl) const;
		
		Item() {};

		void addToList(Array<Item>& list) const
		{
			list.add(*this);

			for (auto c : children)
				c.addToList(list);
		}

		Item(Type t, File root, File f, const String& lastLine) :
			type(t),
			fileName(f.getFileName()),
			url(f.getRelativePathFrom(root).replace("\\", "/"))
		{
			auto s = StringArray::fromTokens(lastLine, "|", "");

			if (s.size() == 2)
			{
				auto kw = s[0].trim().fromFirstOccurrenceOf("keywords:", false, true).trim();

				keywords = StringArray::fromTokens(kw, ";", "\"");

				for (auto& k : keywords)
					k = k.removeCharacters("\"");

				description = s[1].trim().fromFirstOccurrenceOf("description:", false, true).trim();
			}
		}

		ValueTree createValueTree() const
		{
			ValueTree v("Item");
			v.setProperty("Type", (int)type, nullptr);
			v.setProperty("Filename", fileName, nullptr);
			v.setProperty("Description", description, nullptr);
			v.setProperty("Keywords", keywords.joinIntoString(";"), nullptr);
			v.setProperty("URL", url, nullptr);

			for (const auto& c : children)
					v.addChild(c.createValueTree(), -1, nullptr);
			
			return v;
		}

		void loadFromValueTree(ValueTree& v)
		{
			type = (Type)(int)v.getProperty("Type");
			fileName = v.getProperty("Filename");
			keywords = StringArray::fromTokens(v.getProperty("Keywords").toString(), ";", "");
			description = v.getProperty("Description");
			url = v.getProperty("URL");

			for (auto c : v)
			{
				Item newChild;
				newChild.loadFromValueTree(c);
				children.add(newChild);
			}
		}

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
		virtual ~ItemGeneratorBase() {};

		virtual Item createRootItem(MarkdownDataBase& parent) = 0;
	};

	struct DirectoryItemGenerator : public ItemGeneratorBase
	{
		DirectoryItemGenerator(const File& rootDirectory, Colour colour) :
			ItemGeneratorBase(),
			root(rootDirectory),
			c(colour)
		{}

		Item createRootItem(MarkdownDataBase& parent) override
		{
			dbRoot = parent.getRoot();
			Item rItem;
			addFileRecursive(rItem, root);
			return rItem;
		}

		void addFileRecursive(Item& folder, File f);

		File root;
		File dbRoot;
		Colour c;
	};

	MarkdownDataBase()
	{

	}

	~MarkdownDataBase()
	{
#if 0
		auto v = rootItem.createValueTree();

		zstd::ZDefaultCompressor comp;
		comp.compress(v, getDatabaseFile());
#endif
	}

	Item rootItem;

	const Array<Item>& getFlatList()
	{
		if (cachedFlatList.isEmpty())
		{
			rootItem.addToList(cachedFlatList);
		}

		return cachedFlatList;
	}

	void setRoot(const File& newRootDirectory)
	{
		rootDirectory = newRootDirectory;
	}

	File getRoot() const { return rootDirectory; }

	String generateHtmlToc(const String& activeUrl) const;

	void buildDataBase();

	void addItemGenerator(ItemGeneratorBase* newItemGenerator)
	{
		itemGenerators.add(newItemGenerator);
	}

private:

	Array<Item> cachedFlatList;

	File rootDirectory;
	OwnedArray<ItemGeneratorBase> itemGenerators;

	File getDatabaseFile()
	{
		return getRoot().getChildFile("database.dat");
	}

	void loadFromValueTree(ValueTree& v)
	{
		rootItem = {};
		rootItem.loadFromValueTree(v);
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkdownDataBase);
};

}
