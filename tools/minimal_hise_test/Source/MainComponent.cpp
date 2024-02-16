/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"


void MainComponent::build()
{
    using namespace multipage;
    using namespace factory;

#if 1

	addAndMakeVisible(hardcodedDialog = new multipage::library::BroadcasterWizard());

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
        m.addItemWithShortcut(CommandId::EditRedo, "Redo",  KeyPress('y', ModifierKeys::commandModifier, 's'), c->getUndoManager().canRedo());
        m.addSeparator();
		m.addItemWithShortcut(CommandId::EditToggleMode, "Toggle Edit mode", KeyPress(KeyPress::F4Key), c->isEditModeAllowed(), c->isEditModeEnabled());
		m.addItemWithShortcut(CommandId::EditRefreshPage, "Refresh current page", KeyPress(KeyPress::F5Key));
		m.addSeparator();
		m.addItem(CommandId::EditClearState, "Clear state object");
        m.addItem(CommandId::EditAddPage, "Add page", c->isEditModeAllowed());
		m.addItem(CommandId::EditRemovePage, "Remove current page", c->isEditModeAllowed() && c->getNumPages() > 1);
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
	if(key.getKeyCode() == KeyPress::F4Key && c != nullptr)
	{
		menuItemSelected(CommandId::EditToggleMode, 0);
		return true;
	}
	if(key.getKeyCode() == KeyPress::F5Key && c != nullptr)
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
	case EditClearState: rt.globalState.getDynamicObject()->clear(); break;
	case EditUndo: c->getUndoManager().undo(); c->refreshCurrentPage(); break;
	case EditRedo: c->getUndoManager().redo(); c->refreshCurrentPage(); break;
	case EditToggleMode: 
		c->setEditMode(!c->isEditModeEnabled());
		c->repaint();
		break;
	case EditRefreshPage: c->refreshCurrentPage(); tree.setRoot(*c); break;
    case EditAddPage: c->addListPageWithJSON(); break;
	case EditRemovePage: c->removeCurrentPage(); break;
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
#if HISE_MULTIPAGE_INCLUDE_EDIT
			multipage::CodeGenerator cg(c->exportAsJSON());
			manualChange = false;
			doc.removeListener(this);
			c->setVisible(false);
			stateViewer.setVisible(true);
			doc.replaceAllContent(cg.toString());
#endif
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

	tree.setRoot(*c);

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
	addChildComponent(watchTable);
	addChildComponent(tree);

	watchTable.setHolder(&rt);

	Array<var> l({ var("Name"), var("Value")});
	watchTable.restoreColumnVisibility(var(l));
	watchTable.setRefreshRate(200, 1);

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
    
    setSize(1300, 900);
	//setSize (2560, 1080);
#else
    setSize(1300, 720);
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


	auto b = getLocalBounds().toFloat();

	

	b.removeFromTop(24);


	auto tb = b.removeFromTop(32);

	GlobalHiseLookAndFeel::drawFake3D(g, tb.toNearestInt());

	if(c != nullptr)
	{
		b.removeFromLeft(tree.getWidth());
		b.removeFromRight(watchTable.getWidth());

		Colour c = JUCE_LIVE_CONSTANT(Colour(0x22000000));

		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x22000000)));

		g.drawVerticalLine(b.getX(), 0.0f, (float)getHeight());
		g.drawVerticalLine(b.getRight()-1, 0.0f, (float)getHeight());

		g.drawVerticalLine(b.getX(), 24+32, (float)getHeight());
		g.drawVerticalLine(b.getRight()-1, 24+32, (float)getHeight());

		g.setGradientFill(ColourGradient(c, b.getX(), 0.0f, Colours::transparentBlack, b.getX() + 5.0f, 0.0f, false));
		g.fillRect(b.removeFromLeft(20));

		g.setGradientFill(ColourGradient(c, b.getRight(), 0.0f, Colours::transparentBlack, b.getRight() - 5.0f, 0.0f, false));
		g.fillRect(b.removeFromRight(20));

		


	}

	g.setFont(GLOBAL_BOLD_FONT());
	g.setColour(Colours::white.withAlpha(0.7f));
	g.drawText("Component List", tb.removeFromLeft(300.0f), Justification::centred);
	g.drawText("State Variables", tb.removeFromRight(300.0f), Justification::centred);
	g.drawText("Dialog Preview", tb, Justification::centred);
}

void MainComponent::resized()
{
    auto b = getLocalBounds();
	
    menuBar.setBounds(b.removeFromTop(24));

	b.removeFromTop(32);

    if(c != nullptr)
    {
		watchTable.setVisible(true);
		watchTable.setBounds(b.removeFromRight(300));

		tree.setVisible(true);
		tree.setBounds(b.removeFromLeft(300));
		b.removeFromLeft(2);

		b.removeFromRight(2);

	    c->setBounds(b);
    }
        

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
