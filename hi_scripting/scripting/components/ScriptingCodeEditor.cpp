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


class CodeReplacer : public ThreadWithAsyncProgressWindow,
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


JavascriptCodeEditor::JavascriptCodeEditor(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p) :
CodeEditorComponent(document, codeTokeniser),
scriptProcessor(p),
processor(dynamic_cast<Processor*>(p))
{

	setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
	setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
	setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
	setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
	setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
	setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
	setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));

	setFont(GLOBAL_MONOSPACE_FONT().withHeight(processor->getMainController()->getGlobalCodeFontSize()));

	processor->getMainController()->getFontSizeChangeBroadcaster().addChangeListener(this);
}

JavascriptCodeEditor::~JavascriptCodeEditor()
{
	currentPopup = nullptr;

	

	processor->getMainController()->getFontSizeChangeBroadcaster().removeChangeListener(this);

	scriptProcessor = nullptr;
	processor = nullptr;


	currentModalWindow.deleteAndZero();

	stopTimer();
}

void JavascriptCodeEditor::changeListenerCallback(SafeChangeBroadcaster *)
{
	float newFontSize = processor->getMainController()->getGlobalCodeFontSize();

	Font newFont = GLOBAL_MONOSPACE_FONT().withHeight(newFontSize);

	setFont(newFont);
}

void JavascriptCodeEditor::timerCallback()
{
	
	stopTimer();
}

void JavascriptCodeEditor::focusGained(FocusChangeType)
{
#if USE_BACKEND
	if (findParentComponentOfClass<BackendProcessorEditor>() != nullptr)
	{
		grabCopyAndPasteFocus();
	}
#endif
}

String JavascriptCodeEditor::getObjectTypeName()
{
	return "Script Editor";
}

void JavascriptCodeEditor::copyAction()
{
	SystemClipboard::copyTextToClipboard(getTextInRange(getHighlightedRegion()));
}

void JavascriptCodeEditor::pasteAction()
{
	getDocument().replaceSection(getSelectionStart().getPosition(), getSelectionEnd().getPosition(), SystemClipboard::getTextFromClipboard());
}

bool JavascriptCodeEditor::isInterestedInDragSource(const SourceDetails &dragSourceDetails)
{
	return dragSourceDetails.description.isArray() && dragSourceDetails.description.size() == 2;
}

void JavascriptCodeEditor::itemDropped(const SourceDetails &dragSourceDetails)
{
	String toInsert = dragSourceDetails.description[2];

	insertTextAtCaret(toInsert);
}

void JavascriptCodeEditor::itemDragEnter(const SourceDetails &dragSourceDetails)
{
	const Identifier identifier = Identifier(dragSourceDetails.description[1].toString());

	positionFound = selectJSONTag(identifier) ? JSONFound : NoJSONFound;

	repaint();
}

void JavascriptCodeEditor::itemDragMove(const SourceDetails &dragSourceDetails)
{
	if (positionFound == NoJSONFound)
	{
		Point<int> pos = dragSourceDetails.localPosition;
		moveCaretTo(getPositionAt(pos.x, pos.y), false);

		const int currentCharPosition = getCaretPos().getPosition();
		setHighlightedRegion(Range<int>(currentCharPosition, currentCharPosition));

		repaint();
	}
	if (positionFound == JSONFound) return;
}

void JavascriptCodeEditor::selectLineAfterDefinition(Identifier identifier)
{
    const String regexp = "(const)?\\s*(global|var|reg)?\\s*" + identifier.toString() + "\\s*=\\s*.*;[\\n\\r]";
		
	const String allText = getDocument().getAllContent();

	StringArray sa = RegexFunctions::getMatches(regexp, allText, nullptr);

	if(sa.size() > 0)
	{
		const String match = sa[0];

		int startIndex = allText.indexOf(match) + match.length();

		CodeDocument::Position pos(getDocument(), startIndex);

		moveCaretTo(pos, false);
	}

#if 0
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
#endif
        
  

}

bool JavascriptCodeEditor::selectJSONTag(const Identifier &identifier)
{
	String startLine;
	startLine << "// [JSON " << identifier.toString() << "]";

	String endLine;
	endLine << "// [/JSON " << identifier.toString() << "]";


	String allText = getDocument().getAllContent();

	const int startIndex = allText.indexOf(startLine);

	if (startIndex == -1) return false;

	const int endIndex = allText.indexOf(endLine);

	if (endIndex == -1)
	{
		return false;
	}

	setHighlightedRegion(Range<int>(startIndex, endIndex + endLine.length()));

	return true;

#if 0

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
#endif
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

	String s = getTextInRange(getHighlightedRegion()); 
	
	if (s == "include")
	{
		CodeDocument::Position start = getSelectionEnd().movedBy(2);
		CodeDocument::Position end = start;

		while (end.getCharacter() != '\"' && start.getLineNumber() == end.getLineNumber())
		{
			end.moveBy(1);
		};

		String fileName = getTextInRange(Range<int>(start.getPosition(), end.getPosition()));

		m.addItem(110, "Open " + fileName + " in editor popup");
		m.addSeparator();
	}

    CodeEditorComponent::addPopupMenuItems(m, e);
    
    
    
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
	else if (menuId == 110)
	{
		CodeDocument::Position start = getSelectionEnd().movedBy(2);
		CodeDocument::Position end = start;

		while (end.getCharacter() != '\"' && start.getLineNumber() == end.getLineNumber())
		{
			end.moveBy(1);
		};

		String fileName = getTextInRange(Range<int>(start.getPosition(), end.getPosition()));

		for (int i = 0; i < scriptProcessor->getNumWatchedFiles(); i++)
		{
			if (scriptProcessor->getWatchedFile(i).getFileName() == fileName)
			{
				scriptProcessor->showPopupForFile(i);
			}
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

Range<int> JavascriptCodeEditor::getCurrentTokenRange() const
{
	CodeDocument::Position tokenStart = getCaretPos();
	CodeDocument::Position tokenEnd(tokenStart);
	getDocument().findTokenContaining(tokenStart, tokenStart, tokenEnd);

	return Range<int>(tokenStart.getPosition(), tokenEnd.getPosition());
}

void JavascriptCodeEditor::closeAutoCompleteNew(const String returnString)
{
	Desktop::getInstance().getAnimator().fadeOut(currentPopup, 200);

	currentPopup = nullptr;

	if (returnString.isNotEmpty())
	{
		Range<int> tokenRange = getCurrentTokenRange();

		getDocument().replaceSection(tokenRange.getStart(), tokenRange.getEnd(), returnString);

		Range<int> parameterRange = Helpers::getFunctionParameterTextRange(getCaretPos());

		if (!parameterRange.isEmpty())
			setHighlightedRegion(parameterRange);

		else if (parameterRange.getStart() != 0)
			moveCaretTo(CodeDocument::Position(getDocument(), parameterRange.getStart()), false);
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


void JavascriptCodeEditor::paintOverChildren(Graphics& g)
{
	CopyPasteTarget::paintOutlineIfSelected(g);
}


bool JavascriptCodeEditor::isNothingSelected() const
{
	return getSelectionStart() == getSelectionEnd();
}

void JavascriptCodeEditor::handleDoubleCharacter(const KeyPress &k, char openCharacter, char closeCharacter)
{

	// Insert 
	if ((char)k.getTextCharacter() == openCharacter)
	{
		juce_wchar next = getCaretPos().getCharacter();

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
		(char)getCaretPos().movedBy(-1).getCharacter() == openCharacter &&
		(char)getCaretPos().getCharacter() == closeCharacter)
	{
		getDocument().deleteSection(getCaretPos(), getCaretPos().movedBy(1));
	}
}

bool JavascriptCodeEditor::keyPressed(const KeyPress& k)
{
#if USE_BACKEND
	handleDoubleCharacter(k, '(', ')');
	handleDoubleCharacter(k, '[', ']');
	handleDoubleCharacter(k, '\"', '\"');
	
	

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
		Helpers::getIndentForCurrentBlock(pos, getTabString(getTabSize()), blockIndent, lastLineIndent);

		moveCaretToEndOfLine(false);
        
        insertTextAtCaret(";" + getDocument().getNewLineCharacters() + lastLineIndent);
        
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



void JavascriptCodeEditor::handleReturnKey()
{
	CodeEditorComponent::handleReturnKey();
	CodeDocument::Position pos(getCaretPos());

	String blockIndent, lastLineIndent;
	Helpers::getIndentForCurrentBlock(pos, getTabString(getTabSize()), blockIndent, lastLineIndent);

	const String remainderOfBrokenLine(pos.getLineText());
	const int numLeadingWSChars = Helpers::getLeadingWhitespace(remainderOfBrokenLine).length();

	if (numLeadingWSChars > 0)
		getDocument().deleteSection(pos, pos.movedBy(numLeadingWSChars));

if (remainderOfBrokenLine.trimStart().startsWithChar('}'))
		insertTextAtCaret(blockIndent);
	else
		insertTextAtCaret(lastLineIndent);

	const String previousLine(pos.movedByLines(-1).getLineText());
	const String trimmedPreviousLine(previousLine.trim());

	if ((trimmedPreviousLine.startsWith("if ")
		|| trimmedPreviousLine.startsWith("if(")
		|| trimmedPreviousLine.startsWith("for ")
		|| trimmedPreviousLine.startsWith("for(")
		|| trimmedPreviousLine.startsWith("while(")
		|| trimmedPreviousLine.startsWith("while "))
		&& trimmedPreviousLine.endsWithChar(')'))
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
}

void JavascriptCodeEditor::insertTextAtCaret(const String& newText)
{
	if (getHighlightedRegion().isEmpty())
	{
		const CodeDocument::Position pos(getCaretPos());

		if ((newText == "{" || newText == "}")
			&& pos.getLineNumber() > 0
			&& pos.getLineText().trim().isEmpty())
		{
			moveCaretToStartOfLine(true);

			String blockIndent, lastLineIndent;
			if (Helpers::getIndentForCurrentBlock(pos, getTabString(getTabSize()), blockIndent, lastLineIndent))
			{
				insertTextAtCaret(blockIndent);

				if (newText == "{")
					insertTabAtCaret();
			}
		}
	}

#if JUCE_MAC

	if (currentPopup != nullptr)
	{
		if (newText.length() == 1)
		{
			KeyPress k = KeyPress(newText.getLastCharacter());

			currentPopup->handleEditorKeyPress(k);
		}
	}

#endif

	CodeEditorComponent::insertTextAtCaret(newText);
}

String JavascriptCodeEditor::Helpers::getLeadingWhitespace(String line)
{
	line = line.removeCharacters("\r\n");
	const String::CharPointerType endOfLeadingWS(line.getCharPointer().findEndOfWhitespace());
	return String(line.getCharPointer(), endOfLeadingWS);
}

int JavascriptCodeEditor::Helpers::getBraceCount(String::CharPointerType line)
{
	int braces = 0;

	for (;;)
	{
		const juce_wchar c = line.getAndAdvance();

		if (c == 0)                         break;
		else if (c == '{')                  ++braces;
		else if (c == '}')                  --braces;
		else if (c == '/')                  { if (*line == '/') break; }
		else if (c == '"' || c == '\'')     { while (!(line.isEmpty() || line.getAndAdvance() == c)) {} }
	}

	return braces;
}

bool JavascriptCodeEditor::Helpers::getIndentForCurrentBlock(CodeDocument::Position pos, const String& tab, String& blockIndent, String& lastLineIndent)
{
	int braceCount = 0;
	bool indentFound = false;

	while (pos.getLineNumber() > 0)
	{
		pos = pos.movedByLines(-1);

		const String line(pos.getLineText());
		const String trimmedLine(line.trimStart());

		braceCount += getBraceCount(trimmedLine.getCharPointer());

		if (braceCount > 0)
		{
			blockIndent = getLeadingWhitespace(line);
			if (!indentFound)
				lastLineIndent = blockIndent + tab;

			return true;
		}

		if ((!indentFound) && trimmedLine.isNotEmpty())
		{
			indentFound = true;
			lastLineIndent = getLeadingWhitespace(line);
		}
	}

	return false;
}


char JavascriptCodeEditor::Helpers::getCharacterAtCaret(CodeDocument::Position pos, bool beforeCaret /*= false*/)
{
	if (beforeCaret)
	{
		if (pos.getPosition() == 0) return 0;

		pos.moveBy(-1);

		return (char)pos.getCharacter();
	}
	else
	{
		return (char)pos.getCharacter();
	}
}

Range<int> JavascriptCodeEditor::Helpers::getFunctionParameterTextRange(CodeDocument::Position pos)
{
	Range<int> returnRange;

	pos.moveBy(-1);

	if (pos.getCharacter() == ')')
	{
		returnRange.setEnd(pos.getPosition());

		pos.moveBy(-1);

		if (pos.getCharacter() == '(')
		{
			return Range<int>();
		}
		else
		{
			while (pos.getCharacter() != '(' && pos.getIndexInLine() > 0)
			{
				pos.moveBy(-1);
			}

			returnRange.setStart(pos.getPosition() + 1);
			return returnRange;
		}
	}
	else if (pos.getCharacter() == '\n')
	{
		while (pos.getCharacter() != '\t' && pos.getPosition() > 0)
		{
			pos.moveBy(-1);
		}

		returnRange.setStart(pos.getPosition() + 1);
		return returnRange;
	}

	return returnRange;
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

	if (tokenText.containsChar('.'))
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

JavascriptCodeEditor::AutoCompletePopup::~AutoCompletePopup()
{
	infoBox = nullptr;
	listbox = nullptr;

	allInfo.clear();
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

KeyboardFocusTraverser* JavascriptCodeEditor::AutoCompletePopup::createFocusTraverser()
{
	return new AllToTheEditorTraverser(editor);
}

int JavascriptCodeEditor::AutoCompletePopup::getNumRows()
{
	return visibleInfo.size();
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

void JavascriptCodeEditor::AutoCompletePopup::listBoxItemClicked(int row, const MouseEvent &)
{
	selectRowInfo(row);
}

void JavascriptCodeEditor::AutoCompletePopup::listBoxItemDoubleClicked(int row, const MouseEvent &)
{
	editor->closeAutoCompleteNew(visibleInfo[row]->name);
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

void JavascriptCodeEditor::AutoCompletePopup::paint(Graphics& g)
{
	g.setColour(Colour(0xFFBBBBBB));
	g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 3.0f);
}

void JavascriptCodeEditor::AutoCompletePopup::resized()
{
	infoBox->setBounds(3, 3, getWidth() - 6, 3 * fontHeight - 6);
	listbox->setBounds(3, 3 * fontHeight + 3, getWidth() - 6, getHeight() - 3 * fontHeight - 6);
}

void JavascriptCodeEditor::AutoCompletePopup::selectRowInfo(int rowIndex)
{
	listbox->repaintRow(currentlySelectedBox);

	currentlySelectedBox = rowIndex;

	listbox->selectRow(currentlySelectedBox);
	listbox->repaintRow(currentlySelectedBox);
	infoBox->setInfo(visibleInfo[currentlySelectedBox]);
}

void JavascriptCodeEditor::AutoCompletePopup::rebuildVisibleItems(const String &selection)
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
	setSize((int)maxWidth + 6, height + 6);
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

	if (!file.existsAsFile()) editor->setEnabled(false);

	setSize(800, 800);
}

PopupIncludeEditor::~PopupIncludeEditor()
{
	editor = nullptr;
	doc = nullptr;
	tokeniser = nullptr;

	Processor *p = dynamic_cast<Processor*>(sp);

	if (p != nullptr)
		p->setEditorState(p->getEditorStateForIndex(JavascriptMidiProcessor::externalPopupShown), false);

	sp = nullptr;
	p = nullptr;
}

void PopupIncludeEditor::timerCallback()
{
	resultLabel->setColour(Label::backgroundColourId, lastCompileOk ? Colours::green.withBrightness(0.1f) : Colours::red.withBrightness((0.1f)));
	stopTimer();
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

PopupIncludeEditorWindow::PopupIncludeEditorWindow(File f, JavascriptProcessor *s) :
DocumentWindow("Editing external file: " + f.getFullPathName(), Colours::black, DocumentWindow::allButtons, true),
file(f)
{
	editor = new PopupIncludeEditor(s, f);

	setContentNonOwned(editor, true);

	setUsingNativeTitleBar(true);


	centreWithSize(800, 800);

	setResizable(true, true);

	setVisible(true);
}

void PopupIncludeEditorWindow::paint(Graphics &g)
{
	if (editor != nullptr)
	{
		g.setColour(Colour(0xFF262626));
		g.fillAll();
	}
}

bool PopupIncludeEditorWindow::keyPressed(const KeyPress& key)
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
}

void PopupIncludeEditorWindow::closeButtonPressed()
{
	if (Desktop::getInstance().getKioskModeComponent() == this)
	{
		Desktop::getInstance().setKioskModeComponent(nullptr, false);
	}

	delete this;
}

CodeEditorWrapper::CodeEditorWrapper(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p)
{
	addAndMakeVisible(editor = new JavascriptCodeEditor(document, codeTokeniser, p));



	restrainer.setMinimumHeight(50);
	restrainer.setMaximumHeight(600);

	addAndMakeVisible(dragger = new ResizableEdgeComponent(this, &restrainer, ResizableEdgeComponent::Edge::bottomEdge));

	dragger->addMouseListener(this, true);

	setSize(200, 340);


	currentHeight = getHeight();
}

CodeEditorWrapper::~CodeEditorWrapper()
{
	editor = nullptr;
}

void CodeEditorWrapper::resized()
{
	editor->setBounds(getLocalBounds());

	dragger->setBounds(0, getHeight() - 5, getWidth(), 5);
}

void CodeEditorWrapper::timerCallback()
{
#if USE_BACKEND
	ProcessorEditorBody *body = dynamic_cast<ProcessorEditorBody*>(getParentComponent());

	resized();

	if (body != nullptr)
	{
		currentHeight = getHeight();

		body->refreshBodySize();
	}
#endif
}

void CodeEditorWrapper::mouseDown(const MouseEvent &m)
{
	if (m.eventComponent == dragger) startTimer(30);
}

void CodeEditorWrapper::mouseUp(const MouseEvent &)
{
	stopTimer();
}
