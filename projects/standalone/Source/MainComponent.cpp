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

			var layoutDataObj = c.getDynamicObject()->getProperty("LayoutData");

			layoutDataObj.getDynamicObject()->setProperty("Size", -0.5);
		}
	}
	
	setContent(v);
}

Component* FloatingPanelTemplates::createHiseLayout(FloatingTile* rootTile)
{
	rootTile->setLayoutModeEnabled(false);

	FloatingInterfaceBuilder ib(rootTile);

	const int root = 0;

	ib.setNewContentType<HorizontalTile>(root);

	const int topBar = ib.addChild<MainTopBar>(root);

	ib.getContainer(root)->setIsDynamic(false);

	const int personaContainer = ib.addChild<VerticalTile>(root);
	ib.getContainer(personaContainer)->setIsDynamic(true);

	

	ib.setFoldable(root, false, { true, false });
	ib.setId(personaContainer, "PersonaContainer");

	ib.getContent(root)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId,
		HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

	auto mainPanel = createMainPanel(ib.getPanel(personaContainer));

	ib.getContainer(personaContainer)->setIsDynamic(false);

	
	
	
	File scriptJSON = File(PresetHandler::getDataFolder()).getChildFile("Workspaces/ScriptingWorkspace.json");
	File sampleJSON = File(PresetHandler::getDataFolder()).getChildFile("Workspaces/SamplerWorkspace.json");

	var scriptData = JSON::parse(scriptJSON.loadFileAsString());
	var sampleData = JSON::parse(sampleJSON.loadFileAsString());
	
	auto scriptPanel = ib.addChild < EmptyComponent>(personaContainer);

	ib.getPanel(scriptPanel)->setContent(scriptData);

	jassert(!ib.getPanel(scriptPanel)->isEmpty());

	ib.getPanel(scriptPanel)->getLayoutData().setId("ScriptingWorkspace");

	auto samplePanel = ib.addChild < EmptyComponent>(personaContainer);

	ib.getPanel(samplePanel)->setContent(sampleData);
	ib.getPanel(samplePanel)->getLayoutData().setId("SamplerWorkspace");

	jassert(!ib.getPanel(samplePanel)->isEmpty());

	const int customPanel = ib.addChild<HorizontalTile>(personaContainer);
	ib.getContent(customPanel)->setCustomTitle("Custom Workspace");
	ib.getPanel(customPanel)->getLayoutData().setId("CustomWorkspace");
	ib.getContainer(customPanel)->setIsDynamic(true);
	ib.addChild<EmptyComponent>(customPanel);

	return mainPanel;
}





Component* FloatingPanelTemplates::createMainPanel(FloatingTile* rootTile)
{
	MainController* mc = rootTile->findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor();

	jassert(mc != nullptr);


	FloatingInterfaceBuilder ib(rootTile);

	const int personaContainer = 0;

	const int firstVertical = ib.addChild<VerticalTile>(personaContainer);

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
	ib.getPanel(rightToolBar)->getLayoutData().setCurrentSize(28);
	ib.getContent(rightToolBar)->setStyleColour(VisibilityToggleBar::ColourIds::backgroundColour, Colour(0xFF222222));

	const int macroTable = ib.addChild<GenericPanel<MacroParameterTable>>(rightColumn);
	ib.getPanel(macroTable)->setCloseTogglesVisibility(true);
	ib.setId(macroTable, "MainMacroTable");

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

	ib.setVisibility(rightColumn, true, { true, false, false, false, false, false});

	
	


	

	ib.setSizes(firstVertical, { -0.5, 900.0, -0.5 }, dontSendNotification);
	

	const int mainArea = ib.addChild<EmptyComponent>(mainColumn);
	const int keyboard = ib.addChild<MidiKeyboardPanel>(mainColumn);

	

	ib.getContent(keyboard)->setStyleColour(ResizableFloatingTileContainer::ColourIds::backgroundColourId,
											HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

	ib.setFoldable(mainColumn, true, { false, true });
	ib.getPanel(mainColumn)->getLayoutData().setForceFoldButton(true);
	ib.getPanel(keyboard)->getLayoutData().setForceFoldButton(true);
	ib.getPanel(keyboard)->resized();

	ib.setCustomName(firstVertical, "", { "Left Panel", "", "Right Panel" });

	ib.setNewContentType<MainPanel>(mainArea);
	ib.getPanel(mainArea)->getLayoutData().setId("MainColumn");


	ib.getContent<VisibilityToggleBar>(rightToolBar)->refreshButtons();

	ib.finalizeAndReturnRoot();

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
