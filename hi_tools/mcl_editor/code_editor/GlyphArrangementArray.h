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

	int size() const { return lines.size(); }
	void clear() { lines.clear(); }

	void set(int index, const juce::String& string)
	{
		auto newItem = new Entry(string.removeCharacters("\r\n"), maxLineWidth);
		lines.set(index, newItem);
		ensureValid(index);
	}

	void insert(int index, const String& string)
	{
		auto newItem = new Entry(string.removeCharacters("\r\n"), maxLineWidth);
		lines.insert(index, newItem);
		ensureValid(index);
	}

	void removeRange(Range<int> r)
	{
		lines.removeRange(r.getStart(), r.getLength());
	}

	void removeRange(int startIndex, int numberToRemove) { lines.removeRange(startIndex, numberToRemove); }
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

		Entry() {}
		Entry(const juce::String& string, int maxLineWidth) : string(string), maxColumns(maxLineWidth) {}
		juce::String string;
		juce::GlyphArrangement glyphsWithTrailingSpace;
		juce::GlyphArrangement glyphs;
		juce::Array<int> tokens;
		bool glyphsAreDirty = true;
		bool tokensAreDirty = true;
		bool hasLineBreak = false;
		
		bool isBookmark();

		Array<Point<int>> positions;

		static int64 createHash(const String& text, int maxCharacters)
		{
			return text.hashCode64() + (int64)maxCharacters;
		}

		int64 getHash() const
		{
			return createHash(string, maxColumns);
		}

		Array<Line<float>> getUnderlines(Range<int> columnRange, bool createFirstForEmpty)
		{
			struct LR
			{
				void expandLeft(float v)
				{
					l = jmin(l, v);
				}

				void expandRight(float v)
				{
					r = jmax(r, v);
				}

				Line<float> toLine()
				{
					return  Line<float>(l, y, r, y);
				}

				float l = std::numeric_limits<float>::max();
				float r = 0.0f;
				float y = 0.0f;
				bool used = false;
			};

			Array<Line<float>> lines;

			if (string.isEmpty() && createFirstForEmpty)
			{
				LR empty;
				empty.used = true;
				empty.y = 0.0f;
				empty.l = 0.0f;
				empty.r = characterBounds.getRight() / 2.0f;

				lines.add(empty.toLine());
				return lines;
			}

			if (hasLineBreak)
			{
				Array<LR> lineRanges;
				lineRanges.insertMultiple(0, {}, charactersPerLine.size());

				for (int i = columnRange.getStart(); i < columnRange.getEnd(); i++)
				{
					auto pos = getPositionInLine(i, ReturnLastCharacter);
					auto lineNumber = pos.x;
					auto b = characterBounds.translated(pos.y * characterBounds.getWidth(), pos.x * characterBounds.getHeight());

					if (isPositiveAndBelow(lineNumber, lineRanges.size()))
					{
						auto& l = lineRanges.getReference(lineNumber);

						l.used = true;
						l.y = b.getY();
						l.expandLeft(b.getX());
						l.expandRight(b.getRight());
					}
				}

				for (auto& lr : lineRanges)
				{
					if (lr.used)
						lines.add(lr.toLine());
				}

				return lines;
			}
			else
			{
				auto s = (float)getLineLength(string, columnRange.getStart());
				auto e = (float)getLineLength(string, columnRange.getEnd());

				auto w = characterBounds.getWidth();

				Line<float> l(s * w, 0.0f, e * w, 0.0f);
				lines.add(l);

				return lines;
			}
		}

		Point<int> getPositionInLine(int col, OutOfBoundsMode mode) const
		{
			if (!hasLineBreak)
			{
				return { 0, getLineLength(string, col) };
			}

			if (isPositiveAndBelow(col, positions.size()))
				return positions[col];

			if (mode == AssertFalse)
			{
				jassertfalse;
				return {};
			}

			int l = 0;

			if (mode == ReturnLastCharacter)
			{
				if (charactersPerLine.isEmpty())
				{
					return { 0, 0 };
				}

				auto l = (int)charactersPerLine.size() - 1;
				auto c = jmax(0, charactersPerLine[l] - 1);

				return { l, c };
			}

			if (mode == ReturnNextLine)
			{
				auto l = (int)charactersPerLine.size();
				auto c = 0;

				return { l, c };
			}

			if (mode == ReturnBeyondLastCharacter)
			{
				if (charactersPerLine.isEmpty())
				{
					return { 0, 0 };
				}

				auto l = (int)charactersPerLine.size() - 1;
				auto c = charactersPerLine[l];

				auto stringLength = string.length();

				auto isTab = !string.isEmpty() && isPositiveAndBelow(col-1, stringLength) && string[jlimit(0, stringLength, col - 1)] == '\t';

				if (isTab)
					return { l, roundToTab(c) };

				return { l, c };
			}

			jassertfalse;

			if (col >= string.length())
			{
				l = charactersPerLine.size() - 1;

				if (l < 0)
					return { 0, 0 };

				col = charactersPerLine[l];
				return { l, col };
			}

			for (int i = 0; i < charactersPerLine.size(); i++)
			{
				if (col >= charactersPerLine[i])
				{
					col -= charactersPerLine[i];
					l++;
				}
				else
					break;
			}



			return { l, col };
		}

		int getLength() const
		{
			return string.length() + 1;
		}

		void ensureReadyToPaint(const Font& font)
		{
			if (!readyToPaint)
			{
				glyphs.addLineOfText(font, string, 0.f, 0.f);
				glyphsWithTrailingSpace.addLineOfText(font, string, 0.f, 0.f);
				readyToPaint = true;
			}
		}

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

		Cache()
		{

		}

		Entry::Ptr getCachedItem(int line, int64 hash) const
		{
			if (isPositiveAndBelow(line, cachedItems.size()))
			{
				Range<int> rangeToCheck(jmax(0, line - 4), jmin(cachedItems.size(), line + 4));

				for (int i = rangeToCheck.getStart(); i < rangeToCheck.getEnd(); i++)
				{
					auto l = cachedItems.begin() + i;
					if (l->hash == hash)
						return l->p;
				}
			}

			return nullptr;
		}

		Array<Item> cachedItems;
	} cache;

	static int getLineLength(const String& s, int maxCharacterIndex=-1);
	static int roundToTab(int c);

	mutable juce::ReferenceCountedArray<Entry> lines;

	Rectangle<float> characterRectangle;

	bool isLineBreakEnabled() const { return maxLineWidth != -1; }

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
