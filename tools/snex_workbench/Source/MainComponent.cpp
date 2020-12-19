/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"



juce::PopupMenu MainContentComponent::getMenuForIndex(int topLevelMenuIndex, const String& menuName)
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
		
		auto root = getProjectHandler().getSubDirectory(FileHandlerBase::AdditionalSourceCode);

		auto fileList = root.findChildFiles(File::findFiles, true, "*.h");
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
	}
	}

	return m;
}

void MainContentComponent::getAllCommands(Array<CommandID>& commands)
{
	const CommandID id[] = {
		MenuCommandIds::FileNew,
		MenuCommandIds::FileOpen,
		MenuCommandIds::FileSave,
		MenuCommandIds::FileSetProject,
		MenuCommandIds::ToolsAudioConfig,
		MenuCommandIds::ToolsEditTestData
	};

	commands.addArray(id, numElementsInArray(id));
}

void MainContentComponent::getCommandInfo(CommandID commandID, ApplicationCommandInfo &result)
{
	switch (commandID)
	{
	case MenuCommandIds::FileNew: setCommandTarget(result, "New File", true, false, 'N', true, ModifierKeys::commandModifier); break;
	case MenuCommandIds::FileOpen: setCommandTarget(result, "Open File", true, false, 'O', true, ModifierKeys::commandModifier); break;
	case MenuCommandIds::FileSave: setCommandTarget(result, "Save File", true, false, 'S', true, ModifierKeys::commandModifier); break;
	case MenuCommandIds::FileSetProject: setCommandTarget(result, "Set Project", true, false, 0, false); break;
	case MenuCommandIds::ToolsEditTestData: setCommandTarget(result, "Manage Test Files", true, false, 0, false); break;
	case MenuCommandIds::ToolsAudioConfig: setCommandTarget(result, "Settings", true, false, 0, false); break;
	default: jassertfalse;
	}
}

bool MainContentComponent::perform(const InvocationInfo &info)
{
	switch (info.commandID)
	{
	case FileNew:
	{
		auto fileName = PresetHandler::getCustomName("Snex File", "Enter the name of the SNEX file.  \nThe file will be created in the project's `AdditionalSourceFiles` subfolder and it will create");
		fileName = fileName.upToFirstOccurrenceOf(".", false, false);

		if(!fileName.isEmpty())
		{ 
			auto f = getRootFolder().getChildFile(fileName + ".h");

			if (!f.existsAsFile())
				f.replaceWithText(snex::ui::WorkbenchData::getDefaultNodeTemplate(fileName));

			addFile(f);
		}

		return true;
	};
	case FileOpen:
	{
		FileChooser fc("Open File", getRootFolder(), "*.h", true);

		if (fc.browseForFileToOpen())
		{
			auto f = fc.getResult();
			addFile(f);
		}

		return true;
	}
	case ToolsAudioConfig: 
	{
		auto window = new SettingWindows(getProcessor()->getSettingsObject(), {HiseSettings::SettingFiles::SnexWorkbenchSettings, HiseSettings::SettingFiles::AudioSettings});
		window->setModalBaseWindowComponent(this);
		window->activateSearchBox();
		return true;
	}
	}

	return false;
}

using namespace snex::Types;






//==============================================================================
MainContentComponent::MainContentComponent(const String &commandLine):
	standaloneProcessor()
{
	UnitTestRunner runner;
	runner.setPassesAreLogged(true);
	runner.setAssertOnFailure(false);

	runner.runTestsInCategory("node_tests");

	rootTile = new FloatingTile(getProcessor(), nullptr);
	
	

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

		builder.addChild<SnexWorkbenchPanel<snex::ui::Graph>>(r);

		auto v = builder.addChild<VerticalTile>(r);

		auto t = builder.addChild<FloatingTabComponent>(v);
		auto rightColumn = builder.addChild<HorizontalTile>(v);

		builder.setSizes(v, { -1.0, 350.0 });
		builder.setDynamic(v, false);

		builder.addChild<ScriptWatchTablePanel>(rightColumn);
		builder.addChild<SnexWorkbenchPanel<snex::ui::OptimizationProperties>>(rightColumn);
		builder.addChild<Note>(rightColumn);

		

		auto c = builder.addChild<ConsolePanel>(r);
		
		builder.setSizes(r, {-0.25, -0.5, -0.25 });

		builder.finalizeAndReturnRoot();
	}

	tabs = rootTile->findFirstChildWithType<FloatingTabComponent>();
	
	tabs->setAddButtonCallback([this]()
	{
		mainManager.invokeDirectly(FileNew, false);
	});

	if (auto console = rootTile->findFirstChildWithType<ConsolePanel>())
	{
		console->getConsole()->setTokeniser(new snex::jit::Compiler::Tokeniser());
	}

	addAndMakeVisible(rootTile);

	addAndMakeVisible(menuBar);

	setSize(800, 600);
}


MainContentComponent::~MainContentComponent()
{
	//getLayoutFile().replaceWithText(rootTile->exportAsJSON());
	context.detach();

	rootTile = nullptr;
}

void MainContentComponent::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF222222));
}

void MainContentComponent::resized()
{
	auto b = getLocalBounds();

	menuBar.setBounds(b.removeFromTop(24));
	rootTile->setBounds(b);
}

void MainContentComponent::requestQuit()
{
	standaloneProcessor.requestQuit();
}

void MainContentComponent::addFile(const File& f)
{
	FloatingInterfaceBuilder b(tabs->getParentShell());

	int id = b.addChild<SnexEditorPanel>(0);

	b.getContent<SnexEditorPanel>(id)->setCurrentFile(f);
}
