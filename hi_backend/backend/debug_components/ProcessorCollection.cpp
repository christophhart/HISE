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

namespace hise { using namespace juce;

// ====================================================================================================================

SearchableListComponent::SearchableListComponent(BackendRootWindow* window):
	fuzzyness(0.4),
	showEmptyCollections(false),
	internalRebuildFlag(true),
	rootWindow(window)
{
	addAndMakeVisible(fuzzySearchBox = new TextEditor());
	fuzzySearchBox->addListener(this);

	textClearButton = new SearchBoxClearComponent(*fuzzySearchBox);

    setWantsKeyboardFocus(true);

    GlobalHiseLookAndFeel::setTextEditorColours(*fuzzySearchBox);
    
	internalContainer = new InternalContainer();

	addAndMakeVisible(viewport = new Viewport());

    sf.addScrollBarToAnimate(viewport->getVerticalScrollBar());
    viewport->setScrollBarThickness(13.0f);
	viewport->setViewedComponent(internalContainer, false);

	internalContainer->setSize(300, 20);
	viewport->setScrollBarsShown(true, false);
}

void SearchableListComponent::resized()
{
	internalContainer->setSize(getWidth(), internalContainer->getHeight());
	
    auto b = getLocalBounds().removeFromTop(24);
    
	for(auto cb: customButtons)
    {
        auto margin = dynamic_cast<HiseShapeButton*>(cb.getComponent()) ? 2 : 4;
        cb->setBounds(b.removeFromLeft(b.getHeight()).reduced(margin));
    }
    
    b.removeFromLeft(b.getHeight());

    fuzzySearchBox->setBounds(b.reduced(1));
	viewport->setBounds(0, 26, getWidth(), getHeight() - 26);

	rebuildModuleList(false);
}


void SearchableListComponent::paint(Graphics& g)
{
    g.setColour(Colour(0xff353535));
    g.fillRect(0.0f, 0.0f, (float)getWidth(), 25.0f);
    
	
	
	g.setColour(Colour(0xFF262626));
	g.fillRect(0, 25, getWidth(), getHeight());

    g.setGradientFill(ColourGradient(Colours::black.withAlpha(0.3f), 0.0f, 25.0f,
                                     Colours::transparentBlack, 0.0f, 35.0f, false));
    g.fillRect(0.0f, 25.0f, (float)getWidth(), 10.0f);
    
	g.setColour(Colours::white.withAlpha(0.3f));

	static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
		103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
		191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
		218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
		95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

	Path path;
	path.loadPathFromData(searchIcon, sizeof(searchIcon));
	path.applyTransform(AffineTransform::rotation(float_Pi));

	const float xOffset = (int)customButtons.size() * 24.0f;

	path.scaleToFit(xOffset + 4.0f, 4.0f, 16.0f, 16.0f, true);

	g.fillPath(path);
}


void SearchableListComponent::refreshDisplayedItems()
{
	int height = 0;

	const String searchTerm = fuzzySearchBox->getText().toLowerCase();

	Array<Collection*> sortedCollections;

	for (int i = 0; i < getNumCollections(); i++)
	{
		Collection *c = getCollection(i);

		c->searchItems(searchTerm, fuzzyness);

		if (showEmptyCollections || c->hasVisibleItems())
		{
			internalContainer->addAndMakeVisible(c);

			c->setBounds(0, height, internalContainer->getWidth()-8, c->getHeightForCollection());

			height = c->getBottom();

			if(searchTerm.isNotEmpty())
				sortedCollections.add(c);
		}
		else
		{
			c->setVisible(false);
		}
	}

	struct Sorter
	{
		Sorter(const String& searchTerm_):
		  searchTerm(searchTerm_)
		{};

		int compareElements(Collection* c1, Collection* c2) const
		{
			return c1->compareSortState(c2, searchTerm);
		}

		String searchTerm;
	};

	Sorter s(searchTerm);

	sortedCollections.sort(s, true);

	internalContainer->setSortedCollections(sortedCollections);


	internalContainer->setSize(getWidth(), height);

	viewport->setViewPositionProportionately(0.0, 0.0);

	repaintAllItems();
}


void SearchableListComponent::repaintAllItems()
{
	for (int i = 0; i < getNumCollections(); i++)
	{
		auto c = getCollection(i);

		c->repaint();
		c->repaintAllItems();
	}
}

void SearchableListComponent::rebuildModuleList(bool forceRebuild)
{
	if (internalRebuildFlag || forceRebuild)
	{
		internalContainer->collections.clear();

		int numCollections = getNumCollectionsToCreate();

		for (int i = 0; i < numCollections; i++)
		{
			Collection *c = createCollection(i);

			internalContainer->collections.add(c);
		}

		refreshDisplayedItems();
	}

	internalRebuildFlag = false;
    
    rebuilt();
}

void SearchableListComponent::textEditorTextChanged(TextEditor& )
{
	refreshDisplayedItems();
}

void SearchableListComponent::clearSearchResults()
{
	displayedIndexes.clear();
}

void SearchableListComponent::setShowEmptyCollections(bool emptyCollectionsShouldBeShown)
{
	showEmptyCollections = emptyCollectionsShouldBeShown;
	internalContainer->setShowEmptyCollections(emptyCollectionsShouldBeShown);
}

SearchableListComponent::Collection::Collection(int originalIndex_):
  folded(false),
  originalIndex(originalIndex_)
{

}

int SearchableListComponent::Collection::getHeightForCollection() const
{
	int h = COLLECTION_HEIGHT;

	if (folded) return h;

	for (int i = 0; i < items.size(); i++)
	{
		if (items[i]->isIncludedInSearch())
		{
			h += ITEM_HEIGHT;
		}
	}

	return h;
}

void SearchableListComponent::Collection::resized()
{
	int h = COLLECTION_HEIGHT;

	visibleItems = 0;

	for (int i = 0; i < items.size(); i++)
	{
		if (!items[i]->isIncludedInSearch() || isFolded())
		{
			items[i]->setVisible(false);
		}
		else
		{
			items[i]->setVisible(true);


			items[i]->setBounds(12, h, getWidth() - 18, ITEM_HEIGHT);

			//items[i]->setTopLeftPosition(12, h);
			h += ITEM_HEIGHT;

			visibleItems++;
		}
	}
}

bool SearchableListComponent::Collection::hasVisibleItems() const
{
	for (int i = 0; i < items.size(); i++)
	{
		if (items[i]->isIncludedInSearch()) return true;
	}

	return false;
}

void SearchableListComponent::Collection::searchItems(const String &searchTerm, double fuzzyness)
{
	for (int i = 0; i < items.size(); i++)
	{
		items[i]->matchAgainstSearch(searchTerm, fuzzyness);
	}
}

void SearchableListComponent::Collection::setFolded(bool shouldBeFolded) noexcept
{
	folded = shouldBeFolded;
}

SearchableListComponent::InternalContainer::InternalContainer():
showEmptyCollections(false)
{

}

void SearchableListComponent::InternalContainer::resized()
{
	int h = 0;

	if(sortedCollections.isEmpty())
	{
		for (int i = 0; i < collections.size(); i++)
		{
			Collection *c =  collections[i];

			if (!showEmptyCollections && !c->hasVisibleItems())
			{
				continue;
			}

			c->setBounds(0, h, getWidth()-8, c->getHeightForCollection());
			h += c->getHeight();
		}
	}
	else
	{
		auto b = getLocalBounds();
		b.removeFromRight(8);

		for(auto c: sortedCollections)
		{
			c->setBounds(b.removeFromTop(c->getHeightForCollection()));
		}
	}

	
}

void SearchableListComponent::Item::mouseDown(const MouseEvent& event)
{
	if (!usePopupMenu)
	{
		if(auto cp = findParentComponentOfClass<SearchableListComponent>()->currentPopup.get())
		{
			if(cp->parent == this)
			{
				clicksToClose++;

				if(clicksToClose > 1)
				{
					findParentComponentOfClass<SearchableListComponent>()->showPopup(nullptr, FocusChangeType::focusChangedByMouseClick);
					
				}
			}
		}
		return;
	}

	if (event.mods.isRightButtonDown())
	{
		PopupMenu m;

		m.setLookAndFeel(&laf);

		fillPopupMenu(m);

		PopupMenu::Options options;
		
		Point<int> mousePos(Desktop::getInstance().getMousePosition());

		Point<int> pos2 = mousePos;
		pos2.addXY(200, 500);

		m.showMenuAsync(options.withTargetScreenArea(Rectangle<int>(mousePos, mousePos)), new PopupCallback(this));
	}
	else
	{
		findParentComponentOfClass<SearchableListComponent>()->setSelectedItem(this);
	}
}

void SearchableListComponent::Item::matchAgainstSearch(const String &stringToMatch, double fuzzyness)
{
	if (stringToMatch.isEmpty())
	{
		includedInSearch = true;
	}
	else
	{
        if(searchKeywords.contains(";"))
        {
            auto list = StringArray::fromTokens(searchKeywords, ";", "");
            
            includedInSearch = false;
            
            for(auto sa: list)
                includedInSearch |= FuzzySearcher::fitsSearch(stringToMatch, sa, fuzzyness);
        }
		else
        {
            includedInSearch = FuzzySearcher::fitsSearch(stringToMatch, searchKeywords, fuzzyness);
        }
	}
}

SearchableListComponent::Item::PopupComponent::PopupComponent(Item *p) :
parent(p)
{
	parent->findParentComponentOfClass<SearchableListComponent>()->viewport->getVerticalScrollBar().addListener(this);
	setWantsKeyboardFocus(false);
}

void SearchableListComponent::Item::PopupComponent::paint(Graphics& g)
{
	auto cornerSize = 3.0f;

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffe0e0e0)));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), cornerSize);
	
	if (parent.getComponent() != nullptr) parent->paintPopupBox(g);
}

} // namespace hise
