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


MarkdownRenderer::NavigationAction::NavigationAction(MarkdownRenderer* renderer, const MarkdownLink& newLink):
	currentLink(newLink),
	parent(renderer)
{
	lastLink = renderer->getLastLink();
	lastY = renderer->currentY;
}

bool MarkdownRenderer::NavigationAction::perform()
{
	if (parent != nullptr)
	{
		parent->MarkdownParser::gotoLink(currentLink);
		return true;
	}

	return false;
}

bool MarkdownRenderer::NavigationAction::undo()
{
	if (parent != nullptr)
	{
		parent->MarkdownParser::gotoLink(lastLink);
		parent->scrollToY(lastY);
		return true;
	}

	return false;
}

const MarkdownLayout& MarkdownRenderer::LayoutCache::getLayout(const AttributedString& s, float w)
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


MarkdownRenderer::LayoutCache::Layout::Layout(const AttributedString& s, float w):
	l(s, w, {})
{
	hashCode = s.getText().hashCode64();
	width = w;
}



MarkdownParser::MarkdownParser(const String& markdownCode_, const MarkdownLayout::StringWidthFunction& f) :
	markdownCode(markdownCode_.replace("\r\n", "\n")),
	it(markdownCode),
	stringWidthFunction(f),
	currentParseResult(Result::fail("Nothing parsed yet"))
{
	if(!stringWidthFunction)
	{
		stringWidthFunction = [](const Font& f, const String& word)
        {
             return f.getStringWidthFloat(word);
        };
	}

	setImageProvider(new ImageProvider(this));
	
	setLinkResolver(new DefaultLinkResolver(this));
}


MarkdownParser::~MarkdownParser()
{

	elements.clear();
	linkResolvers.clear();
	imageProviders.clear();
}

void MarkdownParser::setFonts(Font normalFont_, Font codeFont_, Font headlineFont_, float defaultFontSize_)
{
	styleData.f = normalFont_;
	
	styleData.fontSize = defaultFontSize_;
}


juce::String MarkdownParser::resolveLink(const MarkdownLink& url)
{
	for (auto lr : linkResolvers)
	{
		auto link = lr->getContent(url);

		if (link.isNotEmpty())
			return link;
	}

	return "Can't resolve link " + url.toString(MarkdownLink::UrlFull);
}

juce::Image MarkdownParser::resolveImage(const MarkdownLink& imageUrl, float width)
{
	for (auto ip: imageProviders)
	{
		auto img = ip->getImage(imageUrl, width);

		if (img.isValid())
			return img;
	}

	return {};
}

void MarkdownParser::setLinkResolver(LinkResolver* ownedResolver)
{
	ScopedPointer<LinkResolver> owned = ownedResolver;

	for (auto r : linkResolvers)
	{
		if (r->getId() == owned->getId())
			return;
	}

	LinkResolver::Sorter s;
	linkResolvers.addSorted(s, owned.release());
}

void MarkdownParser::setImageProvider(ImageProvider* newProvider)
{
	ScopedPointer<ImageProvider> owned = newProvider;

	for (auto p : imageProviders)
	{
		if (*p == *newProvider)
			return;
	}

	ImageProvider::Sorter s;
	imageProviders.addSorted(s, owned.release());
}

void MarkdownParser::setDefaultTextSize(float fontSize)
{
	styleData.fontSize = fontSize;
}

void MarkdownParser::setDatabaseHolder(MarkdownDatabaseHolder* holder_)
{
	holder = holder_;
}

String MarkdownParser::getCurrentText(bool includeMarkdownHeader) const
{
	if (includeMarkdownHeader)
		return markdownCode;
	else
		return markdownCode.fromLastOccurrenceOf("---\n", false, false);
}

MarkdownLink MarkdownParser::getLastLink() const
{ 
	return lastLink;
}


void MarkdownParser::setNewText(const String& newText)
{
	resetCurrentBlock();
	elements.clear();

	markdownCode = newText.withCleanedLineEndings();
	it = Iterator(markdownCode);
	parse();
}


bool MarkdownParser::gotoLink(const MarkdownLink& url)
{
	if (url.isSamePage(lastLink))
	{
		lastLink = url;
		jumpToCurrentAnchor();
		return true;
	}

	auto lastAnchor = lastLink.toString(MarkdownLink::AnchorWithHashtag);
	lastLink = url;

	for (auto r : linkResolvers)
	{
		if (r->linkWasClicked(url))
			return true;
	}

	String newText = resolveLink(url).replace("\r\n", "\n");

	setNewText(newText);
	
	auto thisAnchor = url.toString(MarkdownLink::AnchorWithHashtag);

	if (thisAnchor.isEmpty() || thisAnchor != lastAnchor)
		jumpToCurrentAnchor();
	

	return true;
}


MarkdownLink MarkdownParser::getLinkForMouseEvent(const MouseEvent& event, Rectangle<float> whatArea)
{
	auto link = getHyperLinkForEvent(event, whatArea);
	return link.url;
}

int MarkdownParser::getLineNumberForY(float y) const
{
	float thisY = 0.0f;

	for (auto e : elements)
	{
		auto thisHeight = e->getTopMargin();
		thisHeight += e->getLastHeight();

		thisY += thisHeight;
		

		if (thisY > y)
		{
			auto thisNumber = e->getLineNumber();
			auto idx = elements.indexOf(e);

			int thisNumLines = 0;

			if (auto nextElement = elements[idx + 1])
				thisNumLines = nextElement->getLineNumber() - thisNumber;

			auto deltaY =  thisY - y;
			auto deltaYNormalised = 1.0f - deltaY / thisHeight;

			return thisNumber + roundToInt((float)thisNumLines * deltaYNormalised);
		}
	}

	return 0;
}

float MarkdownParser::getYForLineNumber(int lineNumber) const
{
	float yPos = 0.0f;

	int index = 0;

	for (auto e : elements)
	{
		auto thisHeight = e->getTopMargin();
		thisHeight += e->getLastHeight();

		int nextLine = 0;

		if (auto nextElement = elements[++index])
			nextLine = nextElement->getLineNumber();
		else
			nextLine = e->getLineNumber();

		Range<int> lineRange(e->getLineNumber(), nextLine);

		if (lineRange.contains(lineNumber))
		{
			auto lineDelta = (float)(lineNumber - lineRange.getStart());
			auto lineDeltaNormalised = lineDelta / (float)lineRange.getLength();

			return yPos + lineDeltaNormalised * thisHeight;
		}
			
		yPos += thisHeight;
		
	}

	return 0.0f;
}

Array<MarkdownLink> MarkdownParser::getImageLinks() const
{
	Array<MarkdownLink> sa;

	for (auto e : elements)
	{
		e->addImageLinks(sa);
	}

	return sa;
}

hise::MarkdownParser::HyperLink MarkdownParser::getHyperLinkForEvent(const MouseEvent& event, Rectangle<float> area)
{
	if (!containsLinks)
	{
		return {};
	}

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

			Array<HyperLink> matches;

			for (auto& h : e->hyperLinks)
			{
				if (h.area.contains(translatedPoint))
					matches.add(h);
			}

			if (!matches.isEmpty())
			{
				if (matches.size() == 1)
					return matches.getFirst();
				else
				{
					Array<float> areas;
					float minArea = 12000001250.0f;

					for (auto& m : matches)
					{
						areas.add(m.area.getWidth() * m.area.getHeight());
						minArea = jmin(minArea, areas.getLast());
					}

					int index = areas.indexOf(minArea);

					return matches[index];
				}
			}
			
		}

		y += eBounds.getHeight();
	}

	return {};
}


void MarkdownParser::createDatabaseEntriesForFile(File root, MarkdownDataBase::Item& item, File f, Colour c)
{
	jassert(root.isDirectory());

	MarkdownParser p(f.loadFileAsString());
	
	p.parse();

	if (p.getParseResult().failed())
	{
		DBG(p.getParseResult().getErrorMessage());
	}

	try
	{
		auto saveURL = item.url;

		item = MarkdownDataBase::Item(root, f, p.header.getKeywords(), p.header.getDescription());

		if (saveURL.isValid())
			item.url = saveURL;

		item.c = c;
		item.tocString = item.keywords[0];
		item.icon = p.getHeader().getKeyValue("icon");
		
		item.setIndexFromHeader(p.getHeader());
		item.applyWeightFromHeader(p.getHeader());

		MarkdownDataBase::Item lastHeadLine;

		for (auto e : p.elements)
		{
			if (auto h = dynamic_cast<Headline*>(e))
			{
				MarkdownDataBase::Item headLineItem(root, f, p.header.getKeywords(), p.header.getDescription());

				headLineItem.description = h->content.getText();

				if (headLineItem.description.trim() == item.tocString)
					continue;

				headLineItem.url = item.url.getChildUrl(h->anchorURL);
				headLineItem.c = c;

				headLineItem.tocString << headLineItem.description;

				if (h->headlineLevel >= 3)
					headLineItem.tocString = {};


				item.addChild(std::move(headLineItem));

			}
		}
	}
	catch (String& s)
	{
		ignoreUnused(s);
		DBG("---");
		DBG("Error parsing metadata for file " + f.getFullPathName());
		DBG(s);
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


String MarkdownParser::Element::getTextForRange(Range<int> range) const
{
	ignoreUnused(range);
	jassertfalse;
	return {};
}

void MarkdownParser::Element::drawHighlight(Graphics& g, Rectangle<float> area)
{
	if (selected)
	{
		g.setColour(parent->styleData.backgroundColour.contrasting().withAlpha(0.05f));
		g.fillRoundedRectangle(area.expanded(0.0f, 6.0f), 3.0f);
	}

	for (auto r : searchResults)
	{
		g.setColour(Colours::red.withAlpha(0.5f));

		auto r_t = r.translated(area.getX(), area.getY());
		g.fillRoundedRectangle(r_t, 3.0f);
		g.drawRoundedRectangle(r_t, 3.0f, 1.0f);
	}
}


float MarkdownParser::Element::getHeightForWidthCached(float width, bool forceUpdate)
{
	if (width != lastWidth || forceUpdate)
	{
		cachedHeight = getHeightForWidth(width);
		lastWidth = width;
		return cachedHeight;
	}

	return cachedHeight;
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


void MarkdownParser::Element::prepareLinksForHtmlExport(const String& )
{
	getHeightForWidth(850);

	for (auto& l : hyperLinks)
	{


		auto link = parent->getHolder()->getDatabase().getLink(l.url.toString(MarkdownLink::UrlFull));

		for (auto lr : parent->linkResolvers)
		{
			link = lr->resolveURL(link);
		}

		if (link.getType() == MarkdownLink::MarkdownFileOrFolder)
		{
			throw String("Can't resolve link `" + l.url.toString(MarkdownLink::UrlFull) + "`");
		}
		else
		{
			l.url = link;
		}
	}
}

String MarkdownParser::Element::generateHtmlAndResolveLinks(const File& localRoot) const
{
	auto s = generateHtml();
			
	int index = 0;

	for (const auto& link : hyperLinks)
	{
		String linkWildcard = "{LINK" + String(index++) + "}";

		String resolvedLink;

		if (localRoot.isDirectory())
		{
			auto url = link.url.withRoot(localRoot, false);

			resolvedLink = link.url.toString(MarkdownLink::FormattedLinkHtml);
		}
		else
		{
			resolvedLink = link.url.toString(MarkdownLink::FormattedLinkHtml);
		}

		s = s.replace(linkWildcard, resolvedLink);
	}

	return s;
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
		MarkdownLayout searchLayout(textToSearch, lastWidth, stringWidthFunction, true);
		searchLayout.addYOffset((float)getTopMargin());

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

	juce_wchar c;

	while (--numCharsToSkip >= 0 && next(c))
		;

	return !it.isEmpty();
}

bool MarkdownParser::Iterator::advance(int numCharsToSkip /*= 1*/)
{
	juce_wchar c;

	while (--numCharsToSkip >= 0 && next(c))
		;
	
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

	if (c == '\n')
		currentLine++;

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

bool MarkdownParser::Iterator::matchIf(juce_wchar expected)
{
	juce_wchar c;

	if (peek() == expected)
		return next(c);

	return false;
}

void MarkdownParser::Iterator::skipWhitespace()
{
	juce_wchar c = peek();

	while (CharacterFunctions::isWhitespace(peek()))
	{
		if (!next(c))
			break;
	}
}

juce::String MarkdownParser::Iterator::getRestString(int maxLength) const
{
	if (it.isEmpty())
		return {};

	if(maxLength == -1)
	{
		return String(it);
	}
	else
	{
		maxLength = jmin(maxLength, (int)(text.end() - it));
		return String(it, it+maxLength);
	}
	
}


juce::String MarkdownParser::Iterator::advanceLine()
{
	String s;

	juce_wchar c;
	
	if (!next(c))
		return s;

	while (c != 0 && c != '\n')
	{
		s << c;
		if (!next(c))
			break;
	}

	if (c == '\n')
		s << c;

	return s;
}



int MarkdownParser::Tokeniser::readNextToken(CodeDocument::Iterator& source)
{
	return TokeniserT::readNextToken(source);
}

juce::CodeEditorComponent::ColourScheme MarkdownParser::Tokeniser::getDefaultColourScheme()
{
	CodeEditorComponent::ColourScheme s;

	s.set("normal", Colour(0xFFAAAAAA));
	s.set("headline", Colour(SIGNAL_COLOUR).withAlpha(0.7f));
	s.set("highlighted", Colours::white);
	s.set("fixed", Colours::lightblue);
	s.set("comment", Colour(0xFF777777));
	s.set("metadata", Colour(0xFFaa7777));
	s.set("link", Colour(0xFF8888FF));
	s.set("table", Colour(0xFFCCCCCC));

	return s;
}

void MarkdownParser::setStyleData(MarkdownLayout::StyleData newStyleData)
{
	styleData = newStyleData;

	if (markdownCode.isNotEmpty())
	{
		setNewText(markdownCode);
	}
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

int MarkdownParser::TokeniserT::readNextToken(CodeDocument::Iterator& source)
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
	case '-':
		{
			source.skip();

			if (source.nextChar() == '-')
			{
				if (source.nextChar() == '-')
				{
					source.skipToEndOfLine();

					while (!source.isEOF())
					{
						if (source.peekNextChar() == '-')
						{
							source.nextChar();

							if (source.nextChar() == '-')
							{
								if (source.nextChar() == '-')
								{
									source.skipToEndOfLine();
									break;
								}
							}
						}

						source.skipToEndOfLine();
					}

					return 5;
				}
				else
					return 0;
			}
			else
				return 0;
		}
	case '!':
	case '[':
		{
			source.skip();

			while(!source.isEOF() && source.peekNextChar() != ']')
				source.skip();
                
			while (!source.isEOF() && source.peekNextChar() != ')')
				source.skip();

			source.skip();

			return 6;
		}
	case '|': source.skipToEndOfLine(); return 7;
	default: source.skip(); return 0;
	}
}


void ViewportWithScrollCallback::visibleAreaChanged(const Rectangle<int>& newVisibleArea)
{
	visibleArea = newVisibleArea;

	for (int i = 0; i < listeners.size(); i++)
	{
		if (listeners[i].get() == nullptr)
			listeners.remove(i--);
		else
			listeners[i]->scrolled(visibleArea);
	}
}

int MarkdownParser::LinkResolver::Sorter::compareElements(LinkResolver* first, LinkResolver* second)
{
	auto fp = first->getPriority();
	auto sp = second->getPriority();

	if (fp > sp)
		return -1;
	else if (fp < sp)
		return 1;
	else
		return 0;
}

int MarkdownParser::ImageProvider::Sorter::compareElements(ImageProvider* first, ImageProvider* second)
{
	auto fp = first->getPriority();
	auto sp = second->getPriority();

	if (fp > sp)
		return -1;
	else if (fp < sp)
		return 1;
	else
		return 0;
}

juce::Image MarkdownParser::ImageProvider::getImage(const MarkdownLink& /*imageURL*/, float width)
{
	if (width == 0.0f)
		return {};

	Image img = Image(Image::PixelFormat::ARGB, (int)width, (int)40.0f, true);
	Graphics g(img);
	g.fillAll(Colours::grey);
	g.setColour(Colours::black);
	g.drawRect(0.0f, 0.0f, width, 40.0f, 1.0f);
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Empty", 0, 0, (int)width, (int)40.0f, Justification::centred);
	return img;
}


void MarkdownParser::ImageProvider::updateWidthFromURL(const MarkdownLink& url, float& widthToUpdate)
{
	auto extraData = url.getExtraData();

	if (extraData.isEmpty())
		return;

	float widthValue = widthToUpdate;

	auto size = MarkdownLink::Helpers::getSizeFromExtraData(extraData);

	if (size > 0.0)
	{
		widthValue = (float)size;
				
	}
	else
	{
		widthValue *= (-1.0f * (float)size);
	}

	widthToUpdate = jmin(widthToUpdate, widthValue);
}

juce::Image MarkdownParser::ImageProvider::resizeImageToFit(const Image& otherImage, float width)
{
	if (width == 0.0)
		return {};

	if (otherImage.isNull() || otherImage.getWidth() < (int)width)
		return otherImage;

	float ratio = (float)otherImage.getWidth() / width;

	auto newWidth = jmax((int)width, 10);
	auto newHeight = jmax((int)((float)otherImage.getHeight() / ratio), 10);

	return otherImage.rescaled(newWidth, newHeight);
}


}

