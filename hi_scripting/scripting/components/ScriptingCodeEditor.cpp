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



JavascriptCodeEditor::JavascriptCodeEditor(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p, const Identifier& snippetId_) :
CodeEditorComponent(document, codeTokeniser),
scriptProcessor(p),
processor(dynamic_cast<Processor*>(p)),
snippetId(snippetId_)
{

	p->addEditor(this);

	getGutterComponent()->addMouseListener(this, true);

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

	if (processor.get() != nullptr)
	{
		dynamic_cast<JavascriptProcessor*>(processor.get())->removeEditor(this);
		processor->getMainController()->getFontSizeChangeBroadcaster().removeChangeListener(this);
	}

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
	if (findParentComponentOfClass<BackendRootWindow>() != nullptr)
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

	StringArray sa = RegexFunctions::getFirstMatch(regexp, allText, nullptr);

	if(sa.size() > 0)
	{
		const String match = sa[0];

		int startIndex = allText.indexOf(match) + match.length();

		CodeDocument::Position pos(getDocument(), startIndex);

		moveCaretTo(pos, false);
	}
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
}

bool JavascriptCodeEditor::componentIsDefinedWithFactoryMethod(const Identifier& identifier)
{
	const String regexp = "(const)?\\s*(global|var|reg)?\\s*" + identifier.toString() + "\\s*=\\s*(.*)\\(.*;";

	const String allText = getDocument().getAllContent();

	StringArray sa = RegexFunctions::getFirstMatch(regexp, allText, nullptr);

	if (sa.size() == 4)
	{
		const String def = sa[3];
		return !sa[3].contains("Content.add");
	}

	return false;
}

String JavascriptCodeEditor::createNewDefinitionWithFactoryMethod(const String &oldId, const String &newId, int newX, int newY)
{
	const String regexp = "(const)?\\s*(global|var|reg)?\\s*" + oldId + "\\s*=\\s*([\\w\\.]+)\\(\\\"(\\w+)\\\"\\s*,\\s*(\\d+)\\s*,\\s*(\\d+)(.*)\\);";
	const String allText = getDocument().getAllContent();
	StringArray sa = RegexFunctions::getFirstMatch(regexp, allText, nullptr);

	if (sa.size() == 8)
	{
		const String factoryMethodName = sa[3];

		const String additionalParameters = sa[7];

		String line = "\nconst var " + newId + " = " + factoryMethodName + "(\"" + newId + "\", " + String(newX) + ", " + String(newY) + additionalParameters +  ");\n";
		return line;
	}

	return String();
}

void JavascriptCodeEditor::createMissingCaseStatementsForComponents()
{
	String allText = getDocument().getAllContent();

	if (allText.startsWith("function onControl"))
	{
		int switchIndex = allText.indexOf("switch");

		if (switchIndex == -1)
		{
			const String switchStatement = "switch(number)\r\n\t{\r\n\t}";

			CodeDocument::Position(getDocument(), 2, 0);

			insertTextAtCaret(switchStatement);

			allText = getDocument().getAllContent();

			switchIndex = allText.indexOf("switch");
		}

		if (switchIndex != -1)
		{
			auto c = allText.getCharPointer();
			c += switchIndex;

			while (*c != '{') 
				c++;

			c++;

			const int startIndex = (int)(c - allText.getCharPointer());

			CodeDocument::Position insertPos(getDocument(), startIndex);

			ProcessorWithScriptingContent* ps = dynamic_cast<ProcessorWithScriptingContent*>(processor.get());

			ScriptingApi::Content* content = ps->getScriptingContent();

			int count = 0;

			for (int i = content->getNumComponents()-1; i >= 0 ; i--)
			{
				const String widgetName = content->getComponent(i)->getName().toString();

                const String reg = "case " + widgetName;

                const bool hasCaseStatement = allText.contains(reg);

				if (!hasCaseStatement)
				{
					moveCaretTo(insertPos, false);

					String newCaseStatement = "\n\t\tcase " + widgetName + ":\n\t\t{\n";
                    newCaseStatement << "\t\t\t// Insert logic here...\n\t\t\tbreak;\n\t\t}";

					insertTextAtCaret(newCaseStatement);
					count++;
				}
			}
            
            PresetHandler::showMessageWindow(String(count) + " case statements added", "", PresetHandler::IconType::Info);
            
		}
	}
	else
	{
		PresetHandler::showMessageWindow("Not in the onControl callback", "Case statements can only be created in the onControl callback", PresetHandler::IconType::Warning);
	}

}

void JavascriptCodeEditor::focusLost(FocusChangeType )
{
#if USE_BACKEND
    
    BackendRootWindow *editor = findParentComponentOfClass<BackendRootWindow>();

    if(editor != nullptr)
    {
        MainController *mc = dynamic_cast<MainController*>(editor->getAudioProcessor());
        
        mc->setLastActiveEditor(this, getCaretPos());
    }
#endif
}

void JavascriptCodeEditor::addPopupMenuItems(PopupMenu &menu, const MouseEvent *e)
{
#if USE_BACKEND

    menu.setLookAndFeel(&plaf);

	StringArray all = StringArray::fromLines(getDocument().getAllContent());

	bookmarks.clear();

	for (int i = 0; i < all.size(); i++)
	{
		if (all[i].trim().startsWith("//!"))
		{
			bookmarks.add(Bookmarks(all[i], i));
		}
	}

	menu.addSectionHeader("Code Bookmarks");

	if (bookmarks.size() != 0)
	{
		for (int i = 0; i < bookmarks.size(); i++)
		{
			menu.addItem(bookmarkOffset + i, bookmarks[i].title);
		}

		menu.addSeparator();
	}

	menu.addItem(ContextActions::AddCodeBookmark, "Add code bookmark");
	menu.addSeparator();

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

		menu.addItem(ContextActions::OpenExternalFile, "Open " + fileName + " in editor popup");
		menu.addSeparator();
	}

	menu.addItem(ContextActions::OpenInPopup, "Open in external window", true, false);

	menu.addItem(ContextActions::JumpToDefinition, "Jump to definition", true, false);

    CodeEditorComponent::addPopupMenuItems(menu, e);
    
    if(true)
    {
		menu.addSeparator();
		menu.addSectionHeader("Refactoring");
		menu.addItem(ContextActions::SearchReplace, "Search & replace");

		const String selection = getTextInRange(getHighlightedRegion()).trimEnd().trimStart();
		const bool isUIDefinitionSelected = selection.startsWith("const var");

		menu.addItem(ContextActions::CreateUiFactoryMethod, "Create UI factory method from selection", isUIDefinitionSelected);
		menu.addItem(ContextActions::AddMissingCaseStatements, "Add missing case statements", true);
        menu.addSeparator();
        menu.addSectionHeader("Import / Export");
        menu.addItem(ContextActions::SaveScriptFile, "Save Script To File");
        menu.addItem(ContextActions::LoadScriptFile, "Load Script From File");
        menu.addSeparator();
        menu.addItem(ContextActions::SaveScriptClipboard, "Save Script to Clipboard");
        menu.addItem(ContextActions::LoadScriptClipboard, "Load Script from Clipboard");
        menu.addSeparator();
		menu.addItem(ContextActions::ExportAsCompressedScript, "Export as compressed script");
		menu.addItem(ContextActions::ImportCompressedScript, "Import compressed script");
    }
    
#else

	ignoreUnused(menu, e);

#endif
};

void JavascriptCodeEditor::performPopupMenuAction(int menuId)
{
#if USE_BACKEND
    JavascriptProcessor *s = scriptProcessor;
    
	Processor* p = dynamic_cast<Processor*>(s);

    ScriptingEditor *editor = findParentComponentOfClass<ScriptingEditor>();
    
	ContextActions action = (ContextActions)menuId;

	switch (action)
	{
	case JavascriptCodeEditor::SearchReplace:
	{
		CodeReplacer * replacer = new CodeReplacer(this);

		currentModalWindow = replacer;

		replacer->setModalBaseWindowComponent(this);
		replacer->getTextEditor("search")->grabKeyboardFocus();

		return;
	}
	case JavascriptCodeEditor::CreateUiFactoryMethod:
	{
		const String selection = getTextInRange(getHighlightedRegion()).trimEnd().trimStart();
		const String newText = CodeReplacer::createFactoryMethod(selection);

		insertTextAtCaret(newText);

		return;
	}
	case JavascriptCodeEditor::AddMissingCaseStatements:
	{
		createMissingCaseStatementsForComponents();
		return;
	}
	case JavascriptCodeEditor::JumpToDefinition:
	{
		CodeDocument::Position start = getCaretPos();
		CodeDocument::Position end = start;

		getDocument().findTokenContaining(start, start, end);

		const String token = getDocument().getTextBetween(start, end);

		if (token.isNotEmpty())
		{
			Result result = Result::ok();

			var t = s->getScriptEngine()->evaluate(token, &result);

			if (result.wasOk())
			{
				if (auto obj = dynamic_cast<HiseJavascriptEngine::RootObject::InlineFunction::Object*>(t.getObject()))
				{
					auto parent = findParentComponentOfClass<ScriptingEditor>();

					DebugableObject::Helpers::gotoLocation(parent, s, obj->location);
				}
			}
		}

		return;
	}
		
	case JavascriptCodeEditor::SaveScriptFile:
	{
		if (editor != nullptr)
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

		return;
	}
	case JavascriptCodeEditor::LoadScriptFile:
	{
		FileChooser scriptLoader("Please select the script you want to load",
			File(GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
			"*.js");

		if (scriptLoader.browseForFileToOpen())
		{
			String script = scriptLoader.getResult().loadFileAsString().removeCharacters("\r");
			const bool success = s->parseSnippetsFromString(script);

			if (success)
			{
				editor->compileScript();
				debugToConsole(p, "Script loaded from " + scriptLoader.getResult().getFullPathName());
			}
		}

		return;
	}
	case JavascriptCodeEditor::ExportAsCompressedScript:
	{
		const String compressedScript = s->getBase64CompressedScript();
		const String scriptName = PresetHandler::getCustomName("Compressed Script") + ".cjs";
		File f = GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile(scriptName);

		if (!f.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite", "The file " + scriptName + " already exists. Do you want to overwrite it?"))
		{
			f.deleteFile();
			f.replaceWithText(compressedScript);
		}

		return;
	}
	case JavascriptCodeEditor::ImportCompressedScript:
	{
		FileChooser scriptLoader("Please select the compressed script you want to load",
			File(GET_PROJECT_HANDLER(p).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
			"*.cjs");

		if (scriptLoader.browseForFileToOpen())
		{
			String compressedScript = scriptLoader.getResult().loadFileAsString();

			const bool success = s->restoreBase64CompressedScript(compressedScript);

			if (success)
			{
				editor->compileScript();
				debugToConsole(p, "Compressed Script loaded from " + scriptLoader.getResult().getFullPathName());
			}
		}

		return;
	}
	case JavascriptCodeEditor::SaveScriptClipboard:
	{
		String x;
		s->mergeCallbacksToScript(x);
		SystemClipboard::copyTextToClipboard(x);

		debugToConsole(p, "Script exported to Clipboard.");

		return;
	}
	case JavascriptCodeEditor::LoadScriptClipboard:
	{
		String x = String(SystemClipboard::getTextFromClipboard()).removeCharacters("\r");

		if (x.containsNonWhitespaceChars() && PresetHandler::showYesNoWindow("Replace Script?", "Do you want to replace the script?"))
		{
			const bool success = s->parseSnippetsFromString(x);

			if (success)
				editor->compileScript();
		}

		return;
	}
	case JavascriptCodeEditor::AddCodeBookmark:
	{
		const String bookmarkName = PresetHandler::getCustomName("Bookmark");

		if (bookmarkName.isNotEmpty())
		{
			String bookmarkLine = "//! ";
			const int numChars = bookmarkLine.length() + bookmarkName.length();

			for (int i = numChars; i < 80; i++) 
				bookmarkLine << '=';

			bookmarkLine << " " << bookmarkName << "\r\n";
			moveCaretToStartOfLine(false);
			getDocument().insertText(getCaretPos(), bookmarkLine);
		}

		return;
	}
	case JavascriptCodeEditor::OpenExternalFile:
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

		return;
	}
	case JavascriptCodeEditor::OpenInPopup:
	{
		const CodeDocument* thisDocument = &getDocument();

		for (int i = 0; i < scriptProcessor->getNumSnippets(); i++)
		{
			if (scriptProcessor->getSnippet(i) == thisDocument)
			{
				scriptProcessor->showPopupForCallback(scriptProcessor->getSnippet(i)->getCallbackName(), getCaretPos().getIndexInLine(), getCaretPos().getLineNumber());
			}
		}

		return;
	}
	default:
		break;
	}

	if (menuId >= bookmarkOffset)
	{
		const int index = menuId - bookmarkOffset;

		const int lineNumber = bookmarks[index].line;

		scrollToLine(lineNumber);

		return;
	}
    
	
	CodeEditorComponent::performPopupMenuAction(menuId);
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
		Component *editor = findParentComponentOfClass<BackendRootWindow>();

		if (editor == nullptr)
		{
			editor = findParentComponentOfClass<PopupIncludeEditorWindow>();
		}

		if (editor != nullptr)
		{
			editor->addAndMakeVisible(currentPopup);

			CodeDocument::Position current = getCaretPos();
			moveCaretTo(CodeDocument::Position(getDocument(), tokenRange.getStart()), false);

			Rectangle<int> caretArea = editor->getLocalArea(this, getCaretRectangle());
			Point<int> topLeft = caretArea.getBottomLeft();

			if (caretArea.getY() > editor->getHeight() - currentPopup->getHeight())
			{
				topLeft = Point<int>(topLeft.getX(), jmax<int>(0, caretArea.getY() - currentPopup->getHeight()));
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
	Helpers::findAdvancedTokenRange(tokenStart, tokenStart, tokenEnd);

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
	if (highlightedSelection.size() != 0)
	{
		highlightedSelection.clear();
		repaint();
	}
	else
	{
		if (currentPopup == nullptr) showAutoCompleteNew();
		else						 closeAutoCompleteNew(String());
	}
}


void JavascriptCodeEditor::paintOverChildren(Graphics& g)
{
	CopyPasteTarget::paintOutlineIfSelected(g);

	const int firstLine = getFirstLineOnScreen();
	const int numLinesShown = getNumLinesOnScreen();
	Range<int> lineRange(firstLine, firstLine + numLinesShown);

	if (highlightedSelection.size() != 0)
	{
		for (int i = 0; i < highlightedSelection.size(); i++)
		{
			CodeDocument::Position pos(getDocument(), highlightedSelection[i].getStart());

			const int lineIndex = pos.getLineNumber();
			
			if (lineRange.contains(lineIndex))
			{
				const int x = getCharacterBounds(pos).getX();
                const int y = getCharacterBounds(pos).getY();
                
				const int w = (int)(getCharWidth() * (float)highlightedSelection[i].getLength());
				const int h = getLineHeight();

				g.setColour(Colours::green.withAlpha(0.4f));
				g.fillRoundedRectangle((float)x-1.0f, (float)y, (float)w+2.0f, (float)h, 2.0f);
                g.setColour(Colours::white.withAlpha(0.5f));
                g.drawRoundedRectangle((float)x-1.0f, (float)y, (float)w+2.0f, (float)h, 2.0f, 1.0f);
			}
		}
	}

#if ENABLE_SCRIPTING_BREAKPOINTS
	if (scriptProcessor->anyBreakpointsActive())
	{

		int startLine = getFirstLineOnScreen();

		int endLine = startLine + getNumLinesOnScreen();

		for (int i = startLine; i < endLine; i++)
		{
			HiseJavascriptEngine::Breakpoint bp = scriptProcessor->getBreakpointForLine(snippetId, i);

			if(bp.lineNumber != -1)
			{
				const float x = 5.0f;
				const float y = (float)((bp.lineNumber - getFirstLineOnScreen()) * getLineHeight() + 1);

				const float w = (float)(getLineHeight() - 2);
				const float h = w;

				g.setColour(Colours::darkred.withAlpha(bp.hit ? 1.0f : 0.3f));
				g.fillEllipse(x, y, w, h);
				g.setColour(Colours::white.withAlpha(bp.hit ? 1.0f : 0.5f));
				g.drawEllipse(x, y, w, h, 1.0f);
				g.setFont(GLOBAL_MONOSPACE_FONT().withHeight((float)(getLineHeight() - 3)));
				g.drawText(String(bp.index+1), (int)x, (int)y, (int)w, (int)h, Justification::centred);
			}
		}
	}
#endif
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

void JavascriptCodeEditor::increaseMultiSelectionForCurrentToken()
{
	const auto selectedRange = getHighlightedRegion();
	
	if (!selectedRange.isEmpty() && highlightedSelection.size() == 0)
	{
		highlightedSelection.add(selectedRange);

		moveCaretTo(getSelectionStart(), false);

		repaint();
		return;
	}

	int startIndex = highlightedSelection.getLast().getEnd();
	
	const String currentToken = getTextInRange(highlightedSelection.getLast());
	const String allText = getDocument().getAllContent().substring(startIndex);

	const int nextIndex = allText.indexOf(currentToken);
	
	if (nextIndex != -1)
	{
		CodeRegion nextRegion;
		
		nextRegion.setStart(startIndex + nextIndex);
		nextRegion.setLength(highlightedSelection.getFirst().getLength());

		highlightedSelection.addIfNotAlreadyThere(nextRegion);
	}

	repaint();
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

		CodeDocument::Position currentCaretPosition = getCaretPos();

		StringArray allLines = StringArray::fromLines(getDocument().getAllContent());

		for(int i = startLine; i <= endLine; i++)
		{
			if (isCommented) allLines.set(i, allLines[i].trimCharactersAtStart("/"));
			else allLines.set(i, "//" + allLines[i]);
		}

		getDocument().replaceAllContent(allLines.joinIntoString(getDocument().getNewLineCharacters()));

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
		CodeReplacer * replacer = new CodeReplacer(this);

		currentModalWindow = replacer;

		replacer->setModalBaseWindowComponent(this);
		replacer->getTextEditor("search")->grabKeyboardFocus();
		return true;
	}
	else if (k.isKeyCode('D') && k.getModifiers().isCommandDown())
	{
		increaseMultiSelectionForCurrentToken();
		return true;
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

    if (k == KeyPress::backspaceKey && highlightedSelection.size() != 0)
    {
        Range<int> firstSelectedPosition = highlightedSelection.getFirst();
        
        firstSelectedPosition.setEnd(firstSelectedPosition.getEnd() + 1);
        
        if (firstSelectedPosition.contains(getCaretPos().getPosition()))
        {
            const int offsetInSelection = getCaretPos().getPosition() - firstSelectedPosition.getStart();
            
            for (int i = 0; i < highlightedSelection.size(); i++)
            {
                CodeRegion* r = &highlightedSelection.getRawDataPointer()[i];
                
                r->setStart(r->getStart() - i);
                r->setLength(firstSelectedPosition.getLength() - 2);
                
                CodeDocument::Position nextPos(getDocument(), r->getStart() + offsetInSelection + 1);
                
                if (i != 0)
                {
                    getDocument().deleteSection(nextPos.movedBy(-1), nextPos);
                }
            }
            
            repaint();
        }
    }

    

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

    if (highlightedSelection.size() != 0)
    {
        Range<int> firstSelectedPosition = highlightedSelection.getFirst();
        
        firstSelectedPosition.setEnd(firstSelectedPosition.getEnd() + 1);
        
        if (firstSelectedPosition.contains(getCaretPos().getPosition()))
        {
            const int offsetInSelection = getCaretPos().getPosition() - firstSelectedPosition.getStart();
            
            for (int i = 0; i < highlightedSelection.size(); i++)
            {
                CodeRegion* r = &highlightedSelection.getRawDataPointer()[i];
                
                r->setStart(r->getStart() + i);
                r->setLength(firstSelectedPosition.getLength());
                
                if (i != 0)
                {
                    CodeDocument::Position nextPos(getDocument(), r->getStart() + offsetInSelection - 1);
                    getDocument().insertText(nextPos, newText);
                }
            }
            
            repaint();
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


void JavascriptCodeEditor::mouseDown(const MouseEvent& e)
{
	CodeEditorComponent::mouseDown(e);

#if ENABLE_SCRIPTING_BREAKPOINTS
	if (e.x < 35)
	{
		if (e.mods.isShiftDown())
		{
			scriptProcessor->removeAllBreakpoints();
		}
		else
		{
			int lineNumber = e.y / getLineHeight() + getFirstLineOnScreen();
			CodeDocument::Position start(getDocument(), lineNumber, 0);

			int charNumber = start.getPosition();

			const String content = getDocument().getAllContent().substring(charNumber);

			HiseJavascriptEngine::RootObject::TokenIterator it(content, "");

			try
			{
				it.skipWhitespaceAndComments();
			}
			catch (String &)
			{

			}

			const int offsetToFirstToken = (int)(it.location.location - content.getCharPointer());

			CodeDocument::Position tokenStart(getDocument(), charNumber + offsetToFirstToken);

			scriptProcessor->toggleBreakpoint(snippetId, tokenStart.getLineNumber(), tokenStart.getPosition());
			repaint();
		}
	}
#endif
}

CodeEditorWrapper::CodeEditorWrapper(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p, const Identifier& snippetId)
{
	addAndMakeVisible(editor = new JavascriptCodeEditor(document, codeTokeniser, p, snippetId));

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
