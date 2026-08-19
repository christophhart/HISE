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

#pragma once

namespace scriptnode {

using namespace hise;
using namespace juce;

struct complex_ui_laf : public ScriptnodeComboBoxLookAndFeel,
	public TableEditor::LookAndFeelMethods,
	public SliderPack::LookAndFeelMethods,
	public HiseAudioThumbnail::LookAndFeelMethods,
	public FilterGraph::LookAndFeelMethods,
	public RingBufferComponentBase::LookAndFeelMethods,
	public AhdsrGraph::LookAndFeelMethods,
	public flex_ahdsr_base::FlexAhdsrGraph::LookAndFeelMethods
{
	static constexpr int NodeColourId = 9125451;

	complex_ui_laf() = default;

	void drawTableBackground(Graphics& g, TableEditor& te, Rectangle<float> area, double rulerPosition) override;
	void drawTablePath(Graphics& g, TableEditor& te, Path& p, Rectangle<float> area, float lineThickness) override;
	void drawTablePoint(Graphics& g, TableEditor& te, Rectangle<float> tablePoint, bool isEdge, bool isHover, bool isDragged) override;
	void drawTableMidPoint(Graphics& g, TableEditor& te, Rectangle<float> midPoint, bool isHover, bool isDragged) override;
	void drawTableRuler(Graphics& g, TableEditor& te, Rectangle<float> area, float lineThickness, double rulerPosition) override;
	void drawTableValueLabel(Graphics& g, TableEditor& te, Font f, const String& text, Rectangle<int> textBox) override;

	void drawFilterBackground(Graphics& g, FilterGraph& fg) override;
	void drawFilterGridLines(Graphics& g, FilterGraph& fg, const Path& gridPath) override;
	void drawFilterPath(Graphics& g, FilterGraph& fg, const Path& p) override;

	bool shouldClosePath() const override { return false; }

	void drawLinearSlider(Graphics&, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle, Slider&) override;

	void drawSliderPackBackground(Graphics& g, SliderPack& s) override;
	void drawSliderPackFlashOverlay(Graphics& g, SliderPack& s, int sliderIndex, Rectangle<int> sliderBounds, float intensity) override;

	void drawHiseThumbnailBackground(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, Rectangle<int> area) override;
	void drawHiseThumbnailPath(Graphics& g, HiseAudioThumbnail& th, bool areaIsEnabled, const Path& path) override;
	void drawTextOverlay(Graphics& g, HiseAudioThumbnail& th, const String& text, Rectangle<float> area) override;

	void drawButtonBackground(Graphics&, Button&, const Colour& backgroundColour, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
	void drawButtonText(Graphics&, TextButton&, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

	Colour getNodeColour(Component* c);

	void drawOscilloscopeBackground(Graphics& g, RingBufferComponentBase& ac, Rectangle<float> area) override;
	void drawOscilloscopePath(Graphics& g, RingBufferComponentBase& ac, const Path& p) override;
	void drawGonioMeterDots(Graphics& g, RingBufferComponentBase& ac, const RectangleList<float>& dots, int index) override;
	void drawAnalyserGrid(Graphics& g, RingBufferComponentBase& ac, const Path& p) override;

	void drawAhdsrBackground(Graphics& g, AhdsrGraph& graph) override;
	void drawAhdsrPathSection(Graphics& g, AhdsrGraph& graph, const Path& s, bool isActive) override;
	void drawAhdsrBallPosition(Graphics& g, AhdsrGraph& graph, Point<float> p) override;

	void drawFlexAhdsrBackground(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph) override;
	void drawFlexAhdsrFullPath(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph) override;
	void drawFlexAhdsrPosition(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph, flex_ahdsr_base::State s, Point<float> pointOnPath) override;
	void drawFlexAhdsrBall(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph, flex_ahdsr_base::State s, Point<float> pointOnPath) override;
	void drawFlexAhdsrSegment(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph, flex_ahdsr_base::State s, const Path& segment, bool hover, bool active) override;
	void drawFlexAhdsrText(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph, const String& text) override;
	void drawFlexAhdsrCurvePoint(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph, flex_ahdsr_base::State s, Point<float> curvePoint, bool hover, bool down) override;
	void drawFlexAhdsrDragPoint(Graphics& g, flex_ahdsr_base::FlexAhdsrGraph& graph, flex_ahdsr_base::State s, Point<float> dragPoint, bool hover, bool down) override;

	Colour nodeColour;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(complex_ui_laf);
};


}