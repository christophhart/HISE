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


BackendProcessorEditor::BackendProcessorEditor(AudioProcessor *ownerProcessor, BackendRootWindow* parent, const ValueTree &editorState) :
owner(static_cast<BackendProcessor*>(ownerProcessor)),
parentRootWindow(parent),
rootEditorIsMainSynthChain(true)
{
    setOpaque(true);

	setLookAndFeel(&lookAndFeelV3);

	addAndMakeVisible(viewport = new CachedViewport());
	addAndMakeVisible(breadCrumbComponent = new BreadcrumbComponent());
	
	addChildComponent(debugLoggerWindow = new DebugLoggerComponent(&owner->getDebugLogger()));

	addAndMakeVisible(aboutPage = new AboutPage());
	
	viewport->viewport->setScrollBarThickness(SCROLLBAR_WIDTH);
	viewport->viewport->setSingleStepSizes(0, 6);

	setRootProcessor(owner->synthChain->getRootProcessor());

	owner->addScriptListener(this);

	aboutPage->setVisible(false);
	aboutPage->setBoundsInset(BorderSize<int>(80));

	restoreFromValueTree(editorState);
};

BackendProcessorEditor::~BackendProcessorEditor()
{
	owner->removeScriptListener(this);
	
    ValueTree v = exportAsValueTree();
    
	owner->setEditorState(v);

	// Remove the popup components

	popupEditor = nullptr;
	stupidRectangle = nullptr;
	
	aboutPage = nullptr;
	
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
	container = nullptr;

	viewport->viewport->setViewedComponent(container = new ProcessorEditorContainer());
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

void BackendProcessorEditor::setViewportPositions(int viewportX, const int viewportY, const int viewportWidth, int viewportHeight)
{
	debugLoggerWindow->setBounds(0, getHeight() - 60, getWidth(), 60);

	const int containerHeight = getHeight() - viewportY;

	viewport->setVisible(containerHeight > 0);
	viewport->setBounds(0, viewportY, getWidth(), containerHeight); // Overlap with the fade

	aboutPage->setBounds(viewportX, viewportY, viewportWidth, viewportHeight);

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
    //g.fillAll(Colour(BACKEND_BG_COLOUR));
    
    g.setColour(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourId));
    
    g.fillAll();
}


void BackendProcessorEditor::resized()
{
	const int breadcrumbHeight = rootEditorIsMainSynthChain ? 0 : 30;
	const int viewportY = 0; // 0 + 8 + 24 + 8;
	
	const int viewportHeight = getHeight();// -viewportY - (keyboard->isVisible() ? 0 : 10);
	
	const int viewportWidth = getWidth() - 16;
	
    const int bw = 16;

	int viewportX, poolX, inspectorX;
	int poolY, inspectorY;

	int heightOfSideColumns, sideColumnWidth;

	bool poolVisible, inspectorVisible;

	viewportX = 0;

	poolVisible = false;
	inspectorVisible = false;
	inspectorX = inspectorY = poolX = poolY = 0;

	sideColumnWidth = heightOfSideColumns = 0;

	//setToolBarPosition(viewportX, 4 , viewportWidth, 28);

	breadCrumbComponent->setBounds(viewportX, viewportY, viewportWidth, breadcrumbHeight);

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

	refreshInterfaceAfterPresetLoad();
	rebuildModuleList(false);	
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

	refreshInterfaceAfterPresetLoad();
	rebuildModuleList(false);
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

	rebuildModuleList(false);

	getBackendProcessor()->getMainSynthChain()->setBypassed(false);
}

void BackendProcessorEditor::clearModuleList()
{

	//dynamic_cast<PatchBrowser*>(propertyDebugArea->getComponentForIndex(PropertyDebugArea::ModuleBrowser))->clearCollections();
}

#undef toggleVisibility


#if PUT_FLOAT_IN_CODEBASE
BackendProcessorEditor* MainPanel::set(BackendProcessor* owner, BackendRootWindow* backendRoot, const ValueTree& editorState)
{
	addAndMakeVisible(ed = new BackendProcessorEditor(owner, backendRoot, editorState));

	getParentShell()->getParentContainer()->refreshLayout();

	return ed;
}
#endif

MainTopBar::MainTopBar(FloatingTile* parent) :
	FloatingTileContent(parent)
{
	addAndMakeVisible(tooltipBar = new TooltipBar());
	addAndMakeVisible(voiceCpuBpmComponent = new VoiceCpuBpmComponent(parent->getRootWindow()->getBackendProcessor()));
}
