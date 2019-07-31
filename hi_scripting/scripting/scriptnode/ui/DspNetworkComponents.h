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
 *   which also must be licenced for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once

namespace hise
{
using namespace juce;

class ProcessorWithScriptingContent;
}

namespace scriptnode
{

using namespace juce;
using namespace hise;


class DspNetworkGraph : public Component,
	public AsyncUpdater,
	public DspNetwork::SelectionListener
{
public:
	struct ScrollableParent : public Component
	{
		ScrollableParent(DspNetwork* n);

		~ScrollableParent();

		void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
		void resized() override;

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF262626));
		}

		DspNetworkGraph* getGraph() const
		{
			return dynamic_cast<DspNetworkGraph*>(viewport.getViewedComponent());
		}

		void centerCanvas();

		float zoomFactor = 1.0f;
		Viewport viewport;
		OpenGLContext context;

	};

	struct Actions
	{
		static bool deselectAll(DspNetworkGraph& g);;
		static bool deleteSelection(DspNetworkGraph& g);
		static bool showJSONEditorForSelection(DspNetworkGraph& g);
		static bool undo(DspNetworkGraph& g);
		static bool redo(DspNetworkGraph& g);
	};

	void selectionChanged(const NodeBase::List&) override
	{
		
	}

	

	DspNetworkGraph(DspNetwork* n);
	~DspNetworkGraph();

	bool keyPressed(const KeyPress& key) override;
	void handleAsyncUpdate() override;
	void rebuildNodes();
	void resizeNodes();
	void updateDragging(Point<int> position, bool copyNode);
	void finishDrag();
	void paint(Graphics& g) override;
	void resized() override;

	void paintOverChildren(Graphics& g) override;

	NodeComponent* getComponent(NodeBase::Ptr node);

	static void paintCable(Graphics& g, Rectangle<float> start, Rectangle<float> end, Colour c)
	{
		g.setColour(Colours::black);
		g.fillEllipse(start);
		g.setColour(Colour(0xFFAAAAAA));
		g.drawEllipse(start, 2.0f);

		g.setColour(Colours::black);
		g.fillEllipse(end);
		g.setColour(Colour(0xFFAAAAAA));
		g.drawEllipse(end, 2.0f);

		Path p;

		p.startNewSubPath(start.getCentre());

		Point<float> controlPoint(start.getX() + (end.getX() - start.getX()) / 2.0f, end.getY() + 100.0f);

		p.quadraticTo(controlPoint, end.getCentre());

		g.setColour(Colours::black);
		g.strokePath(p, PathStrokeType(3.0f));
		g.setColour(c);
		g.strokePath(p, PathStrokeType(2.0f));
	};

	static Rectangle<float> getCircle(Component* c, bool getKnobCircle=true)
	{
		if (auto n = c->findParentComponentOfClass<DspNetworkGraph>())
		{
			float width = 6.0f;
			float height = 6.0f;
			float y = getKnobCircle ? 66.0f : c->getHeight();
			float circleX = c->getLocalBounds().toFloat().getWidth() / 2.0f - width / 2.0f;

			Rectangle<float> circleBounds = { circleX, y, width, height };

			return n->getLocalArea(c, circleBounds.toNearestInt()).toFloat();
		}

		return {};
	};


	bool setCurrentlyDraggedComponent(NodeComponent* n);

	ValueTree dataReference;

	bool copyDraggedNode = false;

	valuetree::RecursivePropertyListener cableRepainter;
	valuetree::ChildListener rebuildListener;
	valuetree::RecursivePropertyListener resizeListener;

	valuetree::RecursiveTypedChildListener macroListener;

	ScopedPointer<NodeComponent> root;

	Component::SafePointer<ContainerComponent> currentDropTarget;
	ScopedPointer<NodeComponent> currentlyDraggedComponent;

	WeakReference<DspNetwork> network;
};


}

