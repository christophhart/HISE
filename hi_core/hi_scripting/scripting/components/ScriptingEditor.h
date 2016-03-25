/*
  ==============================================================================

  This is an automatically generated GUI class created by the Introjucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Introjucer version: 3.1.1

  ------------------------------------------------------------------------------

  The Introjucer is part of the JUCE library - "Jules' Utility Class Extensions"
  Copyright 2004-13 by Raw Material Software Ltd.

  ==============================================================================
*/

#ifndef __JUCE_HEADER_87B359E078BBC6D4__
#define __JUCE_HEADER_87B359E078BBC6D4__


class HardcodedScriptEditor: public ProcessorEditorBody
{
public:

	HardcodedScriptEditor(BetterProcessorEditor *p):
		ProcessorEditorBody(p)

	{
		addAndMakeVisible(contentComponent = new ScriptContentComponent(static_cast<ScriptBaseProcessor*>(getProcessor())));
		contentComponent->refreshMacroIndexes();



	};

	void updateGui() override
	{
		contentComponent->changeListenerCallback(getProcessor());
	};

	int getBodyHeight() const override
	{
		return contentComponent->getContentHeight() == 0 ? 0 : contentComponent->getContentHeight() + 38;
	};

	
	void resized()
	{
		contentComponent->setBounds((getWidth() / 2) - ((getWidth() - 90) / 2), 24 ,getWidth() - 90, contentComponent->getContentHeight());
	}

private:

	ScopedPointer<ScriptContentComponent> contentComponent;

};

class ScriptingEditor  : public ProcessorEditorBody,
                         public ScriptComponentEditListener,
                         public ButtonListener
{
public:
    //==============================================================================
    ScriptingEditor (BetterProcessorEditor *p);
    ~ScriptingEditor();

    //==============================================================================
   

	void scriptComponentChanged(DynamicObject *scriptComponent, Identifier /*propertyThatWasChanged*/) override;

	void updateGui() override
	{
		if(getHeight() != getBodyHeight()) setSize(getWidth(), getBodyHeight());

		double x = static_cast<ScriptProcessor*>(getProcessor())->getLastExecutionTime();
		timeLabel->setText(String(x, 3) + " ms", dontSendNotification);

		getProcessor()->setEditorState(Processor::BodyShown, true);

		contentButton->setToggleState(getProcessor()->getEditorState(ScriptProcessor::contentShown), dontSendNotification);

	};

	void setEditedScriptComponent(DynamicObject *component);

	void mouseDown(const MouseEvent &e) override;

	bool isRootEditor() const { return getEditor()->isRootEditor(); }

	void editorInitialized()
	{
		bool anyOpen = false;

		for(int i = 0; i < ScriptProcessor::numCallbacks; i++)
		{
			if(getProcessor()->getEditorState(i + Processor::numEditorStates))
			{
				buttonClicked(getSnippetButton((ScriptProcessor::Callback) i));
				anyOpen = true;
			}
		}


		if (! anyOpen)
		{
			editorShown = false;
			setSize(getWidth(), getBodyHeight());

		}

		checkActiveSnippets();
	}

	bool keyPressed(const KeyPress &k) override;

	void compileScript();

	void checkContent()
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


	ScriptProcessor::Callback getActiveCallback() const;

	void checkActiveSnippets()
	{
		ScriptProcessor *s = static_cast<ScriptProcessor*>(getProcessor());



		for(int i = 0; i < ScriptProcessor::numCallbacks; i++)
		{
			const bool isSnippetEmpty = s->getSnippet((ScriptProcessor::Callback)i)->isSnippetEmpty();

			Button *t = getSnippetButton((ScriptProcessor::Callback)i);

			t->setColour(TextButton::buttonColourId, !isSnippetEmpty ? Colour (0x77cccccc) : Colour (0x4c4b4b4b));
			t->setColour (TextButton::buttonOnColourId, Colours::white.withAlpha(0.7f));
			t->setColour (TextButton::textColourOnId, Colour (0xaa000000));
			t->setColour (TextButton::textColourOffId, Colour (0x99ffffff));

			if(i == ScriptProcessor::onNoteOff || i == ScriptProcessor::onNoteOn || i == ScriptProcessor::onController || i == ScriptProcessor::onTimer)
			{
				t->setButtonText(s->getSnippet(i)->getCallbackName().toString() + (s->isDeferred() ? " (D)" : ""));
			}

		}
	}

	Button *getSnippetButton(ScriptProcessor::Callback c)
	{
		switch(c)
		{
		case ScriptProcessor::onInit:		return onInitButton;
		case ScriptProcessor::onNoteOn:		return noteOnButton;
		case ScriptProcessor::onNoteOff:	return noteOffButton;
		case ScriptProcessor::onController:	return onControllerButton;
		case ScriptProcessor::onTimer:		return onTimerButton;
		case ScriptProcessor::onControl:	return onControlButton;
		default:							jassertfalse; return nullptr;
		}
	}

	int getBodyHeight() const override
	{
		if (isRootEditor())
		{
			return findParentComponentOfClass<Viewport>()->getHeight() - 36;
		}

		const int contentHeight = getProcessor()->getEditorState(ScriptProcessor::contentShown) ? scriptContent->getContentHeight() : 0;

		if (editorShown)
		{
			return 28 + contentHeight + codeEditor->getHeight() + 24;
		}
		else
		{
			return 28 + contentHeight;
		}
	};

    

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

	void goToSavedPosition(ScriptProcessor::Callback newCallback);
	void saveLastCallback();



private:
    
	CodeDocument *doc;

	ScopedPointer<JavascriptTokeniser> tokenizer;

	bool editorShown;

	bool useComponentSelectMode;

	ScopedPointer<ScriptContentComponent> scriptContent;

	Component::SafePointer<Component> currentlyEditedComponent;

	ChainBarButtonLookAndFeel alaf;

	LookAndFeel_V2 laf2;

	OwnedArray<CodeDocument::Position> lastPositions;


    //==============================================================================
    ScopedPointer<CodeEditorWrapper> codeEditor;
    ScopedPointer<TextButton> compileButton;
    ScopedPointer<TextEditor> messageBox;
    ScopedPointer<Label> timeLabel;
    ScopedPointer<TextButton> noteOnButton;
    ScopedPointer<TextButton> noteOffButton;
    ScopedPointer<TextButton> onControllerButton;
    ScopedPointer<TextButton> onTimerButton;
    ScopedPointer<TextButton> onInitButton;
    ScopedPointer<TextButton> onControlButton;
    ScopedPointer<TextButton> contentButton;
	
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptingEditor)
};

#endif   // __JUCE_HEADER_87B359E078BBC6D4__
