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



struct HtmlGenerator
{
	HtmlGenerator() {};

	static String surroundWithTag(const String& content, const String& tag, String additionalProperties = {});
	static String getSubString(const AttributedString& s, int index);
	static String createFromAttributedString(const AttributedString& s, int& linkIndex);
};

class Markdown2HtmlConverter : public MarkdownParser
{
public:

	enum LinkMode
	{
		Unprocessed,
		LocalFile,
		URLBase,
		numLinkModes
	};

	

	Markdown2HtmlConverter(MarkdownDataBase& db, const String& markdownCode);

    virtual ~Markdown2HtmlConverter() {};

	void setHeaderFile(File headerFile);

	void setFooterFile(File footerFile);

	String generateHtml(const String& /*activeLink*/);

	void writeToFile(File f, const String& activeLink);

	void setLinkMode(LinkMode , String);

private:

	void updateLinks();

	LinkMode mode;
	String linkBase;

	String headerContent;
	String footerContent;

	MarkdownDataBase& database;
};

}
