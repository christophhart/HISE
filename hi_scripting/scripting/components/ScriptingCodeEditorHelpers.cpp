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
		auto selection = CommonEditorFunctions::getCurrentSelection(editor);

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

		if (ed == nullptr)
			return;

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
		const String allText = CommonEditorFunctions::getDoc(editor).getAllContent();

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
		const String allText = CommonEditorFunctions::getDoc(editor).getAllContent();

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

			const String selected = CommonEditorFunctions::getCurrentSelection(editor);

			if (selected == search)
			{
				CommonEditorFunctions::insertTextAtCaret(editor, replace);
			}

			goToNextMatch();
		}
		else if (name == "Replace All")
		{
			const String search = getTextEditor("search")->getText();
			const String replace = getTextEditor("replace")->getText();

			auto& doc = CommonEditorFunctions::getDoc(editor);

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
	public ComboBoxListener,
	public ControlledObject,
	public KeyListener,
	public GlobalScriptCompileListener
{
public:

	enum class SearchMode
	{
		TextSearch,
		FileSearch,
		SymbolSearch,
		NamespaceSearch,
		VariableSearch,
		FunctionSearch
	};

	enum Columns
	{
		Document = 1,
		Line,
		SurroundingText,
		numColumns
	};

	void scriptWasCompiled(JavascriptProcessor *processor) override
	{
		if (jp == processor)
		{
			rebuildLines();
			
		}
	}

	struct DummyThread : public Thread
	{
		DummyThread(JavascriptProcessor* p) :
			Thread("dummy"),
			jp(p)
		{
			startThread();
		};

		void checkRecursive(DebugInformationBase::Ptr b)
		{
			if (b != nullptr)
			{
				auto v = b->getTextForValue();

				for (int i = 0; i < b->getNumChildElements(); i++)
				{
					checkRecursive(b->getChildElement(i));
				}
			}
			
		}

		void run() override
		{
			while (!threadShouldExit())
			{
				ApiProviderBase* base = dynamic_cast<ApiProviderBase::Holder*>(jp)->getProviderBase();
				
				int numObjects = base->getNumDebugObjects();

				for (int i = 0; i < numObjects; i++)
				{
					checkRecursive(base->getDebugInformation(i));
				}
			}
		}

		JavascriptProcessor* jp;
	};

    struct SearchOptions
    {
        SearchMode currentMode;
        String searchTerm;
        bool currentFileOnly = false;
        bool ignoreCase = false;
        bool wholeWord = false;
        bool regex = false;
        
        void parse(const String& search)
        {
            if (search.startsWithIgnoreCase("f "))
            {
                searchTerm = search.substring(2).trim();
                currentMode = SearchMode::FileSearch;
            }
            else if (search.startsWithIgnoreCase("s "))
            {
                searchTerm = search.substring(2).trim();
                currentMode = SearchMode::SymbolSearch;
            }
            else if (search.startsWithIgnoreCase("n "))
            {
                searchTerm = search.substring(2).trim();
                currentMode = SearchMode::NamespaceSearch;
            }
            else if (search.startsWithIgnoreCase("fn "))
            {
                searchTerm = search.substring(3).trim();
                currentMode = SearchMode::FunctionSearch;
            }
            else if (search.startsWithIgnoreCase("v "))
            {
                searchTerm = search.substring(2).trim();
                currentMode = SearchMode::VariableSearch;
            }
            else
            {
                searchTerm = search;
                currentMode = SearchMode::TextSearch;
            }
        }
        
        bool matchesWholeWord(const String& expandedSearch) const
        {
            if(!wholeWord)
                return true;
            
            auto first = expandedSearch[0];
            auto last = expandedSearch.getLastCharacter();
            
            auto valid = [](juce_wchar c)
            {
                return !CharacterFunctions::isLetter(c) &&
                       c != '_';
            };
            
            return valid(first) && valid(last);
        }
        
        bool matches(const String& expected, const String& expandedSearch) const
        {
            if(currentMode != SearchMode::TextSearch && searchTerm.isEmpty())
                return true;
            
            if(regex)
                return hise::RegexFunctions::matchesWildcard(expected, searchTerm);
            
            if(ignoreCase)
                return expected.containsIgnoreCase(searchTerm);
            else
                return expected.contains(searchTerm);
        }
        
    } currentOptions;
    
	ReferenceFinder(PopupIncludeEditor::EditorType* editor_, JavascriptProcessor* jp_) :
		DialogWindowWithBackgroundThread("Find all occurrences"),
		ControlledObject(dynamic_cast<ControlledObject*>(jp_)->getMainController()),
		editor(editor_),
		jp(jp_),
        optionsRow(new AdditionalRow(this))
	{
		// This simulates a tightly condensed multithreading access scenario to increase
		// the crash probability for debugging...
		//new DummyThread(jp_);

		setDestroyWhenFinished(false);

		auto initSearchTerm = CommonEditorFunctions::getCurrentToken(editor);

		if (initSearchTerm.contains("\n"))
			initSearchTerm = {};

		addTextEditor("searchTerm", initSearchTerm, "Search term");
		getTextEditor("searchTerm")->addListener(this);
		getTextEditor("searchTerm")->setIgnoreUpDownKeysWhenSingleLine(true);
		getTextEditor("searchTerm")->addKeyListener(this);

		StringArray sa;

		sa.add("All included files");
		sa.add("Currently opened script");

		optionsRow->addComboBox("searchArea", sa, "Look in");
        optionsRow->addComboBox("ignoreCase", {"No", "Yes"}, "Ignore Case");
        optionsRow->addComboBox("wholeWord", {"No", "Yes"}, "WholeWord");
        optionsRow->addComboBox("regex", { "No", "Yes"}, "Use RegEx");

        optionsRow->setSize(600, 40);
        addCustomComponent(optionsRow);
        
        
		

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

		table->setMultipleSelectionEnabled(true);

		addCustomComponent(table);

		addTextEditor("state", "", "Status", false);
		getTextEditor("state")->setReadOnly(true);


		addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

        getButton("Cancel")->addListener(this);
        
		currentOptions.parse(initSearchTerm);
		rebuildLines();

		getTextEditor("searchTerm")->grabKeyboardFocusAsync();

		numFixedComponents = getNumCustomComponents();
		getMainController()->addScriptListener(this);
	}

	~ReferenceFinder()
	{
		getMainController()->removeScriptListener(this);

		if(editor != nullptr)
			CodeReplacer::refreshSelection(editor, "");
	}

	struct TableEntry: public ReferenceCountedObject
	{
		using Ptr = ReferenceCountedObjectPtr<TableEntry>;
		using List = ReferenceCountedArray<TableEntry>;

		bool operator==(const TableEntry& other) const
		{
			return other.doc == this->doc && other.pos.getLineNumber() == this->pos.getLineNumber();
		}

		String fileName;

		const CodeDocument* doc;
		CodeDocument::Position pos;
		DebugInformationBase::Ptr info;

	};

	void textEditorTextChanged(TextEditor& t) override
	{
        currentOptions.parse(t.getText());
		rebuildLines();
	}

	TableEntry::List entries;

	void comboBoxChanged(ComboBox* cb) override
	{
        auto id = cb->getName();
        
        if(id == "searchArea")
            currentOptions.currentFileOnly = (bool)cb->getSelectedItemIndex();
        else if (id == "ignoreCase")
            currentOptions.ignoreCase = (bool)cb->getSelectedItemIndex();
        else if (id == "wholeWord")
            currentOptions.wholeWord = (bool)cb->getSelectedItemIndex();
        else if (id == "regex")
            currentOptions.regex = (bool)cb->getSelectedItemIndex();
            
		rebuildLines();
	}

	void rebuildLines()
	{
		pending = true;
		table->repaint();
		runThread();
	}

	void selectedRowsChanged(int lastRowSelected) override
	{
		removeCustomComponent(numFixedComponents);
		
		MouseEvent e(Desktop::getInstance().getMainMouseSource(), {}, {}, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			this, this, Time::getCurrentTime(), {}, Time::getCurrentTime(), 1, false);

		debugComponentRow = new AdditionalRow(this);

		auto ranges = table->getSelectedRows().getRanges();

		bool addedSomething = false;

		for (const auto& r : ranges)
		{
			for (int i = r.getStart(); i < r.getEnd(); i++)
			{
				if (auto eb = entries[i])
				{
					if (auto info = eb->info)
					{
						if (auto c = info->createPopupComponent(e, this))
						{
							c->setSize(c->getWidth(), 250);
							debugComponentRow->addCustomComponent(c);
							addedSomething = true;
						}
						else
						{
							auto text = info->getDescription().getText();

							if (text.isNotEmpty())
							{
								auto c = new SimpleMarkdownDisplay();
								c->setText(text);
								auto h = c->r.getHeightForWidth(table->getWidth());
								c->setSize(table->getWidth(), h);
								debugComponentRow->addCustomComponent(c);
								addedSomething = true;
								break;
							}
						}
					}
				}
			}
		}

		debugComponentRow->setSize(table->getWidth(), 250);

		if(addedSomething)
			addCustomComponent(debugComponentRow);
	}

	int getFileIndex(const String& filename)
	{
		if (filename.isEmpty())
			return -1;

		if (File::isAbsolutePath(filename))
		{
			File f(filename);

			for (int i = 0; i < jp->getNumWatchedFiles(); i++)
			{
				if (f == jp->getWatchedFile(i))
					return i;
			}

			return -1;
		}
		else
		{
			for (int i = 0; i < jp->getNumWatchedFiles(); i++)
			{
				if (jp->getWatchedFile(i).getFileName() == filename)
					return i;
			}

			return -1;
		}
	}

    
    
	void fillRecursive(TableEntry::List& listToUse, DebugInformationBase::Ptr db, const String& searchTerm, const Array<DebugInformation::Type>& typeFilters)
	{
		if (db == nullptr)
			return;

		if (threadShouldExit())
			return;

		auto location = db->getLocation();
        
		if (location.fileName.isNotEmpty() || location.charNumber != 0)
		{
			auto type = db->getTextForDataType();
			auto name = db->getTextForName();

			auto typeIndex = (DebugInformation::Type)db->getType();
			auto typeFilterMatch = typeFilters.isEmpty() || typeFilters.contains(typeIndex);

            if (typeFilterMatch && (currentOptions.matches(type, {}) ||
                                    currentOptions.matches(name, {})))
			{
				auto j = getFileIndex(location.fileName);

				if (j == -1)
				{
					auto doc = jp->getSnippet(0);

					TableEntry::Ptr e = new TableEntry;
					e->fileName = "onInit()";

					e->doc = doc;
					e->pos = CodeDocument::Position(*doc, location.charNumber);
					e->info = db;

					addIfSameLineDoesntExist(listToUse, e);
				}
				else
				{
					auto& doc = jp->getWatchedFileDocument(j);

					TableEntry::Ptr e = new TableEntry;
					e->fileName = jp->getWatchedFile(j).getFullPathName();

					e->doc = &doc;
					e->pos = CodeDocument::Position(doc, location.charNumber);
					e->info = db;

					addIfSameLineDoesntExist(listToUse, e);
				}
			}
		}

		int numChildren = db->getNumChildElements();

		for (int i = 0; i < numChildren; i++)
			fillRecursive(listToUse, db->getChildElement(i), searchTerm, typeFilters);
	}


	static Array<DebugInformation::Type> getTypeFilterForMode(SearchMode m)
	{
		if (m == SearchMode::NamespaceSearch)
			return { DebugInformation::Type::Namespace };
		if (m == SearchMode::FunctionSearch)
			return { DebugInformation::Type::ExternalFunction, DebugInformation::Type::InlineFunction, DebugInformation::Type::Callback };
		if (m == SearchMode::SymbolSearch)
			return {};
		if (m == SearchMode::VariableSearch)
			return { DebugInformation::Type::Variables, DebugInformation::Type::Constant, DebugInformation::Type::RegisterVariable, DebugInformation::Type::Globals };

		return {};
	}

	void fillForDocument(TableEntry::List& listToUse, SearchMode mode, const String& searchTerm, CodeDocument& t, const String& fileName)
	{
		if (mode == SearchMode::TextSearch)
		{
			auto allText = t.getAllContent();

			String analyseString = allText;

            
			while (searchTerm.isNotEmpty() && analyseString.contains(searchTerm))
			{
				if (threadShouldExit())
					return;

				analyseString = analyseString.fromFirstOccurrenceOf(searchTerm, false, currentOptions.ignoreCase);

				const int index = allText.length() - analyseString.length() - searchTerm.length();

                if(index > 1)
                {
                    auto expanded = allText.substring(index-1, jmin(allText.length(), index + searchTerm.length() + 1));
                    
                    if(!currentOptions.matchesWholeWord(expanded))
                        continue;
                }
                
                
                
                
				JavascriptCodeEditor::CodeRegion newRegion;
				newRegion.setStart(index);
				newRegion.setLength(searchTerm.length());

				TableEntry::Ptr newEntry = new TableEntry();

				newEntry->doc = &t;
				newEntry->pos = CodeDocument::Position(t, index);
				newEntry->fileName = fileName;

				addIfSameLineDoesntExist(listToUse, newEntry);
			}
		}
		if (mode == SearchMode::FileSearch)
		{
			if (searchTerm.isEmpty() || fileName.containsIgnoreCase(searchTerm))
			{
				TableEntry::Ptr e = new TableEntry();
				e->doc = &t;
				e->fileName = fileName;
				e->pos = jp->getLastPosition(t);

				addIfSameLineDoesntExist(listToUse, e);
			}
		}
		else if (mode == SearchMode::SymbolSearch ||
			mode == SearchMode::FunctionSearch ||
			mode == SearchMode::NamespaceSearch ||
			mode == SearchMode::VariableSearch)
		{
			// Only do this once...
			if (fileName != "onInit()")
				return;

			auto typeFilters = getTypeFilterForMode(mode);

			ScopedReadLock sl(jp->getDebugLock());

			auto numObjects = jp->getProviderBase()->getNumDebugObjects();

			for (int i = 0; i < numObjects; i++)
			{
				fillRecursive(listToUse, jp->getProviderBase()->getDebugInformation(i), searchTerm, typeFilters);
			}
		}
	}

	void fillSearchList(TableEntry::List& listToUse, SearchMode mode, const String& searchTerm)
	{
		bool displayAllResults = mode != SearchMode::TextSearch || searchTerm.isNotEmpty();

        if(editor == nullptr)
            editor = CommonEditorFunctions::as(getMainController()->getLastActiveEditor());
        
		if (displayAllResults && editor != nullptr)
		{
			auto thisDoc = &CommonEditorFunctions::getDoc(editor);

			for (int i = 0; i < jp->getNumSnippets(); i++)
			{
				if (threadShouldExit())
					return;

				auto f = jp->getSnippet(i);
				const bool useFile = !currentOptions.currentFileOnly || f == thisDoc;

				if (!useFile)
					continue;

				fillForDocument(listToUse, mode, searchTerm, *f, jp->getSnippet(i)->getCallbackName().toString() + "()");
			}

			for (int i = 0; i < jp->getNumWatchedFiles(); i++)
			{
				auto& doc = jp->getWatchedFileDocument(i);
				const bool useFile = !currentOptions.currentFileOnly || &doc == thisDoc;

				if (!useFile)
					continue;

				fillForDocument(listToUse, mode, searchTerm, doc, jp->getWatchedFile(i).getFullPathName());

				if (threadShouldExit())
					return;
			}
		}
	}

	

	static void addIfSameLineDoesntExist(TableEntry::List& listToUse, TableEntry::Ptr newEntry)
	{
		for (auto e : listToUse)
		{
			if (e->pos.getLineNumber() == newEntry->pos.getLineNumber() &&
				e->fileName == newEntry->fileName)
				return;
		}

		listToUse.add(newEntry);
	}	

	bool keyPressed(const KeyPress& k, Component* o) override
	{
		if (k == KeyPress::upKey || k == KeyPress::downKey ||
			k == KeyPress::pageDownKey || k == KeyPress::pageUpKey)
		{
			if (table->keyPressed(k))
				return true;
		}
		
        if(k == KeyPress::escapeKey)
        {
            if(auto e = getMainController()->getLastActiveEditor())
                e->grabKeyboardFocusAsync();
        }
        
		if (k == KeyPress::returnKey)
		{
			gotoEntry(table->getSelectedRow());
			return false;
		}

		if (k == KeyPress::F5Key)
		{
			selectedRowsChanged(-1);
			return true;
		}

		if (k == KeyPress('c', ModifierKeys::commandModifier, 'c'))
		{
			jassertfalse;
		}

        if(auto root = findParentComponentOfClass<BackendRootWindow>()->getRootFloatingTile())
        {
            if(root->keyPressed(k))
                return true;
        }
        
		return false;
	}
	

	void run() override
	{
		TableEntry::List newEntries;

        fillSearchList(newEntries, currentOptions.currentMode, currentOptions.searchTerm);
        newEntries.swapWith(entries);
	}

	void threadFinished() override
	{
		showStatusMessage(String(entries.size()) + " occurrences found");
		table->updateContent();
		selectedRowsChanged(-1);
		auto search = getTextEditor("searchTerm")->getText();

		if(editor != nullptr)
			CodeReplacer::refreshSelection(editor, search);

		pending = false;
		repaint();
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

	

	void cellClicked(int rowNumber, int columnId, const MouseEvent& e) override
	{
		if (!e.mods.isRightButtonDown())
			gotoEntry(rowNumber);
	}

	void returnKeyPressed(int lastRowSelected) override
	{
		gotoEntry(lastRowSelected);
	}

	void gotoEntry(int rowIndex)
	{
		if (auto entry = entries[rowIndex])
		{
			DebugableObject::Location loc;

			loc.fileName = entry->fileName;
			loc.charNumber = entry->pos.getPosition();

            editor = CommonEditorFunctions::as(getMainController()->getLastActiveEditor());
			
			if (editor != nullptr)
			{
                DebugableObject::Helpers::gotoLocation(editor, jp, loc);
				auto search = getTextEditor("searchTerm")->getText();
				CodeReplacer::refreshSelection(editor, search);
                editor->grabKeyboardFocusAsync();
			}
		}
	}

	void paintCell(Graphics& g, int rowNumber, int columnId, int width, int height, bool /*rowIsSelected*/) override
	{
		auto area = Rectangle<float>({ 2.0f, 0.f, (float)width, (float)height });

		g.setColour(Colours::white.withAlpha(pending ? 0.6f : 0.8f));
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

	int numFixedComponents;

	bool pending = false;

	TableHeaderLookAndFeel laf;
	Component::SafePointer<PopupIncludeEditor::EditorType> editor;
	ScopedPointer<TableListBox> table;
	JavascriptProcessor* jp;

    ScopedPointer<DialogWindowWithBackgroundThread::AdditionalRow> optionsRow;
	ScopedPointer<DialogWindowWithBackgroundThread::AdditionalRow> debugComponentRow;
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

		l.setText(CommonEditorFunctions::getCurrentToken(c->getEditor()), dontSendNotification);
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
			ed->getEditor()->editor.tokenCollection->signalRebuild();
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
		const String selection = CommonEditorFunctions::getCurrentSelection(c);
		const String newText = ScriptRefactoring::createFactoryMethod(selection);
		CommonEditorFunctions::insertTextAtCaret(c, newText);
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
			CommonEditorFunctions::as(c)->editor.tokenCollection->signalRebuild();
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
		const String text = CommonEditorFunctions::getCurrentSelection(c);

		const String newFileName = PresetHandler::getCustomName("Script File", "Enter the file name for the external script file (without .js)");

		if (newFileName.isNotEmpty())
		{
			File scriptDirectory = GET_PROJECT_HANDLER(dynamic_cast<Processor*>(this)).getSubDirectory(ProjectHandler::SubDirectories::Scripts);

			File newFile = scriptDirectory.getChildFile(newFileName + ".js");

			newFile.getParentDirectory().createDirectory();

			if (!newFile.existsAsFile() || PresetHandler::showYesNoWindow("Overwrite existing file", "Do you want to overwrite the file " + newFile.getFullPathName() + "?"))
			{
				newFile.replaceWithText(text);
			}

			String insertStatement = "include(\"" + newFile.getRelativePathFrom(scriptDirectory).replaceCharacter('\\', '/') + "\");" + NewLine();
			CommonEditorFunctions::insertTextAtCaret(c, insertStatement);
		}

		return true;
	}
	case JumpToDefinition:
	{
		auto token = CommonEditorFunctions::getCurrentToken(c);
		const String namespaceId = JavascriptCodeEditor::Helpers::findNamespaceForPosition(CommonEditorFunctions::getCaretPos(c));
		jumpToDefinition(token, namespaceId);

		return true;
	}
	case FindAllOccurences:
	{
		auto finder = new ReferenceFinder(CommonEditorFunctions::as(c), this);
		finder->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(c));
		finder->grabKeyboardFocus();
		return true;
	}
	case SearchAndReplace:
	{
		auto replacer = new CodeReplacer(CommonEditorFunctions::as(c));
		replacer->setModalBaseWindowComponent(GET_BACKEND_ROOT_WINDOW(c));
		replacer->getTextEditor("search")->grabKeyboardFocus();
		return true;
	}
	default:
		return false;
	}
}

} // namespace hise
