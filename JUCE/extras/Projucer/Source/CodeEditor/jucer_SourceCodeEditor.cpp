/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2020 - Raw Material Software Limited

   JUCE is an open source library subject to commercial or open-source
   licensing.

   By using JUCE, you agree to the terms of both the JUCE 6 End-User License
   Agreement and JUCE Privacy Policy (both effective as of the 16th June 2020).

   End User License Agreement: www.juce.com/juce-6-licence
   Privacy Policy: www.juce.com/juce-privacy-policy

   Or: You may also use this code under the terms of the GPL v3 (see
   www.gnu.org/licenses).

   JUCE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES, WHETHER
   EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR PURPOSE, ARE
   DISCLAIMED.

  ==============================================================================
*/

#include "../Application/jucer_Headers.h"
#include "jucer_SourceCodeEditor.h"
#include "../Application/jucer_Application.h"

//==============================================================================
SourceCodeDocument::SourceCodeDocument (Project* p, const File& f)
    : modDetector (f), project (p)
{
}

CodeDocument& SourceCodeDocument::getCodeDocument()
{
    if (codeDoc == nullptr)
    {
        codeDoc.reset (new CodeDocument());
        reloadInternal();
        codeDoc->clearUndoHistory();
    }

    return *codeDoc;
}

Component* SourceCodeDocument::createEditor()
{
    auto* e = new SourceCodeEditor (this, getCodeDocument());
    applyLastState (*(e->editor));
    return e;
}

void SourceCodeDocument::reloadFromFile()
{
    getCodeDocument();
    reloadInternal();
}

void SourceCodeDocument::reloadInternal()
{
    jassert (codeDoc != nullptr);
    modDetector.updateHash();

    auto fileContent = getFile().loadFileAsString();

    auto lineFeed = getLineFeedForFile (fileContent);

    if (lineFeed.isEmpty())
    {
        if (project != nullptr)
            lineFeed = project->getProjectLineFeed();
        else
            lineFeed = "\r\n";
    }

    codeDoc->setNewLineCharacters (lineFeed);

    codeDoc->applyChanges (fileContent);
    codeDoc->setSavePoint();
}

static bool writeCodeDocToFile (const File& file, CodeDocument& doc)
{
    TemporaryFile temp (file);

    {
        FileOutputStream fo (temp.getFile());

        if (! (fo.openedOk() && doc.writeToStream (fo)))
            return false;
    }

    return temp.overwriteTargetFileWithTemporary();
}

bool SourceCodeDocument::save()
{
    if (writeCodeDocToFile (getFile(), getCodeDocument()))
    {
        getCodeDocument().setSavePoint();
        modDetector.updateHash();
        return true;
    }

    return false;
}

bool SourceCodeDocument::saveAs()
{
    FileChooser fc (TRANS("Save As..."), getFile(), "*");

    if (! fc.browseForFileToSave (true))
        return true;

    return writeCodeDocToFile (fc.getResult(), getCodeDocument());
}

void SourceCodeDocument::updateLastState (CodeEditorComponent& editor)
{
    lastState.reset (new CodeEditorComponent::State (editor));
}

void SourceCodeDocument::applyLastState (CodeEditorComponent& editor) const
{
    if (lastState != nullptr)
        lastState->restoreState (editor);
}

//==============================================================================
SourceCodeEditor::SourceCodeEditor (OpenDocumentManager::Document* doc, CodeDocument& codeDocument)
    : DocumentEditorComponent (doc)
{
    GenericCodeEditorComponent* ed = nullptr;
    auto file = document->getFile();

    if (fileNeedsCppSyntaxHighlighting (file))
    {
        ed = new CppCodeEditorComponent (file, codeDocument);
    }
    else
    {
        CodeTokeniser* tokeniser = nullptr;

        if (file.hasFileExtension ("xml;svg"))
        {
            static XmlTokeniser xmlTokeniser;
            tokeniser = &xmlTokeniser;
        }

        if (file.hasFileExtension ("lua"))
        {
            static LuaTokeniser luaTokeniser;
            tokeniser = &luaTokeniser;
        }

        ed = new GenericCodeEditorComponent (file, codeDocument, tokeniser);
    }

    setEditor (ed);
}

SourceCodeEditor::SourceCodeEditor (OpenDocumentManager::Document* doc, GenericCodeEditorComponent* ed)
    : DocumentEditorComponent (doc)
{
    setEditor (ed);
}

SourceCodeEditor::~SourceCodeEditor()
{
    if (editor != nullptr)
        editor->getDocument().removeListener (this);

    getAppSettings().appearance.settings.removeListener (this);

    if (auto* doc = dynamic_cast<SourceCodeDocument*> (getDocument()))
        doc->updateLastState (*editor);
}

void SourceCodeEditor::setEditor (GenericCodeEditorComponent* newEditor)
{
    if (editor != nullptr)
        editor->getDocument().removeListener (this);

    editor.reset (newEditor);
    addAndMakeVisible (newEditor);

    editor->setFont (AppearanceSettings::getDefaultCodeFont());
    editor->setTabSize (4, true);

    updateColourScheme();
    getAppSettings().appearance.settings.addListener (this);

    editor->getDocument().addListener (this);
}

void SourceCodeEditor::scrollToKeepRangeOnScreen (Range<int> range)
{
    auto space = jmin (10, editor->getNumLinesOnScreen() / 3);
    const CodeDocument::Position start (editor->getDocument(), range.getStart());
    const CodeDocument::Position end   (editor->getDocument(), range.getEnd());

    editor->scrollToKeepLinesOnScreen ({ start.getLineNumber() - space, end.getLineNumber() + space });
}

void SourceCodeEditor::highlight (Range<int> range, bool cursorAtStart)
{
    scrollToKeepRangeOnScreen (range);

    if (cursorAtStart)
    {
        editor->moveCaretTo (CodeDocument::Position (editor->getDocument(), range.getEnd()),   false);
        editor->moveCaretTo (CodeDocument::Position (editor->getDocument(), range.getStart()), true);
    }
    else
    {
        editor->setHighlightedRegion (range);
    }
}

void SourceCodeEditor::resized()
{
    editor->setBounds (getLocalBounds());
}

void SourceCodeEditor::updateColourScheme()
{
    getAppSettings().appearance.applyToCodeEditor (*editor);
}

void SourceCodeEditor::checkSaveState()
{
    setEditedState (getDocument()->needsSaving());
}

void SourceCodeEditor::lookAndFeelChanged()
{
    updateColourScheme();
}

void SourceCodeEditor::valueTreePropertyChanged (ValueTree&, const Identifier&)   { updateColourScheme(); }
void SourceCodeEditor::valueTreeChildAdded (ValueTree&, ValueTree&)               { updateColourScheme(); }
void SourceCodeEditor::valueTreeChildRemoved (ValueTree&, ValueTree&, int)        { updateColourScheme(); }
void SourceCodeEditor::valueTreeChildOrderChanged (ValueTree&, int, int)          { updateColourScheme(); }
void SourceCodeEditor::valueTreeParentChanged (ValueTree&)                        { updateColourScheme(); }
void SourceCodeEditor::valueTreeRedirected (ValueTree&)                           { updateColourScheme(); }

void SourceCodeEditor::codeDocumentTextInserted (const String&, int)              { checkSaveState(); }
void SourceCodeEditor::codeDocumentTextDeleted (int, int)                         { checkSaveState(); }

//==============================================================================
GenericCodeEditorComponent::GenericCodeEditorComponent (const File& f, CodeDocument& codeDocument,
                                                        CodeTokeniser* tokeniser)
   : CodeEditorComponent (codeDocument, tokeniser), file (f)
{
    setScrollbarThickness (6);
    setCommandManager (&ProjucerApplication::getCommandManager());
}

GenericCodeEditorComponent::~GenericCodeEditorComponent() {}

enum
{
    showInFinderID = 0x2fe821e3,
    insertComponentID = 0x2fe821e4
};

void GenericCodeEditorComponent::addPopupMenuItems (PopupMenu& menu, const MouseEvent* e)
{
    menu.addItem (showInFinderID,
                 #if JUCE_MAC
                  "Reveal " + file.getFileName() + " in Finder");
                 #else
                  "Reveal " + file.getFileName() + " in Explorer");
                 #endif
    menu.addSeparator();

    CodeEditorComponent::addPopupMenuItems (menu, e);
}

void GenericCodeEditorComponent::performPopupMenuAction (int menuItemID)
{
    if (menuItemID == showInFinderID)
        file.revealToUser();
    else
        CodeEditorComponent::performPopupMenuAction (menuItemID);
}

void GenericCodeEditorComponent::getAllCommands (Array <CommandID>& commands)
{
    CodeEditorComponent::getAllCommands (commands);

    const CommandID ids[] = { CommandIDs::showFindPanel,
                              CommandIDs::findSelection,
                              CommandIDs::findNext,
                              CommandIDs::findPrevious };

    commands.addArray (ids, numElementsInArray (ids));
}

void GenericCodeEditorComponent::getCommandInfo (const CommandID commandID, ApplicationCommandInfo& result)
{
    auto anythingSelected = isHighlightActive();

    switch (commandID)
    {
        case CommandIDs::showFindPanel:
            result.setInfo (TRANS ("Find"), TRANS ("Searches for text in the current document."), "Editing", 0);
            result.defaultKeypresses.add (KeyPress ('f', ModifierKeys::commandModifier, 0));
            break;

        case CommandIDs::findSelection:
            result.setInfo (TRANS ("Find Selection"), TRANS ("Searches for the currently selected text."), "Editing", 0);
            result.setActive (anythingSelected);
            result.defaultKeypresses.add (KeyPress ('l', ModifierKeys::commandModifier, 0));
            break;

        case CommandIDs::findNext:
            result.setInfo (TRANS ("Find Next"), TRANS ("Searches for the next occurrence of the current search-term."), "Editing", 0);
            result.defaultKeypresses.add (KeyPress ('g', ModifierKeys::commandModifier, 0));
            break;

        case CommandIDs::findPrevious:
            result.setInfo (TRANS ("Find Previous"), TRANS ("Searches for the previous occurrence of the current search-term."), "Editing", 0);
            result.defaultKeypresses.add (KeyPress ('g', ModifierKeys::commandModifier | ModifierKeys::shiftModifier, 0));
            result.defaultKeypresses.add (KeyPress ('d', ModifierKeys::commandModifier, 0));
            break;

        default:
            CodeEditorComponent::getCommandInfo (commandID, result);
            break;
    }
}

bool GenericCodeEditorComponent::perform (const InvocationInfo& info)
{
    switch (info.commandID)
    {
        case CommandIDs::showFindPanel:     showFindPanel();         return true;
        case CommandIDs::findSelection:     findSelection();         return true;
        case CommandIDs::findNext:          findNext (true, true);   return true;
        case CommandIDs::findPrevious:      findNext (false, false); return true;
        default:                            break;
    }

    return CodeEditorComponent::perform (info);
}

void GenericCodeEditorComponent::addListener (GenericCodeEditorComponent::Listener* listener)
{
    listeners.add (listener);
}

void GenericCodeEditorComponent::removeListener (GenericCodeEditorComponent::Listener* listener)
{
    listeners.remove (listener);
}

//==============================================================================
class GenericCodeEditorComponent::FindPanel  : public Component
{
public:
    FindPanel()
    {
        editor.setColour (CaretComponent::caretColourId, Colours::black);

        addAndMakeVisible (editor);
        label.setColour (Label::textColourId, Colours::white);
        label.attachToComponent (&editor, false);

        addAndMakeVisible (caseButton);
        caseButton.setColour (ToggleButton::textColourId, Colours::white);
        caseButton.setToggleState (isCaseSensitiveSearch(), dontSendNotification);
        caseButton.onClick = [this] { setCaseSensitiveSearch (caseButton.getToggleState()); };

        findPrev.setConnectedEdges (Button::ConnectedOnRight);
        findNext.setConnectedEdges (Button::ConnectedOnLeft);
        addAndMakeVisible (findPrev);
        addAndMakeVisible (findNext);

        setWantsKeyboardFocus (false);
        setFocusContainer (true);
        findPrev.setWantsKeyboardFocus (false);
        findNext.setWantsKeyboardFocus (false);

        editor.setText (getSearchString());
        editor.onTextChange = [this] { changeSearchString(); };
        editor.onReturnKey  = [] { ProjucerApplication::getCommandManager().invokeDirectly (CommandIDs::findNext, true); };
        editor.onEscapeKey  = [this]
        {
            if (auto* ed = getOwner())
                ed->hideFindPanel();
        };
    }

    void setCommandManager (ApplicationCommandManager* cm)
    {
        findPrev.setCommandToTrigger (cm, CommandIDs::findPrevious, true);
        findNext.setCommandToTrigger (cm, CommandIDs::findNext, true);
    }

    void paint (Graphics& g) override
    {
        Path outline;
        outline.addRoundedRectangle (1.0f, 1.0f, (float) getWidth() - 2.0f, (float) getHeight() - 2.0f, 8.0f);

        g.setColour (Colours::black.withAlpha (0.6f));
        g.fillPath (outline);
        g.setColour (Colours::white.withAlpha (0.8f));
        g.strokePath (outline, PathStrokeType (1.0f));
    }

    void resized() override
    {
        int y = 30;
        editor.setBounds (10, y, getWidth() - 20, 24);
        y += 30;
        caseButton.setBounds (10, y, getWidth() / 2 - 10, 22);
        findNext.setBounds (getWidth() - 40, y, 30, 22);
        findPrev.setBounds (getWidth() - 70, y, 30, 22);
    }

    void changeSearchString()
    {
        setSearchString (editor.getText());

        if (auto* ed = getOwner())
            ed->findNext (true, false);
    }

    GenericCodeEditorComponent* getOwner() const
    {
        return findParentComponentOfClass <GenericCodeEditorComponent>();
    }

    TextEditor editor;
    Label label  { {}, "Find:" };
    ToggleButton caseButton  { "Case-sensitive" };
    TextButton findPrev  { "<" },
               findNext  { ">" };
};

void GenericCodeEditorComponent::resized()
{
    CodeEditorComponent::resized();

    if (findPanel != nullptr)
    {
        findPanel->setSize (jmin (260, getWidth() - 32), 100);
        findPanel->setTopRightPosition (getWidth() - 16, 8);
    }
}

void GenericCodeEditorComponent::showFindPanel()
{
    if (findPanel == nullptr)
    {
        findPanel.reset (new FindPanel());
        findPanel->setCommandManager (&ProjucerApplication::getCommandManager());
        addAndMakeVisible (findPanel.get());
        resized();
    }

    if (findPanel != nullptr)
    {
        findPanel->editor.grabKeyboardFocus();
        findPanel->editor.selectAll();
    }
}

void GenericCodeEditorComponent::hideFindPanel()
{
    findPanel.reset();
}

void GenericCodeEditorComponent::findSelection()
{
    auto selected = getTextInRange (getHighlightedRegion());

    if (selected.isNotEmpty())
    {
        setSearchString (selected);
        findNext (true, true);
    }
}

void GenericCodeEditorComponent::findNext (bool forwards, bool skipCurrentSelection)
{
    auto highlight = getHighlightedRegion();
    const CodeDocument::Position startPos (getDocument(), skipCurrentSelection ? highlight.getEnd()
                                                                               : highlight.getStart());
    auto lineNum = startPos.getLineNumber();
    auto linePos = startPos.getIndexInLine();

    auto totalLines = getDocument().getNumLines();
    auto searchText = getSearchString();
    auto caseSensitive = isCaseSensitiveSearch();

    for (auto linesToSearch = totalLines; --linesToSearch >= 0;)
    {
        auto line = getDocument().getLine (lineNum);
        int index;

        if (forwards)
        {
            index = caseSensitive ? line.indexOf (linePos, searchText)
                                  : line.indexOfIgnoreCase (linePos, searchText);
        }
        else
        {
            if (linePos >= 0)
                line = line.substring (0, linePos);

            index = caseSensitive ? line.lastIndexOf (searchText)
                                  : line.lastIndexOfIgnoreCase (searchText);
        }

        if (index >= 0)
        {
            const CodeDocument::Position p (getDocument(), lineNum, index);
            selectRegion (p, p.movedBy (searchText.length()));
            break;
        }

        if (forwards)
        {
            linePos = 0;
            lineNum = (lineNum + 1) % totalLines;
        }
        else
        {
            if (--lineNum < 0)
                lineNum = totalLines - 1;

            linePos = -1;
        }
    }
}

void GenericCodeEditorComponent::handleEscapeKey()
{
    CodeEditorComponent::handleEscapeKey();
    hideFindPanel();
}

void GenericCodeEditorComponent::editorViewportPositionChanged()
{
    CodeEditorComponent::editorViewportPositionChanged();
    listeners.call ([this] (Listener& l) { l.codeEditorViewportMoved (*this); });
}

//==============================================================================
static CPlusPlusCodeTokeniser cppTokeniser;

CppCodeEditorComponent::CppCodeEditorComponent (const File& f, CodeDocument& doc)
    : GenericCodeEditorComponent (f, doc, &cppTokeniser)
{
}

CppCodeEditorComponent::~CppCodeEditorComponent() {}

void CppCodeEditorComponent::handleReturnKey()
{
    GenericCodeEditorComponent::handleReturnKey();

    auto pos = getCaretPos();

    String blockIndent, lastLineIndent;
    CodeHelpers::getIndentForCurrentBlock (pos, getTabString (getTabSize()), blockIndent, lastLineIndent);

    auto remainderOfBrokenLine = pos.getLineText();
    auto numLeadingWSChars = CodeHelpers::getLeadingWhitespace (remainderOfBrokenLine).length();

    if (numLeadingWSChars > 0)
        getDocument().deleteSection (pos, pos.movedBy (numLeadingWSChars));

    if (remainderOfBrokenLine.trimStart().startsWithChar ('}'))
        insertTextAtCaret (blockIndent);
    else
        insertTextAtCaret (lastLineIndent);

    auto previousLine = pos.movedByLines (-1).getLineText();
    auto trimmedPreviousLine = previousLine.trim();

    if ((trimmedPreviousLine.startsWith ("if ")
          || trimmedPreviousLine.startsWith ("if(")
          || trimmedPreviousLine.startsWith ("for ")
          || trimmedPreviousLine.startsWith ("for(")
          || trimmedPreviousLine.startsWith ("while(")
          || trimmedPreviousLine.startsWith ("while "))
         && trimmedPreviousLine.endsWithChar (')'))
    {
        insertTabAtCaret();
    }
}

void CppCodeEditorComponent::insertTextAtCaret (const String& newText)
{
    if (getHighlightedRegion().isEmpty())
    {
        auto pos = getCaretPos();

        if ((newText == "{" || newText == "}")
             && pos.getLineNumber() > 0
             && pos.getLineText().trim().isEmpty())
        {
            moveCaretToStartOfLine (true);

            String blockIndent, lastLineIndent;
            if (CodeHelpers::getIndentForCurrentBlock (pos, getTabString (getTabSize()), blockIndent, lastLineIndent))
            {
                GenericCodeEditorComponent::insertTextAtCaret (blockIndent);

                if (newText == "{")
                    insertTabAtCaret();
            }
        }
    }

    GenericCodeEditorComponent::insertTextAtCaret (newText);
}

void CppCodeEditorComponent::addPopupMenuItems (PopupMenu& menu, const MouseEvent* e)
{
    GenericCodeEditorComponent::addPopupMenuItems (menu, e);

    menu.addSeparator();
    menu.addItem (insertComponentID, TRANS("Insert code for a new Component class..."));
}

void CppCodeEditorComponent::performPopupMenuAction (int menuItemID)
{
    if (menuItemID == insertComponentID)
        insertComponentClass();

    GenericCodeEditorComponent::performPopupMenuAction (menuItemID);
}

void CppCodeEditorComponent::insertComponentClass()
{
    AlertWindow aw (TRANS ("Insert a new Component class"),
                    TRANS ("Please enter a name for the new class"),
                    AlertWindow::NoIcon, nullptr);

    const char* classNameField = "Class Name";

    aw.addTextEditor (classNameField, String(), String(), false);
    aw.addButton (TRANS ("Insert Code"),  1, KeyPress (KeyPress::returnKey));
    aw.addButton (TRANS ("Cancel"),       0, KeyPress (KeyPress::escapeKey));

    while (aw.runModalLoop() != 0)
    {
        auto className = aw.getTextEditorContents (classNameField).trim();

        if (className == build_tools::makeValidIdentifier (className, false, true, false))
        {
            String code (BinaryData::jucer_InlineComponentTemplate_h);
            code = code.replace ("%%component_class%%", className);

            insertTextAtCaret (code);
            break;
        }
    }
}
