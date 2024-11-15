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
using namespace juce;

class ProcessorWithScriptingContent;

#define DECLARE_ID(x) static const juce::Identifier x(#x);

namespace ScriptnodeShortcuts
{
	DECLARE_ID(sn_deselect_all);
	DECLARE_ID(sn_duplicate);
	DECLARE_ID(sn_new_node);
	DECLARE_ID(sn_fold);
	DECLARE_ID(sn_add_bookmark);
	DECLARE_ID(sn_zoom_in);
	DECLARE_ID(sn_zoom_out);
	DECLARE_ID(sn_zoom_fit);
	DECLARE_ID(sn_zoom_reset);
	DECLARE_ID(sn_edit_property);
	DECLARE_ID(sn_toggle_bypass);
	DECLARE_ID(sn_toggle_cables);
}
#undef DECLARE_ID

}

namespace scriptnode
{

using namespace juce;
using namespace hise;

class KeyboardPopup : public Component,
					  public TextEditor::Listener,
					  public KeyListener,
					  public ButtonListener
{
public:

	enum Mode
	{
		Wrap,
		Surround,
		New,
		numModes
	};

	struct TagList: public Component
	{
		String currentTag;

		void toggleTag(const String& tag)
		{
			if (currentTag == tag)
			{
				currentTag = {};
			}
			else
				currentTag = tag;

			findParentComponentOfClass<KeyboardPopup>()->setSearchText(currentTag);

			refreshTags();
		}

		void refreshTags()
		{
			for (auto t : tags)
			{
				t->setActive(currentTag.isEmpty() ? 0.5f : (currentTag == t->getName() ? 1.0f : 0.2f));
			}
		}

		struct Tag : public Component
		{
			void setActive(float alpha_)
			{
				alpha = alpha_;
				active = alpha > 0.5f;
				
				repaint();
			}

			Tag(const String& t)
			{
				setRepaintsOnMouseActivity(true);

				setName(t);

				auto w = GLOBAL_BOLD_FONT().getStringWidth(t) + 25;
				setSize(w, 28);
			}

			void mouseDown(const MouseEvent& )
			{
				findParentComponentOfClass<TagList>()->toggleTag(getName());
			}

			float alpha = 0.5f;
			bool active = false;

			void paint(Graphics& g) override;
		};

		TagList(DspNetwork* n)
		{
			auto factories = n->getFactoryList();

			for (auto f : factories)
				tags.add(new Tag(f));

			int x = 0;

			for (auto t : tags)
			{
				x += t->getWidth();
				addAndMakeVisible(t);
				x += 3;
			}

			tagWidth = x;
				
			
			setSize(x, 32);
		}

		int getTagWidth() const
		{
			return tagWidth;
		}

		int tagWidth;

		void resized() override
		{
			int x = 0;

			for (auto t : tags)
			{
				t->setTopLeftPosition(x, 2);

				x += t->getWidth() + 3;
			}
		}

		OwnedArray<Tag> tags;
		
	};

	struct Factory : public PathFactory
	{
		String getId() const override { return ""; }

		Path createPath(const String& url) const override
		{
			Path p;

			LOAD_EPATH_IF_URL("help", MainToolbarIcons::help);

			return p;
		}
	} factory;

	int addPosition;

	Image screenshot;

	struct ImagePreviewCreator: public Timer
	{
		ImagePreviewCreator(KeyboardPopup& kp, const String& path);

		~ImagePreviewCreator();

		void timerCallback();

		KeyboardPopup& kp;
		NodeBase::Holder holder;
		DspNetwork* network;
		WeakReference<NodeBase> createdNode;
		ScopedPointer<Component> createdComponent;
		String path;
	};

	struct OneLiner : public Component
	{
		String description;

		void paint(Graphics& g) override
		{
			g.fillAll(Colours::black.withAlpha(0.1f));
			g.setColour(Colours::white);
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText(description, getLocalBounds().toFloat(), Justification::centred);
		}
	};

	ScopedPointer<OneLiner> oneLiner;
	ScopedPointer<ImagePreviewCreator> currentPreview;

	bool parentIsViewport = true;

	KeyboardPopup(NodeBase* container, int addPosition_):
		node(container),
		network(node->getRootNetwork()),
		list(network),
		tagList(network),
		addPosition(addPosition_),
		helpButton("help", this, factory)
	{
		laf.bg = Colours::transparentBlack;
		viewport.setLookAndFeel(&laf);

		setName("Create Node");

		addAndMakeVisible(helpButton);
		addAndMakeVisible(tagList);
		addAndMakeVisible(nodeEditor);

		nodeEditor.setFont(GLOBAL_MONOSPACE_FONT());
		nodeEditor.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.8f));
		nodeEditor.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR).withAlpha(0.5f));
		nodeEditor.setSelectAllWhenFocused(true);
		nodeEditor.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

		nodeEditor.addListener(this);
		addAndMakeVisible(viewport);
		viewport.setViewedComponent(&list, false);

		viewport.setScrollBarThickness(14);
		viewport.setScrollOnDragEnabled(true);

		helpButton.addListener(this);
		setSize(tagList.getWidth(), 64 + 240 + 2 * UIValues::NodeMargin);
		nodeEditor.addKeyListener(this);
		list.rebuild(getWidthForListItems(), true);
	}

	void buttonClicked(Button* )
	{
		auto t = list.getCurrentText();

		URL url("https://docs.hise.audio/");
		url = url.getChildURL("scriptnode").getChildURL("list");
		url = url.getChildURL(t.upToFirstOccurrenceOf(".", false, false));
		url = url.getChildURL(t.fromFirstOccurrenceOf(".", false, false) + ".html");

		url.launchInDefaultBrowser();
	}

	void scrollToMakeVisible(int yPos)
	{
		auto r = viewport.getViewArea();

		Range<int> range(r.getY(), r.getBottom());

		Range<int> itemRange(yPos, yPos + PopupList::ItemHeight);

		if (range.contains(itemRange))
			return;

		bool scrollDown = itemRange.getEnd() > range.getEnd();

		if (scrollDown)
		{
			auto delta = itemRange.getEnd() - range.getEnd();
			viewport.setViewPosition(0, range.getStart() + delta);
		}
		else
			viewport.setViewPosition(0, yPos);
	}

	bool keyPressed(const KeyPress& k, Component*) override;

	void addNodeAndClose(String path);

	void textEditorTextChanged(TextEditor&) override
	{
		setSearchText(nodeEditor.getText());
	}

	int getWidthForListItems() const
	{
		return tagList.getWidth() / 2 - viewport.getScrollBarThickness();
	}

	void setSearchText(const String& text)
	{
		nodeEditor.setText(text, dontSendNotification);
		list.setSearchText(nodeEditor.getText());
		list.rebuild(getWidthForListItems(), true);
		resized();
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(32).reduced(3);

		helpButton.setBounds(top.removeFromRight(top.getHeight()).reduced(2));
		top.removeFromLeft(top.getHeight());
		nodeEditor.setBounds(top.reduced(8, 0));

		b.removeFromTop(UIValues::NodeMargin);

		tagList.setBounds(b.removeFromTop(32));
		list.rebuild(getWidthForListItems(), false);
		
		b.removeFromTop(UIValues::NodeMargin);

		viewport.setBounds(b.removeFromLeft(getWidth() / 2));

		if (oneLiner != nullptr)
			oneLiner->setBounds(b.removeFromBottom(32));
	}

	Rectangle<float> getPreviewBounds();

	void paint(Graphics& g) override;

	struct PopupList : public Component
	{
		static constexpr int ItemHeight = 24;

		enum Type
		{
			Clipboard,
			ExistingNode,
			NewNode
		};

		struct Entry
		{
			bool operator==(const Entry& other) const
			{
				return displayName == other.displayName;
			}

			Type t;
			String insertString;
			String displayName;
		};

		Array<Entry> allIds;
		String searchTerm;

		void rebuildItems()
		{
			allIds.clear();

			auto clipboardContent = SystemClipboard::getTextFromClipboard();

			if (clipboardContent.startsWith("ScriptNode"))
			{
				auto v = ValueTreeConverters::convertBase64ToValueTree(clipboardContent.fromFirstOccurrenceOf("ScriptNode", false, true), true);

				Entry cItem;
				cItem.t = Clipboard;
				cItem.insertString = clipboardContent;
				cItem.displayName = v[PropertyIds::ID].toString();

				allIds.add(cItem);
			}

			for (auto existingNodeId : network->getListOfUnusedNodeIds())
			{
				Entry eItem;
				eItem.t = ExistingNode;
				eItem.insertString = existingNodeId;
				eItem.displayName = existingNodeId;
				allIds.add(eItem);
			}

			for (auto newNodePath : network->getListOfAllAvailableModuleIds())
			{
				Entry nItem;
				nItem.t = NewNode;
				nItem.insertString = newNodePath;
				nItem.displayName = newNodePath;
				allIds.add(nItem);
			}

			rebuild(getWidth(), true);
		}

		WeakReference<DspNetwork> network;

		PopupList(DspNetwork* n):
			network(n)
		{
			rebuildItems();
		}

		String getTextToInsert() const
		{
			if (auto i = items[selectedIndex])
				return i->entry.insertString;

			return {};
		}

		String getCurrentText() const
		{
			if (auto i = items[selectedIndex])
				return i->entry.displayName;

			return {};
		}


		int selectedIndex = 0;

		int selectNext(bool next)
		{
			int nextIndex = selectedIndex;

			if (next)
				nextIndex = jmin(items.size(), selectedIndex + 1);
			else
				nextIndex = jmax(0, selectedIndex - 1);

			

			setSelected(items[nextIndex], false);

			return nextIndex * ItemHeight;
		}

		void setSearchText(const String& text)
		{
			searchTerm = text.toLowerCase();
			rebuild(maxWidth, true);

			selectedIndex = 0;
			setSelected(items[selectedIndex], true);
		}

		void paint(Graphics& g) override
		{
			//g.fillAll(Colours::black.withAlpha(0.2f));
		}

		struct Item : public Component,
					  public ButtonListener
		{
			Item(const Entry& entry_, bool isSelected_);

			void buttonClicked(Button* b) override;

			void mouseDown(const MouseEvent& );

			void mouseDoubleClick(const MouseEvent& event) override;

			bool keyPressed(const KeyPress& k) override;
            
			void paint(Graphics& g) override;

			void resized() override;

			bool selected = false;
			Entry entry;

			Path p;
			NodeComponent::Factory f;
			HiseShapeButton deleteButton;
		};

		void setSelected(Item* i, bool forceRebuild);

		void resized() override
		{
			int y = 0;

			for (auto i : items)
			{
				i->setBounds(0, y, getWidth(), ItemHeight);
				y += ItemHeight;
			}
		}

		int maxWidth = 0;

		void rebuild(int maxWidthToUse, bool force)
		{
            if(maxWidthToUse != maxWidth || force)
            {
                items.clear();

                int y = 0;
                
                maxWidth = maxWidthToUse;

                auto f = GLOBAL_MONOSPACE_FONT();

                for (auto id : allIds)
                {
                    if (searchTerm.isNotEmpty() && !id.displayName.contains(searchTerm))
                        continue;

                    if (searchTerm == id.displayName)
                        selectedIndex = items.size();

                    auto newItem = new Item(id, selectedIndex == items.size());
                    items.add(newItem);
                    addAndMakeVisible(newItem);

                    maxWidth = jmax(f.getStringWidth(id.displayName) + 20, maxWidth);
                    y += ItemHeight;
                }

                setSize(maxWidth, y);
                resized();
            }
		}

		OwnedArray<Item> items;
	};

	NodeBase::Ptr node;
	DspNetwork* network;
	
	TextEditor nodeEditor;
	TagList tagList;
	PopupList list;
	Viewport viewport;

	ZoomableViewport::Laf laf;

	HiseShapeButton helpButton;
};







class DspNetworkGraph : public ComponentWithMiddleMouseDrag,
	public AsyncUpdater,
	public DragAndDropContainer,
	public DspNetwork::SelectionListener
{
public:

	struct ActionButton : public WrapperWithMenuBarBase::ActionButtonBase<DspNetworkGraph, DspNetworkPathFactory>,
						  public DspNetwork::SelectionListener
	{
		ActionButton(DspNetworkGraph* parent_, const String& name) :
			ActionButtonBase<DspNetworkGraph, DspNetworkPathFactory>(parent_, name)
		{
			parent.getComponent()->network->addSelectionListener(this);
		}

		~ActionButton()
		{
			if (auto pc = parent.getComponent())
            {
                if(pc->network != nullptr)
                    pc->network->removeSelectionListener(this);
            }
		}


		void selectionChanged(const NodeBase::List& l) override
		{
			repaint();
		}
	};
	
	struct WrapperWithMenuBar : public WrapperWithMenuBarBase
	{
		static bool selectionEmpty(DspNetworkGraph& g)
		{
			return !g.network->getSelection().isEmpty();
		}

		WrapperWithMenuBar(DspNetworkGraph* g);;

		void rebuildAfterContentChange() override;

		bool isValid() const override
		{
			return n.get() != nullptr;
		}

        int bookmarkAdded() override
        {
            return Actions::addBookMark(n.get());
        }
        
		virtual void bookmarkUpdated(const StringArray& idsToShow);

		virtual ValueTree getBookmarkValueTree()
		{
			return n->getValueTree().getOrCreateChildWithName(PropertyIds::Bookmarks, n->getUndoManager());
		}

		void addButton(const String& b) override;

		ReferenceCountedObjectPtr<DspNetwork> n;
	};

	struct Actions
	{
		static void selectAndScrollToNode(DspNetworkGraph& g, NodeBase::Ptr node);

		static bool swapOrientation(DspNetworkGraph& g);

		static bool freezeNode(NodeBase::Ptr node);
		static bool unfreezeNode(NodeBase::Ptr node);

		static bool toggleBypass(DspNetworkGraph& g);
		static bool toggleFreeze(DspNetworkGraph& g);

		static bool toggleProbe(DspNetworkGraph& g);
		static bool setRandomColour(DspNetworkGraph& g);
        
		static bool toggleDebug(DspNetworkGraph& g);

		static bool lockContainer(DspNetworkGraph& g);

		static bool eject(DspNetworkGraph& g);

		static bool showParameterPopup(DspNetworkGraph& g);

        static bool toggleSignalDisplay(DspNetworkGraph& g);
        
		static bool copyToClipboard(DspNetworkGraph& g);
		static bool toggleCableDisplay(DspNetworkGraph& g);
		static bool toggleComments(DspNetworkGraph& g);
		static bool toggleCpuProfiling(DspNetworkGraph& g);
		static bool editNodeProperty(DspNetworkGraph& g);
		static bool foldSelection(DspNetworkGraph& g);
		static bool foldUnselectedNodes(DspNetworkGraph& g);
		static bool arrowKeyAction(DspNetworkGraph& g, const KeyPress& k);
		static bool showKeyboardPopup(DspNetworkGraph& g, KeyboardPopup::Mode mode);
		static bool duplicateSelection(DspNetworkGraph& g);
		static bool deselectAll(DspNetworkGraph& g);;
		static bool deleteSelection(DspNetworkGraph& g);
		static bool showJSONEditorForSelection(DspNetworkGraph& g);
		static bool undo(DspNetworkGraph& g);
		static bool redo(DspNetworkGraph& g);

        static bool exportAsSnippet(DspNetworkGraph& g);
        static bool save(DspNetworkGraph& g);
        
		static int addBookMark(DspNetwork* n);

		static bool zoomIn(DspNetworkGraph& g);
		static bool zoomOut(DspNetworkGraph& g);
		static bool zoomFit(DspNetworkGraph& g);
	};

	void selectionChanged(const NodeBase::List&) override
	{
		
	}

	struct RootUndoButtons: public Component,
						    public PathFactory,
						    public Button::Listener
	{
		RootUndoButtons(DspNetworkGraph& parent_):
		  parent(parent_),
		  undo("undo", this, *this),
		  redo("redo", this, *this)
		{
			addAndMakeVisible(undo);
			addAndMakeVisible(redo);
			parent.addMouseListener(this, true);
		}

		void mouseDown(const MouseEvent& e) override
		{
			if(e.mods.isX1ButtonDown())
				undo.triggerClick();
			if(e.mods.isX2ButtonDown())
				redo.triggerClick();
		}

		void buttonClicked(Button* b) override
		{
			if(b == &undo)
				parent.rootUndoManager.undo();
			else
				parent.rootUndoManager.redo();
		}

		Path createPath(const String& id) const override
		{
			static const unsigned char pathData[] = { 110,109,0,128,38,68,40,0,64,67,98,2,156,55,68,40,0,64,67,0,128,69,68,24,144,119,67,0,128,69,68,120,255,157,67,98,0,128,69,68,200,55,192,67,2,156,55,68,240,255,219,67,0,128,38,68,240,255,219,67,98,253,99,21,68,240,255,219,67,0,128,7,68,200,55,192,67,0,
			128,7,68,120,255,157,67,98,0,128,7,68,24,144,119,67,253,99,21,68,40,0,64,67,0,128,38,68,40,0,64,67,99,109,186,222,15,68,32,246,169,67,108,58,10,44,68,32,246,169,67,108,58,10,44,68,160,46,192,67,108,70,33,61,68,120,255,157,67,108,58,10,44,68,72,164,119,
			67,108,58,10,44,68,80,10,146,67,108,186,222,15,68,80,10,146,67,108,186,222,15,68,32,246,169,67,99,101,0,0 };

			Path path;
			path.loadPathFromData (pathData, sizeof (pathData));

			if(id == "undo")
				path.applyTransform(AffineTransform::rotation(float_Pi));

			return path;
		}

		void resized() override
		{
			auto b = getLocalBounds().reduced(4);
			undo.setBounds(b.removeFromLeft(b.getWidth() / 2).reduced(3));
			redo.setBounds(b.reduced(3));

			undo.setEnabled(parent.rootUndoManager.canUndo());
			redo.setEnabled(parent.rootUndoManager.canRedo());

			undo.setTooltip(parent.rootUndoManager.getUndoDescription());
			redo.setTooltip(parent.rootUndoManager.getRedoDescription());
		}

		DspNetworkGraph& parent;
		HiseShapeButton undo, redo;
	} rootUndoButtons;

	DspNetworkGraph(DspNetwork* n);
	~DspNetworkGraph();

	bool keyPressed(const KeyPress& key) override;
	void handleAsyncUpdate() override;
	void rebuildNodes();
	void resizeNodes();
	void updateDragging(Point<int> position, bool copyNode);
	void finishDrag();
	void paint(Graphics& g) override;
	void resized() override;

	UndoManager rootUndoManager;

	void setCurrentRootNode(NodeBase* newRoot, bool useUndo=true, bool allowAnimation=true);

	NodeBase* getCurrentRootNode() const { return currentRootNode != nullptr ? currentRootNode.get() : network->getRootNode(); }

	bool isShowingRootNode() const { return getCurrentRootNode() == network->getRootNode(); }

	WeakReference<NodeBase> currentRootNode;

	template <class T> static void fillChildComponentList(Array<T*>& list, Component* c)
	{
		for (int i = 0; i < c->getNumChildComponents(); i++)
		{
			auto child = c->getChildComponent(i);

			if (auto typed = dynamic_cast<T*>(child))
			{
				bool isShowing = child->isVisible();
				Component* c = child;
				while (c != nullptr && isShowing)
				{
					isShowing &= c->isVisible();
					c = c->getParentComponent();
				}

				if (!isShowing)
					continue;

				list.add(typed);
			}

			fillChildComponentList(list, child);
		}
	}

	LambdaBroadcaster<NodeBase*> rootBroadcaster;

	struct BreadcrumbButton: public Component,
							 public SettableTooltipClient
	{
		BreadcrumbButton(NodeBase* n, bool isLast_):
		  node(n),
		  isLast(isLast_),
		  f(GLOBAL_BOLD_FONT())
		{
			setMouseCursor(MouseCursor::PointingHandCursor);

			icon = nf.createPath(node->getPath().id.toString());
			next = nf.createPath("next");

			auto w = f.getStringWidth(node->getName()) + 2 * 32 + 2 * UIValues::NodeMargin;
			setSize(w, 32);
			setRepaintsOnMouseActivity(true);
			setTooltip("Show " + node->getName() + " as root node");
		}

		void mouseDrag(const MouseEvent& e) override
		{
			ZoomableViewport::checkDragScroll(e, false);

			if(draggingIndex != -1)
			{
				auto ng = findParentComponentOfClass<DspNetworkGraph>();

				DynamicObject::Ptr details = new DynamicObject();

				details->setProperty(PropertyIds::Automated, false);
				details->setProperty(PropertyIds::ID, ng->network->getRootNode()->getId());
				details->setProperty(PropertyIds::ParameterId, ng->network->getParameterIdentifier(draggingIndex).toString());
				
				ng->startDragging(var(details.get()), this, ScaledImage(ModulationSourceBaseComponent::createDragImageStatic(false)));
				ng->repaint();
			}
				
		}

		void mouseUp(const MouseEvent& e) override
		{
			ZoomableViewport::checkDragScroll(e, true);

			auto g = findParentComponentOfClass<DspNetworkGraph>();

			if(draggingIndex != -1)
			{
				draggingIndex = -1;
				g->repaint();
				return;
			}
			
			auto n = node.get();

			MessageManager::callAsync([g, n]()
			{
				g->setCurrentRootNode(n);
			});
		}

		void paint(Graphics& g) override
		{
			auto nc = node->getColour();

			if(nc == Colours::transparentBlack)
				nc = Colour(0xFF999999);

			auto hover = isMouseOver();

			g.setColour(nc.withAlpha(hover ? 0.2f : 0.1f));

			auto area = getLocalBounds().toFloat();
			area.removeFromRight(area.getHeight());

			g.fillRect(area.reduced(5.0f));
			g.setColour(nc);
			g.drawRoundedRectangle(area.reduced(2.0f), 3.0f, 2.0f);

			auto n = node->getName();

			float alpha = 0.5f;

			if(isMouseOver())
				alpha += 0.1f;

			if(isMouseButtonDown())
				alpha += 0.3f;

			g.setColour(Colours::white.withAlpha(alpha));
			g.setFont(f);
			auto b = getLocalBounds().toFloat();
			b.removeFromLeft(b.getHeight());
			g.drawText(n, b, Justification::left);
			g.setColour(nc);
			g.fillPath(icon);

			if(!isLast)
			{
				g.setColour(Colours::white.withAlpha(0.2f));
				g.fillPath(next);
			}
		}

		void resized() override
		{
			auto b = getLocalBounds();

			getProperties().set("circleOffsetY", 5);
			getProperties().set("circleOffsetX", -1 * (b.getWidth() / 2) + UIValues::NodeMargin);

			nf.scalePath(icon, b.removeFromLeft(getHeight()).reduced(8).toFloat());
			nf.scalePath(next, b.removeFromRight(getHeight()).reduced(10).toFloat());
		}

		void setIsDraggingParameter(int parameterIndex)
		{
			draggingIndex = parameterIndex;
		}

		int draggingIndex = -1;

		Path icon;
		Path next;
		bool isLast;
		NodeComponentFactory nf;
		WeakReference<NodeBase> node;
		Font f;
	};

	OwnedArray<BreadcrumbButton> breadcrumbs;

	void rebuildBreadCrumbs()
	{
		breadcrumbs.clear();

		if(!isShowingRootNode())
		{
			auto n = getCurrentRootNode();

			while(n != nullptr)
			{
				
				auto b = new BreadcrumbButton(n, n == getCurrentRootNode());
				addAndMakeVisible(b);
				breadcrumbs.add(b);
				n = n->getParentNode();
			}
		}
	}

	void paintOverChildren(Graphics& g) override;

	void toggleProbeMode()
	{
		probeSelectionEnabled = !probeSelectionEnabled;

		auto ft = findParentComponentOfClass<FloatingTile>();

		if (!probeSelectionEnabled && !ft->isRootPopupShown())
		{
			auto obj = new DynamicObject();
			
			auto l = network->getListOfProbedParameters();

			for (auto p : l)
			{
				String key;
				key << p->parent->getId() << "." << p->getId();
				obj->setProperty(Identifier(key), p->getValue());
			}

			String s;
			s << "// Set the properties of this object to the parameter values\n";
			s << "var data = " << JSON::toString(var(obj)) << ";";


			auto n = new JSONEditor(s, new JavascriptTokeniser());

			n->setCompileCallback([this](const String& text, var& data)
			{
				ScopedPointer<JavascriptEngine> e = new JavascriptEngine();

				auto r = e->execute(text);

				data = e->getRootObjectProperties().getWithDefault("data", {});

				return r;
			}, false);

			n->setCallback([this](const var& d)
			{
				network->setParameterDataFromJSON(d);
				
			}, false);

			n->setEditable(true);
			n->setName("Edit Parameter List");
			n->setSize(600, 400);
			
			
			auto c = findParentComponentOfClass<WrapperWithMenuBar>()->actionButtons[3];
			
			ft->showComponentInRootPopup(n, c, c->getLocalBounds().getBottomRight());
		}

		repaint();
	}

	NodeComponent* getComponent(NodeBase::Ptr node);

	

	static Rectangle<float> getCircle(Component* c, bool getKnobCircle=true)
	{
		if (auto n = c->findParentComponentOfClass<DspNetworkGraph>())
		{
			float width = 6.0f;
			float height = 6.0f;
			float y = getKnobCircle ? 64.0f : (c->getHeight());

			float offsetY = (float)c->getProperties()["circleOffsetY"];
			float offsetX = (float)c->getProperties()["circleOffsetX"];

			y += offsetY;
			
			float circleX = c->getLocalBounds().toFloat().getWidth() / 2.0f - width / 2.0f;

			circleX += offsetX;

			Rectangle<float> circleBounds = { circleX, y, width, height };

			return n->getLocalArea(c, circleBounds.toNearestInt()).toFloat();
		}

		return {};
	};

	struct PeriodicRepainter : public PooledUIUpdater::SimpleTimer
	{
		PeriodicRepainter(DspNetworkGraph& p) :
			SimpleTimer(p.network->getScriptProcessor()->getMainController_()->getGlobalUIUpdater()),
			componentToUpdate(&p)
		{
			start();
		}
			

		void timerCallback() override
		{
			componentToUpdate->repaint();
		}

		Component* componentToUpdate;
	};

	void enablePeriodicRepainting(bool shouldBeEnabled)
	{
		if (shouldBeEnabled)
			periodicRepainter = new PeriodicRepainter(*this);
		else
			periodicRepainter = nullptr;
	}
	

	bool showCables = true;
	bool probeSelectionEnabled = false;

	bool setCurrentlyDraggedComponent(NodeComponent* n);

	struct DragOverlay : public Timer
	{
		DragOverlay(DspNetworkGraph& g) :
			parent(g)
		{};

		void timerCallback() override
		{
			float onDelta = JUCE_LIVE_CONSTANT_OFF(.1f);
			float offDelta = JUCE_LIVE_CONSTANT_OFF(.1f);

			if (enabled)
				alpha += onDelta;
			else
				alpha -= offDelta;

			if (alpha >= 1.0f || alpha <= 0.0f)
				stopTimer();

			alpha = jlimit(0.0f, 1.0f, alpha);

			parent.repaint();
		}

		void setEnabled(bool shouldBeEnabled)
		{
			if (shouldBeEnabled != enabled)
			{
				enabled = shouldBeEnabled;

				startTimer(30);
			}

			parent.repaint();
		}

		DspNetworkGraph& parent;
		bool enabled = false;
		float alpha = 0.0f;
	} dragOverlay;

	ValueTree dataReference;

	bool copyDraggedNode = false;

	bool showParameters = false;

	Point<float> lastMousePos;
	valuetree::RecursivePropertyListener cableRepainter;
	valuetree::RecursivePropertyListener lockListener;
	valuetree::ChildListener rebuildListener;
	valuetree::RecursivePropertyListener resizeListener;

	valuetree::RecursiveTypedChildListener macroListener;

	Component* externalDragComponent = nullptr;
	ScopedPointer<NodeComponent> root;

	ScopedPointer<PeriodicRepainter> periodicRepainter;

	WeakReference<NodeDropTarget> currentDropTarget;
	ScopedPointer<NodeComponent> currentlyDraggedComponent;

	ReferenceCountedObjectPtr<DspNetwork> network;

	JUCE_DECLARE_WEAK_REFERENCEABLE(DspNetworkGraph);
};


}

