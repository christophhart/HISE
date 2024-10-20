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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#ifndef APIBROWSER_H_INCLUDED
#define APIBROWSER_H_INCLUDED

namespace hise { using namespace juce;


class ExtendedApiDocumentation
{
public:

	static void init();

	static String getMarkdownText(const Identifier& className, const Identifier& methodName);

private:

	class DocumentationBase
	{
	protected:

		DocumentationBase(const Identifier& id_) :
			id(id_)
		{};

		String description;

		Identifier id;

	public:

		virtual ~DocumentationBase() {};
		virtual String createMarkdownText() const = 0;

		void addDescriptionLine(const String& line)
		{
			description << line << "\n";
		}

		void setDescription(const String& description_)
		{
			description = description_;
		}
	};

	class MethodDocumentation : public DocumentationBase
	{
	public:

		MethodDocumentation(Identifier& className_, const Identifier& id);

		struct Parameter
		{
			bool operator == (const Parameter& other) const
			{
				return id == other.id;
			}

			String id;
			String type;
			String description;
		};

		

		String createMarkdownText() const override;

		using VarArray = Array<var>;

		using Object = DynamicObject;

		template <typename T> void addParameter(const String& id, const String& description)
		{
			parameters.add({ id, getTypeName<T>(), description });
		}

		void setCodeExample(const String& code)
		{
			codeExample = code;
		}

		void addCodeLine(const String& line)
		{
			codeExample << line << "\n";
		}

		template <typename T> void addReturnType(const String& returnDescription)
		{
			returnType.type = getTypeName<T>();
			returnType.description = returnDescription;
		}

	private:

		friend class ExtendedApiDocumentation;

		template <typename T> String getTypeName() const
		{
			String typeName;

			if      (typeid(T) == typeid(String))	typeName = "String";
			else if (typeid(T) == typeid(int))		typeName = "int";
			else if (typeid(T) == typeid(double))	typeName = "double";
			else if (typeid(T) == typeid(VarArray))	typeName = "Array";
			else if (typeid(T) == typeid(Object))	typeName = "Object";
			else									typeName = "Unknown";

			return typeName;
		}

		String className;
		String codeExample;

		Array<Parameter> parameters;
		Parameter returnType;

	};

	class ClassDocumentation : public DocumentationBase
	{
	public:

		ClassDocumentation(const Identifier& className);

		MethodDocumentation* addMethod(const Identifier& methodName);

		String createMarkdownText() const override;

		void addSubClass(Identifier subClassId)
		{
			subClassIds.add(subClassId);
		}

	private:

		Array<Identifier> subClassIds;

		friend class ExtendedApiDocumentation;

		Array<MethodDocumentation> methods;
	};

	static ClassDocumentation* addClass(const Identifier& name);

	

	

	static bool inititalised;
	static Array<ClassDocumentation> classes;
};






class ApiCollection : public SearchableListComponent
{
public:

	ApiCollection(BackendRootWindow *window);

	SET_GENERIC_PANEL_ID("ApiCollection");

	class MethodItem : public SearchableListComponent::Item,
					   public ComponentWithDocumentation
	{
	public:

		const int extendedWidth = 500;

		MethodItem(const ValueTree &methodTree_, const String &className_);

		int getPopupHeight() const override
		{ 
			if (parser != nullptr) 
				return (int)parser->getHeightForWidth((float)extendedWidth);
			else 
				return 150; 
		}

		int getPopupWidth() const override
		{
			if (parser != nullptr)
				return extendedWidth + 20;
			else
				return Item::getPopupWidth();
		}

		void focusGained(FocusChangeType ft) override
		{
			Item::focusGained(ft);
			repaint();
		}

		MarkdownLink getLink() const override
		{
			String s = "scripting/scripting-api/";
			s << className << "#" << name << "/";
			return { File(), s };
		}

		void focusLost(FocusChangeType ft) override
		{
			Item::focusLost(ft);
			repaint();
		}

		void paintPopupBox(Graphics &g) const
		{
			if (parser != nullptr)
			{
				auto bounds = Rectangle<float>(10.0f, -8.0f, (float)extendedWidth, (float)getPopupHeight());
				parser->draw(g, bounds);
			}
			else
			{
				auto bounds = Rectangle<float>(10.0f, -8.0f, 280.0f, (float)getPopupHeight());
				help.draw(g, bounds);
			}

			
		}

		melatonin::DropShadow sh;

		void mouseEnter(const MouseEvent&) override
		{
			repaint();
		}
		void mouseExit(const MouseEvent&) override { repaint(); }
		void mouseDoubleClick(const MouseEvent&) override;

		bool keyPressed(const KeyPress& key) override;

		void paint(Graphics& g) override;

	private:

		void insertIntoCodeEditor();

		AttributedString help;

		String name;
		String description;
		String className;
		String arguments;

		ScopedPointer<MarkdownRenderer> parser;

		const ValueTree methodTree;
	};

	void onPopupClose(FocusChangeType ft) override
	{
		if(auto ed = getRootWindow()->getMainSynthChain()->getMainController()->getLastActiveEditor())
		{
			if(ft == FocusChangeType::focusChangedByMouseClick)
				ed->grabKeyboardFocusAsync();
		}
	}

	class ClassCollection : public SearchableListComponent::Collection
	{
	public:
		ClassCollection(int index, const ValueTree &api);;

		void paint(Graphics &g) override;
	private:

		String getSearchTermForCollection() const override { return name.toLowerCase(); }

		String name;

		const ValueTree classApi;
	};

	int getNumCollectionsToCreate() const override { return apiTree.getNumChildren(); }

	Collection *createCollection(int index) override
	{
		return new ClassCollection(index, apiTree.getChild(index));
	}

	ValueTree apiTree;

private:

	BaseDebugArea *parentArea;

};

} // namespace hise



namespace scriptnode
{
using namespace juce;
using namespace hise;

class DspNodeList : public SearchableListComponent,
	public DspNetwork::SelectionListener
{
public:

	struct AddParameterItem: public SearchableListComponent::Item
	{
		AddParameterItem(DspNetwork* n):
		  Item("addParameter"),
		  network(n)
		{
			setRepaintsOnMouseActivity(true);
		}

		void mouseDown(const MouseEvent& e) override
		{
			auto n = PresetHandler::getCustomName("Parameter", "Enter the name for the parameter");

			if(!n.isEmpty())
			{
				auto parameters = network->getRootNode()->getParameterTree();

				for(auto p: parameters)
				{
					if(p[PropertyIds::ID].toString() == n)
					{
						PresetHandler::showMessageWindow("Parameter already exists", "The parameter already exists", PresetHandler::IconType::Error);
						return;
					}
				}

				ValueTree p(PropertyIds::Parameter);

				InvertableParameterRange rng;
				RangeHelpers::storeDoubleRange(p, rng, nullptr);

				p.setProperty(PropertyIds::ID, n, nullptr);
				p.setProperty(PropertyIds::Value, 0.0, nullptr);

				parameters.addChild(p, -1, network->getUndoManager());
			}
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.03f));
			g.fillRoundedRectangle(getLocalBounds().toFloat(), 3.0f);

			g.setFont(GLOBAL_BOLD_FONT());
			float alpha = 0.2f;

			if(isMouseOver())
				alpha += 0.4f;

			g.setColour(Colours::white.withAlpha(alpha));
			g.drawText("Add parameter", getLocalBounds().toFloat(), Justification::centred);
		}

		WeakReference<DspNetwork> network;
	};

	struct ParameterItem: public SearchableListComponent::Item,
						  public PathFactory,
						  public PooledUIUpdater::SimpleTimer
	{
		ParameterItem(DspNetwork* parent, int parameterIndex);

		void timerCallback();

		struct SliderLookAndFeel: public LookAndFeel_V4
		{
			void drawLinearSlider(Graphics& g, int x, int y, int width, int height, float sliderPos, float minSliderPos, float maxSliderPos, const Slider::SliderStyle, Slider& s) override
			{
				float alpha = 0.4f;

				if(s.isMouseButtonDown(true))
					alpha += 0.3f;

				if(s.isMouseOverOrDragging(true))
					alpha += 0.2f;
				
				g.setColour(Colours::white.withAlpha(alpha));

				auto b = s.getLocalBounds().toFloat();

				g.drawRoundedRectangle(b.reduced(1.0f), b.getHeight() * 0.5f, 1.0f);

				auto rng = s.getRange();
				auto skew = s.getSkewFactor();
				NormalisableRange<double> nr(rng.getStart(), rng.getEnd(), 0.0, skew);

				auto nv = nr.convertTo0to1(s.getValue());

				b = b.reduced(3.0f);

				b = b.removeFromLeft(jmax<float>(b.getHeight(), (float)nv * b.getWidth()));

				g.fillRoundedRectangle(b, b.getHeight() * 0.5f);
			}
		} laf;

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.03f));
			g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 3.0f);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			dragButton.setBounds(b.removeFromRight(b.getHeight()).reduced(1));
			valueSlider.setBounds(b.removeFromRight(64).reduced(3, 5));
			pname.setBounds(b);
		}

		Path createPath(const String& url) const override
		{
			Path p;
			LOAD_PATH_IF_URL("drag", ColumnIcons::targetIcon);
			return p;
		}

		void updateRange(const Identifier& id, const var& newValue)
		{
			auto rng = RangeHelpers::getDoubleRange(ptree);

			valueSlider.setRange(rng.rng.getRange(), rng.rng.interval);
			valueSlider.setSkewFactor(rng.rng.skew);
		}

		valuetree::PropertyListener rangeUpdater;

		ValueTree ptree;

		int pIndex;
		Slider valueSlider;
		HiseShapeButton dragButton;
		Label pname;
		
		DspNetworkGraph* ng;
		DspNetwork* network;

		DspNetworkListeners::MacroParameterDragListener dragListener;
		
	};

	struct CableItem: public SearchableListComponent::Item
	{
		CableItem(DspNetwork* parent, const String& id_):
		  Item(id_)
		{
			addAndMakeVisible(content = parent->createLocalCableListItem(id_));
		}

		void resized() override
		{
			content->setBounds(getLocalBounds());
		}


		ScopedPointer<Component> content;
	};

	struct NodeItem : public SearchableListComponent::Item,
		public ButtonListener,
		public Label::Listener
	{
		
		NodeItem(DspNetwork* parent, const String& id);

        Path icon;

		void updateBypassState(Identifier id, var newValue);

		void updateId(Identifier id, var newValue)
		{
			label.setText(newValue.toString(), dontSendNotification);
		}

		void labelTextChanged(Label*) override
		{
			if (node != nullptr)
				node->setValueTreeProperty(PropertyIds::Name, label.getText());
		}

		void buttonClicked(Button* b) override
		{
			if (node != nullptr)
				node->setValueTreeProperty(PropertyIds::Bypassed, !b->getToggleState());
		}

        int getIntendation() const;

		void paint(Graphics& g) override;
		

		void resized() override;

		void mouseUp(const MouseEvent& event) override
		{
			if(!isEnabled())
				return;

            if(event.mods.isShiftDown())
                label.showEditor();
			else if (node != nullptr)
            {
				node->getRootNetwork()->addToSelection(node, event.mods);
                
                node->getRootNetwork()->zoomToSelection(this);
            }
		}

		valuetree::PropertyListener idListener;
		valuetree::PropertyListener bypassListener;

		NodeBase::Ptr node;
		NodeComponentFactory f;
		NiceLabel label;
		HiseShapeButton powerButton;
		HiseShapeButton dragButton;

		bool currentlyVisible = true;

        Rectangle<int> area;

		ScopedPointer<DspNetworkListeners::MacroParameterDragListener> dragListener;
	};

	struct NodeCollection : public SearchableListComponent::Collection
	{
		NodeCollection(DspNetwork* network_, int idx) :
		    Collection(idx),
			network(network_)
		{};

		void paint(Graphics& g) override;

		void addItems(const StringArray& idList, bool isCable);

	protected:

		WeakReference<DspNetwork> network;
	};

	struct ModuleItem : public SearchableListComponent::Item
	{
		ModuleItem(const String& path) :
			Item(path)
		{};

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawRect(getLocalBounds().reduced(1));
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white);

			auto b = getLocalBounds().toFloat().reduced(1);
			b.removeFromLeft(b.getHeight());

			g.drawText(searchKeywords, b, Justification::centredLeft);
		}
	};

	struct Parameters: public NodeCollection
	{
		Parameters(DspNetwork* network):
		  NodeCollection(network, 0),
		  parameterTree(network->getRootNode()->getParameterTree())
		{
			setName("Parameters");

			auto numParameters = parameterTree.getNumChildren();

			for(int i = 0; i < numParameters; i++)
			{
				auto ni = new ParameterItem(network, i);
				items.add(ni);
				addAndMakeVisible(ni);
			}

			auto ni = new AddParameterItem(network);
			items.add(ni);
			addAndMakeVisible(ni);

			parameterListener.setCallback(parameterTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(Parameters::update));
		}

		void update(ValueTree v, bool wasAdded)
		{
			auto numParameters = parameterTree.getNumChildren();
			auto numItems =  getNumChildComponents() - 1;

			if(numParameters == numItems)
				return;

			if(auto b = findParentComponentOfClass<SearchableListComponent>())
			{
				MessageManager::callAsync([b]()
				{
					b->rebuildModuleList(true);
				});
			}
		}

		String getSearchTermForCollection() const override { return "Parameters"; }

		ValueTree parameterTree;
		valuetree::ChildListener parameterListener;
	};

	struct LocalCables: public NodeCollection
	{
		LocalCables(DspNetwork* network):
		  NodeCollection(network, 0)
		{
			setName("Local cables");

			addItems(network->getListOfLocalCableIds(), true);
		}

		String getSearchTermForCollection() const override { return "LocalCables"; }
	};

	struct UsedNodes : public NodeCollection,
					   public DspNetworkListeners::DspNetworkGraphRootListener	
	{
		UsedNodes(DspNetwork* network) :
			NodeCollection(network, 1)
		{
			setName("Used Nodes");
			addItems(network->getListOfUsedNodeIds(), false);
			currentRoot = network->getRootNode();
			refreshAlpha(false);

			lockListener.setCallback(network->getValueTree(), { PropertyIds::Locked }, valuetree::AsyncMode::Asynchronously, [this](ValueTree v, const Identifier&)
			{
				this->refreshAlpha(true);
			});
		}

		void refreshAlpha(bool fade)
		{
			auto vt = currentRoot->getValueTree();

			callRecursive<NodeItem>(this, [vt, fade](NodeItem* ni)
			{
				auto nt = ni->node->getValueTree();
				auto isVisible = nt == vt || nt.isAChildOf(vt);

				ValueTree lockedParent;

				valuetree::Helpers::forEachParent(nt, [&](const ValueTree& v)
				{
					if(v.getType() == PropertyIds::Node)
					{
						if(v[PropertyIds::Locked])
						{
							lockedParent = v;
							return true;
						}
					}

					return false;
				});

				if(lockedParent.isValid() && lockedParent != nt)
					isVisible &= (vt == lockedParent || vt.isAChildOf(lockedParent));

				auto alpha = isVisible ? 1.0f : 0.2f;

				if(fade)
					Desktop::getInstance().getAnimator().animateComponent(ni, ni->getBoundsInParent(), alpha, 500.0, false, 1.5, 1.0);
				else
					ni->setAlpha(alpha);

				ni->setEnabled(alpha > 0.5f);

				return false;
			});
		}

		WeakReference<NodeBase> currentRoot;

		void onRootChange(NodeBase* newRoot) override
		{
			if(currentRoot != newRoot)
			{
				currentRoot = newRoot;
				refreshAlpha(true);
			}
		}

		static void onRootChange(Component& c, NodeBase* n)
		{
			jassertfalse;
		}

		void resized() override
		{
			NodeCollection::resized();

			if(!initialised)
			{
				DspNetworkListeners::initRootListener(this);
				initialised = true;
			}
		}

		bool initialised = false;

		String getSearchTermForCollection() const override { return "UsedNodes"; }

		valuetree::RecursivePropertyListener lockListener;
	};

	struct UnusedNodes : public NodeCollection
	{
		UnusedNodes(DspNetwork* network) :
			NodeCollection(network, 2)
		{
			setName("Unused Nodes");
			addItems(network->getListOfUnusedNodeIds(), false);
		};

		String getSearchTermForCollection() const override { return "UsedNodes"; }

        void mouseDown(const MouseEvent& e) override
        {
            if(e.mods.isRightButtonDown())
            {
                PopupMenu m;
                PopupLookAndFeel plaf;
                m.setLookAndFeel(&plaf);
                m.addItem(1, "Clear unused nodes");
                
                auto r = m.show();
                
                if(r == 1)
                {
                    network->clear(false, true);
                    
                    auto f = findParentComponentOfClass<SearchableListComponent>();
                    
                    MessageManager::callAsync([f]()
                    {
                        f->rebuildModuleList(true);
                    });
                }
            }
        }
	};

	struct Panel : public NetworkPanel
	{
		Panel(FloatingTile* p) :
			NetworkPanel(p)
		{};

		SET_PANEL_NAME("UnusedDspNodeList");

		Component* createComponentForNetwork(DspNetwork* p) override
		{
			return new DspNodeList(p, getParentShell()->getBackendRootWindow());
		}
	};

	int getNumCollectionsToCreate() const override { return 4; }

	Collection* createCollection(int index) override
	{
		if (index == 0)
			return new Parameters(parent);
		if (index == 1)
			return new LocalCables(parent);
		else if (index == 2)
			return new UsedNodes(parent);
		else 
			return new UnusedNodes(parent);
	}

	DspNodeList(DspNetwork* parent_, BackendRootWindow* window) :
		SearchableListComponent(window),
		parent(parent_),
		networkTree(parent->getValueTree())
	{
		parent->addSelectionListener(this);

		nodeUpdater.setTypeToWatch(PropertyIds::Nodes);
		nodeUpdater.setCallback(networkTree, valuetree::AsyncMode::Asynchronously,
			[this](ValueTree, bool)
		{
			this->rebuildModuleList(true);
		});
        
        colourUpdater.setCallback(networkTree, { PropertyIds::NodeColour, PropertyIds::Automated }, valuetree::AsyncMode::Asynchronously, [this](ValueTree, Identifier)
        {
            this->selectionChanged({});
        });
	}

	~DspNodeList()
	{
		if (parent != nullptr)
			parent->removeSelectionListener(this);

	}

	void selectionChanged(const NodeBase::List&) override
	{
		for (int i = 0; i < getNumCollections(); i++)
			getCollection(i)->repaintAllItems();
	}

	WeakReference<DspNetwork> parent;
	ValueTree networkTree;

	valuetree::RecursiveTypedChildListener nodeUpdater;
    valuetree::RecursivePropertyListener colourUpdater;

};
}


#endif  // APIBROWSER_H_INCLUDED
