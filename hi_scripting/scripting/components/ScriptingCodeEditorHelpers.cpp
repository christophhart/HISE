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

	CodeReplacer(PopupIncludeEditor::EditorType *editor_) :
		DialogWindowWithBackgroundThread("Search & Replace"),
		editor(editor_)
	{
		auto selection = PopupIncludeEditor::CommonEditorFunctions::getCurrentSelection(editor);

		addTextEditor("search", selection, "Search for");
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
		refreshSelection(editor, "");

		editor = nullptr;
	}

	void resized()
	{

	}

	static void refreshSelection(PopupIncludeEditor::EditorType* ed, const String& search)
	{
		auto newHighlight = getRegionsFor(ed, search);

#if HISE_USE_NEW_CODE_EDITOR
		Array<mcl::Selection> newSelections;

		auto& doc = ed->editor.getDocument();

		for (auto h : newHighlight)
			newSelections.add({ doc, h.getStart(), h.getEnd() });

		ed->editor.getTextDocument().setSearchResults(newSelections);
		ed->editor.repaint();
#else
		ed->rebuildHighlightedSelection(newHighlight);
#endif
	}

	static Array<JavascriptCodeEditor::CodeRegion> getRegionsFor(PopupIncludeEditor::EditorType* editor, const String &searchTerm)
	{
		const String allText = PopupIncludeEditor::CommonEditorFunctions::getDoc(editor).getAllContent();

		String analyseString = allText;
		String search = searchTerm;
		Array<JavascriptCodeEditor::CodeRegion> newHighlight;

		if (searchTerm.isEmpty())
			return newHighlight;

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
		const String allText = PopupIncludeEditor::CommonEditorFunctions::getDoc(editor).getAllContent();

		String analyseString = allText;
		String search = e.getText();

		Array<JavascriptCodeEditor::CodeRegion> newHighlight = getRegionsFor(editor, search);

		showStatusMessage(String(newHighlight.size()) + "matches.");

		refreshSelection(editor, search);
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

			const String selected = PopupIncludeEditor::CommonEditorFunctions::getCurrentSelection(editor);

			if (selected == search)
			{
				PopupIncludeEditor::CommonEditorFunctions::insertTextAtCaret(editor, replace);
			}

			goToNextMatch();
		}
		else if (name == "Replace All")
		{
			const String search = getTextEditor("search")->getText();
			const String replace = getTextEditor("replace")->getText();

			auto& doc = PopupIncludeEditor::CommonEditorFunctions::getDoc(editor);

			const String allText = doc.getAllContent();
			doc.replaceAllContent(allText.replace(search, replace, false));

			refreshSelection(editor, search);
		}
		else if (name == "Cancel")
		{
			Array<JavascriptCodeEditor::CodeRegion> emptyRegions = Array<JavascriptCodeEditor::CodeRegion>();

			refreshSelection(editor, "");
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
#if HISE_USE_NEW_CODE_EDITOR

		jassertfalse;

#else
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
#endif
	}

	PopupIncludeEditor::EditorType *editor;

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

	ReferenceFinder(PopupIncludeEditor::EditorType* editor_, JavascriptProcessor* jp_) :
		DialogWindowWithBackgroundThread("Find all occurrences"),
		editor(editor_),
		mc(dynamic_cast<Processor*>(jp_)->getMainController()),
		jp(jp_)
	{
		addTextEditor("searchTerm", PopupIncludeEditor::CommonEditorFunctions::getCurrentToken(editor), "Search term");
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

		getTextEditor("searchTerm")->grabKeyboardFocusAsync();
	}

	~ReferenceFinder()
	{
		if(editor != nullptr)
			CodeReplacer::refreshSelection(editor, "");
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
			auto thisDoc = &PopupIncludeEditor::CommonEditorFunctions::getDoc(editor);

			for (int i = 0; i < jp->getNumSnippets(); i++)
			{
				auto f = jp->getSnippet(i);

				const bool useFile = lookInAllFiles || f == thisDoc;

				if (!useFile)
					continue;

				fillArrayWithDoc(*f, search, jp->getSnippet(i)->getCallbackName().toString() + "()");
			}

			for (int i = 0; i < jp->getNumWatchedFiles(); i++)
			{
				auto& doc = jp->getWatchedFileDocument(i);

				const bool useFile = lookInAllFiles || &doc == thisDoc;

				if (!useFile)
					continue;

				fillArrayWithDoc(doc, search, jp->getWatchedFile(i).getFullPathName());
			}
		}

		showStatusMessage(String(entries.size()) + " occurrences found");

		table->updateContent();

		CodeReplacer::refreshSelection(editor, search);
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

			entries.add(newEntry);
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

			editor = PopupIncludeEditor::CommonEditorFunctions::as(mc->getLastActiveEditor());

			if (editor != nullptr)
			{
				auto search = getTextEditor("searchTerm")->getText();
				CodeReplacer::refreshSelection(editor, search);
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
	Component::SafePointer<PopupIncludeEditor::EditorType> editor;
	ScopedPointer<TableListBox> table;
	MainController* mc;
	JavascriptProcessor* jp;
};


bool JavascriptProcessor::handleKeyPress(const KeyPress& k, Component* c)
{
	if ((k.isKeyCode('f') || k.isKeyCode('F')) && k.getModifiers().isCommandDown()) // Ctrl + F
	{
		performPopupMenuAction(ScriptContextActions::FindAllOccurences, c);
		return true;
	}
	else if ((k.isKeyCode('h') || k.isKeyCode('H')) && k.getModifiers().isCommandDown()) // Ctrl + F
	{
		performPopupMenuAction(ScriptContextActions::SearchAndReplace, c);
		return true;
	}

	return false;
}

struct TemplateSelector : public Component,
	public ButtonListener,
	public Timer
{
	TemplateSelector(PopupIncludeEditor* c, JavascriptProcessor* jp_, const StringArray& allIds_) :
		ed(c),
		ok("OK"),
		cancel("Cancel"),
		jp(jp_),
		allIds(allIds_)
	{
		addAndMakeVisible(l);
		addAndMakeVisible(cb);
		addAndMakeVisible(ok);
		addAndMakeVisible(cancel);

		ok.setLookAndFeel(&alaf);
		cancel.setLookAndFeel(&alaf);
		cb.setLookAndFeel(&alaf);
		l.setLookAndFeel(&alaf);

		ok.addListener(this);
		cancel.addListener(this);

		setWantsKeyboardFocus(true);
		cb.setWantsKeyboardFocus(false);
		l.setWantsKeyboardFocus(false);
		ok.setWantsKeyboardFocus(false);
		cancel.setWantsKeyboardFocus(false);

		cb.addItemList(allIds, 1);

		GlobalHiseLookAndFeel::setDefaultColours(cb);

		l.setFont(GLOBAL_MONOSPACE_FONT());

		l.setText(PopupIncludeEditor::CommonEditorFunctions::getCurrentToken(c->getEditor()), dontSendNotification);
		l.setEditable(false);
		l.setColour(Label::textColourId, Colours::white);
		l.setColour(Label::backgroundColourId, Colours::white.withAlpha(0.1f));

		setName("Add autocomplete template");
		setSize(500, 150);

		if (auto mbw = c->findParentComponentOfClass<ModalBaseWindow>())
			mbw->setModalComponent(this);

		grabKeyboardFocus();
		startTimer(1000);
	}

	void timerCallback() override
	{
		searchTerm = {};
		repaint();
	}

	bool keyPressed(const KeyPress& k) override
	{
		if (k == KeyPress::returnKey)
		{
			ok.triggerClick();
			return true;
		}
		if (k == KeyPress::escapeKey)
		{
			cancel.triggerClick();
			return true;
		}
		if (k == KeyPress::upKey)
		{
			cb.setSelectedItemIndex(jmax(0, cb.getSelectedItemIndex() - 1), dontSendNotification);
			return true;
		}
		if (k == KeyPress::downKey)
		{
			cb.setSelectedItemIndex(jmin(allIds.size() - 1, cb.getSelectedItemIndex() + 1), dontSendNotification);
			return true;
		}

		searchTerm << k.getTextCharacter();
		startTimer(1000);

		repaint();

		int index = 0;

		for (const auto& s : allIds)
		{
			if (s.toLowerCase().startsWith(searchTerm.toLowerCase()))
			{
				cb.setSelectedItemIndex(index, dontSendNotification);
				return true;
			}

			index++;
		}

		return true;
	}

	String searchTerm;

	void buttonClicked(Button* b) override
	{
#if HISE_USE_NEW_CODE_EDITOR
		if (b == &ok)
		{
			jp->autoCompleteTemplates.add({ l.getText(), Identifier(cb.getText()) });
			ed->getEditor()->editor.tokenCollection.signalRebuild();
		}
#endif

		if (auto mbw = b->findParentComponentOfClass<ModalBaseWindow>())
		{
			Component::SafePointer<PopupIncludeEditor> c(ed);

			MessageManager::callAsync([mbw, c]()
				{
					mbw->clearModalComponent();

					if (c.getComponent() != nullptr)
					{
						c->grabKeyboardFocus();

#if !HISE_USE_NEW_CODE_EDITOR
						c->getEditor()->moveCaretTo(c->getEditor()->getSelectionEnd(), false);
#endif
					}

				});
		}
	}

	void paint(Graphics& g) override
	{
		auto dark = Colour(0xFF252525);
		auto bright = Colour(0xFFAAAAAA);

		ColourGradient grad(dark.withMultipliedBrightness(1.4f), 0.0f, 0.0f,
			dark, 0.0f, (float)getHeight(), false);

		g.setGradientFill(grad);
		g.fillAll();
		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillRect(0, 0, getWidth(), 37);
		g.setColour(bright);
		g.setColour(bright);
		g.drawRect(0, 0, getWidth(), getHeight());

		auto b = getLocalBounds().removeFromTop(37).toFloat();

		g.setColour(bright);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText("Select a Class type that will be shown for the given expression", b, Justification::centred);

		Path p;

		Line<float> l(arrowBounds.getX(), arrowBounds.getCentreY(), arrowBounds.getRight(), arrowBounds.getCentreY());

		p.addArrow(l, 5.0, 20.f, 20.0f);

		g.fillPath(p);

		if (searchTerm.isNotEmpty())
		{
			auto d = cb.getBoundsInParent().toFloat().translated(10.0, -24.0f).reduced(3.0f);
			g.setColour(Colours::white.withAlpha(0.4f));
			g.setFont(GLOBAL_FONT());
			g.drawText(searchTerm, d, Justification::left);
		}
	}

	void resized() override
	{
		auto b = getLocalBounds().reduced(20);

		b.removeFromTop(40);
		auto br = b.removeFromBottom(28);
		b.removeFromBottom(10);

		cb.setBounds(b.removeFromRight(130).reduced(5));
		arrowBounds = b.removeFromRight(b.getHeight());
		l.setBounds(b.reduced(5));

		ok.setBounds(br.removeFromRight(br.getWidth() / 2).reduced(10, 0));
		cancel.setBounds(br.reduced(10, 0));
	}

	AlertWindowLookAndFeel alaf;

	Rectangle<int> arrowBounds;

	Label l;
	ComboBox cb;
	TextButton ok;
	TextButton cancel;

	PopupIncludeEditor* ed;
	WeakReference<JavascriptProcessor> jp;
	const StringArray allIds;
};


struct ScriptRefactoring
{
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

				const String newFirstLine = "\tlocal component = " + componentType + "(name, x, y);";

				newLines.add(newFirstLine);

				for (int i = 1; i < lines.size(); i++)
				{
					newLines.add("    " + lines[i].replace(componentId, "name").replace(componentName + ".", "component."));
				}

				newLines.add("    return component;\n};\n");

				const String newComponentDefinition = "const var " + componentName + " = " +
					functionName + "(" +
					componentId + ", " +
					componentX + ", " +
					componentY + ");\n";

				newLines.add(newComponentDefinition);

				return newLines.joinIntoString("\n");
			}


		}

		return definition;
	}


	static const String createScriptComponentReference(const String selection)
	{
		String regexString = "(\\s*)(const\\s+var |local )(\\w+)\\s*=\\s*(Content.add\\w+)\\(\\s*(\"\\w+\"),\\s*(\\d+),\\s*(\\d+)";

		const StringArray firstLineData = RegexFunctions::getFirstMatch(regexString, selection);

		if (firstLineData.size() == 8)
		{

			const String whitespace = firstLineData[1];
			const String variableType = firstLineData[2];
			const String componentName = firstLineData[3];
			const String componentType = firstLineData[4];
			const String componentId = firstLineData[5];

			return whitespace + variableType + componentName + " = Content.getComponent(" + componentId + ");";
		}

		PresetHandler::showMessageWindow("Something went wrong...", "The replacement didn't work");
		return selection;
	}
};



bool JavascriptProcessor::performPopupMenuAction(int menuId, Component* c)
{
	auto action = (ScriptContextActions)menuId;

	switch (action)
	{
	case CreateUiFactoryMethod:
	{
		const String selection = PopupIncludeEditor::CommonEditorFunctions::getCurrentSelection(c);
		const String newText = ScriptRefactoring::createFactoryMethod(selection);
		PopupIncludeEditor::CommonEditorFunctions::insertTextAtCaret(c, newText);
		return true;
	}
	case AddAutocompleteTemplate:
	{
		auto v = createApiTree();

		StringArray classIds;

		for (auto c : v)
			classIds.add(c.getType().toString());

		new TemplateSelector(c->findParentComponentOfClass<PopupIncludeEditor>(), this, classIds);

		return true;
	}
	case ScriptContextActions::ClearAutocompleteTemplates:
	{
#if HISE_USE_NEW_CODE_EDITOR
		if (PresetHandler::showYesNoWindow("Clear autocomplete templates", "Do you want to clear all autocomplete templates?"))
		{
			autoCompleteTemplates.clear();
			PopupIncludeEditor::CommonEditorFunctions::as(c)->editor.tokenCollection.signalRebuild();
		}
#endif

		return true;
	}
	case ClearAllBreakpoints:
	{
		removeAllBreakpoints();
		c->repaint();
		return true;
	}
	case SaveScriptFile:
	{
		FileChooser scriptSaver("Save script as",
			File(GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
			"*.js");

		if (scriptSaver.browseForFileToSave(true))
		{
			String script;
			mergeCallbacksToScript(script);
			scriptSaver.getResult().replaceWithText(script);
			debugToConsole(dynamic_cast<Processor*>(this), "Script saved to " + scriptSaver.getResult().getFullPathName());
		}
		return true;
	}
	case LoadScriptFile:
	{
		FileChooser scriptLoader("Please select the script you want to load",
			File(GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
			"*.js");

		if (scriptLoader.browseForFileToOpen())
		{
			String script = scriptLoader.getResult().loadFileAsString().removeCharacters("\r");
			const bool success = parseSnippetsFromString(script);

			if (success)
			{
				compileScript();

				debugToConsole(dynamic_cast<Processor*>(this), "Script loaded from " + scriptLoader.getResult().getFullPathName());
			}
		}

		return true;
	}
	case ExportAsCompressedScript:
	{
		const String compressedScript = getBase64CompressedScript();
		const String scriptName = PresetHandler::getCustomName("Compressed Script") + ".cjs";
		File f = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getSubDirectory(ProjectHandler::SubDirectories::Scripts).getChildFile(scriptName);

		if (!f.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite", "The file " + scriptName + " already exists. Do you want to overwrite it?"))
		{
			f.deleteFile();
			f.replaceWithText(compressedScript);
		}

		return true;
	}
	case ImportCompressedScript:
	{
		FileChooser scriptLoader("Please select the compressed script you want to load",
			File(GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
			"*.cjs");

		if (scriptLoader.browseForFileToOpen())
		{
			String compressedScript = scriptLoader.getResult().loadFileAsString();

			const bool success = restoreBase64CompressedScript(compressedScript);

			if (success)
			{
				compileScript();
				debugToConsole(dynamic_cast<Processor*>(this), "Compressed Script loaded from " + scriptLoader.getResult().getFullPathName());
			}
		}

		return true;
	}
	case SaveScriptClipboard:
	{
		String x;
		mergeCallbacksToScript(x);
		SystemClipboard::copyTextToClipboard(x);

		debugToConsole(dynamic_cast<Processor*>(this), "Script exported to Clipboard.");

		return true;
	}
	case LoadScriptClipboard:
	{
		String x = String(SystemClipboard::getTextFromClipboard()).removeCharacters("\r");

		if (x.containsNonWhitespaceChars() && PresetHandler::showYesNoWindow("Replace Script?", "Do you want to replace the script?"))
		{
			const bool success = parseSnippetsFromString(x);

			if (success)
				compileScript();
		}

		return true;
	}
	case MoveToExternalFile:
	{
		const String text = PopupIncludeEditor::CommonEditorFunctions::getCurrentSelection(c);

		const String newFileName = PresetHandler::getCustomName("Script File", "Enter the file name for the external script file (without .js)");

		if (newFileName.isNotEmpty())
		{
			File scriptDirectory = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getSubDirectory(ProjectHandler::SubDirectories::Scripts);

			File newFile = scriptDirectory.getChildFile(newFileName + ".js");

			if (!newFile.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite existing file", "Do you want to overwrite the file " + newFile.getFullPathName() + "?"))
			{
				newFile.replaceWithText(text);
			}

			String insertStatement = "include(\"" + newFile.getFileName() + "\");" + NewLine();
			PopupIncludeEditor::CommonEditorFunctions::insertTextAtCaret(c, insertStatement);
		}

		return true;
	}
	case JumpToDefinition:
	{
		auto token = PopupIncludeEditor::CommonEditorFunctions::getCurrentToken(c);
		const String namespaceId = JavascriptCodeEditor::Helpers::findNamespaceForPosition(PopupIncludeEditor::CommonEditorFunctions::getCaretPos(c));
		jumpToDefinition(token, namespaceId);

		return true;
	}
	case FindAllOccurences:
	{
		auto finder = new ReferenceFinder(PopupIncludeEditor::CommonEditorFunctions::as(c), this);
		finder->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(c));
		finder->grabKeyboardFocus();
		return true;
	}
	case SearchAndReplace:
	{
		auto replacer = new CodeReplacer(PopupIncludeEditor::CommonEditorFunctions::as(c));
		replacer->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(c));
		replacer->getTextEditor("search")->grabKeyboardFocus();
		return true;
		return true;
	}
	default:
		return false;
	}
}

} // namespace hise
