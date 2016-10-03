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
   

	void scriptComponentChanged(ReferenceCountedObject *scriptComponent, Identifier /*propertyThatWasChanged*/) override;

	void updateGui() override
	{
		if(getHeight() != getBodyHeight()) setSize(getWidth(), getBodyHeight());

		
		getProcessor()->setEditorState(Processor::BodyShown, true);

		int editorOffset = dynamic_cast<ProcessorWithScriptingContent*>(getProcessor())->getCallbackEditorStateOffset();

		contentButton->setToggleState(getProcessor()->getEditorState(editorOffset + ProcessorWithScriptingContent::EditorStates::contentShown), dontSendNotification);

	};

	void setEditedScriptComponent(ReferenceCountedObject* component);

	void mouseDown(const MouseEvent &e) override;

	void toggleComponentSelectMode(bool shouldSelectOnClick);

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

	void compileScript();

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

    

    void paint (Graphics& g);
    void resized();
    void buttonClicked (Button* buttonThatWasClicked);

	void goToSavedPosition(int newCallback);
	void saveLastCallback();

	void changePositionOfComponent(ScriptingApi::Content::ScriptComponent* sc, int newX, int newY);
    
	class DragOverlay : public Component,
						public ButtonListener
	{
	public:

		DragOverlay();


		void resized();

		void buttonClicked(Button* buttonThatWasClicked);

		void paint(Graphics& g) override;


		class Dragger : public Component
		{
		public:

			Dragger();

			~Dragger();

			void paint(Graphics &g) override;

			void mouseDown(const MouseEvent& e);

			void mouseDrag(const MouseEvent& e);

			void mouseUp(const MouseEvent& e)
			{
				ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

				if (sc != nullptr)
				{
					if (e.eventComponent == this) // just moving
					{
						ScriptingEditor* editor = findParentComponentOfClass<ScriptingEditor>();

						const int oldX = sc->getPosition().getX();
						const int oldY = sc->getPosition().getY();

						const int newX = oldX + e.getDistanceFromDragStartX();
						const int newY = oldY + e.getDistanceFromDragStartY();

						if (editor != nullptr)
						{
							editor->changePositionOfComponent(sc, newX, newY);
						}
					}
					else 
					{
						sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width), getWidth(), dontSendNotification);
						sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::height), getHeight(), sendNotification);
						sc->setChanged();

						ScriptingEditor* editor = findParentComponentOfClass<ScriptingEditor>();

						if (editor != nullptr)
						{
							editor->scriptComponentChanged(sc, sc->getIdFor(ScriptingApi::Content::ScriptComponent::Properties::width));
						}
					}
				}
			}

			void resized()
			{
				resizer->setBounds(getWidth() - 10, getHeight() - 10, 10, 10);
			}

			bool keyPressed(const KeyPress &key) override
			{
				if (currentlyDraggedComponent == nullptr) return false;

				ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

				if (sc == nullptr) return false;

				const int keyCode = key.getKeyCode();

				const int delta = key.getModifiers().isCommandDown() ? 10 : 1;

				const int horizontalProperty = key.getModifiers().isShiftDown() ? ScriptingApi::Content::ScriptComponent::width : ScriptingApi::Content::ScriptComponent::x;
				const int verticalProperty = key.getModifiers().isShiftDown() ? ScriptingApi::Content::ScriptComponent::height : ScriptingApi::Content::ScriptComponent::y;

				int x = sc->getScriptObjectProperty(horizontalProperty);
				int y = sc->getScriptObjectProperty(verticalProperty);

				if (keyCode == KeyPress::upKey)
				{
					sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(verticalProperty), y - delta, sendNotification);
					sc->setChanged();



					return true;
				}
				else if (keyCode == KeyPress::downKey)
				{
					sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(verticalProperty), y + delta, sendNotification);
					sc->setChanged();
					return true;
				}
				else if (keyCode == KeyPress::leftKey)
				{
					sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(horizontalProperty), x - delta, sendNotification);
					sc->setChanged();
					return true;
				}
				else if (keyCode == KeyPress::rightKey)
				{
					sc->setScriptObjectPropertyWithChangeMessage(sc->getIdFor(horizontalProperty), x + delta, sendNotification);
					sc->setChanged();
					return true;
				}

				return false;
			}

			

			void setDraggedControl(Component* componentToDrag, ScriptingApi::Content::ScriptComponent* sc)
			{
				if (componentToDrag != nullptr && componentToDrag != currentlyDraggedComponent.getComponent())
				{
					currentScriptComponent = sc;
					currentlyDraggedComponent = componentToDrag;

					currentMovementWatcher = new MovementWatcher(componentToDrag, this);

					Component* p = componentToDrag->getParentComponent();

					setBounds(getParentComponent()->getLocalArea(p, componentToDrag->getBounds()));
					setVisible(true);
					setWantsKeyboardFocus(true);
					
					

					setAlwaysOnTop(true);
					grabKeyboardFocus();
				}
				else if (componentToDrag == nullptr)
				{
					currentScriptComponent = nullptr;
					currentlyDraggedComponent = nullptr;
					currentMovementWatcher = nullptr;
					setBounds(Rectangle<int>());
					setVisible(false);
					setWantsKeyboardFocus(false);
					setAlwaysOnTop(false);
				}

			};

		private:

			class MovementWatcher : public ComponentMovementWatcher
			{
			public:

				MovementWatcher(Component* c, Component* dragComponent_) :
					ComponentMovementWatcher(c),
					dragComponent(dragComponent_)
				{};

				void componentMovedOrResized(bool wasMoved, bool wasResized) override
				{
					Component* p = getComponent()->getParentComponent();

					dragComponent->setBounds(dragComponent->getParentComponent()->getLocalArea(p, getComponent()->getBounds()));

					//dragComponent->setBounds(getComponent()->getBounds());
				}

				void componentPeerChanged()  override {}
				void componentVisibilityChanged() override {};

				Component* dragComponent;
			};


			Component::SafePointer<Component> currentlyDraggedComponent;
			ScriptingApi::Content::ScriptComponent* currentScriptComponent;

			ScopedPointer<MovementWatcher> currentMovementWatcher;

			ComponentDragger dragger;

			ComponentBoundsConstrainer constrainer;

			ScopedPointer<ResizableCornerComponent> resizer;

			
			
		};

		bool dragMode;

		ScopedPointer<Dragger> dragger;

		ScopedPointer<ShapeButton> dragModeButton;
	};



private:

	ScopedPointer<DragOverlay> dragOverlay;

	ScopedPointer<CodeDocument> doc;

	ScopedPointer<JavascriptTokeniser> tokenizer;

	bool editorShown;

	bool useComponentSelectMode;

	ScopedPointer<ScriptContentComponent> scriptContent;

	Component::SafePointer<Component> currentlyEditedComponent;

	ChainBarButtonLookAndFeel alaf;

	LookAndFeel_V2 laf2;

	Array<int> lastPositions;

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
