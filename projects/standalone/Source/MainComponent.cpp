/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#define S(x) String(x, 2)


#include "MainComponent.h"

#if !PUT_FLOAT_IN_CODEBASE
#include "../../hi_core/hi_components/floating_layout/FloatingLayout.cpp"
#endif

// Use this to quickly scale the window
#define SCALE_2 0



Component* FloatingPanelTemplates::createMainPanel(FloatingTile* rootShell)
{
	rootShell->setLayoutModeEnabled(false, true);

	FloatingInterfaceBuilder ib(rootShell);

	const int root = 0;

	ib.setNewContentType<HorizontalTile>(root);

	const int topBar = ib.addChild<MainTopBar>(root);

	

	const int tabs = ib.addChild<FloatingTabComponent>(root);

	ib.setSizes(root, { 32.0, -1.0 });
	ib.setAbsoluteSize(root, { true, false });

	ib.getContainer(root)->setAllowInserting(false);

	ib.getPanel(topBar)->setCanDoLayoutMode(false);
	ib.getPanel(tabs)->setCanDoLayoutMode(false);

	ib.setFoldable(root, false, { false, false });

	const int firstVertical = ib.addChild<VerticalTile>(tabs);

	const int leftColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int mainColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int rightColumn = ib.addChild<HorizontalTile>(firstVertical);

	ib.setSizes(firstVertical, { -0.5, 900.0, -0.5 }, dontSendNotification);
	ib.setAbsoluteSize(firstVertical, { false, true, false }, dontSendNotification);
	ib.setLocked(firstVertical, { false, true, false }, sendNotification);
	ib.setDeletable(root, false, { false, false });
	ib.setDeletable(firstVertical, false, { false, false, false });

	const int mainArea = ib.addChild<EmptyComponent>(mainColumn);
	const int keyboard = ib.addChild<MidiKeyboardPanel>(mainColumn);

	ib.setSwappable(firstVertical, false, { false, false, false });
	ib.setSwappable(mainColumn, false, { false, false });
	ib.setFoldable(mainColumn, false, { false, false });

	ib.getPanel(firstVertical)->setDeletable(false);
	ib.getContainer(firstVertical)->setAllowInserting(false);
	ib.getPanel(mainColumn)->setReadOnly(true);
	//ib.getPanel(root)->setReadOnly(true);

	ib.setCustomName(firstVertical, "Main Workspace", { "Left Panel", "", "Right Panel" });

#if PUT_FLOAT_IN_CODEBASE
	ib.setNewContentType<MainPanel>(mainArea);
#endif

	ib.finalizeAndReturnRoot(true);

	return dynamic_cast<Component*>(ib.getPanel(mainArea)->getCurrentFloatingPanel());

}

void EmptyComponent::resized()
{
#if 0
	bool useRowLayout = getWidth() > getHeight();

	bool smallMode = false;

	bool isRow = flexBox.flexDirection == FlexBox::Direction::row;


	if (isRow && getWidth() < 200)
		smallMode = true;

	if (!isRow && getHeight() < 200)
		smallMode = true;

	flexBox.flexWrap = smallMode ? FlexBox::Wrap::noWrap : FlexBox::Wrap::wrap;


	int maxHeight = getHeight() - 40;

	if (useRowLayout)
	{
		flexBox.flexDirection = FlexBox::Direction::row;

		int width = (getWidth()-80) / flexBox.items.size();

		for (auto& item : flexBox.items)
		{
			item.width = width;
			item.maxWidth = width;
			item.minWidth = width;

			item.margin = 0;

			item.height = maxHeight;
			item.maxHeight = maxHeight;
			item.minHeight = maxHeight;
		}

	}
	else
	{
		flexBox.flexDirection = FlexBox::Direction::column;

		for (auto& item : flexBox.items)
		{
			
			item.minWidth = getWidth() - 40;
			item.width = getWidth() - 40;
			item.maxWidth = getWidth() - 40;

			int h = (maxHeight-30) / flexBox.items.size();

			item.height = h;

			item.minHeight = h;
			item.maxHeight = h;

		}
	}

	

	

	flexBox.performLayout(getLocalBounds().withTrimmedTop(16));
#endif
}


void EmptyComponent::Category::resized()
{
	//flexBox.flexDirection = getWidth() > 800 ? FlexBox::Direction::row : FlexBox::Direction::column;

	flexBox.alignContent = FlexBox::AlignContent::flexStart;
	flexBox.justifyContent = FlexBox::JustifyContent::center;


	bool smallMode = false;
	
	if (getWidth() < 200)
		smallMode = true;

	


	int maxWidth = getWidth() - 20;
	int maxHeight = getHeight() - 50;

	int s = 72;

	for (auto& item : flexBox.items)
	{
		item.height = s;
		item.maxHeight = s;
		item.minHeight = s;

		item.margin = 5;

		item.width = s;
		item.maxWidth = s;
		item.minWidth = s;
	}

	flexBox.justifyContent = FlexBox::JustifyContent::center;

	if (smallMode)
	{
		flexBox.flexWrap = FlexBox::Wrap::noWrap;
		flexBox.flexDirection = FlexBox::Direction::column;
		flexBox.alignContent = FlexBox::AlignContent::center;
		flexBox.justifyContent = FlexBox::JustifyContent::flexStart;
		
	}
	else
	{
		flexBox.flexWrap = FlexBox::Wrap::wrap;
		flexBox.flexDirection = FlexBox::Direction::row;
		flexBox.alignContent = FlexBox::AlignContent::flexStart;
		flexBox.justifyContent = FlexBox::JustifyContent::spaceAround;

		int numItems = flexBox.items.size();

		int rows = (int)sqrt((double)numItems);

		int columns = numItems / rows + 1;
	}

	

	flexBox.performLayout(getLocalBounds().withTrimmedTop(20));
}


void EmptyComponent::addCategory()
{
	flexBox.items.add(FlexItem().withMargin(30));

	auto& flexItem = flexBox.items.getReference(flexBox.items.size() - 1);

	flexItem.minWidth = 80;
	flexItem.maxWidth = 4000;
	flexItem.width = 400;

	flexItem.minHeight = 120;
	flexItem.maxHeight = 120;
	

	auto c = new Category(flexItem);
	categories.add(c);
	flexItem.associatedComponent = c;
	addAndMakeVisible(c);
}





//==============================================================================
MainContentComponent::MainContentComponent(const String &commandLine)
{
#if !PUT_FLOAT_IN_CODEBASE
	addAndMakeVisible(root = new FloatingTile(nullptr));

	root->setNewContent(HorizontalTile::getPanelId());

	setSize(1200, 800);
#else
	standaloneProcessor = new StandaloneProcessor();

	addAndMakeVisible(editor = standaloneProcessor->createEditor());

	setSize(editor->getWidth(), editor->getHeight());

	handleCommandLineArguments(commandLine);
#endif
}

MainContentComponent::~MainContentComponent()
{
	
	root = nullptr;

	//open.detach();
	editor = nullptr;

	

	standaloneProcessor = nullptr;
}

void MainContentComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
}

void MainContentComponent::resized()
{
#if !PUT_FLOAT_IN_CODEBASE
	root->setBounds(getLocalBounds());
#else
#if SCALE_2
	editor->setSize(getWidth()*2, getHeight()*2);
#else
    editor->setSize(getWidth(), getHeight());
#endif
#endif

}
