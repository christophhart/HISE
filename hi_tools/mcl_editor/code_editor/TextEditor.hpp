/** ============================================================================
 *
 * TextEditor.hpp
 *
 * Copyright (C) Jonathan Zrake
 *
 * You may use, distribute and modify this code under the terms of the GPL3
 * license.
 * =============================================================================
 */


/**
 * 
 TODO:




 */


#pragma once




namespace mcl
{




//==============================================================================
class TextEditor : public juce::Component,
				   public CodeDocument::Listener,
				   public Selection::Listener,
				   public ScrollBar::Listener,
				   public TooltipWithArea::Client,
				   public SearchBoxComponent::Listener,
				   public ComponentWithDocumentation,
				   public Timer
{
public:

	using TokenTooltipFunction = std::function<String(const String&, int)>;
	using GotoFunction = std::function<int(int lineNumber, const String& token)>;

	using PopupMenuFunction = std::function<void(TextEditor&, PopupMenu&, const MouseEvent& e)>;
	using PopupMenuResultFunction = std::function<bool(TextEditor&, int)>;
	using KeyPressFunction = std::function<bool(const KeyPress& k)>;

    enum class RenderScheme {
        usingAttributedStringSingle,
        usingAttributedString,
        usingGlyphArrangement,
    };

	
	std::function<void(bool, FocusChangeType)> onFocusChange;

    TextEditor(TextDocument& doc);
    ~TextEditor();
    void setFont (juce::Font font);
    void setText (const juce::String& text);
    void translateView (float dx, float dy);
    void scaleView (float scaleFactor, float verticalCenter);

	void scrollToLine(float centerLine, bool roundToLine);

	void timerCallback() override
	{
		document.getCodeDocument().getUndoManager().beginNewTransaction();

		document.viewUndoManager.beginNewTransaction();
	}

	void setReadOnly(bool shouldBeReadOnly)
	{
		readOnly = shouldBeReadOnly;
	}

	int getNumDisplayedRows() const;

	void setShowNavigation(bool shouldShowNavigation)
	{
		resized();
	}

    //==========================================================================
    void resized() override;
    void paint (juce::Graphics& g) override;
    void paintOverChildren (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDoubleClick (const juce::MouseEvent& e) override;
    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& d) override;
    void mouseMagnify (const juce::MouseEvent& e, float scaleFactor) override;

	bool keyMatchesId(const KeyPress& k, const Identifier& id);

	static void initKeyPresses(Component* root);

    bool keyPressed (const juce::KeyPress& key) override;
    juce::MouseCursor getMouseCursor() override;

	void focusGained(FocusChangeType t) override
	{
		if (onFocusChange)
			onFocusChange(true, t);

		caret.startTimer(50);
	}

	bool shouldSkipInactiveUpdate() const;

	void focusLost(FocusChangeType t) override
	{
		tokenCollection.setEnabled(false);

		if (onFocusChange)
			onFocusChange(false, t);

		caret.stopTimer();
		caret.repaint();
	}

	Font getFont() const { return document.getFont(); }

	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

	void codeDocumentTextDeleted(int startIndex, int endIndex) override;

	void setGotoFunction(const GotoFunction& f)
	{
		gotoFunction = f;
	}

	void setDeactivatedLines(SparseSet<int> deactivatesLines_)
	{
		if (enablePreprocessorParsing)
		{
			deactivatesLines = deactivatesLines_;
			repaint();
		}
	}

	void clearWarningsAndErrors()
	{
		currentError = nullptr;
		warnings.clear();
		repaint();
	}

	virtual MarkdownLink getLink() const override;

	void addWarning(const String& errorMessage, bool isWarning=true)
	{
		warnings.add(new Error(document, errorMessage, isWarning));
		repaint();
	}

	void setError(const String& errorMessage)
	{
		if (errorMessage.isEmpty())
			currentError = nullptr;
		else
			currentError = new Error(document, errorMessage, false);

		repaint();
	}

    void abortAutocomplete()
    {
        autocompleteTimer.stopTimer();
    }
    
	void refreshLineWidth()
	{
		auto actualLineWidth = (maxLinesToShow - gutter.getGutterWidth()) / viewScaleFactor;

		if (linebreakEnabled)
			document.setMaxLineWidth(actualLineWidth);
		else
			document.setMaxLineWidth(-1);
	}

	CodeDocument& getDocument() { return document.getCodeDocument(); }

	TooltipWithArea::Data getTooltip(Point<float> position) override
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

	void updateAutocomplete(bool forceShow = false);

	bool gotoDefinition(Selection s1 = {})
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

			return document.jumpToLine(sl);
		}

		return false;
	}

	

	void codeDocumentTextInserted(const String& newText, int insertIndex) override;
	
	struct Action : public UndoableAction
	{
		using List = Autocomplete::ParameterSelection::List;
		using Ptr = Autocomplete::ParameterSelection::Ptr;

		Action(TextEditor* te, List nl, Ptr ncp) :
			editor(te),
			oldList(te->currentParameterSelection),
			newList(std::move(nl)),
			oldCurrent(te->currentParameter),
			newCurrent(ncp)
		{}

		bool undo() override
		{
			if (editor != nullptr)
			{
				editor->setParameterSelectionInternal(oldList, oldCurrent, false);
				return true;
			}

			return false;
		}

		bool perform() override
		{
			if (editor != nullptr)
			{
				editor->setParameterSelectionInternal(newList, newCurrent, false);
				return true;
			}

			return false;
		}

		WeakReference<TextEditor> editor;
		List oldList, newList;
		Ptr oldCurrent, newCurrent;
	};

	void setParameterSelectionInternal(Action::List l, Action::Ptr p, bool useUndo)
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

	void setScaleFactor(float newFactor);

	void clearParameters(bool useUndo=true)
	{
		setParameterSelectionInternal({}, nullptr, useUndo);

#if OLD
		currentParameter = nullptr;
		currentParameterSelection.clear();
		repaint();
#endif
	}

	bool incParameter(bool useUndo=true)
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

	bool setParameterSelection(int index, bool useUndo=true)
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

	void closeAutocomplete(bool async, const String& textToInsert, Array<Range<int>> selectRanges);

    void updateLineRanges()
    {
        auto ranges = languageManager->createLineRange(document.getCodeDocument());
        document.getFoldableLineRangeHolder().setRanges(ranges);
    }
    
	void updateAfterTextChange(Range<int> rangeToInvalidate = Range<int>())
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

	void setPopupLookAndFeel(LookAndFeel* ownedLaf)
	{
		plaf = ownedLaf;
	}

	void setIncludeDotInAutocomplete(bool shouldInclude)
	{
		includeDotInAutocomplete = shouldInclude;
	}

	int getFirstLineOnScreen() const
	{
		auto rows = document.getRangeOfRowsIntersecting(getLocalBounds().toFloat().transformed(transform.inverted()));
		return rows.getStart();
	}

	void searchItemsChanged() override
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

	void setFirstLineOnScreen(int firstRow)
	{
		translation.y = -document.getVerticalPosition(firstRow, TextDocument::Metric::top) * viewScaleFactor;
		translateView(0.0f, 0.0f);
	}

	CodeEditorComponent::ColourScheme colourScheme;
	juce::AffineTransform transform;

	TokenCollection tokenCollection;

	void setTokenTooltipFunction(const TokenTooltipFunction& f)
	{
		tokenTooltipFunction = f;
	}

	void grabKeyboardFocusAndActivateTokenBuilding();

	bool isLiveParsingEnabled() const { return enableLiveParsing; }
	bool isPreprocessorParsingEnabled() const { return enablePreprocessorParsing; }

	bool cut();
	bool copy();
	bool paste();

	void displayedLineRangeChanged(Range<int> newRange) override;

	void translateToEnsureCaretIsVisible();

	bool isReadOnly() const { return readOnly; }

	void addPopupMenuFunction(const PopupMenuFunction& pf, const PopupMenuResultFunction& rf)
	{
		popupMenuFunctions.add(pf);
		popupMenuResultFunctions.add(rf);
	}

	TextDocument& getTextDocument() { return document; }

	bool insert(const juce::String& content);

	bool enableLiveParsing = true;
	bool enablePreprocessorParsing = true;

	void addKeyPressFunction(const KeyPressFunction& kf)
	{
		keyPressFunctions.add(kf);
	}

	void setLanguageManager(LanguageManager* ownedLanguageManager);

	void setEnableBreakpoint(bool shouldBeEnabled)
	{
		gutter.setBreakpointsEnabled(shouldBeEnabled);
	}

	void setCodeTokeniser(juce::CodeTokeniser* ownedTokeniser)
	{
		tokeniser = ownedTokeniser;
		colourScheme = tokeniser->getDefaultColourScheme();
	}

	void setEnableAutocomplete(bool shouldBeEnabled)
	{
		autocompleteEnabled = shouldBeEnabled;
		currentAutoComplete = nullptr;
	}

	ScopedPointer<CodeTokeniser> tokeniser;

    LanguageManager* getLanguageManager() { return languageManager; }
    
	ScrollBar& getVerticalScrollBar() { return scrollBar; }

private:

	ScopedPointer<LanguageManager> languageManager;

	friend class Autocomplete;

    struct AutocompleteTimer: public Timer
    {
        AutocompleteTimer(TextEditor& p):
          parent(p)
        {}
        
        void startAutocomplete()
        {
            auto updateSpeed = parent.currentAutoComplete != nullptr ? 30 : 400;

			if(parent.showAutocompleteAfterDelay || parent.currentAutoComplete != nullptr)
				startTimer(updateSpeed);
        }
        
        void abortAutocomplete()
        {
            stopTimer();
        }
        
        void timerCallback() override
        {
            parent.updateAutocomplete();
            stopTimer();
        }
        
        TextEditor& parent;
    } autocompleteTimer;
    
	bool readOnly = false;

	void setLineBreakEnabled(bool shouldBeEnabled)
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

	bool expand(TextDocument::Target target)
	{
		document.navigateSelections(target, TextDocument::Direction::backwardCol, Selection::Part::tail);
		document.navigateSelections(target, TextDocument::Direction::forwardCol, Selection::Part::head);
		updateSelections();
		return true;
	};

	bool expandBack(TextDocument::Target target, TextDocument::Direction direction)
	{
		document.navigateSelections(target, direction, Selection::Part::head);
		translateToEnsureCaretIsVisible();
		updateSelections();
		return true;
	};

	bool nav(ModifierKeys mods, TextDocument::Target target, TextDocument::Direction direction)
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

	struct Error
	{
		Error(TextDocument& doc_, const String& e, bool isWarning_):
			document(doc_),
			isWarning(isWarning_)
		{
			auto s = e.fromFirstOccurrenceOf("Line ", false, false);
			auto l = s.getIntValue() - 1;
			auto c = s.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false).getIntValue();
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

		Selection getSelection() const
		{
			return Selection(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());
		}
		
		static void paintLine(Line<float> l, Graphics& g, const AffineTransform& transform, Colour c)
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

		void paintLines(Graphics& g, const AffineTransform& transform, Colour c)
		{
			for (auto l : errorLines)
			{
				paintLine(l, g, transform, c);
				
			}
		}

		TooltipWithArea::Data getTooltip(const AffineTransform& transform, Point<float> position)
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

		void rebuild()
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

		bool isEntireLine = false;

		TextDocument& document;

		CodeDocument::Position start;
		CodeDocument::Position end;

		juce::Rectangle<float> area;
		Array<Line<float>> errorLines;

		String errorMessage;
		bool isWarning;
	};

	TooltipWithArea tooltipManager;
    
    ScrollbarFader sf;
    
	
	bool skipTextUpdate = false;
	Selection autocompleteSelection;
	ScopedPointer<Autocomplete> currentAutoComplete;
	CodeDocument& docRef;

    //==========================================================================
    
    void updateViewTransform();
    void updateSelections();

	void selectionChanged() override;
    void renderTextUsingGlyphArrangement (juce::Graphics& g);
    bool enableSyntaxHighlighting = true;
    GotoFunction gotoFunction;

	friend struct FullEditor;

    //==========================================================================
    double lastTransactionTime;
    bool tabKeyUsed = true;
    TextDocument& document;
	ScopedPointer<Error> currentError;

	OwnedArray<Error> warnings;

	Array<PopupMenuFunction> popupMenuFunctions;
	Array<PopupMenuResultFunction> popupMenuResultFunctions;

	Array<KeyPressFunction> keyPressFunctions;

    CaretComponent caret;
    GutterComponent gutter;
    HighlightComponent highlight;
	LinebreakDisplay linebreakDisplay;
	ScrollBar scrollBar;
	ScrollBar horizontalScrollBar;

    Array<Selection> tokenSelection;
    
	SparseSet<int> deactivatesLines;
	bool linebreakEnabled = true;
    float viewScaleFactor = 1.f;
	int maxLinesToShow = 0;
	bool lastInsertWasDouble = false;
	float lastGutterWidth = 0.0f;
    juce::Point<float> translation;
	bool showClosures = false;
	float xPos = 0.0f;

	bool autocompleteEnabled = true;
    bool showAutocompleteAfterDelay = false;
	
	Selection currentClosure[2];

	
	// just used in order to send a scrollbar notification
	bool scrollBarRecursion = false;

	// used for all scrolling
	bool scrollRecursion = false;
	bool includeDotInAutocomplete = false;

    StringArray multiSelection;
    
	Autocomplete::ParameterSelection::List currentParameterSelection;
	Autocomplete::ParameterSelection::Ptr currentParameter;
	CodeDocument::Position postParameterPos;

	TokenTooltipFunction tokenTooltipFunction;
	ScopedPointer<SearchBoxComponent> currentSearchBox;

	ScopedPointer<LookAndFeel> plaf;

	JUCE_DECLARE_WEAK_REFERENCEABLE(TextEditor);
};


}

