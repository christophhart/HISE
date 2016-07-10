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
*   Commercial licences for using HISE in an closed source project are
*   available on request. Please visit the project's website to get more
*   information about commercial licencing:
*
*   http://www.hartinstruments.net/hise/
*
*   HISE is based on the JUCE library,
*   which also must be licenced for commercial applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/
#ifndef SCRIPTINGCODEEDITOR_H_INCLUDED
#define SCRIPTINGCODEEDITOR_H_INCLUDED


class ApiHelpers
{
public:

	static AttributedString createAttributedStringFromApi(const ValueTree &method, const String &className, bool multiLine, Colour textColour);

	static String createCodeToInsert(const ValueTree &method, const String &className);

	static void getColourAndCharForType(int type, char &c, Colour &colour);

	struct Api
	{
		Api();

		ValueTree apiTree;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Api)
	};
};


class AutoCompleteEntry: public PopupMenu::CustomComponent,
                         public SettableTooltipClient
{
public:

    void getIdealSize(int &width, int &height) override
    {
        height = 20;
        width = methodName.getText().length() * 7 + 10;
    }

	Font getAutoCompleteFont() const
	{
		return GLOBAL_MONOSPACE_FONT();
        
	}

    void paint(Graphics &g) override
    {
		Colour up = isItemHighlighted() ? Colours::white : Colours::transparentBlack;

		Colour down = isItemHighlighted() ? Colours::white.withBrightness(0.95f) : Colours::black.withAlpha(0.05f);

		g.setGradientFill(ColourGradient(up, 0.0f, 0.0f, down, 0.0f, (float)getHeight(), false));

        g.fillAll();

        methodName.draw(g, Rectangle<float>(2.0f, 2.0f, (float)getWidth(), 14.0f));
    }

	const String getDescription() const
	{
		return description;
	};

	const String getCodeToInsert() const
	{
		return codeToInsert;
	}

	typedef ReferenceCountedObjectPtr<AutoCompleteEntry> Ptr;

protected:

    AttributedString methodName;
    String description;
    String codeToInsert;


private:




};



class ApiEntry: public AutoCompleteEntry
{
public:

	/** Creates an Autocomplete entry from an API XML element.
    *
    *    @param xml the xml element with the detailed information about the autocomplete entry
    *    @param className the name of the class. This will be used to display the function as class method
    *                     like "ClassName.method()".
    */
    ApiEntry(XmlElement *xml, String className)
    {
        const String name = xml->getStringAttribute("name", "");

        const String arguments = xml->getStringAttribute("arguments", "()");

		Font f = getAutoCompleteFont();

        methodName.append(xml->getStringAttribute("returnType", ""), f,Colours::black);
        methodName.append(className,  f, Colours::darkblue);
        methodName.append((className != "" ? "." : " "), f, Colours::black);
        methodName.append(name, f, Colours::darkblue.withAlpha(0.8f));
        methodName.append(arguments, f, Colours::black);

        description = xml->getStringAttribute("description", "");

        setTooltip(description);

        codeToInsert = className != "" ? className << "." << name << arguments : name;

    }

private:



};

class VariableEntry: public AutoCompleteEntry
{
public:

	/** Creates an AutoCompleteEntry for the variable.
	*
	*	@param className the class of the variable
	*	@param variableProperties a NamedValueSet with at least those properties:
	*			- \c'variableName' - the name of the variable.
	*			- \c'value'	- the value (or the string representation of the current value)
	*/
    VariableEntry(const String &className, NamedValueSet variableProperties)
    {
		ScopedPointer<XmlElement> xml= new XmlElement(className);

		variableProperties.copyToXmlAttributes(*xml);

		Font f = getAutoCompleteFont();

        methodName.append(className + " ", f, Colours::black);
		methodName.append(variableProperties["variableName"].toString() + ": ", f, Colours::darkblue);
		methodName.append(variableProperties["value"].toString(), f, Colours::black);

		if(variableProperties["objectName"].toString().isNotEmpty()) codeToInsert <<  variableProperties["objectName"].toString() << ".";

        codeToInsert << variableProperties["variableName"].toString() ;
    }

	void setCodeToInsert(const String &newCode)
	{
		codeToInsert = newCode;
	}
};

class ParameterEntry: public AutoCompleteEntry
{
public:

	/** Creates an AutoCompleteEntry for the variable.
	*
	*	@param className the class of the variable
	*	@param variableProperties a NamedValueSet with at least those properties:
	*			- \c'variableName' - the name of the variable.
	*			- \c'value'	- the value (or the string representation of the current value)
	*/
    ParameterEntry(const String &objectName, const Identifier &parameterName, int parameterValue)
    {
		Font f = getAutoCompleteFont();

		methodName.append(parameterName.toString() + ": ", f, Colours::darkblue);
		methodName.append(String(parameterValue), f, Colours::black);

		codeToInsert <<  objectName << ".";

        codeToInsert << parameterName.toString() ;
    }
};


class ApiClassEntry: public AutoCompleteEntry
{
public:

	ApiClassEntry(const String &className)
	{

		Font f = getAutoCompleteFont();

		methodName.append(className, f, Colours::darkblue);

		codeToInsert << className;

	}


};

class JavascriptCodeEditor;

class PopupIncludeEditor : public Component,
						   public Timer
{
public:
	
	void timerCallback()
	{
		resultLabel->setColour(Label::backgroundColourId, lastCompileOk ? Colours::green.withBrightness(0.1f) : Colours::red.withBrightness((0.1f)));
		stopTimer();
	}


	PopupIncludeEditor(ScriptProcessor *s, const File &fileToEdit);

	~PopupIncludeEditor();

	bool keyPressed(const KeyPress& key) override;

	void resized() override;;

private:

	int fontSize;

	ScopedPointer<JavascriptTokeniser> tokeniser;
	ScopedPointer < JavascriptCodeEditor > editor;
	ScopedPointer <CodeDocument> doc;
	
	ScopedPointer<Label> resultLabel;	

	ScriptProcessor *sp;
	File file;

	bool lastCompileOk;
};

class PopupIncludeEditorWindow: public DocumentWindow
{
public:

	PopupIncludeEditorWindow(File f, ScriptProcessor *s) :
		DocumentWindow("Editing external file: " + f.getFullPathName(), Colours::black, DocumentWindow::allButtons, true),
        file(f)
	{
        editor = new PopupIncludeEditor(s, f);
        
		setContentNonOwned(editor, true);

		setUsingNativeTitleBar(true);


		centreWithSize(800, 800);

		setResizable(true, true);

		setVisible(true);
	};
    
    File getFile() const {return file;};
 
	void paint(Graphics &g) override
	{
		if (editor != nullptr)
		{
			g.setColour(Colour(0xFF262626));
			g.fillAll();
		}
		
	}
    
	bool keyPressed(const KeyPress& key)
	{
		if (key.isKeyCode(KeyPress::F11Key))
		{
			if (Desktop::getInstance().getKioskModeComponent() == this)
			{
				Desktop::getInstance().setKioskModeComponent(nullptr, false);
			}
			else
			{
                Desktop::getInstance().setKioskModeComponent(nullptr, false);
				Desktop::getInstance().setKioskModeComponent(this, false);
			}

			return true;
		}

		return false;
    };
    
	void closeButtonPressed() override
	{
        if(Desktop::getInstance().getKioskModeComponent() == this)
        {
            Desktop::getInstance().setKioskModeComponent(nullptr, false);
        }
        
       delete this;
	};



private:

    ScopedPointer<PopupIncludeEditor> editor;
    
    const File file;

};


/** A subclass of CodeEditorComponent which improves working with Javascript scripts.
*	@ingroup scripting
*
*	It tokenises the code using Javascript keywords and dedicated HI keywords from the Scripting API
*	and applies some neat auto-intending features.
*/
class JavascriptCodeEditor: public CodeEditorComponent,
							public SettableTooltipClient,
							public Timer,
							public DragAndDropTarget,
							public CopyPasteTarget,
                            public SafeChangeListener
{
public:

	enum DragState
	{
		Virgin = 0,
		JSONFound,
		NoJSONFound
	};

	JavascriptCodeEditor(CodeDocument &document, CodeTokeniser *codeTokeniser, ScriptProcessor *p):
		CodeEditorComponent (document, codeTokeniser),
		scriptProcessor(p)
	{
		
		setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
		setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
		setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
		setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
		setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
		setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
		setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));
        
        setFont(GLOBAL_MONOSPACE_FONT().withHeight(p->getMainController()->getGlobalCodeFontSize()));
        
        p->getMainController()->getFontSizeChangeBroadcaster().addChangeListener(this);

        
	};

	virtual ~JavascriptCodeEditor()
	{
		currentPopup = nullptr;

        scriptProcessor->getMainController()->getFontSizeChangeBroadcaster().removeChangeListener(this);
        
		scriptProcessor = nullptr;

		

		currentModalWindow.deleteAndZero();

		stopTimer();
	};

    void changeListenerCallback(SafeChangeBroadcaster *) override
    {
        float newFontSize = scriptProcessor->getMainController()->getGlobalCodeFontSize();
        
        Font newFont = GLOBAL_MONOSPACE_FONT().withHeight(newFontSize);
        
        setFont(newFont);
        
    }
    
    void focusGained(FocusChangeType ) override;
    
	virtual String getObjectTypeName() override
	{
		return "Script Editor";
	}

	virtual void copyAction()
	{
		SystemClipboard::copyTextToClipboard(getTextInRange(getHighlightedRegion()));
	};

	virtual void pasteAction()
	{
		getDocument().replaceSection(getSelectionStart().getPosition(), getSelectionEnd().getPosition(), SystemClipboard::getTextFromClipboard());
	};

	bool isInterestedInDragSource (const SourceDetails &dragSourceDetails) override
	{
		return dragSourceDetails.description.isArray() && dragSourceDetails.description.size() == 2;
	}

	void itemDropped (const SourceDetails &dragSourceDetails) override
	{
		String toInsert = dragSourceDetails.description[2];

		insertTextAtCaret(toInsert);
	}

	void itemDragEnter(const SourceDetails &dragSourceDetails) override
	{
		const Identifier identifier = Identifier(dragSourceDetails.description[1].toString());
		//const String text = dragSourceDetails.description[2];

		positionFound = selectText(identifier) ? JSONFound : NoJSONFound;

		repaint();
	};

	void selectLineAfterDefinition(Identifier identifier);

	/** selects the text between the start and the end tag and returns true if it was found or false if not. */
	bool selectText(const Identifier &identifier);

	void itemDragMove (const SourceDetails &dragSourceDetails)
	{
		if(positionFound == NoJSONFound)
		{
			Point<int> pos = dragSourceDetails.localPosition;
			moveCaretTo(getPositionAt(pos.x, pos.y), false);
			
			const int currentCharPosition = getCaretPos().getPosition();
			setHighlightedRegion(Range<int>(currentCharPosition, currentCharPosition));

			repaint();
		}
		if(positionFound == JSONFound) return;
	}

	void focusLost(FocusChangeType t) override;

	void timerCallback() override
	{
		if (!getCurrentTokenRange().isEmpty() && currentPopup == nullptr)
		{
			showAutoCompleteNew();
		}

		stopTimer();
	}


    void addPopupMenuItems(PopupMenu &m, const MouseEvent *e) override;

    void performPopupMenuAction(int menuId) override;

	static String getValueType(const var &v)
	{
		const bool isObject = v.isObject();
		const bool isCreatableScriptObject = dynamic_cast<CreatableScriptObject*>(v.getDynamicObject()) != nullptr;

		if(v.isBool()) return "bool";
		else if(v.isInt() || v.isInt64()) return "int";
		else if (v.isDouble()) return "double";
		else if (v.isString()) return "String";
		else if (v.isArray()) return "Array";
        else if (v.isMethod()) return "Function";
		else if (isObject && isCreatableScriptObject)
		{
			CreatableScriptObject * obj = dynamic_cast<CreatableScriptObject*>(v.getDynamicObject());

			if(obj != nullptr) return obj->getObjectName().toString();
			else return String::empty;
		}
		else return String::empty;
	}

	class AutoCompletePopup : public ListBoxModel,
							  public Component
	{

	public:

		class AllToTheEditorTraverser : public KeyboardFocusTraverser
		{
		public:
			AllToTheEditorTraverser(JavascriptCodeEditor *editor_): editor(editor_) {};

			virtual Component* getNextComponent(Component* /*current*/) { return editor; }
			virtual Component* getPreviousComponent(Component* /*current*/) { return editor; }
			virtual Component* getDefaultComponent(Component* /*parentComponent*/) { return editor; }

			JavascriptCodeEditor *editor;
		};

		AutoCompletePopup(int fontHeight_, JavascriptCodeEditor* editor_, Range<int> tokenRange_, const String &tokenText);

		void createVariableRows();
		void createApiRows(const ValueTree &apiTree);
		void createObjectPropertyRows(const ValueTree &apiTree, const String &tokenText);

		~AutoCompletePopup()
		{
			infoBox = nullptr;
			listbox = nullptr;

			allInfo.clear();
		}

		KeyboardFocusTraverser* createFocusTraverser() override
		{
			return new AllToTheEditorTraverser(editor);
		}


		int getNumRows() override
		{
			return visibleInfo.size();
		}

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xFFBBBBBB));
		}

		void paintOverChildren(Graphics& g)
		{
			g.setColour(Colours::white);
			g.drawRect(getLocalBounds(), 1);
		}

		void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
		
		virtual void listBoxItemClicked(int row, const MouseEvent &)
		{
			selectRowInfo(row);
		}
			
		virtual void listBoxItemDoubleClicked(int row, const MouseEvent &)
		{
			editor->closeAutoCompleteNew(visibleInfo[row]->name);
		}

		bool handleEditorKeyPress(const KeyPress& k);

		

		void resized()
		{
			infoBox->setBounds(0, 0, getWidth(), 3*fontHeight);
			listbox->setBounds(0, 3*fontHeight, getWidth(), getHeight()-3*fontHeight);
		}

		void selectRowInfo(int rowIndex)
		{
			listbox->repaintRow(currentlySelectedBox);

			currentlySelectedBox = rowIndex;

			listbox->selectRow(currentlySelectedBox);
			listbox->repaintRow(currentlySelectedBox);
			infoBox->setInfo(visibleInfo[currentlySelectedBox]);
		}

		void rebuildVisibleItems(const String &selection)
		{
			visibleInfo.clear();

			int maxNameLength = 0;

			for (int i = 0; i < allInfo.size(); i++)
			{
				if (allInfo[i]->matchesSelection(selection))
				{
					maxNameLength = jmax<int>(maxNameLength, allInfo[i]->name.length());
					visibleInfo.add(allInfo[i]);
				}
			}

			listbox->updateContent();

			const float maxWidth = 450.0f;
			const int height = jmin<int>(200, fontHeight * 3 + (visibleInfo.size()) * (fontHeight + 4));
			setSize((int)maxWidth, height); 
		}

		bool escapeKeyHandled = false;

	private:

		SharedResourcePointer<ApiHelpers::Api> api;

		struct RowInfo
		{
			enum class Type
			{
				ApiClass = (int)DebugInformation::Type::numTypes,
				ApiMethod,
				numTypes
			};

			bool matchesSelection(const String &selection)
			{
				return name.containsIgnoreCase(selection);
			}


			AttributedString description;
			String codeToInsert;
			String name;
			int type;
			String typeName;
			String value;
		};

		struct ApiRows
		{
		public:

			ApiRows(bool init=false)
			{
				if (init)
				{
					
				}	
			}

			Array<String> a;
		};

		void initApiRows()
		{
			if (!apiRowsInitialised)
			{
				apiRows = ApiRows(true);
				apiRowsInitialised = true;
			}
		}

		static ApiRows apiRows;
		static bool apiRowsInitialised;

		class InfoBox : public Component
		{
		public:

			void setInfo(RowInfo *newInfo);

			void paint(Graphics &g);

		private:

			AttributedString infoText;
			RowInfo *currentInfo = nullptr;
		};

		OwnedArray<RowInfo> allInfo;
		Array<RowInfo*> visibleInfo;

		StringArray names;

		int fontHeight;
		
		int currentlySelectedBox = -1;

		ScopedPointer<InfoBox> infoBox;
		ScopedPointer<ListBox> listbox;
		ScriptProcessor *sp;
		
		Range<int> tokenRange;

		JavascriptCodeEditor *editor;

	};

	void showAutoCompleteNew();

	Range<int> getCurrentTokenRange() const
	{
		CodeDocument::Position tokenStart = getCaretPos();
		CodeDocument::Position tokenEnd(tokenStart);
		getDocument().findTokenContaining(tokenStart, tokenStart, tokenEnd);

		return Range<int>(tokenStart.getPosition(), tokenEnd.getPosition());
	}

	void closeAutoCompleteNew(const String returnString);

	void selectFunctionParameters();

	void handleEscapeKey() override;

	void showAutoCompletePopup();

	void addSynthParameterAutoCompleteOptions();

	void addApiAutoCompleteOptions(XmlElement *api);

	void paintOverChildren(Graphics& g)
	{
		CopyPasteTarget::paintOutlineIfSelected(g);
	}

	void addGlobalsAutoCompleteOptions();

	void addDefaultAutocompleteOptions(const String &enteredText);

	char getCharacterAtCaret(bool beforeCaret=false) const
	{
		CodeDocument::Position caretPos = getCaretPos();

		if (beforeCaret)
		{
			if(getCaretPos().getPosition() == 0) return 0;

			return (char)(getDocument().getTextBetween(caretPos.movedBy(-1), caretPos).getCharPointer()[0]);
		}
		else
		{
			if (getCaretPos().getPosition() == getDocument().getNumCharacters()-1) return 0;

			return (char)(getDocument().getTextBetween(caretPos, caretPos.movedBy(1)).getCharPointer()[0]);
		}
	}

	bool isNothingSelected() const
	{
		return getSelectionStart() == getSelectionEnd();
	}

	void handleDoubleCharacter(const KeyPress &k, char openCharacter, char closeCharacter)
	{
		
		// Insert 
		if ((char)k.getTextCharacter() == openCharacter)
		{
			char next = getCharacterAtCaret(false);

			if (getDocument().getNewLineCharacters().containsChar(next))
			{
				insertTextAtCaret(String(&closeCharacter, 1));
				moveCaretLeft(false, false);
			}

			CodeDocument::Iterator it(getDocument());

			char c;

			int numCharacters = 0;

			while (!it.isEOF())
			{
				c = (char)it.nextChar();

				if (c == openCharacter || c == closeCharacter)
				{
					numCharacters++;
				}

			}

			if (numCharacters % 2 == 0)
			{
				insertTextAtCaret(String(&closeCharacter, 1));
				moveCaretLeft(false, false);
			}

		}
		else if ((char)k.getTextCharacter() == closeCharacter)
		{ 
			if (getDocument().getTextBetween(getCaretPos(), getCaretPos().movedBy(1)) == String(&closeCharacter, 1))
			{
				moveCaretRight(false, true); 
				getDocument().deleteSection(getSelectionStart(), getSelectionEnd());
			}
		}

		// Delete both characters if the bracket is empty
		if (k.isKeyCode(KeyPress::backspaceKey) && 
			isNothingSelected() && 
			getCharacterAtCaret(true) == openCharacter && 
			getCharacterAtCaret(false) == closeCharacter)
		{
			getDocument().deleteSection(getCaretPos(), getCaretPos().movedBy(1));
		}
	}

	bool keyPressed(const KeyPress& k) override;

	void handleReturnKey() override
	{
		CodeEditorComponent::handleReturnKey();
		CodeDocument::Position pos (getCaretPos());

		String blockIndent, lastLineIndent;
		getIndentForCurrentBlock (pos, getTabString (getTabSize()), blockIndent, lastLineIndent);

		const String remainderOfBrokenLine (pos.getLineText());
		const int numLeadingWSChars = getLeadingWhitespace (remainderOfBrokenLine).length();

		if (numLeadingWSChars > 0)
			getDocument().deleteSection (pos, pos.movedBy (numLeadingWSChars));

		if (remainderOfBrokenLine.trimStart().startsWithChar ('}'))
			insertTextAtCaret (blockIndent);
		else
			insertTextAtCaret (lastLineIndent);

		const String previousLine (pos.movedByLines (-1).getLineText());
		const String trimmedPreviousLine (previousLine.trim());

		if ((trimmedPreviousLine.startsWith ("if ")
			  || trimmedPreviousLine.startsWith ("if(")
			  || trimmedPreviousLine.startsWith ("for ")
			  || trimmedPreviousLine.startsWith ("for(")
			  || trimmedPreviousLine.startsWith ("while(")
			  || trimmedPreviousLine.startsWith ("while "))
			 && trimmedPreviousLine.endsWithChar (')'))
		{
			insertTabAtCaret();
		}

		if (trimmedPreviousLine.endsWith("{"))
		{
			int openedBrackets = 0;
			CodeDocument::Iterator it(getDocument());

			while (!it.isEOF())
			{
				juce_wchar c = it.nextChar();

				if (c == '{')		openedBrackets++;
				else if (c == '}')	openedBrackets--;
			}

			if (openedBrackets == 1)
			{
				CodeDocument::Position prevPos = getCaretPos();

				insertTextAtCaret("\n" + blockIndent + "}");
				moveCaretTo(prevPos, false);
			}
		}

		resized();

		
	};

	void insertTextAtCaret (const String& newText) override
	{
		if (getHighlightedRegion().isEmpty())
		{
			const CodeDocument::Position pos (getCaretPos());

			if ((newText == "{" || newText == "}")
				 && pos.getLineNumber() > 0
				 && pos.getLineText().trim().isEmpty())
			{
				moveCaretToStartOfLine (true);

				String blockIndent, lastLineIndent;
				if (getIndentForCurrentBlock (pos, getTabString (getTabSize()), blockIndent, lastLineIndent))
				{
					insertTextAtCaret (blockIndent);

					if (newText == "{")
						insertTabAtCaret();
				}
			}
		}

		CodeEditorComponent::insertTextAtCaret (newText);
	};

private:

	Component::SafePointer<Component> currentModalWindow;

	ScopedPointer<AutoCompletePopup> currentPopup;

	ScriptProcessor *scriptProcessor;
    
	PopupMenu m;
	Array<AutoCompleteEntry::Ptr> entries;

	PopupLookAndFeel plaf;

	DragState positionFound;
    
	String getLeadingWhitespace (String line)
    {
        line = line.removeCharacters ("\r\n");
        const String::CharPointerType endOfLeadingWS (line.getCharPointer().findEndOfWhitespace());
        return String (line.getCharPointer(), endOfLeadingWS);
    }

	int getBraceCount (String::CharPointerType line)
    {
        int braces = 0;

        for (;;)
        {
            const juce_wchar c = line.getAndAdvance();

            if (c == 0)                         break;
            else if (c == '{')                  ++braces;
            else if (c == '}')                  --braces;
            else if (c == '/')                  { if (*line == '/') break; }
            else if (c == '"' || c == '\'')     { while (! (line.isEmpty() || line.getAndAdvance() == c)) {} }
        }

        return braces;
    }

	bool getIndentForCurrentBlock (CodeDocument::Position pos, const String& tab,
                                   String& blockIndent, String& lastLineIndent)
    {
        int braceCount = 0;
        bool indentFound = false;

        while (pos.getLineNumber() > 0)
        {
            pos = pos.movedByLines (-1);

            const String line (pos.getLineText());
            const String trimmedLine (line.trimStart());

            braceCount += getBraceCount (trimmedLine.getCharPointer());

            if (braceCount > 0)
            {
                blockIndent = getLeadingWhitespace (line);
                if (! indentFound)
                    lastLineIndent = blockIndent + tab;

                return true;
            }

            if ((! indentFound) && trimmedLine.isNotEmpty())
            {
                indentFound = true;
                lastLineIndent = getLeadingWhitespace (line);
            }
        }

        return false;
    }
	void addSamplerSoundPropertyList();
};


class CodeEditorWrapper: public Component,
						 public Timer
{
public:

	CodeEditorWrapper(CodeDocument &document, CodeTokeniser *codeTokeniser, ScriptProcessor *p)
	{
		addAndMakeVisible(editor = new JavascriptCodeEditor(document, codeTokeniser, p));

		

		restrainer.setMinimumHeight(50);
		restrainer.setMaximumHeight(600);

		addAndMakeVisible(dragger = new ResizableEdgeComponent(this, &restrainer, ResizableEdgeComponent::Edge::bottomEdge));

		dragger->addMouseListener(this, true);

		setSize(200, 340);

		
		currentHeight = getHeight();
	}

	virtual ~CodeEditorWrapper()
	{
		editor = nullptr;
	}

	ScopedPointer<JavascriptCodeEditor> editor;

	void resized() override
	{
		editor->setBounds(getLocalBounds());

		dragger->setBounds(0, getHeight() - 5, getWidth(), 5);
	};

	void timerCallback()
	{
#if USE_BACKEND
		ProcessorEditorBody *body = dynamic_cast<ProcessorEditorBody*>(getParentComponent());

		resized();

		if(body != nullptr)
		{
			currentHeight = getHeight();

			body->refreshBodySize();
		}
#endif

	}

	void mouseDown(const MouseEvent &m) override
	{
		if(m.eventComponent == dragger) startTimer(30);
	};

	void mouseUp(const MouseEvent &) override
	{
		stopTimer();
	};

	int currentHeight;

private:

	ScopedPointer<ResizableEdgeComponent> dragger;

	ComponentBoundsConstrainer restrainer;

	LookAndFeel_V2 laf2;
	

};



#endif  // SCRIPTINGCODEEDITOR_H_INCLUDED
