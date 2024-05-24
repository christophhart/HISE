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
		findParentComponentOfClass<PageBase>()->setModalHelp(help);

#if 0
		if(findParentComponentOfClass<Dialog::ModalPopup>() != nullptr || findParentComponentOfClass<Dialog>()->useHelpBubble)
		{
			
		}
		else
		{
            auto pc = getParentComponent();
			auto img = pc->createComponentSnapshot(pc->getLocalBounds(), true);

			auto mp = findParentComponentOfClass<Dialog>();
			mp->createModalPopup<factory::MarkdownText>()[mpid::Text] = help;
			mp->showModalPopup(false, img);
		}
#endif
	};
}



struct Dialog::PageBase::ModalHelp: public simple_css::FlexboxComponent
{
    ModalHelp(const String& text, PageBase& parent):
	  FlexboxComponent(simple_css::Selector(".help-popup")),
	  textDisplay(),
      closeButton("close", nullptr, parent.rootDialog)
    {

        //r.setStyleData(parent.rootDialog.styleData);
        closeButton.onClick = [&parent]()
        {
            parent.setModalHelp("");
        };

		Helpers::writeSelectorsToProperties(textDisplay, { ".help-text"});
		Helpers::writeSelectorsToProperties(closeButton, { ".help-close"});

		Helpers::setFallbackStyleSheet(textDisplay, "width: 100%;");
		
        //r.parse();

		//auto h = r.getHeightForWidth((float)parent.getWidth() - 26.0f);

		textDisplay.setResizeToFit(true);
		textDisplay.setText(text);
		

		addFlexItem(textDisplay);
		addFlexItem(closeButton);

		auto p = CSSRootComponent::find(parent);

		setParent(p);
		setCSS(p->css);
		
        auto w = jmax(400, parent.getWidth() + 20);
        
		setSize(w, 0);
    	setSize(w, getAutoHeightForWidth(w));
		
    };

#if 0
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
#endif

	SimpleMarkdownDisplay textDisplay;

    //MarkdownRenderer r;
    HiseShapeButton closeButton;
};

Dialog::PageBase::PageBase(Dialog& rootDialog_, int width, const var& obj):
  FlexboxComponent(simple_css::Selector("#" + obj[mpid::ID].toString())),
  rootDialog(rootDialog_),
  infoObject(obj)
{
    stateObject = rootDialog.getState().globalState;

    updateStyleSheetInfo();

	auto help = obj[mpid::Help].toString();

	if(help.isNotEmpty())
	{
		helpButton = new HelpButton(help, rootDialog);
		addFlexItem(*helpButton);

		Helpers::writeSelectorsToProperties(*helpButton, { ".help-button"} );
		Helpers::setFallbackStyleSheet(*helpButton, "order: 1000; height: 24px; width: 32px;");

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

void Dialog::PageBase::updateStyleSheetInfo(bool forceUpdate)
{
	Component* componentToUse = !isInvisibleWrapper() ? this : getChildComponent(0);

	if(componentToUse == nullptr)
		return;

	auto isFirst = classHash == 0 && styleHash == 0;
	auto classList = infoObject[mpid::Class].toString();
	auto newHash = classList.isNotEmpty() ? classList.hashCode() : 0;
	auto didSomething = false;

	if(newHash != classHash || forceUpdate)
	{
		classHash = newHash;
		auto sa = StringArray::fromTokens(classList, " ", "");
		sa.removeEmptyStrings();

		for(auto& s: sa)
		{
			if(!s.startsWithChar('.'))
				s = "." + s;
		}

		Helpers::writeSelectorsToProperties(*componentToUse, sa);
		didSomething = true;
	}

	auto inlineStyle = infoObject[mpid::Style].toString();

	newHash = inlineStyle.isNotEmpty() ? inlineStyle.hashCode() : 0;

	if(newHash != styleHash || forceUpdate)
	{
		styleHash = newHash;
		Helpers::writeInlineStyle(*componentToUse, inlineStyle);
		didSomething = true;
	}

	if(!isFirst && didSomething)
	{
		Component::callRecursive<Component>(this, [](Component* c)
		{
			Helpers::invalidateCache(*c);
			return false;
		});

		// rebuild to allow resizing...
		if(!rootDialog.getSkipRebuildFlag())
			rootDialog.body.setCSS(rootDialog.css);
	}
}

void Dialog::PageBase::forwardInlineStyleToChildren()
{
	auto inlineStyle = simple_css::FlexboxComponent::Helpers::getInlineStyle(*this);

	if(inlineStyle.isNotEmpty())
	{
		for(int i = 0; i < getNumChildComponents(); i++)
			simple_css::FlexboxComponent::Helpers::writeInlineStyle(*getChildComponent(i), inlineStyle);

		simple_css::FlexboxComponent::Helpers::writeInlineStyle(*this, "");
	}
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
	auto popup = TopLevelWindowWithOptionalOpenGL::findRoot(this);

	if(popup == nullptr)
		popup = getTopLevelComponent();

	Component::callRecursive<PageBase>(popup, [](PageBase* b)
	{
		b->modalHelp = nullptr;
		return false;
	});

	if(text.isEmpty())
		return;

	jassert(popup != nullptr);

	popup->addAndMakeVisible(modalHelp = new ModalHelp(text, *this));

#if JUCE_DEBUG
	modalHelp->setVisible(true);
#else
	Desktop::getInstance().getAnimator().fadeIn(modalHelp, 200);
#endif

	modalHelp->toFront(false);

	auto thisBounds = popup->getLocalArea(this, getLocalBounds());

	auto mb = thisBounds.withSizeKeepingCentre(jmax(500, modalHelp->getWidth()), modalHelp->getHeight());
	mb.setY(thisBounds.getBottom() + 3);
	
	mb = mb.constrainedWithin(popup->getLocalBounds());
    
	modalHelp->setBounds(mb);
}



void Dialog::PageBase::onEditModeChange(PageBase& c, bool isOn)
{
	c.editModeChanged(isOn);
}



bool Dialog::PageBase::showDeletePopup(bool isRightClick)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	if(isRightClick)
	{
		return rootDialog.nonContainerPopup(infoObject);
	}

	return rootDialog.showEditor(infoObject);
#else
	return false;
#endif
	
}

String Dialog::PageBase::evaluate(const Identifier& id) const
{
	return factory::MarkdownText::getString(infoObject[id].toString(), rootDialog);
}

void Dialog::PageBase::callOnValueChange(const String& eventType, DynamicObject::Ptr thisObject)
{
	callAdditionalStateCallback();

	auto state = &rootDialog.getState();
		    
	if(auto ms = findParentComponentOfClass<ComponentWithSideTab>())
		state = ms->getMainState();

	engine = state->createJavascriptEngine();

	if(engine != nullptr && (infoObject[mpid::UseOnValue] || !eventListeners.isEmpty()))
	{
		auto ok = Result::ok();
		
		ApiObject::ScopedThisSetter sts(*state, thisObject == nullptr ? new Element(*state, infoObject) : thisObject);

		auto code = infoObject[mpid::Code].toString();

		if(code.isNotEmpty() && infoObject[mpid::UseOnValue])
			engine->evaluate(code, &ok);

		for(auto& v: eventListeners)
		{
			if(v.first == eventType)
			engine->callFunctionObject(sts.getThisObject(), v.second, var::NativeFunctionArgs(var(sts.getThisObject()), nullptr, 0), &ok);

			if(ok.failed())
				break;
		}

		if(ok.failed())
		{
			rootDialog.setCurrentErrorPage(this);
			setModalHelp(ok.getErrorMessage());
		}
	}
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

	updateStyleSheetInfo(false);

	if(!initValue.isUndefined() && !initValue.isVoid())
	{
		writeState(initValue);
		initValue = var();
	}
}


void Dialog::PageBase::editModeChanged(bool isEditMode)
{
	repaint();
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
		return rootDialog.getState().loadText(s, true);
	}

	return s;
}

StringArray Dialog::PageBase::getItemsAsStringArray() const
{
	auto itemList = infoObject[mpid::Items];

	StringArray sa;

	if(itemList.isArray())
	{
		for(auto& v: *itemList.getArray())
		{
			sa.add(v.toString().unquoted().trim());
		}
	}
	else
	{
		sa = StringArray::fromLines(itemList.toString());

		for(auto& s: sa)
			s = s.trim().unquoted();
	}

	sa.removeEmptyStrings();
	return sa;
}

Result Dialog::PageBase::check(const var& obj)
{
	if(cf)
	{
		auto ok = cf(this, obj);
                
		if(ok.failed())
		{
			rootDialog.setCurrentErrorPage(this);
			return ok;
		}
	}

	auto ok = checkGlobalState(obj);

	if(ok.failed() && rootDialog.currentErrorElement == nullptr)
		rootDialog.setCurrentErrorPage(this);
		

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


ComponentWithSideTab::ComponentWithSideTab()
{
	propertyStyleSheet = DefaultCSSFactory::getTemplateCollection(DefaultCSSFactory::Template::PropertyEditor);
    	
}

var Dialog::PositionInfo::toJSON() const
{
	auto obj = new DynamicObject();
            
    obj->setProperty(mpid::StyleSheet, styleSheet);
	obj->setProperty(mpid::Style, additionalStyle);
    obj->setProperty(mpid::UseViewport, useViewport);
    
	obj->setProperty("DialogWidth", fixedSize.getX());
	obj->setProperty("DialogHeight", fixedSize.getY());
	
	return var(obj);
}

void Dialog::PositionInfo::fromJSON(const var& obj)
{
    styleSheet = obj.getProperty(mpid::StyleSheet, styleSheet);
	additionalStyle = obj.getProperty(mpid::Style, additionalStyle).toString();

    useViewport = obj.getProperty(mpid::UseViewport, useViewport);
    
	fixedSize.setX(obj.getProperty("DialogWidth", fixedSize.getX()));
	fixedSize.setY(obj.getProperty("DialogHeight", fixedSize.getY()));
}


Rectangle<int> Dialog::PositionInfo::getBounds(Rectangle<int> fullBounds) const
{
	if(fixedSize.isOrigin())
		return fullBounds;
	else
		return fullBounds.withSizeKeepingCentre(fixedSize.getX(), fixedSize.getY());
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




Dialog::ModalPopup::ModalPopup(Dialog& parent_, PageInfo::Ptr info_, bool addButtons):
	FlexboxComponent(simple_css::Selector(".modal-bg")),
	content(simple_css::Selector(".modal-popup")),
	parent(parent_),
	info(info_),
	contentViewport(simple_css::Selector(".modal-content")),
	bottom(simple_css::Selector(".modal-bottom")),
	okButton("OK"),
	cancelButton("Cancel")
{
	setDefaultStyleSheet("position: absolute; background: rgba(128,128,128, 0.8);");
	content.setDefaultStyleSheet("background: #161616;display:flex;width: 100%;flex-direction: column;margin: 120px 90px;padding: 20px;");
	contentViewport.setDefaultStyleSheet("display: flex;flex-direction: row;width: 100%;flex-grow: 1;");

	okButton.setVisible(addButtons);
	cancelButton.setVisible(addButtons);

	addFlexItem(content);
	content.addFlexItem(contentViewport);

	content.addMouseListener(this, false);

	if(addButtons)
	{
		Helpers::writeSelectorsToProperties(okButton, { ".modal-button", "#modal-ok"});
		Helpers::writeSelectorsToProperties(cancelButton, { ".modal-button", "#modal-cancel"});
		content.addFlexItem(bottom);
		bottom.addFlexItem(okButton);
		bottom.addSpacer();
		bottom.addFlexItem(cancelButton);
		bottom.setDefaultStyleSheet("width: 100%;height: auto;");
	}
	
	okButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::onOk);
	cancelButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::dismiss);
}

void Dialog::ModalPopup::init()
{
	if(info != nullptr)
	{
		contentComponent = info->create(parent, parent.getWidth());
		contentViewport.addFlexItem(*contentComponent);
		contentComponent->postInit();
	}
}

void Dialog::ModalPopup::onOk()
{
	if(contentComponent != nullptr)
	{
		auto obj = info->stateObject;

		if(!obj.isObject())
			obj = getGlobalState(*this, {}, var());

		auto r = contentComponent->check(obj);

		if(!r.wasOk())
		{
			return;
		}

		dismiss();
	}
}

void Dialog::ModalPopup::dismiss()
{
	parent.setCurrentErrorPage(nullptr);

#if JUCE_DEBUG
	setVisible(false);
#else
	Desktop::getInstance().getAnimator().fadeOut(this, 100);
#endif
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

	return false;
}

void Dialog::ModalPopup::mouseDown(const MouseEvent& e)
{
	dismiss();
}

#if 0
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

#endif

Dialog::Dialog(const var& obj, State& rt, bool addEmptyPage):
	HeaderContentFooter(obj[mpid::LayoutData].getProperty(mpid::UseViewport, true)),
	cancelButton("Cancel"),
	nextButton("Next"),
	prevButton("Previous"),
#if HISE_MULTIPAGE_INCLUDE_EDIT
	mouseSelector(*this),
#endif
	//errorComponent(*this),
    runThread(&rt),
	totalProgress(progressValue)
{
	jassert(runThread->currentDialog == nullptr);

    runThread->currentDialog = this;

	if(auto sd = obj[mpid::StyleData].getDynamicObject())
	{

		styleData.fromDynamicObject(var(sd), std::bind(&State::loadFont, &getState(), std::placeholders::_1));
	}
	else
	{
		styleData = MarkdownLayout::StyleData::createDarkStyle();
		styleData.backgroundColour = Colour(0xFF333333);
	}

	auto defaultValues = styleData.toDynamicObject(true);
	setDefaultCSSProperties(defaultValues.getDynamicObject());

	//errorComponent.parser.setStyleData(styleData);

	addChildComponent(popup);

	totalProgress.setOpaque(true);

	if(auto ld = obj[mpid::LayoutData].getDynamicObject())
		positionInfo.fromJSON(var(ld));

	

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
		po->setProperty(mpid::BinaryName, "My Binary");
		po->setProperty(mpid::Icon, "");
		po->setProperty(mpid::UseGlobalAppData, false);
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
		    pageListInfo->add(var(fc));
		}
    }

	rebuildPagesFromJSON();

	header.addTextElement({ "#title"}, properties[mpid::Header].toString());
	header.addTextElement({ "#subtitle" }, properties[mpid::Subtitle].toString() );
	header.addFlexItem(totalProgress);
	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(totalProgress, { "#total-progress" });




	footer.addFlexItem(cancelButton);
	footer.addSpacer();
	footer.addFlexItem(prevButton);
	footer.addFlexItem(nextButton);

	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(cancelButton, { "#cancel", ".nav-button" });
	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(nextButton, { "#next", ".nav-button" });
	simple_css::FlexboxComponent::Helpers::writeSelectorsToProperties(prevButton, { "#prev", ".nav-button" });

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
		auto l = createModalPopup<factory::List>();

		auto& root = *l;
        auto& md = root.addChild<factory::MarkdownText>();
		md[mpid::Text] = "Do you want to close this popup?";

		md.setCustomCheckFunction([this](PageBase*, var obj)
		{
			MessageManager::callAsync(finishCallback);
			return Result::ok();
		});
		
		showModalPopup(true, l);
	};

#if HISE_MULTIPAGE_INCLUDE_EDIT
	selectionUpdater.addListener(*this, [](Dialog& b, const Array<var>& list)
	{
		b.showEditor(list);
	}, false);
#endif

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
	popup = nullptr;

	auto index = jlimit(0, pages.size()-1, getState().currentPageIndex);

	String pt;
	pt << "Step " << String(index+1) << " / " << String(pages.size());


	if(pages.size() > 1)
		progressValue = (double)index / (double)(pages.size() - 1);

	totalProgress.setTextToDisplay(pt);

	css.clearCache();

	logMessage(MessageType::Navigation, "Goto page " + String(index+1));

	if((currentPage = pages[index]->create(*this, dynamic_cast<Component*>(content.get())->getWidth())))
	{
		content->addFlexItem(*currentPage);

		
		currentPage->postInit();
		//currentPage->setLookAndFeel(&getLookAndFeel());
		nextButton.setButtonText(runThread->currentPageIndex == pages.size() - 1 ? "Finish" : "Next");
		refreshBroadcaster.sendMessage(sendNotificationSync, index);

		update(css);
		
		
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

		d->setFixStyleSheet(st->propertyStyleSheet);

		d->additionalChangeCallback = [this]()
		{
			this->refreshCurrentPage();
		};

		d->useHelpBubble = true;

		auto sd = d->getStyleData();
		sd.fontSize = 14.0f;
		sd.f = GLOBAL_FONT();
		sd.textColour = Colours::white.withAlpha(0.7f);
		sd.backgroundColour = Colours::transparentBlack;
		d->setStyleData(sd);

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

void Dialog::showModalPopup(bool addButtons, PageInfo::Ptr content)
{
	popup = new ModalPopup(*this, content, addButtons);
	popup->setVisible(true);
	body.addFlexItem(*popup);
	popup->init();
	body.setCSS(css);
	
}

Result Dialog::checkCurrentPage()
{
	if(currentPage != nullptr)
	{
		setCurrentErrorPage(nullptr);

		auto ok = currentPage->check(runThread->globalState);

		//errorComponent.setError(ok);

		repaint();
		return ok;
	}

	return Result::fail("No page");
}

void Dialog::setCurrentErrorPage(PageBase* b)
{
	if(currentErrorElement == b)
		return;

	if(currentErrorElement != nullptr)
	{
		currentErrorElement->changeClass(simple_css::Selector(".error"), false);
	}

	currentErrorElement = b;

	if(currentErrorElement != nullptr)
		currentErrorElement->changeClass(simple_css::Selector(".error"), true);
	
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

struct VarIterator
{
	static bool forEach(var& obj, const std::function<bool(var&)>& f)
	{
		if(f(obj))
			return true;

		if(auto ar = obj.getArray())
		{
			for(auto& v: *ar)
			{
				if(forEach(v, f))
					return true;
			}
		}

		if(auto o = obj.getDynamicObject())
		{
			for(auto& p: o->getProperties())
			{
				auto v = p.value;
				if(forEach(v, f))
					return true;
			}
		}

		return false;
	}
};

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

	json->setProperty(mpid::StyleData, sd);
	json->setProperty(mpid::Properties, properties);
	json->setProperty(mpid::LayoutData, positionInfo.toJSON());
	json->setProperty(mpid::GlobalState, runThread->globalState);
	json->setProperty(mpid::Children, pageListArrayAsVar);
	json->setProperty(mpid::Assets, runThread->exportAssetsAsJSON(false));

	auto copy = var(json.get()).clone();

	VarIterator::forEach(copy, [](var& v)
	{
		if(auto o = v.getDynamicObject())
			o->removeProperty("onValue");

		return false;
	});

	return copy;
}

Dialog::PageBase* Dialog::findPageBaseForID(const String& id)
{
	PageBase* found = nullptr;

	callRecursive<PageBase>(this, [&](PageBase* b)
	{
		if(b->getId().toString() == id)
		{
			found = b;
			return true;
		}

		return false;
	});

	return found;
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
	return positionInfo;
}

void Dialog::showMainPropertyEditor()
{
	using namespace factory;

	auto no = new DynamicObject();

	createEditorInSideTab(var(no), nullptr, [this](PageInfo& introPage)
	{
	    auto& propList = introPage.addChild<List>({
            { mpid::UseChildState, true },
            { mpid::Text, "Project Properties" },
            { mpid::Foldable, true },
            { mpid::Folded, true },
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

		col0.addChild<Choice>({
	        { mpid::ID, mpid::Image.toString() },
	        { mpid::Text, mpid::Image.toString() },
	        { mpid::Help, "The background image used for the dialog (will be scaled to fit the entire area)." },
			{ mpid::Value, properties[mpid::Image] },
			{ mpid::Items, getState().getAssetReferenceList(Asset::Type::Image) }
	    });

		auto& col1 = propList;//

		col1.addChild<TextInput>({
	        { mpid::ID, mpid::ProjectName.toString() },
	        { mpid::Text, mpid::ProjectName.toString() },
			{ mpid::Value, properties[mpid::ProjectName] },
			{ mpid::Help, "Your project ID. If you're using some high level actions this must be the same as your plugin" }
	    });

		col1.addChild<TextInput>({
	        { mpid::ID, mpid::Company.toString() },
	        { mpid::Text, mpid::Company.toString() },
			{ mpid::Value, properties[mpid::Company] },
			{ mpid::Help, "Your company ID" }
	    });

		col1.addChild<TextInput>({
	        { mpid::ID, mpid::Version.toString() },
	        { mpid::Text, mpid::Version.toString() },
			{ mpid::Value, properties[mpid::Version] },
			{ mpid::Help, "The project version that can be queried with the ProjectInfo constants"}
		});

		col1.addChild<TextInput>({
	        { mpid::ID, mpid::BinaryName.toString() },
	        { mpid::Text, mpid::BinaryName.toString() },
			{ mpid::Value, properties[mpid::BinaryName] },
			{ mpid::Help, "The filename of the compiled executable (without the OS dependent file extension like `.exe`)." }
	    });

		col1.addChild<Button>({
	        { mpid::ID, mpid::UseGlobalAppData.toString() },
	        { mpid::Text, mpid::UseGlobalAppData.toString() },
			{ mpid::Value, properties[mpid::UseGlobalAppData] },
			{ mpid::Help, "Whether to use the global app data or the user app data directory.  \n> This setting must be consistent with your project's setting in order to work correctly." }
	    });

		col1.addChild<Choice>({
	        { mpid::ID, mpid::Icon.toString() },
	        { mpid::Text, mpid::Icon.toString() },
			{ mpid::Items, getState().getAssetReferenceList(Asset::Type::Image) },
			{ mpid::Value,  properties[mpid::Icon] },
			{ mpid::Help, "The image asset that should be used as icon" }
	    });

		{
	        auto& layoutProperties = introPage.addChild<List>({
	            { mpid::ID, mpid::LayoutData.toString() },
	            { mpid::Text, "CSS Styling" },
                { mpid::UseChildState, true },
	            { mpid::Foldable, true },
	            { mpid::Folded, true }
	        });

			auto layoutObj = positionInfo.toJSON();


			int numInCol = 0;

	        for(auto& v: layoutObj.getDynamicObject()->getProperties())
	        {
                if(v.name == mpid::UseViewport)
                {
                    layoutProperties.addChild<Button>({
                        { mpid::ID, mpid::UseViewport.toString() },
                        { mpid::Text, mpid::UseViewport.toString() },
                        { mpid::Value, v.value },
                        { mpid::Help, "Whether to put the content into a scrollable viewport or not (requires reload)." }
                    });
                }
                else if(v.name == mpid::StyleSheet)
                {
                    auto items = getState().getAssetReferenceList(Asset::Type::Stylesheet);
                    
                    items << DefaultCSSFactory::getTemplateList();
                    
                    layoutProperties.addChild<Choice>({
                        { mpid::ID, v.name.toString() },
                        { mpid::Text, v.name.toString() },
                        { mpid::Items, items },
                        { mpid::Value, v.value }
                    });
                }
				else if (v.name == mpid::Style)
				{
					layoutProperties.addChild<CodeEditor>({
						{ mpid::ID, "Style" },
						{ mpid::Text, "Style" },
						{ mpid::Syntax, "CSS" },
						{ mpid::Value, v.value }
					});
				}
                else
                {
                    layoutProperties.addChild<TextInput>({
                        { mpid::ID, v.name.toString() },
                        { mpid::Text, v.name.toString() },
                        { mpid::Value, v.value }
                    });
                }
                

				numInCol++;
	        }
	    }

	    auto& styleProperties = introPage.addChild<List>({
	        { mpid::ID, mpid::StyleData.toString(), },
	        { mpid::Text, "Text & Colour", },
            { mpid::UseChildState, true },
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

	    for(auto& nv: sdData.getDynamicObject()->getProperties())
	    {
	        if(hiddenProps.contains(nv.name))
	            continue;

	        if(nv.name.toString().contains("Colour"))
	        {
				styleProperties.addChild<ColourChooser>({
	                { mpid::ID, nv.name.toString() },
	                { mpid::Text, nv.name.toString() },
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

		introPage.setCustomCheckFunction([&](PageBase* b, var obj)
		{
			Container::checkChildren(b, obj);

			properties = obj[mpid::Properties].clone();
			styleData.fromDynamicObject(obj[mpid::StyleData], std::bind(&State::loadFont, &getState(), std::placeholders::_1));

			auto defaultValues = styleData.toDynamicObject(true);

			setDefaultCSSProperties(defaultValues.getDynamicObject());

			positionInfo.fromJSON(obj[mpid::LayoutData]);

			loadStyleFromPositionInfo();

			resized();
			repaint();
			
			return Result::ok();
		});
	});
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
Dialog::MouseSelector::MouseSelector(Dialog& parent_):
    parent(parent_)
{
	parent.addMouseListener(this, true);
	

	selection.addChangeListener(this);
}

Dialog::MouseSelector::~MouseSelector()
{
	selection.removeChangeListener(this);
}

void Dialog::MouseSelector::changeListenerCallback(ChangeBroadcaster* source)
{
	DBG("Rebuild!!!");

	const auto& list = selection.getItemArray();

	uint64 thisHash = 0;

	for(auto& l: list)
	{
		thisHash += reinterpret_cast<uint64>(l.getObject());
	}

	if(thisHash != lastHash)
	{
		lastHash = thisHash;
		parent.selectionUpdater.sendMessage(sendNotificationSync, list);
	}
}

void Dialog::MouseSelector::findLassoItemsInArea(Array<var>& itemsFound, const Rectangle<int>& area)
{
	parent.currentPage->callRecursive<PageBase>(parent.currentPage, [&](PageBase* b)
	{
		auto la = parent.getLocalArea(b, b->getLocalBounds());

		if(dynamic_cast<factory::Container*>(b) != nullptr)
			return false;

		if(area.intersectRectangle(la))
		{
			struct Sorter
			{
				static int compareElements(const var& b1, const var& b2)
				{
					return b1[mpid::ID].toString().compare(b2[mpid::ID].toString());
				}
			} sorter;

			itemsFound.addSorted(sorter, b->getInfoObject());
		}

		return false;
	});
}

void Dialog::MouseSelector::mouseDrag(const MouseEvent& e)
{
	if(parent.isEditModeEnabled())
	{
		lasso.dragLasso(e.getEventRelativeTo(&parent));
	}
}

void Dialog::MouseSelector::mouseUp(const MouseEvent& e)
{
	if(parent.isEditModeEnabled())
	{
		lasso.endLasso();
	}
}

void Dialog::MouseSelector::mouseDown(const MouseEvent& event)
{
	if(parent.isEditModeEnabled())
	{
		selection.deselectAll();

		auto pb = event.eventComponent->findParentComponentOfClass<PageBase>();

		if(pb != nullptr)
		{
			selection.addToSelectionBasedOnModifiers(pb->getInfoObject(), event.mods);
		}

		parent.addChildComponent(lasso);
		lasso.beginLasso(event.getEventRelativeTo(&parent), this);
	}
}
#endif

void Dialog::loadStyleFromPositionInfo()
{
	auto css = getState().getStyleSheet(positionInfo.styleSheet, positionInfo.additionalStyle);
	
	update(css);
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

void Dialog::rebuildPagesFromJSON()
{
	pages.clear();

	Factory factory;

    for(const auto& p: *pageListArrayAsVar.getArray())
    {
        if(auto pi = factory.create(p))
        {
            pi->setStateObject(getState().globalState);
            pi->useGlobalStateObject = true;
            pages.add(std::move(pi));
        }
    }

	
}

Result Dialog::getCurrentResult()
{ return runThread->currentError; }

void Dialog::showFirstPage()
{
	loadStyleFromPositionInfo();
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

	if(url == "stop")
	{
		static const unsigned char x[] = { 110,109,240,151,39,68,8,19,64,67,98,143,50,56,68,252,97,66,67,0,128,69,68,120,6,121,67,0,128,69,68,0,0,158,67,98,0,128,69,68,212,55,192,67,3,156,55,68,188,255,219,67,0,128,38,68,188,255,219,67,98,5,100,21,68,188,255,219,67,0,128,7,68,212,55,192,67,0,
128,7,68,0,0,158,67,98,0,128,7,68,84,144,119,67,5,100,21,68,132,0,64,67,0,128,38,68,132,0,64,67,98,68,186,38,68,132,0,64,67,95,244,38,68,216,1,64,67,240,46,39,68,32,7,64,67,108,120,17,40,68,48,255,88,67,98,214,140,39,68,36,222,88,67,234,6,39,68,244,204,
88,67,0,128,38,68,244,204,88,67,98,0,208,24,68,244,204,88,67,53,179,13,68,222,159,130,67,53,179,13,68,0,0,158,67,98,53,179,13,68,32,96,185,67,0,208,24,68,132,153,207,67,0,128,38,68,132,153,207,67,98,255,47,52,68,132,153,207,67,203,76,63,68,32,96,185,
67,203,76,63,68,0,0,158,67,98,203,76,63,68,244,244,131,67,90,62,53,68,108,39,93,67,26,123,40,68,72,28,89,67,108,240,151,39,68,8,19,64,67,99,109,56,50,50,68,212,155,134,67,108,200,205,26,68,212,155,134,67,108,200,205,26,68,44,100,181,67,108,56,50,50,68,
44,100,181,67,108,56,50,50,68,212,155,134,67,99,101,0,0 };

		p.loadPathFromData(x, sizeof(x));
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

	setCurrentErrorPage(nullptr);
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
				auto hasOnSubmit = callRecursive<PageBase>(currentPage, [](PageBase* b)
				{
					return b->hasOnSubmitEvent();
				});

				auto ok = currentPage->check(runThread->globalState);
		                
				if(!ok.wasOk())
				{
					if(currentErrorElement != nullptr)
					{
						currentErrorElement->setModalHelp(ok.getErrorMessage());
					}
						
					return false;
				}

				if(hasOnSubmit)
				{
					getState().navigateOnFinish = true;
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

#if 0
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
#endif

	if(auto ss = css.getWithAllStates(nullptr, simple_css::ElementType::Body))
	{
		auto c = ss->getColourOrGradient(getLocalBounds().toFloat(), { "background-color", {}}, Colour(0xFF222222));

		if(c.second.getNumColours() != 0)
			g.setGradientFill(c.second);
		else
			g.setColour(c.first);

		g.fillAll();
	}

	
}

void Dialog::resized()
{
	HeaderContentFooter::resized();

#if 0
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

#endif

}


std::unique_ptr<ComponentTraverser> Dialog::createKeyboardFocusTraverser()
{
	return std::make_unique<TabTraverser>(*this);
}

void Dialog::setEditMode(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	editMode = isEditMode;
	//showInfo(isEditMode);

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

#if HISE_MULTIPAGE_INCLUDE_EDIT
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
	m.addItem(90001, "Copy info JSON", tp != nullptr);

	if(auto r = m.show())
	{
		if(r == 90000)
		{
			createEditorInSideTab(infoObject, tp, [tp](PageInfo& editorList)
			{
				tp->createEditor(editorList);
			});
		}
		
		else if (r == 90001)
		{
			SystemClipboard::copyTextToClipboard(JSON::toString(tp->getInfoObject(), false));
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
	m.addItem(1235, "Edit Code", tp->getInfoObject().hasProperty(mpid::Code), false);
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
	else if (r == 1235)
	{
		if(auto st = findParentComponentOfClass<ComponentWithSideTab>())
		{
			st->addCodeEditor(tp->getInfoObject(), mpid::Code);
		}
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

bool Dialog::showEditor(const Array<var>& infoObjects)
{
	if(auto st = findParentComponentOfClass<ComponentWithSideTab>())
	{
		if(infoObjects.isEmpty())
		{
			st->setSideTab(nullptr, nullptr);
		}
		else
		{
			auto ns = new State(var());
			ns->globalState = infoObjects[0];
			auto d = new Dialog(var(), * ns, true);

			d->setFixStyleSheet(st->propertyStyleSheet);

			d->useHelpBubble = true;

			auto sd = d->getStyleData();
			sd.fontSize = 14.0f;
			sd.f = GLOBAL_FONT();
			sd.textColour = Colours::white.withAlpha(0.7f);
			sd.backgroundColour = Colours::transparentBlack;
			d->setStyleData(sd);

			d->prevButton.setVisible(false);
			

			auto tp = findPageBaseForInfoObject(infoObjects[0]);

			tp->createEditor(*d->pages.getFirst());


			d->additionalChangeCallback = [this]()
			{
				this->refreshCurrentPage();
			};

			d->setFinishCallback([this, st]()
			{
				this->refreshCurrentPage();
				st->setSideTab(nullptr, nullptr);
			});

			d->refreshCurrentPage();

			if(infoObjects.size() > 1)
			{
				Array<var> sameObjects;

				auto initValues = infoObjects[0].getDynamicObject()->clone();
				
				auto firstType = infoObjects[0][mpid::Type].toString();

				auto ud = &getUndoManager();

				for(int i = 1; i < infoObjects.size(); i++)
				{
					sameObjects.add(infoObjects[i]);
				}

				if(!sameObjects.isEmpty())
				{
					Component::callRecursive<PageBase>(d, [ud, sameObjects, initValues](PageBase* b)
					{
						b->setCustomCheckFunction([ud, sameObjects, initValues](PageBase* b, const var& obj)
						{
							auto idToWrite = b->getId();

							if(idToWrite.isNull())
								return Result::ok();

							auto valueToWrite = b->getValueFromGlobalState();

							if(initValues->getProperty(idToWrite) == valueToWrite)
								return Result::ok();

							initValues->setProperty(idToWrite, valueToWrite);

							ud->beginNewTransaction();

							for(auto& v: sameObjects)
							{
								if(!v.getDynamicObject()->hasProperty(idToWrite))
									continue;

								ud->perform(new UndoableVarAction(v, idToWrite, valueToWrite));

								//v.getDynamicObject()->setProperty(idToWrite, valueToWrite);
							}

							return Result::ok();
						});

						return false;
					});
				}

				
			}

			

			if(!st->setSideTab(ns, d))
				currentlyEditedPage = nullptr;
		}
		

		//currentlyEditedPage = pb;
	}

	repaint();

	return false;
#if 0
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
	}

	return true;
#endif
}
#endif

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
