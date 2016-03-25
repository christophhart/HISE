#include "ProtoWindow.h"
#include "../PluginProcessor.h"
#include "../ProtoplugDir.h"
#include "AboutBox.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4355) // 'this' used in base member initializer list
#endif


ApplicationCommandTarget *ProtoCmd::getFirstCommandTarget (CommandID commandID)
{ return pw; }

//==============================================================================
ProtoWindow::ProtoWindow (Component *parent, LuaProtoplugJuceAudioProcessor* ownerFilter)
	: commMgr(this),
    resizer(parent, &resizeLimits),
	menubar(this),
	editor(doc, &tok),
	paramPanel(ownerFilter),
	guiPanel(ownerFilter->luli),
	activePanel(0),
	activePanelComponent(0),
	paramDock(&paramPanel, "protoplug parameters", ownerFilter),
	guiDock(&guiPanel, "protoplug GUI", ownerFilter),
	tab1("code"), tab2("params"), tab3("gui"), 
	bottomPane(this)
{
	dirty = false;
	vstPanel = parent;
	popoutWindow = 0;
	panels[0] = &editor;
	panels[1] = &paramDock;
	panels[2] = &guiDock;
	processor = ownerFilter;
	themeFolder = ProtoplugDir::Instance()->getDir().getChildFile("themes").getFullPathName();
    LookAndFeel::setDefaultLookAndFeel (&newFeel);
    addAndMakeVisible (&menubar);
    menubar.setEnabled (true);

	setLookAndFeel(&alaf);

	resizeLimits.setMinimumHeight(100);
	resizeLimits.setMinimumWidth(419);
    addAndMakeVisible (&resizer);

	// panel 1 : editor
	// (moved to init folder)
    addChildComponent (&editor);
	editor.loadContent(processor->luli->code);

	// panel 2 : parameters
	addChildComponent(&paramDock);

	// panel 3 : custom GUI
	guiPanel.setOpaque(true);
	addChildComponent(&guiDock);

	activePanel = processor->lastUIPanel;
	panels[activePanel]->setVisible (true);
	activePanelComponent = panels[activePanel];
	
	// log text area
	addAndMakeVisible(&bottomPane);
	
	// splitter						item	min		max		default
    horizontalLayout.setItemLayout (0,		-0.1,	-1.0,	processor->lastUISplit-20);		// top (code editor)
    horizontalLayout.setItemLayout (1,		8,		8,		8);										// mid (splitter)
    horizontalLayout.setItemLayout (2,		22,		-0.9,	processor->lastUIHeight-processor->lastUISplit);	// bottom (log)
    horizontalDividerBar = new DarkSplitter (&horizontalLayout, 1, false);
    addAndMakeVisible (horizontalDividerBar);

    addAndMakeVisible (&tab1);
    addAndMakeVisible (&tab2);
    addAndMakeVisible (&tab3);
	tab1.setListener(this);
	tab2.setListener(this);
	tab3.setListener(this);

	tab1.setVisible(false);
	tab2.setVisible(false);
	tab3.setVisible(false);

	msg_UpdateLog = 1;
	msg_ParamsChanged = msg_TakeFocus = 0;
	hackTimer = 0;
    startTimer (50);
	addKeyListener(commMgr.getKeyMappings());
	
	commMgr.registerAllCommandsForTarget(this);
	editor.getDocument().addListener(this);
	readPrefs();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif


void ProtoWindow::initProtoplugDir()
{
	editor.loadContent(processor->luli->code);
	themeFolder = ProtoplugDir::Instance()->getDir().getChildFile("themes").getFullPathName();
	String lastTheme = ProtoplugDir::Instance()->getDir().getChildFile("themes/current theme.txt").loadFileAsString();
	if (lastTheme.isNotEmpty() && File::isAbsolutePath(lastTheme))
		readTheme(File(lastTheme));
	if (processor->lastUIFontSize != -1)
		editor.setFontSize(processor->lastUIFontSize);
}

void ProtoWindow::readTheme(File f)
{
	if (!f.exists())
		return;
	editor.setFont(Font(Font::getDefaultMonospacedFontName(), 14, 0));
	ScopedPointer<XmlElement> root (XmlDocument(f).getDocumentElement());
	if (!root)
		return;
    CodeEditorComponent::ColourScheme cs = tok.getDefaultColourScheme();
	StringArray sa = Font::findAllTypefaceNames();
#if JUCE_WINDOWS
	// todo: why isn't it working on OSX ?
	for (unsigned int i = 0; i < sizeof (protoFonts) / sizeof (protoFonts[0]); ++i)
		sa.add(protoFonts[i].name);
#endif
	String typeface = Font::getDefaultMonospacedFontName();
	int typefacePriority = INT_MAX;
	double height = 14;
	int style = 0;
	bool hint = false;
	int i;
	String s;
	Colour c;
	
	forEachXmlChildElement (*root, e)
	{
		if (e->hasTagName ("color"))
		{
			s = e->getStringAttribute ("target");
			c = Colour::fromString(e->getStringAttribute ("hex"));
			if (s=="Default") {
				editor.setColour(CodeEditorComponent::defaultTextColourId, c);
				cs.set("Identifier",c);
			} else if (s=="Background") {
				editor.setColour(CodeEditorComponent::backgroundColourId, c);
			} else if (s=="Highlight") {
				editor.setColour(CodeEditorComponent::highlightColourId, c);
			} else if (s=="Margintext") {
				editor.setColour(CodeEditorComponent::lineNumberTextId, c);
			} else if (s=="Marginback") {
				editor.setColour(CodeEditorComponent::lineNumberBackgroundId, c);
			} else if (s=="Caret") {
				editor.setColour(CaretComponent::caretColourId, c);
			} else
				cs.set(s,c);
		} 
		else if (e->hasTagName ("font"))
		{
			i = e->getIntAttribute("priority", INT_MAX-1);
			s = e->getStringAttribute("typeface");
			if (!sa.contains(s))
				continue;
			if (i <= typefacePriority) {
				typefacePriority = i;
				typeface = s;
				height = e->getDoubleAttribute("size", 14);
				hint = e->getBoolAttribute("hinted");
			}
		}
	}
	if (typefacePriority!=INT_MAX) { // got a usable font
		if (hint) typeface += "_hinted_";
		Font f(typeface, (float)height, style);
		editor.setFont(f);
	}
	editor.setColourScheme(cs);
	editor.setColour(ScrollBar::thumbColourId, editor.findColour(CodeEditorComponent::backgroundColourId).contrasting(0.25));

	File lastTheme = ProtoplugDir::Instance()->getDir().getChildFile("themes/current theme.txt");
    if (lastTheme.create().wasOk())
		lastTheme.replaceWithText(f.getFullPathName());
}

void ProtoWindow::readPrefs()
{
	File f = ProtoplugDir::Instance()->getDir().getChildFile("prefs.xml");
	if (!f.exists())
		return;
	XmlElement *e = XmlDocument(f).getDocumentElement();
	if (e) {
		commMgr.getKeyMappings()->restoreFromXml(*e);
		delete e;
	}
	/*	// writePrefs()
	XmlElement *e = commMgr.getKeyMappings()->createXml(false);
	e->writeToFile(ProtoplugDir::Instance()->getDir().getChildFile("prefs.xml"), String::empty);
	delete e;*/
}

ProtoWindow::~ProtoWindow()
{
	processor->lastUIFontSize = editor.getFont().getHeight();
	saveCode();
}


void ProtoWindow::paint (Graphics& g)
{
    g.fillAll (Colour(0xffe0e0e0));
}


void ProtoWindow::resized()
{
	int menuHeight = 20;
	
    Component* hcomps[] = { activePanelComponent, horizontalDividerBar, &bottomPane };
    horizontalLayout.layOutComponents (hcomps, 3,
                                        0, menuHeight, getWidth(), getHeight() - menuHeight,
                                        true,      // lay out on top of each other
                                        true);     // resize the components' widths as well as heights

    resizer.setBounds (getWidth() - 16, getHeight() - 16, 16, 16);
    menubar.setBounds (0, 0, getWidth(), menuHeight);
	bottomPane.setBounds(0, bottomPane.getY(), getWidth()-16, bottomPane.getHeight());

    processor->lastUIWidth = getWidth();
    processor->lastUIHeight = getHeight();
	processor->lastUISplit = horizontalLayout.getItemCurrentPosition (2)+menuHeight;
	int tabw = 63, sp = 3;
	tab1.setBounds(getWidth()-tabw*3-sp*2, 0, tabw, menuHeight);
	tab2.setBounds(getWidth()-tabw*2-sp*1, 0, tabw, menuHeight);
	tab3.setBounds(getWidth()-tabw*1-sp*0, 0, tabw, menuHeight);
	activePanelComponent->resized();
}


StringArray ProtoWindow::getMenuBarNames()
{
    const char* const names[] = { "File", "Edit", "Build", "View", "Help", nullptr };
    return StringArray (names);
}

void ProtoWindow::addFolderToMenu(File folder, PopupMenu &menu, String filter, int &mapCounter)
{
	Array<File> res;
	int max = folder.findChildFiles(res, File::findDirectories, 0);
	for (int i=0; i<max; i++) {
		PopupMenu sub = PopupMenu();
		addFolderToMenu(res[i], sub, filter, mapCounter);
		if (sub.getNumItems())
			menu.addSubMenu(res[i].getFileName(), sub);
	}
	res.clear();
	max = folder.findChildFiles(res, File::findFiles, 0, filter);
	for (int i=0; i<max; i++) {
		menuFiles[mapCounter] = File(res[i]);
		menu.addItem(mapCounter, res[i].getFileNameWithoutExtension());
		mapCounter++;
	}
}


PopupMenu ProtoWindow::getMenuForIndex (int menuIndex, const String& /*menuName*/)
{
    PopupMenu menu;
	int i;
	switch (menuIndex) {
	case 0: // File
		menu.addCommandItem(&commMgr, cmdOpen);
		menu.addCommandItem(&commMgr, cmdSaveAs);
		menu.addCommandItem(&commMgr, cmdOpenProto);
		menu.addSectionHeader("Scripts :");
		addFolderToMenu(File(processor->luli->libFolder), menu, "*.lua", i=5000);
		break;
	case 1: // Edit
		menu.addCommandItem(&commMgr, cmdUndo);
		menu.addCommandItem(&commMgr, cmdRedo);
		menu.addSeparator();
		menu.addCommandItem(&commMgr, cmdCut);
		menu.addCommandItem(&commMgr, cmdCopy);
		menu.addCommandItem(&commMgr, cmdPaste);
		menu.addSeparator();
		menu.addCommandItem(&commMgr, cmdFindSelected);
		menu.addCommandItem(&commMgr, cmdFindNext);
		menu.addCommandItem(&commMgr, cmdFindPrev);
		break;
	case 2: // Build
		menu.addCommandItem(&commMgr, cmdCompile);
		menu.addCommandItem(&commMgr, cmdStackDump);
		menu.addCommandItem(&commMgr, cmdLiveMode);
		break;
	case 3: // View
		menu.addCommandItem(&commMgr, cmdPopout);
		menu.addCommandItem(&commMgr, cmdAlwaysOnTop);
		menu.addSectionHeader("Themes :");
		addFolderToMenu(File(themeFolder), menu, "*.xml", i=6000);
		break;
	case 4: // Help
		menu.addCommandItem(&commMgr, cmdWebsite);
		menu.addCommandItem(&commMgr, cmdAPI);
		menu.addCommandItem(&commMgr, cmdAbout);
		break;
	}
    return menu;
}


void ProtoWindow::menuItemSelected (int menuItemID, int /*topLevelMenuIndex*/)
{
	if (menuFiles.count(menuItemID))
    {
		if (menuItemID >= 5000 && menuItemID < 6000) {			// load file from auto list
			editor.getDocument().replaceAllContent(
				menuFiles[menuItemID].loadFileAsString()
			);
			return;
		} else if (menuItemID >= 6000 && menuItemID < 7000) {	// load theme
			readTheme(menuFiles[menuItemID]);
			return;
		}
    }
}

void ProtoWindow::getAllCommands (Array <CommandID>& commands)
{
	const CommandID ids[] = {cmdCompile, cmdStackDump, cmdLiveMode, cmdFindSelected, cmdFindNext, 
							cmdFindPrev, cmdShow0, cmdShow1, cmdShow2, cmdShowNext, cmdShowPrev, 
							cmdOpen, cmdSaveAs, cmdOpenProto, cmdPopout, cmdAlwaysOnTop, cmdWebsite, 
							cmdAPI, cmdAbout, cmdUndo, cmdRedo, cmdCut, cmdCopy, cmdPaste};
	commands.addArray (ids, numElementsInArray (ids));
}

void ProtoWindow::getCommandInfo (CommandID commandID, ApplicationCommandInfo& result)
{
    const String cat ("General");
    switch (commandID)
    {
    case cmdCompile:
        result.setInfo ("Compile Now", "Compile Now", cat, 0);
		result.addDefaultKeypress (KeyPress::returnKey, ModifierKeys::commandModifier);
        break;

    case cmdStackDump:
        result.setInfo ("Stack Dump", "Stack Dump", cat, 0);
        break;

    case cmdLiveMode:
        result.setInfo ("Live Mode", "Live Mode", cat, 0);
		result.setTicked (processor->liveMode);
        break;

    case cmdShow0:
        result.setInfo ("Show Code", "Show Code", cat, 0);
        result.setTicked (activePanelComponent==&editor);
        break;

    case cmdShow1:
        result.setInfo ("Show Parameters", "Show Parameters", cat, 0);
        result.setTicked (activePanelComponent==&paramDock);
        break;

    case cmdShow2:
        result.setInfo ("Show Custom GUI", "Show Custom GUI", cat, 0);
        result.setTicked (activePanelComponent==&guiDock);
        break;

    case cmdShowNext:
        result.setInfo ("Show Next", "Show Next", cat, 0);
		result.addDefaultKeypress (KeyPress::tabKey, ModifierKeys::commandModifier);
        break;

    case cmdShowPrev:
        result.setInfo ("Show Previous", "Show Previous", cat, 0);
		result.addDefaultKeypress (KeyPress::tabKey, ModifierKeys::commandModifier | ModifierKeys::shiftModifier);
        break;

    case cmdOpen:
        result.setInfo ("Open", "Open", cat, 0);
        break;

    case cmdSaveAs:
        result.setInfo ("Save As", "Save As", cat, 0);
        break;

    case cmdOpenProto:
        result.setInfo ("Open Protoplug Folder", "Open Protoplug Folder", cat, 0);
        break;

    case cmdPopout:
        result.setInfo ("Pop Out", "Pop Out", cat, 0);
		result.setTicked (processor->popout);
        break;

    case cmdAlwaysOnTop:
        result.setInfo ("Windows On Top", "Windows Always On Top", cat, 0);
        result.setTicked (processor->alwaysontop);
		//result.setActive(processor->popout);
        break;

    case cmdWebsite:
        result.setInfo ("Website", "Website", cat, 0);
        break;

    case cmdAPI:
        result.setInfo ("API reference", "API reference", cat, 0);
        break;

    case cmdAbout:
        result.setInfo ("About Protoplug", "About Protoplug", cat, 0);
        break;

    case cmdFindSelected:
        result.setInfo ("Find Selected", "Find Selected", cat, 0);
		result.setActive(activePanelComponent==&editor && editor.isHighlightActive());
		result.addDefaultKeypress (KeyPress::F3Key, ModifierKeys::commandModifier);
        break;

    case cmdFindNext:
        result.setInfo ("Find Next", "Find Next", cat, 0);
		result.setActive(activePanelComponent==&editor && !searchTerm.isEmpty());
		result.addDefaultKeypress (KeyPress::F3Key, 0);
        break;

    case cmdFindPrev:
        result.setInfo ("Find Previous", "Find Previous", cat, 0);
		result.setActive(activePanelComponent==&editor && !searchTerm.isEmpty());
		result.addDefaultKeypress (KeyPress::F3Key, ModifierKeys::shiftModifier);
        break;
		
	case cmdUndo:
        result.setInfo ("Undo", "Undo", cat, 0);
		result.setActive(activePanelComponent==&editor && editor.getDocument().getUndoManager().canUndo());
		result.addDefaultKeypress ('z', ModifierKeys::commandModifier);
        break;
	case cmdRedo:
        result.setInfo ("Redo", "Redo", cat, 0);
		result.setActive(activePanelComponent==&editor && editor.getDocument().getUndoManager().canRedo());
		result.addDefaultKeypress ('y', ModifierKeys::commandModifier);
        break;
	case cmdCut:
        result.setInfo ("Cut", "Cut", cat, 0);
		result.setActive(activePanelComponent==&editor && editor.isHighlightActive());
		result.addDefaultKeypress ('x', ModifierKeys::commandModifier);
        break;
	case cmdCopy:
        result.setInfo ("Copy", "Copy", cat, 0);
		result.setActive(activePanelComponent==&editor && editor.isHighlightActive());
		result.addDefaultKeypress ('c', ModifierKeys::commandModifier);
        break;
	case cmdPaste:
        result.setInfo ("Paste", "Paste", cat, 0);
		result.addDefaultKeypress ('v', ModifierKeys::commandModifier);
        break;
		
    default:
        break;
    };
}

bool ProtoWindow::perform (const InvocationInfo& info)
{
	switch (info.commandID)
	{
	case cmdCompile:	compile(); break;
	case cmdStackDump:	processor->luli->stackDump(); break;
	case cmdLiveMode:	processor->liveMode = !processor->liveMode; break;
	case cmdOpenProto:	ProtoplugDir::Instance()->getDir().startAsProcess(); break;
	case cmdShow0:		setActivePanel(0); break;
	case cmdShow1:		setActivePanel(1); break;
	case cmdShow2:		setActivePanel(2); break;
	case cmdShowNext:	setActivePanel(activePanel+1); break;
	case cmdShowPrev:	setActivePanel(activePanel-1); break;
	case cmdPopout:		vstPanel->postCommandMessage(MSG_POPOUT); break;
	case cmdUndo:		editor.getDocument().undo(); break;
	case cmdRedo:		editor.getDocument().redo(); break;
	case cmdCut:		editor.cutToClipboard(); break;
	case cmdCopy:		editor.copyToClipboard(); break;
	case cmdPaste:		editor.pasteFromClipboard(); break;
	case cmdOpen: {
		FileChooser fileOpen("Open", File(processor->luli->libFolder));
		if (fileOpen.browseForFileToOpen())
			editor.getDocument().replaceAllContent(
				fileOpen.getResult().loadFileAsString()
			);
		break;
	}
	case cmdSaveAs: {
		FileChooser fileSave("Save", File(processor->luli->libFolder), "*.lua");
		if (fileSave.browseForFileToSave(1))
			fileSave.getResult().replaceWithText(
				editor.getDocument().getAllContent()
			);
		break;
	}
	case cmdAlwaysOnTop:
		guiDock.setAlwaysOnTop(!processor->alwaysontop);
		paramDock.setAlwaysOnTop(!processor->alwaysontop);
		vstPanel->postCommandMessage(MSG_ALWAYSONTOP);
		break;
	case cmdWebsite: {
		URL url("http://www.osar.fr/protoplug");
		url.launchInDefaultBrowser();
		break;
	}
	case cmdAPI: {
		URL url("file:///"+ProtoplugDir::Instance()->getDir().getChildFile("doc/index.html").getFullPathName());
		url.launchInDefaultBrowser();
		break;
	}
	case cmdAbout: {
			AboutBox::launch(processor, &editor);
			break;
	}
	case cmdFindSelected: {
		Range<int> range;
		if (editor.hasKeyboardFocus(true))
		{
			range = editor.getHighlightedRegion();
			if (range.isEmpty())
				break;
			searchTerm = editor.getTextInRange(range);
			findNext(1);
		}
		break;
	}
	case cmdFindNext:
		if (editor.hasKeyboardFocus(true))
			findNext(1);
		break;
	case cmdFindPrev:
		if (editor.hasKeyboardFocus(true))
			findNext(0);
		break;
	default: return false;
	}
	return true;
}

void ProtoWindow::tabButtonClicked (ProtoTabButton *b)
{
	if (b==&tab1)
		setActivePanel(0);
	else if (b==&tab2)
		if (paramDock.isPoppedOut())
			paramDock.bringWindowToFront();
		else
			setActivePanel(1);
	else if (b==&tab3)
		if (guiDock.isPoppedOut())
			guiDock.bringWindowToFront();
		else
			setActivePanel(2);
}

void ProtoWindow::tabButtonDoubleClicked (ProtoTabButton *b)
{
	if (b==&tab1)
		vstPanel->postCommandMessage(MSG_POPOUT);
	else if (b==&tab2) {
		if (paramDock.isPoppedOut())
			setActivePanel(1);
		else
			setActivePanel(0);
		paramDock.postCommandMessage(1);
	} else if (b==&tab3) {
		if (guiDock.isPoppedOut())
			setActivePanel(2);
		else
			setActivePanel(0);
		guiDock.postCommandMessage(1);
	}
}

void ProtoWindow::codeDocumentTextInserted (const String &newText, int insertIndex)
{ bottomPane.setCompileVisible(true); }

void ProtoWindow::codeDocumentTextDeleted (int startIndex, int endIndex)
{ bottomPane.setCompileVisible(true); }

void ProtoWindow::findNext(bool direction, bool wrap /*= false*/)
{ editor.findNext(searchTerm, direction, wrap); }

void ProtoWindow::saveCode()
{
	processor->luli->code = editor.getDocument().getAllContent();
}

void ProtoWindow::compile()
{
	saveCode();
	processor->luli->compile();

	processor->sendChangeMessage();
	bottomPane.setCompileVisible(false);
}

void ProtoWindow::setActivePanel(int p)
{
	if (p>2) p = 0;
	if (p<0) p = 2;
	if (p==1 && paramDock.isPoppedOut()) p = 2;
	if (p==2 && guiDock.isPoppedOut()) p = 0;
	activePanelComponent->setVisible (false);
	activePanelComponent = panels[p];
	activePanel = p;
	activePanelComponent->setVisible (true);
	if (p==0) {
		takeFocus();
	}
	else if (p==1) {
		paramPanel.paramsChanged();
		paramPanel.updateNames();
	}
	else if (p==2) {
		guiPanel.repaint();
	}
	resized();
	bottomPane.scrollLog();
	processor->lastUIPanel = activePanel;
}

int ProtoWindow::getActivePanel()
{
	return activePanel;
}

void ProtoWindow::setPoppedOut(bool popped)
{
	if (popped)
		resizer.setVisible(false);
	else
		resizer.setVisible(true);
}

void ProtoWindow::timerCallback()
{
	if (msg_UpdateLog) {
		msg_UpdateLog = 0;
		bottomPane.updateLog();
	}
	if (msg_ParamsChanged) {
		msg_ParamsChanged = 0;
		//if (activePanel!=1) return;
		paramPanel.paramsChanged();
	}
	if (msg_TakeFocus) {
		msg_TakeFocus = 0;
		editor.grabKeyboardFocus();
	}
	if (processor->liveMode && editor.somethingChanged) {
		editor.somethingChanged = false;
		compile();
	}
	// pestilentially ugly hack to fix the misplaced menu popups.
	// the problem appeared between Jan and Sept '14 versions of JUCE
	#if JUCE_LINUX
	if (hackTimer++>10)
	{
		hackTimer = 0;
		Component *pc = processor->getProtoEditor()->getParentComponent();
		if (pc) {
			Rectangle<int> pcrect = pc->getBounds();
			pc->setBounds(pcrect.withWidth(pcrect.getWidth()+1));
			pc->setBounds(pcrect);
		}
	}
	#endif
}

void ProtoWindow::takeFocus()
{
	msg_TakeFocus = true;
}
