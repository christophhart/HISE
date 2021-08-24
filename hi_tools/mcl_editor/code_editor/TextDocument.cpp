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

	for (const auto& line : StringArray::fromLines(content))
	{
		lines.add(line);
	}
}

int mcl::TextDocument::getNumRows() const
{
	return lines.size();
}

int mcl::TextDocument::getNumColumns(int row) const
{
	return lines[row].length();
}

float mcl::TextDocument::getVerticalPosition(int row, Metric metric) const
{
	row = jmin(row, lines.size());
	float pos = rowPositions[row];


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

	if (isPositiveAndBelow(row, getNumRows()))
	{
		columns.setStart(jmax(columns.getStart(), 0));
		auto l = lines.lines[row];

		auto boundsToUse = l->characterBounds;

		if (boundsToUse.isEmpty())
			boundsToUse = { 0.0f, 0.0f, font.getStringWidthFloat(" "), font.getHeight() };

		float yPos = getVerticalPosition(row, Metric::top);
		float xPos = TEXT_INDENT;
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

GlyphArrangement mcl::TextDocument::getGlyphsForRow(int row, int token, bool withTrailingSpace) const
{
	return lines.getGlyphs(row,
		getVerticalPosition(row, Metric::baseline),
		token,
		withTrailingSpace);
}

GlyphArrangement mcl::TextDocument::findGlyphsIntersecting(Rectangle<float> area, int token) const
{
	auto range = getRangeOfRowsIntersecting(area);
	auto rows = Array<RowData>();
	auto glyphs = GlyphArrangement();

	for (int n = range.getStart(); n < range.getEnd(); ++n)
	{
		if(!foldManager.isFolded(n))
			glyphs.addGlyphArrangement(getGlyphsForRow(n, token));
	}

	return glyphs;
}

juce::Range<int> mcl::TextDocument::getRangeOfRowsIntersecting(juce::Rectangle<float> area) const
{
	if (rowPositions.isEmpty())
		return { 0, 1 };

    auto topY = jmax<int>(0, area.getY());
    auto bottomY = area.getBottom();
    
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
        bottomIndex = rowPositions.size()-1;
    
    return { topIndex, bottomIndex + 1 };

	return { min, max + 1 };
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

	auto gap = font.getHeight() * lineSpacing - font.getHeight();
	float yPos = gap / 2.0f;

	if (position.y > rowPositions.getLast() + getRowHeight())
	{
		auto x = lines.size() - 1;
		if (x >= 0)
			return { x, getNumColumns(x) - 1 };
	}
		

	for (int l = 0; l < getNumRows(); l++)
	{
		auto line = lines.lines[l];

		if (foldManager.isFolded(l))
			continue;

		Range<float> p(yPos - gap / 2.0f, yPos + line->height + gap / 2.0f);

		if (p.contains(position.y))
		{
			auto glyphs = getGlyphsForRow(l, -1, true);

			int numGlyphs = glyphs.getNumGlyphs();
			auto col = numGlyphs;

			for (int n = 0; n < numGlyphs; ++n)
			{
				auto b = glyphs.getBoundingBox(n, 1, true).expanded(0.0f, gap / 2.f);

				if (b.contains(position))
				{
					col = n+1;
					break;
				}
			}

			return { l, col -1 };
		}

		yPos = p.getEnd();
	}

	return { 0, 0 };

	jassertfalse;

	auto lineHeight = font.getHeight() * lineSpacing;
	auto row = jlimit(0, jmax(getNumRows() - 1, 0), int(position.y / lineHeight));
	auto col = 0;
	auto glyphs = getGlyphsForRow(row);

	if (position.x > 0.f)
	{
		col = glyphs.getNumGlyphs();

		for (int n = 0; n < glyphs.getNumGlyphs(); ++n)
		{
			if (glyphs.getBoundingBox(n, 1, true).getHorizontalRange().contains(position.x))
			{
				col = n;
				break;
			}
		}
	}
	return { row, col };
}

Point<int> mcl::TextDocument::getEnd() const
{
	return { getNumRows(), 0 };
}

bool mcl::TextDocument::next(Point<int>& index) const
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

bool mcl::TextDocument::prev(Point<int>& index) const
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

bool mcl::TextDocument::nextRow(Point<int>& index) const
{
	if (index.x < getNumRows() - 1)
	{
		index.x += 1;
		index.y = jmin(index.y, getNumColumns(index.x));
		return true;
	}
	return false;
}

bool mcl::TextDocument::prevRow(Point<int>& index) const
{
	if (index.x > 0)
	{
		index.x -= 1;
		index.y = jmin(index.y, getNumColumns(index.x));
		return true;
	}
	return false;
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
			auto ok = nextRow(i); 

			while (foldManager.isFolded(i.x))
				ok = nextRow(i);

			return ok;
		};
		get = [this](Point<int> i) { return getCharacter(i); };
		break;
	case Direction::backwardRow:
		advance = [this](Point<int>& i) 
		{ 
			auto ok = prevRow(i);

			while (foldManager.isFolded(i.x))
				ok = prevRow(i);

			return ok;
		};
		get = [this](Point<int> i) { prev(i); return getCharacter(i); };
		break;
	case Direction::forwardCol:
		advance = [this](Point<int>& i) { return next(i); };
		get = [this](Point<int> i) { return getCharacter(i); };
		break;
	case Direction::backwardCol:
		advance = [this](Point<int>& i) { return prev(i); };
		get = [this](Point<int> i) { prev(i); return getCharacter(i); };
		break;
	}

	switch (target)
	{
	case Target::whitespace: while (!CF::isWhitespace(get(i)) && advance(i)) {} break;
	case Target::punctuation: while (!punctuation.containsChar(get(i)) && advance(i)) {} break;

	case Target::character: advance(i); break;
	case Target::firstnonwhitespace:
	{
		if (i.y != 0 && get(i) == '\n' && direction == Direction::backwardCol)
		{
			prev(i);
		}

		jassert(direction == Direction::backwardCol);

		bool skipTofirstNonWhiteCharacter = false;

		while (get(i) != '\n' && prev(i))
			skipTofirstNonWhiteCharacter |= !CF::isWhitespace(get(i));

		while (skipTofirstNonWhiteCharacter && CF::isWhitespace(get(i)))
			next(i);

		if (skipTofirstNonWhiteCharacter)
			prev(i);

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
	case Target::line: while (get(i) != '\n' && advance(i)) {} break;
	case Target::paragraph: while (getNumColumns(i.x) > 0 && advance(i)) {} break;
	case Target::scope: jassertfalse; break; // IMPLEMENT ME
	case Target::document: while (advance(i)) {} break;
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
	return selections.getReference(index);
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
		codeChanged(true, 0, doc.getNumCharacters());
}


}