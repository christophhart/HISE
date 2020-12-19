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
				   public Timer
{
public:

	using TokenTooltipFunction = std::function<String(const String&, int)>;
	using GotoFunction = std::function<int(int lineNumber, const String& token)>;

    enum class RenderScheme {
        usingAttributedStringSingle,
        usingAttributedString,
        usingGlyphArrangement,
    };

	

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
    bool keyPressed (const juce::KeyPress& key) override;
    juce::MouseCursor getMouseCursor() override;

	void focusGained(FocusChangeType t) override
	{
		caret.startTimer(50);
	}

	void focusLost(FocusChangeType t) override
	{
		caret.stopTimer();
		caret.repaint();
	}

	Font getFont() const { return document.getFont(); }

	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

	void codeDocumentTextDeleted(int startIndex, int endIndex) override
	{
		updateAfterTextChange();
	}

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

	void addWarning(const String& errorMessage)
	{
		warnings.add(new Error(document, errorMessage));
		repaint();
	}

	void setError(const String& errorMessage)
	{
		if (errorMessage.isEmpty())
			currentError = nullptr;
		else
			currentError = new Error(document, errorMessage);

		repaint();
	}

	void refreshLineWidth()
	{
		auto firstRow = getFirstLineOnScreen();

		auto actualLineWidth = (maxLinesToShow - gutter.getGutterWidth()) / viewScaleFactor;

		if (linebreakEnabled)
			document.setMaxLineWidth(actualLineWidth);
		else
			document.setMaxLineWidth(-1);

		setFirstLineOnScreen(firstRow);
	}

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

	void updateAutocomplete(bool forceShow = false)
	{
		if (document.getSelections().size() != 1)
		{
			closeAutocomplete(true, {}, {});
			return;
		}

		const auto o = document.getSelections().getFirst().oriented().tail;

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

		auto lineNumber = o.x;
		
		if (forceShow || (input.isNotEmpty() && tokenCollection.hasEntries(input, tokenBefore, lineNumber) || tokenBefore.endsWith(".")))
		{
			if (currentAutoComplete != nullptr)
				currentAutoComplete->setInput(input, tokenBefore, lineNumber);
			else
			{
				addAndMakeVisible(currentAutoComplete = new Autocomplete(tokenCollection, input, tokenBefore, lineNumber));
				addKeyListener(currentAutoComplete);
			}

			auto sToUse = input.isEmpty() ? o.translated(0, 1) : s;

			auto cBounds = document.getBoundsOnRow(sToUse.x, { sToUse.y, sToUse.y + 1 }, GlyphArrangementArray::ReturnLastCharacter).getRectangle(0);
			auto topLeft = cBounds.getBottomLeft();

			currentAutoComplete->setTopLeftPosition(topLeft.roundToInt());

			if (currentAutoComplete->getBoundsInParent().getBottom() > getHeight())
			{
				auto b = cBounds.getTopLeft().translated(0, -currentAutoComplete->getHeight());

				currentAutoComplete->setTopLeftPosition(b.roundToInt());
			}
				

			currentAutoComplete->setTransform(transform);

		}
		else 
			closeAutocomplete(false, {}, {});
	}

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

	

	void codeDocumentTextInserted(const String& newText, int insertIndex) override
	{
		updateAfterTextChange();
	}
	
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

			repaint();
		}
	}

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
		auto s = currentParameterSelection.size();

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

	void updateAfterTextChange()
	{
		if (!skipTextUpdate)
		{
			document.invalidate({});
		
			if (lineRangeFunction)
			{

				auto ranges = lineRangeFunction(document.getCodeDocument());

				

				document.getFoldableLineRangeHolder().setRanges(ranges);
			}

			updateSelections();

			Timer::callAfterDelay(1200, [this]()
			{
				this->updateAutocomplete();
			});

			
			
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

	void setLineRangeFunction(const FoldableLineRange::LineRangeFunction& f)
	{
		lineRangeFunction = f;
	};

	int getFirstLineOnScreen() const
	{
		auto rows = document.getRangeOfRowsIntersecting(getLocalBounds().toFloat().transformed(transform.inverted()));
		return rows.getStart();
	}

	void searchItemsChanged() override
	{
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

	bool isLiveParsingEnabled() const { return enableLiveParsing; }
	bool isPreprocessorParsingEnabled() const { return enablePreprocessorParsing; }

	bool cut();
	bool copy();
	bool paste();

	void displayedLineRangeChanged(Range<int> newRange) override;

	void translateToEnsureCaretIsVisible();

private:

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
		Error(TextDocument& doc_, const String& e):
			document(doc_)
		{
			auto s = e.fromFirstOccurrenceOf("Line ", false, false);
			auto l = s.getIntValue() - 1;
			auto c = s.fromFirstOccurrenceOf("(", false, false).upToFirstOccurrenceOf(")", false, false).getIntValue();
			errorMessage = s.fromFirstOccurrenceOf(": ", false, false);

			Point<int> pos(l, c);
			
			document.navigate(pos, TextDocument::Target::subwordWithPoint, TextDocument::Direction::backwardCol);
			auto endPoint = pos;
			document.navigate(endPoint, TextDocument::Target::subwordWithPoint, TextDocument::Direction::forwardCol);

			if (pos == endPoint)
				endPoint.y += 1;

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
			Selection errorWord(start.getLineNumber(), start.getIndexInLine(), end.getLineNumber(), end.getIndexInLine());
			errorLines = document.getUnderlines(errorWord, mcl::TextDocument::Metric::baseline);
			area = document.getSelectionRegion(errorWord).getRectangle(0);
		}

		TextDocument& document;

		CodeDocument::Position start;
		CodeDocument::Position end;

		juce::Rectangle<float> area;
		Array<Line<float>> errorLines;

		String errorMessage;
	};

	TooltipWithArea tooltipManager;

	
	bool skipTextUpdate = false;
	Selection autocompleteSelection;
	ScopedPointer<Autocomplete> currentAutoComplete;
	CodeDocument& docRef;

    //==========================================================================
    bool insert (const juce::String& content);
    void updateViewTransform();
    void updateSelections();

	void selectionChanged() override;
    

    void renderTextUsingGlyphArrangement (juce::Graphics& g);
    void resetProfilingData();
    bool enableSyntaxHighlighting = true;
    bool allowCoreGraphics = true;
    bool useOpenGLRendering = false;
    bool drawProfilingInfo = false;
    float accumulatedTimeInPaint = 0.f;
    float lastTimeInPaint = 0.f;
    float lastTokeniserTime = 0.f;
    int numPaintCalls = 0;
    RenderScheme renderScheme = RenderScheme::usingGlyphArrangement;
	GotoFunction gotoFunction;

	friend class FullEditor;

    //==========================================================================
    double lastTransactionTime;
    bool tabKeyUsed = true;
    TextDocument& document;
	ScopedPointer<Error> currentError;

	OwnedArray<Error> warnings;

	FoldableLineRange::LineRangeFunction lineRangeFunction;

    CaretComponent caret;
    GutterComponent gutter;
    HighlightComponent highlight;
	LinebreakDisplay linebreakDisplay;
	ScrollBar scrollBar;
	SparseSet<int> deactivatesLines;
	bool linebreakEnabled = true;
    float viewScaleFactor = 1.f;
	int maxLinesToShow = 0;
	bool lastInsertWasDouble = false;
    juce::Point<float> translation;
	bool showClosures = false;
	bool enableLiveParsing = true;
	bool enablePreprocessorParsing = true;
	Selection currentClosure[2];

	bool scrollRecursion = false;

	Autocomplete::ParameterSelection::List currentParameterSelection;
	Autocomplete::ParameterSelection::Ptr currentParameter;
	CodeDocument::Position postParameterPos;

	TokenTooltipFunction tokenTooltipFunction;
	ScopedPointer<SearchBoxComponent> currentSearchBox;

	ScopedPointer<LookAndFeel> plaf;

	JUCE_DECLARE_WEAK_REFERENCEABLE(TextEditor);
};


}

