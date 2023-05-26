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

		~MouseWatcher()
		{
			if (auto c = parent.getContentComponent())
				c->removeMouseListener(this);
		}

		ZoomableViewport& parent;
	};

	ZoomableViewport(Component* contentComponent);

	virtual ~ZoomableViewport();


	void mouseDown(const MouseEvent& e) override;

	void mouseDrag(const MouseEvent& e) override;

	void mouseUp(const MouseEvent& e) override;

	void mouseWheelMove(const MouseEvent& event, const MouseWheelDetails& wheel) override;
    
    void mouseMagnify(const MouseEvent& e, float scaleFactor) override;
    
	void resized() override;

	void paint(Graphics& g) override;

	void positionChanged(DragAnimator&, double newPosition) override;

	void timerCallback() override
	{
		swapBounds = swapBounds.transformedBy(AffineTransform::scale(swapScale, swapScale, swapBounds.getCentreX(), swapBounds.getCentreY()));

		if (getContentComponent()->isVisible())
		{
			swapAlpha *= JUCE_LIVE_CONSTANT_OFF(1.2f);

			getContentComponent()->setAlpha(swapAlpha);

			if (swapAlpha >= 1.0f)
			{
				stopTimer();
			}
		}
		else
		{
			swapAlpha *= JUCE_LIVE_CONSTANT_OFF(0.9f);
		}

		repaint();
	}

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

struct WrapperWithMenuBarBase : public Component,
	public ComboBoxListener,
	public ZoomableViewport::ZoomListener,
	public Timer
{
	struct ButtonWithStateFunction
	{
        virtual ~ButtonWithStateFunction() {};
		virtual bool hasChanged() = 0;
	};

	static Component* showPopup(FloatingTile* ft, Component* parent, const std::function<Component*()>& createFunc, bool show);

	template <typename ContentType, typename PathFactoryType> struct ActionButtonBase :
		public Component,
		public ButtonWithStateFunction,
		public SettableTooltipClient
	{
		ActionButtonBase(ContentType* parent_, const String& name) :
			Component(name),
			parent(parent_)
		{
			PathFactoryType f;

			p = f.createPath(name);
			setSize(MenuHeight, MenuHeight);
			setRepaintsOnMouseActivity(true);

			setColour(TextButton::ColourIds::buttonOnColourId, Colour(SIGNAL_COLOUR));
			setColour(TextButton::ColourIds::buttonColourId, Colour(0xFFAAAAAA));
		}

		virtual ~ActionButtonBase()
		{

		}

		bool hasChanged() override
		{
			bool changed = false;

			if (stateFunction)
			{
				auto thisState = stateFunction(*parent);
				changed |= thisState != lastState;
				lastState = thisState;
				
			}

			if (enabledFunction)
			{
				auto thisState = enabledFunction(*parent);
				changed |= thisState != lastEnableState;
				lastEnableState = thisState;
			}

			return changed;
		}

		void triggerClick(NotificationType sendNotification)
		{
			if (enabledFunction && !enabledFunction(*parent))
				return;

			if (actionFunction)
				actionFunction(*parent);

			SafeAsyncCall::repaint(this);
		}

		/** Call this function with a lambda that creates a component and it will be shown as Floating Tile popup. */
		void setControlsPopup(const std::function<Component*()>& createFunc)
		{
			stateFunction = [this](ContentType&)
			{
				return this->currentPopup != nullptr;
			};

			actionFunction = [this, createFunc](ContentType&)
			{
				auto ft = findParentComponentOfClass<FloatingTile>();

				this->currentPopup = showPopup(ft, this, createFunc, this->currentPopup != nullptr);

				return false;
			};
		}
		

		void paint(Graphics& g) override
		{
			auto on = stateFunction ? stateFunction(*parent) : false;
			auto enabled = enabledFunction ? enabledFunction(*parent) : true;
			auto over = isMouseOver(false);
			auto down = isMouseButtonDown(false);

			Colour c = findColour(on ? TextButton::ColourIds::buttonOnColourId : TextButton::ColourIds::buttonColourId);

			float alpha = 1.0f;

			if (!enabled)
				alpha *= 0.6f;

			if (!over)
				alpha *= 0.9f;
			if (!down)
				alpha *= 0.9f;

			c = c.withAlpha(alpha);

			g.setColour(c);

			PathFactory::scalePath(p, getLocalBounds().toFloat().reduced(down && enabled ? 5.0f : 4.0f));

			g.fillPath(p);
		}

		void mouseDown(const MouseEvent& e) override
		{
			triggerClick(sendNotificationSync);
		}

		void resized() override
		{
			PathFactory::scalePath(p, this, 2.0f);
		}

		Path p;

		using Callback = std::function<bool(ContentType& g)>;

		Component::SafePointer<ContentType> parent;
		Callback stateFunction;
		Callback enabledFunction;
		Callback actionFunction;
		bool lastState = false;
		bool lastEnableState = false;

		Component::SafePointer<Component> currentPopup;

		bool isPopupShown = false;
	};

	constexpr static int MenuHeight = 24;

	struct Spacer : public Component
	{
		Spacer(int width)
		{
			setSize(width, MenuHeight);
		}
	};

	WrapperWithMenuBarBase(Component* contentComponent) :
		canvas(contentComponent)
	{
		addAndMakeVisible(canvas);
		canvas.addZoomListener(this);
        canvas.setMaxZoomFactor(1.5f);
		startTimer(100);
	}

	/** Override this method and initialise the menu bar here. */
	virtual void rebuildAfterContentChange() = 0;

	virtual bool isValid() const { return true; }

	template <typename ComponentType> ComponentType* getComponentWithName(const String& id)
	{
		for (auto b : actionButtons)
		{
			if (b->getName() == id)
			{
				if (auto typed = dynamic_cast<ComponentType*>(b))
					return typed;
			}
		}

		jassertfalse;
		return nullptr;
	}
	
	void setContentComponent(Component* newContent)
	{
		actionButtons.clear();
		bookmarkBox = nullptr;
		rebuildAfterContentChange();
		resized();
	}

	void timerCallback() override
	{
		for (auto a : actionButtons)
		{
			if (!isValid())
				break;

			if (auto asB = dynamic_cast<ButtonWithStateFunction*>(a))
			{
				if (asB->hasChanged())
					a->repaint();
			}
		}
	}

	void addSpacer(int width)
	{
		auto p = new Spacer(width);
		actionButtons.add(p);
		addAndMakeVisible(p);
	}

	void updateBookmarks(ValueTree, bool)
	{
		StringArray sa;

		for (auto b : bookmarkUpdater.getParentTree())
		{
			sa.add(b["ID"].toString());
		}

        sa.add("Add new bookmark");
        
		auto currentIdx = bookmarkBox->getSelectedId();
		bookmarkBox->clear(dontSendNotification);
		bookmarkBox->addItemList(sa, 1);
		bookmarkBox->setSelectedId(currentIdx, dontSendNotification);
	}

    virtual int bookmarkAdded() { return -1; };
    
	virtual void zoomChanged(float newScalingFactor) {};

	virtual void bookmarkUpdated(const StringArray& idsToShow) = 0;
	virtual ValueTree getBookmarkValueTree() = 0;

	void comboBoxChanged(ComboBox* c) override
	{
        auto isLastEntry = c->getSelectedItemIndex() == c->getNumItems() - 1;
        
        if(isLastEntry)
        {
            auto idx = bookmarkAdded();
            
            if(idx == -1)
                c->setSelectedId(0, dontSendNotification);
            else
                c->setSelectedItemIndex(idx, dontSendNotification);
            
            return;
        }
        
		auto bm = bookmarkUpdater.getParentTree().getChildWithProperty("ID", bookmarkBox->getText());

		if (bm.isValid())
		{
			auto l = StringArray::fromTokens(bm["Value"].toString(), ";", "");
			bookmarkUpdated(l);
		}
	}

	void addBookmarkComboBox()
	{
		bookmarkBox = new ComboBox();

		bookmarkBox->setLookAndFeel(&glaf);
		bookmarkBox->addListener(this);

		glaf.setDefaultColours(*bookmarkBox);

		auto cTree = getBookmarkValueTree();

		bookmarkUpdater.setCallback(cTree, valuetree::AsyncMode::Asynchronously, BIND_MEMBER_FUNCTION_2(WrapperWithMenuBarBase::updateBookmarks));
        
        updateBookmarks({}, true);
		bookmarkBox->setSize(100, 24);
		actionButtons.add(bookmarkBox);
		addAndMakeVisible(bookmarkBox);
	}

	virtual void addButton(const String& name) = 0;

	void paint(Graphics& g) override
	{
		GlobalHiseLookAndFeel::drawFake3D(g, getLocalBounds().removeFromTop(MenuHeight));
	}

	void addCustomComponent(Component* c)
	{
		// must be sized before passing in here
		jassert(!c->getBounds().isEmpty());

		addAndMakeVisible(c);
		actionButtons.add(c);
	}

	void setPostResizeFunction(const std::function<void(Component*)>& f)
	{
		resizeFunction = f;
	}

	void resized() override
	{
		auto b = getLocalBounds();
		auto menuBar = b.removeFromTop(MenuHeight);

		for (auto ab : actionButtons)
		{
			ab->setTopLeftPosition(menuBar.getTopLeft());
			menuBar.removeFromLeft(ab->getWidth() + 3);
		}

		canvas.setBounds(b);

		if (resizeFunction)
			resizeFunction(canvas.getContentComponent());
	}
    
	std::function<void(Component*)> resizeFunction;

	ZoomableViewport canvas;
	OwnedArray<Component> actionButtons;

	GlobalHiseLookAndFeel glaf;
	ComboBox* bookmarkBox;

	valuetree::ChildListener bookmarkUpdater;
};

}
