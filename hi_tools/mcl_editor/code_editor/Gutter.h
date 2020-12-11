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
class mcl::GutterComponent : public juce::Component,
							 public FoldableLineRange::Listener
{
public:

	struct BreakpointListener
	{
		virtual ~BreakpointListener() {};

		virtual void breakpointsChanged(GutterComponent& g) {}

		JUCE_DECLARE_WEAK_REFERENCEABLE(BreakpointListener);
	};

	GutterComponent(TextDocument& document);
	void setViewTransform(const juce::AffineTransform& transformToUse);
	void updateSelections();

	float getGutterWidth() const
	{
		auto numRows = document.getNumRows();
		auto digits = numRows > 0 ? (int)(log10(numRows)) : 0;

		auto w = document.getCharacterRectangle().getWidth() * (digits+4);

		w += breakpoints.isEmpty() ? 0.0f : (document.getCharacterRectangle().getHeight() * 0.6f);

		return w * scaleFactor;
	}

	void foldStateChanged(FoldableLineRange::WeakPtr rangeThatHasChanged) override
	{
		repaint();
	}

	void rootWasRebuilt(FoldableLineRange::WeakPtr newRoot) override
	{
		repaint();
	}

	Rectangle<float> getRowBounds(const TextDocument::RowData& r) const;

	void mouseMove(const MouseEvent& event) override
	{
		repaint();
	}

	void mouseDown(const MouseEvent& e) override;

	bool hitTest(int x, int y) override;

	//==========================================================================
	void paint(juce::Graphics& g) override;

	String injectBreakPoints(const String& s)
	{
		
		currentBreakLine = -1;

		Component::SafePointer<GutterComponent> st(this);

		MessageManager::callAsync([st]()
		{
			if (st)
				st.getComponent()->repaint();
		});

		if (breakpoints.isEmpty())
			return s;

		auto lines = StringArray::fromLines(s);

		for (auto bp : breakpoints)
		{
			if (isPositiveAndBelow(bp, lines.size()))
				lines.set(bp, "Console.stop(1); " + lines[bp]);
		}

		return lines.joinIntoString("\n");
	}
	

	void setError(int lineNumber, const String& error)
	{
		errorLine = lineNumber;
		errorMessage = error;
		repaint();
	}

	void addBreakpointListener(BreakpointListener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeBreakpointListener(BreakpointListener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void setCurrentBreakline(int n)
	{
		currentBreakLine = n;
		repaint();
	}

private:

	int currentBreakLine = -1;

	Array<WeakReference<BreakpointListener>> listeners;

	Array<int> breakpoints;

	TextDocument::RowData hoveredData;

	int errorLine;
	String errorMessage;

	float scaleFactor = 1.0f;

	juce::GlyphArrangement getLineNumberGlyphs(int row) const;
	//==========================================================================
	TextDocument& document;
	juce::AffineTransform transform;
	Memoizer<int, juce::GlyphArrangement> memoizedGlyphArrangements;
};





}