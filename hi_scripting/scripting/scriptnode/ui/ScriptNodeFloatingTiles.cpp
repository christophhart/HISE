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
 *   which also must be licensed for commercial applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

namespace scriptnode
{
using namespace juce;
using namespace hise;


DspNetworkGraphPanel::DspNetworkGraphPanel(FloatingTile* parent) :
	NetworkPanel(parent)
{

}


Component* DspNetworkGraphPanel::createComponentForNetwork(DspNetwork* p)
{
	return new DspNetworkGraph::ScrollableParent(p);
}

NodePropertyPanel::NodePropertyPanel(FloatingTile* parent):
	NetworkPanel(parent)
{

}

struct NodePropertyContent : public Component,
							 public DspNetwork::SelectionListener
{
	NodePropertyContent(DspNetwork* n):
		network(n)
	{
		addAndMakeVisible(viewport);
		viewport.setViewedComponent(&content, false);
		n->addSelectionListener(this);
	}

	~NodePropertyContent()
	{
		if(network != nullptr)
			network->removeSelectionListener(this);
	}

	void resized() override
	{
		viewport.setBounds(getLocalBounds());
		content.setSize(viewport.getWidth() - viewport.getScrollBarThickness(), content.getHeight());
	}

	void selectionChanged(const NodeBase::List& selection)
	{
		editors.clear();

		auto y = 0;

		for (auto n : selection)
		{
			PropertyEditor* pe = new PropertyEditor(n, false, n->getValueTree());
			editors.add(pe);
			pe->setTopLeftPosition(0, y);
			pe->setSize(content.getWidth(), pe->getHeight());
			content.addAndMakeVisible(pe);
			y = pe->getBottom();
		}

		content.setSize(content.getWidth(), y);
	}

	WeakReference<DspNetwork> network;
	Component content;
	Viewport viewport;
	OwnedArray<PropertyEditor> editors;
};

juce::Component* NodePropertyPanel::createComponentForNetwork(DspNetwork* p)
{
	return new NodePropertyContent(p);
}

}

