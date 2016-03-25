#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "../PluginProcessor.h"

class LuaEditor	:	public CodeEditorComponent, public CodeDocument::Listener
{
public:
	LuaEditor(CodeDocument &doc, CodeTokeniser *tok) :
	  CodeEditorComponent(doc, tok)
	{
		somethingChanged = false;
		getDocument().addListener(this);
	}
	void setFontSize(float sz)
	{
		if (sz<2) sz = 2;
		setFont(getFont().withHeight(sz)); 
	}
	void mouseWheelMove(const MouseEvent &e,const MouseWheelDetails &wheel)
	{
		if (wheel.deltaY != 0 && ModifierKeys::getCurrentModifiers() == ModifierKeys(ModifierKeys::commandModifier))
			setFontSize(getFont().getHeight()+(wheel.deltaY>0?1.f:-1.f));
		else
			CodeEditorComponent::mouseWheelMove(e, wheel);
	}
	void handleTabKey()
	{
		if (isHighlightActive())
			if (ModifierKeys::getCurrentModifiers() == ModifierKeys::noModifiers)
			{ indentSelection(); return; }
			else if (ModifierKeys::getCurrentModifiers() == ModifierKeys::shiftModifier)
			{ unindentSelection(); return; }
		insertTabAtCaret();
	}
	void handleReturnKey()
	{
		String line = getDocument().getLine(getCaretPos().getLineNumber());
		String padding = line.initialSectionContainingOnly(" \t");
		insertTextAtCaret (getDocument().getNewLineCharacters()+padding);
	}
	void findNext(String searchTerm, bool direction, bool wrap = false)
	{
		if (searchTerm.isEmpty())
			return;
		int searchFrom;
		if (wrap)
			searchFrom = direction ? 0 : getDocument().getAllContent().length();
		else {
			Range<int> range = getHighlightedRegion();
			if (range.isEmpty())
				searchFrom = getCaretPos().getPosition() + (direction ? 1 : 0);
			else
				searchFrom = direction ? range.getEnd() : range.getStart();
		}
		int result;
		if (direction)
			result = getDocument().getAllContent().indexOf(searchFrom, searchTerm);
		else
			result = getDocument().getAllContent().substring(0,searchFrom).lastIndexOf(searchTerm);
		if (result != -1) {
			setHighlightedRegion(Range<int>(result, result+searchTerm.length()));
			return;
		} else {
			if (wrap)
				return;
			else
				findNext(searchTerm, direction, true);
		}
	}
	void codeDocumentTextInserted(const String &newText, int insertIndex)
	{ somethingChanged = true; }
	void codeDocumentTextDeleted(int startIndex, int endIndex)
	{ somethingChanged = true; }
	bool somethingChanged;
};
