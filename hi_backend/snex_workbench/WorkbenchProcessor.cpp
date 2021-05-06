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
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsShowKeyboard);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsAudioConfig);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsEditTestData);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsCompileNetworks);
		m.addCommandItem(&mainManager, MenuCommandIds::ToolsClearDlls);
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
		MenuCommandIds::FileCloseCurrentNetwork,
		MenuCommandIds::FileSetProject,
		MenuCommandIds::FileShowNetworkFolder,
		MenuCommandIds::ToolsEffectMode,
		MenuCommandIds::ToolsSynthMode,
		MenuCommandIds::ToolsShowKeyboard,
		MenuCommandIds::ToolsAudioConfig,
		MenuCommandIds::ToolsCompileNetworks,
		MenuCommandIds::ToolsClearDlls,
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
	case MenuCommandIds::ToolsEditTestData: setCommandTarget(result, "Manage Test Files", true, false, 0, false); break;
	case MenuCommandIds::ToolsCompileNetworks: setCommandTarget(result, "Compile DSP networks", true, false, 'x', false); break;
	case MenuCommandIds::ToolsSynthMode: setCommandTarget(result, "Polyphonic Synth Mode", true, synthMode, 'x', false); break;
	case MenuCommandIds::ToolsEffectMode: setCommandTarget(result, "Stereo Effect Mode", true, !synthMode, 'x', false); 
	  break;
	case MenuCommandIds::ToolsShowKeyboard: setCommandTarget(result, "Show Keyboard", true, bottomComoponent.isVisible(), 'x', false);
		break;
	case MenuCommandIds::ToolsClearDlls: setCommandTarget(result, "Clear DLL build folder", true, false, 'x', false); break;
	case MenuCommandIds::ToolsAudioConfig: setCommandTarget(result, "Settings", true, false, 0, false); break;
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
		auto window = new SettingWindows(getProcessor()->getSettingsObject(), { HiseSettings::SettingFiles::SnexWorkbenchSettings, HiseSettings::SettingFiles::AudioSettings });
		window->setModalBaseWindowComponent(this);
		window->activateSearchBox();
		return true;
	}
	case ToolsEffectMode: setSynthMode(false); return true;
	case ToolsSynthMode: setSynthMode(true); return true;
	case ToolsShowKeyboard: bottomComoponent.setVisible(!bottomComoponent.isVisible()); resized(); return true;
	case ToolsCompileNetworks:
	{
		auto window = new DspNetworkCompileExporter(this);
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
			auto h1 = dllManager->getHash(true);
			auto h2 = dllManager->getHash(false);

			if (h1 != h2)
			{
				PresetHandler::showMessageWindow("DLL mismatch", "The dll is different than the source files. Recompile it");
			}

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
	auto fileName = PresetHandler::getCustomName("DspNetwork", "Enter the name of the new dsp network.");
	fileName = fileName.upToFirstOccurrenceOf(".", false, false);

	if (!fileName.isEmpty())
	{
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
SnexWorkbenchEditor::SnexWorkbenchEditor(const String &commandLine) :
	standaloneProcessor(),
	infoComponent(nullptr),
	bottomComoponent(getProcessor())
{
	dllManager = new BackendDllManager(getProcessor());

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

		dnp = b.add(new DspNetworkProcessor(mc, "internal"), mc->getMainSynthChain(), raw::IDs::Chains::FX);



	}
	

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

		auto dg = builder.addChild<scriptnode::DspNetworkGraphPanel>(v);

		int editor = builder.addChild<SnexEditorPanel>(v);

		ep = builder.getContent<SnexEditorPanel>(editor);

		ep->setCustomTitle("SNEX Code Editor");
		
		auto testTab = builder.addChild<HorizontalTile>(v);

		builder.setVisibility(v, true, { true, true, false, false });
		//auto t = builder.addChild<FloatingTabComponent>(v);
		//builder.getPanel(t)->setCustomIcon((int)FloatingTileContent::Factory::PopupMenuOptions::ScriptEditor);
		//auto rightColumn = builder.addChild<HorizontalTile>(v);
		//builder.getPanel(rightColumn)->setCustomIcon((int)FloatingTileContent::Factory::PopupMenuOptions::ModuleBrowser);
		

		dgp = builder.getContent<scriptnode::DspNetworkGraphPanel>(dg);
		dgp->setCustomTitle("Scriptnode DSP Graph");
		dgp->setForceHideSelector(true);
		
		builder.getContent<HorizontalTile>(testTab)->setPanelColour(ResizableFloatingTileContainer::PanelColourId::bgColour, Colour(0xFF1D1D1D));

		builder.setDynamic(testTab, false);

		auto pl = builder.addChild<SnexWorkbenchPanel<snex::ui::TestDataComponent>>(testTab);
		auto cpl = builder.addChild<SnexWorkbenchPanel<snex::ui::TestComplexDataManager>>(testTab);
		auto uig = builder.addChild<SnexWorkbenchPanel<snex::ui::Graph>>(testTab);

		builder.getContent<FloatingTileContent>(pl)->setCustomTitle("Test Events");
		builder.getContent<FloatingTileContent>(cpl)->setCustomTitle("Test Complex Data");
		builder.getContent<FloatingTileContent>(uig)->setCustomTitle("Test Signal Display");
		builder.setFoldable(v, false, { false, false, false, false });

		builder.setSizes(v, {30.0, -0.4, -0.4, -0.2 });
		builder.setDynamic(v, false);

		

#if 0
		builder.addChild<ScriptWatchTablePanel>(rightColumn);
		builder.addChild<SnexWorkbenchPanel<snex::ui::OptimizationProperties>>(rightColumn);
#endif
		

		//builder.getContent<VisibilityToggleBar>(htb)->setControlledContainer(builder.getContainer(r));

		builder.getContent<VisibilityToggleBar>(vtb)->refreshButtons();;

		builder.finalizeAndReturnRoot();
	}

	if (auto console = rootTile->findFirstChildWithType<ConsolePanel>())
	{
		console->getConsole()->setTokeniser(new snex::DebugHandler::Tokeniser());
	}

	addAndMakeVisible(rootTile);

	

	setSize(1680, 1050);
}


SnexWorkbenchEditor::~SnexWorkbenchEditor()
{
#if JUCE_MAC
    MenuBarModel::setMacMainMenu(nullptr);
#endif
    
	//getLayoutFile().replaceWithText(rootTile->exportAsJSON());
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
	standaloneProcessor.requestQuit();
}

void SnexWorkbenchEditor::saveCurrentFile()
{
	if (df != nullptr)
	{
		if (auto n = dnp->getActiveNetwork())
		{
			ScopedPointer<XmlElement> xml = n->getValueTree().createXml();
			df->getXmlFile().replaceWithText(xml->createDocument(""));
		}
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
	dgp->setContentWithUndo(nullptr, 0);

	auto n = getNetworkHolderForNewFile(getProcessor(), synthMode);

	ValueTree v("N");
	v.addChild(ValueTree("Networks"), -1, nullptr);

	n->restoreNetworks(v);

	snex::ui::WorkbenchData::Ptr p(wb.get());

	static_cast<WorkbenchManager*>(getProcessor()->getWorkbenchManager())->removeWorkbench(p);

	wb = nullptr;
	p = nullptr;

	resized();
}

void SnexWorkbenchEditor::addFile(const File& f)
{
	if (ScopedPointer<XmlElement> xml = XmlDocument::parse(f))
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

	if (dllManager == nullptr)
		dllManager = new BackendDllManager(getProcessor());

	dllManager->loadDll(false);

	auto fh = getNetworkHolderForNewFile(getProcessor(), synthMode);

	if (fh != nullptr)
		fh->setProjectDll(dllManager->projectDll);

	df = new hise::DspNetworkCodeProvider(nullptr, getMainController(), fh, f);

	wb = dynamic_cast<BackendProcessor*>(getMainController())->workbenches.getWorkbenchDataForCodeProvider(df, true);
	wb->getTestData().setMultichannelDataProvider(new PooledAudioFileDataProvider(getMainController()));
	wb->setCompileHandler(new hise::DspNetworkCompileHandler(wb, getMainController(), fh));

	wb->setIsCppPreview(true);

	ep->setWorkbenchData(wb.get());

	auto testRoot = BackendDllManager::getSubFolder(getProcessor(), BackendDllManager::FolderSubType::Tests);
	
	wb->getTestData().setTestRootDirectory(BackendDllManager::createIfNotDirectory(testRoot.getChildFile(f.getFileNameWithoutExtension())));
	wb->getTestData().setUpdater(getProcessor()->getGlobalUIUpdater());

	wb->addListener(this);

	fh->setExternalDataHolderToUse(&wb->getTestData());

	df->initNetwork();

	wb->triggerRecompile();

	dgp->setContentWithUndo(dynamic_cast<Processor*>(fh), 0);
}


DspNetworkCompileExporter::DspNetworkCompileExporter(SnexWorkbenchEditor* e) :
	DialogWindowWithBackgroundThread("Compile DSP networks"),
	ControlledObject(e->getProcessor()),
	CompileExporter(e->getProcessor()->getMainSynthChain()),
	editor(e)
{
	addComboBox("build", { "Debug", "Release" }, "Build Configuration");

	addBasicComponents(true);
}

struct IncludeSorter
{
	

	static int compareElements(const File& f1, const File& f2)
	{
		using namespace scriptnode;
		using namespace snex::cppgen;

		ScopedPointer<XmlElement> xml1 = XmlDocument::parse(f1);
		ScopedPointer<XmlElement> xml2 = XmlDocument::parse(f2);

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

			auto firstIsReferencedInSecond = ValueTreeIterator::forEach(v1, ValueTreeIterator::ChildrenFirst, f2);
			
			auto secondIsReferencedInFirst = ValueTreeIterator::forEach(v1, ValueTreeIterator::ChildrenFirst, f2);

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

void DspNetworkCompileExporter::run()
{
	showStatusMessage("Unload DLL");

	editor->saveCurrentFile();
	editor->dllManager->unloadDll();

	showStatusMessage("Create files");

	auto buildFolder = getFolder(BackendDllManager::FolderSubType::Binaries);

	auto networkRoot = getFolder(BackendDllManager::FolderSubType::Networks);
	auto unsortedList = networkRoot.findChildFiles(File::findFiles, false, "*.xml");

	showStatusMessage("Sorting include dependencies");

	Array<File> list;

	for (auto& nf : unsortedList)
	{
		for (auto& sf : getIncludedNetworkFiles(nf))
		{
			list.addIfNotAlreadyThere(sf);
		}
	}

	jassert(list.size() == unsortedList.size());

	auto sourceDir = getFolder(BackendDllManager::FolderSubType::ProjucerSourceFolder);

	using namespace snex::cppgen;

	for (auto e : list)
	{
		if (ScopedPointer<XmlElement> xml = XmlDocument::parse(e))
		{
			auto v = ValueTree::fromXml(*xml).getChild(0);

			auto id = v[scriptnode::PropertyIds::ID].toString();

			ValueTreeBuilder b(v, ValueTreeBuilder::Format::CppDynamicLibrary);

			b.setCodeProvider(new BackendDllManager::FileCodeProvider(getMainController()));

			auto f = sourceDir.getChildFile(id).withFileExtension(".h");

			auto r = b.createCppCode();

			if (r.r.wasOk())
				f.replaceWithText(r.code);
			else
				showStatusMessage(r.r.getErrorMessage());

			includedFiles.add(f);
		}
	}


	
	

	File includeFile = sourceDir.getChildFile("includes.h");

	Base i(Base::OutputType::AddTabs);

	i.setHeader([]() { return "/* Include file. */"; });

	for (auto f : includedFiles)
	{
		Include m(i, includeFile.getParentDirectory(), f);
	}

	includeFile.replaceWithText(i.toString());

	hisePath = File(GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Compiler::HisePath));

	createProjucerFile();
	createMainCppFile();

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

	

	ok = compileSolution(o, CompileExporter::TargetTypes::numTargetTypes);
}

void DspNetworkCompileExporter::threadFinished()
{
	if (ok == ErrorCodes::OK)
	{
		globalCommandLineExport = false;

		PresetHandler::showMessageWindow("Compilation OK", "Press OK to reload the project dll.");

		auto loadedOk = editor->dllManager->loadDll(true);

		if (loadedOk)
		{
			scriptnode::dll::HostFactory f(editor->dllManager->projectDll);

			StringArray list;
			
			for (int i = 0; i < f.getNumNodes(); i++)
			{
				list.add(f.getId(i));
			}

			String c;

			c << "Included DSP networks:\n  > `";
			c << list.joinIntoString(", ") << "`";

			PresetHandler::showMessageWindow("Loaded DLL " + editor->dllManager->getBestProjectDll(BackendDllManager::DllType::Latest).getFileName(), c);
			return;
		}
	}
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

	if (ScopedPointer<XmlElement> xml = XmlDocument::parse(networkFile))
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

		auto firstIsReferencedInSecond = ValueTreeIterator::forEach(v, ValueTreeIterator::ChildrenFirst, f2);
	}

	list.add(networkFile);

	return list;
}

void DspNetworkCompileExporter::createProjucerFile()
{
	String templateProject = String(projectDllTemplate_jucer);

	
	const File jucePath = hisePath.getChildFile("JUCE/modules");

	auto projectName = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString();

	auto dllFolder = getFolder(BackendDllManager::FolderSubType::DllLocation);
	auto dbgFile = dllFolder.getChildFile("project_debug").withFileExtension(".dll");
	auto rlsFile = dllFolder.getChildFile("project").withFileExtension(".dll");

	auto dbgName = dbgFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");
	auto rlsName = rlsFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");


	REPLACE_WILDCARD_WITH_STRING("%DEBUG_DLL_NAME%", dbgName);
	REPLACE_WILDCARD_WITH_STRING("%RELEASE_DLL_NAME%", rlsName);
	REPLACE_WILDCARD_WITH_STRING("%NAME%", projectName);
	REPLACE_WILDCARD_WITH_STRING("%HISE_PATH%", hisePath.getFullPathName());
	REPLACE_WILDCARD_WITH_STRING("%JUCE_PATH%", jucePath.getFullPathName());

	auto targetFile = getFolder(BackendDllManager::FolderSubType::Binaries).getChildFile("AutogeneratedProject.jucer");

	targetFile.replaceWithText(templateProject);
}

void DspNetworkCompileExporter::createMainCppFile()
{
	auto sourceDirectory = getFolder(BackendDllManager::FolderSubType::ProjucerSourceFolder);
	auto f = sourceDirectory.getChildFile("Main.cpp");

	using namespace cppgen;

	Base b(Base::OutputType::AddTabs);


	b.setHeader([]() { return "/** Autogenerated Main.cpp. */"; });

	b.addEmptyLine();

	Include(b, "JuceHeader.h");

	Include(b, sourceDirectory, sourceDirectory.getChildFile("includes.h"));
	
	b.addEmptyLine();

	{
		b.addComment("Project Factory", snex::cppgen::Base::CommentType::FillTo80);

		Namespace n(b, "project", false);

		DefinitionBase bc(b, "scriptnode::dll::PluginFactory");

		

		Struct s(b, "Factory", { &bc }, {});

		b << "Factory()";

		{
			cppgen::StatementBlock bk(b);

			b.addComment("Node registrations", snex::cppgen::Base::CommentType::FillTo80Light);

			for (int i = 0; i < includedFiles.size(); i++)
			{
				String def;
				def << "registerNode<project::" << includedFiles[i].getFileNameWithoutExtension() << ">();";
				b << def;
			}
		}
	}

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
		b << "DLL_EXPORT int getNumDataObjects(int nodeIndex, int dataTypeAsInt)";
		StatementBlock bk(b);
		b << "return f.getNumDataObjects(nodeIndex, dataTypeAsInt);";
	}

	b.addEmptyLine();

	{
		b << "DLL_EXPORT void initOpaqueNode(scriptnode::OpaqueNode* n, int index)";
		StatementBlock bk(b);
		b << "f.initOpaqueNode(n, index);";
	}

	{
		b << "DLL_EXPORT int getHash()";
		StatementBlock bk(b);
		String def;
		def << "return " << editor->dllManager->getHash(false) << ";";
		b << def;
	}

#if 0
	DLL_EXPORT int getNumNodes()
	{
		return f.getNumNodes();
	}

	DLL_EXPORT size_t getNodeId(int index, char* t)
	{
		return HelperFunctions::writeString(t, f.getId(index).getCharPointer());
	}

	DLL_EXPORT void initOpaqueNode(scriptnode::OpaqueNode* n, int index)
	{
		f.initOpaqueNode(n, index);
	}
#endif


	f.replaceWithText(b.toString());
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
		}

		if (c == DspNetworkCodeProvider::SourceMode::DynamicLibrary)
		{
			editor->dllManager->loadDll(false);

			if (editor->dllManager->getHash(true) != editor->dllManager->getHash(false))
			{
				writeConsoleMessage(" skipping dll test because the compiled dll hash is out of date");
				return;
			}
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

	ScopedPointer<DspNetworkCodeProvider> dnp = new DspNetworkCodeProvider(d, getMainController(), nh, f);

	d->setCodeProvider(dnp);
	auto dch = new DspNetworkCompileHandler(d, getMainController(), nh);
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

int BackendDllManager::getHash(bool getDllHash)
{
	int hash = 0;

	if (!getDllHash)
	{
		auto nr = getSubFolder(getMainController(), FolderSubType::Networks);

		auto list = nr.findChildFiles(File::findFiles, false, "*.xml");

		for (auto f : list)
		{
			if (ScopedPointer<XmlElement> xml = XmlDocument::parse(f))
			{
				auto v = ValueTree::fromXml(*xml);

				zstd::ZDefaultCompressor comp;

				MemoryBlock mb;
				comp.compress(v, mb);

				hash += mb.toBase64Encoding().hashCode();
			}
		}
	}
	else
	{
		loadDll(false);

		if (projectDll != nullptr)
		{
			return projectDll->getHash();
		}
	}

	return hash;
}

bool BackendDllManager::unloadDll()
{
	if (auto fh = ProcessorHelpers::getFirstProcessorWithType<scriptnode::DspNetwork::Holder>(getMainController()->getMainSynthChain()))
	{
		fh->setProjectDll(nullptr);
	}

	if (projectDll != nullptr)
	{
		projectDll = nullptr;
		return true;
	}

	return false;
}

bool BackendDllManager::loadDll(bool forceUnload)
{
	if (forceUnload)
		unloadDll();

	if (projectDll == nullptr)
	{
		auto dllFile = getBestProjectDll(DllType::Latest);

		bool ok = false;

		if (dllFile.existsAsFile())
		{
			projectDll = new scriptnode::dll::ProjectDll(dllFile);

			return *projectDll;
		}

		return ok;
	}

	return false;
}

juce::File BackendDllManager::createIfNotDirectory(const File& f)
{
	if (!f.isDirectory())
		f.createDirectory();

	return f;
}

juce::File BackendDllManager::getSubFolder(const MainController* mc, FolderSubType t)
{
	auto f = mc->getCurrentFileHandler().getSubDirectory(FileHandlerBase::DspNetworks);

	switch (t)
	{
	case FolderSubType::Root:					return createIfNotDirectory(f);
	case FolderSubType::Networks:				return createIfNotDirectory(f.getChildFile("Networks"));
	case FolderSubType::Tests:					return createIfNotDirectory(f.getChildFile("Tests"));
	case FolderSubType::CustomNodes:			return createIfNotDirectory(f.getChildFile("CustomNodes"));
	case FolderSubType::AdditionalCode:			return createIfNotDirectory(f.getChildFile("AdditionalCode"));
	case FolderSubType::CodeLibrary:			return createIfNotDirectory(f.getChildFile("CodeLibrary"));
	case FolderSubType::DllLocation:
#if JUCE_WINDOWS
		return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("dll").getChildFile("Dynamic Library"));
#else
		return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("dll"));
#endif
	case FolderSubType::Binaries:				return createIfNotDirectory(f.getChildFile("Binaries"));
	case FolderSubType::Layouts:				return createIfNotDirectory(f.getChildFile("Layouts"));
	case FolderSubType::ProjucerSourceFolder:	return createIfNotDirectory(f.getChildFile("Binaries").getChildFile("Source"));
	}

	jassertfalse;
	return {};
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
