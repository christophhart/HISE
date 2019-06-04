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

namespace scriptnode
{

using namespace juce;
using namespace hise;

struct MacroPropertyEditor : public Component,
							 public ValueTree::Listener
{
	struct ConnectionEditor : public Component,
							  public ButtonListener
	{
		static Array<Identifier> getHiddenIds()
		{
			return { PropertyIds::NodeId, PropertyIds::ParameterId,
					PropertyIds::UpperLimit, PropertyIds::LowerLimit,
					PropertyIds::Inverted };
		}

		ConnectionEditor(NodeBase* b, ValueTree connectionData) :
			cEditor(b, connectionData, getHiddenIds()),
			node(b),
			deleteButton("delete", this, f),
			data(connectionData)
		{
			addAndMakeVisible(cEditor);
			addAndMakeVisible(deleteButton);

			setSize(cEditor.getWidth(), cEditor.getHeight() + 24);
		}

		void buttonClicked(Button* ) override
		{
			auto dataCopy = data;
			auto undoManager = node->getUndoManager();

			auto func = [dataCopy, undoManager]()
			{
				dataCopy.getParent().removeChild(dataCopy, undoManager);
			};

			MessageManager::callAsync(func);
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Connection", getLocalBounds().removeFromTop(24).toFloat(), Justification::centred);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			b.removeFromTop(24);

			deleteButton.setBounds(b.removeFromRight(24).reduced(4));

			cEditor.setBounds(b);
		}

		NodeBase::Ptr node;
		ValueTree data;
		NodeComponent::Factory f;
		HiseShapeButton deleteButton;
		PropertyEditor cEditor;
	};

	MacroPropertyEditor(NodeBase* b, ValueTree data, Identifier childDataId=PropertyIds::Connections):
		parameterProperties(b, data),
		node(b)
	{
		connectionData = data.getChildWithName(childDataId);
		connectionData.addListener(this);

		addAndMakeVisible(parameterProperties);

		rebuildConnections();
	}

	~MacroPropertyEditor()
	{
		connectionData.removeListener(this);
	}

	void resized() override
	{
		parameterProperties.setTopLeftPosition(0, 0);
		
		int y = parameterProperties.getBottom();

		for (auto c : connectionEditors)
		{
			c->setTopLeftPosition(0, y);
			y += c->getHeight();
		}
	}

	void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override
	{
		if (parentTree == connectionData)
			rebuildConnections();
	}
	void valueTreeChildOrderChanged(ValueTree& parentTree, int , int ) override
	{
		if (parentTree == connectionData)
			rebuildConnections();
	}

	void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& , int ) override
	{
		if (parentTree == connectionData)
			rebuildConnections();
	}
	void valueTreePropertyChanged(ValueTree&, const Identifier& ) override {}
	void valueTreeParentChanged(ValueTree&) override {}

	void rebuildConnections()
	{
		connectionEditors.clear();

		int y = parameterProperties.getHeight();

		for (auto c : connectionData)
		{
			auto newEditor = new ConnectionEditor(node, c);
			addAndMakeVisible(newEditor);
			connectionEditors.add(newEditor);

			y += newEditor->getHeight();
		}

		setSize(parameterProperties.getWidth(), y);
		resized();
	}

	NodeBase::Ptr node;
	ValueTree connectionData;

	PropertyEditor parameterProperties;
	OwnedArray<ConnectionEditor> connectionEditors;
};



class ContainerComponent : public NodeComponent,
							public DragAndDropContainer
{
public:

	struct ParameterComponent : public Component,
								public ButtonListener,
								public ValueTree::Listener
	{
		struct Factory : public PathFactory
		{
			Path createPath(const String& id) const override
			{
				auto url = MarkdownLink::Helpers::getSanitizedFilename(id);
				Path p;

				LOAD_PATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
				LOAD_PATH_IF_URL("edit", ColumnIcons::customizeIcon);
				LOAD_PATH_IF_URL("drag", ColumnIcons::bigResizeIcon);

				return p;
			}

			String getId() const override { return "ParameterEditIcons"; }
		};

		ParameterComponent(ContainerComponent& parent_):
			parent(parent_),
			parameterTree(parent.dataReference.getChildWithName(PropertyIds::Parameters)),
			editButton("edit", this, f),
			dragButton("drag", this, f),
			addButton("add", this, f)
		{
			parameterTree.addListener(this);

			addAndMakeVisible(editButton);
			addAndMakeVisible(dragButton);
			addAndMakeVisible(addButton);

			dragButton.setToggleModeWithColourChange(true);

			rebuildParameters();
		}

		~ParameterComponent()
		{
			parameterTree.removeListener(this);
		}

		void buttonClicked(Button* b) override
		{
			if (b == &addButton)
			{
				auto name = PresetHandler::getCustomName("Parameter", "Enter the parameter name");

				if (name.isNotEmpty())
				{
					ValueTree p(PropertyIds::Parameter);
					p.setProperty(PropertyIds::ID, name, nullptr);
					p.setProperty(PropertyIds::MinValue, 0.0, nullptr);
					p.setProperty(PropertyIds::MaxValue, 1.0, nullptr);
					p.setProperty(PropertyIds::Value, 1.0, nullptr);
					parameterTree.addChild(p, -1, parent.node->getUndoManager());
				}
			}
			if (b == &dragButton)
			{
				for (auto s : sliders)
					s->setEditEnabled(b->getToggleState());
			}
		}

		void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override
		{
			if (parentTree.getType() == PropertyIds::Connections)
				return;

			rebuildParameters();
		}
		void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override
		{
			if (parentTree.getType() == PropertyIds::Connections)
				return;

			rebuildParameters();
		}

		void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override
		{
			if (parentTree.getType() == PropertyIds::Connections)
				return;

			rebuildParameters();
		}
		void valueTreePropertyChanged(ValueTree&, const Identifier&) override {}
		void valueTreeParentChanged(ValueTree&) override {}
		
		void rebuildParameters()
		{
			sliders.clear();

			for (int i = 0; i < parent.node->getNumParameters(); i++)
			{
				auto newSlider = new MacroParameterSlider(parent.node, i);
				addAndMakeVisible(newSlider);
				sliders.add(newSlider);
			}

			resized();
		}

		void resized() override
		{
			auto b = getLocalBounds();

			auto bRow = b.removeFromLeft(b.getHeight() / 3);

			addButton.setBounds(bRow.removeFromTop(bRow.getWidth()).reduced(3));
			editButton.setBounds(bRow.removeFromTop(bRow.getWidth()).reduced(3));
			dragButton.setBounds(bRow.removeFromTop(bRow.getWidth()).reduced(3));

			for (auto s : sliders)
			{
				s->setBounds(b.removeFromLeft(100));
			}
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colours::black.withAlpha(0.1f));

			g.setColour(Colours::white);
			g.drawText(String(sliders.size()), getLocalBounds().toFloat(), Justification::centred);

		}

		

		ContainerComponent& parent;
		ValueTree parameterTree;
		Factory f;
		HiseShapeButton editButton;
		HiseShapeButton dragButton;
		HiseShapeButton addButton;
		OwnedArray<MacroParameterSlider> sliders;
	};

	ContainerComponent(NodeContainer* b);;
	~ContainerComponent();

	void mouseEnter(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;

	virtual int getInsertPosition(Point<int> x) const = 0;
	void removeDraggedNode(NodeComponent* draggedNode);
	void insertDraggedNode(NodeComponent* newNode, bool copyNode);

	void setDropTarget(Point<int> position);
	void clearDropTarget();

	void resized() override
	{
		NodeComponent::resized();

		if (dataReference[PropertyIds::ShowParameters])
		{
			parameters.setVisible(true);
		}
		else
			parameters.setVisible(false);

		auto b = getLocalBounds();
		b.expand(-UIValues::NodeMargin, 0);
		b.removeFromTop(UIValues::HeaderHeight);
		parameters.setBounds(b.removeFromTop(UIValues::ParameterHeight));
	}

protected:

	Point<int> getStartPosition() const;
	float getCableXOffset(int cableIndex, int factor = 1) const;

	OwnedArray<NodeComponent> childNodeComponents;
	int insertPosition = -1;
	int addPosition = -1;

private:

	struct Updater : public SafeChangeBroadcaster,
		public SafeChangeListener,
		public ValueTree::Listener
	{
		Updater(ContainerComponent& parent_);

		~Updater();

		void changeListenerCallback(SafeChangeBroadcaster *b) override;
		void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override;;
		void valueTreeChildOrderChanged(ValueTree& parentTree, int oldIndex, int newIndex) override;
		void valueTreeChildRemoved(ValueTree& parentTree, ValueTree& childWhichHasBeenRemoved, int indexFromWhichChildWasRemoved) override;
		void valueTreePropertyChanged(ValueTree&, const Identifier& id) override;
		void valueTreeParentChanged(ValueTree&) override;

		ContainerComponent& parent;
		ValueTree copy;

	} updater;

	void rebuildNodes();

	ParameterComponent parameters;

};

struct SerialNodeComponent : public ContainerComponent
{
	SerialNodeComponent(SerialNode* node);

	int getInsertPosition(Point<int> position) const override;
	Rectangle<float> getInsertRuler(int position) const;

	void resized() override;
	void paintSerialCable(Graphics& g, int cableIndex);
	void paint(Graphics& g) override;
};

struct ParallelNodeComponent : public ContainerComponent
{
	ParallelNodeComponent(ParallelNode* node);;

	bool isMultiChannelNode() const;
	int getInsertPosition(Point<int> position) const override;
	Rectangle<float> getInsertRuler(int position);

	void resized() override;
	void paint(Graphics& g) override;
	void paintCable(Graphics& g, int cableIndex);
};

struct ModChainNodeComponent : public ContainerComponent
{
	ModChainNodeComponent(ModulationChainNode* node);

	bool isMultiChannelNode() const { return false; }

	int getInsertPosition(Point<int> position) const override;
	Rectangle<float> getInsertRuler(int position);

	void resized() override;
	void paint(Graphics& g) override;
	//void paintCable(Graphics& g, int cableIndex);

	ModulationSourceBaseComponent dragger;

};

}
