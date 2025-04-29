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
*   which must be separately licensed for closed source applications:
*
*   http://www.juce.com
*
*   ===========================================================================
*/


#pragma once

namespace hise
{
using namespace juce;

using DragAnimator = juce::AnimatedPosition<juce::AnimatedPositionBehaviours::ContinuousWithMomentum>;

enum class MouseEventFlags
{
	Down,
	Up,
	Drag
};

struct SubmenuComboBox: public juce::ComboBox
{
	SubmenuComboBox(const String& name={}):
	  ComboBox(name),
	  updater(*this)
	{}

	static juce::PopupMenu parseFromStringArray(const StringArray& itemList, Array<int> activeIndexes, LookAndFeel* laf);

	virtual void createPopupMenu(PopupMenu& m, const StringArray& items, const Array<int>& indexList)
	{
		m = parseFromStringArray(items, indexList, &getLookAndFeel());
	}

	virtual bool useCustomPopupMenu() const { return customPopup; }

    void setUseCustomPopup(bool shouldUse)
    {
        if(customPopup != shouldUse)
        {
            customPopup = shouldUse;

			original = *getRootMenu();

			updater.refreshEnableState();
            rebuildPopupMenu();
        }
    }

    void rebuildPopupMenu();

	void refreshTickState()
	{
		auto currentId = getSelectedId();

		for (PopupMenu::MenuItemIterator iterator (*getRootMenu(), false); iterator.next();)
		{
			auto& item = iterator.getItem();

			if(auto s = item.subMenu.get())
			{
				item.isTicked = isTicked(*s, currentId);
			}
		}
	}

private:

	bool isTicked(PopupMenu& m, int currentId) const
	{
		for (PopupMenu::MenuItemIterator iterator (m, false); iterator.next();)
		{
			auto& item = iterator.getItem();

			if(item.itemID == currentId)
				return true;

			if(auto s = item.subMenu.get())
			{
				auto subTicked = isTicked(*s, currentId);

				if(subTicked)
					return true;
			}
		}

		return false;
	}

	struct Updater: public ComboBoxListener
	{
		Updater(SubmenuComboBox& parent_):
		  parent(parent_)
		{};
		
		void refreshEnableState()
		{
			auto shouldBeEnabled = parent.useCustomPopupMenu();

			if(enabled != shouldBeEnabled)
			{
				enabled = shouldBeEnabled;

				if(enabled)
					parent.addListener(this);
				else
					parent.removeListener(this);
			}
		}

		

		void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override
		{
			SafeAsyncCall::callAsyncIfNotOnMessageThread<Updater>(*this, [](Updater& u)
			{
				u.parent.refreshTickState();
				
			});
		}

	private:

		SubmenuComboBox& parent;
		bool enabled = false;

		JUCE_DECLARE_WEAK_REFERENCEABLE(Updater);
	} updater;


	PopupMenu original;
	bool customPopup = false;

};

struct ZoomableViewport : public Component,
	public ScrollBar::Listener,
	public ComponentListener,
	public Timer,
	public DragAnimator::Listener
{
	struct DragScrollTimer: public Timer
	{
		DragScrollTimer(ZoomableViewport& parent_):
		  parent(parent_)
		{};

		void timerCallback() override;

		void setPosition(const MouseEvent& e, bool isMouseUp);

		bool wasInCentre = false;
		int xDelta = 0;
		int yDelta = 0;

		double lx = 0.0;
		double ly = 0.0;

		void scrollToPosition(Point<double> normalisedTargetPosition)
		{
			wasInCentre = false;
			lx = 0.0;
			ly = 0.0;
			scrollAnimationStart = { parent.hBar.getCurrentRangeStart(), parent.vBar.getCurrentRangeStart() };
			scrollAnimationTarget = normalisedTargetPosition;
			scrollAnimationCounter = 0;
			startTimer(15);
		}

		Point<double> scrollAnimationStart;
		Point<double> scrollAnimationTarget;
		int scrollAnimationCounter = -1;
		

		ZoomableViewport& parent;

	} dragScrollTimer;

	

	enum ColourIds
	{
		backgroundColourId = 9000
	};

    using Laf = ScrollbarFader::Laf;
    
	Laf slaf;

	struct MouseWatcher : public MouseListener
	{
		MouseWatcher(ZoomableViewport& p);

		~MouseWatcher();

		void refreshListener();

		void setToMidAfterResize();

		void mouseMove(const MouseEvent& e) override;

		Point<float> getDeltaAfterResize();
		Point<float> mousePos, graphMousePos;

		ZoomableViewport& parent;
	};

	ZoomableViewport(Component* contentComponent);

	virtual ~ZoomableViewport();

	

	static bool checkDragScroll(const MouseEvent& e, bool isMouseUp);

	static bool checkViewportScroll(const MouseEvent& e, const MouseWheelDetails& details);

	static bool checkMiddleMouseDrag(const MouseEvent& e, MouseEventFlags type);

	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
    
    void mouseMagnify(const MouseEvent& e, float scaleFactor) override;
    
	void resized() override;

	void paint(Graphics& g) override;

	void positionChanged(DragAnimator&, double newPosition) override;

	void timerCallback() override;

	void makeSwapSnapshot(float newSwapScale);

	void clearSwapSnapshot();

	void zoomToRectangle(Rectangle<int> areaToShow);

	void scrollToRectangle(Rectangle<int> areaToShow, bool skipIfVisible, bool animate=true);

	void setZoomFactor(float newZoomFactor, Point<float> centerPositionInGraph);

    void setMaxZoomFactor(float newMaxZoomFactor);

	bool changeZoom(bool zoomIn);

	void componentMovedOrResized(Component& component,
		bool wasMoved,
		bool wasResized) override;


	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved,
		double newRangeStart) override;

	Rectangle<int> getCurrentTarget() const;

	void setCurrentModalWindow(Component* newComponent, Rectangle<int> target);

	void setNewContent(Component* newContentToDisplay, Component* target);

	struct Holder : public Component,
		public ComponentListener
	{
		static constexpr int margin = 20;
		static constexpr int headerHeight = 32;

		Holder(Component* content_, Rectangle<int> targetArea_);

		Image bg;
		PostGraphicsRenderer::DataStack stack;

		bool recursive = false;

		void setBackground(Image img);

		void updateSize();

		void componentMovedOrResized(Component&, bool, bool wasResized);

		void updatePosition();

		void updateShadow();

		void paint(Graphics& g) override;

		void mouseDown(const MouseEvent& event) override;

		void mouseDrag(const MouseEvent& event) override;

		void resized();

		Rectangle<int> target;

		ScopedPointer<Component> content;

		bool lockPosition = false;

		ComponentDragger dragger;
	};

	struct Dark : public Component
	{
		void paint(Graphics& g) override;

		void setRuler(Rectangle<int> area, Rectangle<int> componentArea);

		void mouseDown(const MouseEvent&) override;

		Rectangle<float> ruler;
		Rectangle<int> shadow;

	} dark;

	void centerCanvas();

	float zoomFactor = 1.0f;

	juce::ScrollBar hBar, vBar;



	Rectangle<float> swapBounds;
	Image swapImage;
	float swapScale;
	float swapAlpha;

	void refreshScrollbars();
	void refreshPosition();

	ScopedPointer<Holder> currentModalWindow;

	ScopedPointer<Component> pendingNewComponent;

	bool positionInitialised = false;

	template <typename T> T* getContent()
	{
		return dynamic_cast<T*>(getContentComponent());
	}

	template <typename T> const T* getContent() const
	{
		return dynamic_cast<const T*>(getContentComponent());
	}

	Component* getContentComponent();

	const Component* getContentComponent() const;

	struct ZoomListener
	{
		virtual ~ZoomListener();;

		virtual void zoomChanged(float newScalingFactor) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ZoomListener);
	};

	void addZoomListener(ZoomListener* l);

	void removeZoomListener(ZoomListener* l);

	void setScrollOnDragEnabled(bool shouldBeEnabled);

	void setMouseWheelScrollEnabled(bool shouldBeEnabled);

	std::function<void(Component*)> contentFunction;

	bool keyPressed(const KeyPress& key) override
	{
        return WASDScroller::checkWASD(key);
	}

	bool keyStateChanged (bool isKeyDown) override
    {
        return scroller.keyChangedWASD(isKeyDown, getContent<Component>());
    }

	/** Set a function that will return true or false based on the currently focused component. */
	void setEnableWASD(const std::function<bool(Component*)>& checkFunction)
	{
		scroller.checkFunction = checkFunction;
	}

private:

	struct WASDScroller: public juce::Timer
    {
		WASDScroller(ZoomableViewport& parent_):
		  parent(parent_)
		{}

        void timerCallback() override;

		void setDelta(Point<float> delta);

		static bool checkWASD(const KeyPress & key);

		bool keyChangedWASD(bool isKeyDown, Component* c);

		ZoomableViewport& parent;

		std::function<bool(Component*)> checkFunction;
        Point<float> currentVelocity;
        Point<float> currentDelta;
        
    } scroller;

    float maxZoomFactor = 3.0f;

	bool dragToScroll = false;
	bool mouseWheelScroll = true;

	Point<double> normDragStart;
	Point<double> scrollPosDragStart;

    ScrollbarFader sf;
	
	DragAnimator xDragger, yDragger;
	
	Array<WeakReference<ZoomListener>> listeners;

	ScopedPointer<Component> content;
	ScopedPointer<MouseWatcher> mouseWatcher;
};


#if USE_BACKEND
#if HISE_INCLUDE_PROFILING_TOOLKIT
#define CHECK_TRIGGER(e) if(auto pc = dynamic_cast<ProfiledComponent*>(this)) pc->checkTriggerRecord(e);
#else
#define CHECK_TRIGGER(e) ;

#endif


#define CHECK_MIDDLE_MOUSE_DOWN(e) CHECK_TRIGGER(true); if(ZoomableViewport::checkMiddleMouseDrag(e, MouseEventFlags::Down)) return;
#define CHECK_MIDDLE_MOUSE_UP(e) CHECK_TRIGGER(false); if(ZoomableViewport::checkMiddleMouseDrag(e, MouseEventFlags::Up)) return;
#define CHECK_MIDDLE_MOUSE_DRAG(e) if(ZoomableViewport::checkMiddleMouseDrag(e, MouseEventFlags::Drag)) return;
#define CHECK_VIEWPORT_SCROLL(e, details) if(ZoomableViewport::checkViewportScroll(e, details)) return;

struct ComponentWithMiddleMouseDrag: public Component
{
	void mouseDown(const MouseEvent& e) override
	{
		CHECK_MIDDLE_MOUSE_DOWN(e);
	}
	void mouseUp(const MouseEvent& e) override
	{
		CHECK_MIDDLE_MOUSE_UP(e);
	}
	void mouseDrag(const MouseEvent& e) override
	{
		CHECK_MIDDLE_MOUSE_DRAG(e);
	}
};
#else
#define CHECK_MIDDLE_MOUSE_DOWN(e) ignoreUnused(e);
#define CHECK_MIDDLE_MOUSE_UP(e) ignoreUnused(e);
#define CHECK_MIDDLE_MOUSE_DRAG(e) ignoreUnused(e);
#define CHECK_VIEWPORT_SCROLL(e, details) ignoreUnused(e, details);
using ComponentWithMiddleMouseDrag = juce::Component;
#endif

using Animator = AnimatedPosition<AnimatedPositionBehaviours::ContinuousWithMomentum>;

struct AbstractZoomableView: public ScrollBar::Listener,
							 public Animator::Listener,
							 public TooltipClientWithCustomPosition
{
	static constexpr int ScrollbarHeight = 14;
	static constexpr int FirstItemOffset = 18;

	enum class MouseEventType
	{
		MouseDown,
		MouseUp,
		MouseDrag,
		MouseMove,
		MouseDoubleClick
	};

	struct WASDKeyListener: public juce::KeyListener,
							public Timer 
	{
		enum class Direction: uint8
		{
			Up = 'q',
			Down = 'e',
			Left = 'a',
			Right = 'd',
			In = 'w',
			Out = 's'
		};

		enum class MovementType
		{
			Vertical,
			Horizontal,
			Zoom,
			numMovementTypes
		};

		static constexpr int NumMovementTypes = (int)MovementType::numMovementTypes;

		WASDKeyListener(Component& c);
		~WASDKeyListener();

		static int getDelta(Direction d) { return (d == Direction::Up || d == Direction::Right || d == Direction::In) ? 1 : -1; }
		static bool isHorizontal(Direction d) { return d == Direction::Left || d == Direction::Right; }
		static bool isVertical(Direction d) { return d == Direction::Up || d == Direction::Down; }
		static bool isZoom(Direction d) { return d == Direction::In || d == Direction::Out; }

		static bool isDirection(int kc)
		{
			return kc == (int)Direction::In || kc == (int)Direction::Out || kc == (int)Direction::Left ||
                   kc == (int)Direction::Right || kc == (int)Direction::Up || kc == (int)Direction::Down;   
		}
		static MovementType getMovementType(Direction d) { return isHorizontal(d) ? MovementType::Horizontal : (isVertical(d) ? MovementType::Vertical : MovementType::Zoom); }

		bool keyPressed(const KeyPress& key, Component* originatingComponent) override;
		void setShiftMultiplier(double horizontalFactor, double verticalFactor, double zoomFactor);
		void timerCallback() override;
		bool keyStateChanged (bool isKeyDown, Component* originatingComponent) override;

		LinearSmoothedValue<double> delta[NumMovementTypes];
		double shiftMultipliers[NumMovementTypes] = { 3.0, 3.0, 3.0 };
		std::map<Direction, bool> state;
		AnimatedPositionBehaviours::ContinuousWithMomentum behaviour[NumMovementTypes];
		std::function<void(MovementType, double)> onMovement;
		Component& p;
	};

	struct DragZoomAction: public UndoableAction
	{
		DragZoomAction(AbstractZoomableView& parent_, Range<double> newRange_, Range<double> oldRange_, double newZoom_, double oldZoom_);

		bool perform() override;
		bool undo() override;

		AbstractZoomableView& parent;
		Range<double> oldRange, newRange;
		double oldZoom, newZoom;
	};

	struct ZoomAnimator: public Timer
	{
		ZoomAnimator(AbstractZoomableView& parent_):
		  parent(parent_)
		{}

		void setTarget(Range<double> newTarget, double newZoom);
		void timerCallback() override;

		AbstractZoomableView& parent;
		Range<double> start;
		double startZoom;
		double alpha = 0.0f;
		Range<double> target;
		double zoom;
	};

	AbstractZoomableView();

	virtual ~AbstractZoomableView()
	{
		scrollbar.removeListener(this);
	}

	/** Call this from your constructor to initialise the zoom handler. */
	void setAsParentComponent(Component* p);

	/** Override this and return the text for the distance of the drag zoom. */
	virtual String getTextForDistance(float width) const = 0;

	/** Override this and return the total absolute length of the items you want to display. */
	virtual double getTotalLength() const = 0;

	void changeZoom(double newZoomFactor, int xPos=0);
	void scrollBarMoved (ScrollBar* , double newRangeStart) override;

	void drawDragDistance(Graphics& g, Rectangle<int> viewportPosition);
	void drawBackgroundGrid(Graphics& g, Rectangle<int> viewportPosition);

	void onResize(Rectangle<int> viewportPosition);
	void positionChanged(Animator&, double newPosition) override;

	void handleMouseWheelEvent(const MouseEvent& e, const MouseWheelDetails& wheel);
    void hangleMouseMagnify(const MouseEvent& e, float scaleFactor);
	bool handleMouseEvent(const MouseEvent& e, MouseEventType t);
	void zoomToRange(Range<double> newScaledRange);
	void moveToRange(double normalisedNewXPosition);
	void updateMouseCursorForEvent(const MouseEvent& e);

	double totalLength = 0.0;
	Animator animator;
	double scaleFactor = 1.0;
	double zoomFactor = 1.0;
	
	double first = -1.0;
	float x = 0.0f;

protected:

	virtual bool onWASDMovement(WASDKeyListener::MovementType t, double delta);

	virtual UndoManager* getUndoManager() = 0;

	float getZoomScale() const;
	float getZoomTranspose() const;
	Range<double> getScaledZoomRange() const;
	Range<double> getNormalisedZoomRange() const;
	void updateIndexToShow();

	ScrollbarFader& getFader() { return sf; }

	bool useIntegerGrid = false;

private:

	Component* asComponent = nullptr;

	ZoomAnimator zoomAnimator;

	bool showDragDistance = false;
	Range<int> dragDistance;

	ScrollBar scrollbar;
	ScrollbarFader sf;
	ScopedPointer<WASDKeyListener> wasd;
};

}
