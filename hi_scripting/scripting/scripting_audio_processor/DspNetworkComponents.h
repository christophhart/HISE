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

class ProcessorWithScriptingContent;

namespace scriptnode
{
using namespace juce;

struct NodeComponent : public Component
{
	Colour getOutlineColour() const;

	bool isRoot() const;

	bool isFolded() const
	{
		return (bool)dataReference[PropertyIds::Folded];
	}

	bool isSelected() const;

	struct Factory : public PathFactory
	{
		String getId() const { return "PowerButton"; };

		Path createPath(const String& id) const override
		{
			Path p;
			auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

			LOAD_PATH_IF_URL("on", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
			LOAD_PATH_IF_URL("fold", HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon);
			LOAD_PATH_IF_URL("delete", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
			LOAD_PATH_IF_URL("move", ColumnIcons::moveIcon);

			return p;
		}
	};

	struct Header : public Component,
		public ButtonListener
	{
		Header(NodeComponent& parent_) :
			parent(parent_),
			powerButton("on", this, f),
			deleteButton("delete", this, f)
		{
			addAndMakeVisible(powerButton);
			addAndMakeVisible(deleteButton);
		}

		void buttonClicked(Button* b) override
		{

		}

		void mouseDoubleClick(const MouseEvent& event) override
		{
			parent.dataReference.setProperty(PropertyIds::Folded, !parent.isFolded(), nullptr);
			parent.getParentComponent()->repaint();
		}

		void resized() override
		{
			auto b = getLocalBounds();

			powerButton.setBounds(b.removeFromLeft(getHeight()).reduced(3));

			deleteButton.setBounds(b.removeFromRight(getHeight()).reduced(3));

			powerButton.setVisible(!parent.isRoot());
			deleteButton.setVisible(!parent.isRoot());
		}

		void mouseDown(const MouseEvent& e) override;

		void mouseUp(const MouseEvent& e) override;

		void mouseDrag(const MouseEvent& e) override;

		void paint(Graphics& g) override
		{
			g.setColour(parent.getOutlineColour());
			g.fillAll();

			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(parent.dataReference[PropertyIds::ID].toString(), getLocalBounds(), Justification::centred);
		}

		bool isDragging = false;


		Factory f;
		HiseShapeButton powerButton;
		HiseShapeButton deleteButton;

		NodeComponent& parent;

		ComponentDragger d;
	};

	NodeComponent(NodeBase* b) :
		dataReference(b->getValueTree()),
		header(*this),
		node(b)

	{
		setName(b->getId());
		addAndMakeVisible(header);
	};

	~NodeComponent()
	{
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF444444));
	}

	void paintOverChildren(Graphics& g) override
	{
		if (isSelected())
		{
			g.setColour(Colour(SIGNAL_COLOUR));
			g.drawRect(getLocalBounds(), 1);

		}
	}

	void resized() override
	{
		auto b = getLocalBounds();

		header.setBounds(b.removeFromTop(24));
	}

	ValueTree dataReference;
	Header header;
	NodeBase::Ptr node;

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeComponent);
};


struct DeactivatedComponent : public NodeComponent
{
	DeactivatedComponent(NodeBase* b):
		NodeComponent(b)
	{
		header.setEnabled(false);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colours::grey.withAlpha(0.8f));
		
	}
};

struct ContainerComponent : public NodeComponent
{
	ContainerComponent(NodeContainer* b):
	   NodeComponent(b),
		updater(*this)
	{
		rebuildNodes();
	};

	~ContainerComponent()
	{
	}

	void mouseEnter(const MouseEvent& event) override
	{
		addPosition = getInsertPosition(event.getPosition());
		repaint();
	}

	void mouseMove(const MouseEvent& event) override
	{
		addPosition = getInsertPosition(event.getPosition());
		repaint();
	}

	void mouseExit(const MouseEvent& event) override
	{
		addPosition = -1;
		repaint();
	}

	void mouseDown(const MouseEvent& event) override
	{
		if (event.mods.isRightButtonDown())
		{
			PopupLookAndFeel plaf;
			PopupMenu m;
			m.setLookAndFeel(&plaf);

			m.addSectionHeader("Add existing node");

			auto network = node->getRootNetwork();

			auto nodeList = network->getListOfUnconnectedNodes();

			for (int i = 0; i < nodeList.size(); i++)
			{
				m.addItem(i + 1, "Add " + nodeList[i]->getId());
			}

			m.addSeparator();
			m.addSectionHeader("Create new node");

			m.addItem(9000, "serial");
			m.addItem(9001, "parallel");

			auto sa = network->getListOfAllAvailableModuleIds();

			int newId = 11000;

			for (auto mId : sa)
				m.addItem(newId++, mId);

			int result = m.show();

			if (result != 0)
			{
				var newNode;

				if (result == 9000)
					newNode = network->create("serial", {});
				if (result == 9001)
					newNode = network->create("parallel", {});

				if (result >= 11000)
				{
					auto moduleId = sa[result - 11000];

					newNode =  network->create(moduleId, {});
				}

				auto container = dynamic_cast<NodeContainer*>(node.get());

				container->assign(addPosition, newNode);
			}
		}
	}

	void removeDraggedNode(NodeComponent* draggedNode)
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

	void insertDraggedNode(NodeComponent* newNode)
	{
		if (insertPosition != -1)
		{
			auto newTree = newNode->node->getValueTree();

			newTree.getParent().removeChild(newTree, node->getUndoManager());
			node->getValueTree().addChild(newTree, insertPosition, node->getUndoManager());
		}
	}

	void setDropTarget(Point<int> position)
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

	void clearDropTarget()
	{
		insertPosition = -1;
		repaint();

		for (auto n : childNodeComponents)
		{
			if (auto c = dynamic_cast<ContainerComponent*>(n))
				c->clearDropTarget();
		}
	}

	virtual int getInsertPosition(Point<int> x) const = 0;

protected:

	Point<int> getStartPosition() const;

	float getCableXOffset(int cableIndex, int factor=1) const
	{
		int numCables = node->getNumChannelsToProcess() - 1;
		int cableWidth = numCables * (SerialNode::PinHeight * factor);
		int cableStart = getWidth() / 2 - cableWidth / 2;
		int cableX = cableStart + cableIndex * (SerialNode::PinHeight * factor);

		return (float)cableX - (float)getWidth() / 2.0f;
	}

	OwnedArray<NodeComponent> childNodeComponents;

	int insertPosition = -1;

	int addPosition = -1;

private:

	struct Updater : public SafeChangeBroadcaster,
					 public SafeChangeListener,
					 public ValueTree::Listener
	{
		Updater(ContainerComponent& parent_);
		
		void changeListenerCallback(SafeChangeBroadcaster *b) override
		{
			parent.rebuildNodes();
		}

		void valueTreeChildAdded(ValueTree& parentTree, ValueTree& ) override
		{
			if(parentTree == copy)
				sendPooledChangeMessage();
		};

		void valueTreeChildOrderChanged(ValueTree& parentTree, int oldIndex, int newIndex) override
		{
			if (parentTree == copy)
				sendPooledChangeMessage();
		}

		void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
		{
			if (parentTree == copy)
				sendPooledChangeMessage();
		}

		void valueTreePropertyChanged(ValueTree& , const Identifier& id) override
		{
			if (id == PropertyIds::Folded)
				sendPooledChangeMessage();
		}

		void valueTreeParentChanged(ValueTree& ) override
		{
			
		}

		~Updater()
		{
			copy.removeListener(this);
		}

		ContainerComponent& parent;
		ValueTree copy;
		
	} updater;

	

	void rebuildNodes()
	{
		childNodeComponents.clear();

		if (auto container = dynamic_cast<NodeContainer*>(node.get()))
		{
			for (auto n : container->nodes)
			{
				auto newNode = n->createComponent();

				addAndMakeVisible(newNode);

				childNodeComponents.add(newNode);
			}
		}

		resized();
		repaint();
	}
	
};

struct SerialNodeComponent : public ContainerComponent
{
	SerialNodeComponent(SerialNode* node) :
		ContainerComponent(node)
	{

	}

	int getInsertPosition(Point<int> position) const override
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

	Rectangle<float> getInsertRuler(int position) const
	{
		int targetY = getHeight() - SerialNode::PinHeight;

		if (auto childBeforeInsert = childNodeComponents[position])
			targetY = childBeforeInsert->getY();

		targetY -= (3 * NodeBase::NodeMargin / 4);
		return Rectangle<int>(NodeBase::NodeMargin, targetY, getWidth() - 2 * NodeBase::NodeMargin, NodeBase::NodeMargin/2).toFloat();
	}

	void resized() override;

	void paintSerialCable(Graphics& g, int cableIndex)
	{
		auto xOffset = getCableXOffset(cableIndex);

		auto b2 = getLocalBounds();
		b2.removeFromTop(NodeBase::HeaderHeight);

		auto top = b2.removeFromTop(SerialNode::PinHeight);
		auto start = top.getCentre().toFloat().translated(xOffset, 0.0f);
		auto start1 = start.withY(start.getY() + SerialNode::PinHeight / 2 + NodeBase::NodeMargin);

		if (auto firstNode = childNodeComponents.getFirst())
		{
			auto fb = firstNode->getBounds().toFloat();
			start1 = { fb.getCentreX() + xOffset, fb.getY() };
		}

		auto bottom = b2.removeFromBottom(NodeBase::NodeMargin);
		bottom.removeFromBottom(SerialNode::PinHeight);

		auto end = bottom.getCentre().toFloat().translated(xOffset, 0.0f);
		auto end1 = end.withY(getHeight() - SerialNode::PinHeight - NodeBase::NodeMargin);

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



		float xPos = start.getX();

		DropShadow sh;
		sh.colour = Colours::black.withAlpha(0.5f);
		sh.offset = { 0, 2 };
		sh.radius = 3;

		for (int i = 0; i < childNodeComponents.size() - 1; i++)
		{
			auto tc = childNodeComponents[i];
			auto nc = childNodeComponents[i + 1];

			Point<int> start({ tc->getBounds().getCentreX(), tc->getBounds().getBottom() });
			Point<int> end({ nc->getBounds().getCentreX(), nc->getBounds().getY() });

			Line<float> l(start.toFloat().translated(xOffset, 0.0f), end.toFloat().translated(xOffset, 0.0f));

			p.addLineSegment(l, 2.0f);
		}

		sh.drawForPath(g, p);
		g.setColour(Colour(0xFFAAAAAA));

		g.fillPath(p);
	}

	void paint(Graphics& g) override;
};

struct ParallelNodeComponent : public ContainerComponent
{
	ParallelNodeComponent(ParallelNode* node) :
		ContainerComponent(node)
	{};

	bool isMultiChannelNode() const
	{
		return dataReference[PropertyIds::Type].toString() == PropertyIds::MultiChannelNode.toString();
	}

	int getInsertPosition(Point<int> position) const override
	{
		auto targetX = position.getX();
		auto p = childNodeComponents.size();

		for (auto nc : childNodeComponents)
		{
			if (targetX < nc->getX() + nc->getWidth()/2)
			{
				p = childNodeComponents.indexOf(nc);
				break;
			}
		}

		return p;
	}

	Rectangle<float> getInsertRuler(int position)
	{
		int targetX = getWidth();

		
		if (auto childBeforeInsert = childNodeComponents[position])
			targetX = childBeforeInsert->getX();

		targetX -= (3 * NodeBase::NodeMargin) / 4;

		if (childNodeComponents.size() == 0)
			targetX = getWidth() / 2 - NodeBase::NodeMargin/4;

		return Rectangle<int>(targetX, NodeBase::HeaderHeight, NodeBase::NodeMargin / 2, getHeight() - NodeBase::HeaderHeight).toFloat();
	}
	
	void resized() override;
	void paint(Graphics& g) override;

	void paintCable(Graphics& g, int cableIndex);
};

struct DspNodeComponent : public NodeComponent
{
	DspNodeComponent(DspNode* node);;

	void resized() override
	{
		NodeComponent::resized();

		auto b = getLocalBounds();

		b = b.reduced(NodeBase::NodeMargin);
		
		b.removeFromTop(NodeBase::HeaderHeight);

		

		for (int i = 0; i < sliders.size(); i+= 2)
		{
			auto row = b.removeFromTop(48);
			
			sliders[i]->setBounds(row.removeFromLeft(128));

			if(i < sliders.size() - 1)
				sliders[i + 1]->setBounds(row);
		}
	}
	
	OwnedArray<Slider> sliders;
};



struct DspNetworkGraph : public Component,
	public AsyncUpdater,
	public ValueTree::Listener
{
	struct Actions
	{
		static bool deselectAll(DspNetworkGraph& g)
		{
			if (g.selection.getNumSelected() == 0)
				return false;

			for (auto n : g.selection)
			{
				if (n != nullptr)
					n->repaint();
			}

			g.selection.deselectAll();
			return true;
		};

		static bool deleteSelection(DspNetworkGraph& g)
		{
			if (g.selection.getNumSelected() == 0)
				return false;

			g.root->node->getUndoManager()->beginNewTransaction();

			for (auto n : g.selection)
			{
				if (n == nullptr)
					continue;

				auto tree = n->node->getValueTree();
				tree.getParent().removeChild(tree, n->node->getUndoManager());
			}

			return true;
		}
	};

	DspNetworkGraph(DspNetwork* n) :
		network(n),
		dataReference(n->getValueTree())
	{
		dataReference.addListener(this);
		rebuildNodes();
		setWantsKeyboardFocus(true);
	}

	~DspNetworkGraph()
	{
		dataReference.removeListener(this);
	}

	void valueTreeChildAdded(ValueTree& parentTree, ValueTree& childWhichHasBeenAdded) override
	{
		triggerAsyncUpdate();
	};

	void valueTreeChildOrderChanged(ValueTree& parentTreeWhoseChildrenHaveMoved, int oldIndex, int newIndex) override
	{
		triggerAsyncUpdate();
	}

	void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override
	{
		triggerAsyncUpdate();
	}

	void valueTreePropertyChanged(ValueTree& treeWhosePropertyHasChanged, const Identifier& property) override
	{

	}

	void valueTreeParentChanged(ValueTree& treeWhoseParentHasChanged) override
	{
		triggerAsyncUpdate();
	}

	bool keyPressed(const KeyPress& key) override
	{
		if (key == KeyPress::escapeKey)
			return Actions::deselectAll(*this);
		if (key == KeyPress::deleteKey || key == KeyPress::backspaceKey)
			return Actions::deleteSelection(*this);


		return false;
	}

	struct ScrollableParent : public Component
	{
		ScrollableParent(DspNetwork* n)
		{
			addAndMakeVisible(viewport);
			viewport.setViewedComponent(new DspNetworkGraph(n), true);
		}

		void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override
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

		void resized() override
		{
			viewport.setBounds(getLocalBounds());
		}

		float zoomFactor = 1.0f;
		Viewport viewport;
	};

	void handleAsyncUpdate() override
	{
		rebuildNodes();
	}

	void rebuildNodes()
	{
		addAndMakeVisible(root = dynamic_cast<NodeComponent*>(network->signalPath->createComponent()));

		auto b = network->signalPath->getPositionInCanvas({ NodeBase::NodeMargin, NodeBase::NodeMargin });

		setSize(b.getWidth() + 2 * NodeBase::NodeMargin, b.getHeight() + 2 * NodeBase::NodeMargin);

		resized();
	}

	void updateDragging(Point<int> position)
	{
		if (auto c = dynamic_cast<ContainerComponent*>(root.get()))
		{
			c->setDropTarget({});
		}

		if (auto container = dynamic_cast<ContainerComponent*>(root->getComponentAt(position)))
		{
			currentDropTarget = container;
			DBG(container->getName());
			auto pointInContainer = container->getLocalPoint(this, position);
			container->setDropTarget(pointInContainer);
		}
	}

	void finishDrag()
	{
		if (currentDropTarget != nullptr)
		{
			currentDropTarget->insertDraggedNode(currentlyDraggedComponent);
			currentlyDraggedComponent = nullptr;
		}
	}

	void resized() override
	{
		if (root != nullptr)
		{
			root->setBounds(getLocalBounds().reduced(NodeBase::NodeMargin));
			root->setTopLeftPosition({ NodeBase::NodeMargin, NodeBase::NodeMargin });
		}
	}

	void paint(Graphics& g) override;

	void addToSelection(NodeComponent* p, ModifierKeys mods)
	{
		auto pp = dynamic_cast<NodeComponent*>(p->getParentComponent());

		while (pp != nullptr)
		{
			if (pp->isSelected())
				return;

			pp = dynamic_cast<NodeComponent*>(pp->getParentComponent());
		}

		for (auto n : selection)
		{
			if (n != nullptr)
				n->repaint();
		}
		
		selection.addToSelectionBasedOnModifiers(p, mods);

		p->repaint();
	}
	
	bool setCurrentlyDraggedComponent(NodeComponent* n)
	{
		if (auto parentContainer = dynamic_cast<ContainerComponent*>(n->getParentComponent()))
		{
			



			parentContainer->removeDraggedNode(n);

			n->setBufferedToImage(true);

			auto b = n->getLocalArea(parentContainer, n->getBounds());

			n->setBounds(b);

			addAndMakeVisible(currentlyDraggedComponent = n);

			return true;
		}

		return false;
	}

	struct Panel : public PanelWithProcessorConnection
	{
		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{

		}

		SET_PANEL_NAME("DspNetworkGraph");

		Identifier getProcessorTypeId() const override;

		Component* createContentComponent(int index) override
		{
			if (auto holder = dynamic_cast<DspNetwork::Holder*>(getConnectedProcessor()))
			{
				auto sa = holder->getIdList();

				auto id = sa[index];

				if (id.isNotEmpty())
				{
					auto network = holder->getOrCreate(id);
					return new ScrollableParent(network);
				}
			}

			return nullptr;
		}

		void fillModuleList(StringArray& moduleList) override;

		virtual bool hasSubIndex() const { return true; }

		void fillIndexList(StringArray& sa)
		{
			if (auto holder = dynamic_cast<DspNetwork::Holder*>(getConnectedProcessor()))
			{
				auto sa2 = holder->getIdList();

				sa.clear();
				sa.addArray(sa2);
			}
		}
	};

	SelectedItemSet<WeakReference<NodeComponent>> selection;

	ValueTree dataReference;

	ScopedPointer<NodeComponent> root;

	Component::SafePointer<ContainerComponent> currentDropTarget;
	ScopedPointer<NodeComponent> currentlyDraggedComponent;

	WeakReference<DspNetwork> network;
};


}
}

