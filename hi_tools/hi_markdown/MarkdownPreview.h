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


class MarkdownPreview : public Component
{
public:

	enum MouseMode
	{
		Drag,
		Select,
		numMouseModes
	};

	MarkdownPreview();;

	void setDatabase(MarkdownDataBase* newDataBase)
	{
		topbar.database = newDataBase;
		toc.setDatabase(*newDataBase);
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

	void setNewText(const String& newText, const File& f);

	struct InternalComponent : public Component,
		public MarkdownParser::Listener,
		public juce::SettableTooltipClient
	{
		InternalComponent(MarkdownPreview& parent);

		~InternalComponent();

		int getTextHeight();

		void setNewText(const String& s, const File& f);

		String lastText;
		File lastFile;

		void markdownWasParsed(const Result& r) override;

		void mouseDown(const MouseEvent& e) override;

		void mouseDrag(const MouseEvent& e) override;

		void mouseUp(const MouseEvent& e) override;

		void mouseEnter(const MouseEvent& e) override
		{
			if (enableSelect)
				setMouseCursor(MouseCursor(MouseCursor::IBeamCursor));
			else
				setMouseCursor(MouseCursor(MouseCursor::DraggingHandCursor));
		}

		void mouseExit(const MouseEvent& e) override
		{
			setMouseCursor(MouseCursor::NormalCursor);
		}

		void mouseMove(const MouseEvent& event) override;

		void scrollToAnchor(float v) override;

		void scrollToSearchResult(Rectangle<float> currentSelection);

		void paint(Graphics & g) override;

		void resized() override
		{
			if (parser != nullptr)
			{
				parser->updateCreatedComponents();
			}
		}

		ScopedPointer<MarkdownParser::LayoutCache> layoutCache;

		ScopedPointer<hise::MarkdownParser> parser;
		String errorMessage;

		MarkdownLayout::StyleData styleData;

		Rectangle<float> clickedLink;
		OwnedArray<MarkdownParser::ImageProvider> providers;
		OwnedArray<MarkdownParser::LinkResolver> resolvers;
		MarkdownPreview& parent;

		Rectangle<float> currentSearchResult;

		Rectangle<int> currentLasso;
		bool enableSelect = true;
	};

	struct CustomViewport : public Viewport
	{
		class Listener
		{
		public:

			virtual ~Listener() {}

			virtual void scrolled(Rectangle<int> visibleArea) = 0;

			JUCE_DECLARE_WEAK_REFERENCEABLE(Listener);
		};

		CustomViewport(MarkdownPreview& parent_) :
			parent(parent_)
		{
			//setScrollOnDragEnabled(true);
		}

		void visibleAreaChanged(const Rectangle<int>& newVisibleArea) override
		{
			visibleArea = newVisibleArea;

			if (auto p = parent.internalComponent.parser.get())
			{
				auto s = p->getAnchorForY(newVisibleArea.getY());
				parent.toc.setCurrentAnchor(s);
			}

			for (int i = 0; i < listeners.size(); i++)
			{
				if (listeners[i].get() == nullptr)
				{
					listeners.remove(i--);
				}
				else
				{
					listeners[i]->scrolled(visibleArea);
				}
			}
		}

		void addListener(Listener* l) { listeners.addIfNotAlreadyThere(l); }
		void removeListener(Listener* l) { listeners.removeAllInstancesOf(l); }

		Array<WeakReference<Listener>> listeners;

		Rectangle<int> visibleArea;

		MarkdownPreview& parent;
	};

	struct Topbar : public Component,
		public ButtonListener,
		public LabelListener,
		public TextEditor::Listener,
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
			dragButton("Drag", this, factory),
			selectButton("Select", this, factory)
		{


			addAndMakeVisible(homeButton);
			addAndMakeVisible(tocButton);
			addAndMakeVisible(backButton);
			addAndMakeVisible(forwardButton);
			addAndMakeVisible(lightSchemeButton);
			addAndMakeVisible(searchBar);
			addAndMakeVisible(dragButton);
			addAndMakeVisible(selectButton);
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
		}


		struct TopbarPaths : public PathFactory
		{
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
		HiseShapeButton dragButton;
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

				void mouseEnter(const MouseEvent& e) override
				{
					hover = true;
					setMouseCursor(MouseCursor(MouseCursor::PointingHandCursor));
					repaint();
				}

				void mouseExit(const MouseEvent& e) override
				{
					hover = false;
					setMouseCursor(MouseCursor(MouseCursor::NormalCursor));
					repaint();
				}


				void mouseDown(const MouseEvent& e) override
				{
					down = true;
					repaint();
				}

				void gotoLink()
				{
					auto link = item.url.upToFirstOccurrenceOf("#", false, false);
					auto anchor = item.url.fromFirstOccurrenceOf("#", true, false);

					auto f = findParentComponentOfClass<MarkdownPreview>()->topbar.database->getRoot();

					if (auto p = findParentComponentOfClass<MarkdownPreview>()->internalComponent.parser.get())
					{
						p->gotoLink(link);

						auto mp = findParentComponentOfClass<MarkdownPreview>();

						auto f2 = [mp, anchor]()
						{
							if (auto p = mp->internalComponent.parser.get())
							{
								if (anchor.isNotEmpty())
									p->gotoLink(anchor);
							}

							mp->currentSearchResults = nullptr;
						};

						MessageManager::callAsync(f2);
					}
				}

				void mouseUp(const MouseEvent& e) override
				{
					down = false;
					repaint();

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
						
						Path p;
						p.addStar(starBounds.toFloat().getCentre(), 5, 5.0f, 10.0f);
						g.fillPath(p);
					}

					

					p.draw(g, ar.toFloat().reduced(5.0f).translated(0.0f, -5.0f));

					if (isFuzzyMatch)
						g.fillAll(Colours::grey.withAlpha(0.3f));
				}



				int calculateHeight(int width)
				{
					kBounds = { 0, 0, GLOBAL_BOLD_FONT().getStringWidth(item.keywords[0]) + 20, 0 };

					starBounds = item.type == MarkdownDataBase::Item::Keyword ? Rectangle<int>(kBounds.getRight(), 0, 30, 0) : Rectangle<int>();
					

					if (height == 0)
					{
						height = p.getHeightForWidth((float)(width - 10.0f - kBounds.getWidth() - starBounds.getWidth()));
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
				MarkdownParser p;
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
					

					if (auto p = parent.parent.internalComponent.parser.get())
					{
						currentSearchResultPositions = p->searchInContent(searchString);

						refreshTextResultLabel();

						parent.parent.repaint();
					}


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
					
					if (currentSelection = displayedItems[itemIndex])
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

					if (searchString.startsWith("docs://"))
					{
						displayedItems.clear();
						exactMatches.clear();
						fuzzyMatches.clear();

						auto linkURL = searchString.fromFirstOccurrenceOf("docs://", false, false);

						MarkdownDataBase::Item linkItem;

						for (auto item : allItems)
						{
							if (item.url == linkURL)
							{
								linkItem = item;
								break;
							}
						}

						if (linkItem.type != MarkdownDataBase::Item::Invalid)
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
									if(exactMatches.size() < 50)
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

                    for(auto i: exactMatches)
                        displayedItems.add(i);
                    
                    for(auto i: fuzzyMatches)
                        displayedItems.add(i);
                    
					content.setSize(viewport.getMaximumVisibleWidth(), 20);

					int y = 0;
					auto w = (float)getWidth();

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

			void labelTextChanged(Label* labelThatHasChanged) override
			{

			}

			void textEditorTextChanged(TextEditor& ed)
			{
				if (parent.currentSearchResults != nullptr)
				{
					parent.currentSearchResults->setSearchString(ed.getText());
				}
			}

			void editorShown(Label* l, TextEditor& ed) override
			{
				ed.addListener(this);
				ed.addKeyListener(this);
				if (parent.currentSearchResults == nullptr)
				{
					parent.addAndMakeVisible(parent.currentSearchResults = new SearchResults(*this));

					auto bl = l->getBounds().getBottomLeft();

					auto tl = parent.getLocalPoint(this, bl);

					parent.currentSearchResults->setSize(l->getWidth(), 24);
					parent.currentSearchResults->setTopLeftPosition(tl);
					parent.currentSearchResults->grabKeyboardFocus();
				}
			}

			void textEditorEscapeKeyPressed(TextEditor&)
			{
				parent.currentSearchResults = nullptr;
			}

			void editorHidden(Label*, TextEditor& ed) override
			{
				ed.removeListener(this);
			}

			void buttonClicked(Button* b) override
			{
				if (b == &tocButton)
				{
					parent.toc.setVisible(!parent.toc.isVisible());
					parent.resized();
				}
				if (b == &lightSchemeButton)
				{
					if (b->getToggleState())
					{
						MarkdownLayout::StyleData l;
						l.textColour = Colour(0xFF333333);
						l.headlineColour = Colour(0xFF444444);
						l.backgroundColour = Colours::white;
						l.linkColour = Colour(0xFF000044);
						l.codeColour = Colour(0xFF333333);
						parent.internalComponent.styleData = l;

					}
					else
					{
						parent.internalComponent.styleData = {};
					}

					parent.internalComponent.parser->setStyleData(parent.internalComponent.styleData);

					parent.repaint();

					lightSchemeButton.refreshShape();
				}
				if (b == &selectButton)
				{
					parent.setMouseMode(Select);
				}
				if (b == &dragButton)
				{
					parent.setMouseMode(Drag);
				}
			}

			bool keyPressed(const KeyPress& key, Component* originatingComponent) override
			{
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

			void resized() override
			{
				const auto& s = parent.internalComponent.styleData;

				Colour c = Colours::white;

				tocButton.setColours(c.withAlpha(0.8f), c, c);
				//homeButton.setColours(c.withAlpha(0.8f), c, c);
				//backButton.setColours(c.withAlpha(0.8f), c, c);
				//forwardButton.setColours(c.withAlpha(0.8f), c, c);
				lightSchemeButton.setColours(c.withAlpha(0.8f), c, c);
				dragButton.setColours(c.withAlpha(0.8f), c, c);
				selectButton.setColours(c.withAlpha(0.8f), c, c);

                homeButton.setVisible(false);
                backButton.setVisible(false);
                forwardButton.setVisible(false);
                
				auto ar = getLocalBounds();
				int buttonMargin = 12;
				int margin = 0;
				int height = ar.getHeight();

				tocButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
				ar.removeFromLeft(margin);
				homeButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
				ar.removeFromLeft(margin);
				backButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
				forwardButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
				ar.removeFromLeft(margin);
				lightSchemeButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
				ar.removeFromLeft(margin);
				dragButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
				selectButton.setBounds(ar.removeFromLeft(height).reduced(buttonMargin));
				ar.removeFromLeft(margin);

                auto delta = 0; //parent.toc.getWidth() - ar.getX();

				ar.removeFromLeft(delta);

				auto sBounds = ar.removeFromLeft(height).reduced(buttonMargin).toFloat();
				searchPath.scaleToFit(sBounds.getX(), sBounds.getY(), sBounds.getWidth(), sBounds.getHeight(), true);

				searchBar.setBounds(ar.reduced(5.0f));
			}

			void paint(Graphics& g) override
			{
				g.fillAll(Colour(0xFF333333));
				g.fillAll(Colours::white.withAlpha(0.05f));
				g.setColour(Colours::white.withAlpha(0.7f));
				g.fillPath(searchPath);
			}

		};


	class MarkdownDatabaseTreeview : public Component
	{
	public:

		struct Item : public juce::TreeViewItem
		{
			Item(MarkdownDataBase::Item item_, MarkdownPreview& previewParent_) :
				TreeViewItem(),
				item(item_),
				previewParent(previewParent_)
			{}

			bool mightContainSubItems() override { return !item.children.isEmpty(); }

			String getUniqueName() const override { return item.url; }

			void itemOpennessChanged(bool isNowOpen) override
			{
				clearSubItems();

				if (isNowOpen)
				{
					for (auto c : item.children)
						addSubItem(new Item(c, previewParent));
				}

				//previewParent.resized();
			}

			MarkdownParser* getCurrentParser()
			{
				return previewParent.internalComponent.parser.get();
			}

			Item* selectIfURLMatches(const String& url)
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
			

			void itemClicked(const MouseEvent& e)
			{
				if (auto p = getCurrentParser())
				{
					previewParent.currentSearchResults = nullptr;

					auto link = item.url.upToFirstOccurrenceOf("#", false, false);

					if(p->getLastLink(true, false) != link)
						p->gotoLink(item.url);

					auto anchor = item.url.fromFirstOccurrenceOf("#", true, false);

					if (anchor.isNotEmpty())
					{
						auto mp = &previewParent;

						auto it = this;

						auto f2 = [mp, anchor, it]()
						{
							if (auto p = mp->internalComponent.parser.get())
							{
								if (anchor.isNotEmpty())
									p->gotoLink(anchor);

								it->setSelected(true, true);
							}
						};

						MessageManager::callAsync(f2);
					}
				}
			}

			bool canBeSelected() const override
			{
				return true;
			}

			int getItemHeight() const override
			{
				const auto& s = previewParent.internalComponent.styleData;

				return s.getFont().getHeight() * 3 / 2;
			}

			int getItemWidth() const 
			{ 
				auto intendation = getItemPosition(false).getX();
				
				const auto& s = previewParent.internalComponent.styleData;
				auto f = FontHelpers::getFontBoldened(s.getFont());

				int thisWidth = intendation + f.getStringWidth(item.tocString) + 30.0f;   
				
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

				auto r = area.removeFromLeft(3.0f);

				area.removeFromLeft(5.0f);

				const auto& s = previewParent.internalComponent.styleData;

				g.setColour(item.c);
				g.fillRect(r);

				g.setColour(Colours::white.withAlpha(0.8f));
				g.setFont(FontHelpers::getFontBoldened(s.getFont()));

				g.drawText(item.tocString, area, Justification::centredLeft);
			}

			MarkdownDataBase::Item item;
			MarkdownPreview& previewParent;
		};

		MarkdownDatabaseTreeview(MarkdownPreview& parent_):
			parent(parent_)
		{
			addAndMakeVisible(tree);
		}

		~MarkdownDatabaseTreeview()
		{
			tree.setRootItem(nullptr);
			rootItem = nullptr;
		}

		void scrollToLink(const String& l)
		{
			auto root = tree.getRootItem();

			bool found = false;

			for (int i = 0; i < root->getNumSubItems(); i++)
				found |= closeIfNoMatch(root->getSubItem(i), l);

			if (found)
			{
				if (auto t = dynamic_cast<Item*>(tree.getRootItem())->selectIfURLMatches(l))
				{
					t->setSelected(true, true);
					tree.scrollToKeepItemVisible(t);
				}
			}
		}
		

		void setDatabase(MarkdownDataBase& database)
		{
			tree.setColour(TreeView::ColourIds::backgroundColourId, Colour(0xFF222222));
			tree.setColour(TreeView::ColourIds::selectedItemBackgroundColourId, Colours::transparentBlack);
			tree.setColour(TreeView::ColourIds::linesColourId, Colours::red);
			tree.setRootItemVisible(false);
			tree.setRootItem(rootItem = new Item(database.rootItem, parent));
			tree.getViewport()->setScrollBarsShown(true, false);
		}

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
			if(path.contains(item))
				return;

			item->setOpen(false);
		}

		bool closeIfNoMatch(TreeViewItem* item, const String& id)
		{
			if (item->getUniqueName() == id)
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
			if (auto p = parent.internalComponent.parser.get())
			{
				String nl = p->getLastLink(true, false);

				nl << s;

				if (auto t = dynamic_cast<Item*>(tree.getRootItem())->selectIfURLMatches(nl))
				{
					t->setSelected(true, true);
					tree.scrollToKeepItemVisible(t);
				}
			}
		}

		int getPreferredWidth() const
		{
			return jmax(300, tree.getRootItem()->getItemWidth());
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF222222));
			
		}

		void resized() override
		{
			tree.setBounds(getLocalBounds());
		}

		juce::TreeView tree;
		ScopedPointer<Item> rootItem;
		MarkdownPreview& parent;
	};


	void setStyleData(MarkdownLayout::StyleData d)
	{
		internalComponent.styleData = d;
	}

	void paint(Graphics& g) override
	{
		g.fillAll(internalComponent.styleData.backgroundColour);
	}

	void resized() override;

	LookAndFeel_V3 laf;
	
	String lastText;
	File lastFile;

	MarkdownDatabaseTreeview toc;
	CustomViewport viewport;
	InternalComponent internalComponent;
	Topbar topbar;
	ScopedPointer<Topbar::SearchResults> currentSearchResults;
};


}
