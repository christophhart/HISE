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

namespace hise
{
using namespace juce;

struct Helpers
{
	static double normToPixel(double normValue, double cSize, double totalSize)
	{
		auto minPosition = totalSize * 0.25 - cSize;
		auto maxPosition = totalSize * 0.75;
		auto delta = maxPosition - minPosition;
		auto y = minPosition + (1.0 - normValue) * delta;
		return y;
	}

	static double pixelToNorm(double pixelValue, double cSize, double totalSize)
	{
		auto minPosition = totalSize * 0.25 - cSize;
		auto maxPosition = totalSize * 0.75;
		auto delta = maxPosition - minPosition;

		auto norm = (pixelValue - minPosition) / delta;
		norm = 1.0 - norm;

		return norm;
	}
};

ZoomableViewport::ZoomableViewport(Component* n) :
	hBar(false),
	vBar(true),
	content(n),
	mouseWatcher(new MouseWatcher(*this))
{
    sf.addScrollBarToAnimate(hBar);
    sf.addScrollBarToAnimate(vBar);
    
	setColour(ColourIds::backgroundColourId, Colour(0xff1d1d1d));

	content->addComponentListener(this);

	hBar.setLookAndFeel(&slaf);
	vBar.setLookAndFeel(&slaf);

	vBar.setColour(ScrollBar::ColourIds::thumbColourId, Colours::white.withAlpha(0.5f));

	vBar.setRangeLimits({ 0.0, 1.0 });
	hBar.setRangeLimits({ 0.0, 1.0 });

	addAndMakeVisible(content);
	addAndMakeVisible(hBar);
	addAndMakeVisible(vBar);

	addAndMakeVisible(dark);
	dark.setVisible(false);
	setOpaque(true);

	hBar.addListener(this);
	vBar.addListener(this);

	hBar.setRangeLimits({ 0.0, 1.2 }, sendNotificationSync);
	vBar.setRangeLimits({ 0.0, 1.2 }, sendNotificationSync);

	setScrollOnDragEnabled(true);

	xDragger.addListener(this);
	yDragger.addListener(this);

	
	
}

ZoomableViewport::~ZoomableViewport()
{
	mouseWatcher = nullptr;
	content = nullptr;
}

bool ZoomableViewport::checkViewportScroll(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	if (wheel.deltaX > 0.0 || wheel.deltaY > 0.0)
	{
		if (auto vp = e.eventComponent->findParentComponentOfClass<ZoomableViewport>())
		{
			auto ve = e.getEventRelativeTo(vp);
			vp->mouseWheelMove(e, wheel);
			return true;
		}
	}

	return false;
}

bool ZoomableViewport::checkMiddleMouseDrag(const MouseEvent& e, MouseEventFlags type)
{
	if (e.mods.isMiddleButtonDown())
	{
		if (auto vp = e.eventComponent->findParentComponentOfClass<ZoomableViewport>())
		{
			auto ve = e.getEventRelativeTo(vp);

			switch (type)
			{
			case MouseEventFlags::Down:
				vp->mouseDown(ve);
				e.eventComponent->setMouseCursor(MouseCursor::DraggingHandCursor);
				break;
			case MouseEventFlags::Up:
				vp->mouseUp(ve);
				e.eventComponent->setMouseCursor(MouseCursor::NormalCursor);
				break;
			case MouseEventFlags::Drag:
				vp->mouseDrag(ve);
				break;
			}
		}

		return true;
	}

	return false;
}

void ZoomableViewport::mouseDown(const MouseEvent& e)
{
	auto cBounds = content->getBoundsInParent().toDouble();
	auto tBounds = getLocalBounds().toDouble();

	auto normX = Helpers::pixelToNorm((double)e.getPosition().getX(), cBounds.getWidth(), tBounds.getWidth());
	auto normY = Helpers::pixelToNorm((double)e.getPosition().getY(), cBounds.getHeight(), tBounds.getHeight());

	normDragStart = { normX, normY };
	scrollPosDragStart = { hBar.getCurrentRangeStart(), vBar.getCurrentRangeStart() };

	xDragger.setPosition(hBar.getCurrentRangeStart());
	yDragger.setPosition(vBar.getCurrentRangeStart());
	xDragger.beginDrag();
	yDragger.beginDrag();


	auto fr = JUCE_LIVE_CONSTANT_OFF(0.08);
	auto mv = JUCE_LIVE_CONSTANT_OFF(0.12);

	xDragger.behaviour.setFriction(fr);
	yDragger.behaviour.setFriction(fr);
	xDragger.behaviour.setMinimumVelocity(mv);
	yDragger.behaviour.setMinimumVelocity(mv);


}

void ZoomableViewport::mouseDrag(const MouseEvent& e)
{
	if (e.mods.isX1ButtonDown() || e.mods.isX2ButtonDown())
		return;

	if (dragToScroll || e.mods.isMiddleButtonDown())
	{
		auto cBounds = content->getBoundsInParent().toDouble();
		auto tBounds = getLocalBounds().toDouble();

		auto deltaX = Helpers::pixelToNorm((double)e.getPosition().getX(), cBounds.getWidth(), tBounds.getWidth());
		auto deltaY = Helpers::pixelToNorm((double)e.getPosition().getY(), cBounds.getHeight(), tBounds.getHeight());

		deltaX -= normDragStart.getX();
		deltaY -= normDragStart.getY();

		xDragger.drag(deltaX);
		yDragger.drag(deltaY);
	}
	
}

void ZoomableViewport::mouseUp(const MouseEvent& e)
{
	if (dragToScroll || e.mods.isMiddleButtonDown())
	{
		xDragger.endDrag();
		yDragger.endDrag();
	}
}

void ZoomableViewport::mouseMagnify (const MouseEvent& e, float scaleFactor)
{
    auto cBounds = content->getBoundsInParent().toDouble();
    auto tBounds = getLocalBounds().toDouble();

    auto normX = Helpers::pixelToNorm((double)e.getPosition().getX(), cBounds.getWidth(), tBounds.getWidth());
    auto normY = Helpers::pixelToNorm((double)e.getPosition().getY(), cBounds.getHeight(), tBounds.getHeight());

    normDragStart = { normX, normY };
    scrollPosDragStart = { hBar.getCurrentRangeStart(), vBar.getCurrentRangeStart() };
    
    zoomFactor *= scaleFactor;
    setZoomFactor(zoomFactor, {});
}

void ZoomableViewport::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	if (e.mods.isCommandDown())
	{
		if (wheel.deltaY > 0)
			zoomFactor *= 1.15f;
		else
			zoomFactor /= 1.15f;

		zoomFactor = jlimit(0.25f, maxZoomFactor, zoomFactor);

		setZoomFactor(zoomFactor, {});
	}
	else
	{
		if (mouseWheelScroll)
		{
			auto zDelta = std::sqrt(zoomFactor);

#if JUCE_WINDOWS
			const float speed = 0.1f;
#else
			const float speed = 0.3f;
#endif

			if (e.mods.isShiftDown())
				hBar.setCurrentRangeStart(hBar.getCurrentRangeStart() - wheel.deltaY * speed / zDelta);
			else
			{
				hBar.setCurrentRangeStart(hBar.getCurrentRangeStart() - wheel.deltaX * speed / zDelta);
				vBar.setCurrentRangeStart(vBar.getCurrentRangeStart() - wheel.deltaY * speed / zDelta);
			}
		}
	}
}

void ZoomableViewport::resized()
{
	dark.setBounds(getLocalBounds());

	auto b = getLocalBounds();

	vBar.setBounds(b.removeFromRight(14));
	hBar.setBounds(b.removeFromBottom(14));

	if (!positionInitialised)
	{
		positionInitialised = true;

		Component::SafePointer<ZoomableViewport> safeThis(this);

		MessageManager::callAsync([safeThis]()
		{
			if (safeThis.getComponent() != nullptr)
			{
				safeThis.getComponent()->centerCanvas();
				
			}
		});
	}
	else
	{
		refreshScrollbars();
	}
}

void ZoomableViewport::paint(Graphics& g)
{
	g.fillAll(findColour(ColourIds::backgroundColourId));

	if (!content->isVisible())
	{
		g.setColour(Colours::black.withAlpha(swapAlpha));
		g.drawImage(swapImage, swapBounds);
	}
}

void ZoomableViewport::positionChanged(DragAnimator& p, double newPosition)
{
	if(&p == &xDragger)
		hBar.setCurrentRangeStart(newPosition, sendNotificationAsync);
	else
		vBar.setCurrentRangeStart(newPosition, sendNotificationAsync);
}

void ZoomableViewport::timerCallback()
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

void ZoomableViewport::zoomToRectangle(Rectangle<int> areaToShow)
{
	auto tBounds = getLocalBounds().toFloat();
	auto aBounds = areaToShow.toFloat();

	auto xZoom = tBounds.getWidth() / aBounds.getWidth();
	auto yZoom = tBounds.getHeight() / aBounds.getHeight();

	auto minZoom = 0.25f;

	auto zf = jmin(xZoom, yZoom, maxZoomFactor);

	setZoomFactor(jmax(minZoom, zf), aBounds.getCentre());
}

void ZoomableViewport::setZoomFactor(float newZoomFactor, Point<float> centerPositionInGraph)
{
	zoomFactor = jmin(maxZoomFactor, newZoomFactor);

	auto trans = AffineTransform::scale(zoomFactor);

	content->setTransform(trans);
	refreshPosition();

	if (!centerPositionInGraph.isOrigin())
	{
		auto cBounds = content->getBoundsInParent().toDouble();
		auto tBounds = getLocalBounds().toDouble();

		auto centerX = centerPositionInGraph.getX() * zoomFactor;
		auto centerY = centerPositionInGraph.getY() * zoomFactor;

		auto offsetX = getLocalBounds().getWidth() / 2 - centerX;
		auto offsetY = getLocalBounds().getHeight() / 2 - centerY;

		auto normX = Helpers::pixelToNorm(offsetX, cBounds.getWidth(), tBounds.getWidth());
		auto normY = Helpers::pixelToNorm(offsetY, cBounds.getHeight(), tBounds.getHeight());

		hBar.setCurrentRangeStart(normX, sendNotificationSync);
		vBar.setCurrentRangeStart(normY, sendNotificationSync);

		//graph->setTopLeftPosition(offsetX / zoomFactor, offsetY / zoomFactor);
	}

	for (auto l : listeners)
	{
		if (l != nullptr)
			l->zoomChanged(zoomFactor);
	}
}

bool ZoomableViewport::changeZoom(bool zoomIn)
{
	auto newZoom = zoomFactor;

	if (zoomIn)
		newZoom *= 1.1f;
	else
		newZoom /= 1.1f;

	newZoom = jlimit(0.25f, maxZoomFactor, newZoom);

	if (newZoom != zoomFactor)
	{
		setZoomFactor(newZoom, {});
		return true;
	}

	return false;
}

ZoomableViewport::MouseWatcher::MouseWatcher(ZoomableViewport& p):
	parent(p)
{
	refreshListener();
}

ZoomableViewport::MouseWatcher::~MouseWatcher()
{
	if (auto c = parent.getContentComponent())
		c->removeMouseListener(this);
}

void ZoomableViewport::MouseWatcher::refreshListener()
{
	if (auto c = parent.getContentComponent())
	{
		c->addMouseListener(this, true);
				
	}
}

void ZoomableViewport::MouseWatcher::setToMidAfterResize()
{
	if (auto c = parent.getContentComponent())
	{
		mousePos = parent.getLocalBounds().toFloat().getCentre();
		graphMousePos = parent.getContentComponent()->getLocalBounds().getCentre().toFloat();
	}
}

void ZoomableViewport::MouseWatcher::mouseMove(const MouseEvent& e)
{
	mousePos = e.getEventRelativeTo(&parent).getPosition().toFloat();
	graphMousePos = e.getEventRelativeTo(parent.getContentComponent()).getPosition().toFloat();
}

void ZoomableViewport::makeSwapSnapshot(float newSwapScale)
{
	swapImage = content->createComponentSnapshot(content->getLocalBounds(), true, zoomFactor);
	swapBounds = content->getBoundsInParent().toFloat();
	swapScale = newSwapScale;
	swapAlpha = 1.0f;
	content->setVisible(false);
	repaint();
	startTimer(30);
}

void ZoomableViewport::clearSwapSnapshot()
{
	swapImage = {};
	content->setVisible(true);
	content->setAlpha(swapAlpha);
	repaint();
}

void ZoomableViewport::setMaxZoomFactor(float newMaxZoomFactor)
{
	maxZoomFactor = newMaxZoomFactor;
}

Rectangle<int> ZoomableViewport::getCurrentTarget() const
{
	return dark.ruler.toNearestInt();
}

ZoomableViewport::Holder::Holder(Component* content_, Rectangle<int> targetArea_):
	content(content_),
	target(targetArea_)
{
	addAndMakeVisible(content);

	content->addComponentListener(this);

	updateSize();
}

void ZoomableViewport::Holder::updateSize()
{
	setSize(content->getWidth() + margin, content->getHeight() + margin + headerHeight);
}

void ZoomableViewport::Holder::componentMovedOrResized(Component& component, bool cond, bool wasResized)
{
	if (wasResized)
	{
		updateSize();
		updatePosition();
	}
}

void ZoomableViewport::Holder::updatePosition()
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

void ZoomableViewport::Holder::updateShadow()
{
	auto b = getBoundsInParent();

	findParentComponentOfClass<ZoomableViewport>()->dark.setRuler(target, b);
}

void ZoomableViewport::Holder::mouseDown(const MouseEvent& event)
{
	dragger.startDraggingComponent(this, event);
}

void ZoomableViewport::Holder::mouseDrag(const MouseEvent& event)
{
	dragger.dragComponent(this, event, nullptr);
	updateShadow();
}

void ZoomableViewport::Holder::resized()
{
	auto b = getLocalBounds();
	b.removeFromTop(headerHeight);
	b = b.reduced(margin / 2);
	content->setBounds(b);
}

void ZoomableViewport::Dark::paint(Graphics& g)
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

void ZoomableViewport::Dark::setRuler(Rectangle<int> area, Rectangle<int> componentArea)
{
	ruler = area.toFloat();
	shadow = componentArea;
	repaint();
}

void ZoomableViewport::Dark::mouseDown(const MouseEvent& mouseEvent)
{
	findParentComponentOfClass<ZoomableViewport>()->setCurrentModalWindow(nullptr, {});
}

Component* ZoomableViewport::getContentComponent()
{
	return content.get();
}

const Component* ZoomableViewport::getContentComponent() const
{
	return content.get();
}

ZoomableViewport::ZoomListener::~ZoomListener()
{}

void ZoomableViewport::addZoomListener(ZoomListener* l)
{
	listeners.addIfNotAlreadyThere(l);
}

void ZoomableViewport::removeZoomListener(ZoomListener* l)
{
	listeners.removeAllInstancesOf(l);
}

void ZoomableViewport::setScrollOnDragEnabled(bool shouldBeEnabled)
{
	if (shouldBeEnabled != dragToScroll)
	{
		dragToScroll = shouldBeEnabled;

		setMouseCursor(shouldBeEnabled ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
	}
}

void ZoomableViewport::setMouseWheelScrollEnabled(bool shouldBeEnabled)
{
	mouseWheelScroll = shouldBeEnabled;
}

Point<float> ZoomableViewport::MouseWatcher::getDeltaAfterResize()
{
	auto newGraphPos = parent.getLocalPoint(parent.content, graphMousePos);
	Point<float> delta(newGraphPos.getX() - mousePos.getX(), newGraphPos.getY() - mousePos.getY());
	return delta;
}

void ZoomableViewport::componentMovedOrResized(Component& component, bool wasMoved, bool wasResized)
{
	if ((!wasResized && !wasMoved) || wasResized)
		refreshScrollbars();
}

void ZoomableViewport::scrollBarMoved(ScrollBar* scrollBarThatHasMoved, double newRangeStart)
{
	
	refreshPosition();
    
}


void ZoomableViewport::setCurrentModalWindow(Component* newComponent, Rectangle<int> target)
{
	currentModalWindow = nullptr;

	if (newComponent == nullptr)
	{
		removeChildComponent(&dark);
		content->grabKeyboardFocus();
	}

	else
		addChildComponent(dark);

	Rectangle<int> cBounds;

	if (newComponent != nullptr)
	{
		currentModalWindow = new Holder(newComponent, target);

		addAndMakeVisible(currentModalWindow);

		auto maxWidthRatio = (float)(getWidth() - 50) / (float)currentModalWindow->getWidth();
		auto maxHeightRatio = (float)(getHeight() - 50) / (float)currentModalWindow->getHeight();

		auto sf = jmax(1.0f, jmin(std::pow(zoomFactor, 0.7f), maxWidthRatio, maxHeightRatio));

		currentModalWindow->setTransform(AffineTransform::scale(sf));

		currentModalWindow->updatePosition();

		currentModalWindow->setVisible(false);
		auto img = createComponentSnapshot(currentModalWindow->getBoundsInParent(), true);
		currentModalWindow->setVisible(true);

		currentModalWindow->setBackground(img);

		cBounds = currentModalWindow->getBoundsInParent();

		currentModalWindow->grabKeyboardFocus();
	}

	dark.setRuler(target, cBounds);
	dark.setVisible(currentModalWindow != nullptr);
}

void ZoomableViewport::setNewContent(Component* newContentToDisplay, Component* target)
{
	makeSwapSnapshot(JUCE_LIVE_CONSTANT_OFF(1.005f));

	pendingNewComponent = newContentToDisplay;

	auto f = [this]()
	{
		content->removeComponentListener(this);
		addAndMakeVisible(content = pendingNewComponent.release());
		content->addComponentListener(this);
		this->contentFunction(content);
		
		clearSwapSnapshot();
		mouseWatcher->refreshListener();
		zoomToRectangle(content->getLocalBounds());
	};

	if (target != nullptr)
	{
		auto b = getLocalArea(target, target->getLocalBounds());
		Timer::callAfterDelay(300, [this, b]() {zoomToRectangle(b); });
	}
	else
	{
		auto b = content->getBoundsInParent();
		Timer::callAfterDelay(300, [this, b]() {zoomToRectangle(b); });
	}

	Timer::callAfterDelay(500, f);
}

void ZoomableViewport::centerCanvas()
{
	setZoomFactor(1.0f, {});
	
	auto cBounds = content->getBoundsInParent();
	auto tBounds = getLocalBounds();

	auto x = (tBounds.getWidth() - cBounds.getWidth()) / 2;
	auto y = (tBounds.getHeight() - cBounds.getHeight()) / 2;

	content->setTopLeftPosition(x, y);

	hBar.setCurrentRange({ 0.5, 0.7 }, dontSendNotification);
	vBar.setCurrentRange({ 0.5, 0.7 }, dontSendNotification);

	mouseWatcher->setToMidAfterResize();
}



void ZoomableViewport::refreshScrollbars()
{
	auto cBounds = content->getBoundsInParent().toDouble();
	auto tBounds = getLocalBounds().toDouble();
	auto delta = mouseWatcher->getDeltaAfterResize();


	auto vSize = jmax(0.3, tBounds.getHeight() / cBounds.getHeight());
	auto hSize = jmax(0.3, tBounds.getWidth() / cBounds.getWidth());

	auto vPixel = cBounds.getY();
	auto hPixel = cBounds.getX();

	vPixel -= delta.getY();
	hPixel -= delta.getX();

	auto vStart = Helpers::pixelToNorm(vPixel, cBounds.getHeight(), tBounds.getHeight());
	auto hStart = Helpers::pixelToNorm(hPixel, cBounds.getWidth(), tBounds.getWidth());

	vBar.setRangeLimits({ 0.0, 1.0 + vSize }, sendNotificationSync);
	vBar.setCurrentRange({ vStart, vStart + vSize }, sendNotificationSync);

	hBar.setRangeLimits({ 0.0, 1.0 + hSize }, sendNotificationSync);
	hBar.setCurrentRange({ hStart, hStart + hSize }, sendNotificationSync);

	xDragger.setLimits(hBar.getRangeLimit());
	yDragger.setLimits(vBar.getRangeLimit());
}

void ZoomableViewport::refreshPosition()
{
	auto cBounds = content->getBoundsInParent().toDouble();
	auto tBounds = getLocalBounds().toDouble();

	auto y = Helpers::normToPixel(vBar.getCurrentRangeStart(), cBounds.getHeight(), tBounds.getHeight());
	y /= zoomFactor;

	auto x = Helpers::normToPixel(hBar.getCurrentRangeStart(), cBounds.getWidth(), tBounds.getWidth());
	x /= zoomFactor;

	content->setTopLeftPosition(x, y);
}



void ZoomableViewport::Holder::setBackground(Image img)
{
	bg = img.rescaled(img.getWidth() / 2, img.getHeight() / 2);

	PostGraphicsRenderer g2(stack, bg);
	g2.stackBlur(JUCE_LIVE_CONSTANT_OFF(20));
	
	repaint();
}

void ZoomableViewport::Holder::paint(Graphics& g)
{
	g.setColour(Colour(0xFF262626));
	auto b = getLocalBounds().toFloat();
	g.drawImage(bg, b, RectanglePlacement::fillDestination);
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0xaf2b2b2b)));
	g.setColour(Colours::white.withAlpha(0.2f));
	g.drawRect(b, 1.0f);
	b = b.removeFromTop((float)headerHeight).reduced(1.0f);
	g.setColour(Colours::white.withAlpha(0.05f));
	g.fillRect(b);
	g.setColour(Colours::white);
	g.setFont(GLOBAL_BOLD_FONT().withHeight(16.0f));
	g.drawText(content->getName(), b, Justification::centred);
}


}