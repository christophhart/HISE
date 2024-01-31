/*  ===========================================================================
 *
 *   This file is part of HISE.
 *   Copyright 2016 Christoph Hart
 *
 *   HISE is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   HISE is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with HISE.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Commercial licenses for using HISE in an closed source project are
 *   available on request. Please visit the project's website to get more
 *   information about commercial licensing:
 *
 *   http://www.hise.audio/
 *
 *   HISE is based on the JUCE library,
 *   which must be separately licensed for closed source applications:
 *
 *   http://www.juce.com
 *
 *   ===========================================================================
 */

#pragma once


namespace hise {
namespace multipage {
namespace factory {
using namespace juce;

struct LabelledComponent: public Dialog::PageBase
{
    LabelledComponent(Dialog& r, int width, const var& obj, Component* c);;

    virtual Result loadFromInfoObject(const var& obj)
    {
	    label = obj[mpid::Text];
        return Result::ok();
    }
    
    void postInit() override;
    void paint(Graphics& g) override;
    void resized() override;

    static String getCategoryId() { return "UI Elements"; }
    

    void editModeChanged(bool isEditMode) override;

    template <typename T> T& getComponent() { return *dynamic_cast<T*>(component.get()); }

    String getDefaultPositionName() const
    {
        auto ln = Dialog::PositionInfo::getLabelPositionNames();
	    return ln[inheritsPosition ? (int)Dialog::PositionInfo::LabelPositioning::Default : positionInfo.LabelPosition];
    }

protected:

    String label;
    Dialog::PositionInfo positionInfo;
    bool inheritsPosition = true;
    bool required = false;

private:


    ScopedPointer<Component> component;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LabelledComponent);
};


struct TextInput: public LabelledComponent,
                  public TextEditor::Listener,
                  public Timer
{
    DEFAULT_PROPERTIES(TextInput)
    {
        return {
            { mpid::Text, "Label" },
            { mpid::ID, "textId" },
            { mpid::Help, "" },
            { mpid::Required, false },
            { mpid::Multiline, false },
            { mpid::Items, var(Array<var>({var("Autocomplete 1"), var("Autocomplete 2")})) }
        };
    }

    TextInput(Dialog& r, int width, const var& obj);;
    
    bool keyPressed(const KeyPress& k) override;
    void timerCallback() override;

    void textEditorReturnKeyPressed(TextEditor& e);
    void textEditorTextChanged(TextEditor& e);
    void textEditorEscapeKeyPressed(TextEditor& e);

    void postInit() override;
    Result checkGlobalState(var globalState) override;

    void createEditor(Dialog::PageInfo& info) override;

    Result loadFromInfoObject(const var& obj) override;

private:

    String emptyText;

    void showAutocomplete(const String& currentText);
    void dismissAutocomplete();

    struct Autocomplete;

    ScopedPointer<Autocomplete> currentAutocomplete;
    StringArray autocompleteItems;
    

    bool parseInputAsArray = false;

    JUCE_DECLARE_WEAK_REFERENCEABLE(TextInput);
};

struct Tickbox: public LabelledComponent,
                public ButtonListener
{
    DEFAULT_PROPERTIES(Tickbox)
    {
        return {
            { mpid::Text, "Label" },
            { mpid::ID, "tickId" },
            { mpid::Help, "" },
            { mpid::Required, false }
        };
    }

    Tickbox(Dialog& r, int width, const var& obj);;

    void createEditor(Dialog::PageInfo& info) override;

    void postInit() override;
    Result checkGlobalState(var globalState) override;
    void buttonClicked(Button* b) override;

    Result loadFromInfoObject(const var& obj) override;

private:
    
    Array<ToggleButton*> groupedButtons;
    int thisRadioIndex = -1;
    
    bool requiredOption = false;
};

struct Choice: public LabelledComponent
{
    enum class ValueMode
    {
	    Text,
        Index,
        Id,
        numValueModes
    };

    static StringArray getValueModeNames() { return { "Text", "Index", "ID" }; }

    DEFAULT_PROPERTIES(Choice)
    {
        return {
            { mpid::Text, "Label" },
            { mpid::ID, "choiceId" },
            { mpid::Help, "" },
            { mpid::Items, var(Array<var>({var("Option 1"), var("Option 2")})) }
        };
    }

    Choice(Dialog& r, int width, const var& obj);;

    Result loadFromInfoObject(const var& obj) override;

    void postInit() override;
    Result checkGlobalState(var globalState) override;

    void createEditor(Dialog::PageInfo& info) override;

    ValueMode valueMode = ValueMode::Text;

    bool custom = false;
};

struct ColourChooser: public LabelledComponent
{
    DEFAULT_PROPERTIES(ColourChooser)
    {
        return {
            { mpid::ID, "colourId" }
        };
    }
    
    ColourChooser(Dialog& r, int w, const var& obj);

    void postInit() override;;
    void resized() override;
    Result checkGlobalState(var globalState) override;

    Result loadFromInfoObject(const var& obj) override
    {
	    return Result::ok();
    }

    void createEditor(Dialog::PageInfo& info) override
    {
	    jassertfalse;
        
    }

    LookAndFeel_V4 laf;
    
};

struct FileSelector: public LabelledComponent
{
    DEFAULT_PROPERTIES(FileSelector)
    {
        return {
            { mpid::Directory, true },
            { mpid::ID, "fileId" },
            { mpid::Wildcard, "*.*" },
            { mpid::SaveFile, true }
        };
    }

    void createEditor(Dialog::PageInfo& info) override;

    FileSelector(Dialog& r, int width, const var& obj);
    
    void postInit() override;
    Result checkGlobalState(var globalState) override;

    static Component* createFileComponent(const var& obj);
    static File getInitialFile(const var& path);

private:
    
    bool isDirectory = false;
    Identifier fileId;

    JUCE_DECLARE_WEAK_REFERENCEABLE(FileSelector);
};

} // factory
} // multipage
} // hise