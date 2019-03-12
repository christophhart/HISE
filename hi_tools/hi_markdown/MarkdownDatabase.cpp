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
	rootItem.url = { rootDirectory, "/" };

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
		rootItem.children.add(newItem);
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
	auto f = getRoot();

	rootItem.callForEach([v, f](Item& item)
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

			MarkdownLink scriptRoot(f, "/scripting/scripting-api/");

			if(c.url.isChildOf(scriptRoot) && c.url.toString(MarkdownLink::AnchorWithHashtag).isNotEmpty())
			{
				s = item.tocString + "." + c.tocString + "()";
			}

			String url = c.url.toString(MarkdownLink::FormattedLinkHtml);
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
		folder.url = { rootDirectory, f.getRelativePathFrom(rootDirectory) };

		auto header = folder.url.getHeaderFromFile(rootDirectory, false);

		folder.icon = header.getIcon();
		folder.keywords = header.getKeywords();
		folder.description = header.getDescription();
		folder.tocString = header.getFirstKeyword();
		if (folder.tocString.isEmpty())
			folder.tocString = f.getFileName();
		


		if (folder.url.fileExists({}))
		{
			Item ni;

			MarkdownParser::createDatabaseEntriesForFile(rootDirectory, ni, folder.url.getMarkdownFile({}), folder.c);

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

	auto realURL = url.toString(MarkdownLink::FormattedLinkHtml);

	auto link = g.surroundWithTag(tocString, "a", "href=\"" + realURL + "\"");

	auto d = g.surroundWithTag(link, "summary", styleTag);

	html << d;

	for (auto c : children)
	{
		html << c.generateHtml(rootString, activeURL);
	}

	return g.surroundWithTag(html, "details", "");// containsURL(activeURL) ? "open" : "");
}

void MarkdownDataBase::Item::addToList(Array<Item>& list) const
{
	list.add(*this);

	for (auto c : children)
		c.addToList(list);
}

void MarkdownDataBase::Item::addTocChildren(File root)
{
	auto f = url.getMarkdownFile(root);

	if (f.existsAsFile())
	{
		MarkdownParser::createDatabaseEntriesForFile(root, *this, f, c);
	}
}

MarkdownDataBase::Item MarkdownDataBase::Item::createChildItem(const String& subPath) const
{
	MarkdownDataBase::Item item;
	item.url = url.getChildUrl(subPath);
	item.c = c;
	return item;
}

MarkdownDataBase::Item::Item(Type t, File root, File f, const StringArray& keywords_, String description_) :
	type(t),
	url({ root, f.getRelativePathFrom(root) })
{
	// If you construct an item like this, you need a directory...
	jassert(root.isDirectory());
	keywords = keywords_;
	description = description_;
}

MarkdownDataBase::Item::Item(const MarkdownLink& link):
	url(link)
{
	// You need to pass in a valid root
	jassert(url.getRoot().isDirectory());

	auto header = link.getHeaderFromFile({}, false);

	keywords = header.getKeywords();
	description = header.getDescription();
	tocString = keywords[0];

	if (link.getType() == MarkdownLink::Folder)
	{
		Array<File> childFiles;

		link.getDirectory({}).findChildFiles(childFiles, File::findFilesAndDirectories, false);

		for (auto cf : childFiles)
		{
			auto cUrl = url.getChildUrlWithRoot(cf.getFileNameWithoutExtension(), false);

			Item cItem(cUrl);
			children.add(cItem);
		}
	}
	if (link.getType() == MarkdownLink::Type::MarkdownFile)
	{
		MarkdownParser::createDatabaseEntriesForFile(url.getRoot(), *this, link.toFile(MarkdownLink::FileType::ContentFile), c);
	}
}

juce::ValueTree MarkdownDataBase::Item::createValueTree() const
{
	ValueTree v("Item");
	v.setProperty("Type", (int)type, nullptr);
	v.setProperty("Description", description, nullptr);
	v.setProperty("Keywords", keywords.joinIntoString(";"), nullptr);
	v.setProperty("URL", url.toString(MarkdownLink::Everything), nullptr);
	v.setProperty("TocString", tocString, nullptr);
	v.setProperty("Colour", c.toString(), nullptr);

	for (const auto& c : children)
		v.addChild(c.createValueTree(), -1, nullptr);

	return v;
}

void MarkdownDataBase::Item::loadFromValueTree(ValueTree& v)
{
	type = (Type)(int)v.getProperty("Type");
	keywords = StringArray::fromTokens(v.getProperty("Keywords").toString(), ";", "");
	description = v.getProperty("Description");
	url = MarkdownLink::createWithoutRoot(v.getProperty("URL"));
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
	return MarkdownLink(rootDirectory, folderURL).getMarkdownFile({});
}

void MarkdownDatabaseHolder::rebuildDatabase()
{
	if(progressCounter != nullptr)
		*progressCounter = 0.0;

	db.clear();

	if (shouldUseCachedData())
		db.setRoot(getCachedDocFolder());
	else
		db.setRoot(getDatabaseRootDirectory());

	registerItemGenerators();

	if (progressCounter != nullptr)
		*progressCounter = 0.5;

	double delta = 0.5 / (double)jmax(1, contentProcessors.size());

	for (auto c : contentProcessors)
	{
		if (c.get() == nullptr)
			continue;

		c->clearResolvers();
		
		if (progressCounter != nullptr)
			*progressCounter += delta;

		registerContentProcessor(c);
		c->resolversUpdated();
	}
		

	if (shouldUseCachedData() && !db.getDatabaseFile().existsAsFile())
	{
		jassertfalse;
	}

	db.buildDataBase();

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->databaseWasRebuild();
	}
}

void MarkdownDatabaseHolder::addContentProcessor(MarkdownContentProcessor* contentProcessor)
{
	contentProcessors.add(contentProcessor);
	contentProcessor->clearResolvers();
	registerContentProcessor(contentProcessor);
}

}