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
			auto l = parent->getTextLayoutForString(content, width);
			recalculateHyperLinkAreas(l, hyperLinks, getTopMargin());

			lastWidth = width;
			lastHeight = l.getHeight();

			lastHeight += 2.0f * intendation;

			return lastHeight;
		}
	}

	int getTopMargin() const override { return 10; };

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
	{}

	void draw(Graphics& g, Rectangle<float> area) override;

	float getHeightForWidth(float width) override
	{
		float marginTop = isFirst ? 0.0f : 20.0f;
		float marginBottom = 10.0f;

		if (!hyperLinks.isEmpty())
		{
			TextLayout l;
			l.createLayout(content, width);
			recalculateHyperLinkAreas(l, hyperLinks, getTopMargin());
		}

		if (content.getNumAttributes() > 0)
		{
			return content.getAttribute(0).font.getHeight() + marginTop + marginBottom;
		}

		return 0.0f;
	}

	int getTopMargin() const override { return 20 - jmax<int>(0, 3 - level) * 5; };

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
			auto l = parent->getTextLayoutForString(r.content, width - bulletPointIntendation);

			r.height = l.getHeight();
			lastHeight += l.getHeight();

			lastHeight += bulletMargin;
		}

		lastHeight += 2.0f* intendation;

		return lastHeight;
	}

	int getTopMargin() const override { return 10; };

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

			auto l = parent->getTextLayoutForString(content, widthToUse);
			recalculateHyperLinkAreas(l, hyperLinks, getTopMargin());

			for (auto& h : hyperLinks)
				h.area.translate(intendation, 2.0f * intendation);

			lastHeight = l.getHeight() + 4.0f * intendation;

			return lastHeight;
		}
		else
			return lastHeight;
	}


	int getTopMargin() const override { return 20; };

	const float intendation = 12.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	AttributedString content;
};

struct MarkdownParser::CodeBlock : public MarkdownParser::Element
{
	enum SyntaxType
	{
		Undefined,
		Cpp,
		Javascript,
		XML,
		Snippet,
		numSyntaxTypes
	};

	CodeBlock(MarkdownParser* parent, const String& code_, SyntaxType t) :
		Element(parent),
		code(code_),
		syntax(t)
	{
		code = code.trim();
		//code << "\n"; // hacky..
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
        g.setOpacity(1.0f);
		g.drawImageAt(renderedCodePreview, (int)area.getX(), (int)area.getY() + 10);

		Path p;

		p.loadPathFromData(EditorIcons::pasteIcon, sizeof(EditorIcons::pasteIcon)
		);

		auto pathBounds = hyperLinks.getFirst().area.translated(0.0, area.getY());

		p.scaleToFit(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), true);

		g.setColour(Colour(0xFF777777));
		g.fillPath(p);
	}

	float getHeightForWidth(float width) override
	{
		if (width != lastWidth)
		{
			int numLines = StringArray::fromLines(code).size();

			ScopedPointer<CodeDocument> doc = new CodeDocument();
			ScopedPointer<CodeTokeniser> tok;
			
			switch (syntax)
			{
			case Cpp: tok = new CPlusPlusCodeTokeniser(); break;
			case Javascript: tok = new JavascriptTokeniser(); break;
			case XML:	tok = new XmlTokeniser(); break;
			case Snippet: tok = new SnippetTokeniser(); break;
			default: break;
			}

			doc->replaceAllContent(code);

			ScopedPointer<CodeEditorComponent> editor = new CodeEditorComponent(*doc, tok);

			hyperLinks.clear();
			
			HyperLink copy;
			copy.area = { 2.0f, 12.0f, 14.0f, 14.0f };
			copy.url << "CLIPBOARD::" << code << "";
			copy.valid = true;
			copy.tooltip = "Copy code to clipboard";
			hyperLinks.add(std::move(copy));

			editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
			editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
			editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
			editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
			editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
			editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
			editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
			editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));
			
			editor->setSize((int)width, editor->getLineHeight() * numLines + 25);

			renderedCodePreview = editor->createComponentSnapshot(editor->getLocalBounds());

			renderedCodePreview = renderedCodePreview.getClippedImage({ 0, 0, renderedCodePreview.getWidth(), renderedCodePreview.getHeight() - 22 });

			lastWidth = width;
			lastHeight = (float)renderedCodePreview.getHeight() + 3.0f;

			return lastHeight;
		}
		else
			return lastHeight;


	}

	int getTopMargin() const override { return 20; };

	String code;

	Image renderedCodePreview;

	SyntaxType syntax;

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

		img = parent->resolveImage(imageURL, width);

		if (!img.isNull())
			return (float)img.getHeight();

		return 0.0f;
	}

	void draw(Graphics& g, Rectangle<float> area)
	{
		g.setOpacity(1.0f);
		g.drawImageAt(img, (int)area.getX(), (int)area.getY());
	}

	String getImageURL() const { return imageURL; }

	int getTopMargin() const override { return 5; };

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
			c.cellLinks = h.cellLinks;

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
				c.cellLinks = cell_.cellLinks;

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

	int getTopMargin() const override { return 10; };

	float getHeightForWidth(float width) override
	{
		if (width != lastWidth)
		{
			lastWidth = width;
			lastHeight = 20.0f;
			float y = 0.0f;

			hyperLinks.clearQuick();
			headers.updateHeight(width, y, parent);

			addLinksFromRow(headers);

			lastHeight += headers.rowHeight;

			for (auto& r : rows)
			{
				r.updateHeight(width, y, parent);
				addLinksFromRow(r);
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
		Array<HyperLink> cellLinks;
	};

	struct Row
	{
		const float intendation = 8.0f;

		float calculateHeightForCell(Cell& c, float width, MarkdownParser* parser)
		{
			if (!c.content.getText().isEmpty())
				return getHeightForAttributedString(c.content, width, parser);
			else
			{
				c.img = parser->resolveImage(c.imageURL, width - 4.0f);
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
				auto contentWidth = w - 2.0f * intendation;

				rowHeight = jmax<float>(rowHeight, calculateHeightForCell(h, contentWidth, parser) + 2.0f * intendation );

				TextLayout l;
				l.createLayoutWithBalancedLineLengths(h.content, contentWidth);
				Element::recalculateHyperLinkAreas(l, h.cellLinks, 0.0f);

				h.area = Rectangle<float>(x, 0.0f, w, rowHeight);

				for (auto& link : h.cellLinks)
					link.area.translate(x + intendation, y + 2.0f * intendation);

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
					c.content.draw(g, ar.reduced(intendation));
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

	static float getHeightForAttributedString(const AttributedString& s, float width, MarkdownParser* p)
	{
		auto l = p->getTextLayoutForString(s, width);
		return l.getHeight();
	}

	void addLinksFromRow(Row& r)
	{
		for (const auto& c : r.columns)
			hyperLinks.addArray(c.cellLinks);
	}

	Row headers;
	Array<Row> rows;

	const float intendation = 12.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	
};

}

