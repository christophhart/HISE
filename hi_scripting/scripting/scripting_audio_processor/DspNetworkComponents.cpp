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

namespace hise
{
namespace scriptnode
{
using namespace juce;

void paintCable(Graphics& g, Line<float> line, bool isActive)
{
	Path p;

	p.startNewSubPath(line.getStart());
	p.lineTo(line.getEnd());

	auto ps = PathStrokeType(2.0f, PathStrokeType::JointStyle::curved, PathStrokeType::rounded);

	auto ps6 = PathStrokeType(6.0f, PathStrokeType::JointStyle::curved, PathStrokeType::rounded);

	auto ps5 = PathStrokeType(5.0f, PathStrokeType::JointStyle::curved, PathStrokeType::rounded);

	Colour outlineColour = isActive ? Colours::white : Colours::black;

	g.setColour(Colours::black.withAlpha(0.08f));
	g.strokePath(p, ps6);

	g.setColour(outlineColour.withAlpha(0.2f));
	g.strokePath(p, ps5);

	Colour c(SIGNAL_COLOUR);

	g.setColour(c);
	g.strokePath(p, ps);

	g.setColour(Colours::white.withAlpha(0.3f));
	g.strokePath(p, ps);
}

void DspNetworkGraph::paint(Graphics& g)
{
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF444444)));

	Colour lineColour = Colours::white;

	for (int x = 15; x < getWidth(); x += 10)
	{
		g.setColour(lineColour.withAlpha(((x - 5) % 100 == 0) ? 0.12f : 0.05f));
		g.drawVerticalLine(x, 0.0f, (float)getHeight());
	}

	for (int y = 15; y < getHeight(); y += 10)
	{
		g.setColour(lineColour.withAlpha(((y - 5) % 100 == 0) ? 0.12f : 0.05f));
		g.drawHorizontalLine(y, 0.0f, (float)getWidth());
	}
}

juce::Identifier DspNetworkGraph::Panel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

void DspNetworkGraph::Panel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<JavascriptProcessor>(moduleList);
}

ContainerComponent::Updater::Updater(ContainerComponent& parent_) :
	parent(parent_),
	copy(parent.dataReference)
{
	copy.addListener(this);

	setHandler(parent.node->getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
	addChangeListener(this);
}

void SerialNodeComponent::resized()
{
	NodeComponent::resized();

	Point<int> startPos = getStartPosition();

	for (auto nc : childNodeComponents)
	{
		auto bounds = nc->node->getPositionInCanvas(startPos);

		bounds = nc->node->reduceHeightIfFolded(bounds);

		nc->setBounds(bounds);

		//auto x = (getWidth() - bounds.getWidth()) / 2;
		//nc->setTopLeftPosition(x, bounds.getY());

		startPos = startPos.withY(bounds.getBottom() + NodeBase::NodeMargin);
	}
}



void SerialNodeComponent::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xb83f363a)));

	if (dynamic_cast<DspNetworkGraph*>(getParentComponent()) != nullptr)
	{
		g.setColour(dynamic_cast<Processor*>(node->getScriptProcessor())->getColour().withAlpha(0.2f));
	}

	g.fillRoundedRectangle(b, 5.0f);
	g.setColour(getOutlineColour());
	g.drawRoundedRectangle(b.reduced(1.5f), 5.0f, 3.0f);

	for (int i = 0; i < node->getNumChannelsToProcess(); i++)
	{
		paintSerialCable(g, i);
	}

	
	
	DropShadow sh;
	sh.colour = Colours::black.withAlpha(0.5f);
	sh.offset = { 0, 2 };
	sh.radius = 3;

	for (auto nc : childNodeComponents)
	{
		Path rr;
		rr.addRoundedRectangle(nc->getBounds().toFloat(), 5.0f);
		sh.drawForPath(g, rr);
	}

	if (addPosition != -1)
	{
		g.fillAll(Colours::white.withAlpha(0.01f));
		
		g.setColour(Colours::white.withAlpha(0.08f));
		g.fillRoundedRectangle(getInsertRuler(addPosition), 2.5f);
	}

	if (insertPosition != -1)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.05f));
		g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

		g.setColour(Colour(SIGNAL_COLOUR));
		g.fillRoundedRectangle(getInsertRuler(insertPosition), 2.5f);
	}
}

void ParallelNodeComponent::resized()
{
	NodeComponent::resized();

	auto startPos = getStartPosition();

	for (auto nc : childNodeComponents)
	{
		auto bounds = nc->node->getPositionInCanvas(startPos);

		bounds = nc->node->reduceHeightIfFolded(bounds);

		nc->setBounds(bounds);

		startPos = startPos.withX(bounds.getRight() + NodeBase::NodeMargin);
	}
}

void ParallelNodeComponent::paint(Graphics& g)
{
	if (header.isDragging)
		g.setOpacity(0.3f);

	auto b = getLocalBounds().toFloat();
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xcf39363f)));

	if(isMultiChannelNode())
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xcf393f36)));

	g.fillRoundedRectangle(b, 5.0f);
	g.setColour(getOutlineColour());
	g.drawRoundedRectangle(b.reduced(1.5f), 5.0f, 3.0f);

	for (int i = 0; i < node->getNumChannelsToProcess(); i++)
	{
		paintCable(g, i);
	}

	if (addPosition != -1)
	{
		g.fillAll(Colours::white.withAlpha(0.01f));

		g.setColour(Colours::white.withAlpha(0.08f));
		g.fillRoundedRectangle(getInsertRuler(addPosition), 2.5f);
	}

	if (insertPosition != -1)
	{
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.05f));
		g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);

		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.8f));
		g.fillRoundedRectangle(getInsertRuler(insertPosition), 2.5f);
	}
}

void ParallelNodeComponent::paintCable(Graphics& g, int cableIndex)
{
	auto xOffset = getCableXOffset(cableIndex, 1);

	auto b2 = getLocalBounds();

	b2.removeFromTop(NodeBase::HeaderHeight);
	b2.removeFromTop(NodeBase::NodeMargin / 2);
	b2.removeFromBottom(NodeBase::NodeMargin / 2);

	auto start = b2.removeFromTop(SerialNode::PinHeight).getCentre().toFloat().translated(xOffset, 0.0f);
	auto end = b2.removeFromBottom(SerialNode::PinHeight).getCentre().toFloat().translated(xOffset, 0.0f);

	Path p;

	auto pin1 = Rectangle<float>(start, start).withSizeKeepingCentre(10.0f, 10.0f);
	auto pin2 = Rectangle<float>(end, end).withSizeKeepingCentre(10.0f, 10.0f);

	p.addPieSegment(pin1, 0.0f, float_Pi * 2.0f, 0.5f);
	p.addPieSegment(pin2, 0.0f, float_Pi * 2.0f, 0.5f);

	if (isMultiChannelNode())
	{
		int currentChannelIndex = 0;
		int channelInTarget = -1;
		NodeComponent* targetNode = nullptr;

		for (auto c : childNodeComponents)
		{
			int numChannelsForThisNode = c->node->getNumChannelsToProcess();

			if (currentChannelIndex + numChannelsForThisNode > cableIndex)
			{
				channelInTarget = cableIndex - currentChannelIndex;
				targetNode = c;
				break;
			}

			currentChannelIndex += numChannelsForThisNode;
		}

		if (targetNode != nullptr && !targetNode->node->isBypassed())
		{
			auto targetOffsetX = getCableXOffset(channelInTarget);

			auto b = targetNode->getBounds().toFloat();

			Point<float> p1(b.getCentreX() + targetOffsetX, b.getY());
			Point<float> p2(b.getCentreX() + targetOffsetX, b.getBottom());

			p.addLineSegment({ start, p1 }, 2.0f);
			p.addLineSegment({ p2, end }, 2.0f);
		}

#if 0

		if (auto currentNode = childNodeComponents[nodeIndex++])
		{
			auto numForThisNode = currentNode->node->getNumChannelsToProcess();
			int currentCableIndex = 0;

			for (int i = 0; i < node->getNumChannelsToProcess(); i++)
			{

				if (numForThisNode <= 0)
				{
					currentNode = childNodeComponents[nodeIndex++];

					if (currentNode == nullptr)
						break;

					numForThisNode = currentNode->node->getNumChannelsToProcess();
					currentCableIndex = 0;
				}

				

				numForThisNode--;
			}
		}
#endif

		

		
	}
	else
	{
		for (auto n : childNodeComponents)
		{
			auto b = n->getBounds().toFloat();

			Point<float> p1(b.getCentreX() + xOffset, b.getY());
			Point<float> p2(b.getCentreX() + xOffset, b.getBottom());

			p.addLineSegment({ start, p1 }, 2.0f);
			p.addLineSegment({ p2, end }, 2.0f);
		}
	}

	if (!node->isBypassed())
	{
		g.setColour(Colour(0xFFAAAAAA));
		g.fillPath(p);
	}
}

juce::Point<int> ContainerComponent::getStartPosition() const
{
	return { NodeBase::NodeMargin, NodeBase::NodeMargin + NodeBase::HeaderHeight + SerialNode::PinHeight };
}

void NodeComponent::Header::mouseDown(const MouseEvent& e)
{
	
}

void NodeComponent::Header::mouseUp(const MouseEvent& e)
{
	auto graph = findParentComponentOfClass<DspNetworkGraph>();

	if (isDragging)
		graph->finishDrag();
	else
		graph->addToSelection(&parent, e.mods);
}

void NodeComponent::Header::mouseDrag(const MouseEvent& e)
{
	if (isDragging)
	{
		d.dragComponent(&parent, e, nullptr);
		parent.getParentComponent()->repaint();
		bool copyNode = e.mods.isAltDown();

		if (copyNode != parent.isBeingCopied())
			repaint();

		findParentComponentOfClass<DspNetworkGraph>()->updateDragging(parent.getParentComponent()->getLocalPoint(this, e.getPosition()), copyNode);

		return;
	}

	auto distance = e.getDistanceFromDragStart();

	if (distance > 50)
	{
		isDragging = true;

		if (findParentComponentOfClass<DspNetworkGraph>()->setCurrentlyDraggedComponent(&parent))
			d.startDraggingComponent(&parent, e);
	}
}

DspNodeComponent::DspNodeComponent(DspNode* node) :
	NodeComponent(node)
{
	if (node->obj != nullptr)
	{
		for (int i = 0; i < node->obj->getNumParameters(); i++)
		{
			auto newSlider = new Slider();

			char buf[64];
			int len;

			node->obj->getIdForConstant(i, buf, len);

			newSlider->setName(buf);
			newSlider->setSliderStyle(Slider::RotaryHorizontalVerticalDrag);
			node->getScriptProcessor()->getMainController_()->skin(*newSlider);

			auto value = node->obj->getParameter(i);

			if (value > 0.0)
			{
				newSlider->setRange(0.0, node->obj->getParameter(i), 0.001);
				newSlider->setValue(node->obj->getParameter(i), dontSendNotification);
			}

			addAndMakeVisible(newSlider);
			sliders.add(newSlider);
		}
	}
}

juce::Colour NodeComponent::getOutlineColour() const
{
	if (isRoot())
	{
		return dynamic_cast<const Processor*>(node->getScriptProcessor())->getColour();
	}

	return header.isDragging ? Colour(0x88444444) : Colour(0xFF555555);
}

bool NodeComponent::isRoot() const
{
	return !isDragged() && dynamic_cast<DspNetworkGraph*>(getParentComponent()) != nullptr;
}


bool NodeComponent::isDragged() const
{
	return findParentComponentOfClass<DspNetworkGraph>()->currentlyDraggedComponent == this;
}

bool NodeComponent::isSelected() const
{
	return findParentComponentOfClass<DspNetworkGraph>()->selection.isSelected(const_cast<NodeComponent*>(this));
}


bool NodeComponent::isBeingCopied() const
{
	return isDragged() && findParentComponentOfClass<DspNetworkGraph>()->copyDraggedNode;
}

}
}

