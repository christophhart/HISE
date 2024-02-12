/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"

#include "MultiPageDialog.h"
#include "PageFactory.h"



void MainComponent::build()
{
    using namespace multipage;
    using namespace factory;

#if 1

	addAndMakeVisible(hardcodedDialog = new multipage::library::NewProjectWizard());

#else

    auto mp = new Dialog(var(), rt);
	
	mp->setFinishCallback([](){
    	JUCEApplication::getInstance()->systemRequestedQuit();
    });
    

    mp->setEditMode(true);

    mp->showFirstPage();
    
    addAndMakeVisible(c = mp);
#endif
}

PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const String&)
{
	PopupMenu m;

	if(topLevelMenuIndex == 0) // File
	{
		m.addItem(CommandId::FileNew, "New file");
		m.addItem(CommandId::FileLoad, "Load file");

		PopupMenu r;
		fileList.createPopupMenuItems(r, CommandId::FileRecentOffset, false, false);
		m.addSubMenu("Recent files", r);
		m.addItemWithShortcut(CommandId::FileSave, "Save file",  KeyPress('s', ModifierKeys::commandModifier, 's'), currentFile.existsAsFile());
		m.addItem(CommandId::FileSaveAs, "Save file as");
		m.addSeparator();
		m.addItem(CommandId::FileQuit, "Quit");
	}
	if(topLevelMenuIndex == 1) // Edit
	{
        if(c == nullptr)
        {
            m.addItem(123123123, "Disabled in hardcoded mode", false);
            return m;
        }
        
        m.addItemWithShortcut(CommandId::EditUndo, "Undo",  KeyPress('z', ModifierKeys::commandModifier, 's'), c->getUndoManager().canUndo());
        m.addItemWithShortcut(CommandId::EditRedo, "Undo",  KeyPress('y', ModifierKeys::commandModifier, 's'), c->getUndoManager().canRedo());
        m.addSeparator();
		m.addItemWithShortcut(CommandId::EditToggleMode, "Toggle Edit mode", KeyPress(KeyPress::F4Key), c->isEditModeAllowed(), c->isEditModeEnabled());
		m.addItemWithShortcut(CommandId::EditRefreshPage, "Refresh current page", KeyPress(KeyPress::F5Key));
        m.addItem(CommandId::EditAddPage, "Add page", c->isEditModeAllowed());
	}
	if(topLevelMenuIndex == 2) // View
	{
		m.addItem(CommandId::ViewShowDialog, "Show dialog");
        
        if(c != nullptr)
        {
            m.addItem(CommandId::ViewShowCpp, "Show C++ code", c->isEditModeAllowed());
            m.addItem(CommandId::ViewShowJSON, "Show JSON editor", c->isEditModeAllowed());
        }
	}
	if(topLevelMenuIndex == 3) // Help
	{
		m.addItem(CommandId::HelpAbout, "About");
		m.addItem(CommandId::HelpVersion, "Version");
	}

	return m;
}

bool MainComponent::keyPressed(const KeyPress& key)
{
	if(key.getKeyCode() == KeyPress::F4Key)
	{
		menuItemSelected(CommandId::EditToggleMode, 0);
		return true;
	}
	if(key.getKeyCode() == KeyPress::F5Key)
	{
		menuItemSelected(CommandId::EditRefreshPage, 0);
		return true;
	}
	if(key.getKeyCode() == 's' && key.getModifiers().isCommandDown())
	{
		menuItemSelected(CommandId::FileSave, 0);
		return true;
	}
}

void MainComponent::menuItemSelected(int menuItemID, int)
{
	if(menuItemID >= CommandId::FileRecentOffset)
	{
		auto f = fileList.getFile(menuItemID - CommandId::FileRecentOffset);
		createDialog(f);
		return;
	}

	switch((CommandId)menuItemID)
	{
	case FileNew:
		createDialog(File());
		break;
	case FileLoad:
		{
			FileChooser fc("Open JSON file", File(), "*.json");
			if(fc.browseForFileToOpen())
			{
				createDialog(fc.getResult());
			}

			break;
		}
	case FileSave:
		{
			if(currentFile.existsAsFile())
				currentFile.replaceWithText(JSON::toString(c->exportAsJSON()));

			setSavePoint();

			break;
		}
	case FileSaveAs:
		{
			FileChooser fc("Save JSON file", File(), "*.json");

			if(fc.browseForFileToSave(true))
			{
				currentFile = fc.getResult();
				currentFile.replaceWithText(JSON::toString(c->exportAsJSON()));

				setSavePoint();
			}
                
			break;
		}
	case FileQuit: JUCEApplication::getInstance()->systemRequestedQuit(); break;
	case EditUndo: c->getUndoManager().undo(); c->refreshCurrentPage(); break;
	case EditRedo: c->getUndoManager().redo(); c->refreshCurrentPage(); break;
	case EditToggleMode: 
		c->setEditMode(!c->isEditModeEnabled());
		c->repaint();
		break;
	case EditRefreshPage: c->refreshCurrentPage(); break;
        case EditAddPage: c->addListPageWithJSON(); break;
	case ViewShowDialog:
		{
			if(manualChange && AlertWindow::showOkCancelBox(MessageBoxIconType::QuestionIcon, "Apply manual JSON changes?", "Do you want to reload the dialog after the manual edit?"))
			{
				stateViewer.setVisible(false);
				var obj;
				auto ok = JSON::parse(doc.getAllContent(), obj);

				if(ok.wasOk())
				{
                    c = nullptr;
					addAndMakeVisible(c = new multipage::Dialog(obj, rt));
					resized();

                    c->setFinishCallback([](){
    					JUCEApplication::getInstance()->systemRequestedQuit();
				    });

					c->showFirstPage();
				}
			}

			c->setVisible(true);
			stateViewer.setVisible(false);
			break;
		}
	case ViewShowJSON:
		manualChange = false;
		doc.removeListener(this);
		c->setVisible(false);
		stateViewer.setVisible(true);
		doc.replaceAllContent(JSON::toString(c->exportAsJSON()));
		doc.addListener(this);
		break;
	case ViewShowCpp:
		{
			multipage::CodeGenerator cg(c->exportAsJSON());
			manualChange = false;
			doc.removeListener(this);
			c->setVisible(false);
			stateViewer.setVisible(true);
			doc.replaceAllContent(cg.toString());
			break;
		}
	case HelpAbout: break;
	case HelpVersion: break;
	default: ;
	}
}

void MainComponent::createDialog(const File& f)
{
	var obj;

	if(f.existsAsFile())
	{
		JSON::parse(f.loadFileAsString(), obj);
		fileList.addFile(f);
	}

	currentFile = f;

    c = nullptr;
    hardcodedDialog = nullptr;

	addAndMakeVisible(c = new multipage::Dialog(obj, rt));

    c->setFinishCallback([](){
    	JUCEApplication::getInstance()->systemRequestedQuit();
    });

    c->setEditMode(true);
	resized();

	setSavePoint();
}

//==============================================================================
MainComponent::MainComponent():
  rt({}),
  doc(),
  stateDoc(doc),
  stateViewer(stateDoc),
  menuBar(this)
{
    LookAndFeel::setDefaultLookAndFeel(&plaf);

    auto settings = JSON::parse(getSettingsFile());

    if(auto so = settings.getDynamicObject())
        fileList.restoreFromString(so->getProperty("RecentFiles").toString());

    menuBar.setLookAndFeel(&plaf);
    addAndMakeVisible(menuBar);
    build();
	startTimer(3000);
    
    addChildComponent(stateViewer);

#if JUCE_WINDOWS
    
    setSize(1000, 900);
	//setSize (2560, 1080);
#else
    setSize(860, 720);
#endif
}

MainComponent::~MainComponent()
{
    auto f = fileList.toString();

	auto no = new DynamicObject();
    no->setProperty("RecentFiles", f);
    getSettingsFile().replaceWithText(JSON::toString(var(no), true));

#if JUCE_WINDOWS
	//context.detach();
#endif
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
	g.fillAll(Colour(0xFF333333));
}

void MainComponent::resized()
{
    auto b = getLocalBounds();

    menuBar.setBounds(b.removeFromTop(24));

    if(c != nullptr)
        c->setBounds(b);

	if(hardcodedDialog != nullptr)
		hardcodedDialog->setBounds(b);

    stateViewer.setBounds(b);
}

void MainComponent::timerCallback()
{
	if(c == nullptr)
		return;

    c->getUndoManager().beginNewTransaction();

	const int64 thisHash = JSON::toString(c->exportAsJSON(), true).hashCode64();

    if(firstAfterSave)
    {
	    prevHash = thisHash;
        firstAfterSave = false;
    }
    else
    {
	    modified |= thisHash != prevHash;
		prevHash = thisHash;
    }
}

void MainComponent::setSavePoint()
{
	modified = false;
    firstAfterSave = true;
	
	menuItemsChanged();
}
