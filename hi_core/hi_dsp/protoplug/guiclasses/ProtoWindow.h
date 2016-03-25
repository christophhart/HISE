#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../LuaCodeTokeniser.h"
#include "HintedFeel.h"
#include "DarkSplitter.h"
#include "ParameterPanel.h"
#include "CustomGuiPanel.h"
#include "LuaEditor.h"
#include "Dockable.h"
#include "BottomPane.h"
#include <map>

#define MSG_POPOUT 1
#define MSG_ALWAYSONTOP 2

class LuaProtoplugJuceAudioProcessorEditor;
class ProtoWindow;

class ProtoCmd : public ApplicationCommandManager
{
public:
	ProtoCmd (ProtoWindow *_pw)
	{ pw = _pw; }
	ApplicationCommandTarget *getFirstCommandTarget (CommandID commandID);
	ProtoWindow *pw;
};

class ProtoWindow	:	public Component,
						public MenuBarModel,
						public Timer,
						public ApplicationCommandTarget,
						public ProtoTabButton::Listener,
						public CodeDocument::Listener
{
	friend class BottomPane;
public:
	ProtoWindow (Component *parent, LuaProtoplugJuceAudioProcessor* ownerFilter);
	~ProtoWindow();

	void paint (Graphics& g);
	void resized();
    PopupMenu getMenuForIndex (int menuIndex, const String& /*menuName*/);
    void menuItemSelected (int menuItemID, int /*topLevelMenuIndex*/);
    void timerCallback();
    StringArray getMenuBarNames();
	ApplicationCommandTarget * 	getNextCommandTarget () { return 0; }
	void getAllCommands (Array <CommandID>& commands);
	void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result);
	bool perform (const InvocationInfo& info);
	void tabButtonClicked (ProtoTabButton *b);
	void tabButtonDoubleClicked (ProtoTabButton *b);
	void codeDocumentTextInserted (const String &newText, int insertIndex);
	void codeDocumentTextDeleted (int startIndex, int endIndex);
	void initProtoplugDir();

	void saveCode();
	void compile();
	void setActivePanel(int p);
	int getActivePanel();
	void setPoppedOut(bool popped);
	void takeFocus();
	void readPrefs();
	void readTheme(File theme);
	bool dirty;
	bool msg_UpdateLog, msg_ParamsChanged, msg_TakeFocus; // notifications for the timer
	
    enum CommandIDs
    {
		cmdCompile		= 0x10,
		cmdStackDump	= 0x11,
		cmdLiveMode		= 0x12,
		cmdFindSelected = 0x25,
		cmdFindNext		= 0x26,
		cmdFindPrev		= 0x27,
		cmdShow0		= 0x30,
		cmdShow1		= 0x31,
		cmdShow2		= 0x32,
		cmdShowNext		= 0x40,
		cmdShowPrev		= 0x41,
		cmdOpen			= 0x50,
		cmdSaveAs		= 0x51,
		cmdOpenProto	= 0x52,
		cmdPopout		= 0x60,
		cmdAlwaysOnTop	= 0x61,
		cmdWebsite		= 0x70,
		cmdAPI			= 0x71,
		cmdAbout		= 0x72,
		cmdUndo			= StandardApplicationCommandIDs::undo,
		cmdRedo			= StandardApplicationCommandIDs::redo,
		cmdCut			= StandardApplicationCommandIDs::cut,
		cmdCopy			= StandardApplicationCommandIDs::copy,
		cmdPaste		= StandardApplicationCommandIDs::paste,
    };

private:
	void findNext(bool direction, bool wrap = false);
	void addFolderToMenu(File folder, PopupMenu &menu, String filter, int &mapCounter);

    HintedFeel newFeel;
	ProtoCmd commMgr;
    LuaProtoplugJuceAudioProcessor *processor;
	ResizableCornerComponent resizer;
	ComponentBoundsConstrainer resizeLimits;
    StretchableLayoutManager horizontalLayout;
    ScopedPointer<DarkSplitter> horizontalDividerBar;
	MenuBarComponent menubar;
	std::map<int, File> menuFiles;
	CodeDocument doc;
	ProtoLuaTokeniser tok;
	String themeFolder;
	LuaEditor editor;
	BottomPane bottomPane;
	ParameterPanel paramPanel;
	CustomGuiPanel guiPanel;
	int activePanel;
	Component *panels[3];
	Component *activePanelComponent;
	Component *vstPanel, *popoutWindow;
	Dockable paramDock, guiDock;
	String searchTerm;
	ProtoTabButton tab1, tab2, tab3;
	int hackTimer;

	AlertWindowLookAndFeel alaf;
};

