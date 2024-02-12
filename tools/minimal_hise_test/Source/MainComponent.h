/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "multipage.h"
#include "DialogLibrary.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public Component,
					    public Timer,
					    public MenuBarModel,
					    public CodeDocument::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    bool manualChange = false;

    File getSettingsFile() const
    {
        auto f = File::getSpecialLocation(File::SpecialLocationType::userApplicationDataDirectory);
        
#if JUCE_MAC
        f = f.getChildFile("Application Support");
#endif
        
        return f.getChildFile("HISE").getChildFile("multipage.json");
    }

    void codeDocumentTextInserted (const String& , int ) override
    {
	    manualChange = true;
    }

    
    void codeDocumentTextDeleted (int , int ) override
    {
        manualChange = true;
    }

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override;;

    void setSavePoint();

    void checkSave()
    {
	    if(modified)
	    {
		    if(currentFile.existsAsFile() && AlertWindow::showOkCancelBox(MessageBoxIconType::QuestionIcon, "Save changes", "Do you want to save the changes"))
	        {
		        currentFile.replaceWithText(JSON::toString(c->exportAsJSON()));
                setSavePoint();
	        }
	    }
    }

    void build();

    /** This method must return a list of the names of the menus. */
    StringArray getMenuBarNames()
    {
	    return { "File", "Edit", "View", "Help" };
    }

    enum CommandId
    {
	    FileNew = 1,
        FileLoad,
        FileSave,
        FileSaveAs,
        FileQuit,
        EditUndo,
        EditRedo,
        EditToggleMode,
        EditRefreshPage,
        EditAddPage,
        ViewShowDialog,
        ViewShowJSON,
        ViewShowCpp,
        HelpAbout,
        HelpVersion,
        FileRecentOffset = 9000
    };
    
	PopupMenu getMenuForIndex (int topLevelMenuIndex, const String&) override;

    bool keyPressed(const KeyPress& key) override;

    void menuItemSelected (int menuItemID, int) override;

private:

    int64 prevHash = 0;
    bool modified = false;
    bool firstAfterSave = false;

    File currentFile;

    juce::RecentlyOpenedFilesList fileList;

    void createDialog(const File& f);

    //==============================================================================
    // Your private member variables go here...
    int counter = 0;
	OpenGLContext context;

    multipage::State rt;
    ScopedPointer<multipage::Dialog> c;

    

    juce::CodeDocument doc;
    mcl::TextDocument stateDoc;
    mcl::TextEditor stateViewer;
    
    AlertWindowLookAndFeel plaf;
    MenuBarComponent menuBar;

    ScopedPointer<multipage::library::HardcodedDialogWithState> hardcodedDialog;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};


