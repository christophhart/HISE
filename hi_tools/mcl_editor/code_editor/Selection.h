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


//==============================================================================
/**
	A data structure encapsulating a contiguous range within a TextDocument.
	The head and tail refer to the leading and trailing edges of a selected
	region (the head is where the caret would be rendered). The selection is
	exclusive with respect to the range of columns (y), but inclusive with
	respect to the range of rows (x). It is said to be oriented when
	head <= tail, and singular when head == tail, in which case it would be
	rendered without any highlighting.
 */
struct Selection
{
	struct Listener
	{
		virtual ~Listener() {};

		virtual void selectionChanged() = 0;

		virtual void displayedLineRangeChanged(Range<int> newRange) {};

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	enum class Part
	{
		head, tail, both,
	};

	Selection() {}
	Selection(const juce::CodeDocument& doc, int headChar, int tailChar);
	Selection(juce::Point<int> head) : head(head), tail(head) {}
	Selection(juce::Point<int> head, juce::Point<int> tail) : head(head), tail(tail) {}
	Selection(int r0, int c0, int r1, int c1) : head(r0, c0), tail(r1, c1) {}

	/** Construct a selection whose head is at (0, 0), and whose tail is at the end of
		the given content string, which may span multiple lines.
	 */
	Selection(const juce::String& content);

	bool operator== (const Selection& other) const
	{
		return head == other.head && tail == other.tail;
	}

	bool operator< (const Selection& other) const
	{
		const auto A = this->oriented();
		const auto B = other.oriented();
		if (A.head.x == B.head.x) return A.head.y < B.head.y;
		return A.head.x < B.head.x;
	}

	juce::String toString() const
	{
		return "(" + head.toString() + ") - (" + tail.toString() + ")";
	}

	/** Whether or not this selection covers any extent. */
	bool isSingular() const { return head == tail; }

	/** Whether or not this selection is only a single line. */
	bool isSingleLine() const { return head.x == tail.x; }

	/** Whether the given row is within the selection. */
	bool intersectsRow(int row) const
	{
		return isOriented()
			? head.x <= row && row <= tail.x
			: head.x >= row && row >= tail.x;
	}

	static Selection fromCodePosition(const CodeDocument::Position& p)
	{
		return Selection(p.getLineNumber(), p.getIndexInLine(), p.getLineNumber(), p.getIndexInLine());
	}

	static Selection fromCodePosition(const CodeDocument::Position& s, const CodeDocument::Position& e)
	{
		return Selection(s.getLineNumber(), s.getIndexInLine(), e.getLineNumber(), e.getIndexInLine());
	}

	CodeDocument::Position toCodePosition(const CodeDocument& doc, bool getHead=true) const
	{
		return CodeDocument::Position(doc, getHead ? head.x : tail.x, getHead ? head.y : tail.y);
	}

	/** Return the range of columns this selection covers on the given row.
	 */
	juce::Range<int> getColumnRangeOnRow(int row, int numColumns) const
	{
		const auto A = oriented();

		if (row < A.head.x || row > A.tail.x)
			return { 0, 0 };
		if (row == A.head.x && row == A.tail.x)
			return { A.head.y, A.tail.y };
		if (row == A.head.x)
			return { A.head.y, numColumns };
		if (row == A.tail.x)
			return { 0, A.tail.y };
		return { 0, numColumns };
	}

	/** Whether the head precedes the tail. */
	bool isOriented() const;

	/** Return a copy of this selection, oriented so that head <= tail. */
	Selection oriented() const;

	/** Return a copy of this selection, with its head and tail swapped. */
	Selection swapped() const;

	/** Return a copy of this selection, with head and tail at the beginning and end
		of their respective lines if the selection is oriented, or otherwise with
		the head and tail at the end and beginning of their respective lines.
	 */
	Selection horizontallyMaximized(const TextDocument& document) const;

	/** Return a copy of this selection, with its tail (if oriented) moved to
		account for the shape of the given content, which may span multiple
		lines. If instead head > tail, then the head is bumped forward.
	 */
	Selection measuring(const juce::String& content) const;

	/** Return a copy of this selection, with its head (if oriented) placed
		at the given index, and tail moved as to leave the measure the same.
		If instead head > tail, then the tail is moved.
	 */
	Selection startingFrom(juce::Point<int> index) const;

	Selection withStyle(int token) const { auto s = *this; s.token = token; return s; }

	/** Modify this selection (if necessary) to account for the disapearance of a
		selection someplace else.
	 */
	void pullBy(Selection disappearingSelection);

	/** Modify this selection (if necessary) to account for the appearance of a
		selection someplace else.
	 */
	void pushBy(Selection appearingSelection);

	/** Modify an index (if necessary) to account for the disapearance of
		this selection.
	 */
	void pull(juce::Point<int>& index) const;

	/** Modify an index (if necessary) to account for the appearance of
		this selection.
	 */
	void push(juce::Point<int>& index) const;

    bool contains(Point<int> pos) const
    {
        if(isSingular())
            return false;
        
        auto o = oriented();
        
        auto isBiggerThanHead = pos.x > o.head.x ||
                              ((pos.x == o.head.x) && (pos.y > o.head.y));
        auto isSmallerThanTail = pos.x < o.tail.x ||
                               ((pos.x == o.tail.x) && (pos.y < o.tail.y));
        
        return isBiggerThanHead && isSmallerThanTail;
    }
    
	juce::Point<int> head; // (row, col) of the selection head (where the caret is drawn)
	juce::Point<int> tail; // (row, col) of the tail
	int token = 0;
};




//==============================================================================
struct Transaction
{
	using Callback = std::function<void(const Transaction&)>;
	enum class Direction { forward, reverse };

	/** Return a copy of this transaction, corrected for delete and backspace
		characters. For example, if content == "\b" then the selection head is
		decremented and the content is erased.
	 */
	Transaction accountingForSpecialCharacters(const TextDocument& document) const;

	/** Return an undoable action, whose perform method thill fulfill this
		transaction, and which caches the reciprocal transaction to be
		issued in the undo method.
	 */
	juce::UndoableAction* on(TextDocument& document, Callback callback);

	mcl::Selection selection;
	juce::String content;
	juce::Rectangle<float> affectedArea;
	Direction direction = Direction::forward;

private:
	class Undoable;
};




}
