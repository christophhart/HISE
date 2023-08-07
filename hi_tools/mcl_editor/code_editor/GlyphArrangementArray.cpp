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

namespace mcl
{
using namespace juce;


int GlyphArrangementArray::size() const
{ return lines.size(); }

void GlyphArrangementArray::clear()
{ lines.clear(); }

void GlyphArrangementArray::set(int index, const juce::String& string)
{
	auto newItem = new Entry(string.removeCharacters("\r\n"), maxLineWidth);
	lines.set(index, newItem);
	ensureValid(index);
}

void GlyphArrangementArray::insert(int index, const String& string)
{
	auto newItem = new Entry(string.removeCharacters("\r\n"), maxLineWidth);
	lines.insert(index, newItem);
	ensureValid(index);
}

void GlyphArrangementArray::removeRange(Range<int> r)
{
	lines.removeRange(r.getStart(), r.getLength());
}

void GlyphArrangementArray::removeRange(int startIndex, int numberToRemove)
{ lines.removeRange(startIndex, numberToRemove); }

GlyphArrangementArray::Entry::Entry()
{}

GlyphArrangementArray::Entry::Entry(const juce::String& string, int maxLineWidth): string(string), maxColumns(maxLineWidth)
{}

int64 GlyphArrangementArray::Entry::createHash(const String& text, int maxCharacters)
{
	return text.hashCode64() + (int64)maxCharacters;
}

int64 GlyphArrangementArray::Entry::getHash() const
{
	return createHash(string, maxColumns);
}

Array<Line<float>> GlyphArrangementArray::Entry::getUnderlines(Range<int> columnRange, bool createFirstForEmpty)
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

Point<int> GlyphArrangementArray::Entry::getPositionInLine(int col, OutOfBoundsMode mode) const
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

int GlyphArrangementArray::Entry::getLength() const
{
	return string.length() + 1;
}

void GlyphArrangementArray::Entry::ensureReadyToPaint(const Font& font)
{
	if (!readyToPaint)
	{
		glyphs.addLineOfText(font, string, 0.f, 0.f);
		glyphsWithTrailingSpace.addLineOfText(font, string, 0.f, 0.f);
		readyToPaint = true;
	}
}

GlyphArrangementArray::Cache::Cache()
{

}

GlyphArrangementArray::Entry::Ptr GlyphArrangementArray::Cache::getCachedItem(int line, int64 hash) const
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

bool GlyphArrangementArray::isLineBreakEnabled() const
{ return maxLineWidth != -1; }

//==============================================================================
const String& mcl::GlyphArrangementArray::operator[] (int index) const
{
	if (isPositiveAndBelow(index, lines.size()))
	{
		return lines[index]->string;
	}

	static String empty;
	return empty;
}

int mcl::GlyphArrangementArray::getToken(int row, int col, int defaultIfOutOfBounds) const
{
	if (!isPositiveAndBelow(row, lines.size()))
	{
		return defaultIfOutOfBounds;
	}
	return lines[row]->tokens[col];
}

void mcl::GlyphArrangementArray::clearTokens(int index)
{
	if (!isPositiveAndBelow(index, lines.size()))
		return;

	auto entry = lines[index];

	ensureValid(index);

	for (int col = 0; col < entry->tokens.size(); ++col)
	{
		entry->tokens.setUnchecked(col, 0);
	}
}

void mcl::GlyphArrangementArray::applyTokens(int index, Selection zone)
{
	if (!isPositiveAndBelow(index, lines.size()))
		return;

	auto entry = lines[index];
	auto range = zone.getColumnRangeOnRow(index, entry->tokens.size());

	ensureValid(index);

	for (int col = range.getStart(); col < range.getEnd(); ++col)
	{
		if(isPositiveAndBelow(col, entry->tokens.size()))
			entry->tokens.setUnchecked(col, zone.token);
	}

	entry->tokensAreDirty = false;
}

GlyphArrangement mcl::GlyphArrangementArray::getGlyphs(int index,
	float baseline,
	int token,
	Range<float> visibleRange,
	bool withTrailingSpace) const
{
	if (!isPositiveAndBelow(index, lines.size()))
	{
		GlyphArrangement glyphs;

		if (withTrailingSpace)
		{
			glyphs.addLineOfText(font, " ", TEXT_INDENT, baseline);
		}
		return glyphs;
	}
	ensureValid(index);

	auto entry = lines[index];
	auto glyphSource = withTrailingSpace ? entry->glyphsWithTrailingSpace : entry->glyphs;
	auto glyphs = GlyphArrangement();

	for (int n = 0; n < glyphSource.getNumGlyphs(); ++n)
	{
		if (token == -1 || entry->tokens[n] == token)
		{
			auto glyph = glyphSource.getGlyph(n);



			glyph.moveBy(TEXT_INDENT, baseline);

			if(visibleRange.isEmpty() || visibleRange.expanded(glyph.getBounds().getWidth()).contains(glyph.getRight()))
				glyphs.addGlyph(glyph);
		}
	}

	return glyphs;
}

void mcl::GlyphArrangementArray::ensureValid(int index) const
{
	if (!isPositiveAndBelow(index, lines.size()))
		return;

	auto entry = lines[index];

	if (entry->glyphsAreDirty)
	{
		//entry.string = Helpers::replaceTabsWithSpaces(entry.string, 4);

		auto toDraw = entry->string;// ;

		entry->tokens.resize(toDraw.length());
		entry->glyphs.clear();
		entry->glyphsWithTrailingSpace.clear();
		entry->charactersPerLine.clearQuick();

		auto numCols = roundToInt((float)maxLineWidth / characterRectangle.getWidth());

		auto lineLength = getLineLength(toDraw);

		entry->hasLineBreak = maxLineWidth != -1 && lineLength > numCols;
		entry->characterBounds = characterRectangle;

		if (entry->hasLineBreak)
		{
			entry->glyphs.addJustifiedText(font, toDraw, 0.f, 0.f, maxLineWidth, Justification::centredLeft);
			entry->glyphsWithTrailingSpace.addJustifiedText(font, toDraw + " ", 0.f, 0.f, maxLineWidth, Justification::centredLeft);

			entry->positions.clearQuick();
			entry->positions.ensureStorageAllocated(entry->string.length());
			entry->readyToPaint = true;

			auto n = entry->glyphs.getNumGlyphs();
			auto first = entry->glyphsWithTrailingSpace.getBoundingBox(0, 1, true);

			for (int i = 0; i < n; i++)
			{
				auto box = entry->glyphs.getBoundingBox(i, 1, true);
				box = box.translated(-first.getX(), -first.getY());

				float x = box.getY() / characterRectangle.getHeight();
				float y = box.getX() / characterRectangle.getWidth();

				entry->positions.add({ roundToInt(x), roundToInt(y) });
			}

			for (const auto& p : entry->positions)
			{
				auto l = p.x;
				auto c = p.y + 1;

				if (isPositiveAndBelow(l, entry->charactersPerLine.size()))
				{
					auto& thisC = entry->charactersPerLine.getReference(l);
					thisC = jmax(thisC, c);
				}
				else
				{
					entry->charactersPerLine.set(l, c);
				}
			}

			if (entry->charactersPerLine.isEmpty())
				entry->charactersPerLine.add(0);
		}
		else
		{
			entry->charactersPerLine.set(0, lineLength);
			entry->readyToPaint = false;
			//entry->ensureReadyToPaint(font);
		}

		entry->glyphsAreDirty = !cacheGlyphArrangement;
		entry->height = font.getHeight() * (float)entry->charactersPerLine.size();
	}
}


void mcl::GlyphArrangementArray::invalidate(Range<int> lineRange)
{
	if (lineRange.isEmpty())
	{
		lineRange = { 0, lines.size() };
	}

	for (int i = lineRange.getStart(); i < lineRange.getEnd() + 1; i++)
	{
		if (isPositiveAndBelow(i, lines.size()))
		{
			lines[i]->tokensAreDirty = true;
			lines[i]->glyphsAreDirty = true;
		}
	}

	for (int i = 0; i < lines.size(); i++)
		ensureValid(i);
}



int GlyphArrangementArray::getLineLength(const String& s, int maxCharacterIndex)
{
	int l = 0;
	int characterIndex = 0;

	for (auto c : s)
	{
		if (maxCharacterIndex != -1 && characterIndex++ >= maxCharacterIndex)
			return l;

		static constexpr int TabSize = 4;

		if (c == '\t')
			l += TabSize - (l % TabSize);
		else
			l++;
	}

	return l;
}

int GlyphArrangementArray::roundToTab(int c)
{
	static constexpr int TabSize = 4;

	if (c % TabSize == 0)
		return c;

	c -= (c % TabSize);
	c += TabSize;
	return c;
}

bool GlyphArrangementArray::containsToken(int lineNumber, int token) const
{
	if (auto l = lines[lineNumber])
	{
		for (const auto& n : l->tokens)
		{
			if (n == token)
				return true;
		}
	}
	
	return false;
}

void GlyphArrangementArray::ensureReadyToPaint(Range<int> lineRange)
{
	for (int i = lineRange.getStart(); i < lineRange.getEnd(); i++)
	{
		lines[i]->ensureReadyToPaint(font);
	}
}

bool GlyphArrangementArray::Entry::isBookmark()
{
	auto s = string.begin();
	auto e = string.end();

	while (s != e && CharacterFunctions::isWhitespace(*s))
		s++;

	if ((e - s> 3) && 
		*s == '/' && 
		*(++s) == '/' && 
		*(++s) == '!')
		return true;

	return false;
}

}
