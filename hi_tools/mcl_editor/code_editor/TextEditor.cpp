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
, horizontalScrollBar(false)
, tokenCollection()
, tooltipManager(*this)
, autocompleteTimer(*this)
, plaf(new LookAndFeel_V3())
{
	//tokenCollection.addTokenProvider(new SimpleDocumentTokenProvider(docRef));

	document.addSelectionListener(this);
	startTimer(500);

    lastTransactionTime = Time::getApproximateMillisecondCounter();
    document.setSelections ({ Selection() }, false);

	

	docRef.addListener(this);
	addAndMakeVisible(scrollBar);
	
	docRef.removeListener(&document);
	docRef.addListener(&document);

	
	
	scrollBar.addListener(this);
    sf.addScrollBarToAnimate(scrollBar);
    sf.addScrollBarToAnimate(horizontalScrollBar);
    
    setFont (GLOBAL_MONOSPACE_FONT().withHeight(19.0f));

    translateView (gutter.getGutterWidth(), 0); 
    setWantsKeyboardFocus (true);

	addAndMakeVisible(linebreakDisplay);
    addAndMakeVisible (highlight);
    addAndMakeVisible (caret);
    addAndMakeVisible (gutter);
	addAndMakeVisible(horizontalScrollBar);
	horizontalScrollBar.setColour(ScrollBar::ColourIds::thumbColourId, Colours::white.withAlpha(0.05f));
	horizontalScrollBar.addListener(this);

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
		{ "Deactivated", 0xFF666666 },
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
	
	updateAfterTextChange();
}

mcl::TextEditor::~TextEditor()
{
	if(tokenCollection != nullptr)
		tokenCollection->removeListener(this);
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

void TextEditor::timerCallback()
{
	document.getCodeDocument().getUndoManager().beginNewTransaction();

	document.viewUndoManagerToUse->beginNewTransaction();
}

void TextEditor::setNewTokenCollectionForAllChildren(Component* any, const Identifier& languageId, TokenCollection::Ptr newCollection)
{
	if(newCollection == nullptr)
		newCollection = new TokenCollection(languageId);
	
	auto top = any->getTopLevelComponent();

	Component::callRecursive<TextEditor>(top, [&](TextEditor* t)
	{
		if(t->languageManager->getLanguageId() == languageId && newCollection != nullptr)
		{
			t->tokenCollection = newCollection;
			newCollection->addListener(t);

			if(!newCollection->hasTokenProviders())
			{
				t->languageManager->addTokenProviders(newCollection.get());
			}
		}
			
		return false;
	}, false);

	newCollection->signalRebuild();
}

void TextEditor::setReadOnly(bool shouldBeReadOnly)
{
	readOnly = shouldBeReadOnly;
}

void TextEditor::setShowNavigation(bool shouldShowNavigation)
{
	resized();
}

void TextEditor::focusGained(FocusChangeType t)
{
	if (onFocusChange)
		onFocusChange(true, t);

	caret.startTimer(50);
}

Font TextEditor::getFont() const
{ return document.getFont(); }

void TextEditor::setGotoFunction(const GotoFunction& f)
{
	gotoFunction = f;
}

void TextEditor::clearWarningsAndErrors()
{
	currentError = nullptr;
	warnings.clear();
	repaint();
}

void TextEditor::addWarning(const String& errorMessage, bool isWarning)
{
	warnings.add(new Error(document, errorMessage, isWarning));
	repaint();
}

void TextEditor::setError(const String& errorMessage)
{
	if (errorMessage.isEmpty())
		currentError = nullptr;
	else
		currentError = new Error(document, errorMessage, false);

	repaint();
}

void TextEditor::abortAutocomplete()
{
	autocompleteTimer.stopTimer();
}

void TextEditor::refreshLineWidth()
{
	auto actualLineWidth = (maxLinesToShow - gutter.getGutterWidth()) / viewScaleFactor;

	if (linebreakEnabled)
		document.setMaxLineWidth(actualLineWidth);
	else
		document.setMaxLineWidth(-1);
}

CodeDocument& TextEditor::getDocument()
{ return document.getCodeDocument(); }

TooltipWithArea::Data TextEditor::getTooltip(Point<float> position)
{
	for (auto ps : currentParameterSelection)
	{
		if (ps->p.getBounds().contains(position))
		{
			TooltipWithArea::Data d;
				
			d.id = "ps" + String(currentParameterSelection.indexOf(ps));
			d.relativePosition = ps->p.getBounds().getBottomLeft();
			d.text = ps->tooltip;
			return d;
		}
	}

	if (currentError != nullptr)
	{
		if (auto d = currentError->getTooltip(transform, position))
			return d;
	}

	for (auto w : warnings)
	{
		if (auto d = w->getTooltip(transform, position))
			return d;
	}

	if (tokenTooltipFunction)
	{
		auto start = document.findIndexNearestPosition(position.transformedBy(transform.inverted()));
		auto end = start;

		document.navigate(start, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::backwardCol);
		document.navigate(end, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::forwardCol);

		auto token = document.getSelectionContent({ start, end });

		if (token.isNotEmpty())
		{
			TooltipWithArea::Data d;
			d.id = Identifier(token);
			d.text = tokenTooltipFunction(token, start.x);

				

			auto b = document.getBoundsOnRow(start.x, { start.y, end.y }, GlyphArrangementArray::OutOfBoundsMode::ReturnLastCharacter).getRectangle(0);

			d.relativePosition = b.getBottomLeft().transformedBy(transform);

			if (d.text.isEmpty())
				return {};

			return d;
		}
	}

	return {};
}

bool TextEditor::gotoDefinition(Selection s1)
{
	if (gotoFunction)
	{
		if (s1.tail.isOrigin())
			s1 = document.getSelection(0);

		const auto o = s1.tail;

		auto p = o;
		auto s = p;

		document.navigate(s, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::backwardCol);
		document.navigate(s, mcl::TextDocument::Target::cppToken, mcl::TextDocument::Direction::backwardCol);
		document.navigate(p, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::forwardCol);

		Selection tokenSelection(s.x, s.y, p.x, p.y);
		auto token = document.getSelectionContent(tokenSelection);

		auto sl = gotoFunction(s.x, token);

		document.jumpToLine(sl);
            
		return true;
	}

	return false;
}

TextEditor::Action::Action(TextEditor* te, List nl, Ptr ncp):
	editor(te),
	oldList(te->currentParameterSelection),
	newList(std::move(nl)),
	oldCurrent(te->currentParameter),
	newCurrent(ncp)
{}

bool TextEditor::Action::undo()
{
	if (editor != nullptr)
	{
		editor->setParameterSelectionInternal(oldList, oldCurrent, false);
		return true;
	}

	return false;
}

bool TextEditor::Action::perform()
{
	if (editor != nullptr)
	{
		editor->setParameterSelectionInternal(newList, newCurrent, false);
		return true;
	}

	return false;
}

void TextEditor::setParameterSelectionInternal(Action::List l, Action::Ptr p, bool useUndo)
{
	if (useUndo)
	{
		auto a = new Action(this, l, p);
		document.getCodeDocument().getUndoManager().perform(a);
	}
	else
	{
		currentParameterSelection = l;
		currentParameter = p;

		if (currentParameter != nullptr)
			document.setSelections({ currentParameter->getSelection() }, false);

		grabKeyboardFocus();
		repaint();
	}
}

void TextEditor::clearParameters(bool useUndo)
{
	setParameterSelectionInternal({}, nullptr, useUndo);

#if OLD
		currentParameter = nullptr;
		currentParameterSelection.clear();
		repaint();
#endif
}

bool TextEditor::incParameter(bool useUndo)
{
	if (currentParameter != nullptr)
	{
		auto newIndex = currentParameterSelection.indexOf(currentParameter) + 1;

		if (auto next = currentParameterSelection[newIndex])
			setParameterSelectionInternal(currentParameterSelection, next, useUndo);
		else
		{
			setParameterSelectionInternal(currentParameterSelection, nullptr, useUndo);
			Point<int> p(postParameterPos.getLineNumber(), postParameterPos.getIndexInLine());
			document.setSelections({ Selection(p) }, true);
		}

		return true;
	}

	return false;
}

bool TextEditor::setParameterSelection(int index, bool useUndo)
{
	setParameterSelectionInternal(currentParameterSelection, currentParameterSelection[index], useUndo);

	return true;

#if OLD
		auto s = currentParameterSelection.size();

		if (isPositiveAndBelow(index, s))
		{
			currentParameter = currentParameterSelection[index];
			document.setSelections({ currentParameter->getSelection() });
			repaint();
			return true;
		}
		else
		{
			currentParameter = nullptr;
			repaint();
			return false;
		}

		return false;
#endif
}

void TextEditor::updateLineRanges()
{
	auto ranges = languageManager->createLineRange(document.getCodeDocument());
	document.getFoldableLineRangeHolder().setRanges(ranges);
}

void TextEditor::updateAfterTextChange(Range<int> rangeToInvalidate)
{
	if (!skipTextUpdate)
	{
		document.invalidate(rangeToInvalidate);
		
		if (languageManager != nullptr && rangeToInvalidate.getLength() > 1)
		{
			updateLineRanges();
		}

		updateSelections();

		if(rangeToInvalidate.getLength() != 0 &&
			rangeToInvalidate.getLength() != document.getNumRows())
			autocompleteTimer.startAutocomplete();
			
		updateViewTransform();

		if(currentError != nullptr)
			currentError->rebuild();

		for (auto w : warnings)
			w->rebuild();
	}
}

void TextEditor::setPopupLookAndFeel(LookAndFeel* ownedLaf)
{
	plaf = ownedLaf;
}

void TextEditor::setIncludeDotInAutocomplete(bool shouldInclude)
{
	includeDotInAutocomplete = shouldInclude;
}

int TextEditor::getFirstLineOnScreen() const
{
	auto rows = document.getRangeOfRowsIntersecting(getLocalBounds().toFloat().transformed(transform.inverted()));
	return rows.getStart();
}

void TextEditor::searchItemsChanged()
{
	if (document.getNumSelections() == 0)
		return;

	auto selectedLine = document.getSelection(0).head.x;

	Range<int> visibleLines = document.getRangeOfRowsIntersecting(getLocalBounds().toFloat().transformed(transform.inverted()));

	if (!visibleLines.contains(selectedLine))
	{
		auto firstLineToShow = jmax(0, selectedLine - 4);
		setFirstLineOnScreen(firstLineToShow);
	}

	updateSelections();
	repaint();
}

void TextEditor::setFirstLineOnScreen(int firstRow)
{
	translation.y = -document.getVerticalPosition(firstRow, TextDocument::Metric::top) * viewScaleFactor;
	translateView(0.0f, 0.0f);
}

void TextEditor::setTokenTooltipFunction(const TokenTooltipFunction& f)
{
	tokenTooltipFunction = f;
}

bool TextEditor::isLiveParsingEnabled() const
{ return enableLiveParsing; }

bool TextEditor::isPreprocessorParsingEnabled() const
{ return enablePreprocessorParsing; }

bool TextEditor::isReadOnly() const
{ return readOnly; }

void TextEditor::addPopupMenuFunction(const PopupMenuFunction& pf, const PopupMenuResultFunction& rf)
{
	popupMenuFunctions.add(pf);
	popupMenuResultFunctions.add(rf);
}

TextDocument& TextEditor::getTextDocument()
{ return document; }

void TextEditor::addKeyPressFunction(const KeyPressFunction& kf)
{
	keyPressFunctions.add(kf);
}

void TextEditor::setEnableBreakpoint(bool shouldBeEnabled)
{
	gutter.setBreakpointsEnabled(shouldBeEnabled);
}

void TextEditor::setCodeTokeniser(juce::CodeTokeniser* ownedTokeniser)
{
	tokeniser = ownedTokeniser;
	colourScheme = tokeniser->getDefaultColourScheme();
}

void TextEditor::setEnableAutocomplete(bool shouldBeEnabled)
{
	autocompleteEnabled = shouldBeEnabled;
	currentAutoComplete = nullptr;
}

LanguageManager* TextEditor::getLanguageManager()
{ return languageManager; }

ScrollBar& TextEditor::getVerticalScrollBar()
{ return scrollBar; }

TextEditor::AutocompleteTimer::AutocompleteTimer(TextEditor& p):
	parent(p)
{}

void TextEditor::AutocompleteTimer::startAutocomplete()
{
	auto updateSpeed = parent.currentAutoComplete != nullptr ? 30 : 400;

	if(parent.showAutocompleteAfterDelay || parent.currentAutoComplete != nullptr)
		startTimer(updateSpeed);
}

void TextEditor::AutocompleteTimer::abortAutocomplete()
{
	stopTimer();
}

void TextEditor::AutocompleteTimer::timerCallback()
{
	parent.updateAutocomplete();
	stopTimer();
}

void TextEditor::setLineBreakEnabled(bool shouldBeEnabled)
{
	if (linebreakEnabled != shouldBeEnabled)
	{
		linebreakEnabled = !linebreakEnabled;

		if (linebreakEnabled)
			xPos = 0.0f;

		resized();
		refreshLineWidth();
	}
}

bool TextEditor::expand(TextDocument::Target target)
{
	document.navigateSelections(target, TextDocument::Direction::backwardCol, Selection::Part::tail);
	document.navigateSelections(target, TextDocument::Direction::forwardCol, Selection::Part::head);
	updateSelections();
	return true;
}

bool TextEditor::expandBack(TextDocument::Target target, TextDocument::Direction direction)
{
	document.navigateSelections(target, direction, Selection::Part::head);
	translateToEnsureCaretIsVisible();
	updateSelections();
	return true;
}

bool TextEditor::nav(ModifierKeys mods, TextDocument::Target target, TextDocument::Direction direction)
{
	lastInsertWasDouble = false;



	bool isLineSwap = mods.isCommandDown() && mods.isShiftDown() && (direction == TextDocument::Direction::backwardRow ||
		direction == TextDocument::Direction::forwardRow);

	auto currentSelection = document.getSelection(0).oriented();

	auto up = direction == TextDocument::Direction::backwardRow;

	Range<int> currentLineRange(currentSelection.head.x, currentSelection.tail.x);

	isLineSwap &= (currentLineRange.getStart() > 0 || !up);
	isLineSwap &= (currentLineRange.getEnd() < (document.getNumRows()-1) || up);

	if (mods.isCommandDown() && mods.isShiftDown() && target == TextDocument::Target::word)
		return true;



	if (isLineSwap && document.getNumSelections() == 1)
	{
			

		auto prevSelection = document.getSelection(0).oriented();

			

		document.setSelection(0, prevSelection, true);

		if(prevSelection.head.y != 0)
			document.navigateSelections(TextDocument::Target::line, TextDocument::Direction::backwardCol, Selection::Part::head);

		document.navigateSelections(TextDocument::Target::line, TextDocument::Direction::forwardCol, Selection::Part::tail);
		document.navigateSelections(TextDocument::Target::character, TextDocument::Direction::forwardCol, Selection::Part::tail);

		auto content = document.getSelectionContent(document.getSelection(0));

		insert("");

		auto s = document.getSelection(0).oriented();

		auto delta = up ? -1 : 1;

		s.head.x += delta;
		s.tail.x += delta;

		document.setSelection(0, s, true);
		insert(content);

		prevSelection.head.x += delta;
		prevSelection.tail.x += delta;

		document.setSelection(0, prevSelection, true);

		abortAutocomplete();
            
		return true;
	}

	if (mods.isShiftDown())
		document.navigateSelections(target, direction, Selection::Part::head);
	else
		document.navigateSelections(target, direction, Selection::Part::both);

	translateToEnsureCaretIsVisible();
	updateSelections();
	return true;
}

TextEditor::Error::Error(TextDocument& doc_, const String& e, bool isWarning_):
	document(doc_),
	isWarning(isWarning_)
{
	auto s = e.fromFirstOccurrenceOf("Line ", false, false);
	auto l = s.getIntValue() - 1;

	auto useDefaultJuceErrorFormat = s.contains(", column ");

	auto c = s.fromFirstOccurrenceOf(useDefaultJuceErrorFormat ? "column " : "(", false, false).upToFirstOccurrenceOf(")", false, false).getIntValue();
	errorMessage = s.fromFirstOccurrenceOf(": ", false, false);

	Point<int> pos, endPoint;

	auto maxLength = document.doc.getLine(l).trimCharactersAtEnd(" \t\n").length();

	if (c >= (maxLength-1))
		c = -1;

	if (c == -1)
	{
		isEntireLine = true;
		pos = Point<int>(l, 0);
				
		document.navigate(pos, TextDocument::Target::line, mcl::TextDocument::Direction::forwardCol);

		endPoint = pos;
				

		document.navigate(pos, TextDocument::Target::firstnonwhitespace, mcl::TextDocument::Direction::backwardCol);
	}
	else
	{
		pos = Point<int>(l, c);

		document.navigate(pos, TextDocument::Target::subwordWithPoint, TextDocument::Direction::backwardCol);
		endPoint = pos;
		document.navigate(endPoint, TextDocument::Target::subwordWithPoint, TextDocument::Direction::forwardCol);

		if (pos == endPoint)
			endPoint.y += 1;
	}

	start = CodeDocument::Position(document.getCodeDocument(), pos.x, pos.y);
	end = CodeDocument::Position(document.getCodeDocument(), endPoint.x, endPoint.y);
	start.setPositionMaintained(true);
	end.setPositionMaintained(true);

	rebuild();
}

Selection TextEditor::Error::getSelection() const
{
	return Selection(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());
}

void TextEditor::Error::paintLine(Line<float> l, Graphics& g, const AffineTransform& transform, Colour c)
{
	l.applyTransform(transform);
	Path p;
	p.startNewSubPath(l.getStart());

	auto startX = jmin(l.getStartX(), l.getEndX());
	auto endX = jmax(l.getStartX(), l.getEndX());
	auto y = l.getStartY() - 2.0f;

	float delta = 2.0f;
	float deltaY = delta * 0.5f;

	for (float s = startX + delta; s < endX; s += delta)
	{
		deltaY *= -1.0f;
		p.lineTo(s, y + deltaY);
	}

	p.lineTo(l.getEnd());

	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.0f));
}

void TextEditor::Error::paintLines(Graphics& g, const AffineTransform& transform, Colour c)
{
	for (auto l : errorLines)
	{
		paintLine(l, g, transform, c);
				
	}
}

TooltipWithArea::Data TextEditor::Error::getTooltip(const AffineTransform& transform, Point<float> position)
{
	auto a = area.transformed(transform);

	TooltipWithArea::Data d;

	if (a.contains(position))
	{
		d.text = errorMessage;
		d.relativePosition = a.getBottomLeft().translated(0.0f, 5.0f);

		d.id = String(d.relativePosition.toString().hash());

		d.clickAction = {};
	}

	return d;
}

void TextEditor::Error::rebuild()
{
	if (isEntireLine)
	{
		Selection errorWord(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());
		errorLines = document.getUnderlines(errorWord, mcl::TextDocument::Metric::baseline);
		area = document.getSelectionRegion(errorWord).getRectangle(0).withWidth(errorLines[0].getLength());
	}
	else
	{
		Selection errorWord(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());
		errorLines = document.getUnderlines(errorWord, mcl::TextDocument::Metric::baseline);
		area = document.getSelectionRegion(errorWord).getRectangle(0);
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

void TextEditor::focusLost(FocusChangeType t)
{
	if (onFocusChange)
		onFocusChange(false, t);

	auto newFocus = Component::getCurrentlyFocusedComponent();


	// Do not close the autocomplete when the user clicks on the help popup
	if (newFocus != nullptr && newFocus->findParentComponentOfClass<SimpleMarkdownDisplay>() != nullptr)
		return;

	closeAutocomplete(true, {}, {});

	caret.stopTimer();
	caret.repaint();
}

void TextEditor::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	if (scrollBarRecursion)
		return;

	auto b = document.getBounds();
	
	auto pos = newRangeStart;

	if (scrollBarThatHasMoved == &scrollBar)
		translation.y = jlimit<float>(-b.getHeight() * viewScaleFactor, 0.0f, -pos * viewScaleFactor);
	else
	{
		translation.x = -pos * viewScaleFactor;

		if (translation.x == 0)
			translation.x = gutter.getGutterWidth();

		xPos = translation.x;
	}

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
	if (!autocompleteEnabled)
		return;

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

	tokenBefore = tokenBefore.removeCharacters("!");

	auto hasDotAndNotFloat = !CharacterFunctions::isDigit(tokenBefore[0]) && tokenBefore.endsWith(".");

	auto lineNumber = o.x;

	auto parent = TopLevelWindowWithOptionalOpenGL::findRoot(this); 

	if(parent == nullptr)
		parent = dynamic_cast<Component*>(findParentComponentOfClass<TopLevelWindowWithKeyMappings>());

	if (parent == nullptr)
		parent = this;

	if(tokenCollection != nullptr)
		tokenCollection->updateIfSync();

	if (forceShow || ((input.isNotEmpty() && tokenCollection != nullptr && tokenCollection->hasEntries(input, tokenBefore, lineNumber)) || hasDotAndNotFloat))
	{
		if (!hasKeyboardFocus(true))
		{
			currentAutoComplete = nullptr;
			return;
		}

		if (currentAutoComplete != nullptr)
			currentAutoComplete->setInput(input, tokenBefore, lineNumber);
		else if(tokenCollection != nullptr)
		{
			parent->addAndMakeVisible(currentAutoComplete = new Autocomplete(tokenCollection, input, tokenBefore, lineNumber, this));
			addKeyListener(currentAutoComplete);
		}

        if(currentAutoComplete == nullptr)
            return;
        
		auto sToUse = input.isEmpty() ? o.translated(0, 1) : s;

		auto cBounds = document.getBoundsOnRow(sToUse.x, { sToUse.y, sToUse.y + 1 }, GlyphArrangementArray::ReturnLastCharacter).getRectangle(0);
		auto topLeft = cBounds.getBottomLeft();

		auto ltl = topLeft.roundToInt().transformedBy(transform);

		if(parent != this)
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

void TextEditor::threadStateChanged(bool isRunning)
{
	tokenRebuildPending = isRunning;
	SafeAsyncCall::repaint(this);
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

    Point<float> newPos;
    
    if(!linebreakEnabled)
        newPos = pos.transformedBy(transform);
    else
    {
        newPos = document.getPosition(currentLine, TextDocument::Metric::baseline);
        newPos = newPos.transformedBy(transform);
    }
    
    auto dy = newPos.y - posOnScreen.y;
    translateView(0.0f, -dy);
}

void TextEditor::insertCodeSnippet(const String& textToInsert, Array<Range<int>> selectRanges)
{
    auto textWithoutScope = textToInsert;
    Array<Range<int>> rangesWithScope = selectRanges;
    
    auto lr = document.getFoldableLineRangeHolder();
    if(auto n = lr.getRangeContainingLine(autocompleteSelection.head.x))
    {
        auto scopeId = n->getBookmark().name.replace("namespace ", "").upToFirstOccurrenceOf("(", false, false) + ".";
        
        if(textToInsert.startsWith(scopeId))
        {
            textWithoutScope = textToInsert.fromFirstOccurrenceOf(scopeId, false, false);
            
            auto lengthToSubtract = scopeId.length();
            
            rangesWithScope.clear();
            
            for(auto& sr: selectRanges)
                rangesWithScope.add(sr - (int)lengthToSubtract);
        }
    }
    
    if(textWithoutScope.contains("\n"))
    {
        // Intend
        auto start = autocompleteSelection.head;
        auto end = autocompleteSelection.head;
        document.navigate(start, TextDocument::Target::line, TextDocument::Direction::backwardCol);
        document.navigate(end, TextDocument::Target::firstnonwhitespace, TextDocument::Direction::backwardCol);

        Selection emptyBeforeText(end, start);

        auto ws = document.getSelectionContent(emptyBeforeText);
        
        if(ws.isNotEmpty())
        {
            rangesWithScope.clear();
            textWithoutScope = textWithoutScope.replace("\n", "\n" + ws);
        }
    }
    
	ScopedValueSetter<bool> svs(skipTextUpdate, true);
	document.setSelections({ autocompleteSelection }, false);

	insert(textWithoutScope);

	auto s = document.getSelection(0).oriented();
	CodeDocument::Position insertStart(document.getCodeDocument(), s.tail.x, s.tail.y);

	refreshLineWidth();

	document.rebuildRowPositions();

	updateViewTransform();
	translateView(0.0f, 0.0f);

	auto l = textWithoutScope.length();

	if (currentParameterSelection.size() == 0)
		setParameterSelectionInternal(currentParameterSelection, nullptr, true);

	if (currentParameter == nullptr)
	{
		clearParameters(true);

		if (!rangesWithScope.isEmpty())
		{
			Action::List newList;

			for (auto sr : rangesWithScope)
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

void TextEditor::closeAutocomplete(bool async, const String& textToInsert, Array<Range<int>> selectRanges)
{
	if (!autocompleteEnabled)
		return;

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
                insertCodeSnippet(textToInsert, selectRanges);
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

	auto content = document.getSelectionContent(s);

	if(content.containsNonWhitespaceChars())
		SystemClipboard::copyTextToClipboard(content);

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
    if(document.getNumSelections() != 1)
    {
        multiSelection.clear();
        
        for(int i = 0; i < document.getNumSelections(); i++)
        {
            multiSelection.add(document.getSelectionContent(document.getSelection(i)));
        }
    }
    
	auto s = document.getSelections().getFirst();
	
	if (s.isSingular())
	{
		// expand to whole line
		document.navigate(s.head, TextDocument::Target::line, TextDocument::Direction::backwardCol);
		document.navigate(s.head, TextDocument::Target::character, TextDocument::Direction::backwardCol);
		document.navigate(s.tail, TextDocument::Target::line, TextDocument::Direction::forwardCol);
	}

	auto content = document.getSelectionContent(s);

	SystemClipboard::copyTextToClipboard(content);
	return true;
}

bool TextEditor::paste()
{
    if(document.getNumSelections() == multiSelection.size())
    {
        for(int i = 0; i < multiSelection.size(); i++)
        {
            Transaction t;
            t.content = multiSelection[i];
            t.selection = document.getSelection (i);
            
            auto callback = [this] (const Transaction& r)
            {
                translateToEnsureCaretIsVisible();
                updateSelections();
            };

            ScopedPointer<UndoableAction> op = t.on(document, callback);
            op->perform();
        }
        
        repaint();
        return true;
    }
    
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
            auto trimmed = (whitespaceToRemove.isEmpty() || !l.startsWith(whitespaceToRemove)) ? l : l.fromFirstOccurrenceOf(whitespaceToRemove, false, false);

			if (first)
			{
				l = l.trimCharactersAtStart(" \t");
				first = false;
			}
			else
				l = prevws + trimmed;
		}

		insertText = sa.joinIntoString("\n");
	}

	auto ok = insert(insertText);
    abortAutocomplete();
    return ok;
}

void TextEditor::displayedLineRangeChanged(Range<int> newRange)
{
	if (!scrollRecursion)
	{
		scrollToLine(newRange.getStart() + newRange.getLength() / 2, true);
	}
}

void TextEditor::setLanguageManager(LanguageManager* ownedLanguageManager)
{
	languageManager = ownedLanguageManager;

	if (languageManager != nullptr)
	{
		ownedLanguageManager->setupEditor(this);
		setCodeTokeniser(languageManager->createCodeTokeniser());
        updateLineRanges();
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
                    translateToEnsureCaretIsVisible();
					showClosures = true;
					return;
				}
			}
		}

		currentClosure[0] = {};
		currentClosure[1] = s;
		showClosures = true;
        translateToEnsureCaretIsVisible();
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

	// Ensure that there is no weird gap
	if (translation.x > 0.0)
		translation.x = thisGutterWidth;

	closeAutocomplete(true, {}, {});

	transform = AffineTransform::scale(viewScaleFactor).translated(translation.x, translation.y);
	highlight.setViewTransform(transform);
	caret.setViewTransform(transform);
	gutter.setViewTransform(transform);
	linebreakDisplay.setViewTransform(transform);
	
	auto b = document.getBounds();

	{
		ScopedValueSetter<bool> svs(scrollBarRecursion, true);
		scrollBar.setRangeLimits({ b.getY(), b.getBottom() });
		auto visibleRange = getLocalBounds().toFloat().transformed(transform.inverted());
		scrollBar.setCurrentRange({ visibleRange.getY(), visibleRange.getBottom() }, sendNotificationSync);
	}

	if (!linebreakEnabled)
	{
		ScopedValueSetter<bool> svs(scrollBarRecursion, true);
		horizontalScrollBar.setRangeLimits({ b.getX(), b.getRight() });
		auto visibleRange = getLocalBounds().toFloat().transformed(transform.inverted());
		horizontalScrollBar.setCurrentRange({ visibleRange.getX(), visibleRange.getRight() }, sendNotificationSync);
	}

	auto rows = document.getRangeOfRowsIntersecting(getLocalBounds().toFloat().transformed(transform.inverted()));
	ScopedValueSetter<bool> svs(scrollRecursion, true);
	document.setDisplayedLineRange(rows);
    
    
    if(auto fe = dynamic_cast<FullEditor*>(getParentComponent()))
    {
        currentTitles.clearQuick();
        
        if(showStickyLines)
        {
            fe->foldMap.addLineNumbersForParentItems(currentTitles, rows.getStart()+1);
        }        
    }
    
    repaint();
}

void mcl::TextEditor::updateSelections()
{
    tokenSelection.clear();
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
	
    auto visibleHeight = getHeight();
    
    if(currentSearchBox != nullptr)
        visibleHeight -= currentSearchBox->getHeight() * 2;
    
    if (t.y < 0.f)
        translateView (0.f, -t.y);
    else if (b.y > visibleHeight)
        translateView (0.f, -b.y + visibleHeight);

	if (document.getFoldableLineRangeHolder().isFolded(i.x))
		document.getFoldableLineRangeHolder().unfold(i.x);
}


void mcl::TextEditor::resized()
{
	auto b = getLocalBounds();
	caret.setBounds(b);
	
    
    
	scrollBar.setBounds(b.removeFromRight(14));

	if(linebreakEnabled)
		linebreakDisplay.setBounds(b.removeFromRight(15));

	maxLinesToShow = b.getWidth() - TEXT_INDENT - 10;

	refreshLineWidth();
	
	

    highlight.setBounds (b);
	highlight.updateSelections();
    
    horizontalScrollBar.setVisible(!linebreakEnabled);
    
    auto b2 = getLocalBounds();
    b2.removeFromLeft(gutter.getGutterWidth());
    horizontalScrollBar.setBounds(b2.removeFromBottom(14));
    gutter.setBounds (b);
    
    if(currentSearchBox != nullptr)
    {
        currentSearchBox->setBounds(getLocalBounds().removeFromBottom(document.getRowHeight() * transform.getScaleFactor() * 1.2f + 5));
    }
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

    String renderSchemeString;
	renderTextUsingGlyphArrangement(g);

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

    for(auto b: tokenSelection)
    {
        auto ra = document.getSelectionRegion(b);
        
        g.setColour(Colours::white.withAlpha(0.5f));
        
        auto ar = ra.getBounds();
        
        auto area = ar.withHeight(document.getRowHeight())
                      .reduced(0.0f, 0.8f)
                      .transformedBy(transform);
        
        g.drawRoundedRectangle(area, 3.0f, 1.0f);
        
    }
    
	if (currentError != nullptr)
	{
		currentError->paintLines(g, transform, Colours::red);
		g.setColour(Colours::red.withAlpha(0.08f));
		auto b = currentError->area.transformedBy(transform).withLeft(gutter.getGutterWidth()).withRight(getWidth());;
		g.fillRect(b);
	}
    
    Array<LanguageManager::InplaceDebugValue> inplaceDebugValues;
    
    if(languageManager != nullptr && languageManager->getInplaceDebugValues(inplaceDebugValues))
    {
        for(const auto& ip: inplaceDebugValues)
        {
			auto line = ip.location.getLineNumber();
			auto col = ip.location.getIndexInLine();

            auto b = document.getBoundsOnRow(line, {col, col+1}, GlyphArrangementArray::ReturnBeyondLastCharacter).getRectangle(0);
            
            b = b.translated(document.getCharacterRectangle().getWidth() * 1.0f, 0.0f);
            
            auto area = b.transformed(transform).withRight(getWidth());
            auto vf = document.getFont().withHeight(document.getFontHeight() * viewScaleFactor * 0.7f);
            g.setFont(vf);
            g.setColour(Colours::white.withAlpha(0.05f));
            
            auto r = area;
            r = r.removeFromLeft(vf.getStringWidthFloat(ip.value) + 20.0f);
            
            g.fillRoundedRectangle(r, r.getHeight() / 2.0f);
            
            g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));
            g.drawText(ip.value, area.reduced(10.0f, 0.0f), Justification::left);
        }
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
	
	auto& h = document.getFoldableLineRangeHolder();

	auto rows = document.getRangeOfRowsIntersecting(g.getClipBounds().toFloat());

	for(int i = rows.getStart(); i < rows.getEnd(); i++)
	{
		if(h.scopeStates[i])
		{
			g.setColour(Colour(0xff88bec5).withAlpha(JUCE_LIVE_CONSTANT_OFF(0.03f)));
			auto y = document.getVerticalPosition(i, TextDocument::Metric::top);
			auto b = document.getVerticalPosition(i, TextDocument::Metric::bottom);
			auto x = 0.0f;
			auto w = (float)getWidth();

			Rectangle<float> area(x, y, w, b-y);
			g.fillRect(area.transformedBy(transform));
		}
	}


    if(!currentTitles.isEmpty())
    {
        auto titleArea = getLocalBounds().toFloat();
        auto sf = transform.getScaleFactor();
        auto f = document.getFont();
        auto f1 = f.withHeight(f.getHeight() * sf * 0.8f);
        auto f2 = f.withHeight(f.getHeight() * sf);
        const auto titleHeight = document.getRowHeight() * sf;
        
        int idx = 0;
        
        auto hitArea = titleArea;
            
        hitArea = hitArea.removeFromTop(titleHeight * (currentTitles.size()));
        
        for(const auto& s: caret.getCaretRectangles())
        {
            if(hitArea.intersects(s))
                return;
        }
        
        for(const auto& t: currentTitles)
        {
            if(idx++ >= 2)
                break;
            
            CodeDocument::Position pos(document.doc, t, 0);
            CodeDocument::Iterator it(pos);
            
            Array<std::array<int, 3>> tokens;
            
            int previous = 0;
            
            while (it.getLine() == t && !it.isEOF())
            {
                int tokenType = -1;

                if (tokeniser != nullptr)
                    tokenType = tokeniser->readNextToken(it);
                else
                    tokenType = JavascriptTokeniserFunctions::readNextToken(it);
            
                int now;
                
                if(it.getLine() != t)
                    now = document.getLine(t).length();
                else
                    now = it.getIndexInLine();

                if (previous != now)
                    tokens.add({previous, now, tokenType});
                else
                    break;

                previous = now;
            }

            juce::AttributedString text;
            
            auto content = document.doc.getLine(t);
            
            int numCharsToPrint = 0;
            
            for(auto& tk: tokens)
            {
                auto s = content.substring(tk[0], tk[1]);
                auto c = colourScheme.types[tk[2]].colour;
                text.append(s.replace("\t", "    "), f2, c);
                
                numCharsToPrint += s.length();
            }
            
            if(numCharsToPrint < 3)
            {
                continue;
            }
            
            auto ta = titleArea.removeFromTop(titleHeight);
            
            g.setColour(Colour(0xFF333333));
            g.fillRect(ta.reduced(-0.2f));
            
            
            auto la = ta.removeFromLeft(gutter.getGutterWidth());
            la.removeFromRight(15.0f * sf);
            
            g.setColour(Colours::white.withAlpha(0.3f));
            g.setFont(f1);
            g.drawText(String(t+1), la.reduced(5.0f, 0.0f), Justification::right);
            
            float deltaX = JUCE_LIVE_CONSTANT_OFF(6.0f) * sf;
            float deltaY = JUCE_LIVE_CONSTANT_OFF(3.0f) * sf;
            
            text.draw(g, ta.translated(deltaX, deltaY));
        
            g.setColour(Colour(0x44333333));
            g.fillRect(ta);
            
            g.setColour(Colour(0xff454545));
            g.fillRect(titleArea.removeFromTop(1.0f));
            //g.setFont(f2);
            //g.drawText(document.doc.getLine(t), ta, Justification::left);
        }
        
        
        
        g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.3f),
                                         0.0f,
                                         (float)titleArea.getY(),
                                         Colours::transparentBlack,
                                         0.0f,
                                         (float)titleArea.getY() + 10.0f,
                                         false));
        
        g.fillRect(titleArea.removeFromTop(10.0f));
    }
}

void mcl::TextEditor::mouseDown (const MouseEvent& e)
{
	if (e.mods.isX1ButtonDown() || e.mods.isX2ButtonDown())
		return;

    tokenSelection.clear();
    
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
    else if (e.mods.isRightButtonDown() || e.mods.isMiddleButtonDown())
    {
		PopupLookAndFeel pplaf;
		PopupMenu menu;
		menu.setLookAndFeel(&pplaf);

		if (e.mods.isMiddleButtonDown() || e.mods.isShiftDown())
		{
			auto bm = document.getBookmarks();

			if (!bm.isEmpty())
			{
				menu.addSectionHeader("Custom Bookmarks");

				for (auto b : bm)
				{
					menu.addItem(b.lineNumber + 1, b.name);
				}
			}
			
			menu.addSeparator();
			menu.addItem(-1, "Add custom bookmark");
			
			auto& holder = document.getFoldableLineRangeHolder();

			if (!holder.roots.isEmpty())
			{
				menu.addSeparator();
				menu.addSectionHeader("Automatic bookmarks");

				for (auto r : holder.roots)
				{
					auto b = r->getBookmark();
					menu.addItem(b.lineNumber + 1, b.name);

					for (auto c : r->children)
					{
						auto b = c->getBookmark();
						menu.addItem(b.lineNumber + 1, "   " + b.name);
					}
				}
			}

			auto r = menu.show();

			if (r == -1)
			{
				document.navigateSelections(mcl::TextDocument::Target::firstnonwhitespace, TextDocument::Direction::backwardCol, Selection::Part::both);

				insert("//! Enter bookmark title\n");
				auto s = document.getSelection(0);
				
				for (int i = 0; i < 21; i++)
					document.navigateLeftRight(s.tail, false);

				document.navigateLeftRight(s.head, false);
				
				document.setSelection(0, s, true);
				return;
			}

			if (r != 0)
				setFirstLineOnScreen(r - 1);

			return;
		}

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
            AutoAutocomplete,
            ShowStickyLines,
			BackgroundParsing,
            FixWeirdTab,
			Preprocessor,
			numCommands
		};

		


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
		menu.addItem(Back, "Back", document.viewUndoManagerToUse->canUndo());
		menu.addItem(Forward, "Forward", document.viewUndoManagerToUse->canRedo());
		menu.addItem(FoldAll, "Fold all", true, false);
		menu.addItem(UnfoldAll, "Unfold all", true, false);
		menu.addItem(LineBreaks, "Enable line breaks", true, linebreakEnabled);
        menu.addItem(AutoAutocomplete, "Autoshow Autocomplete", true, showAutocompleteAfterDelay);
        menu.addItem(ShowStickyLines, "Show sticky lines on top", true, showStickyLines);
        
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
			case Forward: document.viewUndoManagerToUse->redo(); break;
			case Back:    document.viewUndoManagerToUse->undo(); break;
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
				FullEditor::saveSetting(this, TextEditorSettings::LineBreaks, !linebreakEnabled);
				break;
            case AutoAutocomplete:
				FullEditor::saveSetting(this, TextEditorSettings::AutoAutocomplete, !showAutocompleteAfterDelay);
                break;
            case ShowStickyLines:
                FullEditor::saveSetting(this, TextEditorSettings::ShowStickyLines, !showStickyLines);
                break;
        }

      
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

	if (e.mods.isX1ButtonDown())
		return;

	if (e.mods.isX2ButtonDown())
		return;

	if (e.mods.isMiddleButtonDown())
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

			document.setSelections(multiLineSelections, true);
			updateSelections();
		}
		else
		{
			auto selection = document.getSelections().getFirst();

			auto pos = e.position;
			pos.x = jmax(pos.x, gutter.getGutterWidth() + 5);

			selection.head = document.findIndexNearestPosition(pos.transformedBy(transform.inverted()));
			document.setSelections({ selection }, true);
			translateToEnsureCaretIsVisible();
			updateSelections();
		}
    }
}

void mcl::TextEditor::mouseDoubleClick (const MouseEvent& e)
{
	if (e.mods.isX1ButtonDown() || e.mods.isX2ButtonDown())
		return;

	if (readOnly)
		return;

    if (e.getNumberOfClicks() == 2)
    {
        document.navigateSelections (TextDocument::Target::subword, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (TextDocument::Target::subword, TextDocument::Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
        
        auto currentToken = document.getSelectionContent(document.getSelection(0));
        
        tokenSelection.clear();
        
        CodeDocument::Position p(document.getCodeDocument(), 0);
        auto firstChar = currentToken[0];
        auto maxIndex = currentToken.length();
        
        while (p.getPosition() < document.getCodeDocument().getNumCharacters())
        {
            if (p.getCharacter() == firstChar)
            {
                auto prev = p.movedBy(-1).getCharacter();
                
                
                
                auto e = p.movedBy(maxIndex);

                auto after = e.getCharacter();
                
                
                
                auto t = document.getCodeDocument().getTextBetween(p, e);

                if (currentToken == t)
                {
                    if(CharacterFunctions::isDigit(after) ||
                       CharacterFunctions::isLetter(after))
                    {
                        p.moveBy(1);
                        continue;
                    }
                    
                    if(CharacterFunctions::isDigit(prev) ||
                       CharacterFunctions::isLetter(prev))
                    {
                        p.moveBy(1);
                        continue;
                    }
                    
                    Point<int> ps(p.getLineNumber(), p.getIndexInLine());
                    Point<int> pe(e.getLineNumber(), e.getIndexInLine());

                    
                    
                    tokenSelection.add({ ps, pe });
                }
            }

            p.moveBy(1);
        }
        
        tokenSelection.removeAllInstancesOf(document.getSelection(0));
        
        repaint();
        return;
    }
    else if (e.getNumberOfClicks() == 3)
    {
        document.navigateSelections (TextDocument::Target::line, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (TextDocument::Target::line, TextDocument::Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
    }
    updateSelections();
}

void mcl::TextEditor::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& d)
{


	if (e.mods.isCommandDown())
	{
		auto factor = 1.0f + (float)d.deltaY / 5.0f;

		scaleView(factor, 0.0f);
		return;
	}

#if JUCE_MAC
	float factors[2] = { 400.0f, 800.0f };
#else
	float factors[2] = { 80.0f, 160.0f };
#endif

    bool handleXScroll = !linebreakEnabled && (e.mods.isShiftDown() || d.deltaX != 0.0);
    
    
	if (handleXScroll)
	{
        auto vToUse = e.mods.isShiftDown() ? d.deltaY : d.deltaX;
        
		if(vToUse < 0.0f)
			xPos = jmin(-gutter.getGutterWidth(), xPos);

		xPos += vToUse * factors[1];

		auto maxWidth = document.getBounds().getWidth() * viewScaleFactor;
		xPos = jmax(xPos, -maxWidth);

	}
	
    auto yToUse = e.mods.isShiftDown() ? 0.0f : d.deltaY;
    
    translateView(0.0f, yToUse * factors[1]);
}

void mcl::TextEditor::mouseMagnify (const MouseEvent& e, float scaleFactor)
{
    scaleView (scaleFactor, e.position.y);
}


bool TextEditor::keyMatchesId(const KeyPress& k, const Identifier& id)
{
	return TopLevelWindowWithKeyMappings::matches(this, k, id);
}

void TextEditor::initKeyPresses(Component* root)
{
	String category = "Code Editor";

	TopLevelWindowWithKeyMappings::addShortcut(root, category, TextEditorShortcuts::show_autocomplete, "Show Autocomplete", KeyPress(KeyPress::escapeKey));

	TopLevelWindowWithKeyMappings::addShortcut(root, category, TextEditorShortcuts::goto_definition, "Goto definition", KeyPress(KeyPress::F12Key));

	TopLevelWindowWithKeyMappings::addShortcut(root, category, TextEditorShortcuts::show_search, "Search in current file", 
		KeyPress('f', ModifierKeys::commandModifier, 0));

	TopLevelWindowWithKeyMappings::addShortcut(root, category, TextEditorShortcuts::select_token, "Select current token",
		KeyPress('t', ModifierKeys::commandModifier, 0));
    
    TopLevelWindowWithKeyMappings::addShortcut(root, category, TextEditorShortcuts::comment_line, "Toggle comment for line",
        KeyPress('#', ModifierKeys::commandModifier, 0));
}

struct TextEditor::DeactivatedRange
{
    DeactivatedRange(CodeDocument& d, Range<int> lineRange):
      start(d, lineRange.getStart(), 0),
      end(d, lineRange.getEnd(), 0)
    {
        
        
        start.moveBy(-1);
        end.moveBy(-1);
        
        auto c = end.getCharacter();
        
        while(c != 0 && CharacterFunctions::isWhitespace(c))
        {
            end.moveBy(-1);
            c = end.getCharacter();
        }
               
        start.setPositionMaintained(true);
        end.setPositionMaintained(true);
    }
    
    bool contains(int characterPos) const
    {
        return characterPos >= start.getPosition() &&
               characterPos < end.getPosition();
    }
    
    CodeDocument::Position start, end;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeactivatedRange);
};

void TextEditor::setDeactivatedLines(const SparseSet<int>& lines)
{
    if (enablePreprocessorParsing)
    {
        deactivatedLines.clear();
        
        for(int i = 0; i < lines.getNumRanges(); i++)
        {
            deactivatedLines.add(
                new DeactivatedRange(document.getCodeDocument(),
                                    lines.getRange(i)));
        }
        
        repaint();
    }
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
                auto line = document.getLine(s.head.x);
                
                
                
                auto lineBefore = line.substring(0, s.head.y);
                auto lineAfter = line.substring(s.head.y);

				int delta = 0;

				for (int i = 0; i < lineBefore.length(); i++)
				{
					auto thisC = lineBefore[i];

					if (ActionHelpers::isMatchingClosure(thisC, c))
						delta++;
					if (ActionHelpers::isRightClosure(thisC))
						delta--;
				}
                for (int i = 0; i < lineAfter.length(); i++)
                {
                    auto thisC = lineAfter[i];

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

	auto insertIfNotOpen = [this, skipIfClosure](juce_wchar openChar, juce_wchar closeChar)
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


		if (closeChar == '"')
			both |= (numAfter % 2 == 0);

        auto surroundSelection = !s.isSingular();
        
        if(this->currentParameter != nullptr)
            surroundSelection &= !(this->currentParameter->getSelection() == s);
        
        if(surroundSelection)
            text << document.getSelectionContent(s);
        
		if (both)
		{
			text << closeChar;
			
		}
			
        if(!both && closeChar =='\"' && skipIfClosure(closeChar))
        {
            return both;
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
            end.y++;
			document.navigate(start, Target::line, Direction::backwardCol);
			document.navigate(end, Target::firstnonwhitespace, Direction::backwardCol);

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
        auto s = document.getSelections().getLast();
		
        bool isOriented = s.isOriented();
        
        s = s.oriented();
        
		CodeDocument::Position start(document.getCodeDocument(), s.head.x, s.head.y);
		CodeDocument::Position end(document.getCodeDocument(), s.tail.x, s.tail.y);

        
        
        //document.setSelection(document.getNumSelections()-1, s, true);
        
		auto t = document.getCodeDocument().getTextBetween(start, end);

		while (start.getPosition() < document.getCodeDocument().getNumCharacters())
		{
			start.moveBy(1);
			end.moveBy(1);

			auto current = document.getCodeDocument().getTextBetween(start, end);

			if (current == t)
			{
				Selection s(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());

                
                
				if(isOriented != s.isOriented())
                    s = s.swapped();

				document.addSelection(s);
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
    
	

    // =======================================================================================
    if (keyMatchesId(key, TextEditorShortcuts::show_autocomplete))
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
    
#if JUCE_MAC
    if(mods.isAltDown())
    {
        if (key.isKeyCode(KeyPress::rightKey)) return nav(mods, Target::commandTokenNav, Direction::forwardCol);
        if (key.isKeyCode(KeyPress::leftKey))  return nav(mods, Target::commandTokenNav, Direction::backwardCol);
        
        updateAutocomplete();
    }
#endif
    
    
    if (mods.isCtrlDown())
    {
#if !JUCE_MAC
		if (key.isKeyCode(KeyPress::rightKey)) return nav(mods, Target::commandTokenNav, Direction::forwardCol);
		if (key.isKeyCode(KeyPress::leftKey))  return nav(mods, Target::commandTokenNav, Direction::backwardCol);
#endif
        if (key.isKeyCode (KeyPress::downKey )) return nav (mods, Target::word, Direction::forwardCol)  && nav (mods, Target::paragraph, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey   )) return nav (mods, Target::word, Direction::backwardCol) && nav (mods, Target::paragraph, Direction::backwardRow);

		if (key.isKeyCode(KeyPress::deleteKey) && document.getSelection(0).isSingular())
		{
			document.navigateSelections(Target::commandTokenNav, Direction::forwardCol, Selection::Part::head);
			return insert("");
		}

        if (key.isKeyCode (KeyPress::backspaceKey))
        {
            return (expandBack (Target::commandTokenNav, Direction::backwardCol)
                                                            && insert (""));
        }
            
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
        closeAutocomplete(true, {}, {});
        
        if (key.isKeyCode (KeyPress::downKey)) return nav (mods, Target::document, Direction::forwardRow);
        if (key.isKeyCode (KeyPress::upKey  )) return nav (mods, Target::document, Direction::backwardRow);
        
#if JUCE_MAC
        if (key.isKeyCode (KeyPress::leftKey)) return nav(mods, Target::firstnonwhitespaceAfterLineBreak, Direction::backwardCol);
        if (key.isKeyCode (KeyPress::rightKey  )) return nav(mods, Target::lineUntilBreak, Direction::forwardCol);
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

		if (!s.isSingular() || key.getModifiers().isShiftDown())
		{
			CodeDocument::Position start(document.getCodeDocument(), s.head.x, s.head.y);
			CodeDocument::Position end(document.getCodeDocument(), s.tail.x, s.tail.y);

			start.setPositionMaintained(true);
			end.setPositionMaintained(true);

			s = s.oriented();

			Range<int> rows(s.head.x, s.tail.x + 1);

			Array<Selection> lineStarts;

			for (int i = rows.getStart(); i < rows.getEnd(); i++)
				lineStarts.add(Selection(i, 0, i, 0));

			if (mods.isShiftDown())
			{
                if (s.head.y == 0)
                {
                    document.navigateSelections(Target::firstnonwhitespace, Direction::forwardCol, Selection::Part::head);
                    s = document.getSelection(0);
                }
                
                // Do not delete if there is no whitespace
                if(s.head.y == 0)
                    return true;
                
                
					
				Selection prevFirstLine;
				prevFirstLine.tail = s.head;
				prevFirstLine.head = s.head;
				prevFirstLine.head.y = 0;

				auto prevText = document.getSelectionContent(prevFirstLine);

				bool hasWhiteSpaceCharAtStart = prevText[0] == ' ' || prevText[0] == '\t';

				if(!hasWhiteSpaceCharAtStart)
					return true;

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

	if (keyMatchesId(key, TextEditorShortcuts::goto_definition))
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

	if (key.isKeyCode(KeyPress::backspaceKey))
	{
		if (key.getModifiers().isAnyModifierKeyDown())
			return false;

		return remove(Target::character, Direction::backwardCol);
	}        
	if (key.isKeyCode(KeyPress::deleteKey))
	{
		// Deactivate double delete when pressing del key
		lastInsertWasDouble = false;
		return remove(Target::character, Direction::forwardCol);
	}

#if JUCE_WINDOWS || JUCE_LINUX
    // Home/End: End / start of line
	if (key.isKeyCode(KeyPress::homeKey)) return nav(mods, Target::firstnonwhitespaceAfterLineBreak, Direction::backwardCol);
	if (key.isKeyCode(KeyPress::endKey))  return nav(mods, Target::lineUntilBreak, Direction::forwardCol);
#else
    // Home/End: Scroll to beginning
    if (key.isKeyCode(KeyPress::homeKey)) return nav (mods, Target::document, Direction::backwardRow);
    if (key.isKeyCode(KeyPress::endKey))  return nav (mods, Target::document, Direction::forwardRow);
#endif

	if (key == KeyPress('+', ModifierKeys::commandModifier, 0)) { scaleView(1.1f, false); return true; }
	if (key == KeyPress('-', ModifierKeys::commandModifier, 0)) { scaleView(0.9f, false); return true; }

    if (key == KeyPress ('a', ModifierKeys::commandModifier, 0)) return expand (Target::document);
	if (key == KeyPress('d', ModifierKeys::commandModifier, 0))  return addNextTokenToSelection();
    if (key == KeyPress ('l', ModifierKeys::commandModifier, 0)) return expand (Target::line);
    if (key == KeyPress ('z', ModifierKeys::commandModifier, 0))
    {
        return document.getCodeDocument().getUndoManager().undo();
    }
    if (key == KeyPress ('z', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0))
    {
        return document.getCodeDocument().getUndoManager().redo();
    }
    if (keyMatchesId(key, TextEditorShortcuts::select_token))
    {
        document.navigateSelections (TextDocument::Target::subword, TextDocument::Direction::backwardCol, Selection::Part::head);
        document.navigateSelections (TextDocument::Target::subword, TextDocument::Direction::forwardCol,  Selection::Part::tail);
        updateSelections();
        
        return true;
    }
    
	if (keyMatchesId(key, TextEditorShortcuts::comment_line)) // "Cmd + #"
	{
		bool anythingCommented = false;
		bool anythingUncommented = false;

		for (auto s : document.getSelections())
		{
			auto thisOne = languageManager->isLineCommented(document, s);

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
			languageManager->toggleCommentForLine(this, true);

			
		}
		else
		{
			languageManager->toggleCommentForLine(this, false);

			
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
	if (keyMatchesId(key, TextEditorShortcuts::show_search))
	{
		currentSearchBox = new SearchBoxComponent(document, transform.getScaleFactor());
		addAndMakeVisible(currentSearchBox);
		currentSearchBox->addListener(this);
		currentSearchBox->setBounds(getLocalBounds().removeFromBottom(document.getRowHeight() * transform.getScaleFactor() * 1.2f + 5));

		auto s = document.getSelections().getFirst();
		auto sel = document.getSelectionContent(s);

		if(sel.isNotEmpty())
			currentSearchBox->searchField.setText(sel, true);

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
    tokenSelection.clear();
    
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

    // Do not open the autocomplete if we delete a character
    if(content.isEmpty() && currentAutoComplete == nullptr)
    {
        abortAutocomplete();
    }
    
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

	if (enableSyntaxHighlighting && !tokenRebuildPending)
	{
		auto rows = document.getRangeOfRowsIntersecting(g.getClipBounds().toFloat());

		auto realStart = document.getFoldableLineRangeHolder().getNearestLineStartOfAnyRange(rows.getStart());

		rows.setStart(realStart);

		auto index = Point<int>(rows.getStart(), 0);

		auto zones = Array<Selection>();

		CodeDocument::Position pos(document.getCodeDocument(), rows.getStart(), 0);
		CodeDocument::Iterator it(pos);

		Point<int> previous(it.getLine(), it.getIndexInLine());

		while (it.getLine() < rows.getEnd() && !it.isEOF())
		{
            int tokenType = -1;

            auto cpos = it.getPosition();
            
            for(auto dr: deactivatedLines)
            {
                if(dr->contains(cpos))
                {
                    tokenType = JavascriptTokeniser::tokenType_deactivated;
                    JavascriptTokeniserFunctions::readNextToken(it);
                    break;
                }
            }
            
            if(tokenType == -1)
            {
                if (tokeniser != nullptr)
                    tokenType = tokeniser->readNextToken(it);
                else
                    tokenType = JavascriptTokeniserFunctions::readNextToken(it);
            }

			Point<int> now(it.getLine(), it.getIndexInLine());

			if (previous != now)
				zones.add(Selection(previous, now).withStyle(tokenType));
			else
				break;
			
			previous = now;
		}
		
		document.clearTokens(rows);
		document.applyTokens(rows, zones);

        for (int i = rows.getStart(); i < rows.getEnd(); i++)
        {
	        document.drawWhitespaceRectangles(i, g);
        }
            
        for (int n = 0; n < colourScheme.types.size(); ++n)
        {
			g.setColour (colourScheme.types[n].colour);
            document.findGlyphsIntersecting (g.getClipBounds().toFloat(), n).draw (g);
        }
    }
    else
    {
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffcecece)));
        document.findGlyphsIntersecting (g.getClipBounds().toFloat()).draw (g);
    }
    g.restoreState();
}

}
