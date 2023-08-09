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

		const int extendedWidth = 600;

		MethodItem(const ValueTree &methodTree_, const String &className_);

		int getPopupHeight() const override
		{ 
			if (parser != nullptr) 
				return (int)parser->getHeightForWidth((float)extendedWidth) + 20;
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

		void focusGained(FocusChangeType) override
		{
			repaint();
		}

		MarkdownLink getLink() const override
		{
			String s = "scripting/scripting-api/";
			s << className << "#" << name << "/";
			return { File(), s };
		}

		void focusLost(FocusChangeType ) override
		{
			repaint();
		}

		void paintPopupBox(Graphics &g) const
		{
			if (parser != nullptr)
			{
				auto bounds = Rectangle<float>(10.0f, 10.0f, (float)extendedWidth, (float)getPopupHeight() - 20);
				parser->draw(g, bounds);
			}
			else
			{
				auto bounds = Rectangle<float>(10.0f, 10.0f, 280.0f, (float)getPopupHeight() - 20);
				help.draw(g, bounds);
			}
			
		}

		void mouseEnter(const MouseEvent&) override { repaint(); }
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

	class ClassCollection : public SearchableListComponent::Collection
	{
	public:
		ClassCollection(const ValueTree &api);;

		void paint(Graphics &g) override;
	private:

		String name;

		const ValueTree classApi;
	};

	int getNumCollectionsToCreate() const override { return apiTree.getNumChildren(); }

	Collection *createCollection(int index) override
	{
		return new ClassCollection(apiTree.getChild(index));
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

	struct NodeItem : public SearchableListComponent::Item,
		public ButtonListener,
		public Label::Listener
	{
		
		NodeItem(DspNetwork* parent, const String& id) :
			Item(id),
			node(dynamic_cast<NodeBase*>(parent->get(id).getObject())),
			label(),
			powerButton("on", this, f)
		{
			label.setText(id, dontSendNotification);
			usePopupMenu = false;

			addAndMakeVisible(powerButton);
			addAndMakeVisible(label);
			label.addListener(this);

			label.setFont(GLOBAL_BOLD_FONT());
			label.setColour(Label::ColourIds::textColourId, Colours::white);
            label.setInterceptsMouseClicks(false, true);
			label.refreshWithEachKey = false;
			label.addMouseListener(this, true);

            
            label.setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
            label.setColour(Label::ColourIds::outlineWhenEditingColourId, Colour(SIGNAL_COLOUR));
            label.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
            label.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
            label.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
            
			powerButton.setToggleModeWithColourChange(true);

			idListener.setCallback(node->getValueTree(), { PropertyIds::ID }, valuetree::AsyncMode::Asynchronously,
				BIND_MEMBER_FUNCTION_2(NodeItem::updateId));

			bypassListener.setCallback(node->getValueTree(), { PropertyIds::Bypassed }, valuetree::AsyncMode::Asynchronously,
				BIND_MEMBER_FUNCTION_2(NodeItem::updateBypassState));

			auto fid = node->getValueTree()[PropertyIds::FactoryPath].toString();

			searchKeywords << ";" << fid;


            if(fid.startsWith("container.") && fid != "container.chain")
            {
				scriptnode::NodeComponentFactory f;
                icon = f.createPath(fid.fromFirstOccurrenceOf("container.", false, false));
            }

		}
        
        Path icon;

		void updateBypassState(Identifier id, var newValue)
		{
			powerButton.setToggleStateAndUpdateIcon(!newValue);
            label.setColour(Label::ColourIds::textColourId, Colours::white.withAlpha(!newValue ? 0.8f : 0.3f));
            repaint();
		}

		void updateId(Identifier id, var newValue)
		{
			label.setText(newValue.toString(), dontSendNotification);
		}

		void labelTextChanged(Label*) override
		{
			if (node != nullptr)
				node->setValueTreeProperty(PropertyIds::ID, label.getText());
		}

		void buttonClicked(Button* b) override
		{
			if (node != nullptr)
				node->setValueTreeProperty(PropertyIds::Bypassed, !b->getToggleState());
		}

        int getIntendation() const
        {
            auto networkData = node->getRootNetwork()->getValueTree();
            
            auto nTree = node->getValueTree();
            
            int index = 0;
            
            while(nTree.isValid() && nTree != networkData)
            {
                index++;
                nTree = nTree.getParent();
            }
            
            return index;
        }
        
        void paint(Graphics& g) override;
		

		void resized() override
		{
			auto b = getLocalBounds().reduced(1);
            b.removeFromLeft(getIntendation()*2);
            area = b;
            b.removeFromLeft(5);
            
            powerButton.setBounds(b.removeFromLeft(b.getHeight()).reduced(2));
            
            if(!icon.isEmpty())
            {
                
                
                PathFactory::scalePath(icon, b.removeFromLeft(b.getHeight() - 4).reduced(2).toFloat());
            }
            
			label.setBounds(b);
		}
        
		void mouseUp(const MouseEvent& event) override
		{
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
        
        Rectangle<int> area;
	};

	struct NodeCollection : public SearchableListComponent::Collection
	{
		NodeCollection(DspNetwork* network_) :
			network(network_)
		{};

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds();
			auto top = b.removeFromTop(30);
			g.setColour(Colours::white.withAlpha(0.8f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(getName(), top.toFloat(), Justification::centred);
		}

		void addItems(const StringArray& idList)
		{
			for (const auto& id : idList)
			{
				auto newItem = new NodeItem(network.get(), id);
				addAndMakeVisible(newItem);
				items.add(newItem);
			}
		}

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

	struct UsedNodes : public NodeCollection
	{
		UsedNodes(DspNetwork* network) :
			NodeCollection(network)
		{
			setName("Used Nodes");
			addItems(network->getListOfUsedNodeIds());
		}
	};

	struct UnusedNodes : public NodeCollection
	{
		UnusedNodes(DspNetwork* network) :
			NodeCollection(network)
		{
			setName("Unused Nodes");
			addItems(network->getListOfUnusedNodeIds());
		};
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

	int getNumCollectionsToCreate() const override { return 2; }

	Collection* createCollection(int index) override
	{
		if (index == 0)
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
