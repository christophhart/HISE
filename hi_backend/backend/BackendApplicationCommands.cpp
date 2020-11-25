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

namespace hise { using namespace juce;

BackendCommandTarget::BackendCommandTarget(BackendProcessor *owner_):
owner(owner_),
currentColumnMode(OneColumn)
{
	CopyPasteTargetHandler* h = this;

	handlerFunction.f = [h](Component*)
	{
		return h;
	};

	CopyPasteTarget::setHandlerFunction(&handlerFunction);

	createMenuBarNames();
}


void BackendCommandTarget::setEditor(BackendRootWindow *editor)
{
	bpe = dynamic_cast<BackendRootWindow*>(editor);

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
		Settings,
		WorkspaceMain,
		WorkspaceScript,
		WorkspaceSampler,
		WorkspaceCustom,
		MenuNewFile,
		MenuOpenFile,
		MenuSaveFile,
		MenuSaveFileAs,
		MenuSaveFileXmlBackup,
		MenuSaveFileAsXmlBackup,
		MenuOpenXmlBackup,
		MenuProjectNew,
		MenuProjectLoad,
		MenuCloseProject,
		MenuFileCreateRecoveryXml,
		MenuFileArchiveProject,
		MenuFileDownloadNewProject,
		MenuProjectShowInFinder,
        MenuFileSaveUserPreset,
		MenuFileSettingsPreset,
		MenuFileSettingsProject,
		MenuFileSettingsUser,
		MenuFileSettingsCompiler,
		MenuFileSettingCheckSanity,
		MenuFileSettingsCleanBuildDirectory,
		MenuReplaceWithClipboardContent,
		MenuExportFileAsPlugin,
		MenuExportFileAsEffectPlugin,
		MenuExportFileAsMidiFXPlugin,
		MenuExportFileAsStandaloneApp,
		MenuExportFileAsPlayerLibrary,
		MenuExportFileAsSnippet,
		MenuExportSampleDataForInstaller,
		MenuExportCompileFilesInPool,
		MenuFileQuit,
		MenuEditUndo,
		MenuEditRedo,
		MenuEditCopy,
		MenuEditPaste,
		MenuEditMoveUp,
		MenuEditMoveDown,
		MenuEditCreateScriptVariable,
		MenuEditCreateBase64State,
        MenuEditCloseAllChains,
        MenuEditPlotModulator,
		MenuViewShowSelectedProcessorInPopup,
		
		MenuToolsRecompile,
		MenuToolsCreateInterface,
        MenuToolsClearConsole,
		MenuToolsSetCompileTimeOut,
		MenuToolsUseBackgroundThreadForCompile,
		MenuToolsRecompileScriptsOnReload,
		MenuToolsEnableCallStack,
		MenuToolsCheckCyclicReferences,
		MenuToolsCreateToolbarPropertyDefinition,
		MenuToolsCreateExternalScriptFile,
		MenuToolsCreateUIDataFromDesktop,
		MenuToolsCheckDeviceSanity,
		MenuToolsValidateUserPresets,
		MenuToolsResolveMissingSamples,
		MenuToolsDeleteMissingSamples,
		MenuToolsCheckAllSampleMaps,
		MenuToolsCollectExternalFiles,
		MenuToolsCheckUnusedImages,
		MenuToolsGetMissingSampleList,
		MenuToolsRedirectScriptFolder,
		MenuToolsForcePoolSearch,
		MenuToolsConvertSampleMapToWavetableBanks,
		MenuToolsConvertAllSamplesToMonolith,
		MenuToolsUpdateSampleMapIdsBasedOnFileName,
		MenuToolsConvertSfzToSampleMaps,
		MenuToolsRemoveAllSampleMaps,
		MenuToolsUnloadAllAudioFiles,
		MenuToolsRecordOneSecond,
		MenuToolsEnableDebugLogging,
		MenuToolsImportArchivedSamples,
		MenuToolsCreateRSAKeys,
		MenuToolsCreateDummyLicenseFile,
		MenuViewResetLookAndFeel,
		MenuViewReset,
        MenuViewFullscreen,
		MenuViewBack,
		MenuViewForward,
		MenuViewEnableGlobalLayoutMode,
		MenuViewAddFloatingWindow,
		MenuViewAddInterfacePreview,
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
        MenuToolsSanityCheck,
		MenuHelpShowAboutPage,
        MenuHelpCheckVersion,
		MenuHelpShowDocumentation,
		MenuHelpShowHelpForComponents
	};

	commands.addArray(id, numElementsInArray(id));
}

void BackendCommandTarget::createMenuBarNames()
{
	menuNames.clear();

	menuNames.add("File");
	menuNames.add("Edit " + (currentCopyPasteTarget.get() == nullptr ? "" : currentCopyPasteTarget->getObjectTypeName()));
	menuNames.add("Export");
	menuNames.add("Tools");
	menuNames.add("View");
	
	menuNames.add("Help");

	jassert(menuNames.size() == numMenuNames);
}

void BackendCommandTarget::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	result.categoryName = "Hidden";

	switch (commandID)
	{
	case HamburgerMenu:
		setCommandTarget(result, "Show Menu", true, false, 'X', false);
		break;
	case Settings:

#if IS_STANDALONE_APP
		setCommandTarget(result, "Show Audio Device Settings", true, false, 'X', false);
#else
		setCommandTarget(result, "Show Audio Device Settings (disabled for plugins)", false, bpe->currentDialog == nullptr, '8');
#endif
		break;

	case WorkspaceMain:
	{
		setCommandTarget(result, "Show Main Workspace", true, bpe->getCurrentWorkspace() == WorkspaceMain, 'X', false);
		result.categoryName = "View";
		break;
	}
	case WorkspaceScript:
	{
		setCommandTarget(result, "Show Scripting Workspace", true, bpe->getCurrentWorkspace() == WorkspaceScript, 'X', false);
		result.categoryName = "View";
		break;
	}
	case WorkspaceSampler:
	{
		setCommandTarget(result, "Show Sampler Workspace", true, bpe->getCurrentWorkspace() == WorkspaceSampler, 'X', false);
		result.categoryName = "View";
		break;
	}
	case WorkspaceCustom:
	{
		setCommandTarget(result, "Show Custom Workspace", true, bpe->getCurrentWorkspace() == WorkspaceCustom, 'X', false);
		break;
	}
	case MenuNewFile:
		setCommandTarget(result, "New", true, false, 'N');
		result.categoryName = "File";
		break;
	case MenuOpenFile:
		setCommandTarget(result, "Open Archive", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuSaveFile:
		setCommandTarget(result, "Save Archive", true, false, 'S');
		result.categoryName = "File";
		break;
	case MenuSaveFileAs:
		setCommandTarget(result, "Save As Archive", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuSaveFileXmlBackup:
	  	setCommandTarget(result, "Save XML (NOT WORKING)", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'S', true, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
		result.categoryName = "File";
		break;
	case MenuSaveFileAsXmlBackup:
		setCommandTarget(result, "Save as XML", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuOpenXmlBackup:
		setCommandTarget(result, "Open XML", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'O');
		result.categoryName = "File";
		break;
	case MenuProjectNew:
		setCommandTarget(result, "Create new Project folder", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuProjectLoad:
		setCommandTarget(result, "Load Project", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuCloseProject:
		setCommandTarget(result, "Close Project", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuFileArchiveProject:
		setCommandTarget(result, "Archive Project", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuFileDownloadNewProject:
		setCommandTarget(result, "Download archived Project", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuProjectShowInFinder:
#if JUCE_WINDOWS
        setCommandTarget(result, "Show Project folder in Explorer",
#else
		setCommandTarget(result, "Show Project folder in Finder",
#endif
        GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "File";
		break;
    case MenuFileSaveUserPreset:
        setCommandTarget(result, "Save current state as new User Preset", true, false, 'X', false);
		result.categoryName = "File";
        break;
    case MenuExportFileAsPlugin:
        setCommandTarget(result, "Export as Instrument (VSTi / AUi) plugin", true, false, 'X', false);
		result.categoryName = "Export";
        break;
	case MenuExportFileAsEffectPlugin:
		setCommandTarget(result, "Export as FX plugin", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuExportFileAsStandaloneApp:
		setCommandTarget(result, "Export as Standalone Application", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuExportFileAsPlayerLibrary:
		setCommandTarget(result, "Export as HISE Player Library", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuExportFileAsMidiFXPlugin:
		setCommandTarget(result, "Export as MIDI FX plugin", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuExportFileAsSnippet:
		setCommandTarget(result, "Export as HISE Snippet", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuFileCreateRecoveryXml:
		setCommandTarget(result, "Create recovery XML from HIP", true, false, 'x', false);
		result.categoryName = "File";
		break;
	case MenuExportSampleDataForInstaller:
		setCommandTarget(result, "Export Samples for Installer", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuExportCompileFilesInPool:
		setCommandTarget(result, "Export Pooled Files to Binary Resource", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuFileSettingsPreset:
		setCommandTarget(result, "Preset Properties", true, false, 'X', false);
		
		break;
	case MenuFileSettingsProject:
		setCommandTarget(result, "Preferences", true, false, 'X', false);
		result.categoryName = "File";
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
	case MenuFileSettingsCleanBuildDirectory:
		setCommandTarget(result, "Clean Build directory", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuReplaceWithClipboardContent:
		setCommandTarget(result, "Import HISE Snippet", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuFileQuit:
		setCommandTarget(result, "Quit", true, false, 'X', false);
		result.categoryName = "File";
	case MenuEditUndo:
		setCommandTarget(result, "Undo: " + bpe->owner->getControlUndoManager()->getUndoDescription(), bpe->owner->getControlUndoManager()->canUndo(), false, 'Z', true, ModifierKeys::commandModifier);
		result.categoryName = "Edit";
		break;
	case MenuEditRedo:
		setCommandTarget(result, "Redo: " + bpe->owner->getControlUndoManager()->getRedoDescription(), bpe->owner->getControlUndoManager()->canRedo(), false, 'Y', true, ModifierKeys::commandModifier);
		result.categoryName = "Edit";
		break;
	case MenuEditCopy:
		setCommandTarget(result, "Copy", currentCopyPasteTarget.get() != nullptr, false, 'C');
		result.categoryName = "Edit";
		break;
	case MenuEditPaste:
		setCommandTarget(result, "Paste", currentCopyPasteTarget.get() != nullptr, false, 'V');
		result.categoryName = "Edit";
		break;
	case MenuEditMoveUp:
		setCommandTarget(result, "Move up", currentCopyPasteTarget.get() != nullptr, false, 'X', false);
		result.categoryName = "Edit";
		result.addDefaultKeypress(KeyPress::upKey, ModifierKeys::ctrlModifier);
		break;
	case MenuEditMoveDown:
		setCommandTarget(result, "Move down", currentCopyPasteTarget.get() != nullptr, false, 'X', false);
		result.categoryName = "Edit";
		result.addDefaultKeypress(KeyPress::downKey, ModifierKeys::ctrlModifier);
		break;
	case MenuEditCreateScriptVariable:
		setCommandTarget(result, "Create script variable", currentCopyPasteTarget.get() != nullptr, false, 'C', true, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
		result.categoryName = "Edit";
		break;
	case MenuEditCreateBase64State:
		setCommandTarget(result, "Create Base64 encoded state", currentCopyPasteTarget.get() != nullptr, false, 'C', false);
		result.categoryName = "Edit";
		break;
    case MenuEditPlotModulator:
        {
            ProcessorEditor * editor = dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget.get());
            
            bool active = false;
            
            bool ticked = false;
            
            if(editor != nullptr)
            {
                auto mod = dynamic_cast<Modulation*>(editor->getProcessor());
                
                if(mod != nullptr)
                {
                    active = true;
                    
                    ticked = mod->isPlotted();
                }
            }
            
            setCommandTarget(result, "Plot Modulator", active, ticked, 'P');
			result.categoryName = "Edit";
            break;
        }

    case MenuEditCloseAllChains:
        setCommandTarget(result, "Close all chains", clipBoardNotEmpty(), false, 'X', false);
		result.categoryName = "Edit";
        break;
	case MenuToolsCreateInterface:
		setCommandTarget(result, "Create User Interface", true, false, 'X', false);
		break;
	case MenuToolsRecompile:
                         setCommandTarget(result, "Recompile all scripts", true, false, 'X', false);
                         result.addDefaultKeypress(KeyPress::F5Key, ModifierKeys::shiftModifier);
						 result.categoryName = "Tools";
        break;
	case MenuToolsSetCompileTimeOut:
		setCommandTarget(result, "Change compile time out duration", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsUseBackgroundThreadForCompile:
		setCommandTarget(result, "Use background thread for script compiling", true, bpe->getBackendProcessor()->isUsingBackgroundThreadForCompiling(), 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsEnableCallStack:
		setCommandTarget(result, "Enable Scripting Call Stack ", true, bpe->getBackendProcessor()->isCallStackEnabled(), 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCheckCyclicReferences:
		setCommandTarget(result, "Check Javascript objects for cyclic references", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsRecompileScriptsOnReload:
		setCommandTarget(result, "Recompile all scripts on preset load", true, bpe->getBackendProcessor()->isCompilingAllScriptsOnPresetLoad(), 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCreateToolbarPropertyDefinition:
		setCommandTarget(result, "Create default Toolbar JSON definition", true, false, 'X', false);
		break;
	case MenuToolsCreateExternalScriptFile:
		setCommandTarget(result, "Create external script file", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCreateUIDataFromDesktop:
		setCommandTarget(result, "Copy UI Data from Desktop", Helpers::canCopyDeviceType(bpe), Helpers::deviceTypeHasUIData(bpe), 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCheckDeviceSanity:
		setCommandTarget(result, "Check Sanity for defined Devices", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsValidateUserPresets:
		setCommandTarget(result, "Validate User Presets", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsDeleteMissingSamples:
		setCommandTarget(result, "Delete missing samples", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsResolveMissingSamples:
		setCommandTarget(result, "Resolve missing samples", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsGetMissingSampleList:
		setCommandTarget(result, "Copy list of missing samples to clipboard", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCheckAllSampleMaps:
		setCommandTarget(result, "Check all sample maps", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsImportArchivedSamples:
		setCommandTarget(result, "Import archived samples", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCollectExternalFiles:
		setCommandTarget(result, "Collect external files into Project folder", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCheckUnusedImages:
		setCommandTarget(result, "Check for unreferenced images", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "Tools";
		break;
    
	case MenuToolsRedirectScriptFolder:
		setCommandTarget(result, "Redirect script folder", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsForcePoolSearch:
		setCommandTarget(result, "Force duplicate search in pool when loading samples", true, bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool()->isPoolSearchForced(), 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsConvertAllSamplesToMonolith:
		setCommandTarget(result, "Convert all samples to Monolith + Samplemap", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsConvertSampleMapToWavetableBanks:
		setCommandTarget(result, "Convert samplemap to Wavetable Bank", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsUpdateSampleMapIdsBasedOnFileName:
		setCommandTarget(result, "Update SampleMap Ids based on file names", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsConvertSfzToSampleMaps:
		setCommandTarget(result, "Convert SFZ files to SampleMaps", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsRemoveAllSampleMaps:
		setCommandTarget(result, "Clear all Samplemaps", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsUnloadAllAudioFiles:
		setCommandTarget(result, "Unload all audio files", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsEnableDebugLogging:
		setCommandTarget(result, "Enable Debug Logger", true, bpe->owner->getDebugLogger().isLogging(), 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsRecordOneSecond:
		setCommandTarget(result, "Record one second audio file", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCreateRSAKeys:
		setCommandTarget(result, "Create RSA Key pair", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCreateDummyLicenseFile:
		setCommandTarget(result, "Create Dummy License File", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuViewResetLookAndFeel:
		setCommandTarget(result, "Reset custom Look and Feel", true, false, 'X', false);
		result.categoryName = "View";
		break;
	case MenuViewReset:
		setCommandTarget(result, "Reset Workspaces", true, false, 'X', false);
		result.categoryName = "View";
		break;
    case MenuViewFullscreen:
        setCommandTarget(result, "Toggle Fullscreen", true, bpe->isFullScreenMode(), 'X', false);
		result.categoryName = "View";
        break;
	case MenuViewBack:
		setCommandTarget(result, "Back: " + bpe->mainEditor->getViewUndoManager()->getUndoDescription(), bpe->mainEditor->getViewUndoManager()->canUndo(), false, 'X', false);
		result.categoryName = "View";
		break;
	case MenuViewForward:
		setCommandTarget(result, "Forward: " + bpe->mainEditor->getViewUndoManager()->getRedoDescription(), bpe->mainEditor->getViewUndoManager()->canRedo(), false, 'X', false);
		result.categoryName = "View";
		break;
	case MenuViewEnableGlobalLayoutMode:
		setCommandTarget(result, "Enable Layout Mode", true, bpe->getRootFloatingTile()->isLayoutModeEnabled(), 'X', false);
		result.addDefaultKeypress(KeyPress::F6Key, ModifierKeys::noModifiers);
		result.categoryName = "View";
		break;
	case MenuViewAddFloatingWindow:
		setCommandTarget(result, "Add floating window", true, false, 'x', false);
		result.categoryName = "View";
		break;
	case MenuViewAddInterfacePreview:
		setCommandTarget(result, "Add Interface preview", true, false, 'x', false);
		result.categoryName = "View";
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
		setCommandTarget(result, "Open Plugin Preview Window", bpe->mainEditor->isPluginPreviewCreatable(), !bpe->mainEditor->isPluginPreviewShown(), 'X', false);
		result.categoryName = "View";
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
    case MenuToolsSanityCheck:
        setCommandTarget(result, "Validate plugin parameters", true, false, 'X', false);
		result.categoryName = "Tools";
        break;
    case MenuToolsClearConsole:
        setCommandTarget(result, "Clear Console", true, false, 'X', false);
		result.categoryName = "Tools";
        break;
	case MenuHelpShowAboutPage:
		setCommandTarget(result, "About HISE", true, false, 'X', false);
		result.categoryName = "Help";
		break;
	case MenuHelpShowDocumentation:
		setCommandTarget(result, "Show Documentation", true, false, 'X', false);
		result.addDefaultKeypress(KeyPress::F1Key, ModifierKeys::noModifiers);
		result.categoryName = "Help";
		break;
	case MenuHelpShowHelpForComponents:
		setCommandTarget(result, "Show Live Help", true, bpe->isHelpEnabled(), 'X', false);
		result.addDefaultKeypress(KeyPress::F1Key, ModifierKeys::commandModifier);
		result.categoryName = "Help";
		break;
    case MenuHelpCheckVersion:
        setCommandTarget(result, "Check for newer version", true, false, 'X', false);
		result.categoryName = "Help";
        break;
            
	default:					jassertfalse; return;
	}
}

bool BackendCommandTarget::perform(const InvocationInfo &info)
{
	CompileExporter exporter(bpe->getMainSynthChain());

	switch (info.commandID)
	{
	case HamburgerMenu:					Actions::showMainMenu(bpe);  return true;
	case Settings:                      bpe->showSettingsWindow(); return true;
	case WorkspaceMain:
	case WorkspaceScript:
	case WorkspaceSampler:
	case WorkspaceCustom:				bpe->showWorkspace(info.commandID); updateCommands(); return true;
	case MenuNewFile:                   if (PresetHandler::showYesNoWindow("New File", "Do you want to start a new preset?"))
                                            bpe->mainEditor->clearPreset(); return true;
	case MenuOpenFile:                  Actions::openFile(bpe); return true;
	case MenuSaveFile:                  Actions::saveFile(bpe, false); updateCommands(); return true;
	case MenuSaveFileAs:				Actions::saveFile(bpe, true); updateCommands(); return true;
    case MenuSaveFileXmlBackup:			Actions::saveFileXml(bpe); updateCommands(); return true;
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
	case MenuFileCreateRecoveryXml:		Actions::createRecoveryXml(bpe); return true;
    case MenuFileSaveUserPreset:        Actions::saveUserPreset(bpe); return true;
	case MenuFileSettingsPreset:		Actions::showFilePresetSettings(bpe); return true;
	case MenuFileSettingsProject:		Actions::showFileProjectSettings(bpe); return true;
	case MenuFileSettingsUser:			Actions::showFileUserSettings(bpe); return true;
	case MenuFileSettingsCompiler:		Actions::showFileCompilerSettings(bpe); return true;
	case MenuFileSettingCheckSanity:	Actions::checkSettingSanity(bpe); return true;
	case MenuFileSettingsCleanBuildDirectory:	Actions::cleanBuildDirectory(bpe); return true;
	case MenuReplaceWithClipboardContent: Actions::replaceWithClipboardContent(bpe); return true;
	case MenuFileQuit:                  if (PresetHandler::showYesNoWindow("Quit Application", "Do you want to quit?"))
                                            JUCEApplicationBase::quit(); return true;
	case MenuEditUndo:					bpe->owner->getControlUndoManager()->undo(); updateCommands(); return true;
	case MenuEditRedo:					bpe->owner->getControlUndoManager()->redo(); updateCommands(); return true;
	case MenuEditCopy:                  if (currentCopyPasteTarget) currentCopyPasteTarget->copyAction(); return true;
	case MenuEditPaste:                 if (currentCopyPasteTarget) currentCopyPasteTarget->pasteAction(); return true;
	case MenuEditMoveUp:				if (currentCopyPasteTarget) Actions::moveModule(currentCopyPasteTarget, true); return true;
	case MenuEditMoveDown:				if (currentCopyPasteTarget) Actions::moveModule(currentCopyPasteTarget, false); return true;
    case MenuEditCreateScriptVariable:  Actions::createScriptVariableDeclaration(currentCopyPasteTarget); return true;
	case MenuEditCreateBase64State:		Actions::createBase64State(currentCopyPasteTarget.get()); return true;
    case MenuEditPlotModulator:         Actions::plotModulator(currentCopyPasteTarget.get()); updateCommands(); return true;
    case MenuEditCloseAllChains:        Actions::closeAllChains(bpe); return true;
	case MenuToolsRecompile:            Actions::recompileAllScripts(bpe); return true;
	case MenuToolsCreateInterface:		Actions::createUserInterface(bpe); return true;
	case MenuToolsSetCompileTimeOut:	Actions::setCompileTimeOut(bpe); return true;
	case MenuToolsUseBackgroundThreadForCompile: Actions::toggleUseBackgroundThreadsForCompiling(bpe); updateCommands(); return true;
	case MenuToolsEnableCallStack:		Actions::toggleCallStackEnabled(bpe); updateCommands(); return true;
	case MenuToolsCheckCyclicReferences:Actions::checkCyclicReferences(bpe); return true;
	case MenuToolsRecompileScriptsOnReload: Actions::toggleCompileScriptsOnPresetLoad(bpe); updateCommands(); return true;
	case MenuToolsCreateExternalScriptFile:	Actions::createExternalScriptFile(bpe); updateCommands(); return true;
    case MenuToolsSanityCheck:			Actions::validatePluginParameters(bpe); return true;
	case MenuToolsValidateUserPresets:	Actions::validateUserPresets(bpe); return true;
	case MenuToolsResolveMissingSamples:Actions::resolveMissingSamples(bpe); return true;
	case MenuToolsGetMissingSampleList:	Actions::copyMissingSampleListToClipboard(bpe); return true;
	case MenuToolsCollectExternalFiles:	Actions::collectExternalFiles(bpe); return true;
	case MenuToolsCreateUIDataFromDesktop: Actions::createUIDataFromDesktop(bpe); updateCommands(); return true;
	case MenuToolsCheckDeviceSanity:	Actions::checkDeviceSanity(bpe); return true;
	case MenuToolsCheckUnusedImages:	Actions::checkUnusedImages(bpe); return true;
	case MenuToolsRedirectScriptFolder: Actions::redirectScriptFolder(bpe); updateCommands(); return true;
	case MenuToolsForcePoolSearch:		Actions::toggleForcePoolSearch(bpe); updateCommands(); return true;
	case MenuToolsConvertSampleMapToWavetableBanks:	Actions::convertSampleMapToWavetableBanks(bpe); return true;
	case MenuToolsConvertAllSamplesToMonolith:	Actions::convertAllSamplesToMonolith(bpe); return true;
	case MenuToolsUpdateSampleMapIdsBasedOnFileName:	Actions::updateSampleMapIds(bpe); return true;
	case MenuToolsConvertSfzToSampleMaps:	Actions::convertSfzFilesToSampleMaps(bpe); return true;
	case MenuToolsRemoveAllSampleMaps:	Actions::removeAllSampleMaps(bpe); return true;
	case MenuToolsUnloadAllAudioFiles:  Actions::unloadAllAudioFiles(bpe); return true;
	case MenuToolsCreateRSAKeys:		Actions::createRSAKeys(bpe); return true;
	case MenuToolsCreateDummyLicenseFile: Actions::createDummyLicenseFile(bpe); return true;
	case MenuToolsCheckAllSampleMaps:	Actions::checkAllSamplemaps(bpe); return true;
	case MenuToolsImportArchivedSamples: Actions::importArchivedSamples(bpe); return true;
	case MenuToolsRecordOneSecond:		bpe->owner->getDebugLogger().startRecording(); return true;
    case MenuToolsEnableDebugLogging:	bpe->owner->getDebugLogger().toggleLogging(); updateCommands(); return true;
    case MenuViewFullscreen:            Actions::toggleFullscreen(bpe); updateCommands(); return true;
	case MenuViewBack:					bpe->mainEditor->getViewUndoManager()->undo(); updateCommands(); return true;
	case MenuViewReset:				    bpe->resetInterface(); updateCommands(); return true;
	case MenuViewForward:				bpe->mainEditor->getViewUndoManager()->redo(); updateCommands(); return true;
	case MenuViewEnableGlobalLayoutMode: bpe->toggleLayoutMode(); updateCommands(); return true;
	case MenuViewAddFloatingWindow:		bpe->addFloatingWindow(); return true;
	case MenuViewAddInterfacePreview:	Actions::addInterfacePreview(bpe); return true;
	case MenuViewShowPluginPopupPreview: Actions::togglePluginPopupWindow(bpe); updateCommands(); return true;
    case MenuViewIncreaseCodeFontSize:  Actions::changeCodeFontSize(bpe, true); return true;
    case MenuViewDecreaseCodeFontSize:   Actions::changeCodeFontSize(bpe, false); return true;
	case MenuExportFileAsPlugin:        exporter.exportMainSynthChainAsInstrument(); return true;
	case MenuExportFileAsEffectPlugin:	exporter.exportMainSynthChainAsFX(); return true;
	case MenuExportFileAsStandaloneApp: exporter.exportMainSynthChainAsStandaloneApp(); return true;
	case MenuExportFileAsMidiFXPlugin:  exporter.exportMainSynthChainAsMidiFx(); return true;
    case MenuExportFileAsSnippet:       Actions::exportFileAsSnippet(bpe->getBackendProcessor()); return true;
	case MenuExportFileAsPlayerLibrary: Actions::exportMainSynthChainAsPlayerLibrary(bpe); return true;
	case MenuExportSampleDataForInstaller: Actions::exportSampleDataForInstaller(bpe); return true;
	case MenuExportCompileFilesInPool:	Actions::exportCompileFilesInPool(bpe); return true;
	case MenuViewResetLookAndFeel:		Actions::resetLookAndFeel(bpe); return true;
    case MenuAddView:                   Actions::addView(bpe); updateCommands();return true;
    case MenuDeleteView:                Actions::deleteView(bpe); updateCommands();return true;
    case MenuRenameView:                Actions::renameView(bpe); updateCommands();return true;
    case MenuViewSaveCurrentView:       Actions::saveView(bpe); updateCommands(); return true;
    case MenuToolsClearConsole:         owner->getConsoleHandler().clearConsole(); return true;
	case MenuHelpShowAboutPage:			Actions::showAboutPage(bpe); return true;
    case MenuHelpCheckVersion:          Actions::checkVersion(bpe); return true;
	case MenuOneColumn:					Actions::setColumns(bpe, this, OneColumn);  updateCommands(); return true;
	case MenuTwoColumns:				Actions::setColumns(bpe, this, TwoColumns);  updateCommands(); return true;
	case MenuThreeColumns:				Actions::setColumns(bpe, this, ThreeColumns);  updateCommands(); return true;
	case MenuHelpShowDocumentation:		Actions::showDocWindow(bpe); return true;
	case MenuHelpShowHelpForComponents: bpe->toggleHelp(); updateCommands(); return true;
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

		p.addSectionHeader("Project Management");
		ADD_ALL_PLATFORMS(MenuProjectNew);
		ADD_DESKTOP_ONLY(MenuProjectLoad);
		
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

		p.addSectionHeader("File Management");

		ADD_ALL_PLATFORMS(MenuNewFile);
		

		
		
		ADD_ALL_PLATFORMS(MenuOpenXmlBackup);
		ADD_ALL_PLATFORMS(MenuSaveFileXmlBackup);
		ADD_ALL_PLATFORMS(MenuSaveFileAsXmlBackup);

		PopupMenu xmlBackups;
		Array<File> xmlBackupFiles = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(ProjectHandler::SubDirectories::XMLPresetBackups);

		for (int i = 0; i < xmlBackupFiles.size(); i++)
		{
			xmlBackups.addItem(i + MenuFileXmlBackupMenuOffset, xmlBackupFiles[i].getFileName());
		}

		p.addSubMenu("Open recent XML", xmlBackups);

		p.addSeparator();

		ADD_DESKTOP_ONLY(MenuOpenFile);
		ADD_ALL_PLATFORMS(MenuSaveFile);
		

		PopupMenu filesInProject;

		if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
		{
			recentFileList = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(ProjectHandler::SubDirectories::Presets, true);

			for (int i = 0; i < recentFileList.size(); i++)
			{
				filesInProject.addItem(MenuOpenFileFromProjectOffset+i, recentFileList[i].getFileNameWithoutExtension(), true, false);
			}
		}

		p.addSubMenu("Open recent Archive", filesInProject, filesInProject.getNumItems() != 0);

		p.addSeparator();

		
		ADD_ALL_PLATFORMS(MenuReplaceWithClipboardContent);
		ADD_ALL_PLATFORMS(MenuFileCreateRecoveryXml);
		


#if HISE_IOS
#else

		p.addSeparator();

		ADD_ALL_PLATFORMS(MenuFileSettingsProject);

		p.addSeparator();
		ADD_ALL_PLATFORMS(MenuFileQuit);
#endif

		break; }
	case BackendCommandTarget::EditMenu:

		ADD_ALL_PLATFORMS(MenuEditUndo);
		ADD_ALL_PLATFORMS(MenuEditRedo);
		p.addSeparator();

        if(dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get()))
        {
            dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get())->addPopupMenuItems(p, nullptr);
            
        }
        else
        {
            ADD_ALL_PLATFORMS(MenuEditCopy);
            ADD_ALL_PLATFORMS(MenuEditPaste);
            p.addSeparator();

            ADD_ALL_PLATFORMS(MenuEditCreateScriptVariable);
			ADD_ALL_PLATFORMS(MenuEditCreateBase64State);
        }
		break;
	case BackendCommandTarget::ExportMenu:
	{
		p.addSectionHeader("Export As");
		ADD_DESKTOP_ONLY(MenuExportFileAsPlugin);
		ADD_DESKTOP_ONLY(MenuExportFileAsEffectPlugin);
		ADD_DESKTOP_ONLY(MenuExportFileAsMidiFXPlugin);
		ADD_DESKTOP_ONLY(MenuExportFileAsStandaloneApp);
		
		p.addSectionHeader("Export Tools");
		ADD_DESKTOP_ONLY(MenuExportFileAsSnippet);
		ADD_DESKTOP_ONLY(MenuExportFileAsPlayerLibrary);
		ADD_DESKTOP_ONLY(MenuExportSampleDataForInstaller);
		ADD_DESKTOP_ONLY(MenuFileSettingsCleanBuildDirectory);
		ADD_DESKTOP_ONLY(MenuExportCompileFilesInPool);
		break;
	}
	case BackendCommandTarget::ToolsMenu:
	{
		p.addSectionHeader("Scripting Tools");
		
		ADD_ALL_PLATFORMS(MenuToolsRecompile);
		ADD_ALL_PLATFORMS(MenuToolsSanityCheck);
		ADD_ALL_PLATFORMS(MenuToolsClearConsole);
		ADD_DESKTOP_ONLY(MenuToolsCheckCyclicReferences);
		
		ADD_DESKTOP_ONLY(MenuToolsCreateExternalScriptFile);
		ADD_DESKTOP_ONLY(MenuToolsValidateUserPresets);
		
		p.addSeparator();

		PopupMenu sub;

		
		for (int i = 0; i < (int)HiseDeviceSimulator::DeviceType::numDeviceTypes; i++)
		{
			sub.addItem(MenuToolsDeviceSimulatorOffset + i, "Simulate " + HiseDeviceSimulator::getDeviceName(i), true, i == (int)HiseDeviceSimulator::getDeviceType());
		}

		p.addSubMenu("Device simulator", sub);

		ADD_DESKTOP_ONLY(MenuToolsCreateUIDataFromDesktop);
		ADD_DESKTOP_ONLY(MenuToolsCheckDeviceSanity);

		p.addSeparator();
		p.addSectionHeader("Sample Management");
		
		ADD_DESKTOP_ONLY(MenuToolsResolveMissingSamples);
		ADD_DESKTOP_ONLY(MenuToolsGetMissingSampleList);
		ADD_DESKTOP_ONLY(MenuToolsDeleteMissingSamples);
		ADD_DESKTOP_ONLY(MenuToolsCheckAllSampleMaps);
		ADD_DESKTOP_ONLY(MenuToolsImportArchivedSamples);
		ADD_DESKTOP_ONLY(MenuToolsCollectExternalFiles);
		ADD_DESKTOP_ONLY(MenuToolsCheckUnusedImages);
		ADD_DESKTOP_ONLY(MenuToolsForcePoolSearch);
		
		ADD_DESKTOP_ONLY(MenuToolsConvertSampleMapToWavetableBanks);
		ADD_DESKTOP_ONLY(MenuToolsConvertAllSamplesToMonolith);
		ADD_DESKTOP_ONLY(MenuToolsUpdateSampleMapIdsBasedOnFileName);
		ADD_DESKTOP_ONLY(MenuToolsConvertSfzToSampleMaps);
		ADD_DESKTOP_ONLY(MenuToolsRemoveAllSampleMaps);
		ADD_DESKTOP_ONLY(MenuToolsUnloadAllAudioFiles);
		ADD_DESKTOP_ONLY(MenuToolsRecordOneSecond);
		p.addSeparator();
		p.addSectionHeader("License Management");
		ADD_DESKTOP_ONLY(MenuToolsCreateDummyLicenseFile);
		ADD_DESKTOP_ONLY(MenuToolsCreateRSAKeys);
		
		break;
	}
	case BackendCommandTarget::ViewMenu: {

		ADD_ALL_PLATFORMS(MenuViewResetLookAndFeel);

		ADD_ALL_PLATFORMS(MenuViewBack);
		ADD_ALL_PLATFORMS(MenuViewForward);
		ADD_ALL_PLATFORMS(MenuViewReset);

		p.addSeparator();

		ADD_ALL_PLATFORMS(WorkspaceMain);
		ADD_ALL_PLATFORMS(WorkspaceScript);
		ADD_ALL_PLATFORMS(WorkspaceSampler);
		ADD_ALL_PLATFORMS(WorkspaceCustom);

		p.addSeparator();

		ADD_DESKTOP_ONLY(MenuViewEnableGlobalLayoutMode);
		ADD_DESKTOP_ONLY(MenuViewAddFloatingWindow);
		ADD_DESKTOP_ONLY(MenuViewAddInterfacePreview);
		ADD_DESKTOP_ONLY(MenuViewFullscreen);
        
		break;
		}
	case BackendCommandTarget::HelpMenu:
			ADD_ALL_PLATFORMS(MenuHelpShowAboutPage);
			ADD_DESKTOP_ONLY(MenuHelpCheckVersion);
			ADD_ALL_PLATFORMS(MenuHelpShowDocumentation);
			ADD_ALL_PLATFORMS(MenuHelpShowHelpForComponents);
		break;
	default:
		break;
	}

	return p;
}


ValueTree getChildWithPropertyRecursive(const ValueTree& v, const Identifier& id, const var& value)
{
	auto r = v.getChildWithProperty(id, value);

	if (r.isValid())
		return r;

	for (auto child : v)
	{
		r = getChildWithPropertyRecursive(child, id, value);

		if (r.isValid())
			return r;
	}

	return ValueTree();
}

void BackendCommandTarget::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
	if (topLevelMenuIndex == 1 && (dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get()) != nullptr))
	{
		dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get())->performPopupMenuAction(menuItemID);
		return;
	}

	if (menuItemID >= MenuOpenFileFromProjectOffset && menuItemID < ((int)(MenuOpenFileFromProjectOffset) + 50))
	{
        const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
        
        if (!shouldDiscard) return;
        
		if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
		{
			File f = recentFileList[menuItemID - (int)MenuOpenFileFromProjectOffset];

			if (f.existsAsFile())
			{
				bpe->mainEditor->loadNewContainer(f);
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

			bpe->mainEditor->clearPreset();

			GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(file);
            
			bpe->getBackendProcessor()->getSettingsObject().refreshProjectData();

			

            menuItemsChanged();

			Actions::loadFirstXmlAfterProjectSwitch(bpe);
#endif
		}
	}
    else if (menuItemID >= MenuFileXmlBackupMenuOffset && menuItemID < ((int)MenuFileXmlBackupMenuOffset+50))
    {
        const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
        
        if (!shouldDiscard) return;
        
        const int index = menuItemID - MenuFileXmlBackupMenuOffset;
        
        Array<File> xmlFileList = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(ProjectHandler::SubDirectories::XMLPresetBackups);
        
		File presetToLoad = xmlFileList[index];
            
        Actions::openFileFromXml(bpe, presetToLoad);
    }
    
	else if (menuItemID >= MenuViewOffset && menuItemID < (int)MenuViewOffset + 200)
	{
		ViewInfo *info = owner->synthChain->getViewInfo(menuItemID - MenuViewOffset);

		if (info != nullptr)
		{
			info->restore();

			bpe->mainEditor->setRootProcessor(info->getRootProcessor());

			owner->synthChain->setCurrentViewInfo(menuItemID - MenuViewOffset);
		}
	}
	else if (menuItemID >= MenuToolsDeviceSimulatorOffset && menuItemID < (MenuToolsDeviceSimulatorOffset + 50))
	{
		HiseDeviceSimulator::DeviceType newDevice = (HiseDeviceSimulator::DeviceType)(menuItemID - (int)MenuToolsDeviceSimulatorOffset);

		bool copyMissing = HiseDeviceSimulator::getDeviceType() == HiseDeviceSimulator::DeviceType::Desktop &&
						   newDevice != HiseDeviceSimulator::DeviceType::Desktop;

		HiseDeviceSimulator::setDeviceType(newDevice);

		auto mp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(owner);

		

		if (mp != nullptr)
		{
			auto c = mp->getContent();


			ValueTree oldContent;
			Array<Identifier> idList;

			if (copyMissing)
			{
				

				oldContent = c->getContentProperties();
				
				for (int i = 0; i < c->getNumComponents(); i++)
				{
					auto cmp = c->getComponent(i);

					//bool x = ScriptingApi::Content::Helpers::hasLocation(cmp);

					if (cmp->getScriptObjectProperty(ScriptComponent::Properties::saveInPreset))
					{
						idList.add(cmp->getName());
					}
				}
			}

			auto contentData = c->exportAsValueTree();
			mp->setDeviceTypeForInterface((int)newDevice);
			mp->compileScript();
			mp->getContent()->restoreFromValueTree(contentData);

			if (copyMissing)
			{
				auto newContentProperties = c->getContentProperties();

				Array<Identifier> missingIds;

				for (const auto& id : idList)
				{
					if (c->getComponentWithName(id) == nullptr)
					{
						missingIds.add(id);
					}
				}
				
				if (!missingIds.isEmpty() && PresetHandler::showYesNoWindow("Missing components found", "There are some components in the desktop UI that are not available in the new UI. Press OK to query which components should be transferred."))
				{
					{
						ValueTreeUpdateWatcher::ScopedDelayer sd(c->getUpdateWatcher());

						for (const auto& id : missingIds)
						{
							if (PresetHandler::showYesNoWindow("Copy " + id.toString(), "Do you want to transfer this component?"))
							{
								auto v = getChildWithPropertyRecursive(oldContent, "id", id.toString());

								if (v.isValid())
								{
									v = v.createCopy();
									v.removeAllChildren(nullptr);

									v.removeProperty("parentComponent", nullptr);

									newContentProperties.addChild(v, -1, nullptr);
									debugToConsole(mp, "Added " + id.toString() + " to " + HiseDeviceSimulator::getDeviceName());
								}
							}
						}
					}

					mp->compileScript();
				}

				
			}

			



		}

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

void BackendCommandTarget::Actions::openFile(BackendRootWindow *bpe)
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

void BackendCommandTarget::Actions::saveFile(BackendRootWindow *bpe, bool forceRename)
{
	if (forceRename || PresetHandler::showYesNoWindow("Save " + bpe->owner->getMainSynthChain()->getId(), "Do you want to save this preset?"))
	{
		const bool hasDefaultName = bpe->owner->getMainSynthChain()->getId() == "Master Chain";

		if (forceRename || hasDefaultName)
		{
			const String newName = PresetHandler::getCustomName("Preset");

			if (newName.isNotEmpty())
			{
				bpe->owner->getMainSynthChain()->setId(newName);
			}
			else return;
		}

		PresetHandler::saveProcessorAsPreset(bpe->owner->getMainSynthChain());
	}
}

void BackendCommandTarget::Actions::replaceWithClipboardContent(BackendRootWindow *bpe)
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

void BackendCommandTarget::Actions::createBase64State(CopyPasteTarget* target)
{
	ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(target);

	if (editor != nullptr)
	{
		ProcessorHelpers::getBase64String(editor->getProcessor());
	}
}

void BackendCommandTarget::Actions::createUserInterface(BackendRootWindow * /*bpe*/)
{
	
}

void BackendCommandTarget::Actions::checkUnusedImages(BackendRootWindow * bpe)
{
	auto& handler = GET_PROJECT_HANDLER(bpe->getMainSynthChain());

	auto imageDirectory = handler.getSubDirectory(ProjectHandler::SubDirectories::Images);
	
	Array<File> allFiles;
	
	imageDirectory.findChildFiles(allFiles, File::findFiles, true);

	Array<File> referencedFiles;

	auto imagePool = bpe->getBackendProcessor()->getCurrentImagePool();

	for (int i = 0; i < allFiles.size(); i++)
	{
		auto f = File(allFiles[i]);

		jassert(f.existsAsFile());

		if (imagePool->contains(f.hashCode64()))
		{
			allFiles.remove(i--);
		}
	}

	for (int i = 0; i < allFiles.size(); i++)
	{
		auto f = allFiles[i];

		if (f.getFileName() == "SplashScreen.png")
		{
			// Don't remove the splash screen
			allFiles.remove(i--);
			continue;
		}

		if (f.getFileName() == "Icon.png")
		{
			// Don't remove the icon file
			allFiles.remove(i--);
			continue;
		}

		if (f.getFileExtension() == ".ttf")
		{
			// Don't remove fonts
			allFiles.remove(i--);
			continue;
		}
	}

	if (allFiles.isEmpty())
	{
		PresetHandler::showMessageWindow("No unreferenced images found", "There are no unreferenced images in the project folder", PresetHandler::IconType::Info);
	}
	else if (PresetHandler::showYesNoWindow(String(allFiles.size()) + " unreferenced images found", "Press OK to move the files into a temporary directory"))
	{
		auto tempDirectory = imageDirectory.getSiblingFile("UnusedImages");

		for (int i = 0; i < allFiles.size(); i++)
		{
			auto path = allFiles[i].getRelativePathFrom(imageDirectory);

			auto target = tempDirectory.getChildFile(path);

			target.getParentDirectory().createDirectory();

			allFiles[i].moveFileTo(target);
		}

		if (PresetHandler::showYesNoWindow("Open temporary location", "Do you want to open the location where the unreferenced files were moved?"))
		{
			tempDirectory.revealToUser();
		}
	}


}

void BackendCommandTarget::Actions::addInterfacePreview(BackendRootWindow * bpe)
{
	auto w = bpe->addFloatingWindow();

	w->getRootFloatingTile()->setNewContent(GET_PANEL_NAME(InterfaceContentPanel));
	w->getRootFloatingTile()->setLayoutModeEnabled(false);
	w->getRootFloatingTile()->setVital(true);
	
	w->setName("Interface Preview");
	
	auto jmp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(bpe->getBackendProcessor());

	if (jmp != nullptr)
	{
		if (auto content = jmp->getScriptingContent())
		{
			auto scaleFactor = dynamic_cast<GlobalSettingManager*>(bpe->getBackendProcessor())->getGlobalScaleFactor();

			const int width = (int)((float)content->getContentWidth()*scaleFactor);
			const int height = (int)((float)content->getContentHeight()*scaleFactor);

			dynamic_cast<Component*>(w->getRootFloatingTile()->getCurrentFloatingPanel())->setTransform(AffineTransform::scale((float)scaleFactor));
			w->centreWithSize(width, height);
			w->setResizable(false, false);
		}
	}

	w->getRootFloatingTile()->refreshRootLayout();
}

void BackendCommandTarget::Actions::updateSampleMapIds(BackendRootWindow* bpe)
{
	if (PresetHandler::showYesNoWindow("Update SampleMap Ids", "Do you really want to update the IDs of all samplemaps in the current project?\nThis is undoable"))
	{
		FileHandlerBase* handlerToUse = &GET_PROJECT_HANDLER(bpe->getMainSynthChain());

		if (auto e = bpe->getMainController()->getExpansionHandler().getCurrentExpansion())
		{
			handlerToUse = e;
		}

		auto result = handlerToUse->updateSampleMapIds(false);

		if (!result.wasOk())
		{
			PresetHandler::showMessageWindow("Error", result.getErrorMessage(), PresetHandler::IconType::Error);
		}
	}
}

void BackendCommandTarget::Actions::toggleCallStackEnabled(BackendRootWindow * bpe)
{
	bpe->getBackendProcessor()->updateCallstackSettingForExistingScriptProcessors();
}

void BackendCommandTarget::Actions::testPlugin(const String& pluginToLoad)
{
	AudioPluginFormatManager fm;
	KnownPluginList list;

	fm.addDefaultFormats();

	

	OwnedArray <PluginDescription> typesFound;

	for (int i = 0; i < fm.getNumFormats(); i++)
	{
		list.scanAndAddFile(pluginToLoad, false, typesFound, *fm.getFormat(i));
	}

	String error;

	if (!typesFound.isEmpty())
	{
		NewLine nl;

		auto desc = typesFound.getUnchecked(0);

		Logger::writeToLog("Loading plugin " + desc->name + nl);

		ScopedPointer<XmlElement> xml = desc->createXml();

		Logger::writeToLog("Plugin description:");
		Logger::writeToLog(xml->createDocument(""));

		Logger::writeToLog("Initialising...");

		ScopedPointer<AudioPluginInstance> plugin = fm.createPluginInstance(*desc, 44100.0, 512, error);

		Logger::writeToLog("OK");

		Logger::writeToLog("Creating Editor...");

		ScopedPointer<AudioProcessorEditor> editor = plugin->createEditor();

		Logger::writeToLog("OK");

		Logger::writeToLog("Removing Editor...");

		editor = nullptr;

		Logger::writeToLog("OK");

		Logger::writeToLog("Closing Plugin...");

		plugin = nullptr;

		Logger::writeToLog("OK");

		return;
	}
}

void BackendCommandTarget::Actions::removeAllSampleMaps(BackendRootWindow * bpe)
{
	if (PresetHandler::showYesNoWindow("Remove all Samplemaps", "Are you sure you want to clear all samplemaps?\nThis is useful before exporting if you recall samplemaps using scripted controls or user presets"))
	{
		Processor::Iterator<ModulatorSampler> iter(bpe->getMainSynthChain(), false);

		while (auto sampler = iter.getNextProcessor())
			sampler->clearSampleMap(sendNotificationAsync);
	}
}

void BackendCommandTarget::Actions::redirectScriptFolder(BackendRootWindow * /*bpe*/)
{
	FileChooser fc("Redirect sample folder to the following location");

	if (fc.browseForDirectory())
	{
		File f = fc.getResult();

		ProjectHandler::createLinkFileInFolder(ProjectHandler::getAppDataDirectory().getChildFile("scripts"), f);
	}
}

void BackendCommandTarget::Actions::exportSampleDataForInstaller(BackendRootWindow * bpe)
{
	auto exporter = new SampleDataExporter(bpe->getMainController());

	exporter->setModalBaseWindowComponent(bpe->mainEditor);
}

void BackendCommandTarget::Actions::importArchivedSamples(BackendRootWindow * bpe)
{
	auto mbw = dynamic_cast<ModalBaseWindow*>(bpe);

	auto importer = new SampleDataImporter(mbw);

	importer->setModalBaseWindowComponent(bpe->mainEditor);

}

void BackendCommandTarget::Actions::checkCyclicReferences(BackendRootWindow * bpe)
{
	auto checker = new CyclicReferenceChecker(bpe->mainEditor);

	checker->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::recompileAllScripts(BackendRootWindow * bpe)
{
	bpe->owner->compileAllScripts();
}

void BackendCommandTarget::Actions::toggleFullscreen(BackendRootWindow * bpe)
{
#if IS_STANDALONE_APP
    
    Component *window = bpe->getParentComponent()->getParentComponent();
    
    if (bpe->isFullScreenMode())
    {
        Desktop::getInstance().setKioskModeComponent(nullptr);
        
        const int height = Desktop::getInstance().getDisplays().getMainDisplay().userArea.getHeight() - 70;
        bpe->setSize(900, height);
        bpe->resized();
        
        window->centreWithSize(bpe->getWidth(), bpe->getHeight());
        
        bpe->setAlwaysOnTop(false);
        bpe->yBorderDragger->setVisible(true);
		bpe->xBorderDragger->setVisible(true);
    }
    else
    {
        Desktop::getInstance().setKioskModeComponent(window);
        
        bpe->yBorderDragger->setVisible(false);
		bpe->xBorderDragger->setVisible(false);
        bpe->setAlwaysOnTop(true);
        
        bpe->setSize(Desktop::getInstance().getDisplays().getMainDisplay().totalArea.getWidth(),
                Desktop::getInstance().getDisplays().getMainDisplay().totalArea.getHeight());
        
        bpe->resized();
        
    }
#else 
	ignoreUnused(bpe);
#endif
}



void BackendCommandTarget::Actions::resetLookAndFeel(BackendRootWindow* bpe)
{
	

	bpe->owner->resetLookAndFeelToDefault(bpe);
}

void BackendCommandTarget::Actions::addView(BackendRootWindow *bpe)
{
    String view = PresetHandler::getCustomName("View");
    
    ViewInfo *info = new ViewInfo(bpe->owner->synthChain, bpe->mainEditor->currentRootProcessor, view);
    
    bpe->owner->synthChain->addViewInfo(info);
}

void BackendCommandTarget::Actions::deleteView(BackendRootWindow *bpe)
{
    bpe->owner->synthChain->removeCurrentViewInfo();
}

void BackendCommandTarget::Actions::renameView(BackendRootWindow *bpe)
{
    String view = PresetHandler::getCustomName("View");
    
    bpe->owner->synthChain->getCurrentViewInfo()->setViewName(view);
}

void BackendCommandTarget::Actions::saveView(BackendRootWindow *bpe)
{
    String view = bpe->owner->synthChain->getCurrentViewInfo()->getViewName();
    
    ViewInfo *info = new ViewInfo(bpe->owner->synthChain, bpe->mainEditor->currentRootProcessor, view);
    
    bpe->owner->synthChain->replaceCurrentViewInfo(info);
}

void BackendCommandTarget::Actions::closeAllChains(BackendRootWindow *bpe)
{
    ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(bpe->currentCopyPasteTarget.get());
    
    if(editor != nullptr)
    {
        editor->getChainBar()->closeAll();
    }
}

void BackendCommandTarget::Actions::validatePluginParameters(BackendRootWindow *bpe)
{
	PresetHandler::checkMetaParameters(bpe->owner->synthChain);

    PresetHandler::checkProcessorIdsForDuplicates(bpe->owner->synthChain, false);

}

void BackendCommandTarget::Actions::showAboutPage(BackendRootWindow * bpe)
{
	bpe->getMainTopBar()->togglePopup(MainTopBar::PopupType::About, true);

	//bpe->mainEditor->aboutPage->showAboutPage();
}

void BackendCommandTarget::Actions::checkVersion(BackendRootWindow *bpe)
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

void BackendCommandTarget::Actions::setColumns(BackendRootWindow * bpe, BackendCommandTarget* target, ColumnMode columns)
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

void BackendCommandTarget::Actions::plotModulator(CopyPasteTarget *currentCopyPasteTarget)
{
    ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(currentCopyPasteTarget);
                                                                         
    if(editor != nullptr)
    {
        
    }
}

void BackendCommandTarget::Actions::resolveMissingSamples(BackendRootWindow *bpe)
{
	bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool()->resolveMissingSamples(bpe);
}

void BackendCommandTarget::Actions::setCompileTimeOut(BackendRootWindow * /*bpe*/)
{
	jassertfalse;
}

void BackendCommandTarget::Actions::toggleUseBackgroundThreadsForCompiling(BackendRootWindow * bpe)
{
	const bool lastState = bpe->getBackendProcessor()->isUsingBackgroundThreadForCompiling();

	bpe->getBackendProcessor()->setShouldUseBackgroundThreadForCompiling(!lastState);
}

void BackendCommandTarget::Actions::toggleCompileScriptsOnPresetLoad(BackendRootWindow * bpe)
{
	const bool lastState = bpe->getBackendProcessor()->isCompilingAllScriptsOnPresetLoad();

	bpe->getBackendProcessor()->setEnableCompileAllScriptsOnPresetLoad(!lastState);
}


void BackendCommandTarget::Actions::collectExternalFiles(BackendRootWindow * bpe)
{
	ExternalResourceCollector *resource = new ExternalResourceCollector(bpe->getBackendProcessor());

	resource->setModalBaseWindowComponent(bpe);
}


void BackendCommandTarget::Actions::exportFileAsSnippet(BackendProcessor* bp)
{
	ValueTree v = bp->getMainSynthChain()->exportAsValueTree();

	MemoryOutputStream mos;

	v.writeToStream(mos);

	MemoryOutputStream mos2;

	GZIPCompressorOutputStream zipper(&mos2, 9);
	
	zipper.write(mos.getData(), mos.getDataSize());
	zipper.flush();

	String data = "HiseSnippet " + mos2.getMemoryBlock().toBase64Encoding();

	SystemClipboard::copyTextToClipboard(data);

	if (!MainController::inUnitTestMode())
	{
		PresetHandler::showMessageWindow("Preset copied as compressed snippet", "You can paste the clipboard content to share this preset", PresetHandler::IconType::Info);
	}
	
}


// GREG ADDITION (Just copied from saveFileAsXml below and made a few mods but not working, of cousre...)
void BackendCommandTarget::Actions::saveFileXml(BackendRootWindow * bpe)
{
	if (PresetHandler::showYesNoWindow("Save " + bpe->owner->getMainSynthChain()->getId(), "Do you want to save this XML?"))	// Greg added
	{
		const bool hasDefaultName = bpe->owner->getMainSynthChain()->getId() == "Master Chain";

		// Overwrite Xml with mainSynthChain name
		if (!hasDefaultName)

			const String newName = bpe->owner->getMainSynthChain()->getId();

			if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
			{

				if (PresetHandler::showYesNoWindow("Overwrite " + bpe->owner->getMainSynthChain()->getId(), "Overwrite the existing XML?"))	// Overwrite dialog
				{
					const String newName = bpe->owner->getMainSynthChain()->getId();

					ValueTree v = bpe->owner->getMainSynthChain()->exportAsValueTree();

		            v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
		            
					ScopedPointer<XmlElement> xml = v.createXml();

					XmlBackupFunctions::removeEditorStatesFromXml(*xml);

					
					Processor::Iterator<ModulatorSampler> siter(bpe->getMainSynthChain());

					while (auto s = siter.getNextProcessor())
					{
						if (s->getSampleMap()->hasUnsavedChanges())
							s->getSampleMap()->saveAndReloadMap();
					}

					File originalScriptDirectory = XmlBackupFunctions::getScriptDirectoryFor(bpe->getMainSynthChain());

					File scriptDirectory = originalScriptDirectory.getSiblingFile("TempScriptDirectory");

					Processor::Iterator<JavascriptProcessor> iter(bpe->getMainSynthChain());

					scriptDirectory.deleteRecursively();

					scriptDirectory.createDirectory();

					String interfaceId = "";

					while (JavascriptProcessor *sp = iter.getNextProcessor())
					{
						if (sp->isConnectedToExternalFile())
							continue;

						String content;

						if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(sp))
						{
							if (jmp->isFront())
								interfaceId = jmp->getId();
						}

						sp->mergeCallbacksToScript(content);

						File scriptFile = XmlBackupFunctions::getScriptFileFor(bpe->getMainSynthChain(), scriptDirectory, dynamic_cast<Processor*>(sp)->getId());

						scriptFile.replaceWithText(content);
					}

					XmlBackupFunctions::removeAllScripts(*xml);

					if(interfaceId.isNotEmpty())
						//XmlBackupFunctions::extractContentData(*xml, interfaceId, fc.getResult());

					//fc.getResult().replaceWithText(xml->createDocument(""));

					debugToConsole(bpe->owner->getMainSynthChain(), "Exported as XML");
		            
					if (originalScriptDirectory.deleteRecursively())
					{
						scriptDirectory.moveFileTo(originalScriptDirectory);
					}
					else
					{
						PresetHandler::showMessageWindow("Error at writing script file",
							"The embedded script files could not be saved (probably because the file is opened somewhere else).\nPress OK to show the folder and move it manually", PresetHandler::IconType::Error);

						scriptDirectory.revealToUser();
					}

		            
				}
			}

		// Else, same as saveFileAsXml (call for manual saving)
		else
		{
			if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
			{
				FileChooser fc("Select XML file to save", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::XMLPresetBackups), "*.xml", true);

				if (fc.browseForFileToSave(true))
				{
					const String newName = fc.getResult().getFileNameWithoutExtension();
					bpe->owner->getMainSynthChain()->setId(newName);

					ValueTree v = bpe->owner->getMainSynthChain()->exportAsValueTree();

		            v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
		            
					ScopedPointer<XmlElement> xml = v.createXml();

					XmlBackupFunctions::removeEditorStatesFromXml(*xml);

					
					Processor::Iterator<ModulatorSampler> siter(bpe->getMainSynthChain());

					while (auto s = siter.getNextProcessor())
					{
						if (s->getSampleMap()->hasUnsavedChanges())
							s->getSampleMap()->saveAndReloadMap();
					}

					File originalScriptDirectory = XmlBackupFunctions::getScriptDirectoryFor(bpe->getMainSynthChain());

					File scriptDirectory = originalScriptDirectory.getSiblingFile("TempScriptDirectory");

					Processor::Iterator<JavascriptProcessor> iter(bpe->getMainSynthChain());

					scriptDirectory.deleteRecursively();

					scriptDirectory.createDirectory();

					String interfaceId = "";

					while (JavascriptProcessor *sp = iter.getNextProcessor())
					{
						if (sp->isConnectedToExternalFile())
							continue;

						String content;

						if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(sp))
						{
							if (jmp->isFront())
								interfaceId = jmp->getId();
						}

						sp->mergeCallbacksToScript(content);

						File scriptFile = XmlBackupFunctions::getScriptFileFor(bpe->getMainSynthChain(), scriptDirectory, dynamic_cast<Processor*>(sp)->getId());

						scriptFile.replaceWithText(content);
					}

					XmlBackupFunctions::removeAllScripts(*xml);
					
					if(interfaceId.isNotEmpty())
						XmlBackupFunctions::extractContentData(*xml, interfaceId, fc.getResult());

					fc.getResult().replaceWithText(xml->createDocument(""));

					debugToConsole(bpe->owner->getMainSynthChain(), "Exported as XML");
		            
					if (originalScriptDirectory.deleteRecursively())
					{
						scriptDirectory.moveFileTo(originalScriptDirectory);
					}
					else
					{
						PresetHandler::showMessageWindow("Error at writing script file",
							"The embedded script files could not be saved (probably because the file is opened somewhere else).\nPress OK to show the folder and move it manually", PresetHandler::IconType::Error);

						scriptDirectory.revealToUser();
					}

		            
				}
			}
		}
	}
}

void BackendCommandTarget::Actions::saveFileAsXml(BackendRootWindow * bpe)
{
	if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
	{
		FileChooser fc("Select XML file to save", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::XMLPresetBackups), "*.xml", true);

		if (fc.browseForFileToSave(true))
		{
			const String newName = fc.getResult().getFileNameWithoutExtension();
			bpe->owner->getMainSynthChain()->setId(newName);

			ValueTree v = bpe->owner->getMainSynthChain()->exportAsValueTree();

            v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
            
			ScopedPointer<XmlElement> xml = v.createXml();

			FullInstrumentExpansion::setNewDefault(bpe->owner, v);

			XmlBackupFunctions::removeEditorStatesFromXml(*xml);

			
			Processor::Iterator<ModulatorSampler> siter(bpe->getMainSynthChain());

			while (auto s = siter.getNextProcessor())
			{
				if (s->getSampleMap()->hasUnsavedChanges())
					s->getSampleMap()->saveAndReloadMap();
			}

			File originalScriptDirectory = XmlBackupFunctions::getScriptDirectoryFor(bpe->getMainSynthChain());

			File scriptDirectory = originalScriptDirectory.getSiblingFile("TempScriptDirectory");

			Processor::Iterator<JavascriptProcessor> iter(bpe->getMainSynthChain());

			scriptDirectory.deleteRecursively();

			scriptDirectory.createDirectory();

			String interfaceId = "";

			while (JavascriptProcessor *sp = iter.getNextProcessor())
			{
				if (sp->isConnectedToExternalFile())
					continue;

				String content;

				if (auto jmp = dynamic_cast<JavascriptMidiProcessor*>(sp))
				{
					if (jmp->isFront())
						interfaceId = jmp->getId();
				}

				sp->mergeCallbacksToScript(content);

				File scriptFile = XmlBackupFunctions::getScriptFileFor(bpe->getMainSynthChain(), scriptDirectory, dynamic_cast<Processor*>(sp)->getId());

				scriptFile.replaceWithText(content);
			}

			XmlBackupFunctions::removeAllScripts(*xml);
			
			if(interfaceId.isNotEmpty())
				XmlBackupFunctions::extractContentData(*xml, interfaceId, fc.getResult());

			fc.getResult().replaceWithText(xml->createDocument(""));

			debugToConsole(bpe->owner->getMainSynthChain(), "Exported as XML");
            
			if (originalScriptDirectory.deleteRecursively())
			{
				scriptDirectory.moveFileTo(originalScriptDirectory);
			}
			else
			{
				PresetHandler::showMessageWindow("Error at writing script file",
					"The embedded script files could not be saved (probably because the file is opened somewhere else).\nPress OK to show the folder and move it manually", PresetHandler::IconType::Error);

				scriptDirectory.revealToUser();
			}

            
		}
	}
}

void BackendCommandTarget::Actions::openFileFromXml(BackendRootWindow * bpe, const File &fileToLoad)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
	{
		ScopedPointer<XmlElement> xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
		{
			XmlBackupFunctions::addContentFromSubdirectory(*xml, fileToLoad);

			String newId = xml->getStringAttribute("ID");

			XmlBackupFunctions::restoreAllScripts(*xml, bpe->getMainSynthChain(), newId);

			ValueTree v = ValueTree::fromXml(*xml);

			bpe->loadNewContainer(v);
		}
		else
		{
			PresetHandler::showMessageWindow("Corrupt File", "The XML file is not valid. Loading aborted", PresetHandler::IconType::Error);
		}
	}
}

void BackendCommandTarget::Actions::createNewProject(BackendRootWindow *bpe)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	FileChooser fc("Create new project directory");

	if (fc.browseForDirectory())
	{
		File f = fc.getResult();

		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).createNewProject(f, bpe);

		bpe->getBackendProcessor()->clearPreset();
		bpe->getBackendProcessor()->getSettingsObject().refreshProjectData();
	}
}

void BackendCommandTarget::Actions::loadProject(BackendRootWindow *bpe)
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

		auto& handler = GET_PROJECT_HANDLER(bpe->getMainSynthChain());

		auto r = handler.setWorkingProject(f);

		if (r.failed())
		{
			PresetHandler::showMessageWindow("Error loading project", r.getErrorMessage(), PresetHandler::IconType::Error);
		}
		else
		{
			bpe->getBackendProcessor()->getSettingsObject().refreshProjectData();
			bpe->getBackendProcessor()->clearPreset();
			loadFirstXmlAfterProjectSwitch(bpe);
		}
			

		
	}
#endif
}

void BackendCommandTarget::Actions::loadFirstXmlAfterProjectSwitch(BackendRootWindow * bpe)
{
	auto& handler = GET_PROJECT_HANDLER(bpe->getMainSynthChain());

	Array<File> files = handler.getFileList(ProjectHandler::SubDirectories::XMLPresetBackups, true);

	if (files.size() > 0 && PresetHandler::showYesNoWindow("Load first XML in project?", "Do you want to load " + files[0].getFileName()))
	{
		BackendCommandTarget::Actions::openFileFromXml(bpe, files[0]);
	}
}

void BackendCommandTarget::Actions::closeProject(BackendRootWindow *bpe)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	GET_PROJECT_HANDLER(bpe->getMainSynthChain()).setWorkingProject(File());
}

void BackendCommandTarget::Actions::showProjectInFinder(BackendRootWindow *bpe)
{
	if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
	{
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getWorkDirectory().revealToUser();
	}
}

void BackendCommandTarget::Actions::saveUserPreset(BackendRootWindow *bpe)
{
	UserPresetHelpers::saveUserPreset(bpe->getMainSynthChain());
}

void BackendCommandTarget::Actions::loadUserPreset(BackendRootWindow *bpe, const File &fileToLoad)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	UserPresetHelpers::loadUserPreset(bpe->getMainSynthChain(), fileToLoad);
}

void BackendCommandTarget::Actions::showFilePresetSettings(BackendRootWindow * /*bpe*/)
{
	
}

void BackendCommandTarget::Actions::showFileProjectSettings(BackendRootWindow * bpe)
{
	//SettingWindows::ProjectSettingWindow *window = new SettingWindows::ProjectSettingWindow(&GET_PROJECT_HANDLER(bpe->getMainSynthChain()));

	auto window = new SettingWindows(bpe->getBackendProcessor()->getSettingsObject());


	window->setModalBaseWindowComponent(bpe);

	window->activateSearchBox();
}

void BackendCommandTarget::Actions::showFileUserSettings(BackendRootWindow * /*bpe*/)
{
	jassertfalse;
}

void BackendCommandTarget::Actions::showFileCompilerSettings(BackendRootWindow * /*bpe*/)
{
	jassertfalse;
}

void BackendCommandTarget::Actions::checkSettingSanity(BackendRootWindow * /*bpe*/)
{
	jassertfalse;
}

void BackendCommandTarget::Actions::togglePluginPopupWindow(BackendRootWindow * bpe)
{
	if (bpe->mainEditor->isPluginPreviewShown())
	{
		bpe->mainEditor->setPluginPreviewWindow(nullptr);
	}
	else
	{
#if HISE_IOS
		bpe->showPseudoModalWindow(new PluginPreviewWindow::Content(bpe), bpe->getMainSynthChain()->getId(), true);
#else
		bpe->mainEditor->setPluginPreviewWindow(new PluginPreviewWindow(bpe->mainEditor));
#endif
	}
}
                    
void BackendCommandTarget::Actions::changeCodeFontSize(BackendRootWindow *, bool /*increase*/)
{
	jassertfalse;
}

void BackendCommandTarget::Actions::createRSAKeys(BackendRootWindow * bpe)
{
	GET_PROJECT_HANDLER(bpe->getMainSynthChain()).createRSAKey();
}


void BackendCommandTarget::Actions::createDummyLicenseFile(BackendRootWindow * bpe)
{
	ProjectHandler *handler = &GET_PROJECT_HANDLER(bpe->getMainSynthChain());

	if (!handler->isActive())
	{
		PresetHandler::showMessageWindow("No Project active", "You need an active project to create a key file.", PresetHandler::IconType::Warning);
		return;
	}

	auto& dataObject = bpe->getBackendProcessor()->getSettingsObject();

	const auto appName = dataObject.getSetting(HiseSettings::Project::Name).toString();
	const auto version = dataObject.getSetting(HiseSettings::Project::Version).toString();

	if (appName.isEmpty() || version.isEmpty())
	{
		PresetHandler::showMessageWindow("No Product name", "You need a product name for a license file.", PresetHandler::IconType::Warning);
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
	File key = handler->getWorkDirectory().getChildFile(productName + FrontendHandler::getLicenseKeyExtension());

	key.replaceWithText(keyContent);

#if JUCE_WINDOWS
#if JUCE_64BIT
	const String message = "A dummy license file for 64bit plugins was created.\nTo load 32bit plugins, please use the 32bit version of HISE to create the license file";
#else
	const String message = "A dummy license file for 32bit plugins was created.\nTo load 64bit plugins, please use the 64bit version of HISE to create the license file";
#endif
#else
	const String message = "A dummy license file for the plugins was created.";
#endif

	PresetHandler::showMessageWindow("License File created", message, PresetHandler::IconType::Info);
}

void BackendCommandTarget::Actions::toggleForcePoolSearch(BackendRootWindow * bpe)
{
	ModulatorSamplerSoundPool *pool = bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool();

	pool->setForcePoolSearch(!pool->isPoolSearchForced());
}


void BackendCommandTarget::Actions::archiveProject(BackendRootWindow * bpe)
{
	ProjectHandler *handler = &GET_PROJECT_HANDLER(bpe->getMainSynthChain());

	if (handler->isRedirected(ProjectHandler::SubDirectories::Samples))
	{
		if (PresetHandler::showYesNoWindow("Sample Folder is redirected", 
										   "The sample folder is redirected to another location.\nIt will not be included in the archive. Press OK to continue or cancel to abort", 
										   PresetHandler::IconType::Warning))
			return;
	}

	FileChooser fc("Select archive destination", File(), "*.zip");

	if (fc.browseForFileToSave(true))
	{
		File archiveFile = fc.getResult();

		File projectDirectory = handler->getWorkDirectory();

		new ProjectArchiver(archiveFile, projectDirectory, bpe->getBackendProcessor());

	}
}

void BackendCommandTarget::Actions::downloadNewProject(BackendRootWindow * bpe)
{
	ProjectDownloader *downloader = new ProjectDownloader(bpe->mainEditor);

	downloader->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::showMainMenu(BackendRootWindow * /*bpe*/)
{
#if 0
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

	bpe->mainEditor->showPseudoModalWindow(newMenu, "Main Menu", true);
#endif

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

			BackendRootWindow *bpe = GET_BACKEND_ROOT_WINDOW(editor);
			bpe->mainEditor->refreshContainer(processor);
		}
	}
}

void BackendCommandTarget::Actions::createExternalScriptFile(BackendRootWindow * bpe)
{
	File scriptDirectory = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Scripts);

	String newScriptName = PresetHandler::getCustomName("Script File");

	if (newScriptName.isNotEmpty())
	{
		File newScriptFile = scriptDirectory.getChildFile(newScriptName + ".js");

		if (newScriptFile.exists())
		{
			PresetHandler::showMessageWindow("File already exists", "The file you are trying to create already exists", PresetHandler::IconType::Warning);
		}
		else
		{
			newScriptFile.create();
			newScriptFile.replaceWithText("/** External Script File " + newScriptName + " */\n\n");

			SystemClipboard::copyTextToClipboard("include(\"" + newScriptFile.getFileName() + "\");");

			PresetHandler::showMessageWindow("File created", "The file " + newScriptFile.getFullPathName() + " was successfully created.\nThe include statement was copied into the clipboard.");
		}
	}
}

void BackendCommandTarget::Actions::exportMainSynthChainAsPlayerLibrary(BackendRootWindow * bpe)
{
	auto& h = GET_PROJECT_HANDLER(bpe->getMainSynthChain());

	if (bpe->getMainController()->getExpansionHandler().getEncryptionKey().isEmpty())
	{
		auto k = dynamic_cast<GlobalSettingManager*>(bpe->owner)->getSettingsObject().getSetting(HiseSettings::Project::EncryptionKey).toString();

		if (k.isNotEmpty())
			bpe->getMainController()->getExpansionHandler().setEncryptionKey(k);
		else
		{
			PresetHandler::showMessageWindow("No encryption key", "You have to specify an encryption key in order to encode the project as full expansion", PresetHandler::IconType::Error);
			return;
		}
	}

	auto existingIntermediate = Expansion::Helpers::getExpansionInfoFile(h.getWorkDirectory(), Expansion::Intermediate);

	if (existingIntermediate.existsAsFile())
		existingIntermediate.deleteFile();

	auto e = new ExpansionEncodingWindow(bpe->owner, nullptr, true);
	e->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::cleanBuildDirectory(BackendRootWindow * bpe)
{
	if (!GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive()) return;

	File buildDirectory = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Binaries);

	if (buildDirectory.isDirectory())
	{
		const bool cleanEverything = PresetHandler::showYesNoWindow("Clean everything", "Do you want to delete all files in the build directory and the pooled resource binary files?\nPress Cancel to just delete the autogenerated IDE projects & include files", PresetHandler::IconType::Question);

		if (cleanEverything)
		{
			buildDirectory.deleteRecursively();
			buildDirectory.createDirectory();

			auto resourceDirectory = buildDirectory.getParentDirectory().getChildFile("PooledResources");

			if (resourceDirectory.isDirectory())
			{
				resourceDirectory.deleteRecursively();
				resourceDirectory.createDirectory();
			}

		}
		else
		{
			buildDirectory.getChildFile("Builds").deleteRecursively();
			buildDirectory.getChildFile("JuceLibraryCode").deleteRecursively();
		}
	}
}

void BackendCommandTarget::Actions::convertAllSamplesToMonolith(BackendRootWindow * bpe)
{
	ModulatorSampler* sampler = dynamic_cast<ModulatorSampler*>(ProcessorHelpers::getFirstProcessorWithName(bpe->getMainSynthChain(), "Sampler"));

	if (sampler != nullptr)
	{
		MonolithConverter *converter = new MonolithConverter(bpe);

		converter->setModalBaseWindowComponent(bpe);
	}
	else
	{
		PresetHandler::showMessageWindow("Missing convert sampler", "You need a sampler with the name 'Sampler' in the Master Chain!", PresetHandler::IconType::Error);
	}
}

void BackendCommandTarget::Actions::convertSfzFilesToSampleMaps(BackendRootWindow * bpe)
{
	ModulatorSampler* sampler = dynamic_cast<ModulatorSampler*>(ProcessorHelpers::getFirstProcessorWithName(bpe->getMainSynthChain(), "Sampler"));

	if (sampler != nullptr)
	{
		FileChooser fc("Select SFZ files to convert", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Samples), "*.sfz;*.SFZ");

		if (fc.browseForMultipleFilesToOpen())
		{
			auto files = fc.getResults();

			for (auto f : files)
			{
				SfzImporter importer(sampler, f);

				importer.importSfzFile();

				const String id = f.getFileNameWithoutExtension();

				sampler->getSampleMap()->setId(f.getFileNameWithoutExtension());
				auto sampleMap = sampler->getSampleMap()->getValueTree();

				File sampleMapFile = GET_PROJECT_HANDLER(sampler).getSubDirectory(ProjectHandler::SubDirectories::SampleMaps).getChildFile(id + ".xml");

				ScopedPointer<XmlElement> xml = sampleMap.createXml();
				xml->writeToFile(sampleMapFile, "");
			}
		}
	}
	else
	{
		PresetHandler::showMessageWindow("Missing convert sampler", "You need a sampler with the name 'Sampler' in the Master Chain!", PresetHandler::IconType::Error);
	}
}

void BackendCommandTarget::Actions::checkAllSamplemaps(BackendRootWindow * bpe)
{
	if (auto exp = bpe->getMainController()->getExpansionHandler().getCurrentExpansion())
	{
		exp->checkAllSampleMaps();
	}
	else
	{
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).checkAllSampleMaps();
	}
}

void removeHiddenFilesFromList(Array<File> &list)
{
	for (int i = 0; i < list.size(); i++)
	{
		if (list[i].isHidden() || list[i].getFileName().startsWith("."))
		{
			list.remove(i--);
		}
	}
}

void BackendCommandTarget::Actions::validateUserPresets(BackendRootWindow * bpe)
{
	MainController* mc = bpe->getBackendProcessor();
	File userPresetDirectory = GET_PROJECT_HANDLER(mc->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::UserPresets);
	Array<File> userPresets;
	userPresetDirectory.findChildFiles(userPresets, File::findFiles, true, "*.preset");
	removeHiddenFilesFromList(userPresets);

	if (PresetHandler::showYesNoWindow("Update for missing controls", "Do you want to add default values if controls are missing?"))
	{
		int numChangedPresets = 0;
		int maxNumChangedControls = 0;

		

		if (numChangedPresets != 0)
			PresetHandler::showMessageWindow("Presets changed", String(numChangedPresets) + " user presets were modified and up to " + String(maxNumChangedControls) + " controls were added.", PresetHandler::IconType::Info);

		int numWrongVersionedFiles = 0;

		if (PresetHandler::showYesNoWindow("Validate version", "Do you want to check / update the version", PresetHandler::IconType::Question))
		{
			for (auto f : userPresets)
			{
				if (UserPresetHelpers::updateVersionNumber(mc->getMainSynthChain(), f))
					numWrongVersionedFiles++;
			}
		}

		if (numWrongVersionedFiles != 0)
			PresetHandler::showMessageWindow("Version updated", String(numWrongVersionedFiles) + " user presets were updated to the recent version", PresetHandler::IconType::Info);

		if (numChangedPresets == 0 && numWrongVersionedFiles == 0)
			PresetHandler::showMessageWindow("Nothing to do", "All user presets are up to date.");
	}
}

void BackendCommandTarget::Actions::unloadAllAudioFiles(BackendRootWindow * bpe)
{
	auto synthChain = bpe->getMainSynthChain();

	Processor::Iterator<AudioSampleProcessor> iter(synthChain);

	while (auto asp = iter.getNextProcessor())
	{
		asp->setLoadedFile("", true);
	}
}


void BackendCommandTarget::Actions::createUIDataFromDesktop(BackendRootWindow * bpe)
{
	auto mp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(bpe->getBackendProcessor());

	if (mp != nullptr)
	{
		mp->createUICopyFromDesktop();
	}
}

#define REPLACE_WILDCARD(wildcard, x) templateProject = templateProject.replace(wildcard, data.getSetting(x).toString())
#define REPLACE_WILDCARD_WITH_STRING(wildcard, s) (templateProject = templateProject.replace(wildcard, s))

juce::String BackendCommandTarget::Actions::createWindowsInstallerTemplate(MainController* mc, bool includeAAX, bool include32, bool include64, bool includeRLottie)
{
	String templateProject(winInstallerTemplate);
	
	auto& data = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject();

	REPLACE_WILDCARD("%PRODUCT%", HiseSettings::Project::Name);
	REPLACE_WILDCARD("%VERSION%", HiseSettings::Project::Version);
	REPLACE_WILDCARD("%COMPANY%", HiseSettings::User::Company);

	REPLACE_WILDCARD_WITH_STRING("%AAX%", includeAAX ? "" : ";");
    REPLACE_WILDCARD_WITH_STRING("%32%", include32 ? "" : ";");
    REPLACE_WILDCARD_WITH_STRING("%64%", include64 ? "" : ";");
    REPLACE_WILDCARD_WITH_STRING("%RLOTTIE%", includeRLottie ? "" : ";");

    if(!include32)
        REPLACE_WILDCARD_WITH_STRING("%ARCHITECTURE%", " x64");
    else if (!include64)
        REPLACE_WILDCARD_WITH_STRING("%ARCHITECTURE%", " x86");
    else
        REPLACE_WILDCARD_WITH_STRING("%ARCHITECTURE%", "");
    
	return templateProject;
}

void BackendCommandTarget::Actions::convertSampleMapToWavetableBanks(BackendRootWindow* bpe)
{
	ignoreUnused(bpe);

#if USE_IPP
	WavetableConverterDialog *converter = new WavetableConverterDialog(bpe->getMainSynthChain());

	converter->setModalBaseWindowComponent(bpe);
#else
	PresetHandler::showMessageWindow("IPP required", "You need to build HISE with enabled IPP in order to use the resynthesis features", PresetHandler::IconType::Error);
#endif
}

void BackendCommandTarget::Actions::exportCompileFilesInPool(BackendRootWindow* bpe)
{
	auto pet = new PoolExporter(bpe->getBackendProcessor());
	pet->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::checkDeviceSanity(BackendRootWindow * bpe)
{
	auto window = new DeviceTypeSanityCheck(bpe->getBackendProcessor());
	window->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::copyMissingSampleListToClipboard(BackendRootWindow * bpe)
{
	auto pool = bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool();

	StreamingSamplerSoundArray missingSounds;

	pool->getMissingSamples(missingSounds);

	if (missingSounds.isEmpty())
	{
		PresetHandler::showMessageWindow("No missing samples found", "All samples could be found", PresetHandler::IconType::Info);
	}
	else
	{
		String text;

		for (auto sound : missingSounds)
		{
			if (sound == nullptr)
				continue;

			if (sound->isMonolithic())
				continue;

			text << sound->getFileName(true) << "\n";
		}

		SystemClipboard::copyTextToClipboard(text);
		PresetHandler::showMessageWindow("Missing samples detected", "There were " + String(missingSounds.size()) + " missing samples.", PresetHandler::IconType::Warning);
	}
}

void BackendCommandTarget::Actions::createRecoveryXml(BackendRootWindow * bpe)
{
	FileChooser fc("Choose .hip file to recover", bpe->getBackendProcessor()->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Presets), "*.hip", true);

	if (fc.browseForFileToOpen())
	{
		auto f = fc.getResult();

		FileInputStream fis(f);

		auto vt = ValueTree::readFromStream(fis);

		if (vt.isValid())
		{
			ScopedPointer<XmlElement> xml = vt.createXml();

			if (xml != nullptr)
			{
				auto xmlContent = xml->createDocument("");

				File s = f.getSiblingFile(f.getFileNameWithoutExtension() + "_Recovered.xml");

				s.replaceWithText(xmlContent);

				PresetHandler::showMessageWindow("HIP file successfully recovered", "The XML file was written to " + s.getFullPathName());
				f.getParentDirectory().revealToUser();
				return;
			}
		}

		PresetHandler::showMessageWindow("Danger!", "The file you supplied got corrupted", PresetHandler::IconType::Error);
	}
}

Component* findFocusedComponent(Component* c)
{
	if (c->hasKeyboardFocus(false))
		return c;

	if (c->hasKeyboardFocus(true))
	{
		for (int i = 0; i < c->getNumChildComponents(); i++)
		{
			if (auto comp = findFocusedComponent(c->getChildComponent(i)))
				return comp;
		}
	}

	return nullptr;
}

void BackendCommandTarget::Actions::showDocWindow(BackendRootWindow * bpe)
{
	MarkdownLink l;

	if (auto comp = findFocusedComponent(bpe))
	{
		if (auto asHelp = dynamic_cast<ComponentWithDocumentation*>(comp))
		{
			l = asHelp->getLink();
		}
		else if (auto compWithHelp = comp->findParentComponentOfClass<ComponentWithDocumentation>())
		{
			l = compWithHelp->getLink();
		}
	}

	bpe->createOrShowDocWindow(l);
}

#undef REPLACE_WILDCARD
#undef REPLACE_WILDCARD_WITH_STRING

#undef ADD_ALL_PLATFORMS
#undef ADD_IOS_ONLY
#undef ADD_DESKTOP_ONLY
#undef toggleVisibility

bool BackendCommandTarget::Helpers::deviceTypeHasUIData(BackendRootWindow* bpe)
{
	auto mp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(bpe->getBackendProcessor());

	if (mp == nullptr)
		return false;

	return mp->hasUIDataForDeviceType();
}

bool BackendCommandTarget::Helpers::canCopyDeviceType(BackendRootWindow* bpe)
{
	if (!HiseDeviceSimulator::isMobileDevice())
		return false;

	auto mp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(bpe->getBackendProcessor());

	if (mp == nullptr)
		return false;

	return !mp->hasUIDataForDeviceType();
	
}

} // namespace hise
