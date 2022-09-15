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
		class PopupComponent : public Component
		{
		public:

			PopupComponent(Item *p);

			void paint(Graphics& g) override;

		private:

			Component::SafePointer<Item> parent;
		};

        
		void mouseDown(const MouseEvent& event) override;

		/** You can overwrite this method if you want to display information in the popup component. */
		virtual void paintPopupBox(Graphics &) const {};

		virtual void fillPopupMenu(PopupMenu &) {};

		virtual void popupCallback(int /*menuIndex*/) {};

        void paintItemBackground(Graphics& g, Rectangle<float> area)
        {
            g.setGradientFill(ColourGradient(JUCE_LIVE_CONSTANT_OFF(Colour(0xff303030)), 0.0f, 0.0f,
                JUCE_LIVE_CONSTANT_OFF(Colour(0xff282828)), 0.0f, (float)area.getHeight(), false));

            g.fillRoundedRectangle(area, 2.0f);

            g.setColour(Colours::white.withAlpha(0.1f));
            g.drawRoundedRectangle(area.reduced(1.0f), 2.0f, 1.0f);
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

		private:

			Component::SafePointer<Item> parent;
		};

        
		
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

		Collection();

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
        
	protected:

		OwnedArray<SearchableListComponent::Item> items;

	private:

		int visibleItems;
		bool folded;
	};


	class InternalContainer : public Component
	{
	public:

		InternalContainer();

		void resized() override;

		void setShowEmptyCollections(bool shouldBeShown) noexcept { showEmptyCollections = shouldBeShown; };

	private:

		friend class SearchableListComponent;

		OwnedArray<Collection> collections;

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

private:

	BackendRootWindow* rootWindow;

	double fuzzyness;

	bool showEmptyCollections;

	ScopedPointer<InternalContainer> internalContainer;
	ScopedPointer<Viewport> viewport;
	ScopedPointer<TextEditor> fuzzySearchBox;

    ScrollbarFader sf;
    
	StringArray moduleNameList;
	
	Array<int> displayedIndexes;

	bool internalRebuildFlag;

	Array<Component::SafePointer<ShapeButton>> customButtons;
};

} // namespace hise

#endif
