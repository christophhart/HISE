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


class BreakpointManager
{
    virtual ~BreakpointManager();;
    
	struct Listener
	{
        virtual ~Listener();;
		virtual void breakpointsChanged() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	void addListener(Listener* l);

    void removeListener(Listener* l);

    void addBreakpoint(int lineNumber, NotificationType notifyListeners);

private:

	void sendListenerMessage();

    Array<int> breakpoints;
	Array<WeakReference<Listener>> listeners;

	JUCE_DECLARE_WEAK_REFERENCEABLE(BreakpointManager);
};

struct Bookmark
{
	String name;
	int lineNumber;
};

class FoldableLineRange : public ReferenceCountedObject
{
public:

	FoldableLineRange(const CodeDocument& doc, Range<int> r, bool folded_=false);;

	using Ptr = ReferenceCountedObjectPtr<FoldableLineRange>;
	using List = ReferenceCountedArray<FoldableLineRange>;
	using WeakPtr = WeakReference<FoldableLineRange>;
	using LineRangeFunction = std::function<FoldableLineRange::List(const CodeDocument&)> ;

	static void sortList(List listToSort, bool sortChildren = false);

	/** This will check whether the list is correctly nested with a weak ref to the parent. */
	static Result checkList(List& listToCheck, WeakPtr parent=nullptr);

	class Listener
	{
	public:

        virtual ~Listener();;
		virtual void foldStateChanged(WeakPtr rangeThatHasChanged) = 0;
		virtual void rootWasRebuilt(WeakPtr newRoot);;

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	class Holder
	{
	public:

		Holder(CodeDocument& d);;

		enum LineType
		{
			Nothing,
			RangeStartOpen,
			RangeStartClosed,
			Between,
			Folded,
			RangeEnd
		};

		void toggleFoldState(int lineNumber);

		void unfold(int lineNumber);

		void updateFoldState(WeakPtr r);

		void sendFoldChangeMessage(WeakPtr r);

		WeakPtr getRangeWithStartAtLine(int lineNumber) const;

		int getNearestLineStartOfAnyRange(int lineNumber);

		WeakPtr getRangeContainingLine(int lineNumber) const;

		Range<int> getRangeForLineNumber(int lineNumber) const;

		LineType getLineType(int lineNumber) const;

		bool isFolded(int lineNumber) const;

		void addToFlatList(List& flatList, const List& nestedList);

		void setRanges(FoldableLineRange::List newRanges);

		CodeDocument& doc;

		BigInteger lineStates;
		Array<WeakReference<Listener>> listeners;

		List all;
		List roots;
	};


	bool contains(Ptr other) const;

	WeakPtr getParent() const;


	bool isFolded() const;

	bool forEach(const std::function<bool(WeakPtr)>& f);

	Range<int> getLineRange() const;

	Bookmark getBookmark() const;
	
	void setFolded(bool shouldBeFolded);

	bool operator==(const FoldableLineRange& other) const;

	List children;
	WeakPtr parent;

	void setEnd(int charPos);

	int getNearestLineStart(int lineNumber);

private:

	CodeDocument::Position start, end;

	bool folded = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(FoldableLineRange);
};




//==============================================================================
class TextDocument : public CoallescatedCodeDocumentListener,
                     public FoldableLineRange::Listener
{
public:
	enum class Metric
	{
		top,
		ascent,
		baseline,
		bottom,
	};

	/**
	 Text categories the caret may be targeted to. For forward jumps,
	 the caret is moved to be immediately in front of the first character
	 in the given catagory. For backward jumps, it goes just after the
	 first character of that category.
	 */
	enum class Target
	{
		whitespace,
		punctuation,
		character,
		subword,
		cppToken,
		commandTokenNav, // used for all keypresses with cmd
		subwordWithPoint,
		word,
		firstnonwhitespace,
        firstnonwhitespaceAfterLineBreak,
		token,
		line,
		lineUntilBreak,
		paragraph,
		scope,
		document,
	};
	enum class Direction { forwardRow, backwardRow, forwardCol, backwardCol, };

	struct SelectionAction : public UndoableAction
	{
		SelectionAction(TextDocument& t, const Array<Selection>& now_);;

		bool perform() override;

		bool undo() override;

		TextDocument& doc;
		Array<Selection> before;
		Array<Selection> now;
	};

	struct RowData
	{
		int rowNumber = 0;
		bool isRowSelected = false;
		juce::RectangleList<float> bounds;
	};

	

	Array<Bookmark> getBookmarks() const;

	TextDocument(CodeDocument& doc_);;

	void deactivateLines(SparseSet<int> deactivatedLines);

	/** Get the current font. */
	juce::Font getFont() const;

	/** Get the line spacing. */
	float getLineSpacing() const;

	/** Set the font to be applied to all text. */
	void setFont(juce::Font fontToUse);

	/** Replace the whole document content. */
	void replaceAll(const juce::String& content);

	/** Replace the list of selections with a new one. */
	void setSelections(const juce::Array<Selection>& newSelections, bool useUndo);

	/** Add a selection to the list. */
	void addSelection(Selection selection);

	/** Replace the selection at the given index. The index must be in range. */
	void setSelection(int index, Selection newSelection, bool useUndo);

	void sendSelectionChangeMessage() const;

	/** Get the number of rows in the document. */
	int getNumRows() const;

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged);

	void rootWasRebuilt(FoldableLineRange::WeakPtr newRoot);

	/** Get the number of columns in the given row. */
	int getNumColumns(int row) const;

	/** Return the vertical position of a metric on a row. */
	float getVerticalPosition(int row, Metric metric) const;

	/** Return the position in the document at the given index, using the given
		metric for the vertical position. */
	juce::Point<float> getPosition(juce::Point<int> index, Metric metric) const;

	/** Return an array of rectangles covering the given selection. If
		the clip rectangle is empty, the whole selection is returned.
		Otherwise it gets only the overlapping parts.
	 */
	RectangleList<float> getSelectionRegion(Selection selection,
		juce::Rectangle<float> clip = {}) const;

	/** Return the bounds of the entire document. */
	juce::Rectangle<float> getBounds() const;

	Array<Line<float>> getUnderlines(const Selection& s, Metric m) const;

	/** Return the bounding box for the glyphs on the given row, and within
		the given range of columns. The range start must not be negative, and
		must be smaller than ncols. The range end is exclusive, and may be as
		large as ncols + 1, in which case the bounds include an imaginary
		whitespace character at the end of the line. The vertical extent is
		that of the whole line, not the ascent-to-descent of the glyph.
	 */
	juce::RectangleList<float> getBoundsOnRow(int row, juce::Range<int> columns, GlyphArrangementArray::OutOfBoundsMode m_) const;

	/** Return the position of the glyph at the given row and column. */
	juce::Rectangle<float> getGlyphBounds(juce::Point<int> index, GlyphArrangementArray::OutOfBoundsMode m) const;

	/** Return a glyph arrangement for the given row. If token != -1, then
	 only glyphs with that token are returned.
	 */
	juce::GlyphArrangement getGlyphsForRow(int row, Range<float> visibleRange, int token = -1, bool withTrailingSpace = false) const;

	/** Return all glyphs whose bounding boxes intersect the given area. This method
		may be generous (including glyphs that don't intersect). If token != -1, then
		only glyphs with that token mask are returned.
	 */
	juce::GlyphArrangement findGlyphsIntersecting(juce::Rectangle<float> area, int token = -1) const;

	void drawWhitespaceRectangles(int row, Graphics& g);

	/** Return the range of rows intersecting the given rectangle. */
	juce::Range<int> getRangeOfRowsIntersecting(juce::Rectangle<float> area) const;

	/** Return data on the rows intersecting the given area. This is sort
		of a convenience method for calling getBoundsOnRow() over a range,
		but could be faster if horizontal extents are not computed.
	 */
	juce::Array<RowData> findRowsIntersecting(juce::Rectangle<float> area,
		bool computeHorizontalExtent = false) const;

	/** Find the row and column index nearest to the given position. */
	juce::Point<int> findIndexNearestPosition(juce::Point<float> position) const;

	/** Return an index pointing to one-past-the-end. */
	juce::Point<int> getEnd() const;

	bool navigateLeftRight(juce::Point<int>& index, bool right) const;

	bool navigateUpDown(juce::Point<int>& index, bool down) const;

	int getColumnIndexAccountingTabs(juce::Point<int>& index) const;
	
	/** Ensures that the y-position of the index equals the positionToMaintain column. */
	void applyTabsToPosition(juce::Point<int>& index, int positionToMaintain) const;

	/** Navigate an index to the first character of the given categaory.
	 */
	void navigate(juce::Point<int>& index, Target target, Direction direction) const;

	/** Navigate all selections. */
	void navigateSelections(Target target, Direction direction, Selection::Part part);

	Selection search(juce::Point<int> start, const juce::String& target) const;

	/** Return the character at the given index. */
	juce::juce_wchar getCharacter(juce::Point<int> index) const;

	

	/** Return the number of active selections. */
	int getNumSelections() const;

	/** Return a line in the document. */
	const juce::String& getLine(int lineIndex) const;

	/** Return one of the current selections. */
	const Selection& getSelection(int index) const;

	

	float getRowHeight() const;

	/** Return the current selection state. */
	const juce::Array<Selection>& getSelections() const;

	Array<Selection>& getSelections();

	Rectangle<float> getCharacterRectangle() const;

	/** Return the content within the given selection, with newlines if the
		selection spans muliple lines.
	 */
	juce::String getSelectionContent(Selection selection) const;

	/** Apply a transaction to the document, and return its reciprocal. The selection
		identified in the transaction does not need to exist in the document.
	 */
	Transaction fulfill(const Transaction& transaction);

	/* Reset glyph token values on the given range of rows. */
	void clearTokens(juce::Range<int> rows);

	/** Apply tokens from a set of zones to a range of rows. */
	void applyTokens(juce::Range<int> rows, const juce::Array<Selection>& zones);

	void setMaxLineWidth(int maxWidth);

	CodeDocument& getCodeDocument();

	void invalidate(Range<int> lineRange);

	void rebuildRowPositions();

	void lineRangeChanged(Range<int> r, bool wasAdded) override;

	void codeChanged(bool wasInserted, int startIndex, int endIndex) override;

	/** returns the amount of lines occupied by the row. This can be > 1 when the line-break is active. */
	int getNumLinesForRow(int rowIndex) const;

	float getFontHeight() const;;

	void addSelectionListener(Selection::Listener* l);

	void removeSelectionListener(Selection::Listener* l);

	void setDisplayedLineRange(Range<int> newRange);

	bool jumpToLine(int lineNumber, bool justScroll=false);

	void setDuplicateOriginal(const Selection& s);

	FoldableLineRange::Holder& getFoldableLineRangeHolder();

	const FoldableLineRange::Holder& getFoldableLineRangeHolder() const;

	void addFoldListener(FoldableLineRange::Listener* l);

	void removeFoldListener(FoldableLineRange::Listener* l);

	void setSearchResults(const Array<Selection>& newSearchResults);

	void setExternalViewUndoManager(UndoManager* um);

	Array<Selection> getSearchResults() const;

private:

	mutable int columnTryingToMaintain = -1;

	UndoManager viewUndoManager;
    UndoManager* viewUndoManagerToUse = &viewUndoManager;

	Array<Selection> searchResults;

	FoldableLineRange::Holder foldManager;

	Range<int> currentlyDisplayedLineRange;

	Array<float> rowPositions;

	bool checkThis = false;
	String shouldBe;
	String isReally;

	friend class TextEditor;

	float lineSpacing = 1.333f;

	Selection duplicateOriginal;

	CodeDocument& doc;
	bool internalChange = false;

	mutable juce::Rectangle<float> cachedBounds;
	GlyphArrangementArray lines;
	juce::Font font;

	Array<WeakReference<Selection::Listener>> selectionListeners;

	juce::Array<Selection> selections;
};

class TokenCollection;
class LanguageManager;


}
