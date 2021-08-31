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


namespace Icons
{
static const unsigned char lineBreak[] = { 110,109,254,60,16,68,10,247,170,68,108,254,60,16,68,0,8,177,68,98,254,60,16,68,215,27,177,68,221,28,16,68,215,43,177,68,63,245,15,68,215,43,177,68,108,72,217,13,68,215,43,177,68,108,72,217,13,68,205,44,177,68,108,172,60,9,68,205,44,177,68,108,172,60,
9,68,10,55,179,68,108,0,104,3,68,205,76,176,68,108,172,60,9,68,143,98,173,68,108,172,60,9,68,205,108,175,68,108,201,38,13,68,205,108,175,68,108,201,38,13,68,10,247,170,68,108,254,60,16,68,10,247,170,68,99,101,0,0 };

}


LinebreakDisplay::LinebreakDisplay(mcl::TextDocument& d) :
	LambdaCodeDocumentListener(d.getCodeDocument()),
	document(d)
{
	setCallback(std::bind(&LinebreakDisplay::refresh, this));
}

void LinebreakDisplay::paint(Graphics& g)
{
	float yPos = 0.0f;

	Path p;
	p.loadPathFromData(Icons::lineBreak, sizeof(Icons::lineBreak));

	for (int i = 0; i < document.getNumRows(); i++)
	{
		yPos = document.getVerticalPosition(i, mcl::TextDocument::Metric::top);
		int numLines = document.getNumLinesForRow(i) - 1;

		g.setColour(Colours::white.withAlpha(0.1f));

		for (int i = 0; i < numLines; i++)
		{
			Rectangle<float> d(0.0f, yPos, (float)getWidth(), (float)getWidth());

			d.reduce(3.0f, 3.0f);

			d = d.transformed(transform).withX(0.0f);

			p.scaleToFit(d.getX(), d.getY(), d.getWidth(), d.getHeight(), true);
			g.fillPath(p);

			yPos += document.getFontHeight();
		}
	}
}

void mcl::CodeMap::mouseEnter(const MouseEvent& e)
{
}

void mcl::CodeMap::mouseExit(const MouseEvent& e)
{
	hoveredLine = -1;

	if (preview != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(preview, 200);
		preview = nullptr;
	}

	repaint();

#if 0
	Desktop::getInstance().getAnimator().fadeOut(preview, 200);
	auto editor = findParentComponentOfClass<TextEditor>();
	editor->removeChildComponent(preview);



	preview = nullptr;
#endif



}

void mcl::CodeMap::mouseMove(const MouseEvent& e)
{
	hoveredLine = getLineNumberFromEvent(e);

	if (allowHover && preview == nullptr)
	{
		getParentComponent()->addChildComponent(preview = new HoverPreview(*this, hoveredLine));
		Desktop::getInstance().getAnimator().fadeIn(preview, 600);
	}

	if (preview != nullptr)
	{
		preview->setBounds(getPreviewBounds(e));
		preview->setCenterRow(getLineNumberFromEvent(e));
	}

	repaint();
}

void mcl::CodeMap::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);
		m.addItem(1, "Small Width", true, getWidth() < 100);
		m.addItem(2, "Normal Width", true, getWidth() > 100);
		m.addItem(3, "Enable Hover Preview", true, allowHover);

		auto r = m.show();

		if (r == 1)
		{
			findParentComponentOfClass<FullEditor>()->mapWidth = 75;
			getParentComponent()->resized();
		}
		if (r == 2)
		{
			findParentComponentOfClass<FullEditor>()->mapWidth = 150;
			getParentComponent()->resized();
		}
		if (r == 3)
			allowHover = !allowHover;

		return;
	}

	if (preview != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(preview, 200);
		preview = nullptr;
	}
	

	currentAnimatedLine = displayedLines.getStart() + displayedLines.getLength() / 2;
	targetAnimatedLine = getLineNumberFromEvent(e);

	startTimer(60);
}


float mcl::CodeMap::getLineNumberFromEvent(const MouseEvent& e) const
{
	auto yNormalised = e.position.getY() / (float)getHeight();

	return surrounding.getStart() + yNormalised * surrounding.getLength();
}


juce::Rectangle<int> mcl::CodeMap::getPreviewBounds(const MouseEvent& e)
{
	auto editor = findParentComponentOfClass<FullEditor>();



	auto b = editor->getBounds();
	b.removeFromRight(getWidth());

	auto sliceWidth = jmin(600, editor->getWidth() / 3);

	auto slice = b.removeFromRight(sliceWidth).toFloat();

	auto yNormalised = e.position.getY() / (float)getHeight();

	auto ratio = (float)editor->getWidth() / (float)editor->getHeight();

	auto height = slice.getWidth() / ratio;

	auto diff = slice.getHeight() - height;

	auto a = yNormalised;
	auto invA = 1.0f - yNormalised;

	slice.removeFromTop(a * diff);
	slice.removeFromBottom(invA * diff);

	return slice.toNearestInt();

}

void mcl::CodeMap::paint(Graphics& g)
{
	//g.fillAll(Colour(0xe3212121));

	if (!isActive())
	{
		return;
	}

	int offsetY = -1.0f;

	BigInteger shownLines;
	int index = 0;
	shownLines.setBit(colouredRectangles.size(), false);

	SparseSet<int> selectedLines;

	for (auto& s : doc.getSelections())
	{
		if (!s.isSingular())
		{
			auto start = s.head;
			auto end = s.tail;

			if (start.x > end.x)
				std::swap(start, end);
			if (start.x == end.x && start.y > end.y)
				std::swap(start, end);

			CodeDocument::Position startPos(doc.getCodeDocument(), start.x, start.y);
			CodeDocument::Position endPos(doc.getCodeDocument(), end.x, end.y);
			selectedLines.addRange({ startPos.getPosition(), endPos.getPosition() + 1 });
		}
	}

	RectangleList<float> selection;

	for (auto& a : colouredRectangles)
	{
		if (doc.getFoldableLineRangeHolder().isFolded(a.lineNumber))
			continue;

		if (surrounding.contains(a.lineNumber))
		{
			if (offsetY == -1.0f)
				offsetY = a.area.getY();

			bool shown = displayedLines.contains(a.lineNumber);

			shownLines.setBit(index, shown);

			Colour c = a.c;





			auto characterArea = a.area.translated(0.0f, -offsetY);

			if (selectedLines.contains(a.position))
				selection.add(characterArea.withLeft(0.0f));

			if (a.isWhitespace())
				continue;

			g.setColour(c.withMultipliedAlpha(shown ? 1.0f : 0.4f));

			characterArea.removeFromBottom(characterArea.getHeight() / 4.0f);
			characterArea.removeFromRight(characterArea.getWidth() * 0.2f);

			if (!a.upper)
				characterArea.removeFromTop(characterArea.getHeight() * 0.33f);



			g.fillRect(characterArea);


		}

		index++;
	}

	g.setColour(Colours::blue.withAlpha(0.4f));
	g.fillRectList(selection);

	auto y1 = lineToY(displayedLines.getStart());
	auto y2 = lineToY(displayedLines.getEnd());

	g.setColour(Colours::grey.withAlpha(0.2f));

	Rectangle<float> shownArea(0.0f, y1, (float)getWidth(), y2 - y1);

	g.fillRoundedRectangle(shownArea, 1.0f);
	g.drawRoundedRectangle(shownArea, 1.0f, 1.0f);

	if (hoveredLine != -1 && !dragging)
	{
		auto numRows = displayedLines.getLength();

		auto x = 0.0f;
		auto y = lineToY(hoveredLine - numRows / 2);
		auto h = lineToY(hoveredLine + numRows / 2) - y;
		auto w = (float)getWidth();

		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillRect(x, y, w, h);
	}
}

float mcl::CodeMap::lineToY(int lineNumber) const
{
	if (surrounding.contains(lineNumber))
	{
		auto normalised = (float)(lineNumber - surrounding.getStart()) / (float)surrounding.getLength();
		return normalised * (float)getHeight();
	}
	else if (lineNumber < surrounding.getStart())
		return 0.0f;
	else
		return (float)getHeight();
}

void mcl::CodeMap::setVisibleRange(Range<int> visibleLines)
{
	if (!isActive())
	{
		return;
	}


	displayedLines = visibleLines;

	auto numRows = doc.getNumRows();

	float a = (float)displayedLines.getStart() / (float)numRows;
	float invA = 1.0f - a;

	auto numToAdd = (float)(getNumLinesToShow() - displayedLines.getLength());

	auto before = roundToInt(a * numToAdd);
	auto after = roundToInt(invA * numToAdd);

	auto sStart = displayedLines.getStart() - before;
	auto sEnd = displayedLines.getEnd() + after;

	surrounding = Range<int>(sStart, sEnd);


	if (surrounding.getStart() < 0)
		surrounding = surrounding.movedToStartAt(0);

	if (surrounding.getEnd() > doc.getNumRows())
		surrounding = surrounding.movedToEndAt(doc.getNumRows());

	if (displayedLines.getEnd() > doc.getNumRows())
		displayedLines = displayedLines.movedToEndAt(doc.getNumRows());

	repaint();
}

void mcl::CodeMap::rebuild()
{
	colouredRectangles.clear();

	if (!isActive() || !isShowing())
	{
		dirty = true;
		return;
	}
		
	dirty = false;

	CodeDocument::Iterator it(doc.getCodeDocument());

	auto lineLength = (float)doc.getCodeDocument().getMaximumLineLength();

	auto xScale = (float)(getWidth() - 6) / jlimit(1.0f, 80.0f, lineLength);

	if (auto tokeniser = getTokeniser())
	{
		auto colourScheme = getColourScheme();

		if (colourScheme == nullptr)
			return;

		while (!it.isEOF())
		{
			CodeDocument::Position start(doc.getCodeDocument(), it.getPosition());
			auto token = tokeniser->readNextToken(it);

			auto colour = colourScheme->types[token].colour;

			CodeDocument::Position end(doc.getCodeDocument(), it.getPosition());

			auto pos = start;
			float height = (float)getHeight() / (float)getNumLinesToShow();

			if (pos == end)
				break;

			while (pos != end)
			{
				float randomValue = (float)((pos.getCharacter() * 120954801) % 313) / 313.0f;

				auto x = 3.0f + xScale * (float)pos.getIndexInLine();
				auto h = height;
				auto y = (float)pos.getLineNumber() * h;
				auto w = xScale;

				ColouredRectangle r;
				r.lineNumber = pos.getLineNumber();
				r.position = pos.getPosition();

				if (!CharacterFunctions::isWhitespace(pos.getCharacter()))
				{
					r.upper = CharacterFunctions::isUpperCase(pos.getCharacter());

					auto alpha = jlimit(0.0f, 1.0f, 0.4f + randomValue);

					r.c = colour.withAlpha(alpha);

				}
				else
				{
					r.c = Colours::transparentBlack;
				}

				r.area = { x, y, w, h };

				colouredRectangles.add(r);

				pos.moveBy(1);
			}
		}
	}

	setVisibleRange(displayedLines);

	repaint();
}

int mcl::CodeMap::yToLine(float y) const
{
	auto normalised = y / (float)getHeight();

	return (float)surrounding.getStart() + normalised * (float)surrounding.getLength();
}

void mcl::CodeMap::mouseDrag(const MouseEvent& e)
{
	if (e.mouseWasDraggedSinceMouseDown() && !dragging)
	{
		dragging = true;
		dragDown = e.getPosition().getY();
		stopTimer();
	}

	if (dragging)
	{
		auto pos = e.getPosition().y;
		auto line = jlimit(0.0f, (float)doc.getNumRows(), (float)pos / (float)getHeight() * (float)doc.getNumRows());

		doc.jumpToLine(line, true);
	}

	hoveredLine = getLineNumberFromEvent(e);
	repaint();
}

void mcl::CodeMap::mouseUp(const MouseEvent& e)
{
	dragging = false;

	if (isTimerRunning())
	{
		stopTimer();
		doc.jumpToLine(targetAnimatedLine, true);
	}
}

void mcl::CodeMap::timerCallback()
{
	currentAnimatedLine = (currentAnimatedLine + targetAnimatedLine) / 2.0f;

	if (currentAnimatedLine == targetAnimatedLine)
	{
		stopTimer();
	}

	doc.jumpToLine(currentAnimatedLine, true);
}

void mcl::CodeMap::HoverPreview::paint(Graphics& g)
{
	auto& document = parent.doc;

	auto realStart = document.getFoldableLineRangeHolder().getNearestLineStartOfAnyRange(rows.getStart());

	Range<int> realRange(realStart, rows.getEnd() + 1);

	auto index = Point<int>(jmax(realStart, 0), 0);

	CodeDocument::Position pos(document.getCodeDocument(), index.x, index.y);

	auto it = CodeDocument::Iterator(pos);
	auto previous = Point<int>(it.getLine(), it.getIndexInLine());
	auto zones = Array<Selection>();
	
	if (auto tokeniser = parent.getTokeniser())
	{
		while (it.getLine() <= rows.getEnd() && !it.isEOF())
		{
			auto tokenType = tokeniser->readNextToken(it);

			Point<int> next(it.getLine(), it.getIndexInLine());

			if (next == previous)
				break;

			zones.add(Selection(previous, next).withStyle(tokenType));
			previous = next;
		}
	}

	document.clearTokens(realRange);
	document.applyTokens(realRange, zones);

	int top = rows.getStart();
	int bottom = rows.getEnd() + 1;

	RectangleList<float> area;

	for (int i = top; i < bottom + 1; i++)
		area.add(document.getBoundsOnRow(i, { 0, document.getNumColumns(i) }, GlyphArrangementArray::ReturnLastCharacter));

	auto displayBounds = area.getBounds();

	g.setColour(Colour(0xEE333333));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 4.0f);

	g.setColour(Colours::white.withAlpha(0.6f));
	g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 4.0f, 1.0f);

	auto transform = AffineTransform::scale(1.5f / scale).translated(displayBounds.getX() - 10, displayBounds.getY() - 10);

	g.saveState();
	g.addTransform(transform.inverted());

	g.setColour(Colours::black);


	if (auto colourScheme = parent.getColourScheme())
	{
		for (int i = 0; i < colourScheme->types.size(); i++)
		{
			g.setColour(colourScheme->types[i].colour);
			auto glyphs = document.findGlyphsIntersecting(displayBounds, i);
			glyphs.draw(g);
		}
	}

	g.restoreState();
}

void mcl::CodeMap::HoverPreview::setCenterRow(int newCenterRow)
{
	centerRow = newCenterRow;
	auto numRowsToShow = getHeight() / parent.doc.getFontHeight();

	rows = Range<int>(centerRow - numRowsToShow / 2, centerRow + numRowsToShow / 2);

	if (rows.getStart() < 0)
		rows = rows.movedToStartAt(0);

	repaint();
}

void FoldMap::Item::mouseDoubleClick(const MouseEvent& e)
{
	setSelected(true, true);
}

void FoldMap::Item::setSelected(bool shouldBeSelected, bool grabFocus)
{
	clicked = shouldBeSelected;

	if (clicked)
	{
		auto line = p->getLineRange().getStart() + 1;
		auto& doc = findParentComponentOfClass<FoldMap>()->doc;
		doc.setDisplayedLineRange(p->getLineRange());
		doc.jumpToLine(line);

		if (grabFocus)
			findParentComponentOfClass<FullEditor>()->editor.grabKeyboardFocusAndActivateTokenBuilding();
	}

	repaint();
}

juce::CodeEditorComponent::ColourScheme* CodeMap::getColourScheme()
{
	if (auto fe = findParentComponentOfClass<FullEditor>())
		return &fe->editor.colourScheme;

	return nullptr;
}

juce::CodeTokeniser* CodeMap::getTokeniser()
{
	if (auto fe = findParentComponentOfClass<FullEditor>())
		return fe->editor.tokeniser;

	return nullptr;
}

void addItem(Array<FoldMap::Item*>& list, FoldMap::Item* item)
{
	if (item == nullptr)
		return;

	list.add(item);

	for (auto c : item->children)
	{
		addItem(list, c);
	}
}

bool FoldMap::keyPressed(const KeyPress& k)
{
	if (k == KeyPress::upKey || k == KeyPress::downKey)
	{
		Array<Item*> allItems;
		
		for (auto i : items)
			addItem(allItems, i);

		bool up = k == KeyPress::upKey;

		for (int i = allItems.size() - 1; i >= 0; --i)
		{
			auto thisItem = allItems[i];
			auto nextItem = allItems[i + 1];
			auto prevItem = allItems[i - 1];

			jassert(thisItem != nullptr);

			if (up && nextItem != nullptr && nextItem->isBoldLine)
			{
				nextItem->setSelected(false, false);
				thisItem->setSelected(true, false);
				return true;
			}

			if (!up && nextItem != nullptr && thisItem->isBoldLine)
			{
				thisItem->setSelected(false, false);
				nextItem->setSelected(true, false);
				return true;
			}
		}

		repaint();
	}

	return false;
}

}
