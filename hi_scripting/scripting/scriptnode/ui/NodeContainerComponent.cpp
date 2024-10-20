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


ContainerComponent::Updater::Updater(ContainerComponent& parent_) :
	parent(parent_),
	copy(parent.dataReference)
{
	copy.addListener(this);

	setHandler(parent.node->getScriptProcessor()->getMainController_()->getGlobalUIUpdater());
	addChangeListener(this);
}

ContainerComponent::Updater::~Updater()
{
	copy.removeListener(this);
}

void ContainerComponent::Updater::changeListenerCallback(SafeChangeBroadcaster *)
{
	if(messageLevel == Level::Rebuild)
		parent.rebuildNodes();
	if (messageLevel == Level::Repaint)
		parent.repaint();

	messageLevel = Level::Nothing;
}


void ContainerComponent::Updater::valueTreeChildAdded(ValueTree& parentTree, ValueTree&)
{
	if (parentTree == copy)
		sendPooledChangeMessage();
}


void ContainerComponent::Updater::valueTreeChildOrderChanged(ValueTree& parentTree, int , int )
{
	if (parentTree == copy)
		sendPooledChangeMessage();
}


void ContainerComponent::Updater::valueTreeChildRemoved(ValueTree& parentTree, ValueTree& , int)
{
	if (parentTree == copy)
		sendPooledChangeMessage();
}


void ContainerComponent::Updater::valueTreePropertyChanged(ValueTree&, const Identifier& id)
{
	if (id == PropertyIds::Bypassed)
	{
		messageLevel = jmax(Level::Repaint, messageLevel);
		sendPooledChangeMessage();
	}

	if (id == PropertyIds::Folded)
	{
		messageLevel = Level::Rebuild;
		sendPooledChangeMessage();
	}

	if (id == PropertyIds::ShowParameters)
	{
		messageLevel = Level::Rebuild;
		sendPooledChangeMessage();
	}
}


void ContainerComponent::Updater::valueTreeParentChanged(ValueTree&)
{

}

ContainerComponent::ContainerComponent(NodeContainer* b) :
	NodeComponent(b->asNode()),
    SimpleTimer(b->asNode()->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
	updater(*this),
	gotoButton("workspace", nullptr, nf),
	parameters(new ParameterComponent(*this))
{
	addAndMakeVisible(gotoButton);

	gotoButton.setTooltip("Show this container as root");

	gotoButton.onClick = [this]()
	{
		auto ng = findParentComponentOfClass<DspNetworkGraph>();
		auto n = node.get();

		MessageManager::callAsync([ng, n]()
		{
			ng->setCurrentRootNode(n);
		});
	};

	if (auto sn = dynamic_cast<SerialNode*>(b))
	{
		verticalValue.referTo(sn->getNodePropertyAsValue(PropertyIds::IsVertical));
		verticalValue.addListener(this);
	}

	addAndMakeVisible(parameters);
	setOpaque(true);
	rebuildNodes();
}


ContainerComponent::~ContainerComponent()
{

}


void ContainerComponent::mouseEnter(const MouseEvent& event)
{
	addPosition = getInsertPosition(event.getPosition());
	repaint();
}


void ContainerComponent::mouseMove(const MouseEvent& e)
{
	setMouseCursor(e.mods.isShiftDown() ? MouseCursor::CrosshairCursor : MouseCursor::NormalCursor);

	addPosition = getInsertPosition(e.getPosition());
	repaint();
}


void ContainerComponent::mouseExit(const MouseEvent& )
{
	addPosition = -1;
	repaint();
}


void ContainerComponent::mouseDown(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DOWN(e);

	if (auto n = findParentComponentOfClass<DspNetworkGraph>())
	{
		if (e.mods.isShiftDown())
		{
			if (!e.mods.isCommandDown())
				node->getRootNetwork()->deselectAll();

			n->addAndMakeVisible(lasso);
			lasso.beginLasso(e.getEventRelativeTo(n), this);
			return;
		}

		DspNetworkGraph::Actions::showKeyboardPopup(*n, KeyboardPopup::Mode::New);
	}
}

bool ContainerComponent::shouldPaintCable(CableLocation location) const
{
    /* Implement the logic here:
       - offline nodes have no cables
       - clone nodes have a proper representation of their flow
     */
    
    
    switch(location)
    {
        case CableLocation::Input: return true;
        case CableLocation::BetweenNodes: return true;
        case CableLocation::Output: return dynamic_cast<ModulationChainNode*>(node.get()) == nullptr;
        default: return false;
    }
}

void ContainerComponent::mouseDrag(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_DRAG(e);

	if (lasso.isVisible())
		lasso.dragLasso(e.getEventRelativeTo(findParentComponentOfClass<DspNetworkGraph>()));
}

void ContainerComponent::mouseUp(const MouseEvent& e)
{
	CHECK_MIDDLE_MOUSE_UP(e);

	if (lasso.isVisible())
	{
		lasso.endLasso();
		findParentComponentOfClass<DspNetworkGraph>()->removeChildComponent(&lasso);
	}
}

bool ContainerComponent::keyPressed(const KeyPress& k)
{
	if(NodeComponent::keyPressed(k))
		return true;

	if(k == KeyPress::F3Key)
	{
		gotoButton.triggerClick(sendNotificationAsync);
		
		return true;
	}

	return false;
}

void ContainerComponent::removeDraggedNode(NodeComponent* draggedNode)
{
	int removeIndex = childNodeComponents.indexOf(draggedNode);

	removeChildComponent(draggedNode);

	auto dea = new DeactivatedComponent(draggedNode->node.get());

	addAndMakeVisible(dea);

	childNodeComponents.removeObject(draggedNode, false);

	childNodeComponents.insert(removeIndex, dea);

	resized();
	repaint();
}


void ContainerComponent::insertDraggedNode(NodeComponent* newNode, bool copyNode)
{
	for (auto nc : childNodeComponents)
	{
		if (auto dnc = dynamic_cast<DeactivatedComponent*>(nc))
		{
			if (!copyNode && childNodeComponents.indexOf(nc) < insertPosition)
				insertPosition--;

			childNodeComponents.removeObject(nc);
			break;
		}
	}

	if (insertPosition != -1)
	{
		auto newTree = newNode->node->getValueTree();
		auto container = dynamic_cast<NodeContainer*>(node.get());
        
		if (copyNode)
		{
            Array<DspNetwork::IdChange> changes;
			auto copy = node->getRootNetwork()->cloneValueTreeWithNewIds(newTree, changes, true);


			BACKEND_ONLY(DuplicateHelpers::removeOutsideConnections({ copy }, changes));
			
			node->getRootNetwork()->createFromValueTree(container->isPolyphonic(), copy, true);
			container->getNodeTree().addChild(copy, insertPosition, node->getUndoManager());
		}
		else
		{
			newNode->node->setParent(var(node.get()), insertPosition);
		}
	}
}


void ContainerComponent::helpChanged(float , float )
{
	if (auto ng = findParentComponentOfClass<DspNetworkGraph>())
	{
		ng->resizeNodes();
	}

	repaint();
}

void ContainerComponent::setDropTarget(Point<int> position)
{
	if (position.isOrigin())
	{
		clearDropTarget();
		return;
	}

	auto prevPosition = insertPosition;
	insertPosition = getInsertPosition(position);

	if (insertPosition != prevPosition)
		repaint();
}


void ContainerComponent::clearDropTarget()
{
	if (insertPosition != -1)
	{
		insertPosition = -1;
		repaint();
	}


	for (auto n : childNodeComponents)
	{
		if (auto c = dynamic_cast<ContainerComponent*>(n))
			c->clearDropTarget();
	}
}

void ContainerComponent::valueChanged(Value& v)
{
	if (auto dnp = findParentComponentOfClass<DspNetworkGraph>())
	{
		Component::SafePointer<DspNetworkGraph> safeDnp(dnp);

		MessageManager::callAsync([safeDnp]()
		{
			if(safeDnp.getComponent() != nullptr)
				safeDnp.getComponent()->rebuildNodes();
		});
	}
}

void ContainerComponent::resized()
{
	NodeComponent::resized();
		
	Component* topComponent = parameters != nullptr ? parameters.get() : extraComponent.get();

	jassert(topComponent != nullptr);

	topComponent->setVisible(dataReference[PropertyIds::ShowParameters]);

	auto b = getLocalBounds();
	b.expand(-UIValues::NodeMargin, 0);
	b.removeFromTop(UIValues::HeaderHeight);
	topComponent->setSize(b.getWidth(), topComponent->getHeight());
	topComponent->setTopLeftPosition(b.getTopLeft());

	gotoButton.setSize(16,16);

	if(auto ng = findParentComponentOfClass<DspNetworkGraph>())
	{
		gotoButton.setVisible(ng->root != this);
	}

	auto pos = topComponent->isVisible() ? topComponent->getBounds().getBottomLeft() : topComponent->getPosition();

	gotoButton.setTopLeftPosition(pos.translated(0, UIValues::NodeMargin));
}

void ContainerComponent::findLassoItemsInArea(Array<NodeBase::Ptr>& itemsFound, const Rectangle<int>& area)
{
	Array<NodeComponent*> nodeComponents;

	auto dng = findParentComponentOfClass<DspNetworkGraph>();
	dng->fillChildComponentList<NodeComponent>(nodeComponents, this);

	for (auto n : nodeComponents)
	{
		if (n->node.get() == node->getRootNetwork()->getRootNode())
			continue;

		if (n == this)
			continue;

		auto la = dng->getLocalArea(n, n->getLocalBounds());

		if (area.intersectRectangle(la))
		{
			bool parentFound = false;

			for (auto i : itemsFound)
			{
				auto p = n->node.get();

				while (!parentFound && p != nullptr)
				{
					parentFound |= itemsFound.contains(p);
					p = p->getParentNode();
				}
			}

			if(!parentFound)
				itemsFound.addIfNotAlreadyThere(n->node.get());
		}
	}
}

juce::Point<int> ContainerComponent::getStartPosition() const
{
	int y = UIValues::NodeMargin;
	y += UIValues::HeaderHeight;
	y += UIValues::PinHeight;

	if (dataReference[PropertyIds::ShowParameters])
		y += UIValues::ParameterHeight + UIValues::MacroDragHeight;

	return { UIValues::NodeMargin, y};
}

float ContainerComponent::getCableXOffset(int cableIndex, int factor /*= 1*/) const
{
	int numCables = node->getNumChannelsToDisplay() - 1;
	int cableWidth = numCables * (UIValues::PinHeight * factor);
	int cableStart = getWidth() / 2 - cableWidth / 2;
	int cableX = cableStart + cableIndex * (UIValues::PinHeight * factor);

	return (float)cableX - (float)getWidth() / 2.0f;
}

struct DuplicateComponent : public Component,
							public SettableTooltipClient
{
	DuplicateComponent(NodeBase* parent_, int numClones):
		parent(parent_),
		numExtra(numClones)
	{
		setRepaintsOnMouseActivity(true);
		setTooltip("Click to edit range of displayed clones");
	}

	void mouseDown(const MouseEvent& e) override
	{
		String message;
		message << "Enter the range of clones you want to display.  \n> Number of clones: **";
		message << String(dynamic_cast<NodeContainer*>(parent.get())->getNodeList().size());
		message << "**";

		auto s = PresetHandler::getCustomName("1-3,5,8", message);

		parent->setValueTreeProperty(PropertyIds::DisplayedClones, s);
	}

	void paintNodeBody(Graphics& g, Rectangle<float> b, float alpha)
	{
		g.setColour(Colour(0xff353535).withAlpha(alpha));

		
		g.fillRect(b);

		g.setColour(Colour(0xFF555555).withAlpha(alpha));
		g.drawRect(b, 1.0f);
		auto h = b.removeFromTop(UIValues::HeaderHeight);
		g.fillRect(h);

		h.removeFromLeft(1);
		h.removeFromRight(1);
		h.removeFromTop(1);

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x2b000000)));
		g.fillRect(h);

		NodeComponent::drawTopBodyGradient(g, b, JUCE_LIVE_CONSTANT_OFF(0.15f * alpha));
	}

	void paint(Graphics& g) override
	{
		auto b = getLocalBounds().reduced(UIValues::NodeMargin).toFloat();

		auto isVertical = getWidth() > getHeight();

		float opacity = 1.0f;
		
		if (!isVertical)
		{
			auto w = jmax(6.0f, b.getWidth() / jmax(1.0f, (float)(numExtra)) - 1.0f);

			while (b.getWidth() > 0)
			{
				opacity = jmax(0.2f, opacity - 0.1f);
				paintNodeBody(g, b.removeFromLeft(w), opacity);
				b.removeFromLeft(1.0f);
				b.removeFromBottom(1.0f);
			}
		}
		else
		{
			auto h = jmax(6.0f, b.getHeight() / jmax(1.0f, (float)(numExtra)) - 1.0f);

			while (b.getHeight() > 0.0f)
			{
				opacity = jmax(0.2f, opacity - 0.1f);
				paintNodeBody(g, b.removeFromTop(h), opacity);
				b.removeFromTop(1.0f);
				b.removeFromLeft(1.0f);
				b.removeFromRight(1.0f);
			}
		}

		float alpha = 0.2f;

		if (isMouseOver(true))
			alpha += 0.07f;

		if (isMouseButtonDown(true))
			alpha += 0.07f;

		Path p;
#if USE_BACKEND

		p.loadPathFromData(BackendBinaryData::ToolbarIcons::viewPanel, SIZE_OF_PATH(BackendBinaryData::ToolbarIcons::viewPanel));
#endif
		PathFactory::scalePath(p, getLocalBounds().toFloat().withSizeKeepingCentre(32.0f, 32.0f));

		g.setColour(Colours::white.withAlpha(alpha));
		g.fillPath(p);

		String s;
		s << "+" << String(numExtra);

		String r;
		
		r << "[" << parent->getValueTree()[PropertyIds::DisplayedClones].toString() << "]";

		if (r.isNotEmpty())
		{
			g.setColour(Colours::white.withAlpha(alpha));
			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.drawText(r, p.getBounds().translated(0.0f, 24.0f).expanded(30.0f, 0.0f), Justification::centredBottom);
		}

		g.setColour(Colours::white.withAlpha(0.8f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(s, getLocalBounds().reduced(UIValues::NodeMargin).toFloat().removeFromTop(UIValues::HeaderHeight), Justification::centred);
	}

	NodeBase::Ptr parent;
	const int numExtra;
};

void ContainerComponent::rebuildNodes()
{
	for (auto nc : childNodeComponents)
	{
		if(nc->node != nullptr)
			nc->node->getHelpManager().removeHelpListener(this);
	}

	duplicateDisplay = nullptr;
	childNodeComponents.clear();

	if(!node->getValueTree()[PropertyIds::Folded])
	{
		if (auto container = dynamic_cast<NodeContainer*>(node.get()))
		{
			int index = 0;
			int numHidden = 0;

			for (auto n : container->nodes)
			{
				if (auto cn = dynamic_cast<CloneNode*>(container))
				{
					if (!cn->shouldCloneBeDisplayed(index++))
					{
						numHidden++;
						continue;
					}
				}
				
				auto newNode = n->createComponent();

				n->getHelpManager().addHelpListener(this);
				n->getHelpManager().initCommentButton(this);
				addAndMakeVisible(newNode);
				childNodeComponents.add(newNode);
			}

			if (numHidden > 0 || (!node->getValueTree()[PropertyIds::ShowClones] && node->getValueTree().hasProperty(PropertyIds::DisplayedClones)))
			{
				addAndMakeVisible(duplicateDisplay = new DuplicateComponent(node.get(), numHidden));
			}
		}
	}

	resized();
	repaint();
}


SerialNodeComponent::SerialNodeComponent(NodeContainer* node) :
	ContainerComponent(node)
{

}


int SerialNodeComponent::getInsertPosition(Point<int> position) const
{
	auto targetY = position.getY();
	auto p = childNodeComponents.size();

	for (auto nc : childNodeComponents)
	{
		if (targetY < nc->getY() + nc->getHeight() / 2)
		{
			p = childNodeComponents.indexOf(nc);
			break;
		}
	}

	return p;
}


juce::Rectangle<float> SerialNodeComponent::getInsertRuler(int position) const
{
	int targetY = getHeight() - UIValues::PinHeight;

	if (auto childBeforeInsert = childNodeComponents[position])
		targetY = childBeforeInsert->getY();

	targetY -= (3 * UIValues::NodeMargin / 4);
	return Rectangle<int>(UIValues::NodeMargin, targetY, getWidth() - 2 * UIValues::NodeMargin, UIValues::NodeMargin / 2).toFloat();
}

juce::Colour SerialNodeComponent::getOutlineColour() const
{
	if (auto c = dynamic_cast<NodeContainer*>(node.get()))
	{
		auto c2 = c->getContainerColour();

		if (!c2.isTransparent())
			return c2;
	}
	
	return NodeComponent::getOutlineColour();
}

void SerialNodeComponent::resized()
{
	ContainerComponent::resized();

	Point<int> startPos = getStartPosition();

	for (auto nc : childNodeComponents)
	{
		if (nc->node == nullptr)
			continue;

		auto bounds = nc->node->getPositionInCanvas(startPos);
		bounds = nc->node->getBoundsWithoutHelp(bounds);

		nc->setBounds(bounds);

		auto helpBounds = nc->node->getHelpManager().getHelpSize().toNearestInt();

		int heightWithHelp, widthWithHelp;

		if(nc->node->getHelpManager().isHelpBelow())
		{
			widthWithHelp = jmax<int>(bounds.getWidth(), helpBounds.getWidth());
			heightWithHelp = bounds.getHeight() + helpBounds.getHeight();
		}
		else
		{
			widthWithHelp = bounds.getWidth() + helpBounds.getWidth();
			heightWithHelp = jmax(bounds.getHeight(), helpBounds.getHeight());
		}

		auto x = (getWidth() - widthWithHelp) / 2;
		nc->setTopLeftPosition(x, bounds.getY());

		startPos = startPos.withY(bounds.getY() + heightWithHelp + UIValues::NodeMargin);
	}

	if (duplicateDisplay != nullptr)
	{
		auto b = getLocalBounds();
		b.removeFromBottom(UIValues::PinHeight);
		duplicateDisplay->setBounds(b.removeFromBottom(UIValues::DuplicateSize));
	}
}

void addCircleAtMidpoint(Path& p, Line<float> l, float offset, bool useTwo, float radius)
{
    if(radius == 0.0f)
        return;
    
    auto lv = JUCE_LIVE_CONSTANT_OFF(12.0f);
    
    auto numDots = jmax(1, roundToInt(l.getLength() / jmax<float>(1.0f, lv)));
    
    float delta = l.getLength() / (float)numDots;
    
    radius *= JUCE_LIVE_CONSTANT_OFF(0.75f);
    
    offset *= JUCE_LIVE_CONSTANT_OFF(20.0f);
    
    for(int i = 0; i < numDots; i++)
    {
        auto pos = std::fmod(offset + (float)i * delta, l.getLength());
        
        auto p1 = l.getPointAlongLine(pos - radius, radius);
        auto p2 = l.getPointAlongLine(pos - radius, -radius);
        auto p3 = l.getPointAlongLine(jmin(l.getLength(), pos + radius), 0.0f);
        
        p.addTriangle(p1, p2, p3);
    }
}

void SerialNodeComponent::paintSerialCable(Graphics& g, int cableIndex)
{
	auto xOffset = getCableXOffset(cableIndex);

	auto b2 = getLocalBounds();
	b2.removeFromTop(UIValues::HeaderHeight);

	if (dataReference[PropertyIds::ShowParameters])
		b2.removeFromTop(UIValues::ParameterHeight + UIValues::MacroDragHeight);

	auto top = b2.removeFromTop(UIValues::PinHeight);
	auto start = top.getCentre().toFloat().translated(xOffset, 0.0f);
	auto start1 = start.withY(start.getY() + UIValues::PinHeight / 2 + UIValues::NodeMargin);

	if (auto firstNode = childNodeComponents.getFirst())
	{
		auto fb = firstNode->getBounds().toFloat();
		start1 = { fb.getCentreX() + xOffset, fb.getY() };
	}

	auto bottom = b2.removeFromBottom(UIValues::NodeMargin);
	bottom.removeFromBottom(UIValues::PinHeight);

	auto end = bottom.getCentre().toFloat().translated(xOffset, 0.0f);
	auto end1 = end.withY((float)(getHeight() - UIValues::PinHeight - UIValues::NodeMargin));

	if (auto lastNode = childNodeComponents.getLast())
	{
		auto lb = lastNode->getBounds().toFloat();
		end1 = { lb.getCentreX() + xOffset, lb.getBottom() };
	}

	auto pin1 = Rectangle<float>(start, start).withSizeKeepingCentre(10.0f, 10.0f);
	auto pin2 = Rectangle<float>(end, end).withSizeKeepingCentre(10.0f, 10.0f);

	static const unsigned char pathData[] = { 110,109,233,38,145,63,119,190,39,64,108,0,0,0,0,227,165,251,63,108,0,0,0,0,20,174,39,63,108,174,71,145,63,0,0,0,0,108,174,71,17,64,20,174,39,63,108,174,71,17,64,227,165,251,63,108,115,104,145,63,119,190,39,64,108,115,104,145,63,143,194,245,63,98,55,137,
145,63,143,194,245,63,193,202,145,63,143,194,245,63,133,235,145,63,143,194,245,63,98,164,112,189,63,143,194,245,63,96,229,224,63,152,110,210,63,96,229,224,63,180,200,166,63,98,96,229,224,63,43,135,118,63,164,112,189,63,178,157,47,63,133,235,145,63,178,
157,47,63,98,68,139,76,63,178,157,47,63,84,227,5,63,43,135,118,63,84,227,5,63,180,200,166,63,98,84,227,5,63,14,45,210,63,168,198,75,63,66,96,245,63,233,38,145,63,143,194,245,63,108,233,38,145,63,119,190,39,64,99,101,0,0 };

	Path plug;

	plug.loadPathFromData(pathData, sizeof(pathData));
	

	Path p;

	g.setColour(Colour(0xFF888888));

	if(shouldPaintCable(CableLocation::Input))
    {
        PathFactory::scalePath(plug, pin1);
        g.fillPath(plug);
        
        p.startNewSubPath(start);
        p.lineTo(start1);//  addLineSegment({ start, start1 }, 2.0f);
        
        addCircleAtMidpoint(p, {start, start1}, signalDotOffset, true, getCircleAmp(-1, cableIndex, false));
        
    }
    
    if(shouldPaintCable(CableLocation::Output))
    {
        PathFactory::scalePath(plug, pin2);
        g.fillPath(plug);
        
        p.startNewSubPath(end);
        p.lineTo(end1);
        
        addCircleAtMidpoint(p, {end1, end}, signalDotOffset, true, getCircleAmp(childNodeComponents.size() - 1, cableIndex, true));
    }

    DropShadow sh;
	sh.colour = Colours::black.withAlpha(0.5f);
	sh.offset = { 0, 2 };
	sh.radius = 3;

    if(shouldPaintCable(CableLocation::BetweenNodes))
    {
        for (int i = 0; i < childNodeComponents.size() - 1; i++)
        {
            auto tc = childNodeComponents[i];
            auto nc = childNodeComponents[i + 1];

            Point<int> start_({ tc->getBounds().getCentreX(), tc->getBounds().getBottom() });
            Point<int> end_({ nc->getBounds().getCentreX(), nc->getBounds().getY() });

            Line<float> l(start_.toFloat().translated(xOffset, 0.0f), end_.toFloat().translated(xOffset, 0.0f));

            p.startNewSubPath(l.getStart());
            p.lineTo(l.getEnd());
            
            addCircleAtMidpoint(p, l, signalDotOffset, false, getCircleAmp(i, cableIndex, true));

            //p.addLineSegment(l, 2.0f);
        }
    }

	g.setColour(Colour(0xFF262626));
	g.strokePath(p, PathStrokeType(4.0f, PathStrokeType::mitered, PathStrokeType::rounded));

	auto c = header.colour.withMultipliedBrightness(0.7f);

	if (c == Colours::transparentBlack)
		c = Colour(0xFFAAAAAA);

	g.setColour(c);
	g.strokePath(p, PathStrokeType(2.0f, PathStrokeType::mitered, PathStrokeType::rounded));
}

void SerialNodeComponent::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();

    auto t = JUCE_LIVE_CONSTANT_OFF(1000.0f);
    signalDotOffset = (double)Time::getMillisecondCounter() / jmax(1.0f, t);


	Colour fc = getOutlineColour();

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff232323)));

	g.fillRect(b);

	g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0x22000000)), 0.0f, (float)UIValues::HeaderHeight, Colours::transparentBlack, 0.0f, 50.0f, false));
	
	auto copy = b;
	g.fillRect(copy.removeFromTop(50.0f));

	g.setColour(fc);
	g.drawRect(b, 1.0f);

	int yStart = 0;
	
	if (auto ng = findParentComponentOfClass<DspNetworkGraph>())
	{
		yStart = (ng->getLocalArea(this, getLocalBounds()).getY() + 15 + UIValues::HeaderHeight) % 10;
	}

	for (int i = yStart; i < getHeight() + 10; i += 10)
	{
		float multiplier = (float)i / float(getHeight());
		multiplier += JUCE_LIVE_CONSTANT_OFF(0.5f);
		g.setColour(fc.withMultipliedAlpha( multiplier * JUCE_LIVE_CONSTANT_OFF(0.08f)));
        g.fillRect(2, i, getWidth() - 2, 9 );
	}

	for (int i = 0; i < node->getNumChannelsToDisplay(); i++)
	{
		paintSerialCable(g, i);
	}

	drawHelp(g);

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

ParallelNodeComponent::ParallelNodeComponent(NodeContainer* node) :
	ContainerComponent(node)
{

}


bool ParallelNodeComponent::isMultiChannelNode() const
{
	return dataReference[PropertyIds::FactoryPath].toString() == "container.multi";
}


int ParallelNodeComponent::getInsertPosition(Point<int> position) const
{
	auto targetX = position.getX();
	auto p = childNodeComponents.size();

	for (auto nc : childNodeComponents)
	{
		if (targetX < nc->getX() + nc->getWidth() / 2)
		{
			p = childNodeComponents.indexOf(nc);
			break;
		}
	}

	return p;
}


juce::Rectangle<float> ParallelNodeComponent::getInsertRuler(int position) const
{
	int targetX = getWidth();

	if (auto childBeforeInsert = childNodeComponents[position])
		targetX = childBeforeInsert->getX();

	targetX -= (3 * UIValues::NodeMargin) / 4;

	if (childNodeComponents.size() == 0)
		targetX = getWidth() / 2 - UIValues::NodeMargin / 4;

	return Rectangle<int>(targetX, UIValues::HeaderHeight, UIValues::NodeMargin / 2, getHeight() - UIValues::HeaderHeight).toFloat();
}

juce::Colour ParallelNodeComponent::getOutlineColour() const
{
	if (auto c = dynamic_cast<NodeContainer*>(node.get()))
	{
		auto c2 = c->getContainerColour();

		if (!c2.isTransparent())
			return c2;
	}

	return NodeComponent::getOutlineColour();
}

void ParallelNodeComponent::resized()
{
	ContainerComponent::resized();

	auto startPos = getStartPosition();

	for (auto nc : childNodeComponents)
	{
		auto bounds = nc->node->getPositionInCanvas(startPos);

		bounds = nc->node->getBoundsWithoutHelp(bounds);
		auto helpBounds = nc->node->getHelpManager().getHelpSize().toNearestInt();

		nc->setBounds(bounds);

		auto x = bounds.getRight() + UIValues::NodeMargin;

		if(!nc->node->getHelpManager().isHelpBelow())
			x += helpBounds.getWidth();

		startPos = startPos.withX(x);
	}

	if (duplicateDisplay != nullptr)
	{
		auto b = getLocalBounds();
		b.removeFromTop(startPos.getY() - UIValues::NodeMargin);

		b.removeFromBottom(UIValues::PinHeight);

		duplicateDisplay->setBounds(b.removeFromRight(UIValues::DuplicateSize));
	}
		
}

void ParallelNodeComponent::paint(Graphics& g)
{
    auto t = JUCE_LIVE_CONSTANT_OFF(1000.0f);
    signalDotOffset = (double)Time::getMillisecondCounter() / jmax(1.0f, t);
    
    if (header.isDragging)
		g.setOpacity(0.3f);

	auto b = getLocalBounds().toFloat();
	

	Colour fc = getOutlineColour();
	
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff232323)));
	g.fillRect(b);
	
	drawTopBodyGradient(g, b);

	g.setColour(fc);

	g.drawRect(b, 1.0f);

	

	for (int i = 2; i < getWidth() + 10; i += 10)
	{
		auto halfWidth = (float)getWidth() / 2.0f;
		float multiplier = std::abs((float)i - halfWidth) / halfWidth;

		multiplier += JUCE_LIVE_CONSTANT_OFF(0.5f);
		g.setColour(fc.withMultipliedAlpha(multiplier * JUCE_LIVE_CONSTANT_OFF(0.07f)));

		g.fillRect(i, 2, 9, getHeight() - 2);

#if 0

		multiplier += JUCE_LIVE_CONSTANT_OFF(0.1f);

		g.setColour(fc.withMultipliedAlpha(multiplier * JUCE_LIVE_CONSTANT_OFF(0.2f)));

		g.drawVerticalLine(i, 2.0f, (float)getHeight() - 2.0f);
#endif
	}

	for (int i = 0; i < node->getNumChannelsToDisplay(); i++)
	{
		paintCable(g, i);
	}

	drawHelp(g);

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
	Path p;

	auto xOffset = getCableXOffset(cableIndex, 1);

	auto b2 = getLocalBounds();

	b2.removeFromTop(UIValues::HeaderHeight);

	if (dataReference[PropertyIds::ShowParameters])
		b2.removeFromTop(UIValues::ParameterHeight + UIValues::MacroDragHeight);

	b2.removeFromTop(UIValues::NodeMargin / 2);
	b2.removeFromBottom(UIValues::NodeMargin / 2);

	auto start = b2.removeFromTop(UIValues::PinHeight).getCentre().toFloat().translated(xOffset, 0.0f);
	auto end = b2.removeFromBottom(UIValues::PinHeight).getCentre().toFloat().translated(xOffset, 0.0f);
	auto pin1 = Rectangle<float>(start, start).withSizeKeepingCentre(10.0f, 10.0f);
	auto pin2 = Rectangle<float>(end, end).withSizeKeepingCentre(10.0f, 10.0f);

	static const unsigned char pathData[] = { 110,109,233,38,145,63,119,190,39,64,108,0,0,0,0,227,165,251,63,108,0,0,0,0,20,174,39,63,108,174,71,145,63,0,0,0,0,108,174,71,17,64,20,174,39,63,108,174,71,17,64,227,165,251,63,108,115,104,145,63,119,190,39,64,108,115,104,145,63,143,194,245,63,98,55,137,
145,63,143,194,245,63,193,202,145,63,143,194,245,63,133,235,145,63,143,194,245,63,98,164,112,189,63,143,194,245,63,96,229,224,63,152,110,210,63,96,229,224,63,180,200,166,63,98,96,229,224,63,43,135,118,63,164,112,189,63,178,157,47,63,133,235,145,63,178,
157,47,63,98,68,139,76,63,178,157,47,63,84,227,5,63,43,135,118,63,84,227,5,63,180,200,166,63,98,84,227,5,63,14,45,210,63,168,198,75,63,66,96,245,63,233,38,145,63,143,194,245,63,108,233,38,145,63,119,190,39,64,99,101,0,0 };

	Path plug;

	plug.loadPathFromData(pathData, sizeof(pathData));

	g.setColour(Colour(0xFF888888));

	PathFactory::scalePath(plug, pin1);
	g.fillPath(plug);
	PathFactory::scalePath(plug, pin2);
	g.fillPath(plug);

	//p.addPieSegment(pin1, 0.0f, float_Pi * 2.0f, 0.5f);
	//p.addPieSegment(pin2, 0.0f, float_Pi * 2.0f, 0.5f);

	if (dynamic_cast<SplitNode*>(node.get()) || (dynamic_cast<CloneNode*>(node.get()) != nullptr && node->getParameterFromIndex(1)->getValue()))
	{
		if (childNodeComponents.size() < 8)
		{
            int idx = 0;
            
            for (auto n : childNodeComponents)
            {
                auto b = n->getBounds().toFloat();

                Point<float> p1(b.getCentreX() + xOffset, b.getY());
                Point<float> p2(b.getCentreX() + xOffset, b.getBottom());

                if(shouldPaintCable(CableLocation::Input))
                {
                    p.addLineSegment({ start, p1 }, 2.0f);
                    addCircleAtMidpoint(p, {start, p1}, signalDotOffset, true, getCircleAmp(-1, cableIndex, false));
                }
                
                if(shouldPaintCable(CableLocation::Output))
                {
                    p.addLineSegment({ p2, end }, 2.0f);
                    addCircleAtMidpoint(p, {p2, end}, signalDotOffset, true, getCircleAmp(idx, cableIndex, true));
                }
                
                idx++;
            }
		}
		else
		{
			auto fn = childNodeComponents.getFirst();
			auto ln = childNodeComponents.getLast();

			{
				auto b = fn->getBounds().toFloat();

				Point<float> p1(b.getCentreX() + xOffset, b.getY());
				Point<float> p2(b.getCentreX() + xOffset, b.getBottom());

                if(shouldPaintCable(CableLocation::Input))
                    p.addLineSegment({ start, p1 }, 2.0f);
                
                if(shouldPaintCable(CableLocation::Output))
                    p.addLineSegment({ p2, end }, 2.0f);
			}
			{
				auto b = ln->getBounds().toFloat();

				Point<float> p1(b.getCentreX() + xOffset, b.getY());
				Point<float> p2(b.getCentreX() + xOffset, b.getBottom());

                if(shouldPaintCable(CableLocation::Input))
                    p.addLineSegment({ start, p1 }, 2.0f);
                
                if(shouldPaintCable(CableLocation::Output))
                    p.addLineSegment({ p2, end }, 2.0f);
			}

			p.addLineSegment({ start, end }, 2.0f);
		}
	}
	else if (auto bn = dynamic_cast<BranchNode*>(node.get()))
	{
		if(auto cn = childNodeComponents[bn->currentIndex])
		{
			auto b = cn->getBounds().toFloat();

			Point<float> p1(b.getCentreX() + xOffset, b.getY());
			Point<float> p2(b.getCentreX() + xOffset, b.getBottom());

            if(shouldPaintCable(CableLocation::Input))
                p.addLineSegment({ start, p1 }, 2.0f);
            
            if(shouldPaintCable(CableLocation::Output))
                p.addLineSegment({ p2, end }, 2.0f);
		}
	}
	else if (auto sn = dynamic_cast<SerialNode*>(node.get()))
	{
		DspNetworkPathFactory df;

		auto icon = df.createPath("swap-orientation");

		auto iconBounds = getLocalBounds().toFloat().removeFromLeft(UIValues::PinHeight).removeFromTop(UIValues::PinHeight).reduced(2.0f);

		df.scalePath(icon, iconBounds);

		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillPath(icon);

		if (auto f = childNodeComponents.getFirst())
		{
			auto b = f->getBounds().toFloat();
			Point<float> p1(b.getCentreX() + xOffset, b.getY());
			p.addLineSegment({ start, p1 }, 2.0f);
		}

		if (auto l = childNodeComponents.getLast())
		{
			auto b = l->getBounds().toFloat();
			Point<float> p2(b.getCentreX() + xOffset, b.getBottom());
			p.addLineSegment({ p2, end }, 2.0f);
		}

		for (int i = 0; i < childNodeComponents.size()-1; i++)
		{
			auto t = childNodeComponents[i];
			auto n = childNodeComponents[i + 1];

			auto tr = t->getBounds().getRight();
			auto tyc = t->getBounds().getCentreY();

			auto nx = n->getBounds().getCentreX();
			auto nyc = n->getBounds().getCentreY(); // if you can make it here

			Point<float> t1(tr, tyc);
			Point<float> n1(nx, nyc);

			p.addLineSegment({ t1, n1 }, 2.0f);
		}
	}
	else if (isMultiChannelNode())
	{
		int currentChannelIndex = 0;
		int channelInTarget = -1;
		NodeComponent* targetNode = nullptr;

		for (auto c : childNodeComponents)
		{
			int numChannelsForThisNode = c->node->getNumChannelsToDisplay();

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
            
            addCircleAtMidpoint(p, {start, p1}, signalDotOffset, true, getCircleAmp(-1, cableIndex, false));
            addCircleAtMidpoint(p, {p2, end}, signalDotOffset, true, getCircleAmp(-1, cableIndex, true));
            
		}
	}
	
	
	if (!node->isBypassed())
	{
		auto c = header.colour.withMultipliedBrightness(0.7f);

		if (c == Colours::transparentBlack)
			c = Colour(Colour(0xFFAAAAAA));

		g.setColour(c);
		g.fillPath(p);
	}
}


void MacroPropertyEditor::ConnectionEditor::buttonClicked(Button* b)
{
	if (b == &deleteButton)
	{
		auto dataCopy = data;
		auto undoManager = node->getUndoManager();

		auto func = [dataCopy, undoManager]()
		{
			dataCopy.getParent().removeChild(dataCopy, undoManager);
		};

		MessageManager::callAsync(func);
	}
	else if (b == &gotoButton)
	{
		auto nodeToShow = node.get();

		if(showSource)
		{
			nodeToShow = node->getRootNetwork()->getNodeForValueTree(valuetree::Helpers::findParentWithType(data, PropertyIds::Node));
		}
		else
		{
			auto nodeId = data[PropertyIds::NodeId].toString();
			nodeToShow = node->getRootNetwork()->getNodeWithId(nodeId);
		}
		
		if (nodeToShow != nullptr)
		{
			auto sp = findParentComponentOfClass<ZoomableViewport>();

			auto gotoNode = [sp, nodeToShow]()
			{
				auto nv = nodeToShow->getValueTree();
				auto um = nodeToShow->getUndoManager();

				ValueTree lockedContainer;

				valuetree::Helpers::forEachParent(nv, [&](ValueTree& v)
				{
					if(v.getType() == PropertyIds::Node)
					{
						v.setProperty(PropertyIds::Folded, false, um);

						if(v[PropertyIds::Locked])
						{
							lockedContainer = v;
							return true;
						}
							
					}
						
					return false;
				});

				sp->setCurrentModalWindow(nullptr, {});

				auto currentRootTree = sp->getContent<DspNetworkGraph>()->getCurrentRootNode()->getValueTree();

				if(!lockedContainer.isValid() && !nodeToShow->getValueTree().isAChildOf(currentRootTree))
				{
					lockedContainer = nodeToShow->getValueTree();

					if(lockedContainer.getParent().getType() == PropertyIds::Nodes)
						lockedContainer = lockedContainer.getParent().getParent();
				}

				if(lockedContainer.isValid())
				{
					if(auto newRoot = nodeToShow->getRootNetwork()->getNodeForValueTree(lockedContainer, false))
					{
						sp->getContent<DspNetworkGraph>()->setCurrentRootNode(newRoot, true, false);
					}
				}

				

				if (auto nc = sp->getContent<DspNetworkGraph>()->getComponent(nodeToShow))
				{
					nc->grabKeyboardFocus();
				}

				nodeToShow->getRootNetwork()->deselectAll();
				nodeToShow->getRootNetwork()->addToSelection(nodeToShow, ModifierKeys());
			};

			MessageManager::callAsync(gotoNode);
		}
	}
}

void MacroPropertyEditor::buttonClicked(Button* b)
{
	if (b == &connectionButton)
	{
		if (parameter != nullptr)
		{
			PopupLookAndFeel plaf;
			PopupMenu m;

			m.setLookAndFeel(&plaf);

			NodeBase::Ptr parent = node->getParentNode();

			struct Entry
			{
				String getName() const { return nId + pId; }
				String nId;
				String pId;
				bool isMod;
			};

			Array<Entry> pEntries;
			Array<Entry> mEntries;

			while (parent != nullptr)
			{
				if (auto c = dynamic_cast<NodeContainer*>(parent.get()))
				{
					for (auto n : c->getNodeList())
					{
						if (auto mod = dynamic_cast<ModulationSourceNode*>(n.get()))
						{
							mEntries.add({ parent->getId(), mod->getId(), true });	
						}
					}
				}

				for (auto p : NodeBase::ParameterIterator(*parent))
					pEntries.add({ parent->getId(), p->getId(), false });

				parent = parent->getParentNode();
			}

			int pOffset = 9000;
			int mOffset = 12000;

			if (pEntries.size() > 0)
			{
				m.addSectionHeader("Connect to Macro Parameter");

				for (int i = 0; i < pEntries.size(); i++)
				{
					m.addItem(pOffset + i, pEntries[i].getName());
				}
			}

			if (mEntries.size() > 0)
			{
				m.addSectionHeader("Connect to Modulation");

				for (int i = 0; i < mEntries.size(); i++)
				{
					m.addItem(mOffset + i, mEntries[i].getName());
				}
			}

			int result = m.show();

			if (result >= mOffset)
			{
				auto entry = mEntries[result - mOffset];
				auto details = DragHelpers::createDescription(entry.pId, "", true);
				parameter->addConnectionFrom(details);

			}

			else if (result >= pOffset)
			{
				auto e = pEntries[result - pOffset];
				auto details = DragHelpers::createDescription(e.nId, e.pId, false);
				parameter->addConnectionFrom(details);
			}
		}
	}
}

void ContainerComponent::ParameterComponent::resized()
{
	auto b = getLocalBounds();
	if (b.isEmpty())
		return;

	b.removeFromTop(15);

	if (leftTabComponent != nullptr)
		leftTabComponent->setBounds(b.removeFromLeft(leftTabComponent->getWidth()));

	for (auto s : sliders)
		s->setBounds(b.removeFromLeft(100));
}

}

