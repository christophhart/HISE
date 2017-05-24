

BackendRootWindow::BackendRootWindow(AudioProcessor *ownerProcessor, ValueTree& editorState) :
	AudioProcessorEditor(ownerProcessor),
	BackendCommandTarget(static_cast<BackendProcessor*>(ownerProcessor)),
	owner(static_cast<BackendProcessor*>(ownerProcessor))
{
#if PUT_FLOAT_IN_CODEBASE
	addAndMakeVisible(floatingRoot = new FloatingTile(nullptr));


	auto mainPanelC = (FloatingPanelTemplates::createMainPanel(floatingRoot));

	auto mainPanel = dynamic_cast<MainPanel*>(mainPanelC);

	mainEditor = mainPanel->set(owner, this, editorState);

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

	setSize(900, 700);


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

	floatingRoot->setBounds(getLocalBounds().withTrimmedTop(menuBarOffset));

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
	mainEditor->loadNewContainer(v);
}

void BackendRootWindow::loadNewContainer(const File &f)
{
	mainEditor->loadNewContainer(f);
}
