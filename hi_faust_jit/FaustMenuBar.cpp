#include "FaustMenuBar.h"

namespace scriptnode {
namespace faust {

FaustMenuBar::FaustMenuBar(faust_jit_node_base *n) :
	addButton("add", this, factory),
	editButton("edit", this, factory),
	reloadButton("reset", this, factory),
	node(n),
    dragger(n->getFaustModulationOutputs(), n->getScriptProcessor()->getMainController_()->getGlobalUIUpdater())
{
	// we must provide a valid faust_jit_node pointer
	jassert(n);
	setLookAndFeel(&claf);
    
    auto h = 24;
    
    dragger.showEditButtons(false);
    addChildComponent(dragger);
        
	setSize(256, h);
    
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
    
    auto& fb = n->getRootNetwork()->faustManager;
    
    editButton.setToggleModeWithColourChange(true);
    
    fb.addFaustListener(this);
    
}

FaustMenuBar::~FaustMenuBar()
{
    node->getRootNetwork()->faustManager.removeFaustListener(this);
}


void FaustMenuBar::createNewFile()
{
	auto name = PresetHandler::getCustomName(node->getClassId(), "Enter the name for the Faust file");

	if (name.isNotEmpty())
	{
		if (!faust_jit_helpers::isValidClassId(name))
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
	if (extension == "dsp" && !faust_jit_helpers::isValidClassId(classId)) {
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


void FaustMenuBar::paintOverChildren(Graphics& g)
{
	if (compilePending)
	{
		g.fillAll(Colour(0xCC353535));
		g.setColour(Colours::white.withAlpha(0.7f));
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Recompiling Faust code...", getLocalBounds().toFloat(), Justification::verticallyCentred);
	}
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
    case REBUILD_PARAMETERS:
    {
        StringArray automatedProperties;
        
        for(auto p: node->getParameterTree())
        {
            if(p[PropertyIds::Automated])
                automatedProperties.add(p[PropertyIds::ID].toString());
        }
        
        node->getParameterTree().removeAllChildren(node->getUndoManager());
        buttonClicked(&reloadButton);
        
        for(auto s: automatedProperties)
        {
            auto newP = node->getParameterTree().getChildWithProperty(PropertyIds::ID, s);
            
            if(newP.isValid())
                newP.setProperty(PropertyIds::Automated, true, node->getUndoManager());
        }
        
        break;
    }
        
	case RENAME_FILE:
		renameFile();
        break;
	case REMOVE_FILE:
		removeFile();
        break;

		// add code for more functions here
	default:
		std::cerr << "FaustMenuBar: Unknown MenuOption: " << String(option) << std::endl;
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
	

    if(dragger.isVisible())
    {
        dragger.setBounds(b.removeFromBottom(parameter::ui::UIConstants::DragHeight));
        b.removeFromBottom(UIValues::NodeMargin);
    }
    
    auto h = b.getHeight();
    
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
        
        auto p = dynamic_cast<Processor*>(node->getScriptProcessor());
        
        auto openInEditor = GET_HISE_SETTING(p, HiseSettings::Compiler::FaustExternalEditor);
        
        if(openInEditor)
        {
            juce::Process::openDocument(sourceFilePath, "");
        }
        else
        {
			auto& faustManager = node->getRootNetwork()->faustManager;
            faustManager.setSelectedFaustFile(b, sourceFile, sendNotificationAsync);
        }
	}
	else if (b == &reloadButton) {
		DBG("Reload button pressed");
        
        auto& faustManager = node->getRootNetwork()->faustManager;
        auto sourceFile = node->getFaustFile(node->getClassId());
        
        faustManager.sendCompileMessage(sourceFile, sendNotificationSync);
	}
}

void FaustMenuBar::comboBoxChanged(ComboBox *comboBoxThatHasChanged)
{
	auto name = comboBoxThatHasChanged->getText();
	DBG("Combobox changed, new text: " + name);
	node->setClass(name);
}

void FaustMenuBar::faustFileSelected(const File& f)
{
    auto sourceFile = node->getFaustFile(node->getClassId());
    
    editButton.setToggleStateAndUpdateIcon(matchesFile(f));
}

void FaustMenuBar::preCompileFaustCode(const File& f)
{
	if (matchesFile(f))
	{
		compilePending = true;
		SafeAsyncCall::repaint(this);
	}
}

Result FaustMenuBar::compileFaustCode(const File& f)
{
    // Nothing to do here
    return Result::ok();
}

void FaustMenuBar::faustCodeCompiled(const File& f, const Result& compileResult)
{
	if (matchesFile(f))
	{
		compilePending = false;
        
        if(compileResult.wasOk())
        {
            auto numOutputs = node->getNumFaustModulationOutputs();
            
            for(int i = 0; i < numOutputs; i++)
            {
                dragger.setTextFunction(i, BIND_MEMBER_FUNCTION_1(FaustMenuBar::getModulationOutputName));
            }
            
            dragger.rebuildDraggers();
            
            auto h = 24;
            
            if(numOutputs > 0)
            {
                dragger.setVisible(true);
                h += parameter::ui::UIConstants::DragHeight + UIValues::NodeMargin;
            }
            else
                dragger.setVisible(false);
            
            if(h != getHeight())
                node->sendResizeMessage(this, true);
            
            setSize(256, h);
        }
        
		repaint();
	}
	
    ignoreUnused(f, compileResult);
}

juce::Path FaustMenuBar::Factory::createPath(const String& url) const
{
	if (url == "snex")
	{
		static const unsigned char pathData[] = { 110,109,112,172,60,68,237,230,229,67,98,128,201,55,68,237,230,229,67,78,215,51,68,100,255,221,67,78,215,51,68,206,63,212,67,98,78,215,51,68,18,125,202,67,128,201,55,68,99,146,194,67,112,172,60,68,99,146,194,67,98,59,140,65,68,99,146,194,67,0,128,69,68,
18,125,202,67,0,128,69,68,206,63,212,67,98,0,128,69,68,100,255,221,67,59,140,65,68,237,230,229,67,112,172,60,68,237,230,229,67,99,109,181,86,16,68,237,230,229,67,98,86,117,11,68,237,230,229,67,0,128,7,68,100,255,221,67,0,128,7,68,206,63,212,67,98,0,128,
7,68,18,125,202,67,86,117,11,68,99,146,194,67,181,86,16,68,99,146,194,67,98,128,54,21,68,99,146,194,67,215,43,25,68,18,125,202,67,215,43,25,68,206,63,212,67,98,215,43,25,68,100,255,221,67,128,54,21,68,237,230,229,67,181,86,16,68,237,230,229,67,99,109,
36,186,46,68,200,177,171,67,108,10,251,37,68,49,190,106,67,108,2,8,29,68,200,177,171,67,108,182,150,28,68,54,168,174,67,108,150,6,13,68,54,168,174,67,108,230,144,29,68,38,50,44,67,108,36,144,47,68,38,50,44,67,108,234,195,63,68,160,155,174,67,108,150,
46,47,68,168,177,174,67,108,36,186,46,68,200,177,171,67,99,101,0,0 };

		Path path;
		path.loadPathFromData (pathData, sizeof (pathData));
		return path;
	}

	Path p;

	LOAD_PATH_IF_URL("new", ColumnIcons::threeDots);
	LOAD_PATH_IF_URL("edit", ColumnIcons::openWorkspaceIcon);
	LOAD_EPATH_IF_URL("compile", EditorIcons::compileIcon);
	LOAD_EPATH_IF_URL("reset", EditorIcons::swapIcon);
	LOAD_PATH_IF_URL("add", ColumnIcons::threeDots);
	LOAD_EPATH_IF_URL("preview", BackendBinaryData::ToolbarIcons::viewPanel);

	return p;
}

}
}
