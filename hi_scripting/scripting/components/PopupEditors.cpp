#include "PopupEditors.h"
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

	addButtonAndCompileLabel();
	refreshAfterCompilation(s->getLastErrorMessage());

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

	jp->addUriBreakpointListener(this);
	jp->errorProvider.addListener(this, true);
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
    
	jp->addUriBreakpointListener(this);
	jp->errorProvider.addListener(this, true);

	refreshAfterCompilation(s->getLastErrorMessage());
}

struct JavascriptLanguageManager : public mcl::LanguageManager
{
	JavascriptLanguageManager(JavascriptProcessor* jp_, const UriLocation& location_, mcl::TextEditor& ed):
		jp(jp_),
        location(location_)
	{
		jp->heatmapManager.heatmapBroadcaster.addListener(ed, [](mcl::TextEditor& ed, DebugInformationBase::Ptr info, const std::map<int, double>* heatmap)
		{
			ed.setHeatMap(info, heatmap);
		});

        jp->inplaceBroadcaster.addListener(ed, [](mcl::TextEditor& ed, Identifier, int)
        {
			ed.rebuildInplaceDebugValues();
        });
    };

	CodeTokeniser* createCodeTokeniser() override 
	{
        auto js = new JavascriptTokeniser();
		js->setUseScopeStatements(true);
		return js;
	}

    Identifier getLanguageId() const override { return mcl::LanguageIds::HiseScript; }
    
    bool getInplaceDebugValues(InplaceDebugValue::List& values) const override
    {
		auto sn = jp->getSnippetOrExternalFile(location);
		
		for(auto& ip: jp->inplaceValues)
		{
			ip->init();

			if(ip->location.getOwner() == sn)
			{
				values.add(ip);
			}
		}

        return true;
    }
    
	virtual void processBookmarkTitle(juce::String& t)
	{
		
	}

	UriLocation createUriLocation(const CodeDocument::Position& pos) const override
	{
		UriLocation loc;
		loc.fillLocation(location);
		loc.fillLocation(pos);
		return loc;
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
			ADD_HS_SNIPPET("smp (...)", "Console.sample($ID$, $VALUE$);", "A shortcut for the debug session sampling command");
			ADD_HS_SNIPPET("pr (...)", "..if(PROFILE):profile(\"$TEXT$\");", "A shortcut for adding scoped profiling code");

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

	UriLocationTree::Ptr getScopeFromLocation(const CodeDocument::Position& pos) const override
	{
		if (lastScopeTree != nullptr)
		{
			auto l = createUriLocation(pos);
			auto t = lastScopeTree->getInnerTree(l);

			if(auto functionTree = t->getThisOrParentWithCategory({ Identifier("function"), Identifier("inline") }))
				return functionTree.get();

			if (t->category == Identifier("loop"))
				return t->getParent().get();
			
			return t;
		}

		return nullptr;
	}

	mcl::FoldableLineRange::List createLineRange(const juce::CodeDocument& doc) override
	{
		if (lastScopeTree != nullptr)
		{
			return mcl::FoldableLineRange::Helpers::fromLocationTree(doc, lastScopeTree);
		}

		return LanguageManager::createLineRange(doc);
	}


	UriLocationTree::Ptr lastScopeTree;

	WeakReference<JavascriptProcessor> jp;
	UriLocation location;
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

void PopupIncludeEditor::refreshAfterCompilation(const UriError& e)
{
    checkUnreferencedExternalFile();
}

PopupIncludeEditor::~PopupIncludeEditor()
{
    Processor *sp = dynamic_cast<Processor*>(jp.get());
    
	if (jp != nullptr)
	{
		jp->errorProvider.removeListener(this);
		jp->removeUriBreakpointListener(this);
	}

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

void PopupIncludeEditor::onUriBreakpointEvent(const UriBreakpoint& knownEvent, const UriError& breakpoint)
{
	bottomBar->setError(breakpoint);

	if (editor->editor.matchesUriLocation(breakpoint) || breakpoint.wasOk())
	{
		SafeAsyncCall::call<PopupIncludeEditor>(*this, [breakpoint](PopupIncludeEditor& pe)
		{
			if (breakpoint.errorType == UriError::ErrorType::Blink)
			{
				pe.editor->editor.getGutterComponent().sendBlinkMessage(breakpoint.line);
				return;
			}

			pe.editor->setCurrentBreakline(breakpoint);

			if (!breakpoint.wasOk())
			{
				pe.editor->editor.setTokenTooltipFunction([&pe](mcl::TooltipWithArea::Data& d)
				{
					pe.onBreakpointTooltip(d);
				});

				PopupIncludeEditor::gotoUriLocation(pe.jp, pe.editor.get(), breakpoint);
			}
			else
			{
				pe.editor->editor.setTokenTooltipFunction({});
			}
		});
	}
	else
	{
		std::vector<UriLocation::WithExpression> callstackMatches;

		for (const auto& cs : breakpoint.callstack)
		{
			if (editor->editor.matchesUriLocation(cs.second))
				callstackMatches.push_back(cs);
		}

		if (!callstackMatches.empty())
			editor->editor.getGutterComponent().setExternalCallstackMatches(callstackMatches);

		if (!breakpoint.wasOk() && breakpoint.errorMessage.startsWith("Step"))
		{
			SafeAsyncCall::call<PopupIncludeEditor>(*this, [breakpoint](PopupIncludeEditor& pe)
			{
				PopupIncludeEditor::gotoUriLocation(pe.jp, pe.editor.get(), breakpoint);
			});
		}
	}
}

void PopupIncludeEditor::onError(const UriError& e, int numOccurrences)
{
	auto s = e.sourceType;
	
	jassert((s & UriError::SourceType::Runtime)     ||
	        (s & UriError::SourceType::Diagnostics) ||
			(s & UriError::SourceType::Debug)       ||
			(s & UriError::SourceType::Compilation) ||
			(s & UriError::SourceType::Include));

	auto shouldProcess = editor->editor.matchesUriLocation(e) || e.wasOk();

	shouldProcess &= numOccurrences == 1;
	
	if (shouldProcess)
	{
		SafeAsyncCall::call<PopupIncludeEditor>(*this, [e](PopupIncludeEditor& pe)
		{
			pe.bottomBar->setError(e);

			if (!e.wasOk())
				pe.editor->editor.addUriError(e);
		});
	}
}

void PopupIncludeEditor::onClear(UriErrorLogger::SourceType s)
{
	jassert((s & UriError::SourceType::Runtime)     ||
	        (s & UriError::SourceType::Diagnostics) ||
			(s & UriError::SourceType::Debug)       ||
			(s & UriError::SourceType::Compilation) ||
			(s & UriError::SourceType::Include));

	

	SafeAsyncCall::call<PopupIncludeEditor>(*this, [s](PopupIncludeEditor& pe)
	{
		if (s & UriError::SourceType::Compilation)
			pe.editor->editor.refreshBreakpoints();

		pe.bottomBar->clearError(s);
		pe.editor->editor.clearWarningsAndErrors(s);
	});
}

void PopupIncludeEditor::onLocationTree(UriLocationTree::Ptr newTree)
{
	if (newTree != nullptr)
	{
		if (auto st = newTree->getSubTree(baseLocation, false))
		{
			if (auto l = dynamic_cast<JavascriptLanguageManager*>(editor->editor.getLanguageManager()))
			{
				l->lastScopeTree = st;

				SafeAsyncCall::call<mcl::TextEditor>(editor->editor, [](mcl::TextEditor& ed)
				{
					ed.updateLineRanges();
				});
			}
		}
	}
}



bool PopupIncludeEditor::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::F5Key) && !key.getModifiers().isShiftDown())
	{
		bottomBar->recompile();
		return true;
	}
    
	if (TopLevelWindowWithKeyMappings::matches(this, key, TextEditorShortcuts::goto_file))
	{
		jassertfalse;
		return true;
	}

#if USE_BACKEND
	if (TopLevelWindowWithKeyMappings::matches(this, key, TextEditorShortcuts::shadow_parse))
	{
		shadowParse();
		return true;
	}
#endif

	return false;
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

struct BreakpointDebugTooltip : public mcl::TooltipWithArea::ComponentWithData,
								public PathFactory
								
{
	struct RowData
	{
		String path;
		String value;
		Colour extraColour;
		int valuePadding = 0;

		int getMessageWidth() const
		{
			String message;

			int w = 0;

			if (valuePadding != 0)
			{
				w = valuePadding;
				message << value;
			}
			else
				message << "path: " << value;
				
			w += GLOBAL_MONOSPACE_FONT().getStringWidth(message);
			
			if (!extraColour.isTransparent())
				w += 24;

			w += 24;

			return w;
		}

		int getPathWidth() const
		{
			return GLOBAL_MONOSPACE_FONT().getStringWidth(path + ": ");
		}

		void paint(Graphics& g, Rectangle<float> b)
		{
			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.setColour(Colour(0xFF444444));

			if (valuePadding)
			{
				g.drawText(path + ":", b.removeFromLeft(valuePadding), Justification::left);
			}
			else
			{
				auto thisPadding = getPathWidth();
				g.drawText(path + ":", b.removeFromLeft(thisPadding), Justification::left);
			}

			g.setColour(Colour(0xFF222222));
			g.drawText(value, b, Justification::left);
			
			if(!extraColour.isTransparent())
			{
				b = b.removeFromRight(b.getHeight());
				g.setColour(extraColour);
				g.fillRect(b.reduced(4));
				g.setColour(Colours::black);
				g.drawRect(b.reduced(4), 1);
			}
		}
	};

	static bool isComplexItem(const var& obj)
	{
		auto useComplexViewer = false;

		useComplexViewer |= obj.isArray();
		useComplexViewer |= obj.getDynamicObject() != nullptr;
		useComplexViewer |= dynamic_cast<DebugableObjectBase*>(obj.getObject()) != nullptr;

		return useComplexViewer;
	}

	static RowData toDisplayString(const String& path, const var& value)
	{
		RowData rd;

		auto f = GLOBAL_MONOSPACE_FONT();

		auto pathColour = Colour(0xFF555555);
		auto valueColour = Colour(0xFF222222);

		rd.path = path.replace("%PARENT%", "");
		
		if (auto obj = dynamic_cast<DebugableObjectBase*>(value.getObject()))
		{
			String v;
			v << obj->getDebugName() << " - ";
			v << obj->getDebugValue();
			v << " (" << obj->getDebugDataType() << ")";
			rd.value = v;
		}
		else
		{
			String v;

			if (value.isString())
				v << value.toString().quoted() << " (String)";
			else if (value.isArray())
				v << "Array[" << String(value.size()) << "]";
			else if (auto obj = value.getDynamicObject())
				v << "JSON";
			else
			{
				if (path.toLowerCase().contains("colour"))
				{
					auto c = ApiHelpers::getColourFromVar(value);
					v << "#" << c.toDisplayString(true);
					rd.extraColour = c;
				}
				else
				{
					v << value.toString();
					auto wt = VarTypeChecker::getType(value);
					v << " (" << VarTypeChecker::getTypeName(wt) << ")";
				}
			}

			rd.value = v;
		}

		return rd;
	}

	struct VarTreeViewItem : public juce::TreeViewItem
	{
		VarTreeViewItem(Component* rootComponent_, const var& value_, const String& path_) :
		  TreeViewItem(),
		  value(value_),
		  path(path_),
		  rootComponent(rootComponent_)
		{
			message = toDisplayString(path, value);
		}

		bool mightContainSubItems() override
		{
			return isComplexItem(value);
		}

		String getUniqueName() const override
		{
			return path;
		}

		int getItemWidth() const override
		{
			return message.getMessageWidth();
		}

		void itemOpennessChanged(bool isNowOpen) override;

		void paintItem(Graphics& g, int width, int height) override
		{
			Rectangle<int> b(0, 0, width, height);
			message.paint(g, b.toFloat());
		}

		RowData message;
		String path;
		var value;

		Component* rootComponent;
	};

	struct Content : public Component,
					 public UriErrorLogger::Listener
	{
		Content(BreakpointDebugTooltip& parent, JavascriptProcessor* jp_) :
			data(parent.data),
			jp(jp_),
			r(Result::ok())
		{
			jp->errorProvider.addListener(this, false);

			triggerEvaluation();
			setSize(150, 24);

			setInterceptsMouseClicks(false, true);
		};

		~Content()
		{
			jp->errorProvider.removeListener(this);
		}

		void triggerEvaluation()
		{
			auto p = dynamic_cast<Processor*>(this->jp.get());
			Component::SafePointer<Content> safeThis(this);
			auto taskType = JavascriptThreadPool::Task::Type::ReplEvaluation;

			auto f = GLOBAL_MONOSPACE_FONT();
			
			message = {};
			message.path = "pending";

			p->getMainController()->getJavascriptThreadPool().addJob(taskType, jp,
			[safeThis](JavascriptProcessor* p)
			{
				if (safeThis.getComponent() != nullptr)
				{
					HiseJavascriptEngine* engine = p->getScriptEngine();
					safeThis->evaluatedValue = engine->evaluate(safeThis->data.text, &safeThis->r);

					SafeAsyncCall::call<Content>(*safeThis, [](Content& c)
					{
						c.onEvaluation();
					});
				}

				return UriError::ok();
			});
		}

		void resizeTreeView()
		{
			if (complexViewer != nullptr)
			{
				auto w = message.getMessageWidth();
				
				auto vw = complexViewer->getViewport()->getViewedComponent()->getWidth();
				auto vh = complexViewer->getViewport()->getViewedComponent()->getHeight();

				auto h = 24;
				h = jmin(h + vh, 400);
				w = jlimit(w, 800, vw);

				getParentComponent()->setSize(w + 24, h);
				repaint();
			}
		}

		void onError(const UriError& e, int numOccurrences) override
		{
			if (e.sourceType == UriError::SourceType::Debug &&
				e.errorType == UriError::ErrorType::Breakpoint)
			{
				triggerEvaluation();
			}
		}

		void onClear(UriError::SourceType s) override
		{
			if (s == UriError::SourceType::Debug)
			{
				triggerEvaluation();
			}
		}

		void resized() override
		{
			if (complexViewer != nullptr)
			{
				auto b = getLocalBounds();
				b.removeFromTop(24);
				complexViewer->setBounds(b);
			}
		}

		void onEvaluation()
		{
			if (r.wasOk())
			{
				message = toDisplayString(data.text, evaluatedValue);

				int h = 24;

				auto useComplexViewer = isComplexItem(evaluatedValue);

				if (useComplexViewer)
				{
					if (complexViewer == nullptr)
					{
						addAndMakeVisible(complexViewer = new TreeView());
						complexViewer->setRootItemVisible(false);
						complexViewer->setMultiSelectEnabled(false);
					}
					
					auto state = complexViewer->getOpennessState(true);

					complexViewer->setRootItem(new VarTreeViewItem(this, evaluatedValue, data.text));

					if(state != nullptr)
						complexViewer->restoreOpennessState(*state, false);

					resizeTreeView();
				}
				else
				{
					auto w = message.getMessageWidth();
					getParentComponent()->setSize(w + 24 + 50, h);
					repaint();
				}
			}
		}

		void paint(Graphics& g) override
		{
			auto b = getLocalBounds().toFloat();
			b = b.removeFromTop(24.0f);
			b.removeFromLeft(20.0f);
			message.paint(g, b);
		}

		RowData message;

		mcl::TooltipWithArea::Data data;
		WeakReference<JavascriptProcessor> jp;
		Result r;
		bool pending = true;
		var evaluatedValue;

		ScopedPointer<juce::TreeView> complexViewer;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Content);
	};

	struct PersistentParent : public mcl::TooltipWithArea::ComponentWithData
	{
		PersistentParent(ScopedPointer<Content>& t):
		  ComponentWithData(t->data),
		  closeButton("close", nullptr, *t->findParentComponentOfClass<BreakpointDebugTooltip>())
		{
			content = t.release();
			auto oldParent = content->getParentComponent();
			oldParent->removeChildComponent(content);
			addChildComponent(content);
			setSize(content->getWidth() + 24, content->getHeight());

			addAndMakeVisible(closeButton);
			closeButton.setColours(Colours::black.withAlpha(0.5f),
								   Colours::black.withAlpha(0.7f),
								   Colours::black.withAlpha(0.8f));

			closeButton.onClick = [this]()
			{
				auto p = findParentComponentOfClass<mcl::TextEditor>();
				p->removeChildComponent(this);
				delete this;
			};
		}

		void paint(Graphics& g) override
		{
			this->drawBackground(g);
		}

		void mouseDown(const MouseEvent& e) override
		{
			dragger.startDraggingComponent(this, e);
		}

		void mouseDrag(const MouseEvent& e) override
		{
			dragger.dragComponent(this, e, nullptr);
		}

		void resized() override
		{
			auto b = getLocalBounds();
			auto topRight = b.removeFromRight(24).removeFromTop(24).reduced(4);
			closeButton.setBounds(topRight);

			if (content != nullptr)
				content->setBounds(b);
		}

		HiseShapeButton closeButton;

		ScopedPointer<Content> content;

		ComponentDragger dragger;
	};

	BreakpointDebugTooltip(const mcl::TooltipWithArea::Data& d, JavascriptProcessor* jp_):
		ComponentWithData(d),
		stickyButton("pin", nullptr, *this),
		content(new Content(*this, jp_))
	{
		addAndMakeVisible(content);
		addAndMakeVisible(stickyButton);
		stickyButton.setColours(Colours::black.withAlpha(0.5f), 
								Colours::black.withAlpha(0.7f), 
								Colours::black.withAlpha(0.8f));

		stickyButton.onClick = [this]()
		{
			auto p = findParentComponentOfClass<mcl::TextEditor>();

			auto nc = new PersistentParent(content);
			
			auto pos = getPosition();
			
			p->addAndMakeVisible(nc);
			nc->setTopLeftPosition(pos);
			
			p->clearTooltip();
		};

		setSize(content->getWidth() + 24, 24);
	}

	Path createPath(const String& url) const override
	{
		Path p;
		LOAD_EPATH_IF_URL("pin", ScriptnodeIcons::pinIcon);
		LOAD_EPATH_IF_URL("close", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
		return p;
	}

	void resized() override
	{
		auto b = getLocalBounds();
		auto topRight = b.removeFromRight(24).removeFromTop(24).reduced(4);
		stickyButton.setBounds(topRight);

		if(content != nullptr)
			content->setBounds(b);
	}

	void paint(Graphics& g) override
	{
		this->drawBackground(g);
	}

	ScopedPointer<Content> content;
	HiseShapeButton stickyButton;

	
};

void BreakpointDebugTooltip::VarTreeViewItem::itemOpennessChanged(bool isNowOpen)
{
	if (isNowOpen)
	{
		clearSubItems();

		int valuePadding = 0;

		if (value.isArray())
		{
			for (int i = 0; i < value.size(); i++)
			{
				auto c = value[i];
				auto cp = path + "[" + String(i) + "]";
				auto ni = new VarTreeViewItem(rootComponent, c, cp);
				valuePadding = jmax(valuePadding, ni->message.getPathWidth());
				addSubItem(ni);
			}
		}
		else if (auto obj = value.getDynamicObject())
		{
			for (auto& nv : obj->getProperties())
			{
				auto ni = new VarTreeViewItem(rootComponent, nv.value, path + "." + nv.name.toString());
				valuePadding = jmax(valuePadding, ni->message.getPathWidth());
				addSubItem(ni);
			}
				
		}
		else if (auto dobj = dynamic_cast<DebugableObjectBase*>(value.getObject()))
		{
			auto numDebugObjects = dobj->getNumChildElements();

			for (int i = 0; i < numDebugObjects; i++)
			{
				auto di = dobj->getChildElement(i);
				auto n = di->getTextForName();
				auto v = di->getVariantCopy();
				auto ni = new VarTreeViewItem(rootComponent, v, path + "." + n);
				valuePadding = jmax(valuePadding, ni->message.getPathWidth());
				addSubItem(ni);
			}
		}

		for (int i = 0; i < getNumSubItems(); i++)
			dynamic_cast<VarTreeViewItem*>(getSubItem(i))->message.valuePadding = valuePadding;
	}

	if (auto r = dynamic_cast<Content*>(rootComponent))
	{
		SafeAsyncCall::callAsyncIfNotOnMessageThread<Content>(*r, [](Content& c)
		{
			c.resizeTreeView();
		});
	}
}

void PopupIncludeEditor::onBreakpointTooltip(mcl::TooltipWithArea::Data& d)
{
	d.customComponentFunction = [this](const mcl::TooltipWithArea::Data& d)
	{
		mcl::TooltipWithArea::ComponentWithData* c;

		auto canBeCustomDisplay = d.text == "this";
		canBeCustomDisplay |= !JavascriptTokeniser::isReservedKeyword(d.text);
		canBeCustomDisplay &= !d.text.containsChar('(');

		if (canBeCustomDisplay)
			c = new BreakpointDebugTooltip(d, jp.get());
		else
			c = new mcl::TooltipWithArea::Display(d);
		
		return c;
	};
}



bool PopupIncludeEditor::gotoUriLocation(UriLocation::Owner* owner, Component* c, const UriLocation& l)
{
	auto resolveState = l.getResolveState();

	if (!(resolveState & UriLocation::ResolveState::PathResolved))
		return false;

	

	mcl::FullEditor* activeEditor = nullptr;

	if (auto t = dynamic_cast<mcl::FullEditor*>(c))
		activeEditor = t;

	if (c == nullptr)
	{
		if (auto p = dynamic_cast<Processor*>(owner))
		{
			activeEditor = dynamic_cast<mcl::FullEditor*>(p->getMainController()->getLastActiveEditor());
		}
	}

	if (activeEditor == nullptr && c != nullptr)
	{
		if (auto brw = GET_BACKEND_ROOT_WINDOW(c))
		{
			activeEditor = dynamic_cast<mcl::FullEditor*>(brw->getBackendProcessor()->getLastActiveEditor());
		}
	}
	
	if (activeEditor != nullptr)
	{
		if (activeEditor->editor.gotoLocation(l))
			return true;

		auto brw = activeEditor->findParentComponentOfClass<ComponentWithBackendConnection>();

		FloatingTile* root = nullptr;

		if (brw != nullptr)
			root = brw->getBackendRootWindow()->getRootFloatingTile();


		if (root != nullptr)
		{
			FloatingTabComponent* codeTabs = nullptr;

			if (auto ptile = activeEditor->findParentComponentOfClass<FloatingTile>())
			{
				codeTabs = ptile->findParentTileWithType<FloatingTabComponent>();
			}

			if (codeTabs == nullptr)
			{
				Component::callRecursive<FloatingTabComponent>(root, [&](FloatingTabComponent* ed)
				{
					auto id = ed->getParentShell()->getLayoutData().getID();

					if (id == Identifier("ScriptEditorTabs"))
					{
						codeTabs = ed;
						return true;
					}

					return false;
				});
			}

			if (codeTabs != nullptr)
			{
				for (int i = 0; i < codeTabs->getNumTabs(); i++)
				{
					auto tc = dynamic_cast<FloatingTile*>(codeTabs->getTabContentComponent(i));
					
					if (auto cep = dynamic_cast<CodeEditorPanel*>(tc->getCurrentFloatingPanel()))
					{
						if (auto pe = cep->getContent<PopupIncludeEditor>())
						{
							if (pe->editor->editor.gotoLocation(l))
							{
								pe->bottomBar->onLocationChange(l);

								codeTabs->setCurrentTabIndex(i);
								return true;
							}
						}
					}
				}

				FloatingInterfaceBuilder ib(codeTabs->getParentShell());
				auto newEditor = ib.addChild<CodeEditorPanel>(0);
				auto ed = ib.getContent<CodeEditorPanel>(newEditor);
				ib.finalizeAndReturnRoot();

				if (auto p = ProcessorHelpers::getFirstProcessorWithName(root->getMainController()->getMainSynthChain(), l.processorId))
				{
					ed->setCurrentProcessor(p);

					StringArray indexList;
					ed->fillIndexList(indexList);

					auto connectionId = l.toLocationString(UriLocation::UriFormat::ConnectionId);
					auto index = indexList.indexOf(connectionId);

					if (index != -1)
					{
						ed->setContentWithUndo(p, index);

						if (auto pe = ed->getContent<PopupIncludeEditor>())
						{
							if (pe->editor->editor.gotoLocation(l))
							{
								pe->bottomBar->onLocationChange(l);
								return true;
							}
						}
					}
				}
			}
		}
	}
	
	return false;
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
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::breakpoint_step_into, "Step into next statement", KeyPress(KeyPress::F11Key));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::breakpoint_step_over, "Step over next statement", KeyPress(KeyPress::F10Key));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::breakpoint_step_out, "Step out of statement", KeyPress(KeyPress::F11Key, ModifierKeys::shiftModifier, 0));
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::breakpoint_step_resume, "Resume execution", KeyPress(KeyPress::F9Key));

	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::goto_file, "Goto file", KeyPress('t', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 't'));
    
    TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::goto_undo, "Undo Goto", KeyPress(KeyPress::F12Key, ModifierKeys::commandModifier, 0));
    
    TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::goto_redo, "Redo Goto", KeyPress(KeyPress::F12Key, ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));

#if USE_BACKEND
	TopLevelWindowWithKeyMappings::addShortcut(root, cat, TextEditorShortcuts::shadow_parse, "Shadow Parse", KeyPress(KeyPress::F7Key));
#endif
}

void PopupIncludeEditor::blink(MainController* mc, const UriLocation& location)
{
	if (auto e = mc->getLastActiveEditor())
	{
		auto i = location.toLocationString(UriLocation::UriFormat::ConnectionId);
		auto l = location.line;

		MessageManager::callAsync([e, i, l]()
		{
			if (PopupIncludeEditor::matchesId(e, i))
			{
				CommonEditorFunctions::as(e)->sendBlinkMessage(l);
			}
		});
	}
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

		bottomBar->setError(UriError::fromResult(ok, jp));

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
				c->css.clearCache();

				using BD = ScriptingApi::Content::ScriptMultipageDialog::Backdrop;

				Component::callRecursive<Component>(c, [&](Component* child)
				{
					if(auto bd = dynamic_cast<BD*>(c))
					{
						bd->create(getEditor()->editor.getDocument().getAllContent());
					}
					else if(auto fb = dynamic_cast<simple_css::FlexboxComponent*>(child))
					{
						fb->setCSS(c->css);
					}

					child->resized();
					child->repaint();
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
    
	    auto f = [safeP](const UriError& r)
	    {
	        if(safeP.getComponent() != nullptr)
	            safeP->refreshAfterCompilation(r);
	    };
	    
		jp->compileScript(f);
	}

    

	if (auto asmcl = dynamic_cast<mcl::TextEditor*>(editor.get()))
	{
		asmcl->clearWarningsAndErrors();
	}
}

#if USE_BACKEND
void PopupIncludeEditor::shadowParse(bool printToConsole)
{
	String code;
	String fileName;
	
	if (printToConsole)
	{
		if (externalFile != nullptr)
		{
			fileName = externalFile->getFile().getFullPathName();
			auto mc = dynamic_cast<Processor*>(jp.get())->getMainController();
			mc->saveAllExternalFiles();
			code = externalFile->getFile().loadFileAsString();
		}
		else
		{
			fileName = "";
			code = editor->editor.getDocument().getAllContent();
		}
	}
	else
	{
		fileName = "";
		code = editor->editor.getDocument().getAllContent();
	}

	Component::SafePointer<PopupIncludeEditor> safeThis(this);

	DiagnosticBase::ShadowParseData sd;
	using Item = DiagnosticBase::ShadowParseData::Item;

	Item::Type t = Item::Type::onInit;

	if (callback.isValid() && callback != Identifier("onInit"))
	{
		t = Item::Type::Callback;
		fileName = callback.toString();
	}
	else if (externalFile != nullptr)
	{
		t = Item::Type::ExternalFile;
		fileName = externalFile->getFile().getFullPathName();
	}
		

	auto selection = editor->editor.getTextDocument().getSelection(0);

	CodeDocument::Position pos(editor->editor.getDocument(), selection.head.x, selection.head.y);

	int breakDelta = pos.getPosition();

	auto item = new Item(t, fileName, code);
	item->setBreakPosition(breakDelta);

	sd.items.add(item);

	jp->shadowParse(sd, [printToConsole, safeThis](const JavascriptProcessor::DiagnosticList& diagnostics)
	{
		if (safeThis.getComponent() == nullptr)
			return;

		auto& ed = safeThis->editor->editor;
		ed.clearWarningsAndErrors();

		using Severity = ApiClass::DiagnosticResult::Severity;

		auto p = dynamic_cast<Processor*>(safeThis->jp.get());

		int numWarnings = 0;
		int numErrors = 0;

		mcl::DiagnosticLines dl;

		auto matchesEditor = [safeThis](const UriLocation& loc)
		{
			auto fileName = safeThis->externalFile != nullptr ? safeThis->externalFile->getFile().getFullPathName() : safeThis->callback.toString();

			auto uri = loc.toLocationString(UriLocation::UriFormat::CallbackId);

			return uri == fileName;
		};

		for (const auto& d : diagnostics)
		{
			auto addToEditor = matchesEditor(d);
				
			auto editorMessage = d.toConsoleMessage();
			auto consoleMessage = d.toFullCallStack();

			auto sev = DiagnosticBase::DiagnosticResult::getSeverityFromMetadata(d.metadata);

			if (sev == Severity::Error)
			{
				if (addToEditor)
				{
					dl.errorLines.addIfNotAlreadyThere(d.line - 1);
					ed.addWarning(d, false);
				}

				if (printToConsole)
				{
					debugError(p, consoleMessage);
				}
				
				numErrors++;
			}
			else if (sev == Severity::Hint)
			{
				if (addToEditor)
				{
					ed.addHint(d);
					dl.hintLines.addIfNotAlreadyThere(d.line - 1);
				}

				if (printToConsole)
				{
					// Hints go to console only — no editor marker
					debugToConsole(p, consoleMessage);
					
				}
			}
			else
			{
				if (addToEditor)
				{
					dl.warningLines.addIfNotAlreadyThere(d.line - 1);
					ed.addWarning(d, true);
				}
				
				if (printToConsole)
				{
					debugToConsole(p, consoleMessage);
				}

				numWarnings++;
			}
		}

		ed.setDiagnostics(dl);
		
		// Update bottom bar with summary
		String summary;

		if (numErrors == 0 && numWarnings == 0)
			summary = "Shadow parse OK";
		else
		{
			if (numWarnings > 0)
				summary << String(numWarnings) << " warning" << (numWarnings > 1 ? "s" : "");

			if (numErrors > 0)
			{
				if (numWarnings > 0)
					summary << ", ";

				summary << String(numErrors) << " error" << (numErrors > 1 ? "s" : "");
			}
		}

		if (printToConsole)
			debugToConsole(p, "Shadow parse: " + summary);

		// setError("") shows "Compiled OK", setError(text) shows text in error style
		// For warnings-only we still want to show the count, so pass it as "error" text
		if (numErrors > 0 || numWarnings > 0)
			safeThis->bottomBar->setError(summary);
		else
			safeThis->bottomBar->setError("");
	}, printToConsole ? sendNotificationAsync : sendNotificationSync);
}
#endif

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

bool PopupIncludeEditor::matchesUriLocation(const UriLocation& loc) const
{
	if (externalFile != nullptr)
		return loc.matchesFile(externalFile->getFile());
	else
		return loc.toLocationString(UriLocation::UriFormat::CallbackId) == callback.toString();
}

bool PopupIncludeEditor::sleepStateChanged(const UriError& location, bool on)
{
	return false;
}

void PopupIncludeEditor::addEditor(CodeDocument& d, bool isJavascript)
{
	auto asComponent = dynamic_cast<Component*>(this);

#if HISE_USE_NEW_CODE_EDITOR
	doc = new mcl::TextDocument(d);

	asComponent->addAndMakeVisible(editor = new mcl::FullEditor(*doc));

	auto& ed = getEditor()->editor;

	if (isJavascript)
	{
		baseLocation.processorId = jp->getUriProcessorId();
		baseLocation.uri = callback.toString();

		if (externalFile != nullptr)
			baseLocation.uri = externalFile->getFile().getFullPathName();
		
		ed.setLanguageManager(new JavascriptLanguageManager(jp, baseLocation, ed));
	}
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
		editor->editor.getGutterComponent().setOwner(jp);

		auto& tp = mc->getJavascriptThreadPool();
		tp.addSleepListener(this);

		auto jp_ = jp;

		getEditor()->editor.onFocusChange = [mc, asComponent, jp_](bool isFocused, Component::FocusChangeType t)
		{
			if (isFocused)
			{
				jp_->checkOnFocusGain();
				mc->setLastActiveEditor(CommonEditorFunctions::as(asComponent), CommonEditorFunctions::getCaretPos(asComponent));
			}
		};
        
		getEditor()->editor.setGotoFunction([jp_](const UriLocation::WithExpression& l)
		{
			return jp_->jumpToDefinition(l);
		});
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
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::breakpoint_step_into))
		{
			jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::StepInto);
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::breakpoint_step_out))
		{
			jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::StepOut);
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::breakpoint_step_resume))
		{
			jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::Resume);
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::breakpoint_step_over))
		{
			jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::StepOver);
			return true;
		}
		if (TopLevelWindowWithKeyMappings::matches(getEditor(), k, TextEditorShortcuts::breakpoint_resume))
		{
			jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::Resume);
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

	if (isJavascript)
	{
		ed.setDiagnosticsFunction([this](bool shouldParse)
		{
			if(shouldParse)
				shadowParse();
			else if (bottomBar != nullptr)
			{
				bottomBar->setCurrentlyParsing();
			}
		});
	}

#else
	asComponent->addAndMakeVisible(editor = new JavascriptCodeEditor(d, tokeniser, sp, callback));
#endif
}

EditorBottomBar::EditorBottomBar(JavascriptProcessor* jp_):
	ControlledObject(dynamic_cast<Processor*>(jp_)->getMainController()),
	jp(jp_)
{
	addAndMakeVisible(resultLabel = new DebugConsoleTextEditor("messageBox", dynamic_cast<Processor*>(jp)));

	addAndMakeVisible(compileButton = new TextButton("new button"));
	compileButton->setButtonText(TRANS("Compile"));
	compileButton->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
	compileButton->addListener(this);
	compileButton->setColour(TextButton::buttonColourId, Colours::transparentBlack);

	addChildComponent(callstackSelector);
	callstackSelector.setLookAndFeel(&callstackLaf);
	callstackSelector.addListener(this);

	callstackSelector.setTooltip("Click to select callstack frame");
	
	addChildComponent(resumeButton = new HiseShapeButton("resume", this, factory));
	addChildComponent(overButton = new HiseShapeButton("over", this, factory));
	addChildComponent(nextButton = new HiseShapeButton("next", this, factory));

	auto bpColour = Colour(HISE_BREAKPOINT_COLOUR).withMultipliedBrightness(1.6f);
	auto offColour = Colour(HISE_ERROR_COLOUR).withMultipliedBrightness(1.6f);
	

	overButton->setTooltip("Step over next statement [F10]");
	nextButton->setTooltip("Step into next statement [F11]");
	resumeButton->setTooltip("Resume execution [F5]");

	overButton->setColours(bpColour.withMultipliedAlpha(0.75f), bpColour, bpColour);
	nextButton->setColours(bpColour.withMultipliedAlpha(0.75f), bpColour, bpColour);
	resumeButton->setColours(bpColour.withMultipliedAlpha(0.75f), bpColour, bpColour);

	addAndMakeVisible(errorButton = new HiseShapeButton("error", this, factory));

	errorButton->setVisible(false);

	errorButton->setColours(offColour.withMultipliedAlpha(0.75f), offColour, offColour);

	errorButton->setTooltip("Navigate to the code position that causes the compiliation error.");
	
	compileButton->setLookAndFeel(&blaf);
	resultLabel->setTooltip("Click to enter a REPL evaluation, then press enter to print result to console");

	setOpaque(true);
}

void EditorBottomBar::resized()
{
	auto b = getLocalBounds();
	compileButton->setBounds(b.removeFromRight(75));

	if (resumeButton->isVisible())
		resumeButton->setBounds(b.removeFromLeft(b.getHeight()).reduced(3));

	if (nextButton->isVisible())
		nextButton->setBounds(b.removeFromLeft(b.getHeight()).reduced(3));

	if (overButton->isVisible())
		overButton->setBounds(b.removeFromLeft(b.getHeight()).reduced(3));

	if (b.getX() > 0)
		b.removeFromLeft(5);

	if (errorButton->isVisible())
		errorButton->setBounds(b.removeFromLeft(35).reduced(2).translated(0, 1));

	if (callstackSelector.isVisible())
		callstackSelector.setBounds(b.removeFromLeft(callstackSelector.getWidth()));

	resultLabel->setBounds(b);
}

void EditorBottomBar::buttonClicked(Button* b)
{
	if (b == resumeButton)
	{
		jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::Resume);
	}
	if (b == nextButton)
	{
		jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::StepInto);
	}
	if (b == overButton)
	{
		jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::StepOver);
	}
	if (b == compileButton)
	{
		jp->resumeExecution(UriBreakpoint::Owner::NextBreakpointAction::Resume);
		recompile();
	}
	if (b == errorButton)
	{
		gotoLocation(getCurrentError());
		
		if(callstackSelector.isVisible())
			callstackSelector.setSelectedItemIndex(0, dontSendNotification);

		getMainController()->getLastActiveEditor()->grabKeyboardFocusAsync();
	}
}

void EditorBottomBar::comboBoxChanged(ComboBox* cb)
{
	auto idx = cb->getSelectedItemIndex();

	auto currentError = getCurrentError();

	if (idx == 0)
	{
		gotoLocation(currentError);
	}
	else
	{
		idx = cb->getSelectedId() - 2;

		if (isPositiveAndBelow(idx, currentError.callstack.size()))
			gotoLocation(currentError.callstack[idx].second);
	}
}

void EditorBottomBar::recompile()
{
	resultLabel->startCompilation();
	compileFunction();
}

void EditorBottomBar::clearError(UriError::SourceType s)
{
	for (auto& e : errors)
	{
		if (e.first & (int)s)
			e.second = {};
	}

	startTimer(200);
}

void EditorBottomBar::setError(const UriError& e)
{
	if (e.wasOk())
	{
		clearError(e.sourceType);
		return;
	}

	jassert(e.sourceType != UriError::SourceType::None);
	jassert(e.sourceType != UriError::SourceType::All);

	errors[e.sourceType] = e;
	startTimer(200);
}

void EditorBottomBar::setCurrentlyParsing()
{
	resultLabel->currentlyParsing = true;
	resultLabel->setText("Waiting for diagnostics...", dontSendNotification);
}

void EditorBottomBar::paint(Graphics& g)
{
	g.fillAll(Colour(0xFF333333));
	auto b = getLocalBounds();
	GlobalHiseLookAndFeel::drawFake3D(g, b);
}

void EditorBottomBar::timerCallback()
{
	auto currentError = getCurrentError();

	
	

	resultLabel->setError(currentError);
	resumeButton->setVisible(currentError.errorType == UriError::ErrorType::Breakpoint);
	nextButton->setVisible(resumeButton->isVisible());
	overButton->setVisible(resumeButton->isVisible());

	auto offColour = Colour(resumeButton->isVisible() ? HISE_BREAKPOINT_COLOUR : HISE_ERROR_COLOUR).withMultipliedBrightness(1.6f);
	errorButton->setColours(offColour.withMultipliedAlpha(0.75f), offColour, offColour);

	callstackSelector.setVisible(currentError.callstack.size() > 0);

	callstackSelector.clear(dontSendNotification);

	auto maxWidth = 0;

	if (!currentError.wasOk())
	{
		auto top = currentError.toLocationString(UriLocation::UriFormat::Compact);

		maxWidth = jmax<int>(maxWidth, GLOBAL_MONOSPACE_FONT().getStringWidthFloat(top));

		callstackSelector.addItem(top, 1);

		currentError.forInvertedCallstack([&](int index, const UriLocation::WithExpression& e)
		{
			auto i = e.second.toLocationString(UriLocation::UriFormat::Compact) + ": " + e.first;
			callstackSelector.addItem(i, index+2);
			maxWidth = jmax<int>(maxWidth, GLOBAL_MONOSPACE_FONT().getStringWidthFloat(i));
		});

		callstackSelector.setSize(maxWidth + getHeight(), getHeight());
		callstackSelector.setSelectedItemIndex(0, dontSendNotification);
	}

	errorButton->setVisible(!currentError.wasOk());
	
	repaint();
	resized();
	stopTimer();
}

UriError EditorBottomBar::getCurrentError() const
{
	auto getBestErrorWithType = [&](UriError::ErrorType t)
	{
		auto sourcePriority = {
			UriError::SourceType::Include,
			UriError::SourceType::Diagnostics,
			UriError::SourceType::Compilation,
			UriError::SourceType::Runtime,
			UriError::SourceType::Debug
		};

		for(auto s: sourcePriority)
		{
			if (errors.find(s) != errors.end())
			{
				auto& item = errors.at(s);
				if (item.errorType == t)
					return item;
			}
		}

		return UriError();
	};

	if (auto e = getBestErrorWithType(UriError::ErrorType::Error))
		return e;

	if (auto e = getBestErrorWithType(UriError::ErrorType::Breakpoint))
		return e;

	return UriError();
}

void hise::EditorBottomBar::gotoLocation(const UriLocation& l)
{
	if(l)
		PopupIncludeEditor::gotoUriLocation(nullptr, this, l);
}

Path EditorBottomBar::Factory::createPath(const String& url) const
{
	Path p;

	if (url == "resume")
	{
		static const unsigned char pathData[] = { 110,109,252,127,69,68,4,0,158,67,108,252,127,7,68,4,0,220,67,108,252,127,7,68,4,0,64,67,108,252,127,69,68,4,0,158,67,99,101,0,0 };
		p.loadPathFromData(pathData, sizeof(pathData));
	}

	if (url == "next")
	{
		static const unsigned char pathData[] = { 110,109,0,128,69,68,144,34,205,67,108,0,128,69,68,88,117,219,67,108,0,128,7,68,88,117,219,67,108,0,128,7,68,144,34,205,67,108,0,128,69,68,144,34,205,67,99,109,174,220,27,68,80,21,65,67,108,162,35,49,68,80,21,65,67,108,162,35,49,68,44,122,135,67,108,128,
229,68,68,44,122,135,67,108,0,128,38,68,148,68,196,67,108,128,26,8,68,44,122,135,67,108,174,220,27,68,44,122,135,67,108,174,220,27,68,80,21,65,67,99,101,0,0 };
		p.loadPathFromData(pathData, sizeof(pathData));
	}

	LOAD_EPATH_IF_URL("error", ColumnIcons::errorIcon);
	LOAD_EPATH_IF_URL("over", EditorIcons::redoIcon);

	return p;
}

} // namespace hise
