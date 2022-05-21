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



//==========================================================================
mcl::GutterComponent::GutterComponent(TextDocument& document)
	: document(document)
	, memoizedGlyphArrangements([this](int row) { return getLineNumberGlyphs(row); })
	, blinkHandler(*this)
{
	setRepaintsOnMouseActivity(true);

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
		bool isErrorLine = r.rowNumber == errorLine;

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


		if ((r.isRowSelected || isErrorLine) && !showFoldRange)
		{
			auto b2 = b.withHeight(jmax(b.getHeight(), document.getRowHeight() * scaleFactor));

			g.setColour(ln.contrasting(0.1f));
			g.fillRect(b2);
		}

		auto lfb = b;

		lfb = lfb.removeFromRight(15 * transform.getScaleFactor());

		g.setColour(getParentComponent()->findColour(CodeEditorComponent::lineNumberTextId).withBrightness(0.35f));

		

		switch (t)
		{
		case FoldableLineRange::Holder::RangeStartOpen:
		case FoldableLineRange::Holder::RangeStartClosed:
		{
			g.setColour(getParentComponent()->findColour(CodeEditorComponent::lineNumberTextId).withBrightness(0.35f));

			auto w = lfb.getWidth() - 4.0f * transform.getScaleFactor();
			auto box = lfb.withSizeKeepingCentre(w, w);

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

	
	if (currentBreakLine != CodeDocument::Position())
	{
		if (auto r = getRowData(currentBreakLine.getLineNumber()-1))
		{
			auto b = getRowBounds(*r).withHeight(document.getRowHeight() * scaleFactor);
			g.setColour(Colours::red.withAlpha(0.05f));
			g.fillRect(b.withWidth(getWidth()));
		}
	}

	for (auto bp : breakpoints)
	{
		if (auto r = getRowData(bp->getLineNumber()))
		{
			auto b = getRowBounds(*r).withHeight(document.getRowHeight() * scaleFactor);

			b = b.withWidth(b.getHeight()).reduced(JUCE_LIVE_CONSTANT_OFF(6.0f) * scaleFactor);

			auto t = h.getLineType(*bp);

			if (t == FoldableLineRange::Holder::LineType::Folded)
				continue;

			auto bPath = bp->createPath();
			PathFactory::scalePath(bPath, b);

			if (bp->enabled.getValue() && breakpointsEnabled)
			{
				g.setColour(bp->breakIfHit.getValue() ? Colour(0xFF683333) : Colour(0xFF333368));
				g.fillPath(bPath);
			}

			g.setColour(Colours::white.withAlpha(0.4f));
			g.strokePath(bPath, PathStrokeType(1.0f));

			if (*bp == currentBreakLine.getLineNumber() - 1)
			{
				g.strokePath(bPath, PathStrokeType(1.0f));
				g.setColour(Colours::white);

				Path arrow = createArrow();
				PathFactory::scalePath(arrow, b.reduced(2.0f * scaleFactor));
				g.fillPath(arrow);
			}
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
		if (e.mods.isCommandDown() || e.mods.isShiftDown())
		{
			breakpoints.clear();
		}
		else
		{
			if (auto bp = getBreakpoint(hoveredData.rowNumber))
			{
				if (e.mods.isRightButtonDown())
				{
					PopupMenu m;
					GlobalHiseLookAndFeel hlaf;
					m.setLookAndFeel(&hlaf);

					m.addItem(1, bp->enabled.getValue() ? "Disable Breakpoint" : "Enable Breakpoint");
					m.addItem(2, "Edit breakpoint");
					m.addItem(5, "Show injected code");
					m.addSeparator();
					m.addItem(3, "Delete all breakpoints");
					m.addItem(4, "Recompile when breakpoints change", true, recompileOnBreakpointChange);
					

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
                        
                        
                        auto root = getTopLevelComponent();
						auto& cb = CallOutBox::launchAsynchronously(std::unique_ptr<Component>(new Popup(bp)), root->getLocalArea(this, calloutBounds), root);

						cb.setColour(LookAndFeel_V4::ColourScheme::UIColour::widgetBackground, Colours::white);

						//cb.grabKeyboardFocus();
						return;
					}
					else if (r == 3)
					{
						breakpoints.clear();
					}
					else if (r == 4)
					{
						recompileOnBreakpointChange = !recompileOnBreakpointChange;
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
                        
                        auto root = getTopLevelComponent();
                        CallOutBox::launchAsynchronously(std::unique_ptr<Component>(t), root->getLocalArea(this, calloutBounds), root);
					}
				}
				else
				{
					for (int i = 0; i < breakpoints.size(); i++)
					{
						if (*breakpoints[i] == hoveredData.rowNumber)
							breakpoints.remove(i--);
					}
				}
			}
			else
				breakpoints.add(new Breakpoint(this, hoveredData.rowNumber, document.getCodeDocument()));

			sendBreakpointChangeMessage();
		}

		findParentComponentOfClass<mcl::TextEditor>()->translateView(0.0f, 0.0f);

		repaint();
		return;
	}

	document.getFoldableLineRangeHolder().toggleFoldState(hoveredData.rowNumber);
}

void mcl::GutterComponent::sendBlinkMessage(int n)
{
	blinkHandler.addBlinkState(n);
}

}
