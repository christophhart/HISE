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

#include <regex>

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
    try
    {
        String regex = "(var )?" + identifier.toString() + " *= *";
        
        
        std::regex reg(regex.toStdString());
        
        
        const int docSize = getDocument().getNumCharacters();
        
        int index = docSize;
        
        while (!std::regex_search(getTextInRange(Range<int>(index, docSize)).toStdString(), reg) && (index > 0))
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
    catch (std::regex_error e)
    {
        //debugError(sampler, e.what());
    }

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

void JavascriptCodeEditor::addPopupMenuItems(PopupMenu &m, const MouseEvent *e)
{
#if USE_BACKEND
    m.setLookAndFeel(&plaf);
    
    CodeEditorComponent::addPopupMenuItems(m, e);
    
    String s = getTextInRange(getHighlightedRegion());
    
    ScriptingEditor *editor = findParentComponentOfClass<ScriptingEditor>();
    
    if(editor != nullptr)
    {
        m.addSeparator();
        m.addSectionHeader("Import / Export");
        m.addItem(101, "Save Script To File");
        m.addItem(102, "Load Script From File");
        m.addSeparator();
        m.addItem(103, "Save Script to Clipboard");
        m.addItem(104, "Load Script from Clipboard");
        m.addSeparator();
    }
    
    if(Identifier::isValidIdentifier(s))
    {
        Identifier selection = Identifier(s);
        
        NamedValueSet set = scriptProcessor->getScriptEngine()->getRootObjectProperties();
        
        if(set.contains(selection))
        {
            m.addSeparator();
            int index = set.indexOf(selection);
            const String itemString = "Set " + selection.toString() + "(" + set.getValueAt(index).toString() + ")";
            m.addItem(99, itemString);
        }
    }

#else

	ignoreUnused(m, e);

#endif
};

void JavascriptCodeEditor::performPopupMenuAction(int menuId)
{
#if USE_BACKEND
    JavascriptProcessor *s = scriptProcessor;
    
	Processor* p = dynamic_cast<Processor*>(s);

    ScriptingEditor *editor = findParentComponentOfClass<ScriptingEditor>();
    
    if(editor != nullptr && menuId == 101) // SAVE
    {
        FileChooser scriptSaver("Save script as",
                                File(GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
                                "*.js");
        
        if (scriptSaver.browseForFileToSave(true))
        {
            String script;
            s->mergeCallbacksToScript(script);
            scriptSaver.getResult().replaceWithText(script);
			debugToConsole(p, "Script saved to " + scriptSaver.getResult().getFullPathName());
        }
    }
    else if (editor != nullptr && menuId == 102) // LOAD
    {
        FileChooser scriptLoader("Please select the script you want to load",
                                 File(GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
                                 "*.js");
        
        if (scriptLoader.browseForFileToOpen())
        {
            String script = scriptLoader.getResult().loadFileAsString().removeCharacters("\r");
            s->parseSnippetsFromString(script);
            editor->compileScript();
            debugToConsole(p, "Script loaded from " + scriptLoader.getResult().getFullPathName());
        }
    }
    else if (editor != nullptr && menuId == 103) // COPY
    {
        String x;
        s->mergeCallbacksToScript(x);
        SystemClipboard::copyTextToClipboard(x);
        
        debugToConsole(p, "Script exported to Clipboard.");
    }
    else if (menuId == 104) // PASTE
    {
        String x = String(SystemClipboard::getTextFromClipboard()).removeCharacters("\r");
        
        if (x.containsNonWhitespaceChars() && PresetHandler::showYesNoWindow("Replace Script?", "Do you want to replace the script?"))
        {
            s->parseSnippetsFromString(x);
            editor->compileScript();
        }
    }
    else if(menuId == 99)
    {
        String s = getTextInRange(getHighlightedRegion());
        
        Identifier selection = Identifier(s);
        
        NamedValueSet set = scriptProcessor->getScriptEngine()->getRootObjectProperties();
        
        var v;
        
        if(set.contains(selection))
        {
            m.addSeparator();
            
            int index = set.indexOf(selection);
            
            v = set.getValueAt(index);
        }
        
        ScopedPointer<AlertWindow> nameWindow = new AlertWindow("Change a variable", "Set the variable to a new value", AlertWindow::AlertIconType::NoIcon);
        
        
        
        nameWindow->addTextEditor("Name", v.toString() );
        nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));
        nameWindow->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));
        
        if(nameWindow->runModalLoop())
        {
            String newValue = nameWindow->getTextEditorContents("Name");
            
            String code = s << " = " << newValue << ";";
            
            Result r = scriptProcessor->getScriptEngine()->execute(code);
            
            if(r != Result::ok())
            {
                AlertWindow::showMessageBox(AlertWindow::NoIcon, "Error parsing expression", "The expression you entered is not valid.");
            }
            
        }
        
    }
    else CodeEditorComponent::performPopupMenuAction(menuId);
#else 
	ignoreUnused(menuId);

#endif
}

void JavascriptCodeEditor::showAutoCompleteNew()
{
	Range<int> tokenRange = getCurrentTokenRange();
	currentPopup = new AutoCompletePopup((int)getFont().getHeight(), this, tokenRange, getTextInRange(tokenRange));

	if (currentPopup->getNumRows() == 0)
	{
		currentPopup = nullptr;
	}
	else
	{
		Component *editor = findParentComponentOfClass<BackendProcessorEditor>();

		if (editor == nullptr)
		{
			editor = findParentComponentOfClass<PopupIncludeEditorWindow>();
		}

		if (editor != nullptr)
		{
			editor->addAndMakeVisible(currentPopup);

			CodeDocument::Position current = getCaretPos();
			moveCaretTo(CodeDocument::Position(getDocument(), tokenRange.getStart()), false);

			Rectangle<int> caret = editor->getLocalArea(this, getCaretRectangle());
			Point<int> topLeft = caret.getBottomLeft();

			if (caret.getY() > editor->getHeight() - currentPopup->getHeight())
			{
				topLeft = Point<int>(topLeft.getX(), jmax<int>(0, caret.getY() - currentPopup->getHeight()));
			}

			moveCaretTo(current, false);

			currentPopup->setTopLeftPosition(topLeft);
		}
	}
}

void JavascriptCodeEditor::closeAutoCompleteNew(const String returnString)
{
	Desktop::getInstance().getAnimator().fadeOut(currentPopup, 200);

	currentPopup = nullptr;

	if (returnString.isNotEmpty())
	{
		Range<int> tokenRange = getCurrentTokenRange();

		getDocument().replaceSection(tokenRange.getStart(), tokenRange.getEnd(), returnString);

		selectFunctionParameters();
	}
}

void JavascriptCodeEditor::selectFunctionParameters()
{
	if (getCharacterAtCaret(true) == ')')
	{
		moveCaretLeft(false, false);

		if (getCharacterAtCaret(true) == '(')
		{
			moveCaretRight(false, false);
		}
		else
		{
			CodeDocument::Position pos = getCaretPos();

			while (pos.getCharacter() != '(' && pos.getIndexInLine() > 0)
			{
				pos.moveBy(-1);
			}
			pos.moveBy(1);

			moveCaretTo(pos, true);
		}
	}
}

void JavascriptCodeEditor::handleEscapeKey()
{
	if (currentPopup == nullptr)
	{
		showAutoCompleteNew();
	}
	else
	{
		closeAutoCompleteNew(String());
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
			PresetHandler::showMessageWindow("Finished searching", "Find reached the end of the file.", PresetHandler::IconType::Info);
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
	
	if (currentPopup != nullptr)
	{
		if (currentPopup->handleEditorKeyPress(k))
		{
			return true;
		}
	}


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

	if(k != KeyPress::escapeKey) startTimer(800);

	return CodeEditorComponent::keyPressed(k);
}



PopupIncludeEditor::PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit) :
sp(s),
file(fileToEdit)
{
	Processor *p = dynamic_cast<Processor*>(sp);
    
	doc = new CodeDocument();

	doc->replaceAllContent(file.loadFileAsString());

	tokeniser = new JavascriptTokeniser();
	addAndMakeVisible(editor = new JavascriptCodeEditor(*doc, tokeniser, s));

	addAndMakeVisible(resultLabel = new Label());

	resultLabel->setFont(GLOBAL_MONOSPACE_FONT());
	resultLabel->setColour(Label::ColourIds::backgroundColourId, Colours::darkgrey);
	resultLabel->setColour(Label::ColourIds::textColourId, Colours::white);
	resultLabel->setEditable(false, false, false);
    
	p->setEditorState(p->getEditorStateForIndex(JavascriptMidiProcessor::externalPopupShown), true);

    if(!file.existsAsFile()) editor->setEnabled(false);
    
	setSize(800, 800);
}

PopupIncludeEditor::~PopupIncludeEditor()
{
	editor = nullptr;
	doc = nullptr;
	tokeniser = nullptr;

	Processor *p = dynamic_cast<Processor*>(sp);

	p->setEditorState(p->getEditorStateForIndex(JavascriptMidiProcessor::externalPopupShown), false);

	sp = nullptr;
	p = nullptr;
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
    
	return false;
}

void PopupIncludeEditor::resized()
{	
	editor->setBounds(0, 5, getWidth(), getHeight() - 23);
	resultLabel->setBounds(0, getHeight() - 18, getWidth(), 18);
}

ApiHelpers::Api::Api()
{
	apiTree = ValueTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize));
}

void ApiHelpers::getColourAndCharForType(int type, char &c, Colour &colour)
{

	const float alpha = 0.6f;
	const float brightness = 0.8f;


	switch (type)
	{
	case (int)DebugInformation::Type::InlineFunction:	c = 'I'; break;
	case (int)DebugInformation::Type::Callback:			c = 'F'; break;
	case (int)DebugInformation::Type::Variables:		c = 'V'; break;
	case (int)DebugInformation::Type::Globals:			c = 'G'; break;
	case (int)DebugInformation::Type::Constant:			c = 'C'; break;
	case (int)DebugInformation::Type::RegisterVariable:	c = 'R'; break;
	case 7:												c = 'A'; break;
	default:											c = 'V'; break;
	}

	switch (c)
	{
	case 'I': colour = Colours::blue.withAlpha(alpha).withBrightness(brightness); break;
	case 'V': colour = Colours::cyan.withAlpha(alpha).withBrightness(brightness); break;
	case 'G': colour = Colours::green.withAlpha(alpha).withBrightness(brightness); break;
	case 'C': colour = Colours::yellow.withAlpha(alpha).withBrightness(brightness); break;
	case 'R': colour = Colours::red.withAlpha(alpha).withBrightness(brightness); break;
	case 'A': colour = Colours::orange.withAlpha(alpha).withBrightness(brightness); break;
	case 'F': colour = Colours::purple.withAlpha(alpha).withBrightness(brightness); break;

	}
}

JavascriptCodeEditor::AutoCompletePopup::AutoCompletePopup(int fontHeight_, JavascriptCodeEditor* editor_, Range<int> tokenRange_, const String &tokenText) :
fontHeight(fontHeight_),
editor(editor_),
tokenRange(tokenRange_)
{
	sp = editor->scriptProcessor;

	

	addAndMakeVisible(listbox = new ListBox());
	addAndMakeVisible(infoBox = new InfoBox());

	const ValueTree apiTree = ValueTree(ValueTree::readFromData(XmlApi::apivaluetree_dat, XmlApi::apivaluetree_datSize));

	if (tokenText.getLastCharacter() == '.')
	{
		createObjectPropertyRows(apiTree, tokenText);
	}
	else
	{
		createVariableRows();
		createApiRows(apiTree);
	}

	listbox->setModel(this);
	
	listbox->setRowHeight(fontHeight + 4);
	listbox->setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);
	

	listbox->getViewport()->setScrollBarThickness(8);
	listbox->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::thumbColourId, Colours::black.withAlpha(0.6f));
	listbox->getVerticalScrollBar()->setColour(ScrollBar::ColourIds::trackColourId, Colours::black.withAlpha(0.4f));

	listbox->setWantsKeyboardFocus(false);
	setWantsKeyboardFocus(false);
	infoBox->setWantsKeyboardFocus(false);

	rebuildVisibleItems(tokenText);
}

void JavascriptCodeEditor::AutoCompletePopup::createVariableRows()
{
	HiseJavascriptEngine *engine = sp->getScriptEngine();

	for (int i = 0; i < engine->getNumDebugObjects(); i++)
	{
		DebugInformation *info = engine->getDebugInformation(i);
		ScopedPointer<RowInfo> row = new RowInfo();

		row->type = info->getType();
		row->description = info->getDescription();
		row->name = info->getTextForName();
		row->typeName = info->getTextForDataType();
		row->value = info->getTextForValue();
		row->codeToInsert = info->getTextForName();

#if 0

		DebugableObject *object = info->getObject();

		if (object != nullptr && false)
		{
			row->type = info->getType();
			
			row->description = object->getDescription();
			row->name = object->getDebugName();
			row->typeName = object->getDebugDataType();
			row->value = object->getDebugValue();
			row->codeToInsert = object->getDebugName();
		}
		else
		{
			
		}
	
#endif

		allInfo.add(row.release());
	}
}

void JavascriptCodeEditor::AutoCompletePopup::createApiRows(const ValueTree &apiTree)
{
	for (int i = 0; i < apiTree.getNumChildren(); i++)
	{
		ValueTree classTree = apiTree.getChild(i);
		const String className = classTree.getType().toString();

		if (className != "Content" && !sp->getScriptEngine()->isApiClassRegistered(className)) continue;

		RowInfo *row = new RowInfo();
		row->codeToInsert = className;
		row->name = className;
		row->type = (int)RowInfo::Type::ApiMethod;

		allInfo.add(row);

		const ApiClass* apiClass = sp->getScriptEngine()->getApiClass(className);

		addApiMethods(classTree, Identifier(className));

		if (apiClass != nullptr) addApiConstants(apiClass, apiClass->getName());
	}
}

void JavascriptCodeEditor::AutoCompletePopup::createObjectPropertyRows(const ValueTree &apiTree, const String &tokenText)
{
	const Identifier objectId = Identifier(tokenText.upToLastOccurrenceOf(String("."), false, true));

	HiseJavascriptEngine *engine = sp->getScriptEngine();
	const ReferenceCountedObject* o = engine->getScriptObject(objectId);

	const ApiClass* apiClass = engine->getApiClass(objectId);

	if (o != nullptr)
	{
		if (const DynamicScriptingObject* cso = dynamic_cast<const DynamicScriptingObject*>(o))
		{
			const Identifier csoName = cso->getObjectName();

			ValueTree documentedMethods = apiTree.getChildWithName(csoName);

			for (int i = 0; i < cso->getProperties().size(); i++)
			{
				const var prop = cso->getProperties().getValueAt(i);

				if (prop.isMethod())
				{
					RowInfo *info = new RowInfo();
					const Identifier id = cso->getProperties().getName(i);

					static const Identifier name("name");

					const ValueTree methodTree = documentedMethods.getChildWithProperty(name, id.toString());

					info->description = ApiHelpers::createAttributedStringFromApi(methodTree, objectId.toString(), false, Colours::black);
					info->codeToInsert = ApiHelpers::createCodeToInsert(methodTree, objectId.toString());
					info->name = info->codeToInsert;
					info->type = (int)RowInfo::Type::ApiMethod;
					allInfo.add(info);
				}
			}

			for (int i = 0; i < cso->getProperties().size(); i++)
			{
				const var prop = cso->getProperties().getValueAt(i);

				if (prop.isMethod()) continue;

				const Identifier id = cso->getProperties().getName(i);
				RowInfo *info = new RowInfo();

				info->name = objectId.toString() + "." + cso->getProperties().getName(i).toString();
				info->codeToInsert = info->name;
				info->typeName = DebugInformation::getVarType(prop);
				info->value = prop.toString();

				allInfo.add(info);
			}
		}
		else if (const ConstScriptingObject* cow = dynamic_cast<const ConstScriptingObject*>(o))
		{
			const Identifier cowName = cow->getObjectName();

			ValueTree documentedMethods = apiTree.getChildWithName(cowName);

			Array<Identifier> functionNames;
			Array<Identifier> constantNames;

			cow->getAllFunctionNames(functionNames);
			cow->getAllConstants(constantNames);

			for (int i = 0; i < functionNames.size(); i++)
			{
				RowInfo *info = new RowInfo();
				const Identifier id = functionNames[i];
				static const Identifier name("name");
				
				const ValueTree methodTree = documentedMethods.getChildWithProperty(name, id.toString());

				info->description = ApiHelpers::createAttributedStringFromApi(methodTree, objectId.toString(), false, Colours::black);
				info->codeToInsert = ApiHelpers::createCodeToInsert(methodTree, objectId.toString());

				if (!info->codeToInsert.contains(id.toString()))
				{
					jassertfalse;

					info->codeToInsert = objectId.toString() + "." + id.toString() + "(";

					int numArgs, functionIndex;
					cow->getIndexAndNumArgsForFunction(id, functionIndex, numArgs);

					for (int i = 0; i < numArgs; i++)
					{
						info->codeToInsert << "arg" + String(i + 1);
						if (i != (numArgs - 1)) info->codeToInsert << ", ";
					}

					
					info->codeToInsert << ")";
				}

				info->name = info->codeToInsert;
				info->type = (int)RowInfo::Type::ApiMethod;

				jassert(info->name.isNotEmpty());

				allInfo.add(info);
			}

			for (int i = 0; i < constantNames.size(); i++)
			{
				const Identifier id = constantNames[i];
				RowInfo *info = new RowInfo();

				var value = cow->getConstantValue(cow->getConstantIndex(id));

				info->name = objectId.toString() + "." + id.toString();
				info->codeToInsert = info->name;
				info->type = (int)DebugInformation::Type::Constant;
				info->typeName = DebugInformation::getVarType(value);
				info->value = value;

				allInfo.add(info);
			}
		}
		else if (const DynamicObject* obj = dynamic_cast<const DynamicObject*>(o))
		{
			for (int i = 0; i < obj->getProperties().size(); i++)
			{
				const var prop = obj->getProperties().getValueAt(i);

				RowInfo *info = new RowInfo();

				info->name = objectId.toString() + "." + obj->getProperties().getName(i).toString();
				info->codeToInsert = info->name;
				info->typeName = DebugInformation::getVarType(prop);
				info->value = prop.toString();

				allInfo.add(info);
			}
		}
	}
	else if (apiClass != nullptr)
	{
		ValueTree classTree = apiTree.getChildWithName(apiClass->getName());

		addApiMethods(classTree, objectId);
		addApiConstants(apiClass, objectId);
	}
	
	addCustomEntries(objectId, apiTree);

}

void JavascriptCodeEditor::AutoCompletePopup::addCustomEntries(const Identifier &objectId, const ValueTree &apiTree)
{
	if (objectId.toString() == "g") // Special treatment for the g variable...
	{
		static const Identifier g("Graphics");

		ValueTree classTree = apiTree.getChildWithName(g);

		addApiMethods(classTree, objectId.toString());
	}
	else if (objectId.toString() == "event")
	{
		StringArray names = MouseCallbackComponent::getCallbackPropertyNames();

		for (int i = 0; i < names.size(); i++)
		{
			RowInfo *info = new RowInfo();

			info->name = objectId.toString() + "." + names[i];
			info->codeToInsert = info->name;
			info->typeName = "int";
			info->value = names[i];
			info->type = (int)DebugInformation::Type::Variables;

			allInfo.add(info);
		}
	}
}

void JavascriptCodeEditor::AutoCompletePopup::addApiConstants(const ApiClass* apiClass, const Identifier &objectId)
{
	Array<Identifier> constants;

	apiClass->getAllConstants(constants);

	for (int i = 0; i < constants.size(); i++)
	{
		const var prop = apiClass->getConstantValue(i);

		RowInfo *info = new RowInfo();

		info->name = objectId.toString() + "." + constants[i].toString();
		info->codeToInsert = info->name;
		info->typeName = DebugInformation::getVarType(prop);
		info->value = prop.toString();
		info->type = (int)DebugInformation::Type::Constant;

		allInfo.add(info);
	}
}

void JavascriptCodeEditor::AutoCompletePopup::addApiMethods(const ValueTree &classTree, const Identifier &objectId)
{
	

	for (int j = 0; j < classTree.getNumChildren(); j++)
	{
		ValueTree methodTree = classTree.getChild(j);

		RowInfo *row = new RowInfo();
		row->description = ApiHelpers::createAttributedStringFromApi(methodTree, objectId.toString(), false, Colours::black);
		row->codeToInsert = ApiHelpers::createCodeToInsert(methodTree, objectId.toString());
		row->name = row->codeToInsert;
		row->type = (int)RowInfo::Type::ApiMethod;

		allInfo.add(row);
	}
}

void JavascriptCodeEditor::AutoCompletePopup::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	RowInfo *info = visibleInfo[rowNumber];
	Colour c = (rowIsSelected ? Colour(0xff333333) : Colours::transparentBlack);

	g.setColour(c);
	g.fillAll();

	g.setColour(Colours::black.withAlpha(0.1f));
	g.drawHorizontalLine(0, 0.0f, (float)width);

	if (rowIsSelected)
	{
		g.setColour(Colours::white.withAlpha(0.1f));
		g.drawHorizontalLine(0, 0.0f, (float)width);
		g.setColour(Colours::black.withAlpha(0.1f));
		g.drawHorizontalLine(height - 1, 0.0f, (float)width);
	}


	char ch;
	Colour colour;

	ApiHelpers::getColourAndCharForType(info->type, ch, colour);

	g.setColour(colour);
	g.fillRoundedRectangle(1.0f, 1.0f, height - 2.0f, height - 2.0f, 4.0f);



	g.setColour(rowIsSelected ? Colours::white : Colours::black.withAlpha(0.7f));
	g.setFont(GLOBAL_MONOSPACE_FONT().withHeight((float)fontHeight));

	const String name = info->name;

	g.drawText(name, height + 2, 1, width - height - 4, height - 2, Justification::centredLeft);
}

bool JavascriptCodeEditor::AutoCompletePopup::handleEditorKeyPress(const KeyPress& k)
{
	if (k == KeyPress::upKey)
	{
		selectRowInfo(jmax<int>(0, currentlySelectedBox - 1));
		return true;
	}
	else if (k == KeyPress::downKey)
	{
		selectRowInfo(jmin<int>(getNumRows() - 1, currentlySelectedBox + 1));
		return true;
	}
	else if (k == KeyPress::returnKey)
	{
        const bool insertSomething = currentlySelectedBox >= 0;
        
        editor->closeAutoCompleteNew(insertSomething ? visibleInfo[currentlySelectedBox]->name : String());

		return insertSomething;
	}
	else if (k == KeyPress::spaceKey || k == KeyPress::tabKey || k.getTextCharacter() == ';' || k.getTextCharacter() == '(')
	{
		editor->closeAutoCompleteNew(String());
		return false;
	}
	else
	{
		String selection = editor->getTextInRange(editor->getCurrentTokenRange());
		
		rebuildVisibleItems(selection);
		return false;
	}
}

void JavascriptCodeEditor::AutoCompletePopup::InfoBox::setInfo(RowInfo *newInfo)
{
	currentInfo = newInfo;

	if (newInfo != nullptr)
	{
		if (newInfo->description.getNumAttributes() == 0)
		{
			infoText = AttributedString();

			infoText.append("Type: ", GLOBAL_BOLD_FONT(), Colours::black);
			infoText.append(newInfo->typeName, GLOBAL_MONOSPACE_FONT(), Colours::black);
			infoText.append(" Value: ", GLOBAL_BOLD_FONT(), Colours::black);
			infoText.append(newInfo->value, GLOBAL_MONOSPACE_FONT(), Colours::black);

			infoText.setJustification(Justification::centredLeft);
		}
		else
		{
			infoText = newInfo->description;
		}

		repaint();
	}

	
}

void JavascriptCodeEditor::AutoCompletePopup::InfoBox::paint(Graphics &g)
{
	g.setColour(Colours::black.withAlpha(0.2f));
	g.fillAll();

	if (currentInfo != nullptr)
	{
		
		char c;

		Colour colour;

		ApiHelpers::getColourAndCharForType(currentInfo->type, c, colour);

		g.setColour(colour);

		const Rectangle<float> area(5.0f, (float)(getHeight()/2 - 12), (float)24.0f, (float)24.0f);

		g.fillRoundedRectangle(area, 5.0f);
		g.setColour(Colours::black.withAlpha(0.4f));
		g.drawRoundedRectangle(area, 5.0f, 1.0f);
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white);

		String type;
		type << c;
		g.drawText(type, area, Justification::centred);

		const Rectangle<float> infoRectangle(area.getRight() + 8.0f, 2.0f, (float)getWidth() - area.getRight() - 8.0f, (float)getHeight()-4.0f);

		infoText.draw(g, infoRectangle);
	}
}
