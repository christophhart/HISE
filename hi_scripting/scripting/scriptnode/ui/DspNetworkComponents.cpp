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
#if 0
	if ((key.isKeyCode('j') || key.isKeyCode('J')))
		return Actions::showJSONEditorForSelection(*this);
#endif
	if ((key.isKeyCode('z') || key.isKeyCode('Z')) && key.getModifiers().isCommandDown())
		return Actions::undo(*this);
	if ((key.isKeyCode('Y') || key.isKeyCode('Y')) && key.getModifiers().isCommandDown())
		return Actions::redo(*this);
	if ((key.isKeyCode('d') || key.isKeyCode('D')) && key.getModifiers().isCommandDown())
		return Actions::duplicateSelection(*this);
	if ((key.isKeyCode('n') || key.isKeyCode('N')))
		return Actions::showKeyboardPopup(*this, KeyboardPopup::Mode::New);
	if ((key).isKeyCode('f') || key.isKeyCode('F'))
		return Actions::foldSelection(*this);
	if ((key).isKeyCode('u') || key.isKeyCode('U'))
		return Actions::toggleFreeze(*this);
	if ((key).isKeyCode('p') || key.isKeyCode('P'))
		return Actions::editNodeProperty(*this);
	if ((key).isKeyCode('q') || key.isKeyCode('Q'))
		return Actions::toggleBypass(*this);
	if (((key).isKeyCode('c') || key.isKeyCode('C')) && key.getModifiers().isCommandDown())
		return Actions::copyToClipboard(*this);
	if (key == KeyPress::upKey || key == KeyPress::downKey)
		return Actions::arrowKeyAction(*this, key);

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
                    if(s->parameterToControl == nullptr)
                        continue;
                    
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




void DspNetworkGraph::Actions::selectAndScrollToNode(DspNetworkGraph& g, NodeBase::Ptr node)
{
	g.network->addToSelection(node, {});

	if (auto nc = g.getComponent(node))
	{
		auto viewport = g.findParentComponentOfClass<Viewport>();

		auto nodeArea = viewport->getLocalArea(nc, nc->getLocalBounds());
		auto viewArea = viewport->getViewArea();

		if (!viewArea.contains(nodeArea))
		{
			int deltaX = 0;
			int deltaY = 0;

			if (nodeArea.getX() < viewArea.getX())
				deltaX = nodeArea.getX() - viewArea.getX();
			else if (nodeArea.getRight() > viewArea.getRight() && viewArea.getWidth() > nodeArea.getWidth())
				deltaX = nodeArea.getRight() - viewArea.getRight();
			

			if (nodeArea.getY() < viewArea.getY())
				deltaY = nodeArea.getY() - viewArea.getY();
			else if (nodeArea.getBottom() > viewArea.getBottom() && viewArea.getHeight() > nodeArea.getHeight())
				deltaY = nodeArea.getBottom() - viewArea.getBottom();
			

			viewport->setViewPosition(viewArea.getX() + deltaX, viewArea.getY() + deltaY);

		}


	}
}

bool DspNetworkGraph::Actions::freezeNode(NodeBase::Ptr node)
{
	auto freezedId = node->getValueTree()[PropertyIds::FreezedId].toString();

	if (freezedId.isNotEmpty())
	{
		if (auto fn = dynamic_cast<NodeBase*>(node->getRootNetwork()->get(freezedId).getObject()))
		{
			node->getRootNetwork()->deselect(fn);

			auto newTree = fn->getValueTree();
			auto oldTree = node->getValueTree();
			auto um = node->getUndoManager();

			auto f = [oldTree, newTree, um]()
			{
				auto p = oldTree.getParent();

				int position = p.indexOf(oldTree);
				p.removeChild(oldTree, um);
				p.addChild(newTree, position, um);
			};

			MessageManager::callAsync(f);

			auto nw = node->getRootNetwork();
			auto s = [newTree, nw]()
			{
				auto newNode = nw->getNodeForValueTree(newTree);
				nw->deselectAll();
				nw->addToSelection(newNode, ModifierKeys());
			};

			MessageManager::callAsync(s);
		}

		return true;
	}

	auto freezedPath = node->getValueTree()[PropertyIds::FreezedPath].toString();

	if (freezedPath.isNotEmpty())
	{
		auto newNode = node->getRootNetwork()->create(freezedPath, node->getId() + "_freezed");

		if (auto nn = dynamic_cast<NodeBase*>(newNode.getObject()))
		{
			auto newTree = nn->getValueTree();
			auto oldTree = node->getValueTree();
			auto um = node->getUndoManager();

			auto f = [oldTree, newTree, um]()
			{
				auto p = oldTree.getParent();

				int position = p.indexOf(oldTree);
				p.removeChild(oldTree, um);
				p.addChild(newTree, position, um);
			};

			MessageManager::callAsync(f);

			auto nw = node->getRootNetwork();
			auto s = [newTree, nw]()
			{
				auto newNode = nw->getNodeForValueTree(newTree);
				nw->deselectAll();
				nw->addToSelection(newNode, ModifierKeys());
			};

			MessageManager::callAsync(s);
		}

		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::unfreezeNode(NodeBase::Ptr node)
{
	if (auto hc = node->getAsRestorableNode())
	{
		// Check if there is already a node that was unfrozen

		for (auto n : node->getRootNetwork()->getListOfUnconnectedNodes())
		{
			if (n->getValueTree()[PropertyIds::FreezedId].toString() == node->getId())
			{
				auto newTree = n->getValueTree();
				auto oldTree = node->getValueTree();
				auto um = node->getUndoManager();

				auto f = [oldTree, newTree, um]()
				{
					auto p = oldTree.getParent();

					int position = p.indexOf(oldTree);
					p.removeChild(oldTree, um);
					p.addChild(newTree, position, um);
				};

				MessageManager::callAsync(f);

				auto nw = node->getRootNetwork();

				auto s = [newTree, nw]()
				{
					auto newNode = nw->getNodeForValueTree(newTree);
					nw->deselectAll();
					nw->addToSelection(newNode, ModifierKeys());
				};

				MessageManager::callAsync(s);

				return true;
			}
		}

		auto t = hc->getSnippetText();

		if (t.isNotEmpty())
		{
			auto newTree = ValueTreeConverters::convertBase64ToValueTree(t, true);
			newTree = node->getRootNetwork()->cloneValueTreeWithNewIds(newTree);
			newTree.setProperty(PropertyIds::FreezedPath, node->getValueTree()[PropertyIds::FactoryPath], nullptr);
			newTree.setProperty(PropertyIds::FreezedId, node->getId(), nullptr);

			{
				auto oldTree = node->getValueTree();
				auto um = node->getUndoManager();

				auto newNode = node->getRootNetwork()->createFromValueTree(true, newTree, true);

				auto f = [oldTree, newTree, um]()
				{
					auto p = oldTree.getParent();

					int position = p.indexOf(oldTree);
					p.removeChild(oldTree, um);
					p.addChild(newTree, position, um);
				};

				MessageManager::callAsync(f);

				auto nw = node->getRootNetwork();

				auto s = [newNode, nw]()
				{
					nw->deselectAll();
					nw->addToSelection(newNode, ModifierKeys());
				};

				MessageManager::callAsync(s);
			}
		}

		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::toggleBypass(DspNetworkGraph& g)
{
	auto selection = g.network->getSelection();

	if (selection.isEmpty())
		return false;

	bool oldState = selection.getFirst()->isBypassed();

	for (auto n : selection)
	{
		n->setBypassed(!oldState);
	}

	return true;
}

bool DspNetworkGraph::Actions::toggleFreeze(DspNetworkGraph& g)
{
	auto selection = g.network->getSelection();

	if (selection.isEmpty())
		return false;

	auto f = selection.getFirst();

	if (auto r = f->getAsRestorableNode())
	{
		unfreezeNode(f);
		return true;
	}
	else if (freezeNode(f))
	{
		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::copyToClipboard(DspNetworkGraph& g)
{
	if (auto n = g.network->getSelection().getFirst())
	{
		g.getComponent(n)->handlePopupMenuResult((int)NodeComponent::MenuActions::ExportAsSnippet);
		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::editNodeProperty(DspNetworkGraph& g)
{
	if (auto n = g.network->getSelection().getFirst())
	{
		g.getComponent(n)->handlePopupMenuResult((int)NodeComponent::MenuActions::EditProperties);
		return true;
	}

	return false;
}

bool DspNetworkGraph::Actions::foldSelection(DspNetworkGraph& g)
{
	auto selection = g.network->getSelection();

	if (selection.isEmpty())
		return false;

	auto shouldBeFolded = !(bool)selection.getFirst()->getValueTree()[PropertyIds::Folded];

	for (auto n : selection)
		n->setValueTreeProperty(PropertyIds::Folded, shouldBeFolded);

	return true;
}

bool DspNetworkGraph::Actions::arrowKeyAction(DspNetworkGraph& g, const KeyPress& k)
{
	auto node = g.network->getSelection().getFirst();

	if (node == nullptr || g.network->getSelection().size() > 1)
		return false;

	auto network = g.network;

	bool swapAction = k.getModifiers().isShiftDown();

	if (swapAction)
	{
		auto swapWithPrev = k == KeyPress::upKey;

		auto tree = node->getValueTree();
		auto parent = tree.getParent();
		auto index = node->getIndexInParent();

		if (swapWithPrev)
			parent.moveChild(index, index - 1, node->getUndoManager());
		else
			parent.moveChild(index, index + 1, node->getUndoManager());

		return true;
	}
	else
	{
		

		auto selectPrev = k == KeyPress::upKey;
		auto index = node->getIndexInParent();

		if (selectPrev)
		{
			auto container = dynamic_cast<NodeContainer*>(node->getParentNode());

			if (container == nullptr)
				return false;

			if (index == 0)
				selectAndScrollToNode(g, node->getParentNode());
			else
			{
				auto prevNode = container->getNodeList()[index - 1];

				if (auto prevContainer = dynamic_cast<NodeContainer*>(prevNode.get()))
				{
					if (auto lastChild = prevContainer->getNodeList().getLast())
					{
						selectAndScrollToNode(g, lastChild);
						return true;
					}
				}

				selectAndScrollToNode(g, prevNode);
				return true;
			}
				

			return true;
		}
		else
		{
			auto container = dynamic_cast<NodeContainer*>(node.get());

			if (container != nullptr && node->isBodyShown())
			{
				if (auto firstChild = container->getNodeList()[0])
				{
					selectAndScrollToNode(g, firstChild);
					return true;
				}
			}
			
			container = dynamic_cast<NodeContainer*>(node->getParentNode());
			
			if (container == nullptr)
				return false;

			if (auto nextSibling = container->getNodeList()[index + 1])
			{
				selectAndScrollToNode(g, nextSibling);
				return true;
			}

			node = node->getParentNode();

			container = dynamic_cast<NodeContainer*>(node->getParentNode());

			if (container == nullptr)
				return false;

			index = node->getIndexInParent();

			if (auto nextSibling = container->getNodeList()[index + 1])
			{
				selectAndScrollToNode(g, nextSibling);
				return true;
			}
		}
	}

	return false;
}

bool DspNetworkGraph::Actions::showKeyboardPopup(DspNetworkGraph& g, KeyboardPopup::Mode )
{
	auto firstInSelection = g.network->getSelection().getFirst();

	NodeBase::Ptr containerToLookFor;

	int addPosition = -1;

	bool somethingSelected = dynamic_cast<NodeContainer*>(firstInSelection.get()) != nullptr;
	bool stillInNetwork = somethingSelected && firstInSelection->getParentNode() != nullptr;
	

	if (somethingSelected && stillInNetwork)
		containerToLookFor = firstInSelection;
	else if (firstInSelection != nullptr)
	{
		containerToLookFor = firstInSelection->getParentNode();
		addPosition = firstInSelection->getIndexInParent() + 1;
	}

	Array<ContainerComponent*> list;

	fillChildComponentList(list, &g);
	
	bool mouseOver = g.isMouseOver(true);

	if (mouseOver)
	{
		int hoverPosition = -1;
		NodeBase* hoverContainer = nullptr;

		for (auto nc : list)
		{
			auto thisAdd = nc->getCurrentAddPosition();

			if (thisAdd != -1)
			{
				hoverPosition = thisAdd;
				hoverContainer = nc->node;
				break;
			}
		}

		if (hoverPosition != -1)
		{
			containerToLookFor = nullptr;
			addPosition = -1;
		}
	}

	for (auto nc : list)
	{
		auto thisAddPosition = nc->getCurrentAddPosition();

		
		bool containerIsSelected = nc->node == containerToLookFor;
		bool nothingSelectedAndAddPositionMatches = (containerToLookFor == nullptr && thisAddPosition != -1);

		if (containerIsSelected || nothingSelectedAndAddPositionMatches)
		{
			if (addPosition == -1)
				addPosition = thisAddPosition;

			KeyboardPopup* newPopup = new KeyboardPopup(nc->node, addPosition);

			auto midPoint = nc->getInsertRuler(addPosition);

			auto sp = g.findParentComponentOfClass<ScrollableParent>();

			auto r = sp->getLocalArea(nc, midPoint.toNearestInt());

			

			sp->setCurrentModalWindow(newPopup, r);
		}
	}

	return true;
}

bool DspNetworkGraph::Actions::duplicateSelection(DspNetworkGraph& g)
{
	int insertIndex = 0;

	for (auto n : g.network->getSelection())
	{
		auto tree = n->getValueTree();
		insertIndex = jmax(insertIndex, tree.getParent().indexOf(tree));
	}

	for (auto n : g.network->getSelection())
	{
		auto tree = n->getValueTree();
		auto copy = n->getRootNetwork()->cloneValueTreeWithNewIds(tree);
		n->getRootNetwork()->createFromValueTree(true, copy, true);
		tree.getParent().addChild(copy, insertIndex, n->getUndoManager());
		insertIndex = tree.getParent().indexOf(copy);
	}

	return true;
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

	g.network->deselectAll();

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
			Array<NodeComponent*> ncList;
			fillChildComponentList<NodeComponent>(ncList, &g);

			for (auto nc : ncList)
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

	p->setCurrentModalWindow(editor, b);

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
	addAndMakeVisible(dark);
	dark.setVisible(false);
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
			zoomFactor += 0.02f;
		else
			zoomFactor -= 0.02f;

		zoomFactor = jlimit(0.15f, 1.0f, zoomFactor);

		auto trans = AffineTransform::scale(zoomFactor);


		viewport.getViewedComponent()->setTransform(trans);
	}
}

void DspNetworkGraph::ScrollableParent::resized()
{
	viewport.setBounds(getLocalBounds());
	dark.setBounds(getLocalBounds());
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

KeyboardPopup::Help::Help(DspNetwork* n) :
	renderer("", nullptr)
{
#if USE_BACKEND

	auto bp = dynamic_cast<BackendProcessor*>(n->getScriptProcessor()->getMainController_())->getDocProcessor();
	
	rootDirectory = bp->getDatabaseRootDirectory();
	renderer.setDatabaseHolder(bp);
	
	initGenerator(rootDirectory, bp);
	renderer.setImageProvider(new doc::ScreenshotProvider(&renderer));
	renderer.setLinkResolver(new doc::Resolver(rootDirectory));
	
#endif

		
}


void KeyboardPopup::Help::showDoc(const String& text)
{
    ignoreUnused(text);
    
#if USE_BACKEND
	if (text.isEmpty())
	{
		renderer.setNewText("> no search results");
		return;
	}

	String link;

	link << doc::ItemGenerator::getWildcard();
	link << "/list/";
	link << text.replace(".", "/");

	MarkdownLink url(rootDirectory, link);
	renderer.gotoLink(url);
	rebuild(getWidth());
#endif
}

bool KeyboardPopup::Help::initialised = false;

void KeyboardPopup::Help::initGenerator(const File& root, MainController* mc)
{
    ignoreUnused(root, mc);
    
#if USE_BACKEND
	if (initialised)
		return;

	auto bp = dynamic_cast<BackendProcessor*>(mc);

	static doc::ItemGenerator gen(root, *bp);
	gen.createRootItem(bp->getDatabase());

	initialised = true;
#endif
}


void KeyboardPopup::addNodeAndClose(String path)
{
	auto sp = findParentComponentOfClass<DspNetworkGraph::ScrollableParent>();

	auto container = dynamic_cast<NodeContainer*>(node.get());
	auto ap = addPosition;

	if (path.startsWith("ScriptNode"))
	{
		auto f = [sp, container, ap]()
		{
			sp->setCurrentModalWindow(nullptr, {});

			auto clipboard = SystemClipboard::getTextFromClipboard();
			auto data = clipboard.fromFirstOccurrenceOf("ScriptNode", false, false);
			auto newTree = ValueTreeConverters::convertBase64ToValueTree(data, true);

			if (newTree.isValid())
			{
				var newNode;
				auto network = container->asNode()->getRootNetwork();

				newNode = network->createFromValueTree(network->isPolyphonic(), newTree, true);

				container->assign(ap, newNode);

				network->deselectAll();
				network->addToSelection(dynamic_cast<NodeBase*>(newNode.getObject()), ModifierKeys());
			}

			sp->setCurrentModalWindow(nullptr, {});
		};

		MessageManager::callAsync(f);
	}
	else
	{
		auto f = [sp, path, container, ap]()
		{
			if (path.isNotEmpty())
			{
				var newNode;

				auto network = container->asNode()->getRootNetwork();

				newNode = network->get(path);

				if(!newNode.isObject())
					newNode = network->create(path, {});

				container->assign(ap, newNode);

				network->deselectAll();
				network->addToSelection(dynamic_cast<NodeBase*>(newNode.getObject()), ModifierKeys());
			}

			sp->setCurrentModalWindow(nullptr, {});
		};

		MessageManager::callAsync(f);
	}

	

	
}

bool KeyboardPopup::keyPressed(const KeyPress& k, Component*)
{
	if (k == KeyPress::F1Key)
	{
		buttonClicked(&helpButton);
		return true;
	}
	if (k == KeyPress::escapeKey)
	{
		addNodeAndClose({});
	}
	if (k == KeyPress::upKey)
	{
		auto pos = list.selectNext(false);
		scrollToMakeVisible(pos);
		nodeEditor.setText(list.getCurrentText(), dontSendNotification);
		updateHelp();
		return true;
	}
	else if (k == KeyPress::downKey)
	{
		auto pos = list.selectNext(true);
		scrollToMakeVisible(pos);
		nodeEditor.setText(list.getCurrentText(), dontSendNotification);
		updateHelp();
		return true;
	}
	else if (k == KeyPress::returnKey)
	{
		addNodeAndClose(list.getTextToInsert());
		return true;
	}

	return false;
}

KeyboardPopup::PopupList::Item::Item(const Entry& entry_, bool isSelected_) :
	entry(entry_),
	selected(isSelected_),
	deleteButton("delete", this, f)
{
	if (entry.t == ExistingNode)
		addAndMakeVisible(deleteButton);

	static const StringArray icons = { "clipboard", "oldnode", "newnode" };

	p = f.createPath(icons[(int)entry.t]);
}

void KeyboardPopup::PopupList::Item::buttonClicked(Button* )
{
	auto plist = findParentComponentOfClass<PopupList>();
	
	plist->network->deleteIfUnused(entry.insertString);

	MessageManager::callAsync([plist]()
	{
		plist->rebuildItems();
	});
}

void KeyboardPopup::PopupList::Item::mouseDown(const MouseEvent&)
{
	findParentComponentOfClass<PopupList>()->setSelected(this);
}

void KeyboardPopup::PopupList::Item::mouseUp(const MouseEvent& event)
{
	if (!event.mouseWasDraggedSinceMouseDown())
	{
		findParentComponentOfClass<KeyboardPopup>()->addNodeAndClose(entry.insertString);
	}
}

void KeyboardPopup::PopupList::Item::paint(Graphics& g)
{
	if (selected)
	{
		g.fillAll(Colours::white.withAlpha(0.2f));
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawRect(getLocalBounds(), 1);
	}

	auto b = getLocalBounds().toFloat();

	auto icon = b.removeFromLeft(b.getHeight());

	PathFactory::scalePath(p, icon.reduced(6.0f));

	b.removeFromLeft(10.0f);

	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.8f));
	g.drawText(entry.displayName, b.reduced(3.0f), Justification::centredLeft);

	g.setColour(Colours::white.withAlpha(0.3f));
	g.fillPath(p);

	g.setColour(Colours::black.withAlpha(0.1f));
	g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());
}

void KeyboardPopup::PopupList::Item::resized()
{
	deleteButton.setBounds(getLocalBounds().removeFromRight(getHeight()).reduced(3));
}

}

