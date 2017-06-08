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


#ifndef SCRIPTINGCONTENTOVERLAY_H_INCLUDED
#define SCRIPTINGCONTENTOVERLAY_H_INCLUDED


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


class ScriptingContentOverlay;

class ScriptEditHandler : public ScriptComponentEditListener
{
public:

	enum class Widgets
	{
		Knob = 0x1000,
		Button,
		Table,
		ComboBox,
		Label,
		Image,
		Viewport,
		Plotter,
		ModulatorMeter,
		Panel,
		AudioWaveform,
		SliderPack,
		duplicateWidget,
		numWidgets
	};

	ScriptEditHandler();

	virtual ScriptContentComponent* getScriptEditHandlerContent() = 0;

	virtual ScriptingContentOverlay* getScriptEditHandlerOverlay() = 0;

	virtual JavascriptCodeEditor* getScriptEditHandlerEditor() = 0;

	virtual JavascriptProcessor* getScriptEditHandlerProcessor() = 0;

	Component* getAsComponent() { return dynamic_cast<Component*>(this); };

	void createNewComponent(Widgets componentType, int x, int y);

	bool editModeEnabled() const { return useComponentSelectMode; }

	void setEditedScriptComponent(ReferenceCountedObject* component);



	void toggleComponentSelectMode(bool shouldSelectOnClick);

	void changePositionOfComponent(ScriptingApi::Content::ScriptComponent* sc, int newX, int newY);

	virtual void selectOnInitCallback() = 0;

	void compileScript();

	void scriptComponentChanged(ReferenceCountedObject* scriptComponent, Identifier) override;



	virtual void scriptEditHandlerCompileCallback()
	{

	}

private:

	bool useComponentSelectMode = false;

	String isValidWidgetName(const String &id);

	Component::SafePointer<ScriptContentComponent> currentContent;

	WeakReference<Processor> currentScriptProcessor;
};

class ScriptingEditor;

class ScriptingContentOverlay : public Component,
	public ButtonListener
{
public:

	ScriptingContentOverlay(ScriptEditHandler* parentHandler);

	void resized();

	void buttonClicked(Button* buttonThatWasClicked);

	void toggleEditMode();

	void setEditMode(bool editModeEnabled);

	void setShowEditButton(bool shouldBeVisible)
	{
		dragModeButton->setVisible(shouldBeVisible);
	}

	void paint(Graphics& g) override;

	void mouseDown(const MouseEvent& e) override;

	class Dragger : public Component
	{
	public:

		Dragger(ScriptEditHandler* parentHandler);

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

		void undo()
		{
			undoManager.undo();
		}

		void redo()
		{
			undoManager.redo();
		}

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



		void setDraggedControl(Component* componentToDrag, ScriptingApi::Content::ScriptComponent* sc);;

	private:

		class Constrainer : public ComponentBoundsConstrainer
		{
		public:

			void checkBounds(Rectangle<int>& newBounds, const Rectangle<int>& /*previousBounds*/, const Rectangle<int>& limits, bool /*isStretchingTop*/, bool /*isStretchingLeft*/, bool isStretchingBottom, bool isStretchingRight)
			{
				newBounds.setWidth(jmax<int>(10, newBounds.getWidth()));
				newBounds.setHeight(jmax<int>(10, newBounds.getHeight()));

				const bool isResizing = isStretchingRight || isStretchingBottom;

				if (rasteredMovement)
				{
					if (isResizing)
					{
						newBounds.setWidth((newBounds.getWidth() / 10) * 10);
						newBounds.setHeight((newBounds.getHeight() / 10) * 10);
					}
					else
					{
						newBounds.setX((newBounds.getX() / 10) * 10);
						newBounds.setY((newBounds.getY() / 10) * 10);
					}
				}

				if (lockMovement)
				{
					if (isResizing)
					{
						const bool lockHorizontally = abs(newBounds.getWidth() - startPosition.getWidth()) > abs(newBounds.getHeight() - startPosition.getHeight());

						if (lockHorizontally)
						{
							newBounds.setHeight(startPosition.getHeight());
						}
						else
						{
							newBounds.setWidth(startPosition.getWidth());
						}
					}
					else
					{
						const bool lockHorizontally = abs(newBounds.getX() - startPosition.getX()) > abs(newBounds.getY() - startPosition.getY());

						if (lockHorizontally)
						{
							newBounds.setY(startPosition.getY());
						}
						else
						{
							newBounds.setX(startPosition.getX());
						}
					}
				}

				if (newBounds.getWidth() < limits.getWidth()) // Skip limiting if the component is bigger than the editor
				{
					if (newBounds.getX() < limits.getX())
					{
						newBounds.setX(limits.getX());
					}
					if (newBounds.getRight() > limits.getRight())
					{
						if (isResizing)
						{
							newBounds.setWidth(limits.getRight() - newBounds.getX());
						}
						else
						{
							newBounds.setX(limits.getRight() - newBounds.getWidth());
						}


					}
					if (newBounds.getY() < limits.getY())
					{
						newBounds.setY(limits.getY());
					}
					if (newBounds.getBottom() > limits.getBottom())
					{
						if (isResizing)
						{
							newBounds.setHeight(limits.getBottom() - newBounds.getY());
						}
						else
						{
							newBounds.setY(limits.getBottom() - newBounds.getHeight());
						}

					}
				}

				currentPosition = Rectangle<int>(newBounds);
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

			void componentMovedOrResized(bool /*wasMoved*/, bool /*wasResized*/) override;

			void componentPeerChanged()  override {}
			void componentVisibilityChanged() override {};

			Component* dragComponent;
		};

		class OverlayAction : public UndoableAction
		{
		public:

			OverlayAction(Dragger* overlay_, bool isResized_, int deltaX_, int deltaY_) :
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

		ScriptEditHandler* parentHandler;

	};

	bool dragMode;


	ScopedPointer<Dragger> dragger;

	ScopedPointer<ShapeButton> dragModeButton;

	ScriptEditHandler* parentHandler;
	
};





#endif  // SCRIPTINGCONTENTOVERLAY_H_INCLUDED
