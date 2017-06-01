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



String FloatingTile::exportAsJSON() const
{
	var obj = getCurrentFloatingPanel()->toDynamicObject();

	auto json = JSON::toString(obj, false);

	return json;
}


void FloatingTile::loadFromJSON(const String& jsonData)
{
	var obj;

	auto result = JSON::parse(jsonData, obj);

	if (result.wasOk())
		setContent(obj);
}


void FloatingTile::swapContainerType(const Identifier& containerId)
{
	var v = getCurrentFloatingPanel()->toDynamicObject();

	v.getDynamicObject()->setProperty("Type", containerId.toString());

	if (auto list = v.getDynamicObject()->getProperty("Content").getArray())
	{
		for (int i = 0; i < list->size(); i++)
		{
			var c = list->getUnchecked(i);

			var layoutData = c.getDynamicObject()->getProperty("LayoutData");

			layoutData.getDynamicObject()->setProperty("Size", -0.5);
		}
	}
	
	setContent(v);
}



struct DebugPanelLookAndFeel : public ResizableFloatingTileContainer::LookAndFeel
{
	void paintBackground(Graphics& g, ResizableFloatingTileContainer& container) override
	{
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff373737)));
		g.fillRect(container.getContainerBounds());

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff2c2c2c)));

		//g.drawVerticalLine(0, 0.0f, (float)container.getHeight());

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff5f5f5f)));

		//g.drawVerticalLine(container.getWidth() - 1, 0.0f, (float)container.getHeight());
	}
};

Component* FloatingPanelTemplates::createMainPanel(FloatingTile* rootTile)
{
	MainController* mc = rootTile->findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor();

	jassert(mc != nullptr);

	rootTile->setLayoutModeEnabled(false, true);

	FloatingInterfaceBuilder ib(rootTile);

	const int root = 0;

	ib.setNewContentType<HorizontalTile>(root);

	const int topBar = ib.addChild<MainTopBar>(root);

	ib.getContainer(root)->setIsDynamic(false);

	const int tabs = ib.addChild<FloatingTabComponent>(root);
	ib.getContainer(tabs)->setIsDynamic(true);

	ib.setSizes(root, { 32.0, -1.0 });
	
	ib.setFoldable(root, false, { false, false });
	ib.setId(tabs, "MainWorkspaceTabs");

	const int firstVertical = ib.addChild<VerticalTile>(tabs);

	ib.getContainer(firstVertical)->setIsDynamic(false);
	ib.getPanel(firstVertical)->setVital(true);
	ib.setId(firstVertical, "MainWorkspace");

	ib.getContent(firstVertical)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId, 
											  HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

	const int leftColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int mainColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int rightColumn = ib.addChild<HorizontalTile>(firstVertical);

	

	ib.getContainer(leftColumn)->setIsDynamic(true);
	ib.getContainer(mainColumn)->setIsDynamic(false);
	ib.getContainer(rightColumn)->setIsDynamic(false);

	ib.getContent(leftColumn)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId, Colour(0xFF222222));
	ib.getPanel(leftColumn)->getLayoutData().setMinSize(150);
	ib.getPanel(leftColumn)->setCanBeFolded(true);
	ib.setId(leftColumn, "MainLeftColumn");

	ib.getContent(mainColumn)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId, 
											  HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

	
	ib.getContent(rightColumn)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId, Colour(0xFF222222));
	ib.getPanel(rightColumn)->getLayoutData().setMinSize(150);
	ib.getPanel(rightColumn)->setCanBeFolded(true);
	ib.setId(rightColumn, "MainRightColumn");

	const int rightToolBar = ib.addChild<VisibilityToggleBar>(rightColumn);
	
	const int scriptWatchTable = ib.addChild<ScriptWatchTablePanel>(rightColumn);
	ib.getPanel(scriptWatchTable)->setCloseTogglesVisibility(true);

	const int editPanel = ib.addChild<GenericPanel<ScriptComponentEditPanel>>(rightColumn);
	ib.getPanel(editPanel)->setCloseTogglesVisibility(true);

	const int plotter = ib.addChild<PlotterPanel>(rightColumn);
	ib.getPanel(plotter)->getLayoutData().setCurrentSize(300.0);
	ib.getPanel(plotter)->setCloseTogglesVisibility(true);

	const int console = ib.addChild<ConsolePanel>(rightColumn);
	mc->getConsoleHandler().setMainConsole(ib.getContent<ConsolePanel>(console)->getConsole());
	ib.getPanel(console)->setCloseTogglesVisibility(true);

	ib.setVisibility(rightColumn, true, { true, false, false, false, false});

	
	


	ib.getContent(root)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId, 
									    HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId));

	ib.setSizes(firstVertical, { -0.5, 900.0, -0.5 }, dontSendNotification);
	

	const int mainArea = ib.addChild<EmptyComponent>(mainColumn);
	const int keyboard = ib.addChild<MidiKeyboardPanel>(mainColumn);

	

	ib.getContent(keyboard)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId,
											HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

	ib.setFoldable(mainColumn, false, { false, false });

	ib.setCustomName(firstVertical, "Main Workspace", { "Left Panel", "", "Right Panel" });

	ib.setNewContentType<MainPanel>(mainArea);
	ib.getPanel(mainArea)->getLayoutData().setId("MainColumn");


	ib.getContent<VisibilityToggleBar>(rightToolBar)->refreshButtons();

	ib.finalizeAndReturnRoot(true);

	jassert(BackendPanelHelpers::getMainTabComponent(rootTile) != nullptr);
	jassert(BackendPanelHelpers::getMainLeftColumn(rootTile) != nullptr);
	jassert(BackendPanelHelpers::getMainRightColumn(rootTile) != nullptr);
	

	return dynamic_cast<Component*>(ib.getPanel(mainArea)->getCurrentFloatingPanel());
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
