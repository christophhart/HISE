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

namespace hise { using namespace juce;

BackendRootWindow::BackendRootWindow(AudioProcessor *ownerProcessor, var editorState) :
	AudioProcessorEditor(ownerProcessor),
	BackendCommandTarget(static_cast<BackendProcessor*>(ownerProcessor)),
	owner(static_cast<BackendProcessor*>(ownerProcessor))
{
	PresetHandler::buildProcessorDataBase(owner->getMainSynthChain());

	addAndMakeVisible(floatingRoot = new FloatingTile(owner, nullptr));

	bool loadedCorrectly = true;
	bool objectFound = editorState.isObject();


	int width = 1500;
	int height = 900;
	

	if (objectFound)
	{
		int dataVersion = editorState.getDynamicObject()->getProperty("UIVersion");

		var mainData = editorState.getProperty("MainEditorData", var());

		if (!mainData.isObject())
			loadedCorrectly = false;

		if (dataVersion != BACKEND_UI_VERSION && PresetHandler::showYesNoWindow("UI Version Mismatch", "The stored layout is deprecated. Press OK to reset the layout data or cancel to use it anyway", PresetHandler::IconType::Question))
			loadedCorrectly = false;

		if (loadedCorrectly)
		{
			floatingRoot->setContent(mainData);

			mainEditor = FloatingTileHelpers::findTileWithId<BackendProcessorEditor>(floatingRoot, Identifier("MainColumn"));

			loadedCorrectly = mainEditor != nullptr;
		}

		if (loadedCorrectly)
		{
			auto mws = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("MainWorkspace"));

			if (mws != nullptr)
				workspaces.add(mws->getParentShell());
			else
				loadedCorrectly = false;
		}

		if (loadedCorrectly)
		{
			auto mws = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("ScriptingWorkspace"));

			if (mws != nullptr)
				workspaces.add(mws->getParentShell());
			else
				loadedCorrectly = false;
		}

		if (loadedCorrectly)
		{
			auto mws = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("SamplerWorkspace"));

			if (mws != nullptr)
				workspaces.add(mws->getParentShell());
			else
				loadedCorrectly = false;
		}

		if (loadedCorrectly)
		{
			auto mws = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("CustomWorkspace"));

			if (mws != nullptr)
				workspaces.add(mws->getParentShell());
			else
				loadedCorrectly = false;
		}

		if (loadedCorrectly)
		{
			auto position = editorState.getProperty("Position", var());

			if (position.isArray())
			{
				width = jmax<int>(960, position[2]);
				height = jmax<int>(500, position[3]);
			}

			const int workspace = editorState.getDynamicObject()->getProperty("CurrentWorkspace");

			if(workspace > 0)
				showWorkspace(workspace);
            
            setEditor(this);
		}

		
	}

	if (!loadedCorrectly)
		PresetHandler::showMessageWindow("Error loading Interface", "The interface data is corrupt. The default settings will be loaded", PresetHandler::IconType::Error);

	if(!objectFound || !loadedCorrectly)
	{
		mainEditor = dynamic_cast<BackendProcessorEditor*>(FloatingPanelTemplates::createHiseLayout(floatingRoot));

		jassert(mainEditor != nullptr);

		workspaces.add(FloatingTileHelpers::findTileWithId<VerticalTile>(floatingRoot, Identifier("MainWorkspace"))->getParentShell());
		workspaces.add(FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("ScriptingWorkspace"))->getParentShell());
		workspaces.add(FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("SamplerWorkspace"))->getParentShell());
		workspaces.add(FloatingTileHelpers::findTileWithId<HorizontalTile>(floatingRoot, Identifier("CustomWorkspace"))->getParentShell());

		showWorkspace(BackendCommandTarget::WorkspaceMain);

		setEditor(this);
	}

	var windowList = editorState.getProperty("FloatingWindows", var());

	if (windowList.isArray())
	{
		for (int i = 0; i < windowList.size(); i++)
		{
			addFloatingWindow();

			auto pw = popoutWindows.getLast();

			var position = windowList[i].getProperty("Position", var());

			if (position.isArray())
				pw->setBounds(position[0], position[1], position[2], position[3]);

			pw->getRootFloatingTile()->setContent(windowList[i]);
		}
	}

	auto consoleParent = FloatingTileHelpers::findTileWithId<ConsolePanel>(getRootFloatingTile(), "MainConsole");

	if (consoleParent != nullptr)
		getBackendProcessor()->getConsoleHandler().setMainConsole(consoleParent->getConsole());
	else
		jassertfalse;

	setOpaque(true);

#if IS_STANDALONE_APP 

	if (owner->callback->getCurrentProcessor() == nullptr)
	{
		showSettingsWindow();
	}

#endif

	

	constrainer = new ComponentBoundsConstrainer();
	constrainer->setMinimumHeight(500);
	constrainer->setMinimumWidth(960);
	constrainer->setMaximumWidth(4000);

	

	addAndMakeVisible(yBorderDragger = new ResizableBorderComponent(this, constrainer));
	addAndMakeVisible(xBorderDragger = new ResizableBorderComponent(this, constrainer));

	BorderSize<int> yBorderSize;
	yBorderSize.setTop(0);
	yBorderSize.setLeft(0);
	yBorderSize.setRight(0);
	yBorderSize.setBottom(7);
	yBorderDragger->setBorderThickness(yBorderSize);


	BorderSize<int> xBorderSize;
	xBorderSize.setTop(0);
	xBorderSize.setLeft(0);
	xBorderSize.setRight(7);
	xBorderSize.setBottom(0);

	xBorderDragger->setBorderThickness(xBorderSize);

	addAndMakeVisible(progressOverlay = new ThreadWithQuasiModalProgressWindow::Overlay());
	owner->setOverlay(progressOverlay);
	progressOverlay->setDialog(nullptr);


#if JUCE_MAC && IS_STANDALONE_APP
	MenuBarModel::setMacMainMenu(this);
#else

	addAndMakeVisible(menuBar = new MenuBarComponent(this));
	menuBar->setLookAndFeel(&plaf);

#endif

	setSize(width, height);

	startTimer(1000);

	updateCommands();
}


BackendRootWindow::~BackendRootWindow()
{
	saveInterfaceData();

	popoutWindows.clear();

	getBackendProcessor()->getCommandManager()->clearCommands();
	getBackendProcessor()->getConsoleHandler().setMainConsole(nullptr);

	clearModalComponent();

	modalComponent = nullptr;

	// Remove the resize stuff

	constrainer = nullptr;
	yBorderDragger = nullptr;
	xBorderDragger = nullptr;
	currentDialog = nullptr;

	// Remove the menu

#if JUCE_MAC && IS_STANDALONE_APP
	MenuBarModel::setMacMainMenu(nullptr);
#else
	menuBar->setModel(nullptr);
	menuBar = nullptr;
#endif

	floatingRoot = nullptr;

	mainEditor = nullptr;


}

bool BackendRootWindow::isFullScreenMode() const
{
#if IS_STANDALONE_APP
	if (getParentComponent() == nullptr) return false;

	Component *kioskComponent = Desktop::getInstance().getKioskModeComponent();

	Component *parentparent = getParentComponent()->getParentComponent();

	return parentparent == kioskComponent;
#else
	return false;
#endif
}

void BackendRootWindow::saveInterfaceData()
{
	if (resetOnClose)
	{
		getBackendProcessor()->setEditorData({});
	}
	else
	{
		DynamicObject::Ptr obj = new DynamicObject();

		var editorData = getRootFloatingTile()->getCurrentFloatingPanel()->toDynamicObject();

		if (auto editorObject = editorData.getDynamicObject())
		{

			Array<var> position;

			position.add(getX());
			position.add(getY());
			position.add(getWidth());
			position.add(getHeight());

			obj->setProperty("Position", position);

			obj->setProperty("CurrentWorkspace", currentWorkspace);
		}

		obj->setProperty("UIVersion", BACKEND_UI_VERSION);
		obj->setProperty("MainEditorData", editorData);

		Array<var> windowList;

		for (int i = 0; i < popoutWindows.size(); i++)
		{
			var windowData = popoutWindows[i]->getRootFloatingTile()->getCurrentFloatingPanel()->toDynamicObject();

			if (auto windowObject = windowData.getDynamicObject())
			{
				Array<var> position;

				position.add(popoutWindows[i]->getX());
				position.add(popoutWindows[i]->getY());
				position.add(popoutWindows[i]->getWidth());
				position.add(popoutWindows[i]->getHeight());

				windowObject->setProperty("Position", position);
			}

			windowList.add(windowData);
		}

		obj->setProperty("FloatingWindows", windowList);

		getBackendProcessor()->setEditorData(var(obj));
	}

}

void BackendRootWindow::resized()
{

#if IS_STANDALONE_APP
	if (getParentComponent() != nullptr)
	{
		getParentComponent()->getParentComponent()->setSize(getWidth(), getHeight());
	}
#endif

	progressOverlay->setBounds(0, 0, getWidth(), getHeight());

	const int menuBarOffset = menuBar == nullptr ? 0 : 20;

	if (menuBarOffset != 0)
		menuBar->setBounds(getLocalBounds().withHeight(menuBarOffset));

	floatingRoot->setBounds(4, menuBarOffset + 4, getWidth() - 8, getHeight() - menuBarOffset - 8);

#if IS_STANDALONE_APP

	if (currentDialog != nullptr)
	{
		currentDialog->centreWithSize(700, 500);
	}

#else

	yBorderDragger->setBounds(getBounds());
	xBorderDragger->setBounds(getBounds());

#endif

}

void BackendRootWindow::showSettingsWindow()
{
	jassert(owner->deviceManager != nullptr);

	if (owner->deviceManager != nullptr && currentDialog == nullptr)
	{
		addAndMakeVisible(currentDialog = new AudioDeviceDialog(owner));

		currentDialog->centreWithSize(700, 500);
	}
	else
	{
		currentDialog = nullptr;
	}
}

void BackendRootWindow::timerCallback()
{
	stopTimer();

	if (!GET_PROJECT_HANDLER(mainEditor->getMainSynthChain()).isActive() && PresetHandler::showYesNoWindow("Welcome to HISE", "Do you want to create a new project?\nA project is a folder which contains all external files needed for a sample library."))
	{
		owner->setChanged(false);

		BackendCommandTarget::Actions::createNewProject(this);
	}
}

void BackendRootWindow::resetInterface()
{
	resetOnClose = true;

	PresetHandler::showMessageWindow("Workspace Layout Reset", "Close and open this instance to reset the interface", PresetHandler::IconType::Info);
}

void BackendRootWindow::loadNewContainer(ValueTree & v)
{
	FloatingTile::Iterator<PanelWithProcessorConnection> iter(getRootFloatingTile());

	while (auto p = iter.getNextPanel())
		p->setContentWithUndo(nullptr, 0);

	mainEditor->loadNewContainer(v);

	

}

void BackendRootWindow::loadNewContainer(const File &f)
{
	FloatingTile::Iterator<PanelWithProcessorConnection> iter(getRootFloatingTile());

	while (auto p = iter.getNextPanel())
		p->setContentWithUndo(nullptr, 0);

	mainEditor->loadNewContainer(f);
}

void BackendRootWindow::showWorkspace(int workspace)
{
	currentWorkspace = workspace;

	int workspaceIndex = workspace - BackendCommandTarget::WorkspaceMain;

	for (int i = 0; i < workspaces.size(); i++)
	{
		workspaces[i].getComponent()->getLayoutData().setVisible(i == workspaceIndex);
	}

	getRootFloatingTile()->refreshRootLayout();
}

VerticalTile* BackendPanelHelpers::getMainTabComponent(FloatingTile* root)
{
	static const Identifier id("PersonaContainer");

	return FloatingTileHelpers::findTileWithId<VerticalTile>(root, id);
}

HorizontalTile* BackendPanelHelpers::getMainLeftColumn(FloatingTile* root)
{
	static const Identifier id("MainLeftColumn");

	return FloatingTileHelpers::findTileWithId<HorizontalTile>(root, id);
}

HorizontalTile* BackendPanelHelpers::getMainRightColumn(FloatingTile* root)
{
	static const Identifier id("MainRightColumn");

	return FloatingTileHelpers::findTileWithId<HorizontalTile>(root, id);
}

void BackendPanelHelpers::showWorkspace(BackendRootWindow* root, Workspace workspaceToShow, NotificationType notifyCommandManager)
{
	if (notifyCommandManager == sendNotification)
	{
		root->getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::WorkspaceMain + (int)workspaceToShow, false);
	}
	else
	{
		root->showWorkspace(BackendCommandTarget::WorkspaceMain + (int)workspaceToShow);
	}
}

bool BackendPanelHelpers::isMainWorkspaceActive(FloatingTile* /*root*/)
{
	return true;
}

FloatingTile* BackendPanelHelpers::ScriptingWorkspace::get(BackendRootWindow* rootWindow)
{
	return FloatingTileHelpers::findTileWithId<FloatingTileContainer>(rootWindow->getRootFloatingTile(), "ScriptingWorkspace")->getParentShell();
}

void BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(BackendRootWindow* rootWindow, JavascriptProcessor* jsp)
{
	auto workspace = get(rootWindow);

	FloatingTile::Iterator<GlobalConnectorPanel<JavascriptProcessor>> iter(workspace);

	if (auto connector = iter.getNextPanel())
	{
		connector->setContentWithUndo(dynamic_cast<Processor*>(jsp), 0);
	}
}

void BackendPanelHelpers::ScriptingWorkspace::showEditor(BackendRootWindow* rootWindow, bool shouldBeVisible)
{
	auto workspace = get(rootWindow);

	auto editor = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(workspace, "ScriptingWorkspaceCodeEditor");

	if (editor != nullptr)
	{
		editor->getParentShell()->getLayoutData().setVisible(shouldBeVisible);
		editor->getParentShell()->refreshRootLayout();
	}
    
    auto toggleBar = FloatingTileHelpers::findTileWithId<VisibilityToggleBar>(workspace, "ScriptingWorkspaceToggleBar");
    
    if(toggleBar != nullptr)
    {
        toggleBar->refreshButtons();
    }
}

void BackendPanelHelpers::ScriptingWorkspace::showInterfaceDesigner(BackendRootWindow* rootWindow, bool shouldBeVisible)
{
    auto workspace = get(rootWindow);
    
    auto editor = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(workspace, "ScriptingWorkspaceInterfaceDesigner");
    
    if (editor != nullptr)
    {
        editor->getParentShell()->getLayoutData().setVisible(shouldBeVisible);
        editor->getParentShell()->refreshRootLayout();
    }
    
    auto toggleBar = FloatingTileHelpers::findTileWithId<VisibilityToggleBar>(workspace, "ScriptingWorkspaceToggleBar");
    
    if(toggleBar != nullptr)
    {
        toggleBar->refreshButtons();
    }
    else
        jassertfalse;
    
}


FloatingTile* BackendPanelHelpers::SamplerWorkspace::get(BackendRootWindow* rootWindow)
{
	return FloatingTileHelpers::findTileWithId<FloatingTileContainer>(rootWindow->getRootFloatingTile(), "SamplerWorkspace")->getParentShell();
}

void BackendPanelHelpers::SamplerWorkspace::setGlobalProcessor(BackendRootWindow* rootWindow, ModulatorSampler* sampler)
{
	auto workspace = get(rootWindow);

	FloatingTile::Iterator<GlobalConnectorPanel<ModulatorSampler>> iter(workspace);

	if (auto connector = iter.getNextPanel())
	{
		connector->setContentWithUndo(dynamic_cast<Processor*>(sampler), 0);
	}
}

} // namespace hise