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
							 public Value::Listener
{
public:

	struct BreakpointListener
	{
		virtual ~BreakpointListener();;

		virtual void breakpointsChanged(GutterComponent& g);

		JUCE_DECLARE_WEAK_REFERENCEABLE(BreakpointListener);
	};

	GutterComponent(TextDocument& document);
	void setViewTransform(const juce::AffineTransform& transformToUse);
	void updateSelections();

	float getGutterWidth() const;

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged) override;

	void rootWasRebuilt(FoldableLineRange::WeakPtr newRoot) override;

	Rectangle<float> getRowBounds(const TextDocument::RowData& r) const;

	void mouseMove(const MouseEvent& event) override;

	void mouseDown(const MouseEvent& e) override;

	bool hitTest(int x, int y) override;

	void valueChanged(Value& value) override;

	void sendBlinkMessage(int n);

	void setBreakpointsEnabled(bool shouldBeEnabled);

	void sendBreakpointChangeMessage();

	Path createArrow();

	//==========================================================================
	void paint(juce::Graphics& g) override;

	bool injectBreakPoints(String& s);

	void setError(int lineNumber, const String& error);

	void addBreakpointListener(BreakpointListener* l);

	void removeBreakpointListener(BreakpointListener* l);

	void setCurrentBreakline(int n);

private:

	struct Breakpoint: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<Breakpoint>;
		using List = ReferenceCountedArray<Breakpoint>;

		Breakpoint(Value::Listener* listener, int l, CodeDocument& doc);;

		String processLine(const String& s);

		int getLineNumber() const;

		bool operator==(int i) const;
		operator int() const;

		Path createPath();

		bool hasCustomCondition() const;

		String getCondition() const;

		Value condition;
		Value useCondition;
		Value enabled;
		Value logExpression;
		Value breakIfHit;
		Value blinkIfHit;
		
		CodeDocument::Position pos;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Breakpoint);
	};

	bool breakpointsEnabled = false;

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
	

	CodeDocument::Position currentBreakLine;

	Array<WeakReference<BreakpointListener>> listeners;

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
