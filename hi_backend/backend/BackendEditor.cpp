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
	addAndMakeVisible(breadCrumbComponent = new BreadcrumbComponent(owner));
	
	addChildComponent(debugLoggerWindow = new DebugLoggerComponent(&owner->getDebugLogger()));

	viewport->viewport->setScrollBarThickness(SCROLLBAR_WIDTH);
	viewport->viewport->setSingleStepSizes(0, 6);

	setRootProcessor(owner->synthChain->getRootProcessor());

	owner->addScriptListener(this);

};

BackendProcessorEditor::~BackendProcessorEditor()
{
	setLookAndFeel(nullptr);
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
    Processor *p = static_cast<Processor*>(owner->synthChain);
    
	rebuildContainer();
    
    container->setRootProcessorEditor(p);
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

	owner->killAndCallOnLoadingThread([](Processor* p) {p->getMainController()->clearPreset(); return SafeFunctionCall::OK; });

}

void BackendProcessorEditor::clearModuleList()
{

	//dynamic_cast<PatchBrowser*>(propertyDebugArea->getComponentForIndex(PropertyDebugArea::ModuleBrowser))->clearCollections();
}

#undef toggleVisibility


MainTopBar::MainTopBar(FloatingTile* parent) :
	FloatingTileContent(parent),
	ComponentWithHelp(parent->getBackendRootWindow())
{
	MainToolbarFactory f;

	setRepaintsOnMouseActivity(true);

	addAndMakeVisible(hiseButton = new ImageButton("HISE"));

	Image hise = ImageCache::getFromMemory(BinaryData::logo_mini_png, BinaryData::logo_mini_pngSize);
	

	hiseButton->setImages(false, true, true, hise, 0.9f, Colour(0), hise, 1.0f, Colours::white.withAlpha(0.1f), hise, 1.0f, Colours::white.withAlpha(0.1f), 0.1f);
	hiseButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuHelpShowAboutPage, true);

	addAndMakeVisible(backButton = new ShapeButton("Back", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.8f), Colours::white));
	backButton->setShape(f.createPath("back"), false, true, true);
	backButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuViewBack, true);

	parent->getRootFloatingTile()->addPopupListener(this);

	addAndMakeVisible(forwardButton = new ShapeButton("Forward", Colours::white.withAlpha(0.4f), Colours::white.withAlpha(0.8f), Colours::white));
	forwardButton->setShape(f.createPath("forward"), false, true, true);
	forwardButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::MenuViewForward, true);

	addAndMakeVisible(macroButton = new HiseShapeButton("Macro Controls", this, f));
	macroButton->setToggleModeWithColourChange(true);
	macroButton->setTooltip("Show 8 Macro Controls");
	

	addAndMakeVisible(presetBrowserButton = new ShapeButton("Preset Browser", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	presetBrowserButton->setTooltip("Show Preset Browser");
	presetBrowserButton->setShape(f.createPath("Preset Browser"), false, true, true);
	presetBrowserButton->addListener(this);

	addAndMakeVisible(pluginPreviewButton = new ShapeButton("Plugin Preview", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	pluginPreviewButton->setTooltip("Show Plugin Preview");
	pluginPreviewButton->setShape(f.createPath("Plugin Preview"), false, true, true);
	pluginPreviewButton->addListener(this);



	addAndMakeVisible(mainWorkSpaceButton = new HiseShapeButton("Main Workspace", this, f));
	mainWorkSpaceButton->setTooltip("Show Main Workspace");
	mainWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceMain, true);

	addAndMakeVisible(scriptingWorkSpaceButton = new HiseShapeButton("Scripting Workspace", this, f));
	scriptingWorkSpaceButton->setTooltip("Show Scripting Workspace");
	scriptingWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceScript, true);

	addAndMakeVisible(samplerWorkSpaceButton = new HiseShapeButton("Sampler Workspace", this, f));
	samplerWorkSpaceButton->setTooltip("Show Sampler Workspace");
	samplerWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceSampler, true);

	addAndMakeVisible(customWorkSpaceButton = new HiseShapeButton("Custom Workspace", this, f));
	customWorkSpaceButton->setTooltip("Show Scripting Workspace");
	customWorkSpaceButton->setCommandToTrigger(getRootWindow()->getBackendProcessor()->getCommandManager(), BackendCommandTarget::WorkspaceCustom, true);
	
	addAndMakeVisible(peakMeter = new ProcessorPeakMeter(getRootWindow()->getMainSynthChain()));

	addAndMakeVisible(settingsButton = new ShapeButton("Audio Settings", Colours::white.withAlpha(0.6f), Colours::white.withAlpha(0.8f), Colours::white));
	settingsButton->setTooltip("Show Audio Settings");
	settingsButton->setShape(f.createPath("Settings"), false, true, true);
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

		sizeSelector->setColour(HiseColourScheme::ComponentFillTopColourId, Colour(0x66333333));
		sizeSelector->setColour(HiseColourScheme::ComponentFillBottomColourId, Colour(0xfb111111));
		sizeSelector->setColour(HiseColourScheme::ComponentOutlineColourId, Colours::white.withAlpha(0.3f));
		sizeSelector->setColour(HiseColourScheme::ComponentTextColourId, Colours::white);

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

		CALL_ASYNC_WITH_COMPONENT(TextButton, b)
		{
			b->grabKeyboardFocus();
		});
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

				jsp->getSnippet(0)->replaceContentAsync(code);
				jsp->compileScript();

				midiChain->getHandler()->add(s, nullptr);

				midiChain->setEditorState(Processor::EditorState::Visible, true);
				s->setEditorState(Processor::EditorState::Folded, true);

				auto root = GET_BACKEND_ROOT_WINDOW(this);
				
				root->sendRootContainerRebuildMessage(true);

				root->getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::WorkspaceScript, false);

				BackendPanelHelpers::ScriptingWorkspace::setGlobalProcessor(root, jsp);
				BackendPanelHelpers::ScriptingWorkspace::showInterfaceDesigner(root, true);
                
                auto rootContainer = root->getMainPanel()->getRootContainer();
                
                auto editorOfParent = rootContainer->getFirstEditorOf(root->getMainSynthChain());
                auto editorOfChain = rootContainer->getFirstEditorOf(midiChain);
                
				if (editorOfParent != nullptr)
				{
					editorOfParent->getChainBar()->refreshPanel();
					editorOfParent->sendResizedMessage();
				}
                
                if(editorOfChain != nullptr)
                {
                    editorOfChain->changeListenerCallback(editorOfChain->getProcessor());
                    editorOfChain->childEditorAmountChanged();
                }
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
	GlobalHiseLookAndFeel klaf;

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
	bool presetBrowserShown = dynamic_cast<PresetBrowser*>(newComponent) != nullptr;

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
	

	Colour c1 = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF424242));
	Colour c2 = JUCE_LIVE_CONSTANT_OFF(Colour(0xFF404040));

	g.setGradientFill(ColourGradient(c1, 0.0f, 0.0f, c2, 0.0f, (float)getHeight(), false));
	g.fillAll();
	
	g.setColour(Colours::white.withAlpha(0.2f));
	g.setFont(GLOBAL_BOLD_FONT());
	g.drawText("Frontend Panels", frontendArea.withTrimmedBottom(11), Justification::centredBottom);
	g.drawText("Workspaces", workspaceArea.withTrimmedBottom(11), Justification::centredBottom);

	
}

void MainTopBar::paintOverChildren(Graphics& g)
{
	ComponentWithHelp::paintHelp(g);
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
	
	settingsButton->setBounds(x, centerY, 28, 28);
	peakMeter->setBounds(voiceCpuBpmComponent->getRight() - 2, centerY + 4, settingsButton->getX() - voiceCpuBpmComponent->getRight(), 24);

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
		PresetBrowser* pr = new PresetBrowser(mc, 700, 500);

		PresetBrowser::Options newOptions;

		newOptions.highlightColour = Colour(SIGNAL_COLOUR);
		newOptions.backgroundColour = Colours::black.withAlpha(0.8f);
		newOptions.textColour = Colours::white;
		newOptions.font = GLOBAL_BOLD_FONT();

		pr->setOptions(newOptions);

		c = dynamic_cast<Component*>(pr);

		button = presetBrowserButton;
		break;
	}
	case MainTopBar::PopupType::numPopupTypes:
		break;
	default:
		break;
	}

	Point<int> point(button->getLocalBounds().getCentreX(), button->getLocalBounds().getBottom());
	auto popup = getParentShell()->showComponentInRootPopup(c, button, point);

	if (popup != nullptr)
		popup->setColour((int)FloatingTilePopup::ColourIds::backgroundColourId, JUCE_LIVE_CONSTANT_OFF(Colour(0xec000000)));

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
