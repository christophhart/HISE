/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#pragma once

#include "../Utility/PIPs/jucer_PIPGenerator.h"
#include "../Project/jucer_Project.h"
#include "../CodeEditor/jucer_OpenDocumentManager.h"

class ProjectContentComponent;

//==============================================================================
/**
    The big top-level window where everything happens.
*/
class MainWindow  : public DocumentWindow,
                    public ApplicationCommandTarget,
                    public FileDragAndDropTarget,
                    public DragAndDropContainer,
                    private Value::Listener
{
public:
    //==============================================================================
    MainWindow();
    ~MainWindow() override;

    enum class OpenInIDE { no, yes };

    //==============================================================================
    void closeButtonPressed() override;

    //==============================================================================
    bool canOpenFile (const File& file) const;
    bool openFile (const File& file);

    void setProject (std::unique_ptr<Project> newProject);
    Project* getProject() const  { return currentProject.get(); }

    void makeVisible();
    void restoreWindowPosition();
    void updateTitleBarIcon();
    bool closeCurrentProject (OpenDocumentManager::SaveIfNeeded askToSave);
    void moveProject (File newProjectFile, OpenInIDE openInIDE);

    void showStartPage();

    void showLoginFormOverlay();
    void hideLoginFormOverlay();
    bool isShowingLoginForm() const noexcept  { return loginFormOpen; }

    bool isInterestedInFileDrag (const StringArray& files) override;
    void filesDropped (const StringArray& filenames, int mouseX, int mouseY) override;

    void activeWindowStatusChanged() override;

    ProjectContentComponent* getProjectContentComponent() const;

    //==============================================================================
    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands (Array <CommandID>& commands) override;
    void getCommandInfo (CommandID commandID, ApplicationCommandInfo& result) override;
    bool perform (const InvocationInfo& info) override;

    bool shouldDropFilesWhenDraggedExternally (const DragAndDropTarget::SourceDetails& sourceDetails,
                                               StringArray& files, bool& canMoveFiles) override;
private:
    void valueChanged (Value&) override;

    static const char* getProjectWindowPosName()   { return "projectWindowPos"; }
    void createProjectContentCompIfNeeded();

    bool openPIP (PIPGenerator);
    void setupTemporaryPIPProject (PIPGenerator&);

    void initialiseProjectWindow();

    std::unique_ptr<Project> currentProject;
    Value projectNameValue;

    std::unique_ptr<Component> blurOverlayComponent;
    bool loginFormOpen = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
};

//==============================================================================
class MainWindowList
{
public:
    MainWindowList();

    void forceCloseAllWindows();
    bool askAllWindowsToClose();
    void closeWindow (MainWindow*);

    void goToSiblingWindow (MainWindow*, int delta);

    void createWindowIfNoneAreOpen();
    void openDocument (OpenDocumentManager::Document*, bool grabFocus);
    bool openFile (const File& file, bool openInBackground = false);

    MainWindow* createNewMainWindow();
    MainWindow* getFrontmostWindow (bool createIfNotFound = true);
    MainWindow* getOrCreateEmptyWindow();
    MainWindow* getMainWindowForFile (const File&);
    MainWindow* getMainWindowWithLoginFormOpen();

    Project* getFrontmostProject();

    void reopenLastProjects();
    void saveCurrentlyOpenProjectList();

    void checkWindowBounds (MainWindow&);

    void sendLookAndFeelChange();

    OwnedArray<MainWindow> windows;

private:
    bool isInReopenLastProjects = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindowList)
};
