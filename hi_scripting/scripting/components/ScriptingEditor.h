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

namespace hise { using namespace juce;


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

	JavascriptProcessor* getScriptEditHandlerProcessor() { return dynamic_cast<JavascriptProcessor*>(getProcessor()); }
	ScriptContentComponent* getScriptEditHandlerContent() { return scriptContent.get(); }
	ScriptingContentOverlay* getScriptEditHandlerOverlay() { return dragOverlay; }
	CommonEditorFunctions::EditorType* getScriptEditHandlerEditor() { return codeEditor->editor; }
	
	void updateGui() override;;

	void showCallback(int callbackIndex, int charToScroll=-1);
	void showOnInitCallback();
	void gotoChar(int character);

	void editorInitialized()
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

	bool keyPressed(const KeyPress &k) override;

	void scriptEditHandlerCompileCallback() override;

	bool isInEditMode() const
	{
		return codeEditor != nullptr && scriptContent->isVisible() && (getActiveCallback() == 0);
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

	void resized();
    void buttonClicked (Button* buttonThatWasClicked);

	void selectOnInitCallback() override
	{
		if (getActiveCallback() != 0)
			buttonClicked(callbackButtons[0]); // we need synchronous execution here
	}
	
private:

	static constexpr int EditorHeight = 500;

	bool isFront = false;
	bool isConnectedToExternalScript = false;

	ScopedPointer<ScriptingContentOverlay> dragOverlay;
	Component::SafePointer<ScriptingContentOverlay> currentDragOverlay;

	ScopedPointer<CodeDocument> doc;
	ScopedPointer<JavascriptTokeniser> tokenizer;

	bool useComponentSelectMode;
	int currentCallbackIndex = -1;

	ScopedPointer<ScriptContentComponent> scriptContent;

	Component::SafePointer<Component> currentlyEditedComponent;

	ChainBarButtonLookAndFeel alaf;

	Array<int> lastPositions;

    //==============================================================================
    ScopedPointer<PopupIncludeEditor> codeEditor;
    
	ScopedPointer<TextButton> contentButton;
	OwnedArray<TextButton> callbackButtons;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ScriptingEditor)
};

} // namespace hise
#endif   // __JUCE_HEADER_87B359E078BBC6D4__
