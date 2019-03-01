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
			parent->gotoLink(url);
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
	Headline(MarkdownParser* parent, int level_, const AttributedString& s, bool isFirst_) :
		Element(parent),
		content(s),
		level(level_),
		isFirst(isFirst_),
		l(s, 0.0f)
	{
		anchorURL = "#" + s.getText().toLowerCase().replaceCharacters(" ", "-");
	}

	void draw(Graphics& g, Rectangle<float> area) override
	{
		anchorY = area.getY() - 15.0f;

		float marginTop = isFirst ? 0.0f : 20.0f;
		area.removeFromTop(marginTop);

		drawHighlight(g, area);

		g.setColour(Colours::grey.withAlpha(0.2f));
		g.drawHorizontalLine((int)area.getBottom() - 8, area.getX(), area.getRight());

		l.drawCopyWithOffset(g, area);
	}

	float getHeightForWidth(float width) override
	{
		float marginTop = isFirst ? 0.0f : 20.0f;
		float marginBottom = 10.0f;

		l = { content, width };
		l.addYOffset(getTopMargin());

		l.styleData = parent->styleData;

		l.styleData.textColour = l.styleData.headlineColour;
		l.styleData.codeColour = l.styleData.headlineColour;
		l.styleData.linkColour = l.styleData.headlineColour;
		l.styleData.linkBackgroundColour = Colours::white.withAlpha(0.1f);
		l.styleData.codebackgroundColour = Colours::white.withAlpha(0.1f);

		return l.getHeight() + marginTop + marginBottom;
	}

	String getTextToCopy() const override
	{
		return content.getText();
	}

	void searchInContent(const String& searchString) override
	{
		searchInStringInternal(content, searchString);
	}

	int getTopMargin() const override 
	{ 
		return 30 - jmax<int>(0, 3 - level) * 5; 
	};

	float anchorY;
	String anchorURL;

	AttributedString content;
	MarkdownLayout l;
	int level;
	bool isFirst;
};



struct MarkdownParser::BulletPointList : public MarkdownParser::Element
{
	struct Row
	{
		AttributedString content;
		MarkdownLayout l;
	};

	BulletPointList(MarkdownParser* parser, Array<AttributedString>& ar) :
		Element(parser)
	{
		for (const auto& r : ar)
			rows.add({ r, {r, 0.0f} });
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

		for (auto& r : rows)
		{
			r.l = parent->getTextLayoutForString(r.content, width - bulletPointIntendation);
			r.l.addXOffset(bulletPointIntendation);
			r.l.styleData = parent->styleData;
			lastHeight += r.l.getHeight();
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

struct MarkdownParser::EnumerationList : public MarkdownParser::BulletPointList
{
	EnumerationList(MarkdownParser* parent, Array<AttributedString>& list) :
		BulletPointList(parent, list)
	{};

	String getTextToCopy() const override
	{
		String s;

		int index = 1;

		for (auto r : rows)
			s << index++ << ". " << r.content.getText() << "\n";

		return s;
	}

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
		drawHighlight(g, area);

		g.setColour(Colours::grey.withAlpha(0.2f));

		g.fillRect(area);
		g.fillRect(area.withWidth(3.0f));

		l.drawCopyWithOffset(g, area);
	}

	float getHeightForWidth(float width) override
	{
		float widthToUse = width - 2.0f * intendation;

		if (widthToUse != lastWidth)
		{
			lastWidth = widthToUse;

			l = { content, widthToUse - intendation };
			l.addYOffset(getTopMargin() + intendation);
			l.addXOffset(intendation);
			l.styleData = parent->styleData;
			recalculateHyperLinkAreas(l, hyperLinks, getTopMargin());

			lastHeight = l.getHeight() + intendation;

			return lastHeight;
		}
		else
			return lastHeight;
	}

	void searchInContent(const String& searchString) override
	{
		searchInStringInternal(content, searchString);
		searchResults.offsetAll(intendation, intendation);

	}

	String getTextToCopy() const override
	{
		return content.getText();
	}

	int getTopMargin() const override { return 10; };

	const float intendation = 12.0f;

	float lastWidth = -1.0f;
	float lastHeight = -1.0f;

	MarkdownLayout l;
	AttributedString content;
};

struct MarkdownParser::CodeBlock : public MarkdownParser::Element
{
	enum SyntaxType
	{
		Undefined,
		Cpp,
		Javascript,
		LiveJavascript,
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

		auto pathBounds = hyperLinks.getFirst().area.translated(0.0, area.getY() - getTopMargin());


		p.scaleToFit(pathBounds.getX(), pathBounds.getY(), pathBounds.getWidth(), pathBounds.getHeight(), true);

		g.setColour(Colour(0xFF777777));
		g.fillPath(p);

		drawHighlight(g, area);
	}

	String getTextToCopy() const override
	{
		return code;
	}

	void searchInContent(const String& searchString) override
	{
		if (code.contains(searchString))
		{
			searchResults.clear();

			ScopedPointer<Content> c = createEditor();

			auto ranges = getMatchRanges(code, searchString, true);
			
			for (auto r : ranges)
			{
				RectangleList<float> area;

				for (int i = 0; i < r.getLength(); i++)
				{
					CodeDocument::Position pos(*c->doc, r.getStart() + i);
					area.add(c->editor->getCharacterBounds(pos).toFloat());
				}

				area.consolidate();
				
				searchResults.add(area.getBounds());
			}

			searchResults.offsetAll(0.0f, 10.0f);
		}
	}

	struct Content: public Component,
					public ButtonListener
	{
		Content(SyntaxType syntax, String code, float width, int numLines)
		{
			doc = new CodeDocument();

			switch (syntax)
			{
			case Cpp: tok = new CPlusPlusCodeTokeniser(); break;
			case LiveJavascript:
			case Javascript: tok = new JavascriptTokeniser(); break;
			case XML:	tok = new XmlTokeniser(); break;
			case Snippet: tok = new SnippetTokeniser(); break;
			default: break;
			}

			doc->replaceAllContent(code);

			editor = new CodeEditorComponent(*doc, tok);

			editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
			editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
			editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
			editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
			editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
			editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
			editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
			editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

			editor->setSize((int)width, editor->getLineHeight() * numLines + 25);

			addAndMakeVisible(editor);

			if (syntax == LiveJavascript)
			{
				addAndMakeVisible(runButton = new TextButton("Run"));
				addAndMakeVisible(resultLabel = new Label());
				runButton->setLookAndFeel(&blaf);
				runButton->addListener(this);

				resultLabel->setColour(Label::ColourIds::backgroundColourId, Colour(0xff363636));
				resultLabel->setColour(Label::ColourIds::textColourId, Colours::white);
				resultLabel->setFont(GLOBAL_MONOSPACE_FONT());
				resultLabel->setText("Click Run to evaluate this code", dontSendNotification);
			}

			setWantsKeyboardFocus(true);
		}

		bool keyPressed(const KeyPress& key) override
		{
			if (key == KeyPress::F5Key)
			{
				runButton->triggerClick();
				return true;
			}
		}

		void buttonClicked(Button* b) override
		{
			String code;
			
			code << "function run(){";
			code << doc->getAllContent();
			code << "}";

			juce::JavascriptEngine engine;

			Result r = Result::ok();

			r = engine.execute(code);

			if (r.wasOk())
			{
				var thisObject;
				var::NativeFunctionArgs args(thisObject, nullptr, 0);

				auto result = engine.callFunction("run", args, &r);

				if (r.wasOk())
				{
					resultLabel->setText("Result: " + result.toString(), dontSendNotification);
				}
				else
				{
					resultLabel->setText("Error: " + r.getErrorMessage(), dontSendNotification);
				}
			}
			else
			{
				resultLabel->setText("Error: " + r.getErrorMessage(), dontSendNotification);
			}
		}

		void resized() override
		{
			editor->setTopLeftPosition(0, 0);

			if (runButton != nullptr)
			{
				auto ar = getLocalBounds();
				ar.removeFromTop(editor->getHeight());
				runButton->setBounds(ar.removeFromLeft(80));
				resultLabel->setBounds(ar);
			}
		}

		Image getSnapshot()
		{
			return editor->createComponentSnapshot(editor->getLocalBounds());
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xff363636));
		}

		AlertWindowLookAndFeel blaf;

		ScopedPointer<CodeDocument> doc;
		ScopedPointer<CodeTokeniser> tok;
		ScopedPointer<CodeEditorComponent> editor;

		ScopedPointer<TextButton> runButton;
		ScopedPointer<Label> resultLabel;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Content);
	};

	Content* createEditor()
	{
		int numLines = StringArray::fromLines(code).size();

		if (syntax == LiveJavascript)
			numLines = jmax(numLines, 12);

		return new Content(syntax, code, lastWidth, numLines);
	}

	Component* createComponent(int maxWidth)
	{
		if (syntax == LiveJavascript && content == nullptr)
		{
			content = createEditor();
			content->setSize(content->editor->getWidth(), content->editor->getHeight() + 32);
		}

		return content;
	}

	float getHeightForWidth(float width) override
	{
		if (width != lastWidth)
		{
			if (syntax == LiveJavascript)
			{
				lastWidth = width;

				content = dynamic_cast<Content*>(createComponent(width));

				return (float)content->getHeight();
			}
			else
			{
				hyperLinks.clear();

				HyperLink copy;
				copy.area = { 2.0f, 12.0f + getTopMargin(), 14.0f, 14.0f };
				copy.url << "CLIPBOARD::" << code << "";
				copy.valid = true;
				copy.tooltip = "Copy code to clipboard";
				hyperLinks.add(std::move(copy));

				lastWidth = width;

				ScopedPointer<Content> c = createEditor();

				renderedCodePreview = c->getSnapshot();

				renderedCodePreview = renderedCodePreview.getClippedImage({ 0, 0, renderedCodePreview.getWidth(), renderedCodePreview.getHeight() - 20 });

				lastWidth = width;
				lastHeight = (float)renderedCodePreview.getHeight() + 6.0f;

				return lastHeight;
			}

			
		}
		else
			return lastHeight;
	}

	ScopedPointer<Content> content;

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
		if (!img.isNull() && width == lastWidth)
			return (float)img.getHeight();

		lastWidth = width;

		img = parent->resolveImage(imageURL, width);

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

	String getImageURL() const { return imageURL; }

	int getTopMargin() const override { return 5; };

	Component* createComponent(int maxWidth) override
	{
		if (isGif && gifPlayer == nullptr)
		{
			gifPlayer = new GifPlayer(*this);
			gifPlayer->setSize(img.getWidth(), img.getHeight());
		}

		return gifPlayer;
	}

	struct GifPlayer : public Component,
					   public MarkdownPreview::CustomViewport::Listener
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
			if(auto viewport = findParentComponentOfClass<MarkdownPreview::CustomViewport>())
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
				totalLength += c.length;

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


}

