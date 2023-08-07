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
   This class wraps a StringArray and memoizes the evaluation of glyph
   arrangements derived from the associated strings.
*/
class GlyphArrangementArray
{
public:

	enum OutOfBoundsMode
	{
		ReturnNextLine,
		ReturnLastCharacter,
		ReturnBeyondLastCharacter,
		AssertFalse,
		numOutOfBoundsModes
	};

	int size() const;
	void clear();

	void set(int index, const juce::String& string);

	void insert(int index, const String& string);

	void removeRange(Range<int> r);

	void removeRange(int startIndex, int numberToRemove);
	const juce::String& operator[] (int index) const;

	int getToken(int row, int col, int defaultIfOutOfBounds) const;
	void clearTokens(int index);
	void applyTokens(int index, Selection zone);
	juce::GlyphArrangement getGlyphs(int index,
		float baseline,
		int token,
		Range<float> visibleRange,
		bool withTrailingSpace = false) const;

	struct Entry : public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Entry>;

		Entry();
		Entry(const juce::String& string, int maxLineWidth);
		juce::String string;
		juce::GlyphArrangement glyphsWithTrailingSpace;
		juce::GlyphArrangement glyphs;
		juce::Array<int> tokens;
		bool glyphsAreDirty = true;
		bool tokensAreDirty = true;
		bool hasLineBreak = false;
		
		bool isBookmark();

		Array<Point<int>> positions;

		static int64 createHash(const String& text, int maxCharacters);

		int64 getHash() const;

		Array<Line<float>> getUnderlines(Range<int> columnRange, bool createFirstForEmpty);

		Point<int> getPositionInLine(int col, OutOfBoundsMode mode) const;

		int getLength() const;

		void ensureReadyToPaint(const Font& font);

		bool readyToPaint = false;
		Rectangle<float> characterBounds;
		Array<int> charactersPerLine;

		float height = 0.0f;

		int maxColumns = 0;
	};

	struct Cache
	{
		struct Item
		{
			int64 hash;
			Entry::Ptr p;
		};

		Cache();

		Entry::Ptr getCachedItem(int line, int64 hash) const;

		Array<Item> cachedItems;
	} cache;

	static int getLineLength(const String& s, int maxCharacterIndex=-1);
	static int roundToTab(int c);

	mutable juce::ReferenceCountedArray<Entry> lines;

	Rectangle<float> characterRectangle;

	bool isLineBreakEnabled() const;

	bool containsToken(int lineNumber, int token) const;

private:



	int maxLineWidth = -1;

	friend class TextDocument;
	friend class TextEditor;
	juce::Font font;
	bool cacheGlyphArrangement = true;

	

	void ensureReadyToPaint(Range<int> lineRange);

	void ensureValid(int index) const;
	void invalidate(Range<int> lineRange);


};


}
