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


#pragma once


namespace mcl
{
using namespace juce;



//==============================================================================
class GutterComponent : public juce::Component,
						public FoldableLineRange::Listener,
						public UriBreakpoint::Owner::UriBreakpointListener,
						public TooltipWithArea::Client,
						public CodeDocument::Listener,
						public Timer
{
public:

	GutterComponent(TextDocument& document);
	~GutterComponent();
	void setViewTransform(const juce::AffineTransform& transformToUse);
	void updateSelections();

	float getGutterWidth() const;

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged) override;

	void timerCallback() override;

	void triggerBreakpointUpdate();

	void codeDocumentTextInserted(const String& newText, int insertIndex) override;

	void codeDocumentTextDeleted(int startIndex, int endIndex) override;

	void rootWasRebuilt(FoldableLineRange::WeakPtr newRoot) override;

	TooltipWithArea::Data getTooltip(Point<float> pos) override;

	Rectangle<float> getRowBounds(const TextDocument::RowData& r) const;

	void mouseMove(const MouseEvent& event) override;

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	bool hitTest(int x, int y) override;

	void sendBlinkMessage(int n);

	void setBreakpointsEnabled(bool shouldBeEnabled);

	Path createArrow();

	//==========================================================================
	void paint(juce::Graphics& g) override;

	bool injectBreakPoints(String& s);

	void setError(int lineNumber, const String& error);

	void setDiagnostics(const DiagnosticLines& dl);

	void setExternalCallstackMatches(const std::vector<UriLocation::WithExpression>& newExternalCallstack);

	void setCurrentBreakline(const UriError& breakPosition, UriLocationTree::Ptr currentScope);

	bool isBreakLineActive() const { return currentBreakLine.getOwner() != nullptr; }

	void onUriBreakpointAttachTypeChange(const UriBreakpoint& bp, UriBreakpoint::Owner::AttachState newState) override;

	void onUriBreakpointChange(const UriBreakpoint& bp, ChangeType changeType) override;

	void onUriBreakpointClear() 
	{
		breakpoints.clear();
		repaint();
	};

	bool matchesUriLocation(const UriLocation& location) const override;

private:

	std::vector<UriLocation::WithExpression> externalCallstack;
	DiagnosticLines diagnosticLines;

	struct Breakpoint: public ReferenceCountedObject,
					   public Value::Listener,
					   public AsyncUpdater
	{
		using Ptr = ReferenceCountedObjectPtr<Breakpoint>;
		using List = ReferenceCountedArray<Breakpoint>;

		Breakpoint(GutterComponent& parent_, const UriBreakpoint& bp, CodeDocument& doc);;

		void valueChanged(Value& value) override
		{
			if (skipUpdate)
				return;

			triggerAsyncUpdate();
		}

		void handleAsyncUpdate() override
		{
			parent.owner->modifyUriBreakpoint(toUriBreakpoint(), sendNotification);
		}

		UriBreakpoint toUriBreakpoint() const;

		void copyPropertiesFrom(const UriBreakpoint& bp);

		bool skipUpdate = false;

		String processLine(const String& s);

		int getLineNumber() const;

		bool operator==(int i) const;
		operator int() const;

		Path createPath();

		bool hasCustomCondition() const;

		String getCondition() const;

		GutterComponent& parent;

		UriBreakpoint up;

		Value condition;
		Value useCondition;
		Value enabled;
		Value logExpression;
		Value breakIfHit;
		Value blinkIfHit;

		const bool profiling;

		CodeDocument::Position pos;

		using State = UriBreakpoint::Owner::AttachState;

		State currentState = State::Unresolved;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Breakpoint);
	};

	bool breakpointsEnabled = false;

	struct BreaklineDragger
	{
		BreaklineDragger(GutterComponent& g);;
		~BreaklineDragger();

		void startDragging(const MouseEvent& e);

		void dragInstructionMarker(const MouseEvent& e);

		UriLocation originalLocation;
		GutterComponent& parent;
	};

	ScopedPointer<BreaklineDragger> breaklineDragger;

	struct BlinkState
	{
		bool blinkCallback();

		void draw(GutterComponent& gutter, const Array<TextDocument::RowData>& rows, Graphics& g);

		int lineNumber = -1;
		float alpha = 0.4f;
	};

	Breakpoint* getBreakpoint(int lineNumber);

	struct BlinkHandler : public Timer
	{
		BlinkHandler(GutterComponent& parent_);;

		void addBlinkState(int n);

		void clear();

		void timerCallback() override;

		void draw(Graphics& g, const Array<TextDocument::RowData>& rowData);

		Array<BlinkState> blinkStates;

		GutterComponent& parent;
	} blinkHandler;

	
	bool recompileOnBreakpointChange = true;

	bool createProfilingBreakpoints = false;
	
	UriError currentBreakPosition;
	CodeDocument::Position currentBreakLine;
	UriLocationTree::Ptr currentBreakpointScope;

	Breakpoint::List breakpoints;

	TextDocument::RowData hoveredData;

    int errorLine = -1;
	String errorMessage;

	float scaleFactor = 1.0f;

	juce::GlyphArrangement getLineNumberGlyphs(int row) const;
	//==========================================================================
	TextDocument& document;
	juce::AffineTransform transform;
	Memoizer<int, juce::GlyphArrangement> memoizedGlyphArrangements;

	JUCE_DECLARE_WEAK_REFERENCEABLE(GutterComponent);
};





}
