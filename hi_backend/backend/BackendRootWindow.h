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

#ifndef BACKENDROOTWINDOW_H_INCLUDED
#define BACKENDROOTWINDOW_H_INCLUDED




#define GET_BACKEND_ROOT_WINDOW(child) child->template findParentComponentOfClass<ComponentWithBackendConnection>()->getBackendRootWindow()

#define GET_ROOT_FLOATING_TILE(child) GET_BACKEND_ROOT_WINDOW(child)->getRootFloatingTile()

// This is a simple counter that gets bumped everytime the layout is changed and shows a hint to reset the workspace
#define BACKEND_UI_VERSION 11

namespace hise { using namespace juce;

class BackendProcessorEditor;

class SuspendedOverlay: public Component
{
	void paint(Graphics& g) override;

	void mouseDown(const MouseEvent& event) override;
};

struct SnippetBrowserHelpers
{
	enum class Category
	{
		Undefined,
		Modules,
		MIDI,
		ScriptingApi,
		Scriptnode,
		UI,
		numCategories
	};

	static std::map<Identifier, bool> getFoldConfiguration(Category c);

	static StringArray getCategoryNames();
};




class BackendRootWindow : public TopLevelWindowWithOptionalOpenGL,
						  public TopLevelWindowWithKeyMappings,
						  public AudioProcessorEditor,
						  public BackendCommandTarget,
                          public snex::ui::WorkbenchManager::WorkbenchChangeListener,
						  public Timer,
						  public ComponentWithKeyboard,
						  public ModalBaseWindow,
						  public ComponentWithBackendConnection,
						  public DragAndDropContainer,
						  public ComponentWithHelp::GlobalHandler,
                          public ProjectHandler::Listener,
						  public GlobalScriptCompileListener,
						  public MainController::LockFreeDispatcher::PresetLoadListener
{
public:

	struct TooltipLookAndFeel: public LookAndFeel_V4
	{
		static TextLayout layoutTooltipText(const String& text, Colour colour) noexcept;

		Rectangle< int > getTooltipBounds(const String& tipText, Point<int> screenPos, Rectangle<int> parentArea) override;

		void drawTooltip(Graphics& g, const String& text, int width,int height) override;
	} ttlaf;

	struct TooltipWindowWithoutScriptContent: public juce::TooltipWindow
	{
		TooltipWindowWithoutScriptContent() :
			TooltipWindow(nullptr, 900)
		{};

		String getTipFor(Component&component) override;
	};

	TooltipWindowWithoutScriptContent funkytooltips;

	BackendRootWindow(AudioProcessor *ownerProcessor, var editorState);

	~BackendRootWindow();

	bool isFullScreenMode() const;
	void deleteThisSnippetInstance(bool sync);

	void toggleSnippetBrowser();

	ScopedPointer<multipage::library::SnippetBrowser> snippetBrowser;

	static mcl::TokenCollection::Ptr getJavascriptTokenCollection(Component* any);

	void rebuildTokenProviders(const Identifier& languageId);

	void scriptWasCompiled(JavascriptProcessor *processor) override;

	File getKeyPressSettingFile() const override;

	void initialiseAllKeyPresses() override;

	void paint(Graphics& g) override;

	void workbenchChanged(WorkbenchData::Ptr newWorkbench) override
    {
        if(newWorkbench != nullptr && newWorkbench->getCodeProvider()->providesCode())
        {
            if(auto firstEditor = FloatingTileHelpers::findTileWithId<SnexEditorPanel>(getRootFloatingTile(), {}))
                firstEditor->getParentShell()->ensureVisibility();
        }
    }

	static void learnModeChanged(BackendRootWindow& brw, ScriptComponent* c);
    


    bool isRotated() const;
    
    bool toggleRotate();
    
	void setScriptProcessorForWorkspace(JavascriptProcessor* jsp);

	void saveInterfaceData();

	void resized();

	void showSettingsWindow();

	void timerCallback() override;

	void toggleLayoutMode()
	{
		const bool shouldBeOn = !bpe->getRootFloatingTile()->isLayoutModeEnabled();

		getRootFloatingTile()->setLayoutModeEnabled(shouldBeOn);

		for (int i = 0; i < popoutWindows.size(); i++)
		{
			popoutWindows[i]->getRootFloatingTile()->setLayoutModeEnabled(shouldBeOn);
		}
	}

	BackendProcessor* getBackendProcessor() { return owner; }
	const BackendProcessor* getBackendProcessor() const { return owner; }

	BackendRootWindow* getBackendRootWindow() override { return this; }

	const BackendRootWindow* getBackendRootWindow() const override { return this; }

	BackendProcessorEditor* getMainPanel() { return mainEditor; }

	FloatingTileDocumentWindow* getLastPopup() { return popoutWindows.getLast(); }

	void resetInterface();

	Component* getKeyboard() const override
	{
		if (floatingRoot == nullptr)
			return nullptr;

		FloatingTile::Iterator<MidiKeyboardPanel> it(floatingRoot);

		while (auto kb = it.getNextPanel())
		{
			if (kb->isVisible())
				return kb->getKeyboard();
		}

		return nullptr;
	}

	const BackendProcessorEditor* getMainPanel() const { return mainEditor; }

	ModulatorSynthChain* getMainSynthChain() { return owner->getMainSynthChain(); }
	const ModulatorSynthChain* getMainSynthChain() const { return owner->getMainSynthChain(); }

	void loadNewContainer(ValueTree & v);

	void loadNewContainer(const File &f);
	
	void newHisePresetLoaded() override;

	FloatingTile* getRootFloatingTile() override { return floatingRoot; }

	MainController::ProcessorChangeHandler &getModuleListNofifier() { return getMainSynthChain()->getMainController()->getProcessorChangeHandler(); }

    void projectChanged(const File& newWorkingDirectory) override
    {
        
    }
    
	void sendRootContainerRebuildMessage(bool synchronous)
	{
		getModuleListNofifier().sendProcessorChangeMessage(getMainSynthChain(), MainController::ProcessorChangeHandler::EventType::RebuildModuleList, synchronous);
	}

	Processor* getCurrentWorkspaceProcessor() { return currentWorkspaceProcessor.get(); }

	int getCurrentWorkspace() const { return currentWorkspace; }

    void gotoIfWorkspace(Processor* p);
    
	void showWorkspace(int workspace);

	MainTopBar* getMainTopBar()
	{
		FloatingTile::Iterator<MainTopBar> iter(getRootFloatingTile());

		auto bar = iter.getNextPanel();

		jassert(bar != nullptr);

		return bar;
	}

	FloatingTileDocumentWindow* addFloatingWindow()
	{
		popoutWindows.add(new FloatingTileDocumentWindow(this));

        popoutWindows.getLast()->addKeyListener(mainCommandManager->getKeyMappings());
        
		return popoutWindows.getLast();
	}

	void removeFloatingWindow(FloatingTileDocumentWindow* windowToRemove)
	{
		if (docWindow == windowToRemove)
			docWindow = nullptr;
		else
			popoutWindows.removeObject(windowToRemove, true);
	}

	void showHelp(ComponentWithHelp* h) override
	{
		auto url = h->getMarkdownHelpUrl();
		createOrShowDocWindow({ File(), url });
	}

	MarkdownPreview* createOrShowDocWindow(const MarkdownLink& l);

	void paintOverChildren(Graphics& g) override;

	template <class EditorType> bool addEditorTabsOfType()
	{
		auto tabs = getCodeTabs();

		if (tabs->getNumChildPanelsWithType<EditorType>() == 0)
		{
			FloatingInterfaceBuilder b(tabs->getParentShell());

			auto newEditor = b.addChild<EditorType>(0);

			if(auto pc = b.getContent<PanelWithProcessorConnection>(newEditor))
				pc->setContentWithUndo(getCurrentWorkspaceProcessor(), 0);

			return true;
		}

		return false;
	}

	void setProjectIsBeingExtracted()
	{
		projectIsBeingExtracted = true;
	}

	Array<Component::SafePointer<BackendRootWindow>> allWindowsAndBrowsers;

	void userTriedToCloseWindow() override;

	void setCurrentlyActiveProcessor();

	SnippetBrowserHelpers::Category currentCategory = SnippetBrowserHelpers::Category::Undefined;

private:

	mcl::TokenCollection::Ptr javascriptTokens;

	bool projectIsBeingExtracted = false;

	friend class ProjectImporter;

	FloatingTabComponent* getCodeTabs();

	bool learnMode = false;

	GlobalHiseLookAndFeel globalLookAndFeel;

	OwnedArray<FloatingTileDocumentWindow> popoutWindows;

	int currentWorkspace = BackendCommandTarget::WorkspaceScript;
	
	Array<Component::SafePointer<FloatingTile>> workspaces;

	WeakReference<Processor> currentWorkspaceProcessor;

	friend class BackendCommandTarget;

	PopupLookAndFeel plaf;

	BackendProcessor *owner;

	Component::SafePointer<BackendProcessorEditor> mainEditor;

	StringArray menuNames;

	ScopedPointer<MenuBarComponent> menuBar;

	ScopedPointer<ThreadWithQuasiModalProgressWindow::Overlay> progressOverlay;

	ScopedPointer<AudioDeviceDialog> currentDialog;

	ScopedPointer<ComponentBoundsConstrainer> constrainer;

	ScopedPointer<ResizableBorderComponent> yBorderDragger;
	ScopedPointer<ResizableBorderComponent> xBorderDragger;


	ScopedPointer<FloatingTile> floatingRoot;

	ScopedPointer<FloatingTileDocumentWindow> docWindow;

	bool resetOnClose = false;

	JUCE_DECLARE_WEAK_REFERENCEABLE(BackendRootWindow);

	ScopedPointer<SuspendedOverlay> suspendedOverlay;
};

struct BackendPanelHelpers
{
	enum class Workspace
	{
		ScriptingWorkspace = 0,
		SamplerWorkspace,
		CustomWorkspace,
		numWorkspaces
	};

	template <class ContentType> static ContentType* toggleVisibilityForRightColumnPanel(FloatingTile* root, bool show)
	{
		return nullptr;
	}

	static VerticalTile* getMainTabComponent(FloatingTile* root);

	static HorizontalTile* getMainLeftColumn(FloatingTile* root);

	static HorizontalTile* getMainRightColumn(FloatingTile* root);

	static void showWorkspace(BackendRootWindow* root, Workspace workspaceToShow, NotificationType notifyCommandManager);

	struct ScriptingWorkspace
	{
		static FloatingTile* get(BackendRootWindow* rootWindow);

		static void setGlobalProcessor(BackendRootWindow* rootWindow, JavascriptProcessor* jsp);

		static void showEditor(BackendRootWindow* rootWindow, bool shouldBeVisible);
		static void showInterfaceDesigner(BackendRootWindow* rootWindow, bool shouldBeVisible);

		static FloatingTabComponent* getCodeTabs(BackendRootWindow* r)
		{
			auto floatingRoot = r->getRootFloatingTile();
			auto codeTabs = FloatingTileHelpers::findTileWithId<FloatingTileContainer>(floatingRoot, Identifier("ScriptEditorTabs"))->getParentShell();
			jassert(codeTabs != nullptr && dynamic_cast<FloatingTabComponent*>(codeTabs->getCurrentFloatingPanel()) != nullptr);
			return dynamic_cast<FloatingTabComponent*>(codeTabs->getCurrentFloatingPanel());
		}

	};

	struct SamplerWorkspace
	{
		static FloatingTile* get(BackendRootWindow* rootWindow);

		static void setGlobalProcessor(BackendRootWindow* rootWindow, ModulatorSampler* sampler);
	};

	static bool isMainWorkspaceActive(FloatingTile* root);

#if JUCE_LINUX
    // This might keep the fonts alive and increase the text
    // rendering performance...
    hise::LinuxFontHandler::Instance fontHandler;
#endif

};

} // namespace hise

#endif  // BACKENDROOTWINDOW_H_INCLUDED
