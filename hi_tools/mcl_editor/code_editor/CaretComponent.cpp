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
#if PROFILE_PAINTS
	auto start = Time::getMillisecondCounterHiRes();
#endif

	auto colour = getParentComponent()->findColour(juce::CaretComponent::caretColourId);
	auto outline = colour.contrasting();

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

#if PROFILE_PAINTS
	std::cout << "[CaretComponent::paint] " << Time::getMillisecondCounterHiRes() - start << std::endl;
#endif
}

float mcl::CaretComponent::squareWave(float wt) const
{
	if (isTimerRunning())
	{
		const float delta = 0.222f;
		const float A = 1.0;
		return 0.5f + A / 3.14159f * std::atan(std::cos(wt) / delta);
	}
	
	return 0.6f;

	
	
}

void mcl::CaretComponent::timerCallback()
{
	phase += 3.2e-1;

	for (const auto &r : getCaretRectangles())
		repaint(r.getSmallestIntegerContainer());
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