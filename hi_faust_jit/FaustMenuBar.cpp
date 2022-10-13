namespace scriptnode {
namespace faust {

FaustMenuBar::FaustMenuBar(faust_jit_node *n) :
	addButton("add", this, factory),
	editButton("edit", this, factory),
	reloadButton("reset", this, factory),
	node(n)
{
	// we must provide a valid faust_jit_node pointer
	jassert(n);
	setLookAndFeel(&claf);
	setSize(200, 24);

	addAndMakeVisible(classSelector);
	classSelector.setColour(ComboBox::ColourIds::textColourId, Colour(0xFFAAAAAA));
	classSelector.setLookAndFeel(&claf);
	classSelector.addListener(this);

	editButton.setTooltip("Edit the current Faust source file in external editor");
	addAndMakeVisible(addButton);
	addAndMakeVisible(editButton);
	addAndMakeVisible(reloadButton);
	// gather existing source files
	rebuildComboBoxItems();
}

void FaustMenuBar::createNewFile()
{
	auto name = PresetHandler::getCustomName(node->getClassId(), "Enter the name for the Faust file");

	if (name.isNotEmpty())
	{
		if (!faust_jit_wrapper::isValidClassId(name))
		{
			// We want a clear and modal feedback that that wasn't a good idea...
			PresetHandler::showMessageWindow("Illegal file name", "Can't add file, because its name is not a valid class identifier: " + name, PresetHandler::IconType::Error);
			return;
		}
		node->createSourceAndSetClass(name);
		rebuildComboBoxItems();
		//refreshButtonState();
	}
}

std::optional<juce::File> FaustMenuBar::promptForDestinationFile(String extension, File& previousDestFile)
{
	auto destChooser = juce::FileChooser("File with the same name already exists, select a destination file",
		previousDestFile, "*." + extension);
	if (!destChooser.browseForFileToSave(true))
		return {};
	return destChooser.getResult();
}

void FaustMenuBar::importFile(String extension)
{
	auto chooser = juce::FileChooser("Faust file to import into the project",
		node->getFaustRootFile(), "*." + extension);
	if (!chooser.browseForFileToOpen())
		return;

	File sourceFile = chooser.getResult();

	if (!sourceFile.existsAsFile())
		return;

	File destFile = node->getFaustRootFile().getChildFile(sourceFile.getFileNameWithoutExtension() + "." + extension);

	if (destFile.exists()) {
		auto maybeNewDestFile = promptForDestinationFile(extension, destFile);
		if (!maybeNewDestFile.has_value())
			return;
		destFile = *maybeNewDestFile;
	}

	auto classId = destFile.getFileNameWithoutExtension();
	if (extension == "dsp" && !faust_jit_wrapper::isValidClassId(classId)) {
		DBG("Can't import file, because its name is not a valid class identifier: " + classId);
		return;
	}

	DBG("Copying: \"" + sourceFile.getFullPathName() + "\" -> \"" + destFile.getFullPathName() + "\"");
	if (!sourceFile.copyFileTo(destFile))
		return;

	// If it's a faust source file, load it
	if (extension == "dsp")
		node->setClass(destFile.getFileNameWithoutExtension());
}

void FaustMenuBar::renameFile()
{
	auto classId = node->getClassId();
	auto faustDir = node->getFaustRootFile();
	auto currentFile = faustDir.getChildFile(classId + ".dsp");

	auto maybeNewDestFile = promptForDestinationFile("dsp", currentFile);
	if (!maybeNewDestFile.has_value())
		return;
	auto destFile = *maybeNewDestFile;

	// TODO race conditions?

	node->removeClassId(classId);
	if (!currentFile.moveFileTo(destFile))
	{
		node->logError("Could not move file to new location: " +
			currentFile.getFullPathName() + " --> " +
			destFile.getFullPathName());
		return;
	}

	node->setClass(destFile.getFileNameWithoutExtension());
}

void FaustMenuBar::executeMenuAction(int option)
{
	switch (option) {
	case NEW_FILE:
		createNewFile();
		break;
	case IMPORT_FILE:
		importFile("dsp");
		break;
	case IMPORT_LIB:
		importFile("lib");
		break;
	case RENAME_FILE:
		renameFile();
	case REMOVE_FILE:
		removeFile();

		// add code for more functions here
	default:
		std::cerr << "FaustMenuBar: Unknown MenuOption: " + option << std::endl;
	}
}

void FaustMenuBar::rebuildComboBoxItems()
{
	// DBG("FaustMenuBar rebuilt");
	classSelector.clear(dontSendNotification);
	classSelector.addItemList(node->getAvailableClassIds(), 1);

	// if (auto w = source->getWorkbench())
	//     classSelector.setText(w->getInstanceId().toString(), dontSendNotification);
	classSelector.setText(node->getClassId());
}

void FaustMenuBar::resized()
{
	// DBG("FaustMenuBar resized");
	auto b = getLocalBounds().reduced(0, 1);
	auto h = getHeight();

	addButton.setBounds(b.removeFromLeft(h - 4));
	classSelector.setBounds(b.removeFromLeft(100));
	b.removeFromLeft(3);
	editButton.setBounds(b.removeFromRight(h).reduced(2));
	reloadButton.setBounds(b.removeFromRight(h + editButton.getWidth() / 2).reduced(2));

	b.removeFromLeft(10);
}

void FaustMenuBar::buttonClicked(Button* b)
{
	if (b == &addButton) {

		juce::PopupMenu m;
		m.setLookAndFeel(&claf);
		for (int o = MENU_OPTION_FIRST; o < MENU_OPTION_LAST; o++) {
			m.addItem(o, getTextForMenuOptionId(o), true);
		}

		int menu_selection = (MenuOption)m.show();
		if (menu_selection > 0)
			executeMenuAction(menu_selection);
	}
	else if (b == &editButton) {
		DBG("Edit button pressed");
		auto sourceFile = node->getFaustFile(node->getClassId());
		auto sourceFilePath = sourceFile.getFullPathName();
		if (sourceFile.existsAsFile()) {
			juce::Process::openDocument(sourceFilePath, "");
			DBG("Opened file: " + sourceFilePath);
		}
		else {
			DBG("File not found: " + sourceFilePath);
		}
	}
	else if (b == &reloadButton) {
		DBG("Reload button pressed");
		node->reinitFaustWrapper();
	}
}

void FaustMenuBar::comboBoxChanged(ComboBox *comboBoxThatHasChanged)
{
	auto name = comboBoxThatHasChanged->getText();
	DBG("Combobox changed, new text: " + name);
	node->setClass(name);
}

juce::Path FaustMenuBar::Factory::createPath(const String& url) const
{
	// TODO: Faust Logo
	if (url == "snex")
	{
		snex::ui::SnexPathFactory f;
		return f.createPath(url);
	}

	Path p;

	LOAD_PATH_IF_URL("new", ColumnIcons::threeDots);
	LOAD_PATH_IF_URL("edit", ColumnIcons::openWorkspaceIcon);
	LOAD_PATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_PATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("add", ColumnIcons::threeDots);

	return p;
}

}
}