/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   HISE is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
*
*   Commercial licenses for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace scriptnode {

using namespace hise;
using namespace juce;

void complex_ui_laf::drawTableBackground(Graphics& g, TableEditor& te, Rectangle<float> area, double rulerPosition)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, area, false);
}

void complex_ui_laf::drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness)
{
	UnblurryGraphics ug(g, te, true);

	LODManager::LODGraphics lg(g, te);

	auto b = area;
	auto b2 = area;

	auto pc = p;

	for (int i = 0; i < 4; i++)
	{
		b.removeFromLeft(area.getWidth() / 4.0f);
		ug.draw1PxVerticalLine(b.getX(), area.getY(), area.getBottom());
		b2.removeFromTop(area.getHeight() / 4.0f);
		ug.draw1PxHorizontalLine(b2.getY(), area.getX(), area.getRight());
	}

	float alpha = 0.8f;

	if (te.isMouseOverOrDragging(true))
		alpha += 0.1f;

	if (te.isMouseButtonDown(true))
		alpha += 0.1f;

	auto c = getNodeColour(&te).withBrightness(1.0f).withAlpha(alpha);

	g.setColour(c);

	if(lg.drawFullDetails())
	{
		Path destP;
		float l[2] = { 4.0f * ug.getPixelSize(), 4.0f * ug.getPixelSize() };
		PathStrokeType(2.0f * ug.getPixelSize()).createDashedStroke(destP, pc, l, 2);

		g.fillPath(destP);
	}
	else
	{
		g.strokePath(pc, PathStrokeType(2.0f * ug.getPixelSize()));
	}

	//g.strokePath(p, PathStrokeType(2.0f * ug.getPixelSize()));



	c = c.withMultipliedAlpha(0.05f);
	g.setColour(c);
	//g.fillPath(p);

	te.setRepaintsOnMouseActivity(true);

	auto i = te.getLastIndex();



	if (auto e = dynamic_cast<SampleLookupTable*>(te.getEditedTable()))
	{
		b = area;
		b.removeFromLeft(i * area.getWidth());
		g.setColour(getNodeColour(&te).withBrightness(1.0f).withAlpha(0.9f));
		g.saveState();
		g.excludeClipRegion(b.toNearestInt());
		g.strokePath(p, PathStrokeType(3.0f * ug.getPixelSize()));
		g.restoreState();
	}

	auto a = te.getPointAreaBetweenMouse();

	if (!a.isEmpty())
	{
		auto fp = p;

		fp.lineTo(te.getLocalBounds().toFloat().getBottomRight());
		fp.closeSubPath();

		g.setColour(Colours::white.withAlpha(0.03f));
		auto l = te.getLocalBounds().removeFromLeft(a.getX());
		auto r = te.getLocalBounds();
		r = r.removeFromRight(r.getWidth() - a.getRight());
		g.saveState();
		g.excludeClipRegion(l);
		g.excludeClipRegion(r);
		g.fillPath(fp);
		g.restoreState();
	}
}

void complex_ui_laf::drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged)
{
	float brightness = 1.0f;

	if (!te.isMouseOverOrDragging(true))
	{
		brightness *= 0.9f;
	}

	if (!te.isMouseButtonDown(true))
		brightness *= 0.9f;

	UnblurryGraphics ug(g, te, true);

	auto c = getNodeColour(&te).withBrightness(brightness);

	g.setColour(c);

	auto s = jmin(tablePoint.getWidth(), isEdge ? 15.0f : 10.0f);

	auto dot = tablePoint.withSizeKeepingCentre(s, s);

	g.drawRoundedRectangle(dot, ug.getPixelSize() * 3.0f, ug.getPixelSize());

	if (isHover || isDragged)
	{
		g.setColour(c.withAlpha(0.7f));
		g.fillRoundedRectangle(dot, ug.getPixelSize() * 3.0f);
	}
}

void complex_ui_laf::drawTableMidPoint(Graphics& g, TableEditor& te, Rectangle<float> midPoint, bool isHover, bool isDragged)
{
	float brightness = 1.0f;

	if (!te.isMouseOverOrDragging(true))
	{
		brightness *= 0.9f;
	}

	if (!te.isMouseButtonDown(true))
		brightness *= 0.9f;

	UnblurryGraphics ug(g, te, true);

	auto c = getNodeColour(&te).withBrightness(brightness);

	g.setColour(c);

	auto s = jmin(midPoint.getWidth(), 10.0f);

	auto dot = midPoint.withSizeKeepingCentre(s, s);

	g.drawEllipse(dot, ug.getPixelSize());

	if (isHover || isDragged)
	{
		g.setColour(c.withAlpha(0.7f));
		g.fillEllipse(dot);
	}
}

void complex_ui_laf::drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition)
{
	auto a = te.getLocalBounds().toFloat();

	Rectangle<float> ar(rulerPosition * a.getWidth() - 10.0f, a.getY(), 20.0f, a.getHeight());

	g.setColour(Colours::white.withAlpha(0.02f));
	g.fillRect(ar);

	UnblurryGraphics ug(g, te, true);

	g.setColour(Colours::white.withAlpha(0.7f));
	ug.draw1PxVerticalLine(ar.getCentreX(), a.getY(), a.getHeight());
}

void complex_ui_laf::drawTableValueLabel(Graphics& g, TableEditor& te, Font f, const String& text, Rectangle<int> textBox)
{
	auto c = getNodeColour(&te);
	g.setColour(Colours::black.withAlpha(0.4f));
	g.fillRoundedRectangle(textBox.toFloat(), textBox.getHeight() / 2);
	g.setColour(c);
	g.setFont(f);
	g.drawText(text, textBox.toFloat(), Justification::centred);
}

void complex_ui_laf::drawFilterBackground(Graphics& g, FilterGraph& fg)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, fg.getLocalBounds().toFloat(), false);
}

void complex_ui_laf::drawFilterGridLines(Graphics& g, FilterGraph& fg, const Path& gridPath)
{
	LODManager::LODGraphics lg(g, fg);

	if(lg.drawFullDetails())
	{
		auto sf = UnblurryGraphics::getScaleFactorForComponent(&fg);
		g.setColour(Colours::white.withAlpha(0.05f));
		g.strokePath(gridPath, PathStrokeType(1.0f / sf));
	}
}

void complex_ui_laf::drawFilterPath(Graphics& g, FilterGraph& fg, const Path& p)
{
	auto c = getNodeColour(&fg).withAlpha(1.0f);

	auto b = fg.getLocalBounds().expanded(2);

	g.excludeClipRegion(b.removeFromLeft(4));
	g.excludeClipRegion(b.removeFromRight(4));
	g.excludeClipRegion(b.removeFromTop(4));
	g.excludeClipRegion(b.removeFromBottom(4));

	g.setColour(c.withAlpha(0.1f));
	g.fillPath(p);
	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.5f));
}

void complex_ui_laf::drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle style, Slider& s)
{
	UnblurryGraphics ug(g, s, true);

	if (style == Slider::SliderStyle::LinearBarVertical)
	{
		LODManager::LODGraphics lg(g, s);

		float leftY;
		float actualHeight;

		const double value = s.getValue();
		const double normalizedValue = (value - s.getMinimum()) / (s.getMaximum() - s.getMinimum());
		const double proportion = pow(normalizedValue, s.getSkewFactor());

		//const float value = ((float)s.getValue() - min) / (max - min);

		auto h = ug.getPixelSize() * 6.0f;

		actualHeight = (float)proportion * (float)(height - h);

		leftY = (float)height - actualHeight;

		float alpha = 0.4f;

		auto p = s.findParentComponentOfClass<SliderPack>();

		auto sb = s.getBoundsInParent();

		auto over = sb.contains(p->getMouseXYRelative());

		if (over)
		{
			alpha += 0.05f;

			if (p->isMouseButtonDown(true))
				alpha += 0.05f;
		}

		Colour c = getNodeColour(&s).withBrightness(1.0f);
		g.setColour(c.withAlpha(1.0f));



		Rectangle<float> ar(0.0f, jmin(s.getHeight() - h, leftY), (float)(width + 1), h);

		lg.fillRoundedRectangle(ar.reduced(3.0f, 0.0f), ar.getHeight() / 2.0f);

		Rectangle<float> ar2(0.0f, ar.getBottom(), ar.getWidth(), s.getHeight() - ar.getBottom());

		g.setColour(c.withAlpha(0.05f));
		g.fillRect(ar2.reduced(ar.getHeight(), 0.0f));

	}
}

void complex_ui_laf::drawSliderPackBackground(Graphics& g, SliderPack& s)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, s.getLocalBounds().toFloat(), false);

	UnblurryGraphics ug(g, s, true);

	auto w = (float)s.getWidth() / (float)s.getNumSliders();

	for (float x = -1.0f; x < (float)(s.getWidth() - 2); x += w)
		ug.draw1PxVerticalLine(x, 0.0f, (float)s.getHeight());
}

void complex_ui_laf::drawSliderPackFlashOverlay(Graphics& g, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity)
{
	auto c = getNodeColour(&s).withAlpha(intensity);
	g.setColour(c);
	//g.fillRect(sliderBounds);

	g.setColour(Colours::white.withAlpha(intensity * 0.1f));
	g.fillRect(sliderBounds.withY(0).withHeight(s.getHeight()));
}

void complex_ui_laf::drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area)
{
	UnblurryGraphics ug(g, th, true);

	g.setColour(Colours::black.withAlpha(0.3f));
	ug.fillUnblurryRect(area.toFloat());
	g.drawRect(area.toFloat(), 2.0f);
}

void complex_ui_laf::drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path)
{

	auto c = getNodeColour(&th).withBrightness(1.0f);

	g.setColour(c.withAlpha(areaIsEnabled ? 0.3f : 0.1f));

	UnblurryGraphics ug(g, th, true);

	if (areaIsEnabled)
	{
		g.fillPath(path);
		g.setColour(c.withAlpha(areaIsEnabled ? 1.0f : 0.1f));
		g.strokePath(path, PathStrokeType(ug.getPixelSize()));
	}
	else
	{
		g.fillPath(path);
		float l[2] = { ug.getPixelSize() * 4.0f, ug.getPixelSize() * 4.0f };
		Path destP;
		g.setColour(c.withAlpha(0.4f));
		PathStrokeType(ug.getPixelSize() * 2.0f).createDashedStroke(destP, path, l, 2);
		g.fillPath(destP);
	}
}

void complex_ui_laf::drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area)
{
	g.setColour(Colours::black.withAlpha(0.3f));

	auto sf = jmax(1.0f, UnblurryGraphics::getScaleFactorForComponent(&th, false));

	auto a = area.withSizeKeepingCentre(area.getWidth() / sf, (area.getHeight() + 5.0f) / sf);

	if (!text.startsWith("Drop"))
	{
		a.setX(th.getRight() - 10.0f - a.getWidth());
		a.setY(area.getY());
	}
	g.fillRoundedRectangle(a, a.getHeight() / 2.0f);

	g.setColour(getNodeColour(&th).withBrightness(1.0f));
	g.setFont(GLOBAL_BOLD_FONT().withHeight(14.0f / sf));
	g.drawText(text, a, Justification::centred);
}

void complex_ui_laf::drawButtonBackground(Graphics& g, Button& b, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	float br = 0.6f;

	if (shouldDrawButtonAsDown)
		br += 0.2f;
	if (shouldDrawButtonAsDown)
		br += 0.2f;

	auto c = getNodeColour(&b).withBrightness(br);

	g.setColour(c);
	g.setFont(GLOBAL_BOLD_FONT());

	auto ar = b.getLocalBounds().toFloat().reduced(3.0f);

	if (b.getToggleState())
	{
		g.fillRoundedRectangle(ar, ar.getHeight() / 2.0f);
		g.setColour(c.contrasting());
		g.drawText(b.getName(), ar, Justification::centred);
	}
	else
	{
		g.drawRoundedRectangle(ar, ar.getHeight() / 2.0f, 2.0f);
		g.drawText(b.getName(), ar, Justification::centred);
	}
}

void complex_ui_laf::drawButtonText(Graphics&, TextButton&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{

}

juce::Colour complex_ui_laf::getNodeColour(Component* comp)
{
	if (!nodeColour.isTransparent())
		return nodeColour;

	auto c = comp->findColour(complex_ui_laf::NodeColourId, true);

	if(c.isTransparent())
		return Colour(0xFFDADADA);

	return c;
}

void complex_ui_laf::drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> area)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, area, false);
}

void complex_ui_laf::drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p)
{
	LODManager::LODGraphics lg(g, *dynamic_cast<Component*>(&ac));

	auto c = getNodeColour(dynamic_cast<Component*>(&ac));

	auto b = dynamic_cast<Component*>(&ac)->getLocalBounds().expanded(2);

	g.excludeClipRegion(b.removeFromLeft(4));
	g.excludeClipRegion(b.removeFromRight(4));
	g.excludeClipRegion(b.removeFromTop(4));
	g.excludeClipRegion(b.removeFromBottom(4));

	if(lg.drawFullDetails())
	{
		g.setColour(c.withAlpha(0.1f));
		g.fillPath(p);
	}

	g.setColour(c);
	g.strokePath(p, PathStrokeType(1.0f));
}

void complex_ui_laf::drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index)
{
	auto c = getNodeColour(dynamic_cast<Component*>(&ac));

	const float alphas[6] = { 1.0f, 0.5f, 0.25f, 0.125f, 0.075f, 0.03f };

	g.setColour(c.withAlpha(alphas[index]));
	g.fillRectList(dots);
}

void complex_ui_laf::drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p)
{
	auto sf = UnblurryGraphics::getScaleFactorForComponent(dynamic_cast<Component*>(&ac));

	LODManager::LODGraphics lg(g, *dynamic_cast<Component*>(&ac));

	if(lg.drawFullDetails())
	{
		g.setColour(Colours::white.withAlpha(0.05f));
		g.strokePath(p, PathStrokeType(1.0f / sf));
	}
}

void complex_ui_laf::drawAhdsrBackground(Graphics& g, AhdsrGraph& graph)
{
	drawOscilloscopeBackground(g, graph, graph.getLocalBounds().toFloat());
}

void complex_ui_laf::drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive)
{
	if (!isActive)
	{
		drawOscilloscopePath(g, graph, s);
	}
	else
	{
		auto c = getNodeColour(dynamic_cast<Component*>(&graph)).withAlpha(0.05f);
		g.setColour(c);
		g.fillPath(s);
	}
}

void complex_ui_laf::drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p)
{
	LODManager::LODGraphics lg(g, graph);

	if (lg.drawFullDetails())
	{
		g.setColour(getNodeColour(&graph).withAlpha(1.0f));
		Rectangle<float> a(p, p);
		g.fillEllipse(a.withSizeKeepingCentre(6.0f, 6.0f));
	}
}

void complex_ui_laf::drawFlexAhdsrBackground(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph)
{
	ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, graph.getLocalBounds().toFloat(), false);

	g.setColour(getNodeColour(&graph).withAlpha(1.0f));
	g.strokePath(graph.fullPath, PathStrokeType(1.0f));
}

void complex_ui_laf::drawFlexAhdsrFullPath(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph)
{
	auto ab = graph.boxes[(int)flex_ahdsr_base::State::ATTACK];
	auto hb = graph.boxes[(int)flex_ahdsr_base::State::HOLD];
	auto db = graph.boxes[(int)flex_ahdsr_base::State::DECAY];
	auto sb = graph.segments[(int)flex_ahdsr_base::State::SUSTAIN].getBounds();
	auto rb = graph.boxes[(int)flex_ahdsr_base::State::RELEASE];

	auto fullBounds = graph.getLocalBounds().toFloat().reduced(10.0f);

	Path gridPath;
	gridPath.startNewSubPath(ab.getRight(), fullBounds.getY());
	gridPath.lineTo(ab.getRight(), fullBounds.getBottom());

	gridPath.startNewSubPath(hb.getRight(), fullBounds.getY());
	gridPath.lineTo(hb.getRight(), fullBounds.getBottom());

	gridPath.startNewSubPath(db.getRight(), fullBounds.getY());
	gridPath.lineTo(db.getRight(), fullBounds.getBottom());

	gridPath.startNewSubPath(sb.getRight(), fullBounds.getY());
	gridPath.lineTo(sb.getRight(), fullBounds.getBottom());

	gridPath.startNewSubPath(rb.getRight(), fullBounds.getY());
	gridPath.lineTo(rb.getRight(), fullBounds.getBottom());

	gridPath.startNewSubPath(fullBounds.getX(), ab.getY());
	gridPath.lineTo(fullBounds.getRight(), ab.getY());

	gridPath.startNewSubPath(fullBounds.getX(), sb.getY());
	gridPath.lineTo(fullBounds.getRight(), sb.getY());

	auto sf = UnblurryGraphics::getScaleFactorForComponent(&graph);

	g.setColour(Colours::white.withAlpha(0.05f));
	g.strokePath(gridPath, PathStrokeType(1.0f / sf));
}

void complex_ui_laf::drawFlexAhdsrPosition(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph,
	flex_ahdsr_base::State s, Point<float> pointOnPath)
{

	Graphics::ScopedSaveState ss(g);

	g.excludeClipRegion({ (int)pointOnPath.getX(), 0, graph.getWidth(), graph.getHeight() });
	g.setColour(getNodeColour(&graph));
	g.strokePath(graph.fullPath, PathStrokeType(3.0f));
}

void complex_ui_laf::drawFlexAhdsrBall(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph,
	flex_ahdsr_base::State s, Point<float> pointOnPath)
{
	if(s == flex_ahdsr_base::State::SUSTAIN || s == flex_ahdsr_base::State::IDLE)
		return;

	LODManager::LODGraphics lg(g, graph);

	if (lg.drawFullDetails())
	{
		g.setColour(getNodeColour(&graph).withAlpha(1.0f));
		Rectangle<float> a(pointOnPath, pointOnPath);
		g.fillEllipse(a.withSizeKeepingCentre(6.0f, 6.0f));
	}
}

void complex_ui_laf::drawFlexAhdsrSegment(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph,
	flex_ahdsr_base::State s, const Path& segment, bool hover, bool active)
{
	if (hover)
	{
		g.setColour(getNodeColour(&graph).withAlpha(0.1f));
		g.fillPath(segment);
	}
}

void complex_ui_laf::drawFlexAhdsrText(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph, const String& text)
{

}

void complex_ui_laf::drawFlexAhdsrCurvePoint(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph,
	flex_ahdsr_base::State s, Point<float> curvePoint, bool hover, bool down)
{
	g.setColour(getNodeColour(&graph));
	auto margin = hover ? 10.0f : 7.0f;

	if (down)
		margin -= 1.0f;

	g.fillEllipse(curvePoint.x - margin * 0.5f, curvePoint.y - margin * 0.5f, margin, margin);
}

void complex_ui_laf::drawFlexAhdsrDragPoint(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph,
	flex_ahdsr_base::State s, Point<float> dragPoint, bool hover, bool down)
{
	g.setColour(getNodeColour(&graph));
	auto margin = hover ? 10.0f : 7.0f;

	if (down)
		margin -= 1.0f;

	g.drawRect(dragPoint.x - margin * 0.5f, dragPoint.y - margin * 0.5f, margin, margin);
}

}