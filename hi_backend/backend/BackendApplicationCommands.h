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

#ifndef BACKENDAPPLICATIONCOMMANDS_H_INCLUDED
#define BACKENDAPPLICATIONCOMMANDS_H_INCLUDED

class BackendRootWindow;




class BackendCommandTarget: public ApplicationCommandTarget,
							  public MenuBarModel
{
public:

	BackendCommandTarget(BackendProcessor *owner);

	enum MenuNames
	{
		FileMenu = 0,
		EditMenu,
		ToolsMenu,
		ViewMenu,
		HelpMenu,
		numMenuNames
	};

	enum ColumnMode
	{
		OneColumn,
		TwoColumns,
		ThreeColumns
	};

	/** All application commands are collected here. */
	enum MainToolbarCommands
	{
		ModulatorList = 0x10000,
		HamburgerMenu,
		CustomInterface,
		DebugPanel,
		ViewPanel,
		Mixer,
		Macros,
		Keyboard,
		Settings,
		WorkspaceMain,
		WorkspaceScript,
		WorkspaceSampler,
		WorkspaceCustom,
		numToolbarButtons,
		MenuFileOffset = 0x20000,
		MenuNewFile,
		MenuOpenFile,
		MenuOpenFileFromProjectOffset,
		MenuSaveFile = 0x23000,
		MenuSaveFileAs,
		MenuSaveFileAsXmlBackup,
		MenuOpenXmlBackup,
        MenuFileXmlBackupMenuOffset,
		MenuProjectNew = 0x24000,
		MenuProjectLoad,
		MenuCloseProject,
		MenuFileArchiveProject,
		MenuFileDownloadNewProject,
		MenuProjectShowInFinder,
		MenuProjectRecentOffset,
		MenuRevertFile = 0x26000,
        MenuFileSaveUserPreset,
        MenuFileUserPresetMenuOffset,
		MenuFileSettingsProject = 0x28000,
		MenuFileSettingsPreset,
		MenuFileSettings,
		MenuFileSettingsCompiler,
		MenuFileSettingsUser,
		MenuFileSettingCheckSanity,
		MenuFileSettingsCleanBuildDirectory,
		MenuReplaceWithClipboardContent,
		MenuExportFileAsPlugin,
		MenuExportFileAsEffectPlugin,
		MenuExportFileAsStandaloneApp,
		MenuExportFileAsPlayerLibrary,
        MenuExportFileAsSnippet,
		MenuFileQuit,
		MenuEditOffset = 0x30000,
		MenuEditUndo,
		MenuEditRedo,
		MenuEditCopy,
		MenuEditPaste,
		MenuEditMoveUp,
		MenuEditMoveDown,
        MenuEditCopyAsSnippet,
        MenuEditPasteAsSnippet,
		MenuViewShowSelectedProcessorInPopup,
        MenuEditPlotModulator,
		MenuEditCreateScriptVariable,
		MenuEditCreateBase64State,
        MenuEditCloseAllChains,
		MenuViewOffset = 0x40000,
        MenuViewFullscreen,
		MenuViewReset,
		MenuViewBack,
		MenuViewForward,
		MenuViewSetMainContainerAsRoot,
		MenuViewEnableGlobalLayoutMode,
		MenuOneColumn,
		MenuTwoColumns,
		MenuThreeColumns,
		MenuViewShowPool,
		MenuViewShowInspector,
		MenuViewShowPluginPopupPreview,
        MenuViewIncreaseCodeFontSize,
        MenuViewDecreaseCodeFontSize,
		MenuAddView,
		MenuDeleteView,
		MenuRenameView,
        MenuViewSaveCurrentView,
        MenuViewRemoveAllSoloProcessors,
        MenuViewShowAllHiddenProcessors,
        MenuViewListOffset = 0x70000,
		MenuViewProcessorListOffset = 0x80000,
		
		MenuToolsRecompile = 0x50000,
		MenuToolsCreateInterface,
        MenuToolsCheckDuplicate,
        MenuToolsClearConsole,
		MenuToolsSetCompileTimeOut,
		MenuToolsUseBackgroundThreadForCompile,
		MenuToolsRecompileScriptsOnReload,
		MenuToolsCreateToolbarPropertyDefinition,
		MenuToolsCreateExternalScriptFile,
		MenuToolsValidateUserPresets,
		MenuToolsExternalScriptFileOffset,
		MenuToolsResolveMissingSamples = 0x60000,
		MenuToolsDeleteMissingSamples,
		MenuToolsCheckAllSampleMaps,
		MenuToolsUseRelativePaths,
		MenuToolsCollectExternalFiles,
        MenuToolsRedirectSampleFolder,
		MenuToolsForcePoolSearch,
		MenuToolsConvertAllSamplesToMonolith,
		MenuToolsConvertSfzToSampleMaps,
		MenuToolsCreateRSAKeys,
		MenuToolsCreateDummyLicenceFile,
		MenuToolsEnableAutoSaving,
		MenuToolsEnableDebugLogging,
		MenuHelpShowAboutPage,
        MenuHelpCheckVersion,
		numCommands
	};

	virtual ~BackendCommandTarget()
	{
		mainCommandManager->setFirstCommandTarget(nullptr);
	};

	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

	void setCommandTarget(ApplicationCommandInfo &result, const String &name, bool active, bool ticked, char shortcut, bool useShortCut=true, ModifierKeys mod=ModifierKeys::commandModifier) {
		result.setInfo(name, name, "Target", 0);
		result.setActive(active); 
		result.setTicked(ticked);

		if (useShortCut) result.addDefaultKeypress(shortcut, mod);
	};

	bool clipBoardNotEmpty() const { return SystemClipboard::getTextFromClipboard().isNotEmpty(); }
    
    bool viewActive() const
    {
        return owner->synthChain->getCurrentViewInfo() != nullptr;
    }

	
	void setEditor(BackendRootWindow *editor);

	void getAllCommands(Array<CommandID>& commands) override;;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform(const InvocationInfo &info) override;
	
	void updateCommands()
	{
		mainCommandManager->commandStatusChanged();
		createMenuBarNames();

		menuItemsChanged();
	}

	void setCopyPasteTarget(CopyPasteTarget *newTarget)
	{
        if (currentCopyPasteTarget.get() != nullptr)
		{
			currentCopyPasteTarget->deselect();
		}
		else
		{
			mainCommandManager->setFirstCommandTarget(this);
		}
        
		currentCopyPasteTarget = newTarget;

		updateCommands();
	}


	void createMenuBarNames();

	StringArray getMenuBarNames() override { return menuNames; };
	PopupMenu getMenuForIndex(int topLevelMenuIndex, const String &menuName) override;
	void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;;

	ColumnMode getColumnMode() const noexcept { return currentColumnMode; }

	class Actions
	{
	public:

		static bool hasProcessorInClipboard();
		static bool hasSnippetInClipboard();
		static void openFile(BackendRootWindow *bpe);
		static void saveFile(BackendRootWindow *bpe, bool forceRename);
		static void replaceWithClipboardContent(BackendRootWindow *bpe);
		static void createScriptVariableDeclaration(CopyPasteTarget *currentCopyPasteTarget);
		static void recompileAllScripts(BackendRootWindow * bpe);
		static void toggleFullscreen(BackendRootWindow * bpe);
		static void addView(BackendRootWindow *bpe);
		static void deleteView(BackendRootWindow *bpe);
		static void saveView(BackendRootWindow *bpe);
		static void renameView(BackendRootWindow *bpe);
		static void closeAllChains(BackendRootWindow *bpe);
		static void checkDuplicateIds(BackendRootWindow *bpe);
		static void showAboutPage(BackendRootWindow * bpe);
		static void checkVersion(BackendRootWindow *bpe);
		static void setColumns(BackendRootWindow * bpe, BackendCommandTarget* target, ColumnMode columns);
		static void plotModulator(CopyPasteTarget *currentCopyPasteTarget);
		static void resolveMissingSamples(BackendRootWindow *bpe);
		static void deleteMissingSamples(BackendRootWindow *bpe);
		static void setCompileTimeOut(BackendRootWindow * bpe);
		static void toggleUseBackgroundThreadsForCompiling(BackendRootWindow * bpe);
		static void toggleCompileScriptsOnPresetLoad(BackendRootWindow * bpe);
		static void createNewProject(BackendRootWindow *bpe);
		static void loadProject(BackendRootWindow *bpe);
		static void closeProject(BackendRootWindow *bpe);
		static void showProjectInFinder(BackendRootWindow *bpe);
		static void saveUserPreset(BackendRootWindow *bpe);
		static void loadUserPreset(BackendRootWindow *bpe, const File &fileToLoad);
		static void toggleRelativePath(BackendRootWindow * bpe);
		static void collectExternalFiles(BackendRootWindow * bpe);
		static void saveFileAsXml(BackendRootWindow * bpe);
		static void openFileFromXml(BackendRootWindow * bpe, const File &fileToLoad);
		static void exportFileAsSnippet(BackendRootWindow* bpe);
		static void redirectSampleFolder(Processor *processorForTheProjectHandler);
		static void showFilePresetSettings(BackendRootWindow * bpe);
		static void showFileProjectSettings(BackendRootWindow * bpe);
		static void showFileUserSettings(BackendRootWindow * bpe);
		static void showFileCompilerSettings(BackendRootWindow * bpe);
		static void checkSettingSanity(BackendRootWindow * bpe);
		static void togglePluginPopupWindow(BackendRootWindow * bpe);
		static void changeCodeFontSize(BackendRootWindow *bpe, bool increase);
		static void createRSAKeys(BackendRootWindow * bpe);
		static void createDummyLicenceFile(BackendRootWindow * bpe);
		static void createDefaultToolbarJSON(BackendRootWindow * bpe);
		static void toggleForcePoolSearch(BackendRootWindow * bpe);
		static void archiveProject(BackendRootWindow * bpe);
		static void downloadNewProject(BackendRootWindow * bpe);
		static void showMainMenu(BackendRootWindow * bpe);
		static void moveModule(CopyPasteTarget *currentCopyPasteTarget, bool moveUp);
		static void createExternalScriptFile(BackendRootWindow * bpe);
		static void exportMainSynthChainAsPlayerLibrary(BackendRootWindow * bpe);
		static void cleanBuildDirectory(BackendRootWindow * bpe);
		static void convertAllSamplesToMonolith(BackendRootWindow * bpe);
		static void convertSfzFilesToSampleMaps(BackendRootWindow * bpe);
		static void checkAllSamplemaps(BackendRootWindow * bpe);
		static void validateUserPresets(BackendRootWindow * bpe);
		static void createBase64State(CopyPasteTarget* target);
		static void createUserInterface(BackendRootWindow * bpe);
	};

private:


	friend class BackendRootWindow;

	ColumnMode currentColumnMode;

	BackendProcessor *owner;

	StringArray menuNames;

	WeakReference<CopyPasteTarget> currentCopyPasteTarget;

	BackendRootWindow *bpe;

	ApplicationCommandManager *mainCommandManager;

	int rootY;

	Array<File> recentFileList;

};


#endif  // BACKENDAPPLICATIONCOMMANDS_H_INCLUDED
