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

juce::String MarkdownDataBase::generateHtmlToc(const String&) const
{
	return {};
}

void MarkdownDataBase::buildDataBase(bool useCache)
{
	rootItem = {};
	rootItem.url = { rootDirectory, "/" };

	if (useCache && getDatabaseFile().existsAsFile())
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

	int numTotal = itemGenerators.size();
	int p = 0;

	for (auto g : itemGenerators)
	{
		if (progressCounter != nullptr)
			*progressCounter = (double)p++ / (double)numTotal;

		if (!MessageManager::getInstance()->isThisTheMessageThread() &&
			Thread::getCurrentThread()->threadShouldExit())
		{
			break;
		}

		auto newItem = g->createRootItem(*this);
		rootItem.addChild(std::move(newItem));
	}

	rootItem.sortChildren();
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
		if (!item.hasChildren())
			return false;

		if (item.tocString.isEmpty())
			return false;

		for (auto& c : item)
		{
			if (c.tocString.isEmpty())
				continue;

			String s = item.tocString + ": " + c.tocString;

			MarkdownLink scriptRoot(f, "/scripting/scripting-api/");

			if(c.url.isChildOf(scriptRoot) && c.url.toString(MarkdownLink::AnchorWithHashtag).isNotEmpty())
			{
				s = item.tocString + "." + c.tocString + "()";
			}

			jassert(c.url.getType() == MarkdownLink::MarkdownFile || c.url.getType() == MarkdownLink::Folder);

			String url = c.url.toString(MarkdownLink::FormattedLinkHtml);
			String colour = "#" + c.c.toDisplayString(false);
			DynamicObject* obj = new DynamicObject();
			obj->setProperty("key", s);
			obj->setProperty("url", url);
			obj->setProperty("weight", c.getWeight());
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

MarkdownDataBase::DirectoryItemGenerator::DirectoryItemGenerator(const File& rootDirectory, Colour colour_) :
	ItemGeneratorBase(rootDirectory),
	startDirectory(rootDirectory)
{
	colour = colour_;
}

hise::MarkdownDataBase::Item MarkdownDataBase::DirectoryItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	rootDirectory = parent.getRoot();
	Item rItem;
	addFileRecursive(rItem, startDirectory);

	if (!rItem.c.isTransparent())
		colour = rItem.c;

	rItem.setDefaultColour(colour);

	return rItem;
}

void MarkdownDataBase::DirectoryItemGenerator::addFileRecursive(Item& folder, File f)
{
	if (f.isDirectory())
	{
		folder.url = { rootDirectory, f.getRelativePathFrom(rootDirectory) };
		jassert(folder.url.getType() == MarkdownLink::Folder);

		folder.fillMetadataFromURL();

		if (folder.url.fileExists({}))
		{
			Item ni;

			MarkdownParser::createDatabaseEntriesForFile(rootDirectory, ni, folder.url.getMarkdownFile(folder.url.getRoot()), folder.c);

			if (ni)
			{
				folder.description = ni.description;

				folder.keywords = ni.keywords;

				auto u = folder.url;

				ni.callForEach([u](Item& i) 
				{ 
					i.index = 0; 
					i.url = u.withAnchor(i.url.toString(MarkdownLink::AnchorWithoutHashtag));
					return false;
				});

				for (auto c : ni)
					folder.addChild(std::move(c));

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

			if (newItem)
				folder.addChild(std::move(newItem));
		}

		folder.sortChildren();
	}
	else
	{
		// Skip README.md files (they will be displayed in the folder item.
		if (f.getFileName().toLowerCase() == "readme.md")
			return;

		MarkdownParser::createDatabaseEntriesForFile(rootDirectory, folder, f, colour);
	}
}

int MarkdownDataBase::Item::Sorter::compareElements(Item& first, Item& second)
{
	if (first.index != -1)
	{
		if (second.index != -1)
		{
			if (first.index < second.index)
				return -1;
			else if (first.index > second.index)
				return 1;
			else
				return 0;
		}
		else
		{
			// items without index are always sorted last
			return -1;
		}
	}
	if (second.index != -1)
	{
		return 1;
	}

	return first.tocString.compareNatural(second.tocString);
}

int MarkdownDataBase::Item::PrioritySorter::PSorter::compareElements(const Item& first, const Item& second) const
{
	if (first.getWeight() > second.getWeight())
		return -1;
	else return 1;
}




juce::Array<hise::MarkdownDataBase::Item> MarkdownDataBase::Item::PrioritySorter::sortItems(Array<Item>& arrayToBeSorted)
{
	PSorter s(searchString);
	arrayToBeSorted.sort(s, false);
	return arrayToBeSorted;
}

bool MarkdownDataBase::Item::callForEach(const IteratorFunction& f)
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

bool MarkdownDataBase::Item::swapChildWithName(Item& itemToSwap, const String& name)
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

var MarkdownDataBase::Item::toJSONObject() const
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

MarkdownDataBase::Item MarkdownDataBase::Item::getChildWithName(const String& name) const
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

#if 0
	if (!FuzzySearcher::searchForIndexes(search, sa, 0.3f).isEmpty())
		return 2;
#endif

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

	for (const auto& child : children)
	{
		html << child.generateHtml(rootString, activeURL);
	}

	return g.surroundWithTag(html, "details", "");// containsURL(activeURL) ? "open" : "");
}

void MarkdownDataBase::Item::addToList(Array<Item>& list) const
{
	list.add(*this);

	for (const auto& child : children)
	{
		child.addToList(list);
	}
		
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
	item.url = url.getChildUrlWithRoot(subPath);
	item.c = c;
	return item;
}

MarkdownDataBase::Item::Item(File root, File f, const StringArray& keywords_, String description_) :
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

	auto header = link.getHeaderFromFile({});

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
			addChild(std::move(cItem));
		}
	}
	if (link.getType() == MarkdownLink::Type::MarkdownFile)
	{
		MarkdownParser::createDatabaseEntriesForFile(url.getRoot(), *this, link.toFile(MarkdownLink::FileType::ContentFile), c);
	}
}

MarkdownDataBase::Item::Item(const Item& other)
{
	description = std::move(other.description);
	keywords =    std::move(other.keywords);
	url = other.url;
	tocString = other.tocString;
	icon = other.icon;
	c = other.c;
	isAlwaysOpen = other.isAlwaysOpen;
	autoWeight = other.autoWeight;
	deltaWeight = other.deltaWeight;
	absoluteWeight = other.absoluteWeight;
	index = other.index;

	children = other.children;

	for (auto& child : children)
		child.parent = this;
}

hise::MarkdownDataBase::Item& MarkdownDataBase::Item::operator=(const Item& other)
{
	description = std::move(other.description);
	keywords = std::move(other.keywords);
	url = other.url;
	tocString = other.tocString;
	icon = other.icon;
	c = other.c;
	isAlwaysOpen = other.isAlwaysOpen;
	autoWeight = other.autoWeight;
	deltaWeight = other.deltaWeight;
	absoluteWeight = other.absoluteWeight;
	index = other.index;

	children = other.children;

	for (auto& child : children)
		child.parent = this;

	return *this;
}

juce::ValueTree MarkdownDataBase::Item::createValueTree() const
{
	ValueTree v("Item");

	jassert(*this);

	v.setProperty("Description", description, nullptr);
	v.setProperty("Keywords", keywords.joinIntoString(";"), nullptr);
	v.setProperty("URL", url.toString(MarkdownLink::Everything), nullptr);
	v.setProperty("LinkType", url.getType(), nullptr);
	v.setProperty("TocString", tocString, nullptr);
	v.setProperty("Colour", c.toString(), nullptr);
	v.setProperty("Icon", icon, nullptr);
	v.setProperty("AlwaysOpen", this->isAlwaysOpen, nullptr);
	v.setProperty("Index", index, nullptr);
	v.setProperty("DeltaWeight", deltaWeight, nullptr);
	v.setProperty("AbsoluteWeight", absoluteWeight, nullptr);

	for (const auto& child : children)
		v.addChild(child.createValueTree(), -1, nullptr);

	return v;
}

void MarkdownDataBase::Item::loadFromValueTree(ValueTree& v)
{
	keywords = StringArray::fromTokens(v.getProperty("Keywords").toString(), ";", "");
	description = v.getProperty("Description");
	url = MarkdownLink::createWithoutRoot(v.getProperty("URL"));
	url.setType((MarkdownLink::Type)(int)v.getProperty("LinkType", (int)MarkdownLink::MarkdownFileOrFolder));
	tocString = v.getProperty("TocString");
	c = Colour::fromString(v.getProperty("Colour").toString());
	icon = v.getProperty("Icon", "");
	isAlwaysOpen = v.getProperty("AlwaysOpen", false);
	deltaWeight = v.getProperty("DeltaWeight", 0);
	absoluteWeight = v.getProperty("AbsoluteWeight", -1);
	index = v.getProperty("Index", -1);

	for (auto child : v)
	{
		Item newChild;
		newChild.loadFromValueTree(child);
		addChild(std::move(newChild));
	}
}

void MarkdownDataBase::Item::setDefaultColour(Colour newColour)
{
	if (c.isTransparent())
		c = newColour;

	for (auto& child : children)
		child.setDefaultColour(c);
}


void MarkdownDataBase::addForumDiscussion(const ForumDiscussionLink& link)
{
	discussions.add(link);
}

MarkdownLink MarkdownDataBase::getForumDiscussion(const MarkdownLink& contentLink) const
{
	for (auto d : discussions)
	{
		if (d.contentLink == contentLink)
			return d.forumLink;
	}

	return {};
}

hise::MarkdownLink MarkdownDataBase::getLink(const String& link)
{
	MarkdownLink linkToReturn({}, link);

	auto linkString = linkToReturn.toString(MarkdownLink::UrlFull);

	if (linkToReturn.getType() == MarkdownLink::MarkdownFileOrFolder)
	{
		MarkdownLink* tmp = &linkToReturn;

		rootItem.callForEach([linkString, tmp](Item& r)
		{
			auto thisLink = r.url.toString(MarkdownLink::UrlFull);

			if (thisLink == linkString)
			{
				*tmp = MarkdownLink(r.url);
				return true;
			}

			return false;
		});
	}

	return linkToReturn;
}

void MarkdownDataBase::Item::fillMetadataFromURL()
{
	auto f = url.toFile(MarkdownLink::FileType::ContentFile);
	
	if (f.existsAsFile())
	{
		auto headerString = url.toString(MarkdownLink::ContentHeader);
		MarkdownParser p(headerString);
		p.parse();
		auto header = p.getHeader();
		keywords = header.getKeywords();
		tocString = header.getFirstKeyword();
		description = header.getDescription();
		icon = header.getIcon();
		Colour hc = header.getColour();

		if (!hc.isTransparent())
			c = hc;
		
		setIndexFromHeader(header);
		applyWeightFromHeader(header);
	}
}

void MarkdownDataBase::Item::addChild(Item&& item)
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

void MarkdownDataBase::Item::sortChildren()
{
	MarkdownDataBase::Item::Sorter sorter;
	children.sort(sorter);
}

void MarkdownDataBase::Item::removeChild(int childIndex)
{
	children.remove(childIndex);
}

void MarkdownDataBase::Item::swapChildren(Array<Item>& other)
{
	children.swapWith(other);
}

int MarkdownDataBase::Item::getWeight() const
{
	if (absoluteWeight != -1)
		return absoluteWeight + deltaWeight;
	else
		return autoWeight + deltaWeight;
}

void MarkdownDataBase::Item::setAutoweight(int newAutoWeight)
{
	autoWeight = newAutoWeight;

	for (auto& child : children)
		child.setAutoweight(getWeight() - 10);
}

void MarkdownDataBase::Item::applyWeightFromHeader(const MarkdownHeader& h)
{
	auto weightString = h.getKeyValue("weight");

	if (weightString.isNotEmpty())
		applyWeightString(weightString);
}

void MarkdownDataBase::Item::setIndexFromHeader(const MarkdownHeader& h)
{
	auto indexString = h.getKeyValue("index");

	if (indexString.isNotEmpty())
		index = indexString.getIntValue();
}

void MarkdownDataBase::Item::applyWeightString(const String& weightString)
{
	if (weightString.contains("!"))
		absoluteWeight = weightString.upToFirstOccurrenceOf("!", false, false).getIntValue();
	else if (weightString.contains("+"))
	{
		deltaWeight = weightString.fromFirstOccurrenceOf("+", false, false).getIntValue();
	}
	else if (weightString.contains("-"))
	{
		deltaWeight = -1 * weightString.fromFirstOccurrenceOf("-", false, false).getIntValue();
	}
}

juce::File MarkdownDataBase::ItemGeneratorBase::getFolderReadmeFile(const String& folderURL)
{
	return MarkdownLink(rootDirectory, folderURL).getMarkdownFile({});
}

juce::URL MarkdownDatabaseHolder::getBaseURL() const
{
	return URL("https://docs.hise.audio/");
}

bool MarkdownDatabaseHolder::shouldUseCachedData() const
{
	if (forceUseCachedData)
		return true;

	return !getDatabaseRootDirectory().isDirectory();
}

void MarkdownDatabaseHolder::setForceCachedDataUse(bool shouldUseCachedData, bool rebuild)
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

bool MarkdownDatabaseHolder::shouldAbort() const
{
	if (!MessageManager::getInstance()->isThisTheMessageThread() &&
		Thread::getCurrentThread()->threadShouldExit())
	{
		return true;
	}

	return false;
}

void MarkdownDatabaseHolder::rebuildDatabase()
{
	nothingToShow = false;

	if(progressCounter != nullptr)
		*progressCounter = 0.0;

	db.clear();

	if (shouldUseCachedData())
		db.setRoot(getCachedDocFolder());
	else
		db.setRoot(getDatabaseRootDirectory());

	if (shouldAbort())
		return;

	registerItemGenerators();

	if (shouldAbort())
		return;


	db.setProgressCounter(progressCounter);
	db.buildDataBase(shouldUseCachedData());

	if (shouldAbort())
		return;

	if (progressCounter != nullptr)
		*progressCounter = 0.5;

	double delta = 0.5 / (double)jmax(1, contentProcessors.size());

	for (auto c : contentProcessors)
	{
		if (c.get() == nullptr)
			continue;

		if (shouldAbort())
			return;

		c->clearResolvers();
		
		if (progressCounter != nullptr)
			*progressCounter += delta;

		registerContentProcessor(c);
		c->resolversUpdated();
	}
		

	if (shouldUseCachedData() && !db.getDatabaseFile().existsAsFile())
	{
		nothingToShow = true;
	}
	

	for (auto l : listeners)
	{
		if (shouldAbort())
			return;

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

void MarkdownDatabaseHolder::removeContentProcessor(MarkdownContentProcessor* contentToRemove)
{
	contentProcessors.removeAllInstancesOf(contentToRemove);
}

void MarkdownDatabaseHolder::addItemGenerator(MarkdownDataBase::ItemGeneratorBase* newItemGenerator)
{
	db.addItemGenerator(newItemGenerator);
}

void MarkdownDatabaseHolder::addServerUpdateListener(ServerUpdateListener* l)
{
	serverUpdateListeners.addIfNotAlreadyThere(l);
}

void MarkdownDatabaseHolder::removeServerUpdateListener(ServerUpdateListener* l)
{
	serverUpdateListeners.removeAllInstancesOf(l);
}

void MarkdownDatabaseHolder::sendServerUpdateMessage(bool started, bool successful)
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
}
