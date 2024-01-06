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
using namespace juce;

MultiPageDialog::PageBase::PageBase(MultiPageDialog& rootDialog_, int width, const var& obj):
  rootDialog(rootDialog_)
{
	padding = (int)obj[MultiPageIds::Padding];

	auto help = obj[MultiPageIds::Help].toString();

	if(help.isNotEmpty())
	{
		helpButton = new HelpButton(help, rootDialog);
		addAndMakeVisible(helpButton);
	}

	auto idString = obj[MultiPageIds::ID].toString();

	if(idString.isNotEmpty())
		id = Identifier(idString);
}

var MultiPageDialog::PageBase::getValueFromGlobalState(var defaultState)
{
	return getGlobalState(*this, id, defaultState);
}

var MultiPageDialog::PositionInfo::toJSON() const
{
	auto obj = new DynamicObject();
            
	obj->setProperty("TopHeight", TopHeight);
	obj->setProperty("ButtonTab", ButtonTab);
	obj->setProperty("ButtonMargin", ButtonMargin);
	obj->setProperty("OuterPadding", OuterPadding);
            
	return var(obj);
}

void MultiPageDialog::PositionInfo::fromJSON(const var& obj)
{
	TopHeight = obj.getProperty("TopHeight", TopHeight);
	ButtonTab = obj.getProperty("ButtonTab", ButtonTab);
	ButtonMargin = obj.getProperty("ButtonMargin", ButtonMargin);
	OuterPadding = obj.getProperty("OuterPadding", OuterPadding);
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
	g.setColour(Colour(HISE_ERROR_COLOUR));
	g.fillRoundedRectangle(getLocalBounds().toFloat(), 5.0f);
            
	auto x = getLocalBounds().toFloat();
	x.removeFromRight(40.0f);
	x.removeFromLeft(10.0f);
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
		height = jmax<int>(40, parser.getHeightForWidth(getWidth() - 50));
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

MultiPageDialog::MultiPageDialog(const var& obj):
	cancelButton("Cancel"),
	nextButton("Next"),
	prevButton("Previous"),
	currentError(Result::ok()),
	popup(*this),
	errorComponent(*this)
{
	sf.addScrollBarToAnimate(content.getVerticalScrollBar());

	setColour(backgroundColour, Colours::black.withAlpha(0.05f));

	if(auto sd = obj[MultiPageIds::StyleData].getDynamicObject())
		styleData.fromDynamicObject(var(sd), [](const String& font){ return Font(font, 13.0f, Font::plain); });
	
	errorComponent.parser.setStyleData(styleData);

	addChildComponent(popup);

	if(auto ld = obj[MultiPageIds::LayoutData].getDynamicObject())
		defaultLaf.defaultPosition.fromJSON(var(ld));

	if(auto gs = obj[MultiPageIds::GlobalState].getDynamicObject())
		globalState = var(gs->clone());
	else
		globalState = var(new DynamicObject());

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
		auto& md = createModalPopup<PageFactory::MarkdownText>();

		md[MultiPageIds::Text] = "Do you want to close this popup?\n> This can't be undone!";

		md.setCustomCheckFunction([this](PageBase*, var obj)
		{
			return Result::fail("NOO!!!!");
			MessageManager::callAsync(finishCallback);
			return Result::ok();
		});

		showModalPopup();
	};
}

void MultiPageDialog::showFirstPage()
{
	currentPage = nullptr;
	currentPageIndex = -1;
	navigate(true);
}

void MultiPageDialog::setFinishCallback(const std::function<void()>& f)
{
	finishCallback = f;
}

void MultiPageDialog::setGlobalState(Component& page, const Identifier& id, var newValue)
{
	if(auto m = page.findParentComponentOfClass<MultiPageDialog>())
	{
		m->globalState.getDynamicObject()->setProperty(id, newValue);
	}
}

var MultiPageDialog::getGlobalState(Component& page, const Identifier& id, const var& defaultValue)
{
	if(auto m = page.findParentComponentOfClass<MultiPageDialog>())
	{
		if(id.isNull())
			return m->globalState;

		return m->globalState.getProperty(id, defaultValue);
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
        
	LOAD_EPATH_IF_URL("close", HiBinaryData::ProcessorEditorHeaderIcons::closeIcon);
	LOAD_EPATH_IF_URL("help", MainToolbarIcons::help);

	return p;
}

void MultiPageDialog::setStyleData(const MarkdownLayout::StyleData& sd)
{
	styleData = sd;
	errorComponent.parser.setStyleData(sd);
}

bool MultiPageDialog::navigate(bool forward)
{
	repaint();
	auto newIndex = currentPageIndex + (forward ? 1 : -1);
        
	prevButton.setEnabled(newIndex != 0);
        
	if(isPositiveAndBelow(newIndex, pages.size()))
	{
		if(forward && currentPage != nullptr)
		{
			auto ok = currentPage->check(globalState);
			errorComponent.setError(ok);
                
			if(!ok.wasOk())
				return false;
		}
            
		if((currentPage = pages[newIndex]->create(*this, content.getWidth() - content.getScrollBarThickness())))
		{
			content.setViewedComponent(currentPage);
			currentPage->postInit();
			currentPageIndex = newIndex;
                
			currentPage->setLookAndFeel(&getLookAndFeel());
                
			nextButton.setButtonText(currentPageIndex == pages.size() - 1 ? "Finish" : "Next");
                
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
	g.fillAll(findColour(backgroundColour));
	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
	{
		laf->drawHeader(g, *this, top);
		laf->drawButtonTab(g, *this, bottom);
	}
}

void MultiPageDialog::resized()
{
	popup.setBounds(getLocalBounds());

	PositionInfo position;

	if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		position = laf->getPositionInfo();
        
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

	errorComponent.setBounds(copy.removeFromTop(errorComponent.isVisible() ? errorComponent.height : 0));
        
	if(errorComponent.isVisible())
		copy.removeFromTop(position.OuterPadding);
        
	content.setBounds(copy);
}

void MultiPageDialog::LookAndFeelMethods::drawHeader(Graphics& g, MultiPageDialog& d, Rectangle<int> area)
{
	auto pos = getPositionInfo();

	g.setColour(d.findColour(ColourIds::backgroundColour));
	g.fillRect(area.expanded(pos.OuterPadding));

	auto f = d.styleData.getBoldFont().withHeight(area.getHeight() - 20);

	auto progress = area.removeFromBottom(2);

	g.setColour(d.styleData.textColour.withAlpha(0.3f));

	g.fillRect(progress);

	auto normProgress = (float)(d.currentPageIndex+1) / jmax(1.0f, (float)d.pages.size());

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
	pt << "Step " << String(d.currentPageIndex+1) << " / " << String(d.pages.size());

	g.drawText(pt, area.toFloat(), Justification::bottomRight);

}

void MultiPageDialog::LookAndFeelMethods::drawButtonTab(Graphics& g, MultiPageDialog& d, Rectangle<int> area)
{
	g.setColour(d.findColour(ColourIds::backgroundColour));
	g.fillRect(area);
}

void MultiPageDialog::DefaultLookAndFeel::drawToggleButton(Graphics& g, ToggleButton& tb,
	bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
	auto c = tb.findColour(ToggleButton::ColourIds::tickColourId);

	g.setColour(c.withMultipliedAlpha(shouldDrawButtonAsHighlighted ? 1.0f : 0.7f));

	g.drawRoundedRectangle(tb.getLocalBounds().reduced(8).toFloat(), 4.0f, 2.0f);

	if(tb.getToggleState())
	{
		g.fillRoundedRectangle(tb.getLocalBounds().reduced(11).toFloat(), 3.0f);
	}
}

MultiPageDialog::PositionInfo MultiPageDialog::DefaultLookAndFeel::getPositionInfo() const
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
	b.removeFromRight(getPositionInfo().OuterPadding);

	filenameBox->setBounds(b);
}

MultiPageDialog::PageBase::HelpButton::HelpButton(String help, const PathFactory& factory):
	HiseShapeButton("help", nullptr, factory)
{
	onClick = [this, help]()
	{
		auto mp = findParentComponentOfClass<MultiPageDialog>();
		mp->createModalPopup<PageFactory::MarkdownText>()[MultiPageIds::Text] = help;
		mp->showModalPopup();
	};
}


}
