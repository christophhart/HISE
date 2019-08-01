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
				repaint();
			}

			Tag(const String& t)
			{
				setRepaintsOnMouseActivity(true);

				setName(t);

				auto w = GLOBAL_BOLD_FONT().getStringWidth(t) + 15;
				setSize(w, 28);
			}

			void mouseDown(const MouseEvent& e)
			{
				findParentComponentOfClass<TagList>()->toggleTag(getName());
			}

			float alpha = 0.5f;

			void paint(Graphics& g) override
			{
				auto c1 = Colour(SIGNAL_COLOUR).withAlpha(alpha);
				auto c2 = Colour(SIGNAL_COLOUR).withAlpha(alpha * 0.8f);

				g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));

				auto area = getLocalBounds().toFloat().reduced(2.0f);

				g.fillRoundedRectangle(area, 3.0f);
				g.setColour(Colours::black);

				float a = 0.0f;

				if (isMouseOver())
					a += 0.1f;

				if (isMouseButtonDown())
					a += 0.2f;

				g.setColour(Colours::white.withAlpha(a));
				g.fillRoundedRectangle(area, 3.0f);

				g.setFont(GLOBAL_BOLD_FONT());
				g.setColour(Colours::black);
				g.drawText(getName(), getLocalBounds().toFloat(), Justification::centred);
			}
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

				x += t->getWidth();
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

			LOAD_PATH_IF_URL("help", MainToolbarIcons::help);

			return p;
		}
	} factory;

	int addPosition;

	KeyboardPopup(NodeBase* container, int addPosition_):
		node(container),
		network(node->getRootNetwork()),
		list(network),
		tagList(network),
		addPosition(addPosition_),
		helpButton("help", this, factory)
	{
		setName("Create Node");

		addAndMakeVisible(helpButton);
		helpButton.setToggleModeWithColourChange(true);
		addAndMakeVisible(tagList);
		addAndMakeVisible(nodeEditor);

		nodeEditor.setFont(GLOBAL_MONOSPACE_FONT());
		nodeEditor.setColour(TextEditor::ColourIds::backgroundColourId, Colours::white.withAlpha(0.6f));
		nodeEditor.setSelectAllWhenFocused(true);
		nodeEditor.setColour(TextEditor::ColourIds::focusedOutlineColourId, Colour(SIGNAL_COLOUR));

		nodeEditor.addListener(this);
		addAndMakeVisible(viewport);
		addAndMakeVisible(helpViewport);
		viewport.setViewedComponent(&list, false);

		viewport.setScrollOnDragEnabled(true);
		helpViewport.setScrollOnDragEnabled(true);

		helpButton.addListener(this);
		setSize(tagList.getWidth(), 64 + 200);
		nodeEditor.addKeyListener(this);
		list.rebuild(getWidthForListItems());
	}

	void buttonClicked(Button* b)
	{
		if (help == nullptr)
		{
			help = new Help(network);
			help->showDoc(list.getCurrentText());
			helpViewport.setViewedComponent(help, false);
			helpViewport.setVisible(true);
			setSize(tagList.getTagWidth() + 100, tagList.getBottom() + 400);
			resized();
		}
		else
		{
			help = nullptr;
			setSize(tagList.getTagWidth(), tagList.getBottom() + viewport.getHeight());
			helpViewport.setVisible(false);
			resized();
		}
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

	

	struct Help : public Component
	{
		Help(DspNetwork* n);

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF262626).withAlpha(0.3f));

			renderer.draw(g, getLocalBounds().toFloat().reduced(10.0f));
		}

		void showDoc(const String& text);

		void rebuild(int maxWidth)
		{
			auto height = renderer.getHeightForWidth(maxWidth-20);
			setSize(maxWidth, height + 20);
		}

		File rootDirectory;

		MarkdownRenderer renderer;

		static void initGenerator(const File& f, MainController* mc);
		static bool initialised;

	};

	

	void textEditorTextChanged(TextEditor&) override
	{
		setSearchText(nodeEditor.getText());
	}

	int getWidthForListItems() const
	{
		return help != nullptr ? 0 : tagList.getWidth() - viewport.getScrollBarThickness();
	}

	void updateHelp()
	{
		if (help != nullptr)
			help->showDoc(list.getCurrentText());
	}

	void setSearchText(const String& text)
	{
		nodeEditor.setText(text, dontSendNotification);
		list.setSearchText(nodeEditor.getText());

		auto minY = tagList.getBottom();

		list.rebuild(getWidthForListItems());
		auto height = jmin(300, list.getHeight() + 10);

		if (help != nullptr)
			height = jmax(400, height);
		

		setSize(tagList.getWidth(), minY + height);

		updateHelp();

		resized();
	}

	void resized() override
	{
		auto b = getLocalBounds();

		auto top = b.removeFromTop(32).reduced(3);

		helpButton.setBounds(top.removeFromRight(top.getHeight()).reduced(2));
		top.removeFromLeft(top.getHeight());
		nodeEditor.setBounds(top);

		tagList.setBounds(b.removeFromTop(32));
		list.rebuild(getWidthForListItems());
		
		if (help != nullptr)
		{
			viewport.setBounds(b.removeFromLeft(list.getWidth() + viewport.getScrollBarThickness()));
			help->rebuild(b.getWidth() - helpViewport.getScrollBarThickness());
			helpViewport.setBounds(b);
		}
		else
		{
			helpViewport.setVisible(false);
			viewport.setBounds(b);
		}
	}

	void paint(Graphics& g) override
	{
		static const unsigned char searchIcon[] = { 110, 109, 0, 0, 144, 68, 0, 0, 48, 68, 98, 7, 31, 145, 68, 198, 170, 109, 68, 78, 223, 103, 68, 148, 132, 146, 68, 85, 107, 42, 68, 146, 2, 144, 68, 98, 54, 145, 219, 67, 43, 90, 143, 68, 66, 59, 103, 67, 117, 24, 100, 68, 78, 46, 128, 67, 210, 164, 39, 68, 98, 93, 50, 134, 67, 113, 58, 216, 67, 120, 192, 249, 67, 83, 151,
				103, 67, 206, 99, 56, 68, 244, 59, 128, 67, 98, 72, 209, 112, 68, 66, 60, 134, 67, 254, 238, 144, 68, 83, 128, 238, 67, 0, 0, 144, 68, 0, 0, 48, 68, 99, 109, 0, 0, 208, 68, 0, 0, 0, 195, 98, 14, 229, 208, 68, 70, 27, 117, 195, 211, 63, 187, 68, 146, 218, 151, 195, 167, 38, 179, 68, 23, 8, 77, 195, 98, 36, 92, 165, 68, 187, 58,
				191, 194, 127, 164, 151, 68, 251, 78, 102, 65, 0, 224, 137, 68, 0, 0, 248, 66, 98, 186, 89, 77, 68, 68, 20, 162, 194, 42, 153, 195, 67, 58, 106, 186, 193, 135, 70, 41, 67, 157, 224, 115, 67, 98, 13, 96, 218, 193, 104, 81, 235, 67, 243, 198, 99, 194, 8, 94, 78, 68, 70, 137, 213, 66, 112, 211, 134, 68, 98, 109, 211, 138, 67,
				218, 42, 170, 68, 245, 147, 37, 68, 128, 215, 185, 68, 117, 185, 113, 68, 28, 189, 169, 68, 98, 116, 250, 155, 68, 237, 26, 156, 68, 181, 145, 179, 68, 76, 44, 108, 68, 16, 184, 175, 68, 102, 10, 33, 68, 98, 249, 118, 174, 68, 137, 199, 2, 68, 156, 78, 169, 68, 210, 27, 202, 67, 0, 128, 160, 68, 0, 128, 152, 67, 98, 163,
				95, 175, 68, 72, 52, 56, 67, 78, 185, 190, 68, 124, 190, 133, 66, 147, 74, 205, 68, 52, 157, 96, 194, 98, 192, 27, 207, 68, 217, 22, 154, 194, 59, 9, 208, 68, 237, 54, 205, 194, 0, 0, 208, 68, 0, 0, 0, 195, 99, 101, 0, 0 };

		Path path;
		path.loadPathFromData(searchIcon, sizeof(searchIcon));
		path.applyTransform(AffineTransform::rotation(float_Pi));
		path.scaleToFit(8.0f, 8.0f, 16.0f, 16.0f, true);

		g.setColour(Colours::white.withAlpha(0.5f));
		g.fillPath(path);
	}

	struct PopupList : public Component
	{
		static constexpr int ItemHeight = 24;

		StringArray allIds;
		String searchTerm;

		PopupList(DspNetwork* n)
		{
			allIds = n->getListOfAllAvailableModuleIds();
			rebuild(0);
		}

		String getCurrentText() const
		{
			if (auto i = items[selectedIndex])
				return i->path;

			return {};
		}

		

		int selectedIndex = 0;

		int selectNext(bool next)
		{
			if (next)
				selectedIndex = jmin(items.size(), selectedIndex + 1);
			else
				selectedIndex = jmax(0, selectedIndex - 1);

			rebuild(maxWidth);

			return selectedIndex * ItemHeight;
		}

		void setSearchText(const String& text)
		{
			selectedIndex = 0;
			searchTerm = text.toLowerCase();
			rebuild(maxWidth);
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colours::black.withAlpha(0.2f));
		}

		struct Item : public Component
		{
			Item(const String& path_, bool isSelected_):
				path(path_),
				selected(isSelected_)
			{}

			void mouseDown(const MouseEvent& event)
			{
				findParentComponentOfClass<PopupList>()->setSelected(this);
			}

			void mouseUp(const MouseEvent& event)
			{
				if (!event.mouseWasDraggedSinceMouseDown())
				{
					findParentComponentOfClass<KeyboardPopup>()->addNodeAndClose(path);
				}
			}

			void paint(Graphics& g) override
			{
				if (selected)
				{
					g.fillAll(Colours::white.withAlpha(0.2f));
					g.setColour(Colours::white.withAlpha(0.1f));
					g.drawRect(getLocalBounds(), 1);
				}

				g.setFont(GLOBAL_BOLD_FONT());
				g.setColour(Colours::white.withAlpha(0.8f));
				g.drawText(path, getLocalBounds().toFloat().reduced(3.0f), Justification::centredLeft);

				g.setColour(Colours::black.withAlpha(0.1f));
				g.drawHorizontalLine(getHeight() - 1, 0.0f, (float)getWidth());

			}

			bool selected = false;
			String path;
		};

		void setSelected(Item* i)
		{
			for (auto item : items)
			{
				item->selected = item == i;
				item->repaint();
			}
		}

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

		void rebuild(int maxWidthToUse)
		{
			items.clear();

			int y = 0;
			
			maxWidth = maxWidthToUse;

			auto f = GLOBAL_MONOSPACE_FONT();

			for (auto id : allIds)
			{
				if (searchTerm.isNotEmpty() && !id.contains(searchTerm))
					continue;

				if (searchTerm == id)
					selectedIndex = items.size();

				auto newItem = new Item(id, selectedIndex == items.size());
				items.add(newItem);
				addAndMakeVisible(newItem);

				maxWidth = jmax(f.getStringWidth(id) + 20, maxWidth);
				y += ItemHeight;
			}

			setSize(maxWidth, y);
			resized();
		}

		OwnedArray<Item> items;
	};

	NodeBase::Ptr node;
	DspNetwork* network;
	
	TextEditor nodeEditor;
	TagList tagList;
	PopupList list;
	Viewport viewport;
	ScopedPointer<Help> help;
	Viewport helpViewport;
	
	
	
	HiseShapeButton helpButton;
	
};

class DspNetworkGraph : public Component,
	public AsyncUpdater,
	public DspNetwork::SelectionListener
{
public:
	struct ScrollableParent : public Component
	{
		ScrollableParent(DspNetwork* n);

		~ScrollableParent();

		void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
		void resized() override;

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFF262626));
		}

		DspNetworkGraph* getGraph() const
		{
			return dynamic_cast<DspNetworkGraph*>(viewport.getViewedComponent());
		}

		

		void setCurrentModalWindow(Component* newComponent, Rectangle<int> target)
		{
			currentModalWindow = nullptr;

			if (newComponent == nullptr)
			{
				removeChildComponent(&dark);
				viewport.getViewedComponent()->grabKeyboardFocus();
			}
				
			else
				addChildComponent(dark);

			Rectangle<int> cBounds;

			if (newComponent != nullptr)
			{
				currentModalWindow = new Holder(newComponent, target);

				
				addAndMakeVisible(currentModalWindow);

				currentModalWindow->updatePosition();

				currentModalWindow->setVisible(false);
				auto img = createComponentSnapshot(currentModalWindow->getBounds(), true);
				currentModalWindow->setVisible(true);

				currentModalWindow->setBackground(img);

				cBounds = currentModalWindow->getBounds();

				currentModalWindow->grabKeyboardFocus();
			}


			
			

			dark.setRuler(target, cBounds);
			dark.setVisible(currentModalWindow != nullptr);
			
			

		}

		struct Holder : public Component,
						public ComponentListener
		{
			static constexpr int margin = 20;
			static constexpr int headerHeight = 32;

			Holder(Component* content_, Rectangle<int> targetArea_):
				content(content_),
				target(targetArea_)
			{
				addAndMakeVisible(content);

				content->addComponentListener(this);

				updateSize();
			}

			Image bg;

			void setBackground(Image img)
			{
				bg = img;
				
				ImageConvolutionKernel kernel(7);
				kernel.createGaussianBlur(60.0f);
				kernel.applyToImage(bg, bg, { 0, 0, bg.getWidth(), bg.getHeight() });

				jassert(getWidth() == img.getWidth());
				repaint();
			}

			void updateSize()
			{
				setSize(content->getWidth() + margin, content->getHeight() + margin + headerHeight);
			}

			void componentMovedOrResized(Component&, bool , bool wasResized)
			{
				if (wasResized)
				{
					updateSize();
					updatePosition();
				}
			}

			void updatePosition()
			{
				if (getParentComponent() == nullptr)
				{
					jassertfalse;
					return;
				}

				bool alignY = target.getWidth() > target.getHeight();

				auto rect = target.withSizeKeepingCentre(getWidth(), getHeight());

				if (alignY)
				{
					auto delta = target.getHeight()/2 + 20;

					auto yRatio = (float)target.getY() / (float)getParentComponent()->getHeight();

					if (yRatio > 0.5f)
					{
						if (lockPosition) // this prevents jumping around...
						{
							updateShadow();
							return;
						}

						rect.translate(0, -rect.getHeight() / 2 - delta);
						lockPosition = true;
					}
					else
						rect.translate(0, rect.getHeight() / 2 + delta);
				}
				else
				{
					auto delta = target.getWidth() / 2 + 20;

					auto xRatio = (float)target.getX() / (float)getParentComponent()->getWidth();

					if (xRatio > 0.5f)
					{
						if (lockPosition) // this prevents jumping around...
						{
							updateShadow();
							return;
						}

						rect.translate(-rect.getWidth() / 2 - delta, 0);
						lockPosition = true;
					}
					else
						rect.translate(rect.getWidth() / 2 + delta, 0);

					rect.setY(target.getY());
				}

				setTopLeftPosition(rect.getTopLeft());
				updateShadow();
			}

			void updateShadow()
			{
				findParentComponentOfClass<ScrollableParent>()->dark.setRuler(target, getBoundsInParent());
			}

			void paint(Graphics& g) override
			{
				g.setColour(Colour(0xFF333333));

				auto b = getLocalBounds().toFloat();
				
				//g.fillRoundedRectangle(b, 3.0f);

				g.drawImageAt(bg, 0, 0);

				g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xb8383838)));

				g.setColour(Colours::white.withAlpha(0.1f));
				
				g.drawRect(b, 1.0f);

				b = b.removeFromTop(headerHeight).reduced(1.0f);



				g.fillRect(b);

				g.setColour(Colours::white.withAlpha(0.6f));
				g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));
				g.drawText(content->getName(), b, Justification::centred);

			}

			void mouseDown(const MouseEvent& event) override
			{
				dragger.startDraggingComponent(this, event);
			}

			void mouseDrag(const MouseEvent& event) override
			{
				dragger.dragComponent(this, event, nullptr);
				updateShadow();
			}

			void resized()
			{
				auto b = getLocalBounds();
				b.removeFromTop(headerHeight);
				b = b.reduced(margin / 2);
				content->setBounds(b);
			}

			Rectangle<int> target;

			ScopedPointer<Component> content;

			bool lockPosition = false;

			ComponentDragger dragger;
		};
		
		struct Dark : public Component
		{
			void paint(Graphics& g) override
			{
				g.fillAll(Colour(0xFF262626).withAlpha(0.5f));

				g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.3f));

				float width = 2.0f;

				g.fillRoundedRectangle(ruler, width);

				DropShadow sh;
				sh.colour = Colours::black.withAlpha(0.3f);
				sh.radius = 40;
				sh.drawForRectangle(g, shadow);
			}

			void setRuler(Rectangle<int> area, Rectangle<int> componentArea)
			{
				ruler = area.toFloat();
				shadow = componentArea;
				repaint();
			}

			void mouseDown(const MouseEvent& event) override
			{
				findParentComponentOfClass<ScrollableParent>()->setCurrentModalWindow(nullptr, {});
			}

			Rectangle<float> ruler;
			Rectangle<int> shadow;

		} dark;

		void centerCanvas();

		float zoomFactor = 1.0f;
		Viewport viewport;
		OpenGLContext context;

		ScopedPointer<Holder> currentModalWindow;
		
		
	};

	struct Actions
	{
		static void selectAndScrollToNode(DspNetworkGraph& g, NodeBase::Ptr node);

		static bool editNodeProperty(DspNetworkGraph& g);
		static bool foldSelection(DspNetworkGraph& g);
		static bool arrowKeyAction(DspNetworkGraph& g, const KeyPress& k);
		static bool showKeyboardPopup(DspNetworkGraph& g, KeyboardPopup::Mode mode);
		static bool duplicateSelection(DspNetworkGraph& g);
		static bool deselectAll(DspNetworkGraph& g);;
		static bool deleteSelection(DspNetworkGraph& g);
		static bool showJSONEditorForSelection(DspNetworkGraph& g);
		static bool undo(DspNetworkGraph& g);
		static bool redo(DspNetworkGraph& g);
	};

	void selectionChanged(const NodeBase::List&) override
	{
		
	}

	

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

	void paintOverChildren(Graphics& g) override;

	NodeComponent* getComponent(NodeBase::Ptr node);

	static void paintCable(Graphics& g, Rectangle<float> start, Rectangle<float> end, Colour c)
	{
		g.setColour(Colours::black);
		g.fillEllipse(start);
		g.setColour(Colour(0xFFAAAAAA));
		g.drawEllipse(start, 2.0f);

		g.setColour(Colours::black);
		g.fillEllipse(end);
		g.setColour(Colour(0xFFAAAAAA));
		g.drawEllipse(end, 2.0f);

		Path p;

		p.startNewSubPath(start.getCentre());

		Point<float> controlPoint(start.getX() + (end.getX() - start.getX()) / 2.0f, end.getY() + 100.0f);

		p.quadraticTo(controlPoint, end.getCentre());

		g.setColour(Colours::black);
		g.strokePath(p, PathStrokeType(3.0f));
		g.setColour(c);
		g.strokePath(p, PathStrokeType(2.0f));
	};

	static Rectangle<float> getCircle(Component* c, bool getKnobCircle=true)
	{
		if (auto n = c->findParentComponentOfClass<DspNetworkGraph>())
		{
			float width = 6.0f;
			float height = 6.0f;
			float y = getKnobCircle ? 66.0f : c->getHeight();
			float circleX = c->getLocalBounds().toFloat().getWidth() / 2.0f - width / 2.0f;

			Rectangle<float> circleBounds = { circleX, y, width, height };

			return n->getLocalArea(c, circleBounds.toNearestInt()).toFloat();
		}

		return {};
	};


	bool setCurrentlyDraggedComponent(NodeComponent* n);

	ValueTree dataReference;

	bool copyDraggedNode = false;

	valuetree::RecursivePropertyListener cableRepainter;
	valuetree::ChildListener rebuildListener;
	valuetree::RecursivePropertyListener resizeListener;

	valuetree::RecursiveTypedChildListener macroListener;

	ScopedPointer<NodeComponent> root;

	Component::SafePointer<ContainerComponent> currentDropTarget;
	ScopedPointer<NodeComponent> currentlyDraggedComponent;

	WeakReference<DspNetwork> network;
};


}

