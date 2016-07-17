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

#ifndef __JUCE_HEADER_87B359E078BBC6D4__
#define __JUCE_HEADER_87B359E078BBC6D4__


class HardcodedScriptEditor: public ProcessorEditorBody
{
public:

	HardcodedScriptEditor(ProcessorEditor *p):
		ProcessorEditorBody(p)

	{
		addAndMakeVisible(contentComponent = new ScriptContentComponent(static_cast<ScriptBaseMidiProcessor*>(getProcessor())));
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
    ScriptingEditor (ProcessorEditor *p);
    ~ScriptingEditor();

    //==============================================================================
   

	void scriptComponentChanged(DynamicObject *scriptComponent, Identifier /*propertyThatWasChanged*/) override;

	void updateGui() override
	{
		if(getHeight() != getBodyHeight()) setSize(getWidth(), getBodyHeight());

		double x = dynamic_cast<JavascriptProcessor*>(getProcessor())->getLastExecutionTime();
		timeLabel->setText(String(x, 3) + " ms", dontSendNotification);

		getProcessor()->setEditorState(Processor::BodyShown, true);

		contentButton->setToggleState(getProcessor()->getEditorState(ProcessorWithScriptingContent::EditorStates::contentShown), dontSendNotification);

	};

	void setEditedScriptComponent(DynamicObject *component);

	void mouseDown(const MouseEvent &e) override;

	bool isRootEditor() const { return getEditor()->isRootEditor(); }

	void editorInitialized()
	{
		bool anyOpen = false;

		JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

		for(int i = 0; i < sp->getNumSnippets(); i++)
		{
			if(getProcessor()->getEditorState(i + ProcessorWithScriptingContent::EditorStates::onInitShown))
			{
				buttonClicked(getSnippetButton(i));
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


	int getActiveCallback() const;

	void checkActiveSnippets();

	Button *getSnippetButton(int i)
	{
		return callbackButtons[i];
	}

	int getBodyHeight() const override
	{
		if (isRootEditor())
		{
			return findParentComponentOfClass<Viewport>()->getHeight() - 36;
		}

		const int contentHeight = getProcessor()->getEditorState(ProcessorWithScriptingContent::EditorStates::contentShown) ? scriptContent->getContentHeight() : 0;

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

	void goToSavedPosition(int newCallback);
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

	ScopedPointer<TextButton> contentButton;
	OwnedArray<TextButton> callbackButtons;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptingEditor)
};

#endif   // __JUCE_HEADER_87B359E078BBC6D4__
