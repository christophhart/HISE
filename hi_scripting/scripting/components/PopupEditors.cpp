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
	Processor *p = dynamic_cast<Processor*>(jp.get());
	externalFile = p->getMainController()->getExternalScriptFile(fileToEdit);

	auto isJavascript = !externalFile->getFile().hasFileExtension("glsl");

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
	
	auto& d = *jp->getSnippet(callback_);
	addEditor(d, true);
	addButtonAndCompileLabel();

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
        
        
		return new JavascriptTokeniser();
	}

    
    
    bool getInplaceDebugValues(Array<InplaceDebugValue>& values) const override
    {
		values.addArray(jp->inplaceValues);
        
        return true;
    }
    
	virtual void processBookmarkTitle(juce::String& t)
	{
		
	}

	/** Add all token providers you want to use for this language. */
	void addTokenProviders(mcl::TokenCollection* t)
	{
		t->addTokenProvider(new HiseJavascriptEngine::TokenProvider(jp));
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

	void addTokenProviders(mcl::TokenCollection* t)
	{
		t->addTokenProvider(new GLSLKeywordProvider());
	}
};

void PopupIncludeEditor::addButtonAndCompileLabel()
{
	addAndMakeVisible(resultLabel = new DebugConsoleTextEditor("messageBox", dynamic_cast<Processor*>(jp.get())));

	addAndMakeVisible(compileButton = new TextButton("new button"));
	compileButton->setButtonText(TRANS("Compile"));
	compileButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	compileButton->addListener(this);
	compileButton->setColour(TextButton::buttonColourId, Colour(0xa2616161));

	addAndMakeVisible(resumeButton = new TextButton("new button"));
	resumeButton->setButtonText(TRANS("Resume"));
	resumeButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	resumeButton->addListener(this);
	resumeButton->setColour(TextButton::buttonColourId, Colour(0xa2616161));
	resumeButton->setVisible(false);

	setSize(800, 800);
}



void PopupIncludeEditor::refreshAfterCompilation(const JavascriptProcessor::SnippetResult& r)
{
	lastCompileOk = r.r.wasOk();
	resultLabel->setColour(TextEditor::ColourIds::backgroundColourId, Colours::white);
	resultLabel->setColour(TextEditor::ColourIds::textColourId, Colours::white);

	if (auto asmcl = dynamic_cast<mcl::FullEditor*>(editor.get()))
	{
		if (!lastCompileOk)
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
				startTimer(200);
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

	startTimer(200);
}

PopupIncludeEditor::~PopupIncludeEditor()
{
	if (jp != nullptr && editor != nullptr)
	{
		auto& doc = editor->editor.getDocument();
		auto pos = editor->editor.getTextDocument().getSelection(0).head;

        CodeDocument::Position p(doc, pos.x, pos.y);
        
		jp->setWatchedFilePosition(p);
	}

	editor = nullptr;
	resultLabel = nullptr;

	tokeniser = nullptr;
	compileButton = nullptr;

	Processor *p = dynamic_cast<Processor*>(jp.get());

	
	p = nullptr;

	externalFile = nullptr;
}

void PopupIncludeEditor::timerCallback()
{
	resultLabel->setColour(TextEditor::backgroundColourId, lastCompileOk ? Colours::green.withBrightness(0.1f) : Colours::red.withBrightness((0.1f)));
	repaint();
	stopTimer();
}



bool PopupIncludeEditor::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::F5Key) && !key.getModifiers().isShiftDown())
	{
		compileInternal();
		return true;
	}

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

#if HISE_USE_NEW_CODE_EDITOR
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

				t.lastCompileOk = false;
				t.startTimer(200);

				if (t.resultLabel != nullptr)
					t.resultLabel->setText("GLSL Compile Error", dontSendNotification);
			}
		}

		asmcl->repaint();
	}
#endif
}

void PopupIncludeEditor::resized()
{
	bool isInPanel = findParentComponentOfClass<FloatingTile>() != nullptr;

	if(isInPanel)
		editor->setBounds(0, 0, getWidth(), getHeight() - 18);
	else
		editor->setBounds(0, 5, getWidth(), getHeight() - 23);

	auto b = getLocalBounds().removeFromBottom(18);

	compileButton->setBounds(b.removeFromRight(95));

	if (resumeButton->isVisible())
		resumeButton->setBounds(b.removeFromRight(95));

	resultLabel->setBounds(b);
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

void PopupIncludeEditor::buttonClicked(Button* b)
{
	if (b == resumeButton)
	{
		dynamic_cast<Processor*>(jp.get())->getMainController()->getJavascriptThreadPool().resume();
	}
	if (b == compileButton)
	{
		dynamic_cast<Processor*>(jp.get())->getMainController()->getJavascriptThreadPool().resume();
		compileInternal();
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

	compileButton->triggerClick();
}

void PopupIncludeEditor::paintOverChildren(Graphics& g)
{
	if (getEditor() == dynamic_cast<Processor*>(jp.get())->getMainController()->getLastActiveEditor())
	{
		g.setColour(Colour(SIGNAL_COLOUR));
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
}

File PopupIncludeEditor::getFile() const
{
	return externalFile != nullptr ? externalFile->getFile() : File();
}

void PopupIncludeEditor::compileInternal()
{
	if (externalFile != nullptr)
	{
		externalFile->getFile().replaceWithText(externalFile->getFileDocument().getAllContent());
		externalFile->getFileDocument().setSavePoint();
	}

	jp->compileScript(BIND_MEMBER_FUNCTION_1(PopupIncludeEditor::refreshAfterCompilation));

	if (auto asmcl = dynamic_cast<mcl::TextEditor*>(editor.get()))
	{
		asmcl->clearWarningsAndErrors();
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
#if HISE_USE_NEW_CODE_EDITOR
		getEditor()->setCurrentBreakline(on ? lineNumber : -1);
#endif
		resumeButton->setVisible(on);
		if (on)
			resultLabel->setText("Breakpoint at line " + String(lineNumber), dontSendNotification);

		resized();
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
		ed.setLanguageManager(new GLSLLanguageManager());
	}

	ed.setPopupLookAndFeel(new PopupLookAndFeel());

	auto mc = dynamic_cast<Processor*>(jp.get())->getMainController();

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
			resultLabel->gotoText();
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

} // namespace hise
