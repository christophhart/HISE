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

namespace faders
{

	dynamic::editor::FaderGraph::FaderGraph(NodeType* f, PooledUIUpdater* updater) :
		ScriptnodeExtraComponent<ObjectType>(f, updater)
	{
		graphRebuilder.setCallback(f->p.parentNode->getValueTree(), { PropertyIds::NumParameters, PropertyIds::Value }, valuetree::AsyncMode::Asynchronously, [this](ValueTree, Identifier)
			{
				this->rebuildFaderCurves();
			});

		rebuildFaderCurves();
	}

	void dynamic::editor::FaderGraph::paint(Graphics& g)
	{
		ScriptnodeComboBoxLookAndFeel::drawScriptnodeDarkBackground(g, getLocalBounds().toFloat(), false);

		auto xPos = (float)getWidth() * inputValue;

		Line<float> posLine(xPos, 2.0f, xPos, (float)getHeight() - 2.0f);
		Line<float> scanLine(xPos, 5.0f, xPos, (float)getHeight() - 5.0f);

		int index = 0;

		g.setColour(Colours::white.withAlpha(0.3f));
		g.drawLine(posLine);

		for (auto& p : faderCurves)
		{
			auto c = MultiOutputDragSource::getFadeColour(index++, faderCurves.size());
			g.setColour(c.withMultipliedAlpha(0.7f));

			g.fillPath(p);
			g.setColour(c);
			g.strokePath(p, PathStrokeType(1.0f));

		}

		index = 0;

		for (auto& p : faderCurves)
		{
			auto c = MultiOutputDragSource::getFadeColour(index++, faderCurves.size());
			auto clippedLine = p.getClippedLine(scanLine, false);

			Point<float> circleCenter;

			if (clippedLine.getStartY() > clippedLine.getEndY())
				circleCenter = clippedLine.getEnd();
			else
				circleCenter = clippedLine.getStart();

			if (!circleCenter.isOrigin())
			{
				Rectangle<float> circle(circleCenter, circleCenter);
				g.setColour(c.withAlpha(1.0f));
				g.fillEllipse(circle.withSizeKeepingCentre(5.0f, 5.0f));
			}
		}
	}

	void dynamic::editor::FaderGraph::setInputValue(double v)
	{
		inputValue = v;
		repaint();
	}

	void dynamic::editor::FaderGraph::rebuildFaderCurves()
	{
		int numPaths = getObject()->p.getNumParameters();

		faderCurves.clear();

		if (numPaths > 0)
			faderCurves.add(createPath<0>());
		if (numPaths > 1)
			faderCurves.add(createPath<1>());
		if (numPaths > 2)
			faderCurves.add(createPath<2>());
		if (numPaths > 3)
			faderCurves.add(createPath<3>());
		if (numPaths > 4)
			faderCurves.add(createPath<4>());
		if (numPaths > 5)
			faderCurves.add(createPath<5>());
		if (numPaths > 6)
			faderCurves.add(createPath<6>());
		if (numPaths > 7)
			faderCurves.add(createPath<7>());

		resized();
	}

	void dynamic::editor::FaderGraph::resized()
	{
		auto b = getLocalBounds().reduced(2).toFloat();

		float maxHeight = 0.0f;

		for (const auto& p : faderCurves)
			maxHeight = jmax(maxHeight, (float)p.getBounds().getHeight());

		if (!b.isEmpty())
		{
			for (auto& p : faderCurves)
			{
				auto sf = p.getBounds().getHeight() / maxHeight;

				if (!p.getBounds().isEmpty())
				{
					auto thisMaxHeight = b.getHeight() * sf;
					auto offset = b.getY() + b.getHeight() - thisMaxHeight;
					p.scaleToFit(b.getX(), offset, b.getWidth(), b.getHeight() * sf, false);
				}

			}

			repaint();
		}
	}

	void dynamic::editor::FaderGraph::timerCallback()
	{
		double v = 0.0;

		if (getObject()->lastValue.getChangedValue(v))
			setInputValue(v);
	}

	dynamic::editor::editor(NodeType* v, PooledUIUpdater* updater_) :
		ScriptnodeExtraComponent(v, updater_),
		graph(v, updater_),
		dragRow(&v->p, updater_),
		faderSelector("Linear")
	{
		addAndMakeVisible(dragRow);

		addAndMakeVisible(faderSelector);
		faderSelector.initModes(faders::dynamic::getFaderModes(), v->p.parentNode);

		addAndMakeVisible(graph);

		setSize(256, 24 + 10 + parameter::ui::UIConstants::ButtonHeight + parameter::ui::UIConstants::DragHeight + parameter::ui::UIConstants::GraphHeight + UIValues::NodeMargin);

		setRepaintsOnMouseActivity(true);

		stop();
	}

	void dynamic::editor::resized()
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(24);
		
		b.removeFromTop(UIValues::NodeMargin);

		faderSelector.setBounds(top);
		graph.setBounds(b.removeFromTop(parameter::ui::UIConstants::GraphHeight));
		dragRow.setBounds(b);
	}

	juce::Component* dynamic::editor::createExtraComponent(void* obj, PooledUIUpdater* updater)
	{
		auto v = static_cast<NodeType*>(obj);
		return new editor(v, updater);
	}

}


}