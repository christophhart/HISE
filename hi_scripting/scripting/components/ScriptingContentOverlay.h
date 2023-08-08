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


#ifndef SCRIPTINGCONTENTOVERLAY_H_INCLUDED
#define SCRIPTINGCONTENTOVERLAY_H_INCLUDED

namespace hise { using namespace juce;



namespace OverlayIcons
{
	DECLARE_DATA(lockShape, 393);
	DECLARE_DATA(penShape, 183);
}


class ScriptingContentOverlay;

class ScriptEditHandler
{
public:

	enum class ComponentType
	{
		Knob = 0x1000,
		Button,
		Table,
		ComboBox,
		Label,
		Image,
		Viewport,
		Panel,
		AudioWaveform,
		SliderPack,
		WebView,
		FloatingTile,
		duplicateComponent,
		numComponentTypes
	};

	ScriptEditHandler();

    virtual ~ScriptEditHandler() {};
    
	virtual ScriptContentComponent* getScriptEditHandlerContent() = 0;

	virtual ScriptingContentOverlay* getScriptEditHandlerOverlay() = 0;

	virtual CommonEditorFunctions::EditorType* getScriptEditHandlerEditor() = 0;

	virtual JavascriptProcessor* getScriptEditHandlerProcessor() = 0;

	Component* getAsComponent() { return dynamic_cast<Component*>(this); };

	void createNewComponent(ComponentType componentType, int x, int y, ScriptComponent* parent=nullptr);

	bool editModeEnabled() const { return useComponentSelectMode; }

	void toggleComponentSelectMode(bool shouldSelectOnClick);

	virtual void selectOnInitCallback() = 0;

	void compileScript();

	virtual void scriptEditHandlerCompileCallback()
	{

	}


private:

	bool useComponentSelectMode = false;

	String isValidComponentName(const String &id);

	Component::SafePointer<ScriptContentComponent> currentContent;

	WeakReference<Processor> currentScriptProcessor;
};

class ScriptingEditor;

class ScriptingContentOverlay : public Component,
								public ButtonListener,
								public ScriptComponentEditListener,
								public LassoSource<ScriptComponent*>
							    
							    
{
public:

	ScriptingContentOverlay(ScriptEditHandler* handler);
	~ScriptingContentOverlay();

	void resized();

	void buttonClicked(Button* buttonThatWasClicked);

	void toggleEditMode();

	bool isEditModeEnabled() const;

	void setEditMode(bool editModeEnabled);

	void setShowEditButton(bool shouldBeVisible)
	{
		dragModeButton->setVisible(shouldBeVisible);
	}

	void paint(Graphics& g) override;

	void scriptComponentSelectionChanged() override;

	void scriptComponentPropertyChanged(ScriptComponent* sc, Identifier idThatWasChanged, const var& newValue) override;

	bool keyPressed(const KeyPress &key) override;

	void setEnablePositioningWithMouse(bool shouldBeEnabled)
	{
		enableMouseDragging = shouldBeEnabled;
	}

	bool isMousePositioningEnabled() const
	{
		return enableMouseDragging;
	}

	struct LassoLaf: public LookAndFeel_V3
	{
		void drawLasso(Graphics& g, Component& c) override;
	} llaf;

	void findLassoItemsInArea(Array<ScriptComponent*> &itemsFound, const Rectangle< int > &area) override;

	SelectedItemSet<ScriptComponent*>& getLassoSelection() override
	{
		return lassoSet;
	}

	void clearDraggers()
	{
		draggers.clear();
	}

	void mouseDown(const MouseEvent& e) override;

	void mouseUp(const MouseEvent &e) override;

	void mouseDrag(const MouseEvent& e) override;

	class Dragger : public Component,
					public ComponentWithDocumentation
	{
	public:

		Dragger(ScriptComponent* sc, Component* componentToDrag);

		~Dragger();

		void paint(Graphics &g) override;

		void mouseDown(const MouseEvent& e);

		void mouseDrag(const MouseEvent& e);

		MarkdownLink getLink() const override;

		void setUseSnapShot(bool shouldUseSnapShot);

		void mouseUp(const MouseEvent& e);

		void resized()
		{
			resizer->setBounds(getWidth() - 10, getHeight() - 10, 10, 10);

			auto b = getLocalBounds();

			auto bb = b.removeFromBottom(5);
			bb.removeFromRight(10);

			auto br = b.removeFromRight(5);
			br.removeFromBottom(10);

			bEdge->setBounds(bb);
			rEdge->setBounds(br);
		}

		void moveOverlayedComponent(int deltaX, int deltaY);

		void resizeOverlayedComponent(int newWidth, int newHeight);

		void duplicateSelection(int deltaX, int deltaY);

		Point<int> getDragDistance() const
		{
			return dragDistance;
		}

		static void learnComponentChanged(Dragger& d, ScriptComponent* newComponent);



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
                    const int R = jlimit<int>(1, 100, HI_RASTER_SIZE);
                    
					if (isResizing)
					{
                        newBounds.setWidth((newBounds.getWidth() / R) * R);
						newBounds.setHeight((newBounds.getHeight() / R) * R);
					}
					else
					{
						newBounds.setX((newBounds.getX() / R) * R);
						newBounds.setY((newBounds.getY() / R) * R);
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

			Rectangle<int> getPosition() const { return currentPosition; }

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

		Component::SafePointer<Component> draggedComponent;
		bool learnModeEnabled = false;
		ScriptComponent* sc;

		ScopedPointer<MovementWatcher> currentMovementWatcher;

		ComponentDragger dragger;

		Point<int> dragDistance;

		Constrainer constrainer;

		ScopedPointer<ResizableCornerComponent> resizer;
		ScopedPointer<ResizableEdgeComponent> lEdge, rEdge, tEdge, bEdge;

		Image snapShot;
		bool copyMode = false;

		ScriptEditHandler* parentHandler;

		Rectangle<int> startBounds;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Dragger);
	};

	struct SelectionMovementWatcher : public ComponentListener,
								      public AsyncUpdater
	{
		SelectionMovementWatcher(ScriptingContentOverlay& parent_) :
			parent(parent_)
		{};

		struct Item
		{
			WeakReference<Dragger> dragger;
			Point<int> topLeftAtDragStart;
		};

		void startDragging(Dragger* newCurrentDragger)
		{
			if (!parent.enableMouseDragging)
				return;

			currentDragger = newCurrentDragger;

			otherDraggers.clear();

			for (auto d : parent.draggers)
			{
				if (d == currentDragger)
					continue;

				Item ni;
				ni.dragger = d;
				d->setUseSnapShot(true);
				ni.topLeftAtDragStart = d->getBoundsInParent().getTopLeft();

				otherDraggers.add(ni);
			}

			currentDragger->addComponentListener(this);
		}

		void endDragging()
		{
			if (!parent.enableMouseDragging)
				return;

			if (currentDragger != nullptr)
				currentDragger->removeComponentListener(this);

			for (auto o : otherDraggers)
			{
				if (o.dragger != nullptr)
					o.dragger->setUseSnapShot(false);
			}

			currentDragger = nullptr;
		}

		void handleAsyncUpdate()
		{
			if (currentDragger != nullptr)
			{
				auto dd = currentDragger->getDragDistance();

				for (auto& o : otherDraggers)
				{
					auto np = o.topLeftAtDragStart.translated(dd.getX(), dd.getY());

					if (o.dragger != nullptr)
						o.dragger->setTopLeftPosition(np);
				}
			}
		}

		void componentMovedOrResized(Component& component,
			bool wasMoved,
			bool wasResized)
		{
			triggerAsyncUpdate();
		}

		Array<Item> otherDraggers;
		WeakReference<Dragger> currentDragger;

		ScriptingContentOverlay& parent;
	} smw;

	bool isDisabledUntilUpdate = false;

	SelectedItemSet<ScriptComponent*> lassoSet;

	bool dragMode;

	bool enableMouseDragging = true;

	OwnedArray<Dragger> draggers;

	ScopedPointer<ShapeButton> dragModeButton;

	LassoComponent<ScriptComponent*> lasso;

	ScriptEditHandler* handler;
};




} // namespace hise
#endif  // SCRIPTINGCONTENTOVERLAY_H_INCLUDED
