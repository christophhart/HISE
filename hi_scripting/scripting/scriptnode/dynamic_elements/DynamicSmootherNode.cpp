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
*   information about commercial licensing:
*
*   http://www.hise.audio/
*
*   HISE is based on the JUCE library,
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace scriptnode {
using namespace juce;
using namespace hise;

namespace control
{
void pma_editor::paint(Graphics& g)
{
	g.setFont(GLOBAL_BOLD_FONT());

	auto r = obj->currentRange;

	String start, mid, end;

	int numDigits = jmax<int>(1, -1 * roundToInt(log10(r.interval)));

	start = String(r.start, numDigits);

	mid = String(r.convertFrom0to1(0.5), numDigits);

	end = String(r.end, numDigits);

	auto b = getLocalBounds().toFloat();

	float w = JUCE_LIVE_CONSTANT_OFF(55.0f);

	auto midCircle = b.withSizeKeepingCentre(w, w).translated(0.0f, 5.0f);

	float r1 = JUCE_LIVE_CONSTANT_OFF(3.0f);
	float r2 = JUCE_LIVE_CONSTANT_OFF(5.0f);

	float startArc = JUCE_LIVE_CONSTANT_OFF(-2.5f);
	float endArc = JUCE_LIVE_CONSTANT_OFF(2.5f);

	Colour trackColour = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF888888));

	auto createArc = [startArc, endArc](Rectangle<float> b, float startNormalised, float endNormalised)
	{
		Path p;

		auto s = startArc + jmin(startNormalised, endNormalised) * (endArc - startArc);
		auto e = startArc + jmax(startNormalised, endNormalised) * (endArc - startArc);

		s = jlimit(startArc, endArc, s);
		e = jlimit(startArc, endArc, e);

		p.addArc(b.getX(), b.getY(), b.getWidth(), b.getHeight(), s, e, true);

		return p;
	};

	auto oc = midCircle;
	auto mc = midCircle.reduced(5.0f);
	auto ic = midCircle.reduced(10.0f);

	auto outerTrack = createArc(oc, 0.0f, 1.0f);
	auto midTrack = createArc(mc, 0.0f, 1.0f);
	auto innerTrack = createArc(ic, 0.0f, 1.0f);

	g.setColour(trackColour);

	g.strokePath(outerTrack, PathStrokeType(r1));
	g.strokePath(midTrack, PathStrokeType(r2));
	g.strokePath(innerTrack, PathStrokeType(r1));

	auto data = obj->getUIData();

	auto nrm = [r](double v)
	{
		return r.convertTo0to1(v);
	};

	auto mulValue = nrm(data.value * data.mulValue);
	auto totalValue = nrm(data.getPmaValue());

	auto outerRing = createArc(oc, mulValue, totalValue);
	auto midRing = createArc(mc, 0.0f, totalValue);
	auto innerRing = createArc(ic, 0.0f, mulValue);
	auto valueRing = createArc(ic, 0.0f, nrm(data.value));

	g.setColour(Colour(0xFF0051FF));
	g.strokePath(outerRing, PathStrokeType(r1));
	g.setColour(Colours::white);
	g.strokePath(midRing, PathStrokeType(r2));
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x88ff7277)));
	g.strokePath(valueRing, PathStrokeType(r1));
	g.setColour(Colour(0xFFE5353C));
	g.strokePath(innerRing, PathStrokeType(r1));

	b.removeFromTop(18.0f);

	g.setColour(Colours::white.withAlpha(0.7f));

	Rectangle<float> t((float)getWidth() / 2.0f - 35.0f, 0.0f, 70.0f, 15.0f);

	g.drawText(start, t.translated(-50.0f, 60.0f), Justification::centred);
	g.drawText(mid, t, Justification::centred);
	g.drawText(end, t.translated(50.0f, 60.0f), Justification::centred);
}
}

namespace smoothers
{
	dynamic::editor::editor(dynamic* p, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<dynamic>(p, updater),
		plotter(updater),
		modeSelector("Linear Ramp")
	{
		addAndMakeVisible(modeSelector);
		addAndMakeVisible(plotter);
		setSize(256, 80);
	}

	void dynamic::editor::paint(Graphics& g)
	{
		float a = JUCE_LIVE_CONSTANT_OFF(0.4f);


		auto b = getLocalBounds();
		b.removeFromTop(24);
		b = b.removeFromTop(5);

		g.setColour(currentColour.withAlpha(a));
		g.fillRect(b);
	}

	void dynamic::editor::timerCallback()
	{
		double v = 0.0;
		currentColour = getObject()->lastValue.getChangedValue(v) ? Colour(SIGNAL_COLOUR) : Colours::grey;
		repaint();

		modeSelector.initModes(smoothers::dynamic::getSmoothNames(), plotter.getSourceNodeFromParent());
	}

}

}