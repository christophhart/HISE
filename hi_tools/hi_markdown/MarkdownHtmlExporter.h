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

	String surroundWithTag(const String& content, const String& tag, String additionalProperties = {})
	{
		String s;

		s << "<" << tag;
		
		if (additionalProperties.isNotEmpty())
			s << " " << additionalProperties;
		
		s<< ">";
		s << content << "";
		s << "</" << tag << ">\n";

		return s;
	}

	String getSubString(const AttributedString& s, int index)
	{
		const auto attribute = s.getAttribute(index);
		return s.getText().substring(attribute.range.getStart(), attribute.range.getEnd()).replace("\n", "<br>");
	}

	
#if 0
	static String createImageLink(String imageURL, const String& rootString)
	{
	
		auto oldExtension = imageURL.fromLastOccurrenceOf(".", true, true);

		if (oldExtension == imageURL)
		{
			oldExtension = ".png";
			imageURL << oldExtension;
		}
			
		return MarkdownLink::Helpers::createHtmlLink(imageURL, rootString).replace(".html", oldExtension);
	}
#endif


	String createFromAttributedString(const AttributedString& s, int& linkIndex)
	{
		String html;

		String content = s.getText();

		for (int i = 0; i < s.getNumAttributes(); i++)
		{
			const auto& a = s.getAttribute(i);

			if (a.font.isUnderlined())
				html << surroundWithTag(getSubString(s, i), "a", "href=\"{LINK" + String(linkIndex++) + "}\"");
			else if (a.font.isBold())
				html << surroundWithTag(getSubString(s, i), "b");
			else if (a.font.isItalic())
				html << surroundWithTag(getSubString(s, i), "i");
			else if (a.font.getTypefaceName() == GLOBAL_MONOSPACE_FONT().getTypefaceName())
				html << surroundWithTag(getSubString(s, i), "code");
			else
				html << getSubString(s, i);
		}

		return html;
	}
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

	

	Markdown2HtmlConverter(MarkdownDataBase& db, const String& markdownCode):
		MarkdownParser(markdownCode),
		database(db)
	{
		setCreateFooter(db.createFooter);
		parse();
	}
    
    virtual ~Markdown2HtmlConverter() {};

	void setHeaderFile(File headerFile)
	{
		headerContent = headerFile.loadFileAsString();
	}

	void setFooterFile(File footerFile)
	{
		footerContent = footerFile.loadFileAsString();
	}

	String generateHtml(const String& /*activeLink*/)
	{
		jassert(headerContent.isNotEmpty());
		jassert(footerContent.isNotEmpty());

		String html;

		html << headerContent;
		
		File root;

		if (mode == LocalFile)
			root = File(linkBase);

		

		for (auto e : elements)
		{
			html << e->generateHtmlAndResolveLinks(root);
		}
			

		html << footerContent;

		return html;
	}

	void writeToFile(File f, const String& activeLink)
	{
		auto r = f.create();
		
		f.replaceWithText(generateHtml(activeLink));
	}

	void setLinkMode(LinkMode , String)
	{
		mode = LinkMode::URLBase;
		
		updateLinks();
	}

private:

	void updateLinks();

	LinkMode mode;
	String linkBase;

	String headerContent;
	String footerContent;

	MarkdownDataBase& database;
};

}
