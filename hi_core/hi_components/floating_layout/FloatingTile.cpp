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



FloatingTilePopup::FloatingTilePopup(Component* content_, Component* attachedComponent_, Point<int> localPoint) :
	content(content_),
	attachedComponent(attachedComponent_),
	localPointInComponent(localPoint)
{
	
	addAndMakeVisible(content);
	addAndMakeVisible(closeButton = new ShapeButton("Close", Colours::white.withAlpha(0.5f), Colours::white.withAlpha(0.8f), Colours::white));

	content->addComponentListener(this);

	attachedComponent->addComponentListener(this);

	Path closePath;
	closePath.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));

	closeButton->setShape(closePath, false, true, true);

	closeButton->addListener(this);

	const int offset = 12;

	setSize(content->getWidth() + 2*offset, content->getHeight() + 24 + 20 + 12);
}

void FloatingTilePopup::paint(Graphics &g)
{
	

	g.setColour(Colours::black.withAlpha(0.96f));

	if (arrowX > 0)
	{
		Path p;

		float l = (float)getWidth() - 12.0f;
		float yOff = 12.0f;
		float arc = 5.0f;
		float h = (float)getHeight() - 12.0f;

		p.startNewSubPath(arc + 1.0f, yOff);

		if (!arrowAtBottom)
		{
			p.lineTo(arrowX - 12.0f, yOff);
			p.lineTo(arrowX, 0.0f);
			p.lineTo(arrowX + 12.0f, yOff);
		}

		
		p.lineTo(l - arc, yOff);
		p.addArc(l - 2.0f*arc, yOff, 2.0f * arc, 2.f * arc, 0.0f, float_Pi / 2.0f);
		p.lineTo(l, h - arc);
		p.addArc(l - 2.0f*arc, h - 2.f*arc, 2.f * arc, 2.f * arc, float_Pi / 2.0f, float_Pi);
		

		if (arrowAtBottom)
		{
			p.lineTo(arrowX + 12.0f, h);
			p.lineTo(arrowX, (float)getHeight()-1.0f);
			p.lineTo(arrowX - 12.0f, h);
		}

		p.lineTo(arc + 1.0f, h);
		p.addArc(1.0f, h - 2.f*arc, 2.f * arc, 2.f * arc, float_Pi, float_Pi * 1.5f);

		p.lineTo(1.0f, yOff + arc);
		p.addArc(1.0f, yOff, 2.f * arc, 2.f * arc, float_Pi * 1.5f, float_Pi * 2.f);

		p.closeSubPath();

		g.fillPath(p);
		g.setColour(Colours::white.withAlpha(0.6f));
		g.strokePath(p, PathStrokeType(2.0f));

	}
	else
	{
		Rectangle<float> area(0.0f, 12.0f, (float)getWidth() - 12.0f, (float)getHeight() - 24.0f);

		g.fillRoundedRectangle(area, 5.0f);
		g.setColour(Colours::white.withAlpha(0.6f));
		g.drawRoundedRectangle((float)area.getX() + 1.0f, (float)area.getY() + 1.0f, (float)area.getWidth() - 2.0f, (float)area.getHeight() - 2.0f, 5.0f, 2.0f);
	}

	if (content != nullptr && content->getName().isNotEmpty())
	{
		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(content->getName(), 0, 12, getWidth(), 20, Justification::centred, false);
	}


	g.setColour(Colours::black);
	g.fillEllipse((float)getWidth() - 22.0f, 2.0f, 20.0f, 20.0f);
	g.setColour(Colours::white.withAlpha(0.5f));
	g.drawEllipse((float)getWidth() - 22.0f, 2.0f, 20.0f, 20.0f, 2.0f);

}

void FloatingTilePopup::resized()
{
	closeButton->setBounds(getWidth() - 21, 3, 18, 18);

	content->setBounds(6, 18 + 20, content->getWidth(), content->getHeight());
}

void FloatingTilePopup::componentMovedOrResized(Component& component, bool /*moved*/, bool wasResized)
{
	if (&component == attachedComponent.getComponent())
	{
		updatePosition();
	}
	else
	{
		if (wasResized)
		{
			const int offset = 12;

			setSize(content->getWidth() + 2 * offset, content->getHeight() + 24 + 20 + 12);

			updatePosition();
		}
	}
}


void FloatingTilePopup::componentBeingDeleted(Component& component)
{
	if (&component == attachedComponent)
	{
		component.removeComponentListener(this);
		attachedComponent = nullptr;
		updatePosition();
	}
}

void FloatingTilePopup::buttonClicked(Button* /*b*/)
{
	if(attachedComponent.getComponent())
		attachedComponent->removeComponentListener(this);

	attachedComponent = nullptr;
	updatePosition();
}

FloatingTilePopup::~FloatingTilePopup()
{
	if (content != nullptr) content->removeComponentListener(this);
	if (attachedComponent.getComponent()) attachedComponent->removeComponentListener(this);

	content = nullptr;
	closeButton = nullptr;
	attachedComponent = nullptr;
}


var FloatingTile::LayoutData::toDynamicObject() const
{
	return var(layoutDataObject);
}

void FloatingTile::LayoutData::fromDynamicObject(const var& objectData)
{
	layoutDataObject = objectData.getDynamicObject();
}

int FloatingTile::LayoutData::getNumDefaultableProperties() const
{
	return LayoutDataIds::numProperties;
}

Identifier FloatingTile::LayoutData::getDefaultablePropertyId(int i) const
{
	RETURN_DEFAULT_PROPERTY_ID(i, LayoutDataIds::Size, "Size");
	RETURN_DEFAULT_PROPERTY_ID(i, LayoutDataIds::Folded, "Folded");
	RETURN_DEFAULT_PROPERTY_ID(i, LayoutDataIds::MinSize, "MinSize");

	jassertfalse;

	return Identifier();
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

	PopupMenu m;

	m.setLookAndFeel(&ec->plaf);

	m.addItem(1, "Swap Position", !ec->isVital(), ec->getLayoutData().swappingEnabled);
	m.addItem(2, "Edit JSON", !ec->isVital(), false, ec->getPanelFactory()->getIcon(FloatingTileContent::Factory::PopupMenuOptions::ScriptEditor));

	if (ec->hasChildren())
	{
		PopupMenu containerTypes;

		auto thisContainer = dynamic_cast<FloatingTabComponent*>(ec->getCurrentFloatingPanel());

		bool isTabs = dynamic_cast<FloatingTabComponent*>(ec->getCurrentFloatingPanel()) != nullptr;
		bool isHorizontal = dynamic_cast<HorizontalTile*>(ec->getCurrentFloatingPanel()) != nullptr;
		bool isVertical = dynamic_cast<VerticalTile*>(ec->getCurrentFloatingPanel()) != nullptr;

		ec->getPanelFactory()->addToPopupMenu(containerTypes, FloatingTileContent::Factory::PopupMenuOptions::Tabs, "Tabs", !isTabs, isTabs);
		ec->getPanelFactory()->addToPopupMenu(containerTypes, FloatingTileContent::Factory::PopupMenuOptions::HorizontalTile, "Horizontal Tile", !isHorizontal, isHorizontal);
		ec->getPanelFactory()->addToPopupMenu(containerTypes, FloatingTileContent::Factory::PopupMenuOptions::VerticalTile, "Vertical Tile", !isVertical, isVertical);

		m.addSubMenu("Swap Container Type", containerTypes, !ec->isVital());	
	}

	const int result = m.show();

	if (result == 1)
	{
		ec->getRootComponent()->enableSwapMode(!ec->layoutData.swappingEnabled, ec);
	}
	else if (result == 2)
	{
		ec->editJSON();
	}
	if (result == (int)FloatingTileContent::Factory::PopupMenuOptions::Tabs)
	{
		ec->swapContainerType(FloatingTabComponent::getPanelId());
	}
	else if (result == (int)FloatingTileContent::Factory::PopupMenuOptions::HorizontalTile)
	{
		ec->swapContainerType(HorizontalTile::getPanelId());
	}
	else if (result == (int)FloatingTileContent::Factory::PopupMenuOptions::VerticalTile)
	{
		ec->swapContainerType(VerticalTile::getPanelId());
	}
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
	
	if (!pc->canBeFolded())
		return;

	pc->setFolded(!pc->isFolded());

	if (auto cl = dynamic_cast<ResizableFloatingTileContainer*>(pc->getParentContainer()))
	{
		cl->refreshLayout();
	}
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




FloatingTile::FloatingTile(FloatingTileContainer* parent, var data) :
	Component("Empty"),
	parentContainer(parent)
{
	setOpaque(true);

	panelFactory.registerAllPanelTypes();

	addAndMakeVisible(closeButton = new CloseButton());
	addAndMakeVisible(moveButton = new MoveButton());
	addAndMakeVisible(foldButton = new FoldButton());
	addAndMakeVisible(resizeButton = new ResizeButton());

	//layoutIcon.loadPathFromData(ColumnIcons::layoutIcon, sizeof(ColumnIcons::layoutIcon));

	setContent(data);
}

bool FloatingTile::isEmpty() const
{
	return dynamic_cast<const EmptyComponent*>(getCurrentFloatingPanel()) != nullptr;
}

bool FloatingTile::showTitle() const
{
	/** Show the title if:
	
		- is folded and in horizontal container
		- is not container and the panel wants a title in presentation mode
		- is dynamic container and in layout mode
		- is container and has a custom title or is foldable

		Don't show the title if:

		- is folded and in vertical container
		- is root
		- is in tab & layout mode is deactivated
	
	
	*/

	auto pt = getParentType();

	const bool isRoot = pt == ParentType::Root;
	const bool isInTab = pt == ParentType::Tabbed;
	
	if (isRoot)
		return false;

	if (isInTab && !isLayoutModeEnabled())
		return false;

	if (isFolded())
		return isInVerticalLayout();

	if (hasChildren())
	{
		const bool isDynamicContainer = dynamic_cast<const FloatingTileContainer*>(getCurrentFloatingPanel())->isDynamic();

		if (isDynamicContainer && isLayoutModeEnabled())
		{
			return true;
		}
		else
		{
			if (getCurrentFloatingPanel()->hasCustomTitle())
				return true;

			if (canBeFolded())
				return true;
		
			if (getCurrentFloatingPanel()->hasCustomTitle())
				return true;

			return false;
		}
			
	}
	else
	{
		return getCurrentFloatingPanel()->showTitleInPresentationMode();
	}
}

Rectangle<int> FloatingTile::getContentBounds()
{
	if (isFolded())
		return Rectangle<int>();

	if (showTitle())
		return Rectangle<int>(0, 16, getWidth(), getHeight() - 16);
	else
		return getLocalBounds();
}

void FloatingTile::setFolded(bool shouldBeFolded)
{
	if (!canBeFolded())
		return;

	layoutData.setFoldState(1 - (int)layoutData.isFolded());

	refreshFoldButton();
}

void FloatingTile::refreshFoldButton()
{
	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, sizeof(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));

	bool rotate = isFolded();

	if (getParentType() == ParentType::Vertical)
		rotate = !rotate;

	if (rotate)
		p.applyTransform(AffineTransform::rotation(float_Pi / 2.0f));

	foldButton->setShape(p, false, true, true);
}

void FloatingTile::setCanBeFolded(bool shouldBeFoldable)
{
	if (!shouldBeFoldable)
		layoutData.setFoldState(-1);
	else
		layoutData.setFoldState(0);

	resized();
}

bool FloatingTile::canBeFolded() const
{
	return layoutData.canBeFolded();
}

void FloatingTile::refreshPinButton()
{
	Path p;

	if (layoutData.isAbsolute())
		p.loadPathFromData(ColumnIcons::absoluteIcon, sizeof(ColumnIcons::absoluteIcon));
	else
		p.loadPathFromData(ColumnIcons::relativeIcon, sizeof(ColumnIcons::relativeIcon));
	
	resizeButton->setShape(p, false, true, true);
}

void FloatingTile::toggleAbsoluteSize()
{
	if (auto pl = dynamic_cast<ResizableFloatingTileContainer*>(getParentContainer()))
	{
		bool shouldBeAbsolute = !layoutData.isAbsolute();

		int totalWidth = pl->getDimensionSize(pl->getContainerBounds());

		if (shouldBeAbsolute)
		{
			layoutData.setCurrentSize(pl->getDimensionSize(getLocalBounds()));
		}
		else
		{
			double newAbsoluteSize = layoutData.getCurrentSize() / (double)totalWidth * -1.0;

			layoutData.setCurrentSize(newAbsoluteSize);
		}

		refreshPinButton();

		pl->refreshLayout();
	}
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

	auto parent = getParentContainer()->getParentShell(); //findParentComponentOfClass<FloatingTile>();

	if (parent == nullptr)
		return nullptr;

	return parent->getRootComponent();
}

const FloatingTile* FloatingTile::getRootComponent() const
{
	return const_cast<FloatingTile*>(this)->getRootComponent();
}

void FloatingTile::clear()
{
	layoutData.reset();
	refreshPinButton();
	refreshFoldButton();
	refreshMouseClickTarget();
	refreshRootLayout();
}

void FloatingTile::refreshRootLayout()
{
	if (getRootComponent() != nullptr)
	{
		auto rootContainer = dynamic_cast<FloatingTileContainer*>(getRootComponent()->getCurrentFloatingPanel());

		if(rootContainer != nullptr)
			rootContainer->refreshLayout();
	}
}

void FloatingTile::setLayoutModeEnabled(bool shouldBeEnabled, bool setChildrenToSameSetting/*=true*/)
{
	if (getParentType() == ParentType::Root)
	{
		layoutModeEnabled = shouldBeEnabled;

		refreshMouseClickTarget();

		Iterator<FloatingTileContent> all(this);

		while (auto p = all.getNextPanel())
		{
			if (auto c = dynamic_cast<FloatingTileContainer*>(p))
				c->refreshLayout();
			
			p->getParentShell()->resized();
			p->getParentShell()->repaint();
			p->getParentShell()->refreshMouseClickTarget();
		}
	}
}

bool FloatingTile::isLayoutModeEnabled() const
{
	if (getParentType() == ParentType::Root)
		return layoutModeEnabled;

	return canDoLayoutMode() && getRootComponent()->isLayoutModeEnabled();
}

bool FloatingTile::canDoLayoutMode() const
{
	if (getParentType() == ParentType::Root)
		return true;

	if (isVital())
		return false;

	return getParentContainer()->isDynamic();
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
	resizeButton->toFront(false);
}

void FloatingTile::paint(Graphics& g)
{
	g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::ModulatorSynthBackgroundColourId));

	auto pt = getParentType();

	if (showTitle())
	{
		g.setGradientFill(ColourGradient(Colour(0xFF222222), 0.0f, 0.0f,
			Colour(0xFF151515), 0.0f, 16.0f, false));

		g.fillRect(0, 0, getWidth(), 16);

		Rectangle<int> titleArea = Rectangle<int>(leftOffsetForTitleText, 0, rightOffsetForTitleText - leftOffsetForTitleText, 16);

		if (titleArea.getWidth() > 40)
		{
			g.setFont(GLOBAL_BOLD_FONT());
			g.setColour(Colours::white.withAlpha(0.8f));

			g.drawText(getCurrentFloatingPanel()->getBestTitle(), titleArea, Justification::centred);
		}
	}
}

void FloatingTile::paintOverChildren(Graphics& g)
{
	if (!hasChildren() && canDoLayoutMode() && isLayoutModeEnabled())
	{
		g.setColour(Colours::black.withAlpha(0.3f));
		g.fillAll();

		if (getWidth() > 80 && getHeight() > 80)
		{
			Path layoutPath;

			layoutPath.loadPathFromData(ColumnIcons::layoutIcon, sizeof(ColumnIcons::layoutIcon));

			g.setColour(Colours::white.withAlpha(0.1f));
			layoutPath.scaleToFit((float)(getWidth() - 40) / 2.0f, (float)(getHeight() - 40) / 2.0f, 40.0f, 40.0f, true);
			g.fillPath(layoutPath);
		}

	}
		

	if (currentSwapSource == this)
		g.fillAll(Colours::white.withAlpha(0.1f));

	if (isSwappable() && layoutData.swappingEnabled && !hasChildren())
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

void FloatingTile::refreshMouseClickTarget()
{
	if (isEmpty())
	{
		setInterceptsMouseClicks(true, false);
	}
	else if (!hasChildren())
	{
		bool allowClicksOnContent = !isLayoutModeEnabled();

		setInterceptsMouseClicks(!allowClicksOnContent, true);
		dynamic_cast<Component*>(getCurrentFloatingPanel())->setInterceptsMouseClicks(allowClicksOnContent, allowClicksOnContent);

	}
}

void FloatingTile::mouseEnter(const MouseEvent& )
{
	if(isLayoutModeEnabled())
		repaint();
}

void FloatingTile::mouseExit(const MouseEvent& )
{
	if (isLayoutModeEnabled())
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
		if (layoutData.swappingEnabled && isSwappable())
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
		leftOffsetForTitleText = 16;
		foldButton->setBounds(1, 1, 14, 14);
		foldButton->setVisible(LayoutHelpers::showFoldButton(this));
	}
	else
		foldButton->setVisible(false);


	rightOffsetForTitleText = getWidth();

	if (LayoutHelpers::showCloseButton(this))
	{
		rightOffsetForTitleText -= 18;

		closeButton->setVisible(rightOffsetForTitleText > 16);
		closeButton->setBounds(rightOffsetForTitleText, 0, 16, 16);
	}
	else
	{
		closeButton->setVisible(false);
	}

	if (LayoutHelpers::showMoveButton(this))
	{
		rightOffsetForTitleText -= 18;
		moveButton->setVisible(rightOffsetForTitleText > 16);
		moveButton->setBounds(rightOffsetForTitleText, 0, 16, 16);
		
	}
	else
		moveButton->setVisible(false);
	
	if (LayoutHelpers::showPinButton(this))
	{
		rightOffsetForTitleText -= 18;
		resizeButton->setVisible(rightOffsetForTitleText > 16);
		resizeButton->setBounds(rightOffsetForTitleText, 0, 16, 16);
		
	}
	else
		resizeButton->setVisible(false);
}


double FloatingTile::getCurrentSizeInContainer()
{
	if (isFolded())
	{
		return layoutData.getCurrentSize();
	}
	else
	{
		if (layoutData.isAbsolute())
		{
			return getParentType() == ParentType::Horizontal ? getWidth() : getHeight();
		}
		else
			return layoutData.getCurrentSize();
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

void FloatingTile::editJSON()
{
	auto codeEditor = new JSONEditor(getCurrentFloatingPanel());

	codeEditor->setSize(300, 400);
	showComponentInRootPopup(codeEditor, moveButton, moveButton->getLocalBounds().getCentre());
}

void FloatingTile::showComponentInRootPopup(Component* newComponent, Component* attachedComponent, Point<int> localPoint)
{
	if (getParentType() != ParentType::Root)
	{
		getRootComponent()->showComponentInRootPopup(newComponent, attachedComponent, localPoint);
	}
	else
	{
		if (newComponent != nullptr)
		{
			addAndMakeVisible(currentPopup = new FloatingTilePopup(newComponent, attachedComponent, localPoint));

			currentPopup->updatePosition();
		}
		else
		{
			currentPopup = nullptr;
		}
	}
}


void FloatingTilePopup::updatePosition()
{
	auto root = findParentComponentOfClass<FloatingTile>();

	if (root == nullptr)
		return;

	jassert(root != nullptr && root->getParentType() == FloatingTile::ParentType::Root);

	jassert(content != nullptr);

	if (attachedComponent.getComponent() != nullptr)
	{
		Point<int> pointInRoot = root->getLocalPoint(attachedComponent.getComponent(), localPointInComponent);

		int desiredWidth = getWidth();
		int desiredHeight = getHeight();

		int distanceToTop = pointInRoot.getY();
		int distanceToBottom = root->getHeight() - pointInRoot.getY();
		int distanceToLeft = pointInRoot.getX();
		int distanceToRight = root->getWidth() - pointInRoot.getX();

		int xToUse = -1;
		int yToUse = -1;

		if ((desiredWidth / 2) < distanceToRight)
		{
			xToUse = jmax<int>(0, pointInRoot.getX() - desiredWidth / 2);
		}
		else
		{
			xToUse = jmax<int>(0, root->getWidth() - desiredWidth);
		}

		arrowX = pointInRoot.getX() - xToUse;

		if (desiredHeight < distanceToBottom)
		{
			yToUse = pointInRoot.getY();
			arrowAtBottom = false;
		}
		else
		{
			yToUse = jmax<int>(0, pointInRoot.getY() - desiredHeight - 30);
			arrowAtBottom = true;

			if (yToUse == 0)
				arrowX = -1;
		}

		setTopLeftPosition(xToUse, yToUse);
		resized();
		repaint();
	}
	else
	{
		root->showComponentInRootPopup(nullptr, nullptr, {});
	}
}

bool FloatingTile::canBeDeleted() const
{
	if (getParentType() == ParentType::Root)
		return false;

	if (isVital())
		return false;

	return getParentContainer()->isDynamic();

	//return layoutData.deletable;
}

bool FloatingTile::isFolded() const
{
	return layoutData.isFolded();
}

bool FloatingTile::isInVerticalLayout() const
{
	return getParentType() == ParentType::Vertical;
}



void FloatingTile::setContent(const var& data)
{
	if (data.isUndefined() || data.isVoid())
	{
		addAndMakeVisible(content = new EmptyComponent(this));
		
	}
	else
	{
		layoutData.fromDynamicObject(data);

		addAndMakeVisible(content = dynamic_cast<Component*>(FloatingTileContent::createPanel(data, this)));
	}
		

	refreshFixedSizeForNewContent();

	refreshFoldButton();
	refreshPinButton();

	if(getParentContainer())
		getParentContainer()->refreshLayout();

	bringButtonsToFront();
	refreshMouseClickTarget();
	resized();
	repaint();
}

void FloatingTile::setNewContent(const Identifier& newId)
{
	addAndMakeVisible(content = dynamic_cast<Component*>(FloatingTileContent::createNewPanel(newId, this)));

	refreshFixedSizeForNewContent();
	
	if (hasChildren())
		setCanBeFolded(false);

	refreshRootLayout();

	bringButtonsToFront();
	refreshMouseClickTarget();

	resized();
}

bool FloatingTile::isSwappable() const
{
	if (getParentType() == ParentType::Root)
		return false;

	if (isVital())
		return false;

	return getParentContainer()->isDynamic();

}

void FloatingTile::refreshFixedSizeForNewContent()
{
	int fixedSize = getCurrentFloatingPanel()->getFixedSizeForOrientation();

	if (fixedSize != 0)
	{
		layoutData.setCurrentSize(fixedSize);
	}
}

void FloatingTile::LayoutHelpers::setContentBounds(FloatingTile* t)
{
	t->content->setVisible(!t->isFolded());
	t->content->setBounds(t->getLocalBounds());
	t->content->resized();
}

bool FloatingTile::LayoutHelpers::showCloseButton(const FloatingTile* t)
{
	if (t->getParentType() == ParentType::Tabbed)
		return false;

	return t->canBeDeleted();

#if 0
	if (!t->getCurrentFloatingPanel()->showTitleInPresentationMode() && !t->isLayoutModeEnabled())
		return false;

	if (!t->canBeDeleted())
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
#endif

}

bool FloatingTile::LayoutHelpers::showMoveButton(const FloatingTile* t)
{
	if (t->hasChildren() && dynamic_cast<const FloatingTileContainer*>(t->getCurrentFloatingPanel())->isDynamic() && t->isLayoutModeEnabled())
		return true;

	return showPinButton(t);
}

bool FloatingTile::LayoutHelpers::showPinButton(const FloatingTile* t)
{
	if (!t->isSwappable())
		return false;

	if (t->getParentType() == ParentType::Tabbed)
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
	if (!t->canBeFolded())
		return false;

	if (t->getParentType() == ParentType::Tabbed)
		return false;

	if (t->getParentType() == ParentType::Horizontal)
		return true;

	if (t->showTitle())
		return true;

	return false;
}

void FloatingTile::TilePopupLookAndFeel::getIdealPopupMenuItemSize(const String &text, bool isSeparator, int standardMenuItemHeight, int &idealWidth, int &idealHeight)
{
	if (isSeparator)
	{
		idealWidth = 50;
		idealHeight = standardMenuItemHeight > 0 ? standardMenuItemHeight / 2 : 10;
	}
	else
	{
		Font font(getPopupMenuFont());

		if (standardMenuItemHeight > 0 && font.getHeight() > standardMenuItemHeight / 1.3f)
			font.setHeight(standardMenuItemHeight / 1.3f);

		idealHeight = JUCE_LIVE_CONSTANT_OFF(26);

		idealWidth = font.getStringWidth(text) + idealHeight * 2;
	}
}

void FloatingTile::TilePopupLookAndFeel::drawPopupMenuSectionHeader(Graphics& g, const Rectangle<int>& area, const String& sectionName)
{
	g.fillAll(JUCE_LIVE_CONSTANT_OFF(Colour(0x1AFFFFFF)));

	g.setFont(getPopupMenuFont());
	g.setColour(Colours::white);

	g.drawFittedText(sectionName,
		area.getX() + 12, area.getY(), area.getWidth() - 16, (int)(area.getHeight() * 0.8f),
		Justification::bottomLeft, 1);
}
