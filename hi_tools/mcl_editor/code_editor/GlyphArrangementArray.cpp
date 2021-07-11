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
		entry->tokens.setUnchecked(col, zone.token);
	}

	entry->tokensAreDirty = false;
}

GlyphArrangement mcl::GlyphArrangementArray::getGlyphs(int index,
	float baseline,
	int token,
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



	if (DEBUG_TOKENS)
	{
		String line;
		String hex("0123456789abcdefg");

		for (auto token : entry->tokens)
			line << hex[token % 16];

		if (withTrailingSpace)
			line << " ";

		glyphSource.clear();
		glyphSource.addLineOfText(font, line, 0.f, 0.f);
	}



	for (int n = 0; n < glyphSource.getNumGlyphs(); ++n)
	{
		if (token == -1 || entry->tokens.getUnchecked(n) == token)
		{
			auto glyph = glyphSource.getGlyph(n);



			glyph.moveBy(TEXT_INDENT, baseline);

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



		if (maxLineWidth != -1)
		{
			entry->glyphs.addJustifiedText(font, toDraw, 0.f, 0.f, maxLineWidth, Justification::centredLeft);
			entry->glyphsWithTrailingSpace.addJustifiedText(font, toDraw + " ", 0.f, 0.f, maxLineWidth, Justification::centredLeft);
		}
		else
		{
			entry->glyphs.addLineOfText(font, toDraw, 0.f, 0.f);
			entry->glyphsWithTrailingSpace.addLineOfText(font, toDraw, 0.f, 0.f);
		}



		entry->positions.clearQuick();
		entry->positions.ensureStorageAllocated(entry->string.length());
		entry->characterBounds = characterRectangle;
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

		entry->charactersPerLine.clear();

		int index = 0;

		for (const auto& p : entry->positions)
		{
			auto l = p.x;
			auto characterIsTab = entry->string[index++] == '\t';
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



}