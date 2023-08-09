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


#ifndef SCRIPTCOMPONENTLIST_H_INCLUDED
#define SCRIPTCOMPONENTLIST_H_INCLUDED

namespace hise { using namespace juce;




//==============================================================================
class ScriptComponentListItem : public TreeViewItem,
								private AsyncValueTreePropertyListener,
								private GlobalScriptCompileListener,
								public Dispatchable,
							    public Timer
{
public:

	ScriptComponentListItem(ValueTree v, UndoManager& um_, ScriptingApi::Content* c, const String& searchTerm);

	~ScriptComponentListItem()
	{
		if (content.get() == nullptr)
			return;

		content->getProcessor()->getMainController()->removeScriptListener(this);
	}

	String getUniqueName() const override
	{
		return id;
	}

	XmlElement* getBetterOpennessState()
	{
		ScopedPointer<XmlElement> xml = new XmlElement("OpenState");
		xml->setAttribute("id", getUniqueName());
		xml->setAttribute("open", (int)getOpenness());
		
		for (int i = 0; i < getNumSubItems(); i++)
		{
			xml->addChildElement(static_cast<ScriptComponentListItem*>(getSubItem(i))->getBetterOpennessState());
		}

		return xml.release();
	}

	void itemDoubleClicked(const MouseEvent& e) override;

	bool isRootItem() const
	{
		return id == "Components";
	}

	void scriptWasCompiled(JavascriptProcessor *processor) override
	{
		if (processor->getContent() == content)
		{
			refreshScriptDefinedState();
		}
	}

	void timerCallback() override
	{
		refreshScriptDefinedState();
		stopTimer();
	}

	bool mightContainSubItems() override
	{
		return tree.getNumChildren() > 0;
	}

	void paintItem(Graphics& g, int width, int height) override;


	int getItemHeight() const
	{
		return fitsSearch ? 20 : 0;
	}

	void restoreFoldStateFromValueTree(const ValueTree& foldState)
	{
		const Identifier id_ = Identifier(getUniqueName());

		if (foldState.hasProperty(id_))
		{
			setOpen(foldState.getProperty(id_));
		}

		for (int i = 0; i < getNumSubItems(); i++)
		{
			static_cast<ScriptComponentListItem*>(getSubItem(i))->restoreFoldStateFromValueTree(foldState);
		}
	}

	void itemSelectionChanged(bool isNowSelected) override;
	void itemOpennessChanged(bool isNowOpen) override;

	var getDragSourceDescription() override;;

	
	bool isInterestedInDragSource(const DragAndDropTarget::SourceDetails& dragSourceDetails) override;
	void itemDropped(const DragAndDropTarget::SourceDetails&, int insertIndex) override;

	static void moveItems(TreeView& treeView, const OwnedArray<ValueTree>& items,
		ValueTree newParent, int insertIndex, UndoManager& undoManager);

	static void getSelectedTreeViewItems(TreeView& treeView, OwnedArray<ValueTree>& items);

	void updateSelection(ScriptComponentSelection newSelection);

	void checkSearchTerm(const String& searchTerm_)
	{
		if (searchTerm_.isEmpty())
		{
			fitsSearch = true;
			return;
		}

		if (searchTerm_.startsWithIgnoreCase("REGEX"))
		{
			auto s = searchTerm_.substring(5).trim();
			fitsSearch = hise::RegexFunctions::matchesWildcard(s, getItemIdentifierString());
		}
		else
        {
            auto st = getItemIdentifierString().toLowerCase();
            
            auto sa = StringArray::fromTokens(st, "/", "");
            sa.removeEmptyStrings();
            fitsSearch = false;
            
            for(auto a: sa)
                fitsSearch |= FuzzySearcher::fitsSearch(searchTerm_, a, 0.4);
        }
			
	}

private:

	friend class ScriptComponentList;
	bool fitsSearch = true;
	
	ValueTree tree;
	UndoManager& undoManager;
    
	WeakReference<ScriptingApi::Content> content;

    String searchTerm;
    
	String id;

	void refreshSubItems()
	{
		clearSubItems();

		bool hasVisibleChildren = false;

		for (int i = 0; i < tree.getNumChildren(); ++i)
		{
			if (content == nullptr)
				break;

			auto scli = new ScriptComponentListItem(tree.getChild(i), undoManager, content, searchTerm);

			addSubItem(scli);
			scli->checkSearchTerm(searchTerm);

			if (!hasVisibleChildren && scli->getItemHeight() > 0)
				hasVisibleChildren = true;
		}
		
		if (!fitsSearch && hasVisibleChildren)
			fitsSearch = true;
	}

	void asyncValueTreePropertyChanged(ValueTree&, const Identifier&) override
	{
		repaintItem();
	}

	void refreshScriptDefinedState();

	void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override { treeChildrenChanged(parentTree); }
	void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override { treeChildrenChanged(parentTree); }
	void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override { treeChildrenChanged(parentTree); }
	void valueTreeParentChanged(ValueTree&) override {}

	void treeChildrenChanged(const ValueTree& parentTree)
	{
		if (content != nullptr && parentTree == tree)
		{
			auto f = [](Dispatchable* obj)
			{
				auto list = static_cast<ScriptComponentListItem*>(obj);

				list->refreshSubItems();
				list->treeHasChanged();
				list->setOpen(true);

				return Dispatchable::Status::OK;
			};

			content->getScriptProcessor()->getMainController_()->getLockFreeDispatcher().callOnMessageThreadAfterSuspension(this, f);
		};
	}
	bool isDefinedInScript = false;
	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComponentListItem)
};

//==============================================================================
class ScriptComponentList : public Component,
	public DragAndDropContainer,
	public ScriptingApi::Content::RebuildListener,
	public ScriptComponentEditListener,
	public TextEditor::Listener,
	public Timer
{
public:
	ScriptComponentList(ScriptingApi::Content* c, bool openess);

	~ScriptComponentList();

	void scriptComponentPropertyChanged(ScriptComponent* sc, Identifier /*idThatWasChanged*/, const var& /*newValue*/) override;

	void scriptComponentSelectionChanged() override;

	void mouseUp(const MouseEvent& e) override;

	class Panel : public PanelWithProcessorConnection
	{
	public:

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{};

		SET_PANEL_NAME("ScriptComponentList");

		Identifier getProcessorTypeId() const override { return JavascriptProcessor::getConnectorId(); }

		Component* createContentComponent(int /*index*/) override;

        var toDynamicObject() const override
        {
            auto obj = PanelWithProcessorConnection::toDynamicObject();
            obj.getDynamicObject()->setProperty("Openess", defaultOpeness);
            return obj;
        }
        
        void fromDynamicObject(const var& obj) override
        {
            defaultOpeness = obj.getProperty("Openess", true);
            PanelWithProcessorConnection::fromDynamicObject(obj);
        }

		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<JavascriptProcessor>(moduleList);
		}
        
        bool defaultOpeness = true;
	};

	void paint(Graphics& g) override;

    void textEditorTextChanged(TextEditor&) override
	{
		searchTerm = fuzzySearchBox->getText().toLowerCase();

		resetRootItem();
		foldAll(false);
		
	}

	void contentWasRebuilt() override
	{
		resetRootItem();
	}

	void resetRootItem();

	void resized() override;

	

	void deleteSelectedItems()
	{
		OwnedArray<ValueTree> selectedItems;
		ScriptComponentListItem::getSelectedTreeViewItems(*tree, selectedItems);

		for (int i = selectedItems.size(); --i >= 0;)
		{
			ValueTree& v = *selectedItems.getUnchecked(i);

			if (v.getParent().isValid())
				v.getParent().removeChild(v, &undoManager);
		}
	}

	bool keyPressed(const KeyPress& key) override;

	

	ValueTree getFoldStateTree() { return foldState; }

private:

	
	struct LookAndFeel : public LookAndFeel_V3
	{
		void drawTreeviewPlusMinusBox(Graphics& g, const Rectangle<float>& area,
			Colour /*backgroundColour*/, bool isOpen, bool isMouseOver)
		{
			Path p;
			p.addTriangle(0.0f, 0.0f, 1.0f, isOpen ? 0.0f : 0.5f, isOpen ? 0.5f : 0.0f, 1.0f);

			g.setColour(Colours::white.withAlpha(isMouseOver ? 0.8f : 0.6f));
			g.fillPath(p, p.getTransformToScaleToFit(area.reduced(3, 3), true));
		}

		bool areLinesDrawnForTreeView(TreeView&)
		{
			return true;
		}
	};

	LookAndFeel laf;

	bool foldStateRestorePending = false;

	void foldAll(bool shouldBeFolded)
	{
		for (int i = 0; i < rootItem->getNumSubItems(); i++)
		{
			rootItem->getSubItem(i)->setOpenness(shouldBeFolded ? TreeViewItem::Openness::opennessClosed : TreeViewItem::Openness::opennessOpen);
		}
	}

	Path searchPath;
    
    UndoManager& undoManager;
    
	ValueTree foldState;
	
	ScopedPointer<XmlElement> openState;

	AlertWindowLookAndFeel alaf;

	ScriptingApi::Content* content;

	ScopedPointer<TextEditor> fuzzySearchBox;

	String searchTerm;

    ScopedPointer<ScriptComponentListItem> rootItem;
    
	ScopedPointer<TreeView> tree;
	
    ScrollbarFader sf;
	
	int scrollY = 0;

    bool defaultOpeness = true;
    
	void timerCallback() override
	{
		if (tree != nullptr)
		{
			scrollY = tree->getViewport()->getViewPositionY();
			openState = tree->getOpennessState(false).release();
		}
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComponentList)
};





} // namespace hise

#endif  // SCRIPTCOMPONENTLIST_H_INCLUDED
