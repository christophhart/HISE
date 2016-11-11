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
#ifndef SCRIPTINGCODEEDITOR_H_INCLUDED
#define SCRIPTINGCODEEDITOR_H_INCLUDED


class JavascriptCodeEditor;

class PopupIncludeEditor : public Component,
						   public Timer
{
public:

	// ================================================================================================================

	PopupIncludeEditor(JavascriptProcessor *s, const File &fileToEdit);
	~PopupIncludeEditor();

	void timerCallback();
	bool keyPressed(const KeyPress& key) override;
	void resized() override;;

	void gotoChar(int character);


private:

	int fontSize;

	ScopedPointer<JavascriptTokeniser> tokeniser;
	ScopedPointer < JavascriptCodeEditor > editor;
	ScopedPointer <CodeDocument> doc;
	
	ScopedPointer<Label> resultLabel;	

	JavascriptProcessor *sp;
	File file;

	bool lastCompileOk;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupIncludeEditor);

	// ================================================================================================================
};

class PopupIncludeEditorWindow: public DocumentWindow
{
public:

	// ================================================================================================================

	PopupIncludeEditorWindow(File f, JavascriptProcessor *s);
    
    File getFile() const {return file;};
	void paint(Graphics &g) override;
	bool keyPressed(const KeyPress& key);;
	void closeButtonPressed() override;;

	void gotoChar(int character);

private:

    ScopedPointer<PopupIncludeEditor> editor;
    const File file;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PopupIncludeEditorWindow);

	// ================================================================================================================
};


/** A subclass of CodeEditorComponent which improves working with Javascript scripts.
*	@ingroup scripting
*
*	It tokenises the code using Javascript keywords and dedicated HI keywords from the Scripting API
*	and applies some neat auto-intending features.
*/
class JavascriptCodeEditor: public CodeEditorComponent,
							public SettableTooltipClient,
							public Timer,
							public DragAndDropTarget,
							public CopyPasteTarget,
                            public SafeChangeListener
{
public:

	// ================================================================================================================

	enum DragState
	{
		Virgin = 0,
		JSONFound,
		NoJSONFound
	};

	JavascriptCodeEditor(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p);;
	virtual ~JavascriptCodeEditor();

	// ================================================================================================================

    void changeListenerCallback(SafeChangeBroadcaster *) override;
	void timerCallback() override;

    void focusGained(FocusChangeType ) override;
	void focusLost(FocusChangeType t) override;
    
	virtual String getObjectTypeName() override;
	virtual void copyAction();
	virtual void pasteAction();;

	bool isInterestedInDragSource (const SourceDetails &dragSourceDetails) override;
	void itemDropped (const SourceDetails &dragSourceDetails) override;
	void itemDragEnter(const SourceDetails &dragSourceDetails) override;;
	void itemDragMove(const SourceDetails &dragSourceDetails);

	

    void addPopupMenuItems(PopupMenu &m, const MouseEvent *e) override;
    void performPopupMenuAction(int menuId) override;


	void showAutoCompleteNew();
	void closeAutoCompleteNew(const String returnString);

	void selectLineAfterDefinition(Identifier identifier);
	bool selectJSONTag(const Identifier &identifier);

	void paintOverChildren(Graphics& g);

	bool keyPressed(const KeyPress& k) override;
	void handleReturnKey() override;;
	void handleEscapeKey() override;
	void insertTextAtCaret(const String& newText) override;

	// ================================================================================================================

	class AutoCompletePopup : public ListBoxModel,
							  public Component
	{

	public:

		// ================================================================================================================

		class AllToTheEditorTraverser : public KeyboardFocusTraverser
		{
		public:
			AllToTheEditorTraverser(JavascriptCodeEditor *editor_): editor(editor_) {};

			virtual Component* getNextComponent(Component* /*current*/) { return editor; }
			virtual Component* getPreviousComponent(Component* /*current*/) { return editor; }
			virtual Component* getDefaultComponent(Component* /*parentComponent*/) { return editor; }

			JavascriptCodeEditor *editor;
		};

		// ================================================================================================================

		AutoCompletePopup(int fontHeight_, JavascriptCodeEditor* editor_, Range<int> tokenRange_, const String &tokenText);
		~AutoCompletePopup();

		void createVariableRows();
		void createApiRows(const ValueTree &apiTree);
		void createObjectPropertyRows(const ValueTree &apiTree, const String &tokenText);

		void addCustomEntries(const Identifier &objectId, const ValueTree &apiTree);
		void addApiConstants(const ApiClass* apiClass, const Identifier &objectId);
		void addApiMethods(const ValueTree &classTree, const Identifier &objectId);

		KeyboardFocusTraverser* createFocusTraverser() override;

		int getNumRows() override;
		void paintListBoxItem(int rowNumber, Graphics &g, int width, int height, bool rowIsSelected) override;
		void listBoxItemClicked(int row, const MouseEvent &) override;
		void listBoxItemDoubleClicked(int row, const MouseEvent &) override;

		bool handleEditorKeyPress(const KeyPress& k);

		void paint(Graphics& g) override;
		void resized();

		void selectRowInfo(int rowIndex);
		void rebuildVisibleItems(const String &selection);

		bool escapeKeyHandled = false;

		// ================================================================================================================

	private:

		SharedResourcePointer<ApiHelpers::Api> api;

		// ================================================================================================================

		struct RowInfo
		{
			enum class Type
			{
				ApiClass = (int)DebugInformation::Type::numTypes,
				ApiMethod,
				numTypes
			};

			bool matchesSelection(const String &selection)
			{
				return name.containsIgnoreCase(selection);
			}

			AttributedString description;
			String codeToInsert, name, typeName, value;
			int type;
		};

		// ================================================================================================================

		class InfoBox : public Component
		{
		public:

			void setInfo(RowInfo *newInfo);
			void paint(Graphics &g);

		private:

			AttributedString infoText;
			RowInfo *currentInfo = nullptr;
		};

		// ================================================================================================================

		OwnedArray<RowInfo> allInfo;
		Array<RowInfo*> visibleInfo;
		StringArray names;
		int fontHeight;
		int currentlySelectedBox = -1;
		ScopedPointer<InfoBox> infoBox;
		ScopedPointer<ListBox> listbox;
		JavascriptProcessor *sp;
		Range<int> tokenRange;
		JavascriptCodeEditor *editor;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoCompletePopup);

		// ================================================================================================================
	};

private:

	// ================================================================================================================

	
	Range<int> getCurrentTokenRange() const;
	bool isNothingSelected() const;
	void handleDoubleCharacter(const KeyPress &k, char openCharacter, char closeCharacter);

	struct Helpers
	{
		static String getLeadingWhitespace(String line);
		static int getBraceCount(String::CharPointerType line);
		static bool getIndentForCurrentBlock(CodeDocument::Position pos, const String& tab,
											 String& blockIndent, String& lastLineIndent);

		static char getCharacterAtCaret(CodeDocument::Position pos, bool beforeCaret = false);
		
		static Range<int> getFunctionParameterTextRange(CodeDocument::Position pos);
	};

	Component::SafePointer<Component> currentModalWindow;

	ScopedPointer<AutoCompletePopup> currentPopup;

	JavascriptProcessor *scriptProcessor;
	Processor *processor;

	PopupMenu m;
	

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
	
};


class CodeEditorWrapper: public Component,
						 public Timer
{
public:

	// ================================================================================================================

	CodeEditorWrapper(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p);
	virtual ~CodeEditorWrapper();

	ScopedPointer<JavascriptCodeEditor> editor;

	void resized() override;;
	void timerCallback();

	void mouseDown(const MouseEvent &m) override;;
	void mouseUp(const MouseEvent &) override;;

	int currentHeight;

	// ================================================================================================================

private:

	ScopedPointer<ResizableEdgeComponent> dragger;

	ComponentBoundsConstrainer restrainer;

	LookAndFeel_V2 laf2;
	
	// ================================================================================================================

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CodeEditorWrapper);
};



#endif  // SCRIPTINGCODEEDITOR_H_INCLUDED
