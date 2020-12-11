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
**/

namespace hise { using namespace juce;

Identifier FloatingTileContent::getDefaultablePropertyId(int index) const
{
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::Type, "Type");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::Title, "Title");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::StyleData, "StyleData");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::ColourData, "ColourData");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::LayoutData, "LayoutData");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::Font, "Font");
	RETURN_DEFAULT_PROPERTY_ID(index, PanelPropertyId::FontSize, "FontSize");

	jassertfalse;

	return Identifier();
}

var FloatingTileContent::getDefaultProperty(int id) const
{
	auto prop = (PanelPropertyId)id;

	switch (prop)
	{
	case PanelPropertyId::Type: return var(); // This property is not defaultable
		break;
	case PanelPropertyId::Title: return "";
	case PanelPropertyId::StyleData:
	{ 
		DynamicObject::Ptr newStyleData = new DynamicObject(); 
		return var(newStyleData);	
	}
	case PanelPropertyId::ColourData:
	{
		return colourData.toDynamicObject();
	}
	case PanelPropertyId::LayoutData: return var(); // this property is restored explicitely
	case PanelPropertyId::FontSize: return 14.0f;
	case PanelPropertyId::Font:	return "Oxygen Bold";
	case PanelPropertyId::numPropertyIds:
	default:
		break;
	}

	return var();
}

MainController* FloatingTileContent::getMainController()
{
	return getParentShell()->getMainController();
}

const MainController* FloatingTileContent::getMainController() const
{
	return getParentShell()->getMainController();
}

BackendRootWindow* FloatingTileContent::getRootWindow()
{
	return getParentShell()->getBackendRootWindow();
}

const BackendRootWindow* FloatingTileContent::getRootWindow() const
{
	return getParentShell()->getBackendRootWindow();
}

var FloatingTileContent::toDynamicObject() const
{
	DynamicObject::Ptr o = new DynamicObject();

	var obj(o);

	storePropertyInObject(obj, (int)PanelPropertyId::Type, getIdentifierForBaseClass().toString());
	storePropertyInObject(obj, (int)PanelPropertyId::Title, getCustomTitle(), "");
	storePropertyInObject(obj, (int)PanelPropertyId::StyleData, var(styleData));
	storePropertyInObject(obj, (int)PanelPropertyId::Font, fontName);
	storePropertyInObject(obj, (int)PanelPropertyId::FontSize, fontSize);

	if (getParentShell() != nullptr)
		storePropertyInObject(obj, (int)PanelPropertyId::LayoutData, var(getParentShell()->getLayoutData().getLayoutDataObject()));
	else
		jassertfalse;

	storePropertyInObject(obj, (int)PanelPropertyId::ColourData, colourData.toDynamicObject());

	if (getParentShell() != nullptr && getFixedSizeForOrientation() != 0)
		o->removeProperty("Size");

	return obj;
}


void FloatingTileContent::fromDynamicObject(const var& object)
{
	setCustomTitle(getPropertyWithDefault(object, (int)PanelPropertyId::Title));
	styleData = getPropertyWithDefault(object, (int)PanelPropertyId::StyleData);
	fontName = getPropertyWithDefault(object, (int)PanelPropertyId::Font);
	fontSize = getPropertyWithDefault(object, (int)PanelPropertyId::FontSize);
	colourData.fromDynamicObject(getPropertyWithDefault(object, (int)PanelPropertyId::ColourData));
	getParentShell()->setLayoutDataObject(getPropertyWithDefault(object, (int)PanelPropertyId::LayoutData));
}

FloatingTileContent* FloatingTileContent::createPanel(const var& data, FloatingTile* parent)
{
	if (auto obj = data.getDynamicObject())
	{
		auto panelIdString = obj->getProperty("Type").toString();

		auto panelId = panelIdString.isNotEmpty() ? Identifier(panelIdString) : EmptyComponent::getPanelId();

		auto p = parent->getPanelFactory()->createFromId(panelId, parent);

		jassert(p != nullptr);

		if (p != nullptr)
		{
			//p->fromDynamicObject(data);
		}
			
		return p;
	}
	else
	{
		jassertfalse;

		return new EmptyComponent(parent);
	}
}


FloatingTileContent* FloatingTileContent::createNewPanel(const Identifier& id, FloatingTile* parent)
{
	return parent->getPanelFactory()->createFromId(id, parent);
}

void FloatingTileContent::setDynamicTitle(const String& newDynamicTitle)
{
	dynamicTitle = newDynamicTitle;
	getParentShell()->repaint();
}

const BackendProcessorEditor* FloatingTileContent::getMainPanel() const
{
#if USE_BACKEND && DONT_INCLUDE_FLOATING_LAYOUT_IN_FRONTEND
	return GET_BACKEND_ROOT_WINDOW(getParentShell())->getMainPanel();
#else
	return nullptr;
#endif
}

BackendProcessorEditor* FloatingTileContent::getMainPanel()
{
#if USE_BACKEND && DONT_INCLUDE_FLOATING_LAYOUT_IN_FRONTEND
	return GET_BACKEND_ROOT_WINDOW(getParentShell())->getMainPanel();
#else
	return nullptr;
#endif
}

int FloatingTileContent::getFixedSizeForOrientation() const
{
	auto pType = getParentShell()->getParentType();

	if (pType == FloatingTile::ParentType::Horizontal)
		return getFixedWidth();
	else if (pType == FloatingTile::ParentType::Vertical)
		return getFixedHeight();
	else
		return 0;
}

int FloatingTileContent::getPreferredHeight() const
{

	jassert(getParentShell()->getParentType() == FloatingTile::ParentType::Root);
	return getFixedHeight();
}

struct FloatingPanelTemplates::Helpers
{
	static void addNewShellTo(FloatingTileContainer* parent)
	{
		parent->addFloatingTile(new FloatingTile(parent->getParentShell()->getMainController(), parent));
	}

	static FloatingTileContainer* getChildContainer(FloatingTile* parent)
	{
		return dynamic_cast<FloatingTileContainer*>(parent->getCurrentFloatingPanel());
	}

	template <class PanelType> static void setContent(FloatingTile* parent)
	{
		parent->setNewContent(PanelType::getPanelId());
	};

	static FloatingTile* getChildShell(FloatingTile* parent, int index)
	{
		auto c = dynamic_cast<FloatingTileContainer*>(parent->getCurrentFloatingPanel());

		jassert(c != nullptr);

		return c->getComponent(index);
	}

	static void setShellSize(FloatingTileContainer* parent, int index, double size, bool isRelative)
	{
		if (isRelative)
			size *= -1.0;

		parent->getComponent(index)->getLayoutData().setCurrentSize(size);
	}
};



void FloatingPanelTemplates::create2x2Matrix(FloatingTile* parent)
{
	Helpers::setContent<HorizontalTile>(parent);
	Helpers::addNewShellTo(Helpers::getChildContainer(parent));

	Helpers::setContent<VerticalTile>(Helpers::getChildShell(parent, 0));
	Helpers::setContent<VerticalTile>(Helpers::getChildShell(parent, 1));

	auto v1 = Helpers::getChildContainer(Helpers::getChildShell(parent, 0));
	auto v2 = Helpers::getChildContainer(Helpers::getChildShell(parent, 1));

	Helpers::addNewShellTo(v1);
	Helpers::addNewShellTo(v2);
}

void FloatingPanelTemplates::create3Columns(FloatingTile* parent)
{
	Helpers::setContent<VerticalTile>(parent);

	auto c = Helpers::getChildContainer(parent);

	Helpers::addNewShellTo(c);
	Helpers::addNewShellTo(c);
}

void FloatingPanelTemplates::create3Rows(FloatingTile* parent)
{
	Helpers::setContent<HorizontalTile>(parent);

	auto c = Helpers::getChildContainer(parent);

	Helpers::addNewShellTo(c);
	Helpers::addNewShellTo(c);
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

	const int personaContainer = ib.addChild<VerticalTile>(root);
	ib.getContainer(personaContainer)->setIsDynamic(true);



	ib.setFoldable(root, false, { true, false });
	ib.setId(personaContainer, "PersonaContainer");

	ib.getContent(root)->setPanelColour(FloatingTileContent::PanelColourId::bgColour,
		HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

	auto mainPanel = createMainPanel(ib.getPanel(personaContainer));

	ib.getContainer(personaContainer)->setIsDynamic(false);




	File scriptJSON = ProjectHandler::getAppDataDirectory().getChildFile("Workspaces/ScriptingWorkspace.json");
	File sampleJSON = ProjectHandler::getAppDataDirectory().getChildFile("Workspaces/SamplerWorkspace.json");

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

	const int toggleBar = ib.addChild<VisibilityToggleBar>(samplePanel);

	auto browserBar = ib.addChild<HorizontalTile>(samplePanel);
	
	auto editBar = ib.addChild<ExpansionEditBar>(browserBar);
	auto fileBrowser = ib.addChild<GenericPanel<FileBrowser>>(browserBar);

	ib.getPanel(editBar)->setCanBeFolded(false);

	ib.setDynamic(browserBar, false);

	auto samplePoolTable = ib.addChild<GenericPanel<SamplePoolTable>>(samplePanel);
    
    ib.getPanel(samplePoolTable)->getLayoutData().setVisible(false);
    
	const int sampleHorizontal = ib.addChild<HorizontalTile>(samplePanel);
	ib.setDynamic(sampleHorizontal, false);
	ib.addChild<GlobalConnectorPanel<ModulatorSampler>>(sampleHorizontal);
	const int sampleEditor = ib.addChild<SampleEditorPanel>(sampleHorizontal);
	const int sampleVertical = ib.addChild<VerticalTile>(sampleHorizontal);
	ib.setDynamic(sampleVertical, false);
    
    
    
	const int sampleMapEditor = ib.addChild<SampleMapEditorPanel>(sampleVertical);
	const int samplerTable = ib.addChild<SamplerTablePanel>(sampleVertical);

    ib.setSizes(sampleVertical, {-0.7, -0.3});
    
	ib.setSizes(samplePanel, { 32.0, 280.0, 280.0, -0.5 });
	ib.getPanel(sampleHorizontal)->setCustomIcon((int)FloatingTileContent::Factory::PopupMenuOptions::SampleEditor);

	ib.setId(fileBrowser, "SampleFileBrowser");
	ib.setId(samplePoolTable, "SamplePoolTable");
	ib.setId(sampleEditor, "MainSampleEditor");
	ib.setId(sampleMapEditor, "MainSampleMapEditor");
	ib.setId(samplerTable, "MainSamplerTable");
	
	ib.setCustomPanels(toggleBar, { browserBar, samplePoolTable, sampleEditor, sampleMapEditor, samplerTable });
	ib.getPanel(toggleBar)->setCanBeFolded(false);

	ib.getContent<FloatingTileContent>(sampleEditor)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(sampleMapEditor)->setStyleProperty("showConnectionBar", false);
	ib.getContent<FloatingTileContent>(samplerTable)->setStyleProperty("showConnectionBar", false);
#endif

	ignoreUnused(rootTile);

	return nullptr;
}

#define SET_FALSE(x) sData->setProperty(sw->getDefaultablePropertyId((int)x), false);

var FloatingPanelTemplates::createSettingsWindow(MainController* mc)
{
	MessageManagerLock mm;
	
	ScopedPointer<FloatingTile> root = new FloatingTile(mc, nullptr);

	root->setAllowChildComponentCreation(false);

	FloatingInterfaceBuilder ib(root);

	ib.setNewContentType<FloatingTabComponent>(0);

	int tabs = 0;

	ib.setDynamic(tabs, false);
	ib.getContent<FloatingTabComponent>(tabs)->setPanelColour(FloatingTabComponent::PanelColourId::bgColour, Colour(0xff000000));
	ib.getContent<FloatingTabComponent>(tabs)->setPanelColour(FloatingTabComponent::PanelColourId::itemColour1, Colour(0xff333333));


	
	const int settingsWindows = ib.addChild<CustomSettingsWindowPanel>(tabs);

#if IS_STANDALONE_APP
	ib.addChild<MidiSourcePanel>(tabs);

	ignoreUnused(settingsWindows);

#else
    
	auto sw = ib.getContent<CustomSettingsWindowPanel>(settingsWindows);

	DynamicObject::Ptr sData = new DynamicObject();

	SET_FALSE(CustomSettingsWindowPanel::SpecialPanelIds::BufferSize);
	SET_FALSE(CustomSettingsWindowPanel::SpecialPanelIds::SampleRate);
	SET_FALSE(CustomSettingsWindowPanel::SpecialPanelIds::Output);
	SET_FALSE(CustomSettingsWindowPanel::SpecialPanelIds::Driver);
	SET_FALSE(CustomSettingsWindowPanel::SpecialPanelIds::Device);

	var sd(sData);

	ib.getContent<CustomSettingsWindowPanel>(settingsWindows)->fromDynamicObject(sd);
#endif

	ib.addChild<MidiChannelPanel>(tabs);

	ib.getContent<FloatingTabComponent>(tabs)->setCurrentTabIndex(0);
	
#if IS_STANDALONE_APP
	ib.setCustomName(tabs, "Settings", { "Audio Settings", "Midi Sources", "MIDI Channels" });
#else
	ib.setCustomName(tabs, "Settings", { "Plugin Settings", "MIDI Channels" });
#endif


	auto v = ib.getContent(0)->toDynamicObject();

	return v;
}

#undef SET_FALSE

Component* FloatingPanelTemplates::createScriptingWorkspace(FloatingTile* rootTile)
{
#if USE_BACKEND
	
	FloatingInterfaceBuilder ib(rootTile);

	const int personaContainer = 0;

	const int scriptPanel = ib.addChild <HorizontalTile>(personaContainer);
	ib.setDynamic(scriptPanel, false);
	ib.getContent(scriptPanel)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colour(0xFF404040));

	ib.getPanel(scriptPanel)->getLayoutData().setId("ScriptingWorkspace");

    ib.addChild<GlobalConnectorPanel<JavascriptProcessor>>(scriptPanel);

	const int mainVertical = ib.addChild<VerticalTile>(scriptPanel);
	ib.setDynamic(mainVertical, false);

	const int toggleBar = ib.addChild < VisibilityToggleBar>(mainVertical);
	ib.getContent(toggleBar)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF404040));

	const int apiCollection = ib.addChild<GenericPanel<ApiCollection>>(mainVertical);
	const int fileBrowser = ib.addChild<GenericPanel<FileBrowser>>(mainVertical);
	const int codeEditor = ib.addChild<HorizontalTile>(mainVertical);
	ib.setDynamic(codeEditor, false);


	const int codeVertical = ib.addChild<VerticalTile>(codeEditor);
	ib.setDynamic(codeVertical, false);
	const int codeTabs = ib.addChild<FloatingTabComponent>(codeVertical);
    ib.addChild<CodeEditorPanel>(codeTabs);
	const int variableWatch = ib.addChild<ScriptWatchTablePanel>(codeVertical);

    ib.addChild<ConsolePanel>(codeEditor);
	ib.setCustomName(codeEditor, "Code Editor");
	ib.setSizes(codeEditor, { -0.75, -0.25 });
	ib.setSizes(codeVertical, { -0.8, -0.2 });

	const int scriptNode = ib.addChild<VerticalTile>(mainVertical);

	{
		
		ib.setDynamic(scriptNode, false);
		ib.getContent(scriptNode)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		const int nodeList = ib.addChild<scriptnode::DspNodeList::Panel>(scriptNode);
		const int scriptNodeHorizontal = ib.addChild<HorizontalTile>(scriptNode);
		ib.getContent(scriptNodeHorizontal)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		const int interfacePanel = ib.addChild<scriptnode::DspNetworkGraphPanel>(scriptNodeHorizontal);
		const int onInitPanel = ib.addChild<CodeEditorPanel>(scriptNodeHorizontal);
		const int nodeKeyboard = ib.addChild<MidiKeyboardPanel>(scriptNodeHorizontal);
		ib.getPanel(nodeKeyboard)->getLayoutData().setForceFoldButton(true);

		ib.setSizes(scriptNodeHorizontal, { -0.7, -0.3, 72.0 });
		ib.setCustomName(scriptNodeHorizontal, "", { "DSP Network", "Script Editor", "" });
		ib.setCustomName(scriptNode, "ScriptNode");

		ib.setCustomName(nodeList, "Node List");
		ib.setDynamic(scriptNodeHorizontal, false);

		const int nodePropertyEditor = ib.addChild<scriptnode::NodePropertyPanel>(scriptNode);

		ib.setCustomName(nodePropertyEditor, "Node Properties");

		ib.getContent<FloatingTileContent>(interfacePanel)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(nodeList)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(nodePropertyEditor)->setStyleProperty("showConnectionBar", false);

		ib.setFoldable(scriptNodeHorizontal, false, { false, true, true });

		ib.setSizes(scriptNode, { -0.15, -0.7, -0.15 });

		ib.getPanel(onInitPanel)->setFolded(true);
		ib.getPanel(nodeKeyboard)->setFolded(true);
	}

	{
		// Interface Designer
		const int interfaceDesigner = ib.addChild <VerticalTile>(mainVertical);
		ib.setDynamic(interfaceDesigner, false);

		ib.getContent(interfaceDesigner)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);
		ib.getContent(codeVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		ib.getContent(mainVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);
		ib.getContent(codeEditor)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		const int componentList = ib.addChild<ScriptComponentList::Panel>(interfaceDesigner);

		const int interfaceHorizontal = ib.addChild<HorizontalTile>(interfaceDesigner);

		ib.getContent(interfaceHorizontal)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);
		ib.getContent(mainVertical)->setPanelColour(FloatingTileContent::PanelColourId::itemColour1, Colours::transparentBlack);

		ib.setDynamic(interfaceHorizontal, false);
		const int interfacePanel = ib.addChild<ScriptContentPanel>(interfaceHorizontal);
		const int onInitPanel = ib.addChild<CodeEditorPanel>(interfaceHorizontal);
		const int keyboard = ib.addChild<MidiKeyboardPanel>(interfaceHorizontal);
		ib.getPanel(keyboard)->getLayoutData().setForceFoldButton(true);

		const int propertyEditor = ib.addChild<ScriptComponentEditPanel::Panel>(interfaceDesigner);

		ib.setSizes(interfaceHorizontal, { -0.7, -0.3, 72.0 });
		ib.setCustomName(interfaceHorizontal, "", { "Canvas", "Script Editor", "" });

		ib.setCustomName(interfaceDesigner, "Interface Designer");
		ib.setCustomName(propertyEditor, "Property Editor");
		ib.setCustomName(componentList, "Component List");

		ib.setId(toggleBar, "ScriptingWorkspaceToggleBar");

		ib.setId(codeEditor, "ScriptingWorkspaceCodeEditor");
		ib.setId(interfaceDesigner, "ScriptingWorkspaceInterfaceDesigner");

		ib.setSizes(interfaceDesigner, { -0.15, -0.7, -0.15 });

		ib.setFoldable(mainVertical, false, { true, true, true, true, true, true });
		ib.setFoldable(interfaceHorizontal, false, { false, true, true });

		ib.setSizes(mainVertical, { 32.0, 300.0, 300.0, -0.33, -0.33, -0.33 });

		ib.getPanel(interfaceDesigner)->getLayoutData().setVisible(false);
		ib.getPanel(apiCollection)->getLayoutData().setVisible(false);
		ib.getPanel(fileBrowser)->getLayoutData().setVisible(false);
		ib.getPanel(scriptNode)->getLayoutData().setVisible(false);

		ib.getContent<VisibilityToggleBar>(toggleBar)->refreshButtons();

		//ib.getContent<FloatingTileContent>(onInitPanel)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(interfacePanel)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(variableWatch)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(variableWatch)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(componentList)->setStyleProperty("showConnectionBar", false);
		ib.getContent<FloatingTileContent>(propertyEditor)->setStyleProperty("showConnectionBar", false);

		ib.getPanel(onInitPanel)->setFolded(true);
		ib.getPanel(keyboard)->setFolded(true);


	}
	
	return ib.getPanel(scriptPanel);
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

	ib.getContent(firstVertical)->setPanelColour(FloatingTileContent::PanelColourId::bgColour,
		HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));

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



void JSONEditor::addButtonAndLabel()
{
	addAndMakeVisible(changeLabel = new Label());
	changeLabel->setColour(Label::ColourIds::backgroundColourId, Colour(0xff363636));
	changeLabel->setFont(GLOBAL_BOLD_FONT());
	changeLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	changeLabel->setEditable(false, false, false);

	addAndMakeVisible(applyButton = new TextButton("Apply"));
	applyButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	applyButton->addListener(this);
	applyButton->setColour(TextButton::buttonColourId, Colour(0xa2616161));
	
}

void JSONEditor::setChanged()
{
	auto now = Time::getApproximateMillisecondCounter();

	if ((now - constructionTime) < 1000)
		return;

	changeLabel->setColour(Label::backgroundColourId, Colour(0x22FF0000));
	changeLabel->setText("Press F5 or Apply to apply the changes", dontSendNotification);
}

void JSONEditor::replace()
{
	if (editedComponent.getComponent() != nullptr)
	{
		var newData;

		auto result = JSON::parse(doc->getAllContent(), newData);

		if (result.wasOk())
		{
			dynamic_cast<ObjectWithDefaultProperties*>(editedComponent.getComponent())->fromDynamicObject(newData);

			auto parent = dynamic_cast<FloatingTileContent*>(editedComponent.getComponent())->getParentShell();

			jassert(parent != nullptr);

			parent->refreshRootLayout();
			parent->refreshPinButton();
			parent->refreshFoldButton();
			parent->refreshMouseClickTarget();

			editedComponent->repaint();
		}
		else
		{
			PresetHandler::showMessageWindow("JSON Parser Error", result.getErrorMessage(), PresetHandler::IconType::Error);
		}
	}
}


void JSONEditor::executeCallback()
{
	var newData;

	auto result = JSON::parse(doc->getAllContent(), newData);

	if (result.wasOk())
	{
		callback(newData);
		
		Component::SafePointer<JSONEditor> safeThis(this);

		auto f = [safeThis]()
		{
			if (safeThis.getComponent() != nullptr)
			{
				if(auto ft = safeThis.getComponent()->findParentComponentOfClass<FloatingTilePopup>())
					ft->deleteAndClose();
			}
				
		};

		if(closeAfterCallbackExecution)
			new DelayedFunctionCaller(f, 200);
	}
	else
	{
		PresetHandler::showMessageWindow("JSON Parser Error", result.getErrorMessage(), PresetHandler::IconType::Error);
	}
}



class ExternalPlaceholder : public FloatingTileContent,
	public Component
{
public:

	ExternalPlaceholder(FloatingTile* parent) :
		FloatingTileContent(parent)
	{};

	Identifier getIdentifierForBaseClass() const override { return id; }

	void setName(const Identifier& newId)
	{
		id = newId;
	}

	void paint(Graphics& g)
	{
		g.fillAll(Colours::grey);
		g.setColour(Colours::black);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(id.toString(), getLocalBounds().toFloat(), Justification::centred);
	}

	Identifier id;
};

hise::FloatingTileContent* FloatingTileContent::Factory::createFromId(const Identifier &id, FloatingTile* parent) const
{
#if USE_BACKEND
	if (id.toString().startsWith("External"))
	{
		auto ft = new ExternalPlaceholder(parent);
		ft->setName(id);

		return ft;
	}
#endif

	const int index = ids.indexOf(id);

	if (index != -1) return functions[index](parent);
	else
	{
#if USE_BACKEND
		auto ft = new ExternalPlaceholder(parent);
		ft->setName(id);

		return ft;
#else
		jassertfalse;
		return functions[0](parent);
#endif

		
	}
}

juce::Path FloatingTileContent::FloatingTilePathFactory::createPath(const String& id) const
{
	auto url = MarkdownLink::Helpers::getSanitizedFilename(id);

	auto index = ids.indexOf(url);

	if (index != -1)
		return Factory::getPath(f.getIndex(index));

	return {};
}

} // namespace hise
