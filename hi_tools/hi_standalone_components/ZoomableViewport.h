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

struct ZoomableViewport : public Component,
	public ScrollBar::Listener,
	public ComponentListener,
	public Timer,
	public DragAnimator::Listener
{
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

private:
	
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
#define CHECK_MIDDLE_MOUSE_DOWN(e) if(ZoomableViewport::checkMiddleMouseDrag(e, MouseEventFlags::Down)) return;
#define CHECK_MIDDLE_MOUSE_UP(e) if(ZoomableViewport::checkMiddleMouseDrag(e, MouseEventFlags::Up)) return;
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

}
