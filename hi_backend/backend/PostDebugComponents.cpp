
namespace hise {
using namespace juce;

Component* FloatingPanelTemplates::createCodeEditorPanel(FloatingTile* root)
{
#if USE_BACKEND
	FloatingInterfaceBuilder ib(root);

	const int codeEditor = ib.addChild<HorizontalTile>(0);
	ib.setDynamic(codeEditor, false);

	const int codeVertical = ib.addChild<VerticalTile>(codeEditor);
	ib.setDynamic(codeVertical, false);
	const int codeTabs = ib.addChild<FloatingTabComponent>(codeVertical);

	ib.getPanel(codeTabs)->getLayoutData().setKeyPress(true, FloatingTileKeyPressIds::focus_editor);
	ib.getPanel(codeEditor)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_editor);
	ib.getContent<FloatingTabComponent>(codeTabs)->setCycleKeyPress(FloatingTileKeyPressIds::cycle_editor);

	const int navTabs = ib.addChild<FloatingTabComponent>(codeVertical);

	ib.setId(codeTabs, "ScriptEditorTabs");
	ib.addChild<CodeEditorPanel>(codeTabs);
    ib.addChild<SnexEditorPanel>(codeTabs);
    
	

	const int variableWatch = ib.addChild<ScriptWatchTablePanel>(navTabs);
	ib.setDynamic(navTabs, false);

	const int broadcasterMap = ib.addChild<ScriptingObjects::ScriptBroadcasterPanel>(navTabs);
	const int consoleId = ib.addChild<ConsolePanel>(codeEditor);

    ib.getPanel(broadcasterMap)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_map);

	ib.getPanel(variableWatch)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_watch);

	ib.getPanel(consoleId)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_console);

	ib.setCustomName(codeEditor, "Code Editor");
	ib.setSizes(codeEditor, { -0.7, -0.3 });
	ib.setSizes(codeVertical, { -0.8, -0.2 });



	ib.getContent<FloatingTileContent>(variableWatch)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(broadcasterMap)->setStyleProperty("showConnectionBar", false);

	ib.getContent(codeVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);
	ib.getContent(codeEditor)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);
	ib.setId(codeEditor, "ScriptingWorkspaceCodeEditor");

	ib.getPanel(codeEditor)->getLayoutData().setVisible(true);

	ib.setFoldable(codeVertical, false, { false, true });

	return ib.getPanel(codeEditor);
#endif

	return nullptr;
}



Component* FloatingPanelTemplates::createScriptingWorkspace(FloatingTile* rootTile)
{
#if USE_BACKEND
	
	FloatingInterfaceBuilder ib(rootTile);

	const int personaContainer = 0;

	const int mainVertical = ib.addChild<VerticalTile>(personaContainer);
	ib.setDynamic(mainVertical, false);
	ib.setId(mainVertical, "ScriptingWorkspace");

	

	const int scriptNode = ib.addChild<VerticalTile>(mainVertical);

	{
		ib.setDynamic(scriptNode, false);
		ib.getContent(scriptNode)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		const int nodeList = ib.addChild<scriptnode::DspNodeList::Panel>(scriptNode);
		
		const int interfacePanel = ib.addChild<scriptnode::DspNetworkGraphPanel>(scriptNode);
		
		ib.setCustomName(scriptNode, "Scriptnode Workspace");

		ib.setCustomName(nodeList, "Node List");
		
		ib.getPanel(nodeList)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_list);

		const int nodePropertyEditor = ib.addChild<scriptnode::NodePropertyPanel>(scriptNode);

		ib.setCustomName(nodePropertyEditor, "Node Properties");

		ib.getPanel(nodePropertyEditor)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_properties);

		ib.getContent<FloatingTileContent>(interfacePanel)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(nodeList)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(nodePropertyEditor)->setStyleProperty("showConnectionBar", false);

		ib.setFoldable(scriptNode, false, {true, false, true});

        ib.getPanel(interfacePanel)->setForceShowTitle(false);
        


		ib.setSizes(scriptNode, { 200, -0.7, -0.15 });

		ib.setId(scriptNode, "ScriptingWorkspaceScriptnode");
	}

	{
		// Interface Designer
		const int interfaceDesigner = ib.addChild <VerticalTile>(mainVertical);
		ib.setDynamic(interfaceDesigner, false);

		ib.getContent(interfaceDesigner)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);
		ib.getContent(mainVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		const int componentList = ib.addChild<ScriptComponentList::Panel>(interfaceDesigner);

		ib.getPanel(componentList)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_list);

		const int interfaceHorizontal = ib.addChild<HorizontalTile>(interfaceDesigner);

		ib.getContent(interfaceHorizontal)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);
		ib.getContent(mainVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		ib.setDynamic(interfaceHorizontal, false);
		const int interfacePanel = ib.addChild<ScriptContentPanel>(interfaceHorizontal);
		
		const int propertyEditor = ib.addChild<ScriptComponentEditPanel::Panel>(interfaceDesigner);

		ib.getPanel(propertyEditor)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_properties);

		ib.setSizes(interfaceHorizontal, { -0.5 });
		ib.setCustomName(interfaceHorizontal, "", { "Canvas"});

		ib.setCustomName(interfaceDesigner, "Interface Designer");
		ib.setCustomName(propertyEditor, "Property Editor");
		ib.setCustomName(componentList, "Component List");

		ib.getPanel(interfacePanel)->getLayoutData().setKeyPress(true, FloatingTileKeyPressIds::focus_interface);
		rootTile->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_interface);

		
		ib.setId(interfaceDesigner, "ScriptingWorkspaceInterfaceDesigner");

		ib.setSizes(interfaceDesigner, { -0.15, -0.7, -0.15 });

		ib.setFoldable(mainVertical, false, { true, true });
		ib.setFoldable(interfaceHorizontal, false, { false });
		ib.setSizes(mainVertical, { -0.33, -0.33 });
		ib.getPanel(scriptNode)->getLayoutData().setVisible(false);

		ib.getPanel(interfacePanel)->setForceShowTitle(false);
		
		ib.getContent<FloatingTileContent>(interfacePanel)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(componentList)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(propertyEditor)->setStyleProperty("showConnectionBar", false);
	}
	
	return ib.getPanel(mainVertical);
#else

	ignoreUnused(rootTile);

	return nullptr;
#endif
}





Component* FloatingPanelTemplates::createMainPanel(FloatingTile* rootTile)
{
#if USE_BACKEND
	FloatingInterfaceBuilder ib(rootTile);

	const int personaContainer = 0;

	const int firstVertical = ib.addChild<VerticalTile>(personaContainer);

	ib.getContainer(firstVertical)->setIsDynamic(false);
	ib.getPanel(firstVertical)->setVital(true);
	ib.setId(firstVertical, "MainWorkspace");

	ib.getContent(personaContainer)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF404040));

	const int leftColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int mainColumn = ib.addChild<HorizontalTile>(firstVertical);
	const int rightColumn = ib.addChild<HorizontalTile>(firstVertical);

	ib.getContainer(leftColumn)->setIsDynamic(true);
	ib.getContainer(mainColumn)->setIsDynamic(false);
	ib.getContainer(rightColumn)->setIsDynamic(false);

	ib.getContent(leftColumn)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF222222));
	ib.getPanel(leftColumn)->getLayoutData().setMinSize(150);
	ib.getPanel(leftColumn)->setCanBeFolded(true);
	ib.getPanel(leftColumn)->setVital(true);
	ib.setId(leftColumn, "MainLeftColumn");

	ib.getContent(mainColumn)->setPanelColour(FloatingTileContent::PanelColourId::bgColour,
		HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));


	ib.getContent(rightColumn)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF222222));
	ib.getPanel(rightColumn)->getLayoutData().setMinSize(150);
	ib.getPanel(rightColumn)->setCanBeFolded(true);
	ib.setId(rightColumn, "MainRightColumn");
	ib.getPanel(rightColumn)->setVital(true);

	const int rightToolBar = ib.addChild<VisibilityToggleBar>(rightColumn);
	ib.getPanel(rightToolBar)->getLayoutData().setCurrentSize(28);
	ib.getContent(rightToolBar)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF222222));

	const int macroTable = ib.addChild<GenericPanel<MacroParameterTable>>(rightColumn);
	ib.getPanel(macroTable)->setCloseTogglesVisibility(true);
	ib.setId(macroTable, "MainMacroTable");

	const int scriptWatchTable = ib.addChild<ScriptWatchTablePanel>(rightColumn);
	ib.getPanel(scriptWatchTable)->setCloseTogglesVisibility(true);

	const int editPanel = ib.addChild<ScriptComponentEditPanel::Panel>(rightColumn);
	ib.getPanel(editPanel)->setCloseTogglesVisibility(true);

	const int plotter = ib.addChild<PlotterPanel>(rightColumn);
	ib.getPanel(plotter)->getLayoutData().setCurrentSize(300.0);
	ib.getPanel(plotter)->setCloseTogglesVisibility(true);

	const int console = ib.addChild<ConsolePanel>(rightColumn);
	
	ib.setId(console, "MainConsole");


	ib.getPanel(console)->setCloseTogglesVisibility(true);

	ib.setVisibility(rightColumn, true, { true, false, false, false, false, false });

	ib.setSizes(firstVertical, { -0.5, 900.0, -0.5 }, dontSendNotification);


	const int mainArea = ib.addChild<BackendProcessorEditor>(mainColumn);

	

	const int keyboard = ib.addChild<MidiKeyboardPanel>(mainColumn);


	ib.getContent(keyboard)->setPanelColour(FloatingTileContent::PanelColourId::bgColour,
		HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

	ib.setFoldable(mainColumn, true, { false, true });
	ib.getPanel(mainColumn)->getLayoutData().setForceFoldButton(true);
	ib.getPanel(keyboard)->getLayoutData().setForceFoldButton(true);
	ib.getPanel(keyboard)->resized();
	

	ib.setCustomName(firstVertical, "", { "Left Panel", "", "Right Panel" });

	
	ib.getPanel(mainArea)->getLayoutData().setId("MainColumn");


	ib.getContent<VisibilityToggleBar>(rightToolBar)->refreshButtons();

	ib.finalizeAndReturnRoot();

	jassert(BackendPanelHelpers::getMainTabComponent(rootTile) != nullptr);
	jassert(BackendPanelHelpers::getMainLeftColumn(rootTile) != nullptr);
	jassert(BackendPanelHelpers::getMainRightColumn(rootTile) != nullptr);


	return dynamic_cast<Component*>(ib.getPanel(mainArea)->getCurrentFloatingPanel());
#else
	ignoreUnused(rootTile);

	return nullptr;
#endif
}


Component* FloatingPanelTemplates::createHiseLayout(FloatingTile* rootTile)
{
#if USE_BACKEND
	rootTile->setLayoutModeEnabled(false);

	FloatingInterfaceBuilder ib(rootTile);

	const int root = 0;

	ib.setNewContentType<HorizontalTile>(root);

    ib.addChild<MainTopBar>(root);

	ib.getContainer(root)->setIsDynamic(false);

	auto con = ib.addChild<GlobalConnectorPanel<JavascriptProcessor>>(root);

	ib.getContent<GlobalConnectorPanel<JavascriptProcessor>>(con)->setFollowWorkspace(true);

	ib.setVisibility(con, false, {});

	const int masterVertical = ib.addChild<VerticalTile>(root);

	

	

	auto leftTab = ib.addChild<FloatingTabComponent>(masterVertical);
	ib.getPanel(leftTab)->getLayoutData().setKeyPress(true, FloatingTileKeyPressIds::focus_browser);
	ib.getPanel(leftTab)->getLayoutData().setKeyPress(false, FloatingTileKeyPressIds::fold_browser);
	ib.getContent<FloatingTabComponent>(leftTab)->setCycleKeyPress(FloatingTileKeyPressIds::cycle_browser);

	const int swappableVertical = ib.addChild<VerticalTile>(masterVertical);

	

	ib.getContent(masterVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF404040));
	ib.getContent(swappableVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF404040));

	ib.getPanel(swappableVertical)->setForceShowTitle(false);

	

	ib.getContent(leftTab)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xff353535));
	ib.getContent(leftTab)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF232323));
	const int mainArea = ib.addChild<GenericPanel<PatchBrowser>>(leftTab);
    
    const int fileBrowserTab = ib.addChild<HorizontalTile>(leftTab);
    
	const int fileBrowser = ib.addChild<GenericPanel<FileBrowser>>(fileBrowserTab);
    ib.addChild<ExpansionEditBar>(fileBrowserTab);
    ib.setDynamic(fileBrowserTab, false);
    ib.getPanel(fileBrowser)->setForceShowTitle(false);
    ib.setFoldable(fileBrowserTab, false, {false, false});
    auto apiBrowser = ib.addChild<GenericPanel<ApiCollection>>(leftTab);
    
    ib.setCustomName(mainArea, "Module Tree");
    ib.setCustomName(fileBrowserTab, "Project Directory");
    ib.setCustomName(apiBrowser, "API");
    
	
	ib.setDynamic(leftTab, false);
	ib.setDynamic(masterVertical, false);
	ib.setDynamic(swappableVertical, false);
	ib.setId(swappableVertical, "SwappableContainer");

	ib.getContent<FloatingTabComponent>(leftTab)->setCurrentTabIndex(0);

	createCodeEditorPanel(ib.getPanel(swappableVertical));

	const int personaContainer = ib.addChild<VerticalTile>(swappableVertical);
	ib.getContainer(personaContainer)->setIsDynamic(true);

	ib.getPanel(personaContainer)->setForceShowTitle(false);
	
	ib.setId(personaContainer, "PersonaContainer");

	ib.getContent(root)->setPanelColour(FloatingTileContent::PanelColourId::bgColour,
		Colour(0xFF404040));

	ib.getContainer(personaContainer)->setIsDynamic(false);

    File scriptJSON = ProjectHandler::getAppDataDirectory(nullptr).getChildFile("Workspaces/ScriptingWorkspace.json");
	File sampleJSON = ProjectHandler::getAppDataDirectory(nullptr).getChildFile("Workspaces/SamplerWorkspace.json");

	var scriptData = JSON::parse(scriptJSON.loadFileAsString());
	var sampleData = JSON::parse(sampleJSON.loadFileAsString());

	createScriptingWorkspace(ib.getPanel(personaContainer));
	createSamplerWorkspace(ib.getPanel(personaContainer));

	
	const int customPanel = ib.addChild<HorizontalTile>(personaContainer);
	ib.getContent(customPanel)->setCustomTitle("Custom Workspace");
	ib.getPanel(customPanel)->getLayoutData().setId("CustomWorkspace");
	ib.getContainer(customPanel)->setIsDynamic(true);
	ib.addChild<EmptyComponent>(customPanel);

	auto be = ib.addChild<BackendProcessorEditor>(root);

	auto mainEditor = ib.getContent<BackendProcessorEditor>(be);

	ib.setSizes(masterVertical, { 300.0, -0.5 });
	ib.setFolded(masterVertical, { false, false });
	ib.setFoldable(masterVertical, false, { true, false });

	ib.setFolded(swappableVertical, { true, false });

	ib.setVisibility(be, false, {});

	return mainEditor;

#else
	return rootTile;
#endif
}



Component* FloatingPanelTemplates::createSamplerWorkspace(FloatingTile* rootTile)
{
#if USE_BACKEND
	
	FloatingInterfaceBuilder ib(rootTile);

	const int personaContainer = 0;

	const int samplePanel = ib.addChild <VerticalTile>(personaContainer);
	ib.setDynamic(samplePanel, false);
	ib.setId(samplePanel, "SamplerWorkspace");

	ib.getContent(samplePanel)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF262626));
    
    
    
	const int sampleHorizontal = ib.addChild<HorizontalTile>(samplePanel);
	ib.setDynamic(sampleHorizontal, false);
	auto con = ib.addChild<GlobalConnectorPanel<ModulatorSampler>>(sampleHorizontal);
    
	ib.getContent<GlobalConnectorPanel<ModulatorSampler>>(con)->setFollowWorkspace(true);

    ib.setVisibility(con, false, {});
    
    ib.getContent(samplePanel)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF404040));

    ib.getContent(sampleHorizontal)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF404040));

    
    
	const int sampleEditor = ib.addChild<SampleEditorPanel>(sampleHorizontal);
	const int sampleVertical = ib.addChild<VerticalTile>(sampleHorizontal);
	ib.setDynamic(sampleVertical, false);
    
    ib.getContent(sampleVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF404040));
    
    
	const int sampleMapEditor = ib.addChild<SampleMapEditorPanel>(sampleVertical);
	const int samplerTable = ib.addChild<SamplerTablePanel>(sampleVertical);

    ib.setSizes(sampleVertical, {-0.7, -0.3});
    
    ib.setFoldable(sampleVertical, false, {true, true});
    
	ib.setSizes(samplePanel, { -0.5 });
	ib.getPanel(sampleHorizontal)->setCustomIcon((int)FloatingTileContent::Factory::PopupMenuOptions::SampleEditor);

	ib.setId(sampleEditor, "MainSampleEditor");
	ib.setId(sampleMapEditor, "MainSampleMapEditor");
	ib.setId(samplerTable, "MainSamplerTable");
	
	ib.getContent<FloatingTileContent>(sampleEditor)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(sampleMapEditor)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(samplerTable)->setStyleProperty("showConnectionBar", false);
#endif

	ignoreUnused(rootTile);

	return nullptr;
}

void FloatingTileContent::Factory::registerBackendPanelTypes()
{
	registerType<GenericPanel<MacroComponent>>(PopupMenuOptions::MacroControls);
	registerType < GenericPanel<MacroParameterTable>>(PopupMenuOptions::MacroTable);

	registerType<GenericPanel<ApiCollection>>(PopupMenuOptions::ApiCollection);
	registerType<scriptnode::DspNodeList::Panel>(PopupMenuOptions::DspNodeList);
	registerType<GenericPanel<PatchBrowser>>(PopupMenuOptions::PatchBrowser);
	registerType<GenericPanel<AutomationDataBrowser>>(PopupMenuOptions::AutomationDataBrowser);
	registerType<GenericPanel<FileBrowser>>(PopupMenuOptions::FileBrowser);
	registerType<GenericPanel<SamplePoolTable>>(PopupMenuOptions::SamplePoolTable);
	
	registerType<MainTopBar>(PopupMenuOptions::MenuCommandOffset);
	registerType<BackendProcessorEditor>(PopupMenuOptions::MenuCommandOffset);
	registerType<ScriptWatchTablePanel>(PopupMenuOptions::ScriptWatchTable);
	registerType<ConsolePanel>(PopupMenuOptions::Console);
	registerType<ScriptComponentList::Panel>(PopupMenuOptions::ScriptComponentList);
	registerType<MarkdownEditorPanel>(PopupMenuOptions::MarkdownEditor);

	registerType<ComplexDataManager>(PopupMenuOptions::ComplexDataManager);
	registerType<ServerControllerPanel>(PopupMenuOptions::ServerController);
	registerType<scriptnode::DspNetworkGraphPanel>(PopupMenuOptions::DspNetworkGraph);
	registerType<scriptnode::NodePropertyPanel>(PopupMenuOptions::DspNodeParameterEditor);
    registerType<scriptnode::FaustEditorPanel>(PopupMenuOptions::DspFaustEditorPanel);

	registerType<ScriptingObjects::ScriptBroadcasterPanel>(PopupMenuOptions::ScriptBroadcasterMap);

	registerType<GenericPanel<PerfettoWebviewer>>(PopupMenuOptions::PerfettoViewer);

	registerType<SamplerTablePanel>(PopupMenuOptions::SamplerTable);
	registerType<GlobalConnectorPanel<JavascriptProcessor>>(PopupMenuOptions::ScriptConnectorPanel);
	registerType<CodeEditorPanel>(PopupMenuOptions::ScriptEditor);
	registerType<ScriptContentPanel>(PopupMenuOptions::ScriptContent);
	registerType<OSCLogger>(PopupMenuOptions::OSCLogger);
	registerType<ScriptComponentEditPanel::Panel>(PopupMenuOptions::ScriptComponentEditPanel);
	registerType<ApplicationCommandButtonPanel>(PopupMenuOptions::MenuCommandOffset);
}

bool FloatingTileContent::Factory::handleBackendMenu(PopupMenuOptions r, FloatingTile* parent)
{
	switch(r)
	{
	case PopupMenuOptions::ScriptComponentList: parent->setNewContent(GET_PANEL_NAME(ScriptComponentList::Panel)); return true;
	case PopupMenuOptions::ScriptComponentEditPanel: parent->setNewContent(GET_PANEL_NAME(ScriptComponentEditPanel::Panel)); return true;
	case PopupMenuOptions::DspNodeList:			parent->setNewContent(GET_PANEL_NAME(scriptnode::DspNodeList::Panel)); return true;
	case PopupMenuOptions::ApiCollection:		parent->setNewContent(GET_PANEL_NAME(GenericPanel<ApiCollection>)); return true;
	case PopupMenuOptions::PatchBrowser:		parent->setNewContent(GET_PANEL_NAME(GenericPanel<PatchBrowser>)); return true;
	case PopupMenuOptions::AutomationDataBrowser: parent->setNewContent(GET_PANEL_NAME(GenericPanel<AutomationDataBrowser>)); return true;
	case PopupMenuOptions::FileBrowser:			parent->setNewContent(GET_PANEL_NAME(GenericPanel<FileBrowser>)); return true;
	case PopupMenuOptions::SamplePoolTable:		parent->setNewContent(GET_PANEL_NAME(GenericPanel<SamplePoolTable>)); return true;
	}

	return false;
}

}