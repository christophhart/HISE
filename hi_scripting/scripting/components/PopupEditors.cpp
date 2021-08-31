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
	sp(s),
	callback(Identifier()),
	tokeniser(new JavascriptTokeniser())
{
	Processor *p = dynamic_cast<Processor*>(sp.get());

	externalFile = p->getMainController()->getExternalScriptFile(fileToEdit);
	
	addEditor(externalFile->getFileDocument(), !externalFile->getFile().hasFileExtension("glsl"));
	

#if 0
	{
		externalFile->getFileDocument()

		doc = new mcl::TextDocument();
		
		auto med = new mcl::TextEditor(*doc);

		

		editor = med;

		med->setLineRangeFunction(snex::debug::Helpers::createLineRanges);

		addAndMakeVisible(editor);

		externalFile->addRuntimeErrorListener(this);

	}
	else
	{
		const Identifier snippetId = Identifier("File_" + fileToEdit.getFileNameWithoutExtension());
		addAndMakeVisible(editor = new JavascriptCodeEditor(externalFile->getFileDocument(), tokeniser, s, snippetId));
	}
#endif

	addButtonAndCompileLabel();
}

PopupIncludeEditor::PopupIncludeEditor(JavascriptProcessor* s, const Identifier &callback_) :
	sp(s),
	callback(callback_),
	tokeniser(new JavascriptTokeniser())
{
	
	auto& d = *sp->getSnippet(callback_);
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
        for(const auto& spv: jp->inplaceValues)
        {
            if(spv.callback == callback)
            {
                values.add({{spv.lineNumber-1, 99}, spv.value});
            }
        }
        
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

void PopupIncludeEditor::addEditor(CodeDocument& d, bool isJavascript)
{
#if HISE_USE_NEW_CODE_EDITOR
	doc = new mcl::TextDocument(d);
	addAndMakeVisible(editor = new mcl::FullEditor(*doc));

	auto& ed = getEditor()->editor;

	if (isJavascript)
		ed.setLanguageManager(new JavascriptLanguageManager(sp, callback, ed));
	else
	{
		ed.setLanguageManager(new GLSLLanguageManager());
		
		if (externalFile != nullptr)
			externalFile->getRuntimeErrorBroadcaster().addListener(*this, runTimeErrorsOccured);
	}
	
	ed.setPopupLookAndFeel(new PopupLookAndFeel());

	auto mc = dynamic_cast<Processor*>(sp.get())->getMainController();

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

		getEditor()->editor.onFocusChange = [mc, this](bool isFocused, FocusChangeType t)
		{
			if (isFocused)
				mc->setLastActiveEditor(CommonEditorFunctions::as(this), CommonEditorFunctions::getCaretPos(this));
		};
	}

	WeakReference<ApiProviderBase::Holder> safeHolder = dynamic_cast<ApiProviderBase::Holder*>(sp.get());

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
		if (k == KeyPress::F8Key)
		{
			auto id = k.getModifiers().isCommandDown() ?
				JavascriptProcessor::ScriptContextActions::ClearAutocompleteTemplates :
				JavascriptProcessor::ScriptContextActions::AddAutocompleteTemplate;

			sp->performPopupMenuAction(id, getEditor());
			return true;
		}
		if (k == KeyPress::F10Key)
		{
			dynamic_cast<Processor*>(sp.get())->getMainController()->getJavascriptThreadPool().resume();
			return true;
		}
		if (k == KeyPress('f', ModifierKeys::shiftModifier | ModifierKeys::commandModifier, 'F'))
		{
			sp->performPopupMenuAction(JavascriptProcessor::ScriptContextActions::FindAllOccurences, getEditor());
			return true;
		}
		if (k == KeyPress('g', ModifierKeys::commandModifier, 'g'))
		{
			sp->performPopupMenuAction(JavascriptProcessor::ScriptContextActions::SearchAndReplace, getEditor());
			return true;
		}

		return false;
	});
#else
	addAndMakeVisible(editor = new JavascriptCodeEditor(d, tokeniser, sp, callback));
#endif
}

void PopupIncludeEditor::addButtonAndCompileLabel()
{
	addAndMakeVisible(resultLabel = new DebugConsoleTextEditor("messageBox", dynamic_cast<Processor*>(sp.get())));

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
	editor = nullptr;
	resultLabel = nullptr;

	tokeniser = nullptr;
	compileButton = nullptr;

	Processor *p = dynamic_cast<Processor*>(sp.get());

	sp = nullptr;
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

void PopupIncludeEditor::sleepStateChanged(const Identifier& id, int lineNumber, bool on)
{
	if (callback == id)
	{
		resumeButton->setVisible(on);

#if HISE_USE_NEW_CODE_EDITOR
		if (on)
			getEditor()->setCurrentBreakline(lineNumber);
		else
			getEditor()->setCurrentBreakline(-1);
#endif

		if (on)
		{
			resultLabel->setText("Breakpoint at line " + String(lineNumber), dontSendNotification);
		}

		resized();
	}
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
		dynamic_cast<Processor*>(sp.get())->getMainController()->getJavascriptThreadPool().resume();
	}
	if (b == compileButton)
	{
		dynamic_cast<Processor*>(sp.get())->getMainController()->getJavascriptThreadPool().resume();
		compileInternal();
	}
}



void PopupIncludeEditor::breakpointsChanged(mcl::GutterComponent& g)
{
	sp->clearPreprocessorFunctions();

	auto cb = callback;

	sp->addPreprocessorFunction([&g, cb](const Identifier& id, String& s)
		{
			if (id == cb)
				return g.injectBreakPoints(s);

			return false;
		});

	compileButton->triggerClick();
}

void PopupIncludeEditor::paintOverChildren(Graphics& g)
{
	if (getEditor() == dynamic_cast<Processor*>(sp.get())->getMainController()->getLastActiveEditor())
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.fillRect(0, 0, 5, 5);
		
	}
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

	sp->compileScript(BIND_MEMBER_FUNCTION_1(PopupIncludeEditor::refreshAfterCompilation));

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

} // namespace hise
