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

#define toggleVisibility(x) {x->setVisible(!x->isVisible()); owner->setComponentShown(info.commandID, x->isVisible());}

namespace hise { using namespace juce;

BackendProcessorEditor::BackendProcessorEditor(FloatingTile* parent) :
FloatingTileContent(parent),
PreloadListener(parent->getMainController()->getSampleManager()),
owner(static_cast<BackendProcessor*>(parent->getMainController())),
parentRootWindow(parent->getBackendRootWindow()),
rootEditorIsMainSynthChain(true),
isLoadingPreset(false)
{
    setOpaque(true);

	setLookAndFeel(&lookAndFeelV3);

	addAndMakeVisible(viewport = new CachedViewport());
	
	
	addChildComponent(debugLoggerWindow = new DebugLoggerComponent(&owner->getDebugLogger()));

	viewport->viewport->setScrollBarThickness(SCROLLBAR_WIDTH);
	viewport->viewport->setSingleStepSizes(0, 6);

	//setRootProcessor(owner->synthChain->getRootProcessor());

	owner->addScriptListener(this);

};

BackendProcessorEditor::~BackendProcessorEditor()
{
	setLookAndFeel(nullptr);
	owner->removeScriptListener(this);
	
	// Remove the popup components

	popupEditor = nullptr;
	
	// Remove the toolbar stuff

	breadCrumbComponent = nullptr;

	// Remove the main stuff

	container = nullptr;
	viewport = nullptr;
}

void BackendProcessorEditor::setRootProcessor(Processor *p, int scrollY/*=0*/)
{
	const bool rootEditorWasMainSynthChain = rootEditorIsMainSynthChain;

	rootEditorIsMainSynthChain = (p == owner->synthChain);


	if (p == nullptr) return;

	rebuildContainer();

	currentRootProcessor = p;
	container->setRootProcessorEditor(p);
	
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

void BackendProcessorEditor::newHisePresetLoaded()
{
	auto rw = GET_BACKEND_ROOT_WINDOW(this);

	if(auto jsp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(getBackendProcessor()))
	{
		BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(rw, jsp);
		BackendPanelHelpers::showWorkspace(rw, BackendPanelHelpers::Workspace::ScriptingWorkspace, sendNotification);
	}
}


void BackendProcessorEditor::preloadStateChanged(bool isPreloading)
{
	if (isLoadingPreset && !isPreloading)
	{
		isLoadingPreset = false;
		viewport->showPreloadMessage(false);
		refreshInterfaceAfterPresetLoad();
		parentRootWindow->sendRootContainerRebuildMessage(true);
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

	

	setViewportPositions(viewportX, viewportY + breadcrumbHeight, viewportWidth, viewportHeight);

	viewport->viewport->setViewPosition(0, owner->getScrollY());

	
    
}

void BackendProcessorEditor::clearPopup()
{
	
}

void BackendProcessorEditor::refreshContainer(Processor* selectedProcessor)
{
	if (container != nullptr)
	{
		const int y = viewport->viewport->getViewPositionY();

		setRootProcessor(container->getRootEditor()->getProcessor(), y);

		ProcessorEditor::Iterator iter(getRootContainer()->getRootEditor());

		while (ProcessorEditor *editor = iter.getNextEditor())
		{
			if (editor->getProcessor() == selectedProcessor)
			{
				editor->grabCopyAndPasteFocus();
			}
		}
	}
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

	addAndMakeVisible(componentToShow);

	componentToShow->setBounds(viewport->getX(), viewport->getY() + 40, viewport->getWidth()-SCROLLBAR_WIDTH, componentToShow->getHeight());

	
	componentToShow->setVisible(true);

	viewport->setEnabled(false);
	viewport->viewport->setScrollBarsShown(false, false);

	componentToShow->setAlwaysOnTop(true);
}

void BackendProcessorEditor::loadNewContainer(const File &f)
{
	clearModuleList();
	container = nullptr;

	isLoadingPreset = true;
	viewport->showPreloadMessage(true);
	

	f.setLastAccessTime(Time::getCurrentTime());

	if (f.getParentDirectory().getFileName() == "Presets")
	{
		GET_PROJECT_HANDLER(getMainSynthChain()).setWorkingProject(f.getParentDirectory().getParentDirectory());
	}

	owner->killAndCallOnLoadingThread([f](Processor* p) {p->getMainController()->loadPresetFromFile(f, nullptr); return SafeFunctionCall::OK; });
}

void BackendProcessorEditor::refreshInterfaceAfterPresetLoad()
{
    
}

void BackendProcessorEditor::loadNewContainer(const ValueTree &v)
{
	getRootWindow()->getRootFloatingTile()->showComponentInRootPopup(nullptr, nullptr, Point<int>());

    clearModuleList();
	container = nullptr;
	isLoadingPreset = true;
	viewport->showPreloadMessage(true);
	
	
	
	if (CompileExporter::isExportingFromCommandLine())
	{
		getRootWindow()->getMainSynthChain()->getMainController()->loadPresetFromValueTree(v, nullptr);
	}
	else
	{
		owner->killAndCallOnLoadingThread([v](Processor* p) {p->getMainController()->loadPresetFromValueTree(v, nullptr); return SafeFunctionCall::OK; });

	}

	
}



void BackendProcessorEditor::clearPreset()
{
	setPluginPreviewWindow(nullptr);

	clearModuleList();
    container = nullptr;
	isLoadingPreset = true;
	viewport->showPreloadMessage(true);

	auto rw = getRootWindow();

	rw->getRootFloatingTile()->showComponentInRootPopup(nullptr, nullptr, {});

	owner->killAndCallOnLoadingThread([rw](Processor* p) 
	{
		p->getMainController()->clearPreset(sendNotificationAsync); 
		dynamic_cast<BackendProcessor*>(p->getMainController())->createInterface(600, 500);

		return SafeFunctionCall::OK;
	});
}

void BackendProcessorEditor::clearModuleList()
{

	//dynamic_cast<PatchBrowser*>(propertyDebugArea->getComponentForIndex(PropertyDebugArea::ModuleBrowser))->clearCollections();
}

#undef toggleVisibility


MainTopBar::MainTopBar(FloatingTile* parent) :
	FloatingTileContent(parent),
	SimpleTimer(parent->getMainController()->getGlobalUIUpdater()),
	PreloadListener(parent->getMainController()->getSampleManager()),
	ComponentWithHelp(parent->getBackendRootWindow()),
	quickPlayButton(parent->getMainController())
{
	parent->getMainController()->getScriptComponentEditBroadcaster()->getLearnBroadcaster().addListener(*this, updateLearnConnection);

    {
        static const String iconData = "1231.nT6K8C1XOzhI.Xiuo5BLOQw..FZHKonCfUDaflA5ZgT2fXIOnsCvqXzRIkTRIR6KqEMqoZZW3Jrm4X+CHC.l..H.Oux8tFOdxKC2FOm8bR3BooKRTgCDZTULUICVnHwzkHMMpHvGRSlhtHUAENTY5AEoJZHhhzjFQanxzEKVHMYhHnfhJThvD8HZBGlZLYhTFbrPiJZRGtXQ4nhJSrVPiJCRQXlhrDMYgiAZTQSHgEVd33++ZPiZCTjlbwiHXr3vLCcDYpxJyrxjIiCGOr3xEINLnQYgxjJtPcqywoQQohjkoHLJ7SF7xLLFlyAZTWTgBYhjvbQRDoHIpHX14HfFkjKTHko.MpHXrD.t5Y6uo1IqZp+m62Vy6kIl26I+p5HdtqPugnoYaod3hOx7sKxZeriO9F5N5uBc80I+4xXb6aPatn57A.nMu2.TNDnQ0A0ENMpKSDMo9pWkJhJSQdfREJQDcQRRlGN3BPfvV1Ne4pp5HtLHt0KqwC11xpfCHZTT.fFkjKRXQS2qew64yEhqsOH.nuT14FO39Xi7siWrCmF0DpKlplHu+tdH3GBt398+7xli58KdI28gMd8cHZcesuLi2q+yNuP72KpKt10saokpiNl6022mkt8cdO6N1WxYtqm7gPX6ZmW27x9tap9ipuV53+My3g4p586h8c+A45+zbs9rzvy46Nndc8w6QH8AMiAhe9puZqoaOdtcJ1Gzd7ldJUDS.s6As6wWrw0Zrx2y6O9wJtdLWcs58K6tNAq5Kkwmm+ki3yuZ61iWm5yq2isfOedB79v9i+mDvPjIE.gADDF5y3WrC09paHrgKE2KWnEWctP56Vu4L1d4He4hOmwaKNgvas5cOGeKD8J3zf.71E53itluv6gM+5mzejuE+fYiHFSbcOK2N+KiBP5V0yc7BDHRnFKQvD.P.PBHAffBgUCLPmQzCHPxncQNk0LAyfEP.4e.crDLxWc0WF5xtPFRNdIN.YbvPxg.izdDQ0OFsF.8tW.dHDORrxg.xnII9jvLBOiG.5xHMxKzzW5eIAiQQ7QA.ZNIBVaFFC49JWE68dx3vmK3QlsG5Aj3bx2.sDjDxJZdlbSipcOtlpvZ9ynMyokW04FbbA4elRMTvGenL8E3CcC1rniW8.olNcQqZ0dUAKJL9hS.xVlOGNzeFmcIRRfWmI1G1N1Vyjn7gJEJ8p8wk9CB7Sba93wICBAYSY7dimsAEOtWJWZo79rvXEo9R05vezh1mgnofRMeLSOesGGWvDXiBfcwW2GUhFKkudo5a13pvfbuX6aqPrmRKOIEFvqBkJLRgMfEVBg4pEFi7hdvFSUq+fSEbuEIdO1pZoy9XNhbLXw0uyymhMUKY0XU6dItKXlqLRn9Vyi67m2g.25Xf9Zya2Rlisb4K.AwJVcVvJltxYOsENFgsjJC0NQBh2sQnn3yxAt36ygkRxPiTzxJ8nUkEVyInJQDQZTENGpx7FhTX.Q2CwoilgfdW0miH8zaJM.laTSj7gIvdAzXS0inQOXyAbwLM3qJ9L.w54NEkbcL7QWjAkEH6rxfWl17OfOChcRzDf94aoxO9ydFGMhmNV9Ot7chYIwjtd+o9Opj8qjTxtUd2Usjj+XtTv4uq0nF52r2SNUJqf6lN.";
        
        MemoryBlock mb;
        mb.fromBase64Encoding(iconData);
        zstd::ZDefaultCompressor comp;
        ValueTree v;
        comp.expand(mb, v);

        auto xml = v.createXml();

        hiseIcon = juce::Drawable::createFromSVG(*xml).release();
    }
    
	MainToolbarFactory f;

	setRepaintsOnMouseActivity(true);

	addAndMakeVisible(hiseButton = new ImageButton("HISE"));

	Image hise = ImageCache::getFromMemory(BinaryData::logo_mini_png, BinaryData::logo_mini_pngSize);
	

	hiseButton->setImages(false, true, true, hise, 0.9f, Colour(0), hise, 1.0f, Colours::white.withAlpha(0.1f), hise, 1.0f, Colours::white.withAlpha(0.1f), 0.1f);
	hiseButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuHelpShowAboutPage, true);

	addAndMakeVisible(backButton = new ShapeButton("Back", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.8f), Colours::white));
	backButton->setShape(f.createPath("back"), false, true, true);
	
	parent->getRootFloatingTile()->addPopupListener(this);

	addAndMakeVisible(forwardButton = new ShapeButton("Forward", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.8f), Colours::white));
	forwardButton->setShape(f.createPath("forward"), false, true, true);
	
	addAndMakeVisible(macroButton = new HiseShapeButton("Macro Controls", this, f));
	macroButton->setToggleModeWithColourChange(true);
	macroButton->setTooltip("Show 8 Macro Controls");
	

	addAndMakeVisible(presetBrowserButton = new ShapeButton("Preset Browser", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	presetBrowserButton->setTooltip("Show Preset Browser");
	presetBrowserButton->setShape(f.createPath("Preset Browser"), false, true, true);
	presetBrowserButton->addListener(this);

    addAndMakeVisible(customPopupButton = new ShapeButton("Custom Popup", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
    customPopupButton->setTooltip("Show Custom Popup");
    customPopupButton->setShape(f.createPath("Custom Popup"), false, true, true);
    customPopupButton->addListener(this);
    
    addAndMakeVisible(keyboardPopupButton = new ShapeButton("Keyboard", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
    keyboardPopupButton->setTooltip("Show Custom Popup");
    keyboardPopupButton->setShape(f.createPath("Keyboard"), false, true, true);
    keyboardPopupButton->addListener(this);
    
	addAndMakeVisible(pluginPreviewButton = new ShapeButton("Plugin Preview", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	pluginPreviewButton->setTooltip("Show Plugin Preview");
	pluginPreviewButton->setShape(f.createPath("Plugin Preview"), false, true, true);
	pluginPreviewButton->addListener(this);

	addAndMakeVisible(peakMeter = new ClickablePeakMeter(getRootWindow()->getMainSynthChain()));

	addAndMakeVisible(settingsButton = new ShapeButton("Audio Settings", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	settingsButton->setTooltip("Show Audio Settings");
	settingsButton->setShape(f.createPath("Settings"), false, true, true);
	settingsButton->addListener(this);

	addAndMakeVisible(layoutButton = new ShapeButton("Toggle Layout", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colour(SIGNAL_COLOUR)));
	layoutButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuViewEnableGlobalLayoutMode, true);
	
	Path layoutPath;
	layoutPath.loadPathFromData(ColumnIcons::layoutIcon, sizeof(ColumnIcons::layoutIcon));
	layoutButton->setShape(layoutPath, false, true, true);

	addAndMakeVisible(quickPlayButton);

	stop();

	if(getRootWindow()->getBackendProcessor()->isSnippetBrowser())
	{
		settingsButton->setVisible(false);
		layoutButton->setVisible(false);
	}

	//getRootWindow()->getBackendProcessor()->getCommandManager()->addListener(this);
}

MainTopBar::~MainTopBar()
{
	getParentShell()->getRootFloatingTile()->removePopupListener(this);

	//getRootWindow()->getBackendProcessor()->getCommandManager()->removeListener(this);
}

void setColoursForButton(ShapeButton* b, bool on)
{
	if(on)
		b->setColours(Colour(SIGNAL_COLOUR).withAlpha(0.95f), Colour(SIGNAL_COLOUR), Colour(SIGNAL_COLOUR));
	else
		b->setColours(Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white);
}




void MainTopBar::paint(Graphics& g)
{
    Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF383838));
	Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF404040));

	g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));
	g.fillAll();

	auto l = getWidth() - keyboardPopupButton->getBounds().getX();

	auto b = getLocalBounds().removeFromRight(l + 500);
	b = b.removeFromLeft(480);

	String infoText;

#if HISE_BACKEND_AS_FX
    infoText << "FX Build";
#endif
    
#if JUCE_DEBUG
	infoText << " (DEBUG)";
#endif

#if HISE_INCLUDE_FAUST

#if JUCE_DEBUG
	infoText << " with ";
#endif

	infoText << "Faust enabled";
#endif

#if PERFETTO
    infoText << " + Perfetto";
#endif

	if(getRootWindow()->getBackendProcessor()->isSnippetBrowser())
		infoText = "HISE Snippet Playground";

	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawText(infoText, b.toFloat(), Justification::right);
}



void MainTopBar::paintOverChildren(Graphics& g)
{

	ComponentWithHelp::paintHelp(g);

	if(currentlyLearnedScriptComponent.isNotEmpty())
	{
		static const unsigned char source[] = { 110,109,238,124,158,67,137,193,202,66,108,20,190,136,67,137,193,202,66,98,223,255,131,67,205,12,170,66,8,236,122,67,117,83,141,66,23,217,106,67,137,193,111,66,98,127,106,86,67,94,58,57,66,244,29,62,67,45,178,27,66,231,91,37,67,94,58,27,66,98,4,22,37,
67,88,57,27,66,33,208,36,67,88,57,27,66,127,138,36,67,94,58,27,66,98,29,218,214,66,217,78,28,66,39,177,82,66,137,129,162,66,156,68,36,66,244,189,12,67,98,2,43,4,66,190,223,53,67,115,104,59,66,219,153,98,67,100,59,155,66,244,13,128,67,98,84,99,213,66,
223,255,141,67,129,117,21,67,254,84,148,67,166,59,61,67,248,115,144,67,98,104,49,96,67,68,11,141,67,111,82,127,67,18,163,129,67,88,217,136,67,223,111,100,67,108,49,168,158,67,223,111,100,67,98,76,23,158,67,8,44,103,67,74,124,157,67,137,225,105,67,233,
214,156,67,92,143,108,67,98,80,189,145,67,14,77,141,67,188,244,118,67,174,151,158,67,121,169,68,67,92,127,163,67,98,240,7,17,67,82,136,168,67,186,9,178,66,180,104,160,67,25,4,75,66,184,14,142,67,98,190,159,194,65,111,98,129,67,70,182,211,64,90,100,96,
67,188,116,195,63,170,17,60,67,98,23,217,182,192,12,98,8,67,129,149,81,65,100,59,163,66,72,225,77,66,250,254,52,66,98,147,88,163,66,231,251,132,65,156,132,245,66,131,192,74,62,193,106,36,67,0,0,0,0,98,162,197,36,67,0,0,0,0,131,32,37,67,0,0,0,0,100,123,
37,67,0,0,0,0,98,236,17,88,67,143,194,117,62,254,228,132,67,219,249,198,65,115,72,148,67,143,2,131,66,98,221,116,152,67,74,12,153,66,152,222,155,67,100,59,177,66,238,124,158,67,137,193,202,66,99,109,43,103,37,67,141,55,67,67,98,193,138,29,67,141,55,67,
67,197,0,22,67,16,24,64,67,236,113,16,67,55,137,58,67,98,209,226,10,67,29,250,52,67,84,195,7,67,33,112,45,67,84,195,7,67,182,147,37,67,98,84,195,7,67,117,147,37,67,84,195,7,67,117,147,37,67,84,195,7,67,51,147,37,67,98,84,195,7,67,133,11,21,67,186,41,
21,67,31,165,7,67,104,177,37,67,31,165,7,67,98,254,244,103,67,31,165,7,67,113,157,192,67,31,165,7,67,113,157,192,67,31,165,7,67,108,113,157,192,67,240,167,160,66,108,80,77,1,68,86,110,37,67,108,113,157,192,67,180,136,122,67,108,113,157,192,67,141,55,
67,67,98,113,157,192,67,141,55,67,67,12,130,103,67,141,55,67,67,43,103,37,67,141,55,67,67,99,101,0,0 };

		Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0xEE383838));
		Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0xEE404040));

		g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));
		g.fillAll();

		Path p;
		p.loadPathFromData(source, sizeof(source));

		g.setColour(Colour(0xFFBBBBBB));
		g.setFont(GLOBAL_BOLD_FONT());

		String t;

		t << "Please click any knob with the connection overlay in order to assign it to the UI element " << currentlyLearnedScriptComponent;
		
		auto b = getLocalBounds().toFloat();
		g.drawText(t, b, Justification::centred);

		b = b.reduced(20, 0);

		g.setColour(Colour(SIGNAL_COLOUR));

		PathFactory::scalePath(p, b.removeFromRight(b.getHeight()).reduced(5));

		g.fillPath(p);

		PathFactory::scalePath(p, b.removeFromLeft(b.getHeight()).reduced(5));

		g.fillPath(p);

		return;
	}

	if (preloadState)
	{
		Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0xEE383838));
		Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0xEE404040));

		g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));
		g.fillAll();

		g.setColour(Colour(0xFFAAAAAA));
		g.setFont(GLOBAL_BOLD_FONT());
		
		auto b = getLocalBounds().toFloat().withSizeKeepingCentre(800.0f, (float)getHeight()).reduced(10.0f, 5.0f);

		

		g.drawText(preloadMessage, b.removeFromBottom(18.0f), Justification::centred);

		g.drawRoundedRectangle(b, b.getHeight() / 2.0f, 1.0f);
		b = b.reduced(2.0f);
		b.setWidth(jmax<float>(b.getHeight(), preloadProgress * b.getWidth()));
		g.fillRoundedRectangle(b, b.getHeight() / 2.0f);
		
	}
}

void MainTopBar::buttonClicked(Button* b)
{
	if (b == macroButton)
	{
		togglePopup(PopupType::Macro, b->getToggleState());
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
    else if (b == keyboardPopupButton)
    {
        togglePopup(PopupType::Keyboard, !b->getToggleState());
    }
    else if (b == customPopupButton)
    {
        togglePopup(PopupType::CustomPopup, !b->getToggleState());
    }
}

void MainTopBar::resized()
{
    hiseButton->setVisible(false);
	
    layoutButton->setVisible(false);
    
    auto bWidth = getHeight() * 2;
    
    frontendArea = getLocalBounds().withSizeKeepingCentre(bWidth * 3, getHeight());

    auto b = getLocalBounds();
    
    customPopupButton->setBounds(b.removeFromLeft(getHeight()).reduced(8));
                                 
    macroButton->setBounds(frontendArea.removeFromLeft(bWidth).reduced(7));
    pluginPreviewButton->setBounds(frontendArea.removeFromLeft(bWidth).reduced(7));
    presetBrowserButton->setBounds(frontendArea.removeFromLeft(bWidth).reduced(7));

	if(settingsButton->isVisible())
		settingsButton->setBounds(b.removeFromRight(b.getHeight()).reduced(7));

    peakMeter->setBounds(b.removeFromRight(180).reduced(8));

	quickPlayButton.setBounds(b.removeFromRight(b.getHeight()).reduced(8));
    keyboardPopupButton->setBounds(b.removeFromRight(b.getHeight()).reduced(8));
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

struct PopupFloatingTile: public Component,
						  public ButtonListener,
						  public PathFactory
{
    static constexpr int ButtonHeight = 24;
    
	PopupFloatingTile(MainController* mc, var data) :
		t(mc, nullptr),
		resizer(this, &constrainer),
		clearButton("clear", this, *this),
		loadButton("load", this, *this),
		saveButton("save", this, *this),
		layoutButton("layout", this, *this)
	{
		setOpaque(true);
		addAndMakeVisible(t);
		addAndMakeVisible(resizer);

		addAndMakeVisible(clearButton);
		addAndMakeVisible(loadButton);
		addAndMakeVisible(saveButton);
		addAndMakeVisible(layoutButton);

        constrainer.setMinimumSize(200, 80);
        
        if(data.isObject())
        {
            showEditBar = false;
            load(JSON::toString(data));
            
            
            
            t.setForceShowTitle(false);
            
            if(auto c = dynamic_cast<FloatingTileContainer*>(t.getCurrentFloatingPanel()))
            {
                if(c->getNumComponents() == 1)
                {
                    c->setIsDynamic(false);
                    c->getComponent(0)->setForceShowTitle(false);
                }
            }
            
            setName("Popup");
        }
        else
        {
            layoutButton.setToggleModeWithColourChange(true);
            clear();
            
            setName("Custom Popup");
            
            setSize(400, 400);
        }
	}

    void load(const String& jsonString)
    {
        auto data = JSON::parse(jsonString);
        int w = data.getProperty("Width", getWidth());
        int h = data.getProperty("Height", getHeight());

        setContent(jsonString);
        
        layoutButton.setToggleStateAndUpdateIcon(false);
        t.setLayoutModeEnabled(false);
        
        setSize(w, h - ButtonHeight);
    }
    
    void setContent(String c)
    {
        if(findParentComponentOfClass<BackendRootWindow>() == nullptr)
        {
            Timer::callAfterDelay(30, [this, c]()
            {
                this->setContent(c);
            });
                                  
            return;
        }
        
        t.loadFromJSON(c);
        setName(t.getCurrentFloatingPanel()->getBestTitle());
    }
    
	void clear()
	{
		t.setLayoutModeEnabled(true);
		t.setNewContent("HorizontalTile");
		layoutButton.setToggleStateAndUpdateIcon(true, true);
		t.setOpaque(true);
	}

	Path createPath(const String& url) const override
	{
		Path p;
		LOAD_EPATH_IF_URL("clear", SampleMapIcons::newSampleMap);
		LOAD_EPATH_IF_URL("load", SampleMapIcons::loadSampleMap);
		LOAD_EPATH_IF_URL("save", SampleMapIcons::saveSampleMap);
		LOAD_PATH_IF_URL("layout", ColumnIcons::customizeIcon);
		return p;
	}
    
    static void fillPopupWithFiles(PopupMenu& m)
    {
        auto files = getFileList();
        
        int index = 1;

        for (auto& f : files)
        {
            m.addItem(index++, f.getFileNameWithoutExtension());
        }
    }
    
    static Array<File> getFileList()
    {
        return getDirectory().findChildFiles(File::findFiles, false, "*.json");
    }

    
    
    static Component* loadWithPopupMenu(Component* c)
    {
        auto w = GET_BACKEND_ROOT_WINDOW(c);
        auto mc = w->getBackendProcessor();
        var dataToLoad;
		PopupLookAndFeel plaf;
        PopupMenu m;
        m.setLookAndFeel(&plaf);

        auto files = getFileList();
        
        
        
        fillPopupWithFiles(m);
        
        if(!files.isEmpty())
            m.addSeparator();
        
        m.addItem(9000, "Create new Popup");
        m.addItem(9001, "Show popup folder");
            
        auto r = m.showAt(c);
        if (r != 0)
        {
            if(r == 9000)
                return new PopupFloatingTile(mc, var());
            
            if(r == 9001)
            {
                PopupFloatingTile::getDirectory().revealToUser();
                return nullptr;
            }
            
            auto content = files[r - 1].loadFileAsString();

            return new PopupFloatingTile(mc, JSON::parse(content));
        }
        
        return nullptr;
    }
    
	void buttonClicked(Button* b) override
	{
		if (b == &clearButton)
		{
			clear();
		}
		if (b == &saveButton)
		{
			auto name = PresetHandler::getCustomName("PopupLayout");

			auto f = getDirectory().getChildFile(name).withFileExtension("json");

			auto v = JSON::parse(t.exportAsJSON());

			if (auto s = v.getDynamicObject())
			{
				s->setProperty("Width", getWidth());
				s->setProperty("Height", getHeight());
			}
			
			f.replaceWithText(JSON::toString(v));
		}
		if (b == &layoutButton)
		{
			t.setLayoutModeEnabled(layoutButton.getToggleState());
		}
		if (b == &loadButton)
		{
            PopupLookAndFeel plaf;
            PopupMenu m;
            m.setLookAndFeel(&plaf);
            
            fillPopupWithFiles(m);
            
            auto r = m.show();
            
            if(r != 0)
            {
                auto content = getFileList()[r - 1].loadFileAsString();
                load(content);
            }
		}
	}

	static File getDirectory()
	{
		auto dir = ProjectHandler::getAppDataDirectory(nullptr).getChildFile("custom_popups");

		if (!dir.isDirectory())
			dir.createDirectory();

		return dir;
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xFF222222));
	}

	void resized() override
	{
		auto b = getLocalBounds();

		
		static constexpr int ButtonMargin = 2;

		auto topRow = b.removeFromTop(showEditBar ? ButtonHeight : 0);

		clearButton.setBounds(topRow.removeFromLeft(ButtonHeight).reduced(ButtonMargin));
		loadButton.setBounds(topRow.removeFromLeft(ButtonHeight).reduced(ButtonMargin));
		saveButton.setBounds(topRow.removeFromLeft(ButtonHeight).reduced(ButtonMargin));
		layoutButton.setBounds(topRow.removeFromLeft(ButtonHeight).reduced(ButtonMargin));

		t.setBounds(b);
		resizer.setBounds(getLocalBounds().removeFromRight(8).removeFromBottom(8));
	}

	HiseShapeButton clearButton;
	HiseShapeButton loadButton;
	HiseShapeButton layoutButton;
	HiseShapeButton saveButton;

    bool showEditBar = true;
    
	FloatingTile t;
	juce::ResizableCornerComponent resizer;
	juce::ComponentBoundsConstrainer constrainer;
};

struct ToolkitPopup : public Component,
					  public ControlledObject,
					  public PooledUIUpdater::SimpleTimer,
					  public ButtonListener,
					  public PathFactory
{
	ToolkitPopup(MainController* mc):
		Component("HISE Controller"),
		ControlledObject(mc),
		SimpleTimer(mc->getGlobalUIUpdater()),
		panicButton("Panic", this, *this),
        sustainButton("pedal", this, *this),
        octaveUpButton("octave_up", this, *this),
        octaveDownButton("octave_down", this, *this),
		keyboard(mc),
        clockController(mc),
		resizer(this, &constrainer, ResizableEdgeComponent::rightEdge)
	{
		constrainer.setMinimumWidth(550);
		constrainer.setMaximumWidth(900);
		resizer.setLookAndFeel(&rlaf);

		addAndMakeVisible(resizer);
		addAndMakeVisible(panicButton);
		addAndMakeVisible(sustainButton);
		addAndMakeVisible(keyboard);
        addAndMakeVisible(octaveUpButton);
        addAndMakeVisible(octaveDownButton);

		panicButton.setTooltip("Send All-Note-Off message.");
        sustainButton.setTooltip("Enable Toggle mode (sustain) for keyboard.");
        sustainButton.setToggleModeWithColourChange(true);
        
		keyboard.setUseVectorGraphics(true);
        keyboard.setRange(36, 127);
		keyboard.setShowOctaveNumber(true);

        addAndMakeVisible(clockController);
        
		setSize(650, 83 + 72 + 10);
	}

	void buttonClicked(Button* b) override
	{
        if(b == &sustainButton)
        {
            keyboard.setEnableToggleMode(b->getToggleState());
            
            if(!b->getToggleState())
                getMainController()->allNotesOff(true);
        }
		if (b == &panicButton)
			getMainController()->allNotesOff(true);
        if(b == &octaveUpButton || b == &octaveDownButton)
        {
            auto delta = b == &octaveUpButton ? 12 : -12;
            
            auto l = keyboard.getRangeStart() + delta;
            auto h = jmin(127, keyboard.getRangeEnd() + delta);
            
            if(l > 0 && l <= 64)
                keyboard.setRange(l, h);
        }
	}

	void paint(Graphics& g) override
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());

		auto statBounds = getLocalBounds();
        statBounds.removeFromRight(10);
		statBounds = statBounds.removeFromTop(30);
        statBounds = statBounds.removeFromRight(250);
        
		
		
		g.drawText(getStatistics(), statBounds.toFloat(), Justification::centredRight);

		g.setColour(Colours::white.withAlpha(0.2f));
		g.fillPath(midiPath);

		if (midiAlpha != 0.0f)
		{
			g.setColour(Colour(SIGNAL_COLOUR).withAlpha(midiAlpha));
			g.fillPath(midiPath);
		}
	}

	void resized() override
	{
        static constexpr int TopHeight = 28;
        static constexpr int Margin = 2;
        
		auto b = getLocalBounds();
		b.removeFromLeft(10);
		resizer.setBounds(b.removeFromRight(10));
        
        clockController.setBounds(b.removeFromTop(83));
        
        b.removeFromTop(10);
        
        auto r = b.removeFromLeft(TopHeight);
        
        b.removeFromLeft(10);
        
        sustainButton.setBounds(r.removeFromBottom(TopHeight).reduced(Margin));
        panicButton.setBounds(r.removeFromTop(TopHeight).reduced(Margin));
        
        auto oct = b.removeFromRight(TopHeight);
        
        octaveUpButton.setBounds(oct.removeFromTop(TopHeight).reduced(Margin));
        octaveDownButton.setBounds(oct.removeFromBottom(TopHeight).reduced(Margin));
        
        keyboard.setBounds(b);

        midiPath = createPath("midi");
        scalePath(midiPath, r.removeFromTop(TopHeight).reduced(Margin).toFloat());
	}

	void timerCallback() override
	{
		if (getMainController()->checkAndResetMidiInputFlag())
			midiAlpha = 1.0f;
		else
			midiAlpha = jmax(0.0f, midiAlpha - 0.1f);

		repaint();
	}

	Path createPath(const String& url) const override
	{
		Path p;

		LOAD_EPATH_IF_URL("Panic", HiBinaryData::FrontendBinaryData::panicButtonShape);
		LOAD_EPATH_IF_URL("midi", HiBinaryData::SpecialSymbols::midiData);
        LOAD_EPATH_IF_URL("pedal", BackendBinaryData::PopupSymbols::sustainIcon);
        LOAD_EPATH_IF_URL("octave_up", BackendBinaryData::PopupSymbols::octaveUpIcon);
        LOAD_EPATH_IF_URL("octave_down", BackendBinaryData::PopupSymbols::octaveDownIcon);

		return p;
	}

	String getStatistics() const
	{
		auto mc = getMainController();
		const int cpuUsage = (int)mc->getCpuUsage();
		const int voiceAmount = mc->getNumActiveVoices();

		auto bytes = mc->getSampleManager().getModulatorSamplerSoundPool2()->getMemoryUsageForAllSamples();

		auto& handler = getMainController()->getExpansionHandler();

		for (int i = 0; i < handler.getNumExpansions(); i++)
			bytes += handler.getExpansion(i)->pool->getSamplePool()->getMemoryUsageForAllSamples();

		const double ramUsage = (double)bytes / 1024.0 / 1024.0;

		String stats = "CPU: ";
		stats << String(cpuUsage) << "%, RAM: " << String(ramUsage, 1) << "MB , Voices: " << String(voiceAmount);
		return stats;
	}

	Path midiPath;
	float midiAlpha = 0.0f;
    HiseShapeButton panicButton, sustainButton, octaveUpButton, octaveDownButton;
	
	CustomKeyboard keyboard;

	juce::ResizableEdgeComponent resizer;
	ComponentBoundsConstrainer constrainer;
	ScrollbarFader::Laf rlaf;
    
    hise::DAWClockController clockController;
};


void MainTopBar::timerCallback()
{
	auto newProgress = getMainController()->getSampleManager().getPreloadProgressConst();
	auto m = getMainController()->getSampleManager().getPreloadMessage();
	auto state = preloadState;

	if (newProgress != preloadProgress ||
		m != preloadMessage ||
		state != preloadState)
	{
		preloadState = state;
		preloadMessage = m;
		preloadProgress = newProgress;
		repaint();
	}
}

void MainTopBar::popupChanged(Component* newComponent)
{
    bool macroShouldBeOn = dynamic_cast<MacroComponent*>(newComponent) != nullptr;
    bool settingsShouldBeOn = (newComponent != nullptr && newComponent->getName() == "Settings");
    bool previewShouldBeShown = (newComponent != nullptr && newComponent->getName() == "Interface Preview") ||
                                (newComponent != nullptr && newComponent->getName() == "Create User Interface");
    bool presetBrowserShown = dynamic_cast<PresetBrowser*>(newComponent) != nullptr;

    bool keyboardShouldBeOn = dynamic_cast<ToolkitPopup*>(newComponent) != nullptr;
    
    bool customShouldBeShown = dynamic_cast<PopupFloatingTile*>(newComponent) != nullptr;
    
    setColoursForButton(macroButton, macroShouldBeOn);
    setColoursForButton(settingsButton, settingsShouldBeOn);
    setColoursForButton(pluginPreviewButton, previewShouldBeShown);
    setColoursForButton(presetBrowserButton, presetBrowserShown);
    setColoursForButton(keyboardPopupButton, keyboardShouldBeOn);
    setColoursForButton(customPopupButton, customShouldBeShown);
    
    macroButton->setToggleState(macroShouldBeOn, dontSendNotification);
    settingsButton->setToggleState(settingsShouldBeOn, dontSendNotification);
    pluginPreviewButton->setToggleState(previewShouldBeShown, dontSendNotification);
    presetBrowserButton->setToggleState(presetBrowserShown, dontSendNotification);
    keyboardPopupButton->setToggleState(keyboardShouldBeOn, dontSendNotification);
    customPopupButton->setToggleState(customShouldBeShown, dontSendNotification);
}

void MainTopBar::preloadStateChanged(bool isPreloading)
{
	preloadState = isPreloading;

	if (isPreloading)
		start();
	else
	{
		timerCallback();
		stop();
	}
        
	repaint();
}


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
		c->setSize(90 * HISE_NUM_MACROS, 74);

		button = macroButton;

		break;

	}
	case MainTopBar::PopupType::PeakMeter:
	{
		c = new ClickablePeakMeter::PopupComponent(peakMeter);
		button = peakMeter;
		break;
	}
	case PopupType::Settings:
	{
		c = nullptr;

		BackendCommandTarget::Actions::showFileProjectSettings(GET_BACKEND_ROOT_WINDOW(this));

		button = settingsButton;

		settingsButton->setToggleState(!settingsButton->getToggleState(), dontSendNotification);

		break;
	}
	case PopupType::PluginPreview:
	{
		if (mc->getMainSynthChain()->hasDefinedFrontInterface())
		{
			auto ft = new FloatingTile(mc, nullptr); 
			
			ft->setNewContent(GET_PANEL_NAME(InterfaceContentPanel));

            ft->setOpaque(false);
            
			auto content = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(mc)->getScriptingContent();

			if (content != nullptr)
			{
				auto scaleFactor = dynamic_cast<GlobalSettingManager*>(mc)->getGlobalScaleFactor();

				ft->setTransform(AffineTransform::scale(scaleFactor));

				ft->setSize(content->getContentWidth(), content->getContentHeight());

				c = new OwningComponent(ft);

				int w = (int)((float)content->getContentWidth()*scaleFactor);
				int h = (int)((float)content->getContentHeight()*scaleFactor);

				c->setName("Interface Preview");

				content->interfaceSizeBroadcaster.addListener(*c, [mc](Component& oc, int w, int h)
				{
					auto scaleFactor = dynamic_cast<GlobalSettingManager*>(mc)->getGlobalScaleFactor();

					oc.getChildComponent(0)->setSize(w, h);

					w = roundToInt((float)w * scaleFactor);
					h = roundToInt((float)h * scaleFactor);

					oc.setSize(w, h);
					oc.resized();
				});
				
				
			}


		}
		else
		{
            PresetHandler::showMessageWindow("No interface defined", "Please create a script processor in the master container and call `Content.makeFrontInterface(x, y);` in order to create a interface module");
		}

		



		button = pluginPreviewButton;
		break;
	}
	case PopupType::PresetBrowser:
	{
		PresetBrowser* pr = new PresetBrowser(mc, 700, 450);

		PresetBrowser::Options newOptions;

		auto expEnabled = getMainController()->getExpansionHandler().isEnabled();

		newOptions.showExpansions = expEnabled;
		newOptions.numColumns = expEnabled ? 2 : 3;
		newOptions.showFavoriteIcons = false;
		newOptions.showNotesLabel = false;
		newOptions.showFolderButton = false;
		newOptions.highlightColour = Colour(SIGNAL_COLOUR);
        newOptions.backgroundColour = Colours::transparentBlack;
		newOptions.textColour = Colours::white;
		newOptions.font = GLOBAL_BOLD_FONT();

		pr->setOptions(newOptions);

		c = dynamic_cast<Component*>(pr);

		button = presetBrowserButton;
		break;
	}
        case MainTopBar::PopupType::CustomPopup:
        c = PopupFloatingTile::loadWithPopupMenu(customPopupButton);
        button = customPopupButton;
        break;
    case MainTopBar::PopupType::Keyboard:
        c = new ToolkitPopup(mc);
        button = keyboardPopupButton;
        break;
	case MainTopBar::PopupType::numPopupTypes:
		break;
	default:
		break;
	}

	Point<int> point(button->getLocalBounds().getCentreX(), button->getLocalBounds().getBottom());
	auto popup = getParentShell()->showComponentInRootPopup(c, button, point);

    auto sb = dynamic_cast<ShapeButton*>(button);
     
    if(popup != nullptr && sb != nullptr)
    {
        popup->onDetach = [this, sb](bool isDetached)
        {
            setColoursForButton(sb, !isDetached);
            sb->setToggleState(!isDetached, dontSendNotification);
        };
    }

}

void MainTopBar::applicationCommandInvoked(const ApplicationCommandTarget::InvocationInfo& info)
{

	
}

MainTopBar::QuickPlayComponent::QuickPlayComponent(MainController* mc):
	ControlledObject(mc),
	SimpleTimer(mc->getGlobalUIUpdater())
{
	setTooltip("Play a note or start the transport with a single click. Right click to adjust the settings.");

	play[0].loadPathFromData(MainToolbarIcons::quickplay, MainToolbarIcons::quickplay_Size);
	note[0].loadPathFromData(MainToolbarIcons::quicknote, MainToolbarIcons::quicknote_Size);
	play[1].loadPathFromData(MainToolbarIcons::quickplay, MainToolbarIcons::quickplay_Size);
	note[1].loadPathFromData(MainToolbarIcons::quicknote, MainToolbarIcons::quicknote_Size);

	setRepaintsOnMouseActivity(true);
	stop();
}

void MainTopBar::QuickPlayComponent::setValue(bool shouldBeOn)
{
	currentValue = shouldBeOn;

	if(playNote)
	{
		MidiMessage event;

		if(shouldBeOn)
			getMainController()->getKeyboardState().noteOn(1, noteToPlay, 1.0f);
					
		else
			getMainController()->getKeyboardState().noteOff(1, noteToPlay, 0.0f);
	}
	else
	{
		auto& clockSim = dynamic_cast<BackendProcessor*>(getMainController())->externalClockSim;

		if(shouldBeOn)
			startPos = clockSim.ppqPos;
		else
			clockSim.ppqPos = startPos;
				
		clockSim.isPlaying = shouldBeOn;
	}

	repaint();
}

void MainTopBar::QuickPlayComponent::timerCallback()
{
	currentValue = dynamic_cast<BackendProcessor*>(getMainController())->externalClockSim.isPlaying;

	if(currentValue)
	{
		playNote = false;
		repaint();
	}
}

void MainTopBar::QuickPlayComponent::paint(Graphics& g)
{
	auto alpha = currentValue ? 0.8f : 0.6f;

	if(!currentValue && isMouseOver())
		alpha += 0.2f;

	if(isMouseButtonDown())
		alpha += 0.2f;

	auto c = Colour(currentValue ? SIGNAL_COLOUR : 0xFFFFFFFF).withAlpha(alpha);

	g.setColour(c);
	g.fillPath(playNote ? note[isMouseButtonDown()] : play[isMouseButtonDown()]);
}

void MainTopBar::QuickPlayComponent::mouseUp(const MouseEvent& e)
{
	if(e.mods.isRightButtonDown())
		return;

	if(!toggle)
		setValue(false);
}

void MainTopBar::QuickPlayComponent::setShouldPlayNote(bool v)
{
	playNote = v;

	if(v)
		stop();
	else
		start();
}

void MainTopBar::QuickPlayComponent::mouseDown(const MouseEvent& e)
{
	if(e.mods.isRightButtonDown())
	{
		PopupMenu m;
		PopupLookAndFeel plaf;
		m.setLookAndFeel(&plaf);

		m.addSectionHeader("Quickplay Settings");
		m.addItem(1, "Play MIDI note", true, playNote);
		m.addItem(2, "Control DAW playback simulator", true, !playNote);
		m.addSeparator();
		m.addItem(3, "Toggle Mode (Sustain)", true, toggle);

		PopupMenu noteMenu;

		int NoteOffset = 900;

		for(int i = 0; i < 127; i++)
		{
			String n;
			n << MidiMessage::getMidiNoteName(i, true, true, 3);
			n << " (" << String(i) << ")";
			noteMenu.addItem(i+NoteOffset, n, true, i == noteToPlay);
		}

		m.addSubMenu("Note to play", noteMenu);

		auto result = m.show();

		if(result == 1)
			playNote = true;
		else if(result == 2)
			playNote = false;
		else if(result == 3)
			toggle = !toggle;
		else if (result >= NoteOffset)
		{
			noteToPlay = result - NoteOffset;
		}

		repaint();

		return;
	}

	if(toggle)
	{
		toggleState = !toggleState;
		setValue(toggleState);
	}
	else
	{
		setValue(true);
	}
}

void MainTopBar::QuickPlayComponent::resized()
{
	auto b = getLocalBounds().toFloat();
	PathFactory::scalePath(play[0], b.reduced(4));
	PathFactory::scalePath(play[1], b.reduced(5));
	PathFactory::scalePath(note[0], b.reduced(1));
	PathFactory::scalePath(note[1], b.reduced(2));
}

void BackendHelpers::callIfNotInRootContainer(std::function<void(void)> func, Component* c)
{
	auto container = c->findParentComponentOfClass<ProcessorEditorContainer>();

	if (container == nullptr)
	{
		func();
	}
}

} // namespace hise
