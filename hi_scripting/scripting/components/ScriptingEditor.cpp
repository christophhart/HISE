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

#include <regex>




//==============================================================================
ScriptingEditor::ScriptingEditor (ProcessorEditor *p)
    : ProcessorEditorBody(p),
      doc (dynamic_cast<JavascriptProcessor*>(getProcessor())->getDocument()),
      tokenizer(new JavascriptTokeniser())
{
	JavascriptProcessor *sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

    addAndMakeVisible (codeEditor = new CodeEditorWrapper (*doc, tokenizer, sp));
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
	messageBox->addMouseListener(this, true);

    addAndMakeVisible (timeLabel = new Label ("new label",
                                              TRANS("2.5 microseconds")));
    timeLabel->setFont (GLOBAL_BOLD_FONT());
    timeLabel->setJustificationType (Justification::centredLeft);
    timeLabel->setEditable (false, false, false);
    timeLabel->setColour (Label::textColourId, Colours::white);
    timeLabel->setColour (TextEditor::textColourId, Colours::black);
    timeLabel->setColour (TextEditor::backgroundColourId, Colour (0x00000000));

	addAndMakeVisible(contentButton = new TextButton("new button"));
	contentButton->setButtonText(TRANS("Content"));
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
		addAndMakeVisible(b);
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

	callbackButtons.getLast()->setConnectedEdges(TextButton::ConnectedEdgeFlags::ConnectedOnLeft);

    timeLabel->setFont (GLOBAL_BOLD_FONT());

	messageBox->setFont(GLOBAL_MONOSPACE_FONT());
	
	doc = sp->getDocument();

	addAndMakeVisible(scriptContent = new ScriptContentComponent(dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())));

	messageBox->setLookAndFeel(&laf2);

	messageBox->setColour(Label::ColourIds::textColourId, Colours::white);

	useComponentSelectMode = false;

	compileButton->setLookAndFeel(&alaf);

	if (dynamic_cast<JavascriptMidiProcessor*>(getProcessor()))
	{
		lastPositions.add(new CodeDocument::Position(*sp->getSnippet(JavascriptMidiProcessor::Callback::onInit), 0));
		lastPositions.add(new CodeDocument::Position(*sp->getSnippet(JavascriptMidiProcessor::Callback::onNoteOn), 23));
		lastPositions.add(new CodeDocument::Position(*sp->getSnippet(JavascriptMidiProcessor::Callback::onNoteOff), 24));
		lastPositions.add(new CodeDocument::Position(*sp->getSnippet(JavascriptMidiProcessor::Callback::onController), 27));
		lastPositions.add(new CodeDocument::Position(*sp->getSnippet(JavascriptMidiProcessor::Callback::onTimer), 22));
		lastPositions.add(new CodeDocument::Position(*sp->getSnippet(JavascriptMidiProcessor::Callback::onControl), 37));
	}
	
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
	callbackButtons.clear();
    contentButton = nullptr;

	lastPositions.clear();
   
}

int ScriptingEditor::getBodyHeight() const
{
	if (isRootEditor())
	{
		return findParentComponentOfClass<Viewport>()->getHeight() - 36;
	}

	int editorOffset = dynamic_cast<const ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

	const int contentHeight = getProcessor()->getEditorState(editorOffset + ProcessorWithScriptingContent::EditorStates::contentShown) ? scriptContent->getContentHeight() : 0;

	const int additionalOffset = (dynamic_cast<const JavascriptModulatorSynth*>(getProcessor()) != nullptr) ? 5 : 0;

	if (editorShown)
	{
		return 28 + additionalOffset + contentHeight + codeEditor->getHeight() + 24;
	}
	else
	{
		return 28 + additionalOffset + contentHeight;
	}
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
		
		Colour c = dynamic_cast<JavascriptProcessor*>(getProcessor())->wasLastCompileOK() ? Colour(0xff323832) : Colour(0xff383232);

		float y = (float)codeEditor->getBottom();

		g.setGradientFill(ColourGradient(c.withMultipliedBrightness(0.8f), 0.0f, y, c, 0.0f, y+6.0f, false));

		g.fillRect(1, codeEditor->getBottom(), getWidth()-2, compileButton->getHeight());

		
	}

    //[/UserPaint]
}

void ScriptingEditor::resized()
{
    
    codeEditor->setBounds ((getWidth() / 2) - ((getWidth() - 90) / 2), 104, getWidth() - 90, getHeight() - 140);
    compileButton->setBounds (((getWidth() / 2) - ((getWidth() - 90) / 2)) + (getWidth() - 90) - 95, getHeight() - 24, 95, 24);
    messageBox->setBounds (((getWidth() / 2) - ((getWidth() - 90) / 2)) + 0, getHeight() - 24, getWidth() - 296, 24);
    
	int buttonWidth = (getWidth()-20) / (callbackButtons.size() + 1);
	int buttonX = 10;

	contentButton->setBounds(buttonX, 0, buttonWidth, 20);
	buttonX = contentButton->getRight();

	for (int i = 0; i < callbackButtons.size(); i++)
	{
		callbackButtons[i]->setBounds(buttonX, 0, buttonWidth, 20);
		buttonX = callbackButtons[i]->getRight();
	}

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
}

void ScriptingEditor::buttonClicked (Button* buttonThatWasClicked)
{
    
	JavascriptProcessor *s = dynamic_cast<JavascriptProcessor*>(getProcessor());

	int editorOffset = dynamic_cast<const ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

	int callbackIndex = callbackButtons.indexOf(dynamic_cast<TextButton*>(buttonThatWasClicked));

	if (callbackIndex != -1)
	{
		saveLastCallback();

		addAndMakeVisible(codeEditor = new CodeEditorWrapper(*s->getSnippet(callbackIndex), tokenizer, dynamic_cast<JavascriptProcessor*>(getProcessor())));
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
			codeEditor->editor->moveCaretTo(*lastPositions[newCallback], false);
		}
		
		codeEditor->editor->scrollToColumn(0);
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
			lastPositions[lastCallback]->setPosition(codeEditor->editor->getCaretPos().getPosition());
		}
	}
}

void ScriptingEditor::setEditedScriptComponent(ReferenceCountedObject* component)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(component);

	scriptContent->setEditedScriptComponent(sc);

	if(component != nullptr)
	{
		callbackButtons[0]->setToggleState(false, dontSendNotification);
		buttonClicked(callbackButtons[0]);

		codeEditor->editor->selectJSONTag(sc->getName());
	}
}

void ScriptingEditor::scriptComponentChanged(ReferenceCountedObject* scriptComponent, Identifier /*propertyThatWasChanged*/)
{
	if (!callbackButtons[0]->getToggleState()) return;

	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(scriptComponent);

	if (sc != nullptr)
	{
		if (!codeEditor->editor->selectJSONTag(sc->getName()))
		{
			codeEditor->editor->selectLineAfterDefinition(sc->getName());
		}
		codeEditor->editor->insertTextAtCaret(CodeDragger::getText(sc));

		codeEditor->editor->selectJSONTag(sc->getName());
	}
}

class ParameterConnector: public ThreadWithAsyncProgressWindow,
						  public ComboBoxListener
{
public:

	ParameterConnector(ScriptingApi::Content::ScriptComponent *sc_, ScriptingEditor *editor_) :
		ThreadWithAsyncProgressWindow("Connect widget to module parameter"),
		sc(sc_),
		editor(editor_),
		sp(dynamic_cast<JavascriptMidiProcessor*>(editor_->getProcessor())),
		processorToAdd(nullptr),
		parameterIndexToAdd(-1)
	{
		if (sp != nullptr)
		{
			Processor::Iterator<Processor> boxIter(sp->getOwnerSynth(), false);


			while (Processor *p = boxIter.getNextProcessor())
			{
				if (dynamic_cast<Chain*>(p) != nullptr) continue;

				processorList.add(p);
			}

			StringArray processorIdList;

			for (int i = 0; i < processorList.size(); i++)
			{
				processorIdList.add(processorList[i]->getId());
			}

			addComboBox("Processors", processorIdList, "Module");

			getComboBoxComponent("Processors")->addListener(this);
			addComboBox("Parameters", StringArray(), "Parameters");

			getComboBoxComponent("Parameters")->addListener(this);
			getComboBoxComponent("Parameters")->setTextWhenNothingSelected("Choose a module");

			addBasicComponents();

			showStatusMessage("Choose a module and its parameter and press OK");
		}
		else
		{
			jassertfalse;
		}

		
	}

	void comboBoxChanged(ComboBox* comboBoxThatHasChanged)
	{
		if (comboBoxThatHasChanged->getName() == "Processors")
		{
			Processor *selectedProcessor = processorList[comboBoxThatHasChanged->getSelectedItemIndex()];

			ComboBox *parameterBox = getComboBoxComponent("Parameters");

			parameterBox->clear();

			for (int i = 0; i < selectedProcessor->getNumParameters(); i++)
			{
				parameterBox->addItem(selectedProcessor->getIdentifierForParameterIndex(i).toString(), i+1);
			}

			setProgress(0.5);
		}
		else if (comboBoxThatHasChanged->getName() == "Parameters")
		{
			processorToAdd = processorList[getComboBoxComponent("Processors")->getSelectedItemIndex()];
			parameterIndexToAdd = comboBoxThatHasChanged->getSelectedItemIndex();

			setProgress(1.0);

			showStatusMessage("Press OK to add the connection code to this script.");
		}
	}

	void run() override
	{
		
	};

	void threadFinished() override
	{
		if (processorToAdd == nullptr || parameterIndexToAdd == -1) return;

		String onInitText = sp->getSnippet(JavascriptMidiProcessor::onInit)->getAllContent();
		String declaration = ProcessorHelpers::getScriptVariableDeclaration(processorToAdd, false);
		String processorId = declaration.upToFirstOccurrenceOf(" ", false, false);

		if (!onInitText.contains(declaration))
		{
			onInitText << "\n" << declaration << "\n";
			sp->getSnippet(JavascriptMidiProcessor::onInit)->replaceAllContent(onInitText);
		}

		String onControlText = sp->getSnippet(JavascriptMidiProcessor::onControl)->getAllContent();
		const String controlStatement = ("if(number == ") + sc->getName() + ")";
        
        const String body = processorId + ".setAttribute(" + processorId + "." + processorToAdd->getIdentifierForParameterIndex(parameterIndexToAdd).toString() + ", value);\n\n";

        if(onControlText.contains(controlStatement))
        {
            const String replaceString = onControlText.fromFirstOccurrenceOf(controlStatement, true, false).upToFirstOccurrenceOf("\n", true, false);
            
            
            
            sp->getSnippet(JavascriptMidiProcessor::onControl)->replaceAllContent(onControlText.replace(replaceString, controlStatement + " " + body));
            
            editor->compileScript();
        }
        else
        {

            
            sp->getSnippet(JavascriptMidiProcessor::onControl)->replaceAllContent(onControlText.upToLastOccurrenceOf("}", false, false) +             (onControlText.contains("if(") ? "\telse " : "\t") + controlStatement + " " + body + onControlText.fromLastOccurrenceOf("}", true, false));
            
            editor->compileScript();
        }
        
		

    };

private:

	Processor *processorToAdd;
	int parameterIndexToAdd;

	ScriptingApi::Content::ScriptComponent *sc;
	ScriptingEditor *editor;
	JavascriptMidiProcessor *sp;
	Array<WeakReference<Processor>> processorList;

};

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
			m.addItem(6, "Connect to Module Parameter");
		}

		int result = m.show();

		JavascriptProcessor *s = dynamic_cast<JavascriptProcessor*>(getProcessor());

		if (result == 1) // SAVE
		{
			FileChooser scriptSaver("Save script as",
				File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
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
				File(GET_PROJECT_HANDLER(getProcessor()).getSubDirectory(ProjectHandler::SubDirectories::Scripts)),
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
		else if (result == 6)
		{
			jassert(sc != nullptr);

			if (ProcessorHelpers::is<JavascriptMidiProcessor>(getProcessor()))
			{
				ParameterConnector *comp = new ParameterConnector(sc, this);

				comp->setModalBaseWindowComponent(findParentComponentOfClass<BackendProcessorEditor>());
			}
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

			while (wrongName && PresetHandler::showYesNoWindow("Wrong variable name", "Press 'OK' to re-enter a valid variable name (no funky characters, no whitespace) or 'Cancel' to abort", PresetHandler::IconType::Warning))
			{
				id = PresetHandler::getCustomName(widgetType);
				wrongName = id.isEmpty() || !Identifier::isValidIdentifier(id);
			}

			if (wrongName) return;

			String textToInsert;

			int insertX = e.getEventRelativeTo(scriptContent).getMouseDownPosition().getX();
			int insertY = e.getEventRelativeTo(scriptContent).getMouseDownPosition().getY();

			textToInsert << "\nconst var " << id << " = Content.add" << widgetType << "(\"" << id << "\", " << insertX << ", " << insertY << ");\n";

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

			if (getActiveCallback() != 0)
			{
				buttonClicked(callbackButtons[0]); // we need synchronous execution here
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

void ScriptingEditor::mouseDoubleClick(const MouseEvent& e)
{
	if (e.eventComponent == messageBox)
	{
		const String fileName = ApiHelpers::getFileNameFromErrorMessage(messageBox->getText());

		if (fileName.isNotEmpty())
		{
			JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

			for (int i = 0; i < sp->getNumWatchedFiles(); i++)
			{
				if (sp->getWatchedFile(i).getFileName() == fileName)
				{
					sp->showPopupForFile(i);
				}
			}

		}
	}
}

bool ScriptingEditor::keyPressed(const KeyPress &k)
{
#if JUCE_WINDOWS
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

void ScriptingEditor::compileScript()
{
	ProcessorWithScriptingContent *s = dynamic_cast<ProcessorWithScriptingContent*>(getProcessor());
	JavascriptProcessor* jsp = dynamic_cast<JavascriptProcessor*>(getProcessor());

	ReferenceCountedObject *component = s->checkContentChangedInPropertyPanel();

	if (component != nullptr)
	{
		if (!PresetHandler::showYesNoWindow("Discard changed properties?", "There are some properties for the component " + String(dynamic_cast<ScriptingApi::Content::ScriptComponent*>(component)->getName().toString()) + " that are not saved. Press OK to discard these changes or Cancel to abort compiling", PresetHandler::IconType::Warning))
		{
			getProcessor()->getMainController()->setEditedScriptComponent(component, this);
			return;
		}
	}

	getProcessor()->getMainController()->setEditedScriptComponent(nullptr, this);

	JavascriptProcessor::SnippetResult resultMessage = jsp->compileScript();

	double x = jsp->getLastExecutionTime();
	timeLabel->setText(String(x, 3) + " ms", dontSendNotification);

	if(resultMessage.r.wasOk()) messageBox->setText("Compiled OK", false);
	else
	{
		const String errorMessage = resultMessage.r.getErrorMessage();

		if (errorMessage.startsWith("Line"))
		{
			messageBox->setText(jsp->getSnippet(resultMessage.c)->getCallbackName().toString() + "() - " + errorMessage, false);
		}
		else
		{
			messageBox->setText(errorMessage, false);
		}

		TextButton* b = callbackButtons[resultMessage.c];
		
		if (b != nullptr && !b->getToggleState())
			buttonClicked(b);

		StringArray errorPosition = RegexFunctions::getMatches(".*Line ([0-9]*), column ([0-9]*)", errorMessage);

		int l = errorPosition[1].getIntValue();
		int c = errorPosition[2].getIntValue();

		CodeDocument::Position pos = CodeDocument::Position(codeEditor->editor->getDocument(), l-1, c-1);

		codeEditor->editor->moveCaretTo(pos, false);
	}

    PresetHandler::setChanged(getProcessor());
    
	checkActiveSnippets();
	refreshBodySize();
	repaint();
}

int ScriptingEditor::getActiveCallback() const
{
	const JavascriptProcessor *sp = dynamic_cast<const JavascriptProcessor*>(getProcessor());

	if (codeEditor == nullptr) return sp->getNumSnippets();

	const CodeDocument &doc = codeEditor->editor->getDocument();

	for (int i = 0; i < sp->getNumSnippets(); i++)
	{
		if (&doc == sp->getSnippet(i))
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
