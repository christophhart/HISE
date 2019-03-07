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
	auto v = data->v;
	auto i = createFromValueTree(data->v);
	return i;
}

hise::MarkdownDataBase::Item ScriptingApiDatabase::ItemGenerator::createFromValueTree(ValueTree& v)
{
	const static Identifier root("Api");
	const static Identifier method("method");


	MarkdownDataBase::Item i;

	i.keywords.add("Scripting API");

	i.c = Colour(0xFFC65638).withMultipliedSaturation(0.8f);

	if (v.getType() == root)
	{
		i.type = hise::MarkdownDataBase::Item::Folder;
		i.fileName = "Scripting API";
		i.tocString = "Scripting API";
		i.description = "The scripting API reference";
		i.url << apiWildcard;

		auto readme = getFolderReadmeFile(i.url);

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
	else if (v.getType() == method)
	{
		auto parent = v.getParent();
		auto className = parent.getType().toString();
		i.type = hise::MarkdownDataBase::Item::Headline;
		i.tocString = v.getProperty("name").toString();
		i.description << "`" << className << "." << i.tocString << "()`  ";
		i.description << v.getProperty("description").toString();
		i.url << apiWildcard << "/" << className.toLowerCase() << "#" << v.getProperty("name").toString().toLowerCase();
	}
	else
	{
		i.type = hise::MarkdownDataBase::Item::Folder;
		i.description << "API class reference: `" << v.getType().toString() << "`";
		i.tocString = v.getType().toString();
		i.fileName = i.tocString;
		i.url << apiWildcard << "/" << v.getType().toString().toLowerCase();

		

	}

	for (auto c : v)
		i.children.add(createFromValueTree(c));

	MarkdownDataBase::Item::Sorter sorter;

	i.children.sort(sorter);

	return i;
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

juce::String ScriptingApiDatabase::Resolver::getContent(const String& url)
{
	if (url.startsWith(apiWildcard))
	{
		auto cleaned = url.fromFirstOccurrenceOf(apiWildcard, false, false);

		auto classLink = cleaned.upToFirstOccurrenceOf("#", false, false).removeCharacters("/");
		auto anchor = cleaned.fromFirstOccurrenceOf("#", true, false);

		if (classLink.isEmpty())
		{
			auto dr = HtmlGenerator::getLocalFileForSanitizedURL(docRoot, url, File::findDirectories);
			
			jassert(dr.isDirectory());

			auto readme = dr.getChildFile("readme.md");

			if (readme.existsAsFile())
				return readme.loadFileAsString();

			return {};
		}

		auto classTree = getChildWithSanitizedName(data->v, classLink);

		if (classTree.isValid())
		{
			String s;

			


			auto classDirectory = HtmlGenerator::getLocalFileForSanitizedURL(docRoot, url, File::findDirectories);

			if (classDirectory.isDirectory())
			{
				auto mdFile = classDirectory.getChildFile("readme.md");

				if (mdFile.existsAsFile())
				{
					s << mdFile.loadFileAsString();
					s << "  \n";

					s << "# Class methods  \n";
				}
				else
					s << "# " << classTree.getType() << "\n";
			}
			

			for (auto c : classTree)
			{
				s << createMethodText(c);
			}

			return s;
		}

		String s;

		s << "# Scripting API";

		return s;
	}

	return {};
}

juce::String ScriptingApiDatabase::Resolver::createMethodText(ValueTree& mv)
{
	String s;

	String className = mv.getParent().getType().toString();
	String methodName = mv.getProperty("name").toString();

	s << "## `" << methodName << "`\n";

	s << "> " << mv.getProperty("description").toString() << "  \n";

	s << "```javascript\n" << className << "." << methodName << mv.getProperty("arguments").toString() << "```  \n";



	auto dRoot = HtmlGenerator::getLocalFileForSanitizedURL(docRoot, apiWildcard, File::findDirectories);

	File additionalDoc = dRoot.getChildFile(className + "/" + methodName + ".md");
	if (additionalDoc.existsAsFile())
	{
		s << additionalDoc.loadFileAsString();
		s << "  \n";
	}



	return s;
}

}