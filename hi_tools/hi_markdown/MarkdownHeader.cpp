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

juce::StringArray MarkdownHeader::getKeywords()
{
	return getKeyList("keywords");
}

juce::String MarkdownHeader::getFirstKeyword()
{
	return getKeywords()[0];
}

juce::String MarkdownHeader::getDescription()
{
	return getKeyValue("summary");
}

juce::String MarkdownHeader::getIcon() const
{
	return getKeyValue("icon");
}

juce::String MarkdownHeader::getKeyValue(const String& key) const
{
	for (const auto& item : items)
	{
		if (item.key == key)
			return item.values[0];
	}

	return {};
}

juce::Colour MarkdownHeader::getColour() const
{
	auto c = getKeyValue("colour");

	if (c.isEmpty())
		return Colours::transparentBlack;

	c = c.substring(1);

	auto number = static_cast<uint32>(c.getHexValue32());

	return Colour(number);
}

juce::String MarkdownHeader::toString() const
{
	String s = "---\n";

	for (auto item : items)
	{
		s << item.toString();
	}


	s << "---\n";

	return s;
}

hise::MarkdownHeader MarkdownHeader::cleaned() const
{
	if (items.size() < 2)
		return {};

	MarkdownHeader copy;
	copy.items.add(items[0]);
	copy.items.add(items[1]);
	return copy;
}

juce::StringArray MarkdownHeader::getKeyList(const String& key) const
{
	for (const auto& item : items)
	{
		if (item.key == key)
			return item.values;
	}

	return {};
}

void MarkdownHeader::checkValid()
{
	if (items[0].key != "keywords")
		throw String(items[0].key + "; expected: keywords");

	if (items[1].key != "summary")
		throw String(items[1].key + "; expected: summary");

	if (items[1].values.size() != 1)
		throw String("summary value not single string");
}

hise::MarkdownHeader MarkdownHeader::getHeaderForFile(File root, const String& url)
{
	auto sanitized = MarkdownLink::Helpers::getSanitizedFilename(url);

	auto f = MarkdownLink::Helpers::getFolderReadmeFile(root, url);

	if (!f.existsAsFile())
		f = MarkdownLink::Helpers::getLocalFileForSanitizedURL(root, url, File::findFiles);

	if (f.existsAsFile())
	{
		MarkdownParser p(f.loadFileAsString());
		p.parse();
		return p.getHeader();
	}

	return {};
}

juce::String MarkdownHeader::Item::toString() const
{
	String s;
	s << key << ": ";

	if (values.size() == 1)
		s << values[0] << "\n";
	else
	{
		s << "\n";

		for (auto v : values)
			s << "- " << v.trim() << "\n";
	}

	return s;
}

juce::File MarkdownHeader::createEmptyMarkdownFileWithMarkdownHeader(File parent, String childName, String description)
{
#if HISE_HEADLESS
	return {};
#endif

	auto titleToUse = childName;

	if (childName.toLowerCase() == "readme")
		titleToUse = MarkdownLink::Helpers::getPrettyName(parent.getFileName());

	auto f = parent.getChildFile(childName + ".md");

	if (f.existsAsFile())
		return f;

	String s;

	s << "---\n";
	s << "keywords: " << titleToUse << "\n";
	s << "summary:  " << (description.isEmpty() ? "[Enter summary]" : description) << "\n";
	s << "author:   " << "Christoph Hart" << "\n";
	s << "modified: " << Time::getCurrentTime().formatted("%d.%m.%Y") << "\n";
	s << "---\n";

	s << "  \n";
	s << "![warning](/images/icon_warning:64px)  \n";
	s << "> Oops, this document has not been created yet. Luckily, you can help out. If you want to learn how to contribute to the documentation, please visit [this site](glossary/contributing#contributing) to learn more.  \n";

	f.create();
	f.replaceWithText(s);

	return f;
}

}