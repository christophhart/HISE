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

#ifndef PROCESSOR_COLLECTION_H_INCLUDED
#define PROCESSOR_COLLECTION_H_INCLUDED



#define COLLECTION_HEIGHT 40
#define ITEM_HEIGHT 22

#define COLLECTION_WIDTH 380
#define ITEM_WIDTH (380 - 16 - 8 - 24)

namespace hise { using namespace juce;


/** A generic list component that allows fuzzy searching.
*
*	It is designed with two hierarchy levels: 
*	
*	1. Collections: the parent container for Items.
*	
*/
class SearchableListComponent: public Component,
							   public TextEditor::Listener
{

public:

	/** This is the base class for all item components that are part of a Collection.
	*
	*	They contain a search keyword which is used to display only the search results.
	*	They also contain a popup component which is displayed on right click.
	*/
	class Item : public Component
	{
	public:

		/** Creates a new Item that uses the string for its search. Use a lower case string only (except you want to handle lower / upper case. */
		Item(const String searchString) :
			isSelected(false),
			includedInSearch(true),
			usePopupMenu(false),
			searchKeywords(searchString.toLowerCase())
		{}

		~Item()
		{
			searchKeywords = String();
		}

		/** A basic component which is displayed whenever the user clicks right on the item.
		*
		*	It can be used to show additional information.
		*/
		class PopupComponent : public Component,
							   public ScrollBar::Listener
		{
		public:

			PopupComponent(Item *p);

			~PopupComponent()
			{
				if(parent != nullptr)
					parent->findParentComponentOfClass<SearchableListComponent>()->viewport->getVerticalScrollBar().removeListener(this);
			}

			void scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart) override
			{
				auto p = parent->findParentComponentOfClass<SearchableListComponent>();

				MessageManager::callAsync([p]()
				{
					p->showPopup(nullptr, FocusChangeType::focusChangedByTabKey);
				});
			}

			void paint(Graphics& g) override;


			Component::SafePointer<Item> parent;
		};

        
		void mouseDown(const MouseEvent& event) override;

		/** You can overwrite this method if you want to display information in the popup component. */
		virtual void paintPopupBox(Graphics &) const {};

		virtual void fillPopupMenu(PopupMenu &) {};

		virtual void popupCallback(int /*menuIndex*/) {};

        void paintItemBackground(Graphics& g, Rectangle<float> area)
        {
            
        }

		void focusGained(FocusChangeType cause) override
        {
	        if(!usePopupMenu)
	        {
		        findParentComponentOfClass<SearchableListComponent>()->showPopup(this, FocusChangeType::focusChangedDirectly);
				clicksToClose = 0;
	        }
        }

		void focusLost(FocusChangeType cause) override
        {
	        if(!usePopupMenu)
	        {
		        findParentComponentOfClass<SearchableListComponent>()->showPopup(nullptr, FocusChangeType::focusChangedDirectly);
	        }
        }

		void setUsePopupMenu(bool shouldUsePopupMenu) noexcept{ usePopupMenu = shouldUsePopupMenu; }

		/** Overwrite this and return the height of the popup component. If you return 0 (default behaviour), the functionality is deactivated. */
		virtual int getPopupHeight() const { return 0; };

		virtual int getPopupWidth() const { return 300; }

		/** This is called whenever the search text updates. */
		void matchAgainstSearch(const String &stringToMatch, double fuzzyness);

		/** Checks if the last search query included this item. */
		bool isIncludedInSearch() const noexcept{ return includedInSearch; };

	protected:

		bool usePopupMenu;
		bool isSelected;
		bool includedInSearch;
		String searchKeywords;

	private:

		int clicksToClose = 0;

		PopupLookAndFeel laf;

		struct PopupCallback : public ModalComponentManager::Callback
		{
			PopupCallback(Item *parent_) : parent(parent_) {};

			void modalStateFinished(int returnValue) override
			{
				if (parent.getComponent() != nullptr)
				{
					parent->popupCallback(returnValue);
				}
			};

			Component::SafePointer<Item> parent;
		};
	};

	virtual void onPopupClose(FocusChangeType ft) {};

	void showPopup(Item* item, FocusChangeType ft)
	{
		if(item == nullptr)
		{
			Desktop::getInstance().getAnimator().fadeOut(currentPopup, 120);
			currentPopup = nullptr;

			onPopupClose(ft);

			return;
		}

		if(currentPopup != nullptr)
		{
			currentPopup = nullptr;
		}
		
		auto parent = TopLevelWindowWithOptionalOpenGL::findRoot(this);

		currentPopup = new Item::PopupComponent(item);

		currentPopup->setSize(item->getPopupWidth(), item->getPopupHeight());

		AffineTransform t;

		if(auto la = dynamic_cast<mcl::FullEditor*>(getRootWindow()->getBackendProcessor()->getLastActiveEditor()))
		{
			t = AffineTransform::scale(la->editor.transform.getScaleFactor());
		}

		

		currentPopup->setTransform(t);

		parent->addAndMakeVisible(currentPopup);
		auto lb = parent->getLocalArea(item, item->getLocalBounds());

		auto b = currentPopup->getLocalBounds();
		b.setPosition(lb.getTopRight().transformedBy(t.inverted()));

		b = b.constrainedWithin(parent->getLocalBounds());

		currentPopup->setBounds(b);
	}

	ScopedPointer<Item::PopupComponent> currentPopup;

	struct SearchBoxClearComponent: public Component,
									public TextEditor::Listener,
								    public ComponentListener
	{
		SearchBoxClearComponent(TextEditor& te):
		  editor(te)
		{
			static const unsigned char pathData[] = { 110,109,48,128,38,68,240,254,63,67,98,166,156,55,68,240,254,63,67,0,128,69,68,240,143,119,67,0,128,69,68,0,0,158,67,98,0,128,69,68,16,56,192,67,166,156,55,68,144,0,220,67,48,128,38,68,144,0,220,67,98,16,100,21,68,144,0,220,67,0,128,7,68,16,56,192,67,
0,128,7,68,0,0,158,67,98,0,128,7,68,240,143,119,67,16,100,21,68,240,254,63,67,48,128,38,68,240,254,63,67,108,242,48,38,68,136,2,64,67,108,242,48,38,68,144,181,139,67,108,42,66,27,68,160,175,107,67,108,0,108,18,68,88,132,135,67,108,4,170,29,68,0,0,158,
67,108,0,108,18,68,176,123,180,67,108,42,66,27,68,48,40,198,67,108,48,128,38,68,136,172,175,67,108,120,190,49,68,48,40,198,67,108,186,148,58,68,176,123,180,67,108,252,85,47,68,0,0,158,67,108,186,148,58,68,88,132,135,67,108,120,190,49,68,160,175,107,67,
108,206,205,38,68,40,185,139,67,108,206,205,38,68,136,2,64,67,108,48,128,38,68,240,254,63,67,99,101,0,0 };

			p.loadPathFromData(pathData, sizeof(pathData));

			editor.getParentComponent()->addChildComponent(this);
			editor.addComponentListener(this);
			editor.addListener(this);
			setRepaintsOnMouseActivity(true);
		}

		Path p;

		~SearchBoxClearComponent()
		{
			editor.removeListener(this);
			editor.removeComponentListener(this);
		}

		void componentMovedOrResized(Component& c, bool wasMoved, bool wasResized) override
		{
			auto b = c.getBoundsInParent();
			b = b.removeFromRight(b.getHeight());
			setBounds(b);
			PathFactory::scalePath(p, b.toFloat().withZeroOrigin().reduced(5));
		}

		void mouseDown(const MouseEvent& e)
		{
			editor.setText("", true);
			
		}

		void paint(Graphics& g) override
		{
			float alpha = 0.4f;

			if(isMouseOver(false))
				alpha += 0.3f;

			g.setColour(Colours::black.withAlpha(alpha));
			g.fillPath(p);
		}

		void textEditorTextChanged(TextEditor&) override
		{
			setVisible(!editor.getText().isEmpty());
		}

		TextEditor& editor;
	};

	/** A Collection is the highest hierarchy level and contains multiple Items.
	*
	*	When a search term is used, it can be hidden.
	*
	*	While the collection amount is dynamically changeable, a collection must create all of its Items in its constructor
	*	(it will be deleted and a new Collection is created if the items change).
	*/
	class Collection : public Component
	{
	public:

		Collection(int originalIndex);

		void searchItems(const String &searchTerm, double fuzzyness);

		/** Checks if the collection has any visible items.
		*
		*	Items can be either hidden because of the search result or because the collection is folded
		*/
		bool hasVisibleItems() const;

		int getHeightForCollection() const;

		void resized() override;

		void paint(Graphics &g) override
		{
			g.fillAll(Colours::red);
		}

		void repaintAllItems()
		{
			for (int i = 0; i < items.size(); i++)
			{
				items[i]->repaint();
			}
		}

		bool isFolded() const noexcept { return folded; };

		void setFolded(bool shouldBeFolded) noexcept;;

		int getNumItems(bool countOnlyVisibleItems) const
		{
			if (countOnlyVisibleItems)
				return visibleItems;

			return items.size();
		}

        SearchableListComponent::Item* getItem(int index)
        {
            return items[index];
        }
		
		int compareSortState(Collection* other, const String& searchTerm)
		{
			if(originalIndex == other->originalIndex)
					return 0;

			if(searchTerm.isEmpty())
			{
				return originalIndex < other->originalIndex ? -1 : 1;
			}
			else
			{
				auto st = getSearchTermForCollection().startsWith(searchTerm);
				auto ost = other->getSearchTermForCollection().startsWith(searchTerm);

				if(st == ost)
					return originalIndex < other->originalIndex ? -1 : 1;
				else if (st)
					return -1;
				else //if (ost)
					return 1;
				
			}
		}

		virtual String getSearchTermForCollection() const = 0;

	protected:

		OwnedArray<SearchableListComponent::Item> items;

	private:

		const int originalIndex = -1;

		int visibleItems;
		bool folded;
	};


	class InternalContainer : public Component
	{
	public:

		InternalContainer();

		void resized() override;

		void setShowEmptyCollections(bool shouldBeShown) noexcept { showEmptyCollections = shouldBeShown; };

		void setSortedCollections(const Array<Collection*>& sorted)
		{
			sortedCollections = sorted;
			resized();
		}

	private:

		friend class SearchableListComponent;

		OwnedArray<Collection> collections;
		Array<Collection*> sortedCollections;

		bool showEmptyCollections;
	};

	virtual ~SearchableListComponent() {};

	/** Remove this. */
	void fillNameList(); 

	void setSelectedItem(Item* /*item*/) noexcept{  };



	void setFuzzyness(double newFuzzyness) { fuzzyness = newFuzzyness; };

	void textEditorTextChanged(TextEditor& editor);

	void clearSearchResults();

	/** Call this when you need to clear the collection. */
	void clearCollections()
	{
		internalContainer->collections.clear();
		resized();
	}

	void resized() override;

	void paint(Graphics& g) override;

	/** Call this whenever the visibility of one of the items changes. */
	void refreshDisplayedItems();

	/** Call this whenever the appearance of one of the items changes. */
	void repaintAllItems();

	/** Call this whenever an item is added / deleted. */
	void rebuildModuleList(bool forceRebuild=false);

    virtual void rebuilt() {};
    
	void setShowEmptyCollections(bool emptyCollectionsShouldBeShown);;

	BackendRootWindow* getRootWindow() { return rootWindow; }

	void selectNext(bool next, Item* thisItem)
	{
		

		Array<Item*> list;
		list.ensureStorageAllocated(2048);

		callRecursive<Item>(this, [&](Item* i)
		{
			if(i->isShowing())
				list.add(i);

			return false;
		});

		

		if(fuzzySearchBox->getText().isNotEmpty())
		{
			struct YSorter
			{
				YSorter(Component& parent):
				 p(parent)
				{};

				int compareElements(Item* c1, Item* c2) const
				{
					auto y1 = p.getLocalArea(c1, c1->getLocalBounds()).getY();
					auto y2 = p.getLocalArea(c2, c2->getLocalBounds()).getY();

					if(y1 < y2)
						return -1;
					else if(y1 > y2)
						return 1;
					else 
						return 0;

				}

				Component& p;
			};

			YSorter s(*this);

			list.sort(s);
		}
		


		auto thisIndex = list.indexOf(thisItem);

		SafePointer<Item> c;

		if(next)
		{
			if(isPositiveAndBelow(thisIndex +1, list.size()))
			{
				c = list[thisIndex + 1];
			}

			if(c == nullptr)
				return;

				
		}
		else
		{
			if(thisIndex >= 1)
				c = list[thisIndex - 1];
		}

		if(c == nullptr)
			return;

		Timer::callAfterDelay(30, [c]()
		{
			if (c == nullptr)
				return;

			c->findParentComponentOfClass<SearchableListComponent>()->currentPopup = nullptr;
			c->findParentComponentOfClass<SearchableListComponent>()->ensureVisibility(c);
			
			c->grabKeyboardFocus();
			c->repaint();
			
		});
		
		repaint();
	}

protected:
	SearchableListComponent(BackendRootWindow* window);

	/** Overwrite this method and return the number of collections.
	*
	*	This method is called whenever the items are rebuilt.
	*	You can do some complex calculations here, there is a lighter method (getNumCollections()) that checks the currently existing amount.
	*/
	virtual int getNumCollectionsToCreate() const = 0;

	/** Overwrite this method and return the collection for the given index. */
	virtual Collection *createCollection(int index) = 0;

	/** Returns the collection  at the specified index. */
	Collection *getCollection(int index) { return internalContainer->collections[index]; };

	/** Returns the collection  at the specified index. */
	const Collection *getCollection(int index) const { return internalContainer->collections[index]; };

	/** Returns the number of currently available collections. */
	int getNumCollections() const { return internalContainer->collections.size(); }

	/** */
	void addCustomButton(ShapeButton *button)
	{
		customButtons.add(button);
	}

	void setSortedCollections(const Array<Collection*>& sorted)
	{
		internalContainer->setSortedCollections(sorted);

	}

private:

	

	void ensureVisibility(Item* i)
	{
		auto lb = viewport->getViewedComponent()->getLocalArea(i, i->getLocalBounds()).getTopLeft();
		auto va = viewport->getViewArea().reduced(-10, 30);

		if(!va.contains(lb))
		{
			if(va.getY() > lb.getY())
			{
				viewport->setViewPosition({0, lb.getY() - 20 });
			}
			else
			{
				viewport->setViewPosition({0,  lb.getY() + i->getHeight() - va.getHeight()});
			}
		}
	}

	BackendRootWindow* rootWindow;

	double fuzzyness;

	bool showEmptyCollections;

	ScopedPointer<InternalContainer> internalContainer;
	ScopedPointer<Viewport> viewport;
	ScopedPointer<TextEditor> fuzzySearchBox;
	ScopedPointer<SearchBoxClearComponent> textClearButton;

    ScrollbarFader sf;
    
	StringArray moduleNameList;
	
	Array<int> displayedIndexes;

	bool internalRebuildFlag;

	Array<Component::SafePointer<ShapeButton>> customButtons;
};

} // namespace hise

#endif
