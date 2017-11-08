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
#ifndef SCRIPTINGCODEEDITOR_H_INCLUDED
#define SCRIPTINGCODEEDITOR_H_INCLUDED

namespace hise { using namespace juce;


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

	enum ContextActions
	{
		SaveScriptFile = 101,
		LoadScriptFile,
		SaveScriptClipboard,
		LoadScriptClipboard,
		JumpToDefinition,
		SearchReplace,
		AddCodeBookmark,
		CreateUiFactoryMethod,
		AddMissingCaseStatements,
		OpenExternalFile,
		OpenInPopup,
		MoveToExternalFile,
		InsertExternalFile,
		ExportAsCompressedScript,
		ImportCompressedScript
	};


	typedef Range<int> CodeRegion;

	// ================================================================================================================

	JavascriptCodeEditor(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p, const Identifier& snippetId_);
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
	void closeAutoCompleteNew(String returnString);

	void selectLineAfterDefinition(Identifier identifier);
	bool selectJSONTag(const Identifier &identifier);
	bool componentIsDefinedWithFactoryMethod(const Identifier& identifier);
	String createNewDefinitionWithFactoryMethod(const String &oldId, const String &newId, int newX, int newY);

	void createMissingCaseStatementsForComponents();

	void paintOverChildren(Graphics& g);

	void rebuildHighlightedSelection(Array<CodeRegion> &newArray) { highlightedSelection.swapWith(newArray); repaint(); }

	bool keyPressed(const KeyPress& k) override;
	void handleReturnKey() override;;
	void handleEscapeKey() override;
	void insertTextAtCaret(const String& newText) override;

	void mouseDown(const MouseEvent& e) override;

	// ================================================================================================================


	struct Helpers
	{
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
			return CharacterFunctions::isLetterOrDigit(c) || c == '.' || c == '_' || c == '[' || c == ']';
		}

		static Range<int> getJSONTag(const CodeDocument& doc, const Identifier& id);

		static CodeDocument::Position getPositionAfterDefinition(const CodeDocument& doc, Identifier id);

		static Range<int> getFunctionParameterTextRange(CodeDocument::Position pos);

		static CodeDocument* gotoAndReturnDocumentWithDefinition(Processor* p, DebugableObject* object);

		static String findNamespaceForPosition(CodeDocument::Position pos);

		static void applyChangesFromActiveEditor(JavascriptProcessor* p);

		static JavascriptCodeEditor* getActiveEditor(JavascriptProcessor* p);
		static JavascriptCodeEditor* getActiveEditor(Processor* p);
	};


private:

	class AutoCompletePopup;

	// ================================================================================================================

	Array<CodeRegion> highlightedSelection;
	
	Range<int> getCurrentTokenRange() const;
	bool isNothingSelected() const;
	void handleDoubleCharacter(const KeyPress &k, char openCharacter, char closeCharacter);

	Component::SafePointer<Component> currentModalWindow;

	ScopedPointer<AutoCompletePopup> currentPopup;

	JavascriptProcessor* scriptProcessor;
	WeakReference<Processor> processor;

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

class DebugConsoleTextEditor : public TextEditor,
	public TextEditor::Listener,
	public GlobalScriptCompileListener
{
public:

	DebugConsoleTextEditor(const String& name, Processor* p);;

	~DebugConsoleTextEditor();

	void scriptWasCompiled(JavascriptProcessor *jp);

	bool keyPressed(const KeyPress& k) override;

	void mouseDown(const MouseEvent& e);
	void mouseDoubleClick(const MouseEvent& e) override;

	void addToHistory(const String& s);

	void textEditorReturnKeyPressed(TextEditor& /*t*/);

private:

	LookAndFeel_V2 laf2;

	WeakReference<Processor> processor;

	StringArray history;
	int currentHistoryIndex = 0;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DebugConsoleTextEditor)
};


class CodeEditorWrapper: public Component,
						 public Timer
{
public:

	// ================================================================================================================

	CodeEditorWrapper(CodeDocument &document, CodeTokeniser *codeTokeniser, JavascriptProcessor *p, const Identifier& snippetId);
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


} // namespace hise
#endif  // SCRIPTINGCODEEDITOR_H_INCLUDED
