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

#pragma once

namespace hise {
using namespace juce;



class MarkdownRenderer : public MarkdownParser,
						 public ViewportWithScrollCallback::Listener
{
public:

	struct NavigationAction : public UndoableAction
	{
		NavigationAction(MarkdownRenderer* renderer, const MarkdownLink& newLink);;

		bool perform() override;

		bool undo() override;

		float lastY = 0.0f;
		
		MarkdownLink lastLink;
		MarkdownLink currentLink;
		
		WeakReference<MarkdownRenderer> parent;
	};

	struct LayoutCache
	{
		void clear() { cachedLayouts.clear(); }

		const MarkdownLayout& getLayout(const AttributedString& s, float w);

	private:

		struct Layout
		{
			Layout(const AttributedString& s, float w);

			MarkdownLayout l;
			int64 hashCode;
			float width;

			JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Layout);
		};

		OwnedArray<Layout> cachedLayouts;

		JUCE_DECLARE_WEAK_REFERENCEABLE(LayoutCache);
	};

	struct ScopedScrollDisabler
	{
		ScopedScrollDisabler(MarkdownRenderer& p):
			parent(p),
			wasDisabled(parent.disableScrolling)
		{
			parent.disableScrolling = true;
		}

		~ScopedScrollDisabler()
		{
			parent.disableScrolling = wasDisabled;
		}

		MarkdownRenderer& parent;
		bool wasDisabled;
	};

	struct Listener
	{
		virtual ~Listener() {};
		virtual void markdownWasParsed(const Result& r) = 0;

		virtual void scrollToAnchor(float v) 
		{
			ignoreUnused(v);
		};

	private:

		JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
	};

	MarkdownRenderer(const String& text, LayoutCache* c = nullptr) :
		MarkdownParser(text),
		layoutCache(c),
		uncachedLayout({}, 0.0f)
	{
		history.add(markdownCode);
		historyIndex = 0;
	};

	~MarkdownRenderer();

	void draw(Graphics& g, Rectangle<float> totalArea, Rectangle<int> viewedArea = {}) const;

	float getHeightForWidth(float width, bool forceUpdate=false);
	void parse() override;
	bool canNavigate(bool back) const;
	void navigate(bool back);
	void jumpToCurrentAnchor();
	

	RectangleList<float> searchInContent(const String& searchString);

	void scrolled(Rectangle<int> visibleArea) override
	{
		currentY = (float)visibleArea.getY();
	}

	String getAnchorForY(int y) const;

	String getSelectionContent() const;

	void updateSelection(Rectangle<float> area);

	bool gotoLink(const MarkdownLink& url) override;

	bool gotoLinkFromMouseEvent(const MouseEvent& e, Rectangle<float> markdownBounds, const File& root);

	void addListener(Listener* l) { listeners.addIfNotAlreadyThere(l); }
	void removeListener(Listener* l) { listeners.removeAllInstancesOf(l); }
	
	void setTargetComponent(Component* newTarget);

	Component* getTargetComponent() const
	{
		return targetComponent.getComponent();
	}

	void updateHeight();

	void setChildComponentBounds(Rectangle<int> renderArea)
	{
		childArea = renderArea;
	}

	void updateCreatedComponents();

	bool navigateFromXButtons(const MouseEvent& e);

	UndoManager* getUndoManager() { return &undoManager; }

	bool disableScrolling = false;

    void scrollToY(float y);

private:

	float currentY;

	JUCE_DECLARE_WEAK_REFERENCEABLE(MarkdownRenderer);

	UndoManager undoManager;

	StringArray history;
	int historyIndex;

	WeakReference<LayoutCache> layoutCache;
	MarkdownLayout uncachedLayout;

	Array<Component::SafePointer<Component>> createdComponents;
	Component::SafePointer<Component> targetComponent;

	Array<WeakReference<Listener>> listeners;

	mutable bool firstDraw = true;
	float lastHeight = -1.0f;
	Rectangle<int> childArea;
	float lastWidth = -1.0f;
};

class SimpleMarkdownDisplay : public Component
{
public:

	struct InternalComp : public Component
	{
		InternalComp(SimpleMarkdownDisplay& parent_) :
			parent(parent_)
		{};

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();
			parent.r.draw(g, b);
		}

		void mouseMove(const MouseEvent& e) override
		{
			auto l = parent.r.getLinkForMouseEvent(e, getLocalBounds().toFloat());

			setMouseCursor(l.isValid() ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
		}

        void mouseDown(const MouseEvent& e) override;

		SimpleMarkdownDisplay& parent;
	};

	SimpleMarkdownDisplay();

	void setText(const String& text);

	void resized() override;

	MarkdownRenderer r;
	float totalHeight = 0.0;
	Viewport vp;
	InternalComp canvas;
    
    ScrollbarFader sf;
};

class MarkdownPreview : public Component,
	public MarkdownContentProcessor
{
public:

	enum MouseMode
	{
		Drag,
		Select,
		numMouseModes
	};

	enum EditingMenuCommands
	{
		EditCurrentPage = 1000,
		CreateMarkdownLink,
		CopyLink,
		RevealFile,
		DebugExactContent,
		numEditingMenuCommands
	};

	enum class ViewOptions
	{
		Naked = 0b00000000,
		Topbar = 0b10000000,
		Toc = 0b01000000,
		Back = 0b00000010,
		Search = 0b00000100,
		Edit = 0b00001000,
		ColourScheme = 0b10010000,
		Everything = 0b11111111
	};

	MarkdownPreview(MarkdownDatabaseHolder& holder);

	~MarkdownPreview();

	void databaseWasRebuild() override
	{
		rootDirectory = getHolder().getDatabaseRootDirectory();
	}

	int getPreferredWidth() const {
		return roundToInt((float)MarkdownParser::DefaultLineWidth * internalComponent.scaleFactor) + viewport.getScrollBarThickness() + 80;
	}

	void resolversUpdated() override;

	virtual void editCurrentPage(const MarkdownLink& link, bool showExactContent = false) {};

	void addEditingMenuItems(PopupMenu& m);

	bool performPopupMenuForEditingIcons(int result, const MarkdownLink& linkToUse);

	void setNavigationShown(bool shouldBeShown);

	virtual void showDoc() { ; };

	virtual void enableEditing(bool shouldBeEnabled) {};

	bool editingEnabled = false;

	void mouseDown(const MouseEvent& e) override;

	bool keyPressed(const KeyPress& k);

	void setMouseMode(MouseMode newMode);

	void setNewText(const String& newText, const File& f, bool scrollToStart=true);

	struct InternalComponent : public Component,
		public MarkdownRenderer::Listener,
		public juce::SettableTooltipClient
	{
		InternalComponent(MarkdownPreview& parent);

		~InternalComponent();

		int getTextHeight();

		void setNewText(const String& s, const File& f, bool scrollToStart=true);

		void markdownWasParsed(const Result& r) override;

		void mouseDown(const MouseEvent& e) override;

		void mouseDrag(const MouseEvent& e) override;

		void mouseUp(const MouseEvent& e) override;

		void mouseEnter(const MouseEvent&) override
		{
			if (enableSelect)
				setMouseCursor(MouseCursor(MouseCursor::IBeamCursor));
			else
				setMouseCursor(MouseCursor(MouseCursor::DraggingHandCursor));
		}

		void mouseExit(const MouseEvent&) override
		{
			setMouseCursor(MouseCursor::NormalCursor);
		}

		void mouseMove(const MouseEvent& e) override;

		void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& details) override;

        void mouseMagnify (const juce::MouseEvent& e, float sf) override
        {
            auto newScaleFactor = scaleFactor * sf;
            setScaleFactor(newScaleFactor);
        }
        
        void setScaleFactor(float newScaleFactor);
        
		void scrollToAnchor(float v) override;

		void scrollToSearchResult(Rectangle<float> currentSelection);

		void paint(Graphics & g) override;

		void resized() override
		{
			renderer.updateCreatedComponents();
			renderer.updateHeight();
			renderer.jumpToCurrentAnchor();
		}

		MarkdownPreview& parent;
		MarkdownRenderer& renderer;

		String errorMessage;

		MarkdownLayout::StyleData styleData;

		Rectangle<float> clickedLink;

		Rectangle<float> currentSearchResult;

		Rectangle<int> currentLasso;
		bool enableSelect = false;
		float scaleFactor = 1.0f;
	};


	struct CustomViewport : public ViewportWithScrollCallback
	{
		CustomViewport(MarkdownPreview& parent_) :
			parent(parent_)
		{
            sf.addScrollBarToAnimate(getVerticalScrollBar());
            setScrollBarThickness(13);
			//setScrollOnDragEnabled(true);
		}

		void visibleAreaChanged(const Rectangle<int>& newVisibleArea) override
		{
			auto s = parent.renderer.getAnchorForY(newVisibleArea.getY());
			parent.toc.setCurrentAnchor(s);

			ViewportWithScrollCallback::visibleAreaChanged(newVisibleArea);
		}

        ScrollbarFader sf;
		MarkdownPreview& parent;
	};


	struct Topbar : public Component,
		public ButtonListener,
		public LabelListener,
		public TextEditor::Listener,
		public MarkdownDatabaseHolder::DatabaseListener,
		public KeyListener
	{
		explicit Topbar(MarkdownPreview& parent_);

		~Topbar() override;

		void databaseWasRebuild() override;

		struct TopbarPaths : public PathFactory
		{
			String getId() const override { return "Markdown Preview"; }

			Path createPath(const String& id) const override;
		};

		MarkdownDataBase* database = nullptr;

		MarkdownPreview& parent;
		TopbarPaths factory;
		HiseShapeButton tocButton;
		HiseShapeButton homeButton;
		HiseShapeButton backButton;
		HiseShapeButton forwardButton;
		HiseShapeButton lightSchemeButton;
		HiseShapeButton selectButton;
		HiseShapeButton refreshButton;
		HiseShapeButton editButton;
		Label searchBar;
		Path searchPath;


		struct SearchResults : public Component,
			public Timer,
			public Button::Listener
		{
			struct ItemComponent : public Component
			{
				ItemComponent(MarkdownDataBase::Item i, const MarkdownLayout::StyleData& l);

				void mouseEnter(const MouseEvent&) override;
				void mouseExit(const MouseEvent&) override;
				void mouseDown(const MouseEvent& e) override;
				void gotoLink();
				void mouseUp(const MouseEvent& e) override;
				void paint(Graphics& g) override;
				int calculateHeight(int width);

				bool hover = false;
				bool down = false;

				Rectangle<int> kBounds;
				Rectangle<int> starBounds;
				const MarkdownLayout::StyleData& style;
				MarkdownRenderer p;
				int height = 0;
				MarkdownDataBase::Item item;
				bool isFuzzyMatch = false;

				JUCE_DECLARE_WEAK_REFERENCEABLE(ItemComponent);
			};

			SearchResults(Topbar& parent_);

			void buttonClicked(Button* b) override;
			void resized() override;
			void refreshTextResultLabel();
			void timerCallback() override;
			void gotoSelection();
			void selectNextItem(bool inc);
			void rebuildItems();
			void setSearchString(const String& s);
			void paint(Graphics& g) override;

			String searchString;
			Array<ItemComponent*> displayedItems;
			OwnedArray<ItemComponent> exactMatches;
			OwnedArray<ItemComponent> fuzzyMatches;

			TextButton textSearchButton;

			Viewport viewport;
			Component content;

			DropShadower shadower;

			Topbar::TopbarPaths factory;

			HiseShapeButton nextButton;
			HiseShapeButton prevButton;
			Label textSearchResults;
			int currentIndex = -1;
			int itemIndex = 0;
			WeakReference<ItemComponent> currentSelection;
			RectangleList<float> currentSearchResultPositions;

			Topbar& parent;

			String lastText;
			File lastFile;
		};

		void labelTextChanged(Label* labelThatHasChanged) override;

		void textEditorTextChanged(TextEditor& ed);

		void editorShown(Label*, TextEditor& ed) override;

		void showPopup();

		void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;

		void textEditorEscapeKeyPressed(TextEditor&);

		void editorHidden(Label*, TextEditor& ed) override;

		void updateNavigationButtons()
		{
			//forwardButton.setEnabled(parent.renderer.canNavigate(false));
			//backButton.setEnabled(parent.renderer.canNavigate(true));
		}

		void buttonClicked(Button* b) override;

		bool keyPressed(const KeyPress& key, Component*) override;

		void resized() override;

		void paint(Graphics& g) override;
	};


	class MarkdownDatabaseTreeview : public Component,
		public MarkdownDatabaseHolder::DatabaseListener
	{
	public:

		struct Item : public juce::TreeViewItem,
			public juce::KeyListener
		{
			Item(MarkdownDataBase::Item item_, MarkdownPreview& previewParent_);
			~Item() override;

			bool keyPressed(const KeyPress& key, Component*) override;
			bool mightContainSubItems() override;

			String getUniqueName() const override;

			void itemOpennessChanged(bool isNowOpen) override;
			MarkdownParser* getCurrentParser();
			Item* selectIfURLMatches(const MarkdownLink& url);
			void gotoLink();
			void itemClicked(const MouseEvent& e) override;

			bool canBeSelected() const override;

			int getItemHeight() const override;

			int getItemWidth() const override;

			void paintItem(Graphics& g, int width, int height) override;

			MarkdownDataBase::Item item;
			MarkdownPreview& previewParent;
		};

		MarkdownDatabaseTreeview(MarkdownPreview& parent_);
		~MarkdownDatabaseTreeview() override;

		void setBgColour(Colour c);
		void scrollToLink(const MarkdownLink& l);
		void databaseWasRebuild() override;
		void openAll(TreeViewItem* item);
		void closeAllExcept(TreeViewItem* item, Array<TreeViewItem*> path);
		bool closeIfNoMatch(TreeViewItem* item, const MarkdownLink& id);
		void setCurrentAnchor(const String& s);
		int getPreferredWidth() const;
		void paint(Graphics& g) override;
		void resized() override;

		Colour bgColour = Colour(0xFF222222);

		int fixWidth = -1;
		juce::TreeView tree;
		ScopedPointer<Item> rootItem;
		MarkdownPreview& parent;
		MarkdownDataBase* db = nullptr;
		MarkdownLink pendingLink;
	};


	void setStyleData(MarkdownLayout::StyleData d)
	{
		internalComponent.styleData = d;
	}

	void paint(Graphics& g) override
	{
		g.fillAll(internalComponent.styleData.backgroundColour);
	}

	void setViewOptions(int newViewOptions)
	{
		currentViewOptions = newViewOptions;
	}

	bool shouldDisplay(ViewOptions option)
	{
		return currentViewOptions & (int)option;
	}

	void resized() override;

	bool navigationShown = true;

	LookAndFeel_V3 laf;

	MarkdownRenderer::LayoutCache layoutCache;
	MarkdownRenderer renderer;

	MarkdownDatabaseTreeview toc;
	CustomViewport viewport;
	InternalComponent internalComponent;
	Topbar topbar;
	File rootDirectory;
	ScopedPointer<Topbar::SearchResults> currentSearchResults;
	int currentViewOptions = (int)ViewOptions::Everything;
};


}
