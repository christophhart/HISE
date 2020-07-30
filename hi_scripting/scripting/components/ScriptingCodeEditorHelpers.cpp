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



class CodeReplacer : public DialogWindowWithBackgroundThread,
	public TextEditorListener,
    public Timer
{
public:

	CodeReplacer(JavascriptCodeEditor *editor_) :
		DialogWindowWithBackgroundThread("Search & Replace"),
		editor(editor_)
	{
		addTextEditor("search", editor->getTextInRange(editor->getHighlightedRegion()), "Search for");
		getTextEditor("search")->addListener(this);

		StringArray regexList; regexList.add("Yes"); regexList.add("No");

		addComboBox("useRegex", regexList, "RegEx");

		addTextEditor("replace", SystemClipboard::getTextFromClipboard().removeCharacters("\n\r"), "Replace with");

		addButton("Next", 3, KeyPress(KeyPress::returnKey));
		addButton("Replace Next", 4);
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

	static Array<JavascriptCodeEditor::CodeRegion> getRegionsFor(JavascriptCodeEditor* editor, const String &searchTerm)
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

		Array<JavascriptCodeEditor::CodeRegion> newHighlight = getRegionsFor(editor, search);

		showStatusMessage(String(newHighlight.size()) + "matches.");

		editor->rebuildHighlightedSelection(newHighlight);


	}

    void timerCallback() override
    {
        debounce = false;
        stopTimer();
    }
    
	void resultButtonClicked(const String &name)
	{
        if(debounce)
            return;
        
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

			Array<JavascriptCodeEditor::CodeRegion> regions = getRegionsFor(editor, replace);
			editor->rebuildHighlightedSelection(regions);
		}
		else if (name == "Cancel")
		{
			Array<JavascriptCodeEditor::CodeRegion> emptyRegions = Array<JavascriptCodeEditor::CodeRegion>();

			editor->rebuildHighlightedSelection(emptyRegions);
		}
        
        debounce = true;
        startTimer(200);
	}


	void run() override
	{

	}

	void threadFinished() override
	{

	}

private:

    bool debounce = false;
    
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


class ReferenceFinder : public DialogWindowWithBackgroundThread,
	public TableListBoxModel,
	public TextEditorListener,
	public ComboBoxListener
{
public:

	enum Columns
	{
		Document = 1,
		Line,
		SurroundingText,
		numColumns
	};

	ReferenceFinder(JavascriptCodeEditor* editor_, JavascriptProcessor* jp_) :
		DialogWindowWithBackgroundThread("Find all occurrences"),
		editor(editor_),
		mc(dynamic_cast<Processor*>(jp_)->getMainController()),
		jp(jp_)
	{

		//GET_BACKEND_ROOT_WINDOW(this)

		addTextEditor("searchTerm", editor->getCurrentToken(), "Search term");
		getTextEditor("searchTerm")->addListener(this);


		StringArray sa;

		sa.add("All included files");
		sa.add("Currently opened script");

		addComboBox("searchArea", sa, "Look in");
		getComboBoxComponent("searchArea")->addListener(this);

		addAndMakeVisible(table = new TableListBox());
		table->setModel(this);
		table->getHeader().setLookAndFeel(&laf);
		table->getHeader().setSize(getWidth(), 22);
		table->setOutlineThickness(0);
		table->getViewport()->setScrollBarsShown(true, false, false, false);

		table->setColour(ListBox::backgroundColourId, JUCE_LIVE_CONSTANT_OFF(Colour(0x04ffffff)));

		table->getHeader().addColumn("File", Document, 110, 110, 110);
		table->getHeader().addColumn("Line", Line, 40, 40, 40);
		table->getHeader().addColumn("Text", SurroundingText, 200, -1, -1);

		table->getHeader().setStretchToFitActive(true);

		table->setSize(600, 300);

		addCustomComponent(table);



		addTextEditor("state", "", "Status", false);
		getTextEditor("state")->setReadOnly(true);

		addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

		rebuildLines();
	}

	struct TableEntry
	{
		bool operator==(const TableEntry& other) const
		{
			return other.doc == this->doc && other.pos.getLineNumber() == this->pos.getLineNumber();
		}


		String fileName;


		const CodeDocument* doc;
		CodeDocument::Position pos;
		String lineContent;
	};

	void textEditorTextChanged(TextEditor&) override
	{


		rebuildLines();
	}

	OwnedArray<TableEntry> entries;

	void comboBoxChanged(ComboBox* /*comboBoxThatHasChanged*/) override
	{
		rebuildLines();
	}

	void rebuildLines()
	{
		entries.clear();

		auto search = getTextEditor("searchTerm")->getText();

		const bool lookInAllFiles = getComboBoxComponent("searchArea")->getSelectedItemIndex() == 0;

		if (search.isNotEmpty())
		{
			for (int i = 0; i < jp->getNumSnippets(); i++)
			{
				auto f = jp->getSnippet(i);

				const bool useFile = lookInAllFiles || f == &editor->getDocument();

				if (!useFile)
					continue;

				fillArrayWithDoc(*f, search, jp->getSnippet(i)->getCallbackName().toString() + "()");
			}

			for (int i = 0; i < jp->getNumWatchedFiles(); i++)
			{
				auto& doc = jp->getWatchedFileDocument(i);

				const bool useFile = lookInAllFiles || &doc == &editor->getDocument();

				if (!useFile)
					continue;

				fillArrayWithDoc(doc, search, jp->getWatchedFile(i).getFullPathName());
			}
		}

		showStatusMessage(String(entries.size()) + " occurrences found");

		table->updateContent();

		auto newHighlight = CodeReplacer::getRegionsFor(editor, search);

		editor->rebuildHighlightedSelection(newHighlight);
	}

	void fillArrayWithDoc(const CodeDocument &doc, const String &search, const String& nameToShow)
	{
		auto allText = doc.getAllContent();

		String analyseString = allText;

		while (search.isNotEmpty() && analyseString.contains(search))
		{
			analyseString = analyseString.fromFirstOccurrenceOf(search, false, false);

			const int index = allText.length() - analyseString.length() - search.length();

			JavascriptCodeEditor::CodeRegion newRegion;
			newRegion.setStart(index);
			newRegion.setLength(search.length());

			auto newEntry = new TableEntry();

			newEntry->doc = &doc;
			newEntry->pos = CodeDocument::Position(doc, index);
			newEntry->lineContent = doc.getLine(newEntry->pos.getLineNumber());
			newEntry->fileName = nameToShow;

			entries.addIfNotAlreadyThere(newEntry);
		}
	}

	void run() override
	{

	}

	void threadFinished() override
	{

	}

	int getNumRows() override
	{
		return entries.size();
	}

	void paintOverChildren(Graphics& g)
	{
		if (entries.size() == 0)
		{
			g.setColour(Colours::white.withAlpha(0.2f));
			g.setFont(GLOBAL_BOLD_FONT());
			g.drawText("Nothing found", 0, 0, getWidth(), getHeight(), Justification::centred);
		}
	}

	void paintRowBackground(Graphics& g, int /*rowNumber*/, int /*width*/, int /*height*/, bool rowIsSelected) override
	{
		if (rowIsSelected)
			g.fillAll(Colour(0x66000000));
	}

	void selectedRowsChanged(int lastRowSelected) override
	{
		if (auto entry = entries[lastRowSelected])
		{
			DebugableObject::Location loc;

			loc.fileName = entry->fileName;
			loc.charNumber = entry->pos.getPosition();

			DebugableObject::Helpers::gotoLocation(editor, jp, loc);

			editor = dynamic_cast<JavascriptCodeEditor*>(mc->getLastActiveEditor());

			if (editor != nullptr)
			{
				auto search = getTextEditor("searchTerm")->getText();

				auto newHighlight = CodeReplacer::getRegionsFor(editor, search);

				editor->rebuildHighlightedSelection(newHighlight);
			}

			

		}
	}

	void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override
	{
		auto area = Rectangle<float>({ 2.0f, 0.f, (float)width, (float)height });

		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());

		if (auto entry = entries[rowNumber])
		{
			switch (columnId)
			{
			case Document:
			{
				if (entry->fileName.contains("()") || entry->fileName == "Current script")
				{
					g.drawText(entry->fileName, area, Justification::centredLeft);
				}
				else
				{
					auto f = File(entry->fileName);
					g.drawText(f.getFileName(), area, Justification::centredLeft);
				}

				break;
			}
			case Line:	g.drawText(String(entry->pos.getLineNumber() + 1), area, Justification::centredLeft); break;
			case SurroundingText:
			{
				g.setFont(GLOBAL_MONOSPACE_FONT());
				auto line = entry->doc->getLine(entry->pos.getLineNumber());
				line = line.trim();
				g.drawText(line, area, Justification::centredLeft);
				break;
			}
			}
		}


	}

	TableHeaderLookAndFeel laf;

	Component::SafePointer<JavascriptCodeEditor> editor;


	ScopedPointer<TableListBox> table;

	MainController* mc;
	JavascriptProcessor* jp;

};


bool JavascriptProcessor::handleKeyPress(const KeyPress& k, Component* c)
{
	if ((k.isKeyCode('f') || k.isKeyCode('F')) && k.getModifiers().isCommandDown()) // Ctrl + F
	{
		ReferenceFinder * finder = new ReferenceFinder(dynamic_cast<JavascriptCodeEditor*>(c), this);
		finder->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(c));
		finder->grabKeyboardFocus();
		return true;
	}
	else if ((k.isKeyCode('h') || k.isKeyCode('H')) && k.getModifiers().isCommandDown()) // Ctrl + F
	{
		CodeReplacer * replacer = new CodeReplacer(dynamic_cast<JavascriptCodeEditor*>(c));
		replacer->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(c));
		replacer->getTextEditor("search")->grabKeyboardFocus();
		return true;
	}

	return false;
}

} // namespace hise
