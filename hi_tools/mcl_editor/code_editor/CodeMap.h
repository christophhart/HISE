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

	SearchReplaceComponent(TextEditor* parent_, Mode searchMode);

	Label searchLabel;
	Label replaceLabel;

	TextEditor* parent;
};


class LinebreakDisplay : public Component,
	public LambdaCodeDocumentListener
{
public:

	LinebreakDisplay(mcl::TextDocument& d);

	void refresh();

	void paint(Graphics& g) override;

	void setViewTransform(const AffineTransform& t);

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
		static bool trimAndGet(String& s, const String& keyword);

		static int getLevel(FoldableLineRange::WeakPtr p);

		static void trimIf(String& s, const String& keyword);

		static EntryType getEntryType(String& s);
	};

	void selectionChanged() override;

	void displayedLineRangeChanged(Range<int> newRange) override;

    void addLineNumbersForParentItems(Array<int>& list, int lineNumber)
    {
        for(auto c: items)
        {
            c->addLineNumbersForParentItems(list, lineNumber);
        }
    }

	String getTextForFoldRange(FoldableLineRange::WeakPtr p);

	struct Item : public Component,
	              public TooltipWithArea::Client
	{
		static const int Height = 24;

		Item(FoldableLineRange::WeakPtr p_, FoldMap& m);;

		TooltipWithArea::Data getTooltip(Point<float> positionInThisComponent) override;

		void setBoldLine(int lineToBolden);

		void setDisplayedRange(Range<int> newRange);


		void updateHeight();

		void mouseDown(const MouseEvent& e) override;

		void mouseDoubleClick(const MouseEvent& e) override;

		void setSelected(bool shouldBeSelected, bool grabFocus);

		void resized() override;

		Font getFont();


		void paint(Graphics& g) override;

		bool folded = false;
		bool isBoldLine = false;

		bool onScreen = false;
		bool edgeOnScreen = false;

        void addLineNumbersForParentItems(Array<int>& list, int lineNumber) const
        {
            auto lr = p->getLineRange();
            
            if(lr.contains(lineNumber))
            {
                list.add(lr.getStart());
                
                for(auto c: children)
                    c->addLineNumbersForParentItems(list, lineNumber);
            }
        }
        
		String text;
		EntryType type;
		FoldableLineRange::WeakPtr p;

		OwnedArray<Item> children;
		bool clicked = false;

		int bestWidth = 0;
	};

	

	FoldMap(TextDocument& d);;

	~FoldMap();

	LanguageManager* getLanguageManager();
    
	bool keyPressed(const KeyPress& k) override;

	int getBestWidth() const;

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged) override;

	void paint(Graphics& g) override;

	void rootWasRebuilt(FoldableLineRange::WeakPtr rangeThatHasChanged) override;

	void rebuild();

	void updateSize();

	void resized() override;

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

	CodeMap(TextDocument& doc_);

	struct DelayedUpdater: public Timer
	{
		DelayedUpdater(CodeMap& p);;

		void timerCallback() override;

		CodeMap& parent;
	} rebuilder;

	void timerCallback();

	~CodeMap();

	void selectionChanged() override;

	void displayedLineRangeChanged(Range<int> newRange) override;

	void codeDocumentTextDeleted(int startIndex, int endIndex) override;

	void codeDocumentTextInserted(const String& newText, int insertIndex) override;

	float getLineNumberFromEvent(const MouseEvent& e) const;

	Rectangle<int> getPreviewBounds(const MouseEvent& e);

	void mouseEnter(const MouseEvent& e) override;

	void mouseExit(const MouseEvent& e) override;

	void mouseMove(const MouseEvent& e) override;

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	int getNumLinesToShow() const;

	void rebuild();

	struct HoverPreview : public Component
	{
		HoverPreview(CodeMap& parent_, int centerRow);

		void setCenterRow(int newCenterRow);

		void paint(Graphics& g) override;

		CodeMap& parent;
		Range<int> rows;
		int centerRow;
		float scale = 1.0f;
	};

	void resized();

	void setVisibleRange(Range<int> visibleLines);

	bool isActive() const;

	float lineToY(int lineNumber) const;

	int yToLine(float y) const;

	CodeEditorComponent::ColourScheme* getColourScheme();

	CodeTokeniser* getTokeniser();

	ScopedPointer<HoverPreview> preview;

	void paint(Graphics& g);

	struct ColouredRectangle
	{
		bool isWhitespace() const;

		int lineNumber;
		bool upper;
		bool selected = false;
		Colour c;
		int position;
		Rectangle<float> area;
	};

	Array<ColouredRectangle> colouredRectangles;

	TextDocument& doc;

	void visibilityChanged() override;

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
