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
      doc (new CodeDocument()),
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
	
	addAndMakeVisible(scriptContent = new ScriptContentComponent(dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())));

    scriptContent->addMouseListenersForComponentWrappers();
    
	messageBox->setLookAndFeel(&laf2);

	messageBox->setColour(Label::ColourIds::textColourId, Colours::white);

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
	
	addAndMakeVisible(dragOverlay = new DragOverlay());

    setSize (800, 500);

	

	editorInitialized();
}

ScriptingEditor::~ScriptingEditor()
{
   
	getProcessor()->getMainController()->setEditedScriptComponent(nullptr, nullptr);

	
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

void ScriptingEditor::createNewComponent(Widgets componentType, int x, int y)
{
	String widgetType;

	switch (componentType)
	{
	case Widgets::Knob:				widgetType = "Knob"; break;
	case Widgets::Button:			widgetType = "Button"; break;
	case Widgets::Table:				widgetType = "Table"; break;
	case Widgets::ComboBox:			widgetType = "ComboBox"; break;
	case Widgets::Label:				widgetType = "Label"; break;
	case Widgets::Image:				widgetType = "Image"; break;
	case Widgets::Plotter:			widgetType = "Plotter"; break;
	case Widgets::ModulatorMeter:	widgetType = "ModulatorMeter"; break;
	case Widgets::Panel:				widgetType = "Panel"; break;
	case Widgets::AudioWaveform:		widgetType = "AudioWaveform"; break;
	case Widgets::SliderPack:		widgetType = "SliderPack"; break;
	case Widgets::duplicateWidget:
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

	textToInsert << "\nconst var " << id << " = Content.add" << widgetType << "(\"" << id << "\", " << x << ", " << y << ");\n";

	if (componentType == Widgets::duplicateWidget)
	{
		const int xOfOriginal = scriptContent->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::x);
		const int yOfOriginal = scriptContent->getEditedComponent()->getScriptObjectProperty(ScriptingApi::Content::ScriptComponent::y);

		const String originalId = scriptContent->getEditedComponent()->getName().toString();

		String jsonDataOfNewComponent = CodeDragger::getText(scriptContent->getEditedComponent());

		jsonDataOfNewComponent = jsonDataOfNewComponent.replace(originalId, id); // change the id of the component
		jsonDataOfNewComponent = jsonDataOfNewComponent.replace("\"x\": " + String(xOfOriginal), "\"x\": " + String(x)); // change the id of the component
		jsonDataOfNewComponent = jsonDataOfNewComponent.replace("\"y\": " + String(yOfOriginal), "\"y\": " + String(y)); // change the id of the component

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

int ScriptingEditor::getBodyHeight() const
{
	if (isRootEditor())
	{
		return findParentComponentOfClass<Viewport>()->getHeight() - 36;
	}

	const ProcessorWithScriptingContent* pwsc = dynamic_cast<const ProcessorWithScriptingContent*>(getProcessor());

	int editorOffset = pwsc->getCallbackEditorStateOffset();

	const int contentHeight = getProcessor()->getEditorState(editorOffset + ProcessorWithScriptingContent::EditorStates::contentShown) ? pwsc->getScriptingContent()->getContentHeight() : 0;

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

//==============================================================================
void ScriptingEditor::paint (Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
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
			codeEditor->editor->moveCaretTo(CodeDocument::Position(codeEditor->editor->getDocument(), lastPositions[newCallback]), false);
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
			lastPositions.set(lastCallback, codeEditor->editor->getCaretPos().getPosition());
		}
	}
}

void ScriptingEditor::changePositionOfComponent(ScriptingApi::Content::ScriptComponent* sc, int newX, int newY)
{
	const String regexMonster = "(Content\\.add\\w+\\s*\\(\\s*\\\"(" + sc->getName().toString() +
		")\\\"\\s*,\\s*)(-?\\d+)(\\s*,\\s*)(-?\\d+)(\\s*\\);)|(create\\w+\\s*\\(\\s*\\\"(" + sc->getName().toString() +
		")\\\"\\s*,\\s*)(-?\\d+)(\\s*,\\s*)(-?\\d+)(\\s*.*\\);)";

	CodeDocument* onInitC = dynamic_cast<JavascriptProcessor*>(getProcessor())->getSnippet(0);

	String allText = onInitC->getAllContent();

	StringArray matches = RegexFunctions::getMatches(regexMonster, allText);

	const bool isContentDefinition = matches[1].isNotEmpty();
	const bool isInlineDefinition = matches[7].isNotEmpty();

	if ((isContentDefinition || isInlineDefinition) && matches.size() > 12)
	{
		const String oldLine = matches[0];
		String replaceLine;

		if (isContentDefinition)
		{
			replaceLine << matches[1] << String(newX) << matches[4] << String(newY) << matches[6];
		}
		else
		{

			replaceLine << matches[7] << String(newX) << matches[10] << String(newY) << matches[12];
		}

		const int start = allText.indexOf(oldLine);
		const int end = start + oldLine.length();

		onInitC->replaceSection(start, end, replaceLine);

		sc->setDefaultPosition(newX, newY);
	}
}

void ScriptingEditor::setEditedScriptComponent(ReferenceCountedObject* component)
{
	ScriptingApi::Content::ScriptComponent *sc = dynamic_cast<ScriptingApi::Content::ScriptComponent*>(component);

	Component* scc = scriptContent->setEditedScriptComponent(sc);

	dragOverlay->dragger->setDraggedControl(scc, sc);

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
						  public ComboBoxListener,
						  public Timer
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

			startTimer(50);
		}
		else
		{
			jassertfalse;
		}

		
	}

	void timerCallback()
	{
		selectProcessor();
		stopTimer();
	}

	void selectProcessor()
	{
		const String controlCode = sp->getSnippet(JavascriptMidiProcessor::onControl)->getAllContent();

		const String switchStatement = containsSwitchStatement(controlCode);

		if (switchStatement.isNotEmpty())
		{
			const String caseStatement = containsCaseStatement(switchStatement);

			if (switchStatement.isNotEmpty())
			{
				const String oldProcessorName = getOldProcessorName(caseStatement);
				const String oldParameterName = getOldParameterName(caseStatement, oldProcessorName).fromFirstOccurrenceOf(".", false, false);

				if (oldProcessorName.isNotEmpty())
				{
					ComboBox *b = getComboBoxComponent("Processors");

					for (int i = 0; i < b->getNumItems(); i++)
					{
						if (b->getItemText(i).removeCharacters(" \n\t\"\'!$%&/()") == oldProcessorName)
						{
							b->setSelectedItemIndex(i, dontSendNotification);
							comboBoxChanged(b);
							break;
						}
					}
				}

				if (oldParameterName.isNotEmpty())
				{
					ComboBox *b = getComboBoxComponent("Parameters");

					b->setText(oldParameterName, dontSendNotification);
					comboBoxChanged(b);
					
				}
			}
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
		String processorId = declaration.fromFirstOccurrenceOf("const var ", false, false).upToFirstOccurrenceOf(" ", false, false);

		if (!onInitText.contains(declaration))
		{
			onInitText << "\n" << declaration << "\n";
			sp->getSnippet(JavascriptMidiProcessor::onInit)->replaceAllContent(onInitText);
		}

		const String onControlText = sp->getSnippet(JavascriptMidiProcessor::onControl)->getAllContent();

		const String switchStatement = containsSwitchStatement(onControlText);

		if (switchStatement.isNotEmpty())
		{
			String caseStatement = containsCaseStatement(switchStatement);

			if (caseStatement.isNotEmpty())
			{
				modifyCaseStatement(caseStatement, processorId, processorToAdd);
			}
			else
			{
				int index = getCaseStatementIndex(onControlText);
				addCaseStatement(index, processorId, processorToAdd);
			}
		}
		else
		{
			addSwitchStatementWithCaseStatement(onControlText, processorId, processorToAdd);
		}

		editor->compileScript();
    };

private:

	String containsSwitchStatement(const String &controlCode)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(controlCode, "");

			int braceLevel = 0;

			it.match(TokenTypes::function);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::openParen);
			it.match(TokenTypes::identifier);

			const Identifier widgetParameterName = Identifier(it.currentValue);

			it.match(TokenTypes::comma);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::closeParen);
			it.match(TokenTypes::openBrace);

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentType == TokenTypes::switch_)
				{
					it.match(TokenTypes::switch_);
					it.match(TokenTypes::openParen);

					if (it.currentType == TokenTypes::identifier && Identifier(it.currentValue) == widgetParameterName)
					{
						it.match(TokenTypes::identifier);
						it.match(TokenTypes::closeParen);
						
						auto start = it.location.location;

						it.match(TokenTypes::openBrace);

						int braceLevel = 1;

						while (it.currentType != TokenTypes::eof && braceLevel > 0)
						{
							if (it.currentType == TokenTypes::openBrace) braceLevel++;
							else if (it.currentType == TokenTypes::closeBrace) braceLevel--;
							
							it.skip();
						}


						return String(start, it.location.location);
					}
				}

				it.skip();
			}

			return String::empty;
		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at parsing the control statement", error, PresetHandler::IconType::Error);

			return String::empty;
		}
	}

	String containsCaseStatement(const String &switchStatement)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(switchStatement, "");
			
			it.match(TokenTypes::openBrace);
			
			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentType == TokenTypes::case_)
				{
					it.match(TokenTypes::case_);

					const Identifier caseId = Identifier(it.currentValue.toString());

					it.match(TokenTypes::identifier);

					if (caseId == sc->getName())
					{
						it.match(TokenTypes::colon);

						auto start = it.location.location;

						while (it.currentType != TokenTypes::eof && it.currentType != TokenTypes::case_)
						{
							it.skip();
						}

						return String(start, it.location.location);
					}

				}
				
				it.skip();
				
			}

			return String::empty;

		}
		catch (String errorMessage)
		{
			PresetHandler::showMessageWindow("Error at parsing the case statement", errorMessage, PresetHandler::IconType::Error);

			return String::empty;
		}

	}

	String getOldProcessorName(const String &caseStatement)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(caseStatement, "");

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentValue == "setAttribute")
				{
					it.match(TokenTypes::identifier);
					it.match(TokenTypes::openParen);
					
					return it.currentValue.toString();

				}

				it.skip();
			}

			return String::empty;
		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at modifying the case statement", error, PresetHandler::IconType::Error);

			return String::empty;
		}
	}

	String getOldParameterName(const String &caseStatement, const String &processorName)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(caseStatement, "");

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentValue == processorName)
				{
					

					it.match(TokenTypes::identifier);
					it.match(TokenTypes::dot);

					if (it.currentValue == "setAttribute")
					{
						it.match(TokenTypes::identifier);
						it.match(TokenTypes::openParen);
						it.match(TokenTypes::identifier);
						it.match(TokenTypes::dot);

						return processorName + "." + it.currentValue.toString();
						break;
					}
				}

				it.skip();
			}

			return String::empty;

		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at modifying the case statement", error, PresetHandler::IconType::Error);
			return String::empty;
		}
	}

	void modifyCaseStatement(const String &caseStatement, String processorId, Processor * processorToAdd)
	{
		String newStatement = String(caseStatement);

		const String newParameterName = processorId + "." + processorToAdd->getIdentifierForParameterIndex(parameterIndexToAdd).toString();
		
		const String oldProcessorName = getOldProcessorName(caseStatement);
		const String oldParameterName = getOldParameterName(caseStatement, oldProcessorName);

		if (oldParameterName.isNotEmpty())
		{
			newStatement = caseStatement.replace(oldParameterName, newParameterName);
			newStatement = newStatement.replace(oldProcessorName, processorId);

			CodeDocument* doc = sp->getSnippet(JavascriptMidiProcessor::onControl);

			String allCode = doc->getAllContent();

			doc->replaceAllContent(allCode.replace(caseStatement, newStatement));
		}
	}

	void addCaseStatement(int &index, String processorId, Processor * processorToAdd)
	{
		String codeToInsert;

		codeToInsert << "\t\tcase " << sc->getName().toString() << ":\n\t\t{\n\t\t\t";
		codeToInsert << processorId << ".setAttribute(" << processorId << "." << processorToAdd->getIdentifierForParameterIndex(parameterIndexToAdd).toString();
		codeToInsert << ", value);\n";
		codeToInsert << "\t\t\tbreak;\n\t\t}\n";

		sp->getSnippet(JavascriptMidiProcessor::onControl)->insertText(index, codeToInsert);

		index += codeToInsert.length();
	}

	void addSwitchStatementWithCaseStatement(const String &onControlText, String processorId, Processor * processorToAdd)
	{
		const String switchStart = "\tswitch(number)\n\t{\n";

		const String switchEnd = "\t};\n";

		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(onControlText, "");

			int braceLevel = 0;

			it.match(TokenTypes::function);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::openParen);
			it.match(TokenTypes::identifier);

			const Identifier widgetParameterName = Identifier(it.currentValue);

			it.match(TokenTypes::comma);
			it.match(TokenTypes::identifier);
			it.match(TokenTypes::closeParen);
			it.match(TokenTypes::openBrace);

			int index = it.location.location - onControlText.getCharPointer();


			sp->getSnippet(JavascriptMidiProcessor::onControl)->insertText(index, switchStart);

			index += switchStart.length();

			addCaseStatement(index, processorId, processorToAdd);

			sp->getSnippet(JavascriptMidiProcessor::onControl)->insertText(index, switchEnd);


		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at adding the switch & case statement", error, PresetHandler::IconType::Error);
		}

		

	}

	int getCaseStatementIndex(const String &onControlText)
	{
		try
		{
			HiseJavascriptEngine::RootObject::TokenIterator it(onControlText, "");

			while (it.currentType != TokenTypes::eof)
			{
				if (it.currentType == TokenTypes::switch_)
				{
					it.match(TokenTypes::switch_);
					it.match(TokenTypes::openParen);
					
					if (it.currentValue == "number")
					{
						it.match(TokenTypes::identifier);


						it.match(TokenTypes::closeParen);
						it.match(TokenTypes::openBrace);

						int braceLevel = 1;

						while (it.currentType != TokenTypes::eof && braceLevel > 0)
						{
							if (it.currentType == TokenTypes::openBrace) braceLevel++;
							
							else if (it.currentType == TokenTypes::closeBrace)
							{
								braceLevel--;
								if (braceLevel == 0)
								{
									return it.location.location - onControlText.getCharPointer() - 1;
								}
							}

							it.skip();
						}

					}
				}

				it.skip();
			}

			return -1;

		}
		catch (String error)
		{
			PresetHandler::showMessageWindow("Error at finding the case statement location", error, PresetHandler::IconType::Error);
			return -1;
		}
	}






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
		
		const int editComponentOffset = 109;

		PopupMenu m;
		ScopedPointer<PopupLookAndFeel> luf = new PopupLookAndFeel();
		m.setLookAndFeel(luf);

		Array<ScriptingApi::Content::ScriptComponent*> components;
		
		scriptContent->getScriptComponentsFor(components, e.getEventRelativeTo(scriptContent).getPosition());

		if (useComponentSelectMode)
		{
			
			m.addSectionHeader("Create new widget");
			m.addItem((int)Widgets::Knob, "Add new Slider");
			m.addItem((int)Widgets::Button, "Add new Button");
			m.addItem((int)Widgets::Table, "Add new Table");
			m.addItem((int)Widgets::ComboBox, "Add new ComboBox");
			m.addItem((int)Widgets::Label, "Add new Label");
			m.addItem((int)Widgets::Image, "Add new Image");
			m.addItem((int)Widgets::Plotter, "Add new Plotter");
			m.addItem((int)Widgets::ModulatorMeter, "Add new ModulatorMeter");
			m.addItem((int)Widgets::Panel, "Add new Panel");
			m.addItem((int)Widgets::AudioWaveform, "Add new AudioWaveform");
			m.addItem((int)Widgets::SliderPack, "Add new SliderPack");

			m.addItem((int)Widgets::duplicateWidget, "Duplicate selected component", scriptContent->getEditedComponent() != nullptr);

			if (components.size() != 0)
			{
				m.addSeparator();

				m.addItem(6, "Connect to Module Parameter");

				if (components.size() == 1)
				{
					m.addItem(editComponentOffset, "Edit \"" + components[0]->getName().toString() + "\" in Panel");
				}
				else
				{
					PopupMenu comp;

					for (int i = 0; i < components.size(); i++)
					{
						comp.addItem(editComponentOffset + i, components[i]->getName().toString());
					}

					m.addSubMenu("Select Component to edit", comp, components.size() != 0);
				}
			}

		}
		else
		{
			return;
		}

		int result = m.show();

		JavascriptProcessor *s = dynamic_cast<JavascriptProcessor*>(getProcessor());

		if (result == 6)
		{
			ScriptingApi::Content::ScriptComponent* sc = components.getFirst();

			jassert(sc != nullptr);

			if (ProcessorHelpers::is<JavascriptMidiProcessor>(getProcessor()))
			{
				ParameterConnector *comp = new ParameterConnector(sc, this);

				comp->setModalBaseWindowComponent(findParentComponentOfClass<BackendProcessorEditor>());
			}
		}
		else if (result >= (int)Widgets::Knob && result < (int)Widgets::numWidgets)
		{
			const int insertX = e.getEventRelativeTo(scriptContent).getMouseDownPosition().getX();
			const int insertY = e.getEventRelativeTo(scriptContent).getMouseDownPosition().getY();

			createNewComponent((Widgets)result, insertX, insertY);
		}
		else if (result >= editComponentOffset) // EDIT IN PANEL
		{
			ReferenceCountedObject* s = components[result - editComponentOffset];

			getProcessor()->getMainController()->setEditedScriptComponent(s, this);
		}
		
	}
}

void ScriptingEditor::toggleComponentSelectMode(bool shouldSelectOnClick)
{
	useComponentSelectMode = shouldSelectOnClick;

	scriptContent->setInterceptsMouseClicks(false, !useComponentSelectMode);
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
	else if (k.getKeyCode() == KeyPress::F5Key)
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



namespace OverlayIcons
{
    static const unsigned char lockShape[] = { 110,109,41,100,31,68,33,48,94,67,98,156,188,33,68,33,48,94,67,248,163,35,68,211,205,101,67,248,163,35,68,92,47,111,67,108,248,163,35,68,223,111,184,67,98,248,163,35,68,164,32,189,67,139,188,33,68,125,239,192,67,41,100,31,68,125,239,192,67,108,37,182,
        213,67,125,239,192,67,98,96,5,209,67,125,239,192,67,135,54,205,67,164,32,189,67,135,54,205,67,223,111,184,67,108,135,54,205,67,92,47,111,67,98,135,54,205,67,211,205,101,67,96,5,209,67,33,48,94,67,37,182,213,67,33,48,94,67,108,41,100,31,68,33,48,94,67,
        99,109,166,91,248,67,68,11,76,67,108,166,171,219,67,68,11,76,67,108,166,171,219,67,160,186,20,67,108,137,129,219,67,160,186,20,67,108,137,129,219,67,184,126,20,67,98,137,129,219,67,254,20,196,66,172,252,239,67,229,80,100,66,84,155,4,68,229,80,100,66,
        98,98,56,17,68,229,80,100,66,227,117,27,68,254,20,196,66,227,117,27,68,184,126,20,67,108,227,117,27,68,160,186,20,67,108,49,112,27,68,160,186,20,67,108,49,112,27,68,193,234,76,67,108,41,28,13,68,193,234,76,67,108,41,28,13,68,160,186,20,67,108,229,24,
        13,68,160,186,20,67,98,229,24,13,68,168,166,20,67,246,24,13,68,176,146,20,67,246,24,13,68,184,126,20,67,98,246,24,13,68,0,192,1,67,242,74,9,68,98,16,229,66,84,155,4,68,98,16,229,66,98,35,235,255,67,98,16,229,66,133,91,248,67,66,128,1,67,231,59,248,67,
        180,8,20,67,108,166,91,248,67,180,8,20,67,108,166,91,248,67,68,11,76,67,99,101,0,0 };
    
    static const unsigned char penShape[] = { 110,109,96,69,112,67,182,243,141,64,108,154,73,133,67,143,194,240,65,98,158,95,136,67,201,118,16,66,59,111,136,67,92,15,56,66,172,108,133,67,125,191,80,66,108,51,179,122,67,100,123,137,66,108,240,7,74,67,172,28,170,65,108,20,46,90,67,82,184,150,64,98,
        51,51,96,67,12,2,187,191,88,25,106,67,131,192,202,191,96,69,112,67,182,243,141,64,99,109,14,173,62,67,164,240,1,66,108,113,29,111,67,213,120,159,66,108,127,42,171,66,0,32,109,67,108,117,147,20,66,190,223,61,67,108,14,173,62,67,164,240,1,66,99,109,236,
        81,200,65,121,9,75,67,108,123,148,145,66,53,158,121,67,108,0,0,0,0,74,60,138,67,108,236,81,200,65,121,9,75,67,99,101,0,0 };
};

ScriptingEditor::DragOverlay::DragOverlay()
{
	addAndMakeVisible(dragger = new Dragger());

	setInterceptsMouseClicks(false, true);

	addAndMakeVisible(dragModeButton = new ShapeButton("Drag Mode", Colours::black.withAlpha(0.6f), Colours::black.withAlpha(0.8f), Colours::black.withAlpha(0.8f)));

	Path path;
    path.loadPathFromData(OverlayIcons::lockShape, sizeof(OverlayIcons::lockShape));

	dragModeButton->setShape(path, true, true, false);

	dragModeButton->addListener(this);

	dragMode = false;
}

void ScriptingEditor::DragOverlay::resized()
{
	dragModeButton->setBounds(getWidth() - 28, 12, 16, 16);
}


void ScriptingEditor::DragOverlay::buttonClicked(Button* buttonThatWasClicked)
{
	dragMode = !dragMode;

	buttonThatWasClicked->setToggleState(dragMode, dontSendNotification);

	findParentComponentOfClass<ScriptingEditor>()->toggleComponentSelectMode(dragMode);

    Path p;
    
    
	if (dragMode == false)
	{
        p.loadPathFromData(OverlayIcons::lockShape, sizeof(OverlayIcons::lockShape));
		dragger->setDraggedControl(nullptr, nullptr);
	}
    else
    {
        p.loadPathFromData(OverlayIcons::penShape, sizeof(OverlayIcons::penShape));
    }

    dragModeButton->setShape(p, true, true, false);
    
    resized();
    
	repaint();
}

Rectangle<float> getFloatRectangle(const Rectangle<int> &r)
{
    return Rectangle<float>((float)r.getX(), (float)r.getY(), (float)r.getWidth(), (float)r.getHeight());
}

void ScriptingEditor::DragOverlay::paint(Graphics& g)
{
	if (dragMode)
	{
        g.setColour(Colours::white.withAlpha(0.05f));
        g.fillAll();
        
		for (int x = 10; x < getWidth(); x += 10)
		{
			g.setColour(Colours::black.withAlpha((x % 100 == 0) ? 0.12f : 0.05f));

			g.drawVerticalLine(x, 0, getHeight());
		}

		for (int y = 10; y < getHeight(); y += 10)
		{
			g.setColour(Colours::black.withAlpha(((y) % 100 == 0) ? 0.1f : 0.05f));
			g.drawHorizontalLine(y, 0.0f, getWidth());
		}
	}

	Colour c = Colours::white;

	g.setColour(c.withAlpha(0.2f));
    
    g.fillRoundedRectangle(getFloatRectangle(dragModeButton->getBounds().expanded(3)), 3.0f);
}

ScriptingEditor::DragOverlay::Dragger::Dragger()
{
	constrainer.setMinimumOnscreenAmounts(0xFFFFFF, 0xFFFFFF, 0xFFFFFF, 0xFFFFFF);



	addAndMakeVisible(resizer = new ResizableCornerComponent(this, &constrainer));

	resizer->addMouseListener(this, true);

	setWantsKeyboardFocus(true);
}

ScriptingEditor::DragOverlay::Dragger::~Dragger()
{
	setDraggedControl(nullptr, nullptr);
}

void ScriptingEditor::DragOverlay::Dragger::paint(Graphics &g)
{
	g.fillAll(Colours::black.withAlpha(0.2f));
	g.setColour(Colours::white.withAlpha(0.5f));

	if(!snapShot.isNull()) g.drawImageAt(snapShot, 0, 0);

	g.drawRect(getLocalBounds(), 1.0f);
    
	if (copyMode)
	{
        g.setColour(Colours::white.withAlpha(0.8f));
		g.setFont(GLOBAL_BOLD_FONT().withHeight(28.0f));
		g.drawText("+", getLocalBounds().withTrimmedLeft(2).expanded(0, 4), Justification::topLeft);
	}
}

void ScriptingEditor::DragOverlay::Dragger::mouseDown(const MouseEvent& e)
{
	constrainer.setStartPosition(getBounds());

	
	if (e.eventComponent == this)
	{
		snapShot = currentlyDraggedComponent->createComponentSnapshot(currentlyDraggedComponent->getLocalBounds());

		dragger.startDraggingComponent(this, e);
	}

	ScriptingEditor* editor = findParentComponentOfClass<ScriptingEditor>();

	if (editor != nullptr && e.mods.isRightButtonDown())
	{
		editor->mouseDown(e);
	}
}

void ScriptingEditor::DragOverlay::Dragger::mouseDrag(const MouseEvent& e)
{
	constrainer.setRasteredMovement(e.mods.isCommandDown());
	constrainer.setLockedMovement(e.mods.isShiftDown());

	copyMode = e.mods.isAltDown();

	if (e.eventComponent == this)
		dragger.dragComponent(this, e, &constrainer);
}

void ScriptingEditor::DragOverlay::Dragger::mouseUp(const MouseEvent& e)
{
	ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

	snapShot = Image();

	if (copyMode)
	{
		ScriptingEditor* editor = findParentComponentOfClass<ScriptingEditor>();

		if (editor != nullptr)
		{
			const int oldX = sc->getPosition().getX();
			const int oldY = sc->getPosition().getY();

			const int newX = oldX + constrainer.getDeltaX();
			const int newY = oldY + constrainer.getDeltaY();

			editor->createNewComponent(Widgets::duplicateWidget, newX, newY);
		}
		

		copyMode = false;

		repaint();
		return;
	}

	

	

	repaint();

	if (sc != nullptr)
	{
		undoManager.beginNewTransaction();

		if (e.eventComponent == this)
		{
			undoManager.perform(new OverlayAction(this, false, constrainer.getDeltaX(), constrainer.getDeltaY()));
		}
		else
		{
			undoManager.perform(new OverlayAction(this, true, constrainer.getDeltaWidth(), constrainer.getDeltaHeight()));
		}
	}
}

void ScriptingEditor::DragOverlay::Dragger::moveOverlayedComponent(int deltaX, int deltaY)
{
	ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

	ScriptingEditor* editor = findParentComponentOfClass<ScriptingEditor>();

	const int oldX = sc->getPosition().getX();
	const int oldY = sc->getPosition().getY();

	const int newX = oldX + deltaX;
	const int newY = oldY + deltaY;

	if (editor != nullptr)
	{
		editor->changePositionOfComponent(sc, newX, newY);
	}
}

void ScriptingEditor::DragOverlay::Dragger::resizeOverlayedComponent(int deltaX, int deltaY)
{
	ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

	const int oldWidth = sc->getPosition().getWidth();
	const int oldHeight = sc->getPosition().getHeight();

	const int newWidth = oldWidth + deltaX;
	const int newHeight = oldHeight + deltaY;

	sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width), newWidth, dontSendNotification);
	sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::height), newHeight, sendNotification);
	sc->setChanged();

	ScriptingEditor* editor = findParentComponentOfClass<ScriptingEditor>();

	if (editor != nullptr)
	{
		editor->scriptComponentChanged(sc, sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width));
	}
}
