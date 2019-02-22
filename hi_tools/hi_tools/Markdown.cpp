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


const juce::TextLayout& MarkdownParser::LayoutCache::getLayout(const AttributedString& s, float w)
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


MarkdownParser::LayoutCache::Layout::Layout(const AttributedString& s, float w)
{
	hashCode = s.getText().hashCode64();
	width = w;
	l.createLayoutWithBalancedLineLengths(s, width);
}



juce::String MarkdownParser::FileLinkResolver::getContent(const String& url)
{
	File f = root.getChildFile(url);

	if (f.existsAsFile())
		return f.loadFileAsString();
	else
		return "File `" + f.getFullPathName() + "` not found";
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
	{
		return ImageCache::getFromFile(imageFile);
	}

	return {};
}

MarkdownParser::MarkdownParser(const String& markdownCode_, LayoutCache* c) :
	markdownCode(markdownCode_.replace("\r\n", "\n")),
	it(markdownCode),
	
	currentParseResult(Result::fail("Nothing parsed yet")),
    layoutCache(c)
{
	imageProviders.add(new ImageProvider(this)),

	history.add(markdownCode);
	historyIndex = 0;

	normalFont = GLOBAL_FONT();
	boldFont = GLOBAL_BOLD_FONT();
	headlineFont = GLOBAL_FONT();
	codeFont = GLOBAL_MONOSPACE_FONT();
}

void MarkdownParser::setFonts(Font normalFont_, Font codeFont_, Font headlineFont_, float defaultFontSize_)
{
	normalFont = normalFont_;
	codeFont = codeFont_;
	headlineFont = headlineFont_;
	defaultFontSize = defaultFontSize_;
}


void MarkdownParser::setDefaultTextSize(float fontSize)
{
	defaultFontSize = fontSize;
}

float MarkdownParser::getHeightForWidth(float width)
{
	float height = 0.0f;

	for (auto* e : elements)
	{
		height += e->getTopMargin();
		height += e->getHeightForWidthCached(width);
	}

	return height;
}


void MarkdownParser::draw(Graphics& g, Rectangle<float> area) const
{
	for (auto* e : elements)
	{
		auto heightToUse = e->getHeightForWidthCached(area.getWidth());
		auto topMargin = e->getTopMargin();
		area.removeFromTop(topMargin);
		auto ar = area.removeFromTop(heightToUse);

		e->draw(g, ar);
	}
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
		if (link.url.startsWith("CLIPBOARD::"))
		{
			String content = link.url.fromFirstOccurrenceOf("CLIPBOARD::", false, false);

			SystemClipboard::copyTextToClipboard(content);
			return true;
		}

		if (link.url.startsWith("http"))
		{
			URL url(link.url);
			url.launchInDefaultBrowser();
			return true;
		}

		String newText = resolveLink(link.url).replace("\r\n", "\n");

		history.removeRange(historyIndex, -1);
		history.add(newText);
		historyIndex = history.size() - 1;

		setNewText(newText);
		return true;
	}

	return false;
}


hise::MarkdownParser::HyperLink MarkdownParser::getHyperLinkForEvent(const MouseEvent& event, Rectangle<float> area)
{
	bool matchesURL = false;
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


struct MarkdownHelpButton::MarkdownHelp : public Component
{
	MarkdownHelp(MarkdownParser* parser, int lineWidth)
	{
		setWantsKeyboardFocus(false);

		img = Image(Image::ARGB, lineWidth, (int)parser->getHeightForWidth((float)lineWidth), true);
		Graphics g(img);

		parser->draw(g, { 0.0f, 0.0f, (float)img.getWidth(), (float)img.getHeight() });

		setSize(img.getWidth()+40, img.getHeight()+40);
		
	}

	void mouseDown(const MouseEvent& /*e*/) override
	{
		if (auto cb = findParentComponentOfClass<CallOutBox>())
		{
			cb->dismiss();
		}
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF333333));

		g.drawImageAt(img, 20, 20);
	}

	Image img;

	MarkdownParser* parser;
};

MarkdownHelpButton::MarkdownHelpButton() :
	ShapeButton("?", Colours::white.withAlpha(0.7f), Colours::white, Colours::white)
{
	setWantsKeyboardFocus(false);

	setShape(getPath(), false, true, true);

	setSize(16, 16);
	addListener(this);
}



void MarkdownHelpButton::buttonClicked(Button* /*b*/)
{
	
	if (parser != nullptr)
	{
		if (currentPopup.getComponent())
		{
			currentPopup->dismiss();
		}
		else
		{
			auto nc = new MarkdownHelp(parser, popupWidth);

			auto window = getTopLevelComponent();

			if (window == nullptr)
				return;

			auto lb = window->getLocalArea(this, getLocalBounds());

			if (nc->getHeight() > 700)
			{
				Viewport* viewport = new Viewport();

				viewport->setViewedComponent(nc);
				viewport->setSize(nc->getWidth() + viewport->getScrollBarThickness(), 700);
				viewport->setScrollBarsShown(true, false, true, false);

				currentPopup = &CallOutBox::launchAsynchronously(viewport, lb, window);
				currentPopup->setWantsKeyboardFocus(!ignoreKeyStrokes);
			}
			else
			{
				currentPopup = &CallOutBox::launchAsynchronously(nc, lb, window);
				currentPopup->setWantsKeyboardFocus(!ignoreKeyStrokes);
			}
		}

		


	}
}


juce::Path MarkdownHelpButton::getPath()
{
	static const unsigned char pathData[] = { 110,109,0,183,97,67,0,111,33,67,98,32,154,84,67,0,111,33,67,128,237,73,67,32,27,44,67,128,237,73,67,0,56,57,67,98,128,237,73,67,224,84,70,67,32,154,84,67,128,1,81,67,0,183,97,67,128,1,81,67,98,224,211,110,67,128,1,81,67,0,128,121,67,224,84,70,67,0,128,
		121,67,0,56,57,67,98,0,128,121,67,32,27,44,67,224,211,110,67,0,111,33,67,0,183,97,67,0,111,33,67,99,109,0,183,97,67,0,111,37,67,98,119,170,108,67,0,111,37,67,0,128,117,67,137,68,46,67,0,128,117,67,0,56,57,67,98,0,128,117,67,119,43,68,67,119,170,108,67,
		128,1,77,67,0,183,97,67,128,1,77,67,98,137,195,86,67,128,1,77,67,128,237,77,67,119,43,68,67,128,237,77,67,0,56,57,67,98,128,237,77,67,137,68,46,67,137,195,86,67,0,111,37,67,0,183,97,67,0,111,37,67,99,109,0,124,101,67,32,207,62,67,108,0,16,94,67,32,207,
		62,67,108,0,16,94,67,32,17,62,67,113,0,16,94,67,32,44,60,67,0,126,94,67,32,0,59,67,113,0,236,94,67,32,207,57,67,0,195,95,67,32,213,56,67,113,0,159,96,67,32,219,55,67,0,151,99,67,32,101,53,67,113,0,44,101,67,32,27,52,67,0,44,101,67,32,8,51,67,113,0,44,
		101,67,32,245,49,67,0,135,100,67,32,95,49,67,113,0,231,99,67,32,196,48,67,0,157,98,67,32,196,48,67,113,0,58,97,67,32,196,48,67,0,79,96,67,32,175,49,67,113,0,105,95,67,32,154,50,67,0,40,95,67,32,227,52,67,108,0,148,87,67,32,243,51,67,113,0,248,87,67,32,
		197,47,67,0,155,90,67,32,59,45,67,113,0,67,93,67,32,172,42,67,0,187,98,67,32,172,42,67,113,0,253,102,67,32,172,42,67,0,155,105,67,32,115,44,67,113,0,41,109,67,32,218,46,67,0,41,109,67,32,219,50,67,113,0,41,109,67,32,132,52,67,0,62,108,67,32,15,54,67,
		113,0,83,107,67,32,154,55,67,0,126,104,67,32,212,57,67,113,0,133,102,67,32,100,59,67,0,254,101,67,32,89,60,67,113,0,124,101,67,32,73,61,67,0,124,101,67,32,207,62,67,99,109,0,207,93,67,32,200,64,67,108,0,194,101,67,32,200,64,67,108,0,194,101,67,32,203,
		71,67,108,0,207,93,67,32,203,71,67,108,0,207,93,67,32,200,64,67,99,101,0,0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));


	return path;
}


bool MarkdownParser::Helpers::isNewElement(juce_wchar c)
{
	return c == '#' || c == '|' || c == '!' || c == '>' || c == '-' || c == 0 || c == '\n';
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

void MarkdownParser::Headline::draw(Graphics& g, Rectangle<float> area)
{
	float marginTop = isFirst ? 0.0f : 20.0f;
	area.removeFromTop(marginTop);

	g.setColour(Colours::grey.withAlpha(0.2f));
	g.drawHorizontalLine((int)area.getBottom() - 8, area.getX(), area.getRight());

	

	

	

	content.draw(g, area);

}

void MarkdownParser::Element::recalculateHyperLinkAreas(TextLayout& l, Array<HyperLink>& links, float topMargin)
{
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

}

