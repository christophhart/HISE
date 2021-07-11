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


//==============================================================================
class ModuleItem   : public ProjectTreeItemBase
{
public:
    ModuleItem (Project& p, const String& modID)
        : project (p), moduleID (modID)
    {
        missingDependencies = project.getEnabledModules().getExtraDependenciesNeeded (moduleID).size() > 0;
        cppStandardHigherThanProject = project.getEnabledModules().doesModuleHaveHigherCppStandardThanProject (moduleID);

        moduleInfo = project.getEnabledModules().getModuleInfo (moduleID);
    }

    bool canBeSelected() const override       { return true; }
    bool mightContainSubItems() override      { return false; }
    String getUniqueName() const override     { return "module_" + moduleID; }
    String getDisplayName() const override    { return moduleID; }
    String getRenamingName() const override   { return getDisplayName(); }
    void setName (const String&) override     {}
    bool isMissing() const override           { return missingDependencies; }
    bool hasWarnings() const override         { return cppStandardHigherThanProject; }

    void showDocument() override
    {
        showSettingsPage (new ModuleSettingsPanel (project, moduleID, getOwnerView()));
    }

    void deleteItem() override
    {
        closeSettingsPage();
        project.getEnabledModules().removeModule (moduleID);
    }

    Icon getIcon() const override
    {
        auto iconColour = getOwnerView()->findColour (isSelected() ? defaultHighlightedTextColourId
                                                                   : treeIconColourId);

        if (! isSelected())
        {
            if (moduleInfo.isValid() && moduleInfo.getVendor() == "juce")
            {
                if (moduleInfo.getLicense() == "ISC")
                    iconColour = Colours::lightblue;
                else if (moduleInfo.getLicense() == "GPL/Commercial")
                    iconColour = Colours::orange;
            }
        }

        return Icon (getIcons().singleModule, iconColour);
    }

    void showAddMenu() override
    {
        if (auto* parent = dynamic_cast<EnabledModulesItem*> (getParentItem()))
            parent->showPopupMenu();
    }

    void showPopupMenu() override
    {
        PopupMenu menu;
        menu.addItem (1, "Remove this module");
        launchPopupMenu (menu);
    }

    void handlePopupMenuResult (int resultCode) override
    {
        if (resultCode == 1)
            deleteItem();
    }

    bool checkCppStandard()
    {
        auto oldVal = cppStandardHigherThanProject;
        cppStandardHigherThanProject = project.getEnabledModules().doesModuleHaveHigherCppStandardThanProject (moduleID);

        if (oldVal != cppStandardHigherThanProject)
            return true;

        return false;
    }

    Project& project;
    String moduleID;

private:
    ModuleDescription moduleInfo;
    bool missingDependencies = false;
    bool cppStandardHigherThanProject = false;

    //==============================================================================
    class ModuleSettingsPanel  : public Component,
                                 private Value::Listener,
                                 private Timer
    {
    public:
        ModuleSettingsPanel (Project& p, const String& modID, TreeView* tree)
            : group (p.getEnabledModules().getModuleInfo (modID).getID(),
                     Icon (getIcons().singleModule, Colours::transparentBlack)),
              project (p),
              modulesTree (tree),
              moduleID (modID)
        {
            addAndMakeVisible (group);
            refresh();
        }

        void refresh()
        {
            auto& modules = project.getEnabledModules();

            setEnabled (modules.isModuleEnabled (moduleID));

            PropertyListBuilder props;

            props.add (new ModuleInfoComponent (project, moduleID));

            if (modules.getExtraDependenciesNeeded (moduleID).size() > 0)
                props.add (new MissingDependenciesComponent (project, moduleID));

            if (modules.doesModuleHaveHigherCppStandardThanProject (moduleID))
                props.add (new CppStandardWarningComponent());

            group.properties.clear();
            exporterModulePathDefaultValues.clear();
            exporterModulePathValues.clear();
            globalPathValues.clear();

            for (Project::ExporterIterator exporter (project); exporter.next();)
            {
                if (exporter->isCLion())
                    continue;

                exporterModulePathDefaultValues.add (exporter->getPathForModuleValue (moduleID));
                auto& defaultValue = exporterModulePathDefaultValues.getReference (exporterModulePathDefaultValues.size() - 1);

                exporterModulePathValues.add (defaultValue.getPropertyAsValue());

                auto pathComponent = std::make_unique<FilePathPropertyComponent> (defaultValue,
                                                                                  "Path for " + exporter->getUniqueName().quoted(),
                                                                                  true,
                                                                                  exporter->getTargetOSForExporter() == TargetOS::getThisOS(),
                                                                                  "*",
                                                                                  project.getProjectFolder());

                pathComponent->setEnabled (! modules.shouldUseGlobalPath (moduleID));

                props.add (pathComponent.release(),
                           "A path to the folder that contains the " + moduleID + " module when compiling the "
                           + exporter->getUniqueName().quoted() + " target. "
                           "This can be an absolute path, or relative to the jucer project folder, but it "
                           "must be valid on the filesystem of the target machine that will be performing this build. If this "
                           "is empty then the global path will be used.");

                globalPathValues.add (getAppSettings().getStoredPath (isJUCEModule (moduleID) ? Ids::defaultJuceModulePath : Ids::defaultUserModulePath,
                                                                      exporter->getTargetOSForExporter()).getPropertyAsValue());
            }

            for (int i = 0; i < exporterModulePathDefaultValues.size(); ++i)
            {
                exporterModulePathDefaultValues.getReference (i).onDefaultChange = [this] { startTimer (50); };

                exporterModulePathValues.getReference (i).addListener (this);
                globalPathValues.getReference (i).addListener (this);
            }

            useGlobalPathValue.removeListener (this);
            useGlobalPathValue.referTo (modules.shouldUseGlobalPathValue (moduleID));
            useGlobalPathValue.addListener (this);

            auto menuItemString = (TargetOS::getThisOS() == TargetOS::osx ? "\"Projucer->Global Paths...\""
                                                                          : "\"File->Global Paths...\"");

            props.add (new BooleanPropertyComponent (useGlobalPathValue,
                                                     "Use global path", "Use global path for this module"),
                       String ("If this is enabled, then the locally-stored global path (set in the ") + menuItemString + " menu item) "
                       "will be used as the path to this module. "
                       "This means that if this Projucer project is opened on another machine it will use that machine's global path as the path to this module.");

            props.add (new BooleanPropertyComponent (modules.shouldCopyModuleFilesLocallyValue (moduleID),
                                                     "Create local copy", "Copy the module into the project folder"),
                       "If this is enabled, then a local copy of the entire module will be made inside your project (in the auto-generated JuceLibraryFiles folder), "
                       "so that your project will be self-contained, and won't need to contain any references to files in other folders. "
                       "This also means that you can check the module into your source-control system to make sure it is always in sync with your own code.");

            props.add (new BooleanPropertyComponent (modules.shouldShowAllModuleFilesInProjectValue (moduleID),
                                                     "Add source to project", "Make module files browsable in projects"),
                       "If this is enabled, then the entire source tree from this module will be shown inside your project, "
                       "making it easy to browse/edit the module's classes. If disabled, then only the minimum number of files "
                       "required to compile it will appear inside your project.");

            auto info = modules.getModuleInfo (moduleID);

            if (info.isValid())
            {
                configFlags.clear();
                LibraryModule (info).getConfigFlags (project, configFlags);

                for (auto* flag : configFlags)
                {
                    auto* c = new ChoicePropertyComponent (flag->value, flag->symbol);
                    c->setTooltip (flag->description);
                    props.add (c);
                }
            }

            group.setProperties (props);
            parentSizeChanged();
        }

        void parentSizeChanged() override      { updateSize (*this, group); }
        void resized() override                { group.setBounds (getLocalBounds().withTrimmedLeft (12)); }

        String getModuleID() const noexcept    { return moduleID; }

    private:
        void valueChanged (Value& v) override
        {
            auto isExporterPathValue = [&]
            {
                for (auto& exporterValue : exporterModulePathValues)
                    if (exporterValue.refersToSameSourceAs (v))
                        return true;

                return false;
            }();

            if (isExporterPathValue)
                project.rescanExporterPathModules();

            startTimer (50);
        }

        void timerCallback() override
        {
            stopTimer();
            refresh();
        }

        //==============================================================================
        Array<ValueWithDefault> exporterModulePathDefaultValues;
        Array<Value> exporterModulePathValues, globalPathValues;
        Value useGlobalPathValue;

        OwnedArray <Project::ConfigFlag> configFlags;

        PropertyGroupComponent group;
        Project& project;
        SafePointer<TreeView> modulesTree;
        String moduleID;

        //==============================================================================
        class ModuleInfoComponent  : public PropertyComponent,
                                     private Value::Listener
        {
        public:
            ModuleInfoComponent (Project& p, const String& modID)
                : PropertyComponent ("Module", 150), project (p), moduleID (modID)
            {
                for (Project::ExporterIterator exporter (project); exporter.next();)
                    listeningValues.add (new Value (exporter->getPathForModuleValue (moduleID).getPropertyAsValue()))->addListener (this);

                refresh();
            }

            void refresh() override
            {
                info = project.getEnabledModules().getModuleInfo (moduleID);
                repaint();
            }

        private:
            void paint (Graphics& g) override
            {
                auto bounds = getLocalBounds().reduced (10);
                bounds.removeFromTop (5);

                if (info.isValid())
                {
                    auto topSlice = bounds.removeFromTop (bounds.getHeight() / 2);
                    bounds.removeFromTop (bounds.getHeight() / 6);
                    auto bottomSlice = bounds;

                    g.setColour (findColour (defaultTextColourId));

                    g.drawFittedText (info.getName(),                   topSlice.removeFromTop (topSlice.getHeight() / 4), Justification::centredLeft, 1);
                    g.drawFittedText ("Version: "  + info.getVersion(), topSlice.removeFromTop (topSlice.getHeight() / 3), Justification::centredLeft, 1);
                    g.drawFittedText ("License: "  + info.getLicense(), topSlice.removeFromTop (topSlice.getHeight() / 2), Justification::centredLeft, 1);
                    g.drawFittedText ("Location: " + info.getFolder().getParentDirectory().getFullPathName(),
                                      topSlice.removeFromTop (topSlice.getHeight()), Justification::centredLeft, 1);

                    g.drawFittedText (info.getDescription(), bottomSlice, Justification::topLeft, 3, 1.0f);
                }
                else
                {
                    g.setColour (Colours::red);
                    g.drawFittedText ("Cannot find this module at the specified path!", bounds, Justification::centred, 1);
                }
            }

            void valueChanged (Value&) override
            {
                refresh();
            }

            Project& project;
            String moduleID;
            OwnedArray<Value> listeningValues;
            ModuleDescription info;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleInfoComponent)
        };

        //==============================================================================
        class MissingDependenciesComponent  : public PropertyComponent
        {
        public:
            MissingDependenciesComponent (Project& p, const String& modID)
                : PropertyComponent ("Dependencies", 100),
                  project (p), moduleID (modID),
                  missingDependencies (project.getEnabledModules().getExtraDependenciesNeeded (modID))
            {
                addAndMakeVisible (fixButton);
                fixButton.setColour (TextButton::buttonColourId, Colours::red);
                fixButton.setColour (TextButton::textColourOffId, Colours::white);
                fixButton.onClick = [this] { fixDependencies(); };
            }

            void refresh() override {}

            void paint (Graphics& g) override
            {
                String text ("This module has missing dependencies!\n\n"
                             "To build correctly, it requires the following modules to be added:\n");
                text << missingDependencies.joinIntoString (", ");

                g.setColour (Colours::red);
                g.drawFittedText (text, getLocalBounds().reduced (10), Justification::topLeft, 3);
            }

            void fixDependencies()
            {
                auto& enabledModules = project.getEnabledModules();

                if (enabledModules.tryToFixMissingDependencies (moduleID))
                {
                    missingDependencies.clear();
                }
                else
                {
                    missingDependencies = enabledModules.getExtraDependenciesNeeded (moduleID);

                    AlertWindow::showMessageBoxAsync (AlertWindow::WarningIcon,
                                                      "Adding Missing Dependencies",
                                                      "Couldn't locate some of these modules - you'll need to find their "
                                                      "folders manually and add them to the list.");
                }
            }

            void resized() override
            {
                fixButton.setBounds (getWidth() - 168, getHeight() - 26, 160, 22);
            }

        private:
            Project& project;

            String moduleID;
            StringArray missingDependencies;
            TextButton fixButton { "Add Required Modules" };

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MissingDependenciesComponent)
        };

        //==============================================================================
        struct CppStandardWarningComponent    : public PropertyComponent
        {
            CppStandardWarningComponent()
                : PropertyComponent ("CppStandard", 100)
            {
            }

            void refresh() override {}

            void paint (Graphics& g) override
            {
                auto text = String ("This module has a higher C++ language standard requirement than your project!\n\n"
                                    "To use this module you need to increase the C++ standard of the project.\n");

                g.setColour (findColour (defaultHighlightColourId));
                g.drawFittedText (text, getLocalBounds().reduced (10), Justification::topLeft, 3);
            }

            StringArray configsToWarnAbout;

            JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CppStandardWarningComponent)
        };
    };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ModuleItem)
};

//==============================================================================
class EnabledModulesItem   : public ProjectTreeItemBase,
                             private Value::Listener,
                             private AvailableModulesList::Listener
{
public:
    EnabledModulesItem (Project& p)
        : project (p),
          modulesListTree (project.getEnabledModules().getState())
    {
        modulesListTree.addListener (this);

        projectCppStandardValue.referTo (project.getProjectValue (Ids::cppLanguageStandard));
        projectCppStandardValue.addListener (this);

        ProjucerApplication::getApp().getJUCEPathModulesList().addListener (this);
        ProjucerApplication::getApp().getUserPathsModulesList().addListener (this);

        project.getExporterPathsModulesList().addListener (this);
    }

    ~EnabledModulesItem() override
    {
        ProjucerApplication::getApp().getJUCEPathModulesList().removeListener (this);
        ProjucerApplication::getApp().getUserPathsModulesList().removeListener (this);

        project.getExporterPathsModulesList().removeListener (this);
    }

    int getItemHeight() const override      { return 22; }
    bool isModulesList() const override     { return true; }
    bool canBeSelected() const override     { return true; }
    bool mightContainSubItems() override    { return true; }
    String getUniqueName() const override   { return "modules"; }
    String getRenamingName() const override { return getDisplayName(); }
    String getDisplayName() const override  { return "Modules"; }
    void setName (const String&) override   {}
    bool isMissing() const override         { return false; }
    Icon getIcon() const override           { return Icon (getIcons().graph, getContentColour (true)); }

    void showDocument() override
    {
        if (auto* pcc = getProjectContentComponent())
            pcc->setEditorComponent (new ModulesInformationComponent (project), nullptr);
    }

    static File getModuleFolder (const File& draggedFile)
    {
        if (draggedFile.hasFileExtension (headerFileExtensions))
            return draggedFile.getParentDirectory();

        return draggedFile;
    }

    bool isInterestedInFileDrag (const StringArray& files) override
    {
        for (auto i = files.size(); --i >= 0;)
            if (ModuleDescription (getModuleFolder (files[i])).isValid())
                return true;

        return false;
    }

    void filesDropped (const StringArray& files, int /*insertIndex*/) override
    {
        Array<ModuleDescription> modules;

        for (auto f : files)
        {
            ModuleDescription m (getModuleFolder (f));

            if (m.isValid())
                modules.add (m);
        }

        for (int i = 0; i < modules.size(); ++i)
            project.getEnabledModules().addModule (modules.getReference (i).getModuleFolder(),
                                                   project.getEnabledModules().areMostModulesCopiedLocally(),
                                                   project.getEnabledModules().areMostModulesUsingGlobalPath());
    }

    void addSubItems() override
    {
        for (int i = 0; i < project.getEnabledModules().getNumModules(); ++i)
            addSubItem (new ModuleItem (project, project.getEnabledModules().getModuleID (i)));
    }

    void showPopupMenu() override
    {
        auto& enabledModules = project.getEnabledModules();
        PopupMenu allModules;

        int index = 100;

        // JUCE path
        PopupMenu jucePathModules;

        for (auto& mod : ProjucerApplication::getApp().getJUCEPathModulesList().getAllModules())
            jucePathModules.addItem (index++, mod.first, ! enabledModules.isModuleEnabled (mod.first));

        jucePathModules.addSeparator();
        jucePathModules.addItem (-1, "Re-scan path");

        allModules.addSubMenu ("Global JUCE modules path", jucePathModules);

        // User path
        index = 200;
        PopupMenu userPathModules;

        for (auto& mod : ProjucerApplication::getApp().getUserPathsModulesList().getAllModules())
            userPathModules.addItem (index++, mod.first, ! enabledModules.isModuleEnabled (mod.first));

        userPathModules.addSeparator();
        userPathModules.addItem (-2, "Re-scan path");

        allModules.addSubMenu ("Global user modules path", userPathModules);

        // Exporter path
        index = 300;
        PopupMenu exporterPathModules;

        for (auto& mod : project.getExporterPathsModulesList().getAllModules())
            exporterPathModules.addItem (index++, mod.first, ! enabledModules.isModuleEnabled (mod.first));

        exporterPathModules.addSeparator();
        exporterPathModules.addItem (-3, "Re-scan path");

        allModules.addSubMenu ("Exporter paths", exporterPathModules);

        PopupMenu menu;
        menu.addSubMenu ("Add a module", allModules);

        menu.addSeparator();
        menu.addItem (1001, "Add a module from a specified folder...");

        launchPopupMenu (menu);
    }

    void handlePopupMenuResult (int resultCode) override
    {
        if (resultCode == 1001)
        {
            project.getEnabledModules().addModuleFromUserSelectedFile();
        }
        else if (resultCode < 0)
        {
            if      (resultCode == -1)  ProjucerApplication::getApp().rescanJUCEPathModules();
            else if (resultCode == -2)  ProjucerApplication::getApp().rescanUserPathModules();
            else if (resultCode == -3)  project.rescanExporterPathModules();
        }
        else if (resultCode > 0)
        {
            std::vector<AvailableModulesList::ModuleIDAndFolder> list;
            int offset = -1;

            if (resultCode < 200)
            {
                list = ProjucerApplication::getApp().getJUCEPathModulesList().getAllModules();
                offset = 100;
            }
            else if (resultCode < 300)
            {
                list = ProjucerApplication::getApp().getUserPathsModulesList().getAllModules();
                offset = 200;
            }
            else if (resultCode < 400)
            {
                list = project.getExporterPathsModulesList().getAllModules();
                offset = 300;
            }

            if (offset != -1)
                project.getEnabledModules().addModuleInteractive (list[(size_t) (resultCode - offset)].first);
        }
    }

    //==============================================================================
    void valueTreeChildAdded (ValueTree& parentTree, ValueTree&) override         { refreshIfNeeded (parentTree); }
    void valueTreeChildRemoved (ValueTree& parentTree, ValueTree&, int) override  { refreshIfNeeded (parentTree); }
    void valueTreeChildOrderChanged (ValueTree& parentTree, int, int) override    { refreshIfNeeded (parentTree); }

    void refreshIfNeeded (ValueTree& changedTree)
    {
        if (changedTree == modulesListTree)
        {
            auto selectedID = getSelectedItemID();

            refreshSubItems();

            if (selectedID.isNotEmpty())
                setSelectedItem (selectedID);
        }
    }

private:
    Project& project;
    ValueTree modulesListTree;
    Value projectCppStandardValue;

    //==============================================================================
    void valueChanged (Value& v) override
    {
        if (v == projectCppStandardValue)
        {
            for (int i = 0; i < getNumSubItems(); ++i)
            {
                if (auto* moduleItem = dynamic_cast<ModuleItem*> (getSubItem (i)))
                {
                    if (moduleItem->checkCppStandard())
                    {
                        refreshSubItems();
                        return;
                    }
                }
            }
        }
    }

    void removeDuplicateModules()
    {
        auto jucePathModulesList = ProjucerApplication::getApp().getJUCEPathModulesList().getAllModules();

        auto& userPathModules = ProjucerApplication::getApp().getUserPathsModulesList();
        userPathModules.removeDuplicates (jucePathModulesList);

        auto& exporterPathModules = project.getExporterPathsModulesList();
        exporterPathModules.removeDuplicates (jucePathModulesList);
        exporterPathModules.removeDuplicates (userPathModules.getAllModules());
    }

    void availableModulesChanged (AvailableModulesList*) override
    {
        removeDuplicateModules();
        refreshSubItems();
    }

    String getSelectedItemID() const
    {
        for (int i = 0; i < getNumSubItems(); ++i)
            if (auto* item = getSubItem (i))
                if (item->isSelected())
                    return item->getUniqueName();

        return {};
    }

    void setSelectedItem (const String& itemID)
    {
        for (int i = 0; i < getNumSubItems(); ++i)
        {
            if (auto* item = getSubItem (i))
            {
                if (item->getUniqueName() == itemID)
                {
                    item->setSelected (true, true);
                    return;
                }
            }
        }
    }

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EnabledModulesItem)
};
