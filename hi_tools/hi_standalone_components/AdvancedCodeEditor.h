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
		static int getOffsetToFirstToken(const String& content);
		static String getLeadingWhitespace(String line);
		static int getBraceCount(String::CharPointerType line);
		static bool getIndentForCurrentBlock(CodeDocument::Position pos, const String& tab,
			String& blockIndent, String& lastLineIndent);
		static char getCharacterAtCaret(CodeDocument::Position pos, bool beforeCaret = false);
		static void findAdvancedTokenRange(const CodeDocument::Position& pos, CodeDocument::Position& start, CodeDocument::Position& end);
		static bool isAdvancedTokenCharacter(juce_wchar c);
		static Range<int> getJSONTag(const CodeDocument& doc, const Identifier& id);
		static CodeDocument::Position getPositionAfterDefinition(const CodeDocument& doc, Identifier id);
		static Range<int> getFunctionParameterTextRange(CodeDocument::Position pos);
		static String findNamespaceForPosition(CodeDocument::Position pos);
	};

	void mouseMove(const MouseEvent& e) override;

private:

	struct HoverManager : public Timer
	{
		HoverManager(JavascriptCodeEditor& p) :
			parent(p)
		{};

		JavascriptCodeEditor& parent;

		void timerCallback() override;

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


