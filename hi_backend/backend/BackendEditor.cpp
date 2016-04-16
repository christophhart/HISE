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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

#define toggleVisibility(x) {x->setVisible(!x->isVisible()); owner->setComponentShown(info.commandID, x->isVisible());}

#include <regex>


BackendProcessorEditor::BackendProcessorEditor(AudioProcessor *ownerProcessor, ValueTree &editorState) :
AudioProcessorEditor(ownerProcessor),
BackendCommandTarget(static_cast<BackendProcessor*>(ownerProcessor)),
owner(static_cast<BackendProcessor*>(getAudioProcessor())),
rootEditorIsMainSynthChain(true)
{
	setOpaque(true);

	setEditor(this);

	PresetHandler::buildProcessorDataBase(owner->getMainSynthChain());

	setLookAndFeel(&lookAndFeelV3);

	addAndMakeVisible(viewport = new CachedViewport());
	addAndMakeVisible(breadCrumbComponent = new BreadcrumbComponent());
	addAndMakeVisible(referenceDebugArea = new CombinedDebugArea(this));
	addAndMakeVisible(propertyDebugArea = new PropertyDebugArea(this));
	addAndMakeVisible(macroKnobs = new MacroComponent(owner, propertyDebugArea->getMacroTable()));
	addAndMakeVisible(mainToolbar = new Toolbar());
	addAndMakeVisible(keyboard = new CustomKeyboard(owner->getKeyboardState()));
	addAndMakeVisible(tooltipBar = new TooltipBar());
	addAndMakeVisible(cpuVoiceComponent = new VoiceCpuBpmComponent(owner));

	cpuVoiceComponent->setColour(Slider::backgroundColourId, Colour(BACKEND_BG_COLOUR));
	cpuVoiceComponent->setOpaque(true);

	addAndMakeVisible(aboutPage = new AboutPage());
	
	constrainer = new ComponentBoundsConstrainer();
	constrainer->setMinimumHeight(200);
	constrainer->setMinimumWidth(0);
	constrainer->setMaximumWidth(1920);

	addAndMakeVisible(borderDragger = new ResizableBorderComponent(this, constrainer));

	viewport->viewport->setScrollBarThickness(16);

	tooltipBar->addMouseListener(this, false);
	tooltipBar->setColour(TooltipBar::ColourIds::iconColour, Colours::lightgrey);
	toolbarFactory = new MainToolbarFactory(this);

	mainToolbar->setStyle(Toolbar::ToolbarItemStyle::iconsOnly);
	mainToolbar->addDefaultItems(*toolbarFactory);
	mainToolbar->setColour(Toolbar::ColourIds::backgroundColourId, Colours::transparentBlack);
	mainToolbar->setColour(Toolbar::ColourIds::buttonMouseOverBackgroundColourId, Colours::white.withAlpha(0.5f));
	mainToolbar->setColour(Toolbar::ColourIds::buttonMouseDownBackgroundColourId, Colours::white.withAlpha(0.7f));

	viewport->viewport->setSingleStepSizes(0, 6);

	setRootProcessor(owner->synthChain->getRootProcessor());

	BetterProcessorEditor *editor = container->getRootEditor();

	owner->addScriptListener(this);

	addAndMakeVisible(interfaceComponent = new ScriptContentContainer(owner->synthChain, dynamic_cast<ModulatorSynthChainBody*>(editor->getBody())));
	interfaceComponent->checkInterfaces();
	interfaceComponent->setIsFrontendContainer(true);
	interfaceComponent->setVisible(owner->isComponentShown(CustomInterface));
	interfaceComponent->setIsFrontendContainer(true);

	aboutPage->setVisible(false);
	aboutPage->setBoundsInset(BorderSize<int>(80));

	BorderSize<int> borderSize;
	borderSize.setTop(0);
	borderSize.setLeft(0);
	borderSize.setRight(0);
	borderSize.setBottom(7);

	borderDragger->setBorderThickness(borderSize);

#if IS_STANDALONE_APP 

	if(owner->callback->getCurrentProcessor() == nullptr)
	{
		showSettingsWindow();
	}

#endif

#if JUCE_MAC && IS_STANDALONE_APP
	MenuBarModel::setMacMainMenu(this);

#else

	addAndMakeVisible(menuBar = new MenuBarComponent(this));
	menuBar->setLookAndFeel(&plaf);

#endif

	restoreFromValueTree(editorState);

	//setSize(referenceDebugArea->isVisible() ? 1280 : 900, 700);
    
	keyboard->grabKeyboardFocus();
    
    updateCommands();
};

BackendProcessorEditor::~BackendProcessorEditor()
{
	owner->removeScriptListener(this);

    ValueTree v = exportAsValueTree();
    
	owner->setEditorState(v);

	clearModalComponent();

	keyboard = nullptr;
	currentDialog = nullptr;

	// Remove the popup components

	popupEditor = nullptr;
	stupidRectangle = nullptr;
	currentDialog = nullptr;
	aboutPage = nullptr;
	modalComponent = nullptr;

	// Remove the resize stuff

	constrainer = nullptr;
	borderDragger = nullptr;

	// Remove the toolbar stuf

	toolbarFactory = nullptr;
	mainToolbar = nullptr;
	tooltipBar = nullptr;

	cpuVoiceComponent = nullptr;
	backButton = nullptr;
	forwardButton = nullptr;
	breadCrumbComponent = nullptr;

	// Remove the menu

#if JUCE_MAC && IS_STANDALONE_APP
	MenuBarModel::setMacMainMenu(nullptr);
#else
	menuBar->setModel(nullptr);
	menuBar = nullptr;
#endif

	// Remove the debug panels

	referenceDebugArea = nullptr;
	propertyDebugArea = nullptr;

	// Remove the main stuff

	macroKnobs = nullptr;
	interfaceComponent = nullptr;
	container = nullptr;
	viewport = nullptr;
	
	
	
	
}

bool BackendProcessorEditor::addProcessorToPopupMenu(PopupMenu &m, Processor *p)
{
	Processor::Iterator<Processor> iter(p, false);

	Processor *ip;

	int i = 1;

	ModulatorSynth *lastSynth = nullptr;

	while((ip = iter.getNextProcessor()) != nullptr)
	{
		//if(ip->isBypassed()) continue;

		const bool isSynth = dynamic_cast<ModulatorSynth*>(ip) != nullptr;

		if(isSynth)
		{
			lastSynth = dynamic_cast<ModulatorSynth*>(ip);

			m.addCustomItem(-1, new ProcessorPopupItem(lastSynth, this));
		}
		else
		{
			const bool allowClick = dynamic_cast<Chain*>(ip) == nullptr;
									

			if(allowClick)
			{
				m.addCustomItem(i++, new ProcessorPopupItem(lastSynth, ip, this));
			}
		}
	}

	return true;
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






void BackendProcessorEditor::showSettingsWindow()
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

void BackendProcessorEditor::showViewPanelPopup()
{
	PopupMenu m;

	ViewInfo *currentView = owner->synthChain->getCurrentViewInfo();

	m.setLookAndFeel(&plaf);

	enum
	{
		AddNewView = -10,
		SaveCurrentView,
		DeleteCurrentView,
		RenameCurrentView,
		RemoveAllSoloProcessors,
		ShowAllHiddenProcessors
	};

	String changed = (currentView != nullptr && currentView->wasChanged()) ? "*" : "";

	m.addSectionHeader("Current View: " + ((currentView == nullptr) ? "None" : currentView->getViewName() + changed));
	m.addSeparator();

	m.addItem(AddNewView, "Add new view", true, false);
	m.addItem(SaveCurrentView, "Save current view", currentView != nullptr, false);
	m.addItem(RenameCurrentView, "Rename current view", currentView != nullptr, false);
	m.addItem(DeleteCurrentView, "Delete current view", currentView != nullptr, false);

	m.addSeparator();

	m.addItem(RemoveAllSoloProcessors, "Unsolo all processors");
	m.addItem(ShowAllHiddenProcessors, "Show all hidden processors");

	m.addSeparator();

	for (int i = 0; i < owner->synthChain->getNumViewInfos(); i++)
	{
		ViewInfo *info = owner->synthChain->getViewInfo(i);

		m.addItem(i + 1, info->getViewName(), true, info == owner->synthChain->getCurrentViewInfo());
	}

	int result = m.show();

	if (result == 0) return;


	if (result == AddNewView)
	{
		String view = PresetHandler::getCustomName("View");

		ViewInfo *info = new ViewInfo(owner->synthChain, currentRootProcessor, view);

		owner->synthChain->addViewInfo(info);
	}
	else if (result == DeleteCurrentView)
	{
		owner->synthChain->removeCurrentViewInfo();
	}
	else if (result == RenameCurrentView)
	{
		String view = PresetHandler::getCustomName("View");

		owner->synthChain->getCurrentViewInfo()->setViewName(view);
	}
	else if (result == SaveCurrentView)
	{
		String view = owner->synthChain->getCurrentViewInfo()->getViewName();

		ViewInfo *info = new ViewInfo(owner->synthChain, currentRootProcessor, view);

		owner->synthChain->replaceCurrentViewInfo(info);
	}
	else if (result == RemoveAllSoloProcessors)
	{
		container->clearSoloProcessors();
	}
	else if (result == ShowAllHiddenProcessors)
	{
		Processor::Iterator<Processor> iter(owner->synthChain);

		Processor *p;

		while ((p = iter.getNextProcessor()) != nullptr)
		{
			if (ProcessorHelpers::isHiddableProcessor(p))
			{
				p->setEditorState(Processor::Visible, true, sendNotification);
			}
		}

		container->refreshSize(false);
	}
	else
	{
		ViewInfo *info = owner->synthChain->getViewInfo(result - 1);

		info->restore();

		setRootProcessor(info->getRootProcessor());

		owner->synthChain->setCurrentViewInfo(result - 1);
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
		{
			addProcessorToPanel(iterProcessor);
		}
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
	container = nullptr;

	viewport->viewport->setViewedComponent(container = new BetterProcessorEditorContainer());
}

void BackendProcessorEditor::setRootProcessorWithUndo(Processor *p)
{
	owner->viewUndoManager->beginNewTransaction(getRootContainer()->getRootEditor()->getProcessor()->getId() + " -> " + p->getId());
	owner->viewUndoManager->perform(new ViewBrowsing(owner->synthChain, this, viewport->viewport->getViewPositionY(), p));
	updateCommands();
}

bool BackendProcessorEditor::getIndexPath(Array<int> &path, Processor *p, const int searchIndex, int &counter)
{
	if(searchIndex == counter) return true;

	if(p->isBypassed()) return false;

	for(int i = 0; i < p->getNumChildProcessors(); i++)
	{
		Processor *child = (p->getChildProcessor(i));
		counter++;

		if(getIndexPath(path, child, searchIndex, counter))
		{
			path.insert(0, i);
			return true;
		}
	}
	return false;
}

BetterProcessorEditor *BackendProcessorEditor::getProcessorEditorFromPath(const Array<int> &path)
{

	Processor *p = owner->synthChain;

	for(int i = 0; i < path.size(); i++)
	{

		jassert(p->getNumChildProcessors() >= path[i]);
		p = p->getChildProcessor(i);
	}

	return new BetterProcessorEditor(nullptr, 0, p, nullptr);
	
}

String BackendProcessorEditor::createStringFromPath(const Array<int> &path)
{

	String p;

	BetterProcessorEditor *pe = container->getRootEditor();

	for(int i = 0; i < path.size(); i++)
	{

		p << pe->getProcessor()->getId() << " :: ";

		pe = dynamic_cast<BetterProcessorEditor*>(pe->getPanel()->getChildEditor(path[i]));
	}

	p << pe->getProcessor()->getId();

	return p;
}


void BackendProcessorEditor::setToolBarPosition(int x, int y, int width, int height)
{
	const int toolbarWidth = mainToolbar->getNumItems() * 24;
	const int cpuVoiceWidth = cpuVoiceComponent->getWidth();

	tooltipBar->setBounds(x, y + 2, width - toolbarWidth - cpuVoiceWidth - 16, height - 4);

	cpuVoiceComponent->setBounds(tooltipBar->getRight() + 8, y, cpuVoiceWidth, height);

	mainToolbar->setBounds(x + width - toolbarWidth, y+2, toolbarWidth, height-4);
}

void BackendProcessorEditor::setViewportPositions(int viewportX, const int viewportY, const int viewportWidth, int viewportHeight)
{
	int y = viewportY;

	macroKnobs->setBounds(viewportX, y, viewportWidth, macroKnobs->getCurrentHeight());

	if (macroKnobs->isVisible()) y = macroKnobs->getBottom();

	const int interfaceWidth = interfaceComponent->getContentWidth();

	interfaceComponent->setBounds(viewportX, y, interfaceWidth != -1 ? interfaceWidth : viewportWidth, interfaceComponent->getContentHeight());

	if (interfaceComponent->isVisible()) y = interfaceComponent->getBottom();

	keyboard->setBounds(viewportX, getHeight() - 72, viewportWidth, 72);

	const int containerHeight = getHeight() - (keyboard->isVisible() ? keyboard->getHeight() : 0)
		- y;

	viewport->setBounds(viewportX, y, viewportWidth + 16, containerHeight); // Overlap with the fade

	aboutPage->setBounds(viewportX, viewportY, viewportWidth, viewportHeight);

	if (currentPopupComponent != nullptr)
	{
		jassert(stupidRectangle != nullptr);
		stupidRectangle->setBounds(viewport->getBounds());
		currentPopupComponent->setTopLeftPosition(currentPopupComponent->getX(), y + 40);
	}

	


}

bool BackendProcessorEditor::isPluginPreviewShown() const
{
	return previewWindow != nullptr;
}

bool BackendProcessorEditor::isPluginPreviewCreatable() const
{
	return interfaceComponent != nullptr && interfaceComponent->getContentWidth() != 0 && interfaceComponent->getContentHeight() != 0;
}

void BackendProcessorEditor::paint(Graphics &g)
{
	g.fillAll(Colour(BACKEND_BG_COLOUR));

    //ProcessorEditorLookAndFeel::drawNoiseBackground(g, getLocalBounds(), Colours::lightgrey);

	
    
	if(keyboard->isVisible()) g.fillRect(viewport->getX(), keyboard->getY() - 40, viewport->getWidth() - 16, 40);
	
	g.setColour (Colour (0x11000000));
    
}

void BackendProcessorEditor::resized()
{
#if IS_STANDALONE_APP

	if (getParentComponent() != nullptr)
	{
        getParentComponent()->getParentComponent()->setSize(getWidth(), getHeight());
	}
#endif

	const int menuBarOffset = menuBar == nullptr ? 0 : 20;

	if (menuBarOffset != 0)
	{
		menuBar->setBounds(0, 0, getWidth(), menuBarOffset);
	}

	const float dpiScale = Desktop::getInstance().getGlobalScaleFactor();

	int columns = 0;

	if ((float)getWidth()*dpiScale < 1279.0f)
	{
		columns = 1;
	}
	else if ((float)getWidth() * dpiScale <= 1600.0f)
	{
		columns = 2;
	}
	else
	{
		columns = 3;
	}

	const int breadcrumbHeight = rootEditorIsMainSynthChain ? 0 : 30;

	const int viewportY = menuBarOffset + 8 + 24 + 8 + breadcrumbHeight;
	const int viewportHeight = getHeight() - viewportY;
	
	const int viewportWidth = 868;
	
    const int bw = 16;

	int viewportX, poolX, inspectorX;
	int poolY, inspectorY;

	int heightOfSideColumns, sideColumnWidth;

	bool poolVisible, inspectorVisible;

	if (columns == 1)
	{
		viewportX = (getWidth() - viewportWidth) / 2;

		poolVisible = false;
		inspectorVisible = false;
		inspectorX = inspectorY = poolX = poolY = 0;

		sideColumnWidth = heightOfSideColumns = 0;
	}

	else if (columns == 2)
	{
		viewportX = getWidth() - viewportWidth - 16;

		

		sideColumnWidth = jmin<int>(400, viewportX - 2*bw);

		poolVisible = true;
		inspectorVisible = true;

		poolX = viewportX - sideColumnWidth - bw;

		inspectorX = poolX;
		
		poolY = menuBarOffset + 12;

		heightOfSideColumns = (getHeight() - poolY) / 2 - bw;

		inspectorY = poolY + heightOfSideColumns + bw;
	}
	else
	{
		viewportX = (getWidth() - viewportWidth) / 2;

		sideColumnWidth = jmin<int>(400, (getWidth() - viewportWidth) / 2 - 2*bw);

		poolVisible = true;
		inspectorVisible = true;

		poolY = menuBarOffset + 12; // viewportY - bw - breadcrumbHeight - ;
		poolX = viewportX - sideColumnWidth - bw;
		inspectorX = viewportX + viewportWidth + bw;
		inspectorY = poolY;

		heightOfSideColumns = getHeight() - poolY - bw;
	}

	setToolBarPosition(viewportX, menuBarOffset + 4 , viewportWidth, 28);

	breadCrumbComponent->setBounds(viewportX, viewportY - breadcrumbHeight, viewportWidth, breadcrumbHeight);

	setViewportPositions(viewportX, viewportY, viewportWidth, viewportHeight);

	viewport->viewport->setViewPosition(0, owner->getScrollY());

	//const int referenceDebugAreaHeight = (getHeight() - menuBarOffset) / 2 - 32;

	referenceDebugArea->setVisible(poolVisible);
	propertyDebugArea->setVisible(inspectorVisible);

    referenceDebugArea->setBounds(poolX, poolY, poolVisible ? sideColumnWidth : 0, heightOfSideColumns);

    propertyDebugArea->setBounds(inspectorX, inspectorY, inspectorVisible ? sideColumnWidth : 0, heightOfSideColumns);

    if(stupidRectangle != nullptr)
    {
        stupidRectangle->setBounds(viewportX, viewportY, viewportWidth, viewportHeight - keyboard->getHeight());
        currentPopupComponent->setBounds(viewportX, viewportY + 40, viewportWidth, viewportHeight-40 - keyboard->getHeight());
    }
    
#if IS_STANDALONE_APP

	if (currentDialog != nullptr)
	{
		currentDialog->centreWithSize(700, 500);
	}

#else
    
	borderDragger->setBounds(getBounds());

#endif
    
    
    

	

	//list->setBounds(viewport->getRight() + 16, 30, list->getWidth(), list->getHeight());
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
			dynamic_cast<SampleMapEditor*>(currentPopupComponent.get())->deletePopup();
			currentPopupComponent = nullptr;
		}
	}
	else
	{
		ownedPopupComponent = nullptr;
	}

	
	
	stupidRectangle = nullptr;
	viewport->setEnabled(true);
}

void BackendProcessorEditor::scriptWasCompiled(ScriptProcessor * /*sp*/)
{
	updateCommands();
}

ToolbarButton * BackendProcessorEditor::getButtonForID(int id) const
{
	for (int i = 0; i < mainToolbar->getNumItems(); i++)
	{
		if (mainToolbar->getItemId(i) == id)
		{
			return dynamic_cast<ToolbarButton*>(mainToolbar->getItemComponent(i));
		}
	}

	return nullptr;
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

	stupidRectangle->setBounds(viewport->getX(), viewport->getY(), viewport->getWidth() - 16, viewport->getHeight());

	addAndMakeVisible(componentToShow);



	componentToShow->setBounds(viewport->getX(), viewport->getY() + 40, viewport->getWidth()-16, componentToShow->getHeight());

	Desktop::getInstance().getAnimator().fadeOut(stupidRectangle, 1);
	Desktop::getInstance().getAnimator().fadeIn(stupidRectangle, 300);

	viewport->setEnabled(false);

	componentToShow->setAlwaysOnTop(true);
}

void BackendProcessorEditor::showModulatorTreePopup()
{
	PopupMenu soloView;

	soloView.setLookAndFeel(&plaf);

	addProcessorToPopupMenu(soloView, owner->synthChain);

	soloView.show();
}

void BackendProcessorEditor::showProcessorPopup(Processor *p, Processor *parent)
{
	popupEditor = new BetterProcessorEditor(nullptr, 1, p, nullptr);

	String pathString;

	if (parent != nullptr)
	{
		pathString << parent->getId() << " :: " << p->getId();
	}
	else
	{
		pathString << p->getId();
	}

	popupEditor->setIsPopup(true);

	showPseudoModalWindow(popupEditor, pathString);

	popupEditor->setBounds(viewport->getX(), viewport->getY()+40, viewport->getWidth()-16, viewport->getHeight()-40);

}

void BackendProcessorEditor::mouseDown(const MouseEvent &m)
{
	Rectangle<int> infoArea(tooltipBar->getX(), tooltipBar->getY(), tooltipBar->getHeight(), tooltipBar->getHeight());

	

	if (m.mods.isRightButtonDown())
	{
		PopupMenu m;

		m.setLookAndFeel(&plaf);

		enum
		{
			LoadPresetIntoMainSynthChain = 1,
			SaveCurrentMainSynthChain,
			ExportAsPackage,
			Recompile,
			MidiPanic,

		};

		m.addItem(LoadPresetIntoMainSynthChain, "Load preset into MainSynthChain");
		m.addItem(SaveCurrentMainSynthChain, "Save Current MainSynthChain");
		m.addItem(ExportAsPackage, "Export as package");
		m.addItem(Recompile, "Recompile all scripts");
		m.addItem(MidiPanic, "MIDI Panic");

		const int result = m.show();

		if (result == LoadPresetIntoMainSynthChain)
		{
			FileChooser fc("Load Preset File");

			if (fc.browseForFileToOpen())
			{
				File f = fc.getResult();

				loadNewContainer(f);

			}
		}
		else if (result == SaveCurrentMainSynthChain) PresetHandler::saveProcessorAsPreset(owner->getMainSynthChain());
		else if (result == ExportAsPackage) CompileExporter::exportMainSynthChainAsPackage(owner->getMainSynthChain());
		else if (result == Recompile) owner->compileAllScripts();
		else if (result == MidiPanic) owner->getMainSynthChain()->getMainController()->allNotesOff();
	}

	if (currentPopupComponent != nullptr || ownedPopupComponent != nullptr)
	{
		clearPopup();
		return;
	}
}

void BackendProcessorEditor::loadNewContainer(const File &f)
{
    getBackendProcessor()->suspendProcessing(true);

	clearPreset();

	f.setLastAccessTime(Time::getCurrentTime());

	if (f.getParentDirectory().getFileName() == "Presets")
	{
		GET_PROJECT_HANDLER(getMainSynthChain()).setWorkingProject(f.getParentDirectory().getParentDirectory());
	}

	getBackendProcessor()->getMainSynthChain()->setBypassed(true);



	clearModuleList();

	container = nullptr;

	owner->loadPreset(f, this);

	refreshInterfaceAfterPresetLoad();
	rebuildModuleList(false);	

    getBackendProcessor()->suspendProcessing(false);
}

void BackendProcessorEditor::refreshInterfaceAfterPresetLoad()
{
    Processor *p = static_cast<Processor*>(owner->synthChain);
    
	rebuildContainer();
    
    container->setRootProcessorEditor(p);
    
    BetterProcessorEditor *editor = container->getRootEditor();
    addAndMakeVisible(interfaceComponent = new ScriptContentContainer(owner->synthChain, dynamic_cast<ModulatorSynthChainBody*>(editor->getBody())));
    
    interfaceComponent->checkInterfaces();
    interfaceComponent->refreshContentBounds();
}

void BackendProcessorEditor::loadNewContainer(ValueTree &v)
{
    const int presetVersion = v.getProperty("BuildVersion", 0);
    
    if(presetVersion > BUILD_SUB_VERSION)
    {
        PresetHandler::showMessageWindow("Version mismatch", "The preset was built with a newer the build of HISE: " + String(presetVersion) + ". To ensure perfect compatibility, update to at least this build.");
    }
    
    getBackendProcessor()->suspendProcessing(true);
    
	clearPreset();

	getBackendProcessor()->getMainSynthChain()->setBypassed(true);

	clearModuleList();

	container = nullptr;

	owner->loadPreset(v, this);

	refreshInterfaceAfterPresetLoad();
	rebuildModuleList(false);

    getBackendProcessor()->suspendProcessing(false);
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

	BetterProcessorEditor *editor = container->getRootEditor();
	addAndMakeVisible(interfaceComponent = new ScriptContentContainer(owner->synthChain, dynamic_cast<ModulatorSynthChainBody*>(editor->getBody())));


    interfaceComponent->checkInterfaces();
    interfaceComponent->refreshContentBounds();
    
	rebuildModuleList(false);

	getBackendProcessor()->getMainSynthChain()->setBypassed(false);
}

void BackendProcessorEditor::clearModuleList()
{
	dynamic_cast<PatchBrowser*>(propertyDebugArea->getComponentForIndex(PropertyDebugArea::ModuleBrowser))->clearCollections();
}

#undef toggleVisibility