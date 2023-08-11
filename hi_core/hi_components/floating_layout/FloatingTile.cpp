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

static constexpr int TitleHeight = 18;

namespace hise { using namespace juce;

juce::Rectangle<int> FloatingTilePopup::getRectangle(RectangleType t) const
{
	static constexpr int BoxMargin = 12;
	static constexpr int ContentMargin = 8;
	static constexpr int PopupTitleHeight = 22;
	static constexpr int CloseButtonWidth = 24;

	auto b = content->getLocalBounds();

	auto th = hasTitle() ? PopupTitleHeight : 0;

	if (t == RectangleType::FullBounds)
		return b.expanded(BoxMargin + ContentMargin, BoxMargin + ContentMargin + th / 2)
				.withPosition(0, 0);
	if (t == RectangleType::ContentBounds)
		return b.withPosition(BoxMargin + ContentMargin, BoxMargin + ContentMargin + th);
	if (t == RectangleType::BoxPath)
		return b.expanded(ContentMargin, ContentMargin + th/2)
				.withPosition(BoxMargin, BoxMargin);
	if (t == RectangleType::CloseButton)
		return getRectangle(RectangleType::FullBounds).removeFromRight(CloseButtonWidth).removeFromTop(CloseButtonWidth);
	if (t == RectangleType::Title)
		return getRectangle(RectangleType::BoxPath).removeFromTop(th);
	
	return {};
}

FloatingTilePopup::FloatingTilePopup(Component* content_, Component* attachedComponent_, Point<int> localPoint) :
	content(content_),
	attachedComponent(attachedComponent_),
	localPointInComponent(localPoint),
	moveButton("move", this, factory)
{
	addAndMakeVisible(moveButton);
	moveButton.setToggleModeWithColourChange(true);

	content->addComponentListener(this);

	setColour(Slider::ColourIds::textBoxOutlineColourId, Colours::white.withAlpha(0.6f));

	if (auto ftc = dynamic_cast<FloatingTile*>(content_))
	{
		auto c = ftc->getCurrentFloatingPanel()->findPanelColour(FloatingTileContent::PanelColourId::bgColour);

		setColour((int)ColourIds::backgroundColourId, c);

		auto c2 = ftc->getCurrentFloatingPanel()->findPanelColour(FloatingTileContent::PanelColourId::itemColour3);

		setColour(Slider::ColourIds::textBoxOutlineColourId, c2);
	}

	addAndMakeVisible(content);
	addAndMakeVisible(closeButton = new CloseButton());


	//content->addComponentListener(this);

	attachedComponent->addComponentListener(this);

	
	closeButton->addListener(this);

	auto b = getRectangle(RectangleType::FullBounds);
	setSize(b.getWidth(), b.getHeight());
}

void FloatingTilePopup::rebuildBoxPath()
{
    boxPath.clear();
    
	auto boxBounds = getRectangle(RectangleType::BoxPath).toFloat();

    if (arrowX > 0 && !moveButton.getToggleState())
    {
		float r = (float)boxBounds.getRight();
		float l = boxBounds.getX();
        float yOff = boxBounds.getY();
        float arc = 5.0f;
		float h = (float)boxBounds.getBottom();

        boxPath.startNewSubPath(arc + boxBounds.getX(), yOff);

        if (!arrowAtBottom)
        {
            boxPath.lineTo((float)arrowX - 12.0f, yOff);
            boxPath.lineTo((float)arrowX, 0.0f);
            boxPath.lineTo((float)arrowX + 12.0f, yOff);
        }

        boxPath.lineTo(r - arc, yOff);
        boxPath.addArc(r - 2.0f*arc, yOff, 2.0f * arc, 2.f * arc, 0.0f, float_Pi / 2.0f);
        boxPath.lineTo(r, h - arc);
        boxPath.addArc(r - 2.0f*arc, h - 2.f*arc, 2.f * arc, 2.f * arc, float_Pi / 2.0f, float_Pi);
        
        if (arrowAtBottom)
        {
            boxPath.lineTo((float)arrowX + 12.0f, h);
            boxPath.lineTo((float)arrowX, (float)getHeight()-1.0f);
            boxPath.lineTo((float)arrowX - 12.0f, h);
        }

        boxPath.lineTo(arc + l, h);
        boxPath.addArc(l, h - 2.f*arc, 2.f * arc, 2.f * arc, float_Pi, float_Pi * 1.5f);
        boxPath.lineTo(l, yOff + arc);
        boxPath.addArc(l, yOff, 2.f * arc, 2.f * arc, float_Pi * 1.5f, float_Pi * 2.f);
        boxPath.closeSubPath();
    }
    else
    {
        boxPath.addRoundedRectangle(boxBounds, 5.0f);
    }


	float scaleFactor = 0.25f;

	auto sf = AffineTransform::scale(scaleFactor);

	auto fb = getRectangle(RectangleType::FullBounds).toFloat();
	auto bb = getRectangle(RectangleType::BoxPath).toFloat();

	fb = fb.transformedBy(sf);
	bb = bb.transformedBy(sf);

	shadowImage = Image(Image::ARGB, fb.getWidth(), fb.getHeight(), true);
	Graphics g2(shadowImage);
	g2.setColour(Colour(JUCE_LIVE_CONSTANT_OFF(Colour(0x32000000))));
	g2.fillRect(bb);
	gin::applyStackBlur(shadowImage, 3);
}



void FloatingTilePopup::paint(Graphics &g)
{
#if USE_BACKEND
    
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xf4242424)));
	g.fillPath(boxPath);
		
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xFF222222)));
	g.strokePath(boxPath, PathStrokeType(1.0f));

	auto copy = boxPath;

	auto bb = copy.getBounds();
	copy.scaleToFit(bb.getX() + 1.0f, bb.getY() + 1.0f, bb.getWidth() - 2.0f, bb.getHeight() - 2.0f, false);

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0x44FFFFFF)));
	g.strokePath(copy, PathStrokeType(0.5f));

	if (hasTitle())
	{
		auto t = getRectangle(RectangleType::Title).toFloat();
		g.setColour(Colours::white.withAlpha(0.05f));
		g.fillRoundedRectangle(t, 3.0f);

		g.setColour(Colours::white);
		g.setFont(GLOBAL_BOLD_FONT());
		g.drawText(content->getName(), t, Justification::centred, false);
	}
#endif
}

bool FloatingTilePopup::hasTitle() const
{
	return content != nullptr && content->getName().containsNonWhitespaceChars();
}

bool FloatingTilePopup::keyPressed(const KeyPress& k)
{
	if(k == KeyPress::escapeKey)
	{
		deleteAndClose();
		return true;
	}
        
	return false;
}

void FloatingTilePopup::resized()
{
    closeButton->setBounds(getRectangle(RectangleType::CloseButton));

	auto tb = getRectangle(RectangleType::Title);
	moveButton.setBounds(tb.removeFromLeft(tb.getHeight()).reduced(2));

	rebuildBoxPath();

	content->setBounds(getRectangle(RectangleType::ContentBounds));
}

void FloatingTilePopup::deleteAndClose()
{
	if (attachedComponent.getComponent())
		attachedComponent->removeComponentListener(this);

	attachedComponent = nullptr;
	updatePosition();
}

void FloatingTilePopup::componentMovedOrResized(Component& component, bool /*moved*/, bool wasResized)
{
	if (&component == attachedComponent.getComponent())
	{
		if (!moveButton.getToggleState())
			updatePosition();
	}
	else
	{
		if (wasResized)
		{
			auto newBounds = getRectangle(RectangleType::FullBounds);

			setSize(newBounds.getWidth(), newBounds.getHeight());

			if (moveButton.getToggleState())
			{
				constrainer.setMinimumOnscreenAmounts(newBounds.getHeight(), newBounds.getWidth(), newBounds.getHeight(), newBounds.getWidth());
				constrainer.checkComponentBounds(this);
			}
			else
			{
				updatePosition();
			}
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

void FloatingTilePopup::buttonClicked(Button* b)
{
	if (b == &moveButton)
	{
		rebuildBoxPath();
		repaint();

		if(!skipToggle)
			findParentComponentOfClass<FloatingTile>()->toggleDetachPopup(this);

		if (!moveButton.getToggleState())
		{
			setMouseCursor(MouseCursor::NormalCursor);
			updatePosition();

			if (attachedComponent != nullptr)
				attachedComponent->addComponentListener(this);
            
            if(onDetach)
                onDetach(false);
		}
		else
		{
            setMouseCursor(MouseCursor::DraggingHandCursor);
			
			if (attachedComponent != nullptr)
				attachedComponent->removeComponentListener(this);
            
            if(onDetach)
                onDetach(true);
		}
	}
	if (b == closeButton.get())
	{
		deleteAndClose();
	}
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

Identifier FloatingTile::LayoutData::getDefaultablePropertyId(int index) const
{
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::ID, "ID");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::Size, "Size");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::Folded, "Folded");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::ForceFoldButton, "ForceFoldButton");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::Visible, "Visible");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::MinSize, "MinSize");
    RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::ForceShowTitle, "ForceShowTitle");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::FocusKeyPress, "FocusKeyPress");
	RETURN_DEFAULT_PROPERTY_ID(index, LayoutDataIds::FoldKeyPress, "FoldKeyPress");

	jassertfalse;

	return Identifier();
}

var FloatingTile::LayoutData::getDefaultProperty(int id) const
{
	switch (id)
	{
	case FloatingTile::LayoutData::LayoutData::ID:		  return var("anonymous");
	case FloatingTile::LayoutData::LayoutDataIds::Size:   return var(-0.5);
	case FloatingTile::LayoutData::LayoutDataIds::Folded: return var(0);
	case FloatingTile::LayoutData::LayoutDataIds::ForceFoldButton: return var(0);
	case FloatingTile::LayoutData::LayoutDataIds::Visible: return var(true);
	case FloatingTile::LayoutData::LayoutDataIds::MinSize: return var(-1);
	case FloatingTile::LayoutData::LayoutDataIds::ForceShowTitle: return var(0);
	case FloatingTile::LayoutData::LayoutDataIds::FocusKeyPress: return var("");
	case FloatingTile::LayoutData::LayoutDataIds::FoldKeyPress: return var("");
	default:
		break;
	}

	jassertfalse;
	return var();
}

void FloatingTile::LayoutData::reset()
{
	layoutDataObject = var(new DynamicObject());

	resetObject(layoutDataObject.getDynamicObject());

	cachedValues = CachedValues();

	swappingEnabled = false;
}

bool FloatingTile::LayoutData::isAbsolute() const
{ 
	double currentSize = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Size);
	return currentSize > 0.0; 
}

bool FloatingTile::LayoutData::isFolded() const
{
	int foldState = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Folded);
	return foldState > 0; 
}

bool FloatingTile::LayoutData::canBeFolded() const
{
	int foldState = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Folded);
	return foldState >= 0; 
}

int FloatingTile::LayoutData::getForceTitleState() const
{
	return getPropertyWithDefault(layoutDataObject, LayoutDataIds::ForceShowTitle);
}

void FloatingTile::LayoutData::setFoldState(int newFoldState)
{
	storePropertyInObject(layoutDataObject, LayoutDataIds::Folded, newFoldState);
	cachedValues.folded = newFoldState;
}

void FloatingTile::LayoutData::setKeyPress(bool isFocus, const Identifier& shortcutId)
{
	String s;
	s << "$" << shortcutId.toString();
			
	storePropertyInObject(layoutDataObject, isFocus ? LayoutDataIds::FocusKeyPress : LayoutDataIds::FoldKeyPress, s);
}

KeyPress FloatingTile::LayoutData::getFoldKeyPress(Component* c) const
{
	auto s = getPropertyWithDefault(layoutDataObject, LayoutDataIds::FoldKeyPress).toString();
	return TopLevelWindowWithKeyMappings::getKeyPressFromString(c, s);
}

KeyPress FloatingTile::LayoutData::getFocusKeyPress(Component* c) const
{
	auto s = getPropertyWithDefault(layoutDataObject, LayoutDataIds::FocusKeyPress).toString();
	return TopLevelWindowWithKeyMappings::getKeyPressFromString(c, s);
}

double FloatingTile::LayoutData::getCurrentSize() const
{
	double currentSize = getPropertyWithDefault(layoutDataObject, LayoutDataIds::Size);

	return currentSize;
}

void FloatingTile::LayoutData::setForceShowTitle(int shouldForceTitle)
{
	storePropertyInObject(layoutDataObject, LayoutDataIds::ForceShowTitle, shouldForceTitle);
	cachedValues.forceShowTitle = shouldForceTitle;
}

void FloatingTile::LayoutData::setCurrentSize(double newSize)
{
	storePropertyInObject(layoutDataObject, LayoutDataIds::Size, newSize);
	cachedValues.size = newSize;
}

void FloatingTile::LayoutData::setMinSize(int minSize)
{
	storePropertyInObject(layoutDataObject, LayoutDataIds::MinSize, minSize);
	cachedValues.minSize = minSize;
}

int FloatingTile::LayoutData::getMinSize() const
{
	int minSize = getPropertyWithDefault(layoutDataObject, LayoutDataIds::MinSize);
	return minSize;
}

var FloatingTile::LayoutData::getLayoutDataObject() const
{
	return layoutDataObject;
}

Identifier FloatingTile::LayoutData::getID() const
{
	String id = getPropertyWithDefault(layoutDataObject, LayoutDataIds::ID);

	if (id.isNotEmpty())
		return Identifier(id);

	static const Identifier an("anonymous");

	return an;
}

void FloatingTile::LayoutData::setId(const String& id)
{
	storePropertyInObject(layoutDataObject, LayoutDataIds::ID, id);
	cachedValues.id = id;
}

void FloatingTile::LayoutData::setVisible(bool shouldBeVisible)
{
	storePropertyInObject(layoutDataObject, LayoutDataIds::Visible, shouldBeVisible);
	cachedValues.visible = shouldBeVisible;
}

bool FloatingTile::LayoutData::isVisible() const
{
	return getPropertyWithDefault(layoutDataObject, LayoutDataIds::Visible);
}

void FloatingTile::LayoutData::setForceFoldButton(bool shouldBeShown)
{
	storePropertyInObject(layoutDataObject, LayoutDataIds::ForceFoldButton, shouldBeShown);
	cachedValues.forceFoldButton = shouldBeShown;
}

bool FloatingTile::LayoutData::mustShowFoldButton() const
{
	return getPropertyWithDefault(layoutDataObject, LayoutDataIds::ForceFoldButton);
}

FloatingTile::CloseButton::CloseButton() :
	ShapeButton("Close", Colours::white.withAlpha(0.2f), Colours::white.withAlpha(0.8f), Colours::white)
{
	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::closeIcon));
	setShape(p, false, true, true);

	addListener(this);

}

void FloatingTile::CloseButton::mouseEnter(const MouseEvent& m)
{
	auto ft = dynamic_cast<FloatingTile*>(getParentComponent());
            
	ft->deleteHover = true;
	ft->repaint();
            
	ShapeButton::mouseEnter(m);
}

void FloatingTile::CloseButton::mouseExit(const MouseEvent& m)
{
	auto ft = dynamic_cast<FloatingTile*>(getParentComponent());
            
	ft->deleteHover = false;
	ft->repaint();
            
	ShapeButton::mouseExit(m);
}

void FloatingTile::CloseButton::buttonClicked(Button* )
{
	auto ec = dynamic_cast<FloatingTile*>(getParentComponent());

	if (ec->closeTogglesVisibility)
	{
		ec->getLayoutData().setVisible(!ec->getLayoutData().isVisible());
		ec->getParentContainer()->refreshLayout();
		ec->getParentContainer()->notifySiblingChange();
	}
	else
	{
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
		ec->getRootFloatingTile()->enableSwapMode(!ec->layoutData.swappingEnabled, ec);
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
	setWantsKeyboardFocus(false);
	addListener(this);
}

void FloatingTile::FoldButton::buttonClicked(Button* )
{
	auto pc = findParentComponentOfClass<FloatingTile>();

	pc->toggleFold();
}

FloatingTile::ParentType FloatingTile::getParentType() const
{
	if (getParentContainer() == nullptr)
		return ParentType::Root;
	else if (auto sl = dynamic_cast<const ResizableFloatingTileContainer*>(getParentContainer()))
	{
		return sl->isVertical() ? ParentType::Vertical : ParentType::Horizontal;
	}
	else if (dynamic_cast<const FloatingTabComponent*>(getParentContainer()) != nullptr)
	{
		return ParentType::Tabbed;
	}

	jassertfalse;

	return ParentType::numParentTypes;
}




FloatingTile::FloatingTile(MainController* mc_, FloatingTileContainer* parent, var data) :
	Component("Empty"),
	mc(mc_),
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

FloatingTile::~FloatingTile()
{
	currentPopup = nullptr;

	content = nullptr;
	foldButton = nullptr;
	moveButton = nullptr;
	resizeButton = nullptr;
	closeButton = nullptr;
}

void FloatingTile::forEachDetachedPopup(const std::function<void(FloatingTilePopup* p)>& f)
{
	if (getParentType() != ParentType::Root)
		getRootFloatingTile()->forEachDetachedPopup(f);

	for (auto p : detachedPopups)
		f(p);
}

bool FloatingTile::isEmpty() const
{
	return dynamic_cast<const EmptyComponent*>(getCurrentFloatingPanel()) != nullptr;
}

void FloatingTile::ensureVisibility()
{
    auto p = this;
    
    while(p != nullptr)
    {
		if (GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Other::AutoShowWorkspace))
		{
			p->setFolded(false);
		}
        
		p->getLayoutData().setVisible(true);
        
        auto c = p->getParentContainer();
        
        if(auto tab = dynamic_cast<FloatingTabComponent*>(c))
        {
            tab->setDisplayedFloatingTile(p);
        }
        
        if(c == nullptr)
            break;
        
        p = c->getParentShell();
    }
    
    refreshRootLayout();
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

    auto forceShow = getLayoutData().getForceTitleState();
    
	if (forceShow != 0)
	{
		return forceShow == 2;
	}

	auto pt = getParentType();

	const bool isRoot = pt == ParentType::Root;
	const bool isInTab = pt == ParentType::Tabbed;
	const bool isDynamicContainer = dynamic_cast<const FloatingTileContainer*>(getCurrentFloatingPanel()) != nullptr && dynamic_cast<const FloatingTileContainer*>(getCurrentFloatingPanel())->isDynamic();

	if (isRoot && !isDynamicContainer)
		return false;

	if (isInTab && !isLayoutModeEnabled())
		return false;

	if (getLayoutData().mustShowFoldButton() && !isFolded())
		return false;

	if (isFolded())
		return isInVerticalLayout();

	if (hasChildren())
	{
		

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
		if (getCurrentFloatingPanel() == nullptr)
			return true;

		return getCurrentFloatingPanel()->showTitleInPresentationMode();
	}
}

Rectangle<int> FloatingTile::getContentBounds()
{
	if (isFolded())
		return Rectangle<int>();

	if (showTitle())
		return Rectangle<int>(0, TitleHeight, getWidth(), getHeight() - TitleHeight);
	else
		return getLocalBounds();
}

void FloatingTile::setFolded(bool shouldBeFolded)
{
	if (!canBeFolded())
		return;

	layoutData.setFoldState(shouldBeFolded ? 1 : 0);

	refreshFoldButton();
}

void FloatingTile::refreshFoldButton()
{
	Path p;
	p.loadPathFromData(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon, SIZE_OF_PATH(HiBinaryData::ProcessorEditorHeaderIcons::foldedIcon));

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

const BackendRootWindow* FloatingTile::getBackendRootWindow() const
{
	auto rw = getRootFloatingTile()->findParentComponentOfClass<ComponentWithBackendConnection>()->getBackendRootWindow();
	
	//auto rw = dynamic_cast<ComponentWithBackendConnection*>(getRootFloatingTile()->getParentComponent())->getBackendRootWindow();

	jassert(rw != nullptr);

	return rw;
}

BackendRootWindow* FloatingTile::getBackendRootWindow()
{
	auto root = getRootFloatingTile();
	jassert(root != nullptr);

	auto cbc = root->findParentComponentOfClass<ComponentWithBackendConnection>();
	jassert(cbc != nullptr);

	auto rw = cbc->getBackendRootWindow();
	jassert(rw != nullptr);

	return rw;
}

FloatingTile* FloatingTile::getRootFloatingTile()
{
	if (getParentType() == ParentType::Root)
		return this;

	auto parent = getParentContainer()->getParentShell(); //findParentComponentOfClass<FloatingTile>();

	if (parent == nullptr)
		return nullptr;

	return parent->getRootFloatingTile();
}

const FloatingTile* FloatingTile::getRootFloatingTile() const
{
	return const_cast<FloatingTile*>(this)->getRootFloatingTile();
}

void FloatingTile::clear()
{
	layoutData.reset();
	refreshPinButton();
	refreshFoldButton();
	refreshMouseClickTarget();
	refreshRootLayout();

	if (getParentContainer())
	{
		getParentContainer()->notifySiblingChange();
		getParentContainer()->refreshLayout();
	}

	resized();
}

void FloatingTile::refreshRootLayout()
{
	if (getRootFloatingTile() != nullptr)
	{
		auto rootContainer = dynamic_cast<FloatingTileContainer*>(getRootFloatingTile()->getCurrentFloatingPanel());

		if(rootContainer != nullptr)
			rootContainer->refreshLayout();
	}
}

void FloatingTile::setLayoutModeEnabled(bool shouldBeEnabled)
{
	if (getParentType() == ParentType::Root)
	{
		layoutModeEnabled = shouldBeEnabled;

		resized();
		repaint();
		refreshMouseClickTarget();

		if (hasChildren())
		{
			dynamic_cast<FloatingTileContainer*>(getCurrentFloatingPanel())->refreshLayout();
		}

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

	return canDoLayoutMode() && getRootFloatingTile()->isLayoutModeEnabled();
}

bool FloatingTile::canDoLayoutMode() const
{
	if (getParentType() == ParentType::Root)
		return true;

	if (isVital())
		return false;

	return getParentContainer()->isDynamic();
}


void FloatingTile::setAllowChildComponentCreation(bool shouldCreateChildComponents)
{
	jassert(getParentType() == ParentType::Root);

	allowChildComponentCreation = shouldCreateChildComponents;
}


bool FloatingTile::shouldCreateChildComponents() const
{
	if (getParentType() == ParentType::Root)
	{
		return allowChildComponentCreation;
	}
	else
	{
		return getRootFloatingTile()->shouldCreateChildComponents();
	}
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
	if (getCurrentFloatingPanel())
	{
		closeButton->setTooltip("Delete " + getCurrentFloatingPanel()->getBestTitle());
		resizeButton->setTooltip("Toggle absolute size for " + getCurrentFloatingPanel()->getBestTitle());
	}

	moveButton->toFront(false);
	foldButton->toFront(false);
	closeButton->toFront(false);
	resizeButton->toFront(false);
}

void FloatingTile::paint(Graphics& g)
{
	if (isOpaque())
	{
		if (findParentComponentOfClass<ScriptContentComponent>() || findParentComponentOfClass<FloatingTilePopup>())
		{
			auto c = getCurrentFloatingPanel()->findPanelColour(FloatingTileContent::PanelColourId::bgColour);

			if (!c.isOpaque())
				c = Colour(0xFF222222);

			g.fillAll(c);
		}
		else
		{
			if (getParentType() == ParentType::Root)
			{
				g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::EditorBackgroundColourIdBright));
			}
			else
			{
				g.fillAll(HiseColourScheme::getColour(HiseColourScheme::ColourIds::ModulatorSynthBackgroundColourId));
			}
		}
	}

	

    if(getLayoutData().isFolded() && getParentType() == ParentType::Horizontal)
    {
        if(isMouseOver(true))
        {
            g.setColour(Colours::white.withAlpha(0.03f));
            auto b = getLocalBounds();
            g.fillRect(b);
        }
        
        g.setColour(Colours::white.withAlpha(0.2f));
        
        Path p = getIcon();
        p.scaleToFit(1.0f, 19.0f, 14.0f, 14.0f, true);
        g.fillPath(p);
    }
    

	if (showTitle())
	{
		g.setGradientFill(ColourGradient(Colour(0xFF222222), 0.0f, 0.0f,
			Colour(0xFF151515), 0.0f, 16.0f, false));

        auto area = getLocalBounds().removeFromTop(TitleHeight).toFloat();
        
		g.fillRect(area);

        g.setColour(Colours::white.withAlpha(0.03f));
        g.drawRect(area.reduced(2.0f), 1.0f);
        
		Rectangle<int> titleArea = Rectangle<int>(leftOffsetForTitleText, 0, rightOffsetForTitleText - leftOffsetForTitleText, TitleHeight);

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
		

    if(deleteHover)
    {
        g.fillAll(Colours::red.withAlpha(0.1f));
        g.setColour(Colours::red.withAlpha(0.3f));
        g.drawRect(getLocalBounds(), 1);
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
	if(isLayoutModeEnabled() || isFolded())
		repaint();
}

void FloatingTile::mouseExit(const MouseEvent& )
{
    if (isLayoutModeEnabled() || isFolded())
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
	else if (layoutData.swappingEnabled && isSwappable())
    {
        currentSwapSource->swapWith(this);

        getRootFloatingTile()->enableSwapMode(false, nullptr);
    }
}

void FloatingTile::setOverlayComponent(Component* newOverlayComponent, int fadeTime)
{
	if (overlayComponent != nullptr)
	{
		if (fadeTime != 0)
			Desktop::getInstance().getAnimator().fadeOut(overlayComponent, fadeTime);
	}

	if (newOverlayComponent != nullptr)
	{
		addAndMakeVisible(overlayComponent = newOverlayComponent);

		overlayComponent->setBounds(getContentBounds());

		if (fadeTime != 0)
			Desktop::getInstance().getAnimator().fadeIn(overlayComponent, fadeTime);
	}
}

void FloatingTile::resized()
{
	if (content.get() == nullptr)
		return;

	setWantsKeyboardFocus(getRootFloatingTile() == this);
	
	LayoutHelpers::setContentBounds(this);

	if (overlayComponent != nullptr)
		overlayComponent->setBounds(getContentBounds());

	if (LayoutHelpers::showFoldButton(this))
	{
		leftOffsetForTitleText = 16;
        
        foldButton->setBounds(getLocalBounds().removeFromTop(TitleHeight + 2));
        
        auto openBorders = BorderSize<int>(1, 1, jmin(getHeight() - TitleHeight, 4), getWidth() - (TitleHeight));
        
        if(dynamic_cast<VerticalTile*>(getParentContainer()) && getLayoutData().isFolded())
        {
            foldButton->setBounds(getLocalBounds().removeFromLeft(TitleHeight + 2));
            openBorders = BorderSize<int>(1, 1, getHeight() - TitleHeight, 1);
        }

		if (dynamic_cast<FloatingTabComponent*>(getCurrentFloatingPanel()))
		{
            if(getLayoutData().isFolded())
            {
                foldButton->setBounds(getLocalBounds().removeFromLeft(TitleHeight + 2));
                openBorders = BorderSize<int>(1, 1, getHeight() - TitleHeight, 1);
            }
            else
            {
                foldButton->setBounds(getLocalBounds().removeFromLeft(TitleHeight + 2).removeFromTop(TitleHeight + 2));
                openBorders = BorderSize<int>(2);
            }
		}
        
        foldButton->setBorderSize(openBorders);
        
		//foldButton->setBounds(1, 1, 14, 14);
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

bool FloatingTile::keyPressed(const KeyPress& key)
{
	if (getRootFloatingTile() != this)
		return false;

	return forEach<FloatingTileContent>([key](FloatingTileContent* c)
	{
		if (!c->getParentShell()->isShowing())
			return false;

		if (auto t = dynamic_cast<FloatingTabComponent*>(c))
		{
            auto ck = t->getCycleKeyPress();
            
            if(ck.isValid())
            {
                auto cycleKey = TopLevelWindowWithKeyMappings::getFirstKeyPress(t, ck);
                
                if (cycleKey == key)
                {
                    int numTabs = t->getNumTabs();
                    auto newIndex = (t->getCurrentTabIndex() + 1) % numTabs;
                    t->setCurrentTabIndex(newIndex);
                    dynamic_cast<Component*>(t)->grabKeyboardFocusAsync();
                    return true;
                }
            }
		};


		auto& ld = c->getParentShell()->getLayoutData();
		auto k = ld.getFoldKeyPress(c->getParentShell());
		auto fk = ld.getFocusKeyPress(c->getParentShell());



		if (fk.isValid() && fk == key)
		{
			dynamic_cast<Component*>(c)->grabKeyboardFocusAsync();
			return true;
		}

		if (k.isValid() && k == key)
		{
			if (auto displayedTile = c->getParentShell()->toggleFold())
				displayedTile->grabKeyboardFocusAsync();
			
			return true;
		}

		return false;
	});
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

bool FloatingTile::isRootPopupShown() const
{
	if (getParentType() != ParentType::Root)
	{
		return getRootFloatingTile()->isRootPopupShown();
	}

	return currentPopup != nullptr;
}

struct ResizableViewport: public Component,
                          public PathFactory,
                          public ButtonListener,
                          public ComponentListener
{
    
    constexpr static int EdgeHeight = 18;
    
    ResizableViewport(int maxHeight_, bool shouldBeMaximised):
     maxHeight(maxHeight_),
     maximiseButton("maximise", this, *this),
     edge(this, nullptr, ResizableEdgeComponent::bottomEdge)
    {
        maximiseButton.setToggleModeWithColourChange(true);
		maximiseButton.setToggleStateAndUpdateIcon(shouldBeMaximised, true);

        slaf.bg = Colours::transparentBlack;
        
        addAndMakeVisible(maximiseButton);
        addAndMakeVisible(vp);
        addAndMakeVisible(edge);
        edge.setLookAndFeel(&slaf);
    }
    
    void componentMovedOrResized (Component& component,
                                  bool wasMoved,
                                  bool wasResized) override
    {
        if(wasResized && maximiseButton.getToggleState())
        {
            maximise();
        }
    }
    
	void addFixComponent(Component* ownedC)
	{
		jassert(ownedC->getHeight() != 0);
		addAndMakeVisible(fixComponent = ownedC);
		
		maxHeight -= fixComponent->getHeight();

		if (maximiseButton.getToggleState())
			maximise();
	}

    void resized() override
    {
        auto b = getLocalBounds();

		if (fixComponent != nullptr)
			fixComponent->setBounds(b.removeFromTop(fixComponent->getHeight()));

        b.removeFromLeft(vp.getScrollBarThickness());
        
        auto bot = b.removeFromBottom(EdgeHeight);
        
        maximiseButton.setBounds(bot.removeFromRight(bot.getHeight()).reduced(1));
        edge.setBounds(bot.reduced(5));
        vp.setBounds(b);
    }
    
    void maximise()
    {
		auto contentHeight = vp.getViewedComponent()->getHeight() + EdgeHeight;

        auto rootBounds = getTopLevelComponent()->getLocalBounds().reduced(100);
        
		if (fixComponent != nullptr)
			contentHeight += fixComponent->getHeight();

        
        
        auto maxHeightToUse = jmin(maxHeight - 80, contentHeight + EdgeHeight);
		auto contentWidth = vp.getViewedComponent()->getWidth() + EdgeHeight;
		auto maxWidthToUse = jmin(1800 - 80, contentWidth + EdgeHeight);

        maxWidthToUse = jmin(rootBounds.getWidth(), maxWidthToUse);
        maxHeightToUse = jmin(rootBounds.getHeight(), maxHeightToUse);
        
        setSize(maxWidthToUse, maxHeightToUse);
		setName(vp.getViewedComponent()->getName());
		
		if (auto pc = findParentComponentOfClass<FloatingTilePopup>())
		{
			pc->rebuildBoxPath();
			pc->repaint();
            pc->resized();
		}
		
        edge.setVisible(false);
    }
    
    void buttonClicked(Button* b) override
    {
        if(b->getToggleState())
        {
            defaultHeight = getHeight();
            maximise();
        }
        else
        {
            setSize(getWidth(), defaultHeight);
            edge.setVisible(true);
        }
    }
    
    Path createPath(const String& url) const override
    {
        Path p;
        p.startNewSubPath(0.0f, 0.0f);
        p.lineTo(1.0f, 1.0f);
        p.lineTo(0.25f, 1.0f);
        p.lineTo(0.25f, 2.0f);
        p.lineTo(1.0f, 2.0f);
        p.lineTo(0.0f, 3.0f);
        p.lineTo(-1.0f, 2.0f);
        p.lineTo(-0.25f, 2.0f);
        p.lineTo(-0.25f, 1.0f);
        p.lineTo(-1.0f, 1.0f);
        p.closeSubPath();
        return p;
    }
    
    void setComponent(Component* c)
    {
		setName(c->getName());

        vp.setViewedComponent(c, true);
        defaultHeight = jmin(c->getHeight(), maxHeight * 3 / 4);
        c->addComponentListener(this);
        vp.getVerticalScrollBar().setLookAndFeel(&slaf);
        vp.setScrollBarThickness(12);
        setSize(c->getWidth() + vp.getScrollBarThickness() * 2, defaultHeight + EdgeHeight);
        
		if (maximiseButton.getToggleState())
			maximise();
    }
    
	ScopedPointer<Component> fixComponent;
    ResizableEdgeComponent edge;
    Viewport vp;
    ZoomableViewport::Laf slaf;
    
    HiseShapeButton maximiseButton;
    
    int maxHeight;
    int defaultHeight;
};

bool hasResizer(Component* c)
{
	if (dynamic_cast<juce::ResizableCornerComponent*>(c) != nullptr)
		return true;

	for (int i = 0; i < c->getNumChildComponents(); i++)
	{
		if(hasResizer(c->getChildComponent(i)))
			return true;
	}

	return false;
}

Component* FloatingTile::wrapInViewport(Component* c, bool shouldBeMaximised)
{
	if (hasResizer(c))
		return c;

	auto maxHeight = getTopLevelComponent()->getHeight();
	auto vp = new ResizableViewport(maxHeight, shouldBeMaximised);
	vp->setComponent(c);
	return vp;
}

FloatingTilePopup* FloatingTile::showComponentInRootPopup(Component* newComponent, Component* attachedComponent, Point<int> localPoint, bool shouldWrapInViewport, bool maximiseViewport)
{
    if(newComponent != nullptr && shouldWrapInViewport)
		newComponent = wrapInViewport(newComponent, maximiseViewport);    
    
    if(attachedComponent != nullptr)
    {
        if(auto f = attachedComponent->findParentComponentOfClass<FloatingTilePopup>())
        {
            auto r = attachedComponent->getTopLevelComponent();
            localPoint = r->getLocalPoint(attachedComponent, localPoint);
            CallOutBox::launchAsynchronously(std::unique_ptr<Component>(newComponent), {localPoint, localPoint}, r);
            return f;
        }
    }
    
	if (getParentType() != ParentType::Root)
	{
		return getRootFloatingTile()->showComponentInRootPopup(newComponent, attachedComponent, localPoint);
	}
	else
	{
		if (newComponent != nullptr)
		{
			if (currentPopup != nullptr)
				Desktop::getInstance().getAnimator().fadeOut(currentPopup, 150);

			addAndMakeVisible(currentPopup = new FloatingTilePopup(newComponent, attachedComponent, localPoint));

			currentPopup->updatePosition();
			currentPopup->setVisible(false);
			Desktop::getInstance().getAnimator().fadeIn(currentPopup, 150);
			currentPopup->grabKeyboardFocusAsync();
		}
		else
		{
			if(currentPopup != nullptr)
				Desktop::getInstance().getAnimator().fadeOut(currentPopup, 150);

			currentPopup = nullptr;
		}

		for (int i = 0; i < listeners.size(); i++)
		{
			if (listeners[i].get() != nullptr)
				listeners[i]->popupChanged(newComponent);
			else
				listeners.remove(i--);
		}

		return currentPopup;
	}
}




hise::FloatingTilePopup* FloatingTile::showComponentAsDetachedPopup(Component* newComponent, Component* attachedComponent, Point<int> localPoint, bool shouldWrapInViewport /*= false*/)
{
	jassert(newComponent != nullptr);
	jassert(getParentType() == ParentType::Root);

	if (shouldWrapInViewport)
		newComponent = wrapInViewport(newComponent, true);

	auto newPopup = new FloatingTilePopup(newComponent, attachedComponent, localPoint);

	addAndMakeVisible(newPopup);

	detachedPopups.add(newPopup);

	newPopup->updatePosition();
	newPopup->skipToggle = true;
	newPopup->moveButton.triggerClick(sendNotificationSync);
	newPopup->skipToggle = false;
	newPopup->rebuildBoxPath();
	newPopup->grabKeyboardFocusAsync();
	newPopup->moveButton.setVisible(false);

	return newPopup;
}

void FloatingTilePopup::addFixComponent(Component* c)
{
	if (auto vp = dynamic_cast<ResizableViewport*>(content.get()))
	{
		vp->addFixComponent(c);
		updatePosition();
	}
}

Component* FloatingTilePopup::getTrueContent()
{
	if (auto vp = dynamic_cast<ResizableViewport*>(content.get()))
	{
		return vp->vp.getViewedComponent();
	}

	return content.get();
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

		int distanceToBottom = root->getHeight() - pointInRoot.getY();
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
		root->removePopup(this);
	}
}

void FloatingTilePopup::mouseDrag(const MouseEvent& e)
{
	if (!moveButton.getToggleState())
		return;

	if (e.mouseWasDraggedSinceMouseDown())
	{
		if (!dragging)
		{
			dragger.startDraggingComponent(this, e);
			dragging = true;
		}
		else
			dragger.dragComponent(this, e, &constrainer);
	}
}

void FloatingTilePopup::mouseUp(const MouseEvent& e)
{
	dragging = false;
}

void FloatingTilePopup::mouseDown(const MouseEvent& e)
{
	if (moveButton.getToggleState())
		toFront(true);
}

bool FloatingTile::canBeDeleted() const
{
#if USE_BACKEND
	const bool isInPopout = findParentComponentOfClass<FloatingTileDocumentWindow>() != nullptr;
#else
    const bool isInPopout = false;
#endif

	if (isVital())
		return false;

	if (getParentType() == ParentType::Root)
		return isInPopout;

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



String FloatingTile::exportAsJSON() const
{
	var obj = getCurrentFloatingPanel()->toDynamicObject();

	auto json = JSON::toString(obj, false, DOUBLE_TO_STRING_DIGITS);

	return json;
}


void FloatingTile::loadFromJSON(const String& jsonData)
{
	var obj;

	auto result = JSON::parse(jsonData, obj);

	if (result.wasOk())
		setContent(obj);
}


void FloatingTile::swapContainerType(const Identifier& containerId)
{
	var v = getCurrentFloatingPanel()->toDynamicObject();



	v.getDynamicObject()->setProperty("Type", containerId.toString());

	if (auto list = v.getDynamicObject()->getProperty("Content").getArray())
	{
		for (int i = 0; i < list->size() / 2; i++)
		{
			list->swap(i, list->size() - 1 - i);
		}

		for (int i = 0; i < list->size(); i++)
		{
			var c = list->getUnchecked(i);

			var layoutDataObj = c.getDynamicObject()->getProperty("LayoutData");

			layoutDataObj.getDynamicObject()->setProperty("Size", -0.5);
		}
	}

	setContent(v);
}


Path FloatingTile::getIcon() const
{
	if (iconId != -1)
	{
		return getPanelFactory()->getPath((FloatingTileContent::Factory::PopupMenuOptions)iconId);
	}

	if (hasChildren())
	{
		auto firstPanel = dynamic_cast<const FloatingTileContainer*>(getCurrentFloatingPanel())->getComponent(0);

		if (firstPanel != nullptr)
		{
			return firstPanel->getIcon();
		}
	}

	auto index = getPanelFactory()->getOption(this);
	return getPanelFactory()->getPath(index);
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
		getCurrentFloatingPanel()->fromDynamicObject(data);
	}

	refreshFixedSizeForNewContent();

	refreshFoldButton();
	refreshPinButton();

	if (getParentContainer())
	{
		getParentContainer()->notifySiblingChange();
		getParentContainer()->refreshLayout();
	}
		
	bringButtonsToFront();
	refreshMouseClickTarget();
	resized();
	repaint();
}


void FloatingTile::setContent(NamedValueSet&& data)
{
	auto d = new DynamicObject();
	var newData(d);

	d->swapProperties(std::move(data));
	
	setContent(newData);
}

void FloatingTile::setNewContent(const Identifier& newId)
{
	auto newObject = dynamic_cast<Component*>(FloatingTileContent::createNewPanel(newId, this));
	setNewContent(newObject);
}

void FloatingTile::setNewContent(Component* newContent)
{
	jassert(dynamic_cast<FloatingTileContent*>(newContent) != nullptr);

	addAndMakeVisible(content = newContent);

	refreshFixedSizeForNewContent();

	if (hasChildren())
		setCanBeFolded(false);

	if (getParentContainer())
	{
		getParentContainer()->notifySiblingChange();
		getParentContainer()->refreshLayout();
	}

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

hise::FloatingTile* FloatingTile::toggleFold()
{
	auto pc = this;

	if (pc->getParentContainer()->getNumVisibleComponents() == 1)
	{
		pc = pc->getParentContainer()->getParentShell();

		while (pc != nullptr && !pc->canBeFolded())
		{
			auto c = pc->getParentContainer();

			if (c == nullptr)
				return nullptr;

			pc = c->getParentShell();
		}

		if (pc == nullptr)
			return nullptr;
	}

	if (!pc->canBeFolded())
		return nullptr;

	pc->setFolded(!pc->isFolded());

	if (auto cl = dynamic_cast<ResizableFloatingTileContainer*>(pc->getParentContainer()))
	{
		auto numVisible = pc->getParentContainer()->getNumVisibleAndResizableComponents();
		auto aboutToClose = pc->isFolded();
		auto isAbsolute = pc->getLayoutData().isAbsolute();

		auto shouldOpenOther = aboutToClose && numVisible == 0 && !isAbsolute;

		if (shouldOpenOther)
		{
			for (int i = 0; i < cl->getNumComponents(); i++)
			{
				auto c = cl->getComponent(i);

				if (c == pc)
					continue;

				auto& l = c->getLayoutData();

				if (l.isAbsolute())
					continue;

				if (l.isFolded())
				{
					c->setFolded(false);
					pc = c;
					break;
				}
			}
		}

		cl->enableAnimationForNextLayout();
		cl->refreshLayout();
	}

	return pc;
}

void FloatingTile::setCloseTogglesVisibility(bool shouldToggleVisibility)
{
	closeTogglesVisibility = shouldToggleVisibility;
}

void FloatingTile::setForceShowTitle(bool shouldShowTitle)
{
	getLayoutData().setForceShowTitle(shouldShowTitle ? 2 : 1);
}

void FloatingTile::addPopupListener(PopupListener* listener)
{
	jassert(getParentType() == ParentType::Root);

	listeners.addIfNotAlreadyThere(listener);
}

void FloatingTile::removePopupListener(PopupListener* listener)
{
	jassert(getParentType() == ParentType::Root);

	listeners.removeAllInstancesOf(listener);
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
	auto scaleFactor = t->content->getTransform().getScaleFactor();

	const int width = (int)((float)t->getWidth() / scaleFactor);
	const int height = (int)((float)t->getHeight() / scaleFactor);

	t->content->setVisible(!t->isFolded());
	t->content->setBounds(0, 0, width, height);
	t->content->resized();
}

bool FloatingTile::LayoutHelpers::showCloseButton(const FloatingTile* t)
{
	auto pt = t->getParentType();

	if (t->closeTogglesVisibility)
		return true;

	if (t->hasChildren() && !t->isLayoutModeEnabled())
		return false;

	if (pt != ParentType::Root && t->isEmpty() && t->getParentContainer()->getNumComponents() == 1)
		return false;

	if (!t->canBeDeleted())
		return false;

	switch (pt)
	{
	case ParentType::Root:
		return !t->isEmpty();
	case ParentType::Horizontal:
		return !t->isFolded() && t->canBeDeleted();
	case ParentType::Vertical:
		return t->canBeDeleted();
	case ParentType::Tabbed:
		return false;
    case ParentType::numParentTypes:
        return false;
	}

	return true;

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
    
	if (t->getLayoutData().mustShowFoldButton())
		return true;

    if(t->getLayoutData().getForceTitleState() == 1 &&
       !t->isFolded())
        return false;
    
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

void FloatingTile::removePopup(FloatingTilePopup* p)
{
	if (currentPopup == p)
	{
		showComponentInRootPopup(nullptr, nullptr, {});
	}
	else
	{
		detachedPopups.removeObject(p);
	}
}

void FloatingTile::detachCurrentPopupAsync()
{
	Component::SafePointer<FloatingTile> safeThis(this);

	MessageManager::callAsync([safeThis]()
	{
		if (safeThis != nullptr)
			safeThis->toggleDetachPopup(safeThis.getComponent()->currentPopup);
	});
}

void FloatingTile::toggleDetachPopup(FloatingTilePopup* p)
{
	if (p == nullptr)
		return;

	if (currentPopup == p)
	{
		detachedPopups.add(currentPopup.release());
	}
	else
	{
		auto index = detachedPopups.indexOf(p);
		currentPopup = detachedPopups.removeAndReturn(index);
	}
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

#if USE_BACKEND

FloatingTileDocumentWindow::FloatingTileDocumentWindow(BackendRootWindow* parentRoot) :
	DocumentWindow("Popout", HiseColourScheme::getColour(HiseColourScheme::EditorBackgroundColourId), DocumentWindow::TitleBarButtons::allButtons, true),
	parent(parentRoot)
{
	setContentOwned(new FloatingTile(parentRoot->getBackendProcessor(), nullptr), false);
	setVisible(true);
	setUsingNativeTitleBar(true);
	setResizable(true, true);

	auto useOpenGL = GET_HISE_SETTING(getMainController()->getMainSynthChain(), HiseSettings::Other::UseOpenGL).toString() == "1";

	if (useOpenGL)
		setEnableOpenGL(this);

    loadKeyPressMap();
    
	centreWithSize(500, 500);
}

FloatingTileDocumentWindow::~FloatingTileDocumentWindow()
{
    saved = true;
	detachOpenGl();
}

void FloatingTileDocumentWindow::closeButtonPressed()
{
	parent->removeFloatingWindow(this);
}

void FloatingTileDocumentWindow::initialiseAllKeyPresses()
{
    mcl::FullEditor::initKeyPresses(this);
    PopupIncludeEditor::initKeyPresses(this);
    scriptnode::DspNetwork::initKeyPresses(this);
    ScriptContentPanel::initKeyPresses(this);
}

bool FloatingTileDocumentWindow::keyPressed(const KeyPress& key)
{
	if (key == KeyPress::F6Key)
	{
		getBackendRootWindow()->getBackendProcessor()->getCommandManager()->invokeDirectly(BackendCommandTarget::MenuViewEnableGlobalLayoutMode, false);
		return true;
	}

	return false;
}

FloatingTile* FloatingTileDocumentWindow::getRootFloatingTile()
{
	return dynamic_cast<FloatingTile*>(getContentComponent());
}

const hise::MainController* FloatingTileDocumentWindow::getMainControllerToUse() const
{
	return parent->getBackendProcessor();
}

hise::MainController* FloatingTileDocumentWindow::getMainControllerToUse()
{
	return parent->getBackendProcessor();
}

#endif


void FloatingTilePopup::CloseButton::paintButton(Graphics& g, bool over, bool down)
{
	auto c = getLocalBounds().toFloat().reduced(JUCE_LIVE_CONSTANT_OFF(0.0f));

	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff212121)));
	g.fillEllipse(c);
	g.setColour(JUCE_LIVE_CONSTANT_OFF(Colour(0xff6a6a6a)));
	g.drawEllipse(c.reduced(2.0f), 1.0f);

	float alpha = 0.5f;

	if (over || down)
		alpha += 0.2f;

	if (down)
		alpha += 0.2f;

	g.setColour(Colours::white.withAlpha(alpha));
	g.strokePath(p, PathStrokeType(2.0f));
}

void FloatingTilePopup::CloseButton::resized()
{
	auto b = getLocalBounds().toFloat();

	p.clear();
	p.startNewSubPath(0.0f, 0.0f);
	p.lineTo(1.0f, 1.0f);
	p.startNewSubPath(0.0f, 1.0f);
	p.lineTo(1.0f, 0.0f);
	PathFactory::scalePath(p, b.reduced(JUCE_LIVE_CONSTANT_OFF(7.0f)));
}



juce::Path FloatingTilePopup::Factory::createPath(const String& url) const
{
	static const unsigned char pathData[] = { 110,109,45,178,1,67,231,251,35,66,108,23,25,206,66,231,251,35,66,108,23,25,206,66,57,180,148,66,108,240,103,8,67,57,180,148,66,108,240,103,8,67,236,209,62,66,108,233,102,49,67,233,102,177,66,108,240,103,8,67,45,178,1,67,108,240,103,8,67,23,25,206,66,
108,23,25,206,66,23,25,206,66,108,23,25,206,66,240,103,8,67,108,45,178,1,67,240,103,8,67,108,233,102,177,66,233,102,49,67,108,236,209,62,66,240,103,8,67,108,57,180,148,66,240,103,8,67,108,57,180,148,66,23,25,206,66,108,231,251,35,66,23,25,206,66,108,
231,251,35,66,45,178,1,67,108,0,0,0,0,233,102,177,66,108,231,251,35,66,236,209,62,66,108,231,251,35,66,57,180,148,66,108,57,180,148,66,57,180,148,66,108,57,180,148,66,231,251,35,66,108,236,209,62,66,231,251,35,66,108,233,102,177,66,0,0,0,0,108,45,178,
1,67,231,251,35,66,99,101,0,0 };

	Path path;
	path.loadPathFromData(pathData, sizeof(pathData));


	return path;
}


const Identifier FloatingTileHelpers::getTileID(FloatingTile* parent)
{
	return parent->getLayoutData().getID();
}

} // namespace hise
