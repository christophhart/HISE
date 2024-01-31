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

#include "MultiPageIds.h"
#include "hi_tools/hi_markdown/MarkdownLayout.h"

namespace hise
{

namespace multipage {
using namespace juce;


Dialog::PageBase::PageBase(Dialog& rootDialog_, int width, const var& obj):
  rootDialog(rootDialog_),
  infoObject(obj)
{
    stateObject = rootDialog.getState().globalState;
    
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

	editMode = isEditMode;
	repaint();
}

void Dialog::PageBase::writeState(const var& newValue) const
{
	if(auto o = stateObject.getDynamicObject())
	{
		o->setProperty(id, newValue);
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

	DBG(JSON::toString(obj));

	LabelPosition = ln.indexOf(obj.getProperty(mpid::LabelPosition, ln[LabelPosition]).toString());

	if(!isPositiveAndBelow(LabelPosition, LabelPositioning::numLabelPositionings))
		LabelPosition = (int)LabelPositioning::Left;
}

String CodeGenerator::toString() const
{
	String x = getNewLine();

	x << getNewLine() << "using namespace multipage;";
	x << getNewLine() << "using namespace factory;";
	x << getNewLine() << "auto mp_ = new Dialog({}, state);";
	x << getNewLine() << "auto& mp = *mp_;";

	for(auto& p: data[mpid::Properties].getDynamicObject()->getProperties())
	{
		x << getNewLine() << "mp.setProperty(mpid::" << p.name << ", " << p.value.toString().quoted() << ");";
	}

	if(data[mpid::Children].isArray())
	{
		int idx = 0;
		for(const auto& page: *data[mpid::Children].getArray())
		{
			x << createAddChild("mp", page, "Page", true);

			
		}
	}

	x << getNewLine() << "return mp_;";

	return x;
}

String CodeGenerator::generateRandomId(const String& prefix) const
{
	String x = prefix;

	if(x.isEmpty())
		x = "element";
	
	x << "_";
	x << String(idCounter++);
	
	return x;
}

String CodeGenerator::getNewLine() const
{
	String x = "\n";

	for(int i = 0; i < numTabs; i++)
		x << "\t";

	return x;
}

String CodeGenerator::arrayToCommaString(const var& value)
{
	String s;

	for(int i = 0; i < value.size(); i++)
	{
		s << value[i].toString();

		if(i < (value.size() - 1))
			s << ", ";
	}

	return s.quoted();
}

String CodeGenerator::createAddChild(const String& parentId, const var& childData, const String& itemType, bool attachCustomFunction) const
{
	auto id = childData[mpid::ID].toString();
	
	if(id.isEmpty())
		id = childData[mpid::Type].toString();

	id = generateRandomId(id);

	while(existingVariables.contains(id))
		id = generateRandomId(id);

	existingVariables.add(id);

	String x = getNewLine();

	x << "auto& " << id << " = " << parentId << ".add" << itemType << "<" << childData[mpid::Type].toString() << ">({";

	const auto& prop = childData.getDynamicObject()->getProperties();

	String cp;

	for(auto& nv: prop)
	{
		if(nv.name == mpid::Type)
			continue;

		if(nv.name == mpid::Children)
			continue;

		if(nv.value.toString().isEmpty())
			continue;

		cp << getNewLine() << "  { mpid::" << nv.name << ", ";

		if(nv.value.isString())
			cp << nv.value.toString().quoted().replace("\n", "\\n");
		else if (nv.value.isArray())
		{
			cp << arrayToCommaString(nv.value);
		}
		else
			cp << (int)(nv.value);

		cp << " }";

		cp << ", ";
	}

	x << cp.upToLastOccurrenceOf(", ", false, false);

	x << getNewLine() << "});\n";

	auto children = childData[mpid::Children];

	if(children.isArray())
	{
		for(const auto& v: *children.getArray())
		{
			x << createAddChild(id, v, "Child", false);
		}
	}

	if(attachCustomFunction)
	{
		x << getNewLine() << "// Custom callback for page " << id;
		x << getNewLine() << id << ".setCustomCheckFunction([](Dialog::PageBase* b, const var& obj)";
		x << "{\n" << getNewLine() << "\treturn Result::ok();\n" << getNewLine() << "});";
	}

	return x;
}

EditorOverlay::Updater::Updater(EditorOverlay& parent_, Component* attachedComponent):
	ComponentMovementWatcher(attachedComponent),
	parent(parent_)
{
	auto root = attachedComponent->findParentComponentOfClass<Dialog>();

	root->addChildComponent(&parent);

	

	componentMovedOrResized(true, true);
	componentVisibilityChanged();

	
}

void EditorOverlay::Updater::componentMovedOrResized(bool wasMoved, bool wasResized)
{
	auto root = parent.findParentComponentOfClass<Dialog>();
	auto b = root->getLocalArea(getComponent(), parent.localBoundFunction(getComponent()));
	parent.setBounds(b);
}

EditorOverlay::EditorOverlay(Dialog& parent)
{
	parent.addChildComponent(*this);

	localBoundFunction = [](Component* c){ return c->getLocalBounds().expanded(10); };
	
	setRepaintsOnMouseActivity(true);
}

void EditorOverlay::resized()
{
	outline.clear();

	auto b = getLocalBounds().toFloat().reduced(3.0f);
	outline.addRoundedRectangle(b, 5.0f);

	bounds = b;

	float l[2] = { 4.0f, 4.0f };
	PathStrokeType(1.0f).createDashedStroke(outline, outline, l, 2);
}

void EditorOverlay::mouseUp(const MouseEvent& e)
{
	if(cb)
		cb();
}

void EditorOverlay::paint(Graphics& g)
{
	g.setColour(Colours::white.withAlpha(0.2f));
	g.fillPath(outline);

	float alpha = 0.0f;

	if(isMouseOver())
		alpha += 0.05f;

	if(isMouseButtonDown())
		alpha += 0.1f;

	if(alpha > 0.0f)
	{
		g.setColour(Colours::white.withAlpha(alpha));
		g.fillRoundedRectangle(bounds, 5.0f);
	}
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
    auto ec = Colour(HISE_ERROR_COLOUR);
    
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
		parser.setNewText(r.getErrorMessage());
		parser.parse();
		height = jmax<int>(40, parser.getHeightForWidth(getWidth() - 80));
	}
            
	show(!currentError.wasOk());
}

void ErrorComponent::show(bool shouldShow)
{
	setVisible(shouldShow);
	getParentComponent()->resized();
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
	Desktop::getInstance().getAnimator().fadeOut(this, 100);
}

void Dialog::ModalPopup::setContent(PageInfo::Ptr newContent)
{
	content = newContent;
}

void Dialog::ModalPopup::show(PositionInfo newInfo, bool addButtons)
{
	info = newInfo;

	okButton.setVisible(addButtons);
	cancelButton.setVisible(addButtons);
            
	if(content != nullptr)
	{
		addAndMakeVisible(contentComponent = content->create(parent, getWidth() - 4 * info.OuterPadding));

		contentComponent->postInit();
		resized();
		Desktop::getInstance().getAnimator().fadeIn(this, 100);
                

		toFront(true);
	}
}

void Dialog::ModalPopup::mouseDown(const MouseEvent& e)
{
	if(!modalBounds.contains(e.getPosition()) && !cancelButton.isVisible())
	{
		dismiss();
	}
}

void Dialog::ModalPopup::paint(Graphics& g)
{
	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		laf->drawMultiPageModalBackground(g, getLocalBounds(), modalBounds);
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
		modalError.setBounds(b.removeFromTop(modalError.height).expanded(6, 3));

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

void Dialog::showModalPopup(bool addButtons)
{
	PositionInfo info;

	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		info = laf->getMultiPagePositionInfo(popup.content->getData());

	popup.show(info, addButtons);
	    
}

Result Dialog::checkCurrentPage()
{
	if(currentPage != nullptr)
	{
		currentErrorElement = nullptr;

		auto ok = currentPage->check(runThread->globalState);

		errorComponent.setError(ok);

		repaint();
		return ok;
	}

	return Result::fail("No page");
}

void Dialog::setCurrentErrorPage(PageBase* b)
{
	currentErrorElement = b;
}

var Dialog::exportAsJSON() const
{
	DynamicObject::Ptr json = new DynamicObject();

	json->setProperty(mpid::StyleData, styleData.toDynamicObject());
	json->setProperty(mpid::Properties, properties);
	json->setProperty(mpid::LayoutData, defaultLaf.defaultPosition.toJSON());
	json->setProperty(mpid::GlobalState, runThread->globalState);
	json->setProperty(mpid::Children, var(pageListInfo));

	return var(json.get());
}

void Dialog::showMainPropertyEditor()
{
	using namespace factory;

	auto& introPage = createModalPopup<List>({
        { mpid::Padding, 10 }
    });

	auto no = new DynamicObject();

	introPage.setStateObject(no);

	DBG(JSON::toString(properties));

    auto& propList = introPage.addChild<List>({
        { mpid::Padding, 10 },
        { mpid::ID, mpid::Properties.toString() }
    });

	propList.setStateObject(properties);

    propList.addChild<TextInput>({
        { mpid::ID, mpid::Header.toString() },
        { mpid::Text, mpid::Header.toString() },
        { mpid::Required, true },
		{ mpid::Help, "This will be shown as big title at the top" },
		{ mpid::Value, properties[mpid::Header] }
    });

    propList.addChild<TextInput>({
        { mpid::ID, mpid::Subtitle.toString() },
        { mpid::Text, mpid::Subtitle.toString() },
        { mpid::Help, "This will be shown as small title below" },
		{ mpid::Value, properties[mpid::Subtitle] }
    });
    
    auto& styleProperties = introPage.addChild<List>({
        { mpid::ID, mpid::StyleData.toString(), },
        { mpid::Text, mpid::StyleData.toString(), },
        { mpid::Padding, 10 },
        { mpid::Foldable, true },
        { mpid::Folded, true }
    });

	auto sd = getStyleData();



    auto sdData = sd.toDynamicObject();

    const Array<Identifier> hiddenProps({
      Identifier("codeBgColour"),
      Identifier("linkBgColour"),
      Identifier("codeColour"),
      Identifier("linkColour"),
      Identifier("tableHeaderBgColour"),
      Identifier("tableLineColour"),
      Identifier("tableBgColour")
    });

	PageInfo* colourCol = nullptr;

    for(auto& nv: sdData.getDynamicObject()->getProperties())
    {
        if(hiddenProps.contains(nv.name))
            continue;

        if(nv.name.toString().contains("Colour"))
        {
			if(colourCol == nullptr)
			{
				colourCol = &styleProperties.addChild<Column>({
				{ mpid::Padding, 10}
				});

				colourCol->addChild<MarkdownText>({
					{ mpid::Text, "Colours:" },
					{ mpid::Padding, 5}
				});
			}

	        auto& ed = colourCol->addChild<ColourChooser>({
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
        }
    }

	introPage.setCustomCheckFunction([&](PageBase* b, var obj)
	{
		Container::checkChildren(b, obj);

		properties = obj[mpid::Properties].clone();

		styleData.fromDynamicObject(obj[mpid::StyleData], [](const String& n){ return Font(n, 15.0f, Font::plain); });

		PositionInfo newPos;

		newPos.fromJSON(obj[mpid::LayoutData]);

		auto ok = newPos.checkSanity();

		if(ok.wasOk())
			defaultLaf.defaultPosition = newPos;

		resized();
		repaint();
		
		return ok;
	});

	showModalPopup(true);

}

void Dialog::showNumPageEditor()
{
	using namespace factory;

	auto& x = createModalPopup<List>();

	x.addChild<MarkdownText>({
	  { mpid::Text, "Enter the number of pages that you want to show in this dialog:"}});

	auto& ti = x.addChild<TextInput>({
	  { mpid::ID, "NumPages" },
	  { mpid::Text, "Number of Pages" }
	});

	var no = new DynamicObject();

	x.setStateObject(no);
	ti.setCustomCheckFunction([&](PageBase* b, var obj)
	{
		auto numPages = (int)b->getValueFromGlobalState();

		if(numPages > 0)
		{
			getState().globalState.getDynamicObject()->clear();
			pageListInfo.clear();
			pages.clear();

			for(int i = 0; i < numPages; i++)
			{
				auto no = new DynamicObject();

				Array<var> childList;

				pageListInfo.add(no);

				no->setProperty(mpid::Type, "List");
				no->setProperty(mpid::Children, var(childList));

				auto& np = addPage<factory::List>();
				np.data = var(no);
			}
			
			showFirstPage();

			return Result::ok();
		}
		else
		{
			return Result::fail("You must enter a valid number of pages");
		}
	});

	showModalPopup(true);
}

Dialog::Dialog(const var& obj, State& rt):
	cancelButton("Cancel"),
	nextButton("Next"),
	prevButton("Previous"),
	popup(*this),
    runThread(&rt),
	errorComponent(*this),
	mainPropertyOverlay(*this),
	numPageOverlay(*this),
	tooltipWindow(this)
{
	editModeBroadcaster.addListener(mainPropertyOverlay, EditorOverlay::onEditChange);
	editModeBroadcaster.addListener(numPageOverlay, EditorOverlay::onEditChange);

	mainPropertyOverlay.setTooltip("Edit the main layout properties (font, colours, etc) of the dialog)");
	numPageOverlay.setTooltip("Set the number of pages of the dialog");

	numPageOverlay.setOnClick(BIND_MEMBER_FUNCTION_0(Dialog::showNumPageEditor));
	mainPropertyOverlay.setOnClick(BIND_MEMBER_FUNCTION_0(Dialog::showMainPropertyEditor));

	
	runThread->globalState = obj[mpid::GlobalState];
    runThread->currentDialog = this;
	sf.addScrollBarToAnimate(content.getVerticalScrollBar());

	setColour(backgroundColour, Colours::black.withAlpha(0.15f));

	if(auto sd = obj[mpid::StyleData].getDynamicObject())
		styleData.fromDynamicObject(var(sd), [](const String& font){ return Font(font, 13.0f, Font::plain); });
	else
	{
		styleData = MarkdownLayout::StyleData::createDarkStyle();
		styleData.backgroundColour = Colours::transparentBlack;
	}
	
	errorComponent.parser.setStyleData(styleData);

	addChildComponent(popup);

	if(auto ld = obj[mpid::LayoutData].getDynamicObject())
		defaultLaf.defaultPosition.fromJSON(var(ld));

	

	if(auto ps = obj[mpid::Properties].getDynamicObject())
		properties = var(ps);
	else
		properties = var(new DynamicObject());

	auto pageList = obj[mpid::Children];

	if(pageList.isArray())
	{
		pageListInfo = *pageList.getArray();

		for(const auto& p: pageListInfo)
		{
			if(auto pi = factory.create(p))
			{
				pi->setStateObject(getState().globalState);
				pi->useGlobalStateObject = true;
				pages.add(std::move(pi));
			}
		}
	}

	addAndMakeVisible(cancelButton);
	addAndMakeVisible(nextButton);
	addAndMakeVisible(prevButton);
	addAndMakeVisible(content);
	addChildComponent(errorComponent);
	setLookAndFeel(&defaultLaf);

	

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
        
        auto& to = l.addChild<factory::Tickbox>({
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

	setEditMode(true);
}

Dialog::~Dialog()
{
	runThread->currentDialog = nullptr;
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
	errorComponent.parser.setStyleData(sd);
}

bool Dialog::navigate(bool forward)
{
    ScopedValueSetter<bool> svs(currentNavigation, forward);
    currentErrorElement = nullptr;
    errorComponent.setError(Result::ok());
	repaint();
	auto newIndex = runThread->currentPageIndex + (forward ? 1 : -1);
        
    if(!forward)
        nextButton.setEnabled(true);
    
	prevButton.setEnabled(newIndex != 0);
        
	if(isPositiveAndBelow(newIndex, pages.size()))
	{
		if(forward && currentPage != nullptr)
		{
			if(!isEditModeEnabled())
			{
				try
				{
					auto ok = currentPage->check(runThread->globalState);
					errorComponent.setError(ok);
		                
					if(!ok.wasOk())
						return false;
				}
				catch(State::CallOnNextAction& cona)
				{
					nextButton.setEnabled(false);
					prevButton.setEnabled(false);
					return false;
				}
			}
		}
            
		if((currentPage = pages[newIndex]->create(*this, content.getWidth() - content.getScrollBarThickness() - 10)))
		{
			content.setViewedComponent(currentPage);
			currentPage->postInit();
            runThread->currentPageIndex = newIndex;
                
			currentPage->setLookAndFeel(&getLookAndFeel());
                
			nextButton.setButtonText(runThread->currentPageIndex == pages.size() - 1 ? "Finish" : "Next");
                
			return true;
		}
	}
	else if (!pages.isEmpty() && newIndex == pages.size())
	{
		MessageManager::callAsync(finishCallback);
		return true;
	}
        
	return false;
}

void Dialog::paint(Graphics& g)
{
	if(editMode)
	{
		GlobalHiseLookAndFeel::draw1PixelGrid(g, this, getLocalBounds(), Colour(0xFF999999));
	}

    Rectangle<int> errorBounds;
    
    if(currentErrorElement != nullptr)
        errorBounds = this->getLocalArea(currentErrorElement, currentErrorElement->getLocalBounds());
    
    if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
	{
        laf->drawMultiPageBackground(g, *this, errorBounds);
		laf->drawMultiPageHeader(g, *this, top);
		laf->drawMultiPageButtonTab(g, *this, bottom);
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
        
	copy = center;

    if(errorComponent.isVisible())
    {
        errorComponent.setBounds(copy.removeFromTop(errorComponent.height));
        copy.removeFromTop(position.OuterPadding);
    }
    else
        errorComponent.setBounds(copy.removeFromTop(0));
        
	content.setBounds(copy);
    
    if(currentPage != nullptr)
    {
        currentPage->setSize(content.getWidth() - content.getScrollBarThickness() - 10, currentPage->getHeight());
        
    }

	auto topCopy = top;

	mainPropertyOverlay.setBounds(top.reduced(-20));
	numPageOverlay.setBounds(topCopy.removeFromBottom(35).removeFromRight(80).translated(12, 6));
}

void Dialog::mouseDown(const MouseEvent& e)
{
	if(e.mods.isRightButtonDown() && editingAllowed)
	{
		PopupLookAndFeel plaf;
		PopupMenu m;
		m.setLookAndFeel(&plaf);

		enum Commands
		{
			EditMode = 1,
			ClearDialog,
			ImportJSON,
			CreateJSON,
			CreateCpp,
			numCommands
		};

		m.addItem(1, "Toggle Edit mode", editingAllowed, editMode);
		m.addSeparator();

		var obj;

		auto ok = JSON::parse(SystemClipboard::getTextFromClipboard(), obj);

		m.addItem(ClearDialog, "Clear dialog");
		//m.addItem(ImportJSON, "Import from JSON", obj.isObject());
		m.addItem(CreateJSON, "Export as JSON", true, createJSON);
		m.addItem(CreateCpp, "Export as C++ code", true, !createJSON);

		if(auto r = m.show())
		{
			switch((Commands)(r))
			{
			case EditMode: 
				setEditMode(!editMode);
				repaint();
				break;
			case ClearDialog:
			{
					
				
				auto& x = createModalPopup<factory::MarkdownText>({
					{ mpid::Text, "Do you really want to clear the current dialog? This action can't be undone" }
				});

				x.setCustomCheckFunction([](PageBase*, var)
				{
					jassertfalse;
					return Result::ok();
				});

				showModalPopup(true);
				break;
			}
			case CreateJSON: createJSON = true; break;
			case CreateCpp: createJSON = false; break;
			}
		}
	}
}

void Dialog::setEditMode(bool isEditMode)
{
	editMode = isEditMode;
	editModeBroadcaster.sendMessage(sendNotificationSync, editMode);

	runThread->currentPageIndex--;
	navigate(true);
	
	repaint();
}

bool Dialog::isEditModeEnabled() const noexcept
{
	return editMode;
}


Dialog::PageBase::HelpButton::HelpButton(String help, const PathFactory& factory):
	HiseShapeButton("help", nullptr, factory)
{
	onClick = [this, help]()
	{
		if(auto p = findParentComponentOfClass<Dialog::ModalPopup>())
		{
			findParentComponentOfClass<PageBase>()->setModalHelp(help);
		}
		else
		{
			auto mp = findParentComponentOfClass<Dialog>();
			mp->createModalPopup<factory::MarkdownText>()[mpid::Text] = help;
			mp->showModalPopup(false);
		}
	};
}

void Dialog::PageBase::setModalHelp(const String& text)
{
	if(modalHelp != nullptr)
	{
		Desktop::getInstance().getAnimator().fadeOut(modalHelp, 200);
		modalHelp = nullptr;
	}
	else
	{

		addAndMakeVisible(modalHelp = new ModalHelp(text, *this));
		Desktop::getInstance().getAnimator().fadeIn(modalHelp, 200);
		modalHelp->setBounds(getLocalBounds());
	}
}
}
}
