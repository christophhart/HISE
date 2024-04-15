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
    void resized() override;
    
    static String getCategoryId() { return "UI Elements"; }

    template <typename T> T& getComponent() { return *dynamic_cast<T*>(component.get()); }
    template <typename T> const T& getComponent() const { return *dynamic_cast<T*>(component.get()); }

protected:

    String label;
    bool required = false;

    bool enabled = true;

    

private:

    bool showLabel = false;

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

    bool useDynamicAutocomplete = false;

    bool callOnEveryChange = false;

    String emptyText;

    void showAutocomplete(const String& currentText);
    void dismissAutocomplete();

    struct Autocomplete;

    ScopedPointer<Autocomplete> currentAutocomplete;
    StringArray autocompleteItems;
    bool parseInputAsArray = false;

    JUCE_DECLARE_WEAK_REFERENCEABLE(TextInput);
};

struct Button:  public ButtonListener,
				public LabelledComponent
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

    
    void createEditor(Dialog::PageInfo& info) override;

    void postInit() override;
    Result checkGlobalState(var globalState) override;
    void buttonClicked(juce::Button* b) override;

    Result loadFromInfoObject(const var& obj) override;

private:

    struct IconFactory: public PathFactory
    {
        IconFactory(Dialog* r, const var& obj_):
          obj(obj_),
          d(r)
        {};

        Dialog* d;
        var obj;

	    Path createPath(const String& url) const override
	    {
		    Path p;

            auto b64 = obj[mpid::Icon].toString();

            if(d != nullptr)
                b64 = d->getState().loadText(b64, false);

            MemoryBlock mb;
            mb.fromBase64Encoding(b64);

            if(mb.getSize() == 0)
                mb.fromBase64Encoding("844.t01G.z.QfCheCwV..d.QfCheCwV..d.QbVhXCIV..d.QL0zSCAyTKPDV..zPCk.DDgE..MDajeuEDgE..MDajeuEDQIvVMDae.PCDQIvVMDae.PCDANH9MzXs4S7aPDk.a0Pr4S7aPDV..zProYojPDV..zProYojPDk.a0Pr4S7aPDk.a0Pi0F8dlBQTBrUCwF8dlBQXA.PCwFTSICQXA.PCwFTSICQTBrUCwF8dlBQTBrUCMVa3xzMDQIvVMDa3xzMDgE..MDa8ZOODgE..MjXQyZPDgE..MT..VDQL0zSCE.fEQDmkH1PrE.fEQD3f32PrI9++PD3f32PrI9++PDk.a0PrgKS2PDk.a0Pi0l3++CQjLPhCwV..VDQjLPhCwV..VDQbullCwl3++CQbullCwl3++CQjLPhCMVah++ODQvWjNDaA.XQDQvWjNDaA.XQDQsw0NDah++ODQsw0NDah++ODQvWjNzXsI9++PD+496PrE.fEQD+496PrE.fEQDhsq7PhE.fEQjHZQ8PQyZPDA8+aOTu1yCQP++1CwFtLcCQP++1CwFtLcCQN+IzCwl3++CQN+IzCwl3++CQ7m6uCMVaPMkLD47mPODaPMkLDA8+aODaz6YJDA8+aODaz6YJD47mPODaPMkLD47mPOzXsoYojPjyeB8ProYojPDz+u8Pr4S7aPDz+u8Pr4S7aPjyeB8ProYojPjyeB8Pi0F42aAQN+IzCwF42aAQP++1Cw1PI.AQP++1CIFLSs.QP++1CE.fGPjHZQ8PA.3ADgX6JODaA.3ADwet+NDae.PCDwet+NDae.PCD47mPODajeuED47mPOzXs8A.MPD0FW6PrE.fGPD0FW6PrE.fGPDAeQ5Pr8A.MPDAeQ5Pr8A.MPD0FW6Pi01G.z.QbullCwV..d.QbullCwV..d.QjLPhCw1G.z.QjLPhCw1G.z.QbullCMVa3QyHDAI.dNDaJpeFDwEiKNDaKXTGD4S8DNDa4+mIDoQZWNDa2m6KD4S8DNDa3UvLDwEiKNDaHtbJDAI.dNDa3UvLDAEcvNDa2m6KDg7B2NDa4+mIDItkjNDaKXTGDg7B2NDaJpeFDAEcvNDa3QyHDAI.dNzXkA");
            else
				p.loadPathFromData(mb.getData(), mb.getSize());
            
			return p;
	    }
    };
    
    String getStringForButtonType() const;

    bool isTrigger = false;
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
						engine = state->createJavascriptEngine();

                    for(auto& nv: engine->getRootObjectProperties())
                    {
                        addRecursive(tokens, nv.name.toString(), nv.value);
                    }
	            }
	        }

            JavascriptEngine* engine = nullptr;
        };

        AllEditor(const String& syntax):
          codeDoc(doc),
          editor(new mcl::TextEditor(codeDoc))
        {
            if(syntax == "CSS")
            {
	            editor->tokenCollection = new mcl::TokenCollection("CSS");
                editor->tokenCollection->setUseBackgroundThread(false);
	            editor->setLanguageManager(new simple_css::LanguageManager(codeDoc));
            }
            else if(syntax == "HTML")
			{
                editor->setLanguageManager(new mcl::XmlLanguageManager());
			}
            else
            {
	            editor->tokenCollection = new mcl::TokenCollection("Javascript");
		        editor->tokenCollection->setUseBackgroundThread(false);
		        editor->tokenCollection->addTokenProvider(new TokenProvider(this));
            }
            
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
      LabelledComponent(r, w, obj, new AllEditor(obj[mpid::Syntax].toString()))
	{
        Helpers::writeInlineStyle(getComponent<AllEditor>(), "height: 360px;");
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
        auto syntax = infoObject[mpid::Syntax].toString();

        auto& editor = *getComponent<AllEditor>().editor.get();

        auto code = getComponent<AllEditor>().doc.getAllContent();

        auto* state = findParentComponentOfClass<ComponentWithSideTab>()->getMainState();

        if(code.startsWith("${"))
            code = state->loadText(code, true);

        if(syntax == "CSS")
        {
            simple_css::Parser p(code);
            auto ok = p.parse();
			editor.clearWarningsAndErrors();
			editor.setError(ok.getErrorMessage());

			for(const auto& w: p.getWarnings())
				editor.addWarning(w);

            writeState(code);

            if(ok.wasOk())
            {
	            if(auto d = state->currentDialog.get())
	            {
		            d->positionInfo.additionalStyle = code;
					d->loadStyleFromPositionInfo();
	            }
            }
            else
                state->eventLogger.sendMessage(sendNotificationSync, MessageType::Javascript, ok.getErrorMessage());
            
            return ok;
        }
        else if (syntax == "HTML")
        {
	        writeState(code);

            if(auto d = state->currentDialog.get())
	        {
                d->refreshCurrentPage();
	        }

            return Result::ok();
        }
        else
        {
            writeState(code);

	        auto e = state->createJavascriptEngine();
	        auto ok = e->execute(code);

	        getComponent<AllEditor>().editor->setError(ok.getErrorMessage());

	        if(!ok.wasOk())
	            state->eventLogger.sendMessage(sendNotificationSync, MessageType::Javascript, ok.getErrorMessage());

	        return ok;
        }
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
