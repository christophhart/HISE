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
*   which must be separately licensed for cloused source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#define toggleVisibility(x) {x->setVisible(!x->isVisible()); owner->setComponentShown(info.commandID, x->isVisible());}

BackendProcessorEditor::BackendProcessorEditor(FloatingTile* parent) :
FloatingTileContent(parent),
owner(static_cast<BackendProcessor*>(parent->getMainController())),
parentRootWindow(parent->getBackendRootWindow()),
rootEditorIsMainSynthChain(true)
{
    setOpaque(true);

	setLookAndFeel(&lookAndFeelV3);

	addAndMakeVisible(viewport = new CachedViewport());
	addAndMakeVisible(breadCrumbComponent = new BreadcrumbComponent(owner));
	
	addChildComponent(debugLoggerWindow = new DebugLoggerComponent(&owner->getDebugLogger()));

	viewport->viewport->setScrollBarThickness(SCROLLBAR_WIDTH);
	viewport->viewport->setSingleStepSizes(0, 6);

	setRootProcessor(owner->synthChain->getRootProcessor());

	owner->addScriptListener(this);

};

BackendProcessorEditor::~BackendProcessorEditor()
{
	owner->removeScriptListener(this);
	
	
	// Remove the popup components

	popupEditor = nullptr;
	stupidRectangle = nullptr;
	
	// Remove the toolbar stuff

	breadCrumbComponent = nullptr;

	// Remove the main stuff

	container = nullptr;
	viewport = nullptr;
}


void BackendProcessorEditor::addProcessorToPanel(Processor *p)
{
	if (p != owner->synthChain->getRootProcessor() && p != owner->synthChain)
	{
		p->setEditorState("Solo", true, sendNotification);
		container->addSoloProcessor(p);
	}
}

void BackendProcessorEditor::removeProcessorFromPanel(Processor *p)
{
	if (p != owner->synthChain->getRootProcessor() && p != owner->synthChain)
	{
		p->setEditorState("Solo", false, sendNotification);
		container->removeSoloProcessor(p);
	}
}


void BackendProcessorEditor::setRootProcessor(Processor *p, int scrollY/*=0*/)
{
	const bool rootEditorWasMainSynthChain = rootEditorIsMainSynthChain;

	rootEditorIsMainSynthChain = (p == owner->synthChain);

	owner->synthChain->setRootProcessor(p);

	if (p == nullptr) return;

	rebuildContainer();

	currentRootProcessor = p;
	container->setRootProcessorEditor(p);
	
	Processor::Iterator<Processor> iter(owner->synthChain, false);

	Processor *iterProcessor;

	while ((iterProcessor = iter.getNextProcessor()) != nullptr)
	{
		if ((bool)iterProcessor->getEditorState("Solo"))
			addProcessorToPanel(iterProcessor);
	}

	breadCrumbComponent->refreshBreadcrumbs();

	if (scrollY != 0)
	{
		owner->setScrollY(scrollY);
		resized();
	}
	else if (rootEditorIsMainSynthChain != rootEditorWasMainSynthChain)
	{
		resized();
	}

	// This is used because the viewport size can change depending on the breadcrumbs and full screen editors will get cut off
	container->refreshSize();
}

void BackendProcessorEditor::rebuildContainer()
{
	removeContainer();

	viewport->viewport->setViewedComponent(container = new ProcessorEditorContainer());
}

void BackendProcessorEditor::removeContainer()
{
	container = nullptr;
}

void BackendProcessorEditor::setRootProcessorWithUndo(Processor *p)
{
    if(getRootContainer()->getRootEditor()->getProcessor() != p)
    {
        owner->viewUndoManager->beginNewTransaction(getRootContainer()->getRootEditor()->getProcessor()->getId() + " -> " + p->getId());
        owner->viewUndoManager->perform(new ViewBrowsing(owner->synthChain, this, viewport->viewport->getViewPositionY(), p));
        parentRootWindow->updateCommands();
    }
}

void BackendProcessorEditor::setViewportPositions(int viewportX, const int viewportY, const int /*viewportWidth*/, int /*viewportHeight*/)
{
	debugLoggerWindow->setBounds(0, getHeight() - 60, getWidth(), 60);

	const int containerHeight = getHeight() - viewportY;

	viewport->setVisible(containerHeight > 0);
	viewport->setBounds(viewportX, viewportY, getWidth()-viewportX, containerHeight); // Overlap with the fade

	if (currentPopupComponent != nullptr)
	{
		jassert(stupidRectangle != nullptr);
		stupidRectangle->setBounds(viewport->getBounds());
		currentPopupComponent->setTopLeftPosition(currentPopupComponent->getX(), viewportY + 40);
	}
}

bool BackendProcessorEditor::isPluginPreviewShown() const
{
	return previewWindow != nullptr;
}

bool BackendProcessorEditor::isPluginPreviewCreatable() const
{
    return owner->synthChain->hasDefinedFrontInterface();
}


void BackendProcessorEditor::paint(Graphics &g)
{
    g.setColour(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));
    
    g.fillAll();
}


void BackendProcessorEditor::resized()
{
	const int breadcrumbHeight = rootEditorIsMainSynthChain ? 0 : 30;
	const int viewportY = 4; // 0 + 8 + 24 + 8;
	
	const int viewportHeight = getHeight();// -viewportY - (keyboard->isVisible() ? 0 : 10);
	
	const int viewportWidth = getWidth() - 32;

	int viewportX, poolX, inspectorX;
	int poolY, inspectorY;

	int heightOfSideColumns, sideColumnWidth;

	bool poolVisible, inspectorVisible;

	viewportX = 16;

	poolVisible = false;
	inspectorVisible = false;
	inspectorX = inspectorY = poolX = poolY = 0;

	sideColumnWidth = heightOfSideColumns = 0;

	//setToolBarPosition(viewportX, 4 , viewportWidth, 28);

	breadCrumbComponent->setBounds(viewportX, viewportY + 3, viewportWidth, breadcrumbHeight);

	setViewportPositions(viewportX, viewportY + breadcrumbHeight, viewportWidth, viewportHeight);

	viewport->viewport->setViewPosition(0, owner->getScrollY());

	if(stupidRectangle != nullptr && currentPopupComponent.get() != nullptr)
    {
        stupidRectangle->setBounds(viewportX, viewportY, viewportWidth, viewportHeight);
        currentPopupComponent->setBounds(viewportX, viewportY + 40, viewportWidth, viewportHeight-40);
    }
    
}

void BackendProcessorEditor::clearPopup()
{
	if (ownedPopupComponent == nullptr)
	{
		if (currentPopupComponent == nullptr) return;

		else if (currentPopupComponent == popupEditor)
		{
			// Update the original editor
			popupEditor->getProcessor()->sendChangeMessage();
			popupEditor = nullptr;

			currentPopupComponent = nullptr;
		}
		else if (dynamic_cast<SampleMapEditor*>(currentPopupComponent.get()) != nullptr)
		{
			stupidRectangle->setVisible(false);
			currentPopupComponent->setVisible(false);

			dynamic_cast<SampleMapEditor*>(currentPopupComponent.get())->deletePopup();
			currentPopupComponent = nullptr;
		}
	}
	else
	{
		stupidRectangle->setVisible(false);

		ownedPopupComponent = nullptr;
	}

	
	
	stupidRectangle = nullptr;
	viewport->setEnabled(true);
	viewport->viewport->setScrollBarsShown(true, false);
}

void BackendProcessorEditor::scriptWasCompiled(JavascriptProcessor * /*sp*/)
{
	parentRootWindow->updateCommands();
}

void BackendProcessorEditor::showPseudoModalWindow(Component *componentToShow, const String &title, bool ownComponent/*=false*/)
{
	if (ownComponent)
	{
		ownedPopupComponent = componentToShow;
		currentPopupComponent = nullptr;		
	}
	else
	{
		ownedPopupComponent = nullptr;
		currentPopupComponent = componentToShow;
	}

	addAndMakeVisible(stupidRectangle = new StupidRectangle());

	stupidRectangle->setText(title);

	stupidRectangle->addMouseListener(this, true);

	const int height = getHeight() - viewport->getY();

	stupidRectangle->setBounds(viewport->getX(), viewport->getY(), viewport->getWidth() - SCROLLBAR_WIDTH, height);

	addAndMakeVisible(componentToShow);

	componentToShow->setBounds(viewport->getX(), viewport->getY() + 40, viewport->getWidth()-SCROLLBAR_WIDTH, componentToShow->getHeight());

	stupidRectangle->setVisible(true);
	componentToShow->setVisible(true);

	viewport->setEnabled(false);

	viewport->viewport->setScrollBarsShown(false, false);

	componentToShow->setAlwaysOnTop(true);
}

void BackendProcessorEditor::loadNewContainer(const File &f)
{
	MainController::ScopedSuspender ss(getBackendProcessor());

	clearPreset();

	f.setLastAccessTime(Time::getCurrentTime());

	if (f.getParentDirectory().getFileName() == "Presets")
	{
		GET_PROJECT_HANDLER(getMainSynthChain()).setWorkingProject(f.getParentDirectory().getParentDirectory(), this);
	}

	getBackendProcessor()->getMainSynthChain()->setBypassed(true);



	clearModuleList();

	container = nullptr;

	owner->loadPreset(f, this);

	auto refreshFunction = [this]()->void {refreshInterfaceAfterPresetLoad(); parentRootWindow->sendRootContainerRebuildMessage(false); };
	new DelayedFunctionCaller(refreshFunction, 300);
	
}

void BackendProcessorEditor::refreshInterfaceAfterPresetLoad()
{
    Processor *p = static_cast<Processor*>(owner->synthChain);
    
	rebuildContainer();
    
    container->setRootProcessorEditor(p);
}

void BackendProcessorEditor::loadNewContainer(ValueTree &v)
{
    const int presetVersion = v.getProperty("BuildVersion", 0);
    
    if(presetVersion > BUILD_SUB_VERSION)
    {
        PresetHandler::showMessageWindow("Version mismatch", "The preset was built with a newer the build of HISE: " + String(presetVersion) + ". To ensure perfect compatibility, update to at least this build.", PresetHandler::IconType::Warning);
    }
    
	MainController::ScopedSuspender ss(getBackendProcessor());

    clearPreset();

	getBackendProcessor()->getMainSynthChain()->setBypassed(true);

	clearModuleList();

	container = nullptr;

	owner->loadPreset(v, this);

	auto refreshFunction = [this]()->void {refreshInterfaceAfterPresetLoad(); parentRootWindow->sendRootContainerRebuildMessage(false); };
	new DelayedFunctionCaller(refreshFunction, 300);
}

void BackendProcessorEditor::clearPreset()
{
	getBackendProcessor()->getMainSynthChain()->setBypassed(true);

	setPluginPreviewWindow(nullptr);

	clearModuleList();

    container = nullptr;
    
    owner->clearPreset();
    
    Processor *p = static_cast<Processor*>(owner->synthChain);
    
    
    
    
	owner->synthChain->setIconColour(Colours::transparentBlack);

	owner->getSampleManager().getImagePool()->clearData();
	owner->getSampleManager().getAudioSampleBufferPool()->clearData();

    owner->synthChain->setId("Master Chain");
    
	for (int i = 0; i < owner->synthChain->getNumInternalChains(); i++)
	{
		owner->synthChain->getChildProcessor(i)->setEditorState(owner->synthChain->getEditorStateForIndex(Processor::Visible), false, sendNotification);
	}

    for(int i = 0; i < ModulatorSynth::numModulatorSynthParameters; i++)
    {
        owner->synthChain->setAttribute(i, owner->synthChain->getDefaultValue(i), dontSendNotification);
    }

	rebuildContainer();

	container->setRootProcessorEditor(p);

	parentRootWindow->sendRootContainerRebuildMessage(false);

	getBackendProcessor()->getMainSynthChain()->setBypassed(false);
}

void BackendProcessorEditor::clearModuleList()
{

	//dynamic_cast<PatchBrowser*>(propertyDebugArea->getComponentForIndex(PropertyDebugArea::ModuleBrowser))->clearCollections();
}

#undef toggleVisibility


MainTopBar::MainTopBar(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(hiseButton = new ImageButton("HISE"));

	Image hise = ImageCache::getFromMemory(BinaryData::logo_mini_png, BinaryData::logo_mini_pngSize);
	

	hiseButton->setImages(false, true, true, hise, 0.9f, Colour(0), hise, 1.0f, Colours::white.withAlpha(0.1f), hise, 1.0f, Colours::white.withAlpha(0.1f), 0.1f);
	hiseButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuHelpShowAboutPage, true);

	addAndMakeVisible(backButton = new ShapeButton("Back", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.8f), Colours::white));
	ScopedPointer<DrawablePath> bPath = dynamic_cast<DrawablePath*>(MainToolbarFactory::MainToolbarPaths::createPath(BackendCommandTarget::MenuViewBack, true));
	backButton->setShape(bPath->getPath(), false, true, true);
	backButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuViewBack, true);

	parent->getRootFloatingTile()->addPopupListener(this);

	addAndMakeVisible(forwardButton = new ShapeButton("Forward", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.8f), Colours::white));
	ScopedPointer<DrawablePath> fPath = dynamic_cast<DrawablePath*>(MainToolbarFactory::MainToolbarPaths::createPath(BackendCommandTarget::MenuViewForward, true));
	forwardButton->setShape(fPath->getPath(), false, true, true);
	forwardButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuViewForward, true);

	addAndMakeVisible(macroButton = new ShapeButton("Macro Controls", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	macroButton->setTooltip("Show 8 Macro Controls");
	macroButton->setShape(FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::MacroControls), false, true, true);
	macroButton->addListener(this);

	addAndMakeVisible(presetBrowserButton = new ShapeButton("Preset Browser", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	presetBrowserButton->setTooltip("Show Preset Browser");
	presetBrowserButton->setShape(FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::PresetBrowser), false, true, true);
	presetBrowserButton->addListener(this);

	addAndMakeVisible(pluginPreviewButton = new ShapeButton("Plugin Preview", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	pluginPreviewButton->setTooltip("Show Plugin Preview");
	pluginPreviewButton->setShape(FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::ScriptContent), false, true, true);
	pluginPreviewButton->addListener(this);



	addAndMakeVisible(mainWorkSpaceButton = new ShapeButton("Main Workspace", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colour(SIGNAL_COLOUR)));
	mainWorkSpaceButton->setTooltip("Show Main Workspace");
	mainWorkSpaceButton->setShape(FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::numOptions), false, true, true);
	mainWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceMain, true);

	addAndMakeVisible(scriptingWorkSpaceButton = new ShapeButton("Scripting Workspace", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colour(SIGNAL_COLOUR)));
	scriptingWorkSpaceButton->setTooltip("Show Scripting Workspace");
	scriptingWorkSpaceButton->setShape(FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::ScriptEditor), false, true, true);
	scriptingWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceScript, true);

	addAndMakeVisible(samplerWorkSpaceButton = new ShapeButton("Sampler Workspace", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colour(SIGNAL_COLOUR)));
	samplerWorkSpaceButton->setTooltip("Show Scripting Workspace");
	samplerWorkSpaceButton->setShape(FloatingTileContent::Factory::getPath(FloatingTileContent::Factory::PopupMenuOptions::SampleEditor), false, true, true);
	samplerWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceSampler, true);

	addAndMakeVisible(customWorkSpaceButton = new ShapeButton("Custom Workspace", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colour(SIGNAL_COLOUR)));
	customWorkSpaceButton->setTooltip("Show Scripting Workspace");
	customWorkSpaceButton->setShape(ColumnIcons::getPath(ColumnIcons::customizeIcon, sizeof(ColumnIcons::customizeIcon)), false, true, true);
	customWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceCustom, true);
	
	addAndMakeVisible(peakMeter = new ProcessorPeakMeter(getRootWindow()->getMainSynthChain()));

	addAndMakeVisible(settingsButton = new ShapeButton("Audio Settings", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	settingsButton->setTooltip("Show Audio Settings");


	static const unsigned char settings[] = { 110,109,8,103,132,67,84,212,84,67,98,3,255,131,67,84,212,84,67,159,170,131,67,25,125,85,67,159,170,131,67,73,77,86,67,98,159,170,131,67,64,29,87,67,3,255,131,67,99,198,87,67,8,103,132,67,99,198,87,67,98,22,207,132,67,99,198,87,67,219,34,133,67,65,29,
		87,67,219,34,133,67,73,77,86,67,98,219,34,133,67,25,125,85,67,22,207,132,67,84,212,84,67,8,103,132,67,84,212,84,67,99,109,202,224,133,67,213,37,87,67,108,212,190,133,67,113,201,87,67,108,102,251,133,67,94,183,88,67,108,99,3,134,67,201,214,88,67,108,103,
		175,133,67,194,126,89,67,108,156,37,133,67,152,252,88,67,108,205,211,132,67,201,63,89,67,108,72,170,132,67,255,61,90,67,108,251,164,132,67,189,95,90,67,108,70,46,132,67,189,95,90,67,108,230,250,131,67,206,64,89,67,108,24,169,131,67,83,253,88,67,108,244,
		49,131,67,47,118,89,67,108,63,34,131,67,231,133,89,67,108,76,206,130,67,20,222,88,67,108,78,15,131,67,87,202,87,67,108,154,237,130,67,223,38,87,67,108,181,110,130,67,249,211,86,67,108,224,93,130,67,17,201,86,67,108,224,93,130,67,203,219,85,67,108,115,
		237,130,67,230,116,85,67,108,39,15,131,67,149,209,84,67,108,195,210,130,67,37,227,83,67,108,202,202,130,67,221,195,83,67,108,161,30,131,67,46,28,83,67,108,156,168,131,67,31,158,83,67,108,79,250,131,67,146,90,83,67,108,203,35,132,67,127,92,82,67,108,37,
		41,132,67,213,58,82,67,108,210,159,132,67,213,58,82,67,108,59,211,132,67,35,90,83,67,108,209,36,133,67,175,157,83,67,108,0,156,133,67,194,36,83,67,108,219,171,133,67,207,20,83,67,108,197,255,133,67,126,188,83,67,108,195,190,133,67,2,208,84,67,108,91,
		224,133,67,178,115,85,67,108,156,95,134,67,170,198,85,67,108,86,112,134,67,93,209,85,67,108,86,112,134,67,143,190,86,67,108,203,224,133,67,208,37,87,67,99,109,112,6,129,67,84,140,74,67,98,83,76,128,67,84,140,74,67,176,106,127,67,74,186,75,67,176,106,
		127,67,198,46,77,67,98,176,106,127,67,221,162,78,67,83,76,128,67,121,209,79,67,112,6,129,67,121,209,79,67,98,157,192,129,67,121,209,79,67,126,86,130,67,219,162,78,67,126,86,130,67,198,46,77,67,98,125,86,130,67,74,186,75,67,157,192,129,67,84,140,74,67,
		112,6,129,67,84,140,74,67,99,109,80,170,131,67,54,178,78,67,108,142,109,131,67,240,214,79,67,108,238,217,131,67,161,128,81,67,108,60,232,131,67,213,184,81,67,108,248,81,131,67,94,229,82,67,108,110,91,130,67,127,252,81,67,108,17,201,129,67,181,116,82,
		67,108,199,126,129,67,139,59,84,67,108,72,117,129,67,232,119,84,67,108,227,160,128,67,232,119,84,67,108,249,68,128,67,136,118,82,67,108,56,101,127,67,204,253,81,67,108,227,186,125,67,7,214,82,67,108,174,130,125,67,30,242,82,67,108,71,86,124,67,216,197,
		81,67,108,230,62,125,67,127,216,79,67,108,75,198,124,67,9,180,78,67,108,58,0,123,67,183,31,78,67,108,253,195,122,67,60,12,78,67,108,253,195,122,67,182,99,76,67,108,194,197,124,67,158,171,75,67,108,93,62,125,67,105,135,74,67,108,68,102,124,67,205,220,
		72,67,108,193,73,124,67,220,164,72,67,108,195,117,125,67,217,120,71,67,108,128,99,127,67,87,97,72,67,108,235,67,128,67,121,232,71,67,108,36,142,128,67,230,33,70,67,108,180,151,128,67,170,229,69,67,108,7,108,129,67,170,229,69,67,108,2,200,129,67,178,231,
		71,67,108,251,89,130,67,144,96,72,67,108,56,47,131,67,52,136,71,67,108,149,75,131,67,177,107,71,67,108,184,225,131,67,180,151,72,67,108,105,109,131,67,168,132,74,67,108,132,169,131,67,133,169,75,67,108,50,141,132,67,247,61,76,67,108,30,171,132,67,23,
		81,76,67,108,30,171,132,67,123,249,77,67,108,76,170,131,67,56,178,78,67,99,101,0,0 };

	Path settingsPath;
	settingsPath.loadPathFromData(settings, sizeof(settings));

	settingsButton->setShape(settingsPath, false, true, true);
	settingsButton->addListener(this);

	addAndMakeVisible(layoutButton = new ShapeButton("Toggle Layout", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colour(SIGNAL_COLOUR)));
	layoutButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuViewEnableGlobalLayoutMode, true);
	
	Path layoutPath;
	layoutPath.loadPathFromData(ColumnIcons::layoutIcon, sizeof(ColumnIcons::layoutIcon));
	layoutButton->setShape(layoutPath, false, true, true);
	
	addAndMakeVisible(tooltipBar = new TooltipBar());
	addAndMakeVisible(voiceCpuBpmComponent = new VoiceCpuBpmComponent(parent->getBackendRootWindow()->getBackendProcessor()));
    
	tooltipBar->setColour(TooltipBar::ColourIds::backgroundColour, HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));
	tooltipBar->setColour(TooltipBar::ColourIds::textColour, Colours::white);
	tooltipBar->setColour(TooltipBar::ColourIds::iconColour, Colours::white);
	//tooltipBar->setShowInfoIcon(false);
}

MainTopBar::~MainTopBar()
{
	getParentShell()->getRootFloatingTile()->removePopupListener(this);
}

void setColoursForButton(ShapeButton* b, bool on)
{
	if(on)
		b->setColours(Colour(SIGNAL_COLOUR).withAlpha(0.95f), Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR));
	else
		b->setColours(Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white);
}


class InterfaceCreator : public Component,
	public ComboBox::Listener,
	public ButtonListener,
	public Label::Listener
{
public:

	enum SizePresets
	{
		Small = 1,
		Medium,
		Large,
		KONTAKT,
		iPhone,
		iPad,
		iPhoneAUv3,
		iPadAUv3
	};

	InterfaceCreator()
	{
		setName("Create User Interface");

		setWantsKeyboardFocus(true);
		grabKeyboardFocus();

		addAndMakeVisible(sizeSelector = new ComboBox());
		sizeSelector->setLookAndFeel(&klaf);

		sizeSelector->addItem("Small", Small);
		sizeSelector->addItem("Medium", Medium);
		sizeSelector->addItem("Large", Large);
		sizeSelector->addItem("KONTAKT Width", KONTAKT);
		sizeSelector->addItem("iPhone", iPhone);
		sizeSelector->addItem("iPad", iPad);
		sizeSelector->addItem("iPhoneAUv3", iPhoneAUv3);
		sizeSelector->addItem("iPadAUv3", iPadAUv3);

		sizeSelector->addListener(this);
		sizeSelector->setTextWhenNothingSelected("Select Preset Size");

		sizeSelector->setColour(MacroControlledObject::HiBackgroundColours::upperBgColour, Colour(0x66333333));
		sizeSelector->setColour(MacroControlledObject::HiBackgroundColours::lowerBgColour, Colour(0xfb111111));
		sizeSelector->setColour(MacroControlledObject::HiBackgroundColours::outlineBgColour, Colours::white.withAlpha(0.3f));
		sizeSelector->setColour(MacroControlledObject::HiBackgroundColours::textColour, Colours::white);

		addAndMakeVisible(widthLabel = new Label("width"));
		addAndMakeVisible(heightLabel = new Label("height"));

		widthLabel->setFont(GLOBAL_BOLD_FONT());
		widthLabel->addListener(this);
		widthLabel->setColour(Label::ColourIds::backgroundColourId, Colours::white);
		widthLabel->setEditable(true, true);

		heightLabel->setFont(GLOBAL_BOLD_FONT());
		heightLabel->addListener(this);
		heightLabel->setColour(Label::ColourIds::backgroundColourId, Colours::white);
		heightLabel->setEditable(true, true);

		addAndMakeVisible(resizer = new ResizableCornerComponent(this, nullptr));
		addAndMakeVisible(closeButton = new TextButton("OK"));
		closeButton->addListener(this);
		closeButton->setLookAndFeel(&alaf);

		addAndMakeVisible(cancelButton = new TextButton("Cancel"));
		cancelButton->addListener(this);
		cancelButton->setLookAndFeel(&alaf);

		setSize(600, 500);
	}

	void resized() override
	{
		sizeSelector->setBounds(getWidth() / 2 - 120, getHeight() / 2 - 40, 240, 30);

		widthLabel->setBounds(getWidth() / 2 - 120, getHeight() / 2, 110, 30);
		heightLabel->setBounds(getWidth() / 2 + 10, getHeight() / 2, 110, 30);

		resizer->setBounds(getWidth() - 20, getHeight() - 20, 20, 20);

		widthLabel->setText(String(getWidth()), dontSendNotification);
		heightLabel->setText(String(getHeight()), dontSendNotification);

		closeButton->setBounds(getWidth() / 2 - 100, getHeight() - 40, 90, 30);
		cancelButton->setBounds(getWidth() / 2 + 10, getHeight() - 40, 90, 30);
	}

	void buttonClicked(Button* b) override
	{
		if (b == closeButton)
		{
			auto bpe = GET_BACKEND_ROOT_WINDOW(this)->getMainPanel();

			if (bpe != nullptr)
			{
				auto midiChain = dynamic_cast<MidiProcessorChain*>(bpe->getMainSynthChain()->getChildProcessor(ModulatorSynthChain::MidiProcessor));

				auto s = bpe->getMainSynthChain()->getMainController()->createProcessor(midiChain->getFactoryType(), "ScriptProcessor", "Interface");

				auto jsp = dynamic_cast<JavascriptProcessor*>(s);

				String code = "Content.makeFrontInterface(" + String(getWidth()) + ", " + String(getHeight()) + ");";

				jsp->getSnippet(0)->replaceAllContent(code);
				jsp->compileScript();

				midiChain->getHandler()->add(s, nullptr);

				midiChain->setEditorState(Processor::EditorState::Visible, true);
				s->setEditorState(Processor::EditorState::Folded, true);

				auto root = GET_BACKEND_ROOT_WINDOW(this);
				
				root->sendRootContainerRebuildMessage(true);

				if (PresetHandler::showYesNoWindow("Switch to Interface Designer", "Do you want to switch to the interface designer mode?"))
				{
					root->getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::WorkspaceScript, false);
					
					BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(root, jsp);
                    BackendPanelHelpers::ScriptingWorkspace::showInterfaceDesigner(root, true);
				}
                
                auto rootContainer = root->getMainPanel()->getRootContainer();
                
                auto editorOfParent = rootContainer->getFirstEditorOf(root->getMainSynthChain());
                auto editorOfChain = rootContainer->getFirstEditorOf(midiChain);
                
                editorOfParent->getChainBar()->refreshPanel();
                editorOfParent->sendResizedMessage();
                editorOfChain->changeListenerCallback(editorOfChain->getProcessor());
                editorOfChain->childEditorAmountChanged();
			}
		}

		findParentComponentOfClass<FloatingTilePopup>()->deleteAndClose();
	};

	bool keyPressed(const KeyPress& key) override
	{
		if (key.isKeyCode(KeyPress::returnKey))
		{
			closeButton->triggerClick();
			return true;
		}
		else if (key.isKeyCode(KeyPress::escapeKey))
		{
			cancelButton->triggerClick();
			return true;
		}

		return false;
	}

	void comboBoxChanged(ComboBox* c) override
	{
		SizePresets p = (SizePresets)c->getSelectedId();

		switch (p)
		{
		case InterfaceCreator::Small:
			centreWithSize(500, 400);
			break;
		case InterfaceCreator::Medium:
			centreWithSize(800, 600);
			break;
		case InterfaceCreator::Large:
			centreWithSize(1200, 700);
			break;
		case InterfaceCreator::KONTAKT:
			centreWithSize(633, 400);
			break;
		case InterfaceCreator::iPhone:
			centreWithSize(568, 320);
			break;
		case InterfaceCreator::iPad:
			centreWithSize(1024, 768);
			break;
		case InterfaceCreator::iPhoneAUv3:
			centreWithSize(568, 240);
			break;
		case InterfaceCreator::iPadAUv3:
			centreWithSize(1024, 440);
			break;
		default:
			break;
		}
	}

	void labelTextChanged(Label* l) override
	{
		if (l == widthLabel)
		{
			setSize(widthLabel->getText().getIntValue(), getHeight());
		}
		else if (l == heightLabel)
		{
			setSize(getWidth(), heightLabel->getText().getIntValue());
		}
	}


	void paint(Graphics& g) override
	{
#if 0
		g.fillAll(Colour(0xFF222222));
		g.setColour(Colour(0xFF555555));
		g.fillRect(getLocalBounds().withHeight(40));
		g.setColour(Colour(0xFFCCCCCC));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(18.0f));
		g.drawText("Create User Interface", getLocalBounds().withTop(10), Justification::centredTop);
#endif

		g.fillAll(Colour(0xFF222222));

		g.setColour(Colours::white.withAlpha(0.4f));

		g.drawRect(getLocalBounds(), 1);

		g.setColour(Colours::white.withAlpha(0.05f));

		for (int i = 10; i < getWidth(); i += 10)
		{
			g.drawVerticalLine(i, 0.0f, (float)getHeight());
		}

		for (int i = 10; i < getHeight(); i += 10)
		{
			g.drawHorizontalLine(i, 0.0f, (float)getWidth());
		}

		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white.withAlpha(0.7f));

		g.drawMultiLineText("Resize this window, or select a size preset and press OK to create a script interface with this size", 10, 20, getWidth() - 20);


	}

private:

	AlertWindowLookAndFeel alaf;
	KnobLookAndFeel klaf;

	ScopedPointer<Label> widthLabel;
	ScopedPointer<Label> heightLabel;

	ScopedPointer<TextButton> closeButton;
	ScopedPointer<TextButton> cancelButton;

	ScopedPointer<ComboBox> sizeSelector;

	ScopedPointer<ResizableCornerComponent> resizer;
	
};


void MainTopBar::popupChanged(Component* newComponent)
{
	bool macroShouldBeOn = dynamic_cast<MacroComponent*>(newComponent) != nullptr;
	bool settingsShouldBeOn = (newComponent != nullptr && newComponent->getName() == "Settings");
	bool previewShouldBeShown = (newComponent != nullptr && newComponent->getName() == "Interface Preview") ||
								(newComponent != nullptr && newComponent->getName() == "Create User Interface");
	bool presetBrowserShown = dynamic_cast<MultiColumnPresetBrowser*>(newComponent) != nullptr;

	setColoursForButton(macroButton, macroShouldBeOn);
	setColoursForButton(settingsButton, settingsShouldBeOn);
	setColoursForButton(pluginPreviewButton, previewShouldBeShown);
	setColoursForButton(presetBrowserButton, presetBrowserShown);
	macroButton->setToggleState(macroShouldBeOn, dontSendNotification);
	settingsButton->setToggleState(settingsShouldBeOn, dontSendNotification);
	pluginPreviewButton->setToggleState(previewShouldBeShown, dontSendNotification);
	presetBrowserButton->setToggleState(presetBrowserShown, dontSendNotification);
}

void MainTopBar::paint(Graphics& g)
{
	//g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId));

	

	Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF424242));
	Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF404040));


	g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));
	g.fillAll();
	
	g.setColour(Colours::white.withAlpha(0.2f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Frontend Panels", frontendArea.withTrimmedBottom(11), Justification::centredBottom);
	g.drawText("Workspaces", workspaceArea.withTrimmedBottom(11), Justification::centredBottom);

#if 0
	g.setColour(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));
	g.drawVerticalLine(frontendArea.getRight() + 20, 0.0f, (float)getHeight());
	g.drawVerticalLine(workspaceArea.getX() - 20, 0.0f, (float)getHeight());
	g.drawVerticalLine(workspaceArea.getRight() + (frontendArea.getX() - workspaceArea.getRight())/2, 0.0f, (float)getHeight());
#endif
	
}

void MainTopBar::buttonClicked(Button* b)
{
	if (b == macroButton)
	{
		togglePopup(PopupType::Macro, !b->getToggleState());
	}
	else if (b == settingsButton)
	{
		togglePopup(PopupType::Settings, !b->getToggleState());
	}
	else if (b == pluginPreviewButton)
	{
		togglePopup(PopupType::PluginPreview, !b->getToggleState());
	}
	else if (b == presetBrowserButton)
	{
		togglePopup(PopupType::PresetBrowser, !b->getToggleState());
	}
}

void MainTopBar::resized()
{
	const int centerY = 3;

	int x = 10;

	const int hiseButtonSize = 40;
	const int hiseButtonOffset = (getHeight() - hiseButtonSize) / 2;

	hiseButton->setBounds(hiseButtonOffset, hiseButtonOffset, hiseButtonSize, hiseButtonSize);

	x = hiseButton->getRight() + 10;

	const int backButtonSize = 24;
	const int backButtonOffset = (getHeight() - backButtonSize) / 2;

	backButton->setBounds(x, backButtonOffset, backButtonSize, backButtonSize);

	x = backButton->getRight() + 4;

	forwardButton->setBounds(x, backButtonOffset, backButtonSize, backButtonSize);

	const int leftX = forwardButton->getRight() + 4;

	const int settingsWidth = 320;
	Rectangle<int> settingsArea(getWidth() - settingsWidth, centerY, settingsWidth, getHeight() - centerY);
	tooltipBar->setBounds(settingsArea.getX(), getHeight() - 24, settingsWidth, 24);
	voiceCpuBpmComponent->setBounds(settingsArea.getX(), 4, 120, 28);
	x = settingsArea.getRight() - 28 - 8;
	layoutButton->setBounds(x, centerY, 28, 28);
	
	x = layoutButton->getX() - 28 - 8;
	
#if IS_STANDALONE_APP 

	settingsButton->setBounds(x, centerY, 28, 28);
	peakMeter->setBounds(voiceCpuBpmComponent->getRight() - 2, centerY + 4, settingsButton->getX() - voiceCpuBpmComponent->getRight(), 24);

#else

	settingsButton->setVisible(false);
	peakMeter->setBounds(voiceCpuBpmComponent->getRight() + 2, centerY + 4, layoutButton->getX() - voiceCpuBpmComponent->getRight() - 4, 24);

#endif

	
	
	


	const int rightX = settingsArea.getX() - 4;

	const int workspaceWidth = 180;

	int frontendWidth = 180;
	
	int centerX = leftX + (rightX - leftX) / 2;

	x = centerX - workspaceWidth - 40;

	workspaceArea = Rectangle<int>(x, centerY + 3, workspaceWidth, getHeight() - centerY);

	mainWorkSpaceButton->setBounds(x, workspaceArea.getY(), 32, 32);
	
	x += (workspaceWidth-32) / 3;

	scriptingWorkSpaceButton->setBounds(x, workspaceArea.getY(), 32, 32);

	x += (workspaceWidth - 32) / 3;

	samplerWorkSpaceButton->setBounds(x, workspaceArea.getY(), 32, 32);
	x += (workspaceWidth - 32) / 3;

	customWorkSpaceButton->setBounds(x, workspaceArea.getY(), 32, 32);
	x += (workspaceWidth - 32) / 3;


	x = centerX + 40;

	frontendArea = Rectangle<int>(x, centerY + 3, frontendWidth, getHeight() - centerY);

	int macroX = frontendArea.getX();

	macroButton->setBounds(macroX, frontendArea.getY(), 32, 32);

	pluginPreviewButton->setBounds(frontendArea.getCentreX() - 16, frontendArea.getY(), 32, 32);

	presetBrowserButton->setBounds(frontendArea.getRight() - 32, frontendArea.getY(), 32, 32);



}


class OwningComponent : public Component
{
	public:

	OwningComponent(Component* c)
	{
		addAndMakeVisible(ownedComponent = c);

		setSize(c->getWidth(), c->getHeight());
	}
	
	~OwningComponent() { ownedComponent = nullptr; }

	void resized() override
	{
		
	}

private:

	ScopedPointer<Component> ownedComponent;
};


void MainTopBar::togglePopup(PopupType t, bool shouldShow)
{
	if (!shouldShow)
	{
		getParentShell()->getRootFloatingTile()->showComponentInRootPopup(nullptr, nullptr, Point<int>());
		return;
	}

	MainController* mc = getRootWindow()->getBackendProcessor();

	Component* c = nullptr;
	Component* button = nullptr;

	switch (t)
	{
	case MainTopBar::PopupType::About:
	{
		c = new AboutPage();
		
		c->setSize(500, 300);

		button = hiseButton;

		hiseButton->setToggleState(!hiseButton->getToggleState(), dontSendNotification);

		break;
	}
	case MainTopBar::PopupType::Macro:
	{
		c = new MacroComponent(getRootWindow());
		c->setSize(90 * 8, 74);

		button = macroButton;

		break;

	}
	case PopupType::Settings:
	{
		auto ft = new FloatingTile(mc, nullptr);

		ft->setContent(FloatingPanelTemplates::createSettingsWindow(mc));

		
		c = ft;

		c->setName("Settings");


		c->setSize(380, 610);

		button = settingsButton;
		break;
	}
	case PopupType::PluginPreview:
	{
		if (mc->getMainSynthChain()->hasDefinedFrontInterface())
		{
			auto ft = new FloatingTile(mc, nullptr); 
			
			ft->setNewContent(GET_PANEL_NAME(InterfaceContentPanel));

			auto content = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(mc)->getScriptingContent();

			if (content != nullptr)
			{
				auto scaleFactor = dynamic_cast<GlobalSettingManager*>(mc)->getGlobalScaleFactor();

				ft->setTransform(AffineTransform::scale(scaleFactor));

				ft->setSize(content->getContentWidth(), content->getContentHeight());

				c = new OwningComponent(ft);

				int w = (int)((float)content->getContentWidth()*scaleFactor);
				int h = (int)((float)content->getContentHeight()*scaleFactor);

				c->setSize(w, h);

				c->setName("Interface Preview");
			}


		}
		else
		{
			c = new InterfaceCreator();
			
		}

		



		button = pluginPreviewButton;
		break;
	}
	case PopupType::PresetBrowser:
	{
		MultiColumnPresetBrowser* pr = new MultiColumnPresetBrowser(mc, 700, 500);

		pr->setShowCloseButton(false);

		Colour c2 = Colours::black.withAlpha(0.8f);
		Colour c1 = Colour(SIGNAL_COLOUR);

		pr->setHighlightColourAndFont(c1, c2, GLOBAL_BOLD_FONT());

		c = dynamic_cast<Component*>(pr);

		button = presetBrowserButton;
		break;
	}
	case MainTopBar::PopupType::numPopupTypes:
		break;
	default:
		break;
	}

	Point<int> p(button->getLocalBounds().getCentreX(), button->getLocalBounds().getBottom());
	auto popup = getParentShell()->showComponentInRootPopup(c, button, p);

	if (popup != nullptr)
		popup->setColour((int)FloatingTilePopup::ColourIds::backgroundColourId, JUCE_LIVE_CONSTANT_OFF(Colour(0xec000000)));

}
