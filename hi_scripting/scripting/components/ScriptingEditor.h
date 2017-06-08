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

#ifndef __JUCE_HEADER_87B359E078BBC6D4__
#define __JUCE_HEADER_87B359E078BBC6D4__



class ScriptingEditor;


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

class DebugConsoleTextEditor : public TextEditor
{
public:

	DebugConsoleTextEditor(const String& name) :
		TextEditor(name) {};

	bool keyPressed(const KeyPress& k) override
	{
		if (k == KeyPress::upKey)
		{
			currentHistoryIndex = jmax<int>(0, currentHistoryIndex - 1);

			setText(history[currentHistoryIndex], dontSendNotification);

			return true;
		}
		else if (k == KeyPress::downKey)
		{
			currentHistoryIndex = jmin<int>(history.size() - 1, currentHistoryIndex + 1);
			setText(history[currentHistoryIndex], dontSendNotification);
		}

		return TextEditor::keyPressed(k);
	}

	void addToHistory(const String& s)
	{
		if (!history.contains(s))
		{
			history.add(s);
			currentHistoryIndex = history.size() - 1;
		}
		else
		{
			history.move(history.indexOf(s), history.size() - 1);
		}
	}

private:

	StringArray history;
	int currentHistoryIndex;
};

class ScriptingEditor  : public ProcessorEditorBody,
						 public ScriptEditHandler,
                         public ButtonListener,
						 public TextEditor::Listener
{
public:
    //==============================================================================

	

    ScriptingEditor (ProcessorEditor *p);
    ~ScriptingEditor();

    //==============================================================================
   
	void textEditorReturnKeyPressed(TextEditor& t)
	{
		String codeToEvaluate = t.getText();

		dynamic_cast<DebugConsoleTextEditor*>(&t)->addToHistory(codeToEvaluate);

		if (codeToEvaluate.startsWith("> "))
		{
			codeToEvaluate = codeToEvaluate.substring(2);
		}

		HiseJavascriptEngine* engine = dynamic_cast<JavascriptProcessor*>(getProcessor())->getScriptEngine();

		if (engine != nullptr)
		{
			var returnValue = engine->evaluate(codeToEvaluate);

			debugToConsole(getProcessor(), "> " + returnValue.toString());
		}
	}

	JavascriptProcessor* getScriptEditHandlerProcessor() { return dynamic_cast<JavascriptProcessor*>(getProcessor()); }

	ScriptContentComponent* getScriptEditHandlerContent() { return scriptContent.get(); }

	ScriptingContentOverlay* getScriptEditHandlerOverlay() { return dragOverlay; }

	JavascriptCodeEditor* getScriptEditHandlerEditor() { return codeEditor->editor; }
	
	void updateGui() override
	{
		JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

		const bool nowConnected = sp->isConnectedToExternalFile();

		if (nowConnected != isConnectedToExternalScript)
		{
			isConnectedToExternalScript = nowConnected;
			useComponentSelectMode = false;
			refreshBodySize();
		}

		if(getHeight() != getBodyHeight()) setSize(getWidth(), getBodyHeight());

		getProcessor()->setEditorState(Processor::BodyShown, true);

		int editorOffset = dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

		contentButton->setToggleState(getProcessor()->getEditorState(editorOffset + ProcessorWithScriptingContent::EditorStates::contentShown), dontSendNotification);

	};

	void showCallback(int callbackIndex, int charToScroll=-1);

	void showOnInitCallback();

	void openContentInPopup();
	void closeContentPopup();

	void editInAllPopup();
	void closeAllPopup();

	void gotoChar(int character);

	void mouseDown(const MouseEvent &e) override;

	void mouseDoubleClick(const MouseEvent& e) override;

	bool isRootEditor() const { return getEditor()->isRootEditor(); }

	void editorInitialized()
	{
		bool anyOpen = false;

		JavascriptProcessor* sp = dynamic_cast<JavascriptProcessor*>(getProcessor());

		int editorOffset = dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

		for(int i = 0; i < sp->getNumSnippets(); i++)
		{
			if(getProcessor()->getEditorState(editorOffset + i + ProcessorWithScriptingContent::EditorStates::onInitShown))
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

	void scriptEditHandlerCompileCallback() override;

	bool isInEditMode() const
	{
		return editorShown && scriptContent->isVisible() && (getActiveCallback() == 0);
	}

	
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

	int getBodyHeight() const override;;

	void gotoLocation(DebugInformation* info);

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

	void goToSavedPosition(int newCallback);
	void saveLastCallback();

	void selectOnInitCallback() override
	{
		if (getActiveCallback() != 0)
		{
			buttonClicked(callbackButtons[0]); // we need synchronous execution here
		}
	}

	class ContentPopup : public DocumentWindow
	{
	public:

		ContentPopup(ProcessorWithScriptingContent* pwsc, ScriptingEditor* parentEditor) :
			DocumentWindow("Interface Popup", Colours::black, DocumentWindow::allButtons, true),
			parent(parentEditor)
		{
			holder = new Component();
			holder->addAndMakeVisible(content = new ScriptContentComponent(pwsc));
			holder->addAndMakeVisible(dragOverlay = new ScriptingContentOverlay(parentEditor));
			holder->setSize(jmax<int>(40, content->getContentWidth()), jmax<int>(40, content->getContentHeight()));

			setContentNonOwned(holder, true);

			setUsingNativeTitleBar(true);

			setResizable(false, false);
			setVisible(true);

			centreWithSize(holder->getWidth(), holder->getHeight());
		}

		void closeButtonPressed() override
		{
			if (parent.getComponent() != nullptr)
			{
				parent->closeContentPopup();
			}
		}


		void mouseDown(const MouseEvent& /*e*/) override
		{

		}

		void resized()
		{
			content->setBounds(0, 0, content->getContentWidth(), content->getContentHeight());
			dragOverlay->setBounds(0, 0, content->getContentWidth(), content->getContentHeight());
			holder->setBounds(0, 0, content->getContentWidth(), content->getContentHeight());
		}

		ScopedPointer<Component> holder;

		Component::SafePointer<ScriptingEditor> parent;

		ScopedPointer<ScriptContentComponent> content;
		ScopedPointer<ScriptingContentOverlay> dragOverlay;
	};


private:

	bool isConnectedToExternalScript = false;

	ScopedPointer<ScriptingContentOverlay> dragOverlay;
	Component::SafePointer<ScriptingContentOverlay> currentDragOverlay;

	ScopedPointer<CodeDocument> doc;

	ScopedPointer<JavascriptTokeniser> tokenizer;

	ScopedPointer<PopupIncludeEditorWindow> allEditor;

	ScopedPointer<ContentPopup> contentPopup;

	bool editorShown;

	bool useComponentSelectMode;

	int currentCallbackIndex = -1;

	ScopedPointer<ScriptContentComponent> scriptContent;

	Component::SafePointer<Component> currentlyEditedComponent;

	ChainBarButtonLookAndFeel alaf;

	LookAndFeel_V2 laf2;

	Array<int> lastPositions;

    //==============================================================================
    ScopedPointer<CodeEditorWrapper> codeEditor;
    ScopedPointer<TextButton> compileButton;
    ScopedPointer<DebugConsoleTextEditor> messageBox;
    ScopedPointer<Label> timeLabel;
    
    ScopedPointer<Component> buttonRow;

	ScopedPointer<TextButton> contentButton;
	OwnedArray<TextButton> callbackButtons;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptingEditor)
};

#endif   // __JUCE_HEADER_87B359E078BBC6D4__
