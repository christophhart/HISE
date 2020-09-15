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



//==========================================================================
mcl::GutterComponent::GutterComponent(TextDocument& document)
	: document(document)
	, memoizedGlyphArrangements([this](int row) { return getLineNumberGlyphs(row); })
{
	setRepaintsOnMouseActivity(true);

}

void mcl::GutterComponent::setViewTransform(const AffineTransform& transformToUse)
{
	transform = transformToUse;
	repaint();
}

void mcl::GutterComponent::updateSelections()
{
	repaint();
}

void mcl::GutterComponent::paint(Graphics& g)
{
#if PROFILE_PAINTS
	auto start = Time::getMillisecondCounterHiRes();
#endif

	/*
	 Draw the gutter background, shadow, and outline
	 ------------------------------------------------------------------
	 */
	auto bg = getParentComponent()->findColour(CodeEditorComponent::backgroundColourId);
	auto ln = bg.overlaidWith(getParentComponent()->findColour(CodeEditorComponent::lineNumberBackgroundId));

	auto GUTTER_WIDTH = getGutterWidth();

	g.setColour(ln);
	g.fillRect(getLocalBounds().removeFromLeft(GUTTER_WIDTH));

	if (transform.getTranslationX() < GUTTER_WIDTH)
	{
		auto shadowRect = getLocalBounds().withLeft(GUTTER_WIDTH).withWidth(12);

		auto gradient = ColourGradient::horizontal(ln.contrasting().withAlpha(0.3f),
			Colours::transparentBlack, shadowRect);
		g.setFillType(gradient);
		g.fillRect(shadowRect);
	}
	else
	{
		g.setColour(ln.darker(0.2f));
		g.drawVerticalLine(GUTTER_WIDTH - 1.f, 0.f, getHeight());
	}

	/*
	 Draw the line numbers and selected rows
	 ------------------------------------------------------------------
	 */
	auto area = g.getClipBounds().toFloat().transformedBy(transform.inverted());
	auto rowData = document.findRowsIntersecting(area);
	auto verticalTransform = transform.withAbsoluteTranslation(0.f, transform.getTranslationY());

	auto f = document.getFont();

	auto gap = (document.getRowHeight() - f.getHeight() * 0.8f) / 2.0f * transform.getScaleFactor();

	f.setHeight(f.getHeight() * transform.getScaleFactor() * 0.8f);
	g.setFont(f);

	g.setColour(ln.contrasting(0.1f));

	auto& h = document.getFoldableLineRangeHolder();


	UnblurryGraphics ug(g, *this);

	for (const auto& r : rowData)
	{
		bool isErrorLine = r.rowNumber == errorLine;

		auto b = getRowBounds(r);

		
			

		bool showFoldRange = false;

		auto t = h.getLineType(r.rowNumber);

		if (isMouseOver(true))
		{
			auto pos = getMouseXYRelative().toFloat();

			if (b.contains(pos))
			{
				hoveredData = r;

				auto range = h.getRangeForLineNumber(r.rowNumber);

				for (const auto& inner : rowData)
				{
					if (!h.isFolded(inner.rowNumber) && range.contains(inner.rowNumber))
					{
						auto ib = getRowBounds(inner);
						g.setColour(Colours::white.withAlpha(0.1f));

						ib = ib.removeFromRight(15.0f * transform.getScaleFactor());

						if (t == FoldableLineRange::Holder::RangeStartClosed)
						{
							ib = ib.withWidth(getGutterWidth()).withX(0);
							showFoldRange = true;
						}

						g.fillRect(ib);
					}
				}
			}
		}


		if ((r.isRowSelected || isErrorLine) && !showFoldRange)
		{
			g.setColour(ln.contrasting(0.1f));
			g.fillRect(b);
		}

		auto lfb = b;

		lfb = lfb.removeFromRight(15 * transform.getScaleFactor());

		g.setColour(getParentComponent()->findColour(CodeEditorComponent::lineNumberTextId).withAlpha(0.5f));

		

		switch (t)
		{
		case FoldableLineRange::Holder::RangeStartOpen:
		case FoldableLineRange::Holder::RangeStartClosed:
		{
			
			


			auto w = lfb.getWidth() - 4.0f * transform.getScaleFactor();

			auto box = lfb.withSizeKeepingCentre(w, w);

			

			box = ug.getRectangleWithFixedPixelWidth(box, (int)box.getWidth());
			
			g.drawRect(box, 1.0f);
			box = box.reduced(2.0f * transform.getScaleFactor());

			g.drawHorizontalLine(box.getCentreY(), box.getX(), box.getRight());

			if (t == FoldableLineRange::Holder::RangeStartClosed)
			{

				ug.draw1PxHorizontalLine(b.getBottom(), 0.0f, b.getRight());
				g.drawVerticalLine((int)box.getCentreX(), box.getY(), box.getBottom());

			}
				

			break;
		}
		case FoldableLineRange::Holder::Between:
		{
			ug.draw1PxVerticalLine(lfb.getCentreX(), lfb.getY(), lfb.getBottom());
			break;
		}
		case FoldableLineRange::Holder::RangeEnd:
		{
			Path p;

			auto b = lfb.getBottom() - 2.0f * transform.getScaleFactor();

			ug.draw1PxVerticalLine(lfb.getCentreX(), lfb.getY(), b);
			ug.draw1PxHorizontalLine(b, lfb.getCentreX(), lfb.getRight() - 3.0f * transform.getScaleFactor());
		}
		}
	}

	

	for (const auto& r : rowData)
	{
		if (h.isFolded(r.rowNumber))
			continue;

		auto A = r.bounds.getRectangle(0)
			.transformedBy(transform)
			.withX(0)
			.withWidth(GUTTER_WIDTH);

		A.removeFromRight(15 * transform.getScaleFactor());

		auto f = document.getFont();

		auto gap = (document.getRowHeight() - f.getHeight() * 0.8f) / 2.0f * transform.getScaleFactor();

		f.setHeight(f.getHeight() * transform.getScaleFactor() * 0.8f);

		

		g.setColour(getParentComponent()->findColour(CodeEditorComponent::lineNumberTextId));
		g.drawText(String(r.rowNumber + 1), A.reduced(5.0f, gap), Justification::topRight, false);

		//memoizedGlyphArrangements (r.rowNumber).draw(g, verticalTransform);
	}

	

	

#if PROFILE_PAINTS
	std::cout << "[GutterComponent::paint] " << Time::getMillisecondCounterHiRes() - start << std::endl;
#endif
}

GlyphArrangement mcl::GutterComponent::getLineNumberGlyphs(int row) const
{
	GlyphArrangement glyphs;
	glyphs.addLineOfText(document.getFont().withHeight(12.f),
		String(row + 1),
		8.f, document.getVerticalPosition(row, TextDocument::Metric::baseline));
	return glyphs;
}

bool mcl::GutterComponent::hitTest(int x, int y)
{
	return x < getGutterWidth();
}

juce::Rectangle<float> mcl::GutterComponent::getRowBounds(const TextDocument::RowData& r) const
{
	auto b = r.bounds.getRectangle(0);
	b.removeFromBottom(2.6f);

	b = b
		.transformedBy(transform)
		.withX(0)
		.withWidth(getGutterWidth());

	return b;

}

void mcl::GutterComponent::mouseDown(const MouseEvent& e)
{
	document.getFoldableLineRangeHolder().toggleFoldState(hoveredData.rowNumber);
}


}