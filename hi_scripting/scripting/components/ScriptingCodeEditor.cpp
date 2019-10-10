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

#include <regex>

namespace hise { using namespace juce;


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
	if (auto root = GET_BACKEND_ROOT_WINDOW(this))
	{
		grabCopyAndPasteFocus();

		root->getBackendProcessor()->setLastActiveEditor(this, getCaretPos());
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
	auto position = Helpers::getPositionAfterDefinition(getDocument(), identifier);

	if(position.getPosition() > 0)
		moveCaretTo(position, false);

}

bool JavascriptCodeEditor::selectJSONTag(const Identifier &identifier)
{
	auto range = Helpers::getJSONTag(getDocument(), identifier);

	if (range.isEmpty())
		return false;

	setHighlightedRegion(range);

	return true;
}

bool JavascriptCodeEditor::componentIsDefinedWithFactoryMethod(const Identifier& identifier)
{
	const String regexp = "(const)?\\s*(global|var|reg)?\\s*" + identifier.toString() + "\\s*=\\s*(.*)\\(.*;";

	const String allText = getDocument().getAllContent();

	StringArray sa = RegexFunctions::getFirstMatch(regexp, allText);

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
	StringArray sa = RegexFunctions::getFirstMatch(regexp, allText);

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
				const String cName = content->getComponent(i)->getName().toString();

                const String reg = "case " + cName;

                const bool hasCaseStatement = allText.contains(reg);

				if (!hasCaseStatement)
				{
					moveCaretTo(insertPos, false);

					String newCaseStatement = "\n\t\tcase " + cName + ":\n\t\t{\n";
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
    
	if (findParentComponentOfClass<ComponentWithBackendConnection>())
	{
		BackendRootWindow *editor = GET_BACKEND_ROOT_WINDOW(this);

		if (editor != nullptr)
		{
			MainController *mc = dynamic_cast<MainController*>(editor->getAudioProcessor());

			mc->setLastActiveEditor(this, getCaretPos());
		}
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
	menu.addItem(ContextActions::ClearAllBreakpoints, "Clear all breakpoints", scriptProcessor->anyBreakpointsActive());
	menu.addItem(ContextActions::FindAllOccurences, "Find all occurrences", true, false);

    CodeEditorComponent::addPopupMenuItems(menu, e);
    
    if(true)
    {
		menu.addSeparator();
		menu.addSectionHeader("Refactoring");
		menu.addItem(ContextActions::SearchReplace, "Search & replace");

		const String selection = getTextInRange(getHighlightedRegion()).trimEnd().trimStart();
		const bool isUIDefinitionSelected = selection.startsWith("const var");

		menu.addItem(ContextActions::CreateUiFactoryMethod, "Create UI factory method from selection", isUIDefinitionSelected);
		menu.addItem(ContextActions::ReplaceConstructorWithReference, "Replace addComponent with Content.getComponent()");
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
		menu.addSeparator();
		menu.addItem(ContextActions::MoveToExternalFile, "Move selection to external file");
		menu.addItem(ContextActions::InsertExternalFile, "Replace include with file content", s == "include");
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

		replacer->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(this));
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
	case JavascriptCodeEditor::ReplaceConstructorWithReference:
	{
		const String selection = getTextInRange(getHighlightedRegion()).trimEnd().trimStart();
		const String newText = CodeReplacer::createScriptComponentReference(selection);

		insertTextAtCaret(newText);
		return;
	}
	case ContextActions::ClearAllBreakpoints:
	{
		scriptProcessor->removeAllBreakpoints();
		repaint();
		return;
	}
	case JavascriptCodeEditor::JumpToDefinition:
	{
		auto token = getCurrentToken();

		const String namespaceId = Helpers::findNamespaceForPosition(getCaretPos());

		if (token.isNotEmpty())
		{
			const String c = namespaceId.isEmpty() ? token : namespaceId + "." + token;

			auto f = [c](Processor* p)
			{
				Result result = Result::ok();

				auto s = dynamic_cast<JavascriptProcessor*>(p);

				var t = s->getScriptEngine()->evaluate(c, &result);

				if (result.wasOk())
				{
					auto info = DebugableObject::Helpers::getDebugInformation(s->getScriptEngine(), t);

					auto f2 = [info, s]()
					{
						if (info != nullptr)
						{
							DebugableObject::Helpers::gotoLocation(dynamic_cast<Processor*>(s), info);
						}
					};

					MessageManager::callAsync(f2);
				}

				return SafeFunctionCall::OK;
			};

			processor->getMainController()->getKillStateHandler().killVoicesAndCall(processor, f, MainController::KillStateHandler::ScriptingThread);

			
		}

		return;
	}
	case ContextActions::FindAllOccurences:
	{
		ReferenceFinder * finder = new ReferenceFinder(this, s);

		finder->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(this));
		
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

			if (editor != nullptr && success)
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
	case JavascriptCodeEditor::MoveToExternalFile:
	{
		const String text = getDocument().getTextBetween(getSelectionStart(), getSelectionEnd());

		const String newFileName = PresetHandler::getCustomName("Script File", "Enter the file name for the external script file (without .js)");

		if (newFileName.isNotEmpty())
		{
			File scriptDirectory = GET_PROJECT_HANDLER(GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Scripts);

			File newFile = scriptDirectory.getChildFile(newFileName + ".js");

			if (!newFile.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite existing file", "Do you want to overwrite the file " + newFile.getFullPathName() + "?"))
			{
				newFile.replaceWithText(text);
			}

			String insertStatement = "include(\"" + newFile.getFileName() + "\");" + NewLine();

			getDocument().replaceSection(getSelectionStart().getPosition(), getSelectionEnd().getPosition(), insertStatement);
		}

		return;
	}
	case JavascriptCodeEditor::InsertExternalFile:
	{
		moveCaretToEndOfLine(true);

		const String text = getDocument().getTextBetween(getSelectionStart(), getSelectionEnd());

		String fileName = text.fromFirstOccurrenceOf("\"", false, true);
		fileName = fileName.upToLastOccurrenceOf("\"", false, true);

		File scriptDirectory = GET_PROJECT_HANDLER(GET_BACKEND_ROOT_WINDOW(this)->getMainSynthChain()).getSubDirectory(ProjectHandler::SubDirectories::Scripts);

		File scriptFile = scriptDirectory.getChildFile(fileName);

		if (scriptFile.existsAsFile())
		{
			const String content = scriptFile.loadFileAsString();

			getDocument().replaceSection(getSelectionStart().getPosition(), getSelectionEnd().getPosition(), content);
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

juce::String JavascriptCodeEditor::getCurrentToken() const
{
	CodeDocument::Position start = getCaretPos();
	CodeDocument::Position end = start;

	getDocument().findTokenContaining(start, start, end);

	

	return getDocument().getTextBetween(start, end);
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
		Component *editor = dynamic_cast<Component*>(findParentComponentOfClass<PanelWithProcessorConnection>());

		if(editor == nullptr || editor->getHeight() < 400)
			editor = dynamic_cast<Component*>(findParentComponentOfClass<ComponentWithBackendConnection>());

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

void JavascriptCodeEditor::closeAutoCompleteNew(String returnString)
{
	Desktop::getInstance().getAnimator().fadeOut(currentPopup, 200);

	currentPopup = nullptr;

	if (returnString.isNotEmpty())
	{
		Range<int> tokenRange = getCurrentTokenRange();

		auto currentNamespace = Helpers::findNamespaceForPosition(getCaretPos());

		if (currentNamespace.isNotEmpty() && returnString.startsWith(currentNamespace))
			returnString = returnString.replace(currentNamespace + ".", "");

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

	if (auto rootWindow = GET_BACKEND_ROOT_WINDOW(this))
	{
		if (rootWindow->getBackendProcessor()->getLastActiveEditor() == this)
		{
			g.setColour(Colour(SIGNAL_COLOUR));
			g.fillRect(0, 0, 4, 4);
		}
	}

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


MarkdownLink JavascriptCodeEditor::getLink() const
{
	if (currentPopup != nullptr)
		return currentPopup->getLink();

	return {};
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
	else if ((k.isKeyCode('f') || k.isKeyCode('F')) && k.getModifiers().isCommandDown()) // Ctrl + F
	{
		ReferenceFinder * finder = new ReferenceFinder(this, dynamic_cast<JavascriptProcessor*>(processor.get()));

		finder->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(this));
		return true;
	}
	else if ((k.isKeyCode('h') || k.isKeyCode('H')) && k.getModifiers().isCommandDown()) // Ctrl + F
	{
		CodeReplacer * replacer = new CodeReplacer(this);

		currentModalWindow = replacer;

		replacer->setModalBaseWindowComponent(this);
		replacer->getTextEditor("search")->grabKeyboardFocus();
		return true;
	}
	else if ((k.isKeyCode('d') || k.isKeyCode('D'))  && k.getModifiers().isCommandDown())
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
	else if (k.isKeyCode(KeyPress::F9Key))
	{
		if (k.getModifiers().isShiftDown())
		{
			scriptProcessor->removeAllBreakpoints();
			repaint();
			return true;
		}
		else
		{
			scriptProcessor->toggleBreakpoint(snippetId, getCaretPos().getLineNumber(), getCaretPos().getPosition());
			repaint();
			return true;
		}
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
	if ((k.isKeyCode('x') || k.isKeyCode('X')) && k.getModifiers().isCommandDown())
	{
		if (getHighlightedRegion().isEmpty())
		{
			auto indexInLine = getCaretPos().getIndexInLine();
			moveCaretToStartOfLine(false);
			
			moveCaretDown(true);
			cutToClipboard();

			auto preMove = getCaretPos();

			auto postMove = preMove.movedBy(indexInLine);

			if (preMove.getLineNumber() == postMove.getLineNumber())
				moveCaretTo(postMove, false);
			else
				moveCaretToEndOfLine(false);

			return true;
		}
	}
	if ((k.isKeyCode('c') || k.isKeyCode('C')) && k.getModifiers().isCommandDown())
	{
		if (getHighlightedRegion().isEmpty())
		{
			auto prePos = getCaretPos();
			moveCaretToStartOfLine(false);

			moveCaretDown(true);
			copyToClipboard();

			moveCaretTo(prePos, false);

			return true;
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
			repaint();
		}
		else if (e.mods.isCommandDown())
		{
			int lineNumber = e.y / getLineHeight() + getFirstLineOnScreen();
			CodeDocument::Position start(getDocument(), lineNumber, 0);

			int charNumber = start.getPosition();

			const String content = getDocument().getAllContent().substring(charNumber);

			const int offsetToFirstToken = Helpers::getOffsetToFirstToken(content);

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

Range<int> JavascriptCodeEditor::Helpers::getJSONTag(const CodeDocument& doc, const Identifier& id)
{
    String startLine;
	startLine << "// [JSON " << id.toString() << "]";

	String endLine;
	endLine << "// [/JSON " << id.toString() << "]";

	String allText = doc.getAllContent();

	const int startIndex = allText.indexOf(startLine);

	if (startIndex == -1) 
		return {};

	const int endIndex = allText.indexOf(endLine);

	if (endIndex == -1)
		return {};

	return Range<int>(startIndex, endIndex + endLine.length());
}

CodeDocument::Position JavascriptCodeEditor::Helpers::getPositionAfterDefinition(const CodeDocument& doc, Identifier id)
{

	const String regexp = "(const)?\\s*(global|var|reg)?\\s*" + id.toString() + "\\s*=\\s*.*;[\\n\\r]";

	const String allText = doc.getAllContent();

	StringArray sa = RegexFunctions::getFirstMatch(regexp, allText);

	if (sa.size() > 0)
	{
		const String match = sa[0];

		int startIndex = allText.indexOf(match) + match.length();

		return CodeDocument::Position(doc, startIndex);
	}
	else
		return CodeDocument::Position(doc, 0);
}

CodeDocument* JavascriptCodeEditor::Helpers::gotoAndReturnDocumentWithDefinition(Processor* p, DebugableObject* object)
{
	if (object == nullptr)
		return nullptr;

	auto jsp = dynamic_cast<JavascriptProcessor*>(p);

	auto info = DebugableObject::Helpers::getDebugInformation(jsp->getScriptEngine(), object);

	if (info != nullptr)
	{
		DebugableObject::Helpers::gotoLocation(p, info);

		auto activeEditor = getActiveEditor(jsp);

		if (activeEditor != nullptr)
		{
			return &activeEditor->getDocument();
		}
	}

	return nullptr;
}

String JavascriptCodeEditor::Helpers::findNamespaceForPosition(CodeDocument::Position pos)
{
	auto startPos = pos;
	String namespaceId;


	while (pos.getLineNumber() > 0)
	{
		auto lineText = pos.getLineText();

		if (lineText.startsWith("namespace"))
		{
			static const String r("namespace\\s+(\\S*)");

			auto matches = RegexFunctions::getFirstMatch(r, lineText);

			if (matches.size() > 1)
			{
				namespaceId = matches[1];
				break;
			}
		}

		pos = pos.movedByLines(-1);
	}

	if (namespaceId.isNotEmpty())
	{
		int bracketCount = 0;
		
		while (pos != startPos)
		{
			if (pos.getCharacter() == '{')
				bracketCount++;

			if (pos.getCharacter() == '}')
			{
				bracketCount--;

				if (bracketCount == 0) // end of namespace is reached
					break;
			}
				
			pos = pos.movedBy(1);
		}

		if (bracketCount > 0)
			return namespaceId;
		else
			return String();
	}

	return String();
}

void JavascriptCodeEditor::Helpers::applyChangesFromActiveEditor(JavascriptProcessor* p)
{
	auto activeEditor = getActiveEditor(p);

	if (activeEditor == nullptr)
		return;

	if (auto pe = activeEditor->findParentComponentOfClass<PopupIncludeEditor>())
	{
		auto f = pe->getFile();

		if(f.existsAsFile())
			f.replaceWithText(activeEditor->getDocument().getAllContent());
	}
}

JavascriptCodeEditor* JavascriptCodeEditor::Helpers::getActiveEditor(JavascriptProcessor* p)
{
	auto processor = dynamic_cast<Processor*>(p);
	auto activeEditor = processor->getMainController()->getLastActiveEditor();

	return dynamic_cast<JavascriptCodeEditor*>(activeEditor);
}

JavascriptCodeEditor* JavascriptCodeEditor::Helpers::getActiveEditor(Processor* p)
{
	auto activeEditor = p->getMainController()->getLastActiveEditor();
	return dynamic_cast<JavascriptCodeEditor*>(activeEditor);
}

DebugConsoleTextEditor::DebugConsoleTextEditor(const String& name, Processor* p) :
	TextEditor(name),
	processor(p)
{
	setMultiLine(false);
	setReturnKeyStartsNewLine(false);
	setScrollbarsShown(false);
	setPopupMenuEnabled(false);
	setColour(TextEditor::textColourId, Colours::white);
	setColour(CaretComponent::ColourIds::caretColourId, Colours::white);
	setColour(TextEditor::backgroundColourId, Colour(0x00ffffff));
	setColour(TextEditor::highlightColourId, Colour(0x40ffffff));
	setColour(TextEditor::shadowColourId, Colour(0x00000000));
	setColour(TextEditor::ColourIds::focusedOutlineColourId, Colours::white.withAlpha(0.1f));
	setText(String());

	setFont(GLOBAL_MONOSPACE_FONT());
	setLookAndFeel(&laf2);
	setColour(Label::ColourIds::textColourId, Colours::white);

	addListener(this);

	p->getMainController()->addScriptListener(this);

	scriptWasCompiled(dynamic_cast<JavascriptProcessor*>(p));
}

DebugConsoleTextEditor::~DebugConsoleTextEditor()
{
	setLookAndFeel(nullptr);

	if (processor != nullptr)
	{
		processor->getMainController()->removeScriptListener(this);
	}
}

void DebugConsoleTextEditor::scriptWasCompiled(JavascriptProcessor *jp)
{
	if (dynamic_cast<Processor*>(jp) == processor)
	{
		auto r = jp->getLastErrorMessage();



		if (r.wasOk()) setText("Compiled OK", dontSendNotification);
		else
			setText(r.getErrorMessage().upToFirstOccurrenceOf("\n", false, false), dontSendNotification);

		setColour(TextEditor::backgroundColourId, r.wasOk() ? Colours::green.withBrightness(0.1f) : Colours::red.withBrightness((0.1f)));
	}
}

bool DebugConsoleTextEditor::keyPressed(const KeyPress& k)
{
	if (k == KeyPress::upKey)
	{
		currentHistoryIndex = jmax<int>(0, currentHistoryIndex - 1);

		setText(history[currentHistoryIndex], dontSendNotification);

		return true;
	}
	else if (k == KeyPress::downKey)
	{
		currentHistoryIndex = jmin<int>(history.size() - 1, currentHistoryIndex + 1);
		setText(history[currentHistoryIndex], dontSendNotification);
	}

	return TextEditor::keyPressed(k);
}

void DebugConsoleTextEditor::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
		setText("> ", dontSendNotification);

	TextEditor::mouseDown(e);
}

void DebugConsoleTextEditor::mouseDoubleClick(const MouseEvent& /*e*/)
{
	DebugableObject::Helpers::gotoLocation(processor->getMainController()->getMainSynthChain(), getText());
}

void DebugConsoleTextEditor::addToHistory(const String& s)
{
	if (!history.contains(s))
	{
		history.add(s);
		currentHistoryIndex = history.size() - 1;
	}
	else
	{
		history.move(history.indexOf(s), history.size() - 1);
	}
}

void DebugConsoleTextEditor::textEditorReturnKeyPressed(TextEditor& /*t*/)
{
	String codeToEvaluate = getText();

	addToHistory(codeToEvaluate);

	if (codeToEvaluate.startsWith("> "))
	{
		codeToEvaluate = codeToEvaluate.substring(2);
	}

	HiseJavascriptEngine* engine = dynamic_cast<JavascriptProcessor*>(processor.get())->getScriptEngine();

	if (engine != nullptr)
	{
		Result r = Result::ok();

		var returnValue = engine->evaluate(codeToEvaluate, &r);

		if (r.wasOk())
		{
			debugToConsole(processor, "> " + returnValue.toString());
		}
		else
		{
			debugToConsole(processor, r.getErrorMessage());
		}
	}
}


struct InteractiveEditor : public MarkdownCodeComponentBase,
	public ControlledObject
{
	void initialiseEditor() override
	{
		if (getMainController() != nullptr && Helpers::createProcessor(syntax))
		{
			jp = new JavascriptMidiProcessor(getMainController(), "TestProcessor");
			jp->setOwnerSynth(getMainController()->getMainSynthChain());

			if (Helpers::createContent(syntax))
			{
				scriptContent = new ScriptContentComponent(jp);
				addAndMakeVisible(scriptContent);
			}
				
			usedDocument = jp->getSnippet(0);
			usedDocument->replaceAllContent(ownedDoc->getAllContent());

			editor = new JavascriptCodeEditor(*usedDocument, tok, jp, "onInit");

			editor->setFont(GLOBAL_MONOSPACE_FONT().withHeight(fontSize));

			addBottomRow();
		}
		else
		{
			MarkdownCodeComponentBase::initialiseEditor();

			if (syntax == EditableFloatingTile)
			{
				editor->setReadOnly(false);

				auto data = JSON::parse(ownedDoc->getAllContent());

				parent->addAndMakeVisible(this);

				addAndMakeVisible(floatingTile = new FloatingTile(getMainController(), nullptr, {}));
				floatingTile->setOpaque(true);

				floatingTile->setContent(data);

				addBottomRow();

				if (auto pc = dynamic_cast<PanelWithProcessorConnection*>(floatingTile->getCurrentFloatingPanel()))
				{
					floatingTileProcessor = pc->createDummyProcessorForDocumentation(getMainController());
					pc->setContentWithUndo(floatingTileProcessor, 0);
					floatingTile->getCurrentFloatingPanel()->setPanelColour(FloatingTileContent::PanelColourId::bgColour, Colour(0xFF363636));
				}
			}
		}
	}

	InteractiveEditor(SyntaxType syntax, String code, float width, float fontsize, MainController* mc, Component* parent_, MarkdownParser* parser) :
		MarkdownCodeComponentBase(syntax, code, width, fontsize, parser),
		ControlledObject(mc),
		parent(parent_),
		copyButton("copy", this, f)
	{
		initialiseEditor();
		createChildComponents();

		addAndMakeVisible(copyButton);

		if (syntax == ScriptContent)
		{
			showContentOnly = true;
			editor->setVisible(false);
			runButton->triggerClick();
		}

		setWantsKeyboardFocus(true);
	}

	~InteractiveEditor()
	{
		floatingTile = nullptr;
		floatingTileProcessor = nullptr;
		scriptContent = nullptr;
		editor = nullptr;
		jp = nullptr;
		tok = nullptr;
		ownedDoc = nullptr;
	}

	void addBottomRow()
	{
		addAndMakeVisible(runButton = new TextButton("Run"));
		addAndMakeVisible(resultLabel = new Label());
		runButton->setLookAndFeel(&blaf);
		runButton->addListener(this);
		resultLabel->setColour(Label::ColourIds::backgroundColourId, Colour(0xff363636));
		resultLabel->setColour(Label::ColourIds::textColourId, Colours::white);
		resultLabel->setFont(GLOBAL_MONOSPACE_FONT());
		resultLabel->setText("Click Run to evaluate this code", dontSendNotification);
	}

	bool keyPressed(const KeyPress& key) override
	{
		if (key == KeyPress::F5Key && runButton != nullptr)
		{
			runButton->triggerClick();
			return true;
		}

		return false;
	}

	void buttonClicked(Button* b) override // TODO: => base class
	{
		MarkdownCodeComponentBase::buttonClicked(b);

		if (b == &copyButton)
		{
			SystemClipboard::copyTextToClipboard(editor->getDocument().getAllContent());
			return;
		}


		if (jp != nullptr)
		{
			auto f2 = [this](const JavascriptProcessor::SnippetResult& result)
			{
				Component::SafePointer<InteractiveEditor> tmp = this;

				auto fc = [tmp, result]()
				{
					if (tmp.getComponent() == nullptr)
						return;

					tmp.getComponent()->isActive = true;
					String s;

					if (result.r.ok())
					{
						auto consoleContent = tmp.getComponent()->jp->getMainController()->getConsoleHandler().getConsoleData()->getAllContent();
						s = consoleContent.removeCharacters("\n").fromFirstOccurrenceOf(":", false, false);
					}
					else
						s = "Error: " + result.r.getErrorMessage();



					tmp.getComponent()->resultLabel->setText(s, dontSendNotification);
					tmp.getComponent()->repaint();

					tmp.getComponent()->updateHeightInParent();
				};

				new DelayedFunctionCaller(fc, 100);

			};

			jp->getMainController()->getConsoleHandler().clearConsole();
			jp->compileScript(f2);
		}
		else if (floatingTile != nullptr)
		{
			auto value = JSON::parse(editor->getDocument().getAllContent());

			floatingTile->setContent(value);

			if (auto pc = dynamic_cast<PanelWithProcessorConnection*>(floatingTile->getCurrentFloatingPanel()))
			{
				pc->setContentWithUndo(floatingTileProcessor, 0);
			}
		}
	}

	bool autoHideEditor() const override
	{
		return !isExpanded && (floatingTile != nullptr || usedDocument->getNumLines() > 20);
	}

	int getPreferredHeight() const
	{
		if (showContentOnly)
		{
			return scriptContent->getContentHeight() + 20;
		}
		else
		{
			int y = scriptContent != nullptr ? (scriptContent->getContentHeight() + 20) : 0;

			if (floatingTile != nullptr)
			{
				int size = floatingTile->getCurrentFloatingPanel()->getPreferredHeight();

				if (size == 0)
					size = 400;

				y += size;
				y += 2 * getGutterWidth();
			}

			if (runButton != nullptr)
				y += (autoHideEditor() ? 2 : 1) * editor->getLineHeight();

			y += MarkdownCodeComponentBase::getPreferredHeight();

			return y;
		}
	}



	void resized() override
	{
		if (showContentOnly)
		{
			scriptContent->setBounds(10, 10, getWidth(), scriptContent->getContentHeight());
			editor->setVisible(false);
			expandButton->setVisible(false);
			runButton->setVisible(false);
			resultLabel->setVisible(false);
		}
		else
		{
			int y = 0;

			if (scriptContent != nullptr)
			{

				scriptContent->setBounds(10, 10, getWidth(), scriptContent->getContentHeight());
				y += scriptContent->getContentHeight() + 20;
			}

			if (floatingTile != nullptr)
			{
				int size = floatingTile->getCurrentFloatingPanel()->getPreferredHeight();;

				if (size == 0)
					size = 400;

				y += size;

				auto gWidth = getGutterWidth();

				floatingTile->setBounds(gWidth, gWidth, getWidth() - 2 * gWidth, size);

				y += 2 * gWidth;
			}

			editor->scrollToLine(0);

			if (autoHideEditor())
			{
				o.setVisible(true);
				expandButton->setVisible(true);

				editor->setSize(getWidth(), 2 * editor->getLineHeight());
				editor->setTopLeftPosition(0, y + editor->getLineHeight() / 2);

				auto b = editor->getBounds();

				b.removeFromLeft(getGutterWidth());

				expandButton->setBounds(b.withSizeKeepingCentre(130, editor->getLineHeight()));
				editor->setEnabled(false);

				auto ob = editor->getBounds();
				ob.removeFromLeft(getGutterWidth());

				o.setBounds(ob);
			}
			else
			{
				y += editor->getLineHeight();

				editor->setSize(getWidth(), getEditorHeight());
				editor->setTopLeftPosition(0, y);

				o.setVisible(false);

				expandButton->setVisible(false);
				editor->setEnabled(true);
			}

			if (runButton != nullptr)
			{
				auto ar = getLocalBounds();
				ar = ar.removeFromBottom(editor->getLineHeight());
				runButton->setBounds(ar.removeFromLeft(getGutterWidth()));
				resultLabel->setBounds(ar);
			}

			copyButton.setBounds(Rectangle<int>(0, y, 24, 24).reduced(4));
		}
	}

	void paint(Graphics& g) override
	{
		g.fillAll(Colour(0xff363636));

		auto b = getLocalBounds();
		b.removeFromLeft(getGutterWidth());

		g.setColour(editor->findColour(CodeEditorComponent::ColourIds::backgroundColourId));
		g.fillRect(b);

		if (floatingTile != nullptr)
		{
			g.setColour(Colour(0xff363636));
			g.fillRect(0, 0, getWidth(), floatingTile->getHeight() + 2 * getGutterWidth());
		}

		if (scriptContent != nullptr)
		{
			auto h = getLocalBounds().removeFromTop(scriptContent->getContentHeight() + 20);

			g.setColour(Colour(0xff363636));
			g.fillRect(h);

			if (!isActive)
			{
				g.setFont(GLOBAL_BOLD_FONT());
				g.setColour(Colours::white.withAlpha(0.5f));
				g.drawText("Press run to show the result", h.toFloat(), Justification::centred);
			}

		}

	}

	ScopedPointer<ScriptContentComponent> scriptContent;
	ScopedPointer<JavascriptMidiProcessor> jp;

	Component* parent = nullptr;

	Rectangle<float> pathBounds;

	ScopedPointer<FloatingTile> floatingTile;
	ScopedPointer<Processor> floatingTileProcessor;

	ScopedPointer<TextButton> runButton;
	ScopedPointer<Label> resultLabel;

	bool isActive = false;
	bool showContentOnly = false;

	HiseShapeButton copyButton;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InteractiveEditor);
};

#if HI_MARKDOWN_ENABLE_INTERACTIVE_CODE
hise::MarkdownCodeComponentBase* MarkdownCodeComponentFactory::createInteractiveEditor(MarkdownParser* parent, MarkdownCodeComponentBase::SyntaxType syntax, const String& code, float width)
{
	auto mainController = dynamic_cast<MainController*>(parent->getHolder());
	auto parentComponent = dynamic_cast<MarkdownRenderer*>(parent)->getTargetComponent();

	if (mainController == nullptr)
	{
		if (auto cObj = dynamic_cast<ControlledObject*>(parent->getHolder()))
			mainController = cObj->getMainController();
	}
		
	jassert(mainController != nullptr);
	
	return new InteractiveEditor(syntax, code, width, parent->getStyleData().fontSize, mainController, parentComponent, parent);
}
#endif


} // namespace hise
