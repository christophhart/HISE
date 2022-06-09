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

#pragma once

namespace hise {
using namespace juce;





/** A subclass of CodeEditorComponent which improves working with Javascript scripts.
*	@ingroup scripting
*
*	It tokenises the code using Javascript keywords and dedicated HI keywords from the Scripting API
*	and applies some neat auto-intending features.
*/
class JavascriptCodeEditor : public CodeEditorComponent,
	public SettableTooltipClient,
	public Timer,
	public DragAndDropTarget,
	public CopyPasteTarget,
	public SafeChangeListener,
	public ApiProviderBase::ApiComponentBase,
	public ComponentWithDocumentation
{
public:

	class AutoCompletePopup : public ListBoxModel,
		public Component,
		public ApiProviderBase::ApiComponentBase,
		public ComponentWithDocumentation
	{

	public:

		// ================================================================================================================

		class AllToTheEditorTraverser : public KeyboardFocusTraverser
		{
		public:
			AllToTheEditorTraverser(JavascriptCodeEditor *editor_) : editor(editor_) {};

			virtual Component* getNextComponent(Component* /*current*/)
			{
				return editor;
			}
			virtual Component* getPreviousComponent(Component* /*current*/)
			{
				return editor;
			}
			virtual Component* getDefaultComponent(Component* /*parentComponent*/)
			{
				return editor;
			}

			JavascriptCodeEditor *editor;
		};

		// ================================================================================================================

		AutoCompletePopup(int fontHeight_, JavascriptCodeEditor* editor, ApiProviderBase::Holder* holder, const String &tokenText);
		~AutoCompletePopup();

		void rebuild(const String& tokenText);

		void createRecursive(DebugInformationBase::Ptr p);

		void createVariableRows();
		void createApiRows(const ValueTree &apiTree, const String& tokenText);
		void createObjectPropertyRows(const ValueTree &apiTree, const String &tokenText);

		//void addApiConstants(const ApiClassBase* apiClass, const Identifier &objectId);
		//void addApiMethods(const ValueTree &classTree, const Identifier &objectId);

		void addRowsFromObject(DebugableObjectBase* obj, const String& originalToken, const ValueTree& classTree);

		void addRowFromApiClass(const ValueTree classTree, const String& originalToken, bool isTemplate=false);

		std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser();

		MarkdownLink getLink() const override;


		int getNumRows() override;
		void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
		void listBoxItemClicked(int row, const MouseEvent &) override;
		void listBoxItemDoubleClicked(int row, const MouseEvent &) override;

		bool handleEditorKeyPress(const KeyPress& k);

		void paint(Graphics& g) override;
		void resized();

		void providerCleared() override
        {
            visibleInfo.clear();
            allInfo.clear();
        }

		void selectRowInfo(int rowIndex);
		void rebuildVisibleItems(const String &selection);

		bool escapeKeyHandled = false;

		// ================================================================================================================

	private:

		ValueTree apiTree;

		String lastText;

		// ================================================================================================================

		struct RowInfo
		{
			RowInfo() = default;
			RowInfo(DebugInformationBase::Ptr p);

            using WeakPtr = WeakReference<RowInfo>;
            using List = Array<WeakPtr>;
            
			enum class Type
			{
				ApiClass = 40,
				ApiMethod,
				numTypes
			};

			bool matchesSelection(const String &selection)
			{
				return name.containsIgnoreCase(selection);
			}

			AttributedString description;
			String codeToInsert, name, typeName, value, category;
			Identifier classId;
			int type;
            
            JUCE_DECLARE_WEAK_REFERENCEABLE(RowInfo);
		};

		// ================================================================================================================

		class InfoBox : public Component
		{
		public:

			void setInfo(RowInfo *newInfo);
			void paint(Graphics &g);

		private:

			AttributedString infoText;
            RowInfo::WeakPtr currentInfo = nullptr;
		};

		// ================================================================================================================

		OwnedArray<RowInfo> allInfo;
        RowInfo::List visibleInfo;
		StringArray names;
		int fontHeight;
		int currentlySelectedBox = -1;
		ScopedPointer<InfoBox> infoBox;
		ScopedPointer<ListBox> listbox;
		ScopedPointer<Button> helpButton;

		Component::SafePointer<JavascriptCodeEditor> editor;

		MarkdownLink currentLink;

		bool hasExtendedHelp = false;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoCompletePopup);

		// ================================================================================================================
	};

	// ================================================================================================================

	struct AutocompleteTemplate
	{
		String token;
		String classId;
	};

	enum DragState
	{
		Virgin = 0,
		JSONFound,
		NoJSONFound
	};

	

	typedef Range<int> CodeRegion;

	// ================================================================================================================

	JavascriptCodeEditor(CodeDocument &document, CodeTokeniser *codeTokeniser, ApiProviderBase::Holder *p, const Identifier& snippetId_);
	virtual ~JavascriptCodeEditor();

	// ================================================================================================================

	void changeListenerCallback(SafeChangeBroadcaster *) override;
	void timerCallback() override;

	void focusGained(FocusChangeType) override;
	void focusLost(FocusChangeType t) override;

	virtual String getObjectTypeName() override;
	virtual void copyAction();
	virtual void pasteAction();;

	bool isInterestedInDragSource(const SourceDetails &dragSourceDetails) override;
	void itemDropped(const SourceDetails &dragSourceDetails) override;
	void itemDragEnter(const SourceDetails &dragSourceDetails) override;;
	void itemDragMove(const SourceDetails &dragSourceDetails);

	void addPopupMenuItems(PopupMenu &m, const MouseEvent *e) override;
	void performPopupMenuAction(int menuId) override;

	void addAutocompleteTemplate(const String& expression, const String& classId)
	{
		autocompleteTemplates.add({ expression, classId });
	}

	String matchesAutocompleteTemplate(const String& token) const;

	String getCurrentToken() const;

	String getTokenForPosition(const CodeDocument::Position& pos) const;

	void showAutoCompleteNew();
	void closeAutoCompleteNew(String returnString);

	void selectLineAfterDefinition(Identifier identifier);
	bool selectJSONTag(const Identifier &identifier);
	bool componentIsDefinedWithFactoryMethod(const Identifier& identifier);
	String createNewDefinitionWithFactoryMethod(const String &oldId, const String &newId, int newX, int newY);

	void paintOverChildren(Graphics& g);

	void rebuildHighlightedSelection(Array<CodeRegion> &newArray) { highlightedSelection.swapWith(newArray); repaint(); }

	MarkdownLink getLink() const override;

	bool keyPressed(const KeyPress& k) override;
	void handleReturnKey() override;;
	void handleEscapeKey() override;
	void insertTextAtCaret(const String& newText) override;

	void mouseDown(const MouseEvent& e) override;

	// ================================================================================================================


	struct Helpers
	{
		static int getOffsetToFirstToken(const String& content)
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

#if 0
		static String getRegexForUIDefinition(ScriptComponent* sc)
		{
			const String regexMonster = "(Content\\.add\\w+\\s*\\(\\s*\\\"(" + sc->getName().toString() +
				")\\\"\\s*,\\s*)(-?\\d+)(\\s*,\\s*)(-?\\d+)(\\s*\\);)|([cC]reate\\w*\\s*\\(\\s*\\\"(" + sc->getName().toString() +
				")\\\"\\s*,\\s*)(-?\\d+)(\\s*,\\s*)(-?\\d+)(\\s*.*\\);)";

			return regexMonster;
		}

		static CodeDocument* getPositionOfUIDefinition(ScriptComponent* sc, CodeDocument::Position& position)
		{
			auto regexMonster = getRegexForUIDefinition(sc);

			auto p = sc->getScriptProcessor();

			CodeDocument* doc = gotoAndReturnDocumentWithDefinition(dynamic_cast<Processor*>(p), sc);

			if (doc == nullptr)
			{
				jassertfalse;
				return nullptr;
			}

			String allText = doc->getAllContent();

			StringArray matches = RegexFunctions::getFirstMatch(regexMonster, allText);

			const bool isContentDefinition = matches[1].isNotEmpty();
			const bool isInlineDefinition = matches[7].isNotEmpty();

			if ((isContentDefinition || isInlineDefinition) && matches.size() > 12)
			{
				const String oldDefinition = matches[0]; // not the whole line

				const int charIndex = allText.indexOf(oldDefinition);

				position = CodeDocument::Position(*doc, charIndex);
				position.setPositionMaintained(true);
				return doc;
			}

			jassertfalse;
			return nullptr;
		}

		static void changeXYOfUIDefinition(ScriptComponent* sc, CodeDocument* doc, const CodeDocument::Position& definitionLine, int newX, int newY)
		{
			auto regexMonster = getRegexForUIDefinition(sc);

			auto oldLine = definitionLine.getLineText();

			StringArray matches = RegexFunctions::getFirstMatch(regexMonster, oldLine);

			const bool isContentDefinition = matches[1].isNotEmpty();
			const bool isInlineDefinition = matches[7].isNotEmpty();

			if ((isContentDefinition || isInlineDefinition) && matches.size() > 12)
			{
				const String oldDefinition = matches[0];

				jassert(oldLine.contains(oldDefinition));

				String replaceDefinition;

				if (isContentDefinition)
				{
					replaceDefinition << matches[1] << String(newX) << matches[4] << String(newY) << matches[6];
				}
				else
				{

					replaceDefinition << matches[7] << String(newX) << matches[10] << String(newY) << matches[12];
				}

				CodeDocument::Position startPos(*doc, 0);
				CodeDocument::Position endPos(*doc, 0);

				doc->findLineContaining(definitionLine, startPos, endPos);

				auto replaceLine = oldLine.replace(oldDefinition, replaceDefinition);

				doc->replaceSection(startPos.getPosition(), endPos.getPosition(), replaceLine);
			}
		}
#endif


		static String getLeadingWhitespace(String line);
		static int getBraceCount(String::CharPointerType line);
		static bool getIndentForCurrentBlock(CodeDocument::Position pos, const String& tab,
			String& blockIndent, String& lastLineIndent);

		static char getCharacterAtCaret(CodeDocument::Position pos, bool beforeCaret = false);

		static void findAdvancedTokenRange(const CodeDocument::Position& pos, CodeDocument::Position& start, CodeDocument::Position& end)
		{
			end = pos;
			while (isAdvancedTokenCharacter(end.getCharacter()))
				end.moveBy(1);

			start = end;
			while (start.getIndexInLine() > 0
				&& isAdvancedTokenCharacter(start.movedBy(-1).getCharacter()))
				start.moveBy(-1);
		}

		static bool isAdvancedTokenCharacter(juce_wchar c)
		{
			return CharacterFunctions::isLetterOrDigit(c) || c == '.' || c == '_' || c == '[' || c == ']' || c == '"';
		}

		static Range<int> getJSONTag(const CodeDocument& doc, const Identifier& id);

		static CodeDocument::Position getPositionAfterDefinition(const CodeDocument& doc, Identifier id);

		static Range<int> getFunctionParameterTextRange(CodeDocument::Position pos);

		

		static String findNamespaceForPosition(CodeDocument::Position pos);

		
	};

	void mouseMove(const MouseEvent& e) override
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


private:

	struct HoverManager : public Timer
	{
		HoverManager(JavascriptCodeEditor& p) :
			parent(p)
		{};

		JavascriptCodeEditor& parent;

		void timerCallback() override
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

		Point<int> position;
		String lastToken;
	} hoverManager;

	String hoverText;
	Point<int> hoverPosition;

	Array<AutocompleteTemplate> autocompleteTemplates;

	// ================================================================================================================

	Array<CodeRegion> highlightedSelection;

	Range<int> getCurrentTokenRange() const;
	bool isNothingSelected() const;
	void handleDoubleCharacter(const KeyPress &k, char openCharacter, char closeCharacter);

	Component::SafePointer<Component> currentModalWindow;

	ScopedPointer<AutoCompletePopup> currentPopup;

	PopupMenu m;

	const Identifier snippetId;

	PopupLookAndFeel plaf;

	DragState positionFound;

	const int bookmarkOffset = 0xffff;

	struct Bookmarks
	{
		Bookmarks() :
			title(""),
			line(-1)
		{};

		Bookmarks(const String& lineText, int lineNumber)
		{
			title = lineText.removeCharacters("/!=-_");
			line = lineNumber;
		};

		String title;
		int line;
	};

	Array<Bookmarks> bookmarks;

	void increaseMultiSelectionForCurrentToken();
};

struct CommonEditorFunctions
{
#if HISE_USE_NEW_CODE_EDITOR
	using EditorType = mcl::FullEditor;
#else
	using EditorType = JavascriptCodeEditor;
#endif

	static EditorType* as(Component* c);
	static CodeDocument::Position getCaretPos(Component* c);
	static CodeDocument& getDoc(Component* c);
	static String getCurrentToken(Component* c);
	static void insertTextAtCaret(Component* c, const String& t);
	static String getCurrentSelection(Component* c);
	static void moveCaretTo(Component* c, CodeDocument::Position& pos, bool select);
};


} // namespace hise


