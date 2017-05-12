/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

// Use this to quickly scale the window
#define SCALE_2 0


namespace ColumnIcons
{
	static const unsigned char pathData1[] = { 110,109,164,3,39,66,229,223,154,65,108,112,148,163,67,229,223,154,65,108,112,148,163,67,180,201,59,67,108,168,3,39,66,180,201,59,67,99,101,0,0 };

	static const unsigned char pathData2[] = { 110,109,0,42,37,66,0,131,74,67,108,0,42,37,66,0,209,185,67,108,0,49,43,67,0,209,185,67,108,0,49,43,67,0,131,74,67,108,0,42,37,66,0,131,74,67,99,109,0,235,68,67,128,143,74,67,108,0,235,68,67,0,215,185,67,108,192,104,163,67,0,215,185,67,108,192,104,163,
		67,128,143,74,67,108,0,235,68,67,128,143,74,67,99,101,0,0 };

	static const unsigned char pathData3[] = { 110,109,0,160,41,66,0,167,199,67,108,0,160,41,66,64,45,14,68,108,0,65,252,66,64,45,14,68,108,0,65,252,66,0,167,199,67,108,0,160,41,66,0,167,199,67,99,109,128,181,14,67,64,173,199,67,108,128,181,14,67,64,48,14,68,108,0,110,98,67,64,48,14,68,108,0,110,
		98,67,64,173,199,67,108,128,181,14,67,64,173,199,67,99,109,0,139,115,67,192,250,199,67,108,0,139,115,67,0,87,14,68,108,192,161,163,67,0,87,14,68,108,192,161,163,67,192,250,199,67,108,0,139,115,67,192,250,199,67,99,101,0,0 };

	static const unsigned char pathData4[] = { 110,109,128,80,63,67,64,129,22,68,108,128,80,63,67,32,229,64,68,108,0,137,125,67,32,229,64,68,108,0,137,125,67,64,129,22,68,108,128,80,63,67,64,129,22,68,99,109,0,238,132,67,96,132,22,68,108,0,238,132,67,64,232,64,68,108,64,10,164,67,64,232,64,68,108,
		64,10,164,67,96,132,22,68,108,0,238,132,67,96,132,22,68,99,109,0,134,38,66,160,142,22,68,108,0,134,38,66,128,242,64,68,108,0,180,207,66,128,242,64,68,108,0,180,207,66,160,142,22,68,108,0,134,38,66,160,142,22,68,99,109,0,90,232,66,192,145,22,68,108,0,
		90,232,66,160,245,64,68,108,128,101,50,67,160,245,64,68,108,128,101,50,67,192,145,22,68,108,0,90,232,66,192,145,22,68,99,101,0,0 };

};

class FloatingFlexBoxWindow : public DocumentWindow
							  
{
public:

	FloatingFlexBoxWindow() :
		DocumentWindow("HISE Floating Window", Colour(0xFF333333), allButtons, true)
		
	{
		setContentOwned(new ContentComponent(), false);

		setResizable(true, true);
		setUsingNativeTitleBar(true);

		centreWithSize(1500, 1000);
	}

	void resized() override
	{
		getContentComponent()->setBounds(getLocalBounds());
	}
	
	class FlexboxableComponent : public Component,
								 public ButtonListener
	{
	public:

		FlexboxableComponent(Component* content_=nullptr) :
			content(content_),
			empty(new EmptyComponent())
		{
			if (content == nullptr)
				content = empty.get();

			addAndMakeVisible(content);

			addAndMakeVisible(deleteButton = new ShapeButton("Delete", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.9f), Colours::white));
			addAndMakeVisible(clearButton = new ShapeButton("Clear", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.9f), Colours::white));

			addAndMakeVisible(addButton = new ShapeButton("Add", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.9f), Colours::white));

			Path addPath;
			addPath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));

			addButton->setShape(addPath, true, true, true);
			addButton->addListener(this);

			Path deletePath;
			deletePath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));

			deleteButton->setShape(deletePath, true, true, true);

			
			static const unsigned char pathDataB[] = { 110,109,0,128,167,67,93,174,158,67,108,0,128,192,67,93,174,158,67,98,143,226,193,67,93,174,158,67,0,0,195,67,206,203,159,67,0,0,195,67,93,46,161,67,108,0,0,195,67,93,46,186,67,98,0,0,195,67,236,144,187,67,143,226,193,67,93,174,188,67,0,128,192,67,93,
				174,188,67,108,0,128,167,67,93,174,188,67,98,113,29,166,67,93,174,188,67,0,0,165,67,236,144,187,67,0,0,165,67,93,46,186,67,108,0,0,165,67,93,46,161,67,98,0,0,165,67,206,203,159,67,113,29,166,67,93,174,158,67,0,128,167,67,93,174,158,67,99,101,0,0 };

			Path pathB;
			pathB.loadPathFromData(pathDataB, sizeof(pathDataB));


			clearButton->setShape(pathB, true, true, true);

			clearButton->addListener(this);

			deleteButton->addListener(this);

		};

		void resized() override
		{
			deleteButton->setBounds(getWidth() - 20, 0, 16, 16);

			clearButton->setBounds(getWidth() - 80, 0, 16, 16);

			TabFlexboxableComponent* tabber = dynamic_cast<TabFlexboxableComponent*>(content.getComponent());
			
			if (tabber != nullptr)
			{
				auto b = tabber->getTabbedButtonBar().getTabButton(tabber->getTabbedButtonBar().getNumTabs() - 1);

				if(b != nullptr)
					addButton->setBounds(b->getRight() + 4, 2, 12, 12);

				else
					addButton->setBounds(2, 2, 12, 12);

				content->setBounds(0, 0, getWidth(), getHeight());
			}
			else
			{
				addButton->setBounds(2, 2, 12, 12);

				if (content.getComponent() != nullptr)
				{
					content->setBounds(0, 16, getWidth(), getHeight() - 16);
				}
			}
		}

		void buttonClicked(Button* b) override
		{
			if (b == addButton)
			{
				TabFlexboxableComponent* tabber = dynamic_cast<TabFlexboxableComponent*>(content.getComponent());

				if (tabber != nullptr)
				{
					tabber->addTabWithCloseButton(new EmptyComponent());
				}
				else
				{
					auto oldContent = content.getComponent();

					removeChildComponent(oldContent);

					addAndMakeVisible(content = new TabFlexboxableComponent());
					TabFlexboxableComponent* tabber = dynamic_cast<TabFlexboxableComponent*>(content.getComponent());

					addButton->toFront(false);
					deleteButton->toFront(false);

					tabber->addTabWithCloseButton(new EmptyComponent());
				}

				
				resized();
			}

			if (b == deleteButton)
			{
				findParentComponentOfClass<Column>()->removeFlexboxableComponent(this);
			}
		}

		class EmptyComponent : public Component
		{
		public:

			EmptyComponent() :
				Component("Empty")
			{};

			void paint(Graphics& g) override
			{
				Random r;

				

				g.setColour(Colour(r.nextInt()).withAlpha(0.1f));
				g.fillRect(getLocalBounds());

				g.setFont(GLOBAL_BOLD_FONT());
				g.setColour(Colours::white.withAlpha(0.3f));
				g.drawText("Right click to choose the window to show", getLocalBounds(), Justification::centred);
			}
		};

	protected:

		ScopedPointer<ShapeButton> addButton;
		ScopedPointer<ShapeButton> deleteButton;
		ScopedPointer<ShapeButton> clearButton;

	private:

		Component::SafePointer<Component> content;

		ScopedPointer<EmptyComponent> empty;

	};

	class TabFlexboxableComponent : public TabbedComponent
	{
	public:

		class CloseButton : public ShapeButton,
							public ButtonListener
		{
		public:

			CloseButton():
				ShapeButton("Close", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
			{
				
				Path p;
				p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
				setShape(p, false, true, true);

				addListener(this);
			}

			void buttonClicked(Button* b)
			{
				TabBarButton* button = findParentComponentOfClass<TabBarButton>();
				TabbedComponent* tab = findParentComponentOfClass<TabbedComponent>();

				auto* fb = findParentComponentOfClass<FlexboxableComponent>();
				tab->getTabbedButtonBar().removeTab(button->getIndex(), true);
				

				fb->resized();
			}

		};

		struct LookAndFeel : public LookAndFeel_V3
		{
			int getTabButtonBestWidth(TabBarButton &b, int tabDepth)
			{
				auto w = GLOBAL_BOLD_FONT().getStringWidthFloat(b.getButtonText());

				return w + 24;
			}

			void 	drawTabButton(TabBarButton &b, Graphics &g, bool isMouseOver, bool isMouseDown)
			{
				g.setColour(Colours::black.withAlpha(0.1f));

				auto a = b.getToggleState() ? 1.0f : (isMouseOver ? 0.8f : 0.6f);

				g.setColour(Colours::white.withAlpha(a));
				g.setFont(GLOBAL_BOLD_FONT());
				g.drawText(b.getButtonText(), 5, 0, b.getWidth() - 10, 16, Justification::centredLeft);
				
			}

			virtual Rectangle< int > 	getTabButtonExtraComponentBounds(const TabBarButton &b, Rectangle< int > &textArea, Component &extraComp)
			{
				return Rectangle<int>(b.getWidth() - 14, 2, 12, 12);
			}

			virtual void 	drawTabAreaBehindFrontButton(TabbedButtonBar &b, Graphics &g, int w, int h)
			{
				g.setColour(Colours::white.withAlpha(0.05f));
				
				if(b.getCurrentTabIndex() != -1)
					g.fillRect(b.getTabButton(b.getCurrentTabIndex())->getBoundsInParent());
			}
#if 0
			virtual void 	drawTabButtonText(TabBarButton &, Graphics &, bool isMouseOver, bool isMouseDown) = 0

			virtual void 	drawTabbedButtonBarBackground(TabbedButtonBar &, Graphics &) = 0

			virtual void 	drawTabAreaBehindFrontButton(TabbedButtonBar &, Graphics &, int w, int h) = 0
#endif
		};

		TabFlexboxableComponent() :
			TabbedComponent(TabbedButtonBar::TabsAtTop)
		{
			setOutline(0);

			setTabBarDepth(16);

			getTabbedButtonBar().setLookAndFeel(&laf);

			setColour(TabbedComponent::ColourIds::outlineColourId, Colours::transparentBlack);

			for (int i = 0; i < getTabbedButtonBar().getNumTabs(); i++)
			{
				getTabbedButtonBar().getTabButton(i)->setExtraComponent(new CloseButton(), TabBarButton::ExtraComponentPlacement::afterText);
			}

			
		}

		void addTabWithCloseButton(Component* newComponent)
		{
			int i = getNumTabs();

			addTab(newComponent->getName(), Colours::transparentBlack, newComponent, false);
			getTabbedButtonBar().getTabButton(i)->setExtraComponent(new CloseButton(), TabBarButton::ExtraComponentPlacement::afterText);
		}

	private:

		LookAndFeel laf;

	};

	class Column : public FlexboxableComponent
	{
	public:

		Column()

		{
			Random r;

			

			
		}

		void paint(Graphics& g) override
		{
			g.setColour(Colours::white.withAlpha(0.1f));
			g.drawRect(getLocalBounds());
		}

		void buttonClicked(Button* b) override
		{
			if (b == addButton)
			{
				addFlexboxableComponent(nullptr);
			}
			
		}

		void resized() override
		{
			addButton->setBounds(0, 0, 16, 16);
			
			deleteButton->setVisible(false);
			deleteButton->setBounds(0, 0, 0, 0);

			clearButton->setVisible(false);

			auto c = currentlyDisplayedComponents.getRawDataPointer();

			rowManager.layOutComponents(c, currentlyDisplayedComponents.size(), 0, 16, getWidth(), getHeight()-16, true, true);
		}

		void refreshLayout()
		{
			rowManager.clearAllItems();

			currentlyDisplayedComponents.clear();
			
			const int numVisibleRows = rowComponents.size();

			for (int i = 0; i < numVisibleRows; i++)
			{
				const bool isLastRow = i == rowComponents.size() - 1;

				currentlyDisplayedComponents.add(rowComponents[i]);
				currentlyDisplayedComponents.add(resizers[i]);

				rowManager.setItemLayout(2 * i, -0.1, -1.0, -0.5);

				resizers[i]->setVisible(!isLastRow);

				if(!isLastRow)
					rowManager.setItemLayout(2 * i + 1, 8, 8, 8);
			}

			currentlyDisplayedComponents.removeLast();

			resized();

		}

		void addFlexboxableComponent(Component* newComponent=nullptr)
		{
			rowComponents.add(new FlexboxableComponent(newComponent));
			addAndMakeVisible(rowComponents.getLast());

			resizers.add(new StretchableLayoutResizerBar(&rowManager, (rowComponents.size()-1)*2+1, false));
			addAndMakeVisible(resizers.getLast());

			refreshLayout();

			
		}

		void removeFlexboxableComponent(Component* toRemove)
		{
			rowComponents.removeObject(toRemove);
			resizers.removeLast();
			refreshLayout();
		}

	private:

		
		
		StretchableLayoutManager rowManager;

		Array<Component*> currentlyDisplayedComponents;

		OwnedArray<Component> rowComponents;
		OwnedArray<StretchableLayoutResizerBar> resizers;
	};

	class ContentComponent : public Component,
							 public ButtonListener
	{
	public:

		ContentComponent()
		{
			addAndMakeVisible(column1Button = new ShapeButton("1 Column", Colours::white.withAlpha(0.7f), Colours::white, Colours::white));
			addAndMakeVisible(column2Button = new ShapeButton("2 Column", Colours::white.withAlpha(0.7f), Colours::white, Colours::white));
			addAndMakeVisible(column3Button = new ShapeButton("3 Column", Colours::white.withAlpha(0.7f), Colours::white, Colours::white));
			addAndMakeVisible(column4Button = new ShapeButton("4 Column", Colours::white.withAlpha(0.7f), Colours::white, Colours::white));

			Path p1;
			p1.loadPathFromData(ColumnIcons::pathData1, sizeof(ColumnIcons::pathData1));

			column1Button->setShape(p1, false, false, true);

			Path p2;
			p2.loadPathFromData(ColumnIcons::pathData2, sizeof(ColumnIcons::pathData2));

			column2Button->setShape(p2, false, false, true);

			Path p3;
			p3.loadPathFromData(ColumnIcons::pathData3, sizeof(ColumnIcons::pathData3));

			column3Button->setShape(p3, false, false, true);

			Path p4;
			p4.loadPathFromData(ColumnIcons::pathData4, sizeof(ColumnIcons::pathData4));

			column4Button->setShape(p4, false, false, true);

			column1Button->addListener(this);
			column2Button->addListener(this);
			column3Button->addListener(this);
			column4Button->addListener(this);

			for (int i = 0; i < 4; i++)
			{
				columns.add(new Column());
				addAndMakeVisible(columns.getLast());
				resizers.add(new StretchableLayoutResizerBar(&columnManager, i * 2 + 1, true));
				addAndMakeVisible(resizers.getLast());
			}

			currentlyDisplayedComponents[0] = columns[0];
			currentlyDisplayedComponents[1] = resizers[0];
			currentlyDisplayedComponents[2] = columns[1];
			currentlyDisplayedComponents[3] = resizers[1];
			currentlyDisplayedComponents[4] = columns[2];
			currentlyDisplayedComponents[5] = resizers[2];
			currentlyDisplayedComponents[6] = columns[3];
			currentlyDisplayedComponents[7] = resizers[3];

			refreshLayout(numVisibleColumns);
		};

		void refreshLayout(int numToShow)
		{
			numVisibleColumns = jlimit<int>(0, 4, numToShow);

			columnManager.clearAllItems();

			for (int i = 0; i < 4; i++)
			{
				columns[i]->setVisible(false);
				resizers[i]->setVisible(false);
			}

			for (int i = 0; i < numToShow; i++)
			{
				columns[i]->setVisible(true);
				resizers[i]->setVisible(true);

				columnManager.setItemLayout(2 * i, -0.1, -1.0, -0.5);
				columnManager.setItemLayout(2 * i + 1, 8, 8, 8);
			}

			resized();
		}

		void buttonClicked(Button* b) override
		{
			if (b == column1Button) refreshLayout(1);
			if (b == column2Button) refreshLayout(2);
			if (b == column3Button) refreshLayout(3);
			if (b == column4Button) refreshLayout(4);
		}

		void paint(Graphics& g) override
		{
			
		}

		void resized() override
		{
			auto c = columnComponents.getRawDataPointer();

			const int buttonWidth = 30;
			const int buttonHeight = 20;

			int x = getWidth()/2 - (2 * buttonWidth + 15);

			column1Button->setBounds(x, 5, buttonWidth, buttonHeight);
			x += buttonWidth + 10;
			column2Button->setBounds(x, 5, buttonWidth, buttonHeight);
			x += buttonWidth + 10;
			column3Button->setBounds(x, 5, buttonWidth, buttonHeight);
			x += buttonWidth + 10;
			column4Button->setBounds(x, 5, buttonWidth, buttonHeight);
			x += buttonWidth + 10;

			

			columnManager.layOutComponents(currentlyDisplayedComponents, numVisibleColumns*2-1, 0, 30, getWidth(), getHeight()-30, false, true);
		}

	private:

		int numVisibleColumns = 1;

		OwnedArray<Component> columns;
		OwnedArray<StretchableLayoutResizerBar> resizers;

		Component* currentlyDisplayedComponents[8];

		ScopedPointer<ShapeButton> column1Button;
		ScopedPointer<ShapeButton> column2Button;
		ScopedPointer<ShapeButton> column3Button;
		ScopedPointer<ShapeButton> column4Button;

		StretchableLayoutManager columnManager;

		OwnedArray<Component> columnComponents;
	};

	

private:

	
	

};














//==============================================================================
MainContentComponent::MainContentComponent(const String &commandLine)
{
	auto f = new FloatingFlexBoxWindow();

	f->setVisible(true);

	f->addToDesktop();

	standaloneProcessor = new StandaloneProcessor();

	addAndMakeVisible(editor = standaloneProcessor->createEditor());

	setSize(editor->getWidth(), editor->getHeight());

	handleCommandLineArguments(commandLine);
}

MainContentComponent::~MainContentComponent()
{
	
	//open.detach();
	editor = nullptr;

	standaloneProcessor = nullptr;
}

void MainContentComponent::paint (Graphics& g)
{
	g.fillAll(Colours::lightgrey);
}

void MainContentComponent::resized()
{
#if SCALE_2
	editor->setSize(getWidth()*2, getHeight()*2);
#else
    editor->setSize(getWidth(), getHeight());
#endif

}
