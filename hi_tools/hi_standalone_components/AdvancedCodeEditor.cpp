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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/

namespace hise {
using namespace juce;


JavascriptCodeEditor::JavascriptCodeEditor(CodeDocument &document, CodeTokeniser *codeTokeniser, ApiProviderBase::Holder *holder, const Identifier& snippetId_) :
	CodeEditorComponent(document, codeTokeniser),
	ApiComponentBase(holder),
	snippetId(snippetId_),
	hoverManager(*this)
{
	holder->addEditor(this);

	getGutterComponent()->addMouseListener(this, true);

	setColour(CodeEditorComponent::backgroundColourId, Colour(0xff262626));
	setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFFCCCCCC));
	setColour(CodeEditorComponent::ColourIds::lineNumberTextId, Colour(0xFFCCCCCC));
	setColour(CodeEditorComponent::ColourIds::lineNumberBackgroundId, Colour(0xff363636));
	setColour(CodeEditorComponent::ColourIds::highlightColourId, Colour(0xff666666));
	setColour(CaretComponent::ColourIds::caretColourId, Colour(0xFFDDDDDD));
	setColour(ScrollBar::ColourIds::thumbColourId, Colour(0x3dffffff));

	setFont(GLOBAL_MONOSPACE_FONT().withHeight(holder->getCodeFontSize()));

#if 0
	.withHeight(processor->getMainController()->getGlobalCodeFontSize()));

	processor->getMainController()->getFontSizeChangeBroadcaster().addChangeListener(this);
#endif
}

JavascriptCodeEditor::~JavascriptCodeEditor()
{
	currentPopup = nullptr;

	if (holder != nullptr)
	{
		holder->removeEditor(this);
	}

	currentModalWindow.deleteAndZero();

	stopTimer();
}

void JavascriptCodeEditor::changeListenerCallback(SafeChangeBroadcaster *)
{
	float newFontSize = holder->getCodeFontSize();

	Font newFont = GLOBAL_MONOSPACE_FONT().withHeight(newFontSize);

	setFont(newFont);
}

void JavascriptCodeEditor::timerCallback()
{

	stopTimer();
}

void JavascriptCodeEditor::focusGained(FocusChangeType)
{
	holder->setActiveEditor(this, getCaretPos());
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

	if (position.getPosition() > 0)
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

		String line = "\nconst var " + newId + " = " + factoryMethodName + "(\"" + newId + "\", " + String(newX) + ", " + String(newY) + additionalParameters + ");\n";
		return line;
	}

	return String();
}

void JavascriptCodeEditor::focusLost(FocusChangeType)
{
	holder->setActiveEditor(this, getCaretPos());
}

void JavascriptCodeEditor::addPopupMenuItems(PopupMenu &menu, const MouseEvent* e)
{
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

	CodeEditorComponent::addPopupMenuItems(menu, e);

	holder->addPopupMenuItems(menu, this, *e);

};

void JavascriptCodeEditor::performPopupMenuAction(int menuId)
{
	if (holder.get()->performPopupMenuAction(menuId, this))
		return;
		

	CodeEditorComponent::performPopupMenuAction(menuId);
}

String JavascriptCodeEditor::matchesAutocompleteTemplate(const String& token) const
{
	if (!token.containsChar('.'))
		return {};

	auto tt = token.upToLastOccurrenceOf(".", false, false);

	for (const auto& t : autocompleteTemplates)
	{
		if (t.token == tt)
			return t.classId;
	}

	return {};
}

juce::String JavascriptCodeEditor::getCurrentToken() const
{
	return getTokenForPosition(getCaretPos());
}

String JavascriptCodeEditor::getTokenForPosition(const CodeDocument::Position& pos) const
{
	CodeDocument::Position start = pos;
	CodeDocument::Position end = start;

	Helpers::findAdvancedTokenRange(pos, start, end);

	return getDocument().getTextBetween(start, end);
}

void JavascriptCodeEditor::showAutoCompleteNew()
{
	Range<int> tokenRange = getCurrentTokenRange();
	auto text = getTextInRange(tokenRange);

	currentPopup = new AutoCompletePopup((int)getFont().getHeight(), this, holder, text);

	if (currentPopup->getNumRows() == 0)
	{
		currentPopup = nullptr;
	}
	else
	{
		if (auto editor = TopLevelWindowWithOptionalOpenGL::findRoot(this))
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

	if (holder->getActiveEditor() == this)
	{
		g.setColour(Colour(SIGNAL_COLOUR));
		g.fillRect(0, 0, 4, 4);
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
				g.fillRoundedRectangle((float)x - 1.0f, (float)y, (float)w + 2.0f, (float)h, 2.0f);
				g.setColour(Colours::white.withAlpha(0.5f));
				g.drawRoundedRectangle((float)x - 1.0f, (float)y, (float)w + 2.0f, (float)h, 2.0f, 1.0f);
			}
		}
	}

	holder->handleBreakpoints(snippetId, g, this);

	if (hoverText.isNotEmpty())
	{
		Font f = GLOBAL_BOLD_FONT();
		auto w = f.getStringWidthFloat(hoverText) + 20.0f;

		auto x = (float)hoverPosition.x - w / 2.0f;



		auto y = (float)hoverPosition.y;

		y -= std::fmod(y, (float)getLineHeight());
		y += (float)getLineHeight() + 5.0f;

		auto h = f.getHeight() + 15.0f;

		Rectangle<float> area(x, y, w, h);

		g.setColour(Colour(0xEEAAAAAA));
		g.fillRoundedRectangle(area, 2.0f);
		g.setColour(Colours::black);
		g.setFont(f);
		g.drawText(hoverText, area, Justification::centred);
		g.drawRoundedRectangle(area, 2.0f, 1.0f);
	}
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

	for (int i = 0; i < highlightedSelection.size(); i++)
	{
		if (highlightedSelection[i].isEmpty())
			highlightedSelection.remove(i--);
	}


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
	handleDoubleCharacter(k, '(', ')');
	handleDoubleCharacter(k, '[', ']');
	handleDoubleCharacter(k, '\"', '\"');

	if (currentPopup != nullptr)
	{
		if (currentPopup->handleEditorKeyPress(k))
			return true;
	}

	if (holder != nullptr)
	{
		if (holder->handleKeyPress(k, this))
			return true;
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

		for (int i = startLine; i <= endLine; i++)
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
	
	else if ((k.isKeyCode('d') || k.isKeyCode('D')) && k.getModifiers().isCommandDown())
	{
		increaseMultiSelectionForCurrentToken();
		return true;
	}
	else if (k.isKeyCode(KeyPress::F8Key))
	{
		if (k.getModifiers().isCommandDown())
		{
			autocompleteTemplates.clear();
			return true;
		}
		else if (!getHighlightedRegion().isEmpty())
		{
			return true;
		}
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
	else if (k.isKeyCode(KeyPress::F12Key))
	{
		auto token = getCurrentToken();
		const String namespaceId = Helpers::findNamespaceForPosition(getCaretPos());
		holder->jumpToDefinition(token, namespaceId);
	}

	if (k != KeyPress::escapeKey) startTimer(800);

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

	if (e.x < 35)
	{
		holder->handleBreakpointClick(snippetId, *this, e);
	}
}

int JavascriptCodeEditor::Helpers::getOffsetToFirstToken(const String& content)
{
	auto p = content.getCharPointer();
	auto start = p;

	for (;;)
	{
		p = p.findEndOfWhitespace();

		if (*p == '/')
		{
			const juce_wchar c2 = p[1];

			if (c2 == '/') { p = CharacterFunctions::find(p, (juce_wchar) '\n'); continue; }

			if (c2 == '*')
			{
				p = CharacterFunctions::find(p + 2, CharPointer_ASCII("*/"));

				if (p.isEmpty()) return 0;
				p += 2; continue;
			}
		}

		break;
	}

	return (int)(p - start);
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
		else if (c == '/') { if (*line == '/') break; }
		else if (c == '"' || c == '\'') { while (!(line.isEmpty() || line.getAndAdvance() == c)) {} }
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

void JavascriptCodeEditor::Helpers::findAdvancedTokenRange(const CodeDocument::Position& pos,
	CodeDocument::Position& start, CodeDocument::Position& end)
{
	end = pos;
	while (isAdvancedTokenCharacter(end.getCharacter()))
		end.moveBy(1);

	start = end;
	while (start.getIndexInLine() > 0
		&& isAdvancedTokenCharacter(start.movedBy(-1).getCharacter()))
		start.moveBy(-1);
}

bool JavascriptCodeEditor::Helpers::isAdvancedTokenCharacter(juce_wchar c)
{
	return CharacterFunctions::isLetterOrDigit(c) || c == '.' || c == '_' || c == '[' || c == ']' || c == '"';
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

void JavascriptCodeEditor::mouseMove(const MouseEvent& e)
{
	auto pos = e.getPosition();
	auto pos2 = getPositionAt(pos.x, pos.y); 
	auto token = getTokenForPosition(pos2);

	if (token != hoverManager.lastToken)
	{
		hoverManager.stopTimer();
		hoverPosition = {};
		hoverText = {};
		repaint();

		hoverManager.position = pos;
		hoverManager.lastToken = token;
		hoverManager.startTimer(700);
	}
}

void JavascriptCodeEditor::HoverManager::timerCallback()
{
	if (auto pr = parent.getProviderBase())
	{
		parent.hoverText = pr->getHoverString(lastToken);

		if (parent.hoverText.isNotEmpty())
		{
			parent.hoverPosition = position;
			parent.repaint();
			startTimer(300);
		}
		else
		{
			parent.hoverPosition = {};
			stopTimer();
		}
					
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

JavascriptCodeEditor::AutoCompletePopup::AutoCompletePopup(int fontHeight_, JavascriptCodeEditor* editor_, ApiProviderBase::Holder* h, const String &tokenText) :
	ApiComponentBase(h),
	editor(editor_),
	fontHeight(fontHeight_)
{
	addAndMakeVisible(listbox = new ListBox());
	addAndMakeVisible(infoBox = new InfoBox());

	listbox->setModel(this);

	listbox->setRowHeight(fontHeight + 4);
	listbox->setColour(ListBox::ColourIds::backgroundColourId, Colours::transparentBlack);


	listbox->getViewport()->setScrollBarThickness(8);
	listbox->getVerticalScrollBar().setColour(ScrollBar::ColourIds::thumbColourId, Colours::black.withAlpha(0.6f));
	listbox->getVerticalScrollBar().setColour(ScrollBar::ColourIds::trackColourId, Colours::black.withAlpha(0.4f));

	addAndMakeVisible(helpButton = new TextButton("?"));
	helpButton->setVisible(false);
	//helpButton->setPopupWidth(600);
	//helpButton->setFontSize(15.0f);
	//helpButton->setIgnoreKeyStrokes(true);

	listbox->setWantsKeyboardFocus(false);
	setWantsKeyboardFocus(false);
	infoBox->setWantsKeyboardFocus(false);

	rebuild(tokenText);
}

JavascriptCodeEditor::AutoCompletePopup::~AutoCompletePopup()
{
	infoBox = nullptr;
	listbox = nullptr;

	allInfo.clear();
}

void JavascriptCodeEditor::AutoCompletePopup::rebuild(const String& tokenText)
{
	allInfo.clear();

	const ValueTree apiTree = holder->createApiTree();

	auto templatedToken = editor->matchesAutocompleteTemplate(tokenText);

	for (const auto& t : editor->autocompleteTemplates)
	{
		auto ri = new RowInfo();
		ri->codeToInsert = t.token;
		ri->classId = t.classId;
		ri->name = t.token;
		ri->category = "Template";
		ri->value = t.classId;
		
		allInfo.add(ri);
	}

	if (tokenText.containsChar('.') || templatedToken.isNotEmpty())
	{
		createObjectPropertyRows(apiTree, tokenText);
	}
	else
	{
		createVariableRows();
		createApiRows(apiTree, tokenText);
	}

	rebuildVisibleItems(tokenText);
}

void JavascriptCodeEditor::AutoCompletePopup::createRecursive(DebugInformationBase::Ptr info)
{
	

	if (auto obj = info->getObject())
	{
		if (obj->isInternalObject())
			return;
	}

	allInfo.add(new RowInfo(info));

	if (!info->isAutocompleteable())
		return;

	int numChildren = info->getNumChildElements();

	for (int i = 0; i < numChildren; i++)
	{
		auto childInfo = info->getChildElement(i);
		createRecursive(childInfo);
	}
}

void JavascriptCodeEditor::AutoCompletePopup::createVariableRows()
{
	if (auto p = getProviderBase())
	{
		for (int i = 0; i < p->getNumDebugObjects(); i++)
		{
			auto info = p->getDebugInformation(i);
			createRecursive(info);
		}
	}
}

void JavascriptCodeEditor::AutoCompletePopup::createApiRows(const ValueTree &apiTree, const String& tokenText)
{
	if (auto p = getProviderBase())
	{
		for (int i = 0; i < apiTree.getNumChildren(); i++)
		{
			ValueTree classTree = apiTree.getChild(i);
			const String className = classTree.getType().toString();

			if (auto apiObject = p->getDebugObject(className))
			{
				addRowsFromObject(apiObject, className, classTree);
			}
            
            addRowFromApiClass(classTree, tokenText);
		}
	}
}

void JavascriptCodeEditor::AutoCompletePopup::createObjectPropertyRows(const ValueTree &apiTree, const String &tokenText)
{
	if (auto p = getProviderBase())
	{
		auto objectId = tokenText.upToLastOccurrenceOf(String("."), false, true);

		auto templatedToken = editor->matchesAutocompleteTemplate(tokenText);

		if (auto obj = p->getDebugObject(objectId))
			addRowsFromObject(obj, objectId, apiTree);
		else if (templatedToken.isNotEmpty())
		{
			addRowFromApiClass(apiTree.getChildWithName(Identifier(templatedToken)), tokenText, true);
		}
		else
		{
			auto c = apiTree.getChildWithName(Identifier(objectId));

			if (c.isValid())
				addRowFromApiClass(c, tokenText.fromFirstOccurrenceOf(objectId + ".", false, false));
		}
	}
}


void JavascriptCodeEditor::AutoCompletePopup::addRowFromApiClass(const ValueTree classTree, const String& originalToken, bool isTemplate)
{
	for (auto methodTree : classTree)
	{
        auto classId = classTree.getType();

        const String name = methodTree.getProperty(Identifier("name")).toString();
        
        if(name.contains(originalToken) || isTemplate)
        {
            RowInfo *info = new RowInfo();
            info->classId = classId;
            info->description = ValueTreeApiHelpers::createAttributedStringFromApi(methodTree, classId.toString(), false, Colours::black);
            info->codeToInsert = ValueTreeApiHelpers::createCodeToInsert(methodTree, classId.toString());

			if (isTemplate)
			{
				String tt;

				for (const auto& t : editor->autocompleteTemplates)
				{
					if (originalToken.startsWith(t.token))
					{
						tt = t.token;
						break;
					}
				}

				String nc;
				nc << tt;
				nc << info->codeToInsert.fromFirstOccurrenceOf(classId.toString(), false, false);
				info->codeToInsert = nc;
			}

            info->name = info->codeToInsert;
            info->type = (int)RowInfo::Type::ApiMethod;
            allInfo.add(info);
        }
	}
}


std::unique_ptr<juce::ComponentTraverser> JavascriptCodeEditor::AutoCompletePopup::createKeyboardFocusTraverser()
{
	return std::make_unique<AllToTheEditorTraverser>(editor);
}

void JavascriptCodeEditor::AutoCompletePopup::addRowsFromObject(DebugableObjectBase* obj, const String& originalToken, const ValueTree& classTree)
{
	auto classId = obj->getObjectName();

	ValueTree documentedMethods = classTree.getChildWithName(classId);

	Array<Identifier> functionNames;

	obj->getAllFunctionNames(functionNames);


	for (auto c : documentedMethods)
	{
		static const Identifier name("name");
		auto thisFuncId = Identifier(c.getProperty(name));

		if (functionNames.contains(thisFuncId))
		{
			RowInfo *info = new RowInfo();
			info->classId = classId;

			info->description = ValueTreeApiHelpers::createAttributedStringFromApi(c, classId.toString(), false, Colours::black);
			info->codeToInsert = ValueTreeApiHelpers::createCodeToInsert(c, originalToken);
			info->name = info->codeToInsert;
			info->type = (int)c.getProperty("typenumber", (int)RowInfo::Type::ApiMethod);
			info->category = obj->getCategory();

			functionNames.removeAllInstancesOf(thisFuncId);
			allInfo.add(info);
		}
	}

	for (auto af : functionNames)
	{
		RowInfo *info = new RowInfo();
		info->classId = classId;
		info->codeToInsert << originalToken << "." << af << "()";
		info->name = info->codeToInsert;
		info->type = (int)RowInfo::Type::ApiMethod;
		info->category = obj->getCategory();
		allInfo.add(info);
	}

	Array<Identifier> constantNames;

	obj->getAllConstants(constantNames);
	int i = 0;

	for (auto constant : constantNames)
	{
		ScopedPointer<DebugInformationBase> childInfo = obj->createDebugInformationForChild(constant);

		RowInfo *info = new RowInfo();

		if (childInfo != nullptr)
		{
			info->classId = classId;
			info->name = childInfo->getTextForName();
			info->codeToInsert = childInfo->getCodeToInsert();
			info->description = childInfo->getDescription();
			info->type = childInfo->getType();
			info->typeName = childInfo->getTextForType();
			info->value = childInfo->getTextForValue();
			info->category = childInfo->getCategory();
		}
		else
		{
			auto value = obj->getConstantValue(i++);
			
			info->classId = classId;
			info->name = classId.toString() + "." + constant;
			info->codeToInsert = info->name;
			info->category = obj->getCategory();
			info->typeName = DebugInformationBase::getVarType(value);
			info->value = value;
		}

		allInfo.add(info);
	}
}
#if 0
void JavascriptCodeEditor::AutoCompletePopup::addApiConstants(const ApiClassBase* apiClass, const Identifier &objectId)
{
	Array<Identifier> constants;

	apiClass->getAllConstants(constants);

	for (int i = 0; i < constants.size(); i++)
	{
		const var prop = apiClass->getConstantValue(i);

		RowInfo *info = new RowInfo();

		info->classId = objectId;
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
		row->classId = objectId;
		row->description = ApiHelpers::createAttributedStringFromApi(methodTree, objectId.toString(), false, Colours::black);
		row->codeToInsert = ApiHelpers::createCodeToInsert(methodTree, objectId.toString());
		row->name = row->codeToInsert;
		row->type = (int)RowInfo::Type::ApiMethod;

		allInfo.add(row);
	}
}
#endif

int JavascriptCodeEditor::AutoCompletePopup::getNumRows()
{
	return visibleInfo.size();
}

void JavascriptCodeEditor::AutoCompletePopup::paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected)
{
	RowInfo *info = visibleInfo[rowNumber];

	if (info == nullptr) return;

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

	if (auto pr = getProviderBase())
		pr->getColourAndLetterForType(info->type, colour, ch);
	
	g.setColour(colour);

	auto icon = Rectangle<float>(1.0f, 1.0f, height - 2.0f, height - 2.0f);

	g.fillRoundedRectangle(icon, 4.0f);



	g.setColour(rowIsSelected ? Colours::white : Colours::black.withAlpha(0.7f));

	auto f = GLOBAL_MONOSPACE_FONT().withHeight((float)fontHeight);

	g.setFont(f);

	const String name = info->name;

	auto left = 10 + (int)f.getStringWidthFloat(name);

	g.drawText(name, height + 2, 1, width - height - 4, height - 2, Justification::centredLeft);

	if (info->category.isNotEmpty() && getWidth() - left > 50)
	{
		g.setFont(GLOBAL_FONT());

		Colour c = rowIsSelected ? Colours::white : Colours::black;

		g.setColour(c.withAlpha(0.5f));
		g.drawText(info->category, height + left, 1, width - height - 4, height - 2, Justification::centredLeft);
	}
}

void JavascriptCodeEditor::AutoCompletePopup::listBoxItemClicked(int row, const MouseEvent &)
{
	selectRowInfo(row);
}

void JavascriptCodeEditor::AutoCompletePopup::listBoxItemDoubleClicked(int row, const MouseEvent &)
{
	editor->closeAutoCompleteNew(visibleInfo[row]->codeToInsert);
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
	else if (k == KeyPress::F1Key)
	{
		helpButton->triggerClick();
	}
	else if (k == KeyPress::returnKey)
	{
		const bool insertSomething = currentlySelectedBox >= 0;

		if (insertSomething && currentlySelectedBox < visibleInfo.size())
			editor->closeAutoCompleteNew(insertSomething ? visibleInfo[currentlySelectedBox]->codeToInsert : String());
		else
			editor->closeAutoCompleteNew({});

		return insertSomething;
	}
	else if (k == KeyPress::spaceKey || k == KeyPress::tabKey || k.getTextCharacter() == ';' || k.getTextCharacter() == '(')
	{
		editor->closeAutoCompleteNew({});
		return false;
	}
	else
	{
		String selection = editor->getTextInRange(editor->getCurrentTokenRange());

		if (k == KeyPress::backspaceKey)
			selection = selection.substring(0, selection.length() - 1);
		else
			selection << k.getTextCharacter();

		if (selection.contains(".") != lastText.contains("."))
			rebuild(selection);
		else
			rebuildVisibleItems(selection);

		return false;
	}

	return false;
}

void JavascriptCodeEditor::AutoCompletePopup::paint(Graphics& g)
{
	g.setColour(Colour(0xFFBBBBBB));
	g.fillRoundedRectangle(0.0f, 0.0f, (float)getWidth(), (float)getHeight(), 3.0f);
}

void JavascriptCodeEditor::AutoCompletePopup::resized()
{
	auto area = getLocalBounds().reduced(3);

	auto topArea = area.removeFromTop(3 * fontHeight);

	infoBox->setBounds(topArea);
	listbox->setBounds(area);



	infoBox->setBounds(3, 3, getWidth() - 6, 3 * fontHeight - 6);
	listbox->setBounds(3, 3 * fontHeight + 3, getWidth() - 6, getHeight() - 3 * fontHeight - 6);
}

void JavascriptCodeEditor::AutoCompletePopup::selectRowInfo(int rowIndex)
{
	listbox->repaintRow(currentlySelectedBox);

	currentlySelectedBox = rowIndex;

	auto thisRow = visibleInfo[rowIndex];

	if (thisRow == nullptr)
		return;

	auto name = thisRow->name;

	auto c_n = name.upToFirstOccurrenceOf(".", false, false);

	auto className = c_n.isNotEmpty() ? Identifier(c_n) : Identifier();

	name = name.fromFirstOccurrenceOf(".", false, false);

	auto m_n = name.upToFirstOccurrenceOf("(", false, false);

	auto methodName = m_n.isNotEmpty() ? Identifier(m_n) : Identifier();

	
	{
		String s;
		s << "scripting/scripting-api/";
		s << MarkdownLink::Helpers::getSanitizedFilename(thisRow->classId.toString());
		s << "#";
		s << MarkdownLink::Helpers::getSanitizedFilename(methodName.toString()) << "/";

		currentLink = { File(), s };
	}

#if 0

	helpButton->setVisible(hasExtendedHelp);

	if (hasExtendedHelp)
	{
		auto r = getLocalArea(listbox, listbox->getRowPosition(rowIndex, true));

		r = r.withWidth(r.getHeight()).reduced(3);

		helpButton->setBounds(r);

		helpButton->setHelpText(extendedHelp);

	}
#endif

	listbox->selectRow(currentlySelectedBox);
	listbox->repaintRow(currentlySelectedBox);
	infoBox->setInfo(visibleInfo[currentlySelectedBox]);
}

void JavascriptCodeEditor::AutoCompletePopup::rebuildVisibleItems(const String &selection)
{
	lastText = selection;

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

hise::MarkdownLink JavascriptCodeEditor::AutoCompletePopup::getLink() const
{
	return currentLink;
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
		char c = 'U';

		Colour colour = Colours::white;

		if (auto p = findParentComponentOfClass<ApiProviderBase::ApiComponentBase>())
		{
			if (auto pr = p->getProviderBase())
				pr->getColourAndLetterForType(currentInfo->type, colour, c);
		}

		g.setColour(colour);

		const Rectangle<float> area(5.0f, (float)(getHeight() / 2 - 12), (float)24.0f, (float)24.0f);

		g.fillRoundedRectangle(area, 5.0f);
		g.setColour(Colours::black.withAlpha(0.4f));
		g.drawRoundedRectangle(area, 5.0f, 1.0f);
		g.setFont(GLOBAL_BOLD_FONT());
		g.setColour(Colours::white);

		String type;
		type << c;
		g.drawText(type, area, Justification::centred);

		const Rectangle<float> infoRectangle(area.getRight() + 8.0f, 2.0f, (float)getWidth() - area.getRight() - 8.0f, (float)getHeight() - 4.0f);

		infoText.draw(g, infoRectangle);
	}
}


JavascriptCodeEditor::AutoCompletePopup::RowInfo::RowInfo(DebugInformationBase::Ptr info)
{
	category = info->getCategory();
	type = info->getType();
	description = info->getDescription();
	name = info->getTextForName();
	typeName = info->getTextForDataType();
	value = info->getTextForValue();
	codeToInsert = info->getCodeToInsert();
}

hise::CommonEditorFunctions::EditorType* CommonEditorFunctions::as(Component* c)
{
	if (c == nullptr)
		return nullptr;

	if (auto e = dynamic_cast<EditorType*>(c))
		return e;

	auto e = c->findParentComponentOfClass<EditorType>();

	if (e == nullptr)
	{
		for (int i = 0; i < c->getNumChildComponents(); i++)
		{
			if ((e = dynamic_cast<EditorType*>(c->getChildComponent(i))) != nullptr)
				return e;
		}
	}
	else
		return e;

	jassertfalse;
	return nullptr;
}

juce::CodeDocument::Position CommonEditorFunctions::getCaretPos(Component* c)
{
	auto ed = as(c);

	// this must always be true...
	jassert(ed != nullptr);

#if HISE_USE_NEW_CODE_EDITOR
	auto pos = ed->editor.getTextDocument().getSelection(0).head;
	return CodeDocument::Position(getDoc(c), pos.x, pos.y);
#else
	return ed->getCaretPos();
#endif
}

juce::CodeDocument& CommonEditorFunctions::getDoc(Component* c)
{
	if (auto ed = as(c))
	{
#if HISE_USE_NEW_CODE_EDITOR
		return ed->editor.getDocument();
#else
		return ed->getDocument();
#endif
	}

	jassertfalse;
	static CodeDocument d;
	return d;
}

String CommonEditorFunctions::getCurrentToken(Component* c)
{
	if (auto ed = as(c))
	{
#if HISE_USE_NEW_CODE_EDITOR

		auto& doc = ed->editor.getTextDocument();

		auto cs = doc.getSelection(0);

		doc.navigate(cs.tail, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::backwardCol);
		doc.navigate(cs.head, mcl::TextDocument::Target::subword, mcl::TextDocument::Direction::forwardCol);

		auto d = doc.getSelectionContent(cs);

		return d;
#else
		return ed->getCurrentToken();
#endif
	}

	return {};
}

void CommonEditorFunctions::insertTextAtCaret(Component* c, const String& t)
{
	if (auto ed = as(c))
	{

#if HISE_USE_NEW_CODE_EDITOR
		ed->editor.insert(t);
#else
		ed->insertTextAtCaret(t);
#endif
	}
}

String CommonEditorFunctions::getCurrentSelection(Component* c)
{
	if (auto ed = as(c))
	{
#if HISE_USE_NEW_CODE_EDITOR
		auto& doc = ed->editor.getTextDocument();
		return doc.getSelectionContent(doc.getSelection(0));
#else
		return ed->getTextInRange(ed->getHighlightedRegion());
#endif
	}

	return {};
}

void CommonEditorFunctions::moveCaretTo(Component* c, CodeDocument::Position& pos, bool select)
{
	if (auto ed = as(c))
	{
#if HISE_USE_NEW_CODE_EDITOR
		mcl::Selection s(*pos.getOwner(), pos.getPosition(), pos.getPosition());
		ed->editor.getTextDocument().setSelection(0, s, true);
#else
		ed->moveCaretTo(pos, select);
#endif
	}

}

} // namespace hise
