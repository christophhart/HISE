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
