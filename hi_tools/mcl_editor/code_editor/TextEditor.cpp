/** ============================================================================
 *
 * TextEditor.cpp
 *
 * Copyright (C) Jonathan Zrake
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


namespace mcl
{

using namespace juce;


//==============================================================================
mcl::TextEditor::TextEditor(CodeDocument& codeDoc)
: document(codeDoc)
, caret (document)
, gutter (document)
, linebreakDisplay(document)
, map(document, new CPlusPlusCodeTokeniser())
, highlight (document)
, docRef(codeDoc)
, scrollBar(true)
, tokenCollection()
, treeview(document)
, foldMap(document)
, tooltipManager(*this)
, plaf(new LookAndFeel_V3())
{
	tokenCollection.addTokenProvider(new SimpleDocumentTokenProvider(codeDoc));

	document.addSelectionListener(this);
	startTimer(1000);

    lastTransactionTime = Time::getApproximateMillisecondCounter();
    document.setSelections ({ Selection() });
	docRef.addListener(this);
	addAndMakeVisible(scrollBar);
	
	addAndMakeVisible(treeview);

	docRef.removeListener(&document);
	docRef.addListener(&document);
	
	scrollBar.addListener(this);
	scrollBar.setColour(ScrollBar::ColourIds::thumbColourId, Colours::white.withAlpha(0.2f));
    setFont (Font(Font::getDefaultMonospacedFontName(), 16.0f, Font::plain));

    translateView (gutter.getGutterWidth(), 0); 
    setWantsKeyboardFocus (true);

	addAndMakeVisible(linebreakDisplay);
    addAndMakeVisible (highlight);
	addAndMakeVisible(foldMap);
    addAndMakeVisible (caret);
    addAndMakeVisible (gutter);
	addAndMakeVisible(map);

	setOpaque(true);

	struct Type
	{
		String name;
		uint32 colour;
	};

	const Type types[] =
	{
		{ "Error", 0xffBB3333 },
		{ "Comment", 0xff77CC77 },
		{ "Keyword", 0xffbbbbff },
		{ "Operator", 0xffCCCCCC },
		{ "Identifier", 0xffDDDDFF },
		{ "Integer", 0xffDDAADD },
		{ "Float", 0xffEEAA00 },
		{ "String", 0xffDDAAAA },
		{ "Bracket", 0xffFFFFFF },
		{ "Punctuation", 0xffCCCCCC },
		{ "Preprocessor Text", 0xffCC7777 },
		{ "Deactivated", 0xFF666666 }
	};

	for (unsigned int i = 0; i < sizeof(types) / sizeof(types[0]); ++i)  // (NB: numElementsInArray doesn't work here in GCC4.2)
		colourScheme.set(types[i].name, Colour(types[i].colour));


	setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xFF606060));

	map.colourScheme = colourScheme;
}

mcl::TextEditor::~TextEditor()
{
	docRef.removeListener(this);
}

void mcl::TextEditor::setFont (Font font)
{
    document.setFont (font);
    repaint();
}

void mcl::TextEditor::setText (const String& text)
{
    document.replaceAll (text);
    repaint();
}

void TextEditor::scrollToLine(float centerLine, bool scrollToLine)
{
	auto W = document.getBounds().getWidth();
	auto H = document.getBounds().getHeight();

	centerLine -= (float)getNumDisplayedRows() / 2.0f;

	auto y = document.getBoundsOnRow(centerLine, { 0, 1 }, GlyphArrangementArray::ReturnLastCharacter).getRectangle(0).getY();

	if (scrollToLine)
		y = (float)roundToInt(y);

	if (translation.y != -y)
	{
		translation.y = jlimit(jmin(0.f, -H + getHeight()/viewScaleFactor), 0.0f, -y) * viewScaleFactor;

		updateViewTransform();
	}
}

int TextEditor::getNumDisplayedRows() const
{
	return roundToInt((float)getHeight() / viewScaleFactor / document.getRowHeight());
}

void TextEditor::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	auto b = document.getBounds();
	
	auto pos = newRangeStart;

	translation.y = jlimit<float>(-b.getHeight(), 0.0f, -pos);
	updateViewTransform();
}

void TextEditor::closeAutocomplete(bool async, const String& textToInsert, Array<Range<int>> selectRanges)
{
	if (currentAutoComplete != nullptr)
	{
		auto f = [this, textToInsert, selectRanges]()
		{
			removeKeyListener(currentAutoComplete);

			Desktop::getInstance().getAnimator().fadeOut(currentAutoComplete, 300);

			currentAutoComplete = nullptr;

			if (textToInsert.isNotEmpty())
			{
				ScopedValueSetter<bool> svs(skipTextUpdate, true);
				document.setSelections({ autocompleteSelection });

				insert(textToInsert);

				auto s = document.getSelection(0).oriented();
				CodeDocument::Position insertStart(document.getCodeDocument(), s.tail.x, s.tail.y);

				

				auto l = textToInsert.length();

				if (currentParameterSelection.size() == 0)
					currentParameter = nullptr;

				if (currentParameter == nullptr)
				{
					clearParameters();

					if (!selectRanges.isEmpty())
					{
						for (auto sr : selectRanges)
						{
							auto copy = insertStart;

							auto ps = new Autocomplete::ParameterSelection(document,
								insertStart.getPosition() - l + sr.getStart(),
								insertStart.getPosition() - l + sr.getEnd());

							currentParameterSelection.add(ps);
						}
					}

					if (!currentParameterSelection.isEmpty())
						setParameterSelection(0);

					postParameterPos = insertStart;
					postParameterPos.setPositionMaintained(true);
				}
				
				if (currentParameter != nullptr)
				{
					document.setSelections({ currentParameter->getSelection() });
				}
			}

			autocompleteSelection = {};
		};

		if (async)
			MessageManager::callAsync(f);
		else
			f();
	}

	repaint();
}

void TextEditor::selectionChanged()
{
	highlight.updateSelections();
	caret.updateSelections();
	gutter.updateSelections();

	auto s = document.getSelections().getFirst();

	auto& doc = document.getCodeDocument();
	CodeDocument::Position pos(doc, s.head.x, s.head.y);
	pos.moveBy(-1);
	auto r = pos.getCharacter();

	highlight.setVisible(currentParameter == nullptr);

	if (ActionHelpers::isRightClosure(r))
	{
		int numToSkip = 0;

		while (pos.getPosition() > 0)
		{
			pos.moveBy(-1);

			auto l = pos.getCharacter();

			if (l == r)
			{
				numToSkip++;
			}

			if (ActionHelpers::isMatchingClosure(l, r))
			{
				numToSkip--;

				if (numToSkip < 0)
				{
					currentClosure[0] = { pos.getLineNumber(), pos.getIndexInLine() + 1, pos.getLineNumber(), pos.getIndexInLine() + 1 };
					currentClosure[1] = s;
					showClosures = true;
					return;
				}
			}
		}

		currentClosure[0] = {};
		currentClosure[1] = s;
		showClosures = true;
		return;
	}

	currentClosure[0] = {};
	currentClosure[1] = {};
	showClosures = false;
}

void mcl::TextEditor::translateView (float dx, float dy)
{
    auto W = viewScaleFactor * document.getBounds().getWidth();
    auto H = viewScaleFactor * document.getBounds().getHeight();

    translation.x = jlimit (jmin (gutter.getGutterWidth(), -W + getWidth()), gutter.getGutterWidth(), translation.x + dx);
    translation.y = jlimit (jmin (-0.f, -H + getHeight()), 0.0f, translation.y + dy);

    updateViewTransform();
}

void mcl::TextEditor::scaleView (float scaleFactorMultiplier, float verticalCenter)
{
    viewScaleFactor = jlimit(0.5f, 4.0f, viewScaleFactor * scaleFactorMultiplier);
	gutter.setScaleFactor(viewScaleFactor);
	
	

	refreshLineWidth();
	//translateView(0.0f, 0.0f);
	
}

void mcl::TextEditor::updateViewTransform()
{
	transform = AffineTransform::scale(viewScaleFactor).translated(translation.x, translation.y);
	highlight.setViewTransform(transform);
	caret.setViewTransform(transform);
	gutter.setViewTransform(transform);
	linebreakDisplay.setViewTransform(transform);
	
	if (currentAutoComplete != nullptr)
	{
		currentAutoComplete->setTransform(transform);

		if (currentAutoComplete->helpPopup != nullptr)
			currentAutoComplete->helpPopup->setTransform(transform);
	}

	auto visibleRange = getLocalBounds().toFloat().transformed(transform.inverted());
	scrollBar.setCurrentRange({ visibleRange.getY(), visibleRange.getBottom() }, dontSendNotification);

	auto rows = document.getRangeOfRowsIntersecting(getLocalBounds().toFloat().transformed(transform.inverted()));

	map.setVisibleRange(rows);

    repaint();
}

void mcl::TextEditor::updateSelections()
{
    
}

void mcl::TextEditor::translateToEnsureCaretIsVisible()
{
    auto i = document.getSelections().getLast().head;
    auto t = Point<float> (0.f, document.getVerticalPosition (i.x, TextDocument::Metric::top))   .transformedBy (transform);
    auto b = Point<float> (0.f, document.getVerticalPosition (i.x, TextDocument::Metric::bottom)).transformedBy (transform);

    if (t.y < 0.f)
    {
        translateView (0.f, -t.y);
    }
    else if (b.y > getHeight())
    {
        translateView (0.f, -b.y + getHeight());
    }

	if (document.getFoldableLineRangeHolder().isFolded(i.x))
	{
		document.getFoldableLineRangeHolder().unfold(i.x);
	}
}


void mcl::TextEditor::resized()
{
	auto b = getLocalBounds();
	
	
	
	
	scrollBar.setBounds(b.removeFromRight(14));

	if(map.isVisible())
		map.setBounds(b.removeFromRight(150));

	foldMap.setBounds(b.removeFromRight(250));

	if(treeview.isVisible())
		treeview.setBounds(b.removeFromRight(250));

	linebreakDisplay.setBounds(b.removeFromRight(15));

	maxLinesToShow = b.getWidth() - TEXT_INDENT - 10;

	refreshLineWidth();
	
    highlight.setBounds (b);
    caret.setBounds (b);
    gutter.setBounds (b);
    resetProfilingData();
}

void mcl::TextEditor::paint (Graphics& g)
{
    auto start = Time::getMillisecondCounterHiRes();
    

    String renderSchemeString;

	renderTextUsingGlyphArrangement(g);

    lastTimeInPaint = Time::getMillisecondCounterHiRes() - start;
    accumulatedTimeInPaint += lastTimeInPaint;
    numPaintCalls += 1;

    if (drawProfilingInfo)
    {
        String info;
        info += "paint mode         : " + renderSchemeString + "\n";
        info += "cache glyph bounds : " + String (document.lines.cacheGlyphArrangement ? "yes" : "no") + "\n";
        info += "core graphics      : " + String (allowCoreGraphics ? "yes" : "no") + "\n";
        info += "opengl             : " + String (useOpenGLRendering ? "yes" : "no") + "\n";
        info += "syntax highlight   : " + String (enableSyntaxHighlighting ? "yes" : "no") + "\n";
        info += "mean render time   : " + String (accumulatedTimeInPaint / numPaintCalls) + " ms\n";
        info += "last render time   : " + String (lastTimeInPaint) + " ms\n";
        info += "tokeniser time     : " + String (lastTokeniserTime) + " ms\n";

        g.setColour (findColour (CodeEditorComponent::defaultTextColourId));
        g.setFont (Font ("Courier New", 12, 0));
        g.drawMultiLineText (info, getWidth() - 280, 10, 280);
    }

	g.setColour(Colours::blue.withAlpha(0.8f));

	for (auto arg : currentParameterSelection)
	{
		arg->rebuildPosition(document, transform);
		arg->draw(g, currentParameter);
	}

	if (showClosures && document.getSelection(0).isSingular())
	{
		bool ok = !(currentClosure[0] == Selection());

		auto rect = [this](const Selection& s)
		{
			auto p = s.head;
			auto l = document.getBoundsOnRow(p.x, { p.y - 1, p.y }, GlyphArrangementArray::ReturnLastCharacter);
			auto r = l.getRectangle(0);
			return r.transformed(transform).expanded(1.0f);
		};

		if (ok)
		{
			g.setColour(findColour(CodeEditorComponent::defaultTextColourId).withAlpha(0.3f));
			g.drawRoundedRectangle(rect(currentClosure[0]), 2.0f, 1.0f);
			g.drawRoundedRectangle(rect(currentClosure[1]), 2.0f, 1.0f);
		}
		else
		{
			g.setColour(Colours::red.withAlpha(0.5f));
			g.drawRoundedRectangle(rect(currentClosure[1]), 2.0f, 1.0f);
		}
	}

	if (currentError != nullptr)
		currentError->paintLines(g, transform, Colours::red);

	for (auto w : warnings)
		w->paintLines(g, transform, Colours::yellow);

	if (currentAutoComplete != nullptr)
	{
		auto area = currentAutoComplete->getBounds().toFloat();
		area = area.transformed(transform).expanded(2.0f);
		
		g.setColour(Colours::black.withAlpha(0.1f));
		area = area.translated(0.3f, 1.0f);
		g.fillRect(area);
		area = area.translated(0.3f, 1.0f);
		g.fillRect(area);
		area = area.translated(0.3f, 1.0f);
		g.fillRect(area);
		area = area.translated(0.3f, 1.0f);
		g.fillRect(area);
	}

#if PROFILE_PAINTS
    std::cout << "[TextEditor::paint] " << lastTimeInPaint << std::endl;
#endif
}

void mcl::TextEditor::paintOverChildren (Graphics& g)
{
}

void mcl::TextEditor::mouseDown (const MouseEvent& e)
{
	closeAutocomplete(true, {}, {});

	for (auto ps : currentParameterSelection)
	{
		if (ps->p.getBounds().contains(e.getPosition().toFloat()))
		{
			setParameterSelection(currentParameterSelection.indexOf(ps));
			return;
		}
	}

	currentParameter = nullptr;
	auto index = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));

    if (e.getNumberOfClicks() > 1)
    {
        return;
    }
    else if (e.mods.isRightButtonDown())
    {
        PopupMenu menu;
		menu.setLookAndFeel(plaf);


		menu.addSeparator();
		menu.addSectionHeader("View options");
		menu.addItem(8, "Fold all", true, false);
		menu.addItem(9, "Unfold all", true, false);
		menu.addItem(7, "Goto definition", true, false);

		menu.addSeparator();



		menu.addItem(10, "Enable line breaks", true, linebreakEnabled);
		menu.addItem(11, "Enable code map", true, map.isVisible());
		menu.addItem(12, "Enable fold map", true, foldMap.isVisible());


		

		menu.addSectionHeader("Language options");
		

		menu.addItem(13, "Background C++ parsing", true, enableLiveParsing);
		menu.addItem(14, "Parse preprocessor conditions", true, enablePreprocessorParsing);

		auto result = menu.show();

        switch (result)
        {
			case 8: 
			case 9: 
			{
				for (auto l : document.getFoldableLineRangeHolder().all)
					l->setFolded(result == 8);

				document.getFoldableLineRangeHolder().updateFoldState(nullptr);
				break;
			}
			case 7: 
			{

				gotoDefinition({index, index}); break;
			}
			case 10: linebreakEnabled = !linebreakEnabled; refreshLineWidth(); break;
			case 11: map.setVisible(!map.isVisible()); resized(); break;
			case 13: enableLiveParsing = !enableLiveParsing; break;
			case 14: enablePreprocessorParsing = !enablePreprocessorParsing; break;
        }

        resetProfilingData();
        repaint();
        return;
    }

    auto selections = document.getSelections();
    

	if (e.mods.isShiftDown())
	{
		auto s = document.getSelection(0).head;

		Selection ns(index, s);

		document.setSelections({ ns }, false);
		updateSelections();
		return;
	}

    if (selections.contains (index))
    {
        return;
    }
    if (! e.mods.isCommandDown() || ! TEST_MULTI_CARET_EDITING)
    {
        selections.clear();
    }

    selections.add (index);
    document.setSelections (selections, false);
}

void mcl::TextEditor::mouseDrag (const MouseEvent& e)
{
    if (e.mouseWasDraggedSinceMouseDown())
    {
		if (e.mods.isAltDown())
		{
			auto start = document.findIndexNearestPosition(e.mouseDownPosition.transformedBy(transform.inverted()));
			auto current = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));

			Range<int> lineRange = { start.x, current.x + 1 };

			Array<Selection> multiLineSelections;

			for (int i = lineRange.getStart(); i < lineRange.getEnd(); i++)
			{
				multiLineSelections.add({ i, current.y, i, start.y});
			}

			document.setSelections(multiLineSelections);
			updateSelections();
		}
		else
		{
			auto selection = document.getSelections().getFirst();
			selection.head = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));
			document.setSelections({ selection });
			translateToEnsureCaretIsVisible();
			updateSelections();
		}
    }
}

void mcl::TextEditor::mouseDoubleClick (const MouseEvent& e)
{
    if (e.getNumberOfClicks() == 2)
    {
        document.navigateSelections (TextDocument::Target::subword, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (TextDocument::Target::subword, TextDocument::Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
    }
    else if (e.getNumberOfClicks() == 3)
    {
        document.navigateSelections (TextDocument::Target::line, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (TextDocument::Target::line, TextDocument::Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
    }
    updateSelections();
}

void mcl::TextEditor::mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& d)
{
	float dx = d.deltaX;

	if (e.mods.isCommandDown())
	{
		auto factor = 1.0f + (float)d.deltaY / 5.0f;

		scaleView(factor, 0.0f);
		return;
	}

#if JUCE_WINDOWS

	translateView(dx * 80, d.deltaY * 160);

#else
    /*
     make scrolling away from the gutter just a little "sticky"
     */
    if (translation.x == gutter.getGutterWidth() && -0.01f < dx && dx < 0.f)
    {
        dx = 0.f;
    }
    translateView (dx * 400, d.deltaY * 800);
#endif
}

void mcl::TextEditor::mouseMagnify (const MouseEvent& e, float scaleFactor)
{
    scaleView (scaleFactor, e.position.y);
}


bool mcl::TextEditor::keyPressed (const KeyPress& key)
{
	startTimer(1000);

	tooltipManager.clearDisplay();

    // =======================================================================================
    using Target     = TextDocument::Target;
    using Direction  = TextDocument::Direction;
    auto mods        = key.getModifiers();
    auto isTab       = tabKeyUsed && (key.getTextCharacter() == '\t');


    // =======================================================================================
    auto nav = [this, mods] (Target target, Direction direction)
    {
		lastInsertWasDouble = false;

		if (mods.isShiftDown())
            document.navigateSelections (target, direction, Selection::Part::head);
        else
            document.navigateSelections (target, direction, Selection::Part::both);

        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto expandBack = [this, mods] (Target target, Direction direction)
    {
        document.navigateSelections (target, direction, Selection::Part::head);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };

	auto skipIfClosure = [this](juce_wchar c)
	{
		if (ActionHelpers::isRightClosure(c))
		{
			auto s = document.getSelections().getFirst();
			auto e = document.getCharacter(s.head);
			
			if (e == c)
			{
				document.navigateSelections(Target::character, Direction::forwardCol, Selection::Part::both);
				updateSelections();
				return true;
			}
		}

		

		insert(String::charToString(c));
		return true;
	};

	auto insertIfNotOpen = [this](juce_wchar openChar, juce_wchar closeChar)
	{
		auto s = document.getSelection(0);

		CodeDocument::Position pos(document.getCodeDocument(), s.tail.x, s.tail.y);

		int numBefore = 0;

		auto sPos = pos;

		while (sPos.getPosition() < document.getCodeDocument().getNumCharacters())
		{
			sPos = sPos.movedBy(1);

			auto c = sPos.getCharacter();

			if (c == openChar)
				numBefore++;
			else if (c == closeChar)
				numBefore--;
		}

		auto ePos = pos;
		int numAfter = 0;

		while (ePos.getPosition() > 0)
		{
			ePos = ePos.movedBy(-1);

			auto c = ePos.getCharacter();

			if (c == openChar)
				numAfter--;
			else if (c == closeChar)
				numAfter++;
		}
		

		juce::String text;
		text << openChar;

		auto both = numBefore == numAfter;

		if (both)
		{
			text << closeChar;
			
		}
			

		insert(text);

		if(both)
			lastInsertWasDouble = true;

		return both;
	};

	auto insertClosure = [this, insertIfNotOpen](juce_wchar c)
	{
		bool both = false;

		switch (c)
		{
		case '<': return insert("<");
		case '"': both = insertIfNotOpen('"', '"'); break;
		case '(': both = insertIfNotOpen('(', ')'); break;
		case '{': both = insertIfNotOpen('{', '}'); break;
		case '[': both = insertIfNotOpen('[', ']'); break;
		}

		if (both)
		{
			auto s = document.getSelection(0);
			document.navigateSelections(Target::character, Direction::backwardCol, Selection::Part::both);
			auto s2 = document.getSelection(0);
			updateSelections();
		}

		

		return true;
	};

    auto expand = [this, nav] (Target target)
    {
		document.navigateSelections(target, Direction::backwardCol, Selection::Part::tail);
		document.navigateSelections(target, Direction::forwardCol, Selection::Part::head);
		//translateToEnsureCaretIsVisible();
		updateSelections();
        return true;
    };

	auto insertTabAfterBracket = [this, mods]()
	{
		clearParameters();

		if (mods.isCommandDown())
		{
			auto s = document.getSelections().getLast();
			Point<int> end = s.head;
			Point<int> start = end;

			document.navigate(start, Target::line, Direction::backwardCol);
			document.navigate(end, Target::firstnonwhitespace, Direction::backwardCol);

			Selection emptyBeforeText(end, start);

			auto ws = document.getSelectionContent(emptyBeforeText);

			

			String textToInsert = "\n";
			textToInsert << ws << "{\n";
			textToInsert << ws << "\t\n";
			textToInsert << ws << "}";

			document.navigateSelections(Target::line, Direction::forwardCol, Selection::Part::both);

			insert(textToInsert);

			document.navigateSelections(Target::line, Direction::backwardCol, Selection::Part::both);
			document.navigateSelections(Target::character, Direction::backwardCol, Selection::Part::both);
			return true;
		}

		if (mods.isShiftDown())
			document.navigateSelections(Target::line, Direction::forwardCol, Selection::Part::both);

		auto s = document.getSelections().getLast();
		auto l = document.getCharacter(s.head.translated(0, -1));

		if (l == '{')
		{
			int numChars = s.head.y-1;

			juce::String ws = "\n\t";
			juce::String t = "\n";

			Point<int> end = s.head;
			Point<int> start = end;
			document.navigate(start, Target::line, Direction::backwardCol);
			document.navigate(end, Target::character, Direction::backwardCol);

			Selection emptyBeforeText(end, start);

			auto s = document.getSelectionContent(emptyBeforeText);

			ws << s;
			t << s;

			insert(ws);
			insert(t);
			document.navigateSelections(Target::line, Direction::backwardCol, Selection::Part::both);
			document.navigateSelections(Target::character, Direction::backwardCol, Selection::Part::both);
			return true;
		}
		else
		{
			
			CodeDocument::Position pos(document.getCodeDocument(), s.head.x, s.head.y);
			CodeDocument::Position lineStart(document.getCodeDocument(), s.head.x, 0);

			auto before = document.getCodeDocument().getTextBetween(lineStart, pos);
			auto trimmed = before.trimCharactersAtStart(" \t");

			auto delta = before.length() - trimmed.length();

			String s;

			if (mods.isShiftDown())
				s << ";";

			s << "\n";
			s << before.substring(0, delta);

			insert(s);
		}

		

		return true;
	};

	auto addNextTokenToSelection = [this]()
	{
		auto s = document.getSelections().getLast().oriented();
		
		CodeDocument::Position start(document.getCodeDocument(), s.head.x, s.head.y);
		CodeDocument::Position end(document.getCodeDocument(), s.tail.x, s.tail.y);

		auto t = document.getCodeDocument().getTextBetween(start, end);

		while (start.getPosition() < document.getCodeDocument().getNumCharacters())
		{
			start.moveBy(1);
			end.moveBy(1);

			auto current = document.getCodeDocument().getTextBetween(start, end);

			if (current == t)
			{
				Selection s(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());

				

				document.addSelection(s.swapped());
				translateToEnsureCaretIsVisible();
				updateSelections();
				return true;
			}
				
		}

		
		return true;
	};

    auto addCaret = [this] (Target target, Direction direction)
    {
        auto s = document.getSelections().getLast();
        document.navigate (s.head, target, direction);
        document.addSelection (s);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };
    auto addSelectionAtNextMatch = [this] ()
    {
        const auto& s = document.getSelections().getLast();

        if (! s.isSingleLine())
        {
            return false;
        }
        auto t = document.search (s.tail, document.getSelectionContent (s));

        if (t.isSingular())
        {
            return false;
        }
        document.addSelection (t);
        translateToEnsureCaretIsVisible();
        updateSelections();
        return true;
    };

	auto remove = [this, expand, expandBack](Target target, Direction direction)
	{
		const auto& s = document.getSelections().getLast();

		auto l = document.getCharacter(s.head.translated(0, -1));
		auto r = document.getCharacter(s.head);
		
		if (lastInsertWasDouble && ActionHelpers::isMatchingClosure(l, r))
		{
			document.navigateSelections(Target::character, Direction::backwardCol, Selection::Part::tail);
			document.navigateSelections(Target::character, Direction::forwardCol, Selection::Part::head);
			
			insert({});
			return true;
		}

		if (s.isSingular())
			expandBack(target, direction);

		insert({});
		return true;
	};

    // =======================================================================================
    if (key.isKeyCode (KeyPress::escapeKey))
    {
		clearParameters();

		bool doneSomething = false;

		for (auto& s : document.getSelections())
		{
			if (!s.isSingular())
			{
				s.tail = s.head;
				doneSomething = true;
			}
		}

		if (!doneSomething)
		{
			if (document.getNumSelections() == 1)
			{
				if (currentSearchBox != nullptr)
				{
					currentSearchBox = nullptr;
					return true;
				}

				updateAutocomplete(true);
				return true;
			}
			else
			{
				document.setSelections(document.getSelections().getLast());
			}

			
		}
			
        updateSelections();
        return true;
    }
    if (mods.isCtrlDown() && mods.isAltDown())
    {
        if (key.isKeyCode (KeyPress::downKey)) return addCaret (Target::character, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey  )) return addCaret (Target::character, Direction::backwardRow);
    }
    if (mods.isCtrlDown())
    {
        if (key.isKeyCode (KeyPress::rightKey)) return nav (Target::whitespace, Direction::forwardCol)  && nav (Target::word, Direction::forwardCol);
        if (key.isKeyCode (KeyPress::leftKey )) return nav (Target::whitespace, Direction::backwardCol) && nav (Target::word, Direction::backwardCol);
        if (key.isKeyCode (KeyPress::downKey )) return nav (Target::word, Direction::forwardCol)  && nav (Target::paragraph, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey   )) return nav (Target::word, Direction::backwardCol) && nav (Target::paragraph, Direction::backwardRow);

        if (key.isKeyCode (KeyPress::backspaceKey)) return (   expandBack (Target::whitespace, Direction::backwardCol)
                                                            && expandBack (Target::word, Direction::backwardCol)
                                                            && insert (""));

		if (key == KeyPress('e', ModifierKeys::ctrlModifier, 0) ||
			key == KeyPress('e', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
		{
			if (currentError != nullptr)
			{
				document.setSelections({ currentError->getSelection() });
				return true;
			}
		}
    }
    if (mods.isCommandDown())
    {
        if (key.isKeyCode (KeyPress::downKey)) return nav (Target::document, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey  )) return nav (Target::document, Direction::backwardRow);
    }



	if (key.isKeyCode(KeyPress::tabKey))
	{
		if (currentParameter != nullptr)
		{
			auto ok = incParameter();

			if (ok)
				return true;
		}

		auto s = document.getSelections().getFirst();

		if (s.head.x != s.tail.x)
		{
			CodeDocument::Position start(document.getCodeDocument(), s.head.x, s.head.y);
			CodeDocument::Position end(document.getCodeDocument(), s.tail.x, s.tail.y);

			start.setPositionMaintained(true);
			end.setPositionMaintained(true);

			s = s.oriented();

			Range<int> rows(s.head.x, s.tail.x + 1);

			Array<Selection> lineStarts;

			for (int i = rows.getStart(); i < rows.getEnd(); i++)
			{
				lineStarts.add(Selection(i, 0, i, 0));
			}

			if (mods.isShiftDown())
			{
				document.setSelections(lineStarts);
				document.navigateSelections(Target::character, Direction::forwardCol, Selection::Part::both);

				remove(Target::character, Direction::backwardCol);
			}
			else
			{
				document.setSelections(lineStarts);
				insert("\t");
			}

			

			Selection prev(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());

			document.setSelections({ prev });

			updateSelections();
			return true;
		}
	}

	if (key.isKeyCode(KeyPress::F12Key))
	{
		return gotoDefinition();

	}
	if (key.isKeyCode(KeyPress::rightKey)) return nav(Target::character, Direction::forwardCol);
	if (key.isKeyCode(KeyPress::leftKey)) return nav(Target::character, Direction::backwardCol);
	if (key.isKeyCode(KeyPress::downKey)) return nav(Target::character, Direction::forwardRow);
    if (key.isKeyCode (KeyPress::upKey   )) return nav (Target::character, Direction::backwardRow);

	if (key.isKeyCode(KeyPress::backspaceKey)) return remove(Target::character, Direction::backwardCol);
	if (key.isKeyCode(KeyPress::deleteKey))	   return remove(Target::character, Direction::forwardCol);

	if (key.isKeyCode(KeyPress::homeKey)) return nav(Target::firstnonwhitespace, Direction::backwardCol);
	if (key.isKeyCode(KeyPress::endKey))  return nav(Target::line, Direction::forwardCol);

    if (key == KeyPress ('a', ModifierKeys::commandModifier, 0)) return expand (Target::document);
	if (key == KeyPress('d', ModifierKeys::commandModifier, 0))  return addNextTokenToSelection();
    if (key == KeyPress ('l', ModifierKeys::commandModifier, 0)) return expand (Target::line);
    if (key == KeyPress ('u', ModifierKeys::commandModifier, 0)) return addSelectionAtNextMatch();
    if (key == KeyPress ('z', ModifierKeys::commandModifier, 0)) return document.getCodeDocument().getUndoManager().undo();
    if (key == KeyPress ('r', ModifierKeys::commandModifier, 0)) return document.getCodeDocument().getUndoManager().redo();

    if (key == KeyPress ('x', ModifierKeys::commandModifier, 0))
    {
		auto s = document.getSelections().getFirst();

		bool move = false;

		if (s.isSingular())
		{
			document.navigate(s.head, Target::line, Direction::backwardCol);
			document.navigate(s.head, Target::character, Direction::backwardCol);
			document.navigate(s.tail, Target::line, Direction::forwardCol);
			document.setSelection(0, s);
			move = true;
		}
		
		SystemClipboard::copyTextToClipboard(document.getSelectionContent(s));

		insert("");

		if (move)
		{
			nav(Target::character, Direction::forwardRow);
			nav(Target::firstnonwhitespace, Direction::backwardCol);
			
			
		}

		return true;
    }
    if (key == KeyPress ('c', ModifierKeys::commandModifier, 0))
    {
        SystemClipboard::copyTextToClipboard (document.getSelectionContent (document.getSelections().getFirst()));
        return true;
    }

	if (key == KeyPress('v', ModifierKeys::commandModifier, 0))
	{
		auto insertText = SystemClipboard::getTextFromClipboard();

		auto sel = document.getSelection(0);
		
		Point<int> p = sel.head;
		auto lineStart = p;
		document.navigate(lineStart, TextDocument::Target::firstnonwhitespace, TextDocument::Direction::backwardCol);

		auto prevws = document.getSelectionContent({ lineStart, p });

		

		if (!prevws.containsNonWhitespaceChars() && sel.isSingular())
		{
			auto sa = StringArray::fromLines(insertText);

			auto getWhitespaceAtBeginning = [](const String& s)
			{
				for (int numWhitespace = 0; numWhitespace < s.length(); numWhitespace++)
				{
					auto c = s[numWhitespace];

					if (!(c == ' ' || c == '\t'))
					{
						return s.substring(0, numWhitespace);
					}
				}

				return s;
			};

			auto whitespaceToRemove = getWhitespaceAtBeginning(sa[0]);

			if (whitespaceToRemove.isEmpty() && sa.size() > 1)
				whitespaceToRemove = getWhitespaceAtBeginning(sa[1]);
			
			bool first = true;

			for (auto& l : sa)
			{
				auto trimmed = whitespaceToRemove.isEmpty() ? l : l.fromFirstOccurrenceOf(whitespaceToRemove, false, false);

				if (first)
				{
					l = l.trimCharactersAtStart(" \t");
					first = false;
				}
				else
				{
					l = prevws + trimmed;
				}
			}


			insertText = sa.joinIntoString("\n");
		}
		

		return insert(insertText);
	}
	if (key == KeyPress('f', ModifierKeys::ctrlModifier, 0))
	{
		currentSearchBox = new SearchBoxComponent(document, transform.getScaleFactor());
		addAndMakeVisible(currentSearchBox);
		currentSearchBox->addListener(this);
		currentSearchBox->setBounds(getLocalBounds().removeFromBottom(document.getRowHeight() * transform.getScaleFactor() * 1.2f + 5));

		currentSearchBox->grabKeyboardFocus();

		return true;
	}
    if (key == KeyPress ('d', ModifierKeys::ctrlModifier, 0))      return insert (String::charToString (KeyPress::deleteKey));
    if (key.isKeyCode (KeyPress::returnKey))                       return insertTabAfterBracket();

	if(ActionHelpers::isLeftClosure(key.getTextCharacter()))   return insertClosure(key.getTextCharacter());
	if (ActionHelpers::isRightClosure(key.getTextCharacter())) return skipIfClosure(key.getTextCharacter());

    if (key.getTextCharacter() >= ' ' || isTab)                    return insert (String::charToString (key.getTextCharacter()));

    return false;
}

bool mcl::TextEditor::insert (const juce::String& content)
{
    double now = Time::getApproximateMillisecondCounter();

	if (currentParameter == nullptr)
	{
		currentParameterSelection.clear();
		repaint();
	}

    if (now > lastTransactionTime + 400)
    {
        lastTransactionTime = Time::getApproximateMillisecondCounter();
        
    }
    
    for (int n = 0; n < document.getNumSelections(); ++n)
    {
        Transaction t;
        t.content = content;
        t.selection = document.getSelection (n);
        
        auto callback = [this, n] (const Transaction& r)
        {
            switch (r.direction) // NB: switching on the direction of the reciprocal here
            {
                case Transaction::Direction::forward: document.setSelection (n, r.selection); break;
                case Transaction::Direction::reverse: document.setSelection (n, r.selection.tail); break;
            }

            if (! r.affectedArea.isEmpty())
            {
                repaint (r.affectedArea.transformedBy (transform).getSmallestIntegerContainer());
            }
        };

		ScopedPointer<UndoableAction> op = t.on(document, callback);
		op->perform();

    }
	
	translateToEnsureCaretIsVisible();
    updateSelections();

	lastInsertWasDouble = false;

    return true;
}

MouseCursor mcl::TextEditor::getMouseCursor()
{
    return getMouseXYRelative().x < gutter.getGutterWidth() ? MouseCursor::NormalCursor : MouseCursor::IBeamCursor;
}


void mcl::TextEditor::renderTextUsingGlyphArrangement (juce::Graphics& g)
{
	g.fillAll(findColour(CodeEditorComponent::backgroundColourId));

    g.saveState();
    g.addTransform (transform);

	highlight.paintHighlight(g);

    if (enableSyntaxHighlighting)
    {
        auto rows = document.getRangeOfRowsIntersecting (g.getClipBounds().toFloat());


		rows.setStart(jmax(0, rows.getStart() - 20));

        auto index = Point<int> (rows.getStart(), 0);
        

        auto it = TextDocument::Iterator (document, index);
        auto previous = it.getIndex();
        auto zones = Array<Selection>();
        auto start = Time::getMillisecondCounterHiRes();

        while (it.getIndex().x < rows.getEnd() && ! it.isEOF())
        {
            auto tokenType = CppTokeniserFunctions::readNextToken (it);
            zones.add (Selection (previous, it.getIndex()).withStyle (tokenType));
            previous = it.getIndex();
        }

		for (auto& z : zones)
		{
			if (deactivatesLines.contains(z.tail.x+1))
				z.token = colourScheme.types.size() - 1;
		}

        document.clearTokens (rows);
        document.applyTokens (rows, zones);

        lastTokeniserTime = Time::getMillisecondCounterHiRes() - start;

        for (int n = 0; n < colourScheme.types.size(); ++n)
        {
            g.setColour (colourScheme.types[n].colour);
            document.findGlyphsIntersecting (g.getClipBounds().toFloat(), n).draw (g);
        }
    }
    else
    {
        lastTokeniserTime = 0.f;
        document.findGlyphsIntersecting (g.getClipBounds().toFloat()).draw (g);
    }
    g.restoreState();
}

void mcl::TextEditor::resetProfilingData()
{
    accumulatedTimeInPaint = 0.f;
    numPaintCalls = 0;
}


}
