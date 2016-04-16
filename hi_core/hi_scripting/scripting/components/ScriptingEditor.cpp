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


//==============================================================================
ScriptingEditor::ScriptingEditor (BetterProcessorEditor *p)
    : ProcessorEditorBody(p),
      doc (dynamic_cast<ScriptProcessor*>(getProcessor())->getDocument()),
      tokenizer(new JavascriptTokeniser())
{
    

    addAndMakeVisible (codeEditor = new CodeEditorWrapper (*doc,
                                                           tokenizer,
                                                           dynamic_cast<ScriptProcessor*>(getProcessor())));
    codeEditor->setName ("new component");

    addAndMakeVisible (compileButton = new TextButton ("new button"));
    compileButton->setButtonText (TRANS("Compile"));
    compileButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    compileButton->addListener (this);
    compileButton->setColour (TextButton::buttonColourId, Colour (0xa2616161));

    addAndMakeVisible (messageBox = new TextEditor ("new text editor"));
    messageBox->setMultiLine (false);
    messageBox->setReturnKeyStartsNewLine (false);
    messageBox->setReadOnly (true);
    messageBox->setScrollbarsShown (false);
    messageBox->setCaretVisible (false);
    messageBox->setPopupMenuEnabled (false);
    messageBox->setColour (TextEditor::textColourId, Colours::white);
    messageBox->setColour (TextEditor::backgroundColourId, Colour (0x00ffffff));
    messageBox->setColour (TextEditor::highlightColourId, Colour (0x40ffffff));
    messageBox->setColour (TextEditor::shadowColourId, Colour (0x00000000));
    messageBox->setText (String::empty);

    addAndMakeVisible (timeLabel = new Label ("new label",
                                              TRANS("2.5 microseconds")));
    timeLabel->setFont (Font ("Khmer UI", 12.40f, Font::plain));
    timeLabel->setJustificationType (Justification::centredLeft);
    timeLabel->setEditable (false, false, false);
    timeLabel->setColour (Label::textColourId, Colours::white);
    timeLabel->setColour (TextEditor::textColourId, Colours::black);
    timeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

    addAndMakeVisible (noteOnButton = new TextButton ("new button"));
    noteOnButton->setButtonText (TRANS("onNoteOn"));
    noteOnButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    noteOnButton->addListener (this);
    noteOnButton->setColour (TextButton::buttonColourId, Colour (0x4c4b4b4b));
    noteOnButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    noteOnButton->setColour (TextButton::textColourOnId, Colour (0x77ffffff));
    noteOnButton->setColour (TextButton::textColourOffId, Colour (0x45ffffff));

    addAndMakeVisible (noteOffButton = new TextButton ("new button"));
    noteOffButton->setButtonText (TRANS("onNoteOff"));
    noteOffButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    noteOffButton->addListener (this);
    noteOffButton->setColour (TextButton::buttonColourId, Colour (0x4c4b4b4b));
    noteOffButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    noteOffButton->setColour (TextButton::textColourOnId, Colour (0x77ffffff));
    noteOffButton->setColour (TextButton::textColourOffId, Colour (0x45ffffff));

    addAndMakeVisible (onControllerButton = new TextButton ("new button"));
    onControllerButton->setButtonText (TRANS("onController"));
    onControllerButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    onControllerButton->addListener (this);
    onControllerButton->setColour (TextButton::buttonColourId, Colour (0x4c4b4b4b));
    onControllerButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    onControllerButton->setColour (TextButton::textColourOnId, Colour (0x77ffffff));
    onControllerButton->setColour (TextButton::textColourOffId, Colour (0x45ffffff));

    addAndMakeVisible (onTimerButton = new TextButton ("new button"));
    onTimerButton->setButtonText (TRANS("onTimer"));
    onTimerButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    onTimerButton->addListener (this);
    onTimerButton->setColour (TextButton::buttonColourId, Colour (0x4c4b4b4b));
    onTimerButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    onTimerButton->setColour (TextButton::textColourOnId, Colour (0x77ffffff));
    onTimerButton->setColour (TextButton::textColourOffId, Colour (0x45ffffff));

    addAndMakeVisible (onInitButton = new TextButton ("new button"));
    onInitButton->setButtonText (TRANS("onInit"));
    onInitButton->setConnectedEdges (Button::ConnectedOnLeft | Button::ConnectedOnRight);
    onInitButton->addListener (this);
    onInitButton->setColour (TextButton::buttonColourId, Colour (0x4c4b4b4b));
    onInitButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    onInitButton->setColour (TextButton::textColourOnId, Colour (0x77ffffff));
    onInitButton->setColour (TextButton::textColourOffId, Colour (0x45ffffff));

    addAndMakeVisible (onControlButton = new TextButton ("new button"));
    onControlButton->setButtonText (TRANS("onControl"));
    onControlButton->setConnectedEdges (Button::ConnectedOnLeft);
    onControlButton->addListener (this);
    onControlButton->setColour (TextButton::buttonColourId, Colour (0x4c4b4b4b));
    onControlButton->setColour (TextButton::buttonOnColourId, Colour (0xff680000));
    onControlButton->setColour (TextButton::textColourOnId, Colour (0x77ffffff));
    onControlButton->setColour (TextButton::textColourOffId, Colour (0x45ffffff));

    addAndMakeVisible (contentButton = new TextButton ("new button"));
    contentButton->setButtonText (TRANS("Content"));
    contentButton->setConnectedEdges (Button::ConnectedOnRight);
    contentButton->addListener (this);
    contentButton->setColour (TextButton::buttonColourId, Colour (0x4c4b4b4b));
    contentButton->setColour (TextButton::buttonOnColourId, Colour (0xffb4b4b4));
    contentButton->setColour (TextButton::textColourOnId, Colour (0x77ffffff));
    contentButton->setColour (TextButton::textColourOffId, Colour (0x45ffffff));

	ScriptProcessor *sp = dynamic_cast<ScriptProcessor*>(getProcessor());
	

	if (getProcessor()->getEditorState(getProcessor()->getEditorStateForIndex(ScriptProcessor::externalPopupShown)))
	{
		//static_cast<ScriptProcessor*>(getProcessor())->showPopupForFile(0);
	}

    timeLabel->setFont (GLOBAL_FONT());

	messageBox->setFont(GLOBAL_MONOSPACE_FONT());

	codeEditor->editor->setFont(GLOBAL_MONOSPACE_FONT());

	

	doc = sp->getDocument();

	addAndMakeVisible(scriptContent = new ScriptContentComponent(static_cast<ScriptProcessor*>(getProcessor())));


	messageBox->setLookAndFeel(&laf2);

	messageBox->setColour(Label::ColourIds::textColourId, Colours::white);

	useComponentSelectMode = false;

	compileButton->setLookAndFeel(&alaf);

	lastPositions.add(new CodeDocument::Position(*sp->getSnippet(ScriptProcessor::Callback::onInit), 0));
	lastPositions.add(new CodeDocument::Position(*sp->getSnippet(ScriptProcessor::Callback::onNoteOn), 23));
	lastPositions.add(new CodeDocument::Position(*sp->getSnippet(ScriptProcessor::Callback::onNoteOff), 24));
	lastPositions.add(new CodeDocument::Position(*sp->getSnippet(ScriptProcessor::Callback::onController), 27));
	lastPositions.add(new CodeDocument::Position(*sp->getSnippet(ScriptProcessor::Callback::onTimer), 22));
	lastPositions.add(new CodeDocument::Position(*sp->getSnippet(ScriptProcessor::Callback::onControl), 37));
	
   

    setSize (800, 500);



	editorInitialized();

}

ScriptingEditor::~ScriptingEditor()
{
   
	getProcessor()->getMainController()->setEditedScriptComponent(nullptr, nullptr);

	doc = nullptr;

	scriptContent = nullptr;

   

    codeEditor = nullptr;
    compileButton = nullptr;
    messageBox = nullptr;
    timeLabel = nullptr;
    noteOnButton = nullptr;
    noteOffButton = nullptr;
    onControllerButton = nullptr;
    onTimerButton = nullptr;
    onInitButton = nullptr;
    onControlButton = nullptr;
    contentButton = nullptr;

	lastPositions.clear();
   
}

//==============================================================================
void ScriptingEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
	g.fillAll(Colours::transparentBlack);
    //[/UserPrePaint]

    //[UserPaint] Add your own custom painting code here..

	if(editorShown && getProcessor() != nullptr)
	{
		g.setColour(dynamic_cast<ScriptProcessor*>(getProcessor())->wasLastCompileOK() ? Colour(0xff323832) : Colour(0xff383232));
		g.fillRect (1, getHeight() - 26, getWidth()-1, 26);

		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawHorizontalLine(getHeight() - 25, 1.0f, (float)getWidth() - 1.0f);
	}

    //[/UserPaint]
}

void ScriptingEditor::resized()
{
    //[UserPreResize] Add your own custom resize code here..


    //[/UserPreResize]

    codeEditor->setBounds ((getWidth() / 2) - ((getWidth() - 90) / 2), 104, getWidth() - 90, getHeight() - 140);
    compileButton->setBounds (((getWidth() / 2) - ((getWidth() - 90) / 2)) + (getWidth() - 90) - 95, getHeight() - 24, 95, 24);
    messageBox->setBounds (((getWidth() / 2) - ((getWidth() - 90) / 2)) + 0, getHeight() - 24, getWidth() - 296, 24);
    timeLabel->setBounds (getWidth() - 136 - 104, getHeight() - -1 - 24, 104, 24);
    noteOnButton->setBounds ((proportionOfWidth (0.0450f) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f), 0, proportionOfWidth (0.1300f), 20);
    noteOffButton->setBounds (((proportionOfWidth (0.0450f) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f), 0, proportionOfWidth (0.1300f), 20);
    onControllerButton->setBounds ((((proportionOfWidth (0.0450f) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f), 0, proportionOfWidth (0.1300f), 20);
    onTimerButton->setBounds (((((proportionOfWidth (0.0450f) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f), 0, proportionOfWidth (0.1300f), 20);
    onInitButton->setBounds (proportionOfWidth (0.0450f) + proportionOfWidth (0.1300f), 0, proportionOfWidth (0.1300f), 20);
    onControlButton->setBounds ((((((proportionOfWidth (0.0450f) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f)) + proportionOfWidth (0.1300f), 0, proportionOfWidth (0.1300f), 20);
    contentButton->setBounds (proportionOfWidth (0.0450f), 0, proportionOfWidth (0.1300f), 20);
    //[UserResized] Add your own custom resize handling here..


	
	int y = 28;

	int xOff = isRootEditor() ? 0 : 1;
	int w = getWidth() - 2*xOff;

	scriptContent->setVisible(getProcessor()->getEditorState(ScriptProcessor::contentShown));

	const int contentHeight = scriptContent->isVisible() ? scriptContent->getContentHeight() : 0;

	scriptContent->setBounds(xOff, y, w, scriptContent->getContentHeight());
	y += contentHeight;

	const bool fullscreen = getEditor()->isPopup() || isRootEditor();

	const int editorHeight = fullscreen ? getHeight() - y - 24 : codeEditor->currentHeight;

	codeEditor->setSize(w, editorHeight);

	if(!editorShown)
	{
		codeEditor->setBounds(0, y, getWidth(), codeEditor->getHeight());

		codeEditor->setTopLeftPosition(0, getHeight());
		compileButton->setTopLeftPosition(0, getHeight());
		messageBox->setTopLeftPosition(0, getHeight());
		timeLabel->setTopLeftPosition(0, getHeight());
	}
	else
	{
		codeEditor->setBounds(xOff, y, getWidth()-2*xOff, codeEditor->getHeight());

		y += codeEditor->getHeight();

		messageBox->setTopLeftPosition(1, y);

		compileButton->setTopLeftPosition(codeEditor->getRight() - compileButton->getWidth(), y);
	}

    //[/UserResized]
}

void ScriptingEditor::buttonClicked (Button* buttonThatWasClicked)
{
    //[UserbuttonClicked_Pre]

	ScriptProcessor *s = static_cast<ScriptProcessor*>(getProcessor());

    //[/UserbuttonClicked_Pre]

    if (buttonThatWasClicked == compileButton)
    {
        //[UserButtonCode_compileButton] -- add your button handler code here..

		
		compileScript();

		

		return;

        //[/UserButtonCode_compileButton]
    }
    else if (buttonThatWasClicked == noteOnButton)
    {
        //[UserButtonCode_noteOnButton] -- add your button handler code here..

		saveLastCallback();
		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(ScriptProcessor::onNoteOn), tokenizer, dynamic_cast<ScriptProcessor*>(getProcessor())));
		goToSavedPosition(ScriptProcessor::Callback::onNoteOn);

        //[/UserButtonCode_noteOnButton]
    }
    else if (buttonThatWasClicked == noteOffButton)
    {
        //[UserButtonCode_noteOffButton] -- add your button handler code here..

		saveLastCallback();
		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(ScriptProcessor::onNoteOff), tokenizer, dynamic_cast<ScriptProcessor*>(getProcessor())));
		goToSavedPosition(ScriptProcessor::Callback::onNoteOff);

        //[/UserButtonCode_noteOffButton]
    }
    else if (buttonThatWasClicked == onControllerButton)
    {
        //[UserButtonCode_onControllerButton] -- add your button handler code here..
		
		saveLastCallback();
		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(ScriptProcessor::onController), tokenizer, dynamic_cast<ScriptProcessor*>(getProcessor())));
		goToSavedPosition(ScriptProcessor::Callback::onController);

        //[/UserButtonCode_onControllerButton]
    }
    else if (buttonThatWasClicked == onTimerButton)
    {
        //[UserButtonCode_onTimerButton] -- add your button handler code here..
		
		saveLastCallback();
		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(ScriptProcessor::onTimer), tokenizer, dynamic_cast<ScriptProcessor*>(getProcessor())));
		goToSavedPosition(ScriptProcessor::Callback::onTimer);

        //[/UserButtonCode_onTimerButton]
    }
    else if (buttonThatWasClicked == onInitButton)
    {
        //[UserButtonCode_onInitButton] -- add your button handler code here..
		
		saveLastCallback();
		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(ScriptProcessor::onInit), tokenizer, dynamic_cast<ScriptProcessor*>(getProcessor())));
		goToSavedPosition(ScriptProcessor::Callback::onInit);

        //[/UserButtonCode_onInitButton]
    }
    else if (buttonThatWasClicked == onControlButton)
    {
        //[UserButtonCode_onControlButton] -- add your button handler code here..
		
		saveLastCallback();
		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(ScriptProcessor::onControl), tokenizer, dynamic_cast<ScriptProcessor*>(getProcessor())));
		goToSavedPosition(ScriptProcessor::Callback::onControl);

        //[/UserButtonCode_onControlButton]
    }
    else if (buttonThatWasClicked == contentButton)
    {
        //[UserButtonCode_contentButton] -- add your button handler code here..

		getProcessor()->setEditorState(ScriptProcessor::contentShown, !toggleButton(contentButton));

		refreshBodySize();
		return;

        //[/UserButtonCode_contentButton]
    }

    //[UserbuttonClicked_Post]



	if(buttonThatWasClicked->getToggleState())
	{
		editorShown = false;

		noteOnButton->setToggleState(false, dontSendNotification);
		noteOffButton->setToggleState(false, dontSendNotification);
		onControllerButton->setToggleState(false, dontSendNotification);
		onInitButton->setToggleState(false, dontSendNotification);
		onTimerButton->setToggleState(false, dontSendNotification);
		onControlButton->setToggleState(false, dontSendNotification);

		getProcessor()->setEditorState(ScriptProcessor::onInitOpen, false);
		getProcessor()->setEditorState(ScriptProcessor::onNoteOnOpen, false);
		getProcessor()->setEditorState(ScriptProcessor::onNoteOffOpen, false);
		getProcessor()->setEditorState(ScriptProcessor::onControllerOpen, false);
		getProcessor()->setEditorState(ScriptProcessor::onControlOpen, false);
		getProcessor()->setEditorState(ScriptProcessor::onTimerOpen, false);
	}
	else
	{
		editorShown = true;

		onInitButton->setToggleState(buttonThatWasClicked == onInitButton, dontSendNotification);
		noteOnButton->setToggleState(buttonThatWasClicked == noteOnButton, dontSendNotification);
		noteOffButton->setToggleState(buttonThatWasClicked == noteOffButton, dontSendNotification);
		onControllerButton->setToggleState(buttonThatWasClicked == onControllerButton, dontSendNotification);
		onTimerButton->setToggleState(buttonThatWasClicked == onTimerButton, dontSendNotification);
		onControlButton->setToggleState(buttonThatWasClicked == onControlButton, dontSendNotification);

		getProcessor()->setEditorState(ScriptProcessor::onInitOpen, buttonThatWasClicked == onInitButton);
		getProcessor()->setEditorState(ScriptProcessor::onNoteOnOpen, buttonThatWasClicked == noteOnButton);
		getProcessor()->setEditorState(ScriptProcessor::onNoteOffOpen, buttonThatWasClicked == noteOffButton);
		getProcessor()->setEditorState(ScriptProcessor::onControllerOpen, buttonThatWasClicked == onControllerButton);
		getProcessor()->setEditorState(ScriptProcessor::onControlOpen, buttonThatWasClicked == onControlButton);
		getProcessor()->setEditorState(ScriptProcessor::onTimerOpen, buttonThatWasClicked == onTimerButton);

		buttonThatWasClicked->setToggleState(true, dontSendNotification);
	}

	resized();

	refreshBodySize();

	codeEditor->editor->setFont(GLOBAL_MONOSPACE_FONT());


    //[/UserbuttonClicked_Post]
}




//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...

void ScriptingEditor::goToSavedPosition(ScriptProcessor::Callback newCallback)
{
	if (newCallback != ScriptProcessor::Callback::numCallbacks)
	{
		codeEditor->editor->moveCaretTo(*lastPositions[newCallback], false);
		codeEditor->editor->scrollToColumn(0);
		codeEditor->editor->grabKeyboardFocus();
	}
}

void ScriptingEditor::saveLastCallback()
{
	ScriptProcessor::Callback lastCallback = getActiveCallback();
	if (lastCallback != ScriptProcessor::Callback::numCallbacks)
	{
		lastPositions[lastCallback]->setPosition(codeEditor->editor->getCaretPos().getPosition());
	}
}

void ScriptingEditor::setEditedScriptComponent(DynamicObject *component)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(component);

	scriptContent->setEditedScriptComponent(sc);

	if(component != nullptr)
	{
		onInitButton->setToggleState(false, dontSendNotification);
		buttonClicked(onInitButton);

		codeEditor->editor->selectText(sc->getName());
	}
}

void ScriptingEditor::scriptComponentChanged(DynamicObject *scriptComponent, Identifier /*propertyThatWasChanged*/)
{
	if (!onInitButton->getToggleState()) return;

	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(scriptComponent);

	if (sc != nullptr)
	{
		if (!codeEditor->editor->selectText(sc->getName()))
		{
			codeEditor->editor->selectLineAfterDefinition(sc->getName());
		}
		codeEditor->editor->insertTextAtCaret(CodeDragger::getText(sc));

		codeEditor->editor->selectText(sc->getName());
	}
}

void ScriptingEditor::mouseDown(const MouseEvent &e)
{
	if (e.mods.isLeftButtonDown() && useComponentSelectMode)
	{
		ScriptingApi::Content::ScriptComponent *sc = scriptContent->getScriptComponentFor(e.getEventRelativeTo(scriptContent).getPosition());

		getProcessor()->getMainController()->setEditedScriptComponent(sc, this);
	}

	if(editorShown && e.mods.isRightButtonDown())
	{
		PopupMenu m;
		ScopedPointer<PopupLookAndFeel> luf = new PopupLookAndFeel();
		m.setLookAndFeel(luf);

		m.addSectionHeader("Export whole script:");
		m.addItem(1, "Save Script To File");
		m.addItem(2, "Load Script From File");
		m.addSeparator();
		m.addItem(3, "Save Script to Clipboard");
		m.addItem(4, "Load Script from Clipboard");

		m.addSeparator();

		PopupMenu widgets;

		enum Widgets
		{
			Knob = 0x1000,
			Button,
			Table,
			ComboBox,
			Label,
			Image,
			Plotter,
			ModulatorMeter,
			Panel,
			AudioWaveform,
			SliderPack,
			duplicateWidget,
			numWidgets
		};

		widgets.addItem(Knob, "Add new Slider");
		widgets.addItem(Button, "Add new Button");
		widgets.addItem(Table, "Add new Table");
		widgets.addItem(ComboBox, "Add new ComboBox");
		widgets.addItem(Label, "Add new Label");
		widgets.addItem(Image, "Add new Image");
		widgets.addItem(Plotter, "Add new Plotter");
		widgets.addItem(ModulatorMeter, "Add new ModulatorMeter");
		widgets.addItem(Panel, "Add new Panel");
		widgets.addItem(AudioWaveform, "Add new AudioWaveform");
		widgets.addItem(SliderPack, "Add new SliderPack");

		m.addSubMenu("Add new Control", widgets, true);

		m.addItem(duplicateWidget, "Duplicate selected component", scriptContent->getEditedComponent() != nullptr);

		m.addItem(numWidgets, "Enable Component Select Mode", true, useComponentSelectMode);

		ScriptingApi::Content::ScriptComponent *sc = scriptContent->getScriptComponentFor(e.getEventRelativeTo(scriptContent).getPosition());

		if(sc != nullptr)
		{
			m.addSeparator();
			m.addItem(5, "Edit \"" + sc->getName().toString() + "\" in Panel");
	}

		int result = m.show();

		ScriptProcessor *s = static_cast<ScriptProcessor*>(getProcessor());

		if (result == 1) // SAVE
		{
			FileChooser scriptSaver("Save script as",
				File(GET_PROJECT_HANDLER(s).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
				"*.js");

			if (scriptSaver.browseForFileToSave(true))
			{
				String script;
				s->mergeCallbacksToScript(script);
				scriptSaver.getResult().replaceWithText(script);
				debugToConsole(getProcessor(), "Script saved to " + scriptSaver.getResult().getFullPathName());
			}
		}
		else if (result == 2) // LOAD
		{
			FileChooser scriptLoader("Please select the script you want to load",
				File(GET_PROJECT_HANDLER(s).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
				"*.js");

			if (scriptLoader.browseForFileToOpen())
			{
				String script = scriptLoader.getResult().loadFileAsString().removeCharacters("\r");
				s->parseSnippetsFromString(script);
				compileScript();
				debugToConsole(getProcessor(), "Script loaded from " + scriptLoader.getResult().getFullPathName());
			}
		}
		else if (result == 3) // COPY
		{
			String x;
			s->mergeCallbacksToScript(x);
			SystemClipboard::copyTextToClipboard(x);

			debugToConsole(getProcessor(), "Script exported to Clipboard.");
		}
		else if (result == 4) // PASTE
		{
			String x = String(SystemClipboard::getTextFromClipboard()).removeCharacters("\r");

			if (x.containsNonWhitespaceChars() && AlertWindow::showOkCancelBox(AlertWindow::QuestionIcon, "Replace Script?", "Do you want to replace the script?"))
			{
				s->parseSnippetsFromString(x);
				compileScript();
			}
		}
		else if (result == 5) // EDIT IN PANEL
		{
			jassert(sc != nullptr);

			getProcessor()->getMainController()->setEditedScriptComponent(sc, this);
		}
		else if (result >= Knob && result < numWidgets)
		{
			String widgetType;

			switch (result)
			{
			case Knob:				widgetType = "Knob"; break;
			case Button:			widgetType = "Button"; break;
			case Table:				widgetType = "Table"; break;
			case ComboBox:			widgetType = "ComboBox"; break;
			case Label:				widgetType = "Label"; break;
			case Image:				widgetType = "Image"; break;
			case Plotter:			widgetType = "Plotter"; break;
			case ModulatorMeter:	widgetType = "ModulatorMeter"; break;
			case Panel:				widgetType = "Panel"; break;
			case AudioWaveform:		widgetType = "AudioWaveform"; break;
			case SliderPack:		widgetType = "SliderPack"; break;
			case duplicateWidget:
			{
				widgetType = scriptContent->getEditedComponent()->getObjectName().toString();
				widgetType = widgetType.replace("Scripted", "");
				widgetType = widgetType.replace("Script", "");
				widgetType = widgetType.replace("Slider", "Knob");
				break;
			}
			}

			String id = PresetHandler::getCustomName(widgetType);

			bool wrongName = id.isEmpty() || !Identifier::isValidIdentifier(id);

			while (wrongName && PresetHandler::showYesNoWindow("Wrong variable name", "Press 'OK' to re-enter a valid variable name (no funky characters, no whitespace) or 'Cancel' to abort"))
			{
				id = PresetHandler::getCustomName(widgetType);
				wrongName = id.isEmpty() || !Identifier::isValidIdentifier(id);
			}

			if (wrongName) return;

			String textToInsert;

			int insertX = e.getEventRelativeTo(scriptContent).getMouseDownPosition().getX();
			int insertY = e.getEventRelativeTo(scriptContent).getMouseDownPosition().getY();

			textToInsert << "\n" << id << " = Content.add" << widgetType << "(\"" << id << "\", " << insertX << ", " << insertY << ");\n";

			if (result == duplicateWidget)
			{

				const int xOfOriginal = scriptContent->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::x);
				const int yOfOriginal = scriptContent->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::x);

				const String originalId = scriptContent->getEditedComponent()->getName().toString();

				String jsonDataOfNewComponent = CodeDragger::getText(scriptContent->getEditedComponent());

				jsonDataOfNewComponent = jsonDataOfNewComponent.replace(originalId, id); // change the id of the component
				jsonDataOfNewComponent = jsonDataOfNewComponent.replace("\"x\": " + String(xOfOriginal), "\"x\": " + String(insertX)); // change the id of the component
				jsonDataOfNewComponent = jsonDataOfNewComponent.replace("\"y\": " + String(yOfOriginal), "\"y\": " + String(insertY)); // change the id of the component

				textToInsert << jsonDataOfNewComponent;
			}

			if (getActiveCallback() != ScriptProcessor::Callback::onInit)
			{
				buttonClicked(onInitButton); // we need synchronous execution here
			}

			codeEditor->editor->moveCaretToEnd(false);

			codeEditor->editor->insertTextAtCaret(textToInsert);
			compileScript();
		}
		else if (result == numWidgets) // Component Select Mode
		{
			useComponentSelectMode = !useComponentSelectMode;

			scriptContent->setInterceptsMouseClicks(false, !useComponentSelectMode);
		}
}
}

bool ScriptingEditor::keyPressed(const KeyPress &k)
{
	if (k.isKeyCode(KeyPress::F3Key) || k.isKeyCode(KeyPress::F2Key))
	{
		int current = getActiveCallback();
		if(k.isKeyCode(KeyPress::F2Key)) current--;
		else							 current++;

		if (current >= ScriptProcessor::onInit && current < ScriptProcessor::Callback::numCallbacks)
		{
			getSnippetButton((ScriptProcessor::Callback)current)->triggerClick();
			return true;
		}
	}

	if (k.isKeyCode(KeyPress::F1Key))
	{
		contentButton->triggerClick();
		return true;
	}
	if (k.isKeyCode('1') && k.getModifiers().isCtrlDown())
	{
		onInitButton->triggerClick();
		return true;
	}
	else if(k.isKeyCode('2') && k.getModifiers().isCtrlDown())
	{
		noteOnButton->triggerClick();
		return true;
	}
	else if (k.isKeyCode('3') && k.getModifiers().isCtrlDown())
	{
		noteOffButton->triggerClick();
		return true;
	}
	else if (k.isKeyCode('4') && k.getModifiers().isCtrlDown())
	{
		onControllerButton->triggerClick();
		return true;
	}
	else if (k.isKeyCode('5') && k.getModifiers().isCtrlDown())
	{
		onTimerButton->triggerClick();
		return true;
	}
	else if (k.isKeyCode('6') && k.getModifiers().isCtrlDown())
	{
		onControlButton->triggerClick();
		return true;
	}

	return false;
}

void ScriptingEditor::compileScript()
{
	ScriptProcessor *s = static_cast<ScriptProcessor*>(getProcessor());

	DynamicObject *component = s->checkContentChangedInPropertyPanel();

	if (component != nullptr)
	{
		if (!PresetHandler::showYesNoWindow("Discard changed properties?", "There are some properties for the component " + String(dynamic_cast<ScriptingApi::Content::ScriptComponent*>(component)->getName().toString()) + " that are not saved. Press OK to discard these changes or Cancel to abort compiling"))
		{
			getProcessor()->getMainController()->setEditedScriptComponent(component, this);
			return;
		}
	}

	getProcessor()->getMainController()->setEditedScriptComponent(nullptr, this);




	ScriptProcessor::SnippetResult resultMessage = static_cast<ScriptProcessor*>(getProcessor())->compileScript();

	double x = static_cast<ScriptProcessor*>(getProcessor())->getLastExecutionTime();
	timeLabel->setText(String(x, 3) + " ms", dontSendNotification);

	if(resultMessage.r.wasOk()) messageBox->setText("Compiled OK", false);
	else
	{
		messageBox->setText(s->getSnippet(resultMessage.c)->getCallbackName().toString() + "() - " + resultMessage.r.getErrorMessage(), false);
	}

	checkActiveSnippets();

	refreshBodySize();

	repaint();
}

ScriptProcessor::Callback ScriptingEditor::getActiveCallback() const
{
	if (codeEditor == nullptr) return ScriptProcessor::Callback::numCallbacks;

	const ScriptProcessor *sp = dynamic_cast<const ScriptProcessor*>(getProcessor());

	const CodeDocument &doc = codeEditor->editor->getDocument();

	for (int i = 0; i < ScriptProcessor::Callback::numCallbacks; i++)
	{
		if (&doc == sp->getSnippet(i))
		{
			return (ScriptProcessor::Callback)i;
		}
	}

	return ScriptProcessor::Callback::numCallbacks;
	
}
