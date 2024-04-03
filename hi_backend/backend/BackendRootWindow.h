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



#define DECLARE_FOLD_ID(id) static const Identifier id("$fold_" + String(#id));

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

struct SnippetBrowser: public simple_css::HeaderContentFooter,
					   public TableListBoxModel
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

	static std::map<Identifier, bool> getFoldConfiguration(Category c)
	{
		std::map<Identifier, bool> map;

		switch(c)
		{
		case Category::Undefined: break;
		case Category::Modules:
			map[fold_ids::editor] = false;
			map[fold_ids::watch] = true;
			map[fold_ids::map] = true;
			map[fold_ids::console] = true;
			map[fold_ids::interface] = true;
			map[fold_ids::list] = true;
			map[fold_ids::properties] = true;
			map[fold_ids::browser] = false;
			break;
		case Category::MIDI:
			map[fold_ids::editor] = false;
			map[fold_ids::watch] = true;
			map[fold_ids::map] = true;
			map[fold_ids::console] = false;
			map[fold_ids::interface] = true;
			map[fold_ids::list] = true;
			map[fold_ids::properties] = true;
			map[fold_ids::browser] = false;
			break;
		case Category::ScriptingApi:
			map[fold_ids::editor] = false;
			map[fold_ids::watch] = true;
			map[fold_ids::map] = true;
			map[fold_ids::console] = false;
			map[fold_ids::interface] = true;
			map[fold_ids::list] = true;
			map[fold_ids::properties] = true;
			map[fold_ids::browser] = true;
			break;
		case Category::Scriptnode:
			map[fold_ids::editor] = true;
			map[fold_ids::watch] = true;
			map[fold_ids::map] = true;
			map[fold_ids::console] = false;
			map[fold_ids::interface] = false;
			map[fold_ids::list] = true;
			map[fold_ids::properties] = true;
			map[fold_ids::browser] = true;
			break;
		case Category::UI:
			map[fold_ids::editor] = false;
			map[fold_ids::watch] = true;
			map[fold_ids::map] = true;
			map[fold_ids::console] = false;
			map[fold_ids::interface] = true;
			map[fold_ids::list] = true;
			map[fold_ids::properties] = true;
			map[fold_ids::browser] = true;
			break;
		case Category::numCategories:
			map[fold_ids::editor] = true;
			map[fold_ids::watch] = true;
			map[fold_ids::map] = true;
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

	static StringArray getCategoryNames()
	{
		return { "All", "Modules", "MIDI", "Scripting", "Scriptnode", "UI" };
	}

	enum ColumnIs
	{
		columnName = 1,
		columnAuthor,
		numColumns
	};

	struct Item
	{
		String data[numColumns];
		String description;
		Category category;
		String snippet;
		StringArray tags;
	};

	Array<Item> items;
	Array<Item> filteredItems;

	int getNumRows() override { return filteredItems.size(); }

	void updateFilter()
	{
		Array<Item> newFilterList;

		for(const auto& f: items)
		{
			if(matchesFilter(f))
				newFilterList.add(f);
		}

		std::swap(filteredItems, newFilterList);
		table.updateContent();
		
		table.resized();
		table.repaint();
	}


	bool matchesFilter(const Item& i) const
	{
		auto t = searchField.getText().toLowerCase();

		auto matchesDescription = t.isEmpty() || i.description.toLowerCase().contains(t);
		auto matchesName = t.isEmpty() || i.data[columnName].toLowerCase().contains(t);
		auto matchesAuthor = t.isEmpty() || i.data[columnAuthor].toLowerCase().contains(t);
		
		auto matchesTag = true;

		StringArray activeTags;

		for(auto f: tagButtons)
		{
			if(f->getToggleState())
			{
				matchesTag &= i.tags.contains(f->getButtonText());
			}
		}

		bool matchesCategory = true;

		for(int j = 0; j < (int)Category::numCategories; j++)
		{
			if(categoryButtons[j].getToggleState())
			{
				if(j == 0)
					matchesCategory = true;
				else
					matchesCategory = i.category == (Category)j;

				break;
			}
		}

		return (matchesAuthor || matchesDescription || matchesName) && matchesTag && matchesCategory;
	}

	void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override
	{
		using namespace simple_css;
		simple_css::Renderer r(nullptr, stateWatcher);

		auto point = table.getMouseXYRelative();

		auto hoverRow = table.getRowContainingPosition(point.getX(), point.getY());

		int flags = 0;

		if(rowNumber == hoverRow)
			flags |= (int)PseudoClassType::Hover;

		if(rowIsSelected)
			flags |= (int)PseudoClassType::Active;

		r.setPseudoClassState(flags);

		if(auto ss = css.getWithAllStates((Selector(ElementType::TableRow))))
		{
			r.drawBackground(g, {0.0f, 0.0f, (float)width, (float)height}, ss);
		}
	}

    void paintCell (Graphics& g,  int rowNumber, int columnId, int width, int height, bool rowIsSelected) override
	{
		using namespace simple_css;
		simple_css::Renderer r(nullptr, stateWatcher);

		int flags = 0;

		if(rowIsSelected)
			flags |= (int)PseudoClassType::Active;

		r.setPseudoClassState(flags);

		if(auto ss = css.getWithAllStates(Selector(ElementType::TableCell)))
		{
			Rectangle<float> area = {0.0f, 0.0f, (float)width, (float)height };
			r.drawBackground(g, area, ss);
			r.renderText(g, area, filteredItems[rowNumber].data[columnId-1], ss);
		}
	}

	struct Updater: public Thread,
				    public AsyncUpdater
	{
		Updater(SnippetBrowser& parent_):
		  Thread("Update snippet database"),
		  parent(parent_)
		{
			startThread(5);
		}

		SnippetBrowser& parent;

		void handleAsyncUpdate() override
		{
			parent.table.updateContent();
			parent.tagButtons.clear();

			StringArray tags;

			for(const auto& i: parent.items)
			{
				for(const auto& t: i.tags)
				{
					auto t2 = t.unquoted();

					if(t2.isNotEmpty())
						tags.addIfNotAlreadyThere(t2);
				}
			}

			for(auto& t: tags)
			{
				auto nb = new TextButton(t);
				nb->setClickingTogglesState(true);
				nb->onClick = std::bind(&SnippetBrowser::updateFilter, &parent);
				simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(*nb, {".tag-button"});
				parent.tags.addAndMakeVisible(nb);
				parent.tagButtons.add(nb);
			}

			parent.table.autoSizeAllColumns();
			parent.body.setCSS(parent.css);
			parent.updateFilter();
		}

		~Updater()
		{
			stopThread(1000);
		}

		void run() override
		{
			Array<Item> newItems;
			
			URL listURL("https://api.github.com/repos/qdr/HiseSnippetDB/git/trees/main?recursive=1");

			auto list = listURL.readEntireTextStream(false);

			auto fileList = JSON::parse(list)["tree"];

			if(fileList.isArray())
			{
				for(auto& a: *fileList.getArray())
				{
					auto exampleName = a["path"].toString();

					auto escape = URL::addEscapeChars(exampleName, false);
						
					String s = "https://raw.githubusercontent.com/qdr/HiseSnippetDB/main/" + escape;
					URL content(s);
					auto c = content.readEntireTextStream(false);

					MarkdownParser p(c);
					p.parse();

					if(p.getHeader().getKeyValue("author").isEmpty())
						continue;

					Item ni;
					ni.data[columnName-1] = exampleName.upToLastOccurrenceOf(".md", false, false);
					ni.data[columnAuthor-1] = p.getHeader().getKeyValue("author");

					auto cat = p.getHeader().getKeyValue("category");

					auto idx = getCategoryNames().indexOf(cat);

					if(idx != -1)
						ni.category = (Category)idx;
					else
						ni.category = Category::Undefined;

					ni.snippet = p.getHeader().getKeyValue("HiseSnippet");
					ni.description = p.getCurrentText(false);
					ni.tags = StringArray::fromTokens(p.getHeader().getKeyValue("tags"), " ", "\"");
					
					newItems.add(ni);
				}
			}

			parent.items.swapWith(newItems);
			triggerAsyncUpdate();
		}
	};

	ScopedPointer<Updater> updater;

	void rebuildDatabase()
	{
		updater = new Updater(*this);
	}

	SnippetBrowser(Component* parent);

	ScrollbarFader sf;

	SimpleMarkdownDisplay description;

	int currentSelection = -1;

	void selectedRowsChanged(int rowNumber) override
	{
		description.setText(filteredItems[rowNumber].description);
		description.setVisible(true);
		table.scrollToEnsureRowIsOnscreen(rowNumber);
		currentSelection = rowNumber;
	}

	void cellClicked (int rowNumber, int columnId, const MouseEvent&) override
	{
		
	}

	void load(int rowIndex);

	void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&) override
	{
		load(rowNumber);
	}

	void returnKeyPressed(int lastRowSelected) override
	{
		load(lastRowSelected);
	}

	

	TextButton categoryButtons[(int)Category::numCategories]; // don't show undefined;

	simple_css::FlexboxComponent searchTab;
	simple_css::FlexboxComponent categories;
	simple_css::FlexboxComponent tags;

	OwnedArray<TextButton> tagButtons;

	TextButton closeButton;
	TextButton loadButton;
	TextButton refreshButton;
	TextEditor searchField;

	TableListBox table;
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
	void deleteThisSnippetInstance(bool sync)
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

	void toggleSnippetBrowser()
	{
		if(snippetBrowser == nullptr)
		{
			addAndMakeVisible(snippetBrowser = new SnippetBrowser(this));
		}
		else
		{
			snippetBrowser->setVisible(!snippetBrowser->isVisible());
		}

		resized();
	}

	ScopedPointer<SnippetBrowser> snippetBrowser;

	static mcl::TokenCollection::Ptr getJavascriptTokenCollection(Component* any)
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

	void rebuildTokenProviders(const Identifier& languageId)
	{
		if(javascriptTokens == nullptr && languageId == mcl::LanguageIds::HiseScript)
			javascriptTokens = new mcl::TokenCollection(languageId);

		mcl::TextEditor::setNewTokenCollectionForAllChildren(this, languageId, javascriptTokens);

		for(auto p: popoutWindows)
		{
			mcl::TextEditor::setNewTokenCollectionForAllChildren(p, languageId, javascriptTokens);
		}
	}

    void scriptWasCompiled(JavascriptProcessor *processor) override
    {
	    if(currentWorkspaceProcessor == dynamic_cast<Processor*>(processor))
	    {
			SafeAsyncCall::call<BackendRootWindow>(*this, [](BackendRootWindow& r)
			{
				r.rebuildTokenProviders("HiseScript");;
			});
	    }
    }
    
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

	void userTriedToCloseWindow()
	{
		jassert(owner->isSnippetBrowser());
		deleteThisSnippetInstance(false);
	}

	void setCurrentlyActiveProcessor()
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
		
		

		
	}

	SnippetBrowser::Category currentCategory = SnippetBrowser::Category::Undefined;

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
