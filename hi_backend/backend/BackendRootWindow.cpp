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

#define DECLARE_FOLD_ID(x) static const Identifier x(#x);

namespace fold_ids
{
	DECLARE_FOLD_ID(editor);
	DECLARE_FOLD_ID(watch);
	DECLARE_FOLD_ID(map);
	DECLARE_FOLD_ID(console);
	DECLARE_FOLD_ID(interface);
	DECLARE_FOLD_ID(list);
	DECLARE_FOLD_ID(properties);
	DECLARE_FOLD_ID(browser);
}

#undef DECLARE_FOLD_ID

void SuspendedOverlay::paint(Graphics& g)
{
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xEE222222)));
	g.setColour(Colours::white.withAlpha(0.8f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("This instance is currently suspended. Click to activate the audio rendering for this instance...", getLocalBounds().toFloat(), Justification::centred);
}

void SuspendedOverlay::mouseDown(const MouseEvent& event)
{
	auto bpe = findParentComponentOfClass<BackendRootWindow>();
	bpe->setCurrentlyActiveProcessor();
}

std::map<Identifier, bool> SnippetBrowserHelpers::getFoldConfiguration(Category c)
{
	std::map<Identifier, bool> map;

	switch(c)
	{
	case Category::Undefined: break;
	case Category::Modules:
		map[fold_ids::editor] = false;
		map[fold_ids::console] = true;
		map[fold_ids::interface] = true;
		map[fold_ids::list] = true;
		map[fold_ids::properties] = true;
		map[fold_ids::browser] = false;
		break;
	case Category::MIDI:
		map[fold_ids::editor] = false;
		map[fold_ids::console] = false;
		map[fold_ids::interface] = true;
		map[fold_ids::list] = true;
		map[fold_ids::properties] = true;
		map[fold_ids::browser] = false;
		break;
	case Category::ScriptingApi:
		map[fold_ids::editor] = false;
		map[fold_ids::console] = false;
		map[fold_ids::interface] = true;
		map[fold_ids::list] = true;
		map[fold_ids::properties] = true;
		map[fold_ids::browser] = true;
		break;
	case Category::Scriptnode:
		map[fold_ids::editor] = true;
		map[fold_ids::console] = false;
		map[fold_ids::interface] = false;
		map[fold_ids::list] = true;
		map[fold_ids::properties] = true;
		map[fold_ids::browser] = true;
		break;
	case Category::UI:
		map[fold_ids::editor] = false;
		map[fold_ids::console] = false;
		map[fold_ids::interface] = true;
		map[fold_ids::list] = true;
		map[fold_ids::properties] = true;
		map[fold_ids::browser] = true;
		break;
	case Category::numCategories:
		map[fold_ids::editor] = true;
		map[fold_ids::console] = true;
		map[fold_ids::interface] = true;
		map[fold_ids::list] = true;
		map[fold_ids::properties] = true;
		map[fold_ids::browser] = true;
		break;
	default: ;
	}

	return map;
}

StringArray SnippetBrowserHelpers::getCategoryNames()
{
	return { "All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI" };
}


	TextLayout BackendRootWindow::TooltipLookAndFeel::layoutTooltipText(const String& text, Colour colour) noexcept
	{
		const int maxToolTipWidth = 400;

		AttributedString s;
		s.setJustification(Justification::centred);
		s.append(text, GLOBAL_BOLD_FONT(), colour);

		TextLayout tl;
		tl.createLayoutWithBalancedLineLengths(s, (float)maxToolTipWidth);
		return tl;
	}

	Rectangle<int> BackendRootWindow::TooltipLookAndFeel::getTooltipBounds(const String& tipText, Point<int> screenPos,
	                                                                       Rectangle<int> parentArea)
	{
		const TextLayout tl(layoutTooltipText(tipText, Colours::black));

		auto w = (int)(tl.getWidth() + 14.0f);
		auto h = (int)(tl.getHeight() + 8.0f);

		auto c = dynamic_cast<TooltipClient*>(Desktop::getInstance().getMainMouseSource().getComponentUnderMouse());

		if(c != nullptr && c->getTooltip() == tipText)
		{
			auto screenBounds = dynamic_cast<Component*>(c)->getScreenBounds();
			return screenBounds.withWidth(w).withHeight(h).translated(0, jmax(screenBounds.getHeight(), h * 3 / 2)).constrainedWithin(parentArea.reduced(10));
		}

		

		return Rectangle<int>(screenPos.x > parentArea.getCentreX() ? screenPos.x - (w + 12) : screenPos.x + 24,
		                      screenPos.y > (parentArea.getBottom() - 90) ? screenPos.y - (h + 9) : screenPos.y + 9,
		                      w, h)
			.constrainedWithin(parentArea);
	}

	void BackendRootWindow::TooltipLookAndFeel::drawTooltip(Graphics& g, const String& text, int width, int height)
	{
		Rectangle<int> bounds(width, height);
		auto cornerSize = 3.0f;

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xffe0e0e0)));
		g.fillRoundedRectangle(bounds.toFloat(), cornerSize);
	
		layoutTooltipText(text, JUCE_LIVE_CONSTANT_OFF(Colour(0xff313131)))
			.draw(g, { static_cast<float> (width), static_cast<float> (height) });
			
	}

	String BackendRootWindow::TooltipWindowWithoutScriptContent::getTipFor(Component& component)
	{
		if (component.findParentComponentOfClass<ScriptContentComponent>())
			return {};

		return TooltipWindow::getTipFor(component);
	}

	BackendRootWindow::BackendRootWindow(AudioProcessor *ownerProcessor, var editorState) :
	AudioProcessorEditor(ownerProcessor),
	BackendCommandTarget(static_cast<BackendProcessor*>(ownerProcessor)),
	owner(static_cast<BackendProcessor*>(ownerProcessor))
{
	funkytooltips.setLookAndFeel(&ttlaf);

	PresetHandler::buildProcessorDataBase(owner->getMainSynthChain());

	Desktop::getInstance().setDefaultLookAndFeel(&globalLookAndFeel);

	addAndMakeVisible(floatingRoot = new FloatingTile(owner, nullptr));

	dynamic_cast<MainController*>(ownerProcessor)->addScriptListener(this, false);

	loadKeyPressMap();

	allWindowsAndBrowsers.add(this);

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

            
            mainEditor = FloatingTileHelpers::findTileWithId<BackendProcessorEditor>(floatingRoot, {});

			loadedCorrectly = mainEditor != nullptr;
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

		workspaces.add(FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("ScriptingWorkspace"))->getParentShell());
		workspaces.add(FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("SamplerWorkspace"))->getParentShell());
		workspaces.add(FloatingTileHelpers::findTileWithId<HorizontalTile>(floatingRoot, Identifier("CustomWorkspace"))->getParentShell());

		showWorkspace(BackendCommandTarget::WorkspaceScript);

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

	auto consoleParent = FloatingTileHelpers::findTileWithId<ConsolePanel>(getRootFloatingTile(), {});

	if (consoleParent != nullptr)
		getBackendProcessor()->getConsoleHandler().setMainConsole(consoleParent->getConsole());
	else
		jassertfalse;

	setOpaque(true);

#if IS_STANDALONE_APP && !HISE_HEADLESS && !IS_MARKDOWN_EDITOR

	if (owner->callback->getCurrentProcessor() == nullptr &&  !CompileExporter::isExportingFromCommandLine())
		showSettingsWindow();

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

	setOpaque(true);

	startTimer(1000);

	updateCommands();

	auto useOpenGL = GET_HISE_SETTING(getMainSynthChain(), HiseSettings::Other::UseOpenGL).toString() == "1";

	if (useOpenGL)
		setEnableOpenGL(this);
    
	if (GET_HISE_SETTING(getBackendProcessor()->getMainSynthChain(), HiseSettings::Other::AutoShowWorkspace))
	{
		auto jsp = ProcessorHelpers::getFirstProcessorWithType<JavascriptMidiProcessor>(getBackendProcessor()->getMainSynthChain());

		BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(this, jsp);
		BackendPanelHelpers::showWorkspace(this, BackendPanelHelpers::Workspace::ScriptingWorkspace, sendNotification);
	}

	getBackendProcessor()->workbenches.addListener(this);

    GET_PROJECT_HANDLER(getBackendProcessor()->getMainSynthChain()).addListener(this, true);
            
	getBackendProcessor()->getScriptComponentEditBroadcaster()->getLearnBroadcaster().addListener(*this, BackendRootWindow::learnModeChanged);

	getMainController()->getLockFreeDispatcher().addPresetLoadListener(this);

	getBackendProcessor()->workspaceBroadcaster.addListener(*this, [](BackendRootWindow& w, const Identifier& id, Processor* p)
	{
		w.currentWorkspaceProcessor = p;

		if (id == JavascriptProcessor::getConnectorId())
		{
			SafeAsyncCall::call<BackendRootWindow>(w, [](BackendRootWindow& w2)
			{
				w2.addEditorTabsOfType<CodeEditorPanel>();
			});
		}
	}, true);
}


BackendRootWindow::~BackendRootWindow()
{
	auto isSnippetBrowser = owner->isSnippetBrowser();

	if(!isSnippetBrowser)
	{
		for(auto w: allWindowsAndBrowsers)
		{
			if(w.getComponent() == nullptr)
				continue;

			if(w != this)
				w->deleteThisSnippetInstance(true);
		}

		saveKeyPressMap();
		saveInterfaceData();
	}
	else
	{
#if JUCE_MAC
        for(auto w: allWindowsAndBrowsers)
        {
            auto brw = w.getComponent();
            
            if(brw != nullptr && brw != this &&
               !brw->getBackendProcessor()->isSnippetBrowser())
            {
                MenuBarModel::setMacMainMenu(nullptr);
                MenuBarModel::setMacMainMenu(brw);
                brw->updateCommands();
                break;
            }
        }
#endif
        
		// I know what I'm doing...
		saved = true;
	}
	
	popoutWindows.clear();

	getMainController()->getLockFreeDispatcher().removePresetLoadListener(this);
	getMainController()->removeScriptListener(this);

    GET_PROJECT_HANDLER(getMainController()->getMainSynthChain()).removeListener(this);
    
	getBackendProcessor()->getCommandManager()->clearCommands();
	getBackendProcessor()->getConsoleHandler().setMainConsole(nullptr);

    getBackendProcessor()->workbenches.removeListener(this);
    
	clearModalComponent();

	modalComponent = nullptr;

	// Remove the resize stuff

	constrainer = nullptr;
	yBorderDragger = nullptr;
	xBorderDragger = nullptr;
	currentDialog = nullptr;

	// Remove the menu

#if JUCE_MAC && IS_STANDALONE_APP
    if(!getBackendProcessor()->isSnippetBrowser())
    	MenuBarModel::setMacMainMenu(nullptr);
#else
	menuBar->setModel(nullptr);
	menuBar = nullptr;
#endif

	floatingRoot = nullptr;

	mainEditor = nullptr;

	detachOpenGl();
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

void BackendRootWindow::deleteThisSnippetInstance(bool sync)
{
	removeFromDesktop();

	if(!sync)
	{
		for(auto w: allWindowsAndBrowsers)
		{
			if(w != this)
				w->setCurrentlyActiveProcessor();
		}
	}
		

	auto f = [this]()
	{
		auto o = this->owner;
		delete this;
		delete o;
	};

	if(sync)
		f();
	else
		MessageManager::callAsync(f);
}

void BackendRootWindow::toggleSnippetBrowser()
{
	if(snippetBrowser == nullptr)
	{
		addAndMakeVisible(snippetBrowser = new multipage::library::SnippetBrowser(this));
	}
	else
	{
		snippetBrowser->setVisible(!snippetBrowser->isVisible());
	}

	resized();
}

mcl::TokenCollection::Ptr BackendRootWindow::getJavascriptTokenCollection(Component* any)
{
	auto cw = any->findParentComponentOfClass<ComponentWithBackendConnection>();
        
	if(cw == nullptr)
		return nullptr;
        
	if(auto brw = cw->getBackendRootWindow())
	{
		if(brw->javascriptTokens == nullptr)
			brw->javascriptTokens = new mcl::TokenCollection(mcl::LanguageIds::HiseScript);

		return brw->javascriptTokens;
	}

	return nullptr;
}

void BackendRootWindow::rebuildTokenProviders(const Identifier& languageId)
{
	if(languageId == mcl::LanguageIds::HiseScript)
	{
		if(javascriptTokens == nullptr)
			javascriptTokens = new mcl::TokenCollection(languageId);
		else
			javascriptTokens->clearTokenProviders();
	}
	
	mcl::TextEditor::setNewTokenCollectionForAllChildren(this, languageId, javascriptTokens);

	for(auto p: popoutWindows)
	{
		mcl::TextEditor::setNewTokenCollectionForAllChildren(p, languageId, javascriptTokens);
	}
}

void BackendRootWindow::scriptWasCompiled(JavascriptProcessor* processor)
{
	if(currentWorkspaceProcessor == dynamic_cast<Processor*>(processor))
	{
		SafeAsyncCall::call<BackendRootWindow>(*this, [](BackendRootWindow& r)
		{
			r.rebuildTokenProviders("HiseScript");;
		});
	}
}

File BackendRootWindow::getKeyPressSettingFile() const
{
	return ProjectHandler::getAppDataDirectory(nullptr).getChildFile("KeyPressMapping.xml");
}

void BackendRootWindow::initialiseAllKeyPresses()
{
	// Workspace Shortcuts

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_browser, "Fold Browser Tab", KeyPress(KeyPress::F2Key, ModifierKeys::shiftModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_editor, "Fold Code Editor", KeyPress(KeyPress::F3Key, ModifierKeys::shiftModifier, 0));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::save_hip, "Save as .HIP", KeyPress('s', ModifierKeys::commandModifier, 's'));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::save_xml, "Save as .XML", KeyPress('s', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 's'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_interface, "Fold Interface Designer", KeyPress(KeyPress::F4Key, ModifierKeys::shiftModifier, 0));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_watch, "Fold Script Variable Watch Table", KeyPress('q', ModifierKeys::commandModifier, 'q'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_list, "Fold Component / Node List", KeyPress('w', ModifierKeys::commandModifier, 'w'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_console, "Fold [K]onsole", KeyPress('k', ModifierKeys::commandModifier, 'k'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_properties, "Fold Component / Node Properties", KeyPress('e', ModifierKeys::commandModifier, 'e'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_browser, "Focus Browser Tab", KeyPress(KeyPress::F2Key));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_editor, "Focus Code Editor", KeyPress(KeyPress::F3Key));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_interface, "Focus Interface Designer", KeyPress(KeyPress::F4Key));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::cycle_browser, "Cycle Browser Tabs", KeyPress(KeyPress::F2Key, ModifierKeys::commandModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::cycle_editor, "Cycle Code Editor Tabs", KeyPress(KeyPress::F3Key, ModifierKeys::commandModifier, 0));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_browser, "Fold Browser Tab", KeyPress(KeyPress::F2Key, ModifierKeys::shiftModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_editor, "Fold Code Editor", KeyPress(KeyPress::F3Key, ModifierKeys::shiftModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_interface, "Fold Interface Designer", KeyPress(KeyPress::F4Key, ModifierKeys::shiftModifier, 0));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_watch, "Fold Script Variable Watch Table", KeyPress('q', ModifierKeys::commandModifier, 'q'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_list, "Fold Component / Node List", KeyPress('w', ModifierKeys::commandModifier, 'w'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_console, "Fold [K]onsole", KeyPress('k', ModifierKeys::commandModifier, 'k'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_properties, "Fold Component / Node Properties", KeyPress('e', ModifierKeys::commandModifier, 'e'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_browser, "Focus Browser Tab", KeyPress(KeyPress::F2Key));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_editor, "Focus Code Editor", KeyPress(KeyPress::F3Key));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_interface, "Focus Interface Designer", KeyPress(KeyPress::F4Key));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::cycle_browser, "Cycle Browser Tabs", KeyPress(KeyPress::F2Key, ModifierKeys::commandModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::cycle_editor, "Cycle Code Editor Tabs", KeyPress(KeyPress::F3Key, ModifierKeys::commandModifier, 0));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_browser, "Fold Browser Tab", KeyPress(KeyPress::F2Key, ModifierKeys::shiftModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_editor, "Fold Code Editor", KeyPress(KeyPress::F3Key, ModifierKeys::shiftModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_interface, "Fold Interface Designer", KeyPress(KeyPress::F4Key, ModifierKeys::shiftModifier, 0));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_watch, "Fold Script Variable Watch Table", KeyPress('q', ModifierKeys::commandModifier, 'q'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_list, "Fold Component / Node List", KeyPress('w', ModifierKeys::commandModifier, 'w'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_console, "Fold [K]onsole", KeyPress('k', ModifierKeys::commandModifier, 'k'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_properties, "Fold Component / Node Properties", KeyPress('e', ModifierKeys::commandModifier, 'e'));

	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_browser, "Focus Browser Tab", KeyPress(KeyPress::F2Key));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_editor, "Focus Code Editor", KeyPress(KeyPress::F3Key));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::focus_interface, "Focus Interface Designer", KeyPress(KeyPress::F4Key));
    addShortcut(this, "Workspaces", FloatingTileKeyPressIds::fold_map, "Focus BroadcasterMap", KeyPress(KeyPress::F6Key));
    
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::cycle_browser, "Cycle Browser Tabs", KeyPress(KeyPress::F2Key, ModifierKeys::commandModifier, 0));
	addShortcut(this, "Workspaces", FloatingTileKeyPressIds::cycle_editor, "Cycle Code Editor Tabs", KeyPress(KeyPress::F3Key, ModifierKeys::commandModifier, 0));

	mcl::FullEditor::initKeyPresses(this);
	PopupIncludeEditor::initKeyPresses(this);
	scriptnode::DspNetwork::initKeyPresses(this);

	ScriptContentPanel::initKeyPresses(this);
}

void BackendRootWindow::paint(Graphics& g)
{
	g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));
}

void BackendRootWindow::setScriptProcessorForWorkspace(JavascriptProcessor* jsp)
{
	sendRootContainerRebuildMessage(true);
	getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::WorkspaceScript, false);

	BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(this, jsp);
	BackendPanelHelpers::ScriptingWorkspace::showInterfaceDesigner(this, true);

	auto rootContainer = getMainPanel()->getRootContainer();

	auto editorOfParent = rootContainer->getFirstEditorOf(getMainSynthChain());
	auto editorOfChain = rootContainer->getFirstEditorOf(dynamic_cast<Processor*>(jsp)->getParentProcessor(false));

	if (editorOfParent != nullptr)
	{
		editorOfParent->getChainBar()->refreshPanel();
		editorOfParent->sendResizedMessage();

		editorOfParent->childEditorAmountChanged();
	}

	if (editorOfChain != nullptr)
	{
		editorOfChain->otherChange(editorOfChain->getProcessor());
		editorOfChain->childEditorAmountChanged();
	}
}

void BackendRootWindow::saveInterfaceData()
{
	if (resetOnClose)
	{
		getBackendProcessor()->setEditorData({});
	}
	else
	{
		auto tabs = BackendPanelHelpers::ScriptingWorkspace::getCodeTabs(this);

		for (int i = 0; i < tabs->getNumTabs(); i++)
		{
			auto c = tabs->getComponent(i);

			// Delete all panels which are not containers
			if (dynamic_cast<FloatingTileContainer*>(c->getCurrentFloatingPanel()) == nullptr)
			{
				tabs->removeFloatingTile(c);
				i--;
			}
		}

		DynamicObject::Ptr obj = new DynamicObject();

		var editorData = getRootFloatingTile()->getCurrentFloatingPanel()->toDynamicObject();

		if (editorData.getDynamicObject() != nullptr)
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

		getBackendProcessor()->setEditorData(var(obj.get()));
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

	auto b = getLocalBounds();

	if(snippetBrowser != nullptr && snippetBrowser->isVisible())
	{
        auto sb = b.removeFromLeft(400);
        
#if JUCE_MAC
        sb.setTop(-20);
#endif
        
		snippetBrowser->setBounds(sb);
	}

	if(suspendedOverlay != nullptr)
	{
		suspendedOverlay->setBounds(b);
	}

	progressOverlay->setBounds(b);

	const int menuBarOffset = menuBar == nullptr ? 0 : 20;

	if (menuBarOffset != 0)
		menuBar->setBounds(b.removeFromTop(menuBarOffset));

	floatingRoot->setBounds(b);

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
	BackendCommandTarget::Actions::showFileProjectSettings(this);
}

void BackendRootWindow::timerCallback()
{
	stopTimer();

	if (!GET_PROJECT_HANDLER(mainEditor->getMainSynthChain()).isActive() && !projectIsBeingExtracted)
	{
		owner->setChanged(false);
		BackendCommandTarget::Actions::createNewProject(this);
	}
}

void BackendRootWindow::resetInterface()
{
	if (PresetHandler::showYesNoWindow("Reset Interface", "The interface layout will be cleared on the next launch"))
	{
		resetOnClose = true;
		PresetHandler::showMessageWindow("Workspace Layout Reset", "Close and open this instance to reset the interface", PresetHandler::IconType::Info);
	}
}

void BackendRootWindow::learnModeChanged(BackendRootWindow& brw, ScriptComponent* c)
{
	brw.learnMode = c != nullptr;
	brw.repaint();
}

bool BackendRootWindow::isRotated() const
{
    auto s = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot.get(), "SwappableContainer");
    
    return dynamic_cast<VerticalTile*>(s) == nullptr;
}

bool BackendRootWindow::toggleRotate()
{
    auto s = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(getRootFloatingTile(), "SwappableContainer");
    auto isVertical = dynamic_cast<VerticalTile*>(s) != nullptr;
    s->getParentShell()->swapContainerType(isVertical ? "HorizontalTile" : "VerticalTile");

    FloatingTileHelpers::findTileWithId<FloatingTileContainer>(getRootFloatingTile(), "PersonaContainer")->getParentShell()->setForceShowTitle(false);

    getRootFloatingTile()->refreshRootLayout();
    return isVertical;

}

void BackendRootWindow::loadNewContainer(ValueTree & v)
{
	getBackendProcessor()->getJavascriptThreadPool().cancelAllJobs(false);

	// Avoid the token collectio thread keeping alive some objects that should be deleted.
	callRecursive<mcl::TextEditor>(this, [](mcl::TextEditor* editor)
	{
		if(editor->tokenCollection != nullptr)
		{
			editor->tokenCollection->stopThread(1000);
			editor->tokenCollection->clearTokenProviders();
		}
		
		return false;
	});

	// Clear all panels with a processor connection
	callRecursive<PanelWithProcessorConnection>(this, [](PanelWithProcessorConnection* p)
	{
		p->setContentWithUndo(nullptr, 0);
		return false;
	});
	
	// Make sure the items in the patch browser that refer to the module are deleted before the module.
	callRecursive<PatchBrowser>(this, [](PatchBrowser* b)
	{
		b->clearCollections();
		return false;
	});
	
	mainEditor->loadNewContainer(v);
}

void BackendRootWindow::loadNewContainer(const File &f)
{
	FloatingTile::Iterator<PanelWithProcessorConnection> iter(getRootFloatingTile());

	while (auto p = iter.getNextPanel())
		p->setContentWithUndo(nullptr, 0);

	mainEditor->loadNewContainer(f);
}

void BackendRootWindow::newHisePresetLoaded()
{
	JavascriptProcessor* p = nullptr;

	switch((SnippetBrowserHelpers::Category)currentCategory)
	{
	case SnippetBrowserHelpers::Category::Scriptnode: 
		p = ProcessorHelpers::getFirstProcessorWithType<JavascriptMasterEffect>(getMainSynthChain());
		break;
	default:
		p = ProcessorHelpers::getFirstProcessorWithType<JavascriptMidiProcessor>(getMainSynthChain());
		break;
	}

	if (p != nullptr)
	{
		BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(this, p);
		BackendPanelHelpers::showWorkspace(this, BackendPanelHelpers::Workspace::ScriptingWorkspace, sendNotificationSync);

		SafeAsyncCall::callWithDelay<BackendRootWindow>(*this, [](BackendRootWindow& r)
		{
			r.rebuildTokenProviders("HiseScript");;
		}, 500);
	}

	if(currentCategory == SnippetBrowserHelpers::Category::UI)
		{
		Component::callRecursive<MainTopBar>(this, [](MainTopBar* t)
		{
			t->togglePopup(MainTopBar::PopupType::PluginPreview, true);
			return true;
		});
		}
}

void BackendRootWindow::gotoIfWorkspace(Processor* p)
{
    if (auto jsp = dynamic_cast<JavascriptProcessor*>(p))
    {
        getBackendProcessor()->workbenches.setCurrentWorkbench(nullptr, false);
        
        BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(this, jsp);
        BackendPanelHelpers::showWorkspace(this, BackendPanelHelpers::Workspace::ScriptingWorkspace, sendNotification);

		SafeAsyncCall::call<BackendRootWindow>(*this, [](BackendRootWindow& r)
		{
			r.rebuildTokenProviders("HiseScript");;
		});
		
    }
    else if (auto sampler = dynamic_cast<ModulatorSampler*>(p))
    {
        BackendPanelHelpers::SamplerWorkspace::setGlobalProcessor(this, sampler);
        BackendPanelHelpers::showWorkspace(this, BackendPanelHelpers::Workspace::SamplerWorkspace, sendNotification);
    }
}

void BackendRootWindow::showWorkspace(int workspace)
{
	currentWorkspace = workspace;

	int workspaceIndex = workspace - BackendCommandTarget::WorkspaceScript;

	static const Array<Identifier> ids = { "ScriptingWorkspace", "SamplerWorkspace" };

	for (int i = 0; i < workspaces.size(); i++)
	{
		auto wb = workspaces[i];

		if (wb == nullptr)
		{
			workspaces.set(i, FloatingTileHelpers::findTileWithId<FloatingTileContainer>(getRootFloatingTile(), ids[i])->getParentShell());

			wb = workspaces[i];
		}

		if(i == workspaceIndex)
        {
			wb->ensureVisibility();
        }
		else
			wb->getLayoutData().setVisible(false);
	}

	getRootFloatingTile()->refreshRootLayout();
}


void BackendRootWindow::paintOverChildren(Graphics& g)
{
	if (learnMode)
	{
		RectangleList<float> areas;

		Component::callRecursive<Learnable>(this, [&areas, this](Learnable* m)
		{
			auto c = m->asComponent();

			if (m->isLearnable() && c->isShowing() && c->findParentComponentOfClass<ScriptContentComponent>() == nullptr) 
			{
				areas.addWithoutMerging(this->getLocalArea(c, c->getLocalBounds()).toFloat());
			}

			return false;
		});

		
		Learnable::Factory f;
		auto p = f.createPath("destination");
		
		for (int i = 0; i < areas.getNumRectangles(); i++)
		{
			auto a = areas.getRectangle(i);
			g.setColour(Colours::black.withAlpha(0.2f));
			g.fillRect(a.reduced(1));
			Learnable::Factory f;
			auto p = f.createPath("source");
			f.scalePath(p, a.reduced(2).removeFromLeft(28).removeFromTop(18).reduced(2));
			g.setColour(Colour(SIGNAL_COLOUR));
			g.drawRect(a, 1.0f);
			g.fillPath(p);
		}
	}
}

void BackendRootWindow::userTriedToCloseWindow()
{
	jassert(owner->isSnippetBrowser());
	deleteThisSnippetInstance(false);
}

void BackendRootWindow::setCurrentlyActiveProcessor()
{
	for(int i = 0; i < allWindowsAndBrowsers.size(); i++)
	{
		if(allWindowsAndBrowsers[i].getComponent() == nullptr)
			allWindowsAndBrowsers.remove(i--);
	}

	for(auto w: allWindowsAndBrowsers)
	{
		auto isActive = this == w;

		if(isActive)
		{
			Desktop::getInstance().getAnimator().fadeOut(suspendedOverlay, 300);
			suspendedOverlay = nullptr;
			w->getBackendProcessor()->callback->setProcessor(w->getBackendProcessor());
			w->getBackendProcessor()->allNotesOff(true);
			resized();
		}
		else
		{
			w->addAndMakeVisible(w->suspendedOverlay = new SuspendedOverlay());
			w->suspendedOverlay->toFront(false);
			w->resized();
		}
	}
		
		
#if JUCE_MAC
        MenuBarModel::setMacMainMenu(this);
#endif
		
}

hise::FloatingTabComponent* BackendRootWindow::getCodeTabs()
{
	return BackendPanelHelpers::ScriptingWorkspace::getCodeTabs(this);
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
		root->getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::WorkspaceScript + (int)workspaceToShow, false); 
	}
	else
	{
		root->showWorkspace(BackendCommandTarget::WorkspaceScript + (int)workspaceToShow);
	}
}

bool BackendPanelHelpers::isMainWorkspaceActive(FloatingTile* /*root*/)
{
	return true;
}

FloatingTile* BackendPanelHelpers::ScriptingWorkspace::get(BackendRootWindow* rootWindow)
{
	return rootWindow->getRootFloatingTile();
}

void BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(BackendRootWindow* rootWindow, JavascriptProcessor* jsp)
{
	auto workspace = get(rootWindow);

    rootWindow->getBackendProcessor()->workspaceBroadcaster.sendMessage(sendNotificationAsync, JavascriptProcessor::getConnectorId(),  dynamic_cast<Processor*>(jsp));
    
	auto shouldShowInterface = dynamic_cast<JavascriptMidiProcessor*>(jsp) != nullptr;

	auto sn = FloatingTileHelpers::findTileWithId<VerticalTile>(workspace, "ScriptingWorkspaceScriptnode")->getParentShell();
	auto id = FloatingTileHelpers::findTileWithId<VerticalTile>(workspace, "ScriptingWorkspaceInterfaceDesigner")->getParentShell();

	sn->getLayoutData().setVisible(!shouldShowInterface);
	id->getLayoutData().setVisible(shouldShowInterface);
	sn->getParentContainer()->refreshLayout();

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
	rootWindow->getBackendProcessor()->workspaceBroadcaster.sendMessage(sendNotificationAsync, ModulatorSampler::getConnectorId(), sampler);
}



} // namespace hise
