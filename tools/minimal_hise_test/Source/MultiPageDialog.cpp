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
#include "PageFactory.h"

namespace hise
{

namespace multipage {
using namespace juce;

MultiPageDialog::PageBase::PageBase(MultiPageDialog& rootDialog_, int width, const var& obj):
  rootDialog(rootDialog_)
{
	padding = (int)obj[MultiPageIds::Padding];

    stateObject = rootDialog.getRunThread().globalState;
    
	auto help = obj[MultiPageIds::Help].toString();

	if(help.isNotEmpty())
	{
		helpButton = new HelpButton(help, rootDialog);
		addAndMakeVisible(helpButton);
		helpButton->setWantsKeyboardFocus(false);
	}

	if(obj.hasProperty(MultiPageIds::Value))
	{
		initValue = obj[MultiPageIds::Value];
	}

	auto idString = obj[MultiPageIds::ID].toString();

	if(idString.isNotEmpty())
		id = Identifier(idString);
}

var MultiPageDialog::PageBase::getValueFromGlobalState(var defaultState)
{
    return stateObject.getProperty(id, defaultState);
}

var MultiPageDialog::PositionInfo::toJSON() const
{
	auto obj = new DynamicObject();
            
	obj->setProperty("TopHeight", TopHeight);
	obj->setProperty("ButtonTab", ButtonTab);
	obj->setProperty("ButtonMargin", ButtonMargin);
	obj->setProperty("OuterPadding", OuterPadding);
    obj->setProperty("LabelWidth", LabelWidth);
            
	return var(obj);
}

void MultiPageDialog::PositionInfo::fromJSON(const var& obj)
{
	TopHeight = obj.getProperty("TopHeight", TopHeight);
	ButtonTab = obj.getProperty("ButtonTab", ButtonTab);
	ButtonMargin = obj.getProperty("ButtonMargin", ButtonMargin);
	OuterPadding = obj.getProperty("OuterPadding", OuterPadding);
    LabelWidth = obj.getProperty("LabelWidth", LabelWidth);
}

MultiPageDialog::ErrorComponent::ErrorComponent(MultiPageDialog& parent_):
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

void MultiPageDialog::ErrorComponent::paint(Graphics& g)
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

void MultiPageDialog::ErrorComponent::setError(const Result& r)
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

void MultiPageDialog::ErrorComponent::show(bool shouldShow)
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

void MultiPageDialog::ErrorComponent::resized()
{
	auto b = getLocalBounds();
	auto cb = b.removeFromRight(40).removeFromTop(40).reduced(10);
	closeButton->setBounds(cb);
}



MultiPageDialog::PageBase* MultiPageDialog::PageInfo::create(MultiPageDialog& r, int currentWidth) const
{
	if(pageCreator)
	{
		auto np = pageCreator(r, currentWidth, data);

		if(stateObject.isObject())
			np->setStateObject(stateObject);

        np->setCustomCheckFunction(customCheck);

		if(auto c = dynamic_cast<PageFactory::Container*>(np))
		{
			for(auto ch: childItems)
				c->addChild(ch);
		}

		return np;
	}
            
	return nullptr;
}

var& MultiPageDialog::PageInfo::operator[](const Identifier& id) const
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

MultiPageDialog::Factory::Factory()
{
    registerPage<PageFactory::MarkdownText>();
    registerPage<PageFactory::List>();
	registerPage<PageFactory::Column>();
    registerPage<PageFactory::FileSelector>();
    registerPage<PageFactory::Tickbox>();
}

MultiPageDialog::PageInfo::Ptr MultiPageDialog::Factory::create(const var& obj)
{
	PageInfo::Ptr info = new PageInfo(obj);
	
	auto typeProp = obj[MultiPageIds::Type].toString();

	if(typeProp.isNotEmpty())
	{
		Identifier id(typeProp);

		for(const auto& i: items)
		{
			if(i.id == id)
			{
				info->setCreateFunction(i.f);
				break;
			}
		}
	}

	return info;
}

MultiPageDialog::MultiPageDialog(const var& obj, MultiPageDialog::RunThread& rt):
	cancelButton("Cancel"),
	nextButton("Next"),
	prevButton("Previous"),
	popup(*this),
    runThread(&rt),
	errorComponent(*this)
{
    runThread->currentDialog = this;
	sf.addScrollBarToAnimate(content.getVerticalScrollBar());

	setColour(backgroundColour, Colours::black.withAlpha(0.15f));

	if(auto sd = obj[MultiPageIds::StyleData].getDynamicObject())
		styleData.fromDynamicObject(var(sd), [](const String& font){ return Font(font, 13.0f, Font::plain); });
	
	errorComponent.parser.setStyleData(styleData);

	addChildComponent(popup);

	if(auto ld = obj[MultiPageIds::LayoutData].getDynamicObject())
		defaultLaf.defaultPosition.fromJSON(var(ld));

	

	if(auto ps = obj[MultiPageIds::Properties].getDynamicObject())
		properties = var(ps);
	else
		properties = var(new DynamicObject());

	auto pageList = obj[MultiPageIds::Children];

	if(pageList.isArray())
	{
		for(const auto& p: *pageList.getArray())
		{
			if(auto pi = factory.create(p))
			{
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
		auto& l = createModalPopup<PageFactory::List>();

        l[MultiPageIds::Padding] = 15;
        
        auto& md = l.addChild<PageFactory::MarkdownText>();
        
        auto& to = l.addChild<PageFactory::Tickbox>({
            { MultiPageIds::Text, "Save progress" },
            { MultiPageIds::ID, "Save" }
        });
        
		md[MultiPageIds::Text] = "Do you want to close this popup?";

		md.setCustomCheckFunction([this](PageBase*, var obj)
		{
			MessageManager::callAsync(finishCallback);
			return Result::ok();
		});
        
        

		showModalPopup(true);
	};
}

void MultiPageDialog::showFirstPage()
{
	currentPage = nullptr;
    runThread->currentPageIndex--;
	navigate(true);
}

void MultiPageDialog::setFinishCallback(const std::function<void()>& f)
{
	finishCallback = f;
}

var MultiPageDialog::getOrCreateChild(const var& obj, const Identifier& id)
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

var MultiPageDialog::getGlobalState(Component& page, const Identifier& id, const var& defaultValue)
{
	if(auto m = page.findParentComponentOfClass<MultiPageDialog>())
	{
		if(id.isNull())
			return m->runThread->globalState;

		return m->runThread->globalState.getProperty(id, defaultValue);
	}
        
	return defaultValue;
}

std::pair<Font, Colour> MultiPageDialog::getDefaultFont(Component& c)
{
	if(auto m = c.findParentComponentOfClass<MultiPageDialog>())
	{
		return { m->styleData.getFont(), m->styleData.textColour };
	}
        
	return { Font(), Colours::white };
}

Path MultiPageDialog::createPath(const String& url) const
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

	return p;
}

void MultiPageDialog::setStyleData(const MarkdownLayout::StyleData& sd)
{
	styleData = sd;
	errorComponent.parser.setStyleData(sd);
}

bool MultiPageDialog::navigate(bool forward)
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
			auto ok = currentPage->check(runThread->globalState);
			errorComponent.setError(ok);
                
			if(!ok.wasOk())
				return false;
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

void MultiPageDialog::paint(Graphics& g)
{
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

void MultiPageDialog::resized()
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
}

void MultiPageDialog::LookAndFeelMethods::drawMultiPageHeader(Graphics& g, MultiPageDialog& d, Rectangle<int> area)
{
    var pd;
    
    if(auto pi = d.pages[d.runThread->currentPageIndex])
        pd = pi->getData();
    
	auto pos = getMultiPagePositionInfo(pd);


	auto f = d.styleData.getBoldFont().withHeight(area.getHeight() - 20);

	auto progress = area.removeFromBottom(2);

	g.setColour(d.styleData.textColour.withAlpha(0.3f));

	g.fillRect(progress);

	auto normProgress = (float)(d.runThread->currentPageIndex+1) / jmax(1.0f, (float)d.pages.size());

	g.setColour(d.styleData.textColour);

	g.fillRect(progress.removeFromLeft(normProgress * (float)area.getWidth()));

	area.removeFromBottom(3);

	g.setFont(f);
	g.setColour(d.styleData.headlineColour);
	g.drawText(d.properties[MultiPageIds::Header], area.toFloat(), Justification::topLeft);

	g.setFont(d.styleData.getFont());
	g.setColour(d.styleData.textColour);
	g.drawText(d.properties[MultiPageIds::Subtitle], area.toFloat(), Justification::bottomLeft);

	g.setColour(d.styleData.textColour.withAlpha(0.4f));

	String pt;
	pt << "Step " << String(d.runThread->currentPageIndex+1) << " / " << String(d.pages.size());

	g.drawText(pt, area.toFloat(), Justification::bottomRight);

}

void MultiPageDialog::LookAndFeelMethods::drawMultiPageButtonTab(Graphics& g, MultiPageDialog& d, Rectangle<int> area)
{
	g.setColour(d.findColour(ColourIds::backgroundColour));
	g.fillRect(area);
}

void MultiPageDialog::DefaultLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& tb,
	bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto c = tb.findColour(ToggleButton::ColourIds::tickColourId);
    auto b = tb.getLocalBounds().toFloat();
    
    b = b.removeFromRight(b.getHeight());
	g.setColour(c.withMultipliedAlpha(shouldDrawButtonAsHighlighted ? 1.0f : 0.7f));

	g.drawRoundedRectangle(b.reduced(8).toFloat(), 4.0f, 2.0f);

	if(tb.getToggleState())
	{
		g.fillRoundedRectangle(b.reduced(11).toFloat(), 3.0f);
	}
}

MultiPageDialog::PositionInfo MultiPageDialog::DefaultLookAndFeel::getMultiPagePositionInfo(const var& pageData) const
{
	return defaultPosition;
}

void MultiPageDialog::DefaultLookAndFeel::layoutFilenameComponent(FilenameComponent& filenameComp,
	ComboBox* filenameBox, Button* browseButton)
{
	if (browseButton == nullptr || filenameBox == nullptr)
		return;

	auto b = filenameComp.getLocalBounds();
            
	browseButton->setBounds(b.removeFromRight(100));
    b.removeFromRight(getMultiPagePositionInfo({}).OuterPadding);

	filenameBox->setBounds(b);
}

MultiPageDialog::PageBase::HelpButton::HelpButton(String help, const PathFactory& factory):
	HiseShapeButton("help", nullptr, factory)
{
	onClick = [this, help]()
	{
		auto mp = findParentComponentOfClass<MultiPageDialog>();
		mp->createModalPopup<PageFactory::MarkdownText>()[MultiPageIds::Text] = help;
		mp->showModalPopup(false);
	};
}


}
}