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

    virtual Result loadFromInfoObject(const var& obj);

    void postInit() override;
    void paint(Graphics& g) override;
    void resized() override;

    void callOnValueChange();

    static String getCategoryId() { return "UI Elements"; }

    void editModeChanged(bool isEditMode) override;

    template <typename T> T& getComponent() { return *dynamic_cast<T*>(component.get()); }
    template <typename T> const T& getComponent() const { return *dynamic_cast<T*>(component.get()); }

protected:

    String label;
    bool required = false;

    bool visible = true;
    bool enabled = true;

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

    struct AutocompleteNavigator: public KeyListener
    {
        AutocompleteNavigator(TextInput& parent_):
          parent(parent_)
        {}
	    bool keyPressed (const KeyPress& key,
                             Component* originatingComponent) override;

        TextInput& parent;
    } navigator;

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

struct Button: public LabelledComponent,
                public ButtonListener,
				public PathFactory
{
    DEFAULT_PROPERTIES(Button)
    {
        return {
            { mpid::Text, "Label" },
            { mpid::ID, "tickId" },
            { mpid::Help, "" },
            { mpid::Required, false }
        };
    }

    Button(Dialog& r, int width, const var& obj);;

    Path createPath(const String& url) const override;
    void createEditor(Dialog::PageInfo& info) override;

    void postInit() override;
    Result checkGlobalState(var globalState) override;
    void buttonClicked(juce::Button* b) override;

    Result loadFromInfoObject(const var& obj) override;

private:

    String getStringForButtonType() const;

    bool isTrigger = false;
    MemoryBlock pathData;
	juce::Button* createButton(const var& obj);
    Array<juce::Button*> groupedButtons;
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

struct CodeEditor: public LabelledComponent
{
    HISE_MULTIPAGE_ID("CodeEditor"); 

    struct AllEditor: public Component
    {
        static void addRecursive(mcl::TokenCollection::List& tokens, const String& parentId, const var& obj)
        {
            auto thisId = parentId;

            if(obj.isMethod())
                thisId << "(args)";

            auto isState = thisId.startsWith("state");
            auto isElement = thisId.startsWith("element");

            auto prio = 100;

            if(isState)
                prio += 10;

            if(isElement)
                prio += 20;

	        auto stateToken = new mcl::TokenCollection::Token(thisId);
            stateToken->c = isState ? Colour(0xFFBE6093) : isElement ? Colour(0xFF22BE84) : Colour(0xFF88BE14);

            if(isState)
				stateToken->markdownDescription << "Global state variable  \n> ";

        	stateToken->markdownDescription << "Value: `" << obj.toString() << "`";

            auto apiObject = dynamic_cast<ApiObject*>(obj.getDynamicObject());

            stateToken->priority = prio;
            tokens.add(stateToken);

            if(auto no = obj.getDynamicObject())
            {
	            for(auto& nv: no->getProperties())
	            {
		            String p = parentId;
                    p << "." << nv.name;
                    addRecursive(tokens, p, nv.value);

                    if(apiObject != nullptr)
                        tokens.getLast()->markdownDescription = apiObject->getHelp(nv.name);
	            }
            }
            if(auto ar = obj.getArray())
            {
                int idx = 0;

	            for(auto& nv: *ar)
	            {
                    String p = parentId;
                    p << "[" << String(idx++) << "]";
		            addRecursive(tokens, p, nv);
	            }
            }
        }

        struct TokenProvider: public mcl::TokenCollection::Provider
        {
            TokenProvider(Component* p_):
              p(p_)
            {};

            Component::SafePointer<Component> p;

	        void addTokens(mcl::TokenCollection::List& tokens) override
	        {
                if(p == nullptr)
                    return;

                auto infoObject = p->findParentComponentOfClass<Dialog>()->getState().globalState;

                DBG(JSON::toString(infoObject));

	            if(auto ms = p->findParentComponentOfClass<ComponentWithSideTab>())
	            {
	                auto* state = ms->getMainState();

                    if(engine == nullptr)
						engine = state->createJavascriptEngine(infoObject);

                    for(auto& nv: engine->getRootObjectProperties())
                    {
                        addRecursive(tokens, nv.name.toString(), nv.value);
                    }
	            }
	        }

            std::unique_ptr<JavascriptEngine> engine;
        };

        AllEditor():
          codeDoc(doc),
          editor(new mcl::TextEditor(codeDoc))
        {
            editor->tokenCollection = new mcl::TokenCollection("Javascript");
            editor->tokenCollection->setUseBackgroundThread(false);
        	editor->tokenCollection->addTokenProvider(new TokenProvider(this));
	        addAndMakeVisible(editor);
        }

        ~AllEditor()
        {
            

	        editor->tokenCollection = nullptr;
            editor = nullptr;
        }

        void resized() override { editor->setBounds(getLocalBounds()); };

        

	    juce::CodeDocument doc;
	    mcl::TextDocument codeDoc;
	    ScopedPointer<mcl::TextEditor> editor;
    };

	CodeEditor(Dialog& r, int w, const var& obj):
      LabelledComponent(r, w, obj, new AllEditor())
	{
		setSize(w, 360);
	};

    void postInit() override
    {
	    LabelledComponent::postInit();

        auto code = getValueFromGlobalState(var()).toString();

        getComponent<AllEditor>().doc.replaceAllContent(code);
    }

    bool keyPressed(const KeyPress& k) override
    {
	    if(k == KeyPress::F5Key)
	    {
		    compile();
            return true;
	    }

        return false;
    }

    Result compile()
    {
	    auto code = getComponent<AllEditor>().doc.getAllContent();

        auto* state = findParentComponentOfClass<ComponentWithSideTab>()->getMainState();

        if(code.startsWith("${"))
            code = state->loadText(code);

        auto infoObject = findParentComponentOfClass<Dialog>()->getState().globalState;

        auto e = state->createJavascriptEngine(infoObject);
        auto ok = e->execute(code);

        getComponent<AllEditor>().editor->setError(ok.getErrorMessage());

        if(ok.wasOk())
            writeState(code);
        else
            state->eventLogger.sendMessage(sendNotificationSync, MessageType::Javascript, ok.getErrorMessage());

        return ok;
    }

    Result checkGlobalState(var globalState) override
    {
        return compile();
    }
};

struct ColourChooser: public LabelledComponent,
					  public ChangeListener
{
    DEFAULT_PROPERTIES(ColourChooser)
    {
        return {
            { mpid::ID, "colourId" }
        };
    }
    
    ColourChooser(Dialog& r, int w, const var& obj);

    ~ColourChooser() override;

    void postInit() override;;
    void resized() override;

    void changeListenerCallback(ChangeBroadcaster* source) override;

    Result checkGlobalState(var globalState) override;
    Result loadFromInfoObject(const var& obj) override;

    void createEditor(Dialog::PageInfo& info) override;

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
    File getInitialFile(const var& path) const;

private:
    
    bool isDirectory = false;

    JUCE_DECLARE_WEAK_REFERENCEABLE(FileSelector);
};



} // factory
} // multipage
} // hise
