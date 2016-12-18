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


class CodeReplacer : public ThreadWithAsyncProgressWindow,
	public TextEditorListener
{
public:

	CodeReplacer(JavascriptCodeEditor *editor_) :
		ThreadWithAsyncProgressWindow("Search & Replace"),
		editor(editor_)
	{
		addTextEditor("search", editor->getTextInRange(editor->getHighlightedRegion()), "Search for");
		getTextEditor("search")->addListener(this);

		StringArray regexList; regexList.add("Yes"); regexList.add("No");

		addComboBox("useRegex", regexList, "RegEx");

		addTextEditor("replace", SystemClipboard::getTextFromClipboard().removeCharacters("\n\r"), "Replace with");

		addButton("Next", 3, KeyPress(KeyPress::rightKey));
		addButton("Replace Next", 4, KeyPress(KeyPress::returnKey));
		addButton("Replace All", 5);


		addBasicComponents(false);
	}

	~CodeReplacer()
	{
		editor = nullptr;
	}

	void resized()
	{

	}

	Array<JavascriptCodeEditor::CodeRegion> getRegionsFor(const String &searchTerm)
	{
		const String allText = editor->getDocument().getAllContent();

		String analyseString = allText;
		String search = searchTerm;
		Array<JavascriptCodeEditor::CodeRegion> newHighlight;

		const bool useRegex = false;

		if (useRegex)
		{
			Array<StringArray> matches = RegexFunctions::findSubstringsThatMatchWildcard(search, allText);

			for (auto m : matches)
			{
				search = m[0];

				analyseString = analyseString.fromFirstOccurrenceOf(search, false, false);

				const int index = allText.length() - analyseString.length() - search.length();

				JavascriptCodeEditor::CodeRegion newRegion;
				newRegion.setStart(index);
				newRegion.setLength(search.length());

				newHighlight.add(newRegion);
			}
		}
		else
		{
			while (search.isNotEmpty() && analyseString.contains(search))
			{
				analyseString = analyseString.fromFirstOccurrenceOf(search, false, false);

				const int index = allText.length() - analyseString.length() - search.length();

				JavascriptCodeEditor::CodeRegion newRegion;
				newRegion.setStart(index);
				newRegion.setLength(search.length());

				newHighlight.add(newRegion);
			}

		}

		return newHighlight;
	}

	void textEditorTextChanged(TextEditor& e)
	{
		const String allText = editor->getDocument().getAllContent();

		String analyseString = allText;

		String search = e.getText();

		Array<JavascriptCodeEditor::CodeRegion> newHighlight = getRegionsFor(search);

		showStatusMessage(String(newHighlight.size()) + "matches.");

		editor->rebuildHighlightedSelection(newHighlight);


	}

	void resultButtonClicked(const String &name)
	{
		if (name == "Next")
		{
			goToNextMatch();

		}
		else if (name == "Replace Next")
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
		else if (name == "Replace All")
		{
			const String search = getTextEditor("search")->getText();
			const String replace = getTextEditor("replace")->getText();

			const String allText = editor->getDocument().getAllContent();
			editor->getDocument().replaceAllContent(allText.replace(search, replace, false));

			Array<JavascriptCodeEditor::CodeRegion> regions = getRegionsFor(replace);
			editor->rebuildHighlightedSelection(regions);
		}
		else if (name == "Cancel")
		{
			Array<JavascriptCodeEditor::CodeRegion> emptyRegions = Array<JavascriptCodeEditor::CodeRegion>();

			editor->rebuildHighlightedSelection(emptyRegions);
		}
	}


	void run() override
	{

	}

	void threadFinished() override
	{

	}

	static String createFactoryMethod(const String& definition)
	{
		StringArray lines = StringArray::fromLines(definition);

		for (int i = 0; i < lines.size(); i++)
		{
			lines.set(i, lines[i].upToFirstOccurrenceOf("//", false, false));
		}

		if (lines.size() != 0)
		{
			const String firstLineRegex = "(const var )(\\w+)\\s*=\\s*(Content.add\\w+)\\(\\s*(\"\\w+\"),\\s*(\\d+),\\s*(\\d+)";
			//const String firstLineRegex = "(const var)\\s+(\\w*)\\s*=\\s*(Content.add\\w+)\\(\\s*(\"\\w+\"),\\s*(\\d+),\\s*(\\d+)";
			const StringArray firstLineData = RegexFunctions::getFirstMatch(firstLineRegex, lines[0]);

			if (firstLineData.size() == 7)
			{

				const String componentName = firstLineData[2];
				const String componentType = firstLineData[3];
				const String componentId = firstLineData[4];
				const String componentX = firstLineData[5];
				const String componentY = firstLineData[6];

				StringArray newLines;

				String functionName = PresetHandler::getCustomName("Factory Method");

				const String inlineDefinition = "inline function " + functionName + "(name, x, y)\n{";

				newLines.add(inlineDefinition);

				const String newFirstLine = "\tlocal widget = " + componentType + "(name, x, y);";

				newLines.add(newFirstLine);

				for (int i = 1; i < lines.size(); i++)
				{
					newLines.add("    " + lines[i].replace(componentId, "name").replace(componentName + ".", "widget."));
				}

				newLines.add("    return widget;\n};\n");

				const String newWidgetDefinition = "const var " + componentName + " = " +
					functionName + "(" +
					componentId + ", " +
					componentX + ", " +
					componentY + ");\n";

				newLines.add(newWidgetDefinition);

				return newLines.joinIntoString("\n");
			}


		}

		return definition;
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
			editor->moveCaretToTop(false);

			remainingText = editor->getDocument().getAllContent();
			offset = remainingText.indexOf(search);
		}
		else
		{
			editor->moveCaretTo(editor->getCaretPos().movedBy(offset), false);
			editor->moveCaretTo(editor->getCaretPos().movedBy(search.length()), true);
		}
	}

	JavascriptCodeEditor *editor;

};



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
