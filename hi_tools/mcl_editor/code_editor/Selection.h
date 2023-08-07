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
		virtual ~Listener();;

		virtual void selectionChanged() = 0;

		virtual void displayedLineRangeChanged(Range<int> newRange);;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	enum class Part
	{
		head, tail, both,
	};

	Selection();
	Selection(const juce::CodeDocument& doc, int headChar, int tailChar);
	Selection(juce::Point<int> head);
	Selection(juce::Point<int> head, juce::Point<int> tail);
	Selection(int r0, int c0, int r1, int c1);

	/** Construct a selection whose head is at (0, 0), and whose tail is at the end of
		the given content string, which may span multiple lines.
	 */
	Selection(const juce::String& content);

	bool operator== (const Selection& other) const;

	bool operator< (const Selection& other) const;

	juce::String toString() const;

	/** Whether or not this selection covers any extent. */
	bool isSingular() const;

	/** Whether or not this selection is only a single line. */
	bool isSingleLine() const;

	/** Whether the given row is within the selection. */
	bool intersectsRow(int row) const;

	static Selection fromCodePosition(const CodeDocument::Position& p);

	static Selection fromCodePosition(const CodeDocument::Position& s, const CodeDocument::Position& e);

	CodeDocument::Position toCodePosition(const CodeDocument& doc, bool getHead=true) const;

	/** Return the range of columns this selection covers on the given row.
	 */
	juce::Range<int> getColumnRangeOnRow(int row, int numColumns) const;

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

	Selection withStyle(int token) const;

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

    bool contains(Point<int> pos) const;

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
