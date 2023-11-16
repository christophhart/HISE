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

namespace mcl
{
using namespace juce;




//==============================================================================
mcl::CaretComponent::CaretComponent(const TextDocument& document) : document(document)
{
	setInterceptsMouseClicks(false, false);
	startTimerHz(20);
}

void mcl::CaretComponent::setViewTransform(const AffineTransform& transformToUse)
{
	transform = transformToUse;
	repaint();
}

void mcl::CaretComponent::updateSelections()
{
	phase = 0.f;
	repaint();
}

void mcl::CaretComponent::paint(Graphics& g)
{
	auto colour = getParentComponent()->findColour(juce::CaretComponent::caretColourId);
	
	UnblurryGraphics ug(g, *this);

	bool drawCaretLine = document.getNumSelections() == 1 && document.getSelections().getFirst().isSingular();


    for (const auto &r : getCaretRectangles())
	{
		g.setColour(colour.withAlpha(squareWave(phase)));

		auto rf = ug.getRectangleWithFixedPixelWidth(r, 2);
		g.fillRect(rf);

		if (drawCaretLine)
		{
			g.setColour(Colours::white.withAlpha(0.04f));
			g.fillRect(r.withX(0.0f).withWidth(getWidth()));
		}
	}
}

float mcl::CaretComponent::squareWave(float wt) const
{
	if (isTimerRunning())
	{
		auto f = 0.5f * std::sin(wt) + 0.5f;

		if (f > 0.3f)
			return f;

		return 0.0f;
	}
	
	return 0.6f;

	
	
}

void mcl::CaretComponent::timerCallback()
{
	phase += 0.32f;

	for (const auto &r : getCaretRectangles())
		repaint(r.getSmallestIntegerContainer().expanded(3));
}

Array<Rectangle<float>> mcl::CaretComponent::getCaretRectangles() const
{
	Array<Rectangle<float>> rectangles;

	for (const auto& selection : document.getSelections())
	{
		if (document.getFoldableLineRangeHolder().isFolded(selection.head.x))
			continue;

		auto b = document.getGlyphBounds(selection.head, GlyphArrangementArray::ReturnBeyondLastCharacter);

		b = b.removeFromLeft(CURSOR_WIDTH).withSizeKeepingCentre(CURSOR_WIDTH, document.getRowHeight());

		rectangles.add(b.translated(selection.head.y == 0 ? 0 : -0.5f * CURSOR_WIDTH, 0.f)
			.transformedBy(transform)
			.expanded(0.f, 1.f));
	}
	return rectangles;
}



}
