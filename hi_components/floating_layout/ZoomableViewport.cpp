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
}

ZoomableViewport::~ZoomableViewport()
{
	mouseWatcher = nullptr;
	content = nullptr;
}

void ZoomableViewport::mouseWheelMove(const MouseEvent& e, const MouseWheelDetails& wheel)
{
	if (e.mods.isCommandDown())
	{
		if (wheel.deltaY > 0)
			zoomFactor *= 1.15f;
		else
			zoomFactor /= 1.15f;

		zoomFactor = jlimit(0.25f, 3.0f, zoomFactor);

		setZoomFactor(zoomFactor, {});
	}
	else
	{
		auto zDelta = std::sqrt(zoomFactor);

		if (e.mods.isShiftDown())
			hBar.setCurrentRangeStart(hBar.getCurrentRangeStart() - wheel.deltaY * 0.1 / zDelta);
		else
			vBar.setCurrentRangeStart(vBar.getCurrentRangeStart() - wheel.deltaY * 0.1 / zDelta);
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
		centerCanvas();
	}
	else
	{
		refreshScrollbars();
	}
}

void ZoomableViewport::paint(Graphics& g)
{
	g.fillAll(Colour(0xff1d1d1d));

	if (!content->isVisible())
	{
		g.setColour(Colours::black.withAlpha(swapAlpha));
		g.drawImage(swapImage, swapBounds);
	}
}

void ZoomableViewport::zoomToRectangle(Rectangle<int> areaToShow)
{
	auto tBounds = getLocalBounds().toFloat();
	auto aBounds = areaToShow.toFloat();

	auto xZoom = tBounds.getWidth() / aBounds.getWidth();
	auto yZoom = tBounds.getHeight() / aBounds.getHeight();

	setZoomFactor(jmin(xZoom, yZoom, 3.0f), aBounds.getCentre());
}

void ZoomableViewport::setZoomFactor(float newZoomFactor, Point<float> centerPositionInGraph)
{
	zoomFactor = newZoomFactor;

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

	newZoom = jlimit(0.25f, 3.0f, newZoom);

	if (newZoom != zoomFactor)
	{
		setZoomFactor(newZoom, {});
		return true;
	}

	return false;
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

		auto sf = jmax(1.0f, jmin(hmath::pow(zoomFactor, 0.7f), maxWidthRatio, maxHeightRatio));

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

void ZoomableViewport::Laf::drawScrollbar(Graphics& g, ScrollBar&, int x, int y, int width, int height, bool isScrollbarVertical, int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown)
{
	g.fillAll(bg);

	float alpha = 0.3f;

	if (isMouseOver || isMouseDown)
		alpha += 0.1f;

	if (isMouseDown)
		alpha += 0.1f;

	g.setColour(Colours::white.withAlpha(alpha));

	auto area = Rectangle<int>(x, y, width, height).toFloat();

	if (isScrollbarVertical)
	{
		area.removeFromTop((float)thumbStartPosition);
		area = area.withHeight((float)thumbSize);
	}
	else
	{
		area.removeFromLeft((float)thumbStartPosition);
		area = area.withWidth((float)thumbSize);
	}

	auto cornerSize = jmin(area.getWidth(), area.getHeight());

	area = area.reduced(4.0f);
	cornerSize = jmin(area.getWidth(), area.getHeight());
	
	g.fillRoundedRectangle(area, cornerSize / 2.0f);
}

void ZoomableViewport::Holder::setBackground(Image img)
{
	bg = img.rescaled(img.getWidth() / 2, img.getHeight() / 2);

	stack.clear();

	ImageConvolutionKernel kernel(7);
	kernel.createGaussianBlur(30.0f);
	kernel.applyToImage(bg, bg, { 0, 0, bg.getWidth(), bg.getHeight() });
	repaint();
}

void ZoomableViewport::Holder::paint(Graphics& g)
{
	g.setColour(Colour(0xFF262626));
	auto b = getLocalBounds().toFloat();
	g.drawImage(bg, b, RectanglePlacement::fillDestination);
	g.fillAll(JUCE_LIVE_CONSTANT(Colour(0xe12b2b2b)));
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