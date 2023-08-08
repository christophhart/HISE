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

	FloatingTileContent::FloatingTileContent(FloatingTile* parent_):
		parent(parent_)
	{
		styleData = getDefaultProperty((int)PanelPropertyId::StyleData);

		setDefaultPanelColour(PanelColourId::itemColour3, Colours::white.withAlpha(0.6f));

	}

	FloatingTileContent::~FloatingTileContent()
	{
		parent = nullptr;
	}

	FloatingTile* FloatingTileContent::getParentShell()
	{
		jassert(parent != nullptr);

		return parent; 
	}

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
		return var(new DynamicObject());
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

const FloatingTile* FloatingTileContent::getParentShell() const
{
	jassert(parent != nullptr);

	return parent; 
}

Rectangle<int> FloatingTileContent::getParentContentBounds() {
    return getParentShell()->getContentBounds();
}

var FloatingTileContent::toDynamicObject() const
{
	auto o = new DynamicObject();
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

void FloatingTileContent::setCustomTitle(String newCustomTitle)
{
	customTitle = newCustomTitle;

	
}

void FloatingTileContent::setDynamicTitle(const String& newDynamicTitle)
{
	dynamicTitle = newDynamicTitle;
	getParentShell()->repaint();

	if (auto asComponent = dynamic_cast<Component*>(this))
	{
		asComponent->repaint();

		if (auto c = dynamic_cast<Component*>(getParentShell()->findParentTileWithType<FloatingTileContainer>()))
        {
			c->resized();
            c->repaint();
        }
	}
}

String FloatingTileContent::getBestTitle() const
{
    if (hasDynamicTitle())
        return getDynamicTitle();

    if (hasCustomTitle())
        return getCustomTitle();

    auto t = getTitle();
    
    if(t.isEmpty())
    {
        if(auto c = dynamic_cast<const FloatingTileContainer*>(this))
        {
            if(auto first = c->getComponent(0))
                return first->getCurrentFloatingPanel()->getBestTitle();
        }
    }
    
    return t;
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

FloatingTileContent::ColourHolder::ColourHolder()
{
	for (int i = 0; i < (int)ColourId::numColourIds; i++)
	{
		colours[i] = Colours::transparentBlack;
		defaultColours[i] = Colours::transparentBlack;
	}	
}

int FloatingTileContent::ColourHolder::getNumDefaultableProperties() const
{ return (int)ColourId::numColourIds; }

Identifier FloatingTileContent::ColourHolder::getDefaultablePropertyId(int i) const
{
	switch ((ColourId)i)
	{
	case ColourId::bgColour: return "bgColour";
	case ColourId::textColour: return "textColour";
	case ColourId::itemColour1: return "itemColour1";
	case ColourId::itemColour2: return "itemColour2";
	case ColourId::itemColour3: return "itemColour3";
	default: jassertfalse;  return Identifier();
	}
}

var FloatingTileContent::ColourHolder::getDefaultProperty(int id) const
{
	auto c = defaultColours[id];

	//jassert(c != Colours::pink);

	return var(c.toString());
}

Colour FloatingTileContent::ColourHolder::getColour(ColourId id) const
{
	if (id < ColourId::numColourIds)
	{
		return colours[(int)id];
	}
	else
	{
		jassertfalse;
		return Colours::transparentBlack;
	}
}

void FloatingTileContent::ColourHolder::setColour(ColourId id, Colour newColour)
{
	if (id < ColourId::numColourIds)
	{
		colours[(int)id] = newColour;
	}
}

void FloatingTileContent::ColourHolder::setDefaultColour(ColourId id, Colour newColour)
{
	if (id < ColourId::numColourIds)
	{
		defaultColours[(int)id] = newColour;
	}
}

Colour FloatingTileContent::ColourHolder::getDefaultColour(ColourId id) const
{
	if (id < ColourId::numColourIds)
		return defaultColours[(int)id];

	return Colours::transparentBlack;
}

var FloatingTileContent::ColourHolder::toDynamicObject() const
{
	auto o = new DynamicObject();

	var obj(o);

	for (int i = 0; i < (int)ColourId::numColourIds; i++)
	{
		storePropertyInObject(obj, i, colours[i].toString(), defaultColours[i].toString());
	}

	return obj;
}

void FloatingTileContent::ColourHolder::fromDynamicObject(const var& object)
{
	for (int i = 0; i < (int)ColourId::numColourIds; i++)
	{
		auto colourVar = getPropertyWithDefault(object, i);

		if (colourVar.isString())
		{
			auto stringValue = colourVar.toString();

			// thanks to the awesomeness of XML, this could still be a number, yay...

			int64 normal = stringValue.getLargeIntValue();
			int64 asHex = stringValue.getHexValue64();

			if (stringValue.containsAnyOf("ABCDEFabcdefx"))
			{
				colours[i] = Colour((uint32)asHex);
			}
			else
			{
				colours[i] = Colour((uint32)normal);
			}
		}
		else if (colourVar.isInt64() || colourVar.isInt64())
		{
			colours[i] = Colour((uint32)(int64)colourVar);
		}
	}
				
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


void FloatingTileContent::setPanelColour(PanelColourId id, Colour newColour)
{
	colourData.setColour(id, newColour);
}

void FloatingTileContent::setDefaultPanelColour(PanelColourId id, Colour newColour)
{
	colourData.setDefaultColour(id, newColour);
	colourData.setColour(id, newColour);
}

Colour FloatingTileContent::getDefaultPanelColour(PanelColourId id) const
{
	return colourData.getDefaultColour(id);
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

	var sd(sData.get());

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

Component* FloatingPanelTemplates::createScriptnodeEditorPanel(FloatingTile* root)
{
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


var ObjectWithDefaultProperties::toDynamicObject() const
{
	jassert(useCustomDefaultValues);

	var obj(new DynamicObject());
	saveValuesFromList(obj);
	return obj;
}

void ObjectWithDefaultProperties::fromDynamicObject(const var& objectData)
{
	jassert(useCustomDefaultValues);
	loadValuesToList(objectData);
}

var ObjectWithDefaultProperties::getDefaultProperty(int id) const
{
	jassert(useCustomDefaultValues);

	return defaultValues.getValueAt(id);
}

var ObjectWithDefaultProperties::operator()(const var& data, int index) const
{
	return getPropertyWithDefault(data, index);
}

void ObjectWithDefaultProperties::operator()(const var& data, int index, const var& value) const
{
	storePropertyInObject(data, index, value);
}

int ObjectWithDefaultProperties::getNumDefaultableProperties() const
{
	jassert(useCustomDefaultValues);

	return defaultValues.size();
}

Identifier ObjectWithDefaultProperties::getDefaultablePropertyId(int i) const
{
	jassert(useCustomDefaultValues);

	return defaultValues.getName(i);
}

void ObjectWithDefaultProperties::resetObject(DynamicObject* objectToClear)
{
	jassert(objectToClear != nullptr);

	objectToClear->clear();

	for (int i = 0; i < getNumDefaultableProperties(); i++)
	{
		objectToClear->setProperty(getDefaultablePropertyId(i), getDefaultProperty(i));
	}
}

void ObjectWithDefaultProperties::storePropertyInObject(var obj, int id, var value, var defaultValue) const
{
	jassert(obj.isObject());

	if ((defaultValue.isUndefined() || defaultValue.isVoid()) || value != defaultValue)
	{
		obj.getDynamicObject()->setProperty(getDefaultablePropertyId(id), value);
	}
}

var ObjectWithDefaultProperties::getPropertyWithDefault(var obj, int id) const
{
	if (auto object = obj.getDynamicObject())
	{
		auto prop = getDefaultablePropertyId(id);

		if (object->hasProperty(prop))
			return object->getProperty(prop);
		else
			return getDefaultProperty(id);
	}

	return getDefaultProperty(id);
}

void ObjectWithDefaultProperties::setDefaultValues(const NamedValueSet& newDefaultValues)
{
	defaultValues = newDefaultValues;
	useCustomDefaultValues = true;
}

void ObjectWithDefaultProperties::setValueList(const Array<Value>& valueList)
{
	jassert(useCustomDefaultValues);
	jassert(valueList.size() == defaultValues.size());

	values = valueList;
}

Colour ObjectWithDefaultProperties::getColourFrom(const Value& colourValue)
{
	return Colour((uint32)(int64)(colourValue.getValue()));
}

void ObjectWithDefaultProperties::saveValuesFromList(var& object) const
{
	jassert(useCustomDefaultValues);

	for (int i = 0; i < getNumDefaultableProperties(); i++)
	{
		storePropertyInObject(object, i, values[i].getValue(), getDefaultProperty(i));
	}
}

JSONEditor::JSONEditor(ObjectWithDefaultProperties* editedObject):
	editedComponent(dynamic_cast<Component*>(editedObject))
{
	constructionTime = Time::getApproximateMillisecondCounter();

	setName("JSON Editor");

	tokeniser = new JavascriptTokeniser();
	doc = new CodeDocument();
	doc->replaceAllContent(JSON::toString(editedObject->toDynamicObject(), false, DOUBLE_TO_STRING_DIGITS));
	doc->setSavePoint();
	doc->clearUndoHistory();
	doc->addListener(this);

	addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

	editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
	editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
	editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
	editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
	editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
	editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
	editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));

	editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

	addButtonAndLabel();

	constrainer.setMinimumWidth(200);
	constrainer.setMinimumHeight(300);

	addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));
}

JSONEditor::JSONEditor(var object)
{
	constructionTime = Time::getApproximateMillisecondCounter();

	auto s = JSON::toString(object, false, DOUBLE_TO_STRING_DIGITS);

	tokeniser = new JavascriptTokeniser();
	doc = new CodeDocument();
	doc->replaceAllContent(s);
	doc->setSavePoint();
	doc->clearUndoHistory();
	doc->addListener(this);

	addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

	editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
	editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
	editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
	editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
	editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
	editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
	editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
	editor->setReadOnly(true);
	editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

	addButtonAndLabel();

	constrainer.setMinimumWidth(200);
	constrainer.setMinimumHeight(300);

	addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));

}

JSONEditor::JSONEditor(const String& f, CodeTokeniser* t)
{
	constructionTime = Time::getApproximateMillisecondCounter();

	setName("External Script Preview");

	tokeniser = t;
	doc = new CodeDocument();
	doc->replaceAllContent(f);
	doc->setSavePoint();
	doc->clearUndoHistory();
	doc->addListener(this);

	addAndMakeVisible(editor = new CodeEditorComponent(*doc, tokeniser));

	editor->setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
	editor->setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
	editor->setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
	editor->setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
	editor->setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
	editor->setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
	editor->setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
	editor->setReadOnly(true);
	editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(17.0f));

	addButtonAndLabel();


	constrainer.setMinimumWidth(200);
	constrainer.setMinimumHeight(300);

	addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));
}

JSONEditor::~JSONEditor()
{
	editor = nullptr;
	doc = nullptr;
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

void JSONEditor::buttonClicked(Button*)
{
	executeCallback();
}

void JSONEditor::codeDocumentTextInserted(const String&, int)
{
	setChanged();
}

void JSONEditor::codeDocumentTextDeleted(int, int)
{
	setChanged();
}

void JSONEditor::setChanged()
{
	auto now = Time::getApproximateMillisecondCounter();

	if ((now - constructionTime) < 1000)
		return;

	changeLabel->setColour(Label::backgroundColourId, Colour(0x22FF0000));
	changeLabel->setText("Press F5 or Apply to apply the changes", dontSendNotification);
}

bool JSONEditor::keyPressed(const KeyPress& key)
{
		

	if (key == KeyPress::F5Key)
	{
		if (callback)
		{
			executeCallback();
		}
		else
		{
			if (editedComponent == nullptr)
				return false;

			replace();
		}

		return true;
	}

	return false;
}

void JSONEditor::setEditable(bool shouldBeEditable)
{
	editor->setReadOnly(!shouldBeEditable);
}

void JSONEditor::setDataToEdit(var newData)
{
	doc->clearUndoHistory();
	doc->replaceAllContent(JSON::toString(newData, false, DOUBLE_TO_STRING_DIGITS));
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

	auto result = compileCallback(doc->getAllContent(), newData);

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

void JSONEditor::setCallback(const F5Callback& newCallback, bool closeAfterExecution)
{
	callback = newCallback;
	closeAfterCallbackExecution = closeAfterExecution;
}

void JSONEditor::resized()
{
	auto b = getLocalBounds();

	auto bottomRow = b.removeFromBottom(24);

	applyButton->setBounds(bottomRow.removeFromRight(50));
	changeLabel->setBounds(bottomRow);

	editor->setBounds(b);
	resizer->setBounds(getWidth() - 12, getHeight() - 12, 12, 12);
}

Result JSONEditor::defaultJSONParse(const String& s, var& value)
{
	return JSON::parse(s, value);
}

void JSONEditor::setCompileCallback(const CompileCallback& c, bool closeAfterExecution)
{
	compileCallback = c;
	closeAfterCallbackExecution = closeAfterExecution;
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
