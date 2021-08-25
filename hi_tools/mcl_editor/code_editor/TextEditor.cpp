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
mcl::TextEditor::TextEditor(TextDocument& codeDoc)
: document(codeDoc)
, caret (document)
, gutter (document)
, linebreakDisplay(document)
, highlight (document)
, docRef(codeDoc.getCodeDocument())
, scrollBar(true)
, tokenCollection()
, tooltipManager(*this)
, autocompleteTimer(*this)
, plaf(new LookAndFeel_V3())
{
	tokenCollection.addTokenProvider(new SimpleDocumentTokenProvider(docRef));

	document.addSelectionListener(this);
	startTimer(500);

    lastTransactionTime = Time::getApproximateMillisecondCounter();
    document.setSelections ({ Selection() }, false);

	

	docRef.addListener(this);
	addAndMakeVisible(scrollBar);
	
	docRef.removeListener(&document);
	docRef.addListener(&document);
	
	
	scrollBar.addListener(this);
	scrollBar.setColour(ScrollBar::ColourIds::thumbColourId, Colours::white.withAlpha(0.2f));
    setFont (GLOBAL_MONOSPACE_FONT().withHeight(19.0f));

    translateView (gutter.getGutterWidth(), 0); 
    setWantsKeyboardFocus (true);

	addAndMakeVisible(linebreakDisplay);
    addAndMakeVisible (highlight);
    addAndMakeVisible (caret);
    addAndMakeVisible (gutter);

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

	setColour(CodeEditorComponent::ColourIds::backgroundColourId, Helpers::getEditorColour(Helpers::EditorBackgroundColour));
	setOpaque(false);
	setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colours::white);
	setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0x33FFFFFF));
	setColour(juce::CaretComponent::ColourIds::caretColourId, Colours::white);
	setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFBBBBBB));

	tokenCollection.setEnabled(false);

	updateAfterTextChange();
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

bool TextEditor::shouldSkipInactiveUpdate() const
{
	auto docHasMultipleEditors = document.getCodeDocument().getNumListeners() > 10;

	if (!docHasMultipleEditors)
		return false;

	auto isBig = document.getNumRows() > 2000;

	if (!isBig)
		return false;
	
	if (!isShowing())
		return true;

	return false;
}

void TextEditor::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	auto b = document.getBounds();
	
	auto pos = newRangeStart;

	translation.y = jlimit<float>(-b.getHeight() * viewScaleFactor, 0.0f, -pos * viewScaleFactor);
	updateViewTransform();
}

hise::MarkdownLink TextEditor::getLink() const
{
	if (currentAutoComplete != nullptr)
	{
		if (auto it = currentAutoComplete->items[currentAutoComplete->viewIndex])
		{
			return it->token->getLink();
		}
	}
	else
	{

	}

	return MarkdownLink();
}

void TextEditor::updateAutocomplete(bool forceShow /*= false*/)
{
	if (document.getSelections().size() != 1)
	{
		closeAutocomplete(true, {}, {});
		return;
	}

	const auto o = document.getSelections().getFirst().oriented().tail;

	if (o.isOrigin())
	{
		return;
	}

	auto p = o;
	auto s = p;

	document.navigate(s, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::backwardCol);
	document.navigate(p, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::forwardCol);


	auto lineStart = o;
	document.navigate(lineStart, mcl::TextDocument::Target::firstnonwhitespace, mcl::TextDocument::Direction::backwardCol);

	auto lineContent = document.getSelectionContent({ lineStart, o });

	auto isCommentLine = lineContent.contains("//") || lineContent.startsWith("/*");

	if (isCommentLine)
	{
		closeAutocomplete(true, {}, {});
		return;
	}

	autocompleteSelection = { s.x, s.y, p.x, p.y };
	auto input = document.getSelectionContent(autocompleteSelection);

	auto te = s;
	auto ts = s;

	document.navigate(ts, TextDocument::Target::cppToken, TextDocument::Direction::backwardCol);

	Selection beforeToken = { ts.x, ts.y, te.x, te.y };

	auto tokenBefore = document.getSelectionContent(beforeToken);

	auto hasDotAndNotFloat = !CharacterFunctions::isDigit(tokenBefore[0]) && tokenBefore.endsWith(".");

	auto lineNumber = o.x;

	auto parent = TopLevelWindowWithOptionalOpenGL::findRoot(this); 

	if (parent == nullptr)
		parent = this;

	if (forceShow || ((input.isNotEmpty() && tokenCollection.hasEntries(input, tokenBefore, lineNumber)) || hasDotAndNotFloat))
	{
		if (!hasKeyboardFocus(true))
		{
			currentAutoComplete = nullptr;
			return;
		}

		if (currentAutoComplete != nullptr)
			currentAutoComplete->setInput(input, tokenBefore, lineNumber);
		else
		{
			parent->addAndMakeVisible(currentAutoComplete = new Autocomplete(tokenCollection, input, tokenBefore, lineNumber, this));
			addKeyListener(currentAutoComplete);
		}

		auto sToUse = input.isEmpty() ? o.translated(0, 1) : s;

		auto cBounds = document.getBoundsOnRow(sToUse.x, { sToUse.y, sToUse.y + 1 }, GlyphArrangementArray::ReturnLastCharacter).getRectangle(0);
		auto topLeft = cBounds.getBottomLeft();

		auto ltl = topLeft.roundToInt().transformedBy(transform);

		ltl = getTopLevelComponent()->getLocalPoint(this, ltl);
		
		currentAutoComplete->setTopLeftPosition(ltl);

		if (currentAutoComplete->getBoundsInParent().getBottom() > parent->getHeight())
		{
			ltl = ltl.translated(0, -currentAutoComplete->getHeight() - cBounds.getHeight() * viewScaleFactor);
			currentAutoComplete->setTopLeftPosition(ltl);
		}
	}
	else
		closeAutocomplete(false, {}, {});
}

void TextEditor::codeDocumentTextInserted(const String& newText, int insertIndex)
{
	if (shouldSkipInactiveUpdate())
		return;

	CodeDocument::Position start(document.getCodeDocument(), insertIndex);
	auto end = start.movedBy(newText.length());
	Range<int> r(start.getLineNumber(), end.getLineNumber() + 1);
	updateAfterTextChange(r);
}

void TextEditor::codeDocumentTextDeleted(int startIndex, int endIndex)
{
	if (shouldSkipInactiveUpdate())
		return;

	CodeDocument::Position start(document.getCodeDocument(), startIndex);
	CodeDocument::Position end(document.getCodeDocument(), endIndex);
	
	Range<int> r(start.getLineNumber(), end.getLineNumber() + 1);

	updateAfterTextChange(r);
}



void TextEditor::setScaleFactor(float newFactor)
{
	auto currentLine = document.getSelection(0).head;

	Range<int> r(getFirstLineOnScreen(), getFirstLineOnScreen() + getNumDisplayedRows());

	if (!r.contains(currentLine.x))
		currentLine.x = r.getStart();

	auto pos = document.getPosition(currentLine, TextDocument::Metric::baseline);

	auto posOnScreen = pos.transformedBy(transform);

	viewScaleFactor = newFactor;

	refreshLineWidth();

	updateViewTransform();

	auto newPos = pos.transformedBy(transform);

	auto dy = newPos.y - posOnScreen.y;

	translateView(0.0f, -dy);
}

void TextEditor::closeAutocomplete(bool async, const String& textToInsert, Array<Range<int>> selectRanges)
{
	if (currentAutoComplete != nullptr)
	{
		auto f = [this, textToInsert, selectRanges]()
		{
			removeKeyListener(currentAutoComplete);

			Desktop::getInstance().getAnimator().fadeOut(currentAutoComplete, 300);

			Component* parent = TopLevelWindowWithOptionalOpenGL::findRoot(this);
			
			if (parent == nullptr)
				parent = this;

			parent->removeChildComponent(currentAutoComplete);

			currentAutoComplete = nullptr;

			if (textToInsert.isNotEmpty())
			{
				ScopedValueSetter<bool> svs(skipTextUpdate, true);
				document.setSelections({ autocompleteSelection }, false);

				insert(textToInsert);

				auto s = document.getSelection(0).oriented();
				CodeDocument::Position insertStart(document.getCodeDocument(), s.tail.x, s.tail.y);

				refreshLineWidth();

				document.rebuildRowPositions();

				updateViewTransform();
				translateView(0.0f, 0.0f);

				auto l = textToInsert.length();

				if (currentParameterSelection.size() == 0)
					setParameterSelectionInternal(currentParameterSelection, nullptr, true);

				if (currentParameter == nullptr)
				{
					clearParameters(true);

					if (!selectRanges.isEmpty())
					{
						Action::List newList;

						for (auto sr : selectRanges)
						{
							auto copy = insertStart;

							auto ps = new Autocomplete::ParameterSelection(document,
								insertStart.getPosition() - l + sr.getStart(),
								insertStart.getPosition() - l + sr.getEnd());

							newList.add(ps);
						}

						if (!newList.isEmpty())
							setParameterSelectionInternal(newList, newList.getFirst(), true);
					}

					postParameterPos = insertStart;
					postParameterPos.setPositionMaintained(true);
				}
				
				if (currentParameter != nullptr)
				{
					document.setSelections({ currentParameter->getSelection() }, false);
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

void TextEditor::grabKeyboardFocusAndActivateTokenBuilding()
{
	if (shouldSkipInactiveUpdate())
		updateAfterTextChange();

	tokenCollection.setEnabled(true);
	grabKeyboardFocus();
}

bool TextEditor::cut()
{
	auto s = document.getSelections().getFirst();

	bool move = false;

	if (s.isSingular())
	{
		document.navigate(s.head, TextDocument::Target::line, TextDocument::Direction::backwardCol);
		document.navigate(s.head, TextDocument::Target::character, TextDocument::Direction::backwardCol);
		document.navigate(s.tail, TextDocument::Target::line, TextDocument::Direction::forwardCol);
		document.setSelection(0, s, false);
		move = true;
	}

	SystemClipboard::copyTextToClipboard(document.getSelectionContent(s));

	insert("");

	if (move)
	{
		nav({}, TextDocument::Target::character, TextDocument::Direction::forwardRow);
		nav({}, TextDocument::Target::firstnonwhitespace, TextDocument::Direction::backwardCol);
	}

	return true;
}

bool TextEditor::copy()
{
	SystemClipboard::copyTextToClipboard(document.getSelectionContent(document.getSelections().getFirst()));
	return true;
}

bool TextEditor::paste()
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
					return s.substring(0, numWhitespace);
			}

			return s;
		};

		auto whitespaceToRemove = getWhitespaceAtBeginning(sa[0]);

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
				l = prevws + trimmed;
		}

		insertText = sa.joinIntoString("\n");
		DBG(insertText);
	}

	return insert(insertText);
}

void TextEditor::displayedLineRangeChanged(Range<int> newRange)
{
	if (!scrollRecursion)
	{
		scrollToLine(newRange.getStart() + newRange.getLength() / 2, true);
	}
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

	translateToEnsureCaretIsVisible();

	
}

void mcl::TextEditor::translateView (float dx, float dy)
{
    auto H = viewScaleFactor * document.getBounds().getHeight();

	gutter.setViewTransform(AffineTransform::scale(viewScaleFactor));

	translation.x = gutter.getGutterWidth() + xPos;
    translation.y = jlimit (jmin (-0.f, -H + getHeight()), 0.0f, translation.y + dy);

    updateViewTransform();
}



void mcl::TextEditor::scaleView (float scaleFactorMultiplier, float verticalCenter)
{
	closeAutocomplete(true, {}, {});

	setScaleFactor(jlimit(0.5f, 4.0f, viewScaleFactor * scaleFactorMultiplier));

	
	
}

void mcl::TextEditor::updateViewTransform()
{
	auto thisGutterWidth = gutter.getGutterWidth();

	if (translation.x < thisGutterWidth)
	{
		//translation.x = thisGutterWidth;
	}
		

	closeAutocomplete(true, {}, {});

	transform = AffineTransform::scale(viewScaleFactor).translated(translation.x, translation.y);
	highlight.setViewTransform(transform);
	caret.setViewTransform(transform);
	gutter.setViewTransform(transform);
	linebreakDisplay.setViewTransform(transform);
	
	auto b = document.getBounds();
	scrollBar.setRangeLimits({ b.getY(), b.getBottom()});

	auto visibleRange = getLocalBounds().toFloat().transformed(transform.inverted());
	scrollBar.setCurrentRange({ visibleRange.getY(), visibleRange.getBottom() }, dontSendNotification);
	auto rows = document.getRangeOfRowsIntersecting(getLocalBounds().toFloat().transformed(transform.inverted()));
	ScopedValueSetter<bool> svs(scrollRecursion, true);
	document.setDisplayedLineRange(rows);
    repaint();
}

void mcl::TextEditor::updateSelections()
{
    
}

void mcl::TextEditor::translateToEnsureCaretIsVisible()
{
	if (getLocalBounds().isEmpty())
		return;

    auto i = document.getSelections().getLast().head;
    auto t = Point<float> (0.f, document.getVerticalPosition (i.x, TextDocument::Metric::top))   .transformedBy (transform);
    auto b = Point<float> (0.f, document.getVerticalPosition (i.x, TextDocument::Metric::bottom)).transformedBy (transform);

	auto c = document.getBoundsOnRow(i.x, Range<int>(i.y, i.y + 1), GlyphArrangementArray::OutOfBoundsMode::ReturnBeyondLastCharacter).getRectangle(0);
	c = c.transformedBy(transform);
	
	auto gw = gutter.getGutterWidth();

	if (c.getRight() > getWidth())
	{
		xPos -= (float)(c.getRight() - getWidth());
		translateView(0.0f, 0.0f);
	}
	else if (gw > 0.0f && c.getX() < gw)
	{
		xPos -= (c.getX() - gw);
		translateView(0.0f, 0.0f);
	}
	
    if (t.y < 0.f)
        translateView (0.f, -t.y);
    else if (b.y > getHeight())
        translateView (0.f, -b.y + getHeight());

	if (document.getFoldableLineRangeHolder().isFolded(i.x))
		document.getFoldableLineRangeHolder().unfold(i.x);
}


void mcl::TextEditor::resized()
{
	auto b = getLocalBounds();
	caret.setBounds(b);
	
    auto l = getFirstLineOnScreen();
    
    
    
	scrollBar.setBounds(b.removeFromRight(14));

	if(linebreakEnabled)
		linebreakDisplay.setBounds(b.removeFromRight(15));

	maxLinesToShow = b.getWidth() - TEXT_INDENT - 10;

	refreshLineWidth();
	
    setFirstLineOnScreen(l);
    
    highlight.setBounds (b);
    
    gutter.setBounds (b);
    resetProfilingData();
}

void mcl::TextEditor::paint (Graphics& g)
{
	if (shouldSkipInactiveUpdate())
	{
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.4f));
		g.drawText("Editor is inactive. Click to activate", getLocalBounds().toFloat(), Justification::centred);
		return;
	}

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
	{
		currentError->paintLines(g, transform, Colours::red);
		g.setColour(Colours::red.withAlpha(0.08f));
		auto b = currentError->area.transformedBy(transform).withLeft(gutter.getGutterWidth()).withRight(getWidth());;
		g.fillRect(b);
	}
	
	for (auto w : warnings)
		w->paintLines(g, transform, w->isWarning ? Colours::yellow : Colours::red);

	if (xPos < -10.0f * transform.getScaleFactor())
	{
		auto b = getLocalBounds().toFloat();
		b.removeFromLeft(gutter.getGutterWidth());
		b = b.removeFromLeft(30);

		auto c = Helpers::getEditorColour(Helpers::EditorBackgroundColour);

		g.setGradientFill(ColourGradient(c.withAlpha(0.6f), b.getX(), 0.0f, c.withAlpha(0.0f), b.getRight(), 0.0f, false));
		g.fillRect(b);
	}
}

void mcl::TextEditor::paintOverChildren (Graphics& g)
{
	
}

void mcl::TextEditor::mouseDown (const MouseEvent& e)
{
	if (readOnly)
		return;

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
		enum Commands
		{
			Cut=1,
			Copy,
			Paste,
			SelectAll,
			Undo,
			Redo,
			Forward,
			Back,
			FoldAll,
			UnfoldAll,
			Goto,
			LineBreaks,
			BackgroundParsing,
			Preprocessor,
			numCommands
		};

        PopupMenu menu;
		menu.setLookAndFeel(plaf);


		menu.addSeparator();

		bool isSomethingSelected = !document.getSelection(0).isSingular();

		menu.addItem(Cut, "Cut", isSomethingSelected, false);
		menu.addItem(Copy, "Copy", isSomethingSelected, false);
		menu.addItem(Paste, "Paste", SystemClipboard::getTextFromClipboard().isNotEmpty(), false);
		menu.addItem(SelectAll, "Select All");
		menu.addSeparator();

		menu.addItem(Undo, "Undo", document.getCodeDocument().getUndoManager().canUndo(), false);
		menu.addItem(Redo, "Redo", document.getCodeDocument().getUndoManager().canRedo(), false);

		menu.addSeparator();

		menu.addSectionHeader("View options");
		menu.addItem(Back, "Back", document.viewUndoManager.canUndo());
		menu.addItem(Forward, "Forward", document.viewUndoManager.canRedo());
		menu.addItem(FoldAll, "Fold all", true, false);
		menu.addItem(UnfoldAll, "Unfold all", true, false);
		menu.addItem(LineBreaks, "Enable line breaks", true, linebreakEnabled);

		menu.addSeparator();

		for (const auto& pf : popupMenuFunctions)
			pf(*this, menu, e);

		auto result = menu.show();

		for (const auto& rf : popupMenuResultFunctions)
		{
			if (rf(*this, result))
				return;
		}

        switch (result)
        {
			case Cut: cut(); break;
			case Copy: copy(); break;
			case Paste: paste(); break;
			case Forward: document.viewUndoManager.redo(); break;
			case Back:    document.viewUndoManager.undo(); break;
			case SelectAll: expand(TextDocument::Target::document); break;
			case Undo: document.getCodeDocument().getUndoManager().undo(); break;
			case Redo: document.getCodeDocument().getUndoManager().redo(); break;
			case FoldAll: 
			case UnfoldAll: 
			{
				for (auto l : document.getFoldableLineRangeHolder().all)
					l->setFolded(result == FoldAll);

				document.getFoldableLineRangeHolder().updateFoldState(nullptr);
				break;
			}
			case LineBreaks: 
				linebreakEnabled = !linebreakEnabled; 
				
				if (linebreakEnabled)
					xPos = 0.0f;

				resized();  
				refreshLineWidth(); break;
        }

        resetProfilingData();
        repaint();
        return;
    }

    auto selections = document.getSelections();
    

	if (e.mods.isShiftDown())
	{
		auto sel = document.getSelection(0).oriented();

		auto midX = (sel.head.x + sel.tail.x) / 2;

		auto s = index.x > midX ? sel.head : sel.tail;

		Selection ns(index, s);

		document.setSelections({ ns }, false);
		updateSelections();
		return;
	}

    if (selections.contains (index))
    {
        return;
    }
    if (!e.mods.isCommandDown())
    {
        selections.clear();
    }

    selections.add (index);
    document.setSelections (selections, true);

	grabKeyboardFocusAndActivateTokenBuilding();
}

void mcl::TextEditor::mouseDrag (const MouseEvent& e)
{
	if (readOnly)
		return;

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

			document.setSelections(multiLineSelections, false);
			updateSelections();
		}
		else
		{
			auto selection = document.getSelections().getFirst();
			selection.head = document.findIndexNearestPosition(e.position.transformedBy(transform.inverted()));
			document.setSelections({ selection }, false);
			translateToEnsureCaretIsVisible();
			updateSelections();
		}
    }
}

void mcl::TextEditor::mouseDoubleClick (const MouseEvent& e)
{
	if (readOnly)
		return;

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
    autocompleteTimer.abortAutocomplete();

	tooltipManager.clearDisplay();

	for (const auto& kf : keyPressFunctions)
	{
		if (kf(key))
			return true;
	}

    // =======================================================================================
    using Target     = TextDocument::Target;
    using Direction  = TextDocument::Direction;
    auto mods        = key.getModifiers();
    auto isTab       = tabKeyUsed && (key.getTextCharacter() == '\t');

	auto skipIfClosure = [this](juce_wchar c)
	{
		if (ActionHelpers::isRightClosure(c))
		{
			auto s = document.getSelections().getFirst();
			auto e = document.getCharacter(s.head);
			
			if (e == c)
			{
				auto newS = s;

				document.navigate(newS.head, TextDocument::Target::firstnonwhitespace, TextDocument::Direction::backwardCol);

				auto lineContent = document.getSelectionContent(newS);

				int delta = 0;

				for (int i = 0; i < lineContent.length(); i++)
				{
					auto thisC = lineContent[i];

					if (ActionHelpers::isMatchingClosure(thisC, c))
						delta++;
					if (ActionHelpers::isRightClosure(thisC))
						delta--;
				}

				if (delta <= 1)
				{
					document.navigateSelections(Target::character, Direction::forwardCol, Selection::Part::both);
					updateSelections();
					return true;
				}
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
			auto c = sPos.getCharacter();

			sPos = sPos.movedBy(1);

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
		
		//numAfter = jmax(numAfter, 0);


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
			document.navigateSelections(Target::character, Direction::backwardCol, Selection::Part::both);
			updateSelections();
		}

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
		document.navigate(s.tail, target, direction);
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

	auto remove = [this](Target target, Direction direction)
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
				document.setSelections(document.getSelections().getLast(), true);
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
        if (key.isKeyCode (KeyPress::rightKey)) return nav (mods, Target::whitespace, Direction::forwardCol)  && nav (mods, Target::word, Direction::forwardCol);
        if (key.isKeyCode (KeyPress::leftKey )) return nav (mods, Target::whitespace, Direction::backwardCol) && nav (mods, Target::word, Direction::backwardCol);
        if (key.isKeyCode (KeyPress::downKey )) return nav (mods, Target::word, Direction::forwardCol)  && nav (mods, Target::paragraph, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey   )) return nav (mods, Target::word, Direction::backwardCol) && nav (mods, Target::paragraph, Direction::backwardRow);

        if (key.isKeyCode (KeyPress::backspaceKey)) return (   expandBack (Target::whitespace, Direction::backwardCol)
                                                            && expandBack (Target::word, Direction::backwardCol)
                                                            && insert (""));

		if (key == KeyPress('e', ModifierKeys::ctrlModifier, 0) ||
			key == KeyPress('e', ModifierKeys::ctrlModifier | ModifierKeys::shiftModifier, 0))
		{
			if (currentError != nullptr)
			{
				document.setSelections({ currentError->getSelection() }, true);
				return true;
			}
		}
    }
    if (mods.isCommandDown())
    {
        if (key.isKeyCode (KeyPress::downKey)) return nav (mods, Target::document, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey  )) return nav (mods, Target::document, Direction::backwardRow);
        
#if JUCE_MAC
        if (key.isKeyCode (KeyPress::leftKey)) return nav(mods, Target::firstnonwhitespace, Direction::backwardCol);
        if (key.isKeyCode (KeyPress::rightKey  )) return nav(mods, Target::line, Direction::forwardCol);
#endif
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
				document.setSelections(lineStarts, false);
				document.navigateSelections(Target::character, Direction::forwardCol, Selection::Part::both);

				remove(Target::character, Direction::backwardCol);
			}
			else
			{
				document.setSelections(lineStarts, false);
				insert("\t");
			}

			

			Selection prev(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());

			document.setSelections({ prev }, false);

			updateSelections();
			return true;
		}
	}

	if (key.isKeyCode(KeyPress::F12Key))
	{
		return gotoDefinition();

	}
	if (key.isKeyCode(KeyPress::rightKey))  return nav(mods, Target::character, Direction::forwardCol);
	if (key.isKeyCode(KeyPress::leftKey))   return nav(mods, Target::character, Direction::backwardCol);
	if (key.isKeyCode(KeyPress::downKey))
	{
		if (mods.isAltDown())
		{
			return addCaret(Target::character, Direction::forwardRow);
		}
		return nav(mods, Target::character, Direction::forwardRow);
	}
	if (key.isKeyCode(KeyPress::upKey))
	{
		if (mods.isAltDown())
		{
			return addCaret(Target::character, Direction::backwardRow);
		}
		return nav(mods, Target::character, Direction::backwardRow);
	}



	if (key.isKeyCode(KeyPress::backspaceKey)) return remove(Target::character, Direction::backwardCol);
	if (key.isKeyCode(KeyPress::deleteKey))	   return remove(Target::character, Direction::forwardCol);

#if JUCE_WINDOWS || JUCE_LINUX
    // Home/End: End / start of line
	if (key.isKeyCode(KeyPress::homeKey)) return nav(mods, Target::firstnonwhitespace, Direction::backwardCol);
	if (key.isKeyCode(KeyPress::endKey))  return nav(mods, Target::line, Direction::forwardCol);
#else
    // Home/End: Scroll to beginning
    if (key.isKeyCode(KeyPress::homeKey)) return nav (mods, Target::document, Direction::forwardRow);
    if (key.isKeyCode(KeyPress::endKey))  return nav (mods, Target::document, Direction::backwardRow);
#endif

    if (key == KeyPress ('a', ModifierKeys::commandModifier, 0)) return expand (Target::document);
	if (key == KeyPress('d', ModifierKeys::commandModifier, 0))  return addNextTokenToSelection();
    if (key == KeyPress ('l', ModifierKeys::commandModifier, 0)) return expand (Target::line);
    if (key == KeyPress ('u', ModifierKeys::commandModifier, 0)) return addSelectionAtNextMatch();
    if (key == KeyPress ('z', ModifierKeys::commandModifier, 0)) return document.getCodeDocument().getUndoManager().undo();
    if (key == KeyPress ('r', ModifierKeys::commandModifier, 0)) return document.getCodeDocument().getUndoManager().redo();

	if (key.isKeyCode(KeyPress::F4Key))
	{
		auto isComment = [this](Selection s)
		{
			document.navigate(s.tail, Target::line, Direction::forwardCol);
			document.navigate(s.head, Target::line, Direction::forwardCol);
			document.navigate(s.tail, Target::firstnonwhitespace, Direction::backwardCol);
			document.navigate(s.head, Target::firstnonwhitespace, Direction::backwardCol);
			s.head.y += 2;
			return document.getSelectionContent(s) == "//";
		};

		bool anythingCommented = false;
		bool anythingUncommented = false;

		for (auto s : document.getSelections())
		{
			auto thisOne = isComment(s);

			anythingCommented |= thisOne;
			anythingUncommented |= !thisOne;
		}

		if (anythingUncommented && anythingCommented)
			return false;

		Array<CodeDocument::Position> positions;

		for (auto s : document.getSelections())
		{
			positions.add(s.toCodePosition(document.getCodeDocument()));
		}

		for (auto& p : positions)
			p.setPositionMaintained(true);

		nav({}, TextDocument::Target::line, TextDocument::Direction::forwardCol);
		nav({}, TextDocument::Target::firstnonwhitespace, TextDocument::Direction::backwardCol);

		if (anythingUncommented)
		{
			insert("//");
		}
		else
		{
			remove(Target::character, Direction::forwardCol);
			remove(Target::character, Direction::forwardCol);
		}

		Array<Selection> newSelection;

		for (auto p : positions)
		{
			newSelection.add(Selection::fromCodePosition(p));
		}

		document.setSelections(newSelection, false);
		return true;
	}
    if (key == KeyPress ('x', ModifierKeys::commandModifier, 0))
		return cut();
    if (key == KeyPress ('c', ModifierKeys::commandModifier, 0))
		return copy();
	if (key == KeyPress('v', ModifierKeys::commandModifier, 0))
		return paste();
	if (key == KeyPress('f', ModifierKeys::ctrlModifier, 0))
	{
		currentSearchBox = new SearchBoxComponent(document, transform.getScaleFactor());
		addAndMakeVisible(currentSearchBox);
		currentSearchBox->addListener(this);
		currentSearchBox->setBounds(getLocalBounds().removeFromBottom(document.getRowHeight() * transform.getScaleFactor() * 1.2f + 5));

		auto s = document.getSelections().getFirst();
		auto sel = document.getSelectionContent(s);

		if(sel.isNotEmpty())
			currentSearchBox->searchField.setText(sel, sendNotificationSync);

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
	if (isShowing())
		tokenCollection.setEnabled(true);


	ScopedValueSetter<bool> scrollDisabler(scrollRecursion, true);

	double now = Time::getApproximateMillisecondCounter();

	if (currentParameter == nullptr)
	{
		clearParameters(true);
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
                case Transaction::Direction::forward: document.setSelection (n, r.selection, false); break;
                case Transaction::Direction::reverse: document.setSelection (n, r.selection.tail, false); break;
            }

            if (! r.affectedArea.isEmpty())
            {
				repaint();
            }
        };

		ScopedPointer<UndoableAction> op = t.on(document, callback);
		op->perform();

    }
	
	//refreshLineWidth();
	//updateViewTransform();
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
	auto c = Helpers::getEditorColour(Helpers::EditorBackgroundColour);

	g.fillAll(c);

    g.saveState();
    g.addTransform (transform);

	highlight.paintHighlight(g);

    if (enableSyntaxHighlighting)
    {
        auto rows = document.getRangeOfRowsIntersecting (g.getClipBounds().toFloat());

		auto realStart = document.getFoldableLineRangeHolder().getNearestLineStartOfAnyRange(rows.getStart());

		rows.setStart(realStart);

        auto index = Point<int> (rows.getStart(), 0);
        
        auto zones = Array<Selection>();

		CodeDocument::Position pos(document.getCodeDocument(), rows.getStart(), 0);
		CodeDocument::Iterator it(pos);
		
		Point<int> previous(it.getLine(), it.getIndexInLine());

        while (it.getLine() < rows.getEnd() && ! it.isEOF())
        {
			int tokenType;

			if (tokeniser != nullptr)
				tokenType = tokeniser->readNextToken(it);
			else
				tokenType = JavascriptTokeniserFunctions::readNextToken(it);

			Point<int> now(it.getLine(), it.getIndexInLine());

			if (previous != now)
				zones.add(Selection(previous, now).withStyle(tokenType));
			else
				break;

            previous = now;
        }

		for (auto& z : zones)
		{
			if (deactivatesLines.contains(z.tail.x+1))
				z.token = colourScheme.types.size() - 1;
		}

        document.clearTokens (rows);
        document.applyTokens (rows, zones);

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
