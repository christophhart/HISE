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



const FloatingTile* FloatingTileContainer::getComponent(int index) const
{
	return components[index];
}

FloatingTile* FloatingTileContainer::getComponent(int index)
{
	return components[index];
}

int FloatingTileContainer::getNumComponents() const
{
	return components.size();
}

void FloatingTileContainer::clear()
{
	// if this doesn't work, you're in trouble soon...
	jassert(dynamic_cast<Component*>(this) != nullptr);

	const int numToClear = components.size();

	for (int i = 0; i < numToClear; i++)
	{
		removeFloatingTile(components.getLast());
	}
}

int FloatingTileContainer::getIndexOfComponent(const FloatingTile* componentToLookFor) const
{
	return components.indexOf(componentToLookFor);
}

void FloatingTileContainer::addFloatingTile(FloatingTile* newComponent)
{
	components.add(newComponent);

	componentAdded(newComponent);

	newComponent->refreshRootLayout();
}

void FloatingTileContainer::removeFloatingTile(FloatingTile* componentToRemove)
{
	ScopedPointer<FloatingTile> temp = components.removeAndReturn(components.indexOf(componentToRemove));

	componentRemoved(temp.get());

	temp.get()->refreshRootLayout();
}

bool FloatingTileContainer::shouldIntendAddButton() const
{
	return getParentShell()->isLayoutModeEnabled() && FloatingTile::LayoutHelpers::showFoldButton(getParentShell());
}

void FloatingTileContainer::enableSwapMode(bool shouldBeSwappable, FloatingTile* source)
{
	resizeSource = shouldBeSwappable ? source : nullptr;

	for (int i = 0; i < components.size(); i++)
	{
		components[i]->enableSwapMode(shouldBeSwappable, resizeSource.getComponent());
	}
}

void FloatingTileContainer::setAllowInserting(bool shouldBeAllowed)
{
	allowInserting = shouldBeAllowed;
}

FloatingTabComponent::CloseButton::CloseButton() :
	ShapeButton("Close", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	setShape(p, false, true, true);

	addListener(this);
}

void FloatingTabComponent::CloseButton::buttonClicked(Button* )
{
	TabBarButton* button = findParentComponentOfClass<TabBarButton>();
	auto* tab = findParentComponentOfClass<FloatingTabComponent>();

	auto cToRemove = tab->getComponent(button->getIndex());

	if (cToRemove->isEmpty() || PresetHandler::showYesNoWindow("Delete tab", "Do you want to delete the tab " + cToRemove->getCurrentFloatingPanel()->getTitle()))
		tab->removeFloatingTile(cToRemove);
}

int FloatingTabComponent::LookAndFeel::getTabButtonBestWidth(TabBarButton &b, int )
{
	auto w = GLOBAL_BOLD_FONT().getStringWidthFloat(b.getButtonText());

	return (int)(w + 48);
}

void FloatingTabComponent::LookAndFeel::drawTabButton(TabBarButton &b, Graphics &g, bool isMouseOver, bool /*isMouseDown*/)
{
	if (isMouseOver)
	{
		g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x13000000)));
		g.fillRect(0, 0, b.getWidth(), 22);
	}

	g.setColour(Colours::black.withAlpha(0.1f));

	auto a = b.getToggleState() ? 1.0f : (isMouseOver ? 0.8f : 0.6f);

	g.setColour(Colours::white.withAlpha(a));
	g.setFont(GLOBAL_BOLD_FONT());

	g.drawText(b.getButtonText(), 5, 4, b.getWidth() - 10, b.getHeight() - 4, Justification::centredLeft);
}


Rectangle< int > FloatingTabComponent::LookAndFeel::getTabButtonExtraComponentBounds(const TabBarButton &b, Rectangle< int > &/*textArea*/, Component &/*extraComp*/)
{
	return Rectangle<int>(b.getWidth() - 18, 6, 16, 16);
}

void FloatingTabComponent::LookAndFeel::drawTabAreaBehindFrontButton(TabbedButtonBar &b, Graphics &g, int , int )
{
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff333333)));

	if (b.getCurrentTabIndex() != -1)
	{
		g.fillRect(b.getTabButton(b.getCurrentTabIndex())->getBoundsInParent());
	}
		
}

FloatingTabComponent::FloatingTabComponent(FloatingTile* parent) :
	FloatingTileContainer(parent),
	TabbedComponent(TabbedButtonBar::TabsAtTop)
{
	addAndMakeVisible(addButton = new ShapeButton("Add Column", Colours::white.withAlpha(0.7f), Colours::white, Colours::white));

	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::addIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::addIcon));

	addButton->setShape(p, false, false, true);

	addButton->addListener(this);

	setOutline(0);

	setTabBarDepth(24);

	getTabbedButtonBar().setLookAndFeel(&laf);

	setColour(TabbedComponent::ColourIds::outlineColourId, Colours::transparentBlack);

	addFloatingTile(new FloatingTile(this));
}

FloatingTabComponent::~FloatingTabComponent()
{
	clear();
	clearTabs();
}

void FloatingTabComponent::componentAdded(FloatingTile* newComponent)
{
	int i = getNumTabs();

	addTab(newComponent->getName(), Colours::transparentBlack, newComponent, false);

	getTabbedButtonBar().getTabButton(i)->setExtraComponent(new CloseButton(), TabBarButton::ExtraComponentPlacement::afterText);


	String text = newComponent->getCurrentFloatingPanel()->getCustomTitle();

	if (text.isEmpty())
		text = "Untitled";

	setTabName(i, text);

	setCurrentTabIndex(getNumTabs() - 1);

	resized();
	repaint();
}

void FloatingTabComponent::componentRemoved(FloatingTile* deletedComponent)
{
	for (int i = 0; i < getNumTabs(); i++)
	{
		if (getTabContentComponent(i) == deletedComponent)
		{
			removeTab(i);
			break;
		}
	}

	setCurrentTabIndex(getNumTabs() - 1);

	resized();
	repaint();
}

ValueTree FloatingTabComponent::exportAsValueTree() const
{
	ValueTree v = FloatingTileContent::exportAsValueTree();

	v.setProperty("CurrentTab", getCurrentTabIndex(), nullptr);

	for (int i = 0; i < getNumComponents(); i++)
	{
		auto child = getComponent(i)->getCurrentFloatingPanel();

		ValueTree cv;

		cv = child->exportAsValueTree();
	
		v.addChild(cv, -1, nullptr);
	}

	return v;
}

void FloatingTabComponent::restoreFromValueTree(const ValueTree &v)
{
	clear();
	clearTabs();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		addFloatingTile(new FloatingTile(this, v.getChild(i)));
	}

	setCurrentTabIndex(v.getProperty("CurrentTab", -1));
}

void FloatingTabComponent::paint(Graphics& g)
{
	g.setColour(HiseColourScheme::getColour(HiseColourScheme::EditorBackgroundColourId));
	g.fillRect(0, 4, getWidth(), 18);

	g.setColour(Colour(0xFF333333));
	g.fillRect(0, 22, getWidth(), 2);
}

void FloatingTabComponent::resized()
{
	if (getParentComponent() == nullptr) // avoid resizing
		return;

	TabbedComponent::resized();

	if (getNumComponents() == getNumTabs())
	{
		for (int i = 0; i < getNumTabs(); i++)
		{
			String text = getComponent(i)->getCurrentFloatingPanel()->getCustomTitle();

			if (text.isEmpty())
				text = "Untitled";

			setTabName(i, text);

			if (getComponent(i) != nullptr && getComponent(i)->isReadOnly() || !getComponent(i)->canBeDeleted())
			{
				getTabbedButtonBar().getTabButton(i)->setExtraComponent(nullptr, TabBarButton::afterText);
			}
		}
	}
	else
		jassertfalse;

	

	if (!isInsertingEnabled())
		addButton->setVisible(false);

	const int intend = FloatingTile::LayoutHelpers::showFoldButton(getParentShell()) ? 16 : 0;

	if (shouldIntendAddButton())
		getTabbedButtonBar().setTopLeftPosition(intend, 0);

	auto b = getTabbedButtonBar().getTabButton(getTabbedButtonBar().getNumTabs() - 1);

	if (b != nullptr)
		addButton->setBounds(b->getRight() + intend + 4, 6, 16, 16);
	else
		addButton->setBounds(intend + 2, 6, 16, 16);
}

void FloatingTabComponent::buttonClicked(Button* )
{
	addFloatingTile(new FloatingTile(this));
}

void ResizableFloatingTileContainer::refreshLayout()
{
	rebuildResizers();
}

int ResizableFloatingTileContainer::getMinimumOffset() const
{
	return getDimensionOffset(getContainerBounds());
}

int ResizableFloatingTileContainer::getMaximumOffset() const
{
	return getDimensionEnd(getContainerBounds());
}

int ResizableFloatingTileContainer::getDimensionSize(Rectangle<int> area) const
{
	return isVertical() ? area.getHeight() : area.getWidth();
}

int ResizableFloatingTileContainer::getDimensionOffset(Rectangle<int> area) const
{
	return isVertical() ? area.getY() : area.getX();
}

int ResizableFloatingTileContainer::getDimensionEnd(Rectangle<int> area) const
{
	return isVertical() ? area.getBottom() : area.getRight();
}

ResizableFloatingTileContainer::ResizableFloatingTileContainer(FloatingTile* parent, bool isVerticalTile) :
	FloatingTileContainer(parent),
	vertical(isVerticalTile)
{
	addAndMakeVisible(addButton = new ShapeButton("Add Column", Colours::white.withAlpha(0.7f), Colours::white, Colours::white));

	Path p;

	if (isVertical())
		p.loadPathFromData(ColumnIcons::addRowIcon, sizeof(ColumnIcons::addRowIcon));
	else

		p.loadPathFromData(ColumnIcons::addColumnIcon, sizeof(ColumnIcons::addColumnIcon));

	addButton->setShape(p, false, false, true);

	addButton->addListener(this);

	addFloatingTile(new FloatingTile(this));

	setInterceptsMouseClicks(false, true);
}

ResizableFloatingTileContainer::~ResizableFloatingTileContainer()
{
	currentlyDisplayedComponents.clear();
	addButton = nullptr;
	resizers.clear();
}

String ResizableFloatingTileContainer::getTitle() const
{
	if (getParentShell()->isLayoutModeEnabled())
	{
		return FloatingTileContent::getCustomTitle();
	}
	else
	{
		if (hasCustomTitle())
			return FloatingTileContent::getCustomTitle();
		else
			return "";
	}
}

void ResizableFloatingTileContainer::buttonClicked(Button* b)
{
	if (b == addButton)
	{

		addFloatingTile(new FloatingTile(this));

#if LAYOUT_OLD
		storeSizes();
#endif
	}
}

void ResizableFloatingTileContainer::resized()
{
	if (getParentComponent() == nullptr) // avoid resizing
		return;

	addButton->setVisible(getParentShell()->isLayoutModeEnabled() && isInsertingEnabled());
	addButton->setBounds(shouldIntendAddButton() ? 18 : 1, 1, 30, 15);
	addButton->toFront(false);

	performLayout(getContainerBounds());
}

void ResizableFloatingTileContainer::foldComponent(Component* c, bool shouldBeFolded)
{
#if LAYOUT_OLD
	for (int i = 0; i < getNumComponents(); i++)
	{
		auto fsc = getComponent(i);

		if (fsc == c)
		{
			if (shouldBeFolded)
			{
				getLayoutManager()->setItemLayout(i * 2, 16, 16, 16);
				getLayoutManager()->setItemLayout(i * 2 + 1, 0, 0, 0);
			}

			else
			{

				int resizerWidth = fsc->isLayoutModeEnabled() ? 8 : 4;

				getLayoutManager()->setItemLayout(i * 2, -0.1, -1.0, -0.5);
				getLayoutManager()->setItemLayout(i * 2 + 1, resizerWidth, resizerWidth, resizerWidth);
			}
		}
	}

	dynamic_cast<Component*>(this)->resized();

#endif
}

void ResizableFloatingTileContainer::componentAdded(FloatingTile* c)
{
	addAndMakeVisible(c);

	c->setBounds(0, 0, isVertical() ? 0 : getWidth(), isVertical() ? getHeight() : 0);

	refreshLayout();
}

void ResizableFloatingTileContainer::componentRemoved(FloatingTile* c)
{
	removeChildComponent(c);
	refreshLayout();
}

ValueTree ResizableFloatingTileContainer::exportAsValueTree() const
{
	ValueTree v = FloatingTileContent::exportAsValueTree();

	
	
	for (int i = 0; i < getNumComponents(); i++)
	{
		auto child = getComponent(i)->getCurrentFloatingPanel();

		ValueTree cv = child->exportAsValueTree();
		getComponent(i)->getLayoutData().updateValueTree(cv);

		v.addChild(cv, -1, nullptr);

	}

	return v;
}

void ResizableFloatingTileContainer::restoreFromValueTree(const ValueTree &v)
{
	clear();
	resizers.clear();

	for (int i = 0; i < v.getNumChildren(); i++)
	{
		auto newShell = new FloatingTile(this, v.getChild(i));

		addFloatingTile(newShell);
	}
}




void ResizableFloatingTileContainer::performLayout(Rectangle<int> area)
{
	int totalSize = getDimensionSize(area);

	// First pass: check all folded & fixed-size panels;

	int availableSize = totalSize;
	double totalRelativeAmount = 0.0;

	//int resizerWidth = getParentShell()->isLayoutModeEnabled() ? 8 : 4;

	for (int i = 0; i < getNumComponents(); i++)
	{
		auto pc = getComponent(i);
		auto& lData = pc->getLayoutData();

		if (i < getNumComponents() - 1)
			availableSize -= resizers[i]->getCurrentSize();

		if (lData.isFolded)
			availableSize -= 16;
		else if (lData.isAbsolute)
			availableSize -= lData.currentSize;
		else
			totalRelativeAmount += lData.currentSize * -1.0;
	}

	int offset = getDimensionOffset(area);
	int numToLayout = getNumComponents();

	for (int i = 0; i < numToLayout; i++)
	{
		auto pc = getComponent(i);

		if (i > 0)
		{
			auto rs = resizers[i - 1]->getCurrentSize();
			setBoundsOneDimension(resizers[i - 1], offset, rs, area);
			offset += rs;
		}

		auto& lData = pc->getLayoutData();

		if (lData.isFolded)
		{
			setBoundsOneDimension(pc, offset, 16, area);
			offset += 16;
		}
		else if (lData.isAbsolute)
		{
			double size = jmax<int>(lData.currentSize, 16);

			setBoundsOneDimension(pc, offset, size, area);
			offset += lData.currentSize;
		}
		else
		{
			double percentage = lData.currentSize * -1.0;
			double scaledPercentage = percentage / totalRelativeAmount;
			int size = availableSize * scaledPercentage;
			size = jmax<int>(size, 16);

			setBoundsOneDimension(pc, offset, size, area);
			offset += size;
		}
	}
}

void ResizableFloatingTileContainer::rebuildResizers()
{
	resizers.clear();

	if (getNumComponents() > 1)
	{
		for (int i = 1; i < getNumComponents(); i++)
		{
			resizers.add(new InternalResizer(this, i-1));
			addAndMakeVisible(resizers.getLast());

			resizers.getLast()->setVisible(resizers.getLast()->hasSomethingToDo());
		}
	}

	resized();
}

void ResizableFloatingTileContainer::setBoundsOneDimension(Component* c, int offset, int size, Rectangle<int> area)
{
	Rectangle<int> newBounds;

	if (isVertical())
	{
		newBounds = Rectangle<int>(area.getX(), offset, area.getWidth(), size);
	}
	else
	{
		newBounds = Rectangle<int>(offset, area.getY(), size, area.getHeight());
	}

	if (animate)
	{
		if (c->isVisible())
			Desktop::getInstance().getAnimator().animateComponent(c, newBounds, 1.0f, 150, false, 1.3, 0.0);
		else
			c->setBounds(newBounds);
	}
	else
	{
		c->setBounds(newBounds);
	}
}

ResizableFloatingTileContainer::InternalResizer::InternalResizer(ResizableFloatingTileContainer* parent_, int index_) :
	parent(parent_),
	index(index_)
{
	int numTotalComponents = parent->getNumComponents();


	for (int i = 0; i < numTotalComponents; i++)
	{
		auto &lData = parent->getComponent(i)->getLayoutData();

		bool cantBeResized = lData.isLocked || lData.isFolded || lData.isAbsolute;

		if (cantBeResized)
			continue;

		if (i <= index)
			prevPanels.add(parent->getComponent(i));
		else
			nextPanels.add(parent->getComponent(i));
	}


	setRepaintsOnMouseActivity(true);
	setMouseCursor(parent_->isVertical() ? MouseCursor::UpDownResizeCursor : MouseCursor::LeftRightResizeCursor);

	resizeIcon.loadPathFromData(ColumnIcons::bigResizeIcon, sizeof(ColumnIcons::bigResizeIcon));

	if (!parent->isVertical())
		resizeIcon.applyTransform(AffineTransform::rotation(float_Pi / 2.0f));
}


bool ResizableFloatingTileContainer::InternalResizer::hasSomethingToDo() const
{
	return prevPanels.size() != 0 && nextPanels.size() != 0;
}

int ResizableFloatingTileContainer::InternalResizer::getCurrentSize() const
{
	const bool isBig = parent->bigResizers[index];

	if (isBig)
		return 32;

	return parent->getParentShell()->isLayoutModeEnabled() ? 4 : 4;
}

void ResizableFloatingTileContainer::InternalResizer::paint(Graphics& g)
{
	g.fillAll(Colours::white.withAlpha(0.02f));

	Colour c = Colour(SIGNAL_COLOUR);

	if (active)
		c = c.withBrightness(0.8f);
	else if (isMouseOver())
		c = c.withAlpha(0.2f);
	else
		c = Colours::transparentBlack;

	g.fillAll(c);

	if (getWidth() > 17 && getHeight() > 17)
	{
		resizeIcon.scaleToFit((float)(getWidth() / 2 - 12), (float)(getHeight() / 2 - 12), 24.0f, 24.0f, true);
		g.setColour(Colours::white.withAlpha(active ? 1.0f : 0.2f));
		g.fillPath(resizeIcon);
	}
}

void ResizableFloatingTileContainer::InternalResizer::mouseDown(const MouseEvent& e)
{
	if (e.mods.isRightButtonDown())
	{
		parent->bigResizers.setBit(index, !parent->bigResizers[index]);
		parent->resized();
		
	}
	else
	{
		auto area = parent->getContainerBounds();

		auto e2 = e.getEventRelativeTo(parent);

		downOffset = parent->isVertical() ? e2.getMouseDownY() : e2.getMouseDownX();

		parent->animate = false;
		active = true;

		prevDownSizes.clear();
		nextDownSizes.clear();

		totalPrevDownSize = 0.0;

		for (auto prevPanel : prevPanels)
		{
			prevDownSizes.add(prevPanel->getLayoutData().currentSize);
			totalPrevDownSize += prevDownSizes.getLast();
		}

		totalNextDownSize = 0.0;

		for (auto nextPanel : nextPanels)
		{
			nextDownSizes.add(nextPanel->getLayoutData().currentSize);
			totalNextDownSize += nextDownSizes.getLast();
		}
	}
}


void ResizableFloatingTileContainer::InternalResizer::mouseUp(const MouseEvent& event)
{
	parent->animate = true;
	active = false;
}

void ResizableFloatingTileContainer::InternalResizer::mouseDrag(const MouseEvent& event)
{
	auto ec = parent->getComponent(index);
	int delta = parent->isVertical() ? event.getDistanceFromDragStartY() : event.getDistanceFromDragStartX();
	auto area = parent->getContainerBounds();
	int totalSize = parent->isVertical() ? area.getHeight() : area.getWidth();
	int newWantedPosition = downOffset + delta;
	int actualPosition = jlimit<int>(parent->getMinimumOffset(), parent->getMaximumOffset(), newWantedPosition);
	int actualDelta = actualPosition - downOffset;
	double deltaRelative = (double)actualDelta / (double)totalSize;
	double prevNew = totalPrevDownSize - deltaRelative;
	double nextNew = totalNextDownSize + deltaRelative;
	double scalePrev = prevNew / totalPrevDownSize;
	double scaleNext = nextNew / totalNextDownSize;

	for (int i = 0; i < prevPanels.size(); i++)
		prevPanels[i]->getLayoutData().currentSize = scalePrev * prevDownSizes[i];

	for (int i = 0; i < nextPanels.size(); i++)
		nextPanels[i]->getLayoutData().currentSize = scaleNext * nextDownSizes[i];

	parent->resized();
}

