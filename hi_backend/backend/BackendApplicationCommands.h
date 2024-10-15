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

#ifndef BACKENDAPPLICATIONCOMMANDS_H_INCLUDED
#define BACKENDAPPLICATIONCOMMANDS_H_INCLUDED

namespace hise { using namespace juce;

class BackendRootWindow;

class BackendCommandTarget: public ApplicationCommandTarget,
							public MenuBarModel,
	                        public CopyPasteTargetHandler
{
public:

	BackendCommandTarget(BackendProcessor *owner);

	enum MenuNames
	{
		FileMenu = 0,
		EditMenu,
		ExportMenu,
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
		
		numToolbarButtons,

		// FILE MENU
		MenuProjectNew = 0x20000,
		MenuProjectLoad,
		MenuProjectShowInFinder,
		MenuFileShowHiseAppDataFolder,
		MenuFileShowProjectAppDataFolder,
		MenuProjectRecentOffset,
		// ------------------------
		MenuSnippetFileNew = 0x22000,
		MenuNewFile,
		MenuOpenXmlBackup,
		MenuSaveFileXmlBackup,
		MenuSaveFileAsXmlBackup,
		MenuFileXmlBackupMenuOffset,
		// --------------------------
		MenuOpenFile = 0x23000,
		MenuSaveFile,
		MenuSaveFileAs,
		MenuOpenFileFromProjectOffset,
		// ---------------------------
		MenuFileImportSnippet = 0x24000,
		MenuFileExtractEmbeddeSnippetFiles,
		MenuFileCreateRecoveryXml,
		MenuSnippetClose,
		// --------------------------------
		MenuFileSettings,
		MenuToolsEditShortcuts,
		// --------------------------------
		MenuFileQuit,
		MenuFileImportProjectFromHXI,
		
		MenuRevertFile = 0x26000,
        
		// Export Menu
		MenuExportSetupWizard,
		MenuExportFileAsPlugin,
		MenuExportFileAsEffectPlugin,
		MenuExportFileAsMidiFXPlugin,
		MenuExportFileAsStandaloneApp,
		// ------------------------------------
		MenuExportFileAsSnippet,
		MenuExportProjectAsExpansion,
        // --------------------------------------
		MenuExportCheckAllSampleMaps,
		MenuExportCheckPluginParameters,
		MenuExportValidateUserPresets,
		MenuExportCheckUnusedImages,
		// --------------------------------------
		MenuExportRestoreToDefault,
		MenuExportUnloadAllSampleMaps,
		MenuExportUnloadAllAudioFiles,
		MenuExportCleanBuildDirectory,
		MenuExportCleanDspNetworkFiles,
		// --------------------------------------
		MenuExportSampleDataForInstaller,
		MenuExportCompileFilesInPool,
		MenuExportCompileNetworksAsDll,

		// Edit Menu
		MenuEditOffset = 0x30000,
		MenuEditUndo,
		MenuEditRedo,
		MenuEditCopy,
		MenuEditPaste,
		MenuEditMoveUp,
		MenuEditMoveDown,
        MenuEditCopyAsSnippet,
        MenuEditPasteAsSnippet,
        MenuEditPlotModulator,
		MenuEditCreateScriptVariable,
		MenuEditCreateBase64State,
        MenuEditCloseAllChains,

		// View Menu
		MenuViewGotoUndo,
        MenuViewGotoRedo,
		// ----------------------
		MenuViewToggleSnippetBrowser,
		MenuViewRotate,
		MenuViewEnableGlobalLayoutMode,
		// -----------------------------
		WorkspaceScript,
		WorkspaceSampler,
		WorkspaceCustom,
		MenuViewAddFloatingWindow,
		// --------------------------------
		MenuViewClearConsole,
		MenuViewResetLookAndFeel,
		MenuViewReset,

		// Tools Menu
		// Scripting Tools
		MenuToolsRecompile = 0x50000,
		MenuToolsCheckCyclicReferences,
		MenuToolsConvertSVGToPathData,
		MenuToolsBroadcasterWizard,
		MenuToolsCreateExternalScriptFile,
		
		// ---------------------------------
		// Sample Management
		MenuToolsApplySampleMapProperties,
		MenuToolsImportArchivedSamples,
		MenuToolsForcePoolSearch,
		MenuToolsConvertAllSamplesToMonolith,
		MenuToolsUpdateSampleMapIdsBasedOnFileName,
		MenuToolsConvertSfzToSampleMaps,
		// ----------------------------------
		// Wavetable Tools
		MenuToolsConvertSampleMapToWavetableBanks,
		MenuToolsWavetablesToMonolith,
		// ----------------------------------
		// DSP Tools
		MenuToolsEnableDebugLogging,
		MenuToolsShowDspNetworkDllInfo,
		MenuToolsRecordOneSecond,
		MenuToolsSimulateChangingBufferSize,
        MenuToolsCreateRnboTemplate,
		MenuToolsCreateThirdPartyNode,
		MenuToolsCreateGlobalCableCppCode,
		// ----------------------------------
		// License Management
		MenuToolsCreateRSAKeys,
		MenuToolsCreateDummyLicenseFile,

		// HELP Menu
		MenuHelpShowDocumentation  = 0x70000,
		MenuFileBrowseExamples,
		MenuHelpCheckVersion,
		MenuHelpShowAboutPage,
        
		numCommands
	};

    struct Updater: public juce::ApplicationCommandManagerListener
    {
        Updater(BackendCommandTarget& parent_):
          parent(parent_)
        {
            parent.mainCommandManager->addListener(this);
        }
        
        ~Updater()
        {
            if(parent.mainCommandManager)
                parent.mainCommandManager->removeListener(this);
        }
        
        void applicationCommandInvoked (const ApplicationCommandTarget::InvocationInfo&) override
        {
            
        }
        
        void applicationCommandListChanged() override
        {
            parent.menuItemsChanged();
        }
        
        BackendCommandTarget& parent;
    };
    
    ScopedPointer<Updater> updater;
    
	virtual ~BackendCommandTarget()
	{
        updater = nullptr;
        
		if(mainCommandManager != nullptr)
        {
			mainCommandManager->setFirstCommandTarget(nullptr);
        }

		CopyPasteTarget::setHandlerFunction(nullptr);
	};

	ApplicationCommandTarget* getNextCommandTarget() override
	{
		return findFirstTargetParentComponent();
	};

   
    
	void setCommandTarget(ApplicationCommandInfo &result, const String &name, bool active, bool ticked, char shortcut, bool useShortCut=true, ModifierKeys mod=ModifierKeys::commandModifier) {
		result.setInfo(name, name, "Unused", 0);
		result.setActive(active); 
		result.setTicked(ticked);

		if (useShortCut) result.addDefaultKeypress(shortcut, mod);
	};

	bool clipBoardNotEmpty() const { return SystemClipboard::getTextFromClipboard().isNotEmpty(); }
    
	void setEditor(BackendRootWindow *editor);

	void getAllCommands(Array<CommandID>& commands) override;;

	void getCommandInfo(CommandID commandID, ApplicationCommandInfo &result) override;;

	bool perform(const InvocationInfo &info) override;
	
	void updateCommands();

	void setCopyPasteTarget(CopyPasteTarget *newTarget);

	void createMenuBarNames();

	StringArray getMenuBarNames() override { return menuNames; };
	PopupMenu getMenuForIndex(int topLevelMenuIndex, const String &menuName) override;
	void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;;

	ColumnMode getColumnMode() const noexcept { return currentColumnMode; }

	class Actions
	{
	public:

		
		static void editShortcuts(BackendRootWindow* bpe);
		static bool hasProcessorInClipboard();
		static bool hasSnippetInClipboard();
		static void openFile(BackendRootWindow *bpe);
		static void saveFile(BackendRootWindow *bpe, bool forceRename);
		static void replaceWithClipboardContent(BackendRootWindow *bpe);

		static void loadSnippet(BackendRootWindow *bpe, const String& snippet);

		static void createScriptVariableDeclaration(CopyPasteTarget *currentCopyPasteTarget);
		static void recompileAllScripts(BackendRootWindow * bpe);
		static void resetLookAndFeel(BackendRootWindow* bpe);
		static void closeAllChains(BackendRootWindow *bpe);
		
		static void showAboutPage(BackendRootWindow * bpe);
		static void checkVersion(BackendRootWindow *bpe);
		static void plotModulator(CopyPasteTarget *currentCopyPasteTarget);
		static void resolveMissingSamples(BackendRootWindow *bpe);
		static void setCompileTimeOut(BackendRootWindow * bpe);
		static void toggleUseBackgroundThreadsForCompiling(BackendRootWindow * bpe);
		static void toggleCompileScriptsOnPresetLoad(BackendRootWindow * bpe);
		static void createNewProject(BackendRootWindow *bpe);
		static void loadProject(BackendRootWindow *bpe);
		static DialogWindowWithBackgroundThread* importProject(BackendRootWindow* bpe);

		static void extractProject(BackendRootWindow* bpe, const File& newProjectRoot, const File& sourceFile);

        static void createRnboTemplate(BackendRootWindow* bpe);
		static void convertSVGToPathData(BackendRootWindow* bpe);

		static void applySampleMapProperties(BackendRootWindow* bpe);

		static void loadFirstXmlAfterProjectSwitch(BackendRootWindow * bpe);


		static void showAppDataFolder(BackendRootWindow* bpe, bool getProjectAppData);

		static void showProjectInFinder(BackendRootWindow *bpe);
		
		static void loadUserPreset(BackendRootWindow *bpe, const File &fileToLoad);
		static void saveFileXml(BackendRootWindow * bpe);
		static void saveFileAsXml(BackendRootWindow * bpe);
		static void openFileFromXml(BackendRootWindow * bpe, const File &fileToLoad);
		static String exportFileAsSnippet(BackendRootWindow* bpe, bool copyToClipboard=true);
		
		static void showFileProjectSettings(BackendRootWindow * bpe);
		static void togglePluginPopupWindow(BackendRootWindow * bpe);
		static void changeCodeFontSize(BackendRootWindow *bpe, bool increase);
		static void createRSAKeys(BackendRootWindow * bpe);
		static void createDummyLicenseFile(BackendRootWindow * bpe);
		static void toggleForcePoolSearch(BackendRootWindow * bpe);
		static void showMainMenu(BackendRootWindow * bpe);
		static void moveModule(CopyPasteTarget *currentCopyPasteTarget, bool moveUp);
		static void createExternalScriptFile(BackendRootWindow * bpe);
		static void exportHiseProject(BackendRootWindow * bpe);
		static Result exportInstrumentExpansion(BackendProcessor* bp);
		static Result createSampleArchive(BackendProcessor* bp);

		static void compileNetworksToDll(BackendRootWindow* bpe);
		static void cleanBuildDirectory(BackendRootWindow * bpe);
		static void convertAllSamplesToMonolith(BackendRootWindow * bpe);
		static void convertSfzFilesToSampleMaps(BackendRootWindow * bpe);
		static void checkAllSamplemaps(BackendRootWindow * bpe);
		static void validateUserPresets(BackendRootWindow * bpe);
		static void createBase64State(CopyPasteTarget* target);
		static void createUserInterface(BackendRootWindow * bpe);
		static void checkUnusedImages(BackendRootWindow * bpe);
		static void updateSampleMapIds(BackendRootWindow * bpe);
		static void toggleCallStackEnabled(BackendRootWindow * bpe);
		static void testPlugin(const String& pluginToLoad);
        static void checkPluginParameterSanity(BackendRootWindow* bpe);
		static void newFile(BackendRootWindow* bpe);

		static void removeAllSampleMaps(BackendRootWindow * bpe);
		static void redirectScriptFolder(BackendRootWindow * bpe);
		static void exportSampleDataForInstaller(BackendRootWindow * bpe);
		static void exportWavetablesToMonolith(BackendRootWindow* bpe);
		static void importArchivedSamples(BackendRootWindow * bpe);
		static void checkCyclicReferences(BackendRootWindow * bpe);
		static void unloadAllAudioFiles(BackendRootWindow * bpe);
		

		static String createWindowsInstallerTemplate(MainController* mc, bool includeAAX, bool include32, bool include64, bool includeVST2, bool includeVST3);
		static void convertSampleMapToWavetableBanks(BackendRootWindow* bpe);
		static void exportCompileFilesInPool(BackendRootWindow* bpe);
		
		static void copyMissingSampleListToClipboard(BackendRootWindow * bpe);
		static void createRecoveryXml(BackendRootWindow * bpe);
		static void showDocWindow(BackendRootWindow * bpe);
		static void showNetworkDllInfo(BackendRootWindow * bpe);

		static void createThirdPartyNode(BackendRootWindow* bpe);
		static void restoreToDefault(BackendRootWindow * bpe);

		static void extractEmbeddedFilesFromSnippet(BackendRootWindow* bpe);

		static void showExampleBrowser(BackendRootWindow* bpe);

		static void setupExportWizard(BackendRootWindow* bpe);

		static void exportProject(BackendRootWindow* bpe, int buildOption);

		static void cleanDspNetworkFiles(BackendRootWindow* bpe);

		static void createGlobalCableCppCode(BackendRootWindow* bpe);

		static void exportAudio(BackendRootWindow* bpe);
	};

private:

	CopyPasteTarget::HandlerFunction handlerFunction;

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

struct XmlBackupFunctions
{
	static void removeEditorStatesFromXml(XmlElement &xml);

	static XmlElement* getFirstChildElementWithAttribute(XmlElement* parent, const String& attributeName, const String& value);

	static void addContentFromSubdirectory(XmlElement& xml, const File& fileToLoad);

	static void extractContentData(XmlElement& xml, const String& interfaceId, const File& xmlFile);

	static void removeAllScripts(XmlElement &xml);

	static void restoreAllScripts(ValueTree &v, ModulatorSynthChain *masterChain, const String &newId);

	static File getScriptDirectoryFor(ModulatorSynthChain *masterChain, const String &chainId = String());

	static File getScriptFileFor(ModulatorSynthChain *, File& directory, const String &id);

private:

	static String getSanitiziedName(const String &id);
};

struct GitHashManager
{
	static void checkHash(const String& hashToUse, const std::function<void(const var&)>& finishCallbackWithNextHash);
};

} // namespace hise

#endif  // BACKENDAPPLICATIONCOMMANDS_H_INCLUDED
