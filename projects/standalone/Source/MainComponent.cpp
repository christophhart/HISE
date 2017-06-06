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
	
	

	createScriptingWorkspace(ib.getPanel(personaContainer));

	//ib.getPanel(scriptPanel)->setContent(scriptData);

	//jassert(!ib.getPanel(scriptPanel)->isEmpty());

	

	//auto samplePanel = ib.addChild < EmptyComponent>(personaContainer);

	createSamplerWorkspace(ib.getPanel(personaContainer));

	//ib.getPanel(samplePanel)->setContent(sampleData);
	//ib.getPanel(samplePanel)->getLayoutData().setId("SamplerWorkspace");

	

	const int customPanel = ib.addChild<HorizontalTile>(personaContainer);
	ib.getContent(customPanel)->setCustomTitle("Custom Workspace");
	ib.getPanel(customPanel)->getLayoutData().setId("CustomWorkspace");
	ib.getContainer(customPanel)->setIsDynamic(true);
	ib.addChild<EmptyComponent>(customPanel);

	return mainPanel;
}



Component* FloatingPanelTemplates::createSamplerWorkspace(FloatingTile* rootTile)
{
	MainController* mc = rootTile->findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor();

	jassert(mc != nullptr);

	FloatingInterfaceBuilder ib(rootTile);

	const int personaContainer = 0;

	const int samplePanel = ib.addChild <VerticalTile>(personaContainer);
	ib.setDynamic(samplePanel, false);
	ib.setId(samplePanel, "SamplerWorkspace");

	const int toggleBar = ib.addChild<VisibilityToggleBar>(samplePanel);
	const int fileBrowser = ib.addChild<GenericPanel<FileBrowser>>(samplePanel);
	const int samplePoolTable = ib.addChild<GenericPanel<SamplePoolTable>>(samplePanel);
	const int sampleHorizontal = ib.addChild<HorizontalTile>(samplePanel);
	ib.setDynamic(sampleHorizontal, false);
	ib.addChild<GlobalConnectorPanel<ModulatorSampler>>(sampleHorizontal);
	const int sampleEditor = ib.addChild<SampleEditorPanel>(sampleHorizontal);
	const int sampleVertical = ib.addChild<VerticalTile>(sampleHorizontal);
	ib.setDynamic(sampleVertical, false);
	const int sampleMapEditor = ib.addChild<SampleMapEditorPanel>(sampleVertical);
	const int samplerTable = ib.addChild<SamplerTablePanel>(sampleVertical);

	ib.setSizes(samplePanel, { 32.0, 280.0, 280.0, -0.5 });
	ib.getPanel(sampleHorizontal)->setCustomIcon((int)FloatingTileContent::Factory::PopupMenuOptions::SampleEditor);

	auto tb = ib.getContent<VisibilityToggleBar>(toggleBar);

	ib.setCustomPanels(toggleBar, { fileBrowser, samplePoolTable, sampleEditor, sampleMapEditor, samplerTable });
	
	ib.getContent<FloatingTileContent>(sampleEditor)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(sampleMapEditor)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(samplerTable)->setStyleProperty("showConnectionBar", false);

	return nullptr;
}

Component* FloatingPanelTemplates::createScriptingWorkspace(FloatingTile* rootTile)
{
	MainController* mc = rootTile->findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor();

	jassert(mc != nullptr);


	FloatingInterfaceBuilder ib(rootTile);

	const int personaContainer = 0;

	const int scriptPanel = ib.addChild <HorizontalTile>(personaContainer);
	ib.setDynamic(scriptPanel, false);

	ib.getPanel(scriptPanel)->getLayoutData().setId("ScriptingWorkspace");

	const int globalConnector = ib.addChild<GlobalConnectorPanel<JavascriptProcessor>>(scriptPanel);

	const int mainVertical = ib.addChild<VerticalTile>(scriptPanel);
	ib.setDynamic(mainVertical, false);

	const int toggleBar = ib.addChild < VisibilityToggleBar>(mainVertical);
	ib.addChild<GenericPanel<ApiCollection>>(mainVertical);
	ib.addChild<GenericPanel<FileBrowser>>(mainVertical);
	const int codeEditor = ib.addChild<HorizontalTile>(mainVertical);
	ib.setDynamic(codeEditor, false);
	

	const int codeVertical = ib.addChild<VerticalTile>(codeEditor);
	ib.setDynamic(codeVertical, false);
	const int codeTabs = ib.addChild<FloatingTabComponent>(codeVertical);
	const int firstEditor = ib.addChild<CodeEditorPanel>(codeTabs);
	const int variableWatch = ib.addChild<ScriptWatchTablePanel>(codeVertical);

	const int console = ib.addChild<ConsolePanel>(codeEditor);
	ib.setCustomName(codeEditor, "Code Editor");
	ib.setSizes(codeEditor, { -0.75, -0.25 });
	ib.setSizes(codeVertical, { -0.8, -0.2 });

	const int interfaceDesigner = ib.addChild <VerticalTile>(mainVertical);
	ib.setDynamic(interfaceDesigner, false);
	const int interfaceHorizontal = ib.addChild<HorizontalTile>(interfaceDesigner);
	ib.setDynamic(interfaceHorizontal, false);
	const int interfacePanel = ib.addChild<ScriptContentPanel>(interfaceHorizontal);
	const int onInitPanel = ib.addChild<CodeEditorPanel>(interfaceHorizontal);
	const int keyboard = ib.addChild<MidiKeyboardPanel>(interfaceHorizontal);
	ib.getPanel(keyboard)->getLayoutData().setForceFoldButton(true);

	ib.addChild<GenericPanel<ScriptComponentEditPanel>>(interfaceDesigner);
	
	ib.setSizes(interfaceHorizontal, { -0.5, 300.0, 72.0 });
	ib.setCustomName(interfaceHorizontal, "", { "Interface", "onInit Callback", "" });

	ib.setCustomName(interfaceDesigner, "Interface Designer");
	ib.setSizes(interfaceDesigner, { -0.8, -0.2 });

	ib.setFoldable(mainVertical, false, { false, true, true, true, true });
	ib.setFoldable(interfaceHorizontal, false, { false, true, true });

	ib.setSizes(mainVertical, { 32.0, 300.0, 300.0, -0.5, -0.5 });

	ib.getContent<VisibilityToggleBar>(toggleBar)->refreshButtons();

	ib.getContent<FloatingTileContent>(onInitPanel)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(interfacePanel)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(variableWatch)->setStyleProperty("showConnectionBar", false);
	//ib.getContent<FloatingTileContent>(onInitPanel)->setStyleProperty("showConnectionBar", false);
	
	return ib.getPanel(scriptPanel);
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
