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


float GutterComponent::getGutterWidth() const
{
	auto numRows = document.getNumRows();
	auto digits = numRows > 0 ? (int)(log10(numRows)) : 0;

	auto w = document.getCharacterRectangle().getWidth() * (digits+4);

	if(breakpointsEnabled)
		w += document.getCharacterRectangle().getHeight() * 0.6f + 5;
	
	w += 3;

	return w * scaleFactor;
}

void GutterComponent::foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged)
{
	repaint();
}

void GutterComponent::timerCallback()
{
	for (auto s : breakpoints)
	{
		owner->modifyUriBreakpoint(s->toUriBreakpoint(), sendNotificationAsync);
	}
	
	stopTimer();
	repaint();
}

void GutterComponent::triggerBreakpointUpdate()
{
	startTimer(100);
}

void GutterComponent::codeDocumentTextInserted(const String& newText, int insertIndex)
{
	triggerBreakpointUpdate();
}

void GutterComponent::codeDocumentTextDeleted(int startIndex, int endIndex)
{
	triggerBreakpointUpdate();
}

void GutterComponent::rootWasRebuilt(FoldableLineRange::WeakPtr newRoot)
{
	repaint();
}

TooltipWithArea::Data GutterComponent::getTooltip(Point<float> pos)
{
	TooltipWithArea::Data d;
	d.relativePosition = pos;
	d.id = "gutter";
	d.text = "Click to add breakpoint";
	d.relativePosition.setX(getGutterWidth());

	if (hoveredData.rowNumber == currentBreakLine.getLineNumber())
	{
		d.id = "breakline";
		d.text = "Drag to move the instruction pointer within the current scope";
	}
	else if (auto bp = getBreakpoint(hoveredData.rowNumber))
	{
		d.id = String(breakpoints.indexOf(bp));

		if (bp->currentState == Breakpoint::State::Stale)
			d.text = "Breakpoint is stale. Recompile the script after text edits. ";
		else if (bp->currentState == Breakpoint::State::Unresolved)
			d.text = "Breakpoint is disabled because no executable location was found. ";
		else if (!bp->enabled.getValue())
			d.text = "Breakpoint is disabled. Toggle via context menu. ";
		else if (!breakpointsEnabled)
			d.text = "Breakpoints are disabled globally for this editor. ";
		else
		{
			d.text = "Breakpoint active. ";

			if (bp->blinkIfHit.getValue())
				d.text << "Blinks on hit. ";

			if (bp->breakIfHit.getValue())
				d.text << "Breaks on hit. ";

			if (bp->useCondition.getValue())
				d.text << "Condition: `" << bp->condition.getValue().toString() << "`. ";
		}
		
		d.text << "Click->delete breakpoint. Right click->Options";
	}
	
	return d;
}

void GutterComponent::mouseMove(const MouseEvent& event)
{
	repaint();
}



void GutterComponent::setBreakpointsEnabled(bool shouldBeEnabled)
{
	breakpointsEnabled = shouldBeEnabled;
	repaint();
}

Path GutterComponent::createArrow()
{
	Path p;
	Line<float> l(0.0f, 0.0f, 1.0f, 0.0f);
	p.addArrow(l, 0.3f, 1.0f, 0.5f);
	return p;
}

bool GutterComponent::injectBreakPoints(String& s)
{
	blinkHandler.clear();

	Component::SafePointer<GutterComponent> st(this);

	MessageManager::callAsync([st]()
	{
		if (st)
			st.getComponent()->repaint();
	});

	if (breakpoints.isEmpty())
		return false;

	auto lines = StringArray::fromLines(s);

	for (auto bp : breakpoints)
	{
		if (isPositiveAndBelow(bp->getLineNumber(), lines.size()))
			lines.set(*bp, bp->processLine(lines[*bp]));
	}

	s = lines.joinIntoString("\n");
	return true;
}

void GutterComponent::setError(int lineNumber, const String& error)
{
	errorLine = lineNumber;
        
	if(error.isEmpty())
		errorLine = -1;
        
	errorMessage = error;
	repaint();
}

void GutterComponent::setExternalCallstackMatches(const std::vector<UriLocation::WithExpression>& newExternalCallstack)
{
	externalCallstack = newExternalCallstack;
	
	SafeAsyncCall::repaint(this);
}

void GutterComponent::setCurrentBreakline(const UriError& breakPosition, UriLocationTree::Ptr scope)
{
	jassert(MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread());

	externalCallstack.clear();
	currentBreakPosition = breakPosition;

	if (!currentBreakPosition.wasOk())
	{
		currentBreakpointScope = scope;
		currentBreakLine = CodeDocument::Position(document.getCodeDocument(), breakPosition.line-1, 0);
		currentBreakLine.setPositionMaintained(true);
	}
	else
	{
		currentBreakpointScope = nullptr;
		currentBreakLine = {};
	}

	repaint();
}

void GutterComponent::onUriBreakpointAttachTypeChange(const UriBreakpoint& bp, UriBreakpoint::Owner::AttachState newState)
{
	for (auto b : breakpoints)
	{
		if (b->toUriBreakpoint().matches(bp))
		{
			b->currentState = newState;
			break;
		}
	}

	SafeAsyncCall::repaint(this);
}

void GutterComponent::onUriBreakpointChange(const UriBreakpoint& bp, ChangeType changeType)
{
	if (changeType == ChangeType::AttachStateChange)
		return;

	if (changeType == ChangeType::Modified)
	{
		for (auto b : breakpoints)
		{
			if (b->up.matches(bp))
			{
				b->copyPropertiesFrom(bp);
				return;
			}
		}

		return;
	}
		
	if (changeType == ChangeType::Added)
	{
		for (auto b : breakpoints)
		{
			if (b->up.matches(bp))
			{
				jassertfalse;
				return;
			}
		}

		Breakpoint::Ptr np = new Breakpoint(*this, bp, document.getCodeDocument());
		np->copyPropertiesFrom(bp);
		breakpoints.add(np);
	}
	else
	{
		for (auto b : breakpoints)
		{
			if (b->toUriBreakpoint().matches(bp))
			{
				breakpoints.removeObject(b);
				break;
			}
		}
	}

	SafeAsyncCall::repaint(this);
}

bool GutterComponent::matchesUriLocation(const UriLocation& location) const
{
	return document.matchesLocation(location);
}

GutterComponent::Breakpoint::Breakpoint(GutterComponent& parent_, const UriBreakpoint& bp_, CodeDocument& doc): 
	parent(parent_),
	pos(doc, bp_.line-1, bp_.col-1),
	up(bp_),
	condition(String("true")), 
	useCondition(var(1)), 
	enabled(var(1)),
	breakIfHit(var(1)),
	blinkIfHit(var(0)),
	profiling(false) 
{
	if(profiling)
	{
#if 0
		while(l >= 0 && !doc.getLine(l).containsChar('{'))
			l--;

		auto col = doc.getLine(0).indexOfChar('{');
		pos = CodeDocument::Position(doc, l, col);
		logExpression.setValue("Scope @ line " + String(l+1));
#endif
	}

	pos.setPositionMaintained(true);
	condition.addListener(this);
	useCondition.addListener(this);
	enabled.addListener(this);
	logExpression.addListener(this);
	breakIfHit.addListener(this);
	blinkIfHit.addListener(this);
}

UriBreakpoint GutterComponent::Breakpoint::toUriBreakpoint() const
{
	auto copy = up;
	copy.fillLocation(pos);

	if (enabled.getValue())
	{
		copy.blinkOnHit = (bool)blinkIfHit.getValue();
		copy.breakOnHit = (bool)breakIfHit.getValue();
		copy.enabled = (bool)enabled.getValue();
		copy.expression = useCondition.getValue() ? condition.getValue().toString() : "true";
	}

	return copy;
}

void GutterComponent::Breakpoint::copyPropertiesFrom(const UriBreakpoint& bp)
{
	up = bp;
	pos.setLineAndIndex(bp.line - 1, bp.col-1);

	ScopedValueSetter<bool> svs(skipUpdate, true);

	blinkIfHit.setValue(bp.blinkOnHit);
	breakIfHit.setValue(bp.breakOnHit);
	enabled.setValue(bp.enabled);
	useCondition.setValue(bp.expression != "true");
	condition.setValue(bp.expression);
}

String GutterComponent::Breakpoint::processLine(const String& s)
{
	if (enabled.getValue())
	{
		String m;

		if(profiling)
		{
			m << s;

			if(getCondition() != "true")
				m << ".if(" << getCondition() << "):";
			else
				m << ".";

			m << "profile(" << logExpression.toString().quoted() << ");";
		}
		else
		{
			auto e = logExpression.toString();

			if (e.isNotEmpty() || blinkIfHit.getValue())
			{
				if (getCondition() != "true")
					m << "if(" << getCondition() << "){ ";

				if (blinkIfHit.getValue())
					m << "Console.blink(); ";

				if(e.isNotEmpty())
					m << "Console.print(" << e << "); ";

				if (getCondition() != "true")
					m << "}";
			}
						
			if (breakIfHit.getValue())
				m << "Console.stop(" << getCondition() << "); ";

			m << s;
		}

		return m;
	}
	else
		return s;
}

int GutterComponent::Breakpoint::getLineNumber() const
{
	return pos.getLineNumber();
}

bool GutterComponent::Breakpoint::operator==(int i) const
{ return getLineNumber() == i; }

GutterComponent::Breakpoint::operator int() const
{ return getLineNumber(); }

Path GutterComponent::Breakpoint::createPath()
{
	Path p;
	if (hasCustomCondition())
	{
		p.startNewSubPath({ 0.0f, 0.5f });
		p.lineTo({ 0.5f, 0.0f });
		p.lineTo({ 1.0f, 0.5f });
		p.lineTo({ 0.5f, 1.0f });
		p.closeSubPath();
	}
	else
	{
		p.addEllipse({ 0.0f, 0.0f, 1.0f, 1.0f });
	}
	return p;
}

bool GutterComponent::Breakpoint::hasCustomCondition() const
{
	return condition.toString() != "true";
}

String GutterComponent::Breakpoint::getCondition() const
{ return (bool)useCondition.getValue() ? condition.toString() : String("true"); }

bool GutterComponent::BlinkState::blinkCallback()
{
	alpha *= JUCE_LIVE_CONSTANT_OFF(0.8f);

	if (alpha < 0.001f)
	{
		alpha = 0.0f;
		return false;
	}

	return true;
}

void GutterComponent::BlinkState::draw(GutterComponent& gutter, const Array<TextDocument::RowData>& rows, Graphics& g)
{
	if (alpha == 0.0f)
		return;

	auto GUTTER_WIDTH = gutter.getGutterWidth();

	for (auto& r : rows)
	{
		if ((r.rowNumber == lineNumber - 1))
		{
			auto b = gutter.getRowBounds(r);
			b.setWidth((float)gutter.getWidth());
			g.setColour(Colour(HISE_BREAKPOINT_COLOUR).withAlpha(alpha));
			g.fillRect(b.removeFromLeft(GUTTER_WIDTH));
			g.setColour(Colour(HISE_BREAKPOINT_COLOUR).withAlpha(alpha * 0.4f));
			g.fillRect(b);
		}
	}
}

GutterComponent::Breakpoint* GutterComponent::getBreakpoint(int lineNumber)
{
	for (auto bp : breakpoints)
	{
		if (*bp == lineNumber)
			return bp;
	}

	return nullptr;
}

GutterComponent::BlinkHandler::BlinkHandler(GutterComponent& parent_):
	parent(parent_)
{}

void GutterComponent::BlinkHandler::addBlinkState(int n)
{
	BlinkState bs;
	bs.lineNumber = n;
	startTimer(30);
	blinkStates.add(bs);
	parent.repaint();
}

void GutterComponent::BlinkHandler::clear()
{
	blinkStates.clear();
	stopTimer();
}

void GutterComponent::BlinkHandler::timerCallback()
{
	for (int i = 0; i < blinkStates.size(); i++)
	{
		if (!blinkStates.getReference(i).blinkCallback())
			blinkStates.remove(i--);
	}

	parent.repaint();

	if (blinkStates.isEmpty())
		stopTimer();
}

void GutterComponent::BlinkHandler::draw(Graphics& g, const Array<TextDocument::RowData>& rowData)
{
	for (auto& bs : blinkStates)
		bs.draw(parent, rowData, g);
}

//==========================================================================
mcl::GutterComponent::GutterComponent(TextDocument& document_)
	: document(document_)
	, memoizedGlyphArrangements([this](int row) { return getLineNumberGlyphs(row); })
	, blinkHandler(*this)
{
	document.getCodeDocument().addListener(this);
	setRepaintsOnMouseActivity(true);

}

GutterComponent::~GutterComponent()
{
	document.getCodeDocument().removeListener(this);
}

void mcl::GutterComponent::setViewTransform(const AffineTransform& transformToUse)
{
	transform = transformToUse;
	scaleFactor = transform.getScaleFactor();
	repaint();
}

void mcl::GutterComponent::updateSelections()
{
	repaint();
}

void mcl::GutterComponent::paint(Graphics& g)
{
	/*
	 Draw the gutter background, shadow, and outline
	 ------------------------------------------------------------------
	 */
	
	auto ln = Helpers::getEditorColour(Helpers::GutterColour);


	auto GUTTER_WIDTH = getGutterWidth();

	g.setColour(ln);
	g.fillRect(getLocalBounds().removeFromLeft(GUTTER_WIDTH));

	g.setColour(ln.darker(0.2f));
	g.drawVerticalLine(GUTTER_WIDTH - 1.f, 0.f, getHeight());

	/*
	 Draw the line numbers and selected rows
	 ------------------------------------------------------------------
	 */
	auto area = g.getClipBounds().toFloat().transformedBy(transform.inverted());
	auto rowData = document.findRowsIntersecting(area);
	
	auto getRowData = [&rowData](int lineNumber)
	{
		TextDocument::RowData* data = nullptr;

		for (auto& r : rowData)
		{
			if (r.rowNumber == lineNumber)
			{
				data = &r;
			}
		}

		return data;
	};

	auto f = document.getFont();

	f.setHeight(f.getHeight() * transform.getScaleFactor() * 0.8f);
	g.setFont(f);

	g.setColour(ln.contrasting(0.1f));

	auto& h = document.getFoldableLineRangeHolder();


	UnblurryGraphics ug(g, *this);

	for (const auto& r : rowData)
	{
		auto b = getRowBounds(r);
		bool showFoldRange = false;

		auto t = h.getLineType(r.rowNumber);

		if (isMouseOver(true))
		{
			auto pos = getMouseXYRelative().toFloat();

			if (b.contains(pos))
			{
				hoveredData = r;

				auto range = h.getRangeForLineNumber(r.rowNumber);

				for (const auto& inner : rowData)
				{
					if (!h.isFolded(inner.rowNumber) && range.contains(inner.rowNumber))
					{
						auto ib = getRowBounds(inner);
						g.setColour(Colours::white.withAlpha(0.1f));

						ib = ib.removeFromRight(15.0f * transform.getScaleFactor());

						if (t == FoldableLineRange::Holder::RangeStartClosed)
						{
							ib = ib.withWidth(getGutterWidth()).withX(0);
							showFoldRange = true;
						}

						//g.fillRect(ib);
					}
				}
			}
		}

		auto lfb = b;

		lfb = lfb.removeFromRight(15 * transform.getScaleFactor());

		auto c = getParentComponent()->findColour(CodeEditorComponent::lineNumberTextId).withBrightness(0.35f);

		g.setColour(c);



		if(h.scopeStates[r.rowNumber])
		{
			c = JUCE_LIVE_CONSTANT_OFF(Colour(0xff88bec5));

			g.setColour(c.withAlpha(0.05f));
			g.fillRect(b);
			g.setColour(c);
		}
		
		switch (t)
		{
		case FoldableLineRange::Holder::RangeStartOpen:
		case FoldableLineRange::Holder::RangeStartClosed:
		{
			auto w = lfb.getWidth() - 4.0f * transform.getScaleFactor();
			auto box = lfb.withSizeKeepingCentre(w, w);

			if(h.scopeStates[r.rowNumber+1])
			{
				g.setColour(Colour(0xff88bec5));
			}

			box = ug.getRectangleWithFixedPixelWidth(box, (int)box.getWidth());

			if (t == FoldableLineRange::Holder::RangeStartClosed)
			{
				g.setColour(Colours::white.withAlpha(0.2f));
				g.fillRect(box);
				g.setColour(getParentComponent()->findColour(CodeEditorComponent::lineNumberTextId).withBrightness(0.7f));
			}

			g.drawRect(box, 1.0f);

			box = box.reduced(2.0f * transform.getScaleFactor());

			g.drawHorizontalLine(box.getCentreY(), box.getX(), box.getRight());

			

			if (t == FoldableLineRange::Holder::RangeStartClosed)
			{

				ug.draw1PxHorizontalLine(b.getBottom(), 0.0f, b.getRight());
				g.drawVerticalLine((int)box.getCentreX(), box.getY(), box.getBottom());

				

			}
				

			break;
		}
		case FoldableLineRange::Holder::Between:
		{
			ug.draw1PxVerticalLine(lfb.getCentreX(), lfb.getY(), lfb.getBottom());
			break;
		}
		case FoldableLineRange::Holder::RangeEnd:
		{
			Path p;

			auto b = lfb.getBottom() - 2.0f * transform.getScaleFactor();

			ug.draw1PxVerticalLine(lfb.getCentreX(), lfb.getY(), b);
			ug.draw1PxHorizontalLine(b, lfb.getCentreX(), lfb.getRight() - 3.0f * transform.getScaleFactor());
		}
        default: break;
		}
	}

	
	auto gw = getGutterWidth();

	for (const auto& r : rowData)
	{
		if (h.isFolded(r.rowNumber))
			continue;

		auto A = r.bounds.getRectangle(0)
			.transformedBy(transform)
			.withX(0)
			.withWidth(gw);

		A.removeFromRight(15 * transform.getScaleFactor());

		g.setColour(getParentComponent()->findColour(CodeEditorComponent::lineNumberTextId).withMultipliedAlpha(0.4f));
		g.drawText(String(r.rowNumber + 1), A.reduced(5.0f, 0.0f), Justification::right, false);
	}

	Array<int> drawnBreakpoints;
	drawnBreakpoints.ensureStorageAllocated(breakpoints.size());

	float BREAKPOINT_OFFSET = JUCE_LIVE_CONSTANT_OFF(1.0f);

	for (auto bp : breakpoints)
	{
		if (auto r = getRowData(bp->getLineNumber()))
		{
			auto b = getRowBounds(*r).withHeight(document.getRowHeight() * scaleFactor);


			drawnBreakpoints.add(r->rowNumber);

			b = b.withWidth(b.getHeight()).reduced(JUCE_LIVE_CONSTANT_OFF(6.0f) * scaleFactor);

			b.translate(BREAKPOINT_OFFSET, 0.0);

			auto t = h.getLineType(*bp);

			if (t == FoldableLineRange::Holder::LineType::Folded)
				continue;

			auto bPath = bp->createPath();
			PathFactory::scalePath(bPath, b);

			auto isActive = (bool)bp->enabled.getValue();

			isActive &= breakpointsEnabled;
			isActive &= bp->currentState == Breakpoint::State::OK;

			if (isActive)
			{
				Colour c;

				if(bp->profiling)
				{
					bPath.clear();
					bPath.loadPathFromData(EditorIcons::profileIcon, EditorIcons::profileIcon_Size);
					PathFactory::scalePath(bPath, b);
					c = Colour(HISE_WARNING_COLOUR);
				}
				else
					c = bp->breakIfHit.getValue() ? Colour(0xFF683333) : Colour(0xFF333368);

				g.setColour(c);
				g.fillPath(bPath);
			}

			g.setColour(Colours::white.withAlpha(0.4f));
			g.strokePath(bPath, PathStrokeType(1.0f));
		}
	}

	auto pos = getMouseXYRelative();

	if (hoveredData.rowNumber != -1 && !drawnBreakpoints.contains(hoveredData.rowNumber) && hitTest(pos.x, pos.y))
	{
		auto b = getRowBounds(hoveredData).withHeight(document.getRowHeight() * scaleFactor);
		b = b.withWidth(b.getHeight()).reduced(JUCE_LIVE_CONSTANT_OFF(7.0f) * scaleFactor);
		b.translate(BREAKPOINT_OFFSET, 0.0);

		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillEllipse(b);
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawEllipse(b, 1.0f);
	}

	if (currentBreakpointScope != nullptr)
	{
		jassert(currentBreakpointScope->hasRange());
		
		auto scopeLineRange = currentBreakpointScope->getLineRange();

		auto scopeStart = getRowData(scopeLineRange.getStart() - 1);
		auto scopeEnd = getRowData(scopeLineRange.getEnd() - 1);

		if (scopeStart != nullptr && scopeEnd != nullptr)
		{
			auto a = getRowBounds(*scopeStart).withHeight(document.getRowHeight() * scaleFactor);
			auto b = getRowBounds(*scopeEnd).withHeight(document.getRowHeight() * scaleFactor);

			auto scopeArea = a.getUnion(b);

			g.setColour(Colour(HISE_BREAKPOINT_COLOUR));
			g.fillRect(scopeArea.withWidth(1).toFloat());
		}
	}

	Array<int> drawnLines;
	Path callstackIcon;
	callstackIcon.loadPathFromData(EditorIcons::redoIcon, EditorIcons::redoIcon_Size);

	auto drawCallstackLine = [&](Graphics& g, const UriLocation& callstackLine)
	{
		auto rowIndex = callstackLine.line - 1;

		if (drawnLines.contains(rowIndex) || rowIndex == currentBreakLine.getLineNumber())
			return;

		if (auto r = getRowData(rowIndex))
		{
			g.setColour(Colour(HISE_BREAKPOINT_COLOUR).withAlpha(0.7f));

			auto b = getRowBounds(*r).withHeight(document.getRowHeight() * scaleFactor);
			auto pb = b.withWidth(b.getHeight()).reduced(5);

			PathFactory::scalePath(callstackIcon, pb);
			g.fillPath(callstackIcon);
			g.setColour(Colour(HISE_BREAKPOINT_COLOUR).withAlpha(0.05f));
			g.fillRect(b.withWidth(getWidth()));
			drawnLines.add(rowIndex);
		}
	};

	for (const auto& ex : externalCallstack)
		drawCallstackLine(g, ex.second);

	if (currentBreakLine != CodeDocument::Position())
	{
		for (auto bp : breakpoints)
			drawnLines.add(bp->getLineNumber());

		for (auto cs : currentBreakPosition.callstack)
		{
			if (!currentBreakPosition.matches(cs.second))
				continue;

			drawCallstackLine(g, cs.second);
		}
			

		if (auto r = getRowData(currentBreakLine.getLineNumber()))
		{
			auto b = getRowBounds(*r).withHeight(document.getRowHeight() * scaleFactor);
			g.setColour(Colour(breaklineDragger != nullptr ? SIGNAL_COLOUR : HISE_BREAKPOINT_COLOUR).withAlpha(0.05f));
			g.fillRect(b.withWidth(getWidth()));

			Path arrow = createArrow();

			b = b.removeFromLeft(getGutterWidth());
			b = b.removeFromRight(b.getHeight()).reduced(JUCE_LIVE_CONSTANT_OFF(6.0f)* scaleFactor).translated(2.0f, 0.0f);

			g.setColour(breaklineDragger != nullptr ? Colour(SIGNAL_COLOUR) : Colour(HISE_BREAKPOINT_COLOUR));
			PathFactory::scalePath(arrow, b);
			g.fillPath(arrow);
			g.setColour(Colours::white.withAlpha(0.9f));
			g.strokePath(arrow, PathStrokeType(1.0f));
		}
	}
	
	blinkHandler.draw(g, rowData);

}

GlyphArrangement mcl::GutterComponent::getLineNumberGlyphs(int row) const
{
	GlyphArrangement glyphs;
	glyphs.addLineOfText(document.getFont().withHeight(12.f),
		String(row + 1),
		8.f, document.getVerticalPosition(row, TextDocument::Metric::baseline));
	return glyphs;
}

bool mcl::GutterComponent::hitTest(int x, int y)
{
	return x < getGutterWidth();
}

juce::Rectangle<float> mcl::GutterComponent::getRowBounds(const TextDocument::RowData& r) const
{
	if (r.bounds.getNumRectangles() == 1)
	{
		auto b = r.bounds.getRectangle(0);
		b.removeFromBottom(2.6f);

		b = b
			.transformedBy(transform)
			.withX(0)
			.withWidth(getGutterWidth());

		return b;
	}
	else
	{
		

		auto y = r.bounds.getRectangle(0).getY();

		auto numLines = r.bounds.getNumRectangles();

		auto h = document.getFontHeight() * (numLines - 1) + document.getRowHeight();
		

		auto x = 0.0;

		Rectangle<float> s(x, y, 0, h);
		return s.transformedBy(transform).withX(0).withWidth(getGutterWidth());
	}
}

void mcl::GutterComponent::mouseDown(const MouseEvent& e)
{
	auto delta = getGutterWidth() - (float)e.getMouseDownX();

	delta /= scaleFactor;

	if (delta > 18.0f)
	{
		if (e.mods.isCommandDown())
		{
			if (owner != nullptr)
				owner->clearBreakpoints(sendNotificationAsync);

			return;
		}
		else if(e.mods.isRightButtonDown())
		{
			PopupMenu m;
			GlobalHiseLookAndFeel hlaf;
			m.setLookAndFeel(&hlaf);

			auto bp = getBreakpoint(hoveredData.rowNumber);

			if (bp != nullptr)
			{
				m.addItem(1, bp->enabled.getValue() ? "Disable Breakpoint" : "Enable Breakpoint");
				m.addItem(2, "Edit breakpoint");
				m.addItem(5, "Show injected code");
				m.addSeparator();
				
				
			}					

			m.addItem(3, "Delete all breakpoints", !breakpoints.isEmpty());
			m.addItem(4, "Recompile when breakpoints change", true, recompileOnBreakpointChange);
			m.addItem(6, "Create profiling breakpoints", true, createProfilingBreakpoints);

			auto r = m.show();

			auto calloutBounds = getRowBounds(hoveredData).toNearestInt();
			calloutBounds.setWidth(calloutBounds.getHeight());
			
			if (r == 1)
			{
				bp->enabled.setValue(!bp->enabled.getValue());
				repaint();
			}
			if (r == 2)
			{
				struct Popup : public Component,
							   public juce::TextEditor::Listener,
							   public Value::Listener
				{
					Popup(Breakpoint::Ptr bp):
						p(bp),
						useCondition("Use Condition"),
						breakButton("Break when hit"),
						blinkButton("Blink when hit")
					{
						setLookAndFeel(&laf);
						laf.setDefaultSansSerifTypeface(GLOBAL_BOLD_FONT().getTypefacePtr());

						bp->useCondition.addListener(this);

						setup(conditionEditor, bp->condition);
						setup(useCondition, bp->useCondition);
						setup(logExpressionEditor, bp->logExpression);
						setup(breakButton, bp->breakIfHit);
						setup(blinkButton, bp->blinkIfHit);

						blinkButton.setVisible(!bp->profiling);
						breakButton.setVisible(!bp->profiling);

						setSize(300, 30 + 5 * 28 + 8);
					}

					~Popup()
					{
						p->useCondition.removeListener(this);
					}

					void setup(ToggleButton& b, Value& v)
					{
						b.getToggleStateValue().referTo(v);
						
						addAndMakeVisible(b);
					}

					void valueChanged(Value& value) override
					{
						auto on = useCondition.getToggleState();
						conditionEditor.setColour(juce::TextEditor::ColourIds::textColourId, on ? Colours::white : Colours::grey);

						conditionEditor.setText(conditionEditor.getText(), dontSendNotification);
						conditionEditor.setEnabled(on);
					}

					void setup(juce::TextEditor& t, Value& v)
					{
						t.addListener(this);
						t.setColour(juce::TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
						t.setColour(juce::TextEditor::ColourIds::textColourId, Colours::white);
						t.setColour(juce::TextEditor::ColourIds::highlightedTextColourId, Colours::white);
						t.setColour(juce::TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.4f));
						t.setColour(juce::TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR).withAlpha(0.8f));
						t.setColour(juce::CaretComponent::ColourIds::caretColourId, Colours::white);
						
						t.setFont(GLOBAL_MONOSPACE_FONT());
						t.setText(v.toString(), dontSendNotification);
						t.setSelectAllWhenFocused(true);
						addAndMakeVisible(t);
					}

					void textEditorReturnKeyPressed(juce::TextEditor& t)
					{
						if (&t == &conditionEditor)
							p->condition.setValue(t.getText());
						else
							p->logExpression.setValue(t.getText());
						
						findParentComponentOfClass<CallOutBox>()->dismiss();
					}

					void resized() override
					{
						auto b = getLocalBounds().reduced(5);
						b.removeFromTop(20);
						
						conditionEditor.setBounds(b.removeFromTop(28));
						b.removeFromTop(8);
						useCondition.setBounds(b.removeFromTop(20));
						b.removeFromTop(18);
						logExpressionEditor.setBounds(b.removeFromTop(28));
						b.removeFromTop(8);

						auto bottom = b.removeFromTop(20);

						breakButton.setBounds(bottom.removeFromLeft(bottom.getWidth() / 2));
						blinkButton.setBounds(bottom);
						b.removeFromTop(8);

					}

					void paint(Graphics& g) override
					{
						auto b = getLocalBounds();
						auto title = b.removeFromTop(20).toFloat();
						g.setFont(GLOBAL_BOLD_FONT());
						g.setColour(Colours::white);
						g.drawText("Edit condition for breakpoint", title, Justification::centred);
					}

					LookAndFeel_V4 laf;

					Breakpoint::Ptr p;
					juce::TextEditor conditionEditor;
					
					juce::ToggleButton useCondition;
					juce::TextEditor logExpressionEditor;
					juce::ToggleButton breakButton;
					juce::ToggleButton blinkButton;
				};
                
                
                auto root = TopLevelWindowWithOptionalOpenGL::findRoot(this);
				auto& cb = CallOutBox::launchAsynchronously(std::unique_ptr<Component>(new Popup(bp)), root->getLocalArea(this, calloutBounds), root);

				cb.setColour(LookAndFeel_V4::ColourScheme::UIColour::widgetBackground, Colours::white);

				//cb.grabKeyboardFocus();
				return;
			}
			else if (r == 3)
			{
				if (owner != nullptr)
					owner->clearBreakpoints(sendNotification);
			}
			else if (r == 4)
			{
				recompileOnBreakpointChange = !recompileOnBreakpointChange;
			}
			else if (r == 6)
			{
				createProfilingBreakpoints = !createProfilingBreakpoints;
			}
			else if (r == 5)
			{
				auto line = bp->processLine("");
				AlertWindowLookAndFeel alaf;

				auto t = new juce::TextEditor();
				t->setFont(GLOBAL_MONOSPACE_FONT());
				t->setColour(juce::TextEditor::ColourIds::backgroundColourId, Colours::transparentBlack);
				t->setColour(juce::TextEditor::ColourIds::textColourId, Colours::white);
				t->setColour(juce::TextEditor::ColourIds::highlightedTextColourId, Colours::white);
				t->setColour(juce::TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.4f));
				t->setColour(juce::TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR).withAlpha(0.8f));
				t->setColour(juce::CaretComponent::ColourIds::caretColourId, Colours::white);
				t->setSize(GLOBAL_MONOSPACE_FONT().getStringWidth(line) + 20.0f, 24.0f);
				t->setText(line, dontSendNotification);
				t->setReadOnly(true);
                
                auto root = TopLevelWindowWithOptionalOpenGL::findRoot(this);
                CallOutBox::launchAsynchronously(std::unique_ptr<Component>(t), root->getLocalArea(this, calloutBounds), root);
			}
			
		}
		else if (owner != nullptr)
		{
			
			auto found = false;

			for (int i = 0; i < breakpoints.size(); i++)
			{
				if (*breakpoints[i] == hoveredData.rowNumber)
				{
					owner->removeUriBreakpoint(breakpoints[i]->toUriBreakpoint(), sendNotification);
					found = true;
					break;
				}
			}

			if(!found)
			{
				UriBreakpoint np;
				
				auto lm = findParentComponentOfClass<TextEditor>()->getLanguageManager();
				auto newPos = lm->createUriLocation(CodeDocument::Position(document.getCodeDocument(), hoveredData.rowNumber, 0));
				np.fillLocation(newPos);

				if (e.mods.isShiftDown())
				{
					np.breakOnHit = false;
					np.blinkOnHit = true;
				}

				auto result = owner->addUriBreakpoint(np, sendNotification);

				if (result.first == UriBreakpoint::Owner::AttachState::Duplicate)
				{
					breakpoints.add(new Breakpoint(*this, result.second, document.getCodeDocument()));
				}
			}
		}

		findParentComponentOfClass<mcl::TextEditor>()->translateView(0.0f, 0.0f);

		repaint();
		return;
	}

	if (owner != nullptr && isBreakLineActive() && hoveredData.rowNumber == currentBreakLine.getLineNumber())
	{
		breaklineDragger = new BreaklineDragger(*this);
		breaklineDragger->startDragging(e);
		return;
	}


	document.getFoldableLineRangeHolder().toggleFoldState(hoveredData.rowNumber);
}

void GutterComponent::mouseDrag(const MouseEvent& e)
{
	if (breaklineDragger != nullptr)
		breaklineDragger->dragInstructionMarker(e);
}

void GutterComponent::mouseUp(const MouseEvent & e)
{
	breaklineDragger = nullptr;
}

void mcl::GutterComponent::sendBlinkMessage(int n)
{
	blinkHandler.addBlinkState(n);
}


GutterComponent::BreaklineDragger::BreaklineDragger(GutterComponent& g) :
	parent(g)
{}

GutterComponent::BreaklineDragger::~BreaklineDragger()
{
	UriBreakpoint newLocation;
	newLocation.fillLocation(parent.currentBreakPosition);
	newLocation.fillLocation(parent.currentBreakLine);
	
	auto resolvedPos = parent.owner->moveInstructionPointer(newLocation);

	if ((bool)resolvedPos)
	{
		parent.currentBreakPosition.fillLocation(resolvedPos);
		parent.setCurrentBreakline(parent.currentBreakPosition, parent.currentBreakpointScope);
	}
	else
	{
		parent.currentBreakPosition.fillLocation(originalLocation);
		parent.setCurrentBreakline(parent.currentBreakPosition, parent.currentBreakpointScope);
	}
}

void GutterComponent::BreaklineDragger::startDragging(const MouseEvent& e)
{
	originalLocation.fillLocation(parent.currentBreakPosition);
	parent.repaint();
}

void GutterComponent::BreaklineDragger::dragInstructionMarker(const MouseEvent& e)
{
	auto pos = e.position.transformedBy(parent.transform.inverted());
	auto newPos = parent.document.findIndexNearestPosition(pos);

	auto lr = parent.currentBreakpointScope->getLineRange();
	lr -= 1;
	
	auto l = lr.clipValue(newPos.x);

	parent.currentBreakLine.setLineAndIndex(l, 0);
	parent.repaint();
}

}
