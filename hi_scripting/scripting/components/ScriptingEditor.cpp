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

#include <regex>

namespace hise { using namespace juce;


	HardcodedScriptEditor::HardcodedScriptEditor(ProcessorEditor* p):
		ProcessorEditorBody(p)
	{
		addAndMakeVisible(contentComponent = new ScriptContentComponent(static_cast<ScriptBaseMidiProcessor*>(getProcessor())));
		contentComponent->refreshMacroIndexes();
	}

	void HardcodedScriptEditor::updateGui()
	{
		contentComponent->changeListenerCallback(nullptr);
	}

	int HardcodedScriptEditor::getBodyHeight() const
	{
		return contentComponent->getContentHeight() == 0 ? 0 : contentComponent->getContentHeight() + 38;
	}

	void HardcodedScriptEditor::resized()
	{
		contentComponent->setBounds((getWidth() / 2) - ((getWidth() - 90) / 2), 24 ,getWidth() - 90, contentComponent->getContentHeight());
	}

//==============================================================================
ScriptingEditor::ScriptingEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p),
	  ScriptEditHandler(),
      doc (new CodeDocument()),
      tokenizer(new JavascriptTokeniser())
{
	JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

	if (auto jsp = dynamic_cast<JavascriptMidiProcessor*>(sp))
	{
		isFront = jsp->isFront();
	}

	if (isFront)
		return;

	addAndMakeVisible(contentButton = new TextButton("new button"));
	contentButton->setButtonText(TRANS("Interface"));
	contentButton->setConnectedEdges(Button::ConnectedOnRight);
	contentButton->addListener(this);
	contentButton->setClickingTogglesState(true);
	contentButton->setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
	contentButton->setColour(TextButton::buttonOnColourId, Colour(0xffb4b4b4));
	contentButton->setColour(TextButton::textColourOnId, Colour(0x77ffffff));
	contentButton->setColour(TextButton::textColourOffId, Colour(0x45ffffff));
	contentButton->setLookAndFeel(&alaf);

	for (int i = 0; i < sp->getNumSnippets(); i++)
	{
		TextButton *b = new TextButton("new button");
		addAndMakeVisible(b);
		b->setButtonText(sp->getSnippet(i)->getCallbackName().toString());
		b->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
		b->addListener(this);
		b->setClickingTogglesState(true);
		b->setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
		b->setColour(TextButton::buttonOnColourId, Colour(0xff680000));
		b->setColour(TextButton::textColourOnId, Colour(0x77ffffff));
		b->setColour(TextButton::textColourOffId, Colour(0x45ffffff));
		b->setLookAndFeel(&alaf);
		callbackButtons.add(b);
	}
    
	callbackButtons.getLast()->setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnLeft);

	addAndMakeVisible(scriptContent = new ScriptContentComponent(dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())));

    scriptContent->addMouseListenersForComponentWrappers();
	useComponentSelectMode = false;

	if (dynamic_cast<JavascriptMidiProcessor*>(getProcessor()))
	{
		lastPositions.add(0);
		lastPositions.add(23);
		lastPositions.add(24);
		lastPositions.add(27);
		lastPositions.add(22);
		lastPositions.add(37);
	}
	
	addAndMakeVisible(dragOverlay = new ScriptingContentOverlay(this));

	currentDragOverlay = dragOverlay;

	editorInitialized();
}

ScriptingEditor::~ScriptingEditor()
{ 
	scriptContent = nullptr;
    codeEditor = nullptr;
	callbackButtons.clear();
    contentButton = nullptr;
	lastPositions.clear();
    doc = nullptr;
}

JavascriptProcessor* ScriptingEditor::getScriptEditHandlerProcessor()
{ return dynamic_cast<JavascriptProcessor*>(getProcessor()); }

ScriptContentComponent* ScriptingEditor::getScriptEditHandlerContent()
{ return scriptContent.get(); }

ScriptingContentOverlay* ScriptingEditor::getScriptEditHandlerOverlay()
{ return dragOverlay; }

CommonEditorFunctions::EditorType* ScriptingEditor::getScriptEditHandlerEditor()
{ return codeEditor->editor; }

void ScriptingEditor::editorInitialized()
{
	JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(getProcessor());
	int editorOffset = dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

	for(int i = 0; i < sp->getNumSnippets(); i++)
	{
		if(getProcessor()->getEditorState(editorOffset + i + ProcessorWithScriptingContent::EditorStates::onInitShown))
			showCallback(i);
	}

	setSize(getWidth(), getBodyHeight());

	checkActiveSnippets();
}

bool ScriptingEditor::isInEditMode() const
{
	return codeEditor != nullptr && scriptContent->isVisible() && (getActiveCallback() == 0);
}

void ScriptingEditor::checkContent()
{
	const bool contentEmpty = scriptContent->getContentHeight() == 0;

	if(contentEmpty) contentButton->setToggleState(false, dontSendNotification);

	Button *t = contentButton;

	t->setColour(TextButton::buttonColourId, !contentEmpty ? Colour (0x77cccccc) : Colour (0x4c4b4b4b));
	t->setColour (TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
	t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
	t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));

	contentButton->setEnabled(!contentEmpty);

	resized();
}

Button* ScriptingEditor::getSnippetButton(int i)
{
	return callbackButtons[i];
}

void ScriptingEditor::selectOnInitCallback()
{
	if (getActiveCallback() != 0)
		buttonClicked(callbackButtons[0]); // we need synchronous execution here
}

int ScriptingEditor::getBodyHeight() const
{
	if (isFront)
		return 0;

	const ProcessorWithScriptingContent* pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(getProcessor());

	auto contentHeight = pwsc->getScriptingContent()->getContentHeight();

	if (isConnectedToExternalScript)
		return contentHeight;
	
	const bool showContent = scriptContent->isVisible();

	if (!showContent)
		contentHeight = 0;

	return 28 + contentHeight + (codeEditor != nullptr ? EditorHeight : 0);
}

void ScriptingEditor::resized()
{
	if (isFront)
		return;
	
	dragOverlay->setVisible(!isConnectedToExternalScript);
	contentButton->setVisible(!isConnectedToExternalScript);

	for (int i = 0; i < callbackButtons.size(); i++)
		callbackButtons[i]->setVisible(!isConnectedToExternalScript);

	auto b = getLocalBounds();

	if (isConnectedToExternalScript)
	{
		scriptContent->setVisible(true);
		scriptContent->setBounds(b);
		return;
	}

	auto buttonRow = b.removeFromTop(28);

	buttonRow = buttonRow.reduced(30, 5);

	auto wPerButton = buttonRow.getWidth() / (callbackButtons.size() + 1);

	contentButton->setBounds(buttonRow.removeFromLeft(wPerButton));

	for (auto& cb : callbackButtons)
		cb->setBounds(buttonRow.removeFromLeft(wPerButton));

	if (scriptContent->isVisible())
		scriptContent->setBounds(b.removeFromTop(scriptContent->getContentHeight()));

	if (codeEditor != nullptr)
		codeEditor->setBounds(b);
}

void ScriptingEditor::buttonClicked (Button* buttonThatWasClicked)
{
	int callbackIndex = callbackButtons.indexOf(dynamic_cast<TextButton*>(buttonThatWasClicked));

	if (callbackIndex != -1)
	{
		for (auto b : callbackButtons)
			b->setToggleState(b == buttonThatWasClicked && buttonThatWasClicked->getToggleState(), dontSendNotification);

		showCallback(callbackIndex);
		return;
	}
    else if (buttonThatWasClicked == contentButton)
    {
		scriptContent->setVisible(buttonThatWasClicked->getToggleState());
		refreshBodySize();
		return;
    }
}

void ScriptingEditor::updateGui()
{
	if (isFront)
		return;

	JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

	const bool nowConnected = sp->isConnectedToExternalFile();

	if (nowConnected != isConnectedToExternalScript)
	{
		isConnectedToExternalScript = nowConnected;

		if (isConnectedToExternalScript)
			codeEditor = nullptr;

		useComponentSelectMode = false;
		refreshBodySize();
	}

	if (getHeight() != getBodyHeight()) setSize(getWidth(), getBodyHeight());

	getProcessor()->setEditorState(Processor::BodyShown, true);

	int editorOffset = dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

	contentButton->setToggleState(getProcessor()->getEditorState(editorOffset + ProcessorWithScriptingContent::EditorStates::contentShown), dontSendNotification);
}

void ScriptingEditor::showCallback(int callbackIndex, int lineToScroll/*=-1*/)
{
	if (currentCallbackIndex != callbackIndex && callbackIndex < callbackButtons.size())
	{
		if (codeEditor != nullptr)
			lastPositions.set(currentCallbackIndex, CommonEditorFunctions::getCaretPos(codeEditor).getPosition());;

		currentCallbackIndex = callbackIndex;

		callbackButtons[callbackIndex]->setToggleState(true, dontSendNotification);

		auto jp = dynamic_cast<JavascriptProcessor*>(getProcessor());

		auto d = jp->getSnippet(callbackIndex);

		addAndMakeVisible(codeEditor = new PopupIncludeEditor(jp, d->getCallbackName()));

		CodeDocument::Position pos(*d, lastPositions[callbackIndex]);
		CommonEditorFunctions::moveCaretTo(codeEditor, pos, false);

	}
	else
	{
		currentCallbackIndex = -1;
		codeEditor = nullptr;
	}

	refreshBodySize();
	resized();
}

void ScriptingEditor::showOnInitCallback()
{
	showCallback(0);
}

void ScriptingEditor::gotoChar(int character)
{
	showOnInitCallback();

	CodeDocument::Position pos(CommonEditorFunctions::getDoc(codeEditor->editor), character);
	CommonEditorFunctions::moveCaretTo(codeEditor->editor, pos, false);
}

bool ScriptingEditor::keyPressed(const KeyPress &k)
{
#if JUCE_WINDOWS || JUCE_LINUX
	if ((k.isKeyCode(KeyPress::leftKey) || k.isKeyCode(KeyPress::rightKey)) && k.getModifiers().isCtrlDown() && k.getModifiers().isAltDown())
#else
    if ((k.isKeyCode(KeyPress::leftKey) || k.isKeyCode(KeyPress::rightKey)) && k.getModifiers().isCtrlDown() && k.getModifiers().isCommandDown())
#endif
	{
		int current = getActiveCallback();
        if(k.isKeyCode(KeyPress::F2Key) || k.isKeyCode(KeyPress::leftKey)) current--;
		else							 current++;

		if (current > 0 && current < callbackButtons.size())
		{
			callbackButtons[current]->triggerClick();
			return true;
		}
	}

	if (k.isKeyCode(KeyPress::F1Key))
	{
		contentButton->triggerClick();
		return true;
	}
	else if (k.getKeyCode() == KeyPress::F5Key && !k.getModifiers().isShiftDown())
	{
		int i = CommonEditorFunctions::getCaretPos(codeEditor->editor).getPosition();

		compileScript();

		CodeDocument::Position pos(CommonEditorFunctions::getDoc(codeEditor->editor), i);

		CommonEditorFunctions::moveCaretTo(codeEditor->editor, pos, false);

		return true;
	}
	if (k.isKeyCode('1') && k.getModifiers().isCtrlDown())
	{
		if (callbackButtons.size() > 0)
			callbackButtons[0]->triggerClick();

		return true;
	}
	else if(k.isKeyCode('2') && k.getModifiers().isCtrlDown())
	{
		if (callbackButtons.size() > 1)
			callbackButtons[1]->triggerClick();

		return true;
	}
	else if (k.isKeyCode('3') && k.getModifiers().isCtrlDown())
	{
		if (callbackButtons.size() > 2)
			callbackButtons[2]->triggerClick();


		return true;
	}
	else if (k.isKeyCode('4') && k.getModifiers().isCtrlDown())
	{
		if (callbackButtons.size() > 3)
			callbackButtons[3]->triggerClick();

		return true;
	}
	else if (k.isKeyCode('5') && k.getModifiers().isCtrlDown())
	{
		if (callbackButtons.size() > 4)
			callbackButtons[4]->triggerClick();

		return true;
	}
	else if (k.isKeyCode('6') && k.getModifiers().isCtrlDown())
	{
		if (callbackButtons.size() > 5)
			callbackButtons[5]->triggerClick();

		return true;
	}

	return false;
}

void ScriptingEditor::scriptEditHandlerCompileCallback()
{
	getScriptEditHandlerProcessor()->compileScript();


	checkActiveSnippets();
	refreshBodySize();
	repaint();
}

int ScriptingEditor::getActiveCallback() const
{
	const JavascriptProcessor *sp = dynamic_cast<const JavascriptProcessor*>(getProcessor());

	if (codeEditor == nullptr) return sp->getNumSnippets();

	const CodeDocument &codeDoc = CommonEditorFunctions::getDoc(codeEditor->editor);

	for (int i = 0; i < sp->getNumSnippets(); i++)
	{
		if (&codeDoc == sp->getSnippet(i))
		{
			return i;
		}
	}

	return sp->getNumSnippets();
}

void ScriptingEditor::checkActiveSnippets()
{
	JavascriptProcessor *s = dynamic_cast<JavascriptProcessor*>(getProcessor());

	for (int i = 0; i < s->getNumSnippets(); i++)
	{
		const bool isSnippetEmpty = s->getSnippet(i)->isSnippetEmpty();

		TextButton *t = callbackButtons[i];

		t->setColour(TextButton::buttonColourId, !isSnippetEmpty ? Colour(0x77cccccc) : Colour(0x4c4b4b4b));
		t->setColour(TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
		t->setColour(TextButton::textColourOnId, Colour(0xaa000000));
		t->setColour(TextButton::textColourOffId, Colour(0x99ffffff));

		if (JavascriptMidiProcessor* jsmp = dynamic_cast<JavascriptMidiProcessor*>(s))
		{
			if (i == JavascriptMidiProcessor::onNoteOff || i == JavascriptMidiProcessor::onNoteOn || i == JavascriptMidiProcessor::onController || i == JavascriptMidiProcessor::onTimer)
			{
				t->setButtonText(s->getSnippet(i)->getCallbackName().toString() + (jsmp->isDeferred() ? " (D)" : ""));
			}
		}
	}
}

} // namespace hise
