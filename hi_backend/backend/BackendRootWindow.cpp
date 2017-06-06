

BackendRootWindow::BackendRootWindow(AudioProcessor *ownerProcessor, ValueTree& editorState) :
	AudioProcessorEditor(ownerProcessor),
	BackendCommandTarget(static_cast<BackendProcessor*>(ownerProcessor)),
	owner(static_cast<BackendProcessor*>(ownerProcessor))
{
#if PUT_FLOAT_IN_CODEBASE
	addAndMakeVisible(floatingRoot = new FloatingTile(nullptr));


	auto mainPanelC = (FloatingPanelTemplates::createHiseLayout(floatingRoot));

	auto mainPanel = dynamic_cast<MainPanel*>(mainPanelC);

	mainEditor = mainPanel->set(owner, this, editorState);

	workspaces.add(FloatingTileHelpers::findTileWithId<VerticalTile>(floatingRoot, Identifier("MainWorkspace"))->getParentShell());
	workspaces.add(FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("ScriptingWorkspace"))->getParentShell());
	workspaces.add(FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("SamplerWorkspace"))->getParentShell());
	workspaces.add(FloatingTileHelpers::findTileWithId<HorizontalTile>(floatingRoot, Identifier("CustomWorkspace"))->getParentShell());

	showWorkspace(BackendCommandTarget::WorkspaceMain);

	setEditor(this);

	setOpaque(true);

#if IS_STANDALONE_APP 

	if (owner->callback->getCurrentProcessor() == nullptr)
	{
		showSettingsWindow();
	}

#endif

	PresetHandler::buildProcessorDataBase(owner->getMainSynthChain());

	constrainer = new ComponentBoundsConstrainer();
	constrainer->setMinimumHeight(200);
	constrainer->setMinimumWidth(0);
	constrainer->setMaximumWidth(1920);

	

	addAndMakeVisible(borderDragger = new ResizableBorderComponent(this, constrainer));
	BorderSize<int> borderSize;
	borderSize.setTop(0);
	borderSize.setLeft(0);
	borderSize.setRight(0);
	borderSize.setBottom(7);
	borderDragger->setBorderThickness(borderSize);

	addAndMakeVisible(progressOverlay = new ThreadWithQuasiModalProgressWindow::Overlay());
	owner->setOverlay(progressOverlay);
	progressOverlay->setDialog(nullptr);


#if JUCE_MAC && IS_STANDALONE_APP
	MenuBarModel::setMacMainMenu(this);
#else

	addAndMakeVisible(menuBar = new MenuBarComponent(this));
	menuBar->setLookAndFeel(&plaf);

#endif

	setSize(1500, 900);


	startTimer(1000);

	updateCommands();
#endif
}


BackendRootWindow::~BackendRootWindow()
{
#if PUT_FLOAT_IN_CODEBASE
	clearModalComponent();

	modalComponent = nullptr;

	// Remove the resize stuff

	constrainer = nullptr;
	borderDragger = nullptr;
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

#endif
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

void BackendRootWindow::resized()
{
#if PUT_FLOAT_IN_CODEBASE
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

	const float dpiScale = Desktop::getInstance().getGlobalScaleFactor();

	floatingRoot->setBounds(4, menuBarOffset + 4, getWidth() - 8, getHeight() - menuBarOffset - 8);

#if IS_STANDALONE_APP

	if (currentDialog != nullptr)
	{
		currentDialog->centreWithSize(700, 500);
	}

#else

	borderDragger->setBounds(getBounds());

#endif
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
	if (notifyCommandManager = sendNotification)
	{
		root->getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::WorkspaceMain + (int)workspaceToShow, false);
	}
	else
	{
		root->showWorkspace(BackendCommandTarget::WorkspaceMain + (int)workspaceToShow);
	}
}

bool BackendPanelHelpers::isMainWorkspaceActive(FloatingTile* root)
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

	auto editor = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(workspace, "ScriptEditor");

	if (editor != nullptr)
	{
		editor->getParentShell()->getLayoutData().setVisible(shouldBeVisible);
		editor->getParentShell()->refreshRootLayout();
	}
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
