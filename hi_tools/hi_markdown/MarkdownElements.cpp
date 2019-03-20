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

float getHeightForLayout(const MarkdownLayout& l)
{
	return l.getHeight();
}


struct MarkdownParser::TextBlock : public MarkdownParser::Element
{
	TextBlock(MarkdownParser* parent, const AttributedString& s) :
		Element(parent),
		content(s),
		l(content, 0.0f)
	{}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		area.removeFromTop(intendation);
		area.removeFromBottom(intendation);

		drawHighlight(g, area);

		l.drawCopyWithOffset(g, area);
	}

	float getHeightForWidth(float width) override
	{
		if (lastWidth == width)
		{
			return lastHeight;
		}
		else
		{
			l = { content, width };

			l.addYOffset(getTopMargin());
			l.styleData = parent->styleData;

			recalculateHyperLinkAreas(l, hyperLinks, getTopMargin());

			lastWidth = width;
			lastHeight = getHeightForLayout(l);

			lastHeight += 2.0f * intendation;

			return lastHeight;
		}
	}

	void searchInContent(const String& searchString) override
	{
		searchInStringInternal(content, searchString);
	}

	String generateHtml() const override
	{
		String html;
		NewLine nl;

		HtmlGenerator g;

		int linkIndex = 0;
		auto c = g.createFromAttributedString(content, linkIndex);
		html << g.surroundWithTag(c, "p");

		return html;
	}

	String getTextToCopy() const override
	{
		return content.getText();
	}

	int getTopMargin() const override { return 10; };

	AttributedString content;
	MarkdownLayout l;

	const float intendation = 5.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

};


struct MarkdownParser::ActionButton : public Element,
									  public ButtonListener
{
	struct ButtonLookAndFeel : public LookAndFeel_V3
	{
		ButtonLookAndFeel(MarkdownParser& parent_) :
			parent(parent_)
		{}

		void drawButtonBackground(Graphics& g, Button&, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override
		{
			g.setColour(parent.styleData.linkBackgroundColour);

			g.fillAll();

			if (isMouseOverButton)
			{
				g.fillAll(Colours::black.withAlpha(0.1f));
			}

			if (isButtonDown)
			{
				g.fillAll(Colours::black.withAlpha(0.1f));
			}
		}

		void drawButtonText(Graphics& g, TextButton& b, bool isMouseOverButton, bool isButtonDown) override
		{
			g.setFont(parent.styleData.getFont());
			g.setColour(parent.styleData.textColour);
			g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
		}

		MarkdownParser& parent;
	};

	ActionButton(MarkdownParser* parent, const String& text_, const String& url_) :
		Element(parent),
		blaf(*parent),
		text(text_),
		url(url_)
	{};

	~ActionButton()
	{
		button = nullptr;
	}

	void draw(Graphics& g, Rectangle<float> totalArea) override
	{

	}

	int getTopMargin() const override
	{
		return 20;
	}

	void searchInContent(const String& searchString) override
	{
		
	}

	String getTextToCopy() const override
	{
		return text;
	}

	float getHeightForWidth(float width) override
	{
		return (float)getPreferredHeight();
	}

	Component* createComponent(int maxWidth) override
	{
		if (button == nullptr)
		{
			button = new TextButton();
			
			button->setButtonText(text);
			button->addListener(this);
			button->setLookAndFeel(&blaf);
			button->setSize(jmin(maxWidth, 200), getPreferredHeight());
		}
		
		return button;
	}

	int getPreferredHeight() const
	{
		return parent->styleData.getFont().getHeight() + 20;
	}

	void buttonClicked(Button* b) override
	{
		auto f = [this]()
		{
			parent->gotoLink(MarkdownLink::createWithoutRoot(url));
		};

		MessageManager::callAsync(f);
	}

	ButtonLookAndFeel blaf;

	ScopedPointer<TextButton> button;
	String text;
	String url;



};

struct MarkdownParser::Headline : public MarkdownParser::Element
{
	Headline(MarkdownParser* parent, int level_, const String& imageURL_, const AttributedString& s, bool isFirst_) :
		Element(parent),
		content(s),
		level(level_),
		isFirst(isFirst_),
		imageURL(imageURL_),
		l(s, 0.0f)
	{
		anchorURL = "#" + s.getText().toLowerCase().replaceCharacters(" ", "-");
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		anchorY = area.getY() - 15.0f;

		float marginTop = isFirst ? 0.0f : 20.0f * getZoomRatio();
		area.removeFromTop(marginTop);

		int imgOffset = 0;

		if (img.isValid())
		{
			auto b = area.removeFromLeft(img.getWidth()).toNearestInt().reduced(5);

			g.drawImageAt(img, b.getX(), area.getY());

			area.removeFromLeft(5.0f);

			imgOffset = 15;
		}

		drawHighlight(g, area);

		g.setColour(Colours::grey.withAlpha(0.2f));

		

		g.drawHorizontalLine((int)(area.getBottom() - 12.0f * getZoomRatio()), area.getX() + imgOffset, area.getRight());

		l.drawCopyWithOffset(g, area);
	}



	float getHeightForWidth(float width) override
	{
		float marginTop = isFirst ? 0.0f : 20.0f * getZoomRatio();
		float marginBottom = 10.0f * getZoomRatio();

		l = { content, width };
		l.addYOffset(getTopMargin());

		

		l.styleData = parent->styleData;

		l.styleData.textColour = l.styleData.headlineColour;
		l.styleData.codeColour = l.styleData.headlineColour;
		l.styleData.linkColour = l.styleData.headlineColour;
		l.styleData.linkBackgroundColour = Colours::white.withAlpha(0.1f);
		l.styleData.codebackgroundColour = Colours::white.withAlpha(0.1f);

		if (imageURL.isNotEmpty())
		{
			Colour tColour = parent->getStyleData().textColour;
			parent->getStyleData().textColour = parent->getStyleData().headlineColour;
			img = parent->resolveImage(MarkdownLink::createWithoutRoot(imageURL), l.getHeight());
			parent->getStyleData().textColour = tColour;
		}

		return l.getHeight() + marginTop + marginBottom;
	}

	virtual void addImageLinks(StringArray& sa) override
	{
		if (imageURL.isNotEmpty())
			sa.add(imageURL + ":256px");
	};

	void prepareLinksForHtmlExport(const String& baseURL)
	{
		Element::prepareLinksForHtmlExport(baseURL);

		if (imageURL.isNotEmpty())
		{
			MarkdownLink l({}, imageURL);

			imageURL = l.toString(MarkdownLink::FormattedLinkHtml);
		}
	}

	String getTextToCopy() const override
	{
		return content.getText();
	}

	String generateHtml() const override
	{
		String html;
		NewLine nl;

		HtmlGenerator g;

		int linkIndex = 0;

		String c;

		if (imageURL.isNotEmpty())
		{
			c << g.surroundWithTag("", "img", "style=\"max-height: 1.5em;\" src=\"" + imageURL + "\"");

		}

		c << g.createFromAttributedString(content, linkIndex);

		html << g.surroundWithTag(c, "h" + String(3 - level), "id=\"" + anchorURL.substring(1) + "\"");

		return html;
	}

	void searchInContent(const String& searchString) override
	{
		searchInStringInternal(content, searchString);
	}

	int getTopMargin() const override 
	{ 
		return (int)((float)(30 - jmax<int>(0, 3 - level) * 5) * getZoomRatio()); 
	};

	float anchorY;
	String anchorURL;

	AttributedString content;
	MarkdownLayout l;
	int level;
	bool isFirst;
	String imageURL;
	Image img;
};



struct MarkdownParser::BulletPointList : public MarkdownParser::Element
{
	struct Row
	{
		AttributedString content;
		MarkdownLayout l;
		Array<HyperLink> links;
	};

	BulletPointList(MarkdownParser* parser, Array<AttributedString>& ar, Array<Array<HyperLink>>& links) :
		Element(parser)
	{
		for (int i = 0; i < ar.size(); i++)
			rows.add({ ar[i],{ ar[i], 0.0f }, links[i] });
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		drawHighlight(g, area);

		area.removeFromTop(intendation);

		for (const auto& r : rows)
		{
			area.removeFromTop(bulletMargin);
			auto ar = area.removeFromTop(r.l.getHeight());
			auto font = parent->styleData.f.withHeight(parent->styleData.fontSize);
			static const String bp = CharPointer_UTF8(" \xe2\x80\xa2 ");
			auto i = font.getStringWidthFloat(bp) + 10.0f;

			g.setColour(parent->styleData.textColour);
			g.setFont(font);
			g.drawText(bp, ar.translated(0.0f, (float)-font.getHeight()), Justification::topLeft);

			r.l.drawCopyWithOffset(g, ar);
		}
	}

	String getTextToCopy() const override
	{
		String s;

		for (auto r : rows)
			s << "- " << r.content.getText() << "\n";

		return s;
	}

	virtual String getHtmlListTag() const { return "ul"; }

	String generateHtml() const override
	{
		String html;
		NewLine nl;

		HtmlGenerator g;

		int linkIndex = 0;
		String listItems;

		for (auto r : rows)
		{
			auto c = g.createFromAttributedString(r.content, linkIndex);
			listItems << g.surroundWithTag(c, "li");
		}

		html << g.surroundWithTag(listItems, getHtmlListTag());

		return html;
	}

	void searchInContent(const String& searchString) override
	{
		RectangleList<float> allMatches;

		float bulletPointIntendation = parent->styleData.fontSize * 1.2f;

		float y = bulletMargin;

		for (auto r : rows)
		{
			searchInStringInternal(r.content, searchString);

			searchResults.offsetAll(bulletPointIntendation, y);

			y += r.l.getHeight();
			y += bulletMargin;

			for (auto r : searchResults)
				allMatches.add(r);
		}

		searchResults = allMatches;
	}
	

	float getHeightForWidth(float width) override
	{
		if (lastWidth == width)
			return lastHeight;

		lastWidth = width;
		lastHeight = 0.0f;

		float bulletPointIntendation = parent->styleData.fontSize * 1.2f;

		hyperLinks.clear();

		for (auto& r : rows)
		{
			r.l = { r.content, width - bulletPointIntendation };
			r.l.addXOffset(bulletPointIntendation);
			r.l.styleData = parent->styleData;

			lastHeight += bulletMargin;

			recalculateHyperLinkAreas(r.l, r.links, lastHeight + getTopMargin() + intendation);

			lastHeight += r.l.getHeight();
			

			for (auto link : r.links)
				hyperLinks.add(link);

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

struct MarkdownParser::EnumerationList : public MarkdownParser::BulletPointList
{
	EnumerationList(MarkdownParser* parent, Array<AttributedString>& list, Array<Array<HyperLink>>& links) :
		BulletPointList(parent, list, links)
	{};

	String getTextToCopy() const override
	{
		String s;

		int index = 1;

		for (auto r : rows)
			s << index++ << ". " << r.content.getText() << "\n";

		return s;
	}

	virtual String getHtmlListTag() const { return "ol"; }

	void draw(Graphics& g, Rectangle<float> area) override
	{
		drawHighlight(g, area);

		area.removeFromTop(intendation);

		int rowIndex = 1;

		for (const auto& r : rows)
		{
			area.removeFromTop(bulletMargin);
			auto ar = area.removeFromTop(r.l.getHeight());
			auto font = FontHelpers::getFontBoldened(parent->styleData.getFont());
			
			String bp;
			bp << rowIndex++ << ".";

			auto i = font.getStringWidthFloat(bp) + 10.0f;

			float xDelta = 5.0f;
			float yDelta = 3.0f;

			g.setColour(parent->styleData.textColour.withMultipliedAlpha(0.8f));
			g.setFont(font);
			g.drawText(bp, ar.translated(xDelta, yDelta + (float)-font.getHeight()), Justification::topLeft);

			r.l.drawCopyWithOffset(g, ar);
		}
	}
};

struct MarkdownParser::Comment : public MarkdownParser::Element
{
	Comment(MarkdownParser* p, const AttributedString& c) :
		Element(p),
		l(c, 0.0f),
		content(c)
	{};

	void draw(Graphics& g, Rectangle<float> area) override
	{
		auto thisIndentation = intendation * getZoomRatio();

		area.removeFromBottom(2 * thisIndentation);

		drawHighlight(g, area);



		g.setColour(Colours::grey.withAlpha(0.2f));

		g.fillRect(area);
		g.fillRect(area.withWidth(3.0f * getZoomRatio()));

		

		l.drawCopyWithOffset(g, area);
	}

	String generateHtml() const override
	{
		String html;

		HtmlGenerator g;
		int linkIndex = 0;
		auto c = g.createFromAttributedString(content, linkIndex);
		html << g.surroundWithTag(c, "p", "class=\"comment\"");
		
		return html;
	}

	float getHeightForWidth(float width) override
	{
		auto thisIndentation = intendation * getZoomRatio();

		float widthToUse = width - (2.0f * thisIndentation);

		if (widthToUse != lastWidth)
		{
			lastWidth = widthToUse;

			l = { content, widthToUse - thisIndentation };
			l.addYOffset(getTopMargin() + thisIndentation);
			l.addXOffset(thisIndentation);
			l.styleData = parent->styleData;
			recalculateHyperLinkAreas(l, hyperLinks, getTopMargin());

			lastHeight = l.getHeight() + thisIndentation;

			lastHeight += thisIndentation * 2.0f;

			return lastHeight;
		}
		else
			return lastHeight;
	}

	void searchInContent(const String& searchString) override
	{
		auto thisIndentation = intendation * getZoomRatio();

		searchInStringInternal(content, searchString);
		searchResults.offsetAll(thisIndentation, thisIndentation);

	}

	String getTextToCopy() const override
	{
		return content.getText();
	}

	int getTopMargin() const override { return intendation * getZoomRatio(); };

	const float intendation = 12.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	MarkdownLayout l;
	AttributedString content;
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
		if (!img.isNull() && width == lastWidth)
			return (float)img.getHeight();

		lastWidth = width;

		img = parent->resolveImage(MarkdownLink::createWithoutRoot(imageURL), width);

		if (imageURL.endsWith("gif"))
			isGif = true;

		if (!img.isNull())
			return (float)img.getHeight() + (isGif ? 50.0f : 0.0f);

		return 0.0f;
	}

	void draw(Graphics& g, Rectangle<float> area)
	{
		g.setOpacity(1.0f);
		g.drawImageAt(img, (int)area.getX(), (int)area.getY());
	}

	String getTextToCopy() const override
	{
		return imageName;
	}

	void addImageLinks(StringArray& sa) override
	{
		sa.add(imageURL);
	}

	String getImageURL() const { return imageURL; }

	int getTopMargin() const override { return 5; };

	String generateHtml() const override
	{
		HtmlGenerator g;

		return g.surroundWithTag(imageName, "img", "src=\"" + imageURL + "\"");
	}

	void prepareLinksForHtmlExport(const String& baseURL)
	{
		Element::prepareLinksForHtmlExport(baseURL);

		MarkdownLink l({}, imageURL);

		imageURL = l.toString(MarkdownLink::FormattedLinkHtml);
	}

	Component* createComponent(int maxWidth) override
	{
		if (isGif && gifPlayer == nullptr)
		{
			gifPlayer = new GifPlayer(*this);
			
		}

		if(gifPlayer != nullptr)
			gifPlayer->setSize(jmax(50, img.getWidth()), jmax(50, img.getHeight()));

		return gifPlayer;
	}

	struct GifPlayer : public Component,
					   public ViewportWithScrollCallback::Listener
	{
		GifPlayer(ImageElement& parent):
			p(parent)
		{

		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colours::grey.withAlpha(0.5f));
			g.setColour(Colours::black.withAlpha(0.5f));
			
			auto ar = getLocalBounds().withSizeKeepingCentre(100, 100).toFloat();

			g.fillEllipse(ar.toFloat());
			g.setColour(Colours::white.withAlpha(0.5f));

			Path p;

			p.addTriangle(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
			ar = ar.reduced(25.0f);

			p.scaleToFit(ar.getX() + 5.0f, ar.getY(), ar.getWidth(), ar.getHeight(), true);
			g.fillPath(p);

		}

		void scrolled(Rectangle<int> visibleArea) override
		{
			if (!visibleArea.contains(getBoundsInParent()))
			{
				gifPlayer = nullptr;
			}
		}

		void mouseEnter(const MouseEvent& e)
		{
			setMouseCursor(MouseCursor(MouseCursor::PointingHandCursor));
		}

		void mouseExit(const MouseEvent& e)
		{
			setMouseCursor(MouseCursor(MouseCursor::NormalCursor));
		}

		void mouseDown(const MouseEvent&)
		{
			if(auto viewport = findParentComponentOfClass<ViewportWithScrollCallback>())
			{
				viewport->addListener(this);
			}

			setMouseCursor(MouseCursor(MouseCursor::NormalCursor));

			addAndMakeVisible(gifPlayer = new WebBrowserComponent());
			gifPlayer->setSize(p.img.getWidth() + 50, p.img.getHeight() + 50);
			gifPlayer->setTopLeftPosition(0, 0);
			gifPlayer->goToURL(p.imageURL);
			gifPlayer->addMouseListener(this, true);
		}

		ImageElement& p;
		ScopedPointer<WebBrowserComponent> gifPlayer;

	};

private:

	ScopedPointer<GifPlayer> gifPlayer;

	bool isGif = false;
	Image img;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	String imageName;
	String imageURL;
};


struct MarkdownParser::MarkdownTable : public MarkdownParser::Element
{
	static constexpr int FixOffset = 100000;

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

	String getTextToCopy() const override
	{
		return "Table";
	}

	String generateHtml() const override
	{
		String html;

		HtmlGenerator g;

		int linkIndex = 0;
		String headHtml;

		for (auto& c : headers.columns)
		{
			auto cContent = g.createFromAttributedString(c.content, linkIndex);

			headHtml << g.surroundWithTag(cContent, "td");
			
		}

		html << g.surroundWithTag(headHtml, "thead");

		for (auto& r : rows)
		{
			String rowHtml;

			for (auto& c : r.columns)
			{
				String cContent;

				if (c.imageURL.isNotEmpty())
				{
					cContent << g.surroundWithTag("", "img", "src=\"" + c.imageURL + "\"");
				}
				else
				{
					cContent << g.createFromAttributedString(c.content, linkIndex);
				}

				

				rowHtml << g.surroundWithTag(cContent, "td");
				
			}
			
			html << g.surroundWithTag(rowHtml, "tr");

		}

		return g.surroundWithTag(html, "table");
	}

	void addImageLinks(StringArray& sa) override
	{
		headers.addImageLinks(sa);

		for (auto& r : rows)
			r.addImageLinks(sa);
	}

	void prepareLinksForHtmlExport(const String& baseURL) override
	{
		Element::prepareLinksForHtmlExport(baseURL);

		headers.prepareImageLinks(baseURL);

		for (auto& r : rows)
			r.prepareImageLinks(baseURL);
	}

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
		Cell() : l({}, 0.0f) {};

		bool operator ==(const Cell& other) const
		{
			return index == other.index;
		}

		AttributedString content;
		MarkdownLayout l;

		int getFixedLength() const
		{
			if (length > FixOffset)
				return length - FixOffset;

			return -1;
		}

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

		void addImageLinks(StringArray& sa)
		{
			for (auto& c : columns)
			{
				if (c.imageURL.isNotEmpty())
					sa.add(c.imageURL);
			}
		}

		void prepareImageLinks(const String& baseURL)
		{
			for (auto& c : columns)
			{
				if (c.imageURL.isNotEmpty())
				{
					auto link = MarkdownLink({}, c.imageURL);
					c.imageURL = link.toString(MarkdownLink::FormattedLinkHtml, {});;
				}
					
			}
		}

		float calculateHeightForCell(Cell& c, float width, MarkdownParser* parser)
		{
			if (!c.content.getText().isEmpty())
				return c.l.getHeight();
			else
			{
				c.img = parser->resolveImage(MarkdownLink::createWithoutRoot(c.imageURL), width - 4.0f);
				return (float)c.img.getHeight();
			}
		}

		void updateHeight(float width, float& y, MarkdownParser* parser)
		{
			rowHeight = 0.0f;
			float x = 0.0f;
			totalLength = 0;

			for (const auto& c : columns)
			{
				if (c.getFixedLength() != -1)
					continue;

				totalLength += c.length;
			}
				

			for (auto& h : columns)
			{
				float w = getColumnWidth(width, h.index);
				auto contentWidth = w - 2.0f * intendation;

				h.l = MarkdownLayout(h.content, contentWidth);
				h.l.styleData = parser->styleData;
				
				rowHeight = jmax(rowHeight, calculateHeightForCell(h, contentWidth, parser) + 2.0f * intendation);

				h.l.addYOffset(2.0f * intendation + 10);
				
				h.l.addXOffset(x + intendation);

				Element::recalculateHyperLinkAreas(h.l, h.cellLinks, y + 2.0f * intendation);

				

				h.area = Rectangle<float>(x, 0.0f, w, rowHeight);

#if 0
				for (auto& link : h.cellLinks)
					link.area.translate(x + intendation, y + 2.0f * intendation);
#endif

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
					c.l.drawCopyWithOffset(g, area);
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
			auto fixedLength = columns[columnIndex].getFixedLength();

			if (fixedLength != -1)
				return (float)fixedLength;

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

struct MarkdownParser::ContentFooter : public MarkdownParser::Element
{
	ContentFooter(MarkdownParser* parent, const MarkdownHeader& header):
		Element(parent)
	{
		auto f = parent->styleData.getFont().withHeight(parent->styleData.fontSize * 0.75f);

		s.append("Author: " + header.getKeyValue("author") + "\n", Colours::grey);
		s.append("Modified: " + header.getKeyValue("modified"), Colours::grey);
	}

	Component* createComponent(int maxWidth) override
	{
		if (content == nullptr)
		{
			auto list = parent->getHolder()->getDatabase().getFlatList();
			auto root = parent->getHolder()->getDatabaseRootDirectory();

			auto thisLink = parent->getLastLink().withAnchor("");
			MarkdownLink nextLink = thisLink;
			String nextName;

			for (int i = 0; i < list.size(); i++)
			{
				if (list[i].url == thisLink)
				{
					int nextIndex = i + 1;

					while (nextIndex < list.size() && nextLink == thisLink)
					{
						nextIndex++;
						nextLink = list[nextIndex].url.withAnchor("");
					}

					auto linkWithoutAnchor = list[nextIndex].url.withAnchor("");
					
					nextLink = parent->getHolder()->getDatabase().getLink(linkWithoutAnchor.toString(MarkdownLink::UrlFull));

					jassert(nextLink.getType() != MarkdownLink::MarkdownFileOrFolder);
					nextName = list[nextIndex].tocString;
					break;
				}
					
			}

			content = new Content(*this, thisLink, nextLink, nextName);

		}
			

		content->setSize(maxWidth, content->getPreferredHeight());

		return content;
	}

	virtual void draw(Graphics& g, Rectangle<float> area)
	{

	}
	
	String generateHtml() const override
	{
		
		HtmlGenerator g;

		String s;
		String nl = "\n";

		{
#if !HISE_HEADLESS
			MessageManagerLock mm;
#endif
			const_cast<ContentFooter*>(this)->createComponent(900.0f);
		}
		
		auto forumLink = content->forumLink;

		if (forumLink.isInvalid())
			forumLink = { {}, "https://forum.hise.audio" };

		auto fl = g.surroundWithTag("Join Discussion", "a", "href=\"" + forumLink.toString(MarkdownLink::FormattedLinkHtml) + "\"");

		auto next = "Next: " + g.surroundWithTag(content->nextName, "a", "href=\"" + content->nextLink.toString(MarkdownLink::FormattedLinkHtml) + "\"");

		s << g.surroundWithTag(fl, "span", "class=\"content-footer-left\"") << nl;
		s << g.surroundWithTag(next, "span", "class=\"content-footer-right\"") << nl;
		String metadata;

		metadata << parent->getHeader().getKeyValue("author") << "<br>";
		metadata << parent->getHeader().getKeyValue("modified") << "<br>";

		s << g.surroundWithTag(metadata, "p", "class=\"content-footer-metadata\"");

		return g.surroundWithTag(s, "div", "class=\"content-footer\"");
	}

	String getTextToCopy() const override
	{
		return s.getText();
	}

	virtual float getHeightForWidth(float width)
	{
		createComponent(width);

		return content->getPreferredHeight();
	}

	virtual int getTopMargin() const override
	{
		return 30;
	}

	Font getFont() const
	{
		
		return parent->getStyleData().getFont();
	}

	Colour getTextColour()
	{
		return parent->getStyleData().textColour;
	}

	String getCurrentKeyword()
	{
		return parent->getHeader().getKeywords()[0];
	}

	MarkdownParser* getParser()
	{
		return parent;
	}

	class Content: public Component,
				   public ButtonListener
	{
	public:

		class Factory : public PathFactory
		{
		public:

			String getId() const override { return "FooterFactory"; }
			Path createPath(const String& l) const override
			{
				Path p;
				auto url = MarkdownLink::Helpers::getSanitizedFilename(l);

				LOAD_PATH_IF_URL("next", MainToolbarIcons::forward);
				LOAD_PATH_IF_URL("discussion", MainToolbarIcons::comment);

				return p;
			}
		};

		struct ButtonLookAndFeel : public LookAndFeel_V3
		{
			void drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour, bool isMouseOverButton, bool isButtonDown) override
			{
				if (isMouseOverButton)
					g.fillAll(Colours::grey.withAlpha(0.1f));
				if(isButtonDown)
					g.fillAll(Colours::grey.withAlpha(0.1f));

				bool isNextLink = button.getButtonText() != "Discussion";

				auto bounds = button.getLocalBounds();

				auto pathBounds = isNextLink ? bounds.removeFromRight(h) : bounds.removeFromLeft(h);

				auto pb = pathBounds.reduced(pathBounds.getHeight() / 8, pathBounds.getHeight() / 8).toFloat();

				auto p = f.createPath(button.getButtonText());

				p.scaleToFit(pb.getX(), pb.getY(), pb.getWidth(), pb.getHeight(), true);

				g.setColour(textColour.withAlpha(button.isEnabled() ? 1.0f : 0.1f));

				g.fillPath(p);
			}

			void drawButtonText(Graphics& g, TextButton& button, bool isMouseOverButton, bool isButtonDown)
			{
				bool isNextLink = button.getButtonText() != "Discussion";
				

				auto bounds = button.getLocalBounds();
				auto pBounds = isNextLink ? bounds.removeFromRight(h) : bounds.removeFromLeft(h);

				g.setFont(font);
				g.setColour(textColour.withAlpha(button.isEnabled() ? 1.0f : 0.1f));

				String s;

				if (isNextLink)
				{
					s << "Next: " << nextLink;
				}
				else
				{
					s << "Read the discussion";
				}

				g.drawText(s, bounds.toFloat().reduced(5.0f), isNextLink ? Justification::centredRight : Justification::centredLeft);
			}

			int h = 0;
			Colour textColour;
			Font font;
			Factory f;
			String nextLink;
		};

		void checkForumLink()
		{
			forumLink = parent.getParser()->getHolder()->getDatabase().getForumDiscussion(currentPage);
			forumButton.setEnabled(forumLink.isValid());
		}

		Content(ContentFooter& parent_, const MarkdownLink& currentPage_, const MarkdownLink& nextLink_, const String& nextName_):
			parent(parent_),
			forumButton("Discussion"),
			nextButton("Next"),
			nextLink(nextLink_),
			currentPage(currentPage_),
			nextName(nextName_)
		{
			addAndMakeVisible(forumButton);
			addAndMakeVisible(nextButton);

			forumButton.addListener(this);
			nextButton.addListener(this);
			
			nextButton.setEnabled(nextLink.isValid());

			checkForumLink();

			

			blaf.textColour = parent.getTextColour();
			blaf.nextLink = nextName;
			blaf.font = parent.getFont();

			forumButton.setLookAndFeel(&blaf);
			nextButton.setLookAndFeel(&blaf);

		}

		void buttonClicked(Button* b) override
		{
			if (b == &nextButton)
			{
				WeakReference<MarkdownParser> p = parent.getParser();
				auto t = nextLink;
				auto f = [p, t]()
				{ 
					if(p != nullptr)
						p.get()->gotoLink(t);
				};

				MessageManager::callAsync(f);

				return;
			}

			if (b == &forumButton && forumLink.isValid())
			{
				URL(forumLink.toString(MarkdownLink::UrlFull)).launchInDefaultBrowser();
			}
		}

		int getPreferredHeight() const
		{
			return getButtonHeight() * 4;
		}

		int getButtonHeight() const
		{
			return parent.getFont().getHeight() * 2;
		}

		void paint(Graphics& g) override
		{
			

			g.setColour(Colours::black.withAlpha(0.1f));

			auto bounds = getLocalBounds();
			
			bounds.removeFromTop(getButtonHeight());

			g.setColour(parent.getTextColour().withAlpha(0.2f));
			bounds.removeFromTop(10.0f);
			g.fillRect(bounds.removeFromTop(2.0f));
			bounds.removeFromTop(6.0f);

			parent.s.draw(g, bounds.toFloat().reduced(10.0f));

		}

		void resized() override
		{
			blaf.h = getButtonHeight();
			auto bounds = getLocalBounds();
			auto top = bounds.removeFromTop(getButtonHeight());
			forumButton.setBounds(top.removeFromLeft(bounds.getWidth() / 4));

			int nextWidth = blaf.font.getStringWidth(nextName) + getButtonHeight() * 3;

			nextButton.setBounds(top.removeFromRight(nextWidth));
		}

		ButtonLookAndFeel blaf;
		TextButton forumButton;
		TextButton nextButton;

		MarkdownLink forumLink;
		String nextName;

		ContentFooter& parent;
		MarkdownLink currentPage;
		MarkdownLink nextLink;
	};

	ScopedPointer<Content> content;

	MarkdownLink discussion;
	MarkdownLink next;
	AttributedString s;
	

};


}

