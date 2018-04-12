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




struct MarkdownParser::TextBlock : public MarkdownParser::Element
{
	TextBlock(MarkdownParser* parent, const AttributedString& s) :
		Element(parent),
		content(s)
	{}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		area.removeFromTop(intendation);
		area.removeFromBottom(intendation);

		content.draw(g, area);
	}

	float getHeightForWidth(float width) override
	{
		if (lastWidth == width)
		{
			return lastHeight;
		}
		else
		{
			TextLayout l;
			l.createLayoutWithBalancedLineLengths(content, width);

			lastWidth = width;
			lastHeight = l.getHeight();

			lastHeight += 2.0f * intendation;

			return lastHeight;
		}
	}

	AttributedString content;

	const float intendation = 5.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

};

struct MarkdownParser::Headline : public MarkdownParser::Element
{
	Headline(MarkdownParser* parent, int level_, const AttributedString& s, bool isFirst_) :
		Element(parent),
		content(s),
		level(level_),
		isFirst(isFirst_)
	{

	}

	void draw(Graphics& g, Rectangle<float> area) override;

	float getHeightForWidth(float /*width*/) override
	{


		float marginTop = isFirst ? 0.0f : 20.0f;

		float marginBottom = 10.0f;

		if (content.getNumAttributes() > 0)
		{
			return content.getAttribute(0).font.getHeight() + marginTop + marginBottom;
		}

		return 0.0f;



	}

	AttributedString content;
	int level;
	bool isFirst;
};

struct MarkdownParser::BulletPointList : public MarkdownParser::Element
{
	struct Row
	{
		AttributedString content;
		float height;
	};

	BulletPointList(MarkdownParser* parser, Array<AttributedString>& ar) :
		Element(parser)
	{
		for (const auto& r : ar)
			rows.add({ r, -1.0f });
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		area.removeFromTop(intendation);

		for (const auto& r : rows)
		{
			area.removeFromTop(bulletMargin);

			auto ar = area.removeFromTop(r.height);


			auto font = parent->normalFont.withHeight(parent->defaultFontSize);

			static const String bp = CharPointer_UTF8(" \xe2\x80\xa2 ");

			auto i = font.getStringWidthFloat(bp) + 10.0f;

			auto ba = ar.removeFromLeft(i);

			g.setColour(parent->textColour);
			g.setFont(font);
			g.drawText(bp, ba, Justification::topLeft);

			r.content.draw(g, ar);
		}
	}

	float getHeightForWidth(float width) override
	{
		if (lastWidth == width)
			return lastHeight;

		lastWidth = width;
		lastHeight = 0.0f;

		float bulletPointIntendation = parent->defaultFontSize * 2.0f;

		for (auto& r : rows)
		{
			TextLayout l;
			l.createLayoutWithBalancedLineLengths(r.content, width - bulletPointIntendation);

			r.height = l.getHeight();
			lastHeight += l.getHeight();

			lastHeight += bulletMargin;
		}

		lastHeight += 2.0f* intendation;

		return lastHeight;
	}

	const float intendation = 8.0f;

	const float bulletMargin = 10.0f;

	Array<Row> rows;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;
};

struct MarkdownParser::Comment : public MarkdownParser::Element
{
	Comment(MarkdownParser* p, const AttributedString& c) :
		Element(p),
		content(c)
	{};

	void draw(Graphics& g, Rectangle<float> area) override
	{
		area.removeFromTop(intendation);
		area.removeFromBottom(intendation);

		g.setColour(Colours::grey.withAlpha(0.2f));

		g.fillRect(area);
		g.fillRect(area.withWidth(3.0f));

		content.draw(g, area.reduced(intendation));
	}

	float getHeightForWidth(float width) override
	{
		float widthToUse = width - 2.0f * intendation;

		if (widthToUse != lastWidth)
		{
			lastWidth = widthToUse;

			TextLayout l;
			l.createLayoutWithBalancedLineLengths(content, widthToUse);

			lastHeight = l.getHeight() + 4.0f * intendation;

			return lastHeight;
		}
		else
			return lastHeight;
	}




	const float intendation = 12.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	AttributedString content;
};

struct MarkdownParser::CodeBlock : public MarkdownParser::Element
{
	CodeBlock(MarkdownParser* parent, const String& code_) :
		Element(parent),
		code(code_)
	{
		code = code.trim();
		code << "\n"; // hacky..
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
        g.setOpacity(1.0f);
		g.drawImageAt(renderedCodePreview, (int)area.getX(), (int)area.getY() + 10);
	}

	float getHeightForWidth(float width) override
	{
		if (width != lastWidth)
		{
			int numLines = StringArray::fromLines(code).size();

			ScopedPointer<CodeDocument> doc = new CodeDocument();
			ScopedPointer<JavascriptTokeniser> tok = new JavascriptTokeniser();

			

			doc->replaceAllContent(code);

			ScopedPointer<CodeEditorComponent> editor = new CodeEditorComponent(*doc, tok);

			editor->setSize((int)width, editor->getLineHeight() * numLines + 25);

			editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
			editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
			editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
			editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
			editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
			editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
			editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));

			editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

			renderedCodePreview = editor->createComponentSnapshot(editor->getLocalBounds());

			renderedCodePreview = renderedCodePreview.getClippedImage({ 0, 0, renderedCodePreview.getWidth(), renderedCodePreview.getHeight() - 22 });

			lastWidth = width;
			lastHeight = (float)renderedCodePreview.getHeight() + 3.0f;

			return lastHeight;
		}
		else
			return lastHeight;


	}

	String code;

	Image renderedCodePreview;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;
};


struct MarkdownParser::ImageElement : public MarkdownParser::Element
{
	ImageElement(MarkdownParser* parent, const String& imageName_, const String& imageURL_) :
		Element(parent),
		imageName(imageName_),
		imageURL(imageURL_)
	{};

	float getHeightForWidth(float width) override
	{
		if (!img.isNull())
			return (float)img.getHeight();

		if (parent->imageProvider != nullptr)
		{
			img = parent->imageProvider->getImage(imageURL, width);
			return (float)img.getHeight();
		}

		return 0.0f;
	}

	void draw(Graphics& g, Rectangle<float> area)
	{
		g.setOpacity(1.0f);
		g.drawImageAt(img, (int)area.getX(), (int)area.getY());
	}

	String getImageURL() const { return imageURL; }

private:

	Image img;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	String imageName;
	String imageURL;
};


struct MarkdownParser::MarkdownTable : public MarkdownParser::Element
{
	MarkdownTable(MarkdownParser* p, const RowContent& headerItems, const Array<int>& lengths, const Array<RowContent>& entries) :
		Element(p)
	{
		int index = 0;

		for (const auto& h : headerItems)
		{
			Cell c;
			c.content = h.s;
			c.imageURL = h.imageURL;
			c.index = index;
			c.area = {};
			c.length = lengths[index];

			headers.columns.add(c);

			index++;
		}

		index = 0;

		for (const auto& e : entries)
		{
			Row newRow;

			int j = 0;

			for (const auto& cell_ : e)
			{
                Cell c;
                
                c.content = cell_.s;
                c.imageURL = cell_.imageURL;
                c.index = j;
                c.area = {};
                c.length = lengths[j];
                
				newRow.columns.add(c);
				j++;
			}

			newRow.index = index++;

			rows.add(newRow);
		}
	};

	void draw(Graphics& g, Rectangle<float> area) override
	{
		area.removeFromTop(10.0f);
		area.removeFromBottom(10.0f);

		

		g.setColour(Colours::grey.withAlpha(0.2f));
		g.fillRect(area);

		auto headerArea = area.removeFromTop(headers.rowHeight);
		g.fillRect(headerArea);
		
		g.setColour(Colours::grey.withAlpha(0.2f));

		g.drawVerticalLine((int)area.getX(), area.getY(), area.getBottom());

		for (const auto& c : headers.columns)
		{
			
			if(c.index != headers.columns.getLast().index)
				g.drawVerticalLine((int)c.area.getRight(), area.getY(), area.getBottom());

		}

		g.drawVerticalLine((int)area.getRight()-1, area.getY(), area.getBottom());
		
		


		headers.draw(g, headerArea);


		for (auto& r : rows)
		{
			auto rowArea = area.removeFromTop(r.rowHeight);
			r.draw(g, rowArea);

			g.setColour(Colours::grey.withAlpha(0.2f));
			g.drawHorizontalLine((int)rowArea.getBottom(), rowArea.getX(), rowArea.getRight());
		}
	}

	float getHeightForWidth(float width) override
	{
		if (width != lastWidth)
		{
			lastWidth = width;

			lastHeight = 20.0f;

			float y = 0.0f;

			headers.updateHeight(width, y, parent);

			lastHeight += headers.rowHeight;

			for (auto& r : rows)
			{
				r.updateHeight(width, y, parent);
				lastHeight += r.rowHeight;
			}

			return lastHeight;
		}
		else
			return lastHeight;
	}

	struct Cell
	{
		bool operator ==(const Cell& other) const
		{
			return index == other.index;
		}

		AttributedString content;
		String imageURL;
		int index = -1;
		Rectangle<float> area;
		int length;
		Image img;
	};

	struct Row
	{
		const float intendation = 8.0f;

		float calculateHeightForCell(Cell& c, float width, MarkdownParser* parser)
		{
			if (!c.content.getText().isEmpty())
			{
				return getHeightForAttributedString(c.content, width);
			}
			else
			{
				c.img = parser->imageProvider->getImage(c.imageURL, width - 4.0f);
				return (float)c.img.getHeight();
			}
		}

		void updateHeight(float width, float& y, MarkdownParser* parser)
		{
			rowHeight = 0.0f;

			float x = 0.0f;

			totalLength = 0;

			for (const auto& c : columns)
				totalLength += c.length;


			for (auto& h : columns)
			{
				float w = getColumnWidth(width, h.index);

				rowHeight = jmax<float>(rowHeight, calculateHeightForCell(h, w - 2.0f * intendation, parser) + 2.0f * intendation );

				h.area = Rectangle<float>(x, 0.0f, w, rowHeight);

				x += w;
			}

			y += rowHeight;
		}

		void draw(Graphics& g, Rectangle<float> area)
		{
			for (const auto& c : columns)
			{
				auto ar = c.area.withX(c.area.getX() + area.getX());
				ar = ar.withY(ar.getY() + area.getY());

				Random r;

				if (c.img.isNull())
				{
					c.content.draw(g, ar.reduced(intendation));
				}
				else
				{
					g.setOpacity(1.0f);
					ar = ar.withSizeKeepingCentre((float)c.img.getWidth(), (float)c.img.getHeight());
					g.drawImageAt(c.img, (int)ar.getX(), (int)ar.getY());
				}
			}
		}

		float getColumnWidth(float width, int columnIndex) const
		{
			if (totalLength > 0)
			{
				float fraction = (float)columns[columnIndex].length / (float)totalLength;
				return fraction * width;
			}
			else return
				0.0f;
		}

		Array<Cell> columns;
		int index = -1;

		float rowHeight = -1.0f;

		int totalLength;
	};

	static float getHeightForAttributedString(const AttributedString& s, float width)
	{
		TextLayout l;
		l.createLayoutWithBalancedLineLengths(s, width);

		return l.getHeight();
	}

	Row headers;
	Array<Row> rows;

	const float intendation = 12.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	
};

MarkdownParser::MarkdownParser(const String& markdownCode_) :
	markdownCode(markdownCode_),
	it(markdownCode_),
	imageProvider(new ImageProvider(this))
{
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

void MarkdownParser::Iterator::skipWhitespace()
{
	juce_wchar c = peek();

	while (CharacterFunctions::isWhitespace(c))
	{
		if (!next(c))
			break;
	}
}


void MarkdownParser::parse()
{
	while (it.peek() != 0)
		parseBlock();
	
}

void MarkdownParser::parseLine()
{
	resetForNewLine();
	currentColour = textColour.withAlpha(0.8f);
	parseText(); 
	elements.add(new TextBlock(this, currentlyParsedBlock));
}

void MarkdownParser::resetForNewLine()
{
	currentFont = normalFont.withHeight(defaultFontSize);
	currentFont.setBold(false);
	currentColour = textColour;
	resetCurrentBlock();
}

void MarkdownParser::parseHeadline()
{
	resetCurrentBlock();

	currentColour = Colour(SIGNAL_COLOUR);

	juce::juce_wchar c = it.peek();
	int headlineLevel = 3;

	while (it.next(c) && c == '#' && headlineLevel > 0)
	{
		headlineLevel--;
	}

	currentFont = headlineFont.withHeight(defaultFontSize + 5 * headlineLevel);
	currentFont.setBold(true);

	if (it.peek() == ' ')
		it.advance();

	parseText();

	isBold = false;

	elements.add(new Headline(this, headlineLevel, currentlyParsedBlock, elements.size() == 0));

}

void MarkdownParser::parseBulletList()
{
	Array<AttributedString> bulletpoints;

	while (it.peek() == '-')
	{
		skipTagAndTrailingSpace();

		resetCurrentBlock();
		resetForNewLine();

		parseText();

		bulletpoints.add(currentlyParsedBlock);
	}

	elements.add(new BulletPointList(this, bulletpoints));


	currentFont = normalFont.withHeight(defaultFontSize);

}



void MarkdownParser::parseText()
{
	juce_wchar c;

	it.next(c);

	while (!Helpers::isEndOfLine(c))
	{
		switch (c)
		{
		case '*':
		{
			if (!isCode)
			{
				if (it.peek() == '*')
				{
					it.next(c);

					isBold = !isBold;

					float size = currentFont.getHeight();

					if (isBold)
						currentFont = boldFont.withHeight(size);
					else
						currentFont = normalFont.withHeight(size);
				}
				else
				{
					isItalic = !isItalic;
					currentFont.setItalic(isItalic);
				}
			}
			else
			{
				addCharacterToCurrentBlock(c);
			}

			break;
		}
		case '`':
		{
			isCode = !isCode;

			auto size = currentFont.getHeight();
			auto b = currentFont.isBold();
			auto i = currentFont.isItalic();
			auto u = currentFont.isUnderlined();

			currentFont = isCode ? codeFont : normalFont;

			currentColour = isCode ? textColour : textColour.withAlpha(0.8f);

			currentFont.setHeight(size);
			currentFont.setBold(b);
			currentFont.setItalic(i);
			currentFont.setUnderline(u);

			break;
		}
		case ' ':
		{
			if (it.peek() == ' ')
			{
				it.next(c);
				addCharacterToCurrentBlock('\n');
			}
			else
			{
				addCharacterToCurrentBlock(c);
			}

			break;
		}
		case '|':
			if (!isCode)
			{
				return;
			}
		default:
			addCharacterToCurrentBlock(c);
			
		}

		if (!it.next(c))
			return;
	}
}



void MarkdownParser::parseBlock()
{
	juce_wchar c = it.peek();

	switch (c)
	{
	case '#': parseHeadline(); 
			  break;
	case '-': parseBulletList(); 
			  break;
	case '>': parseComment(); 
			  break;
	case '`': if (isJavascriptBlock())
				  parseJavascriptBlock();
			  else
				  parseLine();
			  break;
	case '|': parseTable();
			  break;
	case '!': if (isImageLink())
				  elements.add(parseImage());
			  else
				  parseLine();
			  break;
	default:  parseLine(); 
			  break;
	
	};
}

void MarkdownParser::parseJavascriptBlock()
{
	auto code = it.getRestString();
	
	code = code.fromFirstOccurrenceOf("```javascript", false, false);
	
	code = code.upToFirstOccurrenceOf("```", false, false);

	it.advance(code.length() + 16);

	elements.add(new CodeBlock(this, code));
}

void MarkdownParser::parseTable()
{
	RowContent headerItems;


	it.peek();

	while (!Helpers::isEndOfLine(it.peek()))
	{
		skipTagAndTrailingSpace();

		resetForNewLine();
		resetCurrentBlock();

		CellContent newCell;

		if (isImageLink())
		{
			ScopedPointer<ImageElement> e = parseImage();
			newCell.imageURL = e->getImageURL();
		}
		else
		{
			parseText();
			currentlyParsedBlock.setFont(boldFont.withHeight(defaultFontSize));
			newCell.s = currentlyParsedBlock;
		}
			

		if(!newCell.isEmpty())
			headerItems.add(newCell);
	}

	
	it.advance();

	Array<int> lengths;
	
	while (!Helpers::isEndOfLine(it.peek()))
	{
		resetCurrentBlock();

		parseText();

		if(!currentlyParsedBlock.getText().containsOnly("-=_ "))	
		{
			jassertfalse;
			return;
		}

		auto text = currentlyParsedBlock.getText().trim();

		if(text.isNotEmpty())
			lengths.add(text.length());
	}
	
	Array<RowContent> rows;

	it.advance();

	while (it.peek() == '|')
	{
		rows.add(parseTableRow());
	}
	
	elements.add(new MarkdownTable(this, headerItems, lengths, rows));

}

MarkdownParser::ImageElement* MarkdownParser::parseImage()
{
	it.match('!');
	it.match('[');

	auto imageName = it.getRestString().upToFirstOccurrenceOf("]", false, false);

	it.advance(imageName);

	it.match(']');
	it.match('(');

	auto imageLink = it.getRestString().upToFirstOccurrenceOf(")", false, false);

	it.advance(imageLink);

	it.match(')');

	return new ImageElement(this, imageName, imageLink);

	
}

Array<MarkdownParser::CellContent> MarkdownParser::parseTableRow()
{
	Array<CellContent> entries;

	while (!Helpers::isEndOfLine(it.peek()))
	{
		skipTagAndTrailingSpace();
		resetCurrentBlock();
		resetForNewLine();

		CellContent c;

		if (isImageLink())
		{
			ScopedPointer<ImageElement> e = parseImage();
			c.imageURL = e->getImageURL();
		}
		else
		{
			parseText();
			c.s = currentlyParsedBlock;
		}
		
		if (!c.isEmpty())
			entries.add(c);
	}

	it.advance();

	return entries;
}

bool MarkdownParser::isJavascriptBlock() const
{
	auto restString = it.getRestString();
	return restString.startsWith("```javascript");
}

bool MarkdownParser::isImageLink() const
{
	auto restString = it.getRestString();
	return restString.startsWith("![");
}

void MarkdownParser::addCharacterToCurrentBlock(juce_wchar c)
{
	currentlyParsedBlock.append(String::charToString(c), currentFont, currentColour);
}

void MarkdownParser::resetCurrentBlock()
{
	currentlyParsedBlock = AttributedString();
	currentlyParsedBlock.setLineSpacing(8.0f);
}

void MarkdownParser::skipTagAndTrailingSpace()
{
	it.advance();

	if (it.peek() == ' ')
		it.advance();
}

void MarkdownParser::parseComment()
{
	resetForNewLine();

	skipTagAndTrailingSpace();

	parseText();

	elements.add(new Comment(this, currentlyParsedBlock));

	
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


bool MarkdownParser::Helpers::isEndOfLine(juce_wchar c)
{
	return c == '\r' || c == '\n' || c == 0;
}

void MarkdownParser::Headline::draw(Graphics& g, Rectangle<float> area)
{
	float marginTop = isFirst ? 0.0f : 20.0f;
	area.removeFromTop(marginTop);

	g.setColour(Colours::grey.withAlpha(0.2f));
	g.drawHorizontalLine((int)area.getBottom() - 8, area.getX(), area.getRight());

	

	

	

	content.draw(g, area);

}

}

