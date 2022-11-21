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
    virtual ~BreakpointManager() {};
    
	struct Listener
	{
        virtual ~Listener() {};
		virtual void breakpointsChanged() = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	void addListener(Listener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeListener(Listener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void addBreakpoint(int lineNumber, NotificationType notifyListeners)
	{
		if (!breakpoints.contains(lineNumber))
		{
			breakpoints.add(lineNumber);
			breakpoints.sort();

			if (notifyListeners != dontSendNotification)
				sendListenerMessage();
		}
	}

private:

	void sendListenerMessage()
	{
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->breakpointsChanged();
		}
	}

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

	FoldableLineRange(const CodeDocument& doc, Range<int> r, bool folded_=false) :
		start(doc, r.getStart(), 0),
		end(doc, r.getEnd(), 0),
		folded(folded_)
	{
		start.setPositionMaintained(true);
		end.setPositionMaintained(true);
	};

	using Ptr = ReferenceCountedObjectPtr<FoldableLineRange>;
	using List = ReferenceCountedArray<FoldableLineRange>;
	using WeakPtr = WeakReference<FoldableLineRange>;
	using LineRangeFunction = std::function<FoldableLineRange::List(const CodeDocument&)> ;

	static void sortList(List listToSort, bool sortChildren = false)
	{
		struct PositionSorter
		{
			static int compareElements(FoldableLineRange* first, FoldableLineRange* second)
			{
				auto start1 = first->start.getLineNumber();
				auto start2 = second->start.getLineNumber();

				if (start1 < start2)
					return -1;
				if (start1 > start2)
					return 1;

				return 0;
			}
		};

		PositionSorter s;
		listToSort.sort(s);

		if (sortChildren)
		{
			for (auto l : listToSort)
				sortList(l->children, true);
		}
	}

	/** This will check whether the list is correctly nested with a weak ref to the parent. */
	static Result checkList(List& listToCheck, WeakPtr parent=nullptr)
	{
		for (int i = 0; i < listToCheck.size(); i++)
		{
			if (listToCheck[i]->getLineRange().getLength() <= 1)
				listToCheck.remove(i--);
		}

		for (auto l : listToCheck)
		{
			if (l->parent != parent)
				return Result::fail("Illegal parent in list");

			auto r = checkList(l->children, l);

			if (!r.wasOk())
				return r;
		}

		return Result::ok();
	}

	class Listener
	{
	public:

        virtual ~Listener() {};
		virtual void foldStateChanged(WeakPtr rangeThatHasChanged) = 0;
		virtual void rootWasRebuilt(WeakPtr newRoot) {};

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	class Holder
	{
	public:

		Holder(CodeDocument& d) :
			doc(d)
		{};

		enum LineType
		{
			Nothing,
			RangeStartOpen,
			RangeStartClosed,
			Between,
			Folded,
			RangeEnd
		};

		void toggleFoldState(int lineNumber)
		{
			if (auto r = getRangeWithStartAtLine(lineNumber))
			{
				r->folded = !r->folded;
				updateFoldState(r);
			}
		}

		void unfold(int lineNumber)
		{
			for (auto a : all)
			{
				if (a->getLineRange().contains(lineNumber))
					a->folded = false;
			}

			updateFoldState(nullptr);
		}

		void updateFoldState(WeakPtr r)
		{
			lineStates.clear();

			for (auto a : all)
			{
				if (a->folded)
				{
					auto r = a->getLineRange();
					lineStates.setRange(r.getStart() + 1, r.getLength() - 1, true);
				}
			}

			sendFoldChangeMessage(r);
		}

		void sendFoldChangeMessage(WeakPtr r)
		{
			for (auto l : listeners)
			{
				if (l.get() != nullptr)
					l->foldStateChanged(r);
			}
		}

		WeakPtr getRangeWithStartAtLine(int lineNumber) const
		{
			for (auto r : all)
			{
				if (r->getLineRange().getStart() == lineNumber)
					return r;
			}

			return nullptr;
		}

		int getNearestLineStartOfAnyRange(int lineNumber)
		{
			for (auto r : all)
			{
				auto n = r->getNearestLineStart(lineNumber);

				if (n != -1)
					return n;
			}

			return lineNumber;
		}

		WeakPtr getRangeContainingLine(int lineNumber) const
		{
			for (auto r : all)
			{
				if (r->getLineRange().contains(lineNumber))
					return r;
			}

			return nullptr;
		}

		Range<int> getRangeForLineNumber(int lineNumber) const
		{
			if (auto p = getRangeContainingLine(lineNumber))
			{
				if (p->folded)
					return { lineNumber, lineNumber + 1 };
				else
					return p->getLineRange();
			}

			return {};
		}

		LineType getLineType(int lineNumber) const
		{
			bool isBetween = false;
			
			for (auto l : all)
			{
				auto lineRange = l->getLineRange();

				isBetween |= lineRange.contains(lineNumber);

				if (lineRange.getStart() == lineNumber)
					return l->isFolded() ? RangeStartClosed : RangeStartOpen;

				if (lineRange.contains(lineNumber) && l->isFolded())
					return Folded;

				if (lineRange.getEnd()-1 == lineNumber)
					return RangeEnd;
			}

			if (isBetween)
				return Between;
			else
				return Nothing;
		}

		bool isFolded(int lineNumber) const
		{
			return lineStates[lineNumber];
		}

		void addToFlatList(List& flatList, const List& nestedList)
		{
			for (auto l : nestedList)
			{
				flatList.add(l);
				addToFlatList(flatList, l->children);
			}
		}

		void setRanges(FoldableLineRange::List newRanges)
		{
			

			Array<int> foldedLines;

			checkList(newRanges, nullptr);

			List l;
			
			addToFlatList(l, newRanges);

			std::swap(newRanges, roots);

			for (auto old : all)
			{
				if (old->folded)
				{
					for (auto n : l)
					{
						if (*old == *n)
						{
							n->setFolded(true);
							break;
						}
					}
				}
			}

			std::swap(all, l);

			for (auto l : listeners)
			{
				if (l.get() != nullptr)
					l->rootWasRebuilt(nullptr);
			}

			updateFoldState(nullptr);
		}

		CodeDocument& doc;

		BigInteger lineStates;
		Array<WeakReference<Listener>> listeners;

		List all;
		List roots;
	};


	bool contains(Ptr other) const
	{
		return getLineRange().contains(other->getLineRange());
	}

	WeakPtr getParent() const { return parent; }

	
	bool isFolded() const
	{
		if (folded)
			return true;

		WeakPtr p = parent;

		while (p != nullptr)
		{
			if (p->folded)
				return true;

			p = p->parent;
		}

		return false;
	}

	bool forEach(const std::function<bool(WeakPtr)>& f)
	{
		if (f(this))
			return true;

		for (auto c : children)
		{
			if (c->forEach(f))
				return true;
		}

		return false;
	}

	Range<int> getLineRange() const
	{
		return { start.getLineNumber(), end.getLineNumber() +1};
	}

	Bookmark getBookmark() const;
	
	void setFolded(bool shouldBeFolded)
	{
		folded = shouldBeFolded;
	}

	bool operator==(const FoldableLineRange& other) const
	{
		return other.start.getPosition() == start.getPosition();
	}

	List children;
	WeakPtr parent;

	void setEnd(int charPos)
	{
		end.setPosition(charPos);
	}

	int getNearestLineStart(int lineNumber)
	{
		if (getLineRange().contains(lineNumber))
		{
			for (auto c : children)
			{
				auto n = c->getNearestLineStart(lineNumber);
				if (n != -1)
					return n;
			}

			return start.getLineNumber();
		}

		return -1;
	}

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
		SelectionAction(TextDocument& t, const Array<Selection>& now_) :
			now(now_),
			doc(t)
		{
			before.addArray(t.selections);
		};

		bool perform() override
		{
			doc.setSelections(now, false);
			return true;
		}

		bool undo() override
		{
			doc.setSelections(before, false);
			return true;
		}

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

	void deactivateLines(SparseSet<int> deactivatedLines)
	{
		
	}

	/** Get the current font. */
	juce::Font getFont() const { return font; }

	/** Get the line spacing. */
	float getLineSpacing() const { return lineSpacing; }

	/** Set the font to be applied to all text. */
	void setFont(juce::Font fontToUse)
	{

		font = fontToUse; lines.font = fontToUse;
		lines.characterRectangle = { 0.0f, 0.0f, font.getStringWidthFloat(" "), font.getHeight() };
	}

	/** Replace the whole document content. */
	void replaceAll(const juce::String& content);

	/** Replace the list of selections with a new one. */
	void setSelections(const juce::Array<Selection>& newSelections, bool useUndo);

	/** Add a selection to the list. */
	void addSelection(Selection selection) 
	{ 
		auto newSelections = selections;
		newSelections.add(selection);
		setSelections(newSelections, true);
	}

	/** Replace the selection at the given index. The index must be in range. */
	void setSelection(int index, Selection newSelection, bool useUndo) 
	{
		if (useUndo)
		{
			auto copy = selections;
			copy.setUnchecked(index, newSelection);

			viewUndoManagerToUse->perform(new SelectionAction(*this, copy));
		}
		else
			selections.setUnchecked(index, newSelection); sendSelectionChangeMessage(); 
	}

	void sendSelectionChangeMessage() const
	{
		for (auto l : selectionListeners)
		{
			if (l != nullptr)
				l.get()->selectionChanged();
		}
	}

	/** Get the number of rows in the document. */
	int getNumRows() const;

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged)
	{
		rebuildRowPositions();
	}

	void rootWasRebuilt(FoldableLineRange::WeakPtr newRoot)
	{

	}

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
	int getNumSelections() const { return selections.size(); }

	/** Return a line in the document. */
	const juce::String& getLine(int lineIndex) const { return lines[lineIndex]; }

	/** Return one of the current selections. */
	const Selection& getSelection(int index) const;

	

	float getRowHeight() const
	{
		return font.getHeight() * lineSpacing;
	}

	/** Return the current selection state. */
	const juce::Array<Selection>& getSelections() const;

	Array<Selection>& getSelections();

	Rectangle<float> getCharacterRectangle() const
	{
		return lines.characterRectangle;
	}

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

	void setMaxLineWidth(int maxWidth)
	{
		if (maxWidth != lines.maxLineWidth)
		{
			lines.maxLineWidth = maxWidth;
			invalidate({});
		}
	}

	CodeDocument& getCodeDocument()
	{
		return doc;
	}

	void invalidate(Range<int> lineRange)
	{
		lines.invalidate(lineRange);
		cachedBounds = {};
		rebuildRowPositions();
	}

	void rebuildRowPositions()
	{
		rowPositions.clearQuick();
		rowPositions.ensureStorageAllocated(lines.size());

		float yPos = 0.0f;

		float gap = getCharacterRectangle().getHeight() * (lineSpacing - 1.f) * 0.5f;

		for (int i = 0; i < lines.size(); i++)
		{
			rowPositions.add(yPos);

			auto l = lines.lines[i];

			lines.ensureValid(i);

			if(!foldManager.isFolded(i))
				yPos += l->height + gap;
		}

		rowPositions.add(yPos);
	}

	void lineRangeChanged(Range<int> r, bool wasAdded) override
	{
		if (!wasAdded)
		{
			auto rangeToInvalidate = r;
			rangeToInvalidate.setLength(jmax(1, r.getLength()));
			invalidate(rangeToInvalidate);
		}
			
		if (!wasAdded && r.getLength() > 0)
		{
			lines.removeRange(r);
			lines.set(r.getStart(), doc.getLine(r.getStart()));
			return;
		}

		if (r.getLength() > 1)
		{
			lines.set(r.getStart(), doc.getLine(r.getStart()));

			for (int i = r.getStart() + 1; i < r.getEnd(); i++)
				lines.insert(i, doc.getLine(i));
		}
		else
		{
			auto lineNumber = r.getStart();
			lines.set(lineNumber, doc.getLine(lineNumber));
		}

		if (wasAdded && r.getEnd() > getNumRows())
		{
			lines.set(r.getEnd(), "");
			
			
		}


		
	}

	void codeChanged(bool wasInserted, int startIndex, int endIndex) override
	{
		CodeDocument::Position pos(getCodeDocument(), wasInserted ? endIndex : startIndex);

		if (getNumSelections() == 1)
		{
			setSelections(Selection(pos.getLineNumber(), pos.getIndexInLine(), pos.getLineNumber(), pos.getIndexInLine()), false);
		}
	}

	/** returns the amount of lines occupied by the row. This can be > 1 when the line-break is active. */
	int getNumLinesForRow(int rowIndex) const
	{
		if(isPositiveAndBelow(rowIndex, lines.lines.size()))
			return roundToInt(lines.lines[rowIndex]->height / font.getHeight());

		return 1;
	}

	float getFontHeight() const { return font.getHeight(); };

	void addSelectionListener(Selection::Listener* l)
	{
		selectionListeners.addIfNotAlreadyThere(l);
		l->displayedLineRangeChanged(currentlyDisplayedLineRange);
	}

	void removeSelectionListener(Selection::Listener* l)
	{
		selectionListeners.removeAllInstancesOf(l);
	}

	void setDisplayedLineRange(Range<int> newRange)
	{
		if (newRange != currentlyDisplayedLineRange)
		{
			currentlyDisplayedLineRange = newRange;

			for (auto sl : selectionListeners)
			{
                if(sl != nullptr)
                    sl->displayedLineRangeChanged(currentlyDisplayedLineRange);
			}
		}
	}

	bool jumpToLine(int lineNumber, bool justScroll=false)
	{
		if (lineNumber >= 0)
		{
			if (justScroll)
			{
				auto newRange = currentlyDisplayedLineRange.movedToStartAt(lineNumber - currentlyDisplayedLineRange.getLength()/2);
				setDisplayedLineRange(newRange);
				return true;
			}

			auto sourceLine = Point<int>(lineNumber, 0);

			navigate(sourceLine, Target::character, Direction::backwardCol);
			navigate(sourceLine, Target::firstnonwhitespace, Direction::backwardCol);

			Selection ss(sourceLine, sourceLine);

			auto newRange = currentlyDisplayedLineRange.movedToStartAt(lineNumber - currentlyDisplayedLineRange.getLength() / 2 - 4);

			setDisplayedLineRange(newRange);

			setSelections({ ss }, true);



			return true;
		}

		return false;
	}

	void setDuplicateOriginal(const Selection& s)
	{
		duplicateOriginal = s;
	}

	FoldableLineRange::Holder& getFoldableLineRangeHolder()
	{
		return foldManager;
	}

	const FoldableLineRange::Holder& getFoldableLineRangeHolder() const
	{
		return foldManager;
	}

	void addFoldListener(FoldableLineRange::Listener* l)
	{
		foldManager.listeners.addIfNotAlreadyThere(l);
	}

	void removeFoldListener(FoldableLineRange::Listener* l)
	{
		foldManager.listeners.removeAllInstancesOf(l);
	}

	void setSearchResults(const Array<Selection>& newSearchResults)
	{
		searchResults = newSearchResults;
	}

    void setExternalViewUndoManager(UndoManager* um)
    {
        viewUndoManagerToUse = um;
    }
    
	Array<Selection> getSearchResults() const
	{
		return searchResults;
	}

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
