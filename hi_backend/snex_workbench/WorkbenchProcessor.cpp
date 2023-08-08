/*  ===========================================================================
*
*   This file is part of HISE.
*   Copyright 2016 Christoph Hart
*
*   HISE is free software: you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option any later version.
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



namespace hise {
using namespace juce;


juce::PopupMenu SnexWorkbenchEditor::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
{
	PopupMenu m;

	switch (topLevelMenuIndex)
	{
	case MenuItems::FileMenu:
	{
		m.addCommandItem(&mainManager, MenuCommandIds::FileNew);
		m.addCommandItem(&mainManager, MenuCommandIds::FileOpen);

		PopupMenu allFiles;

		auto root = BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Networks);

		networkFiles = root.findChildFiles(File::findFiles, true, "*.xml");
		int i = 0;

		for (auto& f : networkFiles)
		{
			allFiles.addItem(MenuCommandIds::FileOffset + i++, f.getRelativePathFrom(root), true, activeFiles.contains(f));
		}

		m.addSubMenu("Open from Project", allFiles);

		m.addCommandItem(&mainManager, MenuCommandIds::FileSave);
		m.addCommandItem(&mainManager, MenuCommandIds::FileSaveAndReload);
		m.addCommandItem(&mainManager, MenuCommandIds::FileSaveAll);
		m.addCommandItem(&mainManager, MenuCommandIds::FileCloseCurrentNetwork);
		
		m.addSeparator();

		m.addCommandItem(&mainManager, MenuCommandIds::FileShowNetworkFolder);
		m.addCommandItem(&mainManager, MenuCommandIds::FileSetProject);

		PopupMenu recentProjects;
		auto current = getProjectHandler().getWorkDirectory();

		recentProjectFiles = ProjectHandler::getRecentWorkDirectories();

		i = 0;

		for (auto& p : recentProjectFiles)
			recentProjects.addItem(MenuCommandIds::ProjectOffset + i++, p, true, p == current.getFullPathName());

		m.addSubMenu("Recent projects", recentProjects);

		return m;
	}
	case MenuItems::ToolsMenu:
	{
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsSynthMode);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsEffectMode);
		m.addSeparator();
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsRecompile);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsShowKeyboard);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsAudioConfig);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsEditTestData);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsCompileNetworks);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsClearDlls);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsSetBPM);
	}
	case MenuItems::TestMenu:
	{
		m.addCommandItem(&mainManager, MenuCommandIds::TestsRunAll);
	}
	}

	return m;
}

void SnexWorkbenchEditor::getAllCommands(Array<CommandID>& commands)
{
	const CommandID id[] = {
		MenuCommandIds::FileNew,
		MenuCommandIds::FileOpen,
		MenuCommandIds::FileSave,
		MenuCommandIds::FileSaveAll,
		MenuCommandIds::FileSaveAndReload,
		MenuCommandIds::FileCloseCurrentNetwork,
		MenuCommandIds::FileSetProject,
		MenuCommandIds::FileShowNetworkFolder,
		MenuCommandIds::ToolsEffectMode,
		MenuCommandIds::ToolsSynthMode,
		MenuCommandIds::ToolsShowKeyboard,
		MenuCommandIds::ToolsAudioConfig,
		MenuCommandIds::ToolsCompileNetworks,
		MenuCommandIds::ToolsClearDlls,
		MenuCommandIds::ToolsSetBPM,
		MenuCommandIds::ToolsRecompile,
		MenuCommandIds::ToolsEditTestData,
		MenuCommandIds::TestsRunAll
	};

	commands.addArray(id, numElementsInArray(id));
}

void SnexWorkbenchEditor::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	switch (commandID)
	{
	case MenuCommandIds::FileNew: setCommandTarget(result, "New File", true, false, 'N', true, ModifierKeys::commandModifier); break;
	case MenuCommandIds::FileOpen: setCommandTarget(result, "Open File", true, false, 'O', true, ModifierKeys::commandModifier); break;
	case MenuCommandIds::FileSave: setCommandTarget(result, "Save Network", true, false, 'S', true, ModifierKeys::commandModifier); break;
	case MenuCommandIds::FileSaveAll: setCommandTarget(result, "Save Network & Testdata", true, false, 'x', false); break;
	case MenuCommandIds::FileCloseCurrentNetwork: setCommandTarget(result, "Close current File", true, false, 0, false); break;
	case MenuCommandIds::FileSetProject: setCommandTarget(result, "Load Project", true, false, 0, false); break;
	case MenuCommandIds::FileShowNetworkFolder: setCommandTarget(result, "Show Network folder", true, false, 'x', false); break;
	case MenuCommandIds::FileSaveAndReload: setCommandTarget(result, "Save and reload", true, false, 'x', false); break;
	case MenuCommandIds::ToolsEditTestData: setCommandTarget(result, "Manage Test Files", true, false, 0, false); break;
	case MenuCommandIds::ToolsCompileNetworks: setCommandTarget(result, "Compile DSP networks", true, false, 'x', false); break;
	case MenuCommandIds::ToolsSynthMode: setCommandTarget(result, "Polyphonic Synth Mode", true, synthMode, 'x', false); break;
	case MenuCommandIds::ToolsEffectMode: setCommandTarget(result, "Stereo Effect Mode", true, !synthMode, 'x', false); break;
	case MenuCommandIds::ToolsRecompile: setCommandTarget(result, "Recompile C++ preview", true, false, 'x', false);
		result.addDefaultKeypress(KeyPress::F5Key, ModifierKeys());
	  break;
	case MenuCommandIds::ToolsShowKeyboard: setCommandTarget(result, "Show Keyboard", true, bottomComoponent.isVisible(), 'x', false);
		break;
	case MenuCommandIds::ToolsClearDlls: setCommandTarget(result, "Clear DLL build folder", true, false, 'x', false); break;
	case MenuCommandIds::ToolsAudioConfig: setCommandTarget(result, "Settings", true, false, 0, false); break;
	case MenuCommandIds::ToolsSetBPM: setCommandTarget(result, "Set BPM", true, false, 0, false); break;
	case MenuCommandIds::TestsRunAll: setCommandTarget(result, "Run all tests", true, false, 0, false); break;
	
	default: jassertfalse;
	}
}

bool SnexWorkbenchEditor::perform(const InvocationInfo &info)
{
	switch (info.commandID)
	{
	case FileNew:
	{
		createNewFile();

		return true;
	};
	case FileShowNetworkFolder:
	{
		BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Networks).revealToUser();
		return true;
	}
	case FileOpen:
	{
		FileChooser fc("Open File", BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Networks), "*.xml", true);

		if (fc.browseForFileToOpen())
		{
			auto f = fc.getResult();
			addFile(f);
		}

		return true;
	}
	case FileCloseCurrentNetwork:
	{
		closeCurrentNetwork();
		return true;
	}
	case FileSaveAndReload:
	{
		auto f = getCurrentFile();
		closeCurrentNetwork();
		addFile(f);
		return true;
	}
	case FileSetProject:
	{
		FileChooser fc("Set Project", getProcessor()->getActiveFileHandler()->getRootFolder());

		if(fc.browseForDirectory())
		{
			auto f = fc.getResult();

			if(f.getChildFile("Images").isDirectory())
				getProcessor()->getSampleManager().getProjectHandler().setWorkingProject(f);
		}

		return true;
	}
	case FileSaveAll:
	{
		wb->getTestData().saveCurrentTestOutput();
		if (!wb->getTestData().saveToFile())
		{
			PresetHandler::showMessageWindow("Can't save test file", "You haven't specified a test file");
		}

		saveCurrentFile();
		return true;
	}
	case FileSave:
	{
		saveCurrentFile();

		return true;
	}
	
	case ToolsAudioConfig:
	{
		auto window = new SettingWindows(getProcessor()->getSettingsObject(), { HiseSettings::SettingFiles::SnexWorkbenchSettings, HiseSettings::SettingFiles::OtherSettings,  HiseSettings::SettingFiles::AudioSettings });
		window->setModalBaseWindowComponent(this);
		window->activateSearchBox();
		return true;
	}
	case ToolsEffectMode: setSynthMode(false); return true;
	case ToolsSynthMode: setSynthMode(true); return true;
	case ToolsShowKeyboard: bottomComoponent.setVisible(!bottomComoponent.isVisible()); resized(); return true;
	case ToolsCompileNetworks:
	{
		auto window = new DspNetworkCompileExporter(this, getProcessor());
		window->setOpaque(true);
		window->setModalBaseWindowComponent(this);

		return true;
	}
	case ToolsClearDlls:
	{
		if (dllManager->projectDll != nullptr)
		{
			PresetHandler::showMessageWindow("Dll already loaded", "You can't delete the build folder when the dll is already loaded");
			return true;
		}

		BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Binaries).deleteRecursively();

		return true;
	}
	case ToolsRecompile:
	{
		if (wb != nullptr)
			wb->triggerRecompile();
		return true;
	}
	case TestsRunAll:
	{
		auto window = new TestRunWindow(this);
		window->setOpaque(true);
		window->setModalBaseWindowComponent(this);
		return true;
	}
	}

	return false;
}

void SnexWorkbenchEditor::recompiled(snex::ui::WorkbenchData::Ptr p)
{
	if (auto dnp = dynamic_cast<DspNetworkCodeProvider*>(p->getCodeProvider()))
	{
		if (dnp->source == DspNetworkCodeProvider::SourceMode::DynamicLibrary)
		{
			dgp->getParentShell()->setOverlayComponent(new DspNetworkCodeProvider::OverlayComponent(dnp->source, p), 300);
		}
		else if (dnp->source == DspNetworkCodeProvider::SourceMode::InterpretedNode)
		{
			dgp->getParentShell()->setOverlayComponent(nullptr, 300);
		}
		else
		{
			ep->getParentShell()->setOverlayComponent(nullptr, 300);
		}
	}
}

using namespace snex::Types;


void SnexWorkbenchEditor::createNewFile()
{
	auto f = BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Networks).getNonexistentChildFile("DspNetwork", ".xml", false);

	auto fileName = PresetHandler::getCustomName(f.getFileNameWithoutExtension(), "Enter the name of the new dsp network.");

	fileName = snex::cppgen::StringHelpers::makeValidCppName(fileName);

	fileName = fileName.upToFirstOccurrenceOf(".", false, false);

	if (!fileName.isEmpty())
	{
		closeCurrentNetwork();

		auto f = BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Networks).getChildFile(fileName + ".xml");
		addFile(f);
	}
}

void SnexWorkbenchEditor::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
	if (menuItemID >= MenuCommandIds::ProjectOffset)
	{
		auto idx = menuItemID - (int)MenuCommandIds::ProjectOffset;

		File pd(recentProjectFiles[idx]);

		if(pd.isDirectory())
			getProcessor()->getSampleManager().getProjectHandler().setWorkingProject(pd);

		return;
	}
	if (menuItemID >= MenuCommandIds::FileOffset)
	{
		auto idx = menuItemID - (int)MenuCommandIds::FileOffset;
		addFile(networkFiles[idx]);
		return;
	}
}

//==============================================================================
SnexWorkbenchEditor::SnexWorkbenchEditor(BackendProcessor* bp) :
	//standaloneProcessor(),
	AudioProcessorEditor(bp),
	infoComponent(nullptr),
	bottomComoponent(getProcessor())
{
	dllManager = getProcessor()->dllManager;

#if 0
	UnitTestRunner runner;
	runner.setPassesAreLogged(true);
	runner.setAssertOnFailure(false);

	runner.runTestsInCategory("node_tests");
#endif

	addAndMakeVisible(infoComponent);

	addAndMakeVisible(bottomComoponent);

	rootTile = new FloatingTile(getProcessor(), nullptr);

	{
		raw::Builder b(getProcessor());

		auto mc = getProcessor();

		b.add(new WorkbenchSynthesiser(mc), mc->getMainSynthChain(), raw::IDs::Chains::Direct);
		b.add(new DspNetworkProcessor(mc, "internal"), mc->getMainSynthChain(), raw::IDs::Chains::FX);
	}
	

    auto useOpenGL = GET_HISE_SETTING(bp->getMainSynthChain(), HiseSettings::Other::UseOpenGL).toString() == "1";
    
    if(useOpenGL)
        context.attachTo(*this);

	mainManager.registerAllCommandsForTarget(this);
	mainManager.setFirstCommandTarget(this);

	mainManager.getKeyMappings()->resetToDefaultMappings();
	addKeyListener(mainManager.getKeyMappings());

	menuBar.setModel(this);
	menuBar.setLookAndFeel(&mlaf);

    addAndMakeVisible(menuBar);

#if JUCE_MAC
    MenuBarModel::setMacMainMenu(this);
    menuBar.setVisible(false);
#endif
    
	auto json = getLayoutFile().loadFileAsString();

	if (json.isNotEmpty())
	{
		rootTile->loadFromJSON(json);
	}
	else
	{
		FloatingInterfaceBuilder builder(rootTile.get());

		auto v = 0;

		builder.setNewContentType<VerticalTile>(v);

		//auto topBar = builder.addChild<VerticalTile>(r);

		//builder.addChild<TooltipPanel>(topBar);
		//auto htb = builder.addChild<VisibilityToggleBar>(topBar);
		
		//builder.setSizes(topBar, { 250, -1.0, 250 });
		//builder.setDynamic(topBar, false);
		//builder.setFoldable(topBar, false, { false, false, false });

		int vtb = builder.addChild<VisibilityToggleBar>(v);
		
		builder.getContent<VisibilityToggleBar>(vtb)->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF363636));
		
		builder.getContent<VerticalTile>(v)->setPanelColour(ResizableFloatingTileContainer::PanelColourId::itemColour1, Colour(0xFF1D1D1D));

		builder.getContent<VerticalTile>(v)->setPanelColour(ResizableFloatingTileContainer::PanelColourId::bgColour, Colour(0xFF1D1D1D));

		int editor = builder.addChild<SnexEditorPanel>(v);
		auto dg = builder.addChild<scriptnode::DspNetworkGraphPanel>(v);

		ep = builder.getContent<SnexEditorPanel>(editor);

		ep->setCustomTitle("SNEX Code Editor");
		
		builder.setVisibility(v, true, { true, false, true });
		
		dgp = builder.getContent<scriptnode::DspNetworkGraphPanel>(dg);
		dgp->setCustomTitle("Scriptnode DSP Graph");
		dgp->setForceHideSelector(true);
		
		builder.setFoldable(v, false, { false, false, false });

		builder.setSizes(v, {30.0, -0.5, -0.5});
		builder.setDynamic(v, false);
		builder.getContent<VisibilityToggleBar>(vtb)->refreshButtons();;

		builder.finalizeAndReturnRoot();
	}

	if (auto console = rootTile->findFirstChildWithType<ConsolePanel>())
	{
		console->getConsole()->setTokeniser(new snex::DebugHandler::Tokeniser());
	}

	addAndMakeVisible(rootTile);

	

	setSize(1680, 1050);

	setSynthMode(false);
}


SnexWorkbenchEditor::~SnexWorkbenchEditor()
{
#if JUCE_MAC
    MenuBarModel::setMacMainMenu(nullptr);
#endif
    
	//getLayoutFile().replaceWithText(rootTile->exportAsJSON());
    
    if(context.isAttached())
        context.detach();

	rootTile = nullptr;
}

void SnexWorkbenchEditor::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
}

void SnexWorkbenchEditor::resized()
{
	auto b = getLocalBounds();

	auto top = b.removeFromTop(45);

	if (bottomComoponent.isVisible())
	{
		auto bottom = b.removeFromBottom(72);
		bottomComoponent.setBounds(bottom);
	}

	infoComponent.setBounds(top);

#if !JUCE_MAC
	menuBar.setBounds(top.removeFromLeft(300).reduced(5, 5));
#endif

	rootTile->setBounds(b);
}

void SnexWorkbenchEditor::requestQuit()
{
	closeCurrentNetwork();
	//standaloneProcessor.requestQuit();
}

void SnexWorkbenchEditor::saveCurrentFile()
{
	if (autosaver != nullptr)
	{
		autosaver->closeAndDelete(true);

		auto fh = getNetworkHolderForNewFile(getProcessor(), synthMode);

        if(auto nw = fh->getActiveOrDebuggedNetwork())
        {
            autosaver = new DspNetworkListeners::PatchAutosaver(nw, BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Networks));
        }
        else
            jassertfalse;
	}
}

void SnexWorkbenchEditor::setSynthMode(bool shouldBeSynthMode)
{
	synthMode = shouldBeSynthMode;

	auto np = getNetworkHolderForNewFile(getProcessor(), synthMode);

	if (np == nullptr)
	{
		if(PresetHandler::showYesNoWindow("Load file", "Do you want to load a file"))
			createNewFile();
	}
	else
	{
		dgp->setContentWithUndo(dynamic_cast<Processor*>(np), 0);
	}


	resized();
}

bool SnexWorkbenchEditor::getSynthMode() const
{
	return synthMode;
}

scriptnode::DspNetwork::Holder* SnexWorkbenchEditor::getNetworkHolderForNewFile(MainController* mc, bool getSynthHolder)
{
	if (getSynthHolder)
		return ProcessorHelpers::getFirstProcessorWithType<WorkbenchSynthesiser>(mc->getMainSynthChain());
	else
		return ProcessorHelpers::getFirstProcessorWithType<DspNetworkProcessor>(mc->getMainSynthChain());
}

void SnexWorkbenchEditor::closeCurrentNetwork()
{
	if (autosaver != nullptr && autosaver->isChanged() && PresetHandler::showYesNoWindow("Save changes", "Do you want to save the changes before closing"))
		autosaver->closeAndDelete(true);

	autosaver = nullptr;

	if(dgp != nullptr)
		dgp->setContentWithUndo(nullptr, 0);

	if (auto n = getNetworkHolderForNewFile(getProcessor(), synthMode))
	{
		n->clearAllNetworks();
	}

	auto wm = static_cast<snex::ui::WorkbenchManager*>(getProcessor()->getWorkbenchManager());
	wm->setCurrentWorkbench(nullptr, true);

	resized();
}

bool SnexWorkbenchEditor::isCppPreviewEnabled()
{
	auto f = getCurrentSnexProcessor();
	jassert(f != nullptr);

	return f->enableCppPreview;
}

hise::DebuggableSnexProcessor* SnexWorkbenchEditor::getCurrentSnexProcessor()
{
	return dynamic_cast<DebuggableSnexProcessor*>(getNetworkHolderForNewFile(getProcessor(), synthMode));
}

void SnexWorkbenchEditor::addFile(File f)
{
	if (f.getFileName().startsWith("autosave"))
	{
		if (PresetHandler::showYesNoWindow("Replace original file", "Do you want to replace the real file with this autosaved version"))
		{
			auto r = f.getFileNameWithoutExtension().replace("autosave_", "");

			auto original = f.getSiblingFile(r).withFileExtension("xml");

			if (original.existsAsFile())
				original.deleteFile();

			f.moveFileTo(original);
			f = original;

			PresetHandler::showMessageWindow("File renamed", "The file was renamed to " + original.getFileName());
		}
	}

	closeCurrentNetwork();

	if (auto xml = XmlDocument::parse(f))
	{
		auto signalPath = xml->getChildElement(0);

		auto nodeId = signalPath->getStringAttribute(scriptnode::PropertyIds::ID.toString(), "");
		auto fileId = f.getFileNameWithoutExtension();

		if (nodeId != fileId)
		{
			if (PresetHandler::showYesNoWindow("Node ID mismatch", "The file name does not match the node ID. Press OK to replace the node ID with the filename"))
			{
				signalPath->setAttribute(scriptnode::PropertyIds::ID.toString(), fileId);

				f.replaceWithText(xml->createDocument(""));
			}
			else
			{
				return;
			}
		}
	}

	currentFiles[synthMode ? 1 : 0] = f;

	getProcessor()->getKillStateHandler().killVoicesAndCall(getProcessor()->getMainSynthChain(), [this, f](Processor* p)
	{
		loadNewNetwork(f);
		return SafeFunctionCall::OK;
	}, MainController::KillStateHandler::SampleLoadingThread);
}


void SnexWorkbenchEditor::loadNewNetwork(const File& f)
{
	jassert(LockHelpers::freeToGo(getProcessor()));

	if (dllManager == nullptr)
		dllManager = new BackendDllManager(getProcessor());

	dllManager->loadDll(false);

	auto fh = getNetworkHolderForNewFile(getProcessor(), synthMode);

	if (fh != nullptr)
		fh->setProjectDll(dllManager->projectDll);

	df = new hise::DspNetworkCodeProvider(nullptr, getMainController(), fh, f);

	wb = dynamic_cast<BackendProcessor*>(getMainController())->workbenches.getWorkbenchDataForCodeProvider(df, true).get();
	wb->getTestData().setMultichannelDataProvider(new PooledAudioFileDataProvider(getMainController()));
	wb->setCompileHandler(new hise::DspNetworkCompileHandler(wb, getMainController(), fh));

	wb->setIsCppPreview(true);

	

	auto testRoot = BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Tests);

	wb->getTestData().setTestRootDirectory(BackendDllManager::createIfNotDirectory(testRoot.getChildFile(f.getFileNameWithoutExtension())));
	wb->getTestData().setUpdater(getProcessor()->getGlobalUIUpdater());
	wb->addListener(this);
	fh->setExternalDataHolderToUse(&wb->getTestData());
	df->initNetwork();

	MessageManager::callAsync([&]()
	{
		auto h = getNetworkHolderForNewFile(getProcessor(), synthMode);

		ep->setWorkbenchData(wb.get());
		dgp->setContentWithUndo(dynamic_cast<Processor*>(h), 0);

		autosaver = new DspNetworkListeners::PatchAutosaver(h->getActiveOrDebuggedNetwork(), BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Networks));
	});
}

struct IncludeSorter
{
	static int compareElements(const File& f1, const File& f2)
	{
		using namespace scriptnode;
		using namespace snex::cppgen;

		auto xml1 = XmlDocument::parse(f1);
		auto xml2 = XmlDocument::parse(f2);

		if (xml1 != nullptr && xml2 != nullptr)
		{
			ValueTree v1 = ValueTree::fromXml(*xml1);
			ValueTree v2 = ValueTree::fromXml(*xml2);

			auto id1 = "project." + v1.getProperty(PropertyIds::ID).toString();
			auto id2 = "project." + v2.getProperty(PropertyIds::ID).toString();

			auto f1 = [id1](ValueTree& v)
			{
				if (v.getProperty(PropertyIds::FactoryPath).toString() == id1)
					return true;

				return false;
			};

			auto f2 = [id2](ValueTree& v)
			{
				if (v.getProperty(PropertyIds::FactoryPath).toString() == id2)
					return true;

				return false;
			};

			auto firstIsReferencedInSecond = valuetree::Helpers::forEach(v1, f2, valuetree::Helpers::IterationType::ChildrenFirst);

			auto secondIsReferencedInFirst = valuetree::Helpers::forEach(v1, f2, valuetree::Helpers::IterationType::ChildrenFirst);

			String e;
			e << "Cyclic reference: ";
			e << id1 << " && " << id2;

			if (firstIsReferencedInSecond)
			{
				if (secondIsReferencedInFirst)
					throw String(e);

				return -1;
			}

			if (secondIsReferencedInFirst)
			{
				if (firstIsReferencedInSecond)
					throw String(e);

				return 1;
			}
		}

		return 0;
	};
};

DspNetworkCompileExporter::DspNetworkCompileExporter(Component* e, BackendProcessor* bp) :
	DialogWindowWithBackgroundThread("Compile DSP networks"),
	ControlledObject(bp),
	CompileExporter(bp->getMainSynthChain()),
	editor(e)
{
	addComboBox("build", { "Debug", "CI", "Release" }, "Build Configuration");

#if !JUCE_DEBUG
    getComboBoxComponent("build")->setText("Release", dontSendNotification);
#endif
    
	if (getNetwork() == nullptr)
	{
		if (PresetHandler::showYesNoWindow("No DSP Network detected", "You need an active DspNetwork for the compilation process.  \n> Press OK to create a Script FX with an empty embedded Network"))
		{
			raw::Builder builder(bp);
			MainController::ScopedBadBabysitter sb(bp);

			auto jmp = builder.create<JavascriptMasterEffect>(bp->getMainSynthChain(), raw::IDs::Chains::FX);
			jmp->getOrCreate("internal_dsp");
		}
	}

	if (auto n = getNetwork())
		n->createAllNodesOnce();

	auto customProperties = bp->dllManager->getSubFolder(getMainController(), BackendDllManager::FolderSubType::ThirdParty).getChildFile("node_properties.json");

	if (customProperties.existsAsFile())
	{
		auto obj = JSON::parse(customProperties);
		
		if (auto o = obj.getDynamicObject())
		{
			for (auto& nv : o->getProperties())
			{
				if (auto ar = nv.value.getArray())
				{
					for (const auto& prop : *ar)
					{
						cppgen::CustomNodeProperties::addNodeIdManually(nv.name, prop.toString());
					}
				}
			}
		}
	}

	addBasicComponents(true);

	String s;
	s << "Nodes to compile:\n";

	for (auto f : bp->dllManager->getThirdPartyFiles(bp, false))
		s << " - " << f.getFileNameWithoutExtension() << " [external C++]\n";

	for (auto f : bp->dllManager->getNetworkFiles(bp, false))
		s << " - " << f.getFileNameWithoutExtension() << "\n";

	s = s.upToLastOccurrenceOf(", ", false, false);

	addTextBlock(s);

	showStatusMessage("Press OK to compile the nodes into a DLL");
}

void DspNetworkCompileExporter::writeDebugFileAndShowSolution()
{
    auto& settings = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();
    auto hisePath = settings.getSetting(HiseSettings::Compiler::HisePath).toString();
    auto solutionFolder = BackendDllManager::getSubFolder(getMainController(), BackendDllManager::FolderSubType::Binaries).getChildFile("Builds");
    auto projectName = settings.getSetting(HiseSettings::Project::Name).toString();
    auto debugExecutable = File(hisePath).getChildFile("projects/standalone/Builds/");
    
	

	
	
	auto currentExecutable = File::getSpecialLocation(File::currentExecutableFile);


#if JUCE_WINDOWS
    auto isUsingVs2017 = HelperClasses::isUsingVisualStudio2017(settings);
    auto vsString = isUsingVs2017 ? "VisualStudio2017" : "VisualStudio2022";
    auto vsVersion = isUsingVs2017 ? "15.0" : "17.0";
    
    debugExecutable = debugExecutable.getChildFile(vsString).getChildFile("x64/Debug/App/HISE Debug.exe");

	// If this hits, then you have a mismatch between VS2022 and VS2017...
	jassertEqual(debugExecutable, currentExecutable);
	
    solutionFolder = solutionFolder.getChildFile(vsString);
    auto solutionFile = solutionFolder.getChildFile(projectName).withFileExtension("sln");
    
	ScopedPointer<XmlElement> xml = new XmlElement("Project");
	xml->setAttribute("ToolsVersion", vsVersion);
	xml->setAttribute("xmlns", "http://schemas.microsoft.com/developer/msbuild/2003");
	auto pg = new XmlElement("PropertyGroup");
	pg->setAttribute("Condition", "'$(Configuration)|$(Platform)'=='Debug|x64'");
	xml->addChildElement(pg);

	auto ldc = new XmlElement("LocalDebuggerCommand");

	jassert(debugExecutable.existsAsFile());

	ldc->addTextElement(debugExecutable.getFullPathName());
	
	pg->addChildElement(ldc);
	auto df = new XmlElement("DebuggerFlavor");
	df->addTextElement("WindowsLocalDebugger");
	pg->addChildElement(df);

	auto userFile = solutionFile.getSiblingFile(projectName + "_DynamicLibrary.vcxproj.user");

	auto fileContent = xml->createDocument("");

	userFile.replaceWithText(fileContent);
    
	auto hasThirdPartyFiles = includedThirdPartyFiles.isEmpty();

    if (hasThirdPartyFiles && PresetHandler::showYesNoWindow("Quit HISE", "Do you want to quit HISE and show VS solution for debugging the DLL?  \n> Double click on the solution file, then run the VS debugger and it will open HISE with the ability to set VS breakpoints in your C++ nodes"))
    {
        solutionFile.revealToUser();
        JUCEApplication::quit();
    }
    
#elif JUCE_MAC
    debugExecutable = debugExecutable.getChildFile("MacOSX/build/Debug/HISE Debug.app");
    
    jassert(debugExecutable.isDirectory());
    solutionFolder = solutionFolder.getChildFile("MacOSX");
    auto solutionFile = solutionFolder.getChildFile(projectName).withFileExtension("xcodeproj");
    
    if (PresetHandler::showYesNoWindow("Show XCode Project", "Do you want to show the Xcode Project file?  \n> Double click on the file to open XCode, then choose `Debug->Attach to Process->HISE Debug` in order to run your C++ node in the Xcode Debugger"))
    {
        solutionFile.revealToUser();
    }
#endif
    
	
}

hise::DspNetworkCompileExporter::CppFileLocationType DspNetworkCompileExporter::getLocationType(const File& f) const
{
	if (f.getParentDirectory().getFileNameWithoutExtension() == "src")
		return ThirdPartySourceFile;

	if (f.getFileNameWithoutExtension() == "embedded_audiodata")
		return EmbeddedDataFile;

	for (auto& incF : includedFiles)
	{
		if (incF.getFileNameWithoutExtension() == f.getFileNameWithoutExtension())
			return CompiledNetworkFile;
	}

	for (auto& itf : includedThirdPartyFiles)
	{
		if (itf.getFileNameWithoutExtension() == f.getFileNameWithoutExtension())
			return ThirdPartyFile;
	}

	return UnknownFileType;
}

scriptnode::DspNetwork* DspNetworkCompileExporter::getNetwork()
{
	if (auto ed = getEditorWorkbench())
	{
		if (auto fh = ed->getNetworkHolderForNewFile(ed->getProcessor(), ed->getSynthMode()))
		{
			if (auto n = fh->getActiveOrDebuggedNetwork())
				return n;
		}
	}

	Processor::Iterator<JavascriptProcessor> iter(getMainController()->getMainSynthChain());

	while (auto jsp = iter.getNextProcessor())
	{
		if (auto n = jsp->getActiveOrDebuggedNetwork())
			return n;
	}
    
    return nullptr;
}



void DspNetworkCompileExporter::run()
{
	auto n = getNetwork();

	if (n == nullptr)
	{
		ok = (ErrorCodes)(int)DspNetworkErrorCodes::NoNetwork;
		errorMessage << "You need at least one active network for the export process.  \n";
		errorMessage << "> This is used to create all nodes once to setup the codegen properties";
		return;
	}

	if (!cppgen::CustomNodeProperties::isInitialised())
	{
		ok = ErrorCodes::CompileError;
		errorMessage << "the node properties are not initialised. Load a DspNetwork at least once";
		return;
	}

	getSourceDirectory(false).deleteRecursively();
	getSourceDirectory(false).createDirectory();
	getSourceDirectory(true).deleteRecursively();
	getSourceDirectory(true).createDirectory();

	

	showStatusMessage("Unload DLL");

	if (auto ed = getEditorWorkbench())
	{
		ed->saveCurrentFile();
	}

	getDllManager()->unloadDll();

	showStatusMessage("Create files");

	auto buildFolder = getFolder(BackendDllManager::FolderSubType::Binaries);

	auto networkRoot = getFolder(BackendDllManager::FolderSubType::Networks);
	auto unsortedList = BackendDllManager::getNetworkFiles(getMainController(), false);

	auto unsortedListU = BackendDllManager::getNetworkFiles(getMainController(), true);

	

	for (auto s : unsortedList)
		unsortedListU.removeAllInstancesOf(s);

	showStatusMessage("Sorting include dependencies");

	Array<File> list, ulist;

	for (auto& nf : unsortedList)
	{
		if (nf.getFileName().startsWith("autosave"))
			continue;

		for (auto& sf : getIncludedNetworkFiles(nf))
		{
			if (!BackendDllManager::allowCompilation(sf))
			{
				ok = ErrorCodes::CompileError;
				errorMessage << "Error at compiling `" << nf.getFileNameWithoutExtension() << "`:\n> `" << sf.getFileNameWithoutExtension() << "` can't be included because it's not flagged for compilation\n";
				
				errorMessage << "Enable the `AllowCompilation` flag for " << sf.getFileNameWithoutExtension() << " or remove the node from " << nf.getFileNameWithoutExtension();

				return;
			}
				

			list.addIfNotAlreadyThere(sf);
		}
	}

	for (auto& nf : unsortedListU)
	{
		if (nf.getFileName().startsWith("autosave"))
			continue;

		for (auto& sf : getIncludedNetworkFiles(nf))
		{
			ulist.addIfNotAlreadyThere(sf);
		}
	}

	jassert(list.size() == unsortedList.size());

	auto sourceDir = getFolder(BackendDllManager::FolderSubType::ProjucerSourceFolder);

	

	using namespace snex::cppgen;

	ValueTreeBuilder::SampleList externalSamples;

	// set with all files to generate for all networks
	std::set<String> faustClassIds;

	for (auto e : list)
	{
		if (auto xml = XmlDocument::parse(e))
		{
			auto p = ValueTree::fromXml(*xml);
			auto v = p.getChild(0);

			auto id = v[scriptnode::PropertyIds::ID].toString();

			auto cr = ValueTreeBuilder::cleanValueTreeIds(v);

            if(!cr.wasOk())
            {
                errorMessage = "";
                errorMessage << id << ": " << cr.getErrorMessage();
                ok = ErrorCodes::ProjectXmlInvalid;
                return;
            }
            
			if (id.compareIgnoreCase(e.getFileNameWithoutExtension()) != 0)
			{
				errorMessage << "Error at exporting `" << e.getFileName() << "`: Name mismatch between DSP network file and Root container.  \n>";
				errorMessage << "You need to either rename the file to `" << id;
				errorMessage << ".xml` or edit the XML data and set the root node's ID to `" << e.getFileNameWithoutExtension() << "`.";
				ok = ErrorCodes::ProjectXmlInvalid;
				return;
			}

			if(cppgen::StringHelpers::makeValidCppName(id).compareIgnoreCase(id) != 0)
			{
				errorMessage << "Illegal ID: `" << id << "`  \n> The network ID must be a valid C++ identifier";
				ok = ErrorCodes::ProjectXmlInvalid;
				return;
			}
				
            showStatusMessage("Creating C++ file for Network " + id);
            
			ValueTreeBuilder b(v, ValueTreeBuilder::Format::CppDynamicLibrary);

			b.setCodeProvider(new BackendDllManager::FileCodeProvider(getMainController()));
			b.addAudioFileProvider(new PooledAudioFileDataProvider(getMainController()));

			auto f = sourceDir.getChildFile(id).withFileExtension(".h");

			auto r = b.createCppCode();

			faustClassIds.insert(r.faustClassIds->begin(), r.faustClassIds->end());

			externalSamples.addArray(b.getExternalSampleList());
			

			if (r.r.wasOk())
				f.replaceWithText(r.code);
			else
            {
                ok = ErrorCodes::ProjectXmlInvalid;

				errorMessage = "";
				errorMessage << f.getFileNameWithoutExtension() << ": " << r.r.getErrorMessage();

                
                return;
            };

			includedFiles.add(f);
		}
	}

#if HISE_INCLUDE_FAUST_JIT
	DBG("sourceDir: " + sourceDir.getFullPathName());
	auto codeDestDir = getFolder(BackendDllManager::FolderSubType::ThirdParty).getChildFile("src");
	auto codeDestDirPath = codeDestDir.getFullPathName().toStdString();
	if (!codeDestDir.isDirectory())
		codeDestDir.createDirectory();
	DBG("codeDestDirPath: " + codeDestDirPath);

	auto boilerplateDestDirPath = codeDestDir.getParentDirectory().getFullPathName().toStdString();
	DBG("boilerplateDestDirPath: " + boilerplateDestDirPath);
	// we either need to hard code this path and keep it consistent with faust_jit_node or hi_backend will have to depend on hi_faust_jit
	auto codeLibDir = getFolder(BackendDllManager::FolderSubType::CodeLibrary).getChildFile("faust");
	auto codeLibDirPath = codeLibDir.getFullPathName().toStdString();
	DBG("codeLibDirPath: " + codeLibDirPath);

	// create all necessary files before thirdPartyFiles
	for (const auto& classId : faustClassIds)
	{
		auto _classId = classId.toStdString();
		DBG("Found Faust classId: " + classId);
		auto faustSourcePath = codeLibDir.getChildFile(classId + ".dsp").getFullPathName().toStdString();

		auto boilerplate_path = scriptnode::faust::faust_jit_helpers::genStaticInstanceBoilerplate(boilerplateDestDirPath, _classId);
		if (boilerplate_path.size() > 0)
			DBG("Wrote boilerplate file to " + boilerplate_path);
		else
			DBG("Writing generated boilerplate failed.");

		std::vector<std::string> faustLibraryPaths = {codeLibDirPath};
		// lookup FaustPath from settings
		auto& settings = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();
        
        auto faustPath = settings.getFaustPath();
        
		if (faustPath.isDirectory()) {
			auto globalFaustLibraryPath = faustPath.getChildFile("share").getChildFile("faust");
            
			if (globalFaustLibraryPath.isDirectory()) {
				faustLibraryPaths.push_back(globalFaustLibraryPath.getFullPathName().toStdString());
			}
		}

		auto code_path = scriptnode::faust::faust_jit_helpers::genStaticInstanceCode(_classId, faustSourcePath, faustLibraryPaths, codeDestDirPath);
		if (code_path.size() > 0)
			DBG("Wrote code file to " + code_path);
		else
			DBG("Writing generated code failed.");
	}

#endif // HISE_INCLUDE_FAUST_JIT

	auto thirdPartyFiles = BackendDllManager::getThirdPartyFiles(getMainController(), false);

	if (!thirdPartyFiles.isEmpty())
	{
		showStatusMessage("Copying third party files");

		for (auto tpf : thirdPartyFiles)
		{
			includedThirdPartyFiles.insert(0, tpf);
		}
	}

	if (!externalSamples.isEmpty())
	{
        showStatusMessage("Writing embedded audio data file");
        
		auto eadFile = getSourceDirectory(true).getChildFile("embedded_audiodata.h");
		eadFile.deleteFile();

		FileOutputStream fos(eadFile);

		fos << "// Embedded audiodata" << "\n";

        fos << "#pragma once\n\n";
        
		fos << "namespace audiodata {\n";

		for (const auto& es : externalSamples)
		{
			auto& bf = es.data->buffer;

			for (int i = 0; i < bf.getNumChannels(); i++)
			{
				fos << "static const uint32 " << es.className << i << "[] = { \n";
				cppgen::IntegerArray<uint32, float>::writeToStream(fos, reinterpret_cast<uint32*>(bf.getWritePointer(i)), bf.getNumSamples());
				fos << "};\n";
			}
		}

		fos << "}\n";
		fos.flush();

		eadFile.copyFileTo(getSourceDirectory(false).getChildFile("embedded_audiodata.h"));
	}

	for (auto u : unsortedListU)
	{
		if (auto xml = XmlDocument::parse(u))
		{
			auto v = ValueTree::fromXml(*xml);

			zstd::ZDefaultCompressor comp;
			MemoryBlock mb;

			comp.compress(v, mb);

			auto id = v[scriptnode::PropertyIds::ID].toString() + "_networkdata";
			auto f = sourceDir.getChildFile(id).withFileExtension(".h");

			Base c(snex::cppgen::Base::OutputType::AddTabs);

			Namespace n(c, "project", false);

			DefinitionBase b(c, "scriptnode::dll::InterpretedNetworkData");

			Array<DefinitionBase*> baseClasses;
			baseClasses.add(&b);

			Struct s(c, id, baseClasses, {});

			c << "String getId() const override";
			{
				StatementBlock sb(c);
				String def;
				def << "return \"" << v[scriptnode::PropertyIds::ID].toString() << "\";";
				c << def;
			}

			c << "bool isModNode() const override";
			{
				StatementBlock sb(c);
				String def;
				def << "return ";
				auto hasMod = cppgen::ValueTreeIterator::hasChildNodeWithProperty(v, PropertyIds::IsPublicMod);
				def << (hasMod ? "true" : "false") << ";";
				c << def;
			}

			c << "String getNetworkData() const override";
			{
				StatementBlock sb(c);
				String def;
				def << "return \"" << mb.toBase64Encoding() << "\";";
				c << def;
			}

			s.flushIfNot();
			n.flushIfNot();
			
			f.replaceWithText(c.toString());

			includedFiles.add(f);
		}
	}
	
	hisePath = File(GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Compiler::HisePath));

	createProjucerFile();

	for (auto l : getSourceDirectory(true).findChildFiles(File::findFiles, true, "*.h"))
	{
		auto target = getSourceDirectory(false).getChildFile(l.getFileName());

		if (isInterpretedDataFile(target))
			l.moveFileTo(target);
		else
			l.copyFileTo(target);
	}

	createIncludeFile(getSourceDirectory(true));
	createIncludeFile(getSourceDirectory(false));

	
	createMainCppFile(false);

	for (int i = 0; i < includedFiles.size(); i++)
	{
		if (isInterpretedDataFile(includedFiles[i]))
			includedFiles.remove(i--);
	}

	createMainCppFile(true);

    silentMode = true;
    
#if JUCE_WINDOWS
	BuildOption o = CompileExporter::VSTiWindowsx64;
#elif JUCE_MAC
	BuildOption o = CompileExporter::VSTmacOS;
#else
	BuildOption o = CompileExporter::VSTLinux;
#endif

	showStatusMessage("Compiling dll plugin");

	configurationName = getComboBoxComponent("build")->getText();
	
#if JUCE_LINUX
	ok = ErrorCodes::OK;
#else
	ok = compileSolution(o, CompileExporter::TargetTypes::numTargetTypes);
#endif
}

void DspNetworkCompileExporter::threadFinished()
{
#if JUCE_LINUX
	ok = compileSolution(CompileExporter::VSTLinux, CompileExporter::TargetTypes::numTargetTypes);
#endif

	if (ok == ErrorCodes::OK)
	{
		if (isUsingCIMode()) {
			return;
		}

#if JUCE_DEBUG
		writeDebugFileAndShowSolution();
#endif

		globalCommandLineExport = false;

		if (auto ed = getEditorWorkbench())
		{
#if JUCE_LINUX
			PresetHandler::showMessageWindow("Project creation OK", "Please run the makefile, then press OK when the compilation is finished to reload the dynamic library.");
#else
PresetHandler::showMessageWindow("Compilation OK", "Press OK to reload the project dll.");
#endif

			auto loadedOk = ed->dllManager->loadDll(true);

			if (loadedOk)
			{
				scriptnode::dll::DynamicLibraryHostFactory f(ed->dllManager->projectDll);

				StringArray list;

				for (int i = 0; i < f.getNumNodes(); i++)
				{
					list.add(f.getId(i));
				}

				String c;

				c << "Included DSP networks:\n  > `";
				c << list.joinIntoString(", ") << "`";

				PresetHandler::showMessageWindow("Loaded DLL " + ed->dllManager->getBestProjectDll(BackendDllManager::DllType::Current).getFileName(), c);
				return;
			}
		}
		else
		{

#if JUCE_LINUX
		PresetHandler::showMessageWindow("Project creation OK", "Please run the makefile, then restart HISE when the compilation is finished");
#else
		PresetHandler::showMessageWindow("Compilation OK", "Please restart HISE in order to load the new binary");
#endif
		}
	}
	else 
		PresetHandler::showMessageWindow("Compilation Error", errorMessage, PresetHandler::IconType::Error);
}

juce::File DspNetworkCompileExporter::getBuildFolder() const
{
	return getFolder(BackendDllManager::FolderSubType::Binaries);
}



juce::Array<juce::File> DspNetworkCompileExporter::getIncludedNetworkFiles(const File& networkFile)
{
	using namespace scriptnode;
	using namespace snex::cppgen;

	Array<File> list;

	if (auto xml = XmlDocument::parse(networkFile))
	{
		ValueTree v = ValueTree::fromXml(*xml);

		auto f2 = [&list, networkFile](ValueTree& v)
		{
			auto p = v.getProperty(PropertyIds::FactoryPath).toString();

			if (p.startsWith("project."))
			{
				auto pId = p.fromFirstOccurrenceOf("project.", false, false);
				list.add(networkFile.getSiblingFile(pId).withFileExtension("xml"));
			}

			return false;
		};
	}

	list.add(networkFile);

	return list;
}

hise::BackendDllManager* DspNetworkCompileExporter::getDllManager()
{
	if (auto ed = getEditorWorkbench())
		return ed->dllManager.get();
	else
		return dynamic_cast<BackendProcessor*>(getMainController())->dllManager.get();
}

bool DspNetworkCompileExporter::isInterpretedDataFile(const File& f)
{
	return f.getFileNameWithoutExtension().endsWith("_networkdata");
}

void DspNetworkCompileExporter::createIncludeFile(const File& sourceDir)
{
	File includeFile = sourceDir.getChildFile("includes.h");

	cppgen::Base i(cppgen::Base::OutputType::AddTabs);

	i.setHeader([]() { return "/* Autogenerated include file. */"; });

	i << "#if (defined (_WIN32) || defined (_WIN64))";
	i << "#pragma warning( push )";
	i << "#pragma warning( disable : 4189 4373)";  // unused variables, wrong override (from faust classes)
	i << "#else";
    i << "#pragma clang diagnostic push";
    i << "#pragma clang diagnostic ignored \"-Wunused-variable\"";
	i << "#endif";

    i.addEmptyLine();
    

    auto fileList = sourceDir.findChildFiles(File::findFiles, false, "*.h");

	auto thirdPartyFiles = getFolder(BackendDllManager::FolderSubType::ThirdParty).findChildFiles(File::findFiles, false, "*.h");

	fileList.addArray(thirdPartyFiles);

	for (auto& f : fileList)
	{
		if (getLocationType(f) == EmbeddedDataFile)
		{
			i.addComment("Include embedded audio data", cppgen::Base::CommentType::FillTo80Light);
			cppgen::Include m(i, sourceDir, f);
			break;
		}
	}

	bool somethingFound = false;

	for (auto& f : fileList)
	{
		if (getLocationType(f) == ThirdPartyFile)
		{
			if (!somethingFound)
			{
				i.addComment("Include third party header files", cppgen::Base::CommentType::FillTo80Light);
				somethingFound = true;
			}

			cppgen::Base dummyInclude(cppgen::Base::OutputType::NoProcessing);
			{
				dummyInclude.addComment("This just references the real file", cppgen::Base::CommentType::RawWithNewLine);
				cppgen::Include m(dummyInclude, sourceDir, f);
			}
			
			auto fInDir = sourceDir.getChildFile(f.getFileName());
			fInDir.replaceWithText(dummyInclude.toString());
			cppgen::Include m2(i, sourceDir, fInDir);
		}
	}

	if (somethingFound)
		i.addEmptyLine();

	somethingFound = false;

	for (auto& f : fileList)
	{
		if (getLocationType(f) == CompiledNetworkFile)
		{
			if (!somethingFound)
			{
				i.addComment("Include compiled network files", cppgen::Base::CommentType::FillTo80Light);
				somethingFound = true;
			}

			cppgen::Include m(i, sourceDir, f);
		}
	}

    i.addEmptyLine();
    
	i << "#if (defined (_WIN32) || defined (_WIN64))";
	i << "#pragma warning( pop )";
	i << "#else";
    i << "#pragma clang diagnostic pop";
	i << "#endif";
    
	includeFile.replaceWithText(i.toString());
}

void DspNetworkCompileExporter::createProjucerFile()
{
	String templateProject = String(projectDllTemplate_jucer);

	ProjectTemplateHelpers::handleCompilerWarnings(templateProject);
	
	auto& dataObject = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject();

	ProjectTemplateHelpers::handleVisualStudioVersion(dataObject, templateProject);

	const File jucePath = hisePath.getChildFile("JUCE/modules");

	auto projectName = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString();

    auto dllprefix = cppgen::StringHelpers::makeValidCppName(projectName);
    
	auto dllFolder = getFolder(BackendDllManager::FolderSubType::DllLocation);
	auto dbgFile = dllFolder.getChildFile(dllprefix + "_debug").withFileExtension(".dll");
	auto rlsFile = dllFolder.getChildFile(dllprefix).withFileExtension(".dll");
	auto ciFile = dllFolder.getChildFile(dllprefix + "_ci").withFileExtension(".dll");

	auto dbgName = dbgFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");
	auto rlsName = rlsFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");
	auto ciName =  ciFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");

    
    
#if JUCE_MAC
    REPLACE_WILDCARD_WITH_STRING("%USE_IPP_MAC%", useIpp ? "USE_IPP=1" : String());
    REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", useIpp ? "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a" : String());
    REPLACE_WILDCARD_WITH_STRING("%IPP_HEADER%", useIpp ? "/opt/intel/ipp/include" : String());
    REPLACE_WILDCARD_WITH_STRING("%IPP_LIBRARY%", useIpp ? "/opt/intel/ipp/lib" : String());
#endif

#if JUCE_LINUX
    REPLACE_WILDCARD_WITH_STRING("%USE_IPP_LINUX%", useIpp ? "USE_IPP=1" : "USE_IPP=0");
    REPLACE_WILDCARD_WITH_STRING("%IPP_COMPILER_FLAGS%", useIpp ? "/opt/intel/ipp/lib/libippi.a  /opt/intel/ipp/lib/libipps.a /opt/intel/ipp/lib/libippvm.a /opt/intel/ipp/lib/libippcore.a" : String());
#endif

	REPLACE_WILDCARD_WITH_STRING("%DEBUG_DLL_NAME%", dbgName);
	REPLACE_WILDCARD_WITH_STRING("%RELEASE_DLL_NAME%", rlsName);
	REPLACE_WILDCARD_WITH_STRING("%CI_DLL_NAME%", ciName);
	REPLACE_WILDCARD_WITH_STRING("%NAME%", projectName);
	REPLACE_WILDCARD_WITH_STRING("%HISE_PATH%", hisePath.getFullPathName());
	REPLACE_WILDCARD_WITH_STRING("%JUCE_PATH%", jucePath.getFullPathName());

	auto includeFaust = BackendDllManager::shouldIncludeFaust(getMainController());
	REPLACE_WILDCARD_WITH_STRING("%HISE_INCLUDE_FAUST%", includeFaust ? "enabled" : "disabled");

    
    String headerPath;
    
    if (includeFaust)
    {
        auto faustPath = dynamic_cast<GlobalSettingManager*>(getMainController())->getSettingsObject().getFaustPath();
        headerPath = faustPath.getChildFile("include").getFullPathName();
    }
    
    if(BackendDllManager::hasRNBOFiles(getMainController()))
    {
        auto folder = BackendDllManager::getRNBOSourceFolder(getMainController());
        
        headerPath << ";" << folder.getFullPathName();
        headerPath << ";" << folder.getChildFile("common").getFullPathName();
    }
    
    REPLACE_WILDCARD_WITH_STRING("%FAUST_HEADER_PATH%", headerPath);
    
    
    

	auto targetFile = getFolder(BackendDllManager::FolderSubType::Binaries).getChildFile("AutogeneratedProject.jucer");

	targetFile.replaceWithText(templateProject);
}

juce::File DspNetworkCompileExporter::getSourceDirectory(bool isDllMainFile) const
{
	File sourceDirectory;

	if (isDllMainFile)
		return getFolder(BackendDllManager::FolderSubType::ProjucerSourceFolder);
	else
		return GET_PROJECT_HANDLER(getMainController()->getMainSynthChain()).getSubDirectory(FileHandlerBase::AdditionalSourceCode).getChildFile("nodes");
}

void DspNetworkCompileExporter::createMainCppFile(bool isDllMainFile)
{
	File f;
	File sourceDirectory = getSourceDirectory(isDllMainFile);

	if (isDllMainFile)
		f = sourceDirectory.getChildFile("Main.cpp");
	else
		f = sourceDirectory.getChildFile("factory.cpp");

	using namespace cppgen;

	Base b(Base::OutputType::AddTabs);

	b.setHeader([]() { return "/** Autogenerated Main.cpp. */"; });

	b.addEmptyLine();

    b.addComment("Include only the DSP files ", snex::cppgen::Base::CommentType::FillTo80);
	Include(b, "hi_dsp_library/hi_dsp_library.h");
    Include(b, "hi_faust/hi_faust.h");
    
	Include(b, sourceDirectory, sourceDirectory.getChildFile("includes.h"));

    b.addComment("Now we can add the rest of the codebase", snex::cppgen::Base::CommentType::FillTo80);
    Include(b, "JuceHeader.h");
    
	b.addEmptyLine();

	{
		b.addComment("Project Factory", snex::cppgen::Base::CommentType::FillTo80);

		Namespace n(b, "project", false);

		DefinitionBase bc(b, "scriptnode::dll::StaticLibraryHostFactory");

		Struct s(b, "Factory", { &bc }, {});

		b << "Factory()";

		{
			cppgen::StatementBlock bk(b);

			b << "TempoSyncer::initTempoData();";

			b.addComment("Node registrations", snex::cppgen::Base::CommentType::FillTo80Light);

			for (int i = 0; i < includedThirdPartyFiles.size(); i++)
			{
				String def;

				String nid;
				nid << "project::" << includedThirdPartyFiles[i].getFileNameWithoutExtension();

				def << "registerPolyNode<" << nid << "<1>, " << nid << "<NUM_POLYPHONIC_VOICES>>();";
				b << def;
			}

			for (int i = 0; i < includedFiles.size(); i++)
			{
				auto networkFile = getFolder(BackendDllManager::FolderSubType::Networks).getChildFile(includedFiles[i].getFileNameWithoutExtension()).withFileExtension("xml");

				auto isPolyNode = includedFiles[i].loadFileAsString().contains("polyphonic template declaration");
				auto illegalPoly = isPolyNode && !BackendDllManager::allowPolyphonic(networkFile);
				
				String classId = "project::" + includedFiles[i].getFileNameWithoutExtension();

				String def;

				String methodPrefix = "register";

				if (isInterpretedDataFile(includedFiles[i]))
					methodPrefix << "Data";

				if (!isPolyNode)
					def << methodPrefix << "Node<" << classId << ">();";
				else
				{
					def << methodPrefix << "PolyNode<" << classId << "<1>, ";
					
					if (illegalPoly)
						def << "wrap::illegal_poly<" << classId << "<1>>>();";
					else
						def << classId << "<NUM_POLYPHONIC_VOICES>>();";
				}
					

				b << def;
			}
		}
	}

	if (isDllMainFile)
	{
		b << "project::Factory f;";

		b.addEmptyLine();

		b.addComment("Exported DLL functions", snex::cppgen::Base::CommentType::FillTo80);

		{
			b << "DLL_EXPORT int getNumNodes()";
			StatementBlock bk(b);
			b << "return f.getNumNodes();";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT size_t getNodeId(int index, char* t)";
			StatementBlock bk(b);
			b << "return HelperFunctions::writeString(t, f.getId(index).getCharPointer());";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT bool isThirdPartyNode(int index)";
			StatementBlock bk(b);
			String def;
			def << "return index < " << String(includedThirdPartyFiles.size()) << ";";
			b << def;
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT int getNumDataObjects(int nodeIndex, int dataTypeAsInt)";
			StatementBlock bk(b);
			b << "return f.getNumDataObjects(nodeIndex, dataTypeAsInt);";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT void deInitOpaqueNode(scriptnode::OpaqueNode* n)";
			StatementBlock bk(b);
			b << "n->callDestructor();";
		}

		b.addEmptyLine();

		{
			b << "DLL_EXPORT void initOpaqueNode(scriptnode::OpaqueNode* n, int index, bool polyIfPossible)";
			StatementBlock bk(b);
			b << "f.initOpaqueNode(n, index, polyIfPossible);";
		}

		{
			b << "DLL_EXPORT int getHash(int index)";
			StatementBlock bk(b);

			if (includedFiles.size() == 0)
			{
				b << "return 0;";
			}
			else
			{
				String def1;
				def1 << "static const int thirdPartyOffset = " << String(includedThirdPartyFiles.size()) << ";";
				b << def1;

				String def;

				def << "static const int hashIndexes[" << includedFiles.size() << "] =";
				b << def;

				{
					StatementBlock l(b, true);

					for (int i = 0; i < includedFiles.size(); i++)
					{
						if (isInterpretedDataFile(includedFiles[i]))
							break;

						String l;

						auto hash = getDllManager()->getHashForNetworkFile(getMainController(), includedFiles[i].getFileNameWithoutExtension());

						if (hash == 0)
						{
							jassertfalse;
						}

						l << hash;

						if ((i != includedFiles.size() - 1) && !isInterpretedDataFile(includedFiles[i+1]))
							l << ", ";

						b << l;
					}
				}

				b << "return (index >= thirdPartyOffset) ? hashIndexes[index - thirdPartyOffset] : 0;";
			}
		}
		{
			b << "DLL_EXPORT int getWrapperType(int index)";
			StatementBlock bk(b);
			b << "return f.getWrapperType(index);";
		}

		{
			b << "DLL_EXPORT ErrorC getError()";
			StatementBlock bk(b);
			b << "return f.getError();";
		}

		{
			b << "DLL_EXPORT void clearError()";
			StatementBlock bk(b);
			b << "f.clearError();";
		}

		{
			b << "DLL_EXPORT int getDllVersionCounter()";
			StatementBlock bk(b);
			b << "return scriptnode::dll::ProjectDll::DllUpdateCounter;";
		}
	}
	else
	{
		b << "scriptnode::dll::FactoryBase* scriptnode::DspNetwork::createStaticFactory()";
		{
			StatementBlock sb(b);
			b << "return new project::Factory();";
		}
	}

	f.replaceWithText(b.toString());
    
    auto rnboSibling = f.getSiblingFile("RNBO.cpp");
    
    if(BackendDllManager::hasRNBOFiles(getMainController()))
    {
        Base r(Base::OutputType::AddTabs);
        
        r.setHeader([]() { return "/** Autogenerated RNBO.cpp. file */"; });

        b.addEmptyLine();

        auto rroot = BackendDllManager::getRNBOSourceFolder(getMainController());
        
        r << "#define RNBO_NO_PATCHERFACTORY 1";
        r << "#define RNBO_USE_FLOAT32 1";
        
        Include(r, sourceDirectory, rroot.getChildFile("RNBO.cpp"));
        
        rnboSibling.replaceWithText(r.toString());
    }
    else
    {
        rnboSibling.replaceWithText("");
    }
}

void TestRunWindow::run()
{
	auto testId = getSoloTest();

	for (auto c : getConfigurations())
	{
		String cMessage;
		cMessage << "Testing configuration: ";

		switch (c)
		{
		case DspNetworkCodeProvider::SourceMode::InterpretedNode: cMessage << "Interpreted"; break;
		case DspNetworkCodeProvider::SourceMode::JitCompiledNode: cMessage << "JIT"; break;
		case DspNetworkCodeProvider::SourceMode::DynamicLibrary: cMessage << "Dynamic Library"; break;
        default: break;
		}

		writeConsoleMessage(cMessage);

		for (auto f : filesToTest)
		{
			if (testId.isNotEmpty() && testId != f.getFileNameWithoutExtension())
				continue;

			runTest(f, c);
		}
	}
}

void TestRunWindow::runTest(const File& f, DspNetworkCodeProvider::SourceMode m)
{
	String fileMessage;
	fileMessage << " Testing Network " << f.getFileName();
	getMainController()->getConsoleHandler().writeToConsole(fileMessage, 0, nullptr, Colours::white);

	auto testRoot = editor->dllManager->getSubFolder(getMainController(), BackendDllManager::FolderSubType::Tests).getChildFile(f.getFileNameWithoutExtension());
	Array<File> testFiles = testRoot.findChildFiles(File::findFiles, false, "*.json");

	if (testFiles.isEmpty())
	{
		return;
	}

	WorkbenchData::Ptr d = new snex::ui::WorkbenchData();

	auto nh = SnexWorkbenchEditor::getNetworkHolderForNewFile(getMainController(), false);

	ScopedPointer<DspNetworkCodeProvider> dnp = new DspNetworkCodeProvider(d.get(), getMainController(), nh, f);

	d->setCodeProvider(dnp);
	auto dch = new DspNetworkCompileHandler(d.get(), getMainController(), nh);
	d->setCompileHandler(dch);

	d->getTestData().setTestRootDirectory(testRoot);

	// fix the dll stuff...
	jassertfalse;

	//dnp->initialiseNetwork(editor->projectDll);

	

	for (auto testFile : testFiles)
	{
		
			

		String testMessage;
		testMessage << "  Running test " << testFile.getFileName();
		getMainController()->getConsoleHandler().writeToConsole(testMessage, 0, nullptr, Colours::white);

		auto obj = JSON::parse(testFile);

		if (obj.isObject())
		{
			d->getTestData().fromJSON(obj);
			
			

			dnp->setSource(m);

			auto outputFile = d->getTestData().testOutputFile;

			if (outputFile.existsAsFile())
			{
				double s;

				try
				{
					auto expectedBuffer = hlac::CompressionHelpers::loadFile(outputFile, s);
					auto actualBuffer = d->getTestData().testOutputData;

					BufferTestResult r(actualBuffer, expectedBuffer);

					if (!r.r.wasOk())
					{
						getMainController()->getConsoleHandler().writeToConsole("  " + r.r.getErrorMessage(), 1, nullptr, Colours::white);
						allOk = false;
					}	
				}
				catch (String& s)
				{
					showStatusMessage(s);
				}
			}
			else
			{
				writeConsoleMessage("skipping test due to missing reference file");
			}
		}
	}

	d = nullptr;
	dch = nullptr;
	dnp = nullptr;
}

void TestRunWindow::threadFinished()
{
	
	if (!allOk)
		PresetHandler::showMessageWindow("Test failed", "Some tests failed. Check the console output");
	else
		PresetHandler::showMessageWindow("Sucess", "All tests were sucessful");
}



void SnexWorkbenchEditor::MenuLookAndFeel::drawMenuBarBackground(Graphics& g, int width, int height, bool, MenuBarComponent& menuBar)
{

}

void SnexWorkbenchEditor::MenuLookAndFeel::drawMenuBarItem(Graphics& g, int width, int height, int /*itemIndex*/, const String& itemText, bool isMouseOverItem, bool isMenuOpen, bool /*isMouseOverBar*/, MenuBarComponent& menuBar)
{
	Rectangle<float> ar(0.0f, 0.0f, (float)width, (float)height);

	g.setColour(Colours::white.withAlpha(0.1f));

	if(isMouseOverItem)
		g.fillRoundedRectangle(ar, 2.0f);

	g.setColour(Colour(0xFFAAAAAA));

	g.setFont(GLOBAL_FONT());
	g.drawText(itemText, ar, Justification::centred);
}

}
