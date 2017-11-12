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
								private ValueTree::Listener
{
public:
	ScriptComponentListItem(const ValueTree& v, UndoManager& um_, ScriptingApi::Content* c)
		: tree(v),
		  undoManager(um_),
		  content(c)
	{
		static const Identifier coPro("ContentProperties");

		if (tree.getType() == coPro)
			id = "Components";
		else
		    id = tree.getProperty("id");

		connectedComponent = content->getComponentWithName(id);

		tree.addListener(this);
	}

	~ScriptComponentListItem()
	{
		tree.removeListener(this);
	}

	String getUniqueName() const override
	{
		return id;
	}

	bool mightContainSubItems() override
	{
		return tree.getNumChildren() > 0;
	}

	void paintItem(Graphics& g, int width, int height) override
	{
		auto area = Rectangle<int>(0, 0, width - 1, height - 1);

		g.setColour(isSelected() ? Colour(SIGNAL_COLOUR).withAlpha(0.4f) : Colours::black.withAlpha(0.2f));

		g.fillRect(area);
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawRect(area, 1);
		
		


		if (connectedComponent != nullptr)
		{
			static const Identifier sip("saveInPreset");

			const bool saveInPreset = connectedComponent->getScriptObjectProperties()->getProperty(sip);

			Colour c3 = saveInPreset ? Colours::green : Colours::red;

			c3 = c3.withAlpha(JUCE_LIVE_CONSTANT_OFF(0.3f));

			g.setColour(c3);

			const float offset = JUCE_LIVE_CONSTANT_OFF(8.0f);
			Rectangle<float> circle(offset, offset, (float)ITEM_HEIGHT - 2.0f * offset, (float)ITEM_HEIGHT - 2.0f * offset);

			g.fillEllipse(circle);

			g.drawEllipse(circle, 1.0f);
		}

		g.setColour(Colours::white);


		if (connectedComponent == nullptr || !connectedComponent->isShowing())
		{
			g.setColour(Colours::white.withAlpha(0.4f));
		}

		g.setFont(GLOBAL_BOLD_FONT());

		int xOffset = ITEM_HEIGHT + 2;

		g.drawText(id, xOffset, 0, width - 4, height, Justification::centredLeft, true);

		xOffset += GLOBAL_BOLD_FONT().getStringWidth(id) + 10;

		g.setColour(Colours::white.withAlpha(0.2f));

		g.drawText(tree.getProperty("type"), 4 + xOffset, 0, width - 4, height, Justification::centredLeft, true);

	}



	void itemSelectionChanged(bool isNowSelected) override;

	void itemOpennessChanged(bool isNowOpen) override
	{
		if (isNowOpen && getNumSubItems() == 0)
			refreshSubItems();
		else
			clearSubItems();
	}

	var getDragSourceDescription() override
	{
		return "Drag Demo";
	};

	

	bool isInterestedInDragSource(const DragAndDropTarget::SourceDetails& dragSourceDetails) override
	{
		return dragSourceDetails.description == "Drag Demo";
	}

	void itemDropped(const DragAndDropTarget::SourceDetails&, int insertIndex) override
	{
		OwnedArray<ValueTree> selectedTrees;
		getSelectedTreeViewItems(*getOwnerView(), selectedTrees);

		moveItems(*getOwnerView(), selectedTrees, tree, insertIndex, undoManager);

		content->updateAndSetLevel(ScriptingApi::Content::FullRecompile);
	}

	static void moveItems(TreeView& treeView, const OwnedArray<ValueTree>& items,
		ValueTree newParent, int insertIndex, UndoManager& undoManager)
	{
		if (items.size() > 0)
		{
			ScopedPointer<XmlElement> oldOpenness(treeView.getOpennessState(false));

			for (int i = items.size(); --i >= 0;)
			{
				ValueTree& v = *items.getUnchecked(i);

				if (v.getParent().isValid() && newParent != v && !newParent.isAChildOf(v))
				{
					if (v.getParent() == newParent && newParent.indexOf(v) < insertIndex)
						--insertIndex;

					auto cPos = ContentValueTreeHelpers::getLocalPosition(v);
					ContentValueTreeHelpers::getAbsolutePosition(v, cPos);

					auto pPos = ContentValueTreeHelpers::getLocalPosition(v.getParent());
					ContentValueTreeHelpers::getAbsolutePosition(v.getParent(), pPos);

					ContentValueTreeHelpers::updatePosition(v, cPos, pPos);

					v.getParent().removeChild(v, &undoManager);
					newParent.addChild(v, insertIndex, &undoManager);
				}
			}

			if (oldOpenness != nullptr)
				treeView.restoreOpennessState(*oldOpenness, false);
		}
	}

	static void getSelectedTreeViewItems(TreeView& treeView, OwnedArray<ValueTree>& items)
	{
		const int numSelected = treeView.getNumSelectedItems();

		for (int i = 0; i < numSelected; ++i)
			if (const ScriptComponentListItem* vti = dynamic_cast<ScriptComponentListItem*> (treeView.getSelectedItem(i)))
				items.add(new ValueTree(vti->tree));
	}

	void updateSelection(ScriptComponentSelection newSelection)
	{
		bool select = false;

		if (connectedComponent != nullptr)
		{
			select = newSelection.contains(connectedComponent);
		}

		setSelected(select, false, dontSendNotification);

		for (int i = 0; i < getNumSubItems(); i++)
		{
			static_cast<ScriptComponentListItem*>(getSubItem(i))->updateSelection(newSelection);
		}
	}

	

private:

	friend class ScriptComponentList;


	ScriptComponent::Ptr connectedComponent;
	ValueTree tree;
	UndoManager& undoManager;

	ScriptingApi::Content* content;

	String id;

	void refreshSubItems()
	{
		clearSubItems();

		for (int i = 0; i < tree.getNumChildren(); ++i)
			addSubItem(new ScriptComponentListItem(tree.getChild(i), undoManager, content));
	}

	void valueTreePropertyChanged(ValueTree&, const Identifier&) override
	{
		repaintItem();
	}

	void valueTreeChildAdded(ValueTree& parentTree, ValueTree&) override { treeChildrenChanged(parentTree); }
	void valueTreeChildRemoved(ValueTree& parentTree, ValueTree&, int) override { treeChildrenChanged(parentTree); }
	void valueTreeChildOrderChanged(ValueTree& parentTree, int, int) override { treeChildrenChanged(parentTree); }
	void valueTreeParentChanged(ValueTree&) override {}

	void treeChildrenChanged(const ValueTree& parentTree)
	{
		if (parentTree == tree)
		{
			refreshSubItems();
			treeHasChanged();
			setOpen(true);
		}
	}

	

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComponentListItem)
};

//==============================================================================
class ScriptComponentList : public Component,
	public DragAndDropContainer,
	public ScriptingApi::Content::RebuildListener,
	public ScriptComponentEditListener,
	private ButtonListener,
	private Timer
{
public:
	ScriptComponentList(ScriptingApi::Content* c)
		: 
		ScriptComponentEditListener(dynamic_cast<Processor*>(c->getScriptProcessor())),
		undoButton("Undo"),
		redoButton("Redo"),
		foldButton("Fold"),
		unfoldButton("Unfold"),
		content(c)
	{
		addAsScriptEditListener();

		content->addRebuildListener(this);

		addAndMakeVisible(tree);

		tree.setDefaultOpenness(true);
		tree.setMultiSelectEnabled(true);
		tree.setColour(TreeView::backgroundColourId, Colours::transparentBlack);
		tree.setColour(TreeView::ColourIds::dragAndDropIndicatorColourId, Colour(SIGNAL_COLOUR));
		tree.setColour(TreeView::ColourIds::selectedItemBackgroundColourId, Colours::transparentBlack);
		tree.setColour(TreeView::ColourIds::linesColourId, Colours::black.withAlpha(0.1f));
		resetRootItem();

		tree.setRootItemVisible(false);

		addAndMakeVisible(undoButton);
		addAndMakeVisible(redoButton);
		addAndMakeVisible(foldButton);
		addAndMakeVisible(unfoldButton);
		undoButton.addListener(this);
		redoButton.addListener(this);
		foldButton.addListener(this);
		unfoldButton.addListener(this);

		undoButton.setLookAndFeel(&alaf);
		redoButton.setLookAndFeel(&alaf);
		foldButton.setLookAndFeel(&alaf);
		unfoldButton.setLookAndFeel(&alaf);

		tree.addMouseListener(this, true);

		startTimer(500);
	}

	~ScriptComponentList()
	{
		removeAsScriptEditListener();
		content->removeRebuildListener(this);

		tree.setRootItem(nullptr);
	}

	void scriptComponentPropertyChanged(ScriptComponent* sc, Identifier /*idThatWasChanged*/, const var& /*newValue*/) override
	{
		if (sc != nullptr)
		{
			auto item = tree.findItemFromIdentifierString(sc->name.toString());

			if (item != nullptr)
			{
				item->repaintItem();
			}
		}
	}

	void scriptComponentSelectionChanged() override
	{
		if (rootItem != nullptr)
		{
			rootItem->updateSelection(getScriptComponentEditBroadcaster()->getSelection());
		}
	}

	void mouseUp(const MouseEvent& e) override;

	void mouseDoubleClick(const MouseEvent& e) override
	{
		auto i = dynamic_cast<ScriptComponentListItem*>(tree.getItemAt(e.getMouseDownY()));

		if (i != nullptr)
		{
			if (i->connectedComponent)
				ScriptingApi::Content::Helpers::gotoLocation(i->connectedComponent);
		}

		
	}

	class Panel : public PanelWithProcessorConnection
	{
	public:

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{};

		SET_PANEL_NAME("ScriptComponentList");

		Identifier getProcessorTypeId() const override { return JavascriptProcessor::getConnectorId(); }

		Component* createContentComponent(int /*index*/) override;



		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<JavascriptProcessor>(moduleList);
		}
	};

	void paint(Graphics& /*g*/) override
	{
		
	}

	void contentWasRebuilt() override
	{
		resetRootItem();
	}

	void resetRootItem()
	{
		auto v = content->getContentProperties();

		openState = tree.getOpennessState(true);

		tree.setRootItem(rootItem = new ScriptComponentListItem(v, undoManager, content));

		if (openState != nullptr)
		{
			tree.restoreOpennessState(*openState, false);
		}
	}

	void resized() override
	{
		Rectangle<int> r(getLocalBounds().reduced(8));

		Rectangle<int> buttons(r.removeFromBottom(22));
		undoButton.setBounds(buttons.removeFromLeft(60));
		buttons.removeFromLeft(6);
		redoButton.setBounds(buttons.removeFromLeft(60));
		buttons.removeFromLeft(6);
		foldButton.setBounds(buttons.removeFromLeft(60));
		buttons.removeFromLeft(6);
		unfoldButton.setBounds(buttons.removeFromLeft(60));
		buttons.removeFromLeft(6);

		r.removeFromBottom(4);
		tree.setBounds(r);
	}

	void deleteSelectedItems()
	{
		OwnedArray<ValueTree> selectedItems;
		ScriptComponentListItem::getSelectedTreeViewItems(tree, selectedItems);

		for (int i = selectedItems.size(); --i >= 0;)
		{
			ValueTree& v = *selectedItems.getUnchecked(i);

			if (v.getParent().isValid())
				v.getParent().removeChild(v, &undoManager);
		}

		content->updateAndSetLevel(ScriptingApi::Content::FullRecompile);
	}

	bool keyPressed(const KeyPress& key) override;

	void buttonClicked(Button* b) override
	{
		bool ok = false;

		if (b == &undoButton)
		{
			ok = undoManager.undo();
			if (ok)
				content->updateAndSetLevel(ScriptingApi::Content::FullRecompile);
		}
		else if (b == &redoButton)
		{
			ok = undoManager.redo();
			if (ok)
				content->updateAndSetLevel(ScriptingApi::Content::FullRecompile);
		}
		else if (b == &foldButton)
		{
			for (int i = 0; i < rootItem->getNumSubItems(); i++)
			{
				rootItem->getSubItem(i)->setOpenness(TreeViewItem::opennessClosed);
			}
		}
		else if (b == &unfoldButton)
		{
			for (int i = 0; i < rootItem->getNumSubItems(); i++)
			{
				rootItem->getSubItem(i)->setOpenness(TreeViewItem::opennessOpen);
			}
		}
	}

private:

	ScopedPointer<XmlElement> openState;

	AlertWindowLookAndFeel alaf;

	ScriptingApi::Content* content;

	TreeView tree;
	TextButton undoButton, redoButton, foldButton, unfoldButton;
	ScopedPointer<ScriptComponentListItem> rootItem;
	UndoManager undoManager;

	void timerCallback() override
	{
		undoManager.beginNewTransaction();
	}

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ScriptComponentList)
};




#if 0
class ScriptComponentList : public SearchableListComponent,
							public ScriptComponentEditListener,
							public ScriptingApi::Content::RebuildListener,
							public GlobalScriptCompileListener,
							public ButtonListener,
							public DragAndDropContainer
{
public:

	ScriptComponentList(BackendRootWindow* window, JavascriptProcessor* p);

	~ScriptComponentList();

	void buttonClicked(Button* b) override;

	class ScriptComponentItem : public SearchableListComponent::Item,
								public SafeChangeListener,
								public DragAndDropTarget
								
	{
	public:

		ScriptComponentItem(ScriptingApi::Content* c, const Identifier &id);

		~ScriptComponentItem();

		void changeListenerCallback(SafeChangeBroadcaster* b) override;

		void mouseEnter(const MouseEvent&) override;
		void mouseExit(const MouseEvent&) override;
		void mouseDoubleClick(const MouseEvent&) override;

		void paint(Graphics& g) override;

		void mouseUp(const MouseEvent& event) override;

		void mouseDrag(const MouseEvent& event) override;

		bool keyPressed(const KeyPress& key) override;

		bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
		
		void itemDragEnter(const SourceDetails& dragSourceDetails) override;

		void itemDragExit(const SourceDetails& dragSourceDetails) override;

		void itemDragMove(const SourceDetails& dragSourceDetails) override;

		void itemDropped(const SourceDetails& dragSourceDetails) override;


	private:

		bool insertDragAfterComponent = false;
		bool insertDragAsParentComponent = false;

		int childDepth = 0;

		String id;
		String typeName;
		float typeOffset;

		ValueTree data;

		Identifier idAsId;
		
		ScriptingApi::Content* content;

		ScriptComponent::Ptr connectedScriptComponent;
	};

	
	void scriptComponentSelectionChanged() override
	{
		getCollection(0)->repaintAllItems();
	}

	
	void scriptComponentPropertyChanged(ScriptComponent* /*sc*/, Identifier idThatWasChanged, const var& /*newValue*/)
	{
		static const Identifier e("visible");

		if (e == idThatWasChanged && showOnlyVisibleItems)
		{
			rebuildModuleList(true);
		}
	}

	void scriptWasCompiled(JavascriptProcessor* p)
	{
		if (jp == p)
		{
			rebuildModuleList(true);
		}
	}

	void contentWasRebuilt() override
	{
		rebuildModuleList(true);
	}

	class AllCollection : public SearchableListComponent::Collection,
						  public DragAndDropTarget
	{
	public:

		AllCollection(JavascriptProcessor* p, bool showOnlyVisibleItems);

		void paint(Graphics& g) override;

		bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;

		void itemDragEnter(const SourceDetails& dragSourceDetails) override;

		void itemDragExit(const SourceDetails& dragSourceDetails) override;

		void itemDragMove(const SourceDetails& dragSourceDetails) override;

		void itemDropped(const SourceDetails& dragSourceDetails) override;

	private:

		bool isDropTarget = false;
	};

	class Panel : public PanelWithProcessorConnection
	{
	public:

		Panel(FloatingTile* parent) :
			PanelWithProcessorConnection(parent)
		{};

		SET_PANEL_NAME("ScriptComponentList");

		Identifier getProcessorTypeId() const override { return JavascriptProcessor::getConnectorId(); }

		Component* createContentComponent(int /*index*/) override;

		

		void fillModuleList(StringArray& moduleList) override
		{
			fillModuleListWithType<JavascriptProcessor>(moduleList);
		}
	};

	int getNumCollectionsToCreate() const override { return 1; }

	Collection *createCollection(int /*index*/) override
	{
		return new AllCollection(jp, showOnlyVisibleItems);
	}

	void paint(Graphics& g) override
	{
		g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::DebugAreaBackgroundColourId));
		SearchableListComponent::paint(g);
	}

private:

	bool showOnlyVisibleItems = false;

	ScopedPointer<ShapeButton> hideButton;

	ScriptComponent::Ptr lastClickedComponent;

	JavascriptProcessor* jp;
};
#endif


} // namespace hise

#endif  // SCRIPTCOMPONENTLIST_H_INCLUDED
