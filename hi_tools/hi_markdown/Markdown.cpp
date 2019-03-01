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


const MarkdownLayout& MarkdownParser::LayoutCache::getLayout(const AttributedString& s, float w)
{
	int64 hash = s.getText().hashCode64();

	for (auto l : cachedLayouts)
	{
		if (l->hashCode == hash && l->width == w)
			return l->l;
	}

	auto newLayout = new Layout(s, w);

	cachedLayouts.add(newLayout);
	return newLayout->l;
}


MarkdownParser::LayoutCache::Layout::Layout(const AttributedString& s, float w):
	l(s, w)
{
	hashCode = s.getText().hashCode64();
	width = w;
}



MarkdownParser::MarkdownParser(const String& markdownCode_, LayoutCache* c) :
	markdownCode(markdownCode_.replace("\r\n", "\n")),
	it(markdownCode),
	
	currentParseResult(Result::fail("Nothing parsed yet")),
    layoutCache(c),
	uncachedLayout({}, 0.0f)
{
	imageProviders.add(new ImageProvider(this));
	imageProviders.add(new URLImageProvider(File::getSpecialLocation(File::tempDirectory).getChildFile("TempImagesForMarkdown"), this));
	linkResolvers.add(new DefaultLinkResolver(this));
	
	history.add(markdownCode);
	historyIndex = 0;

}

void MarkdownParser::setFonts(Font normalFont_, Font codeFont_, Font headlineFont_, float defaultFontSize_)
{
	styleData.f = normalFont_;
	
	styleData.fontSize = defaultFontSize_;
}


juce::String MarkdownParser::resolveLink(const String& url)
{
	for (auto lr : linkResolvers)
	{
		auto link = lr->getContent(url);

		if (link.isNotEmpty())
			return link;
	}

	return "Can't resolve link " + url;
}

juce::Image MarkdownParser::resolveImage(const String& imageUrl, float width)
{
	for (int i = imageProviders.size() - 1; i >= 0; i--)
	{
		auto img = imageProviders[i]->getImage(imageUrl, width);

		if (img.isValid())
			return img;
	}

	return {};
}

void MarkdownParser::setDefaultTextSize(float fontSize)
{
	styleData.fontSize = fontSize;
}

float MarkdownParser::getHeightForWidth(float width)
{
	if (width == lastWidth)
		return lastHeight;

	float height = 0.0f;

	for (auto* e : elements)
	{
		if (auto h = dynamic_cast<Headline*>(e))
		{
			h->anchorY = height;
		}

		height += e->getTopMargin();
		height += e->getHeightForWidthCached(width);
	}

	lastWidth = width;
	lastHeight = height;
	firstDraw = true;

	return height;
}


void MarkdownParser::draw(Graphics& g, Rectangle<float> totalArea, Rectangle<int> viewedArea) const
{
	for (auto* e : elements)
	{
		auto heightToUse = e->getHeightForWidthCached(totalArea.getWidth());
		auto topMargin = e->getTopMargin();
		totalArea.removeFromTop(topMargin);
		auto ar = totalArea.removeFromTop(heightToUse);

		if(firstDraw || viewedArea.isEmpty() || ar.toNearestInt().intersects(viewedArea))
			e->draw(g, ar);
	}

	firstDraw = false;
}


hise::MarkdownParser::TableOfContent MarkdownParser::createTableOfContent() const
{
	TableOfContent toc;

	for (const auto e : elements)
	{
		if (auto h = dynamic_cast<const Headline*>(e))
		{
			int intendation = 2 - h->level;

			if (intendation < 0)
				continue;

			toc.entries.add({ intendation, h->content.getText(), h->anchorURL});
		}
	}

	return toc;
}

String MarkdownParser::getCurrentText() const
{
	return markdownCode;
}

void MarkdownParser::setNewText(const String& newText)
{
	resetCurrentBlock();
	elements.clear();

	markdownCode = newText;
	it = Iterator(markdownCode);
	parse();
}


bool MarkdownParser::gotoLink(const MouseEvent& event, Rectangle<float> area)
{
	auto link = getHyperLinkForEvent(event, area);

	if (link.valid)
	{
		return gotoLink(link.url);
	}

	return false;
}


bool MarkdownParser::gotoLink(const String& url)
{
	auto newLink = url.upToFirstOccurrenceOf("#", false, false);

	if (newLink.isNotEmpty())
		lastLink = newLink;

	lastAnchor = url.fromFirstOccurrenceOf("#", true, false);

	for (auto r : linkResolvers)
	{
		if (r->linkWasClicked(url))
			return true;
	}

	String newText = resolveLink(url).replace("\r\n", "\n");

	history.removeRange(historyIndex, -1);
	history.add(newText);
	historyIndex = history.size() - 1;

	setNewText(newText);
	return true;
}

juce::String MarkdownParser::getAnchorForY(int y) const
{
	int currentY = 0;

	Headline* lastHeadline = nullptr;

	for (auto e : elements)
	{
		if (auto h = dynamic_cast<Headline*>(e))
		{
			lastHeadline = h;
		}

		currentY += e->getTopMargin();
		currentY += e->getLastHeight();

		if (y <= currentY)
			break;
	}

	if (lastHeadline != nullptr)
		return lastHeadline->anchorURL;

	return {};
}

hise::MarkdownParser::HyperLink MarkdownParser::getHyperLinkForEvent(const MouseEvent& event, Rectangle<float> area)
{
	float y = 0.0f;

	for (auto* e : elements)
	{
		auto heightToUse = e->getHeightForWidthCached(area.getWidth());
		heightToUse += e->getTopMargin();
		Rectangle<float> eBounds(area.getX(), y, area.getWidth(), heightToUse);

		if (eBounds.contains(event.getPosition().toFloat()))
		{
			auto translatedPoint = event.getPosition().toFloat();
			translatedPoint.addXY(eBounds.getX(), -eBounds.getY());

			for (auto& h : e->hyperLinks)
			{
				if (h.area.contains(translatedPoint))
					return h;
			}
		}

		y += eBounds.getHeight();
	}

	return {};
}


void MarkdownParser::createDatabaseEntriesForFile(File root, MarkdownDataBase::Item& item, File f, Colour c)
{
	DBG(f.getFullPathName());
	DBG("-------------------------------------------------");

	MarkdownParser p(f.loadFileAsString());
	
	p.parse();

	if (p.getParseResult().failed())
	{
		DBG(p.getParseResult().getErrorMessage());
	}

	auto last = p.elements.getLast();

	if (auto t = dynamic_cast<Comment*>(p.elements.getLast()))
	{
		auto lastText = t->content.getText();

		item = MarkdownDataBase::Item(MarkdownDataBase::Item::Keyword, root, f, lastText);
		item.c = c;
		item.tocString = item.keywords[0];

		for (auto e : p.elements)
		{
			if (auto h = dynamic_cast<Headline*>(e))
			{
				if (h->level > 1)
					continue;

				MarkdownDataBase::Item headLineItem(MarkdownDataBase::Item::Headline, root, f, lastText);

				headLineItem.description = h->content.getText();
				headLineItem.url << h->anchorURL;
				headLineItem.c = c;
				headLineItem.tocString = headLineItem.description;

				item.children.add(headLineItem);
			}
		}
	}
}





bool MarkdownParser::Helpers::isNewElement(juce_wchar c)
{
	return c == '#' || c == '|' || c == '!' || c == '>' || c == '-' || c == 0 || c == '\n' || CharacterFunctions::isDigit(c);
}

bool MarkdownParser::Helpers::isEndOfLine(juce_wchar c)
{
	return c == '\r' || c == '\n' || c == 0;
}


bool MarkdownParser::Helpers::isNewToken(juce_wchar c, bool isCode)
{
	if (c == '0')
		return true;

	const static String codeDelimiters("`\n\r");
	const static String delimiters("|>#");

	if (isCode)
		return codeDelimiters.indexOfChar(c) != -1;
	else
		return delimiters.indexOfChar(c) != -1;
}


bool MarkdownParser::Helpers::belongsToTextBlock(juce_wchar c, bool isCode, bool stopAtLineEnd)
{
	if (stopAtLineEnd)
		return !isEndOfLine(c);

	return !isNewToken(c, isCode);
}

void MarkdownParser::Element::recalculateHyperLinkAreas(MarkdownLayout& l, Array<HyperLink>& links, float topMargin)
{
	for (const auto& area : l.linkRanges)
	{
		auto r = std::get<0>(area);

		for (auto& link : links)
		{
			if (link.urlRange == r)
			{
				link.area = std::get<1>(area).translated(0.0f, topMargin);
				break;
			}
		}
	}

#if 0
	for (auto& link : links)
	{
		bool found = false;

		
		
		for (int i = 0; i < l.getNumLines(); i++)
		{
			if (found)
				break;

			auto& line = l.getLine(i);
			
			for (auto r : line.runs)
			{
				if (r->font.isUnderlined())
				{
					

					if (r->glyphs.size() > 0)
					{
						auto thisRange = r->stringRange;
						auto urlRange = link.urlRange;

						if (!thisRange.getIntersectionWith(urlRange).isEmpty())
						{
							int i1 = 0;
							int i2 = r->glyphs.size() - 1;



							auto x = r->glyphs.getReference(i1).anchor.getX();
							auto& last = r->glyphs.getReference(i2);
							auto right = last.anchor.getX() + last.width;

							auto offset = line.leading;

							auto yRange = line.getLineBoundsY();

							link.area = { x + offset, yRange.getStart() + topMargin,
								right - x, yRange.getLength() };

							found = true;
							break;
						}
					}
				}
			}

		}
	}
#endif
}


juce::Array<juce::Range<int>> MarkdownParser::Element::getMatchRanges(const String& fullText, const String& searchString, bool countNewLines)
{
	int length = searchString.length();

	if (length <= 1)
		return {};

	int index = 0;
	auto c = fullText.getCharPointer();

	Array<Range<int>> ranges;

	while (*c != 0)
	{
		if (String(c).startsWith(searchString))
		{
			ranges.add({ index, index + length });

			c += length;
			index += length;
			continue;
		}

		if (countNewLines || *c != '\n')
			index++;

		c++;
	}

	return ranges;
}

void MarkdownParser::Element::searchInStringInternal(const AttributedString& textToSearch, const String& searchString)
{
	searchResults.clear();

	if (searchString.isEmpty())
		return;

	auto ranges = getMatchRanges(textToSearch.getText(), searchString, false);
	auto text = textToSearch.getText();

	if (ranges.size() > 0)
	{
		MarkdownLayout searchLayout(textToSearch, lastWidth, true);
		searchLayout.addYOffset(getTopMargin());

		for (auto r : ranges)
		{
			searchResults.add(searchLayout.normalText.getBoundingBox(r.getStart(), r.getLength(), true));
		}
	}
}

MarkdownParser::Iterator::Iterator(const String& text_) :
	text(text_),
	it(text.getCharPointer())
{

}

juce::juce_wchar MarkdownParser::Iterator::peek()
{
	if (it.isEmpty())
		return 0;

	return *(it);
}

bool MarkdownParser::Iterator::advanceIfNotEOF(int numCharsToSkip /*= 1*/)
{
	if (it.isEmpty())
		return false;

	it += numCharsToSkip;
	return !it.isEmpty();
}

bool MarkdownParser::Iterator::advance(int numCharsToSkip /*= 1*/)
{
	it += numCharsToSkip;
	return !it.isEmpty();
}

bool MarkdownParser::Iterator::advance(const String& stringToSkip)
{
	return advance(stringToSkip.length());
}

bool MarkdownParser::Iterator::next(juce::juce_wchar& c)
{
	if (it.isEmpty())
		return false;

	c = *it++;
	return c != 0;
}

bool MarkdownParser::Iterator::match(juce_wchar expected)
{
	juce_wchar c;

	if (!next(c))
		return false;

	if (c != expected)
	{
		String s;

		int lineNumber = 1;

		auto lineCounter = text.getCharPointer();

		while (lineCounter != it)
		{
			if (*lineCounter == '\n')
				lineNumber++;

			lineCounter++;
		}

		auto copy = it;

		int i = 20;

		

		while (*copy != 0 && i > 0)
		{
			copy++;
			i--;
		}

		String excerpt(it, copy);

		s << "Line " << String(lineNumber) << " - ";
		s << "Error at '" << excerpt << "': ";
		s << "Expected: " << expected;
		s << ", Actual: " << c;
		throw s;
	}

	return c == expected;
}

void MarkdownParser::Iterator::skipWhitespace()
{
	juce_wchar c = peek();

	while (CharacterFunctions::isWhitespace(c))
	{
		if (!next(c))
			break;
	}
}

juce::String MarkdownParser::Iterator::getRestString() const
{
	if (it.isEmpty())
		return {};

	return String(it);
}


int MarkdownParser::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	source.skipWhitespace();

	const juce_wchar firstChar = source.peekNextChar();

	switch (firstChar)
	{
	case '#':
	{
		source.skipToEndOfLine();
		return 1;
	}
	case '*':
	{
		while (source.peekNextChar() == '*')
			source.skip();

		while (!source.isEOF() && source.peekNextChar() != '*')
			source.skip();

		while (source.peekNextChar() == '*')
			source.skip();

		return 2;
	}
	case '`':
	{
		source.skip();

		while (!source.isEOF() && source.peekNextChar() != '`')
		{
			source.skip();
		}

		source.skip();

		return 3;
	}
	case '>':
	{
		source.skipToEndOfLine();
		return 4;
	}
	default: source.skip(); return 0;
	}
}

juce::CodeEditorComponent::ColourScheme MarkdownParser::Tokeniser::getDefaultColourScheme()
{
	CodeEditorComponent::ColourScheme s;

	s.set("normal", Colour(0xFFAAAAAA));
	s.set("headline", Colour(SIGNAL_COLOUR).withAlpha(0.7f));
	s.set("highlighted", Colours::white);
	s.set("fixed", Colours::lightblue);
	s.set("comment", Colour(0xFF777777));

	return s;
}


int MarkdownParser::SnippetTokeniser::readNextToken(CodeDocument::Iterator& source)
{

	auto c = source.nextChar();

	if (c == '{')
	{
		source.skipToEndOfLine();
		return 1;
	}
	else
		return 0;
}

juce::CodeEditorComponent::ColourScheme MarkdownParser::SnippetTokeniser::getDefaultColourScheme()
{
	CodeEditorComponent::ColourScheme s;

	s.set("normal", Colour(0xFFAAAAAA));
	s.set("content", Colour(0xFF555555));

	return s;
}


juce::String MarkdownParser::FileLinkResolver::getContent(const String& url)
{
	File f = root.getChildFile(url.upToFirstOccurrenceOf("#", false, true));

	if (f.existsAsFile())
		return f.loadFileAsString();
	else
		return {};
}


bool MarkdownParser::FileLinkResolver::linkWasClicked(const String& url)
{
	return false;
}

juce::Image MarkdownParser::ImageProvider::getImage(const String& /*imageURL*/, float width)
{
	Image img = Image(Image::PixelFormat::ARGB, (int)width, (int)width, true);
	Graphics g(img);
	g.fillAll(Colours::grey);
	g.setColour(Colours::black);
	g.drawRect(0.0f, 0.0f, width, width, 1.0f);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Empty", 0, 0, (int)width, (int)width, Justification::centred);
	return img;
}


MarkdownParser::FileBasedImageProvider::FileBasedImageProvider(MarkdownParser* parent, const File& root) :
	ImageProvider(parent),
	r(root)
{

}

juce::Image MarkdownParser::FileBasedImageProvider::getImage(const String& imageURL, float width)
{
	File imageFile = r.getChildFile(imageURL);

	if (imageFile.existsAsFile() && ImageFileFormat::findImageFormatForFileExtension(imageFile) != nullptr)
		return resizeImageToFit(ImageCache::getFromFile(imageFile), width);

	return {};
}


juce::Image MarkdownParser::HiseDocImageProvider::getImage(const String& urlName, float width)
{
	if (URL::isProbablyAWebsiteURL(urlName))
		return {};

	const String onlineImageDirectory = "http://hise.audio/manual/";

	auto rootDirectory = File("D:/tempImages");

	auto file = rootDirectory.getChildFile(urlName);

	if (file.existsAsFile())
	{
		return resizeImageToFit(ImageCache::getFromFile(file), width);
	}
	
	URL root(onlineImageDirectory);
	auto fileUrl = root.getChildURL(urlName);
	
	int32 start = Time::getApproximateMillisecondCounter();

	file.create();

	ScopedPointer<URL::DownloadTask> task = fileUrl.downloadToFile(file);

	if (task == nullptr)
		return {};

	while (!task->isFinished())
	{
		auto duration = Time::getApproximateMillisecondCounter();

		if (duration - start > 5000)
			break;
	}

	if (task->isFinished() && !task->hadError())
	{
		return resizeImageToFit(ImageCache::getFromFile(file), width);
	}
	else
		return {};
}

bool MarkdownParser::DefaultLinkResolver::linkWasClicked(const String& url)
{
	if (url.startsWith("CLIPBOARD::"))
	{
		String content = url.fromFirstOccurrenceOf("CLIPBOARD::", false, false);

		SystemClipboard::copyTextToClipboard(content);
		return true;
	}

	if (url.startsWith("http"))
	{
		URL u(url);
		u.launchInDefaultBrowser();
		return true;
	}

	if (url.startsWith("#"))
	{
		for (auto e : parser->elements)
		{
			if (auto headLine = dynamic_cast<Headline*>(e))
			{
				if (url == headLine->anchorURL)
				{
					for (auto l : parser->listeners)
					{
						if(l.get() != nullptr)
							l->scrollToAnchor(headLine->anchorY);
					}
				}
			}
		}

		return true;
	}

	return false;
}

juce::String MarkdownParser::FolderTocCreator::getContent(const String& url)
{
	if(url.startsWith("{FOLDER_TOC}"))
	{
		auto filePath = url.fromFirstOccurrenceOf("{FOLDER_TOC}", false, false);

		auto directory = rootFile.getChildFile(filePath.replace("\\", "/"));

		jassert(directory.isDirectory());

		String s;

		s << "# Content of " << directory.getFileName() << "  \n";

		Array<File> files;

		directory.findChildFiles(files, File::findFilesAndDirectories, false);

		for (auto f : files)
		{
			s << "[" << f.getFileNameWithoutExtension() << "](" << f.getRelativePathFrom(rootFile) << ")  \n";
		}

		return s;
	}

	return {};
}

}

