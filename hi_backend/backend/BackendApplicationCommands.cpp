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




#define ADD_MENU_ITEM(x) jassert(checkSanity(x)); p.addCommandItem(mainCommandManager, x);

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
    
    updater = new Updater(*this);
    
	bpe->addKeyListener(mainCommandManager->getKeyMappings());
	mainCommandManager->setFirstCommandTarget(this);
	mainCommandManager->commandStatusChanged();
}

void BackendCommandTarget::getAllCommands(Array<CommandID>& commands)
{
	const CommandID id[] = { 
		Settings,
		WorkspaceScript,
		WorkspaceSampler,
		WorkspaceCustom,
		MenuSnippetFileNew,
		MenuSnippetClose,
		MenuNewFile,
		MenuOpenFile,
		MenuSaveFile,
		MenuSaveFileAs,
		MenuSaveFileXmlBackup,
		MenuSaveFileAsXmlBackup,
		MenuOpenXmlBackup,
		MenuProjectNew,
		MenuProjectLoad,
		MenuFileBrowseExamples,
		MenuFileCreateRecoveryXml,
		MenuProjectShowInFinder,
		MenuFileSettings,
		MenuExportCleanBuildDirectory,
		MenuToolsCreateThirdPartyNode,
		MenuFileExtractEmbeddeSnippetFiles,
		MenuFileImportSnippet,
		MenuExportSetupWizard,
		MenuExportFileAsPlugin,
		MenuExportFileAsEffectPlugin,
		MenuExportFileAsMidiFXPlugin,
		MenuExportFileAsStandaloneApp,
		MenuExportProjectAsExpansion,
		MenuExportFileAsSnippet,
		MenuExportSampleDataForInstaller,
		MenuExportCompileFilesInPool,
		MenuExportCompileNetworksAsDll,
		MenuToolsWavetablesToMonolith,
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
		MenuToolsEditShortcuts,
		MenuToolsRecompile,
        MenuViewClearConsole,
		MenuToolsCheckCyclicReferences,
        MenuExportCheckPluginParameters,
		MenuToolsConvertSVGToPathData,
        MenuToolsBroadcasterWizard,
		MenuExportRestoreToDefault,
		MenuExportValidateUserPresets,
		MenuExportCheckAllSampleMaps,
		MenuExportCheckUnusedImages,
		MenuToolsForcePoolSearch,
		MenuToolsConvertSampleMapToWavetableBanks,
		MenuToolsConvertAllSamplesToMonolith,
		MenuToolsUpdateSampleMapIdsBasedOnFileName,
		MenuToolsConvertSfzToSampleMaps,
		MenuExportUnloadAllSampleMaps,
		MenuExportUnloadAllAudioFiles,
		MenuToolsRecordOneSecond,
		MenuToolsImportArchivedSamples,
		MenuToolsCreateRSAKeys,
		MenuToolsCreateDummyLicenseFile,
		MenuToolsApplySampleMapProperties,
		MenuToolsSimulateChangingBufferSize,
		MenuToolsShowDspNetworkDllInfo,
        MenuToolsCreateRnboTemplate,
		MenuViewResetLookAndFeel,
		MenuViewReset,
        MenuViewRotate,
		MenuViewEnableGlobalLayoutMode,
		MenuViewAddFloatingWindow,
        MenuViewToggleSnippetBrowser,
        MenuViewGotoUndo,
        MenuViewGotoRedo,
		MenuHelpShowAboutPage,
        MenuHelpCheckVersion,
		MenuHelpShowDocumentation
	};
	commands.addArray(id, numElementsInArray(id));

	commands.sort();
}

void BackendCommandTarget::setCopyPasteTarget(CopyPasteTarget* newTarget)
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
		result.categoryName = "View";
		break;
	}
	case MenuSnippetFileNew:
		setCommandTarget(result, "Show snippet browser", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuSnippetClose:
		setCommandTarget(result, "Close this window", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuNewFile:
		setCommandTarget(result, "New", true, false, 'N');
		result.categoryName = "File";
		break;
	case MenuOpenFile:
		setCommandTarget(result, "Open Archive", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuSaveFile: {
		setCommandTarget(result, "Save Archive", true, false, 'S', false);
		auto k = TopLevelWindowWithKeyMappings::getFirstKeyPress(bpe, FloatingTileKeyPressIds::save_hip);
		result.addDefaultKeypress(k.getKeyCode(), k.getModifiers());
		result.categoryName = "File";
		break; }
	case MenuSaveFileAs:
		setCommandTarget(result, "Save As Archive", true, false, 'X', false);

		result.categoryName = "File";
		break;
	case MenuSaveFileXmlBackup: {
	  	setCommandTarget(result, "Save XML", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'S', false);

		auto k = TopLevelWindowWithKeyMappings::getFirstKeyPress(bpe, FloatingTileKeyPressIds::save_xml);
		result.addDefaultKeypress(k.getKeyCode(), k.getModifiers());

		result.categoryName = "File";
		break; }
	case MenuSaveFileAsXmlBackup:
		setCommandTarget(result, "Save as XML", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuOpenXmlBackup:
		setCommandTarget(result, "Open XML", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'O');
		result.categoryName = "File";
		break;
	case MenuFileBrowseExamples:
		setCommandTarget(result, "Browse example snippets", true, false, 'x', false);
		result.categoryName = "Help";
		break;
	case MenuProjectNew:
		setCommandTarget(result, "Create new Project", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuProjectLoad:
		setCommandTarget(result, "Load Project", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuFileExtractEmbeddeSnippetFiles:
		setCommandTarget(result, "Copy snippet script files to current project", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
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
	case MenuToolsCreateThirdPartyNode:
		setCommandTarget(result, "Create C++ third party node template", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuExportSetupWizard:
		setCommandTarget(result, "Setup Export Wizard", true, false, 'X', false);
		result.categoryName = "Export";
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
	case MenuExportProjectAsExpansion:
		setCommandTarget(result, "Export Project as Full Expansion", true, false, 'X', false);
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
	case MenuExportCompileNetworksAsDll:
		setCommandTarget(result, "Compile DSP networks as dll", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuFileCreateRecoveryXml:
		setCommandTarget(result, "Create recovery XML from Archive", true, false, 'x', false);
		result.categoryName = "File";
		break;
	case MenuExportSampleDataForInstaller:
		setCommandTarget(result, "Package sample monolith files", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuToolsWavetablesToMonolith:
		setCommandTarget(result, "Export Wavetables to monolith", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuExportCompileFilesInPool:
		setCommandTarget(result, "Export Pooled Files to Binary Resource", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuFileSettings:
		setCommandTarget(result, "Settings", true, false, 'X', false);
		result.categoryName = "File";
		break;
	case MenuExportCleanBuildDirectory:
		setCommandTarget(result, "Clean Build directory", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuFileImportSnippet:
		setCommandTarget(result, "Import HISE Snippet", true, false, 'V', true, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
		result.categoryName = "File";
		break;
	case MenuFileQuit:
		setCommandTarget(result, "Quit", true, false, 'X', false);
		result.categoryName = "File";
		break;
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
	case MenuToolsEditShortcuts:
		setCommandTarget(result, "Edit Shortcuts", true, false, 'x', false);
		result.categoryName = "File";
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
	case MenuToolsRecompile:
                         setCommandTarget(result, "Recompile all scripts", true, false, 'X', false);
                         result.addDefaultKeypress(KeyPress::F5Key, ModifierKeys::shiftModifier);
						 result.categoryName = "Tools";
        break;
	case MenuToolsCheckCyclicReferences:
		setCommandTarget(result, "Check Javascript objects for cyclic references", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsSimulateChangingBufferSize:
		setCommandTarget(result, "Simulate varying audio buffer size", true, bpe->getBackendProcessor()->isUsingDynamicBufferSize(), 'X', false);

		result.categoryName = "Tools";
		break;
	case MenuToolsBroadcasterWizard:
		setCommandTarget(result, "Show Broadcaster Wizard", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsCreateExternalScriptFile:
		setCommandTarget(result, "Create external script file", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuExportValidateUserPresets:
		setCommandTarget(result, "Validate user presets", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuExportCheckAllSampleMaps:
		setCommandTarget(result, "Validate sample maps", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "Export";
		break;
     case MenuExportCheckPluginParameters:
         setCommandTarget(result, "Validate plugin parameters", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
         result.categoryName = "Export";
         break;
                         
	case MenuToolsImportArchivedSamples:
		setCommandTarget(result, "Import archived samples", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsShowDspNetworkDllInfo:
		setCommandTarget(result, "Show DSP Network DLL info", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuExportCheckUnusedImages:
		setCommandTarget(result, "Collect unreferenced images", GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive(), false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuToolsForcePoolSearch:
		setCommandTarget(result, "Force duplicate search in pool when loading samples", true, bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool()->isPoolSearchForced(), 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuToolsConvertAllSamplesToMonolith:
		setCommandTarget(result, "Convert all samples to Monolith + Samplemap", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
    case MenuToolsCreateRnboTemplate:
        setCommandTarget(result, "Create C++ template for RNBO patch", true, false, 'X', false);
        result.categoryName = "Tools";
        break;
	case MenuToolsConvertSampleMapToWavetableBanks:
		setCommandTarget(result, "Show Wavetable Creator", true, false, 'X', false);
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
	case MenuExportUnloadAllSampleMaps:
		setCommandTarget(result, "Unload all Samplemaps", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuToolsApplySampleMapProperties:
		setCommandTarget(result, "Apply sample map properties to sample files", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuExportUnloadAllAudioFiles:
		setCommandTarget(result, "Unload all audio files", true, false, 'X', false);
		result.categoryName = "Export";
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
	case MenuToolsConvertSVGToPathData:
		setCommandTarget(result, "Show SVG to Path Converter", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuExportRestoreToDefault:
		setCommandTarget(result, "Reset UI controls to default values", true, false, 'X', false);
		result.categoryName = "Export";
		break;
	case MenuToolsCreateDummyLicenseFile:
		setCommandTarget(result, "Create Dummy License File", true, false, 'X', false);
		result.categoryName = "Tools";
		break;
	case MenuViewResetLookAndFeel:
		setCommandTarget(result, "Reset custom Look and Feel", true, false, 'X', false);
		result.categoryName = "View";
		break;
    case MenuViewToggleSnippetBrowser:
        setCommandTarget(result, "Toggle Snippet Browser", true, false, 'X', false);
        result.categoryName = "View";
        break;
	case MenuViewReset:
		setCommandTarget(result, "Reset Workspaces", true, false, 'X', false);
		result.categoryName = "View";
		break;
    case MenuViewGotoUndo:
    case MenuViewGotoRedo:
    {
        auto isUndo = commandID == MenuViewGotoUndo;
        auto l = bpe->getBackendProcessor()->getLocationUndoManager();
        auto shortcutId = isUndo ? TextEditorShortcuts::goto_undo : TextEditorShortcuts::goto_redo;
        String name;
            
        if(isUndo)
        {
            
            name << "Go back to ";
            name << l->getUndoDescription();
        }
        else
            name << "Goto next location";
            
        result.setInfo(name, name, "Unused", 0);
        result.setActive(true);
        result.categoryName = "View";
            result.defaultKeypresses.add(TopLevelWindowWithKeyMappings::getFirstKeyPress(bpe, shortcutId));
        
        break;
    }
	case MenuViewRotate:
        setCommandTarget(result, "Vertical Layout", true, bpe->isRotated(), 'X', false);
		result.categoryName = "View";
        break;
	case MenuViewEnableGlobalLayoutMode:
		setCommandTarget(result, "Enable Layout Mode", true, bpe->getRootFloatingTile()->isLayoutModeEnabled(), 'X', false);
		result.categoryName = "View";
		break;
	case MenuViewAddFloatingWindow:
		setCommandTarget(result, "Add floating window", true, false, 'x', false);
		result.categoryName = "View";
		break;
    case MenuViewClearConsole:
        setCommandTarget(result, "Clear Console", true, false, 'X', false);
		result.categoryName = "View";
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
    case MenuHelpCheckVersion:
        setCommandTarget(result, "Check for newer version", true, false, 'X', false);
		result.categoryName = "Help";
        break;
            
	default:					jassertfalse; return;
	}
}

bool BackendCommandTarget::perform(const InvocationInfo &info)
{
	switch (info.commandID)
	{
	case HamburgerMenu:					Actions::showMainMenu(bpe);  return true;
	case Settings:                      bpe->showSettingsWindow(); return true;
	case WorkspaceScript:
	case WorkspaceSampler:
	case WorkspaceCustom:				bpe->showWorkspace(info.commandID); updateCommands(); return true;
	case MenuViewToggleSnippetBrowser:	bpe->toggleSnippetBrowser(); return true;
	case MenuSnippetClose:				bpe->deleteThisSnippetInstance(false); return true;
	case MenuNewFile:                   Actions::newFile(bpe); return true;
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
	case MenuProjectShowInFinder:		Actions::showProjectInFinder(bpe); return true;
	case MenuFileBrowseExamples:		Actions::showExampleBrowser(bpe); return true;
	case MenuFileCreateRecoveryXml:		Actions::createRecoveryXml(bpe); return true;
	case MenuFileSettings:				Actions::showFileProjectSettings(bpe); return true;
	case MenuExportCleanBuildDirectory:	Actions::cleanBuildDirectory(bpe); return true;
	case MenuToolsCreateThirdPartyNode:	Actions::createThirdPartyNode(bpe); return true;
	case MenuFileImportSnippet: Actions::replaceWithClipboardContent(bpe); return true;
	case MenuFileExtractEmbeddeSnippetFiles: Actions::extractEmbeddedFilesFromSnippet(bpe); return true;
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
	case MenuToolsCheckCyclicReferences:Actions::checkCyclicReferences(bpe); return true;
	case MenuToolsCreateExternalScriptFile:	Actions::createExternalScriptFile(bpe); updateCommands(); return true;
	case MenuExportValidateUserPresets:	Actions::validateUserPresets(bpe); return true;
	case MenuExportRestoreToDefault:		Actions::restoreToDefault(bpe); return true;
	case MenuExportCheckUnusedImages:	Actions::checkUnusedImages(bpe); return true;
	case MenuExportSetupWizard:			Actions::setupExportWizard(bpe); return true;
	case MenuToolsShowDspNetworkDllInfo: Actions::showNetworkDllInfo(bpe); return true;
	case MenuToolsForcePoolSearch:		Actions::toggleForcePoolSearch(bpe); updateCommands(); return true;
	case MenuToolsConvertSampleMapToWavetableBanks:	Actions::convertSampleMapToWavetableBanks(bpe); return true;
	case MenuToolsConvertAllSamplesToMonolith:	Actions::convertAllSamplesToMonolith(bpe); return true;
	case MenuToolsUpdateSampleMapIdsBasedOnFileName:	Actions::updateSampleMapIds(bpe); return true;
	case MenuToolsConvertSfzToSampleMaps:	Actions::convertSfzFilesToSampleMaps(bpe); return true;
	case MenuExportUnloadAllSampleMaps:	Actions::removeAllSampleMaps(bpe); return true;
	case MenuToolsSimulateChangingBufferSize: bpe->getBackendProcessor()->toggleDynamicBufferSize(); return true;
	case MenuExportUnloadAllAudioFiles:  Actions::unloadAllAudioFiles(bpe); return true;
	case MenuToolsCreateRSAKeys:		Actions::createRSAKeys(bpe); return true;
	case MenuToolsCreateDummyLicenseFile: Actions::createDummyLicenseFile(bpe); return true;
	case MenuExportCheckAllSampleMaps:	Actions::checkAllSamplemaps(bpe); return true;
    case MenuExportCheckPluginParameters:    Actions::checkPluginParameterSanity(bpe); return true;
    case MenuToolsCreateRnboTemplate:   Actions::createRnboTemplate(bpe); return true;
	case MenuToolsImportArchivedSamples: Actions::importArchivedSamples(bpe); return true;
	case MenuToolsRecordOneSecond:		bpe->owner->getDebugLogger().startRecording(); return true;
    case MenuToolsEnableDebugLogging:	bpe->owner->getDebugLogger().toggleLogging(); updateCommands(); return true;
	case MenuToolsApplySampleMapProperties: Actions::applySampleMapProperties(bpe); return true;
	case MenuToolsConvertSVGToPathData:	Actions::convertSVGToPathData(bpe); return true;
    case MenuToolsBroadcasterWizard:
    {
        auto s = new multipage::library::BroadcasterWizard(bpe);
        s->setModalBaseWindowComponent(bpe);
        return true;
    }
	case MenuToolsEditShortcuts:		Actions::editShortcuts(bpe); return true;
	case MenuViewReset:				    bpe->resetInterface(); updateCommands(); return true;
	case MenuViewRotate:
        bpe->toggleRotate();
        updateCommands();
        return true;
	case MenuViewEnableGlobalLayoutMode: bpe->toggleLayoutMode(); updateCommands(); return true;
	case MenuViewAddFloatingWindow:		bpe->addFloatingWindow(); return true;
    case MenuViewGotoUndo: bpe->getBackendProcessor()->getLocationUndoManager()->undo(); updateCommands(); return true;
    case MenuViewGotoRedo:  bpe->getBackendProcessor()->getLocationUndoManager()->redo(); updateCommands(); return true;
	case MenuExportFileAsPlugin:
		Actions::exportProject(bpe, (int)CompileExporter::BuildOption::AllPluginFormatsInstrument);
        return true;
	case MenuExportFileAsEffectPlugin:
		Actions::exportProject(bpe, (int)CompileExporter::BuildOption::AllPluginFormatsFX);
        return true;
	case MenuExportFileAsStandaloneApp:
		Actions::exportProject(bpe, (int)CompileExporter::BuildOption::StandaloneLinux);
        return true;
	case MenuExportFileAsMidiFXPlugin:
    	Actions::exportProject(bpe, (int)CompileExporter::BuildOption::AllPluginFormatsMidiFX);
        return true;
	case MenuExportCompileNetworksAsDll: Actions::compileNetworksToDll(bpe); return true;
    case MenuExportFileAsSnippet:        Actions::exportFileAsSnippet(bpe); return true;
	case MenuExportProjectAsExpansion:				Actions::exportHiseProject(bpe); return true;
	case MenuExportSampleDataForInstaller: Actions::exportSampleDataForInstaller(bpe); return true;
	case MenuToolsWavetablesToMonolith: Actions::exportWavetablesToMonolith(bpe); return true;
	case MenuExportCompileFilesInPool:	Actions::exportCompileFilesInPool(bpe); return true;
	case MenuViewResetLookAndFeel:		Actions::resetLookAndFeel(bpe); return true;
    case MenuViewClearConsole:         owner->getConsoleHandler().clearConsole(); return true;
	case MenuHelpShowAboutPage:			Actions::showAboutPage(bpe); return true;
    case MenuHelpCheckVersion:          Actions::checkVersion(bpe); return true;
	case MenuHelpShowDocumentation:		Actions::showDocWindow(bpe); return true;
	}

	return false;
}



PopupMenu BackendCommandTarget::getMenuForIndex(int topLevelMenuIndex, const String &menuName)
{
	MenuNames m = (MenuNames)topLevelMenuIndex;

	auto isSnippetBrowser = bpe->getBackendProcessor()->isSnippetBrowser();
	auto categoryIds = mainCommandManager->getCommandsInCategory(menuName.upToFirstOccurrenceOf(" ", false, false));

	jassert(!categoryIds.isEmpty());

#if JUCE_DEBUG
	int lastMenuId = 0;
	bool allowCheck = true;//

	auto checkSanity = [&](MainToolbarCommands x)
	{
		if(!allowCheck)
			return true;

		// If this hits, then you need to make sure
		// that the category from getCommandInfo() matches the menu name
		jassert(categoryIds.contains(x));

		auto prev = (MainToolbarCommands)lastMenuId;

		// If this hits, then the order of the command menu definition
		// is not correct and needs to be shuffled in the enum definition
		// to match the menu order
		jassert(prev < x);
		ignoreUnused(prev);

		lastMenuId = x;
		return true;
	};
#endif
	

	PopupMenu p;

	

	switch (m)
	{
	case BackendCommandTarget::FileMenu: {

		if(isSnippetBrowser)
		{
			
			ADD_MENU_ITEM(MenuNewFile);
			ADD_MENU_ITEM(MenuFileImportSnippet);
			ADD_MENU_ITEM(MenuFileExtractEmbeddeSnippetFiles);
			ADD_MENU_ITEM(MenuSnippetClose);
		}
		else
		{
			p.addSectionHeader("Project Management");
			ADD_MENU_ITEM(MenuProjectNew);
			ADD_MENU_ITEM(MenuProjectLoad);
			
			ADD_MENU_ITEM(MenuProjectShowInFinder);
			

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

			ADD_MENU_ITEM(MenuNewFile);
			
			

			
			
			ADD_MENU_ITEM(MenuOpenXmlBackup);
			ADD_MENU_ITEM(MenuSaveFileXmlBackup);
			ADD_MENU_ITEM(MenuSaveFileAsXmlBackup);

			PopupMenu xmlBackups;
			Array<File> xmlBackupFiles = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getFileList(ProjectHandler::SubDirectories::XMLPresetBackups);

			for (int i = 0; i < xmlBackupFiles.size(); i++)
			{
				xmlBackups.addItem(i + MenuFileXmlBackupMenuOffset, xmlBackupFiles[i].getFileName());
			}

			p.addSubMenu("Open recent XML", xmlBackups);

			p.addSeparator();

			

			

			ADD_MENU_ITEM(MenuOpenFile);
			ADD_MENU_ITEM(MenuSaveFile);
			

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

			ADD_MENU_ITEM(MenuFileImportSnippet);
			ADD_MENU_ITEM(MenuFileCreateRecoveryXml);

	#if HISE_IOS
	#else

			p.addSeparator();

			ADD_MENU_ITEM(MenuFileSettings);
			ADD_MENU_ITEM(MenuToolsEditShortcuts);

			p.addSeparator();
			ADD_MENU_ITEM(MenuFileQuit);
	#endif
		}

		break;
		}
		case BackendCommandTarget::EditMenu:
			{

			ADD_MENU_ITEM(MenuEditUndo);
			ADD_MENU_ITEM(MenuEditRedo);
			p.addSeparator();

	        if(dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get()))
	        {
	            dynamic_cast<JavascriptCodeEditor*>(bpe->currentCopyPasteTarget.get())->addPopupMenuItems(p, nullptr);
	            
	        }
	        else
	        {
	            ADD_MENU_ITEM(MenuEditCopy);
	            ADD_MENU_ITEM(MenuEditPaste);
	            p.addSeparator();

	            ADD_MENU_ITEM(MenuEditCreateScriptVariable);
				ADD_MENU_ITEM(MenuEditCreateBase64State);
	        }
		}

		
		break;
	case BackendCommandTarget::ExportMenu:
	{
		if(isSnippetBrowser)
		{
			ADD_MENU_ITEM(MenuExportFileAsSnippet);
		}
		else 
		{
			ADD_MENU_ITEM(MenuExportSetupWizard);

			p.addSectionHeader("Export As");
			ADD_MENU_ITEM(MenuExportFileAsPlugin);
			ADD_MENU_ITEM(MenuExportFileAsEffectPlugin);
			ADD_MENU_ITEM(MenuExportFileAsMidiFXPlugin);
			ADD_MENU_ITEM(MenuExportFileAsStandaloneApp);
			
			p.addSeparator();

			ADD_MENU_ITEM(MenuExportFileAsSnippet);
			ADD_MENU_ITEM(MenuExportProjectAsExpansion);

			p.addSeparator();

			p.addSectionHeader("Validation Tools");

			ADD_MENU_ITEM(MenuExportCheckAllSampleMaps);
			ADD_MENU_ITEM(MenuExportCheckPluginParameters);
			ADD_MENU_ITEM(MenuExportValidateUserPresets);
			ADD_MENU_ITEM(MenuExportCheckUnusedImages);

			p.addSeparator();

			p.addSectionHeader("Cleanup Tools");

			ADD_MENU_ITEM(MenuExportRestoreToDefault);
			ADD_MENU_ITEM(MenuExportUnloadAllSampleMaps);
			ADD_MENU_ITEM(MenuExportUnloadAllAudioFiles);
			ADD_MENU_ITEM(MenuExportCleanBuildDirectory);

			p.addSeparator();

			p.addSectionHeader("Export Tools");
			
			ADD_MENU_ITEM(MenuExportSampleDataForInstaller);
			
			ADD_MENU_ITEM(MenuExportCompileFilesInPool);
			ADD_MENU_ITEM(MenuExportCompileNetworksAsDll);
		}

		break;
	}
	case BackendCommandTarget::ToolsMenu:
	{
		if(isSnippetBrowser)
		{
            ADD_MENU_ITEM(MenuToolsRecompile);
            ADD_MENU_ITEM(MenuToolsConvertSVGToPathData);
            ADD_MENU_ITEM(MenuToolsBroadcasterWizard);
            p.addSeparator();
            ADD_MENU_ITEM(MenuToolsShowDspNetworkDllInfo);
            ADD_MENU_ITEM(MenuToolsRecordOneSecond);
            ADD_MENU_ITEM(MenuToolsSimulateChangingBufferSize);

		}
		else
		{
			p.addSectionHeader("Scripting Tools");
		
			ADD_MENU_ITEM(MenuToolsRecompile);
			ADD_MENU_ITEM(MenuToolsCheckCyclicReferences);
			ADD_MENU_ITEM(MenuToolsConvertSVGToPathData);
            ADD_MENU_ITEM(MenuToolsBroadcasterWizard);
            
			p.addSeparator();
			p.addSectionHeader("Sample Management");
			
			ADD_MENU_ITEM(MenuToolsApplySampleMapProperties);
			ADD_MENU_ITEM(MenuToolsImportArchivedSamples);
			ADD_MENU_ITEM(MenuToolsForcePoolSearch);
			
			ADD_MENU_ITEM(MenuToolsConvertAllSamplesToMonolith);
			ADD_MENU_ITEM(MenuToolsUpdateSampleMapIdsBasedOnFileName);
			ADD_MENU_ITEM(MenuToolsConvertSfzToSampleMaps);
			
			p.addSeparator();

			p.addSectionHeader("Wavetable Tools");

			ADD_MENU_ITEM(MenuToolsConvertSampleMapToWavetableBanks);
			ADD_MENU_ITEM(MenuToolsWavetablesToMonolith);

			p.addSeparator();

			p.addSectionHeader("DSP Tools");

			ADD_MENU_ITEM(MenuToolsShowDspNetworkDllInfo);
			ADD_MENU_ITEM(MenuToolsRecordOneSecond);
			ADD_MENU_ITEM(MenuToolsSimulateChangingBufferSize);
	        ADD_MENU_ITEM(MenuToolsCreateRnboTemplate);
			ADD_MENU_ITEM(MenuToolsCreateThirdPartyNode);
			p.addSeparator();
			p.addSectionHeader("License Management");
			ADD_MENU_ITEM(MenuToolsCreateRSAKeys);
			ADD_MENU_ITEM(MenuToolsCreateDummyLicenseFile);
			
		}
		
		break;
	}
	case BackendCommandTarget::ViewMenu: {

		if(isSnippetBrowser)
		{
            ADD_MENU_ITEM(MenuViewGotoUndo);
            ADD_MENU_ITEM(MenuViewGotoRedo);
            
            p.addSeparator();

            ADD_MENU_ITEM(MenuViewToggleSnippetBrowser);
            ADD_MENU_ITEM(WorkspaceCustom);
            
            p.addSeparator();

			ADD_MENU_ITEM(MenuViewClearConsole);
            ADD_MENU_ITEM(MenuViewResetLookAndFeel);

		}
		else
		{
			ADD_MENU_ITEM(MenuViewGotoUndo);
	        ADD_MENU_ITEM(MenuViewGotoRedo);
	        
	        p.addSeparator();
	        
	        ADD_MENU_ITEM(MenuViewRotate);
			ADD_MENU_ITEM(MenuViewEnableGlobalLayoutMode);

			p.addSeparator();
			ADD_MENU_ITEM(WorkspaceCustom);
			ADD_MENU_ITEM(MenuViewAddFloatingWindow);
			
	        p.addSeparator();

			ADD_MENU_ITEM(MenuViewClearConsole);
	        ADD_MENU_ITEM(MenuViewResetLookAndFeel);
	        ADD_MENU_ITEM(MenuViewReset);
		}
        
		break;
		}
	case BackendCommandTarget::HelpMenu:
			ADD_MENU_ITEM(MenuHelpShowDocumentation);
			ADD_MENU_ITEM(MenuFileBrowseExamples);
			p.addSeparator();
			ADD_MENU_ITEM(MenuHelpCheckVersion);
			ADD_MENU_ITEM(MenuHelpShowAboutPage);
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
}



void BackendCommandTarget::Actions::editShortcuts(BackendRootWindow* bpe)
{
	auto s = new ShortcutEditor(bpe);

	s->setModalBaseWindowComponent(bpe);
}

bool BackendCommandTarget::Actions::hasProcessorInClipboard()
{
	if (auto xml = XmlDocument::parse(SystemClipboard::getTextFromClipboard()))
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
		loadSnippet(bpe, clipboardContent);
		return;
	}
	else
	{
		if (auto xml = XmlDocument::parse(clipboardContent))
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

void BackendCommandTarget::Actions::loadSnippet(BackendRootWindow* bpe, const String& snippet)
{
	String data = snippet.fromFirstOccurrenceOf("HiseSnippet ", false, false);

	MemoryBlock mb;

	mb.fromBase64Encoding(data);

	MemoryInputStream mis(mb, false);

	GZIPDecompressorInputStream dezipper(&mis, false);

	ValueTree v = ValueTree::readFromGZIPData(mb.getData(), mb.getSize());

	if (v.isValid())
	{
		bpe->loadNewContainer(v);
	}
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

		auto xml = desc->createXml();

		Logger::writeToLog("Plugin description:");
		Logger::writeToLog(xml->createDocument(""));

		Logger::writeToLog("Initialising...");

		auto plugin = fm.createPluginInstance(*desc, 44100.0, 512, error);

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

void BackendCommandTarget::Actions::newFile(BackendRootWindow* bpe)
{
	if (PresetHandler::showYesNoWindow("New File", "Do you want to start a new preset?"))
	{
		bpe->mainEditor->clearPreset();
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

		ProjectHandler::createLinkFileInFolder(ProjectHandler::getAppDataDirectory(nullptr).getChildFile("scripts"), f);
	}
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





void BackendCommandTarget::Actions::resetLookAndFeel(BackendRootWindow* bpe)
{
	

	bpe->owner->resetLookAndFeelToDefault(bpe);
}


void BackendCommandTarget::Actions::closeAllChains(BackendRootWindow *bpe)
{
    ProcessorEditor *editor = dynamic_cast<ProcessorEditor*>(bpe->currentCopyPasteTarget.get());
    
    if(editor != nullptr)
    {
        editor->getChainBar()->closeAll();
    }
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




String BackendCommandTarget::Actions::exportFileAsSnippet(BackendRootWindow* bpe, bool copyToClipboard)
{
    auto bp = bpe->getBackendProcessor();
            
	MainController::ScopedEmbedAllResources sd(bp);
    
	ValueTree v = bp->getMainSynthChain()->exportAsValueTree();
	
	auto scriptRootFolder = bp->getCurrentFileHandler().getSubDirectory(FileHandlerBase::Scripts);
	auto snexRootFolder = BackendDllManager::getSubFolder(bp, BackendDllManager::FolderSubType::CodeLibrary);

	auto embeddedScripts = bp->collectIncludedScriptFilesForSnippet("embeddedScripts", scriptRootFolder);
	auto embeddedSnexFiles = bp->collectIncludedScriptFilesForSnippet("embeddedSnexFiles", snexRootFolder);
	
	MemoryOutputStream mos;

	if(embeddedScripts.getNumChildren() > 0 || embeddedSnexFiles.getNumChildren() > 0)
	{
		ValueTree nv("extended_snippet");
		nv.addChild(v, -1, nullptr);
		nv.addChild(embeddedScripts, -1, nullptr);
		nv.addChild(embeddedSnexFiles, -1, nullptr);
		nv.writeToStream(mos);
	}
	else
		v.writeToStream(mos);

	MemoryOutputStream mos2;
	GZIPCompressorOutputStream zipper(&mos2, 9);
	
	zipper.write(mos.getData(), mos.getDataSize());
	zipper.flush();

	String data = "HiseSnippet " + mos2.getMemoryBlock().toBase64Encoding();

	if(copyToClipboard)
		SystemClipboard::copyTextToClipboard(data);

	if (!MainController::inUnitTestMode() && copyToClipboard)
	{
		PresetHandler::showMessageWindow("Preset copied as compressed snippet", "You can paste the clipboard content to share this preset", PresetHandler::IconType::Info);
	}

	return data;
}

void BackendCommandTarget::Actions::createRnboTemplate(BackendRootWindow* bpe)
{
    auto s = new RNBOTemplateBuilder(bpe);
    s->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::saveFileXml(BackendRootWindow * bpe)
{
	if (PresetHandler::showYesNoWindow("Save XML", "This will save the current XML file"))
	{
		bpe->owner->getUserPresetHandler().initDefaultPresetManager({});

		const String mainSynthChainId = bpe->owner->getMainSynthChain()->getId();
		const bool hasDefaultName = mainSynthChainId == "Master Chain";

		if (!hasDefaultName)
		{
			File f = GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::XMLPresetBackups).getChildFile(mainSynthChainId + ".xml");

			if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
			{
				if (f.existsAsFile())
				{
					if (PresetHandler::showYesNoWindow("Overwrite " + mainSynthChainId, "Overwrite the existing XML?"))
					{
						ValueTree v = bpe->owner->getMainSynthChain()->exportAsValueTree();

			            v.setProperty("BuildVersion", BUILD_SUB_VERSION, nullptr);
			            
						auto xml = v.createXml();
						
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
							XmlBackupFunctions::extractContentData(*xml, interfaceId, f);

						f.replaceWithText(xml->createDocument(""));

						debugToConsole(bpe->owner->getMainSynthChain(), "Save " + mainSynthChainId + " to " + f.getFullPathName());
			            
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
				else
				{
					debugToConsole(bpe->owner->getMainSynthChain(), "Master Chain name is not default but no corresponding XML found, please create XML first");
					Actions::saveFileAsXml(bpe);
				}
			}
		}
		else
		{
			debugToConsole(bpe->owner->getMainSynthChain(), "This project has never been saved as XML, please create XML first");
			Actions::saveFileAsXml(bpe);
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
            
			auto xml = v.createXml();

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
		auto xml = XmlDocument::parse(fileToLoad);

		if (xml != nullptr)
		{
			XmlBackupFunctions::addContentFromSubdirectory(*xml, fileToLoad);
			String newId = xml->getStringAttribute("ID");
			
			auto v = ValueTree::fromXml(*xml);
			XmlBackupFunctions::restoreAllScripts(v, bpe->getMainSynthChain(), newId);
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
	importProject(bpe);

#if 0
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
#endif
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
			bpe->getBackendProcessor()->clearPreset(dontSendNotification);
			loadFirstXmlAfterProjectSwitch(bpe);
		}
			

		
	}
#endif
}

DialogWindowWithBackgroundThread* BackendCommandTarget::Actions::importProject(BackendRootWindow* bpe)
{
	auto importWindow = new ProjectImporter(bpe);
	importWindow->setModalBaseWindowComponent(bpe);

	return importWindow;
}

void BackendCommandTarget::Actions::extractProject(BackendRootWindow* bpe, const File& newProjectRoot, const File& sourceFile)
{
	auto importWindow = new ProjectImporter(bpe);
	importWindow->setModalBaseWindowComponent(bpe);

	jassert(newProjectRoot.isDirectory());
	jassert(sourceFile.existsAsFile());

	importWindow->newProjectFolder = newProjectRoot;
	importWindow->sourceType = ProjectImporter::SourceType::Import;
	importWindow->header->importFile = sourceFile;

	importWindow->runThread();
}

void BackendCommandTarget::Actions::convertSVGToPathData(BackendRootWindow* bpe)
{
	auto pc = new SVGToPathDataConverter(bpe);
	pc->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::applySampleMapProperties(BackendRootWindow* bpe)
{
	auto downloader = new SampleMapPropertySaverWithBackup(bpe);

	downloader->setModalBaseWindowComponent(bpe);
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



void BackendCommandTarget::Actions::showProjectInFinder(BackendRootWindow *bpe)
{
	if (GET_PROJECT_HANDLER(bpe->getMainSynthChain()).isActive())
	{
		GET_PROJECT_HANDLER(bpe->getMainSynthChain()).getWorkDirectory().revealToUser();
	}
}



void BackendCommandTarget::Actions::loadUserPreset(BackendRootWindow *bpe, const File &fileToLoad)
{
    const bool shouldDiscard = !bpe->getBackendProcessor()->isChanged() || PresetHandler::showYesNoWindow("Discard the current preset?", "The current preset will be discarded", PresetHandler::IconType::Question);
    
    if (!shouldDiscard) return;
    
	UserPresetHelpers::loadUserPreset(bpe->getMainSynthChain(), fileToLoad);
}

void BackendCommandTarget::Actions::showFileProjectSettings(BackendRootWindow * bpe)
{
	//SettingWindows::ProjectSettingWindow *window = new SettingWindows::ProjectSettingWindow(&GET_PROJECT_HANDLER(bpe->getMainSynthChain()));

	auto window = new SettingWindows(bpe->getBackendProcessor()->getSettingsObject());
	window->setModalBaseWindowComponent(bpe);
	window->activateSearchBox();
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

Result checkPluginParameterComponent(ScriptingApi::Content* c, ScriptComponent* sc)
{
	auto name = sc->getScriptObjectProperty(ScriptComponent::pluginParameterName).toString();

	if (!sc->getScriptObjectProperty(ScriptComponent::isPluginParameter))
	{
		if (name.isEmpty())
		{
			if(sc->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::isMetaParameter))
				return Result::fail(sc->getName() + " has the isMetaParameter flag set but is not a plugin parameter");
			else
				return Result::ok();
		}
		else
			return Result::fail(sc->getName() + " has an non-empty plugin parameter ID but is not set as plugin parameter");
	}
		
    if(name.isEmpty())
        return Result::fail(sc->getName() + " has an empty plugin parameter ID");
    
    for(int i = 0; i < c->getNumComponents(); i++)
    {
        auto other = c->getComponent(i);
        
        if(sc == other)
            continue;
        
        if(other->getScriptObjectProperty(ScriptComponent::isPluginParameter))
        {
            auto otherName = other->getScriptObjectProperty(ScriptComponent::pluginParameterName).toString();
            auto thisName = sc->getScriptObjectProperty(ScriptComponent::pluginParameterName).toString();
            
            if(otherName == thisName)
            {
                String e;
                e << sc->getName() << " has the same plugin parameter name as " << other->getName();
                return Result::fail(e);
            }
        }
    }
            
    if(!sc->getScriptObjectProperty(ScriptComponent::isMetaParameter))
    {
        auto state = c->exportAsValueTree();
        
        std::map<ScriptComponent*, double> values;
        
        for(int i = 0; i < c->getNumComponents(); i++)
        {
            auto other = c->getComponent(i);
            
            if(other == sc)
                continue;
            
            if(other->getScriptObjectProperty(ScriptComponent::isPluginParameter))
                values.emplace(other, (double)other->getValue());
        }
        
        auto v = sc->getPropertyValueTree();
        
        NormalisableRange<double> nr;
        
        nr.start = (double)v["min"];
        nr.end = (double)v["max"];
        
        if(v.hasProperty("stepSize"))
        {
            nr.interval = (double)v["stepSize"];
        }
        
        if(v.hasProperty("middlePosition"))
        {
            auto midPoint = (double)v["middlePosition"];
            
            if(nr.getRange().contains(midPoint))
                nr.setSkewForCentre(midPoint);
        }
        
        auto newValue = (double)Random::getSystemRandom().nextFloat();
        
        MainController::ScopedBadBabysitter bb(c->getScriptProcessor()->getMainController_());
        
        sc->setValue(newValue);
        
        c->getScriptProcessor()->controlCallback(sc, newValue);
        
        for(const auto& pp: values)
        {
            auto prevValue = pp.second;
            auto currentValue = (double)pp.first->getValue();
            
            if(prevValue != currentValue)
            {
                String e;
                e << "`" << sc->getName() << "` changed another plugin parameter `" << pp.first->getName() << "` without having the `isMetaParameter` flag set";
                
                c->restoreFromValueTree(state);
                return Result::fail(e);
            }
        }
        
        c->restoreFromValueTree(state);
    }
            
    return Result::ok();
}
                         
void BackendCommandTarget::Actions::checkPluginParameterSanity(BackendRootWindow* bpe)
{
	PresetHandler::checkMetaParameters(bpe->owner->synthChain);

    PresetHandler::checkProcessorIdsForDuplicates(bpe->owner->synthChain, false);

	auto chain = bpe->getMainController()->getMainSynthChain();

    auto list = ProcessorHelpers::getListOfAllProcessors<JavascriptMidiProcessor>(chain);
    
    String report;
    String nl = "\n";

	int numPluginParameters = 0;
            
    for(auto jp: list)
    {
        if(!jp->isFront())
            continue;
        
        auto content = jp->getContent();
        
        report << "Plugin parameters from " << jp->getId() << nl;
        
        for(int i = 0; i < content->getNumComponents(); i++)
        {
            auto sc = content->getComponent(i);
            
            auto r = checkPluginParameterComponent(content, sc);
            
            if(!r.wasOk())
            {
                PresetHandler::showMessageWindow("Plugin Parameter validation failed", r.getErrorMessage(), PresetHandler::IconType::Error);
                return;
            }
            
			if (!sc->getScriptObjectProperty(ScriptComponent::isPluginParameter))
				continue;

			report << "ID: " << sc->getName();
			report << " (" << sc->getObjectName() << ") ";
			report << "Parameter ID: " << sc->getScriptObjectProperty(ScriptComponent::pluginParameterName).toString();

			if (sc->getScriptObjectProperty(ScriptComponent::isMetaParameter))
				report << " (meta parameter)";
				
			report << nl;

			numPluginParameters++;
        }
    }

	report << String(numPluginParameters) << " plugin parameters found";

	debugToConsole(chain, report);
	PresetHandler::showMessageWindow("Plugin Parameters OK", "No issues were found. Check the console for a report", PresetHandler::IconType::Info);
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

    StringArray ids = OnlineUnlockStatus::MachineIDUtilities::getLocalMachineIDs();
    
	RSAKey privateKey = RSAKey(handler->getPrivateKey());

	if (!privateKey.isValid())
	{
		PresetHandler::showMessageWindow("No RSA key", "You have to create a RSA Key pair first.", PresetHandler::IconType::Warning);
		return;
	}

	String keyContent = KeyGeneration::generateKeyFile(productName, dummyEmail, userName, ids.joinIntoString("\n"), privateKey);
	File key = handler->getWorkDirectory().getChildFile(productName + FrontendHandler::getLicenseKeyExtension());

	key.replaceWithText(keyContent);
    const String message = "A dummy license file for the plugins was created.";

	PresetHandler::showMessageWindow("License File created", message, PresetHandler::IconType::Info);
}

void BackendCommandTarget::Actions::toggleForcePoolSearch(BackendRootWindow * bpe)
{
	ModulatorSamplerSoundPool *pool = bpe->getBackendProcessor()->getSampleManager().getModulatorSamplerSoundPool();

	pool->setForcePoolSearch(!pool->isPoolSearchForced());
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

void BackendCommandTarget::Actions::exportSampleDataForInstaller(BackendRootWindow * bpe)
{
    auto exporter = new SampleDataExporter(bpe->getMainController());

    exporter->setModalBaseWindowComponent(bpe->mainEditor);
}
                         
void BackendCommandTarget::Actions::exportWavetablesToMonolith(BackendRootWindow* bpe)
{
	auto exporter = new WavetableMonolithExporter(bpe->getMainController());
	exporter->setModalBaseWindowComponent(bpe->mainEditor);
}

void BackendCommandTarget::Actions::exportHiseProject(BackendRootWindow * bpe)
{
	auto e = new ExpansionEncodingWindow(bpe->owner, nullptr, true);

	if (e->encodeResult.failed())
	{
		PresetHandler::showMessageWindow("Encoding Error", e->encodeResult.getErrorMessage(), PresetHandler::IconType::Error);
		return;
	}

	e->setModalBaseWindowComponent(bpe);
}

juce::Result BackendCommandTarget::Actions::exportInstrumentExpansion(BackendProcessor* bp)
{
	ScopedPointer<ExpansionEncodingWindow> ne = new ExpansionEncodingWindow(bp, nullptr, true);

	ne->runSynchronous(false);

	return ne->encodeResult;
}

juce::Result BackendCommandTarget::Actions::createSampleArchive(BackendProcessor* bp)
{
	auto infoFile = GET_PROJECT_HANDLER(bp->getMainSynthChain()).getWorkDirectory().getChildFile("info.hxi");

	auto exporter = new SampleDataExporter(bp);

	if (auto fc = dynamic_cast<FilenameComponent*>(exporter->getCustomComponent(1)))
	{
		
		fc->setCurrentFile(infoFile.getParentDirectory(), dontSendNotification);
	}

	if (infoFile.existsAsFile())
	{
		exporter->showStatusMessage("Adding info.hxi file to metadata");

		if (auto fc = dynamic_cast<FilenameComponent*>(exporter->getCustomComponent(0)))
		{
			fc->setCurrentFile(infoFile, dontSendNotification);
		}
	}

	exporter->runSynchronous();

	return Result::ok();
}

void BackendCommandTarget::Actions::compileNetworksToDll(BackendRootWindow* bpe)
{
	auto exportIsReady = (bool)bpe->getBackendProcessor()->getSettingsObject().getSetting(HiseSettings::Compiler::ExportSetup);

	if(!exportIsReady)
	{
		if(PresetHandler::showYesNoWindow("System not configured", "Your system has not been setup for export. Do you want to launch the Export Setup wizard?"))
			Actions::setupExportWizard(bpe);

		return;
	}

	auto s = new DspNetworkCompileExporter(bpe, bpe->getBackendProcessor());
	s->setModalBaseWindowComponent(bpe);
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

				auto xml = sampleMap.createXml();
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


#define REPLACE_WILDCARD(wildcard, x) templateProject = templateProject.replace(wildcard, data.getSetting(x).toString())
#define REPLACE_WILDCARD_WITH_STRING(wildcard, s) (templateProject = templateProject.replace(wildcard, s))

juce::String BackendCommandTarget::Actions::createWindowsInstallerTemplate(MainController* mc, bool includeAAX, bool include32, bool include64, bool includeVST2, bool includeVST3)
{
	String templateProject(winInstallerTemplate);
	
	auto& data = dynamic_cast<GlobalSettingManager*>(mc)->getSettingsObject();

	REPLACE_WILDCARD("%PRODUCT%", HiseSettings::Project::Name);
	REPLACE_WILDCARD("%VERSION%", HiseSettings::Project::Version);
	REPLACE_WILDCARD("%COMPANY%", HiseSettings::User::Company);

	REPLACE_WILDCARD_WITH_STRING("%AAX%", includeAAX ? "" : ";");
    REPLACE_WILDCARD_WITH_STRING("%32%", include32 ? "" : ";");
    REPLACE_WILDCARD_WITH_STRING("%64%", include64 ? "" : ";");
    
	REPLACE_WILDCARD_WITH_STRING("%VST2%", includeVST2 ? "" : ";");
	REPLACE_WILDCARD_WITH_STRING("%VST2CODE%", includeVST2 ? "" : "//");

	REPLACE_WILDCARD_WITH_STRING("%VST3%", includeVST3 ? "" : ";");
	REPLACE_WILDCARD_WITH_STRING("%VST3CODE%", includeVST3 ? "" : "//");

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

	WavetableConverterDialog *converter = new WavetableConverterDialog(bpe->getMainSynthChain());

	converter->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::exportCompileFilesInPool(BackendRootWindow* bpe)
{
	auto pet = new PoolExporter(bpe->getBackendProcessor());
	pet->setModalBaseWindowComponent(bpe);
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
			if (auto xml = vt.createXml())
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


void BackendCommandTarget::Actions::showNetworkDllInfo(BackendRootWindow * bpe)
{
	auto v = bpe->getBackendProcessor()->dllManager->getStatistics();

	String t;
	
	t << "DLL Info:  \n> `" << JSON::toString(v, true) << "`";

	PresetHandler::showMessageWindow("DllInfo", t, PresetHandler::IconType::Info);
}

void BackendCommandTarget::Actions::createThirdPartyNode(BackendRootWindow* bpe)
{
	auto n = PresetHandler::getCustomName("custom_node", "Please enter the name of the custom node");

	if (n.isNotEmpty())
	{
		using namespace snex::cppgen;

		n = StringHelpers::makeValidCppName(n);

		auto thirdPartyFolder = BackendDllManager::getSubFolder(bpe->getMainController(), BackendDllManager::FolderSubType::ThirdParty);
		auto codeFile = thirdPartyFolder.getChildFile(n).withFileExtension("h");

		Base b(Base::OutputType::AddTabs);

		b.addComment("Third Party Node Template", Base::CommentType::FillTo80);

		{
            b << "#pragma once";
            
            b << "#include <JuceHeader.h>";
            
            b.addEmptyLine();
            
			Namespace pn(b, "project", false);

            auto rawComment = snex::cppgen::Base::CommentType::Raw;
            
			UsingNamespace(b, NamespacedIdentifier("juce"));
			UsingNamespace(b, NamespacedIdentifier("hise"));
			UsingNamespace(b, NamespacedIdentifier("scriptnode"));

            b.addEmptyLine();
            
			Array<NamespacedIdentifier> baseClasses;
			baseClasses.add(snex::NamespacedIdentifier::fromString("data::base"));

			snex::TemplateParameter::List templates;
			templates.add(snex::TemplateParameter(NamespacedIdentifier::fromString("NV"), 0, false));

			b.addComment("The node class with all required callbacks", snex::cppgen::Base::CommentType::FillTo80);

			Struct s(b, Identifier(n), baseClasses, templates, true);

			b.addComment("Metadata Definitions", snex::cppgen::Base::CommentType::FillTo80Light);

			Macro(b, "SNEX_NODE", { n });

			{
				Struct mt(b, "MetadataClass", {}, {});
				cppgen::Macro(b, "SN_NODE_ID", { n.quoted() });
			}
			
			b.addEmptyLine();

			b.addComment("set to true if you want this node to have a modulation dragger", rawComment);
			b << "static constexpr bool isModNode() { return false; };";
			b << "static constexpr bool isPolyphonic() { return NV > 1; };";

			b.addComment("set to true if your node produces a tail", Base::CommentType::Raw);
			b << "static constexpr bool hasTail() { return false; };";

			b.addComment("set to true if your doesn't generate sound from silence and can be suspended when the input signal is silent", Base::CommentType::Raw);
			b << "static constexpr bool isSuspendedOnSilence() { return false; };";

			b.addComment("Undefine this method if you want a dynamic channel count", Base::CommentType::Raw);
			b << "static constexpr int getFixChannelAmount() { return 2; };";

            b.addEmptyLine();

            b.addComment("Define the amount and types of external data slots you want to use", Base::CommentType::Raw);
            
			ExternalData::forEachType([&](ExternalData::DataType t)
			{
				String s;
				s << "static constexpr int Num" << ExternalData::getDataTypeName(t, true) << " = 0;" ;
				b << s;
			});

			b.addEmptyLine();

			auto callbacks = snex::jit::ScriptnodeCallbacks::getAllPrototypes(nullptr, -1);

			callbacks.add(ScriptnodeCallbacks::getPrototype(nullptr, ScriptnodeCallbacks::ID::HandleModulation, 2));
			callbacks.add(ScriptnodeCallbacks::getPrototype(nullptr, ScriptnodeCallbacks::ID::SetExternalDataFunction, 2));
			
			b.addComment("Scriptnode Callbacks", snex::cppgen::Base::CommentType::FillTo80Light);

			for (auto c : callbacks)
			{
				b.addEmptyLine();
				if (c.id.getIdentifier().toString().startsWith("process"))
				{
					String s;
					s << "template <typename T> void " << c.id.getIdentifier() << "(T& data)";
					b << s;
				}
				else
					Function f(b, c);

				StatementBlock sb(b, false);

				b << "";

                if(c.id.getIdentifier() == Identifier("process"))
                {
					b << "static constexpr int NumChannels = getFixChannelAmount();";
                    b.addComment("Cast the dynamic channel data to a fixed channel amount", rawComment);
                    b << "auto& fixData = data.template as<ProcessData<NumChannels>>();";
                    b.addEmptyLine();
                    
                    b.addComment("Create a FrameProcessor object", rawComment);
                    b << "auto fd = fixData.toFrameData();";
                    b.addEmptyLine();
                    b << "while(fd.next())";
                    {
                        StatementBlock sb3(b, false);
                        b.addComment("Forward to frame processing", rawComment);
                        b << "processFrame(fd.toSpan());";
                    }
                }
                
				if (c.returnType.isValid())
				{
					VariableStorage v(c.returnType.getType(), var(0));
					String rt;
					rt << "return " << Types::Helpers::getCppValueString(v) << ";";
					b << rt;
				}

				b.addEmptyLine();
			}

            b.addComment("Parameter Functions", snex::cppgen::Base::CommentType::FillTo80Light);
            
			{
				b.addEmptyLine();
				String pf;
				pf << "template <int P> void setParameter(double v)";
				b << pf;
				StatementBlock sb(b, false);

				b << "if (P == 0)";

				{
					StatementBlock sb2(b, false);
					b.addComment("This will be executed for MyParameter (see below)", snex::cppgen::Base::CommentType::Raw);
					b << "jassertfalse;";
				}

				b.addEmptyLine();
			}

			{
				b.addEmptyLine();
				
				b << "void createParameters(ParameterDataList& data)";
				StatementBlock sb(b);

				{
					StatementBlock sb2(b);
					String l;
					b.addComment("Create a parameter like this", Base::CommentType::Raw);
					b << "parameter::data p(\"MyParameter\", { 0.0, 1.0 });";
					b.addComment("The template parameter (<0>) will be forwarded to setParameter<P>()", Base::CommentType::Raw);
					b << "registerCallback<0>(p);";
					b << "p.setDefaultValue(0.5);";
					b << "data.add(std::move(p));";
				}
			}
		}

		codeFile.replaceWithText(b.toString());

		codeFile.revealToUser();

        BackendDllManager::addNodePropertyToJSONFile(bpe->getMainController(), n, PropertyIds::IsPolyphonic);
	}
}

void BackendCommandTarget::Actions::restoreToDefault(BackendRootWindow * bpe)
{
	auto mp = JavascriptMidiProcessor::getFirstInterfaceScriptProcessor(bpe->getBackendProcessor());

	if (mp == nullptr)
		return;

	auto content = mp->getContent();

	String message;

	for (int i = 0; i < content->getNumComponents(); i++)
	{
		auto sc = content->getComponent(i);

		if (sc->getScriptObjectProperty(ScriptComponent::saveInPreset))
		{
			auto v = sc->getScriptObjectProperty(ScriptComponent::defaultValue);
			auto cv = sc->getValue().toString();
			
			message << sc->getName() << ": ";
			message << cv << " -> " << v.toString() << "\n";

			sc->resetValueToDefault();
		}
	}

	debugToConsole(mp, message);
}

void BackendCommandTarget::Actions::extractEmbeddedFilesFromSnippet(BackendRootWindow* bpe)
{
	auto gp = dynamic_cast<GlobalScriptCompileBroadcaster*>(bpe->getBackendProcessor());

	String m;

	m << "Do you want to copy the embedde script files into your current working project ? \n";

	for(int i = 0; i <gp->getNumExternalScriptFiles(); i++)
	{
		if(gp->getExternalScriptFile(i)->getResourceType() == ExternalScriptFile::ResourceType::EmbeddedInSnippet)
		{
			m << "- " << gp->getExternalScriptFile(i)->getFile().getFullPathName() << "\n";
		}
	}

	if(!PresetHandler::showYesNoWindow("Copy script resource files", m))
	{
		return;
	}

	

	int numWritten = 0;

	auto chain = bpe->getBackendProcessor()->getMainSynthChain();

	for(int i = 0; i < gp->getNumExternalScriptFiles(); i++)
	{
		if(gp->getExternalScriptFile(i)->extractEmbedded())
		{
			debugToConsole(chain, "Extracted " + gp->getExternalScriptFile(i)->getFile().getFullPathName());
			numWritten++;
		}
	}

	debugToConsole(chain, "Extracted " + String(numWritten) + " files from currently loaded HISE snippet");
	
}

void BackendCommandTarget::Actions::showExampleBrowser(BackendRootWindow* bpe)
{
	auto dm = bpe->getBackendProcessor()->deviceManager;
	auto cb = bpe->getBackendProcessor()->callback;

	auto bp = new BackendProcessor(dm, cb);

	bp->setIsSnippetBrowser();
	
	auto nw = dynamic_cast<BackendRootWindow*>(bp->createEditor());

	for(auto w: bpe->allWindowsAndBrowsers)
		nw->allWindowsAndBrowsers.addIfNotAlreadyThere(w);

	bpe->allWindowsAndBrowsers.addIfNotAlreadyThere(nw);

	nw->toggleSnippetBrowser();
	nw->setVisible(true);
	nw->centreWithSize(1600, 1000);

	

	int flags = 0;

	flags |= ComponentPeer::StyleFlags::windowAppearsOnTaskbar;
	flags |= ComponentPeer::StyleFlags::windowHasCloseButton;
	flags |= ComponentPeer::StyleFlags::windowHasDropShadow;
	flags |= ComponentPeer::StyleFlags::windowHasMaximiseButton;
	flags |= ComponentPeer::StyleFlags::windowHasTitleBar;
	flags |= ComponentPeer::StyleFlags::windowIsResizable;

	nw->setName("HISE Snippet browser");

	nw->addToDesktop(flags, nullptr);

	nw->setCurrentlyActiveProcessor();
	
}

namespace multipage
{

#define MULTIPAGE_BIND_CPP(className, methodName) state->bindCallback(#methodName, BIND_MEMBER_FUNCTION_1(className::methodName));

struct EncodedDialogBase: public Component,
						  public QuasiModalComponent
{
	void writeState(const Identifier& id, const var& value)
	{
		state->globalState.getDynamicObject()->setProperty(id, value);
	}

	var readState(const Identifier& id) const
	{
		return state->globalState[id];
	}

	virtual void bindCallbacks() = 0;

	void loadFrom(const String& d)
	{
		MemoryBlock mb;
		mb.fromBase64Encoding(d);
		MemoryInputStream mis(mb, false);
		MonolithData md(&mis);

		state = new State(var());
		addAndMakeVisible(dialog = md.create(*state));

		dialog->setFinishCallback([this]()
		{
			findParentComponentOfClass<ModalBaseWindow>()->clearModalComponent();
		});

		bindCallbacks();

		setSize(dialog->getWidth(), dialog->getHeight());

		dialog->showFirstPage();
	}
	
	void resized() override
	{
		dialog->setBounds(getLocalBounds());
	}

	void navigate(int pageIndex, bool shouldSubmit)
	{
		SafeAsyncCall::call<EncodedDialogBase>(*this, [pageIndex, shouldSubmit](EncodedDialogBase& db)
		{
			if(shouldSubmit)
			{
				db.state->currentPageIndex = pageIndex-1;
				db.dialog->navigate(true);
			}
			else
			{
				db.state->currentPageIndex = pageIndex;
				db.dialog->refreshCurrentPage();
			}
		});
	}

protected:

	ScopedPointer<State> state;
	ScopedPointer<Dialog> dialog;

	JUCE_DECLARE_WEAK_REFERENCEABLE(EncodedDialogBase);
};



struct ExportSetupWizard: public EncodedDialogBase
{
	ExportSetupWizard():
	  EncodedDialogBase()
	{
        auto c = "2628.jNB..vhB........nT6K8C1zpTQT.nXXTGQKv5LiafZwFcJQrY8PqhrrVxbrbSM5eCfUP5iXKrUmB9bggfSzoI7Ok9SS4+vhTD.FALQ.vIOImS+.Bio8mw8kWWc4RKfsh5QGaBseT+podVeUsUcODim9qBeUqz+m6BDVqgMIGQ9QBFMor7vVo8Onn+C4GYP9Qru3n7CIRfn7luaMqmy6XLjnjlSJh7kGafD0D0zCRGVosG0OdjgOKzUnVuwZ7X8E1tL5j7by07nrHvHrStus1m5yQUHsUR0F9zd5zNRiraCcMxYQFh0SqtBgGnXgBFIrzRX7wsVKgXPQoBlJTz9w8Ec7heKXhvBDXtTACkHv7i52hDXrDAFPRXrxkJQHwkL8ywffRkJQT4gKThXqjnbdz074hZ5xCakTKBzRhzBxfz1ZRUxVJ3E8LRj7mwPZqjdoagzt61PF1JI94Mcr6sj1sgLDUZOCqa8ygRiMrbPWobGLoE0usncD97WEYn70xI6MIbqRG9XRtrLucMhX6SxyXegk12Q6PUrgiTj+.Q4LllPPUQdru1Q901Z+BBC96W4tvJg5Ya2iaZj2NDqkwYWf7icmpvhyEq1O6suV+2s9qc0kx8cc2Yr01XTzVgZd9Q8Wg8ryiztUnouHTSCEkEsOv9BJpHroqdTyFElIaN5uURhZnGXnrUejv2imureoiIf3UksRj+Xs5jQTlrUS+gXq9rNNZJwVIgen7VBj9XBdjwDb.Ex3PiDTz4AQZ5JO3BN13QlNhd5bANxPwqyHJLRDNxL4U7PjlXEPKbL8llfh5nCGSgzxXgHMTpMDgCJhfBsGn9oHtH8wV0e3szXRdi1yMxa9uu9Iarlp8Aps0B7vURcCIDI29QxXcwmkz9iTbf4Dxggtioouv8AarVDf.M8iKh4+1Tt51f3MpjitRIGfBBJJS7fXQNahNDrV4IAjLFMFkyqDvxiIBQftiIhsDi.SvApx.GlLYxDPRV6cneNr4XSelWcJFbapFxAGppy+0L.HMs0pvt4zrE9pX6dnjaiBEJJqCebu0FYtJzi8sck63bkruKy3TqbTyZkwpYTEbYpaZXRlZswjIS5hy80wH+UHs09VSztwXAZQ6WoPKjTwFNVLsmaz.HMEV6MYbjH8vsKNKL5ObG4Fz9yGH0SHK9IjhU4PrCF4wdTJx2DpoyppMZvISFpcd9s.QhppSGNA.XjF.Yju5cj9CUSMskA3oc0iZUnDl0YmpNlFN3PHH8savwpNm71ROxODdr7X6nbs1XszCnWUxwQIi0YAQ8HPaLt9h6hG1.4VqMcl4+dweUi1KgvjSxMETk8LghLQ6uxaN7MFc.gAHI23htlXa0Q8GqovMb08DL8eh0lWzs30UVTk6YMGFh05q80OF70zoji6nmc5x2Z+IgOmQWFlG24iwzoIsPB46ZK69Zqr1Jx0nFc+WZGp.Mlz+84d1bTuo7V6QN72t1IY4GfLExV+Ijc18RnnMxR3Fs+kk8z6pbuUl8bfKtOUXhMp6+l1oo9asyIEZe3bpuGn1TXueseNJg94Xi.IBxEpROoDxTJEMin.....BgA.DNDYgoRJ95gz3QDRjAA4T.lAP.B..DI.B.PH.hf.fP.fZBPDMfvNz3jXSUX.sGzUlVdjNXCIpOzQaLAE+xHA5i7Zb+WfLOXSYFiyF6aZvQq2GG0WNITHZz5BQ1x3zOTbZIfJG6EEAe5c8gNyKSdqSnzDzEnrNAe3SZmX9IEHjQzeloprTQZeKSa4tn8WH3hX3dkUSgUF8fOUv48gLSSLJsYg6gLgDUCaLuHz6vXdlZOb2whFGFQwUgrtKwWOtX1mKRuvVvZTL68QajPRlVTRPOnbPSlQwYNVzA+U4JLNSl0OLRd9PF8WYLiqet1BGlMyJEbku3nFHRTUzuoP+Gr9iJcswKbSCihx28CjkV9g.TAaKgv4CtB2b0ma2WSYViNFhMmts1pD24mZvS5rooxN8lavvlv0k4t2ZmTsPnEICBRzB8JIBKrAZw4rUx22hqQ7LZoNYsR0KQz2NM+RQ7MlFD.t4JSkvjjRlRkFe.6E0cLhOdgM0BTixfqHlirxQaFjpmcBmaHOT9lfW9kGuoKj6AAUHWh0b1wTYIQwv9ZzwIHSCgKDiQx0peGw5YPV1VpEzXotK7QQqL3mG6aY9LIwLCjl0OVSYjBjO0wsPFWVEz+zMpnZPWoB+8ftfOgSkIh+97RUwPOP7KegCnvtjHVGP0jERayYEE+O3Fd0p2sRL9PGVF.BLRL3wFSS4oMGGbtrHoqpnbhjR02EQYJWR8RgJimKf4bh1PkqiqkEqbbbYUQMDTQ7uTPOY0fCrEgiRlc4VsLj0lrEc5ZPXvpd60mlN5Wxg1RhAZAVhXiKQuWMNwOsgtx1NFUW5kwfPm7+Dc0iJvLb8hIJhSeW8R44enUHQb56gWDkiMa6sykG+Wosmn6HfusyOSCCoQzDYmTPyYpDYXlBp7mFEg2u4E+2O2XiNDMZm1o2Wqro.LdPq9sYcn5ahpTK7TigEf9qLdIyrGFzcUvjcWB1ojkMaAdY8HWi5HFxbhijLHK0.5tOkAtwsmdVApTyC1M.yyb6YQqh.jDAjAr5T4fGa8mHBhsYaEGoVBIxhM9polIScAY4PDi+6QBWX4lJVzgyfeoCasrX6Zl6dUvbayN+ZpYFcOI7+Q0wvzT1ODUmZHKbP4p4NY2fM.l6cptRvCpTP29ivB0WXX5CUmqGKCZPKVJy3g1lQ3lRUqtO3jFthucj922hx.CglckY7N8NEPqgzkWuFSMVC8Su68H56Q55EppVLYqRJIrZ9AyfIrM+92we.yGifURFbz40z1+Sun.oGz1egHPQZ.FG9s9K15.31wfcH+twa6QeqaUptk7dn5jHs+t9+P21oZV2zdOm3xnNjYQNS.bxzWchXBLSJnfL6VESs7QdT04BhQcDWZYAQ9aL+G4h.wnvRy6BAwl7Vv6OSqdUmKRUFfCJHkfbpmn6qz5i.0S5rEF3wJPbvXMkfMcgJOelAp6nA+3zpbOxx4Ht40vwBbvNKfh1+2l6tKhkd.kvCam9UVV4yIfMqOPfcK4sjwFsPTRbdTM0lgsgbHVzuG3aSfbUTo1vI0EMMBuhoyNR78sFCB05F1KPJ.jPmI8b3NTB5Y0lUM3lXriEowtMF8Qhae42hhnP3avIC42mt6L53zWAOcl55yCaaFaa8cUfSA404OwDZxE99I3bSlwlB0hh8OVOcsvwu0G24ao83TuYleTPnRQCX+EQTFZfIowMnD.7aqmusgPDPxdV6lHEARNd0QBzXlX8.l7wjCfbf9hwd+xj0n36CmOXYmk5v+kmOMGXMs9nihGpSbp3+dAA5yn2GPiCRKJbPvL6w3J+Kbi+Jvlz7zH+IqKfTHXAoSBGqD9qwA.yqg5F92yzAIz3fVfbubNKc4UPdl1LNIGKLjBYrLdLQG3GbdTvDirSWGCzW7CtOMS6MT+az+78x28.kNB..X5H..vpi...";
        
		loadFrom(c);
	}

	void bindCallbacks() override
	{
		MULTIPAGE_BIND_CPP(ExportSetupWizard, prevDownload);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, skipIfDesired);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, checkIDE);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, checkHisePath);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, checkSDK);
		MULTIPAGE_BIND_CPP(ExportSetupWizard, onPost);
	}

	var checkHisePath(const var::NativeFunctionArgs& args)
	{
		auto exists = readState("HisePath").toString().isNotEmpty();

		writeState("hisePathExists", exists);
		writeState("hisePathExtract", !exists);
		writeState("hisePathDownload", !exists);
		writeState("hiseVersionMatches", true); // TODO: make a proper version check against the source code

		return var();
	}

	var checkIDE(const var::NativeFunctionArgs& args)
	{
#if JUCE_WINDOWS

		auto MSBuildPath = "C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe";
		writeState("msBuildExists", File(MSBuildPath).existsAsFile());

		if(readState("UseIPP"))
		{
			auto IppPath = "C:/Program Files (x86)/Intel/oneAPI/ipp/latest/include/ipp.h";
			writeState("ippExists", File(IppPath).existsAsFile());
		}
		else
		{
			writeState("ippExists", true);
		}
#elif JUCE_MAC
        {
            juce::ChildProcess xc;
            xc.start("xcodebuild --help");
            auto output = xc.readAllProcessOutput();
            auto xcodeExists = xc.getExitCode() == 0;
            writeState("xcodeExists", xcodeExists);
        }
        {
            juce::ChildProcess xcp;
            xcp.start("gem list");
            auto output = xcp.readAllProcessOutput();
            auto xcPrettyExists = output.contains("xcpretty");
            writeState("xcPrettyExists", xcPrettyExists);
        }
#endif

		return var();
	}
	
	var checkSDK(const var::NativeFunctionArgs& args)
	{
		auto toolsDir = File(readState("HisePath").toString()).getChildFile("tools");
		auto vst3sdk = toolsDir.getChildFile("SDK/VST3 SDK");

#if JUCE_WINDOWS
		auto projucer = toolsDir.getChildFile("projucer/Projucer.exe");

		auto ok = projucer.startAsProcess("--help");
#elif JUCE_MAC
        
        auto projucer = toolsDir.getChildFile("projucer/Projucer.app/Contents/MacOS/Projucer");
        
        jassert(projucer.existsAsFile());
        
        auto ok = projucer.startAsProcess("--help");
        
#else
		auto ok = true;
#endif

		writeState("projucerWorks", ok);
		writeState("sdkExists", vst3sdk.isDirectory());
		writeState("sdkExtract", !vst3sdk.isDirectory());

		return var();
	}

	var prevDownload(const var::NativeFunctionArgs& args)
	{
		auto id = args.arguments[0].toString();
		String url;

		url << "https://github.com/christophhart/HISE/archive/refs/tags/";
		url << GlobalSettingManager::getHiseVersion();
		url << ".zip";
		
		writeState("sourceURL", url);
		return var();
	}

	var skipIfDesired(const var::NativeFunctionArgs& args)
	{
		if(readState("skipEverything"))
			navigate(4, false);

		return var();
	}

	var onPost(const var::NativeFunctionArgs& args)
	{
		auto bp = findParentComponentOfClass<BackendRootWindow>()->getBackendProcessor();
		bp->getSettingsObject().loadSettingsFromFile(HiseSettings::SettingFiles::CompilerSettings);
		
		return var();
	}
};

}


void BackendCommandTarget::Actions::setupExportWizard(BackendRootWindow* bpe)
{
	auto np = new multipage::ExportSetupWizard();
	np->setModalBaseWindowComponent(bpe);
}

void BackendCommandTarget::Actions::exportProject(BackendRootWindow* bpe, int buildOption)
{
	auto exportIsReady = (bool)bpe->getBackendProcessor()->getSettingsObject().getSetting(HiseSettings::Compiler::ExportSetup);

	if(!exportIsReady)
	{
		if(PresetHandler::showYesNoWindow("System not configured", "Your system has not been setup for export. Do you want to launch the Export Setup wizard?"))
			Actions::setupExportWizard(bpe);

		return;
	}

	CompileExporter exporter(bpe->getMainSynthChain());

	switch((CompileExporter::BuildOption)buildOption)
	{
	case CompileExporter::BuildOption::AllPluginFormatsInstrument:
		exporter.exportMainSynthChainAsInstrument();
		break;
	case CompileExporter::BuildOption::AllPluginFormatsFX:
		exporter.exportMainSynthChainAsFX();
		break;
	case CompileExporter::BuildOption::AllPluginFormatsMidiFX:
		exporter.exportMainSynthChainAsMidiFx();
		break;
	case CompileExporter::BuildOption::StandaloneLinux:
		exporter.exportMainSynthChainAsStandaloneApp();
		break;
	}
}

#undef REPLACE_WILDCARD
#undef REPLACE_WILDCARD_WITH_STRING

#undef ADD_MENU_ITEM
#undef ADD_IOS_ONLY
#undef ADD_MENU_ITEM
#undef toggleVisibility

void XmlBackupFunctions::removeEditorStatesFromXml(XmlElement &xml)
{
	xml.deleteAllChildElementsWithTagName("EditorStates");

	for (int i = 0; i < xml.getNumChildElements(); i++)
	{
		removeEditorStatesFromXml(*xml.getChildElement(i));
	}
}

juce::XmlElement* XmlBackupFunctions::getFirstChildElementWithAttribute(XmlElement* parent, const String& attributeName, const String& value)
{
	if (parent->getStringAttribute(attributeName) == value)
		return parent;

	for (int i = 0; i < parent->getNumChildElements(); i++)
	{
		auto* e = parent->getChildElement(i);

		if (e->getStringAttribute(attributeName) == value)
			return e;

		auto c = getFirstChildElementWithAttribute(e, attributeName, value);

		if (c != nullptr)
			return c;
	}

	return nullptr;
}

void XmlBackupFunctions::addContentFromSubdirectory(XmlElement& xml, const File& fileToLoad)
{
	String subDirectoryId = fileToLoad.getFileNameWithoutExtension() + "UIData";

	auto folder = fileToLoad.getParentDirectory().getChildFile(subDirectoryId);

	Array<File> contentFiles;

	folder.findChildFiles(contentFiles, File::findFiles, false, "*.xml");

	if (auto uiData = getFirstChildElementWithAttribute(&xml, "Source", subDirectoryId))
	{
		for (const auto& f : contentFiles)
		{
			if (auto child = XmlDocument::parse(f))
			{
				uiData->addChildElement(child.release());
			}
		}

		uiData->removeAttribute("Source");
	}
}

void XmlBackupFunctions::extractContentData(XmlElement& xml, const String& interfaceId, const File& xmlFile)
{
	auto folder = xmlFile.getParentDirectory().getChildFile(xmlFile.getFileNameWithoutExtension() + "UIData");

	if (folder.isDirectory())
		folder.createDirectory();

	if (auto pXml = getFirstChildElementWithAttribute(&xml, "ID", interfaceId))
	{
		if (auto uiData = pXml->getChildByName("UIData"))
		{
			for (int i = 0; i < uiData->getNumChildElements(); i++)
			{
				auto e = uiData->getChildElement(i);
				auto name = e->getStringAttribute("DeviceType");

				auto file = folder.getChildFile(xmlFile.getFileNameWithoutExtension() + name + ".xml");

				file.create();
				file.replaceWithText(e->createDocument(""));
			}

			uiData->deleteAllChildElements();
			uiData->setAttribute("Source", folder.getRelativePathFrom(xmlFile.getParentDirectory()));
		}
	}
}

void XmlBackupFunctions::removeAllScripts(XmlElement &xml)
{
	const String t = xml.getStringAttribute("Script");

	if (!t.startsWith("{EXTERNAL_SCRIPT}"))
	{
		xml.removeAttribute("Script");
	}

	for (int i = 0; i < xml.getNumChildElements(); i++)
	{
		removeAllScripts(*xml.getChildElement(i));
	}
}

juce::File XmlBackupFunctions::getScriptDirectoryFor(ModulatorSynthChain *masterChain, const String &chainId /*= String()*/)
{
	if (chainId.isEmpty())
	{
		return GET_PROJECT_HANDLER(masterChain).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile("ScriptProcessors/" + getSanitiziedName(masterChain->getId()));
	}
	else
	{
		return GET_PROJECT_HANDLER(masterChain).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile("ScriptProcessors/" + getSanitiziedName(chainId));
	}
}

juce::File XmlBackupFunctions::getScriptFileFor(ModulatorSynthChain *, File& directory, const String &id)
{
	return directory.getChildFile(getSanitiziedName(id) + ".js");
}

String XmlBackupFunctions::getSanitiziedName(const String &id)
{
	return id.removeCharacters(" .()");
}

} // namespace hise
