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
	GutterComponent(TextDocument& document);
	void setViewTransform(const juce::AffineTransform& transformToUse);
	void updateSelections();

	float getGutterWidth() const
	{
		auto w = document.getCharacterRectangle().getWidth() * 6;

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

	void setScaleFactor(float newFactor)
	{
		scaleFactor = newFactor;
		repaint();
	}

	

	void setError(int lineNumber, const String& error)
	{
		errorLine = lineNumber;
		errorMessage = error;
		repaint();
	}

private:

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