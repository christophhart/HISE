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


DspNetworkGraph::DspNetworkGraph(DspNetwork* n) :
	network(n),
	dataReference(n->getValueTree())
{
	network->addSelectionListener(this);
	rebuildNodes();
	setWantsKeyboardFocus(true);

	cableRepainter.setCallback(dataReference, {PropertyIds::Bypassed },
		valuetree::AsyncMode::Asynchronously,
		[this](ValueTree v, Identifier id)
	{
		if (v[PropertyIds::DynamicBypass].toString().isNotEmpty())
			repaint();
	});

	rebuildListener.setCallback(dataReference, valuetree::AsyncMode::Synchronously,
		[this](ValueTree c, bool)
	{
		if (c.getType() == PropertyIds::Node)
			triggerAsyncUpdate();
	});

	macroListener.setTypeToWatch(PropertyIds::Parameters);
	macroListener.setCallback(dataReference, valuetree::AsyncMode::Asynchronously,
		[this](ValueTree, bool)
	{
		this->rebuildNodes();
	});

	rebuildListener.forwardCallbacksForChildEvents(true);

	resizeListener.setCallback(dataReference, { PropertyIds::Folded, PropertyIds::ShowParameters },
		valuetree::AsyncMode::Asynchronously,
	  [this](ValueTree, Identifier)
	{
		this->resizeNodes();
	});

	setOpaque(true);
}

DspNetworkGraph::~DspNetworkGraph()
{
	if (network != nullptr)
		network->removeSelectionListener(this);
}

bool DspNetworkGraph::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::escapeKey)
		return Actions::deselectAll(*this);
	if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
		return Actions::deleteSelection(*this);
	if ((key.isKeyCode('j') || key.isKeyCode('J')))
		return Actions::showJSONEditorForSelection(*this);
	if ((key.isKeyCode('z') || key.isKeyCode('Z')) && key.getModifiers().isCommandDown())
		return Actions::undo(*this);
	if ((key.isKeyCode('Y') || key.isKeyCode('Y')) && key.getModifiers().isCommandDown())
		return Actions::redo(*this);

	return false;
}

void DspNetworkGraph::handleAsyncUpdate()
{
	rebuildNodes();
}

void DspNetworkGraph::rebuildNodes()
{
	addAndMakeVisible(root = dynamic_cast<NodeComponent*>(network->signalPath->createComponent()));
	resizeNodes();
}

void DspNetworkGraph::resizeNodes()
{
	auto b = network->signalPath->getPositionInCanvas({ UIValues::NodeMargin, UIValues::NodeMargin });
	setSize(b.getWidth() + 2 * UIValues::NodeMargin, b.getHeight() + 2 * UIValues::NodeMargin);
	resized();
}

void DspNetworkGraph::updateDragging(Point<int> position, bool copyNode)
{
	copyDraggedNode = copyNode;

	if (auto c = dynamic_cast<ContainerComponent*>(root.get()))
	{
		c->setDropTarget({});
	}

	if (auto hoveredComponent = root->getComponentAt(position))
	{
		auto container = dynamic_cast<ContainerComponent*>(hoveredComponent);

		if (container == nullptr)
			container = hoveredComponent->findParentComponentOfClass<ContainerComponent>();

		if (container != nullptr)
		{
			currentDropTarget = container;
			DBG(container->getName());
			auto pointInContainer = container->getLocalPoint(this, position);
			container->setDropTarget(pointInContainer);
		}
	}
}

void DspNetworkGraph::finishDrag()
{
	if (currentDropTarget != nullptr)
	{
		currentDropTarget->insertDraggedNode(currentlyDraggedComponent, copyDraggedNode);
		currentlyDraggedComponent = nullptr;
	}
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

void DspNetworkGraph::resized()
{
	if (root != nullptr)
	{
		root->setBounds(getLocalBounds().reduced(UIValues::NodeMargin));
		root->setTopLeftPosition({ UIValues::NodeMargin, UIValues::NodeMargin });
	}

	if (auto sp = findParentComponentOfClass<ScrollableParent>())
		sp->centerCanvas();
}

template <class T> void fillChildComponentList(Array<T*>& list, Component* c)
{
	for (int i = 0; i < c->getNumChildComponents(); i++)
	{
		auto child = c->getChildComponent(i);

		if (!child->isShowing())
			continue;

		if (auto typed = dynamic_cast<T*>(child))
		{
			list.add(typed);
		}

		fillChildComponentList(list, child);
	}
}

void DspNetworkGraph::paintOverChildren(Graphics& g)
{
	Array<ModulationSourceBaseComponent*> modSourceList;
	fillChildComponentList(modSourceList, this);

	for (auto modSource : modSourceList)
	{
		if (!modSource->getSourceNodeFromParent()->isBodyShown())
			continue;

		auto start = getCircle(modSource, false);

		g.setColour(Colours::black);
		g.fillEllipse(start);
		g.setColour(Colour(0xFFAAAAAA));
		g.drawEllipse(start, 2.0f);
	}

	Array<ParameterSlider*> list;
	fillChildComponentList(list, this);

	for (auto sourceSlider : list)
	{
		if (auto macro = dynamic_cast<NodeContainer::MacroParameter*>(sourceSlider->parameterToControl.get()))
		{
			for (auto c : macro->connections)
			{
				for (auto targetSlider : list)
				{
					auto target = targetSlider->parameterToControl;

					if (target == nullptr || !target->parent->isBodyShown())
						continue;

					if (c->matchesTarget(target))
					{
						auto start = getCircle(sourceSlider);
						auto end = getCircle(targetSlider);

						paintCable(g, start, end, Colour(MIDI_PROCESSOR_COLOUR));
					}
				}
			}
		}
	}

	for (auto modSource : modSourceList)
	{
		auto start = getCircle(modSource, false);

		if (auto sourceNode = modSource->getSourceNodeFromParent())
		{
			if (!sourceNode->isBodyShown())
				continue;

			auto modTargets = sourceNode->getModulationTargetTree();

			for (auto c : modTargets)
			{
				
				for (auto s : list)
				{
					if (!s->parameterToControl->parent->isBodyShown())
						continue;

					auto parentMatch = s->parameterToControl->parent->getId() == c[PropertyIds::NodeId].toString();
					auto paraMatch = s->parameterToControl->getId() == c[PropertyIds::ParameterId].toString();

					if (parentMatch && paraMatch)
					{
						auto end = getCircle(s);
						paintCable(g, start, end, Colour(0xffbe952c));
						break;
					}
				}
			}
		}

	}

	Array<NodeComponent::Header*> bypassList;
	fillChildComponentList(bypassList, this);

	for (auto b : bypassList)
	{
		auto n = b->parent.node.get();

		if (n == nullptr)
			continue;

		auto connection = n->getValueTree().getProperty(PropertyIds::DynamicBypass).toString();

		if (connection.isNotEmpty())
		{
			auto nodeId = connection.upToFirstOccurrenceOf(".", false, false);
			auto pId = connection.fromFirstOccurrenceOf(".", false, false);

			for (auto sourceSlider : list)
			{
				if (!sourceSlider->parameterToControl->parent->isBodyShown())
					continue;

				if (sourceSlider->parameterToControl->getId() == pId &&
					sourceSlider->parameterToControl->parent->getId() == nodeId)
				{
					auto start = getCircle(sourceSlider);
					auto end = getCircle(&b->powerButton).translated(0.0, -60.0f);

					auto c = n->isBypassed() ? Colours::grey : Colour(SIGNAL_COLOUR).withAlpha(0.8f);

					paintCable(g, start, end, c);
					break;
				}
			}
		}
	}

}

scriptnode::NodeComponent* DspNetworkGraph::getComponent(NodeBase::Ptr node)
{
	Array<NodeComponent*> nodes;
	fillChildComponentList(nodes, this);

	for (auto nc : nodes)
		if (nc->node == node)
			return nc;

	return nullptr;
}

bool DspNetworkGraph::setCurrentlyDraggedComponent(NodeComponent* n)
{
	if (auto parentContainer = dynamic_cast<ContainerComponent*>(n->getParentComponent()))
	{
		n->setBufferedToImage(true);
		auto b = n->getLocalArea(parentContainer, n->getBounds());
		parentContainer->removeDraggedNode(n);
		addAndMakeVisible(currentlyDraggedComponent = n);

		n->setBounds(b);
		return true;
	}

	return false;
}




bool DspNetworkGraph::Actions::deselectAll(DspNetworkGraph& g)
{
	g.network->deselectAll();

	return true;
}

bool DspNetworkGraph::Actions::deleteSelection(DspNetworkGraph& g)
{
	for (auto n : g.network->getSelection())
	{
		if (n == nullptr)
			continue;

		auto tree = n->getValueTree();
		tree.getParent().removeChild(tree, n->getUndoManager());
	}

	return true;
}

bool DspNetworkGraph::Actions::showJSONEditorForSelection(DspNetworkGraph& g)
{
	Array<var> list;
	NodeBase::List selection = g.network->getSelection();

	if (selection.size() != 1)
		return false;

	for (auto node : selection)
	{
		auto v = ValueTreeConverters::convertScriptNodeToDynamicObject(node->getValueTree());
		list.add(v);
	}

	JSONEditor* editor = new JSONEditor(var(list));

	editor->setEditable(true);

	auto callback = [&g, selection](const var& newData)
	{
		if (auto n = selection.getFirst())
		{
			if (newData.isArray())
			{
				auto newTree = ValueTreeConverters::convertDynamicObjectToScriptNodeTree(newData.getArray()->getFirst());
				n->getValueTree().copyPropertiesAndChildrenFrom(newTree, n->getUndoManager());
			}
			
		}

		return;
	};

	editor->setCallback(callback, true);
	editor->setName("Editing JSON");
	editor->setSize(400, 400);

	Component* componentToPointTo = &g;

	if (list.size() == 1)
	{
		if (auto fn = g.network->getSelection().getFirst())
		{
			Array<NodeComponent*> list;
			fillChildComponentList<NodeComponent>(list, &g);

			for (auto nc : list)
			{
				if (nc->node == fn)
				{
					componentToPointTo = nc;
					break;
				}
			}
		}
	}

	auto p = g.findParentComponentOfClass<ScrollableParent>();
	auto b = p->getLocalArea(componentToPointTo, componentToPointTo->getLocalBounds());

	CallOutBox::launchAsynchronously(editor, b, p);
	editor->grabKeyboardFocus();

	return true;
}

bool DspNetworkGraph::Actions::undo(DspNetworkGraph& g)
{
	if (auto um = g.network->getUndoManager())
		return um->undo();

	return false;
}

bool DspNetworkGraph::Actions::redo(DspNetworkGraph& g)
{
	if (auto um = g.network->getUndoManager())
		return um->redo();

	return false;
}

DspNetworkGraph::ScrollableParent::ScrollableParent(DspNetwork* n)
{
	addAndMakeVisible(viewport);
	viewport.setViewedComponent(new DspNetworkGraph(n), true);
	context.attachTo(*this);
	setOpaque(true);
}

DspNetworkGraph::ScrollableParent::~ScrollableParent()
{
	context.detach();
}

void DspNetworkGraph::ScrollableParent::mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel)
{
	if (event.mods.isCommandDown())
	{
		if (wheel.deltaY > 0)
			zoomFactor += 0.1f;
		else
			zoomFactor -= 0.1f;

		zoomFactor = jlimit(0.25f, 1.0f, zoomFactor);

		auto trans = AffineTransform::scale(zoomFactor);


		viewport.getViewedComponent()->setTransform(trans);
	}
}

void DspNetworkGraph::ScrollableParent::resized()
{
	viewport.setBounds(getLocalBounds());
	centerCanvas();
}

void DspNetworkGraph::ScrollableParent::centerCanvas()
{
	auto contentBounds = viewport.getViewedComponent()->getLocalBounds();
	auto sb = getLocalBounds();

	int x = 0;
	int y = 0;

	if (contentBounds.getWidth() < sb.getWidth())
		x = (sb.getWidth() - contentBounds.getWidth()) / 2;

	if (contentBounds.getHeight() < sb.getHeight())
		y = (sb.getHeight() - contentBounds.getHeight()) / 2;

	viewport.setTopLeftPosition(x, y);
}

juce::Identifier NetworkPanel::getProcessorTypeId() const
{
	return JavascriptProcessor::getConnectorId();
}

juce::Component* NetworkPanel::createContentComponent(int index)
{
	if (auto holder = dynamic_cast<DspNetwork::Holder*>(getConnectedProcessor()))
	{
		auto sa = holder->getIdList();

		auto id = sa[index];

		if (id.isNotEmpty())
		{
			auto network = holder->getOrCreate(id);

			return createComponentForNetwork(network);


		}
	}

	return nullptr;
}

void NetworkPanel::fillModuleList(StringArray& moduleList)
{
	fillModuleListWithType<JavascriptProcessor>(moduleList);
}

void NetworkPanel::fillIndexList(StringArray& sa)
{
	if (auto holder = dynamic_cast<DspNetwork::Holder*>(getConnectedProcessor()))
	{
		auto sa2 = holder->getIdList();

		sa.clear();
		sa.addArray(sa2);
	}
}

}

