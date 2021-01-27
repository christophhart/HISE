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
	parent.rebuildNodes();
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
		parent.repaint();

	if (id == PropertyIds::Folded)
		sendPooledChangeMessage();

	if (id == PropertyIds::ShowParameters)
		sendPooledChangeMessage();
}


void ContainerComponent::Updater::valueTreeParentChanged(ValueTree&)
{

}

ContainerComponent::ContainerComponent(NodeContainer* b) :
	NodeComponent(b->asNode()),
	updater(*this),
	parameters(*this)
{
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


void ContainerComponent::mouseMove(const MouseEvent& event)
{
	addPosition = getInsertPosition(event.getPosition());
	repaint();
}


void ContainerComponent::mouseExit(const MouseEvent& )
{
	addPosition = -1;
	repaint();
}


void ContainerComponent::mouseDown(const MouseEvent& event)
{
	if (auto n = findParentComponentOfClass<DspNetworkGraph>())
	{
		DspNetworkGraph::Actions::showKeyboardPopup(*n, KeyboardPopup::Mode::New);
	}

	if (event.mods.isRightButtonDown())
	{
		

#if 0
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		String clipboard = SystemClipboard::getTextFromClipboard();

		if (clipboard.startsWith("ScriptNode"))
		{
			m.addItem(120000, "Add node from clipboard", true);
		}

		m.addSectionHeader("Add existing node");

		auto network = node->getRootNetwork();

		auto nodeList = network->getListOfUnconnectedNodes();

		for (int i = 0; i < nodeList.size(); i++)
		{
			m.addItem(i + 1, "Add " + nodeList[i]->getId());
		}

		m.addSeparator();
		m.addSectionHeader("Create new node");

		auto sa = network->getListOfAllAvailableModuleIds();

		//sa.sort(true);

		

		String currentFactory;
		PopupMenu sub;
		int newId = 11000;

		for (const auto& s : sa)
		{
			auto thisFactory = s.upToFirstOccurrenceOf(".", false, false);

			if (thisFactory != currentFactory)
			{
				if (sub.getNumItems() != 0)
					m.addSubMenu(currentFactory, sub);

				sub = PopupMenu();

				currentFactory = thisFactory;
			}

			sub.addItem(newId++, s.fromFirstOccurrenceOf(".", false, false));
		}

		if (sub.getNumItems() != 0)
			m.addSubMenu(currentFactory, sub);

		int result = m.show();

		if (result != 0)
		{
			var newNode;

			auto container = dynamic_cast<NodeContainer*>(node.get());

			if (result == 120000)
			{
				auto data = clipboard.fromFirstOccurrenceOf("ScriptNode", false, false);
				auto newTree = ValueTreeConverters::convertBase64ToValueTree(data, true);

				if (newTree.isValid())
				{
					newNode = network->createFromValueTree(network->isPolyphonic(), newTree, true);
				}
			}
			else if (result >= 11000)
			{
				auto moduleId = sa[result - 11000];
				newNode = network->create(moduleId, {}, container->isPolyphonic());
			}

			container->assign(addPosition, newNode);
		}
#endif
	}
}


void ContainerComponent::removeDraggedNode(NodeComponent* draggedNode)
{
	int removeIndex = childNodeComponents.indexOf(draggedNode);

	removeChildComponent(draggedNode);

	auto dea = new DeactivatedComponent(draggedNode->node);

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
			auto copy = node->getRootNetwork()->cloneValueTreeWithNewIds(newTree);
			node->getRootNetwork()->createFromValueTree(container->isPolyphonic(), copy, true);
			container->getNodeTree().addChild(copy, insertPosition, node->getUndoManager());
		}
		else
		{
			newNode->node->setParent(var(node), insertPosition);

#if 0
			newTree.getParent().removeChild(newTree, node->getUndoManager());
			container->getNodeTree().addChild(newTree, insertPosition, node->getUndoManager());
#endif
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

void ContainerComponent::setDropTarget(juce::Point<int> position)
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

juce::Point<int> ContainerComponent::getStartPosition() const
{
	int y = UIValues::NodeMargin;
	y += UIValues::HeaderHeight;
	y += UIValues::PinHeight;

	if (dataReference[PropertyIds::ShowParameters])
		y += UIValues::ParameterHeight;

	return { UIValues::NodeMargin, y};
}

float ContainerComponent::getCableXOffset(int cableIndex, int factor /*= 1*/) const
{
	int numCables = node->getNumChannelsToProcess() - 1;
	int cableWidth = numCables * (UIValues::PinHeight * factor);
	int cableStart = getWidth() / 2 - cableWidth / 2;
	int cableX = cableStart + cableIndex * (UIValues::PinHeight * factor);

	return (float)cableX - (float)getWidth() / 2.0f;
}


void ContainerComponent::rebuildNodes()
{
	for (auto nc : childNodeComponents)
	{
		if(nc->node != nullptr)
			nc->node->getHelpManager().removeHelpListener(this);
	}

	childNodeComponents.clear();

	if (auto container = dynamic_cast<NodeContainer*>(node.get()))
	{
		for (auto n : container->nodes)
		{
			auto newNode = n->createComponent();

			n->getHelpManager().addHelpListener(this);
			addAndMakeVisible(newNode);
			childNodeComponents.add(newNode);
		}
	}

	resized();
	repaint();
}


SerialNodeComponent::SerialNodeComponent(SerialNode* node) :
	ContainerComponent(node)
{

}


int SerialNodeComponent::getInsertPosition(juce::Point<int> position) const
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

void SerialNodeComponent::resized()
{
	ContainerComponent::resized();

	juce::Point<int> startPos = getStartPosition();

	for (auto nc : childNodeComponents)
	{
		auto bounds = nc->node->getPositionInCanvas(startPos);
		bounds = nc->node->getBoundsWithoutHelp(bounds);

		nc->setBounds(bounds);

		auto helpBounds = nc->node->getHelpManager().getHelpSize().toNearestInt();

		auto widthWithHelp = bounds.getWidth() + helpBounds.getWidth();
		auto heightWithHelp = jmax(bounds.getHeight(), helpBounds.getHeight());

		auto x = (getWidth() - widthWithHelp) / 2;
		nc->setTopLeftPosition(x, bounds.getY());

		startPos = startPos.withY(bounds.getY() + heightWithHelp + UIValues::NodeMargin);
	}
}

void SerialNodeComponent::paintSerialCable(Graphics& g, int cableIndex)
{
	auto xOffset = getCableXOffset(cableIndex);

	auto b2 = getLocalBounds();
	b2.removeFromTop(UIValues::HeaderHeight);

	if (dataReference[PropertyIds::ShowParameters])
		b2.removeFromTop(UIValues::ParameterHeight);

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

	Path p;

	p.addPieSegment(pin1, 0.0, float_Pi*2.0f, 0.5f);
	p.addPieSegment(pin2, 0.0, float_Pi*2.0f, 0.5f);
	p.addLineSegment({ start, start1 }, 2.0f);
	p.addLineSegment({ end, end1 }, 2.0f);

    DropShadow sh;
	sh.colour = Colours::black.withAlpha(0.5f);
	sh.offset = { 0, 2 };
	sh.radius = 3;

	for (int i = 0; i < childNodeComponents.size() - 1; i++)
	{
		auto tc = childNodeComponents[i];
		auto nc = childNodeComponents[i + 1];

		juce::Point<int> start_({ tc->getBounds().getCentreX(), tc->getBounds().getBottom() });
		juce::Point<int> end_({ nc->getBounds().getCentreX(), nc->getBounds().getY() });

		Line<float> l(start_.toFloat().translated(xOffset, 0.0f), end_.toFloat().translated(xOffset, 0.0f));

		p.addLineSegment(l, 2.0f);
	}

	//sh.drawForPath(g, p);
	g.setColour(Colour(0xFFAAAAAA));

	g.fillPath(p);
}

void SerialNodeComponent::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF3f363a)));

	

	if (dynamic_cast<DspNetworkGraph*>(getParentComponent()) != nullptr)
	{
		g.setColour(dynamic_cast<Processor*>(node->getScriptProcessor())->getColour().withMultipliedSaturation(0.6f).withMultipliedBrightness(0.8f));
	}

	g.fillRoundedRectangle(b, 5.0f);
	g.setColour(getOutlineColour());
	g.drawRoundedRectangle(b.reduced(1.5f), 0.0f, 3.0f);

	g.setColour(Colours::white.withAlpha(0.03f));

	int yStart = 0;
	
	if (auto ng = findParentComponentOfClass<DspNetworkGraph>())
	{
		yStart = (ng->getLocalArea(this, getLocalBounds()).getY() + 15) % 10;
	}

	for (int i = yStart; i < getHeight(); i += 10)
	{
		g.drawHorizontalLine(i, 3.0f, (float)getWidth() - 3.0f);
	}

	for (int i = 0; i < node->getNumChannelsToProcess(); i++)
	{
		paintSerialCable(g, i);
	}

	drawHelp(g);


	DropShadow sh;
	sh.colour = Colours::black.withAlpha(0.5f);
	sh.offset = { 0, 2 };
	sh.radius = 3;

	for (auto nc : childNodeComponents)
	{
		Path rr;
		rr.addRoundedRectangle(nc->getBounds().toFloat(), 5.0f);
		//sh.drawForPath(g, rr);
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

ParallelNodeComponent::ParallelNodeComponent(ParallelNode* node) :
	ContainerComponent(node)
{

}


bool ParallelNodeComponent::isMultiChannelNode() const
{
	return dataReference[PropertyIds::FactoryPath].toString() == "container.multi";
}


int ParallelNodeComponent::getInsertPosition(juce::Point<int> position) const
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

		startPos = startPos.withX(bounds.getRight() + helpBounds.getWidth() + UIValues::NodeMargin);
	}
}

void ParallelNodeComponent::paint(Graphics& g)
{
	if (header.isDragging)
		g.setOpacity(0.3f);

	auto b = getLocalBounds().toFloat();
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF39363f)));

	if (isMultiChannelNode())
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF393f36)));

	

	g.fillRoundedRectangle(b, 5.0f);
	g.setColour(getOutlineColour());
	g.drawRoundedRectangle(b.reduced(1.5f), 5.0f, 3.0f);

	g.setColour(Colours::white.withAlpha(0.03f));

	for (int i = 0; i < getWidth(); i += 10)
	{
		g.drawVerticalLine(i, 0.0f, (float)getHeight());
	}

	for (int i = 0; i < node->getNumChannelsToProcess(); i++)
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
	auto xOffset = getCableXOffset(cableIndex, 1);

	auto b2 = getLocalBounds();

	b2.removeFromTop(UIValues::HeaderHeight);

	if (dataReference[PropertyIds::ShowParameters])
		b2.removeFromTop(UIValues::ParameterHeight);

	b2.removeFromTop(UIValues::NodeMargin / 2);
	b2.removeFromBottom(UIValues::NodeMargin / 2);

	auto start = b2.removeFromTop(UIValues::PinHeight).getCentre().toFloat().translated(xOffset, 0.0f);
	auto end = b2.removeFromBottom(UIValues::PinHeight).getCentre().toFloat().translated(xOffset, 0.0f);

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

			juce::Point<float> p1(b.getCentreX() + targetOffsetX, b.getY());
			juce::Point<float> p2(b.getCentreX() + targetOffsetX, b.getBottom());

			p.addLineSegment({ start, p1 }, 2.0f);
			p.addLineSegment({ p2, end }, 2.0f);
		}
	}
	else
	{
		for (auto n : childNodeComponents)
		{
			auto b = n->getBounds().toFloat();

			juce::Point<float> p1(b.getCentreX() + xOffset, b.getY());
			juce::Point<float> p2(b.getCentreX() + xOffset, b.getBottom());

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



ModChainNodeComponent::ModChainNodeComponent(ModulationChainNode* node):
	ContainerComponent(node)
{
}

int ModChainNodeComponent::getInsertPosition(juce::Point<int> position) const
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

juce::Rectangle<float> ModChainNodeComponent::getInsertRuler(int position) const
{
	int targetY = getHeight() - UIValues::PinHeight;

	if (auto childBeforeInsert = childNodeComponents[position])
		targetY = childBeforeInsert->getY();

	targetY -= (3 * UIValues::NodeMargin / 4);
	return Rectangle<int>(UIValues::NodeMargin, targetY, getWidth() - 2 * UIValues::NodeMargin, UIValues::NodeMargin / 2).toFloat();
}

void ModChainNodeComponent::resized()
{
	ContainerComponent::resized();

	juce::Point<int> startPos = getStartPosition();

	for (auto nc : childNodeComponents)
	{
		auto bounds = nc->node->getPositionInCanvas(startPos);
		bounds = nc->node->getBoundsToDisplay(bounds);

		nc->setBounds(bounds);

		auto helpBounds = nc->node->getHelpManager().getHelpSize().toNearestInt();
		auto widthWithHelp = bounds.getWidth() + helpBounds.getWidth();
		auto heightWithHelp = jmax(bounds.getHeight(), helpBounds.getHeight());
		auto x = (getWidth() - widthWithHelp) / 2;
		nc->setTopLeftPosition(x, bounds.getY());
		startPos = startPos.withY(bounds.getY() + heightWithHelp + UIValues::NodeMargin);
	}
}

void ModChainNodeComponent::paint(Graphics& g)
{
	auto b = getLocalBounds().toFloat();
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xcd403c32)));

	g.fillRoundedRectangle(b, 5.0f);
	g.setColour(getOutlineColour());
	g.drawRoundedRectangle(b.reduced(1.5f), 5.0f, 3.0f);

	int yStart = 0;
	g.setColour(Colours::white.withAlpha(0.03f));

	if (auto ng = findParentComponentOfClass<DspNetworkGraph>())
	{
		yStart = (ng->getLocalArea(this, getLocalBounds()).getY() + 15) % 10;
	}

	for (int i = yStart; i < getHeight(); i += 10)
	{
		g.drawHorizontalLine(i, 3.0f, (float)getWidth() - 3.0f);
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
	else
	{
		if (auto targetNode = node->getRootNetwork()->getNodeWithId(data[PropertyIds::NodeId].toString()))
		{
			auto sp = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();

			auto gotoNode = [sp, targetNode]()
			{
				sp->setCurrentModalWindow(nullptr, {});

				if (auto nc = sp->getGraph()->getComponent(targetNode))
				{
					nc->grabKeyboardFocus();
				}

				targetNode->getRootNetwork()->deselectAll();
				targetNode->getRootNetwork()->addToSelection(targetNode, ModifierKeys());
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
							if(mod->isUsingModulation())
								mEntries.add({ parent->getId(), mod->getId(), true });
						}
					}
				}

				for (int i = 0; i < parent->getNumParameters(); i++)
					pEntries.add({ parent->getId(), parent->getParameter(i)->getId(), false });

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

}

