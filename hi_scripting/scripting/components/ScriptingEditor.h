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

	enum class Widgets
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

    ScriptingEditor (ProcessorEditor *p);
    ~ScriptingEditor();

    //==============================================================================
   
	void createNewComponent(Widgets componentType, int x, int y);

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

			void mouseUp(const MouseEvent& e);

			void resized()
			{
				resizer->setBounds(getWidth() - 10, getHeight() - 10, 10, 10);
			}

			void moveOverlayedComponent(int deltaX, int deltaY);

			void resizeOverlayedComponent(int deltaX, int deltaY);

			bool keyPressed(const KeyPress &key) override
			{
				if (currentlyDraggedComponent == nullptr) return false;

				ScriptingApi::Content::ScriptComponent *sc = currentScriptComponent;

				if (sc == nullptr) return false;

				const int keyCode = key.getKeyCode();
				const int delta = key.getModifiers().isCommandDown() ? 10 : 1;
				const bool resizeComponent = key.getModifiers().isShiftDown();

				if (keyCode == KeyPress::leftKey)
				{
					if (resizeComponent)
					{
						
						undoManager.perform(new OverlayAction(this, true, -delta, 0));
					}
					else
					{
						undoManager.perform(new OverlayAction(this, false, -delta, 0));
					}

					return true;
				}
				else if (keyCode == KeyPress::rightKey)
				{
					if (resizeComponent)
					{
						undoManager.perform(new OverlayAction(this, true, delta, 0));
					}
					else
					{
						undoManager.perform(new OverlayAction(this, false, delta, 0));
					}


					return true;
				}
				else if (keyCode == KeyPress::upKey)
				{
                    if (resizeComponent)
                    {
                        undoManager.perform(new OverlayAction(this, true, 0, -delta));
                    }
                    else
                    {
                        undoManager.perform(new OverlayAction(this, false, 0, -delta));
                    }
                    
                    
                    return true;
				}
				else if (keyCode == KeyPress::downKey)
				{
					if (resizeComponent)
					{
						undoManager.perform(new OverlayAction(this, true, 0, delta));
					}
					else
					{
						undoManager.perform(new OverlayAction(this, false, 0, delta));
					}


					return true;
				}
				else if (keyCode == 'Z' && key.getModifiers().isCommandDown())
				{
					DBG("Undo");
					undoManager.undo();
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

			class Constrainer : public ComponentBoundsConstrainer
			{
			public:

				void checkBounds(Rectangle<int>& bounds, const Rectangle<int>& previousBounds, const Rectangle<int>& limits, bool isStretchingTop, bool isStretchingLeft, bool isStretchingBottom, bool isStretchingRight)
				{
					bounds.setWidth(jmax<int>(10, bounds.getWidth()));
					bounds.setHeight(jmax<int>(10, bounds.getHeight()));

					const bool isResizing = isStretchingRight || isStretchingBottom;

					if (rasteredMovement)
					{
						if (isResizing)
						{
							bounds.setWidth((bounds.getWidth() / 10) * 10);
							bounds.setHeight((bounds.getHeight() / 10) * 10);
						}
						else
						{
							bounds.setX((bounds.getX() / 10) * 10);
							bounds.setY((bounds.getY() / 10) * 10);
						}
					}

					if (lockMovement)
					{
						if (isResizing)
						{
							const bool lockHorizontally = abs(bounds.getWidth() - startPosition.getWidth()) > abs(bounds.getHeight() - startPosition.getHeight());

							if (lockHorizontally)
							{
								bounds.setHeight(startPosition.getHeight());
							}
							else
							{
								bounds.setWidth(startPosition.getWidth());
							}
						}
						else
						{
							const bool lockHorizontally = abs(bounds.getX() - startPosition.getX()) > abs(bounds.getY() - startPosition.getY());

							if (lockHorizontally)
							{
								bounds.setY(startPosition.getY());
							}
							else
							{
								bounds.setX(startPosition.getX());
							}
						}
					}

					if (bounds.getX() < limits.getX())
					{
						bounds.setX(limits.getX());
					}
					if (bounds.getRight() > limits.getRight())
					{
						if (isResizing)
						{
							bounds.setWidth(limits.getRight() - bounds.getX());
						}
						else
						{
							bounds.setX(limits.getRight() - bounds.getWidth());
						}

						
					}
					if (bounds.getY() < limits.getY())
					{
						bounds.setY(limits.getY());
					}
					if (bounds.getBottom() > limits.getBottom())
					{
						if (isResizing)
						{
							bounds.setHeight(limits.getBottom() - bounds.getY());
						}
						else
						{
							bounds.setY(limits.getBottom() - bounds.getHeight());
						}
						
					}

					currentPosition = Rectangle<int>(bounds);
				}


				void setStartPosition(const Rectangle<int>& startPos)
				{
					startPosition = Rectangle<int>(startPos);
					currentPosition = Rectangle<int>(startPos);
				}

				int getDeltaX() const
				{
					return currentPosition.getX() - startPosition.getX();
				}

				int getDeltaY() const
				{
					return currentPosition.getY() - startPosition.getY();
				}

				int getDeltaWidth() const
				{
					return currentPosition.getWidth() - startPosition.getWidth();
				}

				int getDeltaHeight() const
				{
					return currentPosition.getHeight() - startPosition.getHeight();
				}




				void setRasteredMovement(bool shouldBeRastered)
				{
					rasteredMovement = shouldBeRastered;
				}

				void setLockedMovement(bool shouldBeLocked)
				{
					lockMovement = shouldBeLocked;
				}

			private:

				bool rasteredMovement = false;
				bool lockMovement = false;

				Rectangle<int> startPosition;
				Rectangle<int> currentPosition;
			};

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

			class OverlayAction: public UndoableAction
			{
			public:

				OverlayAction(Dragger* overlay_, bool isResized_, int deltaX_, int deltaY_):
					overlay(overlay_),
					draggedComponent(overlay_->currentlyDraggedComponent),
					isResized(isResized_),
					deltaX(deltaX_),
					deltaY(deltaY_)
				{}

				bool perform() override
				{
					if (overlay.getComponent() != nullptr)
					{
						if (isResized)
						{
							overlay->resizeOverlayedComponent(deltaX, deltaY);
						}
						else
						{
							overlay->moveOverlayedComponent(deltaX, deltaY);
						}

						return true;
					}

					return false;

				}

				bool undo() override
				{
					if (draggedComponent.getComponent() != nullptr &&
						overlay.getComponent() != nullptr &&
						overlay->currentlyDraggedComponent.getComponent() == draggedComponent.getComponent())
					{
						if (isResized)
						{
							overlay->resizeOverlayedComponent(-deltaX, -deltaY);
						}
						else
						{
							overlay->moveOverlayedComponent(-deltaX, -deltaY);
						}

						return true;
					}

					return false;

				}

			private:

				Component::SafePointer<Dragger> overlay;
				Component::SafePointer<Component> draggedComponent;

				const bool isResized;
				const int deltaX;
				const int deltaY;

			};


			Component::SafePointer<Component> currentlyDraggedComponent;
			ScriptingApi::Content::ScriptComponent* currentScriptComponent;

			ScopedPointer<MovementWatcher> currentMovementWatcher;

			ComponentDragger dragger;

			UndoManager undoManager;

			Constrainer constrainer;

			ScopedPointer<ResizableCornerComponent> resizer;

			Image snapShot;
			bool copyMode = false;

			
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
