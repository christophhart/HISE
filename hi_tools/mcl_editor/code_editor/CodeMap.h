/** ============================================================================
 *
 * MCL Text Editor JUCE module 
 *
 * Copyright (C) Jonathan Zrake, Christoph Hart
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


#pragma once

namespace mcl
{
using namespace juce;

class SearchReplaceComponent : public Component
{
	enum Mode
	{
		Search,
		SearchAndReplace,
		numModes
	};

	SearchReplaceComponent(TextEditor* parent_, Mode searchMode):
		parent(parent_)
	{
		addAndMakeVisible(searchLabel);

		if (searchMode)
			addAndMakeVisible(replaceLabel);
	}

	Label searchLabel;
	Label replaceLabel;

	TextEditor* parent;
};


class LinebreakDisplay : public Component,
	public LambdaCodeDocumentListener
{
public:

	LinebreakDisplay(mcl::TextDocument& d);

	void refresh()
	{
		repaint();
	}

	void paint(Graphics& g) override;

	void setViewTransform(const AffineTransform& t)
	{
		transform = t;
		repaint();
	}

	AffineTransform transform;

	mcl::TextDocument& document;
};


class FoldMap : public Component,
				public FoldableLineRange::Listener,
				public Selection::Listener
{
public:

	enum EntryType
	{
		Skip,
		Class,
		Namespace,
		Enum,
		Function
	};

	struct Helpers
	{
		static bool trimAndGet(String& s, const String& keyword)
		{
			if (s.startsWith(keyword))
			{
				s = s.fromFirstOccurrenceOf(keyword, false, false).trim();
				return true;
			}

			return false;
		}

		static int getLevel(FoldableLineRange::WeakPtr p)
		{
			int level = 0;

			while (p != nullptr)
			{
				p = p->getParent();
				level++;
			}

			return level;
		}

		static void trimIf(String& s, const String& keyword)
		{
			if (s.startsWith(keyword))
				s = s.fromFirstOccurrenceOf(keyword, false, false).trim();
		}

		static EntryType getEntryType(String& s)
		{
			static const StringArray skipWords = { "for", "if", "else", "while", "switch", "/*", "```", "---" };

			auto trimmed = s.trim();

			for (auto& w : skipWords)
				if (trimmed.startsWith(w))
					return EntryType::Skip;

			if (s.startsWith("template"))
				s = s.fromFirstOccurrenceOf(">", false, false).trim();

			if (trimAndGet(s, "class"))
				return EntryType::Class;

			if (trimAndGet(s, "struct"))
				return EntryType::Class;

			if (trimAndGet(s, "namespace"))
				return EntryType::Namespace;

			if (auto t = trimAndGet(s, "enum"))
				return EntryType::Enum;

			trimIf(s, "static");
			trimIf(s, "inline");
			trimIf(s, "function");
			trimIf(s, "void");
			trimIf(s, "int");
			trimIf(s, "float");
			trimIf(s, "double");

			return EntryType::Function;
		}
	};

	void selectionChanged() override
	{
		if (doc.getNumSelections() == 1)
		{
			auto lineToBolden = doc.getSelection(0).head.x;

			for (auto i : items)
			{
				i->setBoldLine(lineToBolden);
			}
		}
	}

	void displayedLineRangeChanged(Range<int> newRange) override
	{
		lastRange = newRange;

		for (auto i : items)
			i->setDisplayedRange(newRange);
			
		repaint();
	}

	

	String getTextForFoldRange(FoldableLineRange::WeakPtr p)
	{
		return doc.getCodeDocument().getLine(p->getLineRange().getStart());
	}

	struct Item : public Component,
				  public TooltipWithArea::Client
	{
		static const int Height = 24;

		Item(FoldableLineRange::WeakPtr p_, FoldMap& m) :
			p(p_)
		{
            auto* lm = m.getLanguageManager();
            
			text = m.getTextForFoldRange(p);
            
            if(lm != nullptr)
                lm->processBookmarkTitle(text);
            
			type = Helpers::getEntryType(text);

			bestWidth = getFont().boldened().getStringWidth(text) + roundToInt((float)Helpers::getLevel(p) * 5.0f);

			bestWidth = jmin(bestWidth, 600);

			int h = Height;

			for (auto c : p->children)
			{
				ScopedPointer<Item> n = new Item(c, m);
				addAndMakeVisible(n);

				if (n->type == Skip)
					continue;

				addAndMakeVisible(n);
				h += n->getHeight();
				children.add(n.release());

				bestWidth = jmax(bestWidth, children.getLast()->bestWidth + 10);
			}

			setRepaintsOnMouseActivity(true);
			setSize(1, h);
		};

		TooltipWithArea::Data getTooltip(Point<float> positionInThisComponent) override
		{
			TooltipWithArea::Data d;
			d.id = Identifier(text);
			d.text = text;

			auto f = TooltipWithArea::getBasicFont();

			auto w = f.getStringWidthFloat(text);

			

			

			d.relativePosition = { (float)getWidth() - w, (float)Height };

			if (d.relativePosition.x < 0.0f)
				return d;
			else
				return {};
		}

		void setBoldLine(int lineToBolden)
		{
			isBoldLine = p->getLineRange().contains(lineToBolden);

			for (auto c : children)
				c->setBoldLine(lineToBolden);

			repaint();
		}

		void setDisplayedRange(Range<int> newRange)
		{
			auto r = p->getLineRange();
			onScreen = newRange.contains(r);

			edgeOnScreen = r.intersects(newRange) && !r.contains(newRange);

			for (auto c : children)
				c->setDisplayedRange(newRange);

			repaint();

		}


		void updateHeight()
		{
			int h = Height;

			if (!folded)
			{
				for (auto c : children)
				{
					h += c->getHeight();
				}
			}

			setSize(getWidth(), h);

			if (auto p = findParentComponentOfClass<Item>())
			{
				p->updateHeight();
			}
		}

		void mouseDown(const MouseEvent& e) override
		{
			folded = !folded;
			updateHeight();
			findParentComponentOfClass<FoldMap>()->resized();
		}

		void mouseDoubleClick(const MouseEvent& e) override;

		void setSelected(bool shouldBeSelected, bool grabFocus);

		void resized() override
		{
			int y = Height;

			if (!folded)
			{
				for (auto c : children)
				{
					c->setBounds(0, y, getWidth(), c->getHeight());
					y = c->getBottom();
				}
			}
		}

		Font getFont() { return Font(Font::getDefaultMonospacedFontName(), 14.0f, clicked ? Font::bold : Font::plain); }


		void paint(Graphics& g) override
		{
			auto a = getLocalBounds().removeFromTop(Height).toFloat();;

			auto f = getFont();

			//auto f = GLOBAL_FONT().withHeight(12.0f);

			

			if (isBoldLine)
				f = f.boldened();

			if (edgeOnScreen) 
			{
				auto b = a;// .removeFromRight(4.0f);
				g.setColour(Colours::white.withAlpha(0.03f));
				g.fillRect(b);

				if (onScreen)
					g.fillRect(b);
			}

			if (isMouseOver(false))
			{
				g.setColour(Colours::white.withAlpha(0.05f));
				g.fillRect(a);
				g.drawRect(a, 1.0f);
			}



			auto icon = a.removeFromLeft(a.getHeight()).reduced(5.0f);

			switch (type)
			{
			case Class:		g.setColour(Colour(0xFF3B4261).brighter(0.2f)); break;
			case Function:	g.setColour(Colour(0xFF76425A).brighter(0.2f)); break;
			case Enum:		g.setColour(Colour(0xFF6C8249).brighter(0.2f)); break;
			case Namespace: g.setColour(Colour(0xFF8D7B4F).brighter(0.2f)); break;
            default:                                                        break;
			}


			if (children.isEmpty())
			{
				g.fillEllipse(icon.reduced(2.0f));

				if (isBoldLine)
				{
					g.setColour(Colours::white.withAlpha(0.5f));
					g.drawEllipse(icon.reduced(2.0f), 1.0f);
				}
			}
			else
			{

				Path path;
				path.addTriangle({ 0.0f, 0.0f }, { 0.5f, 1.0f }, { 1.0f, 0.0f });

				if (folded)
				{
					path.applyTransform(AffineTransform::rotation(float_Pi * -0.5f));
				}


				path.scaleToFit(icon.getX(), icon.getY(), icon.getWidth(), icon.getHeight(), true);

				g.fillPath(path);

				if (isBoldLine)
				{
					g.setColour(Colours::white.withAlpha(0.5f));
					g.strokePath(path, PathStrokeType(1.0f));
				}
			}

			

			g.setFont(f);
			g.setColour(Colours::white.withAlpha(isBoldLine ? 1.0f : 0.6f));

			a.removeFromLeft((float)Helpers::getLevel(p) * 5.0f);

			g.drawText(text, a, Justification::centredLeft);
		}

		bool folded = false;
		bool isBoldLine = false;

		bool onScreen = false;
		bool edgeOnScreen = false;

		String text;
		EntryType type;
		FoldableLineRange::WeakPtr p;

		OwnedArray<Item> children;
		bool clicked = false;

		int bestWidth = 0;
	};

	

	FoldMap(TextDocument& d) :
		doc(d)
	{
		doc.addFoldListener(this);
		doc.addSelectionListener(this);

		vp.setViewedComponent(&content, false);
		addAndMakeVisible(vp);
		vp.setColour(ScrollBar::ColourIds::thumbColourId, Colours::white.withAlpha(0.2f));
		vp.setScrollBarThickness(10);
        vp.getVerticalScrollBar().setWantsKeyboardFocus(false);
        vp.setScrollBarsShown(true, false);
	};

	~FoldMap()
	{
		doc.removeFoldListener(this);
		doc.removeSelectionListener(this);
	}

    LanguageManager* getLanguageManager();
    
	bool keyPressed(const KeyPress& k) override;

	int getBestWidth() const
	{
		Font f(Font::getDefaultMonospacedFontName(), 13.0f, Font::bold);

		int maxWidth = 0;

		for (auto i : items)
		{
			maxWidth = jmax(maxWidth, i->bestWidth + JUCE_LIVE_CONSTANT_OFF(35));
		}

		return maxWidth + 10;
	}

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged) override
	{
		rebuild();
	}

	void paint(Graphics& g) override
	{
		g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xe3212121)));

	}

	void rootWasRebuilt(FoldableLineRange::WeakPtr rangeThatHasChanged) override
	{
		
		rebuild();
	}

	void rebuild()
	{
		items.clear();

		int h = 0;

		for (auto r : doc.getFoldableLineRangeHolder().roots)
		{
			ScopedPointer<Item> n = new Item(r, *this);

			if (n->type == Skip)
				continue;

			h += n->getHeight();

			content.addAndMakeVisible(n);
			items.add(n.release());
		}

		content.setSize(getWidth() - vp.getScrollBarThickness(), h);
		updateSize();

		selectionChanged();
		displayedLineRangeChanged(lastRange);
	}

	void updateSize()
	{
		auto y = 0;

		for (auto i : items)
		{
			i->setBounds(0, y, content.getWidth(), i->getHeight());
			y += i->getHeight();
		}

		repaint();
	}

	void resized() override
	{
		updateSize();

		auto b = getLocalBounds();
		b.removeFromLeft(10);

		vp.setBounds(b);
	}

	Viewport vp;
	Component content;

	OwnedArray<Item> items;

	FoldableLineRange::Ptr root;

	Range<int> lastRange;

	TextDocument& doc;
};


class CodeMap : public Component,
                public CodeDocument::Listener,
                public Timer,
                public Selection::Listener
{
public:

	CodeMap(TextDocument& doc_) :
		doc(doc_),
		rebuilder(*this),
		transformToUse(defaultTransform)
	{
		doc.addSelectionListener(this);
		doc.getCodeDocument().addListener(this);
	}

	struct DelayedUpdater: public Timer
	{
		DelayedUpdater(CodeMap& p) :
			parent(p)
		{};

		void timerCallback() override
		{
			parent.rebuild();
			stopTimer();
		}

		CodeMap& parent;
	} rebuilder;

	void timerCallback();

	~CodeMap()
	{
		doc.getCodeDocument().removeListener(this);
		doc.removeSelectionListener(this);
	}

	void selectionChanged() override
	{
		rebuilder.startTimer(300);
	}

	void displayedLineRangeChanged(Range<int> newRange) override
	{
		setVisibleRange(newRange);
	}

	void codeDocumentTextDeleted(int startIndex, int endIndex) override
	{
		rebuilder.startTimer(300);
	}

	void codeDocumentTextInserted(const String& newText, int insertIndex) override
	{
		rebuilder.startTimer(300);
	}

	float getLineNumberFromEvent(const MouseEvent& e) const;

	Rectangle<int> getPreviewBounds(const MouseEvent& e);

	void mouseEnter(const MouseEvent& e) override;

	void mouseExit(const MouseEvent& e) override;

	void mouseMove(const MouseEvent& e) override;

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	int getNumLinesToShow() const
	{
		auto numLinesFull = getHeight() / 2;

		return jmin(doc.getCodeDocument().getNumLines(), numLinesFull);
	}

	void rebuild();

	struct HoverPreview : public Component
	{
		HoverPreview(CodeMap& parent_, int centerRow) :
			parent(parent_)
		{
			setCenterRow(centerRow);
		}

		void setCenterRow(int newCenterRow);

		void paint(Graphics& g) override;

		CodeMap& parent;
		Range<int> rows;
		int centerRow;
		float scale = 1.0f;
	};

	void resized()
	{
		rebuild();
	}

	void setVisibleRange(Range<int> visibleLines);

	bool isActive() const
	{
		return doc.getNumRows() < 10000;
	}

	float lineToY(int lineNumber) const;

	int yToLine(float y) const;

	CodeEditorComponent::ColourScheme* getColourScheme();

	CodeTokeniser* getTokeniser();

	ScopedPointer<HoverPreview> preview;

	void paint(Graphics& g);

	struct ColouredRectangle
	{
		bool isWhitespace() const
		{
			return c.isTransparent();
		}

		int lineNumber;
		bool upper;
		bool selected = false;
		Colour c;
		int position;
		Rectangle<float> area;
	};

	Array<ColouredRectangle> colouredRectangles;

	TextDocument& doc;

	void visibilityChanged() override
	{
		if (isVisible() && dirty)
			rebuild();
	}

	bool dirty = true;
	bool allowHover = true;

	float currentAnimatedLine = -1.0f;
	float targetAnimatedLine = -1.0f;

	int hoveredLine = -1;

	int dragDown = 0;
	bool dragging = false;

	Range<int> displayedLines;
	Range<int> surrounding;
	int offsetY = 0;

	

	AffineTransform defaultTransform;
	AffineTransform& transformToUse;
};


}
