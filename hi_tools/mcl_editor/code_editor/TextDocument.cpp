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
void mcl::TextDocument::replaceAll(const String& content)
{
	lines.clear();

	int index = 0;

	for (const auto& line : StringArray::fromLines(content))
	{
		lines.set(index++, line);
	}
}

int mcl::TextDocument::getNumRows() const
{
	return doc.getNumLines();
}

int mcl::TextDocument::getNumColumns(int row) const
{
	return lines[row].length();
}

float mcl::TextDocument::getVerticalPosition(int row, Metric metric) const
{
	row = jmin(row, lines.size());
	float pos = rowPositions[jmin(rowPositions.size()-1, row)];


	float gap = font.getHeight() * (lineSpacing - 1.f) * 0.5f;


	float lineHeight = getCharacterRectangle().getHeight() + gap;

	if (isPositiveAndBelow(row, lines.size()))
		lineHeight = lines.lines[row]->height + gap;

	switch (metric)
	{
	case Metric::top: return pos;
	case Metric::ascent: return pos + gap;
	case Metric::baseline: return pos + gap + font.getAscent();
	case Metric::bottom: return pos + lineHeight;
	default:
		jassertfalse;
		return 0.0f;
	}

}

Point<float> mcl::TextDocument::getPosition(Point<int> index, Metric metric) const
{
	return Point<float>(getGlyphBounds(index, GlyphArrangementArray::ReturnBeyondLastCharacter).getX(), getVerticalPosition(index.x, metric));
}

RectangleList<float> mcl::TextDocument::getSelectionRegion(Selection selection, Rectangle<float> clip) const
{
	RectangleList<float> patches;
	Selection s = selection.oriented();

	auto m = GlyphArrangementArray::ReturnBeyondLastCharacter;

	if (s.head.x == s.tail.x)
	{
		int c0 = s.head.y;
		int c1 = s.tail.y;

		auto b = getBoundsOnRow(s.head.x, Range<int>(c0, c1), m);

		patches.add(b);

	}
	else
	{
		int r0 = s.head.x;
		int c0 = s.head.y;
		int r1 = s.tail.x;
		int c1 = s.tail.y;

		for (int n = r0; n <= r1; ++n)
		{
			if (!clip.isEmpty() &&
				!clip.getVerticalRange().intersects(
					{
						getVerticalPosition(n, Metric::top),
						getVerticalPosition(n, Metric::bottom)
					})) continue;

			if (n == r1 && c1 == 0) continue;
			else if (n == r0) patches.add(getBoundsOnRow(r0, Range<int>(c0, getNumColumns(r0) + 1), m));
			else if (n == r1) patches.add(getBoundsOnRow(r1, Range<int>(0, c1), m));
			else              patches.add(getBoundsOnRow(n, Range<int>(0, getNumColumns(n) + 1), m));
		}
	}
	return patches;
}

Rectangle<float> mcl::TextDocument::getBounds() const
{
	if (cachedBounds.isEmpty())
	{
		int maxX = 0;

		for (auto l : lines.lines)
		{
			for (int i = 0; i < l->charactersPerLine.size(); i++)
			{
				maxX = jmax(maxX, l->charactersPerLine[i]);
			}

		}

		auto bottom = getVerticalPosition(lines.size() - 1, Metric::bottom);
		auto right = maxX * getCharacterRectangle().getWidth() + TEXT_INDENT;

		Rectangle<float> newBounds(0.0f, 0.0f, (float)right, bottom);

#if 0
		RectangleList<float> bounds;

		for (int n = 0; n < getNumRows(); ++n)
		{
			auto b = getBoundsOnRow(n, Range<int>(0, getNumColumns(n)), GlyphArrangementArray::ReturnBeyondLastCharacter);

			if (b.isEmpty())
				bounds.add(getCharacterRectangle().translated(0.0f, getVerticalPosition(n, Metric::top)));
			else
				bounds.add(b);
		}

		auto newBounds = bounds.getBounds();



		newBounds = newBounds.withHeight(newBounds.getHeight() + gap / 2.0f);
#endif

		return cachedBounds = newBounds;
	}
	return cachedBounds;
}

RectangleList<float> mcl::TextDocument::getBoundsOnRow(int row, Range<int> columns, GlyphArrangementArray::OutOfBoundsMode m) const
{
	RectangleList<float> b;

	float yPos = getVerticalPosition(row, Metric::top);
	float xPos = TEXT_INDENT;

	if (isPositiveAndBelow(row, getNumRows()))
	{
		columns.setStart(jmax(columns.getStart(), 0));
		auto l = lines.lines[row];

		auto boundsToUse = l->characterBounds;

		if (boundsToUse.isEmpty())
			boundsToUse = { 0.0f, 0.0f, font.getStringWidthFloat(" "), font.getHeight() };

		if (l->charactersPerLine.size() == 1)
		{
			auto startCol = (float)GlyphArrangementArray::getLineLength(l->string, columns.getStart());
			auto endCol = (float)GlyphArrangementArray::getLineLength(l->string, columns.getEnd());
			
			auto cw = boundsToUse.getWidth();

			Rectangle<float> a(xPos + startCol * cw, yPos, (endCol - startCol) * cw, getRowHeight());

			if (a.getWidth() == 0.0f)
				a.setWidth(boundsToUse.getWidth());

			b.add(a);
			return b;
		}
		else
		{
			if (columns == Range<int>(0, getNumColumns(row)) && m == GlyphArrangementArray::ReturnBeyondLastCharacter)
			{
				auto rowHeight = getRowHeight();

				for (auto& numCol : l->charactersPerLine)
				{
					Rectangle<float> l(xPos, yPos, numCol * boundsToUse.getWidth(), rowHeight);
					yPos += rowHeight;
					b.add(l);
				}

				return b;
			}

			float gap = lineSpacing * font.getHeight() - font.getHeight();

			auto length = l->string.length();

			for (int i = columns.getStart(); i < columns.getEnd(); i++)
			{
				auto p = l->getPositionInLine(i, m);
				auto cBound = boundsToUse.translated(xPos + p.y * boundsToUse.getWidth(), yPos + p.x * boundsToUse.getHeight());

				if (p.x == l->charactersPerLine.size() - 1)
					cBound = cBound.withHeight(cBound.getHeight() + gap);

				bool isTab = isPositiveAndBelow(i, length) && l->string[i] == '\t';

				if (isTab)
				{
					int tabLength = 4 - p.y % 4;

					cBound.setWidth((float)tabLength * boundsToUse.getWidth());
				}

				b.add(cBound);
			}

			b.consolidate();
		}
	}
	else
	{
		float yPos = getVerticalPosition(row, Metric::top);
		float xPos = TEXT_INDENT;

		b.add(Rectangle<float>(xPos, yPos, getCharacterRectangle().getWidth(), getRowHeight()));
	}

	return b;
}

Rectangle<float> mcl::TextDocument::getGlyphBounds(Point<int> index, GlyphArrangementArray::OutOfBoundsMode m) const
{
#if 0

	auto topOfRow = getVerticalPosition(index.x, Metric::top);

	auto firstBounds = lines.lines[index.x].characterBounds;

	auto b = getGlyphsForRow(index.x, -1, true).getBoundingBox(index.y, 1, true);

	if (index.y == getNumColumns(index.x))
	{
		b = getGlyphsForRow(index.x, -1, true).getBoundingBox(jmax(0, index.y - 1), 1, true);
		b = b.translated(b.getWidth(), 0.0f);
	}

	if (getNumColumns(index.x) == 0)
	{
		b = { (float)TEXT_INDENT, topOfRow, font.getStringWidthFloat(" "), font.getHeight() * lineSpacing };
	}

	//auto b = lines.lines[index.x].glyphs.getBoundingBox(index.y-1, 1, true);

	//b = b.translated(0, topOfRow);

	b = b.withSizeKeepingCentre(b.getWidth(), font.getHeight() * lineSpacing);

	return b;
#endif

	index.x = jmax(0, jmin(lines.size() - 1, index.x));

	index.y = jlimit(0, getNumColumns(index.x), index.y);


	auto numColumns = getNumColumns(index.x);

	auto first = jlimit(0, numColumns, index.y);


	return getBoundsOnRow(index.x, Range<int>(first, first + 1), m).getRectangle(0);
}

GlyphArrangement mcl::TextDocument::getGlyphsForRow(int row, Range<float> visibleRange, int token, bool withTrailingSpace) const
{
	lines.lines[row]->ensureReadyToPaint(lines.font);

	return lines.getGlyphs(row,
		getVerticalPosition(row, Metric::baseline),
		token,
		visibleRange,
		withTrailingSpace);
}

GlyphArrangement mcl::TextDocument::findGlyphsIntersecting(Rectangle<float> area, int token) const
{
	auto range = getRangeOfRowsIntersecting(area);
	auto rows = Array<RowData>();
	auto glyphs = GlyphArrangement();

	Range<float> visibleRange;

	if (!lines.isLineBreakEnabled())
	{
		visibleRange = { area.getX(), area.getRight() };
	}

	for (int n = range.getStart(); n < range.getEnd(); ++n)
	{
		if (!foldManager.isFolded(n))
		{
			if (token != -1 && !lines.containsToken(n, token))
				continue;

			auto l = getGlyphsForRow(n, visibleRange, token);

			glyphs.addGlyphArrangement(std::move(l));
		}

	}

	return glyphs;
}

juce::Range<int> mcl::TextDocument::getRangeOfRowsIntersecting(juce::Rectangle<float> area) const
{
	if (rowPositions.isEmpty())
		return { 0, 1 };

    auto topY = jmax<int>(0, area.getY());
    auto bottomY = area.getBottom();
    
	int topIndex = 0;

	for (const auto& p : rowPositions)
	{
		if (p >= topY)
			break;

		topIndex++;
	}

	auto numRows = rowPositions.size();

	int bottomIndex = numRows;

	while (--bottomIndex >= topIndex)
	{
		if (rowPositions[bottomIndex] < bottomY)
			break;

		
	}

	Range<int> range(topIndex, bottomIndex);

	range = range.expanded(1);
	range.setStart(jmax(0, range.getStart()));
	range.setEnd(jmin(range.getEnd(), getNumRows()));

	return range;


#if 0
    int topIndex = -1;
    int bottomIndex = -1;
    
	Range<float> yRange = { area.getY() - getRowHeight(), area.getBottom() + getRowHeight() };

	int min = getNumRows()-1;
	int max = 0;

	for (int i = 0; i < rowPositions.size(); i++)
	{
        auto pos = rowPositions[i];
        
        
        
        if(topIndex == -1 && pos >= topY)
            topIndex = jmax(0, i - 1);
        
        if(bottomIndex == -1 && pos >= bottomY)
        {
            bottomIndex = jmax(0, i - 1);
            break;
        }
            
        
		if (yRange.contains(rowPositions[i]))
		{
			min = jmin(min, i);
			max = jmax(max, i);
		}
	}
    
    if(bottomIndex == -1)
        bottomIndex = rowPositions.size()-2;
    
    return { topIndex, bottomIndex + 1 };

	return { min, max + 1 };
#endif
}

Array<mcl::TextDocument::RowData> mcl::TextDocument::findRowsIntersecting(Rectangle<float> area,
	bool computeHorizontalExtent) const
{
	auto range = getRangeOfRowsIntersecting(area);
	auto rows = Array<RowData>();

	for (int n = range.getStart(); n < range.getEnd(); ++n)
	{
		RowData data;
		data.rowNumber = n;

		data.bounds = getBoundsOnRow(n, Range<int>(0, getNumColumns(n)), GlyphArrangementArray::ReturnBeyondLastCharacter);

		if (data.bounds.isEmpty())
		{
			data.bounds.add(0.0f, getVerticalPosition(n, Metric::top), 1.0f, font.getHeight() * lineSpacing);
		}

		for (const auto& s : selections)
		{
			if (s.intersectsRow(n))
			{
				data.isRowSelected = true;
				break;
			}
		}
		rows.add(data);
	}
	return rows;
}

Point<int> mcl::TextDocument::findIndexNearestPosition(Point<float> position) const
{
	position = position.translated(getCharacterRectangle().getWidth() * 0.5f, 0.0f);

	if (position.getX() < TEXT_INDENT)
		position.setX(TEXT_INDENT);

	auto gap = font.getHeight() * lineSpacing - font.getHeight();
	float yPos = gap / 2.0f;

	if (position.y > rowPositions.getLast())
	{
		auto x = lines.size() - 1;
		if (x >= 0)
			return { x, getNumColumns(x) };
	}
		

	for (int l = 0; l < getNumRows(); l++)
	{
		auto line = lines.lines[l];

		if (foldManager.isFolded(l))
			continue;

		Range<float> p(yPos - gap / 2.0f, yPos + line->height + gap / 2.0f);

		if (p.contains(position.y))
		{
			auto glyphs = getGlyphsForRow(l, {}, -1, true);

			int numGlyphs = glyphs.getNumGlyphs();
			auto col = numGlyphs;

			for (int n = 0; n < numGlyphs; ++n)
			{
				auto b = glyphs.getBoundingBox(n, 1, true).expanded(0.0f, gap / 2.f);

				if (b.contains(position))
				{
					col = n;
					break;
				}
			}

			return { l, col };
		}

		yPos = p.getEnd();
	}

	return { 0, 0 };
}

Point<int> mcl::TextDocument::getEnd() const
{
	return { getNumRows(), 0 };
}

juce::Array<Bookmark> TextDocument::getBookmarks() const
{
	int index = 0;
	Array<Bookmark> bookmarks;

	for (auto l : lines.lines)
	{
		if (l->isBookmark())
		{
			Bookmark b;
			b.lineNumber = index;
			b.name = l->string.fromFirstOccurrenceOf("//!", false, false).trim();
			bookmarks.add(b);
		}

		index++;
	}

	return bookmarks;
}

void TextDocument::setSelections(const juce::Array<Selection>& newSelections, bool useUndo)
{
	columnTryingToMaintain = -1;

	if (useUndo)
	{
		viewUndoManagerToUse->perform(new SelectionAction(*this, newSelections));
	}
	else
	{
		selections = newSelections;
		sendSelectionChangeMessage();
	}
}

void TextDocument::drawWhitespaceRectangles(int row, Graphics& g)
{
	if (getFoldableLineRangeHolder().isFolded(row))
		return;

	g.setColour(Colours::white.withAlpha(0.2f));

	if (auto l = lines.lines[row])
	{
		const auto& s = l->string;
		auto numChars = s.length();

		// let's skip this for big lines
		if (numChars > 400)
			return;

		for (int i = 0; i < numChars; i++)
		{
            if (CharacterFunctions::isWhitespace(s[i]))
			{
                Point<int> pos(row, i);
                bool found = false;
                
                for(auto& s: selections)
                {
                    if(s.contains(pos))
                    {
                        found = true;
                        break;
                    }
                }
                
                if(!found)
                    continue;
                
				auto r = getBoundsOnRow(row, { i, i + 1 }, GlyphArrangementArray::ReturnBeyondLastCharacter).getRectangle(0);
				
				bool isSpace = s[i] == ' ';

				if (isSpace)
					g.fillRect(r.withSizeKeepingCentre(2.0f, 2.0f));
				else
					g.fillRect(r.withSizeKeepingCentre(r.getWidth() - 2.0f, 1.0f));
			}
		}
	}
}

bool TextDocument::navigateLeftRight(juce::Point<int>& index, bool right) const
{
	columnTryingToMaintain = -1;

	if (right)
	{
		if (index.y < getNumColumns(index.x))
		{
			index.y += 1;
			return true;
		}
		else if (index.x < getNumRows())
		{
			index.x += 1;
			index.y = 0;
			return true;
		}
		return false;
	}
	else
	{
		if (index.y > 0)
		{
			index.y -= 1;
			return true;
		}
		else if (index.x > 0)
		{
			index.x -= 1;
			index.y = getNumColumns(index.x);
			return true;
		}
		return false;
	}
}

bool TextDocument::navigateUpDown(juce::Point<int>& index, bool down) const
{
	auto canMove = (down && index.x < getNumRows() - 1) ||
		(!down && index.x > 0);

	auto isLineBreak = getNumLinesForRow(index.x) > 1;

	if (canMove || isLineBreak)
	{
		if (isLineBreak)
		{
			auto b = getGlyphBounds(index, GlyphArrangementArray::ReturnBeyondLastCharacter);

			b = b.translated(0.0f, getRowHeight() * (down ? 1.0f : -1.0f));

			auto newIndex = findIndexNearestPosition(Point<float>(b.getX(), down ? b.getY() : b.getBottom()));
			auto sameLine = newIndex.x == index.x;

			// Sometimes the method will not find the correct index (eg. because a long word that
			// is wrapped), so in this case we just jump to the next line...
			auto isLastCharacter = newIndex.y >= getNumColumns(newIndex.x);

			if (sameLine && !isLastCharacter)
			{
				index = newIndex;
				return true;
			}
		}

		if (columnTryingToMaintain != -1)
			index.y = columnTryingToMaintain;
		else
			columnTryingToMaintain = getColumnIndexAccountingTabs(index);

		

		auto upIntoLineBreak = !down && getNumLinesForRow(index.x - 1) > 1;

		if (upIntoLineBreak)
		{
			auto b = getGlyphBounds(index, GlyphArrangementArray::ReturnBeyondLastCharacter);
			b = b.withX(TEXT_INDENT + (float)columnTryingToMaintain * getCharacterRectangle().getWidth());

			b = b.translated(0.0f, -1.0f * getRowHeight());

			index = findIndexNearestPosition(b.getBottomLeft());
			return true;
		}
		else
		{
			index.x += down ? 1 : -1;
			index.x = jlimit(0, getNumRows()-1, index.x);
			index.y = jmin(index.y, getNumColumns(index.x));

			applyTabsToPosition(index, columnTryingToMaintain);
		}

		return true;
	}

	return false;
}


int TextDocument::getColumnIndexAccountingTabs(juce::Point<int>& index) const
{
	auto isLineBreak = getNumLinesForRow(index.x) > 1;
	
	if (isLineBreak)
	{
		auto b = getBoundsOnRow(index.x, { index.y, index.y + 1 }, GlyphArrangementArray::ReturnBeyondLastCharacter).getRectangle(0);
		return roundToInt((b.getX() - TEXT_INDENT) / getCharacterRectangle().getWidth());
	}

	auto line = doc.getLine(index.x);
	auto t = line.getCharPointer();
	int col = 0;

	for (int i = 0; i < index.y; ++i)
	{
		if (t.isEmpty())
		{
			col = i;
			break;
		}

		static constexpr int TabSize = 4;

		if (t.getAndAdvance() != '\t')
			++col;
		else
			col += TabSize - (col % TabSize);
	}

	return col;
}

void TextDocument::applyTabsToPosition(juce::Point<int>& index, int positionToMaintain) const
{
	auto l = doc.getLine(index.x);

	int realCol = 0;

	for (int i = 0; i < l.length(); i++)
	{
		static constexpr int TabSize = 4;

		if (realCol >= positionToMaintain)
		{
			index.y = i;
			break;
		}

		if (l[i] == '\t')
			realCol += TabSize - (realCol % TabSize);
		else
			realCol++;
	}
}

void mcl::TextDocument::navigate(juce::Point<int>& i, Target target, Direction direction) const
{
	std::function<bool(Point<int>&)> advance;
	std::function<juce_wchar(Point<int>&)> get;

	using CF = CharacterFunctions;
	static String punctuation = "{}<>()[],.;:";

	switch (direction)
	{
	case Direction::forwardRow:
		advance = [this](Point<int>& i) 
		{
            auto lastX = i.x;
			auto ok = navigateUpDown(i, true); 

            if(i.x == lastX)
                return false;
            
			while (foldManager.isFolded(i.x))
				ok = navigateUpDown(i, true);

			return ok;
		};
		get = [this](Point<int> i) { return getCharacter(i); };
		break;
	case Direction::backwardRow:
		advance = [this](Point<int>& i) 
		{ 
			auto ok = navigateUpDown(i, false);

			while (foldManager.isFolded(i.x))
				ok = navigateUpDown(i, false);

			return ok;
		};
		get = [this](Point<int> i) { navigateLeftRight(i, false); return getCharacter(i); };
		break;
	case Direction::forwardCol:
		advance = [this](Point<int>& i) { return navigateLeftRight(i, true); };
		get = [this](Point<int> i) { return getCharacter(i); };
		break;
	case Direction::backwardCol:
		advance = [this](Point<int>& i) { return navigateLeftRight(i, false); };
		get = [this](Point<int> i) { navigateLeftRight(i, false); return getCharacter(i); };
		break;
	}

	switch (target)
	{
	case Target::whitespace: while (!CF::isWhitespace(get(i)) && advance(i)) {} break;
	case Target::punctuation: while (!punctuation.containsChar(get(i)) && advance(i)) {} break;

	case Target::character: advance(i); break;
	case Target::commandTokenNav:
	{
		if (getCharacter(i.translated(0, -1)) == ';' && direction == Direction::backwardCol)
			advance(i);

		auto shouldSkipBracket = false;

		auto startX = i.x;

		do
		{
			if (direction == TextDocument::Direction::backwardCol)
				shouldSkipBracket = String(")]}\"").containsChar(getCharacter(i.translated(0, -1)));
			else
				shouldSkipBracket = String("([{\"").containsChar(getCharacter(i));

			if (shouldSkipBracket)
			{
				if (!advance(i))
					break;
			}

			if (i.x != startX)
				return;
		} 
		while (shouldSkipBracket);

		advance(i);

		while (CF::isWhitespace(getCharacter(i)))
		{
			if (i.x != startX)
				break;

			if (!advance(i))
				break;
		}

		bool didSomething = false;

		while (CharacterFunctions::isLetterOrDigit(getCharacter(i)))
		{
			if (i.x != startX)
				break;

			didSomething = true;
            
			if(!advance(i))
                break;
		}
			

		if (direction == TextDocument::Direction::backwardCol)
		{
			while (CharacterFunctions::isWhitespace(getCharacter(i)))
			{
				if (i.x != startX)
					break;

				if (!navigateLeftRight(i, true))
					break;
			}
				

			if (didSomething && !CharacterFunctions::isLetterOrDigit(getCharacter(i)))
			{
				if (!navigateLeftRight(i, true))
					break;
			}
		}

		break;
	}
    case Target::firstnonwhitespaceAfterLineBreak:
    {
        auto isLineBreak = getNumLinesForRow(i.x) > 1;

        if (isLineBreak)
        {
            auto y = getGlyphBounds(i, GlyphArrangementArray::ReturnLastCharacter).getY();
            
            while (navigateLeftRight(i, false))
            {
                auto thisY = getGlyphBounds(i, GlyphArrangementArray::ReturnLastCharacter).getY();

                if (y != thisY)
                {
                    navigateLeftRight(i, true);
                    break;
                }
            }
            
            break;
        }
    }
	case Target::firstnonwhitespace:
	{
        if(direction == Direction::forwardCol)
        {
            while(CF::isWhitespace(get(i)) && navigateLeftRight(i, true))
                ;
                
            return;
        }
        
        if (i.y != 0 && get(i) == '\n' && direction == Direction::backwardCol)
        {
            navigateLeftRight(i, false);
        }

        jassert(direction == Direction::backwardCol);

        bool skipTofirstNonWhiteCharacter = false;

        while (get(i) != '\n' && navigateLeftRight(i, false))
            skipTofirstNonWhiteCharacter |= !CF::isWhitespace(get(i));

        while (skipTofirstNonWhiteCharacter && CF::isWhitespace(get(i)))
            navigateLeftRight(i, true);

        if (skipTofirstNonWhiteCharacter)
            navigateLeftRight(i, false);

        break;
	}
	case Target::subword: while ((CF::isLetterOrDigit(get(i)) || get(i) == '_') && advance(i)) {} break;
	case Target::subwordWithPoint: while ((CF::isLetterOrDigit(get(i)) || get(i) == '_' || get(i) == '.') && advance(i)) {} break;
	case Target::word: while (CF::isWhitespace(get(i)) && advance(i)) {} break;
	case Target::cppToken:
	{
		String breakChar = "+-*/%=?\t;\n}{";

		enum RangeCharType
		{
			Paren,
			Brace,
			Bracket
		};

		bool ok = true;

		while (ok)
		{
			auto c = get(i);

			auto skipBetween = [&](juce_wchar o, juce_wchar c)
			{
				while (advance(i))
				{
					auto c = get(i);
					if (c == '\n' || c == ';' || c == o)
						break;
				}
			};

			switch (c)
			{
			case ')':
			{
				skipBetween('(', ')');
				break;
			}
			case ']':
			{
				skipBetween('[', ']');
				break;
			}
			case '>':
			{
				skipBetween('<', '>');
				break;
			}
			case ':':
			{
				auto ti = i.translated(0, -1);

				if (get(ti) != ':')
					return;
				else
					advance(i);

				break;
			}
			case '(':
			case '?':
			case ' ':
			case '+':
			case '-':
			case ';':
			case '\n':
			case '\t':
			case '=':
			case ',':
			case '}':
			case '<':
			case '{': return;
			}

			
			ok = advance(i);
		}
	}
	case Target::token:
	{
		int s = lines.getToken(i.x, i.y, -1);
		int t = s;

		while (s == t && advance(i))
		{
			if (getNumColumns(i.x) > 0)
			{
				//s = t;
				t = lines.getToken(i.x, i.y, s);
			}
		}
		break;
	}
	case Target::lineUntilBreak:
	case Target::line:
	{
		auto isLineBreak = getNumLinesForRow(i.x) > 1 && target == Target::lineUntilBreak;

		if (isLineBreak)
		{
			auto y = getGlyphBounds(i, GlyphArrangementArray::ReturnLastCharacter).getY();

			while (get(i) != '\n' && advance(i))
			{
				auto thisY = getGlyphBounds(i, GlyphArrangementArray::ReturnBeyondLastCharacter).getY();

				if (thisY > y)
				{
					i.y--;
					break;
				}
			}
		}
		else
		{
			while (get(i) != '\n' && advance(i))
			{}
		}

		break;
	}
	case Target::paragraph: while (getNumColumns(i.x) > 0 && advance(i)) {} break;
	case Target::scope: jassertfalse; break; // IMPLEMENT ME
	case Target::document:
        
        if(direction == TextDocument::Direction::forwardRow ||
           direction == TextDocument::Direction::forwardCol)
            i = { getNumRows()-1, getNumColumns(getNumRows()-1)};
        else
            i = {0, 0};
            
        break;
	}

}

void mcl::TextDocument::navigateSelections(Target target, Direction direction, Selection::Part part)
{
	for (auto& selection : selections)
	{
		switch (part)
		{
		case Selection::Part::head: navigate(selection.head, target, direction); break;
		case Selection::Part::tail: navigate(selection.tail, target, direction); break;
		case Selection::Part::both: navigate(selection.head, target, direction); selection.tail = selection.head; break;
		}
	}

	sendSelectionChangeMessage();

}

mcl::Selection mcl::TextDocument::search(juce::Point<int> start, const juce::String& target) const
{
	while (start != getEnd())
	{
		auto y = lines[start.x].indexOf(start.y, target);

		if (y != -1)
			return Selection(start.x, y, start.x, y + target.length());

		start.y = 0;
		start.x += 1;
	}
	return Selection();
}

juce_wchar mcl::TextDocument::getCharacter(Point<int> index) const
{
	if (index.x < 0 || index.y < 0)
		return 0;

	//jassert(0 <= index.x && index.x <= lines.size());
	

	if (index == getEnd() || index.y >= lines[index.x].length())
	{
		return '\n';
	}
	return lines[index.x].getCharPointer()[index.y];
}

const mcl::Selection& mcl::TextDocument::getSelection(int index) const
{
	if(isPositiveAndBelow(index, selections.size()))
		return selections.getReference(index);

	static mcl::Selection empty;
	return empty;
}

const Array<mcl::Selection>& mcl::TextDocument::getSelections() const
{
	return selections;
}

juce::Array<mcl::Selection>& mcl::TextDocument::getSelections()
{
	return selections;
}

String mcl::TextDocument::getSelectionContent(Selection s) const
{
	s = s.oriented();

	if (s.isSingleLine())
	{
		return lines[s.head.x].substring(s.head.y, s.tail.y);
	}
	else
	{
		String content = lines[s.head.x].substring(s.head.y) + "\n";

		for (int row = s.head.x + 1; row < s.tail.x; ++row)
		{
			content += lines[row] + "\n";
		}
		content += lines[s.tail.x].substring(0, s.tail.y);
		return content;
	}
}

mcl::Transaction mcl::TextDocument::fulfill(const Transaction& transaction)
{
    setSearchResults({});
    
	cachedBounds = {}; // invalidate the bounds

	const auto t = transaction.accountingForSpecialCharacters(*this);
	const auto s = t.selection.oriented();
	const auto L = getSelectionContent(s.horizontallyMaximized(*this));
	const auto i = s.head.y;
	const auto j = L.lastIndexOf("\n") + s.tail.y + 1;
	const auto M = L.substring(0, i) + t.content + L.substring(j);

	for (auto& existingSelection : selections)
	{
		existingSelection.pullBy(s);
		existingSelection.pushBy(Selection(t.content).startingFrom(s.head));
	}

	auto sPos = CodeDocument::Position(doc, s.head.x, s.head.y);
	auto ePos = CodeDocument::Position(doc, s.tail.x, s.tail.y);

	shouldBe = M;

	ScopedValueSetter<bool> svs(checkThis, true);

	doc.replaceSection(sPos.getPosition(), ePos.getPosition(), t.content);

	using D = Transaction::Direction;
	auto inf = std::numeric_limits<float>::max();

	Transaction r;
	r.selection = Selection(t.content).startingFrom(s.head);
	r.content = L.substring(i, j);
	r.affectedArea = Rectangle<float>(0, 0, inf, inf);
	r.direction = t.direction == D::forward ? D::reverse : D::forward;

	return r;
}

void mcl::TextDocument::clearTokens(juce::Range<int> rows)
{
	for (int n = rows.getStart(); n < rows.getEnd(); ++n)
	{
		lines.clearTokens(n);
	}
}

void mcl::TextDocument::applyTokens(juce::Range<int> rows, const juce::Array<Selection>& zones)
{
	for (int n = rows.getStart(); n < rows.getEnd(); ++n)
	{
		for (const auto& zone : zones)
		{
			if (zone.intersectsRow(n))
			{
				lines.applyTokens(n, zone);
			}
		}
	}
}

juce::Array<juce::Line<float>> mcl::TextDocument::getUnderlines(const Selection& s, Metric m) const
{
	auto o = s.oriented();

	Range<int> lineRange = { o.head.x, o.tail.x + 1 };
	Array<Line<float>> underlines;

	for (int l = lineRange.getStart(); l < lineRange.getEnd(); l++)
	{
		if (isPositiveAndBelow(l, getNumRows()) && !foldManager.isFolded(l))
		{
			int left = 0;
			int right = getNumColumns(l);

			if (l == lineRange.getStart())
				left = o.head.y;

			if (l == lineRange.getEnd() - 1)
				right = o.tail.y;

			auto ul = lines.lines[l]->getUnderlines({ left, right }, !s.isSingular());

			float delta = 0.0f;

			switch (m)
			{
			case Metric::top:		delta = 0.0f; break;
			case Metric::ascent:
			case Metric::baseline:  delta = (getRowHeight() + getFontHeight()) / 2.0f + 2.0f; break;
			case Metric::bottom:	delta = getRowHeight();
			default:
				break;
			}

			auto t = AffineTransform::translation(TEXT_INDENT, getVerticalPosition(l, Metric::top) + delta);
			for (auto& u : ul)
			{
				u.applyTransform(t);
			}

			underlines.addArray(ul);
		}
	}

	return underlines;
}

mcl::TextDocument::TextDocument(CodeDocument& doc_) :
	CoallescatedCodeDocumentListener(doc_),
	doc(doc_),
	foldManager(doc_)
{
	addFoldListener(this);

	if (doc.getNumCharacters() > 0)
    {
        lineRangeChanged({0, doc.getNumLines()}, true);
		codeChanged(true, 0, doc.getNumCharacters());
    }
}


BreakpointManager::~BreakpointManager()
{}

BreakpointManager::Listener::~Listener()
{}

void BreakpointManager::addListener(Listener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void BreakpointManager::removeListener(Listener* l)
{
	listeners.removeAllInstancesOf(l);
}

void BreakpointManager::addBreakpoint(int lineNumber, NotificationType notifyListeners)
{
	if (!breakpoints.contains(lineNumber))
	{
		breakpoints.add(lineNumber);
		breakpoints.sort();

		if (notifyListeners != dontSendNotification)
			sendListenerMessage();
	}
}

void BreakpointManager::sendListenerMessage()
{
	for (auto l : listeners)
	{
		if (l != nullptr)
			l->breakpointsChanged();
	}
}

FoldableLineRange::FoldableLineRange(const CodeDocument& doc, Range<int> r, bool folded_):
	start(doc, r.getStart(), 0),
	end(doc, r.getEnd(), 0),
	folded(folded_)
{
	start.setPositionMaintained(true);
	end.setPositionMaintained(true);
}

void FoldableLineRange::sortList(List listToSort, bool sortChildren)
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

Result FoldableLineRange::checkList(List& listToCheck, WeakPtr parent)
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

FoldableLineRange::Listener::~Listener()
{}

void FoldableLineRange::Listener::rootWasRebuilt(WeakPtr newRoot)
{}

FoldableLineRange::Holder::Holder(CodeDocument& d):
	doc(d)
{}

void FoldableLineRange::Holder::toggleFoldState(int lineNumber)
{
	if (auto r = getRangeWithStartAtLine(lineNumber))
	{
		r->folded = !r->folded;
		updateFoldState(r);
	}
}

void FoldableLineRange::Holder::unfold(int lineNumber)
{
	for (auto a : all)
	{
		if (a->getLineRange().contains(lineNumber))
			a->folded = false;
	}

	updateFoldState(nullptr);
}

void FoldableLineRange::Holder::updateFoldState(WeakPtr r)
{
	lineStates.clear();
	scopeStates.clear();

	for (auto a : all)
	{
		if(a->isScoped())
		{
			auto r = a->getLineRange();
			scopeStates.setRange(r.getStart()+1, r.getLength()-1, true);
		}
		if (a->folded)
		{
			auto r = a->getLineRange();
			lineStates.setRange(r.getStart() + 1, r.getLength() - 1, true);
		}
	}

	sendFoldChangeMessage(r);
}

void FoldableLineRange::Holder::sendFoldChangeMessage(WeakPtr r)
{
	for (auto l : listeners)
	{
		if (l.get() != nullptr)
			l->foldStateChanged(r);
	}
}

FoldableLineRange::WeakPtr FoldableLineRange::Holder::getRangeWithStartAtLine(int lineNumber) const
{
	for (auto r : all)
	{
		if (r->getLineRange().getStart() == lineNumber)
			return r;
	}

	return nullptr;
}

int FoldableLineRange::Holder::getNearestLineStartOfAnyRange(int lineNumber)
{
	for (auto r : all)
	{
		auto n = r->getNearestLineStart(lineNumber);

		if (n != -1)
			return n;
	}

	return lineNumber;
}

FoldableLineRange::WeakPtr FoldableLineRange::Holder::getRangeContainingLine(int lineNumber) const
{
	for (auto r : all)
	{
		if (r->getLineRange().contains(lineNumber))
			return r;
	}

	return nullptr;
}

Range<int> FoldableLineRange::Holder::getRangeForLineNumber(int lineNumber) const
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

FoldableLineRange::Holder::LineType FoldableLineRange::Holder::getLineType(int lineNumber) const
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

bool FoldableLineRange::Holder::isFolded(int lineNumber) const
{
	return lineStates[lineNumber];
}

void FoldableLineRange::Holder::addToFlatList(List& flatList, const List& nestedList)
{
	for (auto l : nestedList)
	{
		flatList.add(l);
		addToFlatList(flatList, l->children);
	}
}

void FoldableLineRange::Holder::setRanges(FoldableLineRange::List newRanges)
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

bool FoldableLineRange::contains(Ptr other) const
{
	return getLineRange().contains(other->getLineRange());
}

FoldableLineRange::WeakPtr FoldableLineRange::getParent() const
{ return parent; }

bool FoldableLineRange::isFolded() const
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

bool FoldableLineRange::forEach(const std::function<bool(WeakPtr)>& f)
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

Range<int> FoldableLineRange::getLineRange() const
{
	return { start.getLineNumber(), end.getLineNumber() +1};
}

void FoldableLineRange::setFolded(bool shouldBeFolded)
{
	folded = shouldBeFolded;
}

bool FoldableLineRange::operator==(const FoldableLineRange& other) const
{
	return other.start.getPosition() == start.getPosition();
}

void FoldableLineRange::setEnd(int charPos)
{
	end.setPosition(charPos);
}

int FoldableLineRange::getNearestLineStart(int lineNumber)
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

TextDocument::SelectionAction::SelectionAction(TextDocument& t, const Array<Selection>& now_):
	now(now_),
	doc(&t)
{
	before.addArray(t.selections);
}

bool TextDocument::SelectionAction::perform()
{
	if(doc != nullptr)
	{
		doc->setSelections(now, false);
		return true;
	}
	
	return true;
}

bool TextDocument::SelectionAction::undo()
{
	if(doc != nullptr)
	{
		doc->setSelections(before, false);
		return true;
	}
	
	return true;
}

void TextDocument::deactivateLines(SparseSet<int> deactivatedLines)
{
		
}

juce::Font TextDocument::getFont() const
{ return font; }

float TextDocument::getLineSpacing() const
{ return lineSpacing; }

void TextDocument::setFont(juce::Font fontToUse)
{

	font = fontToUse; lines.font = fontToUse;
	lines.characterRectangle = { 0.0f, 0.0f, font.getStringWidthFloat(" "), font.getHeight() };
}

void TextDocument::addSelection(Selection selection)
{ 
	auto newSelections = selections;
	newSelections.add(selection);
	setSelections(newSelections, true);
}

void TextDocument::setSelection(int index, Selection newSelection, bool useUndo)
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

void TextDocument::sendSelectionChangeMessage() const
{
	for (auto l : selectionListeners)
	{
		if (l != nullptr)
			l.get()->selectionChanged();
	}
}

void TextDocument::foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged)
{
	rebuildRowPositions();
}

void TextDocument::rootWasRebuilt(FoldableLineRange::WeakPtr newRoot)
{

}

int TextDocument::getNumSelections() const
{ return selections.size(); }

const juce::String& TextDocument::getLine(int lineIndex) const
{ return lines[lineIndex]; }

float TextDocument::getRowHeight() const
{
	return font.getHeight() * lineSpacing;
}

Rectangle<float> TextDocument::getCharacterRectangle() const
{
	return lines.characterRectangle;
}

void TextDocument::setMaxLineWidth(int maxWidth)
{
	if (maxWidth != lines.maxLineWidth)
	{
		lines.maxLineWidth = maxWidth;
		invalidate({});
	}
}

CodeDocument& TextDocument::getCodeDocument()
{
	return doc;
}

void TextDocument::invalidate(Range<int> lineRange)
{
	lines.invalidate(lineRange);
	cachedBounds = {};
	rebuildRowPositions();
}

void TextDocument::rebuildRowPositions()
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

void TextDocument::lineRangeChanged(Range<int> r, bool wasAdded)
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

void TextDocument::codeChanged(bool wasInserted, int startIndex, int endIndex)
{
	CodeDocument::Position pos(getCodeDocument(), wasInserted ? endIndex : startIndex);

	if (getNumSelections() == 1)
	{
		setSelections(Selection(pos.getLineNumber(), pos.getIndexInLine(), pos.getLineNumber(), pos.getIndexInLine()), false);
	}
}

int TextDocument::getNumLinesForRow(int rowIndex) const
{
	if(isPositiveAndBelow(rowIndex, lines.lines.size()))
		return roundToInt(lines.lines[rowIndex]->height / font.getHeight());

	return 1;
}

float TextDocument::getFontHeight() const
{ return font.getHeight(); }

void TextDocument::addSelectionListener(Selection::Listener* l)
{
	selectionListeners.addIfNotAlreadyThere(l);
	l->displayedLineRangeChanged(currentlyDisplayedLineRange);
}

void TextDocument::removeSelectionListener(Selection::Listener* l)
{
	selectionListeners.removeAllInstancesOf(l);
}

void TextDocument::setDisplayedLineRange(Range<int> newRange)
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

bool TextDocument::jumpToLine(int lineNumber, bool justScroll)
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

void TextDocument::setDuplicateOriginal(const Selection& s)
{
	duplicateOriginal = s;
}

FoldableLineRange::Holder& TextDocument::getFoldableLineRangeHolder()
{
	return foldManager;
}

const FoldableLineRange::Holder& TextDocument::getFoldableLineRangeHolder() const
{
	return foldManager;
}

void TextDocument::addFoldListener(FoldableLineRange::Listener* l)
{
	foldManager.listeners.addIfNotAlreadyThere(l);
}

void TextDocument::removeFoldListener(FoldableLineRange::Listener* l)
{
	foldManager.listeners.removeAllInstancesOf(l);
}

void TextDocument::setSearchResults(const Array<Selection>& newSearchResults)
{
	searchResults = newSearchResults;
}

void TextDocument::setExternalViewUndoManager(UndoManager* um)
{
	viewUndoManagerToUse = um;
}

Array<Selection> TextDocument::getSearchResults() const
{
	return searchResults;
}

mcl::Bookmark FoldableLineRange::getBookmark() const
{
	Bookmark b;
	b.lineNumber = start.getLineNumber();

	auto copy = start;

	while (copy.getLineNumber() == b.lineNumber)
	{
		b.name << copy.getCharacter();
		auto p = copy.getPosition();
		copy.moveBy(1);

		if (p == copy.getPosition())
			break;
	}
		
	b.name = b.name.trimCharactersAtStart("#").trim();

	return b;
}





}
