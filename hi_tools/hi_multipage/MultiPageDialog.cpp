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

#include "MultiPageDialog.h"
#include "MultiPageDialog.h"
#include "MultiPageIds.h"

namespace hise
{

namespace multipage {
using namespace juce;

struct Dialog::TabTraverser: public ComponentTraverser
{
    TabTraverser(Dialog& parent_):
      parent(parent_)
    {};

    Dialog& parent;
    
    Component* getDefaultComponent (Component* parentComponent) override { return &parent; }

    Component* getNextComponent (Component* current) override
    {
        bool useNext = false;
        Component* nextComponent = nullptr;

	    Component::callRecursive<Component>(&parent, [& ](Component* c)
	    {
		    if(c == current)
                useNext = true;
            else if (useNext && c->getWantsKeyboardFocus())
            {
                nextComponent = c;
                return true;
            }

            return false;
	    });

        return nextComponent;
    }

    Component* getPreviousComponent (Component* current) override
    {
	    return nullptr;
    }

    std::vector<Component*> getAllComponents (Component* parentComponent) override
    {
	    std::vector<Component*> list;

        Component::callRecursive<Component>(parentComponent, [&](Component* c)
        {
            if(c->getWantsKeyboardFocus())
                list.push_back(c);

            return false;
        });

        return list;
    }
};


Dialog::PageBase::HelpButton::HelpButton(String help, const PathFactory& factory):
	HiseShapeButton("help", nullptr, factory)
{
	setWantsKeyboardFocus(false);

	onClick = [this, help]()
	{
		if(findParentComponentOfClass<Dialog::ModalPopup>() != nullptr || findParentComponentOfClass<Dialog>()->useHelpBubble)
		{
			findParentComponentOfClass<PageBase>()->setModalHelp(help);
		}
		else
		{
            auto pc = getParentComponent();
			auto img = pc->createComponentSnapshot(pc->getLocalBounds(), true);

			auto mp = findParentComponentOfClass<Dialog>();
			mp->createModalPopup<factory::MarkdownText>()[mpid::Text] = help;
			mp->showModalPopup(false, img);
		}
	};
}



struct Dialog::PageBase::ModalHelp: public Component
{
    ModalHelp(const String& text, PageBase& parent):
      r(text),
      closeButton("close", nullptr, parent.rootDialog)
    {
        r.setStyleData(parent.rootDialog.styleData);
        closeButton.onClick = [&parent]()
        {
            parent.setModalHelp("");
        };

        r.parse();

		auto h = r.getHeightForWidth((float)parent.getWidth() - 26.0f);

		setSize(parent.getWidth() + 20, h + 30);

        addAndMakeVisible(closeButton);
    };

    void paint(Graphics& g) override
    {
		auto b = getLocalBounds();

		auto t = b.removeFromTop(10).withSizeKeepingCentre(20, 10).toFloat();

		Path p;
		p.addRoundedRectangle(b, 6.0f);
		p.addTriangle(t.getBottomLeft(), t.getBottomRight(), {t.getCentreX(), t.getY()});

        g.setColour(Colour(0xFF111111));

		g.fillPath(p);

        auto area = b.reduced(10);
        area.removeFromRight(26);
        
        r.draw(g, area.toFloat());
    }

    void resized() override
    {

        auto b = getLocalBounds().reduced(10);

		b.removeFromTop(10);

		b = b.removeFromRight(18).removeFromTop(18);

        closeButton.setBounds(b);
    }

    MarkdownRenderer r;
    HiseShapeButton closeButton;
};

Dialog::PageBase::PageBase(Dialog& rootDialog_, int width, const var& obj):
  rootDialog(rootDialog_),
  infoObject(obj)
{
    stateObject = rootDialog.getState().globalState;

	positionInfo = rootDialog.getPositionInfo(obj);

	if(obj.hasProperty(mpid::LabelPosition))
    {
        auto np = positionInfo.getLabelPositionNames().indexOf(obj[mpid::LabelPosition].toString());

        if(np != Dialog::PositionInfo::LabelPositioning::Default)
        {
            positionInfo.LabelPosition = np;
	        inheritsPosition = false;
        }
    }

	auto help = obj[mpid::Help].toString();

	if(help.isNotEmpty())
	{
		helpButton = new HelpButton(help, rootDialog);
		addAndMakeVisible(helpButton);
		helpButton->setWantsKeyboardFocus(false);
	}

	if(obj.hasProperty(mpid::Value))
	{
		initValue = obj[mpid::Value];
	}

	auto idString = obj[mpid::ID].toString();

	if(idString.isNotEmpty())
		id = Identifier(idString);
}

DefaultProperties Dialog::PageBase::getDefaultProperties() const
{
	return {};
}

void Dialog::PageBase::setStateObject(const var& newStateObject)
{
	stateObject = newStateObject;
}

var Dialog::PageBase::getPropertyFromInfoObject(const Identifier& id) const
{
	return infoObject.getProperty(id, var(""));
}

void Dialog::PageBase::deleteFromParent()
{
	if(auto pc = findParentComponentOfClass<factory::Container>())
	{
		auto l = pc->infoObject[mpid::Children];
		auto idx = l.indexOf(infoObject);
		rootDialog.getUndoManager().perform(new UndoableVarAction(l, idx, var()));
		rootDialog.refreshCurrentPage();
	}
}

void Dialog::PageBase::duplicateInParent()
{
	if(auto pc = findParentComponentOfClass<factory::Container>())
	{
		auto l = pc->infoObject[mpid::Children];
		auto idx = l.indexOf(infoObject);
		rootDialog.getUndoManager().perform(new UndoableVarAction(l, idx, infoObject.clone()));
		rootDialog.refreshCurrentPage();
	}
}

bool Dialog::PageBase::isEditModeAndNotInPopup() const
{
	return rootDialog.isEditModeEnabled() && findParentComponentOfClass<Dialog::ModalPopup>() == nullptr;
}


void Dialog::PageBase::setModalHelp(const String& text)
{
	if(modalHelp != nullptr)
	{
#if JUCE_DEBUG
		modalHelp->setVisible(false);
#else
		Desktop::getInstance().getAnimator().fadeOut(modalHelp, 200);
#endif
		modalHelp = nullptr;
	}
	else
	{
		Component* popup = findParentComponentOfClass<ModalPopup>();

		if(popup == nullptr)
			popup = dynamic_cast<Component*>(findParentComponentOfClass<ComponentWithSideTab>());

		Component::callRecursive<PageBase>(popup, [](PageBase* b)
		{
			b->modalHelp = nullptr;
			return false;
		});
		
		jassert(popup != nullptr);

		popup->addAndMakeVisible(modalHelp = new ModalHelp(text, *this));

#if JUCE_DEBUG
		modalHelp->setVisible(true);
#else
		Desktop::getInstance().getAnimator().fadeIn(modalHelp, 200);
#endif

		modalHelp->toFront(false);

		auto thisBounds = popup->getLocalArea(this, getLocalBounds());

		auto mb = thisBounds.withSizeKeepingCentre(modalHelp->getWidth(), modalHelp->getHeight());
		mb.setY(thisBounds.getBottom() + 3);
		
		modalHelp->setBounds(mb);
	}
}

Rectangle<int> Dialog::PageBase::getArea(AreaType t) const
{
	auto b = getLocalBounds();

	using Pos = Dialog::PositionInfo::LabelPositioning;

	auto posToUse = (Pos)positionInfo.LabelPosition;

	if(rootDialog.isEditModeEnabled() && posToUse == Pos::None)
		posToUse = Pos::Left;

	Rectangle<int> lb;

	switch(posToUse)
	{
	case Pos::Left:  lb = b.removeFromLeft(positionInfo.getWidthForLabel(getWidth())); break;
	case Pos::Above: lb = b.removeFromTop(positionInfo.LabelHeight); break;
	case Pos::None:  break;
	}
	        
	return t == AreaType::Component ? b : lb;
}

void Dialog::PageBase::onEditModeChange(PageBase& c, bool isOn)
{
	c.editModeChanged(isOn);
}



bool Dialog::PageBase::showDeletePopup(bool isRightClick)
{
	if(isRightClick)
	{
		return rootDialog.nonContainerPopup(infoObject);
	}

	return rootDialog.showEditor(infoObject);
	
}

String Dialog::PageBase::getDefaultPositionName() const
{
	auto ln = Dialog::PositionInfo::getLabelPositionNames();
	return ln[inheritsPosition ? (int)Dialog::PositionInfo::LabelPositioning::Default : positionInfo.LabelPosition];
}

String Dialog::PageBase::evaluate(const Identifier& id) const
{
	return factory::MarkdownText::getString(infoObject[id].toString(), rootDialog);
}

Asset::Ptr Dialog::PageBase::getAsset(const Identifier& id) const
{
	auto assetId = infoObject[id].toString().trim();

	if(assetId.startsWith("${"))
	{
		assetId = assetId.substring(2, assetId.length() - 1);

		for(auto a: rootDialog.getState().assets)
		{
			if(a->id == assetId)
				return a;
		}
	}

	return nullptr;
}

void Dialog::PageBase::init()
{
	if(findParentComponentOfClass<Dialog::ModalPopup>() == nullptr)
		rootDialog.getEditModeBroadcaster().addListener(*this, PageBase::onEditModeChange);

	if(!initValue.isUndefined() && !initValue.isVoid())
	{
		writeState(initValue);
		initValue = var();
	}
}


void Dialog::PageBase::editModeChanged(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	if(isEditMode)
	{
		overlay = new EditorOverlay(rootDialog);
		overlay->setAttachToComponent(this);
	            
	}
	else
	{
		if(overlay != nullptr)
			rootDialog.removeChildComponent(overlay);
		overlay = nullptr;
	}
	
	repaint();
#endif
}


void Dialog::PageBase::writeState(const var& newValue) const
{
	if(id.isNull())
		return;

	if(auto o = stateObject.getDynamicObject())
	{
		if(stateObject[id] != newValue)
		{
			if(stateObject.getDynamicObject() == rootDialog.getState().globalState.getDynamicObject())
			{
				String s;
				s << "state." << id << " = " << JSON::toString(newValue, true);
				rootDialog.logMessage(MessageType::ValueChangeMessage, s);
			}

			rootDialog.getUndoManager().perform(new UndoableVarAction(stateObject, id, newValue));
		}
			
	}
	else
	{
		jassertfalse;
	}
}

var Dialog::PageBase::getValueFromGlobalState(var defaultState)
{
    return stateObject.getProperty(id, defaultState);
	
}

String Dialog::PageBase::loadValueOrAssetAsText()
{
	auto s = getValueFromGlobalState("").toString();

	if(s.startsWith("${"))
	{
		return rootDialog.getState().loadText(s);
	}

	return s;
}

Result Dialog::PageBase::check(const var& obj)
{
	if(cf)
	{
		auto ok = cf(this, obj);
                
		if(ok.failed())
		{
			if(rootDialog.currentErrorElement == nullptr)
				rootDialog.currentErrorElement = this;

			return ok;
		}
	}

	auto ok = checkGlobalState(obj);

	if(ok.failed() && rootDialog.currentErrorElement == nullptr)
		rootDialog.currentErrorElement = this;

	return ok;
}

void Dialog::PageBase::clearCustomFunction()
{
	cf = {};
}

void Dialog::PageBase::setCustomCheckFunction(const CustomCheckFunction& cf_)
{
	if(!cf)
		cf = cf_;
}

bool Dialog::PageBase::isError() const
{ return rootDialog.currentErrorElement == this; }

var Dialog::PositionInfo::toJSON() const
{
	auto obj = new DynamicObject();
            
	obj->setProperty("TopHeight", TopHeight);
	obj->setProperty("ButtonTab", ButtonTab);
	obj->setProperty("ButtonMargin", ButtonMargin);
	obj->setProperty("OuterPadding", OuterPadding);
    obj->setProperty("LabelWidth", LabelWidth);
	obj->setProperty("LabelHeight", LabelHeight);
	obj->setProperty("DialogWidth", fixedSize.getX());
	obj->setProperty("DialogHeight", fixedSize.getY());
	obj->setProperty("LabelPosition", getLabelPositionNames()[LabelPosition]);
	
	return var(obj);
}

void Dialog::PositionInfo::fromJSON(const var& obj)
{
	TopHeight = obj.getProperty("TopHeight", TopHeight);
	ButtonTab = obj.getProperty("ButtonTab", ButtonTab);
	ButtonMargin = obj.getProperty("ButtonMargin", ButtonMargin);
	OuterPadding = obj.getProperty("OuterPadding", OuterPadding);
    LabelWidth = obj.getProperty("LabelWidth", LabelWidth);

	LabelHeight = obj.getProperty("LabelHeight", LabelHeight);

	auto ln = getLabelPositionNames();

	fixedSize.setX(obj.getProperty("DialogWidth", fixedSize.getX()));
	fixedSize.setY(obj.getProperty("DialogHeight", fixedSize.getY()));

	LabelPosition = ln.indexOf(obj.getProperty(mpid::LabelPosition, ln[LabelPosition]).toString());

	if(!isPositiveAndBelow(LabelPosition, LabelPositioning::numLabelPositionings))
		LabelPosition = (int)LabelPositioning::Left;
}

Result Dialog::PositionInfo::checkSanity() const
{
	if(ButtonTab - 2 * ButtonMargin < 5)
		return Result::fail("ButtonTab / ButtonMargin invalid");

	if(OuterPadding > 100)
		return Result::fail("OuterPadding too high");
            
	return Result::ok();
}

void Dialog::PositionInfo::setDefaultPosition(LabelPositioning p)
{
	if(LabelPosition == LabelPositioning::Default)
		LabelPosition = p;
}

int Dialog::PositionInfo::getWidthForLabel(int totalWidth) const
{
	if(LabelWidth < 0.0)
		return jmax(10, roundToInt(totalWidth * -1.0 * LabelWidth));
	else
		return jmax(10, roundToInt(LabelWidth));
}

int Dialog::PositionInfo::getHeightForComponent(int heightWithoutLabel)
{
	if(LabelPosition == (int)LabelPositioning::Above)
		heightWithoutLabel += LabelHeight;

	return heightWithoutLabel;
}

Rectangle<int> Dialog::PositionInfo::getBounds(Rectangle<int> fullBounds) const
{
	if(fixedSize.isOrigin())
		return fullBounds;
	else
		return fullBounds.withSizeKeepingCentre(fixedSize.getX(), fixedSize.getY());
}


ErrorComponent::ErrorComponent(Dialog& parent_):
	parent(parent_),
	currentError(Result::ok()),
	parser("")
	
{
	addAndMakeVisible(closeButton = new HiseShapeButton("close", nullptr, parent));
	closeButton->onClick = [this]()
	{
		this->show(false);
	};
}

void ErrorComponent::paint(Graphics& g)
{
    auto ec = Colour(0xFF161616);
    
	g.setGradientFill(ColourGradient(ec.withMultipliedBrightness(1.1f), 0.0f, 0.0f, ec.withMultipliedBrightness(0.9f), 0.0f, (float)getHeight(), false));
	g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 5.0f);
    
    g.setColour(Colours::black.withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(1.0f), 5.0f, 1.0f);
            
	auto x = getLocalBounds().toFloat();
	x.removeFromRight(40.0f);
	x.removeFromLeft(40.0f);
	x.removeFromTop(3.0f);
            
	parser.draw(g, x);
}

void ErrorComponent::setError(const Result& r)
{
	currentError = r;
            
	if(!r.wasOk())
	{
		auto w = getWidth() - 80;

		if(w < 10)
			w = 300;

		parser.setNewText(r.getErrorMessage());
		parser.parse();
		height = jmax<int>(40, parser.getHeightForWidth(w));
	}
            
	show(!currentError.wasOk());
}

void ErrorComponent::show(bool shouldShow)
{
	setVisible(shouldShow);
	getParentComponent()->resized();
    getParentComponent()->repaint();
	setVisible(!shouldShow);
            
	auto& animator = Desktop::getInstance().getAnimator();
            
	if(shouldShow)
		animator.fadeIn(this, 50);
	else
		animator.fadeOut(this, 50);

	repaint();
}

void ErrorComponent::resized()
{
	auto b = getLocalBounds();
	auto cb = b.removeFromRight(40).removeFromTop(40).reduced(10);
	closeButton->setBounds(cb);
}


Dialog::PageBase* Dialog::PageInfo::create(Dialog& r, int currentWidth) const
{
	if(pageCreator)
	{
		auto np = pageCreator(r, currentWidth, data);

		if(!useGlobalStateObject && stateObject.isObject())
			np->setStateObject(stateObject);

        np->setCustomCheckFunction(customCheck);

		if(auto c = dynamic_cast<factory::Container*>(np))
		{
			for(auto ch: childItems)
				c->addChild(ch);
		}

		return np;
	}
            
	return nullptr;
}

var& Dialog::PageInfo::operator[](const Identifier& id) const
{
	if(auto obj = data.getDynamicObject())
	{
		if(!obj->hasProperty(id))
			obj->setProperty(id, var());

		return *obj->getProperties().getVarPointer(id);
	}
            
	static var nullValue;
	return nullValue;
}

Dialog::PageInfo* Dialog::PageInfo::getChildRecursive(PageInfo* pi, const Identifier& id)
{
	if(pi->data[mpid::ID].toString() == id.toString())
			return pi;

	for(auto c: pi->childItems)
	{
		if(auto child = getChildRecursive(c, id))
				return child;
	}

	return nullptr;
}

Dialog::PageInfo& Dialog::PageInfo::getChild(const Identifier& id)
{
	if(auto c = getChildRecursive(this, id))
	{
		return *c;
	}

	jassertfalse;
	return *this;
}

void Dialog::PageInfo::setCreateFunction(const CreateFunction& f)
{
	pageCreator = f;
}

void Dialog::PageInfo::setCustomCheckFunction(const PageBase::CustomCheckFunction& f)
{
	customCheck = f;
}

void Dialog::PageInfo::setStateObject(const var& newStateObject)
{
	stateObject = newStateObject;
	useGlobalStateObject = false;
}




Dialog::ModalPopup::ModalPopup(Dialog& parent_):
	parent(parent_),
	modalError(parent),
	okButton("OK"),
	cancelButton("Cancel")
{
	addChildComponent(modalError);
	addAndMakeVisible(okButton);
	addAndMakeVisible(cancelButton);

	okButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::onOk);
	cancelButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::dismiss);
}

void Dialog::ModalPopup::onOk()
{
	if(contentComponent != nullptr)
	{
		auto obj = content->stateObject;

		if(!obj.isObject())
			obj = getGlobalState(*this, {}, var());

		auto r = contentComponent->check(obj);

		if(!r.wasOk())
		{
			modalError.setError(r);
			return;
		}

		dismiss();
	}
}

void Dialog::ModalPopup::dismiss()
{
	parent.currentErrorElement = nullptr;

#if JUCE_DEBUG
	setVisible(false);
#else
	Desktop::getInstance().getAnimator().fadeOut(this, 100);
#endif
}

void Dialog::ModalPopup::setContent(PageInfo::Ptr newContent)
{
	content = newContent;
}

void Dialog::ModalPopup::show(PositionInfo newInfo, bool addButtons, const Image& additionalScreenshot)
{
	info = newInfo;

	okButton.setVisible(addButtons);
	cancelButton.setVisible(addButtons);

	screenshot = additionalScreenshot;

	if(content != nullptr)
	{
		addAndMakeVisible(contentComponent = content->create(parent, getWidth() - 4 * info.OuterPadding));

		contentComponent->postInit();
		resized();

#if JUCE_DEBUG
		setVisible(true);
#else
		Desktop::getInstance().getAnimator().fadeIn(this, 100);
#endif        

		toFront(true);
	}
}

bool Dialog::ModalPopup::keyPressed(const KeyPress& k)
{
	if(k.getKeyCode() == KeyPress::returnKey)
	{
		okButton.triggerClick();
		return true;
	}
	if(k.getKeyCode() == KeyPress::escapeKey)
	{
		dismiss();
		return true;
	}
}

void Dialog::ModalPopup::mouseDown(const MouseEvent& e)
{
	if(!modalBounds.contains(e.getPosition()))
	{
		dismiss();
	}
}

void Dialog::ModalPopup::paint(Graphics& g)
{
	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		laf->drawMultiPageModalBackground(g, *this, getLocalBounds(), modalBounds);

	if(screenshot.isValid())
	{
        auto imageBounds = modalBounds.withSizeKeepingCentre(screenshot.getWidth(), screenshot.getHeight());
        
        imageBounds.translate(0, - modalBounds.getHeight() / 2 - imageBounds.getHeight());

        const auto r = imageBounds.expanded(20, 4).toFloat();

        g.setColour(Colour(0xFF2222222));
        g.fillRoundedRectangle(r, 4.0f);
        
        g.setColour(Colour(HISE_OK_COLOUR));
        g.drawRoundedRectangle(r, 4.0f, 2.0f);

		g.drawImageAt(screenshot, imageBounds.getX(), imageBounds.getY());
	}
}

void Dialog::ModalPopup::resized()
{
	auto b = getLocalBounds();

	int thisHeight = 2 * info.OuterPadding;

	if(contentComponent != nullptr)
		thisHeight += contentComponent->getHeight();

	if(modalError.isVisible())
		thisHeight += modalError.height;

	if(okButton.isVisible())
		thisHeight += info.ButtonTab;

	b = b.withSizeKeepingCentre(getWidth() - 2 * info.OuterPadding, thisHeight);

	modalBounds = b;

	b = b.reduced(info.OuterPadding);
            
	if(modalError.isVisible())
	{
		modalError.setBounds(b.removeFromTop(modalError.height).expanded(6, 3));
		b.removeFromTop(info.OuterPadding);
	}
		

	if(okButton.isVisible())
	{
		auto bottom = b.removeFromBottom(info.ButtonTab);
                
		okButton.setBounds(bottom.removeFromLeft(100).reduced(info.ButtonMargin));
		cancelButton.setBounds(bottom.removeFromRight(100).reduced(info.ButtonMargin));
	}
            
	if(contentComponent != nullptr)
		contentComponent->setBounds(b);

	repaint();
}


Dialog::Dialog(const var& obj, State& rt, bool addEmptyPage):
	cancelButton("Cancel"),
	nextButton("Next"),
	prevButton("Previous"),
	popup(*this),
	//errorComponent(*this),
    runThread(&rt)
{

	jassert(runThread->currentDialog == nullptr);

    runThread->currentDialog = this;
	sf.addScrollBarToAnimate(content.getVerticalScrollBar());

	if(auto sd = obj[mpid::StyleData].getDynamicObject())
	{
		styleData.fromDynamicObject(var(sd), std::bind(&State::loadFont, &getState(), std::placeholders::_1));
		refreshAdditionalColours(var(sd));
	}
	else
	{
		styleData = MarkdownLayout::StyleData::createDarkStyle();
		styleData.backgroundColour = Colour(0xFF333333);
		refreshAdditionalColours({});
	}
	
	//errorComponent.parser.setStyleData(styleData);

	addChildComponent(popup);

	if(auto ld = obj[mpid::LayoutData].getDynamicObject())
		defaultLaf.defaultPosition.fromJSON(var(ld));

	if(auto ps = obj[mpid::Properties].getDynamicObject())
		properties = var(ps);
	else
    {
        auto po = new DynamicObject();
        properties = var(po);
        
        po->setProperty(mpid::Header, "Header");
        po->setProperty(mpid::Subtitle, "Subtitle");
		po->setProperty(mpid::Image, "");
		po->setProperty(mpid::ProjectName, "MyProject");
		po->setProperty(mpid::Company, "MyCompany");
		po->setProperty(mpid::Version, "1.0.0");
    }

	backgroundImage = getState().loadImage(properties[mpid::Image].toString());

	auto pageList = obj[mpid::Children];

	if(pageList.isArray())
    {
		pageListArrayAsVar = pageList;
        pageListInfo = pageListArrayAsVar.getArray();
    }
    else
    {
		pageListArrayAsVar = var(Array<var>());
		pageListInfo = pageListArrayAsVar.getArray();

		if(addEmptyPage)
		{
			auto fc = new DynamicObject();
		    fc->setProperty(mpid::Type, "List");
		    fc->setProperty(mpid::Padding, 10);
		    pageListInfo->add(var(fc));
		}
    }
	
	Factory factory;

    for(const auto& p: *pageListInfo)
    {
        if(auto pi = factory.create(p))
        {
            pi->setStateObject(getState().globalState);
            pi->useGlobalStateObject = true;
            pages.add(std::move(pi));
        }
    }
	
	addAndMakeVisible(cancelButton);
	addAndMakeVisible(nextButton);
	addAndMakeVisible(prevButton);
	addAndMakeVisible(content);
	//addChildComponent(errorComponent);
	setLookAndFeel(&defaultLaf);

    setWantsKeyboardFocus(true);

	setSize(700, 400);
        
	nextButton.onClick = [this]()
	{
		navigate(true);
	};
        
	prevButton.onClick = [this]()
	{
		navigate(false);
	};

	cancelButton.onClick = [this]()
	{
		auto& l = createModalPopup<factory::List>();

        l[mpid::Padding] = 15;
        
        auto& md = l.addChild<factory::MarkdownText>();
        
        auto& to = l.addChild<factory::Button>({
            { mpid::Text, "Save progress" },
            { mpid::ID, "Save" }
        });
        
		md[mpid::Text] = "Do you want to close this popup?";

		md.setCustomCheckFunction([this](PageBase*, var obj)
		{
			MessageManager::callAsync(finishCallback);
			return Result::ok();
		});
        
        

		showModalPopup(true);
	};

	content.getVerticalScrollBar().addListener(this);

	//setEditMode(true);
}

Dialog::~Dialog()
{
	runThread->currentDialog = nullptr;
}


String Dialog::getStringFromModalInput(const String& message, const String& prefilledValue)
{
	auto laf = &getLookAndFeel();

	ScopedPointer<AlertWindow> nameWindow = new AlertWindow(message, "", AlertWindow::AlertIconType::QuestionIcon);

	nameWindow->setLookAndFeel(laf);

	nameWindow->addTextEditor("Name", prefilledValue );
	nameWindow->addButton("OK", 1, KeyPress(KeyPress::returnKey));
	nameWindow->addButton("Cancel", 0, KeyPress(KeyPress::escapeKey));

	nameWindow->getTextEditor("Name")->setSelectAllWhenFocused(true);
	nameWindow->getTextEditor("Name")->grabKeyboardFocusAsync();

	if(nameWindow->runModalLoop()) 
		return nameWindow->getTextEditorContents("Name");

	else return String();
}

bool Dialog::refreshCurrentPage()
{
	auto index = getState().currentPageIndex;

	logMessage(MessageType::Navigation, "Goto page " + String(index+1));

	if((currentPage = pages[index]->create(*this, content.getWidth() - content.getScrollBarThickness() - 10)))
	{

		content.setViewedComponent(currentPage);
		currentPage->postInit();
		currentPage->setLookAndFeel(&getLookAndFeel());
		nextButton.setButtonText(runThread->currentPageIndex == pages.size() - 1 ? "Finish" : "Next");

		refreshBroadcaster.sendMessage(sendNotificationSync, index);

		return true;
	}

	return false;
}

void Dialog::createEditorInSideTab(const var& obj, PageBase* pb, const std::function<void(PageInfo&)>& r)
{
	if(auto st = findParentComponentOfClass<ComponentWithSideTab>())
	{
		currentlyEditedPage = pb;
		
		auto ns = new State(var());
		ns->globalState = obj;
		auto d = new Dialog(var(), * ns, true);

		d->useHelpBubble = true;

		d->getViewport().setScrollBarThickness(12);

		auto& layout = d->defaultLaf.defaultPosition;
		layout.OuterPadding = 10;
		layout.TopHeight = 0;
		auto sd = d->getStyleData();
		sd.fontSize = 14.0f;
		sd.f = GLOBAL_FONT();
		sd.textColour = Colours::white.withAlpha(0.7f);
		sd.backgroundColour = Colours::transparentBlack;
		d->setStyleData(sd);
		layout.LabelPosition = PositionInfo::LabelPositioning::Left;
		layout.ButtonTab = 0;
		layout.ButtonMargin = 0;
		layout.LabelWidth = 85.0;

		(*d->pages.getFirst())[mpid::Padding] = 5;

		r(*d->pages.getFirst());

		d->prevButton.setVisible(false);
		d->nextButton.setButtonText("Apply");
		d->refreshCurrentPage();

		d->setFinishCallback([this, st]()
		{
			this->refreshCurrentPage();
			st->setSideTab(nullptr, nullptr);
		});
            
		if(!st->setSideTab(ns, d))
			currentlyEditedPage = nullptr;
	}

	repaint();
}

String Dialog::joinVarArrayToNewLineString(const var& v)
{
	String s;

	if(v.isArray())
	{
		for(auto& item: *v.getArray())
			s << item.toString() << "\n";
	}
	else
		s << v.toString();

	return s;
}

var Dialog::parseCommaList(const String& text)
{
	auto sa = StringArray::fromTokens(text, ",", "");
	sa.trim();

	Array<var> values;

	for(const auto& s: sa)
		values.add(var(s));

	return var(values);
}

void Dialog::showModalPopup(bool addButtons, const Image& additionalScreenshot)
{
	PositionInfo info;

	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		info = laf->getMultiPagePositionInfo(popup.content->getData());

	popup.show(info, addButtons, additionalScreenshot);
	    
}

Result Dialog::checkCurrentPage()
{
	if(currentPage != nullptr)
	{
		currentErrorElement = nullptr;

		auto ok = currentPage->check(runThread->globalState);

		//errorComponent.setError(ok);

		repaint();
		return ok;
	}

	return Result::fail("No page");
}

void Dialog::setCurrentErrorPage(PageBase* b)
{
	currentErrorElement = b;
	repaint();
}

bool Dialog::keyPressed(const KeyPress& k)
{
	if(k.getKeyCode() == KeyPress::F5Key)
	{
		if(currentPage != nullptr)
			currentPage->check(getState().globalState);

		if(auto st = findParentComponentOfClass<ComponentWithSideTab>())
		{
			st->refreshDialog();
		}
		
		return true;
	}
	if(k.getKeyCode() == KeyPress::returnKey)
	{
		if(nextButton.isEnabled())
		{
			nextButton.triggerClick();
			return true;
		}
	}

	return false;
}

var Dialog::exportAsJSON() const
{
	DynamicObject::Ptr json = new DynamicObject();

	auto sd = styleData.toDynamicObject();

	auto fontName = sd[MarkdownStyleIds::Font].toString();
	auto boldFontName = sd[MarkdownStyleIds::BoldFont].toString();

	for(auto a: runThread->assets)
	{
		if(a->type == Asset::Type::Font)
		{
			auto f = a->toFont();

			if(f.getTypefaceName() == fontName)
			{
				sd.getDynamicObject()->setProperty(MarkdownStyleIds::Font, a->toReferenceVariable());
			}
			if(f.getTypefaceName() == boldFontName)
			{
				sd.getDynamicObject()->setProperty(MarkdownStyleIds::BoldFont, a->toReferenceVariable());
			}	
		}
	}

#define SET_ADDITIONAL_COLOUR(x, unused) sd.getDynamicObject()->setProperty(#x, additionalColours[x].getARGB());

	SET_ADDITIONAL_COLOUR(buttonTabBackgroundColour, 0x22000000);
    SET_ADDITIONAL_COLOUR(buttonBgColour, 0xFFAAAAAA);
    SET_ADDITIONAL_COLOUR(buttonTextColour, 0xFF252525);
    SET_ADDITIONAL_COLOUR(modalPopupBackgroundColour, 0xFF333333);
	SET_ADDITIONAL_COLOUR(modalPopupOverlayColour, 0xEE222222);
    SET_ADDITIONAL_COLOUR(modalPopupOutlineColour, 0xFFAAAAAA);
    SET_ADDITIONAL_COLOUR(pageProgressColour, 0x77FFFFFF);

#undef SET_ADDITIONAL_COLOUR

	json->setProperty(mpid::StyleData, sd);
	json->setProperty(mpid::Properties, properties);
	json->setProperty(mpid::LayoutData, defaultLaf.defaultPosition.toJSON());
	json->setProperty(mpid::GlobalState, runThread->globalState);
	json->setProperty(mpid::Children, pageListArrayAsVar);
	json->setProperty(mpid::Assets, runThread->exportAssetsAsJSON(false));

	return var(json.get());
}

Dialog::PageBase* Dialog::findPageBaseForInfoObject(const var& obj)
{
	PageBase* found = nullptr;

	callRecursive<PageBase>(this, [&](PageBase* b)
	{
		if(b->matches(obj))
		{
			found = b;
			return true;
		}

		return false;
	});

	return found;
}

Dialog::PositionInfo Dialog::getPositionInfo(const var& pageData) const
{
	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		return laf->getMultiPagePositionInfo(pageData);

	return {};
}

void Dialog::showMainPropertyEditor()
{
	using namespace factory;

	auto no = new DynamicObject();

	createEditorInSideTab(var(no), nullptr, [this](PageInfo& introPage)
	{
	    auto& propList = introPage.addChild<List>({
	        { mpid::Padding, 10 },
	        { mpid::ID, mpid::Properties.toString() }
	    });

		

		auto& col0 = propList;//

	    col0.addChild<TextInput>({
	        { mpid::ID, mpid::Header.toString() },
	        { mpid::Text, mpid::Header.toString() },
	        { mpid::Required, true },
			{ mpid::Help, "This will be shown as big title at the top" },
			{ mpid::Value, properties[mpid::Header] }
	    });

	    col0.addChild<TextInput>({
	        { mpid::ID, mpid::Subtitle.toString() },
	        { mpid::Text, mpid::Subtitle.toString() },
	        { mpid::Help, "This will be shown as small title below" },
			{ mpid::Value, properties[mpid::Subtitle] }
	    });

		col0.addChild<TextInput>({
	        { mpid::ID, mpid::Image.toString() },
	        { mpid::Text, mpid::Image.toString() },
	        { mpid::Help, "The background image used for the dialog (will be scaled to fit the entire area)." },
			{ mpid::Value, properties[mpid::Image] }
	    });

		auto& col1 = propList;//

		col1.addChild<TextInput>({
	        { mpid::ID, mpid::ProjectName.toString() },
	        { mpid::Text, mpid::ProjectName.toString() },
			{ mpid::Value, properties[mpid::ProjectName] }
	    });

		col1.addChild<TextInput>({
	        { mpid::ID, mpid::Company.toString() },
	        { mpid::Text, mpid::Company.toString() },
			{ mpid::Value, properties[mpid::Company] }
	    });

		col1.addChild<TextInput>({
	        { mpid::ID, mpid::Version.toString() },
	        { mpid::Text, mpid::Version.toString() },
			{ mpid::Value, properties[mpid::Version] }
	    });

	    auto& styleProperties = introPage.addChild<List>({
	        { mpid::ID, mpid::StyleData.toString(), },
	        { mpid::Text, mpid::StyleData.toString(), },
	        { mpid::Padding, 10 },
	        { mpid::Foldable, true },
	        { mpid::Folded, true }
	    });

		auto sdData = exportAsJSON()[mpid::StyleData];
	    
	    const Array<Identifier> hiddenProps({
	      Identifier("codeBgColour"),
	      Identifier("linkBgColour"),
	      Identifier("codeColour"),
	      Identifier("linkColour"),
	      Identifier("tableHeaderBgColour"),
	      Identifier("tableLineColour"),
	      Identifier("tableBgColour")
	    });

		PageInfo* colourCol = &styleProperties;

		int numColoursInRow = 0;

	    for(auto& nv: sdData.getDynamicObject()->getProperties())
	    {
	        if(hiddenProps.contains(nv.name))
	            continue;

	        if(nv.name.toString().contains("Colour"))
	        {
				auto& ed = styleProperties.addChild<ColourChooser>({
	                { mpid::ID, nv.name.toString() },
	                { mpid::Text, nv.name.toString() },
					{ mpid::LabelPosition, "Above" },
	                { mpid::Value, nv.value }
		        });
	        }
	        else
	        {
	            auto& ed = styleProperties.addChild<TextInput>({
	                { mpid::ID, nv.name.toString() },
	                { mpid::Text, nv.name.toString() },
	                { mpid::Value, nv.value }
	            });

	            if(nv.name == Identifier("Font") || nv.name == Identifier("BoldFont"))
	            {
		            ed[mpid::Items] = Font::findAllTypefaceNames().joinIntoString("\n");
	            }
	        }
	    }

	    {
	        auto& layoutProperties = introPage.addChild<List>({
	            { mpid::ID, mpid::LayoutData.toString() },
	            { mpid::Text, mpid::LayoutData.toString() },
	            { mpid::Padding, 10 },
	            { mpid::Foldable, true },
	            { mpid::Folded, true }
	        });

			

			auto layoutObj = defaultLaf.getMultiPagePositionInfo({}).toJSON();

	        std::map<Identifier, String> help;
	        help["OuterPadding"] = "The distance between the content and the component bounds in pixel.";
	        help["ButtonTab"] = "The height of the bottom tab with the Cancel / Prev / Next buttons.";
	        help["ButtonMargin"] = "The distance between the buttons in the bottom tab in pixel.";
	        help["TopHeight"] = "The height of the top bar with the title and the step progress in pixel.";
	        help["LabelWidth"] = "The width of the text labels of each property component. You can use either relative or absolute size values:\n- Negative values are relative to the component width (minus the `OuterPadding` property)\n- positive values are absolute pixel values.";
			help["LabelHeight"] = "The height of the label if the positioning is set to either `Above` or `Below`";
			help[mpid::LabelPosition] = "The default position of the text label";

			PageInfo* currentCol = &layoutProperties;

			int numInCol = 0;

	        for(auto& v: layoutObj.getDynamicObject()->getProperties())
	        {
				if(v.name == mpid::LabelPosition)
				{
					layoutProperties.addChild<Choice>({
		                { mpid::ID, v.name.toString() },
						{ mpid::Text, v.name.toString() },
		                { mpid::Value, v.value },
						{ mpid::Items, PositionInfo::getLabelPositionNames().joinIntoString("\n") },
		                { mpid::Help, help[v.name] }
			        });
				}
				else
				{
					layoutProperties.addChild<TextInput>({
		                { mpid::ID, v.name.toString() },
						{ mpid::Text, v.name.toString() },
		                { mpid::Value, v.value },
		                { mpid::Help, help[v.name] }
			        });
				}

				numInCol++;
	        }
	    }

		introPage.setCustomCheckFunction([&](PageBase* b, var obj)
		{
			Container::checkChildren(b, obj);

			properties = obj[mpid::Properties].clone();
			styleData.fromDynamicObject(obj[mpid::StyleData], std::bind(&State::loadFont, &getState(), std::placeholders::_1));

			refreshAdditionalColours(obj[mpid::StyleData]);

			PositionInfo newPos;

			newPos.fromJSON(obj[mpid::LayoutData]);

			auto ok = newPos.checkSanity();

			if(ok.wasOk())
				defaultLaf.defaultPosition = newPos;

			resized();
			repaint();
			
			return ok;
		});
	});
}


bool Dialog::removeCurrentPage()
{
	pageListInfo->remove(runThread->currentPageIndex);
	pages.remove(runThread->currentPageIndex);

	runThread->currentPageIndex = jmin(runThread->currentPageIndex, pageListInfo->size()-1);

	refreshCurrentPage();
	resized();
	repaint();
	return true;
}

void Dialog::addListPageWithJSON()
{
	auto fc = new DynamicObject();
	fc->setProperty(mpid::Type, "List");
	fc->setProperty(mpid::Padding, 10);
        
	pageListInfo->add(var(fc));

	Factory factory;

	if(auto pi = factory.create(pageListInfo->getLast()))
	{
		pi->setStateObject(getState().globalState);
		pi->useGlobalStateObject = true;
		pages.add(std::move(pi));
	}
        
	refreshCurrentPage();
	resized();
	repaint();
}

void Dialog::refreshAdditionalColours(const var& styleDataObject)
{
#define SET_ADDITIONAL_COLOUR(x, defaultColour) additionalColours[x] = Colour((uint32)(int64)styleDataObject.getProperty(#x, Colour(defaultColour).getARGB()));

	SET_ADDITIONAL_COLOUR(buttonTabBackgroundColour, 0x22000000);
    SET_ADDITIONAL_COLOUR(buttonBgColour, 0xFFAAAAAA);
    SET_ADDITIONAL_COLOUR(buttonTextColour, 0xFF252525);
    SET_ADDITIONAL_COLOUR(modalPopupBackgroundColour, 0xFF333333);
	SET_ADDITIONAL_COLOUR(modalPopupOverlayColour, 0xEE222222);
    SET_ADDITIONAL_COLOUR(modalPopupOutlineColour, 0xFFAAAAAA);
    SET_ADDITIONAL_COLOUR(pageProgressColour, 0x77FFFFFF);

#undef SET_ADDITIONAL_COLOUR
}

Result Dialog::getCurrentResult()
{ return runThread->currentError; }

void Dialog::showFirstPage()
{
	currentPage = nullptr;
    runThread->currentPageIndex = -1;
	navigate(true);
}

void Dialog::setFinishCallback(const std::function<void()>& f)
{
	finishCallback = f;
}

var Dialog::getOrCreateChild(const var& obj, const Identifier& id)
{
	if(id.isNull())
		return obj;

	if(obj.hasProperty(id))
		return obj[id];

	jassert(obj.getDynamicObject() != nullptr);

	auto no = new DynamicObject();
	obj.getDynamicObject()->setProperty(id, var(no));
	return var(no);
}

/*
void MultiPageDialog::setGlobalState(Component& page, const Identifier& id, var newValue)
{
    auto typed = dynamic_cast<MultiPageDialog*>(&page);
    
    if(typed == nullptr)
        typed = page.findParentComponentOfClass<MultiPageDialog>();
    
    if(typed != nullptr)
	{
		typed->runThread->globalState.getDynamicObject()->setProperty(id, newValue);
	}
}
*/

var Dialog::getGlobalState(Component& page, const Identifier& id, const var& defaultValue)
{
	if(auto m = page.findParentComponentOfClass<Dialog>())
	{
		if(id.isNull())
			return m->runThread->globalState;

		return m->runThread->globalState.getProperty(id, defaultValue);
	}
        
	return defaultValue;
}

std::pair<Font, Colour> Dialog::getDefaultFont(Component& c)
{
	if(auto m = c.findParentComponentOfClass<Dialog>())
	{
		return { m->styleData.getFont(), m->styleData.textColour };
	}
        
	return { Font(), Colours::white };
}

State::Job::Ptr Dialog::getJob(const var& obj) const
{
	return runThread->getJob(obj);
}

Path Dialog::createPath(const String& url) const
{
	auto assetPath = assets[Identifier(url)].toString();

	Path p;

	if(assetPath.isNotEmpty())
	{
		MemoryBlock mb;
		mb.fromBase64Encoding(assetPath);
		p.loadPathFromData(mb.getData(), mb.getSize());
		return p;
	}
        
    if(url == "retry")
    {
        static const unsigned char x[] = { 110,109,63,53,160,65,6,129,27,65,98,82,184,148,65,78,98,240,64,16,88,128,65,180,200,194,64,86,14,85,65,248,83,195,64,98,82,184,234,64,170,241,198,64,2,43,63,64,84,227,107,65,84,227,253,64,141,151,156,65,98,104,145,55,65,113,61,184,65,205,204,148,65,246,
        40,170,65,72,225,165,65,223,79,134,65,108,78,98,216,65,223,79,134,65,98,193,202,214,65,244,253,142,65,49,8,212,65,203,161,151,65,207,247,207,65,98,16,160,65,98,119,190,177,65,119,190,222,65,61,10,37,65,100,59,242,65,92,143,114,64,188,116,192,65,98,217,
        206,119,192,12,2,134,65,217,206,247,62,217,206,119,62,242,210,83,65,111,18,131,58,98,57,180,86,65,0,0,0,0,33,176,86,65,0,0,0,0,104,145,89,65,111,18,131,58,98,147,24,142,65,10,215,163,61,240,167,172,65,182,243,21,64,145,237,192,65,197,32,180,64,108,250,
            126,233,65,23,217,14,63,108,45,178,234,65,131,192,102,65,108,0,0,172,65,90,100,101,65,108,168,198,170,65,90,100,101,65,108,168,198,170,65,66,96,101,65,108,104,145,119,65,29,90,100,65,108,63,53,160,65,6,129,27,65,99,101,0,0 };
        
        p.loadPathFromData(x, sizeof(x));
        return p;
    }
    LOAD_EPATH_IF_URL("retry", EditorIcons::redoIcon);
	LOAD_EPATH_IF_URL("close", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
	LOAD_EPATH_IF_URL("help", MainToolbarIcons::help);
    LOAD_EPATH_IF_URL("add", HiBinaryData::ProcessorEditorHeaderIcons::addIcon);
	LOAD_EPATH_IF_URL("edit", EditorIcons::penShape);

	return p;
}

void Dialog::setProperty(const Identifier& id, const var& newValue)
{
	properties.getDynamicObject()->setProperty(id, newValue);
}

void Dialog::setStyleData(const MarkdownLayout::StyleData& sd)
{
	styleData = sd;
	repaint();
	//errorComponent.parser.setStyleData(sd);
}

bool Dialog::navigate(bool forward)
{
    ScopedValueSetter<bool> svs(currentNavigation, forward);
    currentErrorElement = nullptr;
    //errorComponent.setError(Result::ok());
	repaint();
	auto newIndex = jlimit(0, pages.size(), runThread->currentPageIndex + (forward ? 1 : -1));
        
    if(!forward)
        nextButton.setEnabled(true);
    
	prevButton.setEnabled(newIndex != 0);
        
	if(isPositiveAndBelow(newIndex, pages.size()+1))
	{
		if(forward && currentPage != nullptr)
		{
			if(!isEditModeEnabled())
			{
				try
				{
					auto ok = currentPage->check(runThread->globalState);
					//errorComponent.setError(ok);
		                
					if(!ok.wasOk())
					{
						if(currentErrorElement != nullptr)
						{
							currentErrorElement->setModalHelp(ok.getErrorMessage());
						}
							
						return false;
					}
						
				}
				catch(State::CallOnNextAction& cona)
				{
					nextButton.setEnabled(false);
					prevButton.setEnabled(false);
					return false;
				}
			}
		}

		if (newIndex == pages.size())
		{
			if(!editMode && finishCallback)
				Timer::callAfterDelay(600, finishCallback);
				
			return true;
		}

		runThread->currentPageIndex = newIndex;
		
		return refreshCurrentPage();
	}

	

	return false;
}

void Dialog::paint(Graphics& g)
{
    Rectangle<int> errorBounds;
    
    if(currentErrorElement != nullptr)
        errorBounds = this->getLocalArea(currentErrorElement, currentErrorElement->getLocalBounds()).expanded(5, 2);
    
    if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
	{
        laf->drawMultiPageBackground(g, *this, errorBounds);
		laf->drawMultiPageHeader(g, *this, top);
		laf->drawMultiPageButtonTab(g, *this, bottom);
	}

	if(editMode)
	{
		GlobalHiseLookAndFeel::draw1PixelGrid(g, this, getLocalBounds(), Colour(0x44999999));
        
        g.setColour(Colours::white.withAlpha(0.1f));
        g.setFont(GLOBAL_MONOSPACE_FONT());
	}

	if(currentlyEditedPage != nullptr)
	{
		auto b = getLocalArea(currentlyEditedPage, currentlyEditedPage->getLocalBounds()).expanded(2.0f);
		g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));
		g.drawRoundedRectangle(b.toFloat(), 3.0f, 1.0f);
	}
}

void Dialog::resized()
{
	popup.setBounds(getLocalBounds());

	PositionInfo position;

	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
    {
        var pd;
        
        if(auto cp = pages[runThread->currentPageIndex])
            pd = cp->getData();
        
        position = laf->getMultiPagePositionInfo(pd);
    }
        
	auto b = getLocalBounds().reduced(position.OuterPadding);
        
	top = b.removeFromTop(position.TopHeight);

	

	b.removeFromTop(position.OuterPadding);
	bottom = b.removeFromBottom(position.ButtonTab);
	b.removeFromBottom(position.OuterPadding);
	center = b;
        
	auto copy = bottom;
        
	bottom = bottom.expanded(position.OuterPadding);

	cancelButton.setBounds(copy.removeFromLeft(140).reduced(position.ButtonMargin));
    
	nextButton.setBounds(copy.removeFromRight(140).reduced(position.ButtonMargin));
	prevButton.setBounds(copy.removeFromRight(140).reduced(position.ButtonMargin));

	//prevButton.setVisible(pageListInfo.size() > 1);

	copy = center;

#if 0
    if(errorComponent.isVisible())
    {
        errorComponent.setBounds(copy.removeFromTop(errorComponent.height));
        copy.removeFromTop(position.OuterPadding);
    }
    else
        errorComponent.setBounds(copy.removeFromTop(0));
#endif
        
	content.setBounds(copy);
    
    if(currentPage != nullptr)
    {
        currentPage->setSize(content.getWidth() - (content.getVerticalScrollBar().isVisible() ? (content.getScrollBarThickness() + 10) : 0), currentPage->getHeight());
        
    }

	auto topCopy = top;

}


std::unique_ptr<ComponentTraverser> Dialog::createKeyboardFocusTraverser()
{
	return std::make_unique<TabTraverser>(*this);
}

void Dialog::setEditMode(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	editMode = isEditMode;
	editModeBroadcaster.sendMessage(sendNotificationSync, editMode);

	currentPage = nullptr;

	runThread->currentPageIndex--;
	navigate(true);
	
	repaint();
#endif
}

bool Dialog::isEditModeEnabled() const noexcept
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	return editMode;
#else
	return false;
#endif
}

String Dialog::getExistingKeysAsItemString() const
{
	String x;
	const auto& props = runThread->globalState.getDynamicObject()->getProperties();
	for(auto& nv: props)
		x << nv.name << "\n";
        
	return x;
}


void Dialog::containerPopup(const var& infoObject)
{
	multipage::Factory factory;
	
	auto tp = findPageBaseForInfoObject(infoObject);

	PopupLookAndFeel plaf;
	    
	auto list = factory.getPopupMenuList();
	
	PopupMenu m = SubmenuComboBox::parseFromStringArray(list, {}, &plaf);
	
	PopupMenu::MenuItemIterator iter(m, true);
	
	while(iter.next())
	{
		auto& item = iter.getItem();
		
		auto p = factory.createPath(item.text);

		if(!p.isEmpty())
		{
			auto dp = new juce::DrawablePath();
			dp->setPath(p);
			item.image.reset(dp);
		}
	}

	auto typeName = infoObject[mpid::Type].toString();

	m.addSeparator();
	m.addItem(90000, "Edit " + typeName, tp != nullptr, tp == currentlyEditedPage);
	m.addItem(924, "Delete " + typeName, tp != nullptr && tp->findParentComponentOfClass<factory::Container>() != nullptr);

	if(auto r = m.show())
	{
		if(r == 90000)
		{
			createEditorInSideTab(infoObject, tp, [tp](PageInfo& editorList)
			{
				tp->createEditor(editorList);
			});

#if 0
			auto& x = createModalPopup<factory::List>();
			x.setStateObject(infoObject);
			tp->createEditor(x);
				
			x.setCustomCheckFunction([tp](Dialog::PageBase* p, const var& obj){ return dynamic_cast<factory::Container*>(tp)->customCheckOnAdd(p, obj); });

			showModalPopup(true);
#endif
		}
		else if (r == 924)
		{
			tp->deleteFromParent();
		}
		else
		{
			PopupMenu::MenuItemIterator it(m, true);

			while(it.next())
			{
				auto& item = it.getItem();

				if(item.itemID == r)
				{
					auto typeId = item.text.fromLastOccurrenceOf("::", false, false);
					auto catId = item.text.upToLastOccurrenceOf("::", false, false);

					auto no = new DynamicObject();
					no->setProperty(mpid::Type, typeId);
					no->setProperty(mpid::Text, "LabelText");

					if(factory.needsIdAtCreation(typeId))
					{
						no->setProperty(mpid::ID, typeId + "Id");
					}

					if(!infoObject[mpid::Children].isArray())
					{
						Array<var> newList;
						infoObject.getDynamicObject()->setProperty(mpid::Children, var(newList));
					}

					auto childList = infoObject[mpid::Children];

					getUndoManager().perform(new UndoableVarAction(childList, childList.size(), var(no)));
					refreshCurrentPage();

					Component::callRecursive<PageBase>(this, [this, no](PageBase* pb)
					{
						if(pb->matches(var(no)))
						{
							this->createEditorInSideTab(var(no), pb, [pb](PageInfo& editorList)
							{
								pb->createEditor(editorList);
							});

							return true;
						}

						return false;
					});
				}
			}
		}
	}
}

bool Dialog::nonContainerPopup(const var& infoObject)
{
	auto tp = findPageBaseForInfoObject(infoObject);

	auto typeName = infoObject[mpid::Type].toString();

	PopupLookAndFeel plaf;
	PopupMenu m;
	m.setLookAndFeel(&plaf);
	m.addItem(2, "Edit " + typeName, true, currentlyEditedPage == tp);
	m.addItem(1, "Delete " + typeName);
	m.addSeparator();
	m.addItem(3, "Duplicate " + typeName);
	m.addItem(4, "Copy " + typeName);

	auto clipboard = JSON::parse(SystemClipboard::getTextFromClipboard());

	if(clipboard.getDynamicObject() != nullptr)
		m.addItem(5, "Paste " + clipboard[mpid::Type].toString());

	const auto r = m.show();

	if(r == 0)
		return true;

    if(r == 2 && tp != nullptr)
    {
        showEditor(infoObject);
        return true;
    }
	if(r == 1 && tp != nullptr)
	{
		tp->deleteFromParent();
		return true;
	}
	if(r == 3)
	{
		tp->duplicateInParent();
		return true;
	}
	if(r == 4)
	{
		SystemClipboard::copyTextToClipboard(JSON::toString(infoObject, true));
		return true;
	}
	if(r == 5)
	{
		if(auto pc = tp->findParentComponentOfClass<factory::Container>())
		{
			auto l = pc-> getPropertyFromInfoObject(mpid::Children);
			auto idx = l.indexOf(infoObject)+1;
			getUndoManager().perform(new UndoableVarAction(l, idx, clipboard));
			refreshCurrentPage();
		}

		return true;
	}

	return false;
}

bool Dialog::showEditor(const var& infoObject)
{
	auto tp = findPageBaseForInfoObject(infoObject);

	if(tp == nullptr)
		return false;

	if(tp != nullptr)
	{
		createEditorInSideTab(infoObject, tp, [tp](PageInfo& editorList)
		{
			tp->createEditor(editorList);
		});

		return true;
		auto& l = createModalPopup<factory::List>({
	        { mpid::Padding, 10 }
		});

		

	    l.setStateObject(infoObject);
	    tp->createEditor(l);

		if(auto lc = dynamic_cast<factory::LabelledComponent*>(tp))
		{
			l.setCustomCheckFunction([lc](PageBase* b, var obj)
		    {
				auto ok = lc->loadFromInfoObject(obj);
				lc->repaint();
		        return ok;
		    });
		}

	    showModalPopup(true);
	}

	return true;
}

void Dialog::gotoPage(int newIndex)
{
	if(isPositiveAndBelow(newIndex, pageListInfo->size()))
	{
		getState().currentPageIndex = newIndex;
		refreshCurrentPage();
	}
}
}
}
