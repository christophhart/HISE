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
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise { using namespace juce;

struct GLSLKeywordProvider : public mcl::TokenCollection::Provider
{
	struct GLSLKeyword : public mcl::TokenCollection::Token
	{
		GLSLKeyword(const String& name) :
			Token(name)
		{
			c = Colours::lightcoral;
			priority = 40;
		}
	};

	struct GLSLToken : public mcl::TokenCollection::Token
	{
		GLSLToken(const String& codeToInsert, const String& typeString) :
			Token(typeString + " " + codeToInsert),
			code(codeToInsert)
		{
			c = Colours::magenta;
			priority = 50;
		}

		bool matches(const String& input, const String& previousToken, int lineNumber) const
		{
			return previousToken.isEmpty() && matchesInput(input, code);
		}

		String getCodeToInsert(const String& input) const override { return code; }

		String code;
	};

	void addKeyword(mcl::TokenCollection::List& tokens, const String& word, const String& description)
	{
		auto c = new GLSLKeyword(word);
		c->markdownDescription = description;
		tokens.add(c);
	}

	void addGlobalVariable(mcl::TokenCollection::List& tokens, const String& word, const String& type, const String& description)
	{
		auto c = new GLSLToken(word, type);

		c->markdownDescription = description;

		tokens.add(c);
	}

	void addTokens(mcl::TokenCollection::List& tokens) override
	{
		addGlobalVariable(tokens, "iResolution", "vec2", "The actual pixel size of the canvas");
		addGlobalVariable(tokens, "pixelPos", "vec2", "The unscaled pixel position on the monitor");
		addGlobalVariable(tokens, "fragCoord", "vec2", "The scaled pixel coordinate relative to the bottom left");
		addGlobalVariable(tokens, "iTime", "float", "The time in seconds since compilation");
		addGlobalVariable(tokens, "fragColor", "vec4", "The output colour for the given pixel");
		addGlobalVariable(tokens, "pixelAlpha", "float", "The alpha value that needs to be multiplied with the output colour");

		addKeyword(tokens, "vec2", "A two dimensional vector");
		addKeyword(tokens, "vec3", "A three dimensional vector");
		addKeyword(tokens, "vec4", "A four dimensional vector");
		addKeyword(tokens, "float", "A single precision float number");

		addKeyword(tokens, "uniform", "A keyword for specifying uniform data");
		addKeyword(tokens, "main", "The main entry function");
	}
};

PopupIncludeEditor::PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit) :
	jp(s),
	callback(Identifier()),
	tokeniser(new JavascriptTokeniser())
{
	
	if(fileToEdit.getFileExtension() == ".glsl")
		t = FileTypes::GLSL;
	else if (fileToEdit.getFileExtension() == ".css")
		t = FileTypes::CSS;
	else
		t = FileTypes::Javascript;

	tokeniser->setUseScopeStatements(true);
	

	Processor *p = dynamic_cast<Processor*>(jp.get());
	externalFile = p->getMainController()->getExternalScriptFile(fileToEdit, t == FileTypes::Javascript || t == FileTypes::CSS);

    p->getMainController()->addScriptListener(this);
    
    checkUnreferencedExternalFile();
    
	auto isJavascript = t == FileTypes::Javascript;

	addEditor(externalFile->getFileDocument(), isJavascript);

	if (externalFile != nullptr && !isJavascript)
		externalFile->getRuntimeErrorBroadcaster().addListener(*this, runTimeErrorsOccured);

	addButtonAndCompileLabel();
	refreshAfterCompilation(JavascriptProcessor::SnippetResult(s->getLastErrorMessage(), 0));

	for (int i = 0; i < jp->getNumWatchedFiles(); i++)
	{
		if (jp->getWatchedFile(i) == fileToEdit)
		{
			auto storedPos = jp->getLastPosition(jp->getWatchedFileDocument(i));

			if (storedPos.getPosition() != 0)
			{
				mcl::Selection sel;
				sel.head = { storedPos.getLineNumber(), storedPos.getIndexInLine() };
				sel.tail = sel.head;
				editor->editor.getTextDocument().setSelection(0, sel, false);
			}
		}
	}
}

PopupIncludeEditor::PopupIncludeEditor(JavascriptProcessor* s, const Identifier &callback_) :
	jp(s),
	callback(callback_),
	tokeniser(new JavascriptTokeniser())
{
	tokeniser->setUseScopeStatements(true);
	
	auto& d = *jp->getSnippet(callback_);
	addEditor(d, true);
	addButtonAndCompileLabel();

    dynamic_cast<Processor*>(jp.get())->getMainController()->addScriptListener(this);
    
	refreshAfterCompilation(JavascriptProcessor::SnippetResult(s->getLastErrorMessage(), 0));
}

struct JavascriptLanguageManager : public mcl::LanguageManager
{
	JavascriptLanguageManager(JavascriptProcessor* jp_, const Identifier& callback_, mcl::TextEditor& ed):
		jp(jp_),
        callback(callback_)
	{
        jp->inplaceBroadcaster.addListener(ed, [](mcl::TextEditor& ed, Identifier, int)
        {
            ed.repaint();
        });
    };

	CodeTokeniser* createCodeTokeniser() override 
	{
        auto js = new JavascriptTokeniser();
		js->setUseScopeStatements(true);
		return js;
	}

    Identifier getLanguageId() const override { return mcl::LanguageIds::HiseScript; }
    
    bool getInplaceDebugValues(Array<InplaceDebugValue>& values) const override
    {
		auto sn = jp->getSnippet(callback);
		
		for(auto& ip: jp->inplaceValues)
		{
			ip.init();

			if(ip.location.getOwner() == sn)
			{
				values.add(ip);
			}
		}

        return true;
    }
    
	virtual void processBookmarkTitle(juce::String& t)
	{
		
	}

	String getDefaultScriptCodeSnippets(bool getDefault)
	{
		Array<var> list;

		static const Identifier name_("name");
		static const Identifier language_("language");
		static const Identifier code_("code");
		static const Identifier description_("description");
		static const Identifier priority_("priority");

#define ADD_HS_SNIPPET(name, code, description); { DynamicObject::Ptr p = new DynamicObject(); p->setProperty(name_, name); \
	                                                                p->setProperty(language_, "HiseScript"); p->setProperty(code_, code); \
																	p->setProperty(description_, description); p->setProperty(priority_, 100); \
														            list.add(var(p.get())); }
			

		if(!getDefault)
		{
			DynamicObject::Ptr p = new DynamicObject();

			ADD_HS_SNIPPET("userSnippet", "This is a $SNIPPET$ from the user snippet list", 
				        "This is a description that will show up in the autocomplete textbox");
		}
		else
		{
	

			ADD_HS_SNIPPET("reg (...)", "reg $VAR_NAME$ = $VALUE$;", "A shortcut to a reg variable definition.");
			ADD_HS_SNIPPET("const (...)", "const var $VAR_NAME$ = $VALUE$;", "A shortcut to a const variable definition.");
			ADD_HS_SNIPPET("local (...)", "local $VAR_NAME$ = $VALUE$;", "A shortcut to a local variable definition.");
			ADD_HS_SNIPPET("var (...)", "var $VAR_NAME$ = $VALUE$;", "A shortcut to a standard variable definition.");
			ADD_HS_SNIPPET("for (...)", "for($LOOP_VAR$ = 0; $LOOP_VAR$ < 10; $LOOP_VAR$++)\n{\n\t$// loop body$\n}",
				           "A simple for loop");
			ADD_HS_SNIPPET("tr (...)", "Console.print(trace($data$));", "A shortcut for printing something to the console using `trace`");

			ADD_HS_SNIPPET("inline1 (...)", "inline function $functionName$($args1$)\n{\n\t$// body$\n};",
				           "A shortcut for a inline function definition with a single argument");

			ADD_HS_SNIPPET("inline2 (...)", "inline function $functionName$($args1$, $args2$)\n{\n\t$// body$\n};",
				           "A shortcut for a inline function definition with two arguments");

			ADD_HS_SNIPPET("inline3 (...)", "inline function $functionName$($args1$, $args2$, $args3$)\n{\n\t$// body$\n};",
				           "A shortcut for a inline function definition with 3 argument");

			ADD_HS_SNIPPET("inline4 (...)", "inline function $functionName$($args1$, $args2$, $args3$, $args4$)\n{\n\t$// body$\n};",
				           "A shortcut for a inline function definition with 4 arguments");

			ADD_HS_SNIPPET("inline5 (...)", "inline function $functionName$($args1$, $args2$, $args3$, $args4$, $args5$)\n{\n\t$// body$\n};",
				           "A shortcut for a inline function definition with a 5 arguments");

			ADD_HS_SNIPPET("JSON1 (...)", "{\n  \"$key1$\": $v1$ }", 
				           "A shortcut for a JSON object definition with a single property");

			ADD_HS_SNIPPET("JSON2 (...)", "{\n  \"$key1$\": $v1$,\n  \"$key2$\": $v2$\n}", 
				           "A shortcut for a JSON object definition with two properties");

			ADD_HS_SNIPPET("JSON3 (...)", "{\n  \"$key1$\": $v1$,\n  \"$key2$\": $v2$,\n  \"$key3$\": $v3$\n}", 
				           "A shortcut for a JSON object definition with a 3 properties");

			ADD_HS_SNIPPET("JSON4 (...)", "{\n  \"$key1$\": $v1$,\n  \"$key2$\": $v2$,\n  \"$key3$\": $v4$,\n  \"$key4$\": $v3$\n}", 
				           "A shortcut for a JSON object definition with a 4 properties");

			ADD_HS_SNIPPET("broadcaster1 (...)", "const var $BROADCASTER_ID$ = Engine.createBroadcaster({\n  id: \"$BROADCASTER_ID$\",\n  args: [\"$a1$\"]\n});", 
				           "A broadcaster definition with a single argument");
		    ADD_HS_SNIPPET("broadcaster2 (...)", "const var $BROADCASTER_ID$ = Engine.createBroadcaster({\n  id: \"$BROADCASTER_ID$\",\n  args: [\"$a1$\", \"$a2$\"]\n});", 
				           "A broadcaster definition with a two arguments");
			ADD_HS_SNIPPET("broadcaster3 (...)", "const var $BROADCASTER_ID$ = Engine.createBroadcaster({\n  id: \"$BROADCASTER_ID$\",\n  args: [\"$a1$\", \"$a2$\", \"$a3$\"]\n});", 
				           "A broadcaster definition with a two arguments");

			ADD_HS_SNIPPET("onControl (...)", "inline function on$UI_CONTROL$Control(component, value)\n{\n\t$Console.print(value);$\n};\nContent.getComponent(\"$UI_CONTROL$\").setControlCallback(on$UI_CONTROL$Control);",
						   "A code template for a control callback.  \n>Use Ctrl+D for selecting all `UI_CONTROL` tokens and rename it to the control ID");

		    ADD_HS_SNIPPET("timer (...)", "const var $TIMER_VAR$ = Engine.createTimerObject();\n\n$TIMER_VAR$.setTimerCallback(function()\n{\n\t$// timer callback$\n});\n\n$TIMER_VAR$.startTimer($30$);\n",
                           "A code snippet that will create a UI timer definition with a callback and a default period of 30Hz");

		    ADD_HS_SNIPPET("css_inline (...)", "const var $UI_CONTROL$_laf = Content.createLocalLookAndFeel();\n$UI_CONTROL$_laf.setInlineStyleSheet(\"#$UI_CONTROL$ {\n  background-color: blue;\n  color: white;\n}\n\n#$UI_CONTROL$:hover {\n  background-color: red;\n}\");\n\nContent.getComponent(\"$UI_CONTROL$\").setLocalLookAndFeel($UI_CONTROL$_laf);\n",
				           "A code snippet that will create a LAF class with an inline CSS style sheet that can be applied to a UI control using its id selector.  \n>Use `Ctrl+D` for selecting all `UI_CONTROL` tokens and rename it to the control ID");
	
#undef  ADD_HS_SNIPPET
		}

		return JSON::toString(var(list), false);
	}

	/** Add all token providers you want to use for this language. */
	void addTokenProviders(mcl::TokenCollection* t)
	{
		t->addTokenProvider(new HiseJavascriptEngine::TokenProvider(jp));

		if(auto p = jp.get())
		{
			auto mc = dynamic_cast<Processor*>(p)->getMainController();

			Array<File> fileList;
			
			auto appDataFolder = ProjectHandler::getAppDataDirectory(mc);

			fileList.add(appDataFolder.getChildFile("hiseCodeSnippets.json"));

			fileList[0].replaceWithText(getDefaultScriptCodeSnippets(true));
			
			fileList.add(appDataFolder.getChildFile("userCodeSnippets.json"));

			if(!fileList[1].existsAsFile())
			{
				fileList[1].replaceWithText(getDefaultScriptCodeSnippets(false));
			}

			t->addTokenProvider(new mcl::CodeSnippetProvider(fileList, "HiseScript", [mc](const String& m)
			{
				mc->getConsoleHandler().writeToConsole(m, 2, mc->getMainSynthChain(), Colours::red);
			}));
		}
	}

	/** Use this for additional setup. */
	void setupEditor(mcl::TextEditor* editor) override
	{
		editor->setIncludeDotInAutocomplete(true);
		editor->setEnableBreakpoint(true);
	}

	WeakReference<JavascriptProcessor> jp;
    Identifier callback;
};

struct GLSLLanguageManager : public mcl::LanguageManager
{
	CodeTokeniser* createCodeTokeniser() override {
		return new JavascriptTokeniser();
	}

	virtual void processBookmarkTitle(juce::String& )
	{
		
	}

	Identifier getLanguageId() const override { return mcl::LanguageIds::GLSL; }

	void addTokenProviders(mcl::TokenCollection* t)
	{
		t->addTokenProvider(new GLSLKeywordProvider());
	}

	void setupEditor(mcl::TextEditor* e) override
	{
		e->tokenCollection = new mcl::TokenCollection(getLanguageId());
		addTokenProviders(e->tokenCollection.get());
	}
};

void PopupIncludeEditor::addButtonAndCompileLabel()
{
	addAndMakeVisible(bottomBar = new EditorBottomBar(jp.get()));
    
	bottomBar->setCompileFunction(BIND_MEMBER_FUNCTION_0(PopupIncludeEditor::compileInternal));

    
    
	setSize(800, 800);
}



void PopupIncludeEditor::refreshAfterCompilation(const JavascriptProcessor::SnippetResult& r)
{
    checkUnreferencedExternalFile();
    
	bottomBar->setError(r.r.getErrorMessage());

	if (auto asmcl = dynamic_cast<mcl::FullEditor*>(editor.get()))
	{
		if (!r.r.wasOk())
		{
			auto errorMessage = r.r.getErrorMessage();

			auto secondLine = errorMessage.fromFirstOccurrenceOf("\n", false, false).substring(1).trim();

			auto fileName = getFile().getFileName();

			bool isSameFile = true;

			if (!secondLine.startsWith(callback.toString()))
				isSameFile = false;

			if(fileName.isNotEmpty() && secondLine.contains(fileName))
				isSameFile = true;

			if (fileName.isEmpty() && secondLine.contains(".js"))
				isSameFile = false;

			if (secondLine.isEmpty())
				isSameFile = true;

			if (!isSameFile)
			{
				asmcl->editor.clearWarningsAndErrors();
				return;
			}

			auto message = errorMessage.upToFirstOccurrenceOf("{", false, false);
			auto line = errorMessage.fromFirstOccurrenceOf("Line ", false, false).getIntValue();
			auto col = errorMessage.fromFirstOccurrenceOf("column ", false, false).getIntValue();

			String mclError = "Line ";
			mclError << line << "(" << col << "): " << message;

			asmcl->editor.setError(mclError);
		}
		else
		{
			asmcl->editor.clearWarningsAndErrors();
		}
	}
}

PopupIncludeEditor::~PopupIncludeEditor()
{
    Processor *sp = dynamic_cast<Processor*>(jp.get());
    
	if (jp != nullptr && editor != nullptr)
	{
		auto& doc = editor->editor.getDocument();
		auto pos = editor->editor.getTextDocument().getSelection(0).head;

        CodeDocument::Position p(doc, pos.x, pos.y);
     
        sp->getMainController()->removeScriptListener(this);
        
		jp->setWatchedFilePosition(p);
	}

	editor = nullptr;

	tokeniser = nullptr;

	bottomBar = nullptr;

    doc = nullptr;
	externalFile = nullptr;
}



bool PopupIncludeEditor::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::F5Key) && !key.getModifiers().isShiftDown())
	{
		bottomBar->recompile();
		return true;
	}

#if 0
    if (TopLevelWindowWithKeyMappings::matches(this, key, TextEditorShortcuts::goto_undo))
    {
        return dynamic_cast<Processor*>(jp.get())->getMainController()->getLocationUndoManager()->undo();
        return true;
    }
    if (TopLevelWindowWithKeyMappings::matches(this, key, TextEditorShortcuts::goto_redo))
    {
        return dynamic_cast<Processor*>(jp.get())->getMainController()->getLocationUndoManager()->redo();
        return true;
    }
#endif
    
	if (TopLevelWindowWithKeyMappings::matches(this, key, TextEditorShortcuts::goto_file))
	{
		jassertfalse;
		return true;
	}
	

	return false;
}

void PopupIncludeEditor::runTimeErrorsOccured(PopupIncludeEditor& t, Array<ExternalScriptFile::RuntimeError>* errors)
{
	if (errors == nullptr)
		return;

	if (auto asmcl = CommonEditorFunctions::as(&t))
	{
		for (const auto& e : *errors)
		{
			auto matchesFile = e.matches(t.externalFile->getFile().getFileNameWithoutExtension());

			if (matchesFile && e.errorLevel == ExternalScriptFile::RuntimeError::ErrorLevel::Warning)
				asmcl->editor.addWarning(e.toString(), true);

			if (e.errorLevel == ExternalScriptFile::RuntimeError::ErrorLevel::Error)
			{
				if (matchesFile)
					asmcl->editor.addWarning(e.toString(), false);

				if(t.bottomBar != nullptr)
					t.bottomBar->setError("GLSL Compile Error");
			}
		}

		asmcl->repaint();
	}
}

void PopupIncludeEditor::resized()
{
    checkUnreferencedExternalFile();
    
	bool isInPanel = findParentComponentOfClass<FloatingTile>() != nullptr;

	if(isInPanel)
		editor->setBounds(0, 0, getWidth(), getHeight() - EditorBottomBar::BOTTOM_HEIGHT);
	else
		editor->setBounds(0, 5, getWidth(), getHeight() - 23);

	auto b = getLocalBounds().removeFromBottom(EditorBottomBar::BOTTOM_HEIGHT);

	bottomBar->setBounds(b);
}

void PopupIncludeEditor::gotoChar(int character, int lineNumber/*=-1*/)
{
	CodeDocument::Position pos;

	if (auto ed = getEditor())
	{
		auto& doc = CommonEditorFunctions::getDoc(this);

		pos = lineNumber != -1 ? CodeDocument::Position(doc, lineNumber, character) :
			CodeDocument::Position(doc, character);

#if HISE_USE_NEW_CODE_EDITOR
		ed->editor.scrollToLine(jmax<int>(0, pos.getLineNumber() - 1), true);
#else
		ed->scrollToLine(jmax<int>(0, pos.getLineNumber() - 1));
		ed->moveCaretTo(pos, false);
		ed->moveCaretToStartOfLine(false);
		ed->moveCaretToEndOfLine(true);
#endif
	}
}



void PopupIncludeEditor::breakpointsChanged(mcl::GutterComponent& g)
{
	jp->clearPreprocessorFunctions();

	auto cb = callback;

	WeakReference<mcl::GutterComponent> safeGutter(&g);

	jp->addPreprocessorFunction([safeGutter, cb](const Identifier& id, String& s)
	{
		if (id == cb && safeGutter != nullptr)
			return safeGutter->injectBreakPoints(s);

		return false;
	});

	bottomBar->compileButton->triggerClick();
}

void PopupIncludeEditor::paintOverChildren(Graphics& g)
{
    if (getEditor() == dynamic_cast<Processor*>(jp.get())->getMainController()->getLastActiveEditor())
	{
		g.setColour(unreferencedExternalFile ? Colour(HISE_ERROR_COLOUR) : Colour(SIGNAL_COLOUR));
		g.fillRect(0, 0, 5, 5);
	}
}

void PopupIncludeEditor::initKeyPresses(Component* root)
{
	String cat = "Code Editor";
	
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::add_autocomplete_template, "Add Autocomplete Template", 
		KeyPress(KeyPress::F8Key));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::clear_autocomplete_templates, "Clear Autocomplete Templates", 
		KeyPress(KeyPress::F8Key, ModifierKeys::commandModifier, 0));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::show_search_replace, "Search & Replace", KeyPress('g', ModifierKeys::commandModifier, 'g'));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::breakpoint_resume, "Resume breakpoint", KeyPress(KeyPress::F10Key));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::goto_file, "Goto file", KeyPress('t', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 't'));
    
    TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::goto_undo, "Undo Goto", KeyPress(KeyPress::F12Key, ModifierKeys::commandModifier, 0));
    
    TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::goto_redo, "Redo Goto", KeyPress(KeyPress::F12Key, ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
}

File PopupIncludeEditor::getFile() const
{
	return externalFile != nullptr ? externalFile->getFile() : File();
}

void PopupIncludeEditor::compileInternal()
{
	auto mc = dynamic_cast<Processor*>(getScriptProcessor())->getMainController();
	mc->saveAllExternalFiles();

	if(t == FileTypes::CSS)
	{
		auto& ed = editor->editor;

		simple_css::Parser p(ed.getTextDocument().getCodeDocument().getAllContent());

		ed.clearWarningsAndErrors();

		auto ok = p.parse();

		for(auto w: p.getWarnings())
			ed.addWarning(w, true);

		bottomBar->setError(ok.getErrorMessage());

		if(!ok.wasOk())
		{
			ed.setError(ok.getErrorMessage());
		}
		else
		{
			auto top = getTopLevelComponent();
			auto css = p.getCSSValues();
			auto scriptFolder = dynamic_cast<Processor*>(getScriptProcessor())->getMainController()->getActiveFileHandler()->getSubDirectory(FileHandlerBase::Scripts);
			auto fileName = getFile().getRelativePathFrom(scriptFolder).replaceCharacter('\\', '/');

			Component::callRecursive<ScriptContentComponent>(top, [&](ScriptContentComponent* c)
			{
				c->css.updateIsolatedCollection(fileName, css);

				using BD = ScriptingApi::Content::ScriptMultipageDialog::Backdrop;

				Component::callRecursive<BD>(c, [&](BD* mp)
				{
					mp->create(getEditor()->editor.getDocument().getAllContent());
					return false;
				});

				c->repaint();

				return false;
			});
		}
		
		return;
	}
	else
	{
		Component::SafePointer<PopupIncludeEditor> safeP(this);
    
	    auto f = [safeP](const JavascriptProcessor::SnippetResult& r)
	    {
	        if(safeP.getComponent() != nullptr)
	        {
	            safeP->refreshAfterCompilation(r);
	        }
	    };
	    
		jp->compileScript(f);
	}

    

	if (auto asmcl = dynamic_cast<mcl::TextEditor*>(editor.get()))
	{
		asmcl->clearWarningsAndErrors();
	}
}

void PopupIncludeEditor::scriptWasCompiled(JavascriptProcessor* p)
{
	if (p == jp)
	{
        String pid;
        
        if(callback.isValid())
            pid << dynamic_cast<Processor*>(p)->getId() << "." << callback.toString();
        else
            pid << getFile().getFullPathName();
        
		auto deactivatedLines = p->getScriptEngine()->preprocessor->getDeactivatedLinesForFile(pid);
		getEditor()->editor.setDeactivatedLines(deactivatedLines);

		checkUnreferencedExternalFile();
		repaint();
	}
}

float PopupIncludeEditor::getGlobalCodeFontSize(Component* c)
{
	FloatingTile* ft = c->findParentComponentOfClass<FloatingTile>();
	return ft->getMainController()->getGlobalCodeFontSize();
}

void PopupIncludeEditor::sleepStateChanged(const Identifier& id, int lineNumber, bool on)
{
	if (callback == id)
	{
		getEditor()->setCurrentBreakline(on ? lineNumber : -1);

		bottomBar->setShowResume(on, lineNumber);
	}
}

void PopupIncludeEditor::addEditor(CodeDocument& d, bool isJavascript)
{
	auto asComponent = dynamic_cast<Component*>(this);

#if HISE_USE_NEW_CODE_EDITOR
	doc = new mcl::TextDocument(d);

	asComponent->addAndMakeVisible(editor = new mcl::FullEditor(*doc));

	auto& ed = getEditor()->editor;

	if (isJavascript)
		ed.setLanguageManager(new JavascriptLanguageManager(jp, callback, ed));
	else
	{
		if(t == FileTypes::GLSL)
			ed.setLanguageManager(new GLSLLanguageManager());
		else
		{
			ed.tokenCollection = new mcl::TokenCollection("CSS");
			ed.tokenCollection->setUseBackgroundThread(false);
			ed.setLanguageManager(new simple_css::LanguageManager(ed.getTextDocument()));
			ed.tokenCollection->rebuild();
		}
			
	}

	ed.setPopupLookAndFeel(new PopupLookAndFeel());
	
	auto mc = dynamic_cast<Processor*>(jp.get())->getMainController();

    ed.getTextDocument().setExternalViewUndoManager(mc->getLocationUndoManager());
    
	mc->getFontSizeChangeBroadcaster().addListener(ed, [mc](mcl::TextEditor& e, float s)
		{
			auto fs = mc->getGlobalCodeFontSize();
			auto factor = fs / e.getFont().getHeight();
			e.setScaleFactor(factor);
		});

	if (isJavascript)
	{
		getEditor()->addBreakpointListener(this);

		auto& tp = mc->getJavascriptThreadPool();
		tp.addSleepListener(this);

		getEditor()->editor.onFocusChange = [mc, asComponent](bool isFocused, Component::FocusChangeType t)
		{
			if (isFocused)
				mc->setLastActiveEditor(CommonEditorFunctions::as(asComponent), CommonEditorFunctions::getCaretPos(asComponent));
		};
        
        getEditor()->editor.setGotoFunction(BIND_MEMBER_FUNCTION_2(::hise::PopupIncludeEditor::jumpToFromShortcut));
	}

	WeakReference<ApiProviderBase::Holder> safeHolder = dynamic_cast<ApiProviderBase::Holder*>(jp.get());

	ed.addPopupMenuFunction([safeHolder](mcl::TextEditor& ed, PopupMenu& m, const MouseEvent& e)
	{
		safeHolder->addPopupMenuItems(m, &ed, e);
	},
	[safeHolder](mcl::TextEditor& ed, int result)
	{
		if (safeHolder->performPopupMenuAction(result, &ed))
			return true;

		return false;
	});

	ed.addKeyPressFunction([this](const KeyPress& k)
	{
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::add_autocomplete_template))
		{
			jp->performPopupMenuAction(JavascriptProcessor::ScriptContextActions::AddAutocompleteTemplate, getEditor());
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::clear_autocomplete_templates))
		{
			jp->performPopupMenuAction(JavascriptProcessor::ScriptContextActions::ClearAutocompleteTemplates, getEditor());
			return true;
		}
		if (k == KeyPress::F9Key)
		{
			bottomBar->resultLabel->gotoText();
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::breakpoint_resume))
		{
			dynamic_cast<Processor*>(jp.get())->getMainController()->getJavascriptThreadPool().resume();
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::show_full_search))
		{
			jp->performPopupMenuAction(JavascriptProcessor::ScriptContextActions::FindAllOccurences, getEditor());
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::show_search_replace))
		{
			jp->performPopupMenuAction(JavascriptProcessor::ScriptContextActions::SearchAndReplace, getEditor());
			return true;
		}

		return false;
	});

	getEditor()->loadSettings(getPropertyFile());
#else
	asComponent->addAndMakeVisible(editor = new JavascriptCodeEditor(d, tokeniser, sp, callback));
#endif
}

EditorBottomBar::EditorBottomBar(JavascriptProcessor* jp):
	ControlledObject(dynamic_cast<Processor*>(jp)->getMainController())
{
	addAndMakeVisible(resultLabel = new DebugConsoleTextEditor("messageBox", dynamic_cast<Processor*>(jp)));

	addAndMakeVisible(compileButton = new TextButton("new button"));
	compileButton->setButtonText(TRANS("Compile"));
	compileButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	compileButton->addListener(this);
	compileButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);

	addAndMakeVisible(resumeButton = new TextButton("new button"));
	resumeButton->setButtonText(TRANS("Resume"));
	resumeButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	resumeButton->addListener(this);
	resumeButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);
	resumeButton->setVisible(false);

	addAndMakeVisible(errorButton = new HiseShapeButton("error", this, factory));

	errorButton->setVisible(false);

	auto offColour = Colour(HISE_ERROR_COLOUR).withMultipliedBrightness(1.6f);

	errorButton->setColours(offColour.withMultipliedAlpha(0.75f), offColour, offColour);

	errorButton->setTooltip("Navigate to the code position that causes the compiliation error.");
	
	compileButton->setLookAndFeel(&blaf);
	resumeButton->setLookAndFeel(&blaf);

	setOpaque(true);
}

void EditorBottomBar::resized()
{
	auto b = getLocalBounds();
	compileButton->setBounds(b.removeFromRight(75));

	if (errorButton->isVisible())
		errorButton->setBounds(b.removeFromLeft(35).reduced(2).translated(0, 1));

	if (resumeButton->isVisible())
		resumeButton->setBounds(b.removeFromRight(75));

	resultLabel->setBounds(b);
}

void EditorBottomBar::buttonClicked(Button* b)
{
	if (b == resumeButton)
	{
		getMainController()->getJavascriptThreadPool().resume();
	}
	if (b == compileButton)
	{
		getMainController()->getJavascriptThreadPool().resume();
		recompile();
	}
	if (b == errorButton)
	{
		resultLabel->gotoText();
		getMainController()->getLastActiveEditor()->grabKeyboardFocusAsync();
	}
}

void EditorBottomBar::recompile()
{
	resultLabel->startCompilation();
	compileFunction();
}

void EditorBottomBar::setShowResume(bool on, int lineNumber)
{
	resumeButton->setVisible(on);

	if (on)
		resultLabel->setText("Breakpoint at line " + String(lineNumber), dontSendNotification);

	resized();
}

void EditorBottomBar::setError(const String& errorMessage)
{
	lastCompileOk = errorMessage.isEmpty();
	startTimer(200);

	auto m = errorMessage.upToFirstOccurrenceOf("{", false, false).upToFirstOccurrenceOf("\n", false, false);

	if (errorMessage.isEmpty())
		m = "Compiled OK";

	if (resultLabel != nullptr)
		resultLabel->setText(m, dontSendNotification);
}

void EditorBottomBar::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF333333));
	auto b = getLocalBounds();
	GlobalHiseLookAndFeel::drawFake3D(g, b);
}

void EditorBottomBar::timerCallback()
{
	errorButton->setVisible(!lastCompileOk);
	resultLabel->setOK(lastCompileOk);
	repaint();
	resized();
	stopTimer();
}

} // namespace hise
