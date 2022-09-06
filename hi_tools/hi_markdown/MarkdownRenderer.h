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
		NavigationAction(MarkdownRenderer* renderer, const MarkdownLink& newLink):
			currentLink(newLink),
			parent(renderer)
		{
			lastLink = renderer->getLastLink();
			lastY = renderer->currentY;
		};

		bool perform()
		{
			if (parent != nullptr)
			{
				parent->MarkdownParser::gotoLink(currentLink);
				return true;
			}

			return false;
		}

		bool undo() override
		{
			if (parent != nullptr)
			{
				parent->MarkdownParser::gotoLink(lastLink);
				parent->scrollToY(lastY);
				return true;
			}

			return false;
		}

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

	~MarkdownRenderer()
	{
		setTargetComponent(nullptr);
	}

	void draw(Graphics& g, Rectangle<float> totalArea, Rectangle<int> viewedArea = {}) const
	{
		for (auto* e : elements)
		{
			auto heightToUse = e->getHeightForWidthCached(totalArea.getWidth());
			auto topMargin = e->getTopMargin();
			totalArea.removeFromTop((float)topMargin);
			auto ar = totalArea.removeFromTop(heightToUse);

			if (firstDraw || viewedArea.isEmpty() || ar.toNearestInt().intersects(viewedArea))
				e->draw(g, ar);
		}

		firstDraw = false;
	}

	float getHeightForWidth(float width, bool forceUpdate=false);

	void parse() override;

	bool canNavigate(bool back) const
	{
		if (back)
			return undoManager.canUndo();
		else
			return undoManager.canRedo();
	}

	void navigate(bool back)
	{
		if (back)
			undoManager.undo();
		else
			undoManager.redo();
	}

	void jumpToCurrentAnchor();

	

	RectangleList<float> searchInContent(const String& searchString)
	{
		RectangleList<float> positions;

		float y = 0.0f;

		for (auto e : elements)
		{
			e->searchInContent(searchString);

			y += e->getTopMargin();

			for (auto r : e->searchResults)
				positions.add(r.translated(0.0f, y));

			y += e->getLastHeight();
		}

		return positions;
	}

	void scrolled(Rectangle<int> visibleArea) override
	{
		currentY = (float)visibleArea.getY();
	}

	String getAnchorForY(int y) const;

	String getSelectionContent() const
	{
		String s;

		for (auto e : elements)
		{
			if (e->selected)
			{
				s << e->getTextToCopy() << "\n";
			}
		}

		return s;
	}

	void updateSelection(Rectangle<float> area)
	{
		Range<float> yRange(area.getY(), area.getBottom());

		float y = 0.0f;

		for (auto e : elements)
		{
			float h = e->getTopMargin() + e->getLastHeight();

			e->setSelected(Range<float>(y, y + h).intersects(yRange));

			y += h;
		}
	}

	bool gotoLink(const MarkdownLink& url) override
	{
		undoManager.beginNewTransaction("New Link");
		return undoManager.perform(new NavigationAction(this, url));
	}

	void addListener(Listener* l) { listeners.addIfNotAlreadyThere(l); }
	void removeListener(Listener* l) { listeners.removeAllInstancesOf(l); }

#if 0
	const MarkdownLayout& getTextLayoutForString(const AttributedString& s, float width)
	{
		if (layoutCache.get() != nullptr)
			return layoutCache->getLayout(s, width);

		uncachedLayout = { s, width };
		return  uncachedLayout;
	}
#endif


	void setTargetComponent(Component* newTarget)
	{
		if (targetComponent == newTarget)
			return;

		if (auto existing = targetComponent.getComponent())
		{
			for (auto e : elements)
			{
				if (auto c = e->createComponent(existing->getWidth()))
					existing->removeChildComponent(c);
			}
		}

		targetComponent = newTarget;
	}

	Component* getTargetComponent() const
	{
		return targetComponent.getComponent();
	}

	void updateHeight()
	{
		float y = currentY;

		getHeightForWidth(lastWidth, true);
		
		for (auto l : listeners)
		{
			if (l != nullptr)
				l->markdownWasParsed(getParseResult());
		}

		scrollToY(y);

	}

	void setChildComponentBounds(Rectangle<int> renderArea)
	{
		childArea = renderArea;
	}

	void updateCreatedComponents()
	{
		if (targetComponent == nullptr)
			return;

		if (targetComponent->getWidth() == 0)
			return;

		float y = (float)childArea.getY();

		auto wToUse = childArea.getWidth();

		if (wToUse == 0)
			wToUse = (int)targetComponent->getWidth();

		for (auto e : elements)
		{
			y += e->getTopMargin();

			if (auto c = e->createComponent(wToUse))
			{
				if (c->getParentComponent() == nullptr)
					targetComponent->addAndMakeVisible(c);

				jassert(c->getWidth() > 0);
				jassert(c->getHeight() > 0);

				c->setTopLeftPosition(childArea.getX(), (int)y);

				y += (float)e->getLastHeight();
			}
			else
			{
				y += e->getLastHeight();
			}
		}

		
	}

	bool navigateFromXButtons(const MouseEvent& e)
	{
		if (e.mods.isX1ButtonDown())
		{
			navigate(true);
			return true;
		}
		if (e.mods.isX2ButtonDown())
		{
			navigate(false);
			return true;
		}

		return false;
	}

	UndoManager* getUndoManager() { return &undoManager; }

	bool disableScrolling = false;

    void scrollToY(float y)
    {
        if (disableScrolling)
            return;

        currentY = y;

        WeakReference<MarkdownRenderer> r = this;

        auto f = [r, y]()
        {
            if (r != nullptr)
            {
                for (auto l : r->listeners)
                {
                    if (l.get() != nullptr)
                        l->scrollToAnchor(y);
                }
            }
            
        };

        MessageManager::callAsync(f);
    }
    
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

		SimpleMarkdownDisplay& parent;
	};

	SimpleMarkdownDisplay() :
		r("", nullptr),
		canvas(*this)
	{
		vp.setViewedComponent(&canvas, false);
		addAndMakeVisible(vp);
		vp.setScrollOnDragEnabled(true);
	}

	void setText(const String& text)
	{
		r.setNewText(text);
		r.setTargetComponent(&canvas);
		

		resized();
		r.updateCreatedComponents();
	}

	void resized() override
	{
		auto b = getLocalBounds();
		vp.setBounds(b);
		
		auto w = b.getWidth() - vp.getScrollBarThickness();

		totalHeight = r.getHeightForWidth(w, true);

		canvas.setSize(w, totalHeight);
		repaint();
	}

	MarkdownRenderer r;
	float totalHeight = 0.0;
	Viewport vp;
	InternalComp canvas;
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

	void resolversUpdated() override
	{
		renderer.clearResolvers();

		for (auto l : linkResolvers)
			renderer.setLinkResolver(l->clone(&renderer));

		for (auto ip : imageProviders)
			renderer.setImageProvider(ip->clone(&renderer));
	}

	virtual void editCurrentPage(const MarkdownLink& link, bool showExactContent = false) {};

	void addEditingMenuItems(PopupMenu& m)
	{
		m.addItem(EditingMenuCommands::CopyLink, "Copy link");

		if (editingEnabled)
		{
			m.addSectionHeader("Editing Tools");
			m.addItem(EditingMenuCommands::EditCurrentPage, "Edit this page in new editor tab");
			m.addItem(EditingMenuCommands::CreateMarkdownLink, "Create markdown formatted link", true);
			m.addItem(EditingMenuCommands::RevealFile, "Show file");
			m.addItem(EditingMenuCommands::DebugExactContent, "Debug current content");
		}
	}

	bool performPopupMenuForEditingIcons(int result, const MarkdownLink& linkToUse)
	{
		if (result == EditingMenuCommands::EditCurrentPage)
		{
			editCurrentPage(linkToUse);
			return true;
		}
		if (result == EditingMenuCommands::CreateMarkdownLink)
		{
			SystemClipboard::copyTextToClipboard(linkToUse.toString(MarkdownLink::FormattedLinkMarkdown));
			return true;
		}
		if (result == EditingMenuCommands::CopyLink)
		{
			SystemClipboard::copyTextToClipboard(linkToUse.toString(MarkdownLink::Everything));
			return true;
		}
		if (result == EditingMenuCommands::RevealFile)
		{
			auto f = linkToUse.getDirectory({});

			if (f.isDirectory())
			{
				f.revealToUser();
				return true;
			}

			f = linkToUse.getMarkdownFile({});

			if (f.existsAsFile())
			{
				f.revealToUser();
				return true;
			}
		}
		if (result == EditingMenuCommands::DebugExactContent)
		{
			editCurrentPage({}, true);
			return true;
		}

		return false;
	}

	void setNavigationShown(bool shouldBeShown)
	{
		navigationShown = shouldBeShown;
	}

	virtual void showDoc() { ; };

	virtual void enableEditing(bool shouldBeEnabled) {};

	bool editingEnabled = false;

	void mouseDown(const MouseEvent& e) override
	{
		currentSearchResults = nullptr;

		if (renderer.navigateFromXButtons(e))
			return;

		if (e.mods.isRightButtonDown())
		{
			PopupLookAndFeel plaf;
			PopupMenu m;
			m.setLookAndFeel(&plaf);

			addEditingMenuItems(m);

			int result = m.show();

			if (performPopupMenuForEditingIcons(result, renderer.getLastLink()))
				return;
		}
	}

	bool keyPressed(const KeyPress& k);

	void setMouseMode(MouseMode newMode)
	{
		if (newMode == Drag)
		{
			viewport.setScrollOnDragEnabled(true);
			internalComponent.enableSelect = false;
		}
		else
		{
			viewport.setScrollOnDragEnabled(false);
			internalComponent.enableSelect = true;
		}
	}

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

		Topbar(MarkdownPreview& parent_) :
			parent(parent_),
			tocButton("TOC", this, factory),
			homeButton("Home", this, factory),
			backButton("Back", this, factory),
			forwardButton("Forward", this, factory),
			searchPath(factory.createPath("Search")),
			lightSchemeButton("Sun", this, factory, "Night"),
			selectButton("Select", this, factory, "Drag"),
			refreshButton("Rebuild", this, factory),
			editButton("Edit", this, factory, "Lock")
		{
			parent.getHolder().addDatabaseListener(this);
			selectButton.setToggleModeWithColourChange(true);
			editButton.setToggleModeWithColourChange(true);

			addAndMakeVisible(homeButton);
			addAndMakeVisible(tocButton);
			addAndMakeVisible(backButton);
			addAndMakeVisible(forwardButton);
			addAndMakeVisible(lightSchemeButton);
			addAndMakeVisible(searchBar);
			addAndMakeVisible(selectButton);
			addAndMakeVisible(editButton);
			addAndMakeVisible(refreshButton);
			lightSchemeButton.setClickingTogglesState(true);

			const auto& s = parent.internalComponent.styleData;

			searchBar.setColour(Label::backgroundColourId, Colour(0x22000000));
			searchBar.setFont(s.getFont());
			searchBar.setEditable(true);
			searchBar.setColour(Label::textColourId, Colours::white);
			searchBar.setColour(Label::ColourIds::textWhenEditingColourId, Colours::white);
			searchBar.setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
			searchBar.setColour(TextEditor::ColourIds::highlightColourId, Colour(SIGNAL_COLOUR));
			searchBar.setColour(TextEditor::ColourIds::highlightedTextColourId, Colours::black);
			searchBar.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));
			searchBar.addListener(this);
			databaseWasRebuild();
		}

		~Topbar()
		{
			parent.getHolder().removeDatabaseListener(this);
		}

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
				ItemComponent(MarkdownDataBase::Item i, const MarkdownLayout::StyleData& l) :
					item(i),
					p(i.description),
					style(l)
				{
					p.parse();
					setInterceptsMouseClicks(true, true);
				}

				void mouseEnter(const MouseEvent&) override
				{
					hover = true;
					setMouseCursor(MouseCursor(MouseCursor::PointingHandCursor));
					repaint();
				}

				void mouseExit(const MouseEvent&) override
				{
					hover = false;
					setMouseCursor(MouseCursor(MouseCursor::NormalCursor));
					repaint();
				}

				void mouseDown(const MouseEvent& e) override
				{
					down = true;
					repaint();

					if (e.mods.isRightButtonDown())
					{
						PopupLookAndFeel plaf;
						PopupMenu m;
						m.setLookAndFeel(&plaf);

						auto mp = findParentComponentOfClass<MarkdownPreview>();
						mp->addEditingMenuItems(m);

						int result = m.show();

						if (mp->performPopupMenuForEditingIcons(result, item.url))
							return;
					}
				}

				void gotoLink()
				{
					if (auto mp = findParentComponentOfClass<MarkdownPreview>())
					{
						auto& r = mp->renderer;

						r.gotoLink(item.url.withRoot(mp->rootDirectory, true));

						auto f2 = [mp]()
						{
							mp->currentSearchResults = nullptr;
							mp->topbar.searchBar.setText("", dontSendNotification);
						};

						MessageManager::callAsync(f2);
					}
				}

				void mouseUp(const MouseEvent& e) override
				{
					down = false;
					repaint();

					if (!e.mouseWasDraggedSinceMouseDown())
						gotoLink();
				}

				void paint(Graphics& g) override
				{
					g.fillAll(Colours::grey.withAlpha(down ? 0.6f : (hover ? 0.3f : 0.1f)));

					g.setColour(item.c);

					g.fillRect(0.0f, 0.0f, 3.0f, (float)getHeight());

					auto ar = getLocalBounds();

					auto f = GLOBAL_BOLD_FONT();

					g.setColour(Colours::black.withAlpha(0.1f));
					g.fillRect(kBounds);

					g.setFont(f);
					g.setColour(Colours::white);

					ar.removeFromLeft(kBounds.getWidth());

					g.drawText(item.keywords[0], kBounds.toFloat(), Justification::centred);

					if (!starBounds.isEmpty())
					{
						ar.removeFromLeft(starBounds.getWidth());

						g.setColour(item.c);

						Path starPath;
						starPath.addStar(starBounds.toFloat().getCentre(), 5, 5.0f, 10.0f);
						g.fillPath(starPath);
					}



					p.draw(g, ar.toFloat().reduced(5.0f).translated(0.0f, -5.0f));

					if (isFuzzyMatch)
						g.fillAll(Colours::grey.withAlpha(0.3f));
				}



				int calculateHeight(int width)
				{
					kBounds = { 0, 0, GLOBAL_BOLD_FONT().getStringWidth(item.keywords[0]) + 20, 0 };

					starBounds = {};


					if (height == 0)
					{
						height = (int)p.getHeightForWidth((float)(width - 10.0f - kBounds.getWidth() - starBounds.getWidth()));
					}

					kBounds.setHeight(height);
					starBounds.setHeight(height);

					return height;
				}

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

			SearchResults(Topbar& parent_) :
				parent(parent_),
				nextButton("Forward", this, factory),
				prevButton("Back", this, factory),
				shadower(DropShadow(Colours::black.withAlpha(0.5f), 10, {}))
			{
				addAndMakeVisible(nextButton);
				addAndMakeVisible(prevButton);
				addAndMakeVisible(textSearchResults);
				textSearchResults.setEditable(false);
				textSearchResults.setColour(Label::backgroundColourId, Colours::red.withSaturation(0.3f));
				textSearchResults.setFont(parent.parent.internalComponent.styleData.getFont());
				addAndMakeVisible(viewport);
				viewport.setViewedComponent(&content, false);
				viewport.setScrollOnDragEnabled(true);
				shadower.setOwner(this);
			}


			void buttonClicked(Button* b) override
			{
				if (b == &nextButton)
				{
					currentIndex++;

					if (currentIndex >= currentSearchResultPositions.getNumRectangles())
						currentIndex = 0;


				}
				if (b == &prevButton)
				{
					currentIndex--;

					if (currentIndex == -1)
						currentIndex = currentSearchResultPositions.getNumRectangles() - 1;
				}

				setSize(getWidth(), 32);



				parent.parent.internalComponent.scrollToSearchResult(currentSearchResultPositions.getRectangle(currentIndex));

				refreshTextResultLabel();
			}

			void resized() override
			{
				auto ar = getLocalBounds();

				if (currentSearchResultPositions.isEmpty())
				{
					nextButton.setVisible(false);
					prevButton.setVisible(false);
					textSearchResults.setVisible(false);
				}
				else
				{
					nextButton.setVisible(true);
					prevButton.setVisible(true);
					textSearchResults.setVisible(true);

					auto top = ar.removeFromTop(32);

					nextButton.setBounds(top.removeFromRight(32).reduced(6));
					prevButton.setBounds(top.removeFromRight(32).reduced(6));
					textSearchResults.setBounds(top);
				}

				viewport.setBounds(ar);
			}

			void refreshTextResultLabel()
			{
				if (!currentSearchResultPositions.isEmpty())
				{
					String s;

					s << "Search in current page:" << String(currentIndex + 1) << "/" << String(currentSearchResultPositions.getNumRectangles());

					textSearchResults.setText(s, dontSendNotification);
				}
				else
					textSearchResults.setText("No matches", dontSendNotification);
			}

			void timerCallback() override
			{
				currentSearchResultPositions = parent.parent.renderer.searchInContent(searchString);

				refreshTextResultLabel();

				parent.parent.repaint();

				int textSearchOffset = currentSearchResultPositions.isEmpty() ? 0 : 32;

				rebuildItems();

				if (viewport.getViewedComponent()->getHeight() > 350)
				{
					setSize(getWidth(), 350 + textSearchOffset);
				}
				else
				{
					setSize(getWidth(), viewport.getViewedComponent()->getHeight() + textSearchOffset);
				}

				stopTimer();
			}

			void gotoSelection()
			{
				if (currentSelection != nullptr)
				{
					currentSelection->gotoLink();
				}
			}

			void selectNextItem(bool inc)
			{
				if (currentSelection == nullptr)
				{
					itemIndex = 0;
				}
				else
				{
					if (inc)
					{
						itemIndex++;

						if (itemIndex >= displayedItems.size())
							itemIndex = 0;
					}
					else
					{
						itemIndex--;

						if (itemIndex < 0)
							itemIndex = displayedItems.size();
					}
				}

				if ((currentSelection = displayedItems[itemIndex]))
				{
					for (auto s : displayedItems)
					{
						s->hover = s == currentSelection.get();
						s->repaint();
					}

					auto visibleArea = viewport.getViewArea();

					if (!visibleArea.contains(currentSelection->getPosition()))
					{
						if (currentSelection->getY() > visibleArea.getBottom())
						{
							auto y = currentSelection->getBottom() - visibleArea.getHeight();

							viewport.setViewPosition(0, y);
						}
						else
						{
							viewport.setViewPosition(0, currentSelection->getY());
						}
					}
				}
			}

			void rebuildItems()
			{
				if (parent.database == nullptr)
					return;

				if (searchString.isEmpty())
				{
					displayedItems.clear();
					exactMatches.clear();
					fuzzyMatches.clear();

					content.setSize(viewport.getMaximumVisibleWidth(), 20);
					return;
				}


				auto allItems = parent.database->getFlatList();

				if (searchString.startsWith("/"))
				{
					displayedItems.clear();
					exactMatches.clear();
					fuzzyMatches.clear();

					MarkdownLink linkURL = { parent.parent.rootDirectory, searchString };

					MarkdownDataBase::Item linkItem;

					for (auto item : allItems)
					{
						if (item.url == linkURL)
						{
							linkItem = item;
							break;
						}
					}

					if (linkItem)
					{
						ScopedPointer<ItemComponent> newItem(new ItemComponent(linkItem, parent.parent.internalComponent.styleData));

						displayedItems.add(newItem);
						exactMatches.add(newItem);
						content.addAndMakeVisible(newItem.release());
					}
				}
				else
				{
					MarkdownDataBase::Item::PrioritySorter sorter(searchString);

					auto sorted = sorter.sortItems(allItems);

					displayedItems.clear();
					exactMatches.clear();
					fuzzyMatches.clear();

					for (const auto& item : sorted)
					{
						int matchLevel = item.fits(searchString);

						if (matchLevel > 0)
						{
							ScopedPointer<ItemComponent> newItem(new ItemComponent(item, parent.parent.internalComponent.styleData));

							if (matchLevel == 1)
							{
								if (exactMatches.size() < 50)
								{
									content.addAndMakeVisible(newItem);
									exactMatches.add(newItem.release());
								}
							}
							else
							{
								if (fuzzyMatches.size() < 10)
								{
									content.addAndMakeVisible(newItem);
									newItem->isFuzzyMatch = true;
									fuzzyMatches.add(newItem.release());
								}
							}

						}
					}


				}

				for (auto i : exactMatches)
					displayedItems.add(i);

				for (auto i : fuzzyMatches)
					displayedItems.add(i);

				content.setSize(viewport.getMaximumVisibleWidth(), 20);

				int y = 0;

				for (auto d : displayedItems)
				{
					auto h = d->calculateHeight(content.getWidth());

					d->setBounds(0, y, content.getWidth(), h);
					y += h;

					if (h == 0)
						continue;

					y += 2;

				}

				content.setSize(content.getWidth(), y);
			}

			void setSearchString(const String& s)
			{
				searchString = s;

				startTimer(200);
				itemIndex = 0;
			}

			void paint(Graphics& g) override
			{
				g.fillAll(Colour(0xFF333333));
				g.fillAll(Colours::black.withAlpha(0.1f));
			}


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

		bool keyPressed(const KeyPress& key, Component*) override
		{
			if (key == KeyPress('f') && key.getModifiers().isCommandDown())
			{
				showPopup();
				return true;
			}
			if (key == KeyPress::upKey)
			{
				if (parent.currentSearchResults != nullptr)
					parent.currentSearchResults->selectNextItem(false);

				return true;
			}
			else if (key == KeyPress::downKey)
			{
				if (parent.currentSearchResults != nullptr)
					parent.currentSearchResults->selectNextItem(true);

				return true;
			}
			else if (key == KeyPress::returnKey)
			{
				if (searchBar.getText(true).startsWith("/"))
				{

					parent.renderer.gotoLink({ parent.rootDirectory, searchBar.getText(true) });
					searchBar.hideEditor(false);
					searchBar.setText("", dontSendNotification);
					parent.currentSearchResults = nullptr;
					return true;
				}

				if (parent.currentSearchResults != nullptr)
					parent.currentSearchResults->gotoSelection();

				return true;
			}
			else if (key == KeyPress::tabKey)
			{

				if (parent.currentSearchResults != nullptr)
					parent.currentSearchResults->nextButton.triggerClick();

				return true;
			}

			return false;

		}

		void resized() override;

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF444444));
			//g.fillAll(Colours::white.withAlpha(0.05f));
			g.setColour(Colours::white.withAlpha(0.7f));
			g.fillPath(searchPath);
		}

	};


	class MarkdownDatabaseTreeview : public Component,
		public MarkdownDatabaseHolder::DatabaseListener
	{
	public:

		struct Item : public juce::TreeViewItem,
			public juce::KeyListener
		{
			Item(MarkdownDataBase::Item item_, MarkdownPreview& previewParent_) :
				TreeViewItem(),
				item(item_),
				previewParent(previewParent_)
			{
				previewParent_.toc.tree.addKeyListener(this);
			}

			~Item()
			{
				previewParent.toc.tree.removeKeyListener(this);
			}

			bool keyPressed(const KeyPress& key, Component*) override
			{
				if (key.getKeyCode() == KeyPress::returnKey)
				{
					gotoLink();
					return true;
				}

				return false;
			}

			bool mightContainSubItems() override { return item.hasChildren(); }

			String getUniqueName() const override { return item.url.toString(MarkdownLink::UrlFull); }

			void itemOpennessChanged(bool isNowOpen) override
			{
				if (item.isAlwaysOpen && !isNowOpen)
					return;

				clearSubItems();

				if (isNowOpen)
				{
					for (auto c : item)
					{
						if (c.tocString.isEmpty())
							continue;

						auto i = new Item(c, previewParent);

						addSubItem(i);

						auto currentLink = previewParent.renderer.getLastLink();

						const bool open = c.isAlwaysOpen || currentLink.isChildOf(c.url);

						if (open)
							i->setOpen(true);


					}

				}

				//previewParent.resized();
			}

			MarkdownParser* getCurrentParser()
			{
				return &previewParent.renderer;
			}



			Item* selectIfURLMatches(const MarkdownLink& url)
			{
				if (item.url == url)
				{
					return this;
				}

				for (int i = 0; i < getNumSubItems(); i++)
				{
					if (auto it = dynamic_cast<Item*>(getSubItem(i))->selectIfURLMatches(url))
						return it;
				}

				return nullptr;
			}

			void gotoLink()
			{
				if (auto p = getCurrentParser())
				{
					previewParent.currentSearchResults = nullptr;

					previewParent.renderer.gotoLink(item.url.withRoot(previewParent.rootDirectory, true));

#if 0
					auto link = item.url.upToFirstOccurrenceOf("#", false, false);

					if (p->getLastLink(true, false) != link)
						p->gotoLink(item.url);

					auto anchor = item.url.fromFirstOccurrenceOf("#", true, false);

					if (anchor.isNotEmpty())
					{
						auto mp = &previewParent;

						auto it = this;

						auto f2 = [mp, anchor, it]()
						{
							if (anchor.isNotEmpty())
								mp->renderer.gotoLink(anchor);

							it->setSelected(true, true);
						};

						MessageManager::callAsync(f2);
					}
#endif
				}
			}

			void itemClicked(const MouseEvent& e)
			{
				if (e.mods.isRightButtonDown())
				{
					PopupLookAndFeel plaf;
					PopupMenu m;
					m.setLookAndFeel(&plaf);

					previewParent.addEditingMenuItems(m);

					int result = m.show();

					if (previewParent.performPopupMenuForEditingIcons(result, item.url))
						return;
				}
				else
				{
					setOpen(true);
					gotoLink();
				}


			}

			bool canBeSelected() const override
			{
				return true;
			}

			int getItemHeight() const override
			{
				return 26;
			}

			int getItemWidth() const
			{
				auto intendation = getItemPosition(false).getX();

				const auto& s = previewParent.internalComponent.styleData;
				auto f = s.getBoldFont().withHeight(16.0f);

				int thisWidth = intendation + f.getStringWidth(item.tocString) + 30;

				int maxWidth = thisWidth;

				for (int i = 0; i < getNumSubItems(); i++)
				{
					maxWidth = jmax<int>(maxWidth, getSubItem(i)->getItemWidth());
				}

				return maxWidth;
			}

			void paintItem(Graphics& g, int width, int height)
			{



				Rectangle<float> area({ 0.0f, 0.0f, (float)width, (float)height });



				if (isSelected())
				{
					g.setColour(Colours::white.withAlpha(0.3f));
					g.fillRoundedRectangle(area, 2.0f);
				}

				auto r = area.removeFromLeft(3.0f).reduced(0.0f, 1.0f);

				area.removeFromLeft(5.0f);


				const auto& s = previewParent.internalComponent.styleData;

				g.setColour(item.c);
				g.fillRect(r);




				g.setColour(Colours::white.withAlpha(0.8f));

				auto f = s.getBoldFont().withHeight(16.0f);

				g.setFont(f);

				g.drawText(item.tocString, area, Justification::centredLeft);
			}

			MarkdownDataBase::Item item;
			MarkdownPreview& previewParent;
		};

		MarkdownDatabaseTreeview(MarkdownPreview& parent_) :
			parent(parent_)
		{
			parent.getHolder().addDatabaseListener(this);
			addAndMakeVisible(tree);

			setBgColour(bgColour);
			tree.setColour(TreeView::ColourIds::selectedItemBackgroundColourId, Colours::transparentBlack);
			tree.setColour(TreeView::ColourIds::linesColourId, Colours::red);
			tree.setRootItemVisible(false);

			tree.getViewport()->setScrollBarsShown(true, false);
			databaseWasRebuild();
		}

		void setBgColour(Colour c)
		{
			bgColour = c;
			tree.setColour(TreeView::ColourIds::backgroundColourId, bgColour);
		}

		~MarkdownDatabaseTreeview()
		{
			parent.getHolder().removeDatabaseListener(this);

			tree.setRootItem(nullptr);
			rootItem = nullptr;
		}

		void scrollToLink(const MarkdownLink& l);

		void databaseWasRebuild() override;

		void openAll(TreeViewItem* item)
		{
			item->setOpen(true);

			for (int i = 0; i < item->getNumSubItems(); i++)
			{
				openAll(item->getSubItem(i));
			}
		}

		void closeAllExcept(TreeViewItem* item, Array<TreeViewItem*> path)
		{
			if (path.contains(item))
				return;

			item->setOpen(false);
		}

		bool closeIfNoMatch(TreeViewItem* item, const MarkdownLink& id)
		{
			if (dynamic_cast<Item*>(item)->item.url == id)
				return true;

			item->setOpen(true);

			bool found = false;

			for (int i = 0; i < item->getNumSubItems(); i++)
			{
				found |= closeIfNoMatch(item->getSubItem(i), id);
			}

			if (!found)
				item->setOpen(false);

			return found;
		}



		void setCurrentAnchor(const String& s)
		{
			if (tree.getRootItem() == nullptr)
				return;

			auto nl = parent.renderer.getLastLink();

			if (auto t = dynamic_cast<Item*>(tree.getRootItem())->selectIfURLMatches(nl.withAnchor(s)))
			{
				t->setSelected(true, true);
				tree.scrollToKeepItemVisible(t);
			}
		}

		int getPreferredWidth() const
		{
			if (fixWidth != -1)
				return fixWidth;

			if (rootItem == nullptr)
				return 300;

			return jmax(300, tree.getRootItem()->getItemWidth());
		}

		void paint(Graphics& g) override
		{
			g.fillAll(bgColour);

		}

		void resized() override
		{
			tree.setBounds(getLocalBounds());
		}

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
