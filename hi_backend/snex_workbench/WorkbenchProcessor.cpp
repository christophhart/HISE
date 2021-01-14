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
		MenuCommandIds::ToolsEditTestData
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
	case MenuCommandIds::ToolsCompileNetworks: setCommandTarget(result, "Compile DSP networks", true, false, 'x', false);
		break;
	case MenuCommandIds::ToolsAudioConfig: setCommandTarget(result, "Settings", true, false, 0, false); break;
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
		if (df != nullptr)
		{
			if (auto n = dnp->getActiveNetwork())
			{
				ScopedPointer<XmlElement> xml = n->getValueTree().createXml();
				df->getXmlFile().replaceWithText(xml->createDocument(""));
			}
		}

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
		auto window = new DspNetworkCompileExporter(getMainController());
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

	addAndMakeVisible(menuBar);

	auto f = getSubFolder(getProcessor(), FolderSubType::DllLocation).getChildFile("project_debug.dll");

	if (f.existsAsFile())
	{
		projectDll = new DynamicLibrary();
		projectDll->open(f.getFullPathName());
	}

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

	menuBar.setBounds(b.removeFromTop(24));
	rootTile->setBounds(b);
}

void SnexWorkbenchEditor::requestQuit()
{
	standaloneProcessor.requestQuit();
}

void SnexWorkbenchEditor::addFile(const File& f)
{
	df = new hise::DspNetworkCodeProvider(nullptr, getMainController(), f);

	wb = dynamic_cast<BackendProcessor*>(getMainController())->workbenches.getWorkbenchDataForCodeProvider(df, true);

	if (auto h = ProcessorHelpers::getFirstProcessorWithType<scriptnode::DspNetwork::Holder>(getProcessor()->getMainSynthChain()))
	{
		if (projectDll != nullptr)
			df->setProjectFactory(projectDll, h->getActiveNetwork());
	}

	wb->setCompileHandler(new hise::DspNetworkCompileHandler(wb, getMainController()));

	ep->setWorkbenchData(wb.get());

	auto testRoot = getSubFolder(getProcessor(), FolderSubType::Tests);
	
	wb->getTestData().setTestRootDirectory(createIfNotDirectory(testRoot.getChildFile(f.getFileNameWithoutExtension())));

	wb->addListener(this);

	wb->triggerRecompile();

	dgp->setContentWithUndo(dnp, dnp->getActiveNetworkIndex());
}


void DspNetworkCompileExporter::run()
{
	auto buildFolder = getFolder(SnexWorkbenchEditor::FolderSubType::Binaries);

	auto networkRoot = getFolder(SnexWorkbenchEditor::FolderSubType::Networks);
	auto list = networkRoot.findChildFiles(File::findFiles, false, "*.xml");
	auto sourceDir = getFolder(SnexWorkbenchEditor::FolderSubType::ProjucerSourceFolder);

	Array<File> generatedFiles;

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

			generatedFiles.add(f);
		}
	}

	File includeFile = sourceDir.getChildFile("includes.h");

	Base i(Base::OutputType::AddTabs);

	i.setHeader([]() { return "/* Include file. */"; });

	for (auto f : generatedFiles)
	{
		Include m(i, includeFile, f);
	}

	includeFile.replaceWithText(i.toString());

	createMainCppFile();
}

void DspNetworkCompileExporter::createMainCppFile()
{
	auto sourceDirectory = getFolder(SnexWorkbenchEditor::FolderSubType::ProjucerSourceFolder);
	auto f = sourceDirectory.getChildFile("Main.cpp");

	cppgen::Base b(snex::cppgen::Base::OutputType::AddTabs);

	b.setHeader([]() { return "/** Autogenerated Main.cpp. */"; });

	f.replaceWithText(b.toString());
}

}
