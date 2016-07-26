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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

#define SET_COMMAND_TARGET(result, name, active, ticked, shortcut) { result.setInfo(name, name, "Target", 0); \\
															 result.setActive(active); \\
															 result.setTicked(ticked); \\
															 result.addDefaultKeypress(shortcut, ModifierKeys::commandModifier); }



#define ADD_ALL_PLATFORMS(x)(p.addCommandItem(mainCommandManager, x))

#if HISE_IOS
#define ADD_IOS_ONLY(x)(p.addCommandItem(mainCommandManager, x))
#define ADD_DESKTOP_ONLY(x)
#else
#define ADD_IOS_ONLY(x)()
#define ADD_DESKTOP_ONLY(x)(p.addCommandItem(mainCommandManager, x))
#endif


BackendCommandTarget::BackendCommandTarget(BackendProcessor *owner_):
owner(owner_),
currentColumnMode(OneColumn)
{
	

	createMenuBarNames();
}


void BackendCommandTarget::setEditor(BackendProcessorEditor *editor)
{
	bpe = dynamic_cast<BackendProcessorEditor*>(editor);

	mainCommandManager = owner->getCommandManager();
	mainCommandManager->registerAllCommandsForTarget(this);
	mainCommandManager->getKeyMappings()->resetToDefaultMappings();

	bpe->addKeyListener(mainCommandManager->getKeyMappings());
	mainCommandManager->setFirstCommandTarget(this);
	mainCommandManager->commandStatusChanged();
}

void BackendCommandTarget::getAllCommands(Array<CommandID>& commands)
{
	const CommandID id[] = { 
		HamburgerMenu,
		ModulatorList,
		DebugPanel,
		ViewPanel,
		Mixer,
		Macros,
		Keyboard,
		Settings,
		MenuNewFile,
		MenuOpenFile,
		MenuSaveFile,
		MenuSaveFileAsXmlBackup,
		MenuOpenXmlBackup,
		MenuProjectNew,
		MenuProjectLoad,
		MenuCloseProject,
		MenuFileArchiveProject,
		MenuFileDownloadNewProject,
		MenuProjectShowInFinder,
        MenuFileSaveUserPreset,
		MenuFileSettingsPreset,
		MenuFileSettingsProject,
		MenuFileSettingsUser,
		MenuFileSettingsCompiler,
		MenuFileSettingCheckSanity,
		MenuReplaceWithClipboardContent,
		MenuExportFileAsPlugin,
		MenuExportFileAsSnippet,
		MenuFileQuit,
		MenuEditCopy,
		MenuEditPaste,
		MenuEditMoveUp,
		MenuEditMoveDown,
		MenuEditCreateScriptVariable,
        MenuEditCloseAllChains,
        MenuEditPlotModulator,
		MenuViewShowSelectedProcessorInPopup,
		
		MenuToolsRecompile,
        MenuToolsClearConsole,
		MenuToolsSetCompileTimeOut,
		MenuToolsUseBackgroundThreadForCompile,
		MenuToolsRecompileScriptsOnReload,
		MenuToolsCreateToolbarPropertyDefinition,
		MenuToolsResolveMissingSamples,
		MenuToolsDeleteMissingSamples,
		MenuToolsUseRelativePaths,
		MenuToolsCollectExternalFiles,
        MenuToolsRedirectSampleFolder,
		MenuToolsForcePoolSearch,
		MenuToolsEnableAutoSaving,
		MenuToolsCreateRSAKeys,
		MenuToolsCreateDummyLicenceFile,
        MenuViewFullscreen,
		MenuViewBack,
		MenuViewForward,
        MenuOneColumn,
		MenuTwoColumns,
		MenuThreeColumns,
		MenuViewShowPluginPopupPreview,
        MenuViewIncreaseCodeFontSize,
        MenuViewDecreaseCodeFontSize,
        MenuAddView,
        MenuDeleteView,
        MenuRenameView,
        MenuViewSaveCurrentView,
        MenuToolsCheckDuplicate,
		MenuHelpShowAboutPage,
        MenuHelpCheckVersion
		/*        MenuRevertFile,
		
		
		MenuExportFileAsPlayerLibrary,
		MenuEditOffset,
		MenuViewOffset,
		MenuOneColumn,
		MenuTwoColumns,
		MenuThreeColumns,
		MenuAddView,
		MenuDeleteView,
		MenuRenameView*/
	};

	commands.addArray(id, numElementsInArray(id));
}

void BackendCommandTarget::createMenuBarNames()
{
	menuNames.clear();

	menuNames.add("File");
	menuNames.add("Edit " + (currentCopyPasteTarget.get() == nullptr ? "" : currentCopyPasteTarget->getObjectTypeName()));
	menuNames.add("Tools");
	menuNames.add("View");
	menuNames.add("Help");

	jassert(menuNames.size() == numMenuNames);
}

void BackendCommandTarget::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	switch (commandID)
	{
	case HamburgerMenu:
		setCommandTarget(result, "Show Menu", true, false, 'X', false);
		break;
	case Macros:
		setCommandTarget(result, "Show Macro Controls", true, (bpe->macroKnobs != nullptr && !bpe->macroKnobs->isVisible()), 'X', false);
		break;
	case DebugPanel:
		setCommandTarget(result, "Show Debug Panel", true, (bpe->referenceDebugArea != nullptr && !bpe->referenceDebugArea->isVisible()), 'X', false);
		break;
	case ViewPanel:
		setCommandTarget(result, "Show View Panel", true, true, 'X', false);
		break;
	case Mixer:
		setCommandTarget(result, "Show Mixer", false, true, 'X', false);
		break;
	case Keyboard:
		setCommandTarget(result, "Show Keyboard", true, (bpe->keyboard != nullptr && !bpe->keyboard->isVisible()), 'X', false);
		break;
	case Settings:

#if IS_STANDALONE_APP
		setCommandTarget(result, "Show Audio Device Settings", true, bpe->currentDialog == nullptr, 'X', false);
#else
		setCommandTarget(result, "Show Audio Device Settings (disabled for plugins)", false, bpe->currentDialog == nullptr, '8');
#endif
		break;
	case ModulatorList:
		setCommandTarget(result, "Show Processor List", true, (bpe->popupEditor == nullptr), 'x', false);
		break;
	case MenuNewFile:
		setCommandTarget(result, "New File", true, false, 'N');
		break;
	case MenuOpenFile:
		setCommandTarget(result, "Open File", true, false, 'O');
		break;
	case MenuSaveFile:
		setCommandTarget(result, "Save", true, false, 'S');
		break;
	case MenuSaveFileAsXmlBackup:
		setCommandTarget(result, "Save File as XML Backup", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		break;
	case MenuOpenXmlBackup:
		setCommandTarget(result, "Open XML Backup", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		break;
	case MenuProjectNew:
		setCommandTarget(result, "Create new Project folder", true, false, 'X', false);
		break;
	case MenuProjectLoad:
		setCommandTarget(result, "Load Project", true, false, 'X', false);
		break;
	case MenuCloseProject:
		setCommandTarget(result, "Close Project", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		break;
	case MenuFileArchiveProject:
		setCommandTarget(result, "Archive Project", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		break;
	case MenuFileDownloadNewProject:
		setCommandTarget(result, "Download archived Project", true, false, 'X', false);
		break;
	case MenuProjectShowInFinder:
#if JUCE_WINDOWS
        setCommandTarget(result, "Show Project folder in Explorer",
#else
		setCommandTarget(result, "Show Project folder in Finder",
#endif
        GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		break;
    case MenuFileSaveUserPreset:
        setCommandTarget(result, "Save current state as new User Preset", true, false, 'X', false);
        break;
    case MenuExportFileAsPlugin:
        setCommandTarget(result, "Export as VST/AU plugin", true, false, 'X', false);
        break;
	case MenuExportFileAsSnippet:
		setCommandTarget(result, "Export as pasteable web snippet", true, false, 'X', false);
		break;
	case MenuFileSettingsPreset:
		setCommandTarget(result, "Preset Properties", true, false, 'X', false);
		break;
	case MenuFileSettingsProject:
		setCommandTarget(result, "Project Properties", true, false, 'X', false);
		break;
	case MenuFileSettingsUser:
		setCommandTarget(result, "User Settings", true, false, 'X', false);
		break;
	case MenuFileSettingsCompiler:
		setCommandTarget(result, "Compiler Settings", true, false, 'X', false);
		break;
	case MenuFileSettingCheckSanity:
		setCommandTarget(result, "Check for missing properties", true, false, 'X', false);
		break;
	case MenuReplaceWithClipboardContent:
		setCommandTarget(result, "Replace with clipboard content", true, false, 'X', false);
		break;
	case MenuFileQuit:
		setCommandTarget(result, "Quit", true, false, 'X', false); break;
	case MenuEditCopy:
		setCommandTarget(result, "Copy", currentCopyPasteTarget.get() != nullptr, false, 'C');
		break;
	case MenuEditPaste:
		setCommandTarget(result, "Paste", currentCopyPasteTarget.get() != nullptr, false, 'V');
		break;
	case MenuEditMoveUp:
		setCommandTarget(result, "Move up", currentCopyPasteTarget.get() != nullptr, false, 'X', false);
		result.addDefaultKeypress(KeyPress::upKey, ModifierKeys::ctrlModifier);
		break;
	case MenuEditMoveDown:
		setCommandTarget(result, "Move down", currentCopyPasteTarget.get() != nullptr, false, 'X', false);
		result.addDefaultKeypress(KeyPress::downKey, ModifierKeys::ctrlModifier);
		break;
	case MenuEditCreateScriptVariable:
		setCommandTarget(result, "Create script variable", clipBoardNotEmpty(), false, 'C', true, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
		break;
    case MenuEditPlotModulator:
        {
            ProcessorEditor * editor = dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget.get());
            
            bool active = false;
            
            bool ticked = false;
            
            if(editor != nullptr)
            {
                Modulator *mod = dynamic_cast<Modulator*>(editor->getProcessor());
                
                if(mod != nullptr)
                {
                    active = true;
                    
                    ticked = mod->isPlotted();
                }
            }
            
            setCommandTarget(result, "Plot Modulator", active, ticked, 'P');
            break;
        }

    case MenuEditCloseAllChains:
        setCommandTarget(result, "Close all chains", clipBoardNotEmpty(), false, 'X', false);
        break;
	case MenuToolsRecompile:
        setCommandTarget(result, "Recompile all scripts", true, false, 'X', false);
        break;
	case MenuToolsSetCompileTimeOut:
		setCommandTarget(result, "Change compile time out duration", true, false, 'X', false);
		break;
	case MenuToolsUseBackgroundThreadForCompile:
		setCommandTarget(result, "Use background thread for script compiling", true, bpe->getBackendProcessor()->isUsingBackgroundThreadForCompiling(), 'X', false);
		break;
	case MenuToolsRecompileScriptsOnReload:
		setCommandTarget(result, "Recompile all scripts on preset load", true, bpe->getBackendProcessor()->isCompilingAllScriptsOnPresetLoad(), 'X', false);
		break;
	case MenuToolsCreateToolbarPropertyDefinition:
		setCommandTarget(result, "Create default Toolbar JSON definition", true, false, 'X', false);
		break;
	case MenuToolsDeleteMissingSamples:
		setCommandTarget(result, "Delete missing samples", true, false, 'X', false);
		break;
	case MenuToolsResolveMissingSamples:
		setCommandTarget(result, "Resolve missing samples", true, false, 'X', false);
		break;
	case MenuToolsUseRelativePaths:
		setCommandTarget(result, "Use relative paths to project folder", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), bpe->getBackendProcessor()->getSampleManager().shouldUseRelativePathToProjectFolder(), 'X', false);
		break;
	case MenuToolsCollectExternalFiles:
		setCommandTarget(result, "Collect external files into Project folder", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		break;
    case MenuToolsRedirectSampleFolder:
		setCommandTarget(result, "Redirect sample folder", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(),
			GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isRedirected(ProjectHandler::SubDirectories::Samples), 'X', false);
        break;
	case MenuToolsForcePoolSearch:
		setCommandTarget(result, "Force duplicate search in pool when loading samples", true, bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool()->isPoolSearchForced(), 'X', false);
		break;
	case MenuToolsEnableAutoSaving:
		setCommandTarget(result, "Enable Autosaving", true, bpe->owner->getAutoSaver().isAutoSaving(), 'X', false);
		break;
	case MenuToolsCreateRSAKeys:
		setCommandTarget(result, "Create RSA Key pair", true, false, 'X', false);
		break;
	case MenuToolsCreateDummyLicenceFile:
		setCommandTarget(result, "Create Dummy Licence File", true, false, 'X', false);
		break;
    case MenuViewFullscreen:
        setCommandTarget(result, "Toggle Fullscreen", true, bpe->isFullScreenMode(), 'F');
        break;
	case MenuViewBack:
		setCommandTarget(result, "Back: " + bpe->getViewUndoManager()->getUndoDescription(), bpe->getViewUndoManager()->canUndo(), true, (char)KeyPress::backspaceKey, true, ModifierKeys::noModifiers);
		break;
	case MenuViewForward:
		setCommandTarget(result, "Forward: " + bpe->getViewUndoManager()->getRedoDescription(), bpe->getViewUndoManager()->canRedo(), true, (char)KeyPress::backspaceKey, true, ModifierKeys::shiftModifier);
		break;
	case MenuOneColumn:
		setCommandTarget(result, "One Column", true, currentColumnMode == OneColumn, '1', true, ModifierKeys::altModifier);
		break;
	case MenuTwoColumns:
		setCommandTarget(result, "Two Columns", true, currentColumnMode == TwoColumns, '2', true, ModifierKeys::altModifier);
		break;
	case MenuThreeColumns:
		setCommandTarget(result, "Three Columns", true, currentColumnMode == ThreeColumns, '3', true, ModifierKeys::altModifier);
		break;
	case MenuViewShowPluginPopupPreview:
		setCommandTarget(result, "Open Plugin Preview Window", bpe->isPluginPreviewCreatable(), !bpe->isPluginPreviewShown(), 'X', false);
		break;
    case MenuViewIncreaseCodeFontSize:
        setCommandTarget(result, "Increase code font size", true, false, 'X', false);
        break;
    case MenuViewDecreaseCodeFontSize:
        setCommandTarget(result, "Decrease code font size", true, false, 'X', false);
        break;
    case MenuAddView:
        setCommandTarget(result, "Add new view", true, false, 'X', false);
        break;
    case MenuDeleteView:
        setCommandTarget(result, "Delete current view", viewActive(), false, 'X', false);
        break;
    case MenuRenameView:
        setCommandTarget(result, "Rename current view", viewActive(), false, 'X', false);
        break;
    case MenuViewSaveCurrentView:
        setCommandTarget(result, "Save current view", viewActive(), false, 'X', false);
        break;
	case MenuViewShowSelectedProcessorInPopup:
		setCommandTarget(result, "Show Processor in full screen", dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget.get()) != nullptr, false, 'X', false);
		result.addDefaultKeypress(KeyPress::F11Key, ModifierKeys::noModifiers);
		break;
    case MenuToolsCheckDuplicate:
        setCommandTarget(result, "Check duplicate IDs", true, false, 'X', false);
        break;
    case MenuToolsClearConsole:
        setCommandTarget(result, "Clear Console", true, false, 'X', false);
        break;
	case MenuHelpShowAboutPage:
		setCommandTarget(result, "About HISE", true, false, 'X', false);

		break;
    case MenuHelpCheckVersion:
        setCommandTarget(result, "Check for newer version", true, false, 'X', false);
        break;
            
	default:					jassertfalse; return;
	}
}

bool BackendCommandTarget::perform(const InvocationInfo &info)
{
	switch (info.commandID)
	{
	case HamburgerMenu:					Actions::showMainMenu(bpe);  return true;
	case DebugPanel:                    toggleVisibility(bpe->referenceDebugArea);
                                        bpe->viewedComponentChanged();
                                        return true;
	case Keyboard:                      toggleVisibility(bpe->keyboard);
                                        bpe->viewedComponentChanged();
                                        return true;
	case Macros:                        toggleVisibility(bpe->macroKnobs);
                                        bpe->viewedComponentChanged();
                                        return true;
	case ModulatorList:                 bpe->showModulatorTreePopup(); return true;
	case ViewPanel:                     bpe->showViewPanelPopup(); return true;
	case Settings:                      bpe->showSettingsWindow(); return true;
	case MenuNewFile:                   if (PresetHandler::showYesNoWindow("New File", "Do you want to start a new preset?"))
                                            bpe->clearPreset(); return true;
	case MenuOpenFile:                  Actions::openFile(bpe); return true;
	case MenuSaveFile:                  Actions::saveFile(bpe); return true;
    case MenuSaveFileAsXmlBackup:		Actions::saveFileAsXml(bpe); updateCommands(); return true;
    case MenuOpenXmlBackup:             { FileChooser fc("Select XML file to load",
                                                         GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::XMLPresetBackups), "*.xml", true);
                                        if (fc.browseForFileToOpen()) Actions::openFileFromXml(bpe, fc.getResult()); return true;}
	case MenuProjectNew:				Actions::createNewProject(bpe); updateCommands();  return true;
	case MenuProjectLoad:				Actions::loadProject(bpe); updateCommands(); return true;
	case MenuCloseProject:				Actions::closeProject(bpe); updateCommands(); return true;
	case MenuFileArchiveProject:		Actions::archiveProject(bpe); return true;
	case MenuFileDownloadNewProject:	Actions::downloadNewProject(bpe); return true;
	case MenuProjectShowInFinder:		Actions::showProjectInFinder(bpe); return true;
    case MenuFileSaveUserPreset:        Actions::saveUserPreset(bpe); return true;
	case MenuFileSettingsPreset:		Actions::showFilePresetSettings(bpe); return true;
	case MenuFileSettingsProject:		Actions::showFileProjectSettings(bpe); return true;
	case MenuFileSettingsUser:			Actions::showFileUserSettings(bpe); return true;
	case MenuFileSettingsCompiler:		Actions::showFileCompilerSettings(bpe); return true;
	case MenuFileSettingCheckSanity:	Actions::checkSettingSanity(bpe); return true;
	case MenuReplaceWithClipboardContent: Actions::replaceWithClipboardContent(bpe); return true;
	case MenuFileQuit:                  if (PresetHandler::showYesNoWindow("Quit Application", "Do you want to quit?"))
                                            JUCEApplicationBase::quit(); return true;
	case MenuEditCopy:                  if (currentCopyPasteTarget) currentCopyPasteTarget->copyAction(); return true;
	case MenuEditPaste:                 if (currentCopyPasteTarget) currentCopyPasteTarget->pasteAction(); return true;
	case MenuEditMoveUp:				if (currentCopyPasteTarget) Actions::moveModule(currentCopyPasteTarget, true); return true;
	case MenuEditMoveDown:				if (currentCopyPasteTarget) Actions::moveModule(currentCopyPasteTarget, false); return true;
    case MenuEditCreateScriptVariable:  Actions::createScriptVariableDeclaration(currentCopyPasteTarget); return true;
    case MenuEditPlotModulator:         Actions::plotModulator(currentCopyPasteTarget.get()); updateCommands(); return true;
    case MenuEditCloseAllChains:        Actions::closeAllChains(bpe); return true;
	case MenuToolsRecompile:            Actions::recompileAllScripts(bpe); return true;
	case MenuToolsSetCompileTimeOut:	Actions::setCompileTimeOut(bpe); return true;
	case MenuToolsUseBackgroundThreadForCompile: Actions::toggleUseBackgroundThreadsForCompiling(bpe); updateCommands(); return true;
	case MenuToolsRecompileScriptsOnReload: Actions::toggleCompileScriptsOnPresetLoad(bpe); updateCommands(); return true;
	case MenuToolsCreateToolbarPropertyDefinition:	Actions::createDefaultToolbarJSON(bpe); return true;
    case MenuToolsCheckDuplicate:       Actions::checkDuplicateIds(bpe); return true;
	case MenuToolsDeleteMissingSamples: Actions::deleteMissingSamples(bpe); return true;
	case MenuToolsResolveMissingSamples:Actions::resolveMissingSamples(bpe); return true;
	case MenuToolsUseRelativePaths:		Actions::toggleRelativePath(bpe); updateCommands();  return true;
	case MenuToolsCollectExternalFiles:	Actions::collectExternalFiles(bpe); return true;
    case MenuToolsRedirectSampleFolder: Actions::redirectSampleFolder(bpe->getMainSynthChain()); updateCommands(); return true;
	case MenuToolsForcePoolSearch:		Actions::toggleForcePoolSearch(bpe); updateCommands(); return true;
	case MenuToolsCreateRSAKeys:		Actions::createRSAKeys(bpe); return true;
	case MenuToolsCreateDummyLicenceFile: Actions::createDummyLicenceFile(bpe); return true;
    case MenuViewFullscreen:            Actions::toggleFullscreen(bpe); updateCommands(); return true;
	case MenuViewBack:					bpe->getViewUndoManager()->undo(); updateCommands(); return true;
	case MenuViewForward:				bpe->getViewUndoManager()->redo(); updateCommands(); return true;
	case MenuViewShowPluginPopupPreview: Actions::togglePluginPopupWindow(bpe); updateCommands(); return true;
    case MenuViewIncreaseCodeFontSize:  Actions::changeCodeFontSize(bpe, true); return true;
    case MenuViewDecreaseCodeFontSize:   Actions::changeCodeFontSize(bpe, false); return true;
    case MenuExportFileAsPlugin:        CompileExporter::exportMainSynthChainAsPackage(owner->getMainSynthChain()); return true;
    case MenuExportFileAsSnippet:       Actions::exportFileAsSnippet(bpe); return true;
    case MenuAddView:                   Actions::addView(bpe); updateCommands();return true;
    case MenuDeleteView:                Actions::deleteView(bpe); updateCommands();return true;
    case MenuRenameView:                Actions::renameView(bpe); updateCommands();return true;
    case MenuViewSaveCurrentView:       Actions::saveView(bpe); updateCommands(); return true;
    case MenuToolsClearConsole:         owner->clearConsole(); return true;
	case MenuHelpShowAboutPage:			Actions::showAboutPage(bpe); return true;
    case MenuHelpCheckVersion:          Actions::checkVersion(bpe); return true;
	case MenuOneColumn:					Actions::setColumns(bpe, this, OneColumn);  updateCommands(); return true;
	case MenuTwoColumns:				Actions::setColumns(bpe, this, TwoColumns);  updateCommands(); return true;
	case MenuThreeColumns:				Actions::setColumns(bpe, this, ThreeColumns);  updateCommands(); return true;
	case MenuToolsEnableAutoSaving:		bpe->owner->getAutoSaver().toggleAutoSaving(); updateCommands(); return true;
	case MenuViewShowSelectedProcessorInPopup: Actions::showProcessorInPopup(bpe, dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget.get())); return true;
	}

	return false;
}



PopupMenu BackendCommandTarget::getMenuForIndex(int topLevelMenuIndex, const String &/*menuName*/)
{
	MenuNames m = (MenuNames)topLevelMenuIndex;

	PopupMenu p;

	switch (m)
	{
	case BackendCommandTarget::FileMenu: {
		ADD_ALL_PLATFORMS(MenuNewFile);

		ADD_DESKTOP_ONLY(MenuOpenFile);
		ADD_ALL_PLATFORMS(MenuSaveFile);
		ADD_ALL_PLATFORMS(MenuReplaceWithClipboardContent);

		PopupMenu filesInProject;

		if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
		{
			GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(recentFileList, ProjectHandler::SubDirectories::Presets, "*.hip", true);

			for (int i = 0; i < recentFileList.size(); i++)
			{
				filesInProject.addItem(MenuOpenFileFromProjectOffset+i, recentFileList[i].getFileNameWithoutExtension(), true, false);
			}
		}

		p.addSubMenu("Open File from Project", filesInProject, filesInProject.getNumItems() != 0);

		p.addSeparator();

		ADD_ALL_PLATFORMS(MenuProjectNew);
		ADD_DESKTOP_ONLY(MenuProjectLoad);
		ADD_DESKTOP_ONLY(MenuCloseProject);
		ADD_DESKTOP_ONLY(MenuProjectShowInFinder);

		PopupMenu recentProjects;

#if HISE_IOS

		Array<File> results;

		File userDataDirectory = File::getSpecialLocation(File::userDocumentsDirectory);

		userDataDirectory.findChildFiles(results, File::findDirectories, false);

		String currentProject = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getWorkDirectory().getFullPathName();
		
		const String menuTitle = "Available Projects";

		for (int i = 0; i < results.size(); i++)
		{
			recentProjects.addItem(MenuProjectRecentOffset + i, results[i].getFileName(), true, results[i].getFullPathName() == currentProject);
		}

#else

		StringArray recentProjectDirectories = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getRecentWorkDirectories();

		const String menuTitle = "Recent Projects";

		String currentProject = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getWorkDirectory().getFullPathName();

		for (int i = 0; i < recentProjectDirectories.size(); i++)
		{
			recentProjects.addItem(MenuProjectRecentOffset + i, recentProjectDirectories[i], true, currentProject == recentProjectDirectories[i]);
		}

#endif

		p.addSubMenu(menuTitle, recentProjects);
        
        p.addSeparator();

		ADD_DESKTOP_ONLY(MenuFileArchiveProject);
		ADD_ALL_PLATFORMS(MenuFileDownloadNewProject);

		p.addSeparator();

        ADD_ALL_PLATFORMS(MenuFileSaveUserPreset);
        
        PopupMenu userPresets;
        Array<File> userPresetFiles;
        
        ProjectHandler *handler = &GET_PROJECT_HANDLER(bpe->getMainSynthChain());
        
        handler->getFileList(userPresetFiles, ProjectHandler::SubDirectories::UserPresets, "*.preset");
            
        for(int i = 0; i < userPresetFiles.size(); i++)
            userPresets.addItem(i + MenuFileUserPresetMenuOffset, userPresetFiles[i].getFileName());
        
        
        p.addSubMenu("User Presets", userPresets);
        p.addSeparator();
        
		ADD_ALL_PLATFORMS(MenuOpenXmlBackup);
		ADD_ALL_PLATFORMS(MenuSaveFileAsXmlBackup);

        PopupMenu xmlBackups;
        Array<File> xmlBackupFiles;
        
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(xmlBackupFiles, ProjectHandler::SubDirectories::XMLPresetBackups, "*.xml");
        
        for(int i = 0; i < xmlBackupFiles.size(); i++)
        {
            xmlBackups.addItem(i + MenuFileXmlBackupMenuOffset, xmlBackupFiles[i].getFileName());
        }
        
        p.addSubMenu("Load XML Backup from Project", xmlBackups);


#if HISE_IOS
#else
		p.addSeparator();

		PopupMenu settingsSub;

		settingsSub.addCommandItem(mainCommandManager, MenuFileSettingsPreset);
		settingsSub.addCommandItem(mainCommandManager, MenuFileSettingsProject);
		settingsSub.addCommandItem(mainCommandManager, MenuFileSettingsUser);
		settingsSub.addCommandItem(mainCommandManager, MenuFileSettingsCompiler);
		settingsSub.addSeparator();
		settingsSub.addCommandItem(mainCommandManager, MenuFileSettingCheckSanity);

		p.addSubMenu("Settings", settingsSub);


		PopupMenu exportSub;

        exportSub.addCommandItem(mainCommandManager, MenuExportFileAsPlugin);
		exportSub.addItem(4, "Export as HISE Player library");
        exportSub.addCommandItem(mainCommandManager, MenuExportFileAsSnippet);

		p.addSubMenu("Export", exportSub);
		p.addSeparator();
		ADD_ALL_PLATFORMS(MenuFileQuit);
#endif

		break; }
	case BackendCommandTarget::EditMenu:
        if(dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get()))
        {
            dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get())->addPopupMenuItems(p, nullptr);
            
        }
		else if (dynamic_cast<SampleMapEditor*>(mainCommandManager->getFirstCommandTarget(SampleMapEditor::Undo)))
		{
			dynamic_cast<SampleMapEditor*>(mainCommandManager->getFirstCommandTarget(SampleMapEditor::Undo))->fillPopupMenu(p);
		}
        else
        {
            ADD_ALL_PLATFORMS(MenuEditCopy);
            ADD_ALL_PLATFORMS(MenuEditPaste);
            p.addSeparator();

			ADD_ALL_PLATFORMS(MenuEditMoveUp);
			ADD_ALL_PLATFORMS(MenuEditMoveDown);
            
            const int chainOffset = 0x6000;
            
            ProcessorEditor * editor = dynamic_cast<ProcessorEditor*>(bpe->currentCopyPasteTarget.get());
            if(editor != nullptr)
            {
                for(int i = 0; i < editor->getProcessor()->getNumInternalChains(); i++)
                {
                    Processor *child =  editor->getProcessor()->getChildProcessor(i);
                    
                    p.addItem(chainOffset, "Show " + child->getId(), true, child->getEditorState(Processor::EditorState::Visible));
                }
            }
            
            ADD_ALL_PLATFORMS(MenuEditCreateScriptVariable);
            ADD_ALL_PLATFORMS(MenuEditCloseAllChains);
            ADD_DESKTOP_ONLY(MenuEditPlotModulator);
        }
		break;
	case BackendCommandTarget::ToolsMenu:
	{
		p.addSectionHeader("Scripting Tools");
		ADD_ALL_PLATFORMS(MenuToolsRecompile);
		ADD_ALL_PLATFORMS(MenuToolsCheckDuplicate);
		ADD_ALL_PLATFORMS(MenuToolsClearConsole);
		ADD_DESKTOP_ONLY(MenuToolsRecompileScriptsOnReload);
		ADD_DESKTOP_ONLY(MenuToolsSetCompileTimeOut);
		ADD_DESKTOP_ONLY(MenuToolsUseBackgroundThreadForCompile);
		ADD_DESKTOP_ONLY(MenuToolsCreateToolbarPropertyDefinition);

		PopupMenu sub;

		Array<File> files;
		StringArray processors;

		bpe->getBackendProcessor()->fillExternalFileList(files, processors);

		for (int i = 0; i < files.size(); i++)
		{
			sub.addItem(MenuToolsExternalScriptFileOffset + i, processors[i] + ": " + files[i].getFileName());
		}

		p.addSubMenu("Edit external script files", sub, files.size() != 0);

		p.addSeparator();
		p.addSectionHeader("Sample Management");
		ADD_DESKTOP_ONLY(MenuToolsResolveMissingSamples);
		ADD_DESKTOP_ONLY(MenuToolsDeleteMissingSamples);
		ADD_DESKTOP_ONLY(MenuToolsUseRelativePaths);
		ADD_DESKTOP_ONLY(MenuToolsCollectExternalFiles);
		ADD_DESKTOP_ONLY(MenuToolsRedirectSampleFolder);
		ADD_DESKTOP_ONLY(MenuToolsForcePoolSearch);
		ADD_DESKTOP_ONLY(MenuToolsEnableAutoSaving);
		p.addSeparator();
		p.addSectionHeader("Licence Management");
		ADD_DESKTOP_ONLY(MenuToolsCreateDummyLicenceFile);
		ADD_DESKTOP_ONLY(MenuToolsCreateRSAKeys);
		break;
	}
	case BackendCommandTarget::ViewMenu: {
		ADD_ALL_PLATFORMS(MenuViewBack);
		ADD_ALL_PLATFORMS(MenuViewForward);
		p.addSeparator();
		ADD_DESKTOP_ONLY(MenuViewFullscreen);
		ADD_ALL_PLATFORMS(MenuViewShowSelectedProcessorInPopup);
		p.addSeparator();
		ADD_DESKTOP_ONLY(MenuOneColumn);
		ADD_DESKTOP_ONLY(MenuTwoColumns);
		ADD_DESKTOP_ONLY(MenuThreeColumns);
        p.addSeparator();
        ADD_ALL_PLATFORMS(MenuViewIncreaseCodeFontSize);
        ADD_ALL_PLATFORMS(MenuViewDecreaseCodeFontSize);
		p.addSeparator();
		ADD_ALL_PLATFORMS(MenuViewShowPluginPopupPreview);
		ADD_ALL_PLATFORMS(DebugPanel);
		ADD_ALL_PLATFORMS(Macros);
		ADD_ALL_PLATFORMS(Keyboard);
		ADD_ALL_PLATFORMS(Settings);
		p.addSeparator();
		ADD_ALL_PLATFORMS(MenuAddView);
		ADD_ALL_PLATFORMS(MenuDeleteView);
		ADD_ALL_PLATFORMS(MenuRenameView);
		ADD_ALL_PLATFORMS(MenuViewSaveCurrentView);

		if (viewActive())
		{
			p.addSeparator();
			p.addSectionHeader("Current View: " + owner->synthChain->getCurrentViewInfo()->getViewName());

			for (int i = 0; i < owner->synthChain->getNumViewInfos(); i++)
			{
				ViewInfo *info = owner->synthChain->getViewInfo(i);

				p.addItem(MenuViewOffset + i, info->getViewName(), true, info == owner->synthChain->getCurrentViewInfo());
			}

		}

		PopupMenu processorList;

		Processor::Iterator<Processor> iter(owner->getMainSynthChain());

		int i = 0;

		while (Processor *pl = iter.getNextProcessor())
		{
			if (ProcessorHelpers::is<ModulatorChain>(pl)) continue;
			if (ProcessorHelpers::is<MidiProcessorChain>(pl)) continue;
			if (ProcessorHelpers::is<EffectProcessorChain>(pl)) continue;

			processorList.addItem(MenuViewProcessorListOffset + i, pl->getId());

			i++;
		}

		if (processorList.containsAnyActiveItems())
		{
			p.addSeparator();
			p.addSubMenu("Solo Processor", processorList);
		}

		break;
		}
	case BackendCommandTarget::HelpMenu:
			ADD_ALL_PLATFORMS(MenuHelpShowAboutPage);
			ADD_DESKTOP_ONLY(MenuHelpCheckVersion);
		break;
	default:
		break;
	}

	return p;
}



void BackendCommandTarget::menuItemSelected(int menuItemID, int /*topLevelMenuIndex*/)
{
	if (menuItemID >= MenuOpenFileFromProjectOffset && menuItemID < ((int)(MenuOpenFileFromProjectOffset) + 50))
	{
        const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
        
        if (!shouldDiscard) return;
        
		if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
		{
			File f = recentFileList[menuItemID - (int)MenuOpenFileFromProjectOffset];

			if (f.existsAsFile())
			{
				bpe->loadNewContainer(f);
			}
		}
	}
	else if (menuItemID >= MenuProjectRecentOffset && menuItemID < (int)MenuProjectRecentOffset + 12)
	{
        const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
        
        if (!shouldDiscard) return;
        
		const int index = menuItemID - MenuProjectRecentOffset;

		if (PresetHandler::showYesNoWindow("Switch projects?", "Do you want to switch projects? The current preset will be cleared"))
		{
#if HISE_IOS
			Array<File> results;

			File userDataDirectory = File::getSpecialLocation(File::userDocumentsDirectory);

			userDataDirectory.findChildFiles(results, File::findDirectories, false);

			File file = results[index];

			bpe->clearPreset();

			GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(file);

#else

			String file = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getRecentWorkDirectories()[index];

			bpe->clearPreset();

			GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(file);
            
            menuItemsChanged();
#endif
		}
	}
    else if (menuItemID >= MenuFileUserPresetMenuOffset && menuItemID < ((int)MenuFileUserPresetMenuOffset+50))
    {
        const int index = menuItemID - MenuFileUserPresetMenuOffset;
        
        Array<File> userPresetFiles;
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(userPresetFiles, ProjectHandler::SubDirectories::UserPresets, "*.preset");

        File presetToLoad = userPresetFiles[index];
		Actions::loadUserPreset(bpe, presetToLoad);
    }
    else if (menuItemID >= MenuFileXmlBackupMenuOffset && menuItemID < ((int)MenuFileXmlBackupMenuOffset+50))
    {
        const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
        
        if (!shouldDiscard) return;
        
        const int index = menuItemID - MenuFileXmlBackupMenuOffset;
        
        Array<File> xmlFileList;
        
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(xmlFileList, ProjectHandler::SubDirectories::XMLPresetBackups, "*.xml");
        
		File presetToLoad = xmlFileList[index];
            
        Actions::openFileFromXml(bpe, presetToLoad);
    }
    
	else if (menuItemID >= MenuViewOffset && menuItemID < (int)MenuViewOffset + 200)
	{
		ViewInfo *info = owner->synthChain->getViewInfo(menuItemID - MenuViewOffset);

		if (info != nullptr)
		{
			info->restore();

			bpe->setRootProcessor(info->getRootProcessor());

			owner->synthChain->setCurrentViewInfo(menuItemID - MenuViewOffset);
		}
	}

	else if (menuItemID >= MenuViewProcessorListOffset && menuItemID < (int)MenuViewProcessorListOffset + 200)
	{
		Processor::Iterator<Processor> iter(owner->synthChain);

		int i = 0;

		const int indexToLookFor = (menuItemID - (int)MenuViewProcessorListOffset);

		while (Processor *p = iter.getNextProcessor())
		{
			if (ProcessorHelpers::is<ModulatorChain>(p)) continue;
			if (ProcessorHelpers::is<MidiProcessorChain>(p)) continue;
			if (ProcessorHelpers::is<EffectProcessorChain>(p)) continue;

			if (i == indexToLookFor)
			{
				bpe->showProcessorPopup(p, ProcessorHelpers::findParentProcessor(p, false));
			}

			i++;
		}
	}
	else if (menuItemID >= MenuToolsExternalScriptFileOffset && menuItemID < (MenuToolsExternalScriptFileOffset + 50))
	{
		Array<File> files;
		StringArray processors;

		bpe->getBackendProcessor()->fillExternalFileList(files, processors);

		File f = files[menuItemID - MenuToolsExternalScriptFileOffset];

		Processor::Iterator<JavascriptProcessor> iter(bpe->getMainSynthChain());

		while (JavascriptProcessor *sp = iter.getNextProcessor())
		{
			for (int i = 0; i < sp->getNumWatchedFiles(); i++)
			{
				if (sp->getWatchedFile(i) == f)
				{
					sp->showPopupForFile(i);
					return;
				}
			}
		}

		jassertfalse;
	}
}



bool BackendCommandTarget::Actions::hasProcessorInClipboard()
{
	ScopedPointer<XmlElement> xml = XmlDocument::parse(SystemClipboard::getTextFromClipboard());

	if (xml != nullptr)
	{
		ValueTree v = ValueTree::fromXml(*xml);

		if (v.isValid() && v.getProperty("Type") == "SynthChain")
		{
			return true;
			
		}
	}

	return false;
}
                         
      
bool BackendCommandTarget::Actions::hasSnippetInClipboard()
{
    String clipboardContent = SystemClipboard::getTextFromClipboard();
 
    return clipboardContent.startsWith("HiseSnippet ");
}

void BackendCommandTarget::Actions::openFile(BackendProcessorEditor *bpe)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
#if HISE_IOS

	Array<File> fileList;

	GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Presets).findChildFiles(fileList, File::findFiles, false, "*.hip");

	MainMenuWithViewPort *menu = new MainMenuWithViewPort();

	menu->getContainer()->addFileIds(fileList, bpe);

	bpe->showPseudoModalWindow(menu, "Load File from Project", true);

#else

	FileChooser fc("Load Preset File", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Presets), "*.hip", true);

	if (fc.browseForFileToOpen()) bpe->loadNewContainer(fc.getResult());
#endif
}

void BackendCommandTarget::Actions::saveFile(BackendProcessorEditor *bpe)
{
	if (PresetHandler::showYesNoWindow("Save " + bpe->owner->getMainSynthChain()->getId(), "Do you want to save this preset?"))
	{
		PresetHandler::saveProcessorAsPreset(bpe->owner->getMainSynthChain());
	}
}

void BackendCommandTarget::Actions::replaceWithClipboardContent(BackendProcessorEditor *bpe)
{
	String clipboardContent = SystemClipboard::getTextFromClipboard();

    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	if (hasSnippetInClipboard())
	{
		String data = clipboardContent.fromFirstOccurrenceOf("HiseSnippet ", false, false);

		MemoryBlock mb;

		mb.fromBase64Encoding(data);

		MemoryInputStream mis(mb, false);

		GZIPDecompressorInputStream dezipper(&mis, false);

		ValueTree v = ValueTree::readFromGZIPData(mb.getData(), mb.getSize());

		if (v.isValid())
		{
			bpe->loadNewContainer(v);
			return;
		}
	}
	else
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(clipboardContent);

		if (xml != nullptr)
		{
			ValueTree v = ValueTree::fromXml(*xml);

			if (v.isValid() && v.getProperty("Type") == "SynthChain")
			{
				bpe->loadNewContainer(v);
				return;
			}
		}
	}
    
	PresetHandler::showMessageWindow("Invalid Preset", "The clipboard does not contain a valid container / snippet.", PresetHandler::IconType::Warning);
}

void BackendCommandTarget::Actions::createScriptVariableDeclaration(CopyPasteTarget *currentCopyPasteTarget)
{
	ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget);

	if (editor != nullptr)
	{
		ProcessorHelpers::getScriptVariableDeclaration(editor->getProcessor());
	}
}

void BackendCommandTarget::Actions::recompileAllScripts(BackendProcessorEditor * bpe)
{
	bpe->owner->compileAllScripts();
}

void BackendCommandTarget::Actions::toggleFullscreen(BackendProcessorEditor * bpe)
{
#if IS_STANDALONE_APP
    
    Component *window = bpe->getParentComponent()->getParentComponent();
    
    if (bpe->isFullScreenMode())
    {
        Desktop::getInstance().setKioskModeComponent(nullptr);
        
        const int height = Desktop::getInstance().getDisplays().getMainDisplay().userArea.getHeight() - 70;
        bpe->setSize(bpe->referenceDebugArea->isVisible() ? 1280 : 900, height);
        bpe->resized();
        
        window->centreWithSize(bpe->getWidth(), bpe->getHeight());
        
        bpe->setAlwaysOnTop(false);
        bpe->borderDragger->setVisible(true);
    }
    else
    {
        Desktop::getInstance().setKioskModeComponent(window);
        
        bpe->borderDragger->setVisible(false);
        bpe->setAlwaysOnTop(true);
        
        bpe->setSize(Desktop::getInstance().getDisplays().getMainDisplay().totalArea.getWidth(),
                Desktop::getInstance().getDisplays().getMainDisplay().totalArea.getHeight());
        
        bpe->resized();
        
    }
#endif
}

void BackendCommandTarget::Actions::addView(BackendProcessorEditor *bpe)
{
    String view = PresetHandler::getCustomName("View");
    
    ViewInfo *info = new ViewInfo(bpe->owner->synthChain, bpe->currentRootProcessor, view);
    
    bpe->owner->synthChain->addViewInfo(info);
}

void BackendCommandTarget::Actions::deleteView(BackendProcessorEditor *bpe)
{
    bpe->owner->synthChain->removeCurrentViewInfo();
}

void BackendCommandTarget::Actions::renameView(BackendProcessorEditor *bpe)
{
    String view = PresetHandler::getCustomName("View");
    
    bpe->owner->synthChain->getCurrentViewInfo()->setViewName(view);
}

void BackendCommandTarget::Actions::saveView(BackendProcessorEditor *bpe)
{
    String view = bpe->owner->synthChain->getCurrentViewInfo()->getViewName();
    
    ViewInfo *info = new ViewInfo(bpe->owner->synthChain, bpe->currentRootProcessor, view);
    
    bpe->owner->synthChain->replaceCurrentViewInfo(info);
}

void BackendCommandTarget::Actions::closeAllChains(BackendProcessorEditor *bpe)
{
    ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(bpe->currentCopyPasteTarget.get());
    
    if(editor != nullptr)
    {
        editor->getChainBar()->closeAll();
    }
}

void BackendCommandTarget::Actions::checkDuplicateIds(BackendProcessorEditor *bpe)
{
    PresetHandler::checkProcessorIdsForDuplicates(bpe->owner->synthChain, false);

}

void BackendCommandTarget::Actions::showAboutPage(BackendProcessorEditor * bpe)
{
	bpe->aboutPage->showAboutPage();
}

void BackendCommandTarget::Actions::checkVersion(BackendProcessorEditor *bpe)
{
    if (areMajorWebsitesAvailable())
    {
        UpdateChecker * checker = new UpdateChecker();
        
        checker->setModalBaseWindowComponent(bpe);
    }
    else
    {
        PresetHandler::showMessageWindow("Offline", "Could not connect to the server", PresetHandler::IconType::Warning);
    }
}

void BackendCommandTarget::Actions::setColumns(BackendProcessorEditor * bpe, BackendCommandTarget* target, ColumnMode columns)
{
	target->currentColumnMode = columns;

	switch (columns)
	{
	case BackendCommandTarget::OneColumn:
		bpe->setSize(900, bpe->getHeight());
		break;
	case BackendCommandTarget::TwoColumns:
		bpe->setSize(jmin<int>(Desktop::getInstance().getDisplays().getMainDisplay().totalArea.getWidth(), 1280), bpe->getHeight());
		break;
	case BackendCommandTarget::ThreeColumns:
		bpe->setSize(jmin<int>(Desktop::getInstance().getDisplays().getMainDisplay().totalArea.getWidth(), 1650), bpe->getHeight());
		break;
	default:
		break;
	}

	bpe->resized();
	
}

void BackendCommandTarget::Actions::showProcessorInPopup(BackendProcessorEditor * bpe, ProcessorEditor* editor)
{
	bpe->showProcessorPopup(editor->getProcessor(), editor->getParentEditor() != nullptr ? editor->getParentEditor()->getProcessor() : nullptr);
}

void BackendCommandTarget::Actions::plotModulator(CopyPasteTarget *currentCopyPasteTarget)
{
    ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget);
                                                                         
    if(editor != nullptr)
    {
        Modulator *mod = dynamic_cast<Modulator*>(editor->getProcessor());
        if(mod != nullptr)
        {
            if(mod->isPlotted())
            {
                mod->getMainController()->removePlottedModulator(mod);
            }
            else
            {
                mod->getMainController()->addPlottedModulator(mod);
            }
        }
    }
}

void BackendCommandTarget::Actions::resolveMissingSamples(BackendProcessorEditor *bpe)
{
	bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool()->resolveMissingSamples(bpe);
}

void BackendCommandTarget::Actions::deleteMissingSamples(BackendProcessorEditor *bpe)
{
	bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool()->deleteMissingSamples();
}

void BackendCommandTarget::Actions::setCompileTimeOut(BackendProcessorEditor * bpe)
{
	AlertWindowLookAndFeel alaf;

	AlertWindow newTime("Enter new compile time out duration", "Current time out: " + String(bpe->getBackendProcessor()->getCompileTimeOut(),1) + " seconds.", AlertWindow::QuestionIcon, bpe);

	newTime.setLookAndFeel(&alaf);

	newTime.addTextEditor("time", String(bpe->getBackendProcessor()->getCompileTimeOut(), 1), "", false);
	
	newTime.addButton("OK", 1, KeyPress(KeyPress::returnKey));
	newTime.addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	if (newTime.runModalLoop())
	{
		double value = newTime.getTextEditor("time")->getText().getDoubleValue();

		if (value != 0.0)
		{
			bpe->getBackendProcessor()->setCompileTimeOut(value);
		}
	}
}

void BackendCommandTarget::Actions::toggleUseBackgroundThreadsForCompiling(BackendProcessorEditor * bpe)
{
	const bool lastState = bpe->getBackendProcessor()->isUsingBackgroundThreadForCompiling();

	bpe->getBackendProcessor()->setShouldUseBackgroundThreadForCompiling(!lastState);
}

void BackendCommandTarget::Actions::toggleCompileScriptsOnPresetLoad(BackendProcessorEditor * bpe)
{
	const bool lastState = bpe->getBackendProcessor()->isCompilingAllScriptsOnPresetLoad();

	bpe->getBackendProcessor()->setEnableCompileAllScriptsOnPresetLoad(!lastState);
}


void BackendCommandTarget::Actions::toggleRelativePath(BackendProcessorEditor * bpe)
{
	const bool state = bpe->getBackendProcessor()->getSampleManager().shouldUseRelativePathToProjectFolder();

	bpe->getBackendProcessor()->getSampleManager().setShouldUseRelativePathToProjectFolder(!state);
}


void BackendCommandTarget::Actions::collectExternalFiles(BackendProcessorEditor * bpe)
{
	ExternalResourceCollector *resource = new ExternalResourceCollector(bpe->getBackendProcessor());

	resource->setModalBaseWindowComponent(bpe);
}


void BackendCommandTarget::Actions::exportFileAsSnippet(BackendProcessorEditor* bpe)
{
	ValueTree v = bpe->getMainSynthChain()->exportAsValueTree();

	MemoryOutputStream mos;

	v.writeToStream(mos);

	MemoryOutputStream mos2;

	GZIPCompressorOutputStream zipper(&mos2, 9);
	
	zipper.write(mos.getData(), mos.getDataSize());
	zipper.flush();

	String data = "HiseSnippet " + mos2.getMemoryBlock().toBase64Encoding();

	SystemClipboard::copyTextToClipboard(data);

	PresetHandler::showMessageWindow("Preset copied as compressed snippet", "You can paste the clipboard content to share this preset", PresetHandler::IconType::Info);
}

void BackendCommandTarget::Actions::saveFileAsXml(BackendProcessorEditor * bpe)
{
	if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
	{
		FileChooser fc("Select XML file to save", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::XMLPresetBackups), "*.xml", true);

		if (fc.browseForFileToSave(true))
		{
			ValueTree v = bpe->owner->getMainSynthChain()->exportAsValueTree();

            v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
            
			ScopedPointer<XmlElement> xml = v.createXml();

			if(PresetHandler::showYesNoWindow("Strip Editor View information", "Do you want to strip the editor view information? This reduces the noise when using version controls systems."))
			{
				XmlBackupFunctions::removeEditorStatesFromXml(*xml);
			}

			if (PresetHandler::showYesNoWindow("Write Scripts to external file?", "Do you want to write the scripts into an external file (They will be imported when loading this backup"))
			{
				File scriptDirectory = XmlBackupFunctions::getScriptDirectoryFor(bpe->getMainSynthChain());

				Processor::Iterator<JavascriptProcessor> iter(bpe->getMainSynthChain());

				scriptDirectory.deleteRecursively();

				scriptDirectory.createDirectory();

				while (JavascriptProcessor *sp = iter.getNextProcessor())
				{
					String content;

					sp->mergeCallbacksToScript(content);

					File scriptFile = XmlBackupFunctions::getScriptFileFor(bpe->getMainSynthChain(), dynamic_cast<Processor*>(sp)->getId());

					scriptFile.replaceWithText(content);
				}

				XmlBackupFunctions::removeAllScripts(*xml);
			}

			fc.getResult().replaceWithText(xml->createDocument(""));

			debugToConsole(bpe->owner->getMainSynthChain(), "Exported as XML");
            
            
		}
	}
}

void BackendCommandTarget::Actions::openFileFromXml(BackendProcessorEditor * bpe, const File &fileToLoad)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		String newId = xml->getStringAttribute("ID");

		XmlBackupFunctions::restoreAllScripts(*xml, bpe->getMainSynthChain(), newId);

		ValueTree v = ValueTree::fromXml(*xml);

		bpe->loadNewContainer(v);
        
        
	}
}

void BackendCommandTarget::Actions::createNewProject(BackendProcessorEditor *bpe)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	FileChooser fc("Create new project directory");

	if (fc.browseForDirectory())
	{
		File f = fc.getResult();

		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).createNewProject(f);

		bpe->getBackendProcessor()->createUserPresetData();
	}
}

void BackendCommandTarget::Actions::loadProject(BackendProcessorEditor *bpe)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
#if HISE_IOS

	Array<File> fileList;
	
	File::getSpecialLocation(File::userDocumentsDirectory).findChildFiles(fileList, File::findDirectories, false);


	MainMenuWithViewPort *newMenu = new MainMenuWithViewPort();

	newMenu->getContainer()->addFileIds(fileList, bpe);
	
	bpe->showPseudoModalWindow(newMenu, "Open a project from the list", true);

#else
	FileChooser fc("Load project (set as working directory)");

	if (fc.browseForDirectory())
	{
		File f = fc.getResult();

		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(f);

		bpe->getBackendProcessor()->createUserPresetData();
	}
#endif
}

void BackendCommandTarget::Actions::closeProject(BackendProcessorEditor *bpe)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(File::nonexistent);


	bpe->getBackendProcessor()->createUserPresetData();

}

void BackendCommandTarget::Actions::showProjectInFinder(BackendProcessorEditor *bpe)
{
	if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
	{
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getWorkDirectory().revealToUser();
	}
}

void BackendCommandTarget::Actions::saveUserPreset(BackendProcessorEditor *bpe)
{
    UserPresetHandler::saveUserPreset(bpe->getMainSynthChain());
}

void BackendCommandTarget::Actions::loadUserPreset(BackendProcessorEditor *bpe, const File &fileToLoad)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
    UserPresetHandler::loadUserPreset(bpe->getMainSynthChain(), fileToLoad);
}

void BackendCommandTarget::Actions::redirectSampleFolder(Processor *processorForTheProjectHandler)
{
    FileChooser fc("Redirect sample folder to the following location");
    
    if (fc.browseForDirectory())
    {
        File f = fc.getResult();
        
		GET_PROJECT_HANDLER(processorForTheProjectHandler).createLinkFile(ProjectHandler::SubDirectories::Samples, f);
    }
}

void BackendCommandTarget::Actions::showFilePresetSettings(BackendProcessorEditor * /*bpe*/)
{
	
}

void BackendCommandTarget::Actions::showFileProjectSettings(BackendProcessorEditor * bpe)
{
	SettingWindows::ProjectSettingWindow *window = new SettingWindows::ProjectSettingWindow(&GET_PROJECT_HANDLER(bpe->getMainSynthChain()));

	window->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::showFileUserSettings(BackendProcessorEditor * bpe)
{
	SettingWindows::UserSettingWindow *window = new SettingWindows::UserSettingWindow();

	window->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::showFileCompilerSettings(BackendProcessorEditor * bpe)
{
	SettingWindows::CompilerSettingWindow *window = new SettingWindows::CompilerSettingWindow();

	window->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::checkSettingSanity(BackendProcessorEditor * bpe)
{
	SettingWindows::checkAllSettings(&GET_PROJECT_HANDLER(bpe->getMainSynthChain()));
}

void BackendCommandTarget::Actions::togglePluginPopupWindow(BackendProcessorEditor * bpe)
{
	if (bpe->isPluginPreviewShown())
	{
		bpe->setPluginPreviewWindow(nullptr);
	}
	else
	{
#if HISE_IOS
		bpe->showPseudoModalWindow(new PluginPreviewWindow::Content(bpe), bpe->getMainSynthChain()->getId(), true);
#else
		bpe->setPluginPreviewWindow(new PluginPreviewWindow(bpe));
#endif
	}
}
                    
void BackendCommandTarget::Actions::changeCodeFontSize(BackendProcessorEditor *bpe, bool increase)
{
    float currentFontSize = bpe->getMainSynthChain()->getMainController()->getGlobalCodeFontSize();
    
    if(increase)
    {
        currentFontSize = jmin<float>(28.0f, currentFontSize + 1.0f);
    }
    else
    {
        currentFontSize = jmax<float>(10.0f, currentFontSize - 1.0f);
    }
    
    bpe->getMainSynthChain()->getMainController()->setGlobalCodeFontSize(currentFontSize);
}

void BackendCommandTarget::Actions::createRSAKeys(BackendProcessorEditor * bpe)
{
	GET_PROJECT_HANDLER(bpe->getMainSynthChain()).createRSAKey();
}


void BackendCommandTarget::Actions::createDummyLicenceFile(BackendProcessorEditor * bpe)
{
	ProjectHandler *handler = &GET_PROJECT_HANDLER(bpe->getMainSynthChain());

	if (!handler->isActive())
	{
		PresetHandler::showMessageWindow("No Project active", "You need an active project to create a key file.", PresetHandler::IconType::Warning);
		return;
	}

	const String appName = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Name, handler);

	const String version = SettingWindows::getSettingValue((int)SettingWindows::ProjectSettingWindow::Attributes::Version, handler);

	if (appName.isEmpty() || version.isEmpty())
	{
		PresetHandler::showMessageWindow("No Product name", "You need a product name for a licence file.", PresetHandler::IconType::Warning);
		return;
	}

	const String productName = appName + " " + version;

	const String dummyEmail = "dummy@email.com";
	const String userName = "Dummy McLovin";

	StringArray ids;

#if JUCE_WINDOWS
	OnlineUnlockStatus::MachineIDUtilities::addFileIDToList(ids, File::getSpecialLocation(File::windowsSystemDirectory));
#else
	OnlineUnlockStatus::MachineIDUtilities::addFileIDToList(ids, File("~"));
#endif

	// ..if that fails, use the MAC addresses..
	if (ids.size() == 0)
		OnlineUnlockStatus::MachineIDUtilities::addMACAddressesToList(ids);

	RSAKey privateKey = RSAKey(handler->getPrivateKey());

	if (!privateKey.isValid())
	{
		PresetHandler::showMessageWindow("No RSA key", "You have to create a RSA Key pair first.", PresetHandler::IconType::Warning);
		return;
	}

	String keyContent = KeyGeneration::generateKeyFile(productName, dummyEmail, userName, ids.joinIntoString("\n"), privateKey);
	File key = handler->getWorkDirectory().getChildFile(productName + ProjectHandler::Frontend::getLicenceKeyExtension());

	key.replaceWithText(keyContent);

#if JUCE_WINDOWS
#if JUCE_64BIT
	const String message = "A dummy licence file for 64bit plugins was created.\nTo load 32bit plugins, please use the 32bit version of HISE to create the licence file";
#else
	const String message = "A dummy licence file for 32bit plugins was created.\nTo load 64bit plugins, please use the 64bit version of HISE to create the licence file";
#endif
#else
	const String message = "A dummy licence file for the plugins was created.";
#endif

	PresetHandler::showMessageWindow("Licence File created", message, PresetHandler::IconType::Info);
}

void BackendCommandTarget::Actions::createDefaultToolbarJSON(BackendProcessorEditor * bpe)
{
	String json = FrontendBar::createJSONString(bpe->getBackendProcessor()->getToolbarPropertiesObject());

	String clipboard = "var toolbarData = ";
	
	clipboard << json;

	clipboard << ";\n\nContent.setToolbarProperties(toolbarData);";

	SystemClipboard::copyTextToClipboard(clipboard);

	PresetHandler::showMessageWindow("JSON Data copied to clipboard", 
		"The current toolbar properties are copied into the clipboard.\nPaste it into any script and change the data", 
		PresetHandler::IconType::Info);

}

void BackendCommandTarget::Actions::toggleForcePoolSearch(BackendProcessorEditor * bpe)
{
	ModulatorSamplerSoundPool *pool = bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool();

	pool->setForcePoolSearch(!pool->isPoolSearchForced());
}


void BackendCommandTarget::Actions::archiveProject(BackendProcessorEditor * bpe)
{
	ProjectHandler *handler = &GET_PROJECT_HANDLER(bpe->getMainSynthChain());

	if (handler->isRedirected(ProjectHandler::SubDirectories::Samples))
	{
		if (PresetHandler::showYesNoWindow("Sample Folder is redirected", 
										   "The sample folder is redirected to another location.\nIt will not be included in the archive. Press OK to continue or cancel to abort", 
										   PresetHandler::IconType::Warning))
			return;
	}

	FileChooser fc("Select archive destination", File::nonexistent, "*.zip");

	if (fc.browseForFileToSave(true))
	{
		File archiveFile = fc.getResult();

		File projectDirectory = handler->getWorkDirectory();

		new ProjectArchiver(archiveFile, projectDirectory, bpe->getBackendProcessor());

	}
}

void BackendCommandTarget::Actions::downloadNewProject(BackendProcessorEditor * bpe)
{
	ProjectDownloader *downloader = new ProjectDownloader(bpe);

	downloader->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::showMainMenu(BackendProcessorEditor * bpe)
{
	const int commandIDs[] = { MenuNewFile,
		MenuOpenFile,
		MenuSaveFile,
		0,
		MenuReplaceWithClipboardContent,
		MenuExportFileAsSnippet,
		0,
		MenuFileDownloadNewProject,
		MenuProjectLoad,
		0,
		MenuEditCopy,
		MenuEditPaste,
		0,
		MainToolbarCommands::Settings};

	Array<int> ids;
	ids.addArray(commandIDs, numElementsInArray(commandIDs));

	MainMenuContainer *newMenu = new MainMenuContainer();

	newMenu->addCommandIds(bpe->mainCommandManager, ids);

	bpe->showPseudoModalWindow(newMenu, "Main Menu", true);

}

void BackendCommandTarget::Actions::moveModule(CopyPasteTarget *currentCopyPasteTarget, bool moveUp)
{
	if (ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget))
	{
		Processor *processor = editor->getProcessor();
		ProcessorEditor *parentEditor = editor->getParentEditor();

		if (parentEditor == nullptr) return;

		if (Chain *c = parentEditor->getProcessorAsChain())
		{
			c->getHandler()->moveProcessor(editor->getProcessor(), moveUp ? -1 : 1);
			editor->childEditorAmountChanged();

			BackendProcessorEditor *bpe = editor->findParentComponentOfClass<BackendProcessorEditor>();
			bpe->refreshContainer(processor);
		}
	}
}

#undef ADD_ALL_PLATFORMS
#undef ADD_IOS_ONLY
#undef ADD_DESKTOP_ONLY
#undef toggleVisibility