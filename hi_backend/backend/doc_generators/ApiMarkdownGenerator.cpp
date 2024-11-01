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

ScriptingApiDatabase::Data::Data()
{
	v = ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize);
}

ScriptingApiDatabase::Data::~Data()
{

}

hise::MarkdownDataBase::Item ScriptingApiDatabase::ItemGenerator::createRootItem(MarkdownDataBase& parent)
{
	auto item = DirectoryItemGenerator::createRootItem(parent);

	auto v = data->v;
    auto c = item.getChildWithName("scripting-api");
	auto scriptingApi = updateWithValueTree(c ,data->v);

	scriptingApi.fillMetadataFromURL();

	item.swapChildWithName(scriptingApi, "scripting-api");

	item.setDefaultColour(colour);

	return item;
}

hise::MarkdownDataBase::Item ScriptingApiDatabase::ItemGenerator::updateWithValueTree(MarkdownDataBase::Item& item, ValueTree& v)
{
	const static Identifier root("Api");
	const static Identifier method("method");

	if (v.getType() == root)
	{
		Array<MarkdownDataBase::Item> newItems;

		// Create all classes
		for (auto c : v)
		{
			auto t = MarkdownLink::Helpers::getSanitizedFilename(c.getType().toString());

			MarkdownDataBase::Item i = item.getChildWithName(t);

			if (!i)
			{
				i.description << "API class reference: `" << c.getType().toString() << "`";
				i.tocString = c.getType().toString();
				i.url = rootUrl.getChildUrl(c.getType().toString());
			}
			else
			{
				i.tocString = c.getType().toString();
				i.url = rootUrl.getChildUrl(c.getType().toString());
			}

			newItems.add(updateWithValueTree(i, c));
		}

		item.swapChildren(newItems);
	}

	

#if 0
	if (v.getType() == root)
	{
		i.type = hise::MarkdownDataBase::Item::Folder;
		i.fileName = "Scripting API";
		i.tocString = "Scripting API";
		i.description = "The scripting API reference";
		i.url = rootUrl;

		FloatingTileContent::FloatingTilePathFactory f;

		i.icon = "/images/icon_script";

		auto readme = i.url.getMarkdownFile({});

		if (readme.existsAsFile())
		{
			MarkdownDataBase::Item ni;
			MarkdownParser::createDatabaseEntriesForFile(rootDirectory, ni, readme, i.c);

			if (ni.type != MarkdownDataBase::Item::Invalid)
			{
				i.description = ni.description;
				i.keywords = ni.keywords;

				i.children = ni.children;

				i.callForEach([](MarkdownDataBase::Item& item) {item.tocString = {}; return false; });
				i.tocString = "Scripting API";
			}
		}
	}
#endif
	if (v.getType() != method && v.getType() != root)
	{

		item.url.setType(MarkdownLink::Folder);

		Array<MarkdownDataBase::Item> newChildren;

		item.swapChildren(newChildren);

#if 0
		for(int i = 0; i < item.getNumChildren(); i++)
		{
			auto url = item[i].url;
			
			if (url.hasAnchor())
				item.removeChild(i--);
		}
#endif

		for (auto c : v)
		{
			MarkdownDataBase::Item i;

			i.c = item.c;
			auto className = v.getType().toString();
			i.tocString = c.getProperty("name").toString();
			i.description << "`" << className << "." << i.tocString << "()`  ";
			i.description << c.getProperty("description").toString();
			i.url = rootUrl.getChildUrl(className).getChildUrl(c.getProperty("name").toString(), true);
			item.addChild(std::move(i));
		}

		item.sortChildren();
	}
	else
	{
		

#if 0
		i.type = hise::MarkdownDataBase::Item::Folder;
		i.description << "API class reference: `" << v.getType().toString() << "`";
		i.tocString = v.getType().toString();
		i.fileName = i.tocString;
		i.url = rootUrl.getChildUrl(v.getType().toString());
#endif
	}

	

	return item;
}

ValueTree getChildWithSanitizedName(ValueTree v, const String& sanitizedName)
{
	if (v.getType().toString().toLowerCase() == sanitizedName)
		return v;

	for (auto c : v)
	{
		auto result = getChildWithSanitizedName(c, sanitizedName);

		if (result.isValid())
			return result;
	}

	return {};
}

juce::String ScriptingApiDatabase::Resolver::getContent(const MarkdownLink& url)
{
	if (url.isChildOf(rootURL))
	{
		auto anchor = url.toString(MarkdownLink::AnchorWithoutHashtag);

		auto classLink = url.toString(MarkdownLink::UrlSubPath);

		// Just load the readme
		if (classLink.isEmpty())
			return url.toString(MarkdownLink::ContentFull);

		auto classTree = getChildWithSanitizedName(data->v, classLink);

		if (classTree.isValid())
		{
			String s;

			s << url.toString(MarkdownLink::ContentFull);

			if (s.isEmpty())
				s << "# " << classTree.getType() << "\n";

			s << "  \n";
			s << "# Class methods  \n";
			
			for (auto c : classTree)
			{
				s << createMethodText(c);
			}

			return s;
		}

		String s;

		s << url.toString(MarkdownLink::ContentFull);

		return s;
	}

	return {};
}

juce::String ScriptingApiDatabase::Resolver::createMethodText(ValueTree& mv)
{
	String s;

	String className = mv.getParent().getType().toString();
	String methodName = mv.getProperty("name").toString();

	s << "## `" << methodName;
	
	s << "`\n";

	s << "> " << mv.getProperty("description").toString().trim();

	auto fileLink = rootURL.getChildUrl(className).getChildUrl(methodName);
	auto docFile = fileLink.getMarkdownFile(rootURL.getRoot());

	if(rootURL.getRoot().isDirectory() && (!docFile.existsAsFile() || docFile.loadFileAsString().isEmpty()))
	{
		if(!docFile.existsAsFile())
			docFile.create();

		s << fileLink.getEditLinkOnGitHub(false);
	}

	s << "\n";

	s << "```javascript\n" << className << "." << methodName << mv.getProperty("arguments").toString() << "```  \n";

	s << fileLink.toString(MarkdownLink::ContentWithoutHeader, rootURL.getRoot());
	s << "  \n";

	return s;
}

juce::File ScriptingApiDatabase::Resolver::getFileToEdit(const MarkdownLink& url)
{
	if(!url.isChildOf(rootURL))
		return {};

	if (url.toString(MarkdownLink::AnchorWithoutHashtag).isEmpty())
		return {};

	auto folderUrl = url.withAnchor({});

	

	bool anchorValid = false;

	auto className = folderUrl.toString(MarkdownLink::UrlSubPath);
	auto anchorName = url.toString(MarkdownLink::AnchorWithoutHashtag);

	for (const auto& c : data->v)
	{
		if (MarkdownLink::Helpers::getSanitizedFilename(c.getType().toString()) == className)
		{
			for (const auto& m : c)
			{
				if (MarkdownLink::Helpers::getSanitizedFilename(m.getProperty("name").toString()) == anchorName)
				{
					anchorValid = true;
					break;
				}
			}

			if (anchorValid)
				break;
		}
	}

	if (!anchorValid)
	{
		PresetHandler::showMessageWindow("You tried to edit a sub headline of a API method", "Please click on the headline of the method to edit.", PresetHandler::IconType::Error);
		return {};
	}
		

	auto f = folderUrl.toFile(MarkdownLink::FileType::Directory);

	if (!f.isDirectory())
		f.createDirectory();

	if (f.isDirectory())
	{
		auto e = f.getChildFile(url.toString(MarkdownLink::AnchorWithoutHashtag) + ".md");

		if (MessageManager::getInstance()->isThisTheMessageThread())
		{
			if (!e.existsAsFile() && PresetHandler::showYesNoWindow("Create File for method description", "Do you want to create the file\n" + e.getFullPathName()))
			{
				e.create();
			}
		}

		return e;
	}
	
	return {};
}

}
