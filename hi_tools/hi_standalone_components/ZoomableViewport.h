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
		MouseWatcher(ZoomableViewport& p) :
			parent(p)
		{
			refreshListener();
		}

		~MouseWatcher()
		{
			if (auto c = parent.getContentComponent())
				c->removeMouseListener(this);
		}

		void refreshListener()
		{
			if (auto c = parent.getContentComponent())
			{
				c->addMouseListener(this, true);
				
			}
		}

		void setToMidAfterResize()
		{
			if (auto c = parent.getContentComponent())
			{
				mousePos = parent.getLocalBounds().toFloat().getCentre();
				graphMousePos = parent.getContentComponent()->getLocalBounds().getCentre().toFloat();
			}
		}

		void mouseMove(const MouseEvent& e) override
		{
			mousePos = e.getEventRelativeTo(&parent).getPosition().toFloat();
			graphMousePos = e.getEventRelativeTo(parent.getContentComponent()).getPosition().toFloat();
		}

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

	void makeSwapSnapshot(float newSwapScale)
	{
		swapImage = content->createComponentSnapshot(content->getLocalBounds(), true, zoomFactor);
		swapBounds = content->getBoundsInParent().toFloat();
		swapScale = newSwapScale;
		swapAlpha = 1.0f;
		content->setVisible(false);
		repaint();
		startTimer(30);
	}

	void clearSwapSnapshot()
	{
		swapImage = {};
		content->setVisible(true);
		content->setAlpha(swapAlpha);
		repaint();
	}

	void zoomToRectangle(Rectangle<int> areaToShow);

	void setZoomFactor(float newZoomFactor, Point<float> centerPositionInGraph);

    void setMaxZoomFactor(float newMaxZoomFactor)
    {
        maxZoomFactor = newMaxZoomFactor;
    }
    
	bool changeZoom(bool zoomIn);

	void componentMovedOrResized(Component& component,
		bool wasMoved,
		bool wasResized) override;


	void scrollBarMoved(ScrollBar* scrollBarThatHasMoved,
		double newRangeStart) override;

	Rectangle<int> getCurrentTarget() const
	{
		return dark.ruler.toNearestInt();
	}

	void setCurrentModalWindow(Component* newComponent, Rectangle<int> target);

	void setNewContent(Component* newContentToDisplay, Component* target);

	struct Holder : public Component,
		public ComponentListener
	{
		static constexpr int margin = 20;
		static constexpr int headerHeight = 32;

		Holder(Component* content_, Rectangle<int> targetArea_) :
			content(content_),
			target(targetArea_)
		{
			addAndMakeVisible(content);

			content->addComponentListener(this);

			updateSize();
		}

		Image bg;
		PostGraphicsRenderer::DataStack stack;

		bool recursive = false;

		void setBackground(Image img);

		void updateSize()
		{
			setSize(content->getWidth() + margin, content->getHeight() + margin + headerHeight);
		}

		void componentMovedOrResized(Component&, bool, bool wasResized)
		{
			if (wasResized)
			{
				updateSize();
				updatePosition();
			}
		}

		void updatePosition()
		{
			if (getParentComponent() == nullptr)
			{
				jassertfalse;
				return;
			}

			auto thisBounds = getBoundsInParent();

			auto parentBounds = getParentComponent()->getLocalBounds();

			if (parentBounds.getWidth() - 150 > thisBounds.getWidth() || parentBounds.getHeight() - 150)
			{
				lockPosition = true;

				auto newBounds = parentBounds.withSizeKeepingCentre(thisBounds.getWidth(), thisBounds.getHeight());

				Point<int> x(newBounds.getX(), newBounds.getY());

				setTopLeftPosition(x.transformedBy(getTransform().inverted()));
				updateShadow();
				return;
			}

			bool alignY = target.getWidth() > target.getHeight();

			auto rect = target.withSizeKeepingCentre(getWidth(), getHeight());

			if (alignY)
			{
				auto delta = target.getHeight() / 2 + 20;

				auto yRatio = (float)target.getY() / (float)getParentComponent()->getHeight();

				if (yRatio > 0.5f)
				{
					if (lockPosition) // this prevents jumping around...
					{
						updateShadow();
						return;
					}

					rect.translate(0, -rect.getHeight() / 2 - delta);
					lockPosition = true;
				}
				else
					rect.translate(0, rect.getHeight() / 2 + delta);
			}
			else
			{
				auto delta = target.getWidth() / 2 + 20;

				auto xRatio = (float)target.getX() / (float)getParentComponent()->getWidth();

				if (xRatio > 0.5f)
				{
					if (lockPosition) // this prevents jumping around...
					{
						updateShadow();
						return;
					}

					rect.translate(-rect.getWidth() / 2 - delta, 0);
					lockPosition = true;
				}
				else
					rect.translate(rect.getWidth() / 2 + delta, 0);

				rect.setY(target.getY());
			}

			setTopLeftPosition(rect.getTopLeft());
			updateShadow();
		}

		void updateShadow()
		{
			auto b = getBoundsInParent();

			findParentComponentOfClass<ZoomableViewport>()->dark.setRuler(target, b);
		}

		void paint(Graphics& g) override;

		void mouseDown(const MouseEvent& event) override
		{
			dragger.startDraggingComponent(this, event);
		}

		void mouseDrag(const MouseEvent& event) override
		{
			dragger.dragComponent(this, event, nullptr);
			updateShadow();
		}

		void resized()
		{
			auto b = getLocalBounds();
			b.removeFromTop(headerHeight);
			b = b.reduced(margin / 2);
			content->setBounds(b);
		}

		Rectangle<int> target;

		ScopedPointer<Component> content;

		bool lockPosition = false;

		ComponentDragger dragger;
	};

	struct Dark : public Component
	{
		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0xff1d1d1d).withAlpha(0.5f));
			g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.1f));
			float width = 2.0f;
			g.fillRoundedRectangle(ruler, width);

			DropShadow sh;
			sh.colour = Colours::black.withAlpha(0.5f);
			sh.radius = 40;
			sh.drawForRectangle(g, shadow);
		}

		void setRuler(Rectangle<int> area, Rectangle<int> componentArea)
		{
			ruler = area.toFloat();
			shadow = componentArea;
			repaint();
		}

		void mouseDown(const MouseEvent&) override
		{
			findParentComponentOfClass<ZoomableViewport>()->setCurrentModalWindow(nullptr, {});
		}

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

	Component* getContentComponent()
	{
		return content.get();
	}

	const Component* getContentComponent() const
	{
		return content.get();
	}

	struct ZoomListener
	{
		virtual ~ZoomListener() {};

		virtual void zoomChanged(float newScalingFactor) = 0;

		JUCE_DECLARE_WEAK_REFERENCEABLE(ZoomListener);
	};

	void addZoomListener(ZoomListener* l)
	{
		listeners.addIfNotAlreadyThere(l);
	}

	void removeZoomListener(ZoomListener* l)
	{
		listeners.removeAllInstancesOf(l);
	}

	void setScrollOnDragEnabled(bool shouldBeEnabled)
	{
		if (shouldBeEnabled != dragToScroll)
		{
			dragToScroll = shouldBeEnabled;

			setMouseCursor(shouldBeEnabled ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
		}
	}

	void setMouseWheelScrollEnabled(bool shouldBeEnabled)
	{
		mouseWheelScroll = shouldBeEnabled;
	}

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
