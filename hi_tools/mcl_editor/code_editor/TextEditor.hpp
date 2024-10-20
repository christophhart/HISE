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
				   public TokenCollection::Listener,
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

	void timerCallback() override;

	static void setNewTokenCollectionForAllChildren(Component* any, const Identifier& languageId, TokenCollection::Ptr newCollection);

	void setReadOnly(bool shouldBeReadOnly);

	int getNumDisplayedRows() const;

	void setShowNavigation(bool shouldShowNavigation);

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

	void focusGained(FocusChangeType t) override;

	bool shouldSkipInactiveUpdate() const;

	void focusLost(FocusChangeType t) override;

	Font getFont() const;

	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override;

	void codeDocumentTextDeleted(int startIndex, int endIndex) override;

	void setGotoFunction(const GotoFunction& f);

	void setDeactivatedLines(const SparseSet<int>& lines);
	

	void clearWarningsAndErrors();

	virtual MarkdownLink getLink() const override;

	void addWarning(const String& errorMessage, bool isWarning=true);

	void setError(const String& errorMessage);

	void abortAutocomplete();

	void refreshLineWidth();

	void insertCodeSnippet(const String& textToInsert, Array<Range<int>> selectRanges);

	CodeDocument& getDocument();

	TooltipWithArea::Data getTooltip(Point<float> position) override;

	void updateAutocomplete(bool forceShow = false);

    void prepareExternalInsert()
    {
        autocompleteSelection = document.getSelection(0);
    }
    
	bool gotoDefinition(Selection s1 = {});

	void tokenListWasRebuild() override {};

	void threadStateChanged(bool isRunning) override;

	void codeDocumentTextInserted(const String& newText, int insertIndex) override;
	
	struct Action : public UndoableAction
	{
		using List = Autocomplete::ParameterSelection::List;
		using Ptr = Autocomplete::ParameterSelection::Ptr;

		Action(TextEditor* te, List nl, Ptr ncp);

		bool undo() override;

		bool perform() override;

		WeakReference<TextEditor> editor;
		List oldList, newList;
		Ptr oldCurrent, newCurrent;
	};

	void setParameterSelectionInternal(Action::List l, Action::Ptr p, bool useUndo);

	void setScaleFactor(float newFactor);

	void clearParameters(bool useUndo=true);

	bool incParameter(bool useUndo=true);

	bool setParameterSelection(int index, bool useUndo=true);

	void closeAutocomplete(bool async, const String& textToInsert, Array<Range<int>> selectRanges);

    void updateLineRanges();

	void updateAfterTextChange(Range<int> rangeToInvalidate = Range<int>());

	void setPopupLookAndFeel(LookAndFeel* ownedLaf);

	void setIncludeDotInAutocomplete(bool shouldInclude);

	int getFirstLineOnScreen() const;

	void searchItemsChanged() override;

	void setFirstLineOnScreen(int firstRow);

	CodeEditorComponent::ColourScheme colourScheme;
	juce::AffineTransform transform;

	TokenCollection::Ptr tokenCollection;

	void setTokenTooltipFunction(const TokenTooltipFunction& f);

	void grabKeyboardFocusAndActivateTokenBuilding();

	bool isLiveParsingEnabled() const;
	bool isPreprocessorParsingEnabled() const;

	bool cut();
	bool copy();
	bool paste();

	void displayedLineRangeChanged(Range<int> newRange) override;

	void translateToEnsureCaretIsVisible();

	bool isReadOnly() const;

	void addPopupMenuFunction(const PopupMenuFunction& pf, const PopupMenuResultFunction& rf);

	TextDocument& getTextDocument();

	bool insert(const juce::String& content);

	bool remove(TextDocument::Target target, TextDocument::Direction direction)
	{
		const auto& s = document.getSelections().getLast();

		auto l = document.getCharacter(s.head.translated(0, -1));
		auto r = document.getCharacter(s.head);
		
		if (lastInsertWasDouble && ActionHelpers::isMatchingClosure(l, r))
		{
			document.navigateSelections(TextDocument::Target::character, TextDocument::Direction::backwardCol, Selection::Part::tail);
			document.navigateSelections(TextDocument::Target::character, TextDocument::Direction::forwardCol, Selection::Part::head);
			
			insert({});
			return true;
		}

		if (s.isSingular())
			expandBack(target, direction);

		insert({});
		return true;
	};

	bool enableLiveParsing = true;
	bool enablePreprocessorParsing = true;

	void addKeyPressFunction(const KeyPressFunction& kf);

	void setLanguageManager(LanguageManager* ownedLanguageManager);

	void setEnableBreakpoint(bool shouldBeEnabled);

	void setCodeTokeniser(juce::CodeTokeniser* ownedTokeniser);

	void setEnableAutocomplete(bool shouldBeEnabled);

	ScopedPointer<CodeTokeniser> tokeniser;

    LanguageManager* getLanguageManager();

	ScrollBar& getVerticalScrollBar();

private:

    Array<int> currentTitles;
    
	ScopedPointer<LanguageManager> languageManager;

	friend class Autocomplete;

    struct AutocompleteTimer: public Timer
    {
        AutocompleteTimer(TextEditor& p);

        void startAutocomplete();

        void abortAutocomplete();

        void timerCallback() override;

        TextEditor& parent;
    } autocompleteTimer;
    
	bool readOnly = false;

	void setLineBreakEnabled(bool shouldBeEnabled);

	bool expand(TextDocument::Target target);;

	bool expandBack(TextDocument::Target target, TextDocument::Direction direction);;

	bool nav(ModifierKeys mods, TextDocument::Target target, TextDocument::Direction direction);

	struct Error
	{
		Error(TextDocument& doc_, const String& e, bool isWarning_);

		Selection getSelection() const;

		static void paintLine(Line<float> l, Graphics& g, const AffineTransform& transform, Colour c);

		void paintLines(Graphics& g, const AffineTransform& transform, Colour c);

		TooltipWithArea::Data getTooltip(const AffineTransform& transform, Point<float> position);

		void rebuild();

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

	bool tokenRebuildPending = false;
	
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
    
    struct DeactivatedRange;
    
    
    OwnedArray<DeactivatedRange> deactivatedLines;
    
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
    bool showStickyLines = true;
	
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

