/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "MainComponent.h"
#include "Exporter.h"

void MainComponent::build()
{
    using namespace multipage;
    using namespace factory;

#if 0

	addAndMakeVisible(hardcodedDialog = new multipage::library::BroadcasterWizard());

#else

	createDialog(File());

	

	
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
		m.addItem(CommandId::FileExportAsProjucerProject, "Export as Projucer project");
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

		m.addItem(CommandId::ViewShowConsole, "Show console", true, console->isVisible());
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
				rt.currentRootDirectory = currentFile.getParentDirectory();
				setSavePoint();
			}
                
			break;
		}
	case FileExportAsProjucerProject:
	{
		addAndMakeVisible(modalDialog = new ModalDialog(*this, new multipage::projucer_exporter(rt.currentRootDirectory, rt)));
		
		break;
	}
	case FileQuit: JUCEApplication::getInstance()->systemRequestedQuit(); break;
	case EditClearState: rt.globalState.getDynamicObject()->clear(); break;
	case EditUndo: c->getUndoManager().undo(); c->refreshCurrentPage(); break;
	case EditRedo: c->getUndoManager().redo(); c->refreshCurrentPage(); break;
	case EditToggleMode: 
		c->setEditMode(!c->isEditModeEnabled());
		c->repaint();
        resized();
		break;
	case EditRefreshPage: c->refreshCurrentPage(); tree->getContent<multipage::Tree>()->setRoot(*c); resized(); break;
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
	case ViewShowConsole:
		console->setVisible(!console->isVisible());
		resized();
		break;
	case ViewShowCpp:
		{
#if HISE_MULTIPAGE_INCLUDE_EDIT
			multipage::CodeGenerator cg(rt.currentRootDirectory, "MyClass", c->exportAsJSON());
			manualChange = false;
			doc.removeListener(this);
			c->setVisible(false);
			stateViewer.setVisible(true);

			MemoryOutputStream mos;
			cg.write(mos, multipage::CodeGenerator::FileType::DialogImplementation, nullptr);
			doc.replaceAllContent(mos.toString());
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
		rt.currentRootDirectory = f.getParentDirectory();

		autosaver = new Autosaver(f, rt);
	}

	currentFile = f;

    c = nullptr;
    hardcodedDialog = nullptr;

	rt.reset(obj);

	addAndMakeVisible(c = new multipage::Dialog(obj, rt));

	if(f.existsAsFile())
		c->logMessage(multipage::MessageType::Navigation, "Load file " + f.getFullPathName());

	tree->getContent<multipage::Tree>()->setRoot(*c);
	assetManager.listbox.updateContent();
	assetManager.repaint();
	assetManager.resized();

    c->setFinishCallback([](){
    	JUCEApplication::getInstance()->systemRequestedQuit();
    });

    c->setEditMode(true);
	resized();

	c->toBack();

	setSavePoint();
}

//==============================================================================
MainComponent::MainComponent():
  rt({}),
  doc(),
  stateDoc(doc),
  assetManager(rt),
  stateViewer(stateDoc),
  menuBar(this),
  tooltips(this)
{
	TopLevelWindowWithKeyMappings::loadKeyPressMap();

	mcl::TextEditor::initKeyPresses(this);

	addAndMakeVisible(rightTab);

	auto watchTable = new ScriptWatchTable();

	rightTab.add(new SideTab(), 0.5);
	rightTab.add(watchTable, 0.3);
	rightTab.add(new AssetManager(rt), 0.2);

	addAndMakeVisible(tree = ComponentWithEdge::wrap(new hise::multipage::Tree(), ResizableEdgeComponent::rightEdge, 360));
	addAndMakeVisible(assetManager);
	addAndMakeVisible(console = ComponentWithEdge::wrap(new hise::multipage::EventConsole(rt), ResizableEdgeComponent::topEdge, 200));

	watchTable->setHolder(&rt);

	Array<var> l({ var("Name"), var("Value")});
	watchTable->restoreColumnVisibility(var(l));
	watchTable->setRefreshRate(200, 1);
	watchTable->setUseParentTooltipClient(true);
	watchTable->setName("State variables");

    LookAndFeel::setDefaultLookAndFeel(&plaf);

    auto settings = JSON::parse(getSettingsFile());

    if(auto so = settings.getDynamicObject())
        fileList.restoreFromString(so->getProperty("RecentFiles").toString());

#if JUCE_MAC
    setMacMainMenu (this);
#else
    menuBar.setLookAndFeel(&plaf);
    addAndMakeVisible(menuBar);
#endif
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
	TopLevelWindowWithKeyMappings::saveKeyPressMap();

#if JUCE_MAC
    setMacMainMenu (nullptr);
#endif
    
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
    
    g.fillAll(Colour(0xFF222222));
    
    if(c != nullptr)
    {
        g.setColour(c->getStyleData().backgroundColour);
        g.fillRect(c->getBoundsInParent());
        g.setColour(Colours::white.withAlpha(0.2f));
        g.drawRect(c->getBoundsInParent().reduced(-1.0f), 1.0f);
    }
    

	auto b = getLocalBounds().toFloat();

	if(c != nullptr)
	{
		b.removeFromLeft(tree->getWidth());
		b.removeFromRight(rightTab.getWidth());

		Colour c = JUCE_LIVE_CONSTANT(Colour(0x22000000));

		g.setGradientFill(ColourGradient(c, b.getX(), 0.0f, Colours::transparentBlack, b.getX() + 5.0f, 0.0f, false));
		g.fillRect(b.removeFromLeft(20));

		g.setGradientFill(ColourGradient(c, b.getRight(), 0.0f, Colours::transparentBlack, b.getRight() - 5.0f, 0.0f, false));
		g.fillRect(b.removeFromRight(20));
	}
}

void MainComponent::resized()
{
    auto b = getLocalBounds();

	if(b.isEmpty())
		return;

#if !JUCE_MAC
    menuBar.setBounds(b.removeFromTop(24));
	b.removeFromTop(5);
#endif



    if(c != nullptr)
    {
		auto rb = b.removeFromRight(rightTab.getWidth());

		rightTab.setBounds(rb);
		
		tree->setVisible(true);
		tree->setBounds(b.removeFromLeft(tree->getWidth()));

		if(console->isVisible())
			console->setBounds(b.removeFromBottom(console->getHeight()));

		b.removeFromLeft(2);
		b.removeFromRight(2);

		

		auto pb = b.reduced(20);

		auto dialogBounds = c->defaultLaf.defaultPosition.getBounds(pb);

		if(dialogBounds.getWidth() > pb.getWidth() ||
		   dialogBounds.getHeight() > pb.getHeight())
		{
			auto wratio = (float)dialogBounds.getWidth() / (float)pb.getWidth();
			auto hratio = (float)dialogBounds.getHeight() / (float)pb.getHeight();

			auto ratio = jmax(wratio, hratio);

			auto topLeft = dialogBounds.getTopLeft();

			topLeft.applyTransform(AffineTransform::scale(1.0f / ratio));

			
			c->setTransform(AffineTransform::scale(1.0f / ratio));

			auto parentArea = dialogBounds.transformedBy(c->getTransform().inverted());

			auto width = dialogBounds.getWidth();
			auto height = dialogBounds.getHeight();

		    c->setBounds (parentArea.getCentreX() - width / 2,
		               parentArea.getCentreY() - height / 2,
		               width, height);

			//c->centreWithSize(dialogBounds.getWidth(), dialogBounds.getHeight());

		}
		else
		{
			c->setTransform(AffineTransform());
			c->setBounds(dialogBounds);
		}
		
    }
	else if(console->isVisible())
		console->setBounds(b.removeFromBottom(200));
        

	if(hardcodedDialog != nullptr)
		hardcodedDialog->setBounds(hardcodedDialog->dialog->defaultLaf.defaultPosition.getBounds(b));

    stateViewer.setBounds(b);
    
    repaint();
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
