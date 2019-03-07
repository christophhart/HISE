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

namespace hise {
using namespace juce;


MarkdownDataBase::MarkdownDataBase()
{

}

MarkdownDataBase::~MarkdownDataBase()
{

}

const juce::Array<hise::MarkdownDataBase::Item>& MarkdownDataBase::getFlatList()
{
	if (cachedFlatList.isEmpty())
	{
		rootItem.addToList(cachedFlatList);
	}

	return cachedFlatList;
}

void MarkdownDataBase::setRoot(const File& newRootDirectory)
{
	rootDirectory = newRootDirectory;
}

juce::String MarkdownDataBase::generateHtmlToc(const String& activeUrl) const
{
	HtmlGenerator g;

	String s;

	for (auto c : rootItem.children)
		s << c.generateHtml(rootDirectory.getChildFile("html").getFullPathName() + "/", activeUrl);

	return g.surroundWithTag(s, "div", "class=\"toc\"");
}

void MarkdownDataBase::buildDataBase()
{
	rootItem = {};
	rootItem.type = Item::Root;
	rootItem.url = "/";

	if (getDatabaseFile().existsAsFile())
	{
		zstd::ZDefaultCompressor compressor;

		ValueTree v;
		auto r = compressor.expand(getDatabaseFile(), v);

		if (r.wasOk())
		{
			loadFromValueTree(v);
			return;
		}
	}

	for (auto g : itemGenerators)
	{
		auto newItem = g->createRootItem(*this);

		if(g->rootURL.isEmpty())
			rootItem.children.add(newItem);
		else
		{
			if (auto c = rootItem.findChildWithURL(g->rootURL))
			{
				*c = newItem;
			}
			else
				jassertfalse; // Can't resolve URL
		}
	}
}


void MarkdownDataBase::addItemGenerator(ItemGeneratorBase* newItemGenerator)
{
	itemGenerators.add(newItemGenerator);
}

juce::var MarkdownDataBase::getHtmlSearchDatabaseDump()
{
	Array<var> list;

	var v(list);

	rootItem.callForEach([v](Item& item)
	{ 
		if (item.children.isEmpty())
			return false;

		if (item.tocString.isEmpty())
			return false;

		for (auto& c : item.children)
		{
			if (c.tocString.isEmpty())
				continue;

			String s = item.tocString + ": " + c.tocString;

			if (c.url.startsWith("/scripting/scripting-api/") && c.url.contains("#"))
			{
				s = item.tocString + "." + c.tocString + "()";
			}

			String url = HtmlGenerator::createHtmlLink(c.url, "");
			String colour = "#" + c.c.toDisplayString(false);
			DynamicObject* obj = new DynamicObject();
			obj->setProperty("key", s);
			obj->setProperty("url", url);
			obj->setProperty("color", colour);

			v.getArray()->add(var(obj));
		}

		return false;
	});

#if 0
	File f("D:\\docdummy\\db.json");
	f.create();
	f.replaceWithText(JSON::toString(v));

	DBG(JSON::toString(v));
#endif

	return v;
}

MarkdownDataBase::DirectoryItemGenerator::DirectoryItemGenerator(const File& rootDirectory, Colour colour) :
	ItemGeneratorBase(rootDirectory),
	startDirectory(rootDirectory),
	c(colour)
{

}

hise::MarkdownDataBase::Item MarkdownDataBase::DirectoryItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	rootDirectory = parent.getRoot();
	Item rItem;
	addFileRecursive(rItem, startDirectory);
	return rItem;
}

void MarkdownDataBase::DirectoryItemGenerator::addFileRecursive(Item& folder, File f)
{
	if (f.isDirectory())
	{
		folder.c = c;
		folder.type = Item::Folder;
		folder.description = "Folder";
		folder.fileName = f.getFileName();
		folder.keywords.add(folder.fileName);
		
		
		auto path = f.getRelativePathFrom(rootDirectory).replaceCharacter('\\', '/');

		path = HtmlGenerator::getSanitizedFilename(path);

		folder.tocString = folder.fileName.trimCharactersAtStart("01234567890 ");

		folder.url = "/" + path;

		if (f.getChildFile("Readme.md").existsAsFile())
		{
			Item ni;

			MarkdownParser::createDatabaseEntriesForFile(rootDirectory, ni, f.getChildFile("Readme.md"), folder.c);

			if (ni.type != Item::Invalid)
			{
				folder.description = ni.description;

				folder.keywords = ni.keywords;

				ni.callForEach([](Item& i) { i.tocString = {}; return false; });

				for (auto c : ni.children)
					folder.children.add(c);

			}
		}

		Array<File> childFiles;

		f.findChildFiles(childFiles, File::findFilesAndDirectories, false);

		childFiles.sort();

		for (auto c : childFiles)
		{
			if (!c.isDirectory() && !c.hasFileExtension(".md"))
				continue;

			Item newItem;
			addFileRecursive(newItem, c);

			if (newItem.type != Item::Invalid)
				folder.children.add(newItem);
		}
	}
	else
	{
		// Skip README.md files (they will be displayed in the folder item.
		if (f.getFileName().toLowerCase() == "readme.md")
			return;

		MarkdownParser::createDatabaseEntriesForFile(rootDirectory, folder, f, c);
	}
}

int MarkdownDataBase::Item::Sorter::compareElements(Item& first, Item& second)
{
	return first.tocString.compareNatural(second.tocString);
}



juce::Array<hise::MarkdownDataBase::Item> MarkdownDataBase::Item::PrioritySorter::sortItems(Array<Item>& arrayToBeSorted)
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



int MarkdownDataBase::Item::fits(String search) const
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



juce::String MarkdownDataBase::Item::generateHtml(const String& rootString, const String& activeURL) const
{
	String html;

	HtmlGenerator g;

	String styleTag;
	
	styleTag << "style=\"padding-left: 10px; border-left: 3px solid #" << c.toDisplayString(false) << "\"";

	String urlToUse = url;

	auto realURL = HtmlGenerator::createHtmlLink(urlToUse, rootString);

	auto link = g.surroundWithTag(tocString, "a", "href=\"" + realURL + "\"");

	auto d = g.surroundWithTag(link, "summary", styleTag);

	html << d;

	for (auto c : children)
	{
		html << c.generateHtml(rootString, activeURL);
	}




	return g.surroundWithTag(html, "details", containsURL(activeURL) ? "open" : "");
}

void MarkdownDataBase::Item::addToList(Array<Item>& list) const
{
	list.add(*this);

	for (auto c : children)
		c.addToList(list);
}

bool MarkdownDataBase::Item::containsURL(const String& urlToLookFor) const
{
	for (const auto& c : children)
	{
		if (c.containsURL(urlToLookFor))
			return true;
	}

	return url == urlToLookFor;
}

MarkdownDataBase::Item* MarkdownDataBase::Item::findChildWithURL(const String& urlToLookFor) const
{
	if (url == urlToLookFor)
		return const_cast<Item*>(this);

	for (const auto& c : children)
	{
		if (auto r = c.findChildWithURL(urlToLookFor))
			return r;
	}

	return nullptr;
}

MarkdownDataBase::Item::Item(Type t, File root, File f, const StringArray& keywords_, String description_) :
	type(t),
	fileName(f.getFileName()),
	url(HtmlGenerator::getSanitizedFilename(f.getRelativePathFrom(root)))
{
	keywords = keywords_;
	description = description_;
}

juce::ValueTree MarkdownDataBase::Item::createValueTree() const
{
	ValueTree v("Item");
	v.setProperty("Type", (int)type, nullptr);
	v.setProperty("Filename", fileName, nullptr);
	v.setProperty("Description", description, nullptr);
	v.setProperty("Keywords", keywords.joinIntoString(";"), nullptr);
	v.setProperty("URL", url, nullptr);
	v.setProperty("TocString", tocString, nullptr);
	v.setProperty("Colour", c.toString(), nullptr);

	for (const auto& c : children)
		v.addChild(c.createValueTree(), -1, nullptr);

	return v;
}

void MarkdownDataBase::Item::loadFromValueTree(ValueTree& v)
{
	type = (Type)(int)v.getProperty("Type");
	fileName = v.getProperty("Filename");
	keywords = StringArray::fromTokens(v.getProperty("Keywords").toString(), ";", "");
	description = v.getProperty("Description");
	url = v.getProperty("URL");
	tocString = v.getProperty("TocString");
	c = Colour::fromString(v.getProperty("Colour").toString());

	for (auto c : v)
	{
		Item newChild;
		newChild.loadFromValueTree(c);
		children.add(newChild);
	}
}

juce::File MarkdownDataBase::ItemGeneratorBase::getFolderReadmeFile(const String& folderURL)
{
	auto f = HtmlGenerator::getLocalFileForSanitizedURL(rootDirectory, folderURL, File::findDirectories);

	if (f.isDirectory())
		return f.getChildFile("Readme.md");

	return {};
}

}