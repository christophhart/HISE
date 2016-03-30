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
*   along with Foobar.  If not, see <http://www.gnu.org/licenses/>.
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

void JavascriptCodeEditor::focusGained(FocusChangeType)
{
#if USE_BACKEND
	if (findParentComponentOfClass<BackendProcessorEditor>() != nullptr)
	{
		grabCopyAndPasteFocus();
	}
#endif
}

void JavascriptCodeEditor::selectLineAfterDefinition(Identifier identifier)
{
	String lineStart;

	lineStart << identifier.toString() << " = Content.add";

	const int docSize = getDocument().getNumCharacters();

	int index = docSize;

	while (!getTextInRange(Range<int>(index, docSize)).startsWith(lineStart) && (index > 0))
	{
		index--;
	}

	const int lineStartIndex = index;

	while (!getTextInRange(Range<int>(lineStartIndex, index)).containsChar('\n') && (index < docSize))
	{
		index++;
	}

	moveCaretTo(CodeDocument::Position(getDocument(), index), false);

	const int currentCharPosition = getCaretPos().getPosition();
	setHighlightedRegion(Range<int>(currentCharPosition, currentCharPosition));
}

bool JavascriptCodeEditor::selectText(const Identifier &identifier)
{
	String startLine;
	startLine << "// [JSON " << identifier.toString() << "]";

	

	String endLine;
	endLine << "// [/JSON " << identifier.toString() << "]";

	int startLineIndex = -1;
	CodeDocument::Position startPosition;
	CodeDocument::Position endPosition;



	for (int i = 0; i < getDocument().getNumLines(); i++)
	{
		String line = getDocument().getLine(i);

		if (line.contains(startLine))
		{
			startLineIndex = i;

			const int startPositionOfCommentBlock = line.indexOf(startLine);

			startPosition = CodeDocument::Position(getDocument(), i, startPositionOfCommentBlock);

			moveCaretTo(startPosition, false);
			break;
		}
	}

	if (startLineIndex == -1) return false;

	for(int i = startLineIndex; i < getDocument().getNumLines(); i++)
	{
		String line = getDocument().getLine(i);

		if(line.contains(endLine))
		{
			CodeDocument::Position endPosition(getDocument(), i, 0);
			
			moveCaretTo(endPosition, true);
			moveCaretToEndOfLine(true);

			return true;
		}
	}

	return false;
}

void JavascriptCodeEditor::focusLost(FocusChangeType )
{
#if USE_BACKEND
    
    BackendProcessorEditor *editor = findParentComponentOfClass<BackendProcessorEditor>();

    if(editor != nullptr)
    {
        MainController *mc = dynamic_cast<MainController*>(editor->getAudioProcessor());
        
        mc->setLastActiveEditor(this, getCaretPos());
    }
#endif
}

/*
  ==============================================================================

    ScriptingCodeEditor.cpp
    Created: 31 May 2015 2:27:47pm
    Author:  Christoph

  ==============================================================================
*/

void JavascriptCodeEditor::handleEscapeKey()
{
	entries.clear();

	int length = getCaretPos().getPosition();

	ScopedPointer<AutocompleteLookAndFeel> laf = new AutocompleteLookAndFeel();
	//laf->setColour(PopupMenu::backgroundColourId, Colours::transparentBlack);
	m.setLookAndFeel(laf);

	if (length == 0)
	{
		addDefaultAutocompleteOptions();
		showAutoCompletePopup();
		return;
	}

	for(int i = 0; i <= length; i++)
	{
		Range<int> range = Range<int>(length - i, length);

		String textInRange = getTextInRange(Range<int>(length - i, length));

		ScopedPointer<XmlElement> api = scriptProcessor->textInputMatchesApiClass(textInRange);

		if(textInRange == "SynthParameters")
		{
			addSynthParameterAutoCompleteOptions();
		}
		else if (textInRange == "Globals")
		{
			

			setHighlightedRegion(range);
			addGlobalsAutoCompleteOptions();
		}
		else if(api != nullptr)
		{
			setHighlightedRegion(range);

			addApiAutoCompleteOptions(api);

			if (textInRange == "Sampler")
			{
				addSamplerSoundPropertyList();
			}
		}
		else if(DynamicObject *obj = scriptProcessor->textInputMatchesScriptingObject(textInRange))
		{
			if(CreatableScriptObject* cso = dynamic_cast<CreatableScriptObject*>(obj))
			{
				setHighlightedRegion(range);

				ScopedPointer<XmlElement> api = scriptProcessor->textInputMatchesApiClass(cso->getObjectName().toString());

				setHighlightedRegion(range);

				if(api != nullptr)
				{
					for(int i = 0; i < api->getNumChildElements(); i++)
					{
						entries.add(new ApiEntry(api->getChildElement(i), String(textInRange)));
						m.addCustomItem(i + 1, entries[i]);
					}

				}
					
				bool insertSection = true;

				for(int i = 0; i < obj->getProperties().size(); i++)
				{
					var value = obj->getProperties().getValueAt(i);

					if(value.isInt())
					{
						Identifier id = obj->getProperties().getName(i);	
						entries.add(new ParameterEntry(textInRange, id, value));
						if(insertSection)
						{
							insertSection = false;
							m.addSeparator();
							m.addSectionHeader(cso->getProperties()["Name"].toString() + String(" Parameters"));
						}
						m.addCustomItem(entries.size(), entries.getLast());
					}
					
				}
			}
		}
		else if(ReferenceCountedObject *obj = scriptProcessor->textInputisArrayElementObject(textInRange))
		{
			if(CreatableScriptObject* cso = dynamic_cast<CreatableScriptObject*>(obj))
			{
				setHighlightedRegion(range);

				ScopedPointer<XmlElement> api = scriptProcessor->textInputMatchesApiClass(cso->getObjectName().toString());

				if(api != nullptr)
				{
					setHighlightedRegion(range);

					for(int i = 0; i < api->getNumChildElements(); i++)
					{
						entries.add(new ApiEntry(api->getChildElement(i), String(textInRange)));
						m.addCustomItem(i + 1, entries[i]);
					}
				}
			}
		}
		else if(textInRange.containsAnyOf(" \t\n({"))
		{
			addDefaultAutocompleteOptions();
		}
		else
		{
			continue;
		}

		showAutoCompletePopup();
		return;
	}
}

void JavascriptCodeEditor::showAutoCompletePopup()
{
	Rectangle<int> pos = getCaretRectangle();
	Rectangle<int> pos2 = Rectangle<int>(pos.getX() + getScreenPosition().getX(), pos.getBottom() + getScreenPosition().getY(), 0, 2);

	int result = m.showAt(pos2);

	if (result == 0)
	{
		entries.clear();
		m.clear();
	}
	else
	{
		const String codeToInsert = entries[result - 1]->getCodeToInsert();

		insertTextAtCaret(codeToInsert);

		const bool containsNonEmptyArgumentList = codeToInsert.contains("(") && !codeToInsert.contains("()");

		if (containsNonEmptyArgumentList)
		{
			moveCaretLeft(false, false);
			while (getCharacterAtCaret(false) != '(')
			{
				moveCaretLeft(false, true);
			}
			moveCaretRight(false, true);
		}

		entries.clear();
		m.clear();
	}
}

void JavascriptCodeEditor::addSynthParameterAutoCompleteOptions()
{
	for (int i = 0; i < scriptProcessor->getOwnerSynth()->getNumParameters(); i++)
	{
		entries.add(new ParameterEntry("", scriptProcessor->getOwnerSynth()->getIdentifierForParameterIndex(i), i));
		m.addCustomItem(entries.size(), entries.getLast());
	}
}

void JavascriptCodeEditor::addSamplerSoundPropertyList()
{
	m.addSeparator();
	m.addSectionHeader("Sampler Sound Property Indexes");

	for (int i = 1; i < ModulatorSamplerSound::numProperties; i++)
	{
		entries.add(new ParameterEntry("Sampler", ModulatorSamplerSound::getPropertyName((ModulatorSamplerSound::Property)i), i));
		m.addCustomItem(entries.size(), entries.getLast());
	}
}

void JavascriptCodeEditor::addApiAutoCompleteOptions(XmlElement *api)
{
	for (int i = 0; i < api->getNumChildElements(); i++)
	{
		entries.add(new ApiEntry(api->getChildElement(i), String(api->getTagName())));
		m.addCustomItem(i + 1, entries[i]);
	}
}

void JavascriptCodeEditor::addGlobalsAutoCompleteOptions()
{
	DynamicObject::Ptr globalObject = scriptProcessor->getMainController()->getGlobalVariableObject();

	NamedValueSet globalSet = globalObject->getProperties();

	for (int i = 0; i < globalSet.size(); i++)
	{
		NamedValueSet displaySet;

		String name = globalSet.getName(i).toString();

		displaySet.set("variableName", name);
		displaySet.set("value", globalSet.getValueAt(i).toString());

		VariableEntry *entry = new VariableEntry(getValueType(globalSet.getValueAt(i)), displaySet);

		entry->setCodeToInsert("Globals." + name);

		entries.add(entry);
		m.addCustomItem(i + 1, entry);
	}
}

void JavascriptCodeEditor::addDefaultAutocompleteOptions()
{
	entries.add(new ApiClassEntry("Console"));
	entries.add(new ApiClassEntry("Content"));
	entries.add(new ApiClassEntry("Engine"));
	entries.add(new ApiClassEntry("Message"));
	entries.add(new ApiClassEntry("Synth"));
	entries.add(new ApiClassEntry("Sampler"));
	entries.add(new ApiClassEntry("Globals"));

	NamedValueSet set = scriptProcessor->getScriptEngine()->getRootObjectProperties();

	for (int i = 0; i < set.size(); i++)
	{

		if (set.getVarPointerAt(i)->isMethod())
		{
			continue;
		}
		if (set.getVarPointerAt(i)->isObject())
		{
			DynamicObject *o = set.getVarPointerAt(i)->getDynamicObject();

			if (CreatableScriptObject *cso = dynamic_cast<CreatableScriptObject*>(o))
			{
				NamedValueSet displaySet;

				displaySet.set("variableName", set.getName(i).toString());
				displaySet.set("value", cso->getProperties()["Name"]);

				entries.add(new VariableEntry(cso->getObjectName().toString(), displaySet));
			}

		}

		if (set.getVarPointerAt(i)->isObject() && !set.getVarPointerAt(i)->isArray())
		{

		}
		else
		{
			NamedValueSet displaySet;

			displaySet.set("variableName", set.getName(i).toString());
			displaySet.set("value", set.getValueAt(i).toString());

			entries.add(new VariableEntry(getValueType(set.getValueAt(i)), displaySet));

		}

	}

	for (int i = 0; i < entries.size(); i++)
	{
		m.addCustomItem(i + 1, entries[i]);
	}
}

class CodeReplacer: public ThreadWithAsyncProgressWindow,
					public TextEditorListener
{
public:

	CodeReplacer(JavascriptCodeEditor *editor_) :
		ThreadWithAsyncProgressWindow("Search & Replace"),
		editor(editor_)
	{
		addTextEditor("search", SystemClipboard::getTextFromClipboard().removeCharacters("\n\r"), "Search for");
		getTextEditor("search")->addListener(this);

		addTextEditor("replace", SystemClipboard::getTextFromClipboard().removeCharacters("\n\r"), "Replace with");

		addButton("Next", 3, KeyPress(KeyPress::rightKey));
		addButton("Replace", 4, KeyPress(KeyPress::returnKey));

		addBasicComponents(false);
	}

	void textEditorTextChanged(TextEditor& e)
	{
		String analyseString = editor->getDocument().getAllContent();

		String search = e.getText();

		int numOccurences = 0;

		while (search.isNotEmpty() && analyseString.contains(search))
		{	
			analyseString = analyseString.fromFirstOccurrenceOf(search, false, false);

			numOccurences++;
		}

		showStatusMessage(String(numOccurences) + "matches.");
	}

	void resultButtonClicked(const String &name)
	{
		if (name == "Next")
		{
			goToNextMatch();

		}
		else if (name == "Replace")
		{
			const String search = getTextEditor("search")->getText();
			const String replace = getTextEditor("replace")->getText();

			const String selected = editor->getTextInRange(Range<int>(editor->getSelectionStart().getPosition(), editor->getSelectionEnd().getPosition()));

			if (selected == search)
			{
				editor->insertTextAtCaret(replace);
			}

			goToNextMatch();
		}
	}
	

	void run() override
	{

	}

	void threadFinished() override
	{

	}

private:

	void goToNextMatch()
	{
		const int start = editor->getCaretPos().getPosition();

		const String search = getTextEditor("search")->getText();

		String remainingText = editor->getDocument().getAllContent().substring(start);

		int offset = remainingText.indexOf(search);

		if (offset == -1)
		{
			PresetHandler::showMessageWindow("Finished searching", "Find reached the end of the file.");
		}
		else
		{
			editor->moveCaretTo(editor->getCaretPos().movedBy(offset), false);

			editor->moveCaretTo(editor->getCaretPos().movedBy(search.length()), true);
		}
	}

	JavascriptCodeEditor *editor;

};


bool JavascriptCodeEditor::keyPressed(const KeyPress& k)
{
#if USE_BACKEND
	handleDoubleCharacter(k, '(', ')');
	handleDoubleCharacter(k, '[', ']');
	

	const bool somethingSelected = (getSelectionStart() != getSelectionEnd());


	if (k.getKeyCode() == KeyPress::tabKey && k.getModifiers().isShiftDown() && somethingSelected)
	{
		unindentSelection();
		return true;
	}
	else if (k.getKeyCode() == 55 && k.getModifiers().isShiftDown() && somethingSelected)
	{
		const int startLine = getSelectionStart().getLineNumber();
		const int endLine = getSelectionEnd().getLineNumber();

		bool isCommented = getDocument().getLine(startLine).startsWith("//");

		CodeDocument::Position caret = getCaretPos();

		StringArray lines = StringArray::fromLines(getDocument().getAllContent());

		for(int i = startLine; i <= endLine; i++)
		{
			if (isCommented) lines.set(i, lines[i].trimCharactersAtStart("/"));
			else lines.set(i, "//" + lines[i]);
		}

		getDocument().replaceAllContent(lines.joinIntoString(getDocument().getNewLineCharacters()));

		CodeDocument::Position startP = CodeDocument::Position(getDocument(), startLine, 0);
		CodeDocument::Position endP = CodeDocument::Position(getDocument(), endLine, 0);

		moveCaretTo(startP, false);
		moveCaretTo(endP, true);
		moveCaretToEndOfLine(true);

		return true;
	}
	else if (k.getKeyCode() == KeyPress::tabKey && !k.getModifiers().isAnyModifierKeyDown() && somethingSelected)
	{
		indentSelection();
		return true;
	}
	else if (k.getKeyCode() == KeyPress::returnKey && k.getModifiers().isShiftDown())
	{
		CodeDocument::Position pos(getCaretPos());

		String blockIndent, lastLineIndent;
		getIndentForCurrentBlock(pos, getTabString(getTabSize()), blockIndent, lastLineIndent);

		moveCaretToEndOfLine(false);
        
        insertTextAtCaret(";" + getDocument().getNewLineCharacters() + lastLineIndent);
        
	}

	else if (k.getKeyCode() == KeyPress::F5Key)
	{
		CodeDocument::Position pos = getCaretPos();

		ScriptingEditor *parent = findParentComponentOfClass<ScriptingEditor>();
		
		if(parent != nullptr) parent->compileScript();
		else return false;

		moveCaretTo(pos, false);

		return true;
	}
	else if (k.isKeyCode(72) && k.getModifiers().isCommandDown()) // Ctrl + H 
	{
		if (currentModalWindow.getComponent() != 0)
		{
			currentModalWindow.deleteAndZero();
		}

		CodeReplacer * replacer = new CodeReplacer(this);

		currentModalWindow = replacer;

		replacer->showOnDesktop();
	}
	else if (k.isKeyCode(KeyPress::F4Key))
	{
		CodeDocument::Position pos = getCaretPos();
		pos.setPositionMaintained(true);
		moveCaretToStartOfLine(false);

		int posStart = getCaretPos().getPosition();

		if (getTextInRange(Range<int>(posStart, posStart + 2)) == "//")
		{
			getDocument().replaceSection(posStart, posStart + 2, "");
		}
		else
		{
			insertTextAtCaret("//");
		}

		moveCaretTo(pos, false);

		return true;
	}

#endif
	if (CodeEditorComponent::keyPressed(k))
	{
		return true;
	}
	return false;
}



PopupIncludeEditor::PopupIncludeEditor(ScriptProcessor *s, const File &fileToEdit) :
sp(s),
file(fileToEdit),
fontSize(14)
{
	doc = new CodeDocument();

	doc->replaceAllContent(file.loadFileAsString());

	tokeniser = new JavascriptTokeniser();
	addAndMakeVisible(editor = new JavascriptCodeEditor(*doc, tokeniser, s));

	addAndMakeVisible(resultLabel = new Label());

	resultLabel->setFont(GLOBAL_MONOSPACE_FONT());
	resultLabel->setColour(Label::ColourIds::backgroundColourId, Colours::darkgrey);
	resultLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	resultLabel->setEditable(false, false, false);

    editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight((float)fontSize));
    
	sp->setEditorState(sp->getEditorStateForIndex(ScriptProcessor::externalPopupShown), true);

    if(!file.existsAsFile()) editor->setEnabled(false);
    
	setSize(800, 800);
}

PopupIncludeEditor::~PopupIncludeEditor()
{
	editor = nullptr;
	doc = nullptr;
	tokeniser = nullptr;

	sp->getMainController()->setConsole(nullptr, true);

	sp->setEditorState(sp->getEditorStateForIndex(ScriptProcessor::externalPopupShown), false);

	sp = nullptr;
}

bool PopupIncludeEditor::keyPressed(const KeyPress& key)
{
	if (key.isKeyCode(KeyPress::F5Key))
	{
        CodeDocument::Position pos = editor->getCaretPos();
        
        String editorContent = doc->getAllContent();
        
		file.replaceWithText(editorContent);
		sp->compileScript();

		Result r = sp->getWatchedResult(0);

		lastCompileOk = r.wasOk();

		resultLabel->setColour(Label::backgroundColourId, Colours::white);
		resultLabel->setColour(Label::ColourIds::textColourId, Colours::white);
		resultLabel->setText(r.wasOk() ? "Compiled OK" : r.getErrorMessage(), dontSendNotification);
        
        startTimer(200);
        
		return true;
	}
    else if(key.isKeyCode('+') && key.getModifiers().isCommandDown())
    {
        fontSize += 1;
        
        if(fontSize > 30) fontSize = 30;
        
        editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight((float)fontSize));
    }
    else if(key.isKeyCode('-') && key.getModifiers().isCommandDown())
    {
        fontSize -= 1;
        
        if(fontSize < 6) fontSize = 6;
        
        editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight((float)fontSize));
    }
    
	return false;
}

void PopupIncludeEditor::resized()
{	
	editor->setBounds(0, 5, getWidth(), getHeight() - 23);
	resultLabel->setBounds(0, getHeight() - 18, getWidth(), 18);
}
