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

void FloatingTile::LayoutData::updateValueTree(ValueTree & v) const
{
	v.setProperty("Locked", isLocked, nullptr);
	v.setProperty("Folded", isFolded, nullptr);
	v.setProperty("isAbsolute", isAbsolute, nullptr);
	v.setProperty("Size", currentSize, nullptr);
	
	v.setProperty("LayoutEnabled", layoutModeEnabled, nullptr);
	v.setProperty("LayoutPossible", layoutModePossible, nullptr);
	v.setProperty("Swappable", swappable, nullptr);
	v.setProperty("Deletable", deletable, nullptr);
	v.setProperty("ReadOnly", readOnly, nullptr);
}


void FloatingTile::LayoutData::restoreFromValueTree(const ValueTree &v)
{
	isLocked = v.getProperty("Locked", false);
	isFolded = v.getProperty("Folded", false);
	isAbsolute = v.getProperty("Folded", false);
	currentSize = v.getProperty("Size", -0.5);

	layoutModeEnabled = v.getProperty("LayoutEnabled", true);
	layoutModePossible = v.getProperty("LayoutPossible", true);
	swappable = v.getProperty("Swappable", true);
	deletable = v.getProperty("Deletable", true);
	readOnly = v.getProperty("ReadOnly", false);
}

FloatingTile::CloseButton::CloseButton() :
	ShapeButton("Close", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	setShape(p, false, true, true);

	addListener(this);
}

void FloatingTile::CloseButton::buttonClicked(Button* )
{
	auto ec = dynamic_cast<FloatingTile*>(getParentComponent());

	Desktop::getInstance().getAnimator().fadeOut(ec->content.get(), 300);

	if (!ec->isEmpty())
	{
		if (auto tc = dynamic_cast<FloatingTileContainer*>(ec->content.get()))
			tc->clear();

		ec->addAndMakeVisible(ec->content = new EmptyComponent(ec));

		Desktop::getInstance().getAnimator().fadeIn(ec->content.get(), 300);

		ec->clear();

	
	}
	else
	{
		auto* cl = findParentComponentOfClass<FloatingTileContainer>();

		cl->removeFloatingTile(ec);
	}
}

FloatingTile::MoveButton::MoveButton() :
	ShapeButton("Move", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	Path p;
	p.loadPathFromData(ColumnIcons::moveIcon, sizeof(ColumnIcons::moveIcon));

	setShape(p, false, true, true);

	addListener(this);
}

void FloatingTile::MoveButton::buttonClicked(Button* )
{
	auto ec = dynamic_cast<FloatingTile*>(getParentComponent());

	ec->getRootComponent()->enableSwapMode(!ec->layoutData.swappingEnabled, ec);
}

FloatingTile::ResizeButton::ResizeButton() :
	ShapeButton("Move", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	addListener(this);
}


void FloatingTile::ResizeButton::buttonClicked(Button* )
{
	auto pc = findParentComponentOfClass<FloatingTile>();

	pc->toggleAbsoluteSize();
}

FloatingTile::FoldButton::FoldButton() :
	ShapeButton("Fold", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white.withAlpha(0.8f))
{
	addListener(this);
}

void FloatingTile::FoldButton::buttonClicked(Button* )
{
	auto pc = findParentComponentOfClass<FloatingTile>();
	
	pc->setFolded(!pc->getLayoutData().isFolded);

	if (auto cl = dynamic_cast<ResizableFloatingTileContainer*>(pc->getParentContainer()))
	{
		cl->refreshLayout();
	}
}

FloatingTile::LockButton::LockButton():
	ShapeButton("Lock", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white.withAlpha(0.8f))
{
	Path p;
	p.loadPathFromData(ColumnIcons::lockIcon, sizeof(ColumnIcons::lockIcon));

	setShape(p, false, true, true);

	addListener(this);
}


void FloatingTile::LockButton::buttonClicked(Button* b)
{
	auto pc = findParentComponentOfClass<FloatingTile>();
	
	pc->setLockedSize(!b->getToggleState());

	if (auto cl = dynamic_cast<ResizableFloatingTileContainer*>(pc->getParentContainer()))
	{
		cl->refreshLayout();
	}
}

void FloatingTile::toggleLayoutModeForParentContainer()
{
	if (getParentType() == ParentType::Root)
	{
		setLayoutModeEnabled(!isLayoutModeEnabled());
		return;
	}

	if(!getParentContainer())
		return;
	
	if(!getParentContainer()->getParentShell())
		return;
	
	bool en = getParentContainer()->getParentShell()->isLayoutModeEnabled();

	getParentContainer()->getParentShell()->setLayoutModeEnabled(!en);

}

FloatingTile::ParentType FloatingTile::getParentType() const
{
	if (getParentContainer() == nullptr)
		return ParentType::Root;
	else if (auto sl = dynamic_cast<const ResizableFloatingTileContainer*>(getParentContainer()))
	{
		return sl->isVertical() ? ParentType::Vertical : ParentType::Horizontal;
	}
	else if (auto tb = dynamic_cast<const FloatingTabComponent*>(getParentContainer()))
	{
		return ParentType::Tabbed;
	}

	jassertfalse;

	return ParentType::numParentTypes;
}


FloatingTile::FloatingTile(FloatingTileContainer* parent, ValueTree state /*= ValueTree()*/) :
	Component("Empty"),
	parentContainer(parent)
{
	panelFactory.registerAllPanelTypes();

	addAndMakeVisible(closeButton = new CloseButton());
	addAndMakeVisible(moveButton = new MoveButton());
	addAndMakeVisible(foldButton = new FoldButton());
	addAndMakeVisible(lockButton = new LockButton());
	addAndMakeVisible(resizeButton = new ResizeButton());

	setContent(state);
}

bool FloatingTile::isEmpty() const
{
	return dynamic_cast<const EmptyComponent*>(getCurrentFloatingPanel()) != nullptr;
}

void FloatingTile::setFolded(bool shouldBeFolded)
{
	if (shouldBeFolded != layoutData.isFolded)
	{
		Path p;
		p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));

		layoutData.isFolded = shouldBeFolded;

		bool rotate = shouldBeFolded;

		if (getParentType() == ParentType::Vertical)
			rotate = !rotate;

		if (rotate)
			p.applyTransform(AffineTransform::rotation(float_Pi / 2.0f));

		foldButton->setShape(p, false, true, true);
	}
}

void FloatingTile::setCanBeFolded(bool shouldBeFoldable)
{
	layoutData.foldable = shouldBeFoldable;
	resized();
}

bool FloatingTile::canBeFolded() const
{
	return layoutData.foldable;
}

void FloatingTile::setUseAbsoluteSize(bool shouldUseAbsoluteSize)
{
	layoutData.isAbsolute = shouldUseAbsoluteSize;

	Path p;

	if (shouldUseAbsoluteSize)
		p.loadPathFromData(ColumnIcons::absoluteIcon, sizeof(ColumnIcons::absoluteIcon));
	else
		p.loadPathFromData(ColumnIcons::relativeIcon, sizeof(ColumnIcons::relativeIcon));
	
	resizeButton->setShape(p, false, true, true);
}

void FloatingTile::toggleAbsoluteSize()
{
	if (auto pl = dynamic_cast<ResizableFloatingTileContainer*>(getParentContainer()))
	{
		setUseAbsoluteSize(!layoutData.isAbsolute);

		int totalWidth = pl->getDimensionSize(pl->getContainerBounds());

		if (layoutData.isAbsolute)
		{
			layoutData.currentSize = (double)pl->getDimensionSize(getLocalBounds());
		}
		else
		{
			double newAbsoluteSize = layoutData.currentSize / (double)totalWidth * -1.0;

			layoutData.currentSize = newAbsoluteSize;
		}

		pl->refreshLayout();
	}

}

void FloatingTile::setLockedSize(bool shouldBeLocked)
{
	layoutData.isLocked = shouldBeLocked;

	lockButton->setToggleState(shouldBeLocked, dontSendNotification);

	if(getParentContainer())
		getParentContainer()->refreshLayout();
}

const BackendRootWindow* FloatingTile::getRootWindow() const
{
	auto rw = findParentComponentOfClass<BackendRootWindow>();

	jassert(rw != nullptr);

	return rw;
}

BackendRootWindow* FloatingTile::getRootWindow()
{
	auto rw = dynamic_cast<BackendRootWindow*>(getRootComponent()->getParentComponent());

	jassert(rw != nullptr);

	return rw;
}

FloatingTile* FloatingTile::getRootComponent()
{
	if (getParentType() == ParentType::Root)
		return this;

	auto parent = findParentComponentOfClass<FloatingTile>();

	if (parent == nullptr)
	{
		return nullptr;
	}

	return parent->getRootComponent();
}

void FloatingTile::clear()
{
	layoutData.reset();
	setUseAbsoluteSize(false);
	setLockedSize(false);
	setFolded(false);

	refreshRootLayout();
}

void FloatingTile::refreshRootLayout()
{
	if (getRootComponent() != nullptr)
	{
		auto rootContainer = dynamic_cast<FloatingTileContainer*>(getRootComponent()->getCurrentFloatingPanel());

		Iterator<FloatingTileContainer> it(getRootComponent());

		while (auto tc = it.getNextPanel())
		{
			tc->refreshLayout();
		}
	}
}

void FloatingTile::setLayoutModeEnabled(bool shouldBeEnabled, bool setChildrenToSameSetting/*=true*/)
{
	if (isReadOnly())
		layoutData.layoutModeEnabled = false;
	else
		layoutData.layoutModeEnabled = shouldBeEnabled;


	if (setChildrenToSameSetting)
	{
		if (auto tc = dynamic_cast<FloatingTileContainer*>(getCurrentFloatingPanel()))
		{
			for (int i = 0; i < tc->getNumComponents(); i++)
			{
				tc->getComponent(i)->setLayoutModeEnabled(shouldBeEnabled, true);
			}

			tc->refreshLayout();
		}
	}
	
	resized();
	repaint();
}

void FloatingTile::setCanDoLayoutMode(bool shouldBeAllowed)
{
	layoutData.layoutModePossible = shouldBeAllowed;
}

bool FloatingTile::canDoLayoutMode() const
{
	return layoutData.layoutModePossible;
}

bool FloatingTile::hasChildren() const
{
	return dynamic_cast<FloatingTileContainer*>(content.get()) != nullptr;
}

void FloatingTile::enableSwapMode(bool shouldBeEnabled, FloatingTile* source)
{
	currentSwapSource = source;

	layoutData.swappingEnabled = shouldBeEnabled;

	if (auto cl = dynamic_cast<FloatingTileContainer*>(content.get()))
	{
		cl->enableSwapMode(shouldBeEnabled, source);
	}

	repaint();
}

void FloatingTile::swapWith(FloatingTile* otherComponent)
{
	if (otherComponent->isParentOf(this) || isParentOf(otherComponent))
	{
		PresetHandler::showMessageWindow("Error", "Can't swap parents with their children", PresetHandler::IconType::Error);
		return;
	}

	removeChildComponent(content);
	otherComponent->removeChildComponent(otherComponent->content);

	content.swapWith(otherComponent->content);

	addAndMakeVisible(content);
	otherComponent->addAndMakeVisible(otherComponent->content);


	resized();
	otherComponent->resized();

	repaint();
	otherComponent->repaint();

	bringButtonsToFront();
	otherComponent->bringButtonsToFront();
}

void FloatingTile::bringButtonsToFront()
{
	moveButton->toFront(false);
	foldButton->toFront(false);
	closeButton->toFront(false);
	lockButton->toFront(false);
	resizeButton->toFront(false);
}

void FloatingTile::paint(Graphics& g)
{
	auto pt = getParentType();

	const bool isVerticalAndFolded = getLayoutData().isFolded && !isInVerticalLayout();

	if (isVerticalAndFolded)
		return;

	const bool isInTabOrRoot = pt == ParentType::Tabbed || pt == ParentType::Root;

	if (isInTabOrRoot)
		return;

	if (!getCurrentFloatingPanel()->showTitleInPresentationMode() && !isLayoutModeEnabled())
		return;

	if (!canDoLayoutMode())
		return;

	Rectangle<int> titleArea = Rectangle<int>(leftOffset, 0, rightOffset - leftOffset, 16);

	if (titleArea.getWidth() > 40)
	{
		g.setGradientFill(ColourGradient(Colour(0xFF333333), 0.0f, 0.0f,
									     Colour(0xFF2D2D2D), 0.0f, 16.0f, false));

		g.setFont(GLOBAL_BOLD_FONT());
		g.fillRect(0, 0, getWidth(), 16);
		g.setColour(Colours::white.withAlpha(0.8f));
		g.drawText(getCurrentFloatingPanel()->getTitle(), titleArea, Justification::centred);
	}
}

void FloatingTile::paintOverChildren(Graphics& g)
{
	if (currentSwapSource == this)
		g.fillAll(Colours::white.withAlpha(0.1f));

	if (layoutData.swappable && layoutData.swappingEnabled && !hasChildren())
	{
		if (isMouseOver(true))
		{
			g.fillAll(Colours::white.withAlpha(0.1f));
			g.setColour(Colours::white.withAlpha(0.4f));
			g.drawRect(getLocalBounds());
		}
		else
		{
			g.fillAll(Colours::green.withAlpha(0.1f));
			g.setColour(Colours::green.withAlpha(0.2f));
			g.drawRect(getLocalBounds());
		}
			
	}
}

void FloatingTile::mouseEnter(const MouseEvent& )
{
	repaint();
}

void FloatingTile::mouseExit(const MouseEvent& )
{
	repaint();
}

void FloatingTile::mouseDown(const MouseEvent& event)
{
	if (event.mods.isRightButtonDown())
	{
		PopupMenu m;

		m.setLookAndFeel(&plaf);

		getPanelFactory()->handlePopupMenu(m, this);

	}
	else
	{
		if (layoutData.swappingEnabled && layoutData.swappable)
		{
			currentSwapSource->swapWith(this);

			getRootComponent()->enableSwapMode(false, nullptr);
		}
	}
}

void FloatingTile::resized()
{
	if (content.get() == nullptr)
		return;

	LayoutHelpers::setContentBounds(this);

	if (LayoutHelpers::showFoldButton(this))
	{
		leftOffset = 16;
		foldButton->setBounds(1, 1, 14, 14);
		foldButton->setVisible(LayoutHelpers::showFoldButton(this));
	}
	else
		foldButton->setVisible(false);


	rightOffset = getWidth();

	if (LayoutHelpers::showCloseButton(this))
	{
		rightOffset -= 18;

		closeButton->setVisible(rightOffset > 16);
		closeButton->setBounds(rightOffset, 0, 16, 16);
	}
	else
	{
		closeButton->setVisible(false);
	}

	if (LayoutHelpers::showMoveButton(this))
	{
		rightOffset -= 18;
		moveButton->setVisible(rightOffset > 16);
		moveButton->setBounds(rightOffset, 0, 16, 16);
		
	}
	else
		moveButton->setVisible(false);
	
	if (LayoutHelpers::showMoveButton(this))
	{
		rightOffset -= 18;
		resizeButton->setVisible(rightOffset > 16);
		resizeButton->setBounds(rightOffset, 0, 16, 16);
		
	}
	else
		resizeButton->setVisible(false);

	if (LayoutHelpers::showLockButton(this))
	{
		rightOffset -= 18;
		lockButton->setVisible(rightOffset > 16);
		lockButton->setBounds(rightOffset, 0, 16, 16);
		
	}
	else
		lockButton->setVisible(false);


	
}


double FloatingTile::getCurrentSizeInContainer()
{
	if (layoutData.isFolded)
	{
		return layoutData.currentSize;
	}
	else
	{
		if (layoutData.isAbsolute)
		{
			return getParentType() == ParentType::Horizontal ? getWidth() : getHeight();
		}
		else
			return layoutData.currentSize;
	}
}

const FloatingTileContent* FloatingTile::getCurrentFloatingPanel() const
{
	auto c = content.get();

	if(c != nullptr)
		return dynamic_cast<FloatingTileContent*>(content.get());

	return nullptr;
}

FloatingTileContent* FloatingTile::getCurrentFloatingPanel()
{
	auto c = content.get();

	if (c != nullptr)
		return dynamic_cast<FloatingTileContent*>(content.get());

	return nullptr;
}

bool FloatingTile::canBeDeleted() const
{
	return layoutData.deletable;
}

bool FloatingTile::isFolded() const
{
	return layoutData.isFolded;
}

bool FloatingTile::isInVerticalLayout() const
{
	return getParentType() == ParentType::Vertical;
}

void FloatingTile::setContent(ValueTree& data)
{
	if (data.isValid())
	{
		layoutData.restoreFromValueTree(data);
		addAndMakeVisible(content = dynamic_cast<Component*>(FloatingTileContent::createPanel(data, this)));
	}

	else
		addAndMakeVisible(content = new EmptyComponent(this));

	layoutData.isFolded = !layoutData.isFolded;

	setFolded(!layoutData.isFolded);
	setLockedSize(layoutData.isLocked);
	setUseAbsoluteSize(layoutData.isAbsolute);

	bringButtonsToFront();
}

void FloatingTile::setNewContent(const Identifier& newId)
{
	addAndMakeVisible(content = dynamic_cast<Component*>(FloatingTileContent::createNewPanel(newId, this)));

	int fixedSize = getCurrentFloatingPanel()->getFixedSizeForOrientation();

	if (fixedSize != 0)
	{
		layoutData.currentSize = fixedSize;
		layoutData.isLocked = true;
		layoutData.isAbsolute = true;
	}
		
	setLockedSize(layoutData.isLocked);
	refreshRootLayout();

	bringButtonsToFront();

	resized();
}

void FloatingTile::setDeletable(bool shouldBeDeletable)
{
	layoutData.deletable = shouldBeDeletable;
}

void FloatingTile::setSwappable(bool shouldBeSwappable)
{
	layoutData.swappable = shouldBeSwappable;
}

void FloatingTile::setReadOnly(bool shouldBeReadOnly)
{
	layoutData.readOnly = shouldBeReadOnly;

	setLayoutModeEnabled(false);
	setDeletable(false);
	setSwappable(false);

	if (layoutData.readOnly)
	{
		FloatingTile::Iterator<FloatingTileContent> it(this);

		while (auto p = it.getNextPanel())
			p->getParentShell()->setReadOnly(shouldBeReadOnly);
	}
}

bool FloatingTile::isReadOnly() const noexcept
{
	return layoutData.readOnly;
}

void FloatingTile::setSelector(const FloatingTileContent* originPanel, Point<int> mousePosition)
{
	if (originPanel != nullptr)
	{
		jassert(getParentType() == ParentType::Root);

		auto b = getLocalArea(originPanel->getParentShell(), originPanel->getParentShell()->getLocalBounds());

		addAndMakeVisible(currentSelector = new Selector(b, mousePosition));

		currentSelector->setBounds(getLocalBounds());
	}
	else
	{
		Desktop::getInstance().getAnimator().fadeOut(currentSelector, 150);
		currentSelector = nullptr;
	}
}

void FloatingTile::LayoutHelpers::setContentBounds(FloatingTile* t)
{
	t->content->setVisible(!t->isFolded());
	t->content->setBounds(t->getLocalBounds());
}

bool FloatingTile::LayoutHelpers::showCloseButton(const FloatingTile* t)
{
	if (!t->getCurrentFloatingPanel()->showTitleInPresentationMode() && !t->isLayoutModeEnabled())
		return false;

	if (!t->canBeDeleted())
		return false;

	if (t->isReadOnly())
		return false;

	if (!t->canDoLayoutMode())
		return false;

	if (!t->isLayoutModeEnabled() && (t->isEmpty() || t->hasChildren()))
		return false;

	auto pt = t->getParentType();

	switch (pt)
	{
	case ParentType::Root:
		return !t->isEmpty();
	case ParentType::Horizontal:
		return !t->isFolded();
	case ParentType::Vertical:
		return true;
	case ParentType::Tabbed:
		return false;
	}

	return false;
}

bool FloatingTile::LayoutHelpers::showMoveButton(const FloatingTile* t)
{
	if (!t->layoutData.swappable)
		return false;

	if (t->isReadOnly())
		return false;

	if (!t->isLayoutModeEnabled())
		return false;

	if (!t->canDoLayoutMode())
		return false;

	auto pt = t->getParentType();

	if (pt == ParentType::Root)
		return false;

	if (!t->isInVerticalLayout() && t->isFolded())
		return false;

	return true;
}

bool FloatingTile::LayoutHelpers::showFoldButton(const FloatingTile* t)
{
	if (!t->getCurrentFloatingPanel()->showTitleInPresentationMode() && !t->isLayoutModeEnabled())
		return false;

	if (!t->canBeFolded())
		return false;

	auto pt = t->getParentType();

	if (pt == ParentType::Root)
		return false;

	const bool isInTile = pt == ParentType::Horizontal || pt == ParentType::Vertical;

	return isInTile;
}

bool FloatingTile::LayoutHelpers::showLockButton(const FloatingTile* t)
{
	if (!t->isLayoutModeEnabled())
		return false;

	if (t->isReadOnly())
		return false;

	if (!t->canDoLayoutMode())
		return false;

	auto pt = t->getParentType();

	if (pt == ParentType::Horizontal || pt == ParentType::Vertical)
	{
		return t->isInVerticalLayout() || !t->isFolded();
	}

	return false;
}

