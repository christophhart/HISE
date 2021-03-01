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
		m.addCommandItem(&mainManager, MenuCommandIds::FileSave);

		PopupMenu allFiles;

		auto root = getProjectHandler().getSubDirectory(FileHandlerBase::DspNetworks);

		auto fileList = root.findChildFiles(File::findFiles, true, "*.xml");
		int i = 0;

		for (auto& f : fileList)
		{
			allFiles.addItem(MenuCommandIds::FileOffset + i++, f.getRelativePathFrom(root), true, activeFiles.contains(f));
		}

		m.addSubMenu("Open from Project", allFiles);

		m.addSeparator();

		m.addCommandItem(&mainManager, MenuCommandIds::FileSetProject);

		PopupMenu recentProjects;
		auto current = getProjectHandler().getWorkDirectory();

		auto s = ProjectHandler::getRecentWorkDirectories();

		i = 0;

		for (auto& p : s)
			recentProjects.addItem(MenuCommandIds::ProjectOffset + i++, p, true, p == current.getFullPathName());

		m.addSubMenu("Recent projects", recentProjects);

		return m;
	}
	case MenuItems::ToolsMenu:
	{
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
		MenuCommandIds::FileSetProject,
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
	case MenuCommandIds::FileSave: setCommandTarget(result, "Save File", true, false, 'S', true, ModifierKeys::commandModifier); break;
	case MenuCommandIds::FileSetProject: setCommandTarget(result, "Set Project", true, false, 0, false); break;
	case MenuCommandIds::ToolsEditTestData: setCommandTarget(result, "Manage Test Files", true, false, 0, false); break;
	case MenuCommandIds::ToolsCompileNetworks: setCommandTarget(result, "Compile DSP networks", true, false, 'x', false); break;
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
		auto fileName = PresetHandler::getCustomName("DspNetwork", "Enter the name of the new dsp network.");
		fileName = fileName.upToFirstOccurrenceOf(".", false, false);

		if (!fileName.isEmpty())
		{
			auto f = getSubFolder(getProcessor(), FolderSubType::Networks).getChildFile(fileName + ".xml");
			addFile(f);
		}

		return true;
	};
	case FileOpen:
	{
		FileChooser fc("Open File", getSubFolder(getProcessor(), FolderSubType::Networks), "*.xml", true);

		if (fc.browseForFileToOpen())
		{
			auto f = fc.getResult();
			addFile(f);
		}

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
	case ToolsCompileNetworks:
	{
		auto window = new DspNetworkCompileExporter(this);
		window->setOpaque(true);
		window->setModalBaseWindowComponent(this);

		return true;
	}
	case ToolsClearDlls:
	{
		if (projectDll != nullptr)
		{
			PresetHandler::showMessageWindow("Dll already loaded", "You can't delete the build folder when the dll is already loaded");
			return true;
		}

		getSubFolder(getProcessor(), FolderSubType::Binaries).deleteRecursively();

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

using namespace snex::Types;


//==============================================================================
SnexWorkbenchEditor::SnexWorkbenchEditor(const String &commandLine) :
	standaloneProcessor()
{
#if 0
	UnitTestRunner runner;
	runner.setPassesAreLogged(true);
	runner.setAssertOnFailure(false);

	runner.runTestsInCategory("node_tests");
#endif

	rootTile = new FloatingTile(getProcessor(), nullptr);

	{
		raw::Builder b(getProcessor());

		auto mc = getProcessor();
		dnp = b.add(new DspNetworkProcessor(mc, "internal"), mc->getMainSynthChain(), raw::IDs::Chains::FX);
	}
	

	context.attachTo(*this);

	mainManager.registerAllCommandsForTarget(this);
	mainManager.setFirstCommandTarget(this);

	mainManager.getKeyMappings()->resetToDefaultMappings();
	addKeyListener(mainManager.getKeyMappings());

	menuBar.setModel(this);
	menuBar.setLookAndFeel(&getProcessor()->getGlobalLookAndFeel());

    
    
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

		auto r = 0;

		builder.setNewContentType<HorizontalTile>(r);
		builder.setDynamic(r, false);

		//auto topBar = builder.addChild<VerticalTile>(r);

		//builder.addChild<TooltipPanel>(topBar);
		//auto htb = builder.addChild<VisibilityToggleBar>(topBar);
		
		auto info = builder.addChild<SnexWorkbenchPanel<hise::WorkbenchInfoComponent>>(r);

		//builder.setSizes(topBar, { 250, -1.0, 250 });
		//builder.setDynamic(topBar, false);
		//builder.setFoldable(topBar, false, { false, false, false });

		auto v = builder.addChild<VerticalTile>(r);

		int vtb = builder.addChild<VisibilityToggleBar>(v);
		
		auto dg = builder.addChild<scriptnode::DspNetworkGraphPanel>(v);

		int editor = builder.addChild<SnexEditorPanel>(v);

		ep = builder.getContent<SnexEditorPanel>(editor);

		ep->setCustomTitle("SNEX Code Editor");
		

		//auto t = builder.addChild<FloatingTabComponent>(v);
		//builder.getPanel(t)->setCustomIcon((int)FloatingTileContent::Factory::PopupMenuOptions::ScriptEditor);
		//auto rightColumn = builder.addChild<HorizontalTile>(v);
		//builder.getPanel(rightColumn)->setCustomIcon((int)FloatingTileContent::Factory::PopupMenuOptions::ModuleBrowser);
		

		dgp = builder.getContent<scriptnode::DspNetworkGraphPanel>(dg);
		dgp->setCustomTitle("Scriptnode DSP Graph");

		dgp->setForceHideSelector(true);
		


		auto pl = builder.addChild<SnexWorkbenchPanel<snex::ui::TestDataComponent>>(r);
		auto cpl = builder.addChild<SnexWorkbenchPanel<snex::ui::TestComplexDataManager>>(r);
		auto uig = builder.addChild<SnexWorkbenchPanel<snex::ui::Graph>>(r);

		builder.getContent<FloatingTileContent>(pl)->setCustomTitle("Test Events");
		builder.getContent<FloatingTileContent>(cpl)->setCustomTitle("Test Complex Data");
		builder.getContent<FloatingTileContent>(uig)->setCustomTitle("Test Signal Display");

		
		builder.getContent< SnexWorkbenchPanel<hise::WorkbenchInfoComponent>>(info)->forceShowTitle = false;

		builder.setFoldable(v, false, { false, false, false });

		builder.setSizes(v, {30.0, -0.33, -0.33 });
		builder.setDynamic(v, false);

#if 0
		builder.addChild<ScriptWatchTablePanel>(rightColumn);
		builder.addChild<SnexWorkbenchPanel<snex::ui::OptimizationProperties>>(rightColumn);
#endif
		

		auto c = builder.addChild<ConsolePanel>(r);

		//builder.getContent<VisibilityToggleBar>(htb)->setControlledContainer(builder.getContainer(r));

		builder.setSizes(r, { 30.0, -0.4, -0.2, -0.2, -0.2, -0.2 });

		builder.getContent<VisibilityToggleBar>(vtb)->refreshButtons();;

		builder.finalizeAndReturnRoot();
	}

	if (auto console = rootTile->findFirstChildWithType<ConsolePanel>())
	{
		console->getConsole()->setTokeniser(new snex::DebugHandler::Tokeniser());
	}

	addAndMakeVisible(rootTile);

	

	setSize(800, 600);
}


SnexWorkbenchEditor::~SnexWorkbenchEditor()
{
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

#if !JUCE_MAC
	menuBar.setBounds(b.removeFromTop(24));
#endif
	rootTile->setBounds(b);
}

void SnexWorkbenchEditor::requestQuit()
{
	standaloneProcessor.requestQuit();
}

juce::File SnexWorkbenchEditor::getBestProjectDll(DllType t) const
{
	auto dllFolder = getSubFolder(getProcessor(), FolderSubType::DllLocation);

#if JUCE_WINDOWS
    String extension = "*.dll";
#else
    String extension = "*.dylib";
#endif
    
	auto files = dllFolder.findChildFiles(File::findFiles, false, extension);


	auto removeWildcard = "Never";

	if (t == DllType::Debug)
		removeWildcard = "release";
	if (t == DllType::Release)
		removeWildcard = "debug";

	for (int i = 0; i < files.size(); i++)
	{
		if(files[i].getFileName().contains(removeWildcard))
			files.remove(i--);
	}

	if (files.isEmpty())
		return File();

	struct FileDateComparator
	{
		int compareElements(const File& first, const File& second)
		{
			auto t1 = first.getCreationTime();
			auto t2 = second.getCreationTime();

			if (t1 < t2) return 1;
			if (t2 < t1) return -1;
			return 0;
		}
	};

	FileDateComparator c;
	files.sort(c);

	return files.getFirst();
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

bool SnexWorkbenchEditor::loadDll(bool forceUnload)
{
	if (forceUnload)
		unloadDll();

	if (projectDll == nullptr)
	{
		auto dllFile = getBestProjectDll(DllType::Latest);

		bool ok = false;

		if (dllFile.existsAsFile())
		{
			projectDll = new DynamicLibrary();
			ok = projectDll->open(dllFile.getFullPathName());
		}

		if (ok && df != nullptr)
		{
			loadDllFactory(df);
		}
	}

	return false;
}

void SnexWorkbenchEditor::loadDllFactory(DspNetworkCodeProvider* cp)
{
	auto chain = getProcessor()->getMainSynthChain();

	if (auto h = ProcessorHelpers::getFirstProcessorWithType<scriptnode::DspNetwork::Holder>(chain))
	{
		cp->setProjectFactory(projectDll, h->getActiveNetwork());
	}
}

int64 SnexWorkbenchEditor::getHash(bool getDllHash)
{
	int64 hash = 0;

	if (!getDllHash)
	{
		auto nr = getSubFolder(getProcessor(), FolderSubType::Networks);

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
			if(auto f = (int64(*)())projectDll->getFunction("getHash"))
				return f();
		}
	}

	return hash;
}

void SnexWorkbenchEditor::addFile(const File& f)
{
	df = new hise::DspNetworkCodeProvider(nullptr, getMainController(), f);

	wb = dynamic_cast<BackendProcessor*>(getMainController())->workbenches.getWorkbenchDataForCodeProvider(df, true);

	

	wb->setCompileHandler(new hise::DspNetworkCompileHandler(wb, getMainController()));

	ep->setWorkbenchData(wb.get());

	auto testRoot = getSubFolder(getProcessor(), FolderSubType::Tests);
	
	wb->getTestData().setTestRootDirectory(createIfNotDirectory(testRoot.getChildFile(f.getFileNameWithoutExtension())));

	wb->addListener(this);

	loadDll(false);

	wb->triggerRecompile();

	dgp->setContentWithUndo(dnp, dnp->getActiveNetworkIndex());
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

void DspNetworkCompileExporter::run()
{
	

	showStatusMessage("Unload DLL");

	editor->saveCurrentFile();
	editor->unloadDll();

	showStatusMessage("Create files");

	auto buildFolder = getFolder(SnexWorkbenchEditor::FolderSubType::Binaries);

	auto networkRoot = getFolder(SnexWorkbenchEditor::FolderSubType::Networks);
	auto list = networkRoot.findChildFiles(File::findFiles, false, "*.xml");
	auto sourceDir = getFolder(SnexWorkbenchEditor::FolderSubType::ProjucerSourceFolder);

	using namespace snex::cppgen;

	for (auto e : list)
	{
		if (ScopedPointer<XmlElement> xml = XmlDocument::parse(e))
		{
			auto v = ValueTree::fromXml(*xml).getChild(0);

			auto id = v[scriptnode::PropertyIds::ID].toString();

			ValueTreeBuilder b(v, ValueTreeBuilder::Format::CppDynamicLibrary);

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

		auto loadedOk = editor->loadDll(true);

		if (loadedOk)
		{
			auto list = editor->df->projectDllFactory->getModuleList();

			String c;

			c << "Included DSP networks:\n  > `";
			c << list.joinIntoString(", ") << "`";

			PresetHandler::showMessageWindow("Loaded DLL " + editor->getBestProjectDll(SnexWorkbenchEditor::DllType::Latest).getFileName(), c);
			return;
		}
	}
}

juce::File DspNetworkCompileExporter::getBuildFolder() const
{
	return getFolder(SnexWorkbenchEditor::FolderSubType::Binaries);
}

void DspNetworkCompileExporter::createProjucerFile()
{
	String templateProject = String(projectDllTemplate_jucer);

	
	const File jucePath = hisePath.getChildFile("JUCE/modules");

	auto projectName = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Project::Name).toString();

	auto dllFolder = getFolder(SnexWorkbenchEditor::FolderSubType::DllLocation);


	auto dbgFile = dllFolder.getChildFile("project_debug").withFileExtension(".dll");
	auto rlsFile = dllFolder.getChildFile("project").withFileExtension(".dll");

	auto dbgName = dbgFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");
	auto rlsName = rlsFile.getNonexistentSibling(false).getFileNameWithoutExtension().removeCharacters(" ");


	REPLACE_WILDCARD_WITH_STRING("%DEBUG_DLL_NAME%", dbgName);
	REPLACE_WILDCARD_WITH_STRING("%RELEASE_DLL_NAME%", rlsName);
	REPLACE_WILDCARD_WITH_STRING("%NAME%", projectName);
	REPLACE_WILDCARD_WITH_STRING("%HISE_PATH%", hisePath.getFullPathName());
	REPLACE_WILDCARD_WITH_STRING("%JUCE_PATH%", jucePath.getFullPathName());

	auto targetFile = getFolder(SnexWorkbenchEditor::FolderSubType::Binaries).getChildFile("AutogeneratedProject.jucer");

	targetFile.replaceWithText(templateProject);
}

void DspNetworkCompileExporter::createMainCppFile()
{
	auto sourceDirectory = getFolder(SnexWorkbenchEditor::FolderSubType::ProjucerSourceFolder);
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
		b << "DLL_EXPORT void initOpaqueNode(scriptnode::OpaqueNode* n, int index)";
		StatementBlock bk(b);
		b << "f.initOpaqueNode(n, index);";
	}

	{
		b << "DLL_EXPORT int64 getHash()";
		StatementBlock bk(b);
		String def;
		def << "return " << editor->getHash(false) << ";";
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
			editor->loadDll(false);

			if (editor->getHash(true) != editor->getHash(false))
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

	auto testRoot = editor->getSubFolder(getMainController(), SnexWorkbenchEditor::FolderSubType::Tests).getChildFile(f.getFileNameWithoutExtension());
	Array<File> testFiles = testRoot.findChildFiles(File::findFiles, false, "*.json");

	if (testFiles.isEmpty())
	{
		return;
	}

	WorkbenchData::Ptr d = new snex::ui::WorkbenchData();

	ScopedPointer<DspNetworkCodeProvider> dnp = new DspNetworkCodeProvider(d, getMainController(), f);

	d->setCodeProvider(dnp);
	auto dch = new DspNetworkCompileHandler(d, getMainController());
	d->setCompileHandler(dch);

	d->getTestData().setTestRootDirectory(testRoot);

	if (m == DspNetworkCodeProvider::SourceMode::DynamicLibrary)
	{
		editor->loadDllFactory(dnp);
		
	}

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

}
