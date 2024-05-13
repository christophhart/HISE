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
							 public TextEditor::Listener,
							 public ButtonListener
{
	struct ConnectionEditor : public Component,
							  public ButtonListener
	{
		

		ConnectionEditor(NodeBase* b, ValueTree connectionData, bool showSourceInTitle) :
			cEditor(b, true, connectionData, RangeHelpers::getHiddenIds()),
			node(b),
			deleteButton("delete", this, f),
			gotoButton("goto", this, f),
			data(connectionData),
			showSource(showSourceInTitle)
		{
			if(auto targetNode = b->getRootNetwork()->getNodeWithId(connectionData[PropertyIds::NodeId].toString()))
				c = PropertyHelpers::getColour(targetNode->getValueTree());

			addAndMakeVisible(cEditor);
			addAndMakeVisible(deleteButton);
			addAndMakeVisible(gotoButton);

			updateSize();
		}

		void updateSize()
		{
			cEditor.updateSize();
			setSize(cEditor.getWidth(), cEditor.getHeight() + 24);
		}

		void enableProperties(Identifier, var newValue)
		{
			bool shouldBeEnabled = newValue.toString().isEmpty();

			auto rangeIds = RangeHelpers::getRangeIds();
			cEditor.enableProperties(rangeIds, shouldBeEnabled);

			findParentComponentOfClass<MacroPropertyEditor>()->resizeConnections();

		}

		void buttonClicked(Button* b) override;

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.05f));
			g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);

			String text = getPathFromNode(showSource, data);
			Font font = GLOBAL_MONOSPACE_FONT().withHeight(15.0f);

			auto title = getLocalBounds().removeFromTop(24).toFloat();

			if (!c.isTransparent())
			{
				auto w = font.getStringWidthFloat(text) + 15.0f;

				auto fill = title.reduced(3.0);
				fill.removeFromTop(1.0f);
				fill.removeFromBottom(1.0f);

				auto left = (title.getWidth() - w) / 2.0f;
				
				auto leftFill = fill.removeFromLeft((float)left);
				auto rightFill = fill.removeFromRight((float)left);

				leftFill.removeFromLeft(20.0f);

				g.setColour(c);
				g.fillRoundedRectangle(leftFill, 3.0f);
				g.fillRoundedRectangle(rightFill, 3.0f);
			}

			g.setColour(Colours::white);
			g.setFont(font);
			
			g.fillPath(icon);

			g.drawText(text, title, Justification::centred);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			auto top = b.removeFromTop(24);

			deleteButton.setBounds(top.removeFromRight(24).reduced(4));

			gotoButton.setBounds(2, 2, 16, 16);

			cEditor.setBounds(b);
		}

		Path icon;

		Colour c;

		NodeBase::Ptr node;
		ValueTree data;
		NodeComponent::Factory f;
		HiseShapeButton deleteButton;
		HiseShapeButton gotoButton;
		PropertyEditor cEditor;
		bool showSource = false;

		valuetree::PropertyListener expressionEnabler;
	};

	

	struct Content: public Component
	{
		Content(MacroPropertyEditor& parent_) :
			parent(parent_),
			searchBar("Search")
		{
			addAndMakeVisible(searchBar);
			searchBar.addListener(&parent);
			searchBar.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.2f));
			searchBar.setFont(GLOBAL_FONT());
			searchBar.setSelectAllWhenFocused(true);
			searchBar.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
		};

		~Content()
		{
			searchBar.removeListener(&parent);
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT().withHeight(15.0f));

			auto b = getLocalBounds();
			b.removeFromTop(5);

			auto titleArea = b.removeFromTop(30).toFloat();
			titleArea.removeFromLeft(16);
			g.setColour(Colours::white);
			g.drawText("Connections", titleArea, Justification::centred);

			g.setColour(Colours::white.withAlpha(0.6f));

			static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
				103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
				191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
				218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
				95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

			Path path;
			path.loadPathFromData(searchIcon, sizeof(searchIcon));
			path.applyTransform(AffineTransform::rotation(float_Pi));

			path.scaleToFit(4.0f, 44.0f, 16.0f, 16.0f, true);

			g.fillPath(path);
		}
		
		void resized() override
		{
			auto b = getLocalBounds();

			b.removeFromTop(40);
			auto top = b.removeFromTop(24);
			top.removeFromLeft(24);

			searchBar.setBounds(top);
		}

		TextEditor searchBar;

		MacroPropertyEditor& parent;
	};

	MacroPropertyEditor(NodeBase* b, ValueTree data, Identifier childDataId = PropertyIds::Connections) :
		parameterProperties(b, false, data),
		node(b),
		connectionContent(*this),
		containerMode(dynamic_cast<NodeContainer*>(b) != nullptr || childDataId == PropertyIds::ModulationTargets),
		resizer(this, &constrainer),
		connectionButton("Add connection")
	{
		if (containerMode)
		{
			connectionData = data.getChildWithName(childDataId);

			connectionListener.setCallback(connectionData, valuetree::AsyncMode::Asynchronously,
				[this](ValueTree v, bool wasAdded)
			{ 
				if (!wasAdded)
					connectionArray.removeAllInstancesOf(v);

				this->rebuildConnections();
			});

			for (auto c : connectionData)
				connectionArray.add(c);
		}
		else
		{
			for (auto p: NodeBase::ParameterIterator(*b))
			{
				if (parameter != nullptr)
					break;

				if (p->data == data)
				{
					parameter = p;

					auto list = p->getConnectedMacroParameters();

					for (auto mp : list)
					{
						for (auto c : dynamic_cast<NodeContainer::MacroParameter*>(mp)->getConnectionTree())
						{
							if (p->matchesConnection(c))
								connectionArray.add(c);
						}
					}
				}
			}
		}

		addAndMakeVisible(parameterProperties);

		addAndMakeVisible(connectionViewport);
		connectionViewport.setViewedComponent(&connectionContent, false);

		int height = jmin(700, connectionArray.isEmpty() ? 10 : (100 + (connectionArray.size() * 110)));

		if (!containerMode)
		{
			height += 32;
			addAndMakeVisible(connectionButton);
			connectionButton.setLookAndFeel(&blaf);
			connectionButton.addListener(this);
		}

		setSize(parameterProperties.getWidth() + connectionViewport.getScrollBarThickness(), parameterProperties.getHeight() + height);

		constrainer.setMaximumWidth(getWidth());
		constrainer.setMinimumWidth(getWidth());

		addAndMakeVisible(resizer);

		rebuildConnections();
	}

	void buttonClicked(Button* b) override;

	~MacroPropertyEditor()
	{
		
	}

	void paint(Graphics& ) override
	{
	}

	void resized() override
	{
		connectionViewport.setVisible(!connectionArray.isEmpty());
		resizer.setVisible(connectionArray.size() > 2);

		parameterProperties.setTopLeftPosition(0, 0);
		int y = parameterProperties.getBottom();

		auto b = getLocalBounds();
		b.removeFromTop(y);

		if(!containerMode)
			connectionButton.setBounds(b.removeFromBottom(32));

		connectionViewport.setBounds(b);

		auto s = connectionViewport.getScrollBarThickness();
		resizer.setBounds(getLocalBounds().removeFromRight(s).removeFromBottom(s));
	}

	static String getPathFromNode(bool showSource, ValueTree& data)
	{
		String text;

		if (showSource)
		{
			text << data.getParent().getParent().getParent().getParent()[PropertyIds::ID].toString() << ".";
			text << data.getParent().getParent()[PropertyIds::ID].toString();
		}
		else
		{
			text << data[PropertyIds::NodeId].toString() << ".";
			text << data[PropertyIds::ParameterId].toString();
		}

		return text;
	}

	void textEditorTextChanged(TextEditor& te) override
	{
		searchTerm = te.getText().toLowerCase();
		rebuildConnections();
	}

	void resizeConnections()
	{
		int y = 84;

		for (auto c : connectionEditors)
		{
			c->updateSize();
			y += c->getHeight() + 10;
		}

		connectionContent.setSize(parameterProperties.getWidth(), y);

		y = 84;

		for (auto c : connectionEditors)
		{
			c->setTopLeftPosition(0, y);
			y += c->getHeight() + 10;
		}
	}

	void rebuildConnections()
	{
		connectionEditors.clear();

		for (auto c : connectionArray)
		{
			if (searchTerm.isNotEmpty() && !getPathFromNode(!containerMode, c).toLowerCase().contains(searchTerm))
			{
				continue;
			}

			auto newEditor = new ConnectionEditor(node, c, !containerMode);
			connectionContent.addAndMakeVisible(newEditor);
			connectionEditors.add(newEditor);
		}

		resizeConnections();

		resized();
	}

	String searchTerm;
	
	bool containerMode = false;

	NodeBase::Parameter* parameter = nullptr;
	NodeBase::Ptr node;
	ValueTree connectionData;
	Array<ValueTree> connectionArray;

	valuetree::ChildListener connectionListener;

	PropertyEditor parameterProperties;
	OwnedArray<ConnectionEditor> connectionEditors;

	Viewport connectionViewport;
	Content connectionContent;

	ComponentBoundsConstrainer constrainer;
	juce::ResizableCornerComponent resizer;

	TextButton connectionButton;
	hise::BlackTextButtonLookAndFeel blaf;
};


struct NodeDropTarget
{
	virtual ~NodeDropTarget() {};

	Component* asComponent() { return dynamic_cast<Component*>(this); }

	virtual void setDropTarget(Point<int> position) = 0;
	virtual void removeDraggedNode(NodeComponent* draggedNode) = 0;
	virtual void insertDraggedNode(NodeComponent* newNode, bool copyNode) = 0;
	virtual void clearDropTarget() = 0;

private:

	JUCE_DECLARE_WEAK_REFERENCEABLE(NodeDropTarget);
};

class ContainerComponent :  public NodeComponent,
							public NodeDropTarget,
							public DragAndDropContainer,
							public HelpManager::Listener,
							public Value::Listener,
                            public PooledUIUpdater::SimpleTimer,
						    public LassoSource<NodeBase::Ptr>
{
public:

    enum class CableLocation
    {
        Input,
        BetweenNodes,
        Output,
        numCableLocations
    };
    
	struct MacroToolbar : public ComponentWithMiddleMouseDrag,
						  public ButtonListener
	{
		MacroToolbar() :
			dragButton("drag", this, f),
			addButton("add", this, f)
		{
			addAndMakeVisible(dragButton);

			dragButton.setTooltip("Enable drag mode to draw connections between the parameters");
			addButton.setTooltip("Create a new parameter");

			addAndMakeVisible(addButton);
			dragButton.setToggleModeWithColourChange(true);
			setSize(32, 40);
		};

		void resized() override
		{
			auto b = getLocalBounds();
			auto bRow = b.removeFromLeft(b.getHeight() / 3);

			addButton.setBounds(bRow.removeFromTop(bRow.getWidth()).reduced(3));
			dragButton.setBounds(bRow.removeFromTop(bRow.getWidth()).reduced(3));
		}

		bool fixed = false;

		void setFixedParameter(bool shouldBeFixed)
		{
			fixed = shouldBeFixed;

			addButton.setVisible(!fixed);
			dragButton.setVisible(!fixed);
		}

		void buttonClicked(Button* b) override
		{
			auto pc = findParentComponentOfClass<ParameterComponent>();

			if (b == &addButton)
			{
				auto name = PresetHandler::getCustomName("Parameter", "Enter the parameter name");

				while (name.isNotEmpty() && pc->parent.node->getParameterFromName(name) != nullptr)
				{
					PresetHandler::showMessageWindow("Already there", "The parameter " + name + " already exists. You need to be more creative.");

					name = PresetHandler::getCustomName("Parameter", "Enter a new parameter name");
				}

                pc->parent.node->getParameter(name);
			}
			if (b == &dragButton)
			{
				for (auto s : pc->sliders)
					dynamic_cast<MacroParameterSlider*>(s)->setEditEnabled(b->getToggleState());
			}
		}

		struct Factory : public PathFactory
		{
			Path createPath(const String& id) const override
			{
				auto url = MarkdownLink::Helpers::getSanitizedFilename(id);
				Path p;

				LOAD_EPATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
				LOAD_PATH_IF_URL("drag", ColumnIcons::targetIcon);

				return p;
			}

			String getId() const override { return "ParameterEditIcons"; }
		};


		Factory f;
		HiseShapeButton dragButton;
		HiseShapeButton addButton;
	};
	
	struct ParameterComponent : public ComponentWithMiddleMouseDrag,
								public ValueTree::Listener,
								public AsyncUpdater
	{
		
		ParameterComponent(ContainerComponent& parent_):
			parent(parent_),
			parameterTree(parent.dataReference.getChildWithName(PropertyIds::Parameters))
		{
			parameterTree.addListener(this);

			if ((leftTabComponent = dynamic_cast<NodeContainer*>(parent.node.get())->createLeftTabComponent()))
				addAndMakeVisible(leftTabComponent);

			if(auto mt = dynamic_cast<ContainerComponent::MacroToolbar*>(leftTabComponent.get()))
				mt->setFixedParameter(isFixedParameterComponent());
			
			setSize(500, UIValues::ParameterHeight);
			rebuildParameters();
		}

		~ParameterComponent()
		{
			parameterTree.removeListener(this);
		}

		bool isFixedParameterComponent() const
		{
			return dynamic_cast<NodeContainer*>(parent.node.get())->hasFixedParameters();
		}

		void handleAsyncUpdate() override
		{
			rebuildParameters();
		}

		void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override
		{
			if (isFixedParameterComponent())
				return;

			if (parentTree.getType() == PropertyIds::Connections)
				return;

			triggerAsyncUpdate();
		}
		void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override
		{
			if (isFixedParameterComponent())
				return;

			if (parentTree.getType() == PropertyIds::Connections)
				return;

			triggerAsyncUpdate();
		}

		void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override
		{
			if (isFixedParameterComponent())
				return;

			if (parentTree.getType() == PropertyIds::Connections)
				return;

			triggerAsyncUpdate();
		}
		void valueTreePropertyChanged(ValueTree&, const Identifier&) override {}
		void valueTreeParentChanged(ValueTree&) override {}
		
		void rebuildParameters()
		{
			sliders.clear();

			for (int i = 0; i < parent.node->getNumParameters(); i++)
			{
				Component* newSlider;

				if (isFixedParameterComponent())
					newSlider = new ParameterSlider(parent.node.get(), i);
				else
					newSlider = new MacroParameterSlider(parent.node.get(), i);

				addAndMakeVisible(newSlider);
				sliders.add(newSlider);
			}

			resized();
		}

		void resized() override;

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();
			
			b.removeFromTop(10.0f);

			g.setColour(Colours::black.withAlpha(0.1f));

			g.fillRoundedRectangle(b, 10.0f);
		}

		ContainerComponent& parent;
		ValueTree parameterTree;
		ScopedPointer<Component> leftTabComponent;
		OwnedArray<Component> sliders;
	};

	ContainerComponent(NodeContainer* b);;
	~ContainerComponent();

	void mouseEnter(const MouseEvent& event) override;
	void mouseMove(const MouseEvent& event) override;
	void mouseExit(const MouseEvent& event) override;
	void mouseDown(const MouseEvent& event) override;
	void mouseDrag(const MouseEvent& event) override;
	void mouseUp(const MouseEvent& e) override;

    void timerCallback() override { repaint(); }

    float getCircleAmp(int nodeIndex, int channelIndex, bool post)
    {
        if(!node->getRootNetwork()->isSignalDisplayEnabled())
            return 0.0f;
        
        NodeBase* n;
        
        if(nodeIndex == -1)
            n = node.get();
        else
        {
            if(auto nc = childNodeComponents[nodeIndex])
                n = nc->node.get();
            else
                return 0.0f;
        }
            
        
        auto peak = jlimit(0.0f, 1.0f, n->getSignalPeak(channelIndex, post));
        
        if(peak >= 0.001f)
            peak = 0.25f + 0.75f * peak;
        
        return getMaxCircleRadius() * peak;
    }
    
    virtual float getMaxCircleRadius() const = 0;
    
	virtual int getInsertPosition(Point<int> x) const = 0;
	void removeDraggedNode(NodeComponent* draggedNode);
	void insertDraggedNode(NodeComponent* newNode, bool copyNode);

	void helpChanged(float newWidth, float newHeight) override;

	void repaintHelp() override { repaint(); }

	void drawHelp(Graphics& g)
	{
		for (auto nc : childNodeComponents)
		{
			auto helpBounds = nc->node->getHelpManager().getHelpSize();

			if (!helpBounds.isEmpty())
			{
				helpBounds.setPosition(nc->getBounds().toFloat().getTopRight());
				nc->node->getHelpManager().render(g, helpBounds);
			}
		}
	}

    bool shouldPaintCable(CableLocation l) const;
    
	void setDropTarget(Point<int> position) override;
	void clearDropTarget();

	void valueChanged(Value& v) override;

	virtual Rectangle<float> getInsertRuler(int ) const { jassertfalse; return {}; }

	void resized() override
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
	}

	int getCurrentAddPosition() const { return addPosition; }

	void setExtraComponent(Component* newExtraComponent)
	{
		extraComponent = newExtraComponent;
		addAndMakeVisible(extraComponent);
		parameters = nullptr;
		resized();
	}

	void findLassoItemsInArea(Array<NodeBase::Ptr>& itemsFound,
		const Rectangle<int>& area) override;

	SelectedItemSet<NodeBase::Ptr>& getLassoSelection() override
	{
		return node->getRootNetwork()->getRawSelection();
	}

protected:

	Value verticalValue;

	Point<int> getStartPosition() const;
	float getCableXOffset(int cableIndex, int factor = 1) const;

	OwnedArray<NodeComponent> childNodeComponents;
	int insertPosition = -1;
	int addPosition = -1;

	ScopedPointer<Component> duplicateDisplay;

    float signalDotOffset = 0.0f;
    
private:

	struct Updater : public SafeChangeBroadcaster,
		public SafeChangeListener,
		public ValueTree::Listener
	{
		enum class Level
		{
			Nothing,
			Repaint,
			Rebuild
		};

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

		Level messageLevel = Level::Nothing;
	} updater;

	void rebuildNodes();

	ScopedPointer<ParameterComponent> parameters;
	ScopedPointer<Component> extraComponent;

	LassoComponent<NodeBase::Ptr> lasso;
	
    
};

struct SerialNodeComponent : public ContainerComponent
{
	SerialNodeComponent(NodeContainer* node);

	int getInsertPosition(Point<int> position) const override;
	Rectangle<float> getInsertRuler(int position) const override;

    float getMaxCircleRadius() const override { return 4.0f; }
    
	Colour getOutlineColour() const override;

	void resized() override;
	void paintSerialCable(Graphics& g, int cableIndex);
	void paint(Graphics& g) override;
};

struct ParallelNodeComponent : public ContainerComponent
{
	ParallelNodeComponent(NodeContainer* node);;

    float getMaxCircleRadius() const override { return 6.0f; }
    
	bool isMultiChannelNode() const;
	int getInsertPosition(Point<int> position) const override;
	Rectangle<float> getInsertRuler(int position) const override;

	juce::Colour getOutlineColour() const override;

	void resized() override;
	void paint(Graphics& g) override;
	void paintCable(Graphics& g, int cableIndex);
};


}
