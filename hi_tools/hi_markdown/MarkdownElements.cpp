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
	TextBlock(MarkdownParser* parent, int lineNumber, const AttributedString& s) :
		Element(parent, lineNumber),
		content(s),
		l(content, 0.0f, parent->stringWidthFunction)
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
			l = { content, width, parent->stringWidthFunction };

			l.addYOffset((float)getTopMargin());
			l.styleData = parent->styleData;

			recalculateHyperLinkAreas(l, hyperLinks, (float)getTopMargin());

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

	float getTopMargin() const override { return 10.0f; };

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

		void drawButtonBackground(Graphics& g, Button&, const Colour& , bool isMouseOverButton, bool isButtonDown) override
		{
			g.setColour(parent.styleData.linkBackgroundColour);

			g.fillAll();

			if (isMouseOverButton)
				g.fillAll(Colours::black.withAlpha(0.1f));

			if (isButtonDown)
				g.fillAll(Colours::black.withAlpha(0.1f));
		}

		void drawButtonText(Graphics& g, TextButton& b, bool /*isMouseOverButton*/, bool /*isButtonDown*/) override
		{
			g.setFont(parent.styleData.getFont());
			g.setColour(parent.styleData.textColour);
			g.drawText(b.getButtonText(), b.getLocalBounds().toFloat(), Justification::centred);
		}

		MarkdownParser& parent;
	};

	ActionButton(MarkdownParser* parent, int lineNumber, const String& text_, const String& url_) :
		Element(parent, lineNumber),
		blaf(*parent),
		text(text_),
		url(url_)
	{};

	~ActionButton()
	{
		button = nullptr;
	}

	void draw(Graphics& , Rectangle<float> ) override
	{

	}

	float getTopMargin() const override
	{
		return 20.0f;
	}

	void searchInContent(const String& ) override
	{
		
	}

	String getTextToCopy() const override
	{
		return text;
	}

	float getHeightForWidth(float ) override
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
		return (int)parent->styleData.getFont().getHeight() + 20;
	}

	void buttonClicked(Button* ) override
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

struct MarkdownParser::HorizontalRuler: public MarkdownParser::Element
{
	HorizontalRuler(MarkdownParser* p, int lineNumber):
	  Element(p, lineNumber)
	{};

	void draw(Graphics& g, Rectangle<float> area) override
	{
		auto c = parent->styleData.tableLineColour;
		g.setColour(c);
		g.drawHorizontalLine(area.getY() + 1, area.getX(), area.getRight());
	};

	float getHeightForWidth(float width) override { return 22; };

	float getTopMargin() const override { return 10.0f; };

	String getTextToCopy() const override { return {}; };
};

struct MarkdownParser::Headline : public MarkdownParser::Element
{
	Headline(MarkdownParser* parent, int lineNumber, int level_, const String& imageURL_, const AttributedString& s, bool isFirst_) :
		Element(parent, lineNumber),
		content(s),
		headlineLevel(level_),
		isFirst(isFirst_),
		imageURL({}, imageURL_),
		l(s, 0.0f, parent->stringWidthFunction)
	{
		using namespace simple_css;

		auto topMargin = 15.0f + ((4.0f - (float)headlineLevel) * 5.0f) * getZoomRatio();

		auto idx = jlimit(0, 4, headlineLevel-1);
		static ElementType headlines[4] = { ElementType::Headline1, ElementType::Headline2, ElementType::Headline3, ElementType::Headline4 };
		margins = parent->styleData.getMargin((int)headlines[idx], { topMargin, 10.0f });

		if(!isFirst)
			margins.first += 20.0f;

		anchorURL = "#" + s.getText().toLowerCase().replaceCharacters(" ", "-");
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		anchorY = area.getY() - 15.0f;

		area.removeFromBottom(margins.second);

		int imgOffset = 0;

		if (img.isValid())
		{
			auto b = area.removeFromLeft((float)img.getWidth()).toNearestInt().reduced(5);

			g.drawImageAt(img, b.getX(), (int)area.getY());

			area.removeFromLeft(5.0f);

			imgOffset = 15;
		}

		drawHighlight(g, area);

		g.setColour(Colours::grey.withAlpha(0.2f));
		
		if(headlineLevel <= 3)
			g.drawHorizontalLine((int)(area.getBottom()), area.getX() + imgOffset, area.getRight());
		
 		l.drawCopyWithOffset(g, area);
	}

	static int getSizeLevelForHeadline(int headlineLevel)
	{
		return 3 - jmin<int>(headlineLevel, 3);
	}


	float getHeightForWidth(float width) override
	{
		l = { content, width, parent->stringWidthFunction};
		
		l.styleData = parent->styleData;

		auto idx = jlimit(0, 4, headlineLevel-1);
		l.addYOffset(l.styleData.headlineFontSize[idx] * l.styleData.fontSize);

		l.styleData.textColour = l.styleData.headlineColour;
		l.styleData.codeColour = l.styleData.headlineColour;
		l.styleData.linkColour = l.styleData.headlineColour;
		l.styleData.linkBackgroundColour = Colours::white.withAlpha(0.1f);
		l.styleData.codebackgroundColour = Colours::white.withAlpha(0.1f);

		if (imageURL.isValid())
		{
			Colour tColour = parent->getStyleData().textColour;
			parent->getStyleData().textColour = parent->getStyleData().headlineColour;
			img = parent->resolveImage(imageURL, l.getHeight());
			parent->getStyleData().textColour = tColour;
		}

		auto h = l.getHeight();
		h += margins.second;
		
		return h;
	}

	virtual void addImageLinks(Array<MarkdownLink>& sa) override
	{
		if (imageURL.isValid())
			sa.add(imageURL.withExtraData("256px"));
	};

	String getTextToCopy() const override
	{
		return content.getText();
	}

	String generateHtml() const override
	{
		String html;
		HtmlGenerator g;
		int linkIndex = 0;
		String c;

		if (imageURL.isValid())
		{
			auto imageString = imageURL.toString(MarkdownLink::FormattedLinkHtml);
			c << g.surroundWithTag("", "img", "src=\"" + imageString + "\"");
		}

		c << g.createFromAttributedString(content, linkIndex);
		html << g.surroundWithTag(c, "h" + String(headlineLevel), "id=\"" + anchorURL.substring(1) + "\"");

		return html;
	}

	void searchInContent(const String& searchString) override
	{
		searchInStringInternal(content, searchString);
	}

	float getTopMargin() const override 
	{
		return margins.first;
	};

	std::pair<float, float> margins;

	float anchorY;
	String anchorURL;

	AttributedString content;
	MarkdownLayout l;
	//int level;

	int headlineLevel;

	bool isFirst;
	MarkdownLink imageURL;
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

	BulletPointList(MarkdownParser* parser, int lineNumber, Array<AttributedString>& ar, Array<Array<HyperLink>>& links) :
		Element(parser, lineNumber)
	{
		for (int i = 0; i < ar.size(); i++)
			rows.add({ ar[i],{ ar[i], 0.0f, parser->stringWidthFunction }, links[i] });

		for (const auto& r : rows)
		{
			hyperLinks.addArray(r.links);
		}
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
		HtmlGenerator g;
		int linkIndex = 0;
		String listItems;

		for (auto r : rows)
		{
			if (r.links.size() > 0)
				jassert(!hyperLinks.isEmpty());

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

			for (auto result : searchResults)
				allMatches.add(result);
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
			r.l = { r.content, width - bulletPointIntendation, parent->stringWidthFunction };
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

	float getTopMargin() const override { return 10.0f; };

	const float intendation = 8.0f;

	const float bulletMargin = 10.0f;

	Array<Row> rows;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;
};

struct MarkdownParser::EnumerationList : public MarkdownParser::BulletPointList
{
	EnumerationList(MarkdownParser* parent, int lineNumber, Array<AttributedString>& list, Array<Array<HyperLink>>& links) :
		BulletPointList(parent, lineNumber, list, links)
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
			auto font = parent->styleData.getBoldFont();
			
			String bp;
			bp << rowIndex++ << ".";

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
	Comment(MarkdownParser* p, int lineNumber, const AttributedString& c) :
		Element(p, lineNumber),
		l(c, 0.0f, p->stringWidthFunction),
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

			l = { content, widthToUse - thisIndentation, parent->stringWidthFunction };
			l.addYOffset((float)getTopMargin() + thisIndentation);
			l.addXOffset(thisIndentation);
			l.styleData = parent->styleData;
			recalculateHyperLinkAreas(l, hyperLinks, (float)getTopMargin());

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

	float getTopMargin() const override { return intendation * getZoomRatio(); };

	const float intendation = 12.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	MarkdownLayout l;
	AttributedString content;
};


struct MarkdownParser::ImageElement : public MarkdownParser::Element
{
	ImageElement(MarkdownParser* parent, int lineNumber, const String& imageName_, const String& imageURL_) :
		Element(parent, lineNumber),
		imageName(imageName_),
		imageURL({}, imageURL_)
	{
		HyperLink link;
		link.url = imageURL;
		link.displayString = imageName;
		link.valid = true;

		hyperLinks.add(link);
	};

	float getHeightForWidth(float width) override
	{
		if (imageURL.toString(MarkdownLink::UrlSubPath).endsWith("gif"))
			isGif = true;

		if (!img.isNull() && width == lastWidth)
			return (float)img.getHeight() + (isGif ? 50.0f : 0.0f);

		lastWidth = width;

		img = parent->resolveImage(imageURL, width);

		

		if (!img.isNull())
			return (float)img.getHeight() + (isGif ? 50.0f : 0.0f);

		return 0.0f;
	}

	void draw(Graphics& g, Rectangle<float> area)
	{
		g.setOpacity(1.0f);


		g.drawImageAt(img, (int)area.getX(), (int)area.getY() - getTopMargin()/2);
	}

	String getTextToCopy() const override
	{
		return imageName;
	}

	void addImageLinks(Array<MarkdownLink>& sa) override
	{
		sa.add(imageURL);
	}

	MarkdownLink getImageURL() const { return imageURL; }

	float getTopMargin() const override { return 30.0f; };

	String generateHtml() const override
	{
		HtmlGenerator g;

        float width = 2000.0f;
        
        MarkdownParser::ImageProvider::updateWidthFromURL(imageURL, width);
        
        
        
        String s;
        
        if(width == 2000.0f)
        {
            
        }
        else if(width <= 1.0f)
        {
            s << "style=\"max-width:" << String(roundToInt(width*100.0f)) << "%;\" ";
        }
        else
        {
            s << "style=\"max-width:" << String(roundToInt(width)) << "px;\" ";
        }
        
        
        s << "src=\"{LINK0}\"";
        
        return g.surroundWithTag("", "img", s);
        
        
        
		//return g.surroundWithTag("", "img", "src=\" " + imageURL.toString(MarkdownLink::FormattedLinkHtml) + "\"");
	}

	Component* createComponent(int) override
	{
		if (isGif && gifPlayer == nullptr)
		{
			gifPlayer = new GifPlayer(*this);
			
		}

		if(gifPlayer != nullptr)
			gifPlayer->setSize(jmax(50, img.getWidth()), jmax(50, 50 + img.getHeight()));

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

			Path path;

			path.addTriangle(0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.5f);
			ar = ar.reduced(25.0f);

			path.scaleToFit(ar.getX() + 5.0f, ar.getY(), ar.getWidth(), ar.getHeight(), true);
			g.fillPath(path);

		}

		void scrolled(Rectangle<int> visibleArea) override
		{
			ignoreUnused(visibleArea);

#if USE_BACKEND && JUCE_WEB_BROWSER
			if (!visibleArea.contains(getBoundsInParent()))
			{
				gifPlayer = nullptr;
			}
#endif
		}

		void mouseEnter(const MouseEvent& )
		{
			setMouseCursor(MouseCursor(MouseCursor::PointingHandCursor));
		}

		void mouseExit(const MouseEvent& )
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

#if USE_BACKEND && JUCE_WEB_BROWSER
			addAndMakeVisible(gifPlayer = new WebBrowserComponent());
			gifPlayer->setSize(p.img.getWidth() + 50, p.img.getHeight() + 50);
			gifPlayer->setTopLeftPosition(0, 0);
			gifPlayer->goToURL(p.imageURL.toString(MarkdownLink::UrlFull));
			gifPlayer->addMouseListener(this, true);
#endif
		}

		ImageElement& p;

#if USE_BACKEND && JUCE_WEB_BROWSER
		ScopedPointer<WebBrowserComponent> gifPlayer;
#endif

	};

private:

	ScopedPointer<GifPlayer> gifPlayer;

	bool isGif = false;
	Image img;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	String imageName;
	MarkdownLink imageURL;
};


struct MarkdownParser::CodeBlock : public MarkdownParser::Element
{
	CodeBlock(MarkdownParser* parent, int lineNumber, const String& code_, MarkdownCodeComponentBase::SyntaxType t) :
		Element(parent, lineNumber),
		code(code_),
		syntax(t)
	{
		code = code.trim();
		//code << "\n"; // hacky..
	}

	void draw(Graphics& , Rectangle<float> ) override
	{
	}

	String getTextToCopy() const override
	{
		return code;
	}

	String generateHtml() const override
	{
		return MarkdownCodeComponentBase::HtmlHelpers::createSnapshot(syntax, code);
	}

	void searchInContent(const String& searchString) override
	{
		if (code.contains(searchString))
		{
			searchResults.clear();

			ScopedPointer<MarkdownCodeComponentBase> c = createEditor((int)lastWidth);

			auto ranges = getMatchRanges(code, searchString, true);

			for (auto r : ranges)
			{
				RectangleList<float> area;

				for (int i = 0; i < r.getLength(); i++)
				{
					CodeDocument::Position pos(*c->usedDocument, r.getStart() + i);
					area.add(c->editor->getCharacterBounds(pos).toFloat());
				}

				area.consolidate();

				searchResults.add(area.getBounds());
			}

			searchResults.offsetAll(0.0f, 10.0f);
		}
	}

	MarkdownCodeComponentBase* createEditor(int newWidth)
	{
		auto widthToUse = lastWidth;

		if (widthToUse == -1.0f)
			widthToUse = (float)newWidth;

#if HI_MARKDOWN_ENABLE_INTERACTIVE_CODE
		
		return MarkdownCodeComponentFactory::createInteractiveEditor(parent, syntax, code, widthToUse);

#else
		if (useSnapshot)
			return MarkdownCodeComponentFactory::createSnapshotEditor(parent, syntax, code, widthToUse);
		else
			return MarkdownCodeComponentFactory::createBaseEditor(parent, syntax, code, widthToUse);
#endif
	}

	
	
	bool useSnapshot = false;

	Component* createComponent(int maxWidth)
	{
		if (content == nullptr)
			content = createEditor(maxWidth);

		content->setSize(maxWidth, content->getPreferredHeight());
		content->resized();

		return content;
	}

	void addImageLinks(Array<MarkdownLink>& sa) override
	{
#if !HISE_HEADLESS
		MessageManagerLock mm;
#endif

		createComponent(MarkdownParser::DefaultLineWidth);

		content->addImageLinks(sa);
	}

	float getHeightForWidth(float width) override
	{
		if (width != lastWidth)
		{
			createComponent((int)width);

			return (float)content->getPreferredHeight() + 20;
		}
		else
			return lastHeight;
	}

	ScopedPointer<MarkdownCodeComponentBase> content;

	float getTopMargin() const override { return 30.0f; };

	String code;

	Image renderedCodePreview;

	MarkdownCodeComponentBase::SyntaxType syntax;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;
};


struct MarkdownParser::MarkdownTable : public MarkdownParser::Element
{
	static constexpr int FixOffset = 100000;

	MarkdownTable(MarkdownParser* p, int lineNumber, const RowContent& headerItems, const Array<int>& lengths, const Array<RowContent>& entries) :
		Element(p, lineNumber),
		styleData(p->getStyleData())
	{
		int index = 0;

		for (const auto& h : headerItems)
		{
			Cell c(p->stringWidthFunction);
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
				Cell c(p->stringWidthFunction);
                
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

				if (c.imageURL.isValid())
				{
					cContent << g.surroundWithTag("", "img", "src=\"" + c.imageURL.toString(MarkdownLink::FormattedLinkHtml) + "\"");
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

	void addImageLinks(Array<MarkdownLink>& sa) override
	{
		headers.addImageLinks(sa);

		for (auto& r : rows)
			r.addImageLinks(sa);
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		area.removeFromTop(10.0f);
		area.removeFromBottom(10.0f);

		g.setColour(styleData.tableBgColour);
		g.fillRect(area);

		auto headerArea = area.removeFromTop(headers.rowHeight);
		g.setColour(styleData.tableHeaderBackgroundColour);
		g.fillRect(headerArea.reduced(1.0f));
		g.setColour(styleData.tableLineColour);
		g.drawRect(headerArea, 1.0f);
		g.drawVerticalLine((int)area.getX(), area.getY(), area.getBottom());

		for (const auto& c : headers.columns)
		{
			if(c.index != headers.columns.getLast().index)
				g.drawVerticalLine((int)c.area.getRight(), area.getY(), area.getBottom());
		}

		g.drawVerticalLine((int)area.getRight()-1, area.getY(), area.getBottom());
		
		auto yOffset = 20.0f - styleData.fontSize;

		headers.draw(g, headerArea, yOffset);

		if (headers.columns.isEmpty())
		{
			g.drawHorizontalLine((int)area.getY(), area.getX(), area.getRight());
		}

		

		for (auto& r : rows)
		{
			auto rowArea = area.removeFromTop(r.rowHeight);

			r.draw(g, rowArea, yOffset);

			g.setColour(styleData.tableLineColour);
			g.drawHorizontalLine((int)rowArea.getBottom(), rowArea.getX(), rowArea.getRight());
		}
	}

	float getTopMargin() const override { return 10.0f; };

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
		Cell(const MarkdownLayout::StringWidthFunction& f={}) : l({}, 0.0f, f) {};

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

		MarkdownLink imageURL;
		int index = -1;
		Rectangle<float> area;
		int length;
		Image img;
		Array<HyperLink> cellLinks;
	};

	struct Row
	{
		const float intendation = 8.0f;

		void addImageLinks(Array<MarkdownLink>& sa)
		{
			for (auto& c : columns)
			{
				if (c.imageURL.isValid())
					sa.add(c.imageURL);
			}
		}

		float calculateHeightForCell(Cell& c, float width, MarkdownParser* parser)
		{
			if (!c.content.getText().isEmpty())
				return c.l.getHeight();
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
			{
				if (c.getFixedLength() != -1)
					continue;

				totalLength += c.length;
			}
				

			for (auto& h : columns)
			{
				float w = getColumnWidth(width, h.index);
				auto contentWidth = w - 2.0f * intendation;

				h.l = MarkdownLayout(h.content, contentWidth, parser->stringWidthFunction);
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

		void draw(Graphics& g, Rectangle<float> area, float textOffset)
		{
			for (const auto& c : columns)
			{
				auto ar = c.area.withX(c.area.getX() + area.getX());
				ar = ar.withY(ar.getY() + area.getY());

				Random r;

				if (c.img.isNull())
					c.l.drawCopyWithOffset(g, area.translated(0.0f, -textOffset));
				else
				{
					g.setOpacity(1.0f);

					// Align the image vertically..
					ar.setHeight(area.getHeight());

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

	MarkdownLayout::StyleData styleData;
};

struct MarkdownParser::ContentFooter : public MarkdownParser::Element
{
	ContentFooter(MarkdownParser* parent, int lineNumber, const MarkdownHeader& header):
		Element(parent, lineNumber)
	{
		auto f = parent->styleData.getFont().withHeight(parent->styleData.fontSize * 0.75f);

		s.append("Author: " + header.getKeyValue("author") + "\n", Colours::grey);
		s.append("Modified: " + header.getKeyValue("modified"), Colours::grey);
	}

	struct ContentLinks
	{
		MarkdownLink thisLink;
		MarkdownLink nextLink;
		MarkdownLink forumLink;
		String nextName;
	};

	static ContentLinks createContentLinks(MarkdownParser* parent)
	{
		auto list = parent->getHolder()->getDatabase().getFlatList();
		auto root = parent->getHolder()->getDatabaseRootDirectory();

		ContentLinks links;

		links.thisLink = parent->getLastLink().withAnchor("");
		links.nextLink = links.thisLink;

		for (int i = 0; i < list.size(); i++)
		{
			if (list[i].url == links.thisLink)
			{
				int nextIndex = i + 1;

				links.nextLink = list[nextIndex].url.withAnchor("");

				while (nextIndex < list.size() && links.nextLink == links.thisLink)
				{
					nextIndex++;
					links.nextLink = list[nextIndex].url.withAnchor("");
				}

				auto linkWithoutAnchor = list[nextIndex].url.withAnchor("");

				links.nextLink = parent->getHolder()->getDatabase().getLink(linkWithoutAnchor.toString(MarkdownLink::UrlFull));

				jassert(links.nextLink.getType() != MarkdownLink::MarkdownFileOrFolder);
				links.nextName = list[nextIndex].tocString;
				break;
			}

		}

		links.forumLink = parent->getHolder()->getDatabase().getForumDiscussion(links.thisLink);

		return links;

	}

	Component* createComponent(int maxWidth) override
	{
		if (content == nullptr)
		{
			ContentLinks links = createContentLinks(parent);
			content = new Content(*this,  links.thisLink, links.nextLink, links.nextName);
		}
			

		content->setSize(maxWidth, content->getPreferredHeight());

		return content;
	}

	virtual void draw(Graphics& , Rectangle<float> )
	{

	}
	
	String generateHtml() const override
	{
		
		HtmlGenerator g;

		String html;
		String nl = "\n";

		ContentFooter::ContentLinks links = ContentFooter::createContentLinks(parent);

		auto forumLink = links.forumLink;

		if (forumLink.isInvalid())
			forumLink = { {}, "https://forum.hise.audio" };

		auto fl = g.surroundWithTag("Join Discussion", "a", "href=\"" + forumLink.toString(MarkdownLink::FormattedLinkHtml) + "\"");

		auto nextString = "Next: " + g.surroundWithTag(links.nextName, "a", "href=\"" + links.nextLink.toString(MarkdownLink::FormattedLinkHtml) + "\"");

		html << g.surroundWithTag(fl, "span", "class=\"content-footer-left\"") << nl;
		html << g.surroundWithTag(nextString, "span", "class=\"content-footer-right\"") << nl;
		String metadata;

		metadata << parent->getHeader().getKeyValue("author") << "<br>";
		metadata << parent->getHeader().getKeyValue("modified") << "<br>";

		html << g.surroundWithTag(metadata, "p", "class=\"content-footer-metadata\"");

		return g.surroundWithTag(html, "div", "class=\"content-footer\"");
	}

	String getTextToCopy() const override
	{
		return s.getText();
	}

	virtual float getHeightForWidth(float width)
	{
		createComponent((int)width);

		return (float)content->getPreferredHeight();
	}

	float getTopMargin() const override { return 30.0f; }

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

				LOAD_EPATH_IF_URL("next", MainToolbarIcons::forward);
				LOAD_EPATH_IF_URL("discussion", MainToolbarIcons::comment);

				return p;
			}
		};

		struct ButtonLookAndFeel : public LookAndFeel_V3
		{
			void drawButtonBackground(Graphics& g, Button& button, const Colour&, bool isMouseOverButton, bool isButtonDown) override
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

			void drawButtonText(Graphics& g, TextButton& button, bool /*isMouseOverButton*/, bool /*isButtonDown*/)
			{
				bool isNextLink = button.getButtonText() != "Discussion";
				

				auto bounds = button.getLocalBounds();
				isNextLink ? bounds.removeFromRight(h) : bounds.removeFromLeft(h);

				g.setFont(font);
				g.setColour(textColour.withAlpha(button.isEnabled() ? 1.0f : 0.1f));

				String text = "Next: " + nextLink;
				g.drawText(text, bounds.toFloat().reduced(5.0f), isNextLink ? Justification::centredRight : Justification::centredLeft);
			}

			int h = 0;
			Colour textColour;
			Font font;
			Factory f;
			String nextLink;
		};

		Content(ContentFooter& parent_, const MarkdownLink& currentPage_, const MarkdownLink& nextLink_, const String& nextName_):
			parent(parent_),
			nextButton("Next"),
			nextLink(nextLink_),
			currentPage(currentPage_),
			nextName(nextName_)
		{
			addAndMakeVisible(nextButton);

			nextButton.addListener(this);
			
			nextButton.setEnabled(nextLink.isValid());

			blaf.textColour = parent.getTextColour();
			blaf.nextLink = nextName;
			blaf.font = parent.getFont();

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
		}

		int getPreferredHeight() const
		{
			return getButtonHeight() * 4;
		}

		int getButtonHeight() const
		{
			return (int)parent.getFont().getHeight() * 2;
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::black.withAlpha(0.1f));

			auto bounds = getLocalBounds();
			
			bounds.removeFromTop(getButtonHeight());

			g.setColour(parent.getTextColour().withAlpha(0.2f));
			bounds.removeFromTop(10);
			g.fillRect(bounds.removeFromTop(2));
			bounds.removeFromTop(6);

			parent.s.draw(g, bounds.toFloat().reduced(10.0f));

		}

		void resized() override
		{
			blaf.h = getButtonHeight();
			auto bounds = getLocalBounds();
			auto top = bounds.removeFromTop(getButtonHeight());

			int nextWidth = blaf.font.getStringWidth(nextName) + getButtonHeight() * 3;

			nextButton.setBounds(top.removeFromRight(nextWidth));
		}

		ButtonLookAndFeel blaf;
		
		TextButton nextButton;

		MarkdownLink forumLink;
		String nextName;

		ContentFooter& parent;
		MarkdownLink currentPage;
		MarkdownLink nextLink;
	};

	ScopedPointer<Content> content;

	MarkdownLink next;
	AttributedString s;
	

};


}

