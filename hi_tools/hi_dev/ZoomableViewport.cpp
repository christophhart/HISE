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
	dragScrollTimer(*this),
	mouseWatcher(new MouseWatcher(*this)),
	scroller(*this)
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

bool ZoomableViewport::checkDragScroll(const MouseEvent& e, bool isMouseUp)
{
	if(auto vp = e.eventComponent->findParentComponentOfClass<ZoomableViewport>())
	{
		vp->dragScrollTimer.setPosition(e, isMouseUp);
	}

	return false;
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
	if(e.mods.isX1ButtonDown())
		return true;

	if(e.mods.isX2ButtonDown())
		return true;

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
		const float zoomSpeed = e.mods.isShiftDown() ? 1.03f : 1.15f;

		if (wheel.deltaY > 0)
			zoomFactor *= zoomSpeed;
		else
			zoomFactor /= zoomSpeed;

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

	if (content != nullptr && !content->isVisible())
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

void ZoomableViewport::scrollToRectangle(Rectangle<int> areaToShow, bool skipIfVisible, bool animate)
{
	auto tBounds = getLocalBounds().toDouble();
	
	auto areaInViewport = getLocalArea(content, areaToShow).toDouble();

	auto centerPositionInGraph = areaToShow.getCentre().toDouble();
	auto cBounds = getLocalArea(content, content->getLocalBounds()).toDouble();

	auto visibleArea = cBounds.getIntersection(tBounds);

	if(!skipIfVisible || !visibleArea.contains(areaInViewport))
	{
		auto centerX = centerPositionInGraph.getX() * zoomFactor;
		auto centerY = centerPositionInGraph.getY() * zoomFactor;

		auto offsetX = getLocalBounds().getWidth() / 2 - centerX;
		auto offsetY = getLocalBounds().getHeight() / 2 - centerY;

		auto normX = Helpers::pixelToNorm(offsetX, cBounds.getWidth(), tBounds.getWidth());
		auto normY = Helpers::pixelToNorm(offsetY, cBounds.getHeight(), tBounds.getHeight());

		if(animate)
		{
			dragScrollTimer.scrollToPosition({normX, normY});
		}
		else
		{
			hBar.setCurrentRangeStart(normX, sendNotificationSync);
			vBar.setCurrentRangeStart(normY, sendNotificationSync);
		}
	}
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

struct PopupMenuParser
{
    enum SpecialItem
    {
        ItemEntry = 0,
        Sub = 1,
        Header = 2,
        Deactivated = 4,
        Separator = 8
    };
    
    static int getSpecialItemType(const String& item)
    {
        int flags = SpecialItem::ItemEntry;
        
        if(item.contains("~~"))
            flags = SpecialItem::Deactivated;
        
        if(item.contains("___") || item.contains("___"))
            flags = SpecialItem::Separator;

        if(item.contains("**"))
            flags = SpecialItem::Header;
        
        if(item.contains("::"))
            flags |= SpecialItem::Sub;
        
        return flags;
    };
    
    static bool addToPopupMenu(PopupMenu& tm, int& menuIndex, const String& item, const Array<int>& activeIndexes)
    {
        if (item == "%SKIP%")
        {
            menuIndex++;
            return false;
        }
        
        auto isTicked = activeIndexes.contains(menuIndex-1);
        
        auto fl = getSpecialItemType(item);
        
        jassert((fl & SpecialItem::Sub) == 0);
        
        auto withoutSpecialChars = item;

        if(fl & SpecialItem::Header)
        {
	        tm.addSectionHeader(item.removeCharacters("*"));
			return false;
		}
        else if (fl & SpecialItem::Separator)
        {
	        tm.addSeparator();
			return false;
        }

		PopupMenu::Item newItem;

		newItem.text = item.removeCharacters("~|");
		newItem.isEnabled = (fl & SpecialItem::Deactivated) == 0;
		newItem.isTicked = isTicked;
		newItem.itemID = menuIndex++;
		newItem.shouldBreakAfter = item.getLastCharacter() == '|';

        tm.addItem(newItem);

        return isTicked;
    };
    
    struct SubInfo
    {
        SubInfo() = default;
        
        PopupMenu sub;
        bool ticked = false;
        String name;
        StringArray items;
        
        void flush(PopupMenu& m, int& firstId, const Array<int>& activeIndexes)
        {
            if(items.isEmpty() && children.isEmpty())
                return;
            
            for(auto& s: items)
            {
                ticked |= addToPopupMenu(sub, firstId, s, activeIndexes);
            }
            
            for(auto c: children)
            {
                c->flush(sub, firstId, activeIndexes);
            }
            
            m.addSubMenu(name, sub, true, nullptr, ticked);
            
            items.clear();
            children.clear();
        }
        
        OwnedArray<SubInfo> children;
        
        JUCE_DECLARE_WEAK_REFERENCEABLE(SubInfo);
        JUCE_DECLARE_NON_COPYABLE(SubInfo);
    };
    
    static SubInfo* getSubMenuFromArray(OwnedArray<SubInfo>& list, const String& name)
    {
        auto thisName = name.upToFirstOccurrenceOf("::", false, false);
        auto rest = name.fromFirstOccurrenceOf("::", false, false);
        
        for(auto s: list)
        {
            if(s->name == thisName)
            {
                if(rest.isEmpty())
                    return s;
                else
                    return getSubMenuFromArray(s->children, rest);
            }
        }
        
        auto newInfo = new SubInfo();
        newInfo->name = thisName;
        list.add(newInfo);
        
        if(rest.isEmpty())
            return newInfo;
        else
            return getSubMenuFromArray(newInfo->children, rest);
    }
    
    SubInfo* getSubMenu(const String& name)
    {
        return getSubMenuFromArray(subMenus, name);
    };
    
    OwnedArray<SubInfo> subMenus;
};

juce::PopupMenu SubmenuComboBox::parseFromStringArray(const StringArray& itemList, Array<int> activeIndexes,
	LookAndFeel* laf)
{
	PopupMenu m;
	m.setLookAndFeel(laf);

    PopupMenuParser parser;

	for (const auto& item: itemList)
	{
        auto fl = parser.getSpecialItemType(item);
        
        if (fl & PopupMenuParser::SpecialItem::Sub)
		{
			String subMenuName = item.upToLastOccurrenceOf("::", false, false);
			String subMenuItem = item.fromLastOccurrenceOf("::", false, false);

			if (subMenuName.isEmpty() || subMenuItem.isEmpty())
                continue;

            parser.getSubMenu(subMenuName)->items.add(subMenuItem);
		}
	}

    int menuIndex = 1;

    for(const auto& item: itemList)
    {
        auto fl = parser.getSpecialItemType(item);
        auto isSubEntry = (fl & PopupMenuParser::SpecialItem::Sub) != 0;
        
        if(isSubEntry)
        {
            // We want to only get the first name so that it can
            // flush its children
            auto firstMenuName = item.upToFirstOccurrenceOf("::", false, false);
            
            parser.getSubMenu(firstMenuName)->flush(m, menuIndex, activeIndexes);
        }
        else
            parser.addToPopupMenu(m, menuIndex, item, activeIndexes);
    }
    
	return m;
}

void SubmenuComboBox::rebuildPopupMenu()
{
	if(!useCustomPopupMenu())
		return;
        
	auto& menu = *getRootMenu();
        
	StringArray sa;
	Array<int> activeIndexes;

	Array<std::pair<int, String>> list;

	for (PopupMenu::MenuItemIterator iterator (menu, true); iterator.next();)
	{
		auto& item = iterator.getItem();

		if(item.isSectionHeader)
			continue;

		if(item.itemID == getSelectedId())
			activeIndexes.add(item.itemID);
            
		sa.add(item.text);
	}

	createPopupMenu(menu, sa, activeIndexes);

	refreshTickState();
}

void ZoomableViewport::DragScrollTimer::timerCallback()
{
	if(scrollAnimationCounter != -1)
	{
		auto numFrames = JUCE_LIVE_CONSTANT_OFF(30);
		auto alpha = (double)scrollAnimationCounter++ / (double)(numFrames);
		alpha = hmath::pow(alpha, 6.0);

		auto newX = Interpolator::interpolateLinear(scrollAnimationStart.getX(), scrollAnimationTarget.getX(), alpha);
		auto newY = Interpolator::interpolateLinear(scrollAnimationStart.getY(), scrollAnimationTarget.getY(), alpha);

		parent.hBar.setCurrentRangeStart(newX, sendNotificationSync);
		parent.vBar.setCurrentRangeStart(newY, sendNotificationSync);

		if(scrollAnimationCounter > numFrames)
		{
			scrollAnimationCounter = -1;
			stopTimer();
			scrollAnimationStart = {};
			scrollAnimationTarget = {};
		}
	}

	auto factor = JUCE_LIVE_CONSTANT_OFF(0.03);
	auto expo = JUCE_LIVE_CONSTANT_OFF(1.2);
	
	auto dx = jlimit(-1.0, 1.0, (double)xDelta / (double)(parent.getWidth() / 5));
	auto dy = jlimit(-1.0, 1.0, (double)yDelta / (double)(parent.getHeight() / 5));

	dx = hmath::sign(dx) * std::pow(hmath::abs(dx), expo);
	dy = hmath::sign(dy) * std::pow(hmath::abs(dy), expo);

	auto a = JUCE_LIVE_CONSTANT_OFF(0.74);

	dx = lx * a + dx * (1.0 - a);
	dy = ly * a + dy * (1.0 - a);

	lx = dx;
	ly = dy;

	dx *= factor;
	dy *= factor;

	parent.hBar.setCurrentRangeStart(jlimit(0.0, 1.0, parent.hBar.getCurrentRangeStart() + dx));
	parent.vBar.setCurrentRangeStart(jlimit(0.0, 1.0, parent.vBar.getCurrentRangeStart() + dy));
}

void ZoomableViewport::DragScrollTimer::setPosition(const MouseEvent& e, bool isMouseUp)
{
	if(isMouseUp)
	{
		wasInCentre = false;
		lx = 0.0;
		ly = 0.0;

		if(scrollAnimationCounter == -1)
			stopTimer();
		return;
	}

	if(scrollAnimationCounter != -1)
		return;

	auto vpos = parent.getLocalPoint(e.eventComponent, e.getPosition());
	auto vb = parent.getLocalBounds();
	auto padding = jmin(vb.getWidth(), vb.getHeight()) / 6;

	vb = vb.reduced(padding);

	if(vpos.getX() > vb.getRight())
		xDelta = vpos.getX() - vb.getRight();
	else if (vpos.getX() < vb.getX())
		xDelta = vpos.getX() - vb.getX();
	else
		xDelta = 0;

	if(vpos.getY() > vb.getBottom())
		yDelta = vpos.getY() - vb.getBottom();
	else if (vpos.getY() < vb.getY())
		yDelta = vpos.getY() - vb.getY();
	else 
		yDelta = 0;

	if(xDelta == 0 && yDelta == 0)
		wasInCentre = true;

	auto shouldScroll = wasInCentre && (xDelta != 0 || yDelta != 0);

	if(shouldScroll != isTimerRunning())
	{
		if(shouldScroll)
			startTimer(30);
		else
		{
			if(hmath::abs(lx) < 0.005 && hmath::abs(ly) < 0.005)
				stopTimer();
		}
	}
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

void ZoomableViewport::WASDScroller::timerCallback()
{
	if(currentDelta.isOrigin())
	{
		currentVelocity *= JUCE_LIVE_CONSTANT_OFF(0.8f);
                
		if(currentVelocity.getDistanceFromOrigin() < 0.01f)
		{
			currentVelocity = {};
			stopTimer();
		}
	}
	else
	{
		currentVelocity += currentDelta;
                
		currentVelocity.x = jlimit(-1.0f, 1.0f, currentVelocity.x);
		currentVelocity.y = jlimit(-1.0f, 1.0f, currentVelocity.y);
	}

	auto f = currentVelocity;

	f *= JUCE_LIVE_CONSTANT_OFF(-0.1f);

	auto x = parent.hBar.getCurrentRangeStart();
	x += f.x;
	parent.hBar.setCurrentRangeStart(x, sendNotificationAsync);

	auto y = parent.vBar.getCurrentRangeStart();
	y += f.y;
	parent.vBar.setCurrentRangeStart(y, sendNotificationAsync);
}

void ZoomableViewport::WASDScroller::setDelta(Point<float> delta)
{
	currentDelta = delta;
            
	if(currentVelocity.isOrigin() && !delta.isOrigin())
		startTimer(15);
}

bool ZoomableViewport::WASDScroller::checkWASD(const KeyPress& key)
{
	if(key == KeyPress('w', ModifierKeys(), 'w'))
		return true;
	if(key == KeyPress('a', ModifierKeys(), 'a'))
		return true;
	if(key == KeyPress('s', ModifierKeys(), 's'))
		return true;
	if(key == KeyPress('d', ModifierKeys(), 'd'))
		return true;
	        
	return false;
}

bool ZoomableViewport::WASDScroller::keyChangedWASD(bool isKeyDown, Component* c)
{
	auto fc = c->getCurrentlyFocusedComponent();

	if(checkFunction && checkFunction(fc)) // make better check function
	{
		auto u = KeyPress::isKeyCurrentlyDown('w');
		auto d = KeyPress::isKeyCurrentlyDown('s');
		auto l = KeyPress::isKeyCurrentlyDown('a');
		auto r = KeyPress::isKeyCurrentlyDown('d');

		auto turbo = ModifierKeys::getCurrentModifiers().isShiftDown();

		Point<float> delta;

		if(u)
			delta.y += 1.0f;
		if(d)
			delta.y -= 1.0f;

		if(l)
			delta.x += 1.0f;
		if(r)
			delta.x -= 1.0f;
	            
		delta *= JUCE_LIVE_CONSTANT(0.1);
		delta *= 0.05f;

		if(turbo)
			delta *= JUCE_LIVE_CONSTANT_OFF(3.0f);

		setDelta(delta);
	            
		return !delta.isOrigin();
	}
	        
	return false;
}

AbstractZoomableView::WASDKeyListener::WASDKeyListener(Component& c):
	p(c)
{
	p.setWantsKeyboardFocus(true);
	p.addKeyListener(this);

	delta[0].setValueAndRampTime(0.0, 60.0, 0.12);
	delta[1].setValueAndRampTime(0.0, 60.0, 0.12);
	delta[2].setValueAndRampTime(0.0, 60.0, 0.12);
}

AbstractZoomableView::WASDKeyListener::~WASDKeyListener()
{
	p.removeKeyListener(this);
}

bool AbstractZoomableView::WASDKeyListener::keyPressed(const KeyPress& key, Component* originatingComponent)
{
	return isDirection(key.getKeyCode());
}

void AbstractZoomableView::WASDKeyListener::setShiftMultiplier(double horizontalFactor, double verticalFactor, double zoomFactor)
{
	shiftMultipliers[0] = horizontalFactor;
	shiftMultipliers[1] = verticalFactor;
	shiftMultipliers[2] = zoomFactor;
}

void AbstractZoomableView::WASDKeyListener::timerCallback()
{
	auto mods = ComponentPeer::getCurrentModifiersRealtime();

	double thisDelta[NumMovementTypes] = { 0.0, 0.0, 0.0 };

	for(auto s: state)
	{
		if(s.second)
		{
			auto idx = (int)getMovementType(s.first);

			thisDelta[idx] = (float)getDelta(s.first);

			if(mods.isShiftDown())
				thisDelta[idx] *= shiftMultipliers[idx];
		}
	}

	delta[0].setTargetValue(thisDelta[0]);
	delta[1].setTargetValue(thisDelta[1]);
	delta[2].setTargetValue(thisDelta[2]);

	if(!onMovement)
		return;

	bool movement = std::abs(delta[0].getTargetValue()) > 0.5 || 
		            std::abs(delta[1].getTargetValue()) > 0.5 ||
					std::abs(delta[2].getTargetValue()) > 0.5;

	if(auto x = delta[(int)MovementType::Horizontal].getNextValue())
	{
		onMovement(MovementType::Horizontal, x);
		movement |= std::abs(x) > 0.0005;
	}
				
	if(auto y = delta[(int)MovementType::Vertical].getNextValue())
	{
		onMovement(MovementType::Vertical, y);
		movement |= std::abs(y) > 0.0005;
	}

	if(auto z = delta[(int)MovementType::Zoom].getNextValue())
	{
		onMovement(MovementType::Zoom, z);
		movement |= std::abs(z) > 0.0005;
	}
				
	if(!movement)
		stopTimer();
}

bool AbstractZoomableView::WASDKeyListener::keyStateChanged(bool isKeyDown, Component* originatingComponent)
{
	state[Direction::Left] = KeyPress::isKeyCurrentlyDown((int)Direction::Left);
	state[Direction::Up] = KeyPress::isKeyCurrentlyDown((int)Direction::Up);
	state[Direction::Down] = KeyPress::isKeyCurrentlyDown((int)Direction::Down);
	state[Direction::Right] = KeyPress::isKeyCurrentlyDown((int)Direction::Right);
	state[Direction::In] = KeyPress::isKeyCurrentlyDown((int)Direction::In);
	state[Direction::Out] = KeyPress::isKeyCurrentlyDown((int)Direction::Out);

	if(state[Direction::Left] && state[Direction::Right])
	{
		state[Direction::Left] = false;
		state[Direction::Right] = false;
	}

	if(state[Direction::In] && state[Direction::Out])
	{
		state[Direction::In] = false;
		state[Direction::Out] = false;
	}

	if(state[Direction::Up] && state[Direction::Down])
	{
		state[Direction::Up] = false;
		state[Direction::Down] = false;
	}

	auto somePressed = false;

	for(auto& s: state)
		somePressed |= s.second;

	if(somePressed && !isTimerRunning())
		startTimer(15);

	return somePressed;
}

AbstractZoomableView::DragZoomAction::DragZoomAction(AbstractZoomableView& parent_, Range<double> newRange_,
	Range<double> oldRange_, double newZoom_, double oldZoom_):
	UndoableAction(),
	parent(parent_),
	newRange(newRange_),
	oldRange(oldRange_),
	newZoom(newZoom_),
	oldZoom(oldZoom_)
{}

bool AbstractZoomableView::DragZoomAction::perform()
{
	parent.zoomAnimator.setTarget(newRange, newZoom);
	return true;
}

bool AbstractZoomableView::DragZoomAction::undo()
{
	parent.zoomAnimator.setTarget(oldRange, oldZoom);
	return true;
}

void AbstractZoomableView::ZoomAnimator::setTarget(Range<double> newTarget, double newZoom)
{
	start = parent.scrollbar.getCurrentRange();
	startZoom = parent.zoomFactor;

	alpha = 0.0;

	zoom = newZoom;
	target = newTarget;
	startTimer(15);
}

void AbstractZoomableView::ZoomAnimator::timerCallback()
{
	auto a = jlimit(0.0, 1.0, std::pow(alpha, JUCE_LIVE_CONSTANT_OFF(2.0)));
	auto invAlpha = 1.0 - a;

	Range<double> thisRange(start.getStart() * invAlpha + target.getStart() * a, start.getEnd() * invAlpha + target.getEnd() * a);

	auto thisZoom = start.getLength() / thisRange.getLength();

	parent.zoomFactor = startZoom * thisZoom;
	parent.scrollbar.setCurrentRange(thisRange, sendNotificationSync);

	alpha += JUCE_LIVE_CONSTANT_OFF(0.04);

	if(alpha >= 1.0)
	{
		parent.scrollbar.setCurrentRange(target, sendNotificationSync);
		parent.zoomFactor = zoom;

		stopTimer();
	}
				
}

AbstractZoomableView::AbstractZoomableView():
	zoomAnimator(*this),
	scrollbar(false)
{
	sf.addScrollBarToAnimate(scrollbar);
}

void AbstractZoomableView::setAsParentComponent(Component* p)
{
	jassert(p == dynamic_cast<Component*>(this));
	asComponent = p;

	wasd = new WASDKeyListener(*asComponent);

	p->addAndMakeVisible(scrollbar);
	sf.addScrollBarToAnimate(scrollbar);
	scrollbar.addListener(this);
	animator.addListener(this);

	wasd->onMovement = BIND_MEMBER_FUNCTION_2(AbstractZoomableView::onWASDMovement);

	scrollbar.setRangeLimits({0.0, 1.0});
	animator.setLimits({0.0, 1.0});
}

void AbstractZoomableView::changeZoom(double newZoomFactor, int xPos)
{
	if(asComponent->getWidth() == 0)
		return;

	double mousePos = (double)xPos / (double)(asComponent->getWidth());
	auto oldZoom = 1.0 / zoomFactor;

	zoomFactor = jlimit(1.0, 80000000.0, newZoomFactor);

	auto pan = scrollbar.getCurrentRangeStart();
		
	// Adjust pan to keep zoom centered on mouse position
	double deltaZoom = oldZoom - (1.0 / newZoomFactor);
	pan += mousePos * deltaZoom;

	auto nz = 1.0 / zoomFactor;

	// Ensure pan stays within bounds
	pan = jlimit(0.0, 1.0 - nz, pan);
	    
	scrollbar.setCurrentRange(pan, nz, sendNotificationSync);
}

void AbstractZoomableView::scrollBarMoved(ScrollBar*, double newRangeStart)
{
	asComponent->repaint();
}

void AbstractZoomableView::drawDragDistance(Graphics& g, Rectangle<int> viewportPosition)
{
	if(showDragDistance)
	{
		g.setColour(Colours::black.withAlpha(0.6f));
		int y = viewportPosition.getY();
		auto h = viewportPosition.getHeight();
		Rectangle<int> a(dragDistance.getStart(), y, dragDistance.getLength(), h);
		g.fillRect(a);
		g.setColour(Colours::white.withAlpha(0.9f));
		g.drawVerticalLine(dragDistance.getStart(), (float)y, (float)(y + h));
		g.drawVerticalLine(dragDistance.getEnd(), (float)y, (float)(y + h));
		auto w = (float)dragDistance.getLength() / (float)asComponent->getWidth();
		w *= totalLength / zoomFactor;
		auto x = getTextForDistance(w);
		g.drawText(x, a.toFloat(), Justification::centred);
	}
}

void AbstractZoomableView::drawBackgroundGrid(Graphics& g, Rectangle<int> viewportPosition)
{
	g.fillAll(Colour(0xFF1D1D1D));

	auto r = getNormalisedZoomRange();

	if(r.getLength() == 0.0)
		return;

	NormalisableRange<double> nr(r);
	auto y = viewportPosition.getY();
	auto h = viewportPosition.getBottom();

	g.setFont(GLOBAL_FONT());

	if(useIntegerGrid)
	{
		auto s = getScaledZoomRange();

		int numElementsToShow = roundToInt(s.getLength()) - 1;

		auto elementWidth = (double)viewportPosition.getWidth() / (s.getLength() - 1);

		int numToSkip = 1;

		while(elementWidth * (double)numToSkip < 50.0)
		{
			numToSkip *= 10;
		}

		auto xOffset = viewportPosition.getX() - std::fmod(s.getStart(), (double)numToSkip) * elementWidth;

		for(int i = 0; i <= (numElementsToShow + numToSkip); i += numToSkip)
		{
			auto xpos = xOffset + (double)(i * elementWidth);
			
			Rectangle<float> tb((float)xpos + 5.0f, (float)y, 100.0f, (float)FirstItemOffset);

			auto idx = i + s.getStart() - std::fmod(s.getStart(), (double)numToSkip);

			auto label = getTextForDistance(idx);
			g.setColour(Colours::white.withAlpha(0.3f));
			g.drawText(label, tb, Justification::centredLeft);
			g.drawVerticalLine(xpos, y, h);
		}
	}
	else
	{
		auto l_r = std::log10(r.getLength()) - 0.1;

		auto l = std::floor(l_r);

		auto alpha = 1.0f - jlimit(0.0f, 1.0f, (float)(l_r - l));

		auto delta = std::pow(10.0, l);
		auto x = r.getStart();

		if(x != 0.0)
			x += (delta - std::fmod(r.getStart(), delta));

		x -= delta;

		g.setColour(Colours::black.withAlpha(0.02f));
		g.fillRect(viewportPosition);

		g.setColour(Colours::white.withAlpha(0.3f));
		

		auto smallDelta = delta * 0.1;

		while(x <= r.getEnd() && delta != 0.0f)
		{
			if(nr.getRange().contains(x))
			{
				auto xpos = viewportPosition.getX() + roundToInt(nr.convertTo0to1(x) * (float)viewportPosition.getWidth());
				Rectangle<float> tb((float)xpos + 5.0f, (float)y, 100.0f, (float)FirstItemOffset);
				auto label = getTextForDistance(x * (totalLength));
				g.setColour(Colours::white.withAlpha(0.3f));
				g.drawText(label, tb, Justification::centredLeft);
				g.drawVerticalLine(xpos, y, h);
			}

			g.setColour(Colours::white.withAlpha(alpha * 0.25f));

			for(int i = 1; i < 10; i++)
			{
				auto x2 = x + smallDelta * (float)i;

				if(nr.getRange().contains(x2))
				{
					x2 = nr.convertTo0to1(x2);
					g.drawVerticalLine(viewportPosition.getX() + roundToInt(x2 * (float)viewportPosition.getWidth()), y + (float)FirstItemOffset - 2.0f, h);
				}
			}

			x += delta;
		}
	}
}

void AbstractZoomableView::onResize(Rectangle<int> viewportPosition)
{
	scrollbar.setBounds(viewportPosition.removeFromBottom(ScrollbarHeight));

	totalLength = getTotalLength();

	if(totalLength > 0.0)
	{
		scaleFactor = (float)asComponent->getWidth() / totalLength;
		asComponent->repaint();
	}
}

void AbstractZoomableView::positionChanged(Animator&, double newPosition)
{
	scrollbar.setCurrentRangeStart(newPosition, sendNotificationSync);
}

void AbstractZoomableView::handleMouseWheelEvent(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	if(e.mods.isAnyModifierKeyDown())
	{
		if(wheel.deltaY > 0)
			changeZoom(zoomFactor * 1.2f, e.getPosition().x);
		else
			changeZoom(zoomFactor * 0.8f, e.getPosition().x);
	}
	else
	{
			
		float increment = wheel.deltaY;

		if (increment < 0)
			increment = -1.0 * scrollbar.getCurrentRange().getLength() * 0.33;
		else if (increment > 0)
			increment = 1.0 * scrollbar.getCurrentRange().getLength() * 0.33;

		auto vr = scrollbar.getCurrentRange();
		scrollbar.setCurrentRange (vr - increment, sendNotificationSync);
	}
}

void AbstractZoomableView::hangleMouseMagnify(const MouseEvent& e, float scaleFactor)
{
	changeZoom(zoomFactor * (double)scaleFactor, e.getPosition().x);
}

bool AbstractZoomableView::handleMouseEvent(const MouseEvent& e, MouseEventType t)
{
	updateMouseCursorForEvent(e);

	switch(t)
	{
	case MouseEventType::MouseDown:
		{
			if(e.mods.isX1ButtonDown())
			{
				if(auto um = getUndoManager())
					um->undo();
				return true;
			}
			if(e.mods.isX2ButtonDown())
			{
				if(auto um = getUndoManager())
					um->redo();
				return true;
			}
			if (e.mods.isShiftDown())
			{
				showDragDistance = true;
				return true;
			}
			if (!e.mods.isAnyModifierKeyDown())
			{
				animator.setPosition(scrollbar.getCurrentRangeStart());
				animator.beginDrag();
				return true;
			}
		}
	case MouseEventType::MouseUp:
		{
			if(showDragDistance)
			{
				if(e.mods.isShiftDown())
				{
					auto x = (float)dragDistance.getStart() / (float)asComponent->getWidth();
					auto w = (float)dragDistance.getLength() / (float)asComponent->getWidth();

					auto sr = scrollbar.getCurrentRange();

					auto newStart = sr.getStart() + x * sr.getLength();
					auto newLength = sr.getLength() * w;

					zoomToRange({newStart, newStart + newLength});
				}

				showDragDistance = false;
				return false;
			}
			else
			{
				animator.endDrag();
				return true;
			}
		}
	case MouseEventType::MouseDoubleClick:
		{
			changeZoom(1.0);
			return true;
		}
	case MouseEventType::MouseDrag:
		{
			if(showDragDistance)
			{
				auto x1 = e.getMouseDownPosition().x;
				auto x2 = e.getPosition().x;

				dragDistance = { jmin(x1, x2), jmax(x1, x2) };
				asComponent->repaint();
				return true;
			}
			else
			{
				auto dx = (float)e.getDistanceFromDragStartX() / (float)asComponent->getWidth();
				dx *= scrollbar.getCurrentRange().getLength();
				animator.drag(-1.0 * dx);
				return true;
			}
		}
		
	case MouseEventType::MouseMove:
		break;
	default: ;
	}

	return false;
		
}

void AbstractZoomableView::zoomToRange(Range<double> newScaledRange)
{
	auto sr = scrollbar.getCurrentRange();
	auto w = newScaledRange.getLength() / sr.getLength();
		
	ScopedPointer<DragZoomAction> a = new DragZoomAction(*this, newScaledRange, sr, zoomFactor / w, zoomFactor);

	if(auto um = getUndoManager())
	{
		um->beginNewTransaction();
		um->perform(a.release());
	}
	else
	{
		a->perform();
	}
}

void AbstractZoomableView::moveToRange(double normalisedNewXPosition)
{
	auto sr = scrollbar.getCurrentRange();
	auto nr = sr.movedToStartAt(normalisedNewXPosition);
	ScopedPointer<DragZoomAction> a = new DragZoomAction(*this, nr, sr, zoomFactor, zoomFactor);

	if(auto um = getUndoManager())
	{
		um->beginNewTransaction();
		um->perform(a.release());
	}
	else
	{
		a->perform();
	}
}

void AbstractZoomableView::updateMouseCursorForEvent(const MouseEvent& e)
{
	if(e.mods.isShiftDown())
		asComponent->setMouseCursor(MouseCursor::CrosshairCursor);
	else
		asComponent->setMouseCursor(getTooltip().isNotEmpty() ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
}

bool AbstractZoomableView::onWASDMovement(WASDKeyListener::MovementType t, double delta)
{
	if(t == WASDKeyListener::MovementType::Horizontal)
	{
		auto p = scrollbar.getCurrentRangeStart();

		auto l = scrollbar.getCurrentRange().getLength();
		auto d = l * delta;
		d *= JUCE_LIVE_CONSTANT_OFF(0.02);

		p = jlimit(0.0, 1.0 - l, p + d);
		scrollbar.setCurrentRangeStart(p, sendNotificationSync);
		return true;
	}
	else if (t == WASDKeyListener::MovementType::Zoom)
	{
		auto dx = 1.0 + delta * JUCE_LIVE_CONSTANT_OFF(0.1);

		auto z = zoomFactor * dx;

		auto pos = asComponent->getWidth() / 2;

		if(asComponent->isMouseOver(true))
			pos = asComponent->getMouseXYRelative().getX();

		changeZoom(z, pos);
		return true;
	}

	return false;
}

float AbstractZoomableView::getZoomScale() const
{
	return scaleFactor * zoomFactor;
}

float AbstractZoomableView::getZoomTranspose() const
{
	return scrollbar.getCurrentRangeStart() * totalLength;
}

Range<double> AbstractZoomableView::getScaledZoomRange() const
{
	auto nr = getNormalisedZoomRange();
	return { nr.getStart() * totalLength, nr.getEnd() * totalLength };
}

Range<double> AbstractZoomableView::getNormalisedZoomRange() const
{
	return scrollbar.getCurrentRange();
}

void AbstractZoomableView::updateIndexToShow()
{
	totalLength = getTotalLength();
	asComponent->resized();
	changeZoom(1.0);
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

struct ZoomableDataViewer: public AbstractZoomableView,
                           public Component
{
    static constexpr int LabelHeight = 20;

    struct DrawContext
    {
        Rectangle<float> fullBounds;
        Rectangle<float> legend;
        Range<double> dataRangeToShow;
		NormalisableRange<float> valueRange;
    };

    enum class IndexDomain
    {
        Absolute,
        Relative,
        Time
    };

    static StringArray getIndexDomainTypes() { return { "Absolute", "Relative", "Time" }; }

    enum class ValueDomain
    {
        Absolute,
        Relative,
        Decibel
    };

    static StringArray getValueDomainTypes() { return { "Absolute", "Relative", "Decibel" }; }

    struct Data
    {
        Colour getColour() const
        {
            return Colour((uint32)label.hash()).withSaturation(0.5).withBrightness(0.7f).withAlpha(1.0f);
        }

        void draw(Graphics& g, DrawContext& ctx)
        {
            auto tw = (float)LabelHeight + GLOBAL_MONOSPACE_FONT().getStringWidthFloat(label) + 20.0f;
            
            auto tl = ctx.legend.removeFromLeft(tw);

            auto c = getColour();

            g.setColour(c);

            auto thisRange = ctx.dataRangeToShow;//.expanded(1.0).getIntersectionWith({0.0, (double)size});

            if(!thisRange.isEmpty() && enabled)
            {
                bool first = true;

                float xDelta = ctx.fullBounds.getWidth() / (thisRange.getLength() - 1);

                if(xDelta < 0.25)
                {
                    RectangleList<float> list;

                    auto samplesPerPixel = 1.0 / xDelta;
                    auto sampleIndex = thisRange.getStart();

                    for(int i = (int)ctx.fullBounds.getX(); i < (int)ctx.fullBounds.getRight(); i++)
                    {
                        auto thisIndex = (int)(sampleIndex);
                        auto numThisTime = jmin<int>((int)size - sampleIndex, roundToInt(samplesPerPixel + 1.0));

                        auto range = FloatVectorOperations::findMinAndMax(data.get() + thisIndex, numThisTime);

                        auto yMax = ctx.valueRange.convertTo0to1(range.getEnd());
                        auto yMin = ctx.valueRange.convertTo0to1(range.getStart());

                        auto y = ctx.fullBounds.getY() + (1.0f - yMax) * ctx.fullBounds.getHeight();
                        auto bottom = ctx.fullBounds.getY() + (1.0f - yMin) * ctx.fullBounds.getHeight();
                        auto h = bottom - y;

                        auto x = (float)i - 0.25f;
                        auto w = 1.5f;

                        list.addWithoutMerging({x, y, w, h});

                        sampleIndex += samplesPerPixel;
                    }

                    g.fillRectList(list);
                }
                else
                {
                    Path p;

                    g.saveState();
                    g.reduceClipRegion(ctx.fullBounds.toNearestInt());

                    RectangleList<float> points;

                    for(int i = (int)thisRange.getStart(); i < ((int)thisRange.getEnd() + 1); i++)
                    {
                        auto x = -1.0 *  (thisRange.getStart() - (double)i) * xDelta;
                        x += ctx.fullBounds.getX();

                        auto y = 1.0f - ctx.valueRange.convertTo0to1(data[i]);

                        Point<float> pos { (float)x, ctx.fullBounds.getY() + y * ctx.fullBounds.getHeight() };

                        if(thisRange.getLength() < 20.0)
                            points.add(Rectangle<float>(pos, pos).withSizeKeepingCentre(6.0f, 6.0f));

                        if(first)
                        {
                            p.startNewSubPath(pos);
                            first = false;
                        }
                        else
                            p.lineTo(pos);
                    }

                    g.strokePath(p, PathStrokeType(1.5f));
                    g.restoreState();

                    if(!points.isEmpty())
                        g.fillRectList(points);
                }
            }


            g.setColour(c);

            if(enabled)
                g.fillRoundedRectangle(tl.removeFromLeft(tl.getHeight()).reduced(2.0f), 4.0f);
            else
                g.drawRoundedRectangle(tl.removeFromLeft(tl.getHeight()).reduced(2.0f), 4.0f, 2.0f);
            
            tl.removeFromLeft(5.0f);
            g.setFont(GLOBAL_MONOSPACE_FONT());
            g.setColour(Colours::white.withAlpha(enabled ? 0.7f : 0.4f));
            g.drawText(label, tl, Justification::left);
        }

        int index;
        String label;
        HeapBlock<float> data;
        NormalisableRange<float> valueRange;
        size_t size;
        bool enabled = true;
    };

    ZoomableDataViewer()
    {
		useIntegerGrid = true;
        setOpaque(true);
        setAsParentComponent(this);
    }

    UndoManager um;
    OwnedArray<Data> data;

    double getTotalLength() const override { return (double)(maxLength); }

    UndoManager* getUndoManager() override { return &um; }

    

    void paint(Graphics& g) override
    {
		if(yRange.isEmpty())
			return;

        DrawContext ctx;
        auto b = getLocalBounds().toFloat().reduced(10.0f);
        ctx.legend = b.removeFromTop(24.0f);

        drawBackgroundGrid(g, b.toNearestInt());

        drawDragDistance(g, getViewportPosition());

        b.removeFromTop((float)FirstItemOffset);
        
        ctx.fullBounds = b;
        ctx.dataRangeToShow = getScaledZoomRange();
		ctx.valueRange = yRange;

        for(auto d: data)
            d->draw(g, ctx);

        String v;

        auto zr = getScaledZoomRange();
        auto vr = yRange;

        v << "X: [" << getXString(zr.getStart(), timeDomain,  xRange) << " - " << getXString(zr.getEnd(), timeDomain, xRange) << "]";
        v << "Y: [" << getYString(vr.getStart(), valueDomain, vr)     << " - " << getYString(vr.getEnd(), valueDomain, vr)    << "]";

        g.setFont(GLOBAL_MONOSPACE_FONT());
        g.drawText(v, ctx.legend, Justification::right);

        if(currentTooltipData)
        {
            g.setColour(currentTooltipData.d->getColour());

            auto r = Rectangle<int>(currentTooltipData.position, currentTooltipData.position).withSizeKeepingCentre(14, 14).toFloat();
            g.drawEllipse(r, 3.0f);
        }
    }

    static String getXString(float xValue, IndexDomain d, Range<float> contextValue)
    {
        switch(d)
        {
        case IndexDomain::Absolute:
            return String(xValue, 1);
        case IndexDomain::Relative:
            return String(xValue / contextValue.getLength(), 1) + "%";
        case IndexDomain::Time:
            return String(1000.0 * xValue / contextValue.getLength()) + "ms";
        }

        return String(xValue, 1);
    }

    static String getYString(float yValue, ValueDomain d, Range<float> contextRange)
    {
        switch(d)
        {
        case ValueDomain::Absolute:
            return String(yValue);
        case ValueDomain::Relative:
            return String(100.0 * NormalisableRange<float>(contextRange).convertTo0to1(yValue), 1) + "%";
        case ValueDomain::Decibel:
            return String(Decibels::gainToDecibels(std::abs(yValue)), 1) + "dB";
        default: ;
        }

        return String(yValue);
    }

    struct TooltipData
    {
        operator bool() const { return d != nullptr; }
        Data* d = nullptr;

        String toString(ValueDomain vd, Range<float> r) const
        {
            jassert(*this);

            String s;
            s << d->label << "[";
            s << String(value.first) << "] = " << getYString(value.second, vd, r);
            return s;
        }
        std::pair<int, float> value;
        Point<int> position;
    };

    Rectangle<int> getViewportPosition() const
    {
        auto b = getLocalBounds().reduced(10);
        b.removeFromTop(24 + FirstItemOffset);
        return b;
    }

    TooltipData getTooltipData(const MouseEvent& e)
    {
        auto vp = getViewportPosition();

        if(!vp.contains(e.getPosition()))
            return {};

        auto normX = (double)(e.getPosition().getX() - vp.getX()) / (double)vp.getWidth();

        auto zr = getScaledZoomRange();

        if(zr.getLength() == 0.0)
            return {};

        zr.setEnd(zr.getEnd() - 1);

        NormalisableRange<double> scaled(zr);
        auto index = roundToInt((float)scaled.convertFrom0to1(normX));

        auto xPos = scaled.convertTo0to1((double)index);

        auto x = vp.getX() + roundToInt(xPos * (float)vp.getWidth());

        std::vector<TooltipData> matches;

		NormalisableRange<float> fullValueRange(yRange);

        for(auto d: data)
        {
            TooltipData nd;
            nd.d = d;
            auto v = d->data[index];
            nd.value = { index, v };
            auto yPos = vp.getY() + roundToInt((1.0f - fullValueRange.convertTo0to1(v)) * (float)vp.getHeight());
            nd.position = { x, yPos};
            matches.push_back(nd);
        }

        

        auto distance = INT_MAX;
        TooltipData bestMatch;

        for(const auto& d: matches)
        {
            auto thisDistance = d.position.getDistanceFrom(e.getPosition());
            if(thisDistance < distance)
            {
                distance = thisDistance;
                bestMatch = d;
            }
        }

        return distance < 30 ? bestMatch : TooltipData();
    }

    TooltipData currentTooltipData;

    String getTooltip() override
    {
        if(currentTooltipData)
            return currentTooltipData.toString(valueDomain, yRange);

        return "";
    }

    String getTextForDistance(float width) const override
    {
		if(useIntegerGrid)
			return String(roundToInt(width));
		else
			return getXString(width, timeDomain, xRange);
    }

    void resized() override
    {
        auto b = getLocalBounds();
        b.removeFromTop(24);
        onResize(b);
    }
    
    void mouseMagnify(const MouseEvent& e, float sf) override
    {
        hangleMouseMagnify(e, sf);
    }

    void mouseDown(const MouseEvent& e) override
    {
        if(e.mods.isRightButtonDown())
        {
            PopupLookAndFeel plaf;
            PopupMenu m;
            m.setLookAndFeel(&plaf);

            m.addSectionHeader("Toggle graphs");

            for(int i = 0; i < data.size(); i++)
            {
                m.addItem(i + 30, data[i]->label, true, data[i]->enabled);
            }

            m.addSectionHeader("Select X Domain");

            auto xdomains = getIndexDomainTypes();
            auto ydomains = getValueDomainTypes();

            for(int i = 0; i < xdomains.size(); i++)
                m.addItem(i+10, xdomains[i], true, (int)timeDomain == i);

            m.addSectionHeader("Select Y Domain");

            for(int i = 0; i < ydomains.size(); i++)
                m.addItem(i+20, ydomains[i], true, (int)valueDomain == i);

            auto r = m.show();

            if(r >= 30)
            {
                data[r-30]->enabled = !data[r-30]->enabled;
            }
            if(r >= 20)
                valueDomain = (ValueDomain)(r - 20);
            else if(r >= 10)
                timeDomain = (IndexDomain)(r - 10);

            repaint();
            return;
        }

        handleMouseEvent(e, MouseEventType::MouseDown);
    }
    void mouseDrag(const MouseEvent& e) override { handleMouseEvent(e, MouseEventType::MouseDrag); }
    void mouseDoubleClick(const MouseEvent& e) override { handleMouseEvent(e, MouseEventType::MouseDoubleClick); }
    void mouseUp(const MouseEvent& e) override { handleMouseEvent(e, MouseEventType::MouseUp); }
    void mouseMove(const MouseEvent& e) override
    {
        handleMouseEvent(e, MouseEventType::MouseMove);
        currentTooltipData = getTooltipData(e);
        repaint();
    }
    void mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel) override { handleMouseWheelEvent(e, wheel); }

    void applyPosition(const Rectangle<int>& screenBoundsOfTooltipClient, Rectangle<int>& tooltipRectangleAtOrigin) override
    {
        if(currentTooltipData)
        {
            auto pos = screenBoundsOfTooltipClient.getPosition();
            auto center = pos + currentTooltipData.position;
            center = center.translated(0, 30);
            tooltipRectangleAtOrigin.setCentre(center.x, center.y);
        }
    }

    void setData(const var& var, const float* ptr, size_t size)
    {
        auto index = (int)var["index"];
        auto label = var["graph"].toString();
        bool found = false;

        for(auto d: data)
        {
            if(d->index == index)
            {
                d->label = label;
                d->data.allocate(size, false);
                d->size = size;
                FloatVectorOperations::copy(d->data.get(), ptr, (int)size);
                d->valueRange = {FloatVectorOperations::findMinAndMax(ptr, (int)size)};
                found = true;
            }
        }

        if(!found)
        {
            auto d = new Data();
            d->index = index;
            d->label = label;
            d->data.allocate(size, false);
            d->size = size;
            d->valueRange = {FloatVectorOperations::findMinAndMax(ptr, (int)size)};

            FloatVectorOperations::copy(d->data.get(), ptr, (int)size);
            data.add(d);
        }

        maxLength = 0;
        yRange = {};
        xRange = {};

        for(auto d: data)
        {
            yRange = yRange.getUnionWith(d->valueRange.getRange());
            maxLength = jmax(maxLength, d->size);
            xRange = xRange.getUnionWith({0.0f, (float)d->size});
        }
            

        updateIndexToShow();
    }

    Range<float> yRange;
    Range<float> xRange;

    size_t maxLength = 0;

    ValueDomain valueDomain = ValueDomain::Absolute;
    IndexDomain timeDomain = IndexDomain::Absolute;
};

RectViewer::RectViewer(const String& label, const var& data):
	resizer(this, nullptr)
{
	setMouseCursor(MouseCursor::CrosshairCursor);

	addAndMakeVisible(resizer);

	callRecursive(data, label, [&](const String& label, const var& obj)
	{
		if(auto r = dynamic_cast<RectangleDynamicObject*>(obj.getDynamicObject()))
		{
			Item ni;
			ni.label = label;
			ni.rectangle = r->getRectangle();
			ni.c = Colour((uint32)ni.label.hash()).withSaturation(0.5).withBrightness(0.7f).withAlpha(1.0f);

			items.add(ni);
		}
		return false;
	});

	for(auto& i: items)
	{
		fullBounds = fullBounds.getUnion(i.rectangle);
	}

	auto w = fullBounds.getWidth() + jmax(0.0, -1.0 * fullBounds.getX());
	auto h = fullBounds.getHeight() + jmax(0.0, -1.0 * fullBounds.getY());

	setSize(jlimit(200, 800, (int)w),
		    jlimit(200, 800, (int)h));
}

void RectViewer::mouseMove(const MouseEvent& e)
{
	auto lb = getLocalBounds().toFloat();

	auto t = AffineTransform::translation(jmax(0.0, -1.0 * fullBounds.getX()), jmax(0.0, -1.0 * fullBounds.getY()));

	auto w = fullBounds.getWidth() + jmax(fullBounds.getX(), 0.0);
	auto h = fullBounds.getHeight() + jmax(fullBounds.getY(), 0.0);

	t = t.followedBy(AffineTransform::scale(lb.getWidth() / w, lb.getHeight() / h));

	int idx = 0;
	hoveredIndexes.clear();

	for(const auto& i: items)
	{
		if(i.rectangle.transformedBy(t).contains(e.getPosition().toDouble()))
		{
			hoveredIndexes.add(idx);
		}

		idx++;
	}

	repaint();
}

void RectViewer::paint(Graphics& g)
{
	auto lb = getLocalBounds().toFloat();

	auto t = AffineTransform::translation(jmax(0.0, -1.0 * fullBounds.getX()), jmax(0.0, -1.0 * fullBounds.getY()));

	auto w = fullBounds.getWidth() + jmax(fullBounds.getX(), 0.0);
	auto h = fullBounds.getHeight() + jmax(fullBounds.getY(), 0.0);

	t = t.followedBy(AffineTransform::scale(lb.getWidth() / w, lb.getHeight() / h));

	auto f = GLOBAL_MONOSPACE_FONT();

	Array<Rectangle<float>> textBounds;

	int idx = 0;

	for(const auto& i: items)
	{
		auto r = i.rectangle.toFloat().transformed(t);
		g.setColour(i.c.withAlpha(hoveredIndexes.contains(idx) ? 1.0f : 0.7f));
		g.setFont(f);

		auto tb = r.withSizeKeepingCentre(f.getStringWidthFloat(i.label) + 10.0f, 20.f).constrainedWithin(lb);

		for(const auto& e: textBounds)
		{
			if(e.intersects(tb))
				tb = tb.translated(0.0f, 20.0f).constrainedWithin(lb);
		}

		textBounds.add(tb);

		g.drawText(i.label, tb, Justification::centred);
		g.setColour(i.c.withAlpha(hoveredIndexes.contains(idx++) ? 0.1f : 0.05f));
		g.fillRect(r);
		g.setColour(i.c.withAlpha(0.3f));
		g.drawRect(r, 1.0f);
	}

	if(!currentDragRectangle.isEmpty())
	{
		auto r = currentDragRectangle.toFloat().transformedBy(t.inverted()).toString();

		auto tb = currentDragRectangle.toFloat().withSizeKeepingCentre(f.getStringWidthFloat(r) + 10.0f, 20.0f).constrainedWithin(lb);

		for(auto e: textBounds)
		{
			if(e.intersects(tb))
				tb = tb.translated(0.0f, 20.0f).constrainedWithin(lb);
		}

		auto c = Colour(SIGNAL_COLOUR);

		g.setColour(c.withAlpha(0.1f));
		g.fillRect(currentDragRectangle);
		g.setColour(c);
		g.drawRect(currentDragRectangle, 1);
		g.drawText(r, tb, Justification::centred);
	}
}

bool RectViewer::callRecursive(const var& v, const String& label,
	const std::function<bool(const String&, const var&)>& f)
{

	if(f(label, v))
		return true;

	if(v.isArray())
	{
		int idx = 0;

		for(const auto& i: *v.getArray())
		{
			auto thisLabel = label + "[" + String(idx++) + "]";	

			if(callRecursive(i, thisLabel, f))
				return true;
		}
	}
	else if (auto dyn = v.getDynamicObject())
	{
		for(const auto& p: dyn->getProperties())
		{
			auto thisLabel = label + "." + p.name.toString();	

			if(callRecursive(p.value, thisLabel, f))
				return true;
		}
	}

	return false;
}

bool RectViewer::wantsRectangleViewer(const var& v)
{
	return callRecursive(v, {}, [](const String&, const var& obj)
	{
		return dynamic_cast<RectangleDynamicObject*>(obj.getObject()) != nullptr;
	});
}

BufferViewer::BufferViewer(DebugInformationBase* info, ApiProviderBase::Holder* holder_):
    ApiComponentBase(holder_),
    Component("Graph Viewer: " + info->getCodeToInsert()),
    resizer(this, nullptr)
{
    addAndMakeVisible(newViewer = new ZoomableDataViewer());
    addAndMakeVisible(resizer);
    setFromDebugInformation(info);
    startTimer(30);
    setSize(700, 500);
}

void BufferViewer::providerCleared()
{
    dataToShow = var();
}

void BufferViewer::providerWasRebuilt()
{
    if (auto p = getProviderBase())
    {
        for (int i = 0; i < p->getNumDebugObjects(); i++)
        {
            auto di = p->getDebugInformation(i);

            di->callRecursive([&](DebugInformationBase& info)
            {
                if (info.getCodeToInsert() == codeToInsert)
                {
                    setFromDebugInformation(&info);
                    dirty = true;
                    return true;
                }

                return false;
            });
        };
    }
}

void BufferViewer::removeSizeOneArrays(var& dataToReduce)
{
    if(dataToReduce.isArray())
    {
        if(dataToReduce.size() == 1)
        {
            auto firstValue = dataToReduce[0];
            dataToReduce = firstValue;
        }
        else
        {
            for(auto& v: *dataToReduce.getArray())
                removeSizeOneArrays(v);
        }
    }

	if(auto obj = dataToReduce.getDynamicObject())
	{
		DynamicObject::Ptr keepAlive(obj);

		if(obj->getProperties().size() == 1)
			dataToReduce = obj->getProperties().getValueAt(0);
		
	}
}

void BufferViewer::setFromDebugInformation(DebugInformationBase* info)
{
    if (info != nullptr)
    {
        codeToInsert = info->getCodeToInsert();
        dataToShow = info->getVariantCopy();

        labels.clear();

        for(int i = 0; i < info->getNumChildElements(); i++)
            labels.add(info->getChildElement(i)->getCodeToInsert());
    }
}

std::string BufferViewer::btoa(const void* data, size_t numBytes)
{
    static const std::string b64Characters =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    auto bytesToEncode = reinterpret_cast<const uint8*>(data);

    std::string ret;
    ret.reserve(numBytes);
    int i = 0;
    int j = 0;
    uint8 d1[3];
    uint8 d2[4];

    while (numBytes--)
    {
        d1[i++] = *(bytesToEncode++);
        if (i == 3)
        {
            d2[0] = (d1[0] & 0xfc) >> 2;
            d2[1] = ((d1[0] & 0x03) << 4) + ((d1[1] & 0xf0) >> 4);
            d2[2] = ((d1[1] & 0x0f) << 2) + ((d1[2] & 0xc0) >> 6);
            d2[3] = d1[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += b64Characters[d2[i]];

            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
            d1[j] = '\0';

        d2[0] = (d1[0] & 0xfc) >> 2;
        d2[1] = ((d1[0] & 0x03) << 4) + ((d1[1] & 0xf0) >> 4);
        d2[2] = ((d1[1] & 0x0f) << 2) + ((d1[2] & 0xc0) >> 6);
        d2[3] = d1[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += b64Characters[d2[j]];

        while((i++ < 3))
            ret += '=';
    }

    return ret;
}




bool BufferViewer::isArrayOrBuffer(const var& v, bool numbersAreOK)
{
    if(v.isArray())
    {
        auto isArray = true;

        for(const auto& c: *v.getArray())
            isArray &= isArrayOrBuffer(c, true);

        return isArray;
    }
    if(v.isBuffer())
        return true;
    if(auto obj = v.getDynamicObject())
    {
		auto isArray = true;

		for(const auto& nv: obj->getProperties())
		{
			isArray &= isArrayOrBuffer(nv.value, false);
		}

        return isArray;
    }
    if(v.isInt() || v.isDouble() || v.isInt64() || v.isBool())
        return numbersAreOK;

    return false;
}

void BufferViewer::timerCallback()
{
    if (dirty && !dataToShow.isUndefined())
    {
        if(!isArrayOrBuffer(dataToShow))
            return;

        removeSizeOneArrays(dataToShow);

        auto createCode = [&](int index, const String& name, const var& d)
        {
            DynamicObject::Ptr obj = new DynamicObject();
            obj->setProperty("index", index);
            obj->setProperty("graph", name);

            auto header = JSON::toString(var(obj.get()), true);

            if(d.isBuffer())
            {
                auto ptr = d.getBuffer()->buffer.getReadPointer(0);
                auto size = d.getBuffer()->size;

                setData(var(obj.get()), ptr, size);
            }
                
            if(d.isArray())
            {
                heap<float> data;
                data.setSize(d.size());
                for(int i = 0; i < d.size(); i++)
                    data[i] = (float)d[i];

                auto ptr = data.begin();
                auto size = data.size();

                setData(var(obj.get()), ptr, size);
            }
        };

        if(dataToShow.isBuffer())
        {
            createCode(0, codeToInsert, dataToShow);
        }

        else if(dataToShow.isArray())
        {
            auto numElements = dataToShow.size();

            if(numElements == 0)
                createCode(0, "empty", var(Array<var>()));
            else if(numElements == 1)
            {
                if(isArrayOrBuffer(dataToShow[0]))
                    createCode(0, labels[0], dataToShow[0]);
            }
            else
            {
                auto multiArray = dataToShow[0].isArray() || dataToShow[0].isBuffer();

                if(multiArray)
                {
                    int idx = 0;
                    for(const auto& d: *dataToShow.getArray())
                    {
                        createCode(idx, labels[idx], d);
                        idx++;
                    }
                }
                else
                    createCode(0, codeToInsert, dataToShow);
            }
        }
		else if(auto obj = dataToShow.getDynamicObject())
		{
			int idx = 0;
			for(const auto& nv: obj->getProperties())
            {
                createCode(idx, labels[idx], nv.value);
                idx++;
            }
		}
        dirty = false;
    }
}

void BufferViewer::resized()
{
    auto b = getLocalBounds();
    resizer.setBounds(b.removeFromBottom(20).removeFromRight(20));
    newViewer->setBounds(b);
}

void BufferViewer::setData(const var& obj, const float* data, size_t numElements)
{
    dynamic_cast<ZoomableDataViewer*>(newViewer.get())->setData(obj, data, numElements);
}

}
