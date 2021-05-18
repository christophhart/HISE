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
	{
		return;
	}

	static const Identifier empty("empty");

    addAndMakeVisible (codeEditor = new CodeEditorWrapper (*doc, tokenizer, sp, empty));
    codeEditor->setName ("new component");

    addAndMakeVisible (compileButton = new TextButton ("new button"));
    compileButton->setButtonText (TRANS("Compile"));
    compileButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    compileButton->addListener (this);
    compileButton->setColour (TextButton::buttonColourId, Colour (0xa2616161));

    addAndMakeVisible (messageBox = new DebugConsoleTextEditor("new text editor", getProcessor()));
    
	
    addAndMakeVisible (timeLabel = new Label ("new label",
                                              TRANS("2.5 microseconds")));
    timeLabel->setFont (GLOBAL_BOLD_FONT());
    timeLabel->setJustificationType (Justification::centredLeft);
    timeLabel->setEditable (false, false, false);
    timeLabel->setColour (Label::textColourId, Colours::white);
    timeLabel->setColour (TextEditor::textColourId, Colours::black);
    timeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

	addAndMakeVisible(buttonRow = new Component());

	buttonRow->addAndMakeVisible(contentButton = new TextButton("new button"));
	contentButton->setButtonText(TRANS("Interface"));
	contentButton->setConnectedEdges(Button::ConnectedOnRight);
	contentButton->addListener(this);
	contentButton->setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
	contentButton->setColour(TextButton::buttonOnColourId, Colour(0xffb4b4b4));
	contentButton->setColour(TextButton::textColourOnId, Colour(0x77ffffff));
	contentButton->setColour(TextButton::textColourOffId, Colour(0x45ffffff));
	contentButton->setLookAndFeel(&alaf);

	for (int i = 0; i < sp->getNumSnippets(); i++)
	{
		TextButton *b = new TextButton("new button");
		buttonRow->addAndMakeVisible(b);
		b->setButtonText(sp->getSnippet(i)->getCallbackName().toString());
		b->setConnectedEdges(Button::ConnectedOnLeft | Button::ConnectedOnRight);
		b->addListener(this);
		b->setColour(TextButton::buttonColourId, Colour(0x4c4b4b4b));
		b->setColour(TextButton::buttonOnColourId, Colour(0xff680000));
		b->setColour(TextButton::textColourOnId, Colour(0x77ffffff));
		b->setColour(TextButton::textColourOffId, Colour(0x45ffffff));
		b->setLookAndFeel(&alaf);
		callbackButtons.add(b);
	}

    buttonRow->setBufferedToImage(true);
    
	callbackButtons.getLast()->setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnLeft);

    timeLabel->setFont (GLOBAL_BOLD_FONT());

	
	
	addAndMakeVisible(scriptContent = new ScriptContentComponent(dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())));

    scriptContent->addMouseListenersForComponentWrappers();
    
	

	useComponentSelectMode = false;

	compileButton->setLookAndFeel(&alaf);

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

    setSize (800, 500);

	

	editorInitialized();
}

ScriptingEditor::~ScriptingEditor()
{ 
	scriptContent = nullptr;

    codeEditor = nullptr;
    compileButton = nullptr;
    messageBox = nullptr;
    timeLabel = nullptr;
	callbackButtons.clear();
    contentButton = nullptr;

	lastPositions.clear();
   
    doc = nullptr;
    
}

int ScriptingEditor::getBodyHeight() const
{
	if (isFront)
		return 0;

	if (isRootEditor())
	{
		if(auto viewport = findParentComponentOfClass<Viewport>())
			return viewport->getHeight() - 36;

		if (findParentComponentOfClass<FloatingTilePopup>() != nullptr)
			return 500;
	}

	const ProcessorWithScriptingContent* pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(getProcessor());

	if (isConnectedToExternalScript)
	{
		return pwsc->getScriptingContent()->getContentHeight();
	}
	else
	{
		int editorOffset = pwsc->getCallbackEditorStateOffset();

		const bool showContent = getProcessor()->getEditorState(editorOffset + ProcessorWithScriptingContent::EditorStates::contentShown);

		const int contentHeight = showContent ? pwsc->getScriptingContent()->getContentHeight() : 0;

		const int additionalOffset = (dynamic_cast<const JavascriptModulatorSynth*>(getProcessor()) != nullptr) ? 5 : 0;

		if (editorShown)
		{
			return 28 + additionalOffset + contentHeight + codeEditor->currentHeight + 24;
		}
		else
		{
			return 28 + additionalOffset + contentHeight;
		}
	}
}

void ScriptingEditor::gotoLocation(DebugInformation* info)
{
	showOnInitCallback();
	gotoChar(info->getLocation().charNumber);
}

//==============================================================================
void ScriptingEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..

	if(!isConnectedToExternalScript && editorShown && getProcessor() != nullptr)
	{
		
		Colour c = dynamic_cast<JavascriptProcessor*>(getProcessor())->wasLastCompileOK() ? Colour(0xff323832) : Colour(0xff383232);

		float y = (float)codeEditor->getBottom();

		g.setGradientFill(ColourGradient(c.withMultipliedBrightness(0.8f), 0.0f, y, c, 0.0f, y+6.0f, false));

		g.fillRect(1, codeEditor->getBottom(), getWidth()-2, compileButton->getHeight());
	}

    //[/UserPaint]
}

void ScriptingEditor::resized()
{
	if (isFront)
		return;

	codeEditor->setVisible(!isConnectedToExternalScript);
	codeEditor->setVisible(!isConnectedToExternalScript);
	messageBox->setVisible(!isConnectedToExternalScript);
	dragOverlay->setVisible(!isConnectedToExternalScript);
	contentButton->setVisible(!isConnectedToExternalScript);

	for (int i = 0; i < callbackButtons.size(); i++)
		callbackButtons[i]->setVisible(!isConnectedToExternalScript);

	if (isConnectedToExternalScript)
	{
		

		scriptContent->setVisible(true);
		scriptContent->setBounds(0, 0, getWidth(), scriptContent->getContentHeight());
		return;
	}

	codeEditor->setBounds ((getWidth() / 2) - ((getWidth() - 90) / 2), 104, getWidth() - 90, getHeight() - 140);
    compileButton->setBounds (((getWidth() / 2) - ((getWidth() - 90) / 2)) + (getWidth() - 90) - 95, getHeight() - 24, 95, 24);
    messageBox->setBounds (((getWidth() / 2) - ((getWidth() - 90) / 2)) + 0, getHeight() - 24, getWidth() - 296, 24);
    
	int buttonWidth = (getWidth()-20) / (callbackButtons.size() + 1);
	int buttonX = 10;

	int y = 28;

	int xOff = isRootEditor() ? 0 : 1;
	int w = getWidth() - 2*xOff;

	int editorOffset = dynamic_cast<const ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

	scriptContent->setVisible(getProcessor()->getEditorState(editorOffset + ProcessorWithScriptingContent::contentShown));

	const int contentHeight = scriptContent->isVisible() ? scriptContent->getContentHeight() : 0;

	scriptContent->setBounds(xOff, y, w, scriptContent->getContentHeight());
	y += contentHeight;

	const bool fullscreen = getEditor()->isPopup() || isRootEditor();

	const int editorHeight = fullscreen ? getHeight() - y - 24 : codeEditor->currentHeight;

	codeEditor->setSize(w, editorHeight);

    setWantsKeyboardFocus(editorShown);
    
	if(!editorShown)
	{
		codeEditor->setBounds(0, y, getWidth(), codeEditor->getHeight());

		codeEditor->setTopLeftPosition(0, getHeight());
		compileButton->setTopLeftPosition(0, getHeight());
		messageBox->setTopLeftPosition(0, getHeight());
        
		
	}
	else
	{
		codeEditor->setBounds(xOff, y, getWidth() - 2 * xOff, codeEditor->getHeight());

		y += codeEditor->getHeight();

		messageBox->setBounds(1, y, getWidth() - compileButton->getWidth() - 4, 24);

		compileButton->setTopLeftPosition(codeEditor->getRight() - compileButton->getWidth(), y);
	}

	dragOverlay->setBounds(scriptContent->getBounds());
	dragOverlay->setVisible(true);

    // The editor gets weirdly disabled occasionally, so this is a hacky fix for this...
    setEnabled(true);

	contentButton->setBounds(buttonX, 0, buttonWidth, 20);
	buttonX = contentButton->getRight();

	buttonRow->setBounds(0, 0, getWidth(), 28);

	for (int i = 0; i < callbackButtons.size(); i++)
	{
		callbackButtons[i]->setBounds(buttonX, 0, buttonWidth, 20);
		buttonX = callbackButtons[i]->getRight();
	}

}

void ScriptingEditor::buttonClicked (Button* buttonThatWasClicked)
{
    
	JavascriptProcessor *s = dynamic_cast<JavascriptProcessor*>(getProcessor());

	int editorOffset = dynamic_cast<const ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

	int callbackIndex = callbackButtons.indexOf(dynamic_cast<TextButton*>(buttonThatWasClicked));

	if (callbackIndex != -1)
	{
		saveLastCallback();

		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(callbackIndex), tokenizer, dynamic_cast<JavascriptProcessor*>(getProcessor()), s->getSnippet(callbackIndex)->getCallbackName()));
		goToSavedPosition(callbackIndex);

	}
	else if (buttonThatWasClicked == compileButton)
	{

		compileScript();

		return;

	}
    else if (buttonThatWasClicked == contentButton)
    {
		

		getProcessor()->setEditorState(editorOffset + ProcessorWithScriptingContent::EditorStates::contentShown, !toggleButton(contentButton));
		refreshBodySize();
		resized();
		return;
    }

	if(buttonThatWasClicked->getToggleState())
	{
		editorShown = false;

		for (int i = 0; i < callbackButtons.size(); i++)
		{
			callbackButtons[i]->setToggleState(false, dontSendNotification);
			getProcessor()->setEditorState(editorOffset + (int)ProcessorWithScriptingContent::EditorStates::onInitShown + i, false);
		}
	}
	else
	{
		editorShown = true;

		for (int i = 0; i < callbackButtons.size(); i++)
		{
			const bool isShown = buttonThatWasClicked == callbackButtons[i];

			if (isShown)
				currentCallbackIndex = i;

			callbackButtons[i]->setToggleState(isShown, dontSendNotification);
			getProcessor()->setEditorState(editorOffset + (int)ProcessorWithScriptingContent::EditorStates::onInitShown + i, isShown);
		}

		buttonThatWasClicked->setToggleState(true, dontSendNotification);
	}

	resized();
	refreshBodySize();
}

//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void ScriptingEditor::goToSavedPosition(int newCallback)
{
	if (newCallback < callbackButtons.size())
	{
		if (newCallback < lastPositions.size())
		{
			codeEditor->editor->moveCaretTo(CodeDocument::Position(codeEditor->editor->getDocument(), lastPositions[newCallback]), false);
		}
		
		codeEditor->editor->scrollToColumn(0);

		if(codeEditor->editor->isShowing())
			codeEditor->editor->grabKeyboardFocus();
	}
}

void ScriptingEditor::saveLastCallback()
{
	int lastCallback = getActiveCallback();
	if (lastCallback < callbackButtons.size())
	{
		if (lastCallback < lastPositions.size())
		{
			lastPositions.set(lastCallback, codeEditor->editor->getCaretPos().getPosition());
		}
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
		callbackButtons[callbackIndex]->setToggleState(false, dontSendNotification);
		buttonClicked(callbackButtons[callbackIndex]);

		

		if (lineToScroll != -1)
		{
			CodeDocument::Position pos(codeEditor->editor->getDocument(), lineToScroll, 0);
			codeEditor->editor->scrollToLine(jmax<int>(0, lineToScroll));

			Array<Range<int>> underlines;

			underlines.add(Range<int>(0, 6));

			codeEditor->editor->moveCaretTo(pos, false);
			codeEditor->editor->moveCaretToEndOfLine(true);

			//codeEditor->editor->setTemporaryUnderlining(underlines);
		}
	}
}

void ScriptingEditor::showOnInitCallback()
{
	showCallback(0);
}

void ScriptingEditor::openContentInPopup()
{
#if USE_BACKEND
	BackendCommandTarget::Actions::addInterfacePreview(GET_BACKEND_ROOT_WINDOW(this));
#endif
}

void ScriptingEditor::closeAllPopup()
{
	
	codeEditor->setEnabled(true);
	checkActiveSnippets();
}

void ScriptingEditor::gotoChar(int character)
{
	showOnInitCallback();

	CodeDocument::Position pos(codeEditor->editor->getDocument(), character);

	codeEditor->editor->scrollToLine(jmax<int>(0, pos.getLineNumber()));
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
		int i = codeEditor->editor->getCaretPos().getPosition();

		compileScript();

		CodeDocument::Position pos(codeEditor->editor->getDocument(), i);

		codeEditor->editor->moveCaretTo(pos, false);

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

	const CodeDocument &codeDoc = codeEditor->editor->getDocument();

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