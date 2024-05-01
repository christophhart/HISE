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


namespace hise {
namespace multipage {
namespace factory {
using namespace juce;

Container::Container(Dialog& r, int width, const var& obj):
	PageBase(r, width, obj)
{
	
}



Result Container::checkChildren(PageBase* b, const var& toUse)
{
	if(auto ct = dynamic_cast<Container*>(b))
	{

		for(auto c: ct->childItems)
		{
			auto ok = c->check(toUse);
		            
			if(!ok.wasOk())
				return ok;
		}
	}
	else
	{
		return b->check(toUse);
	}

	return Result::ok();
}

void Container::recalculateParentSize(PageBase* b)
{
	b->rebuildRootLayout();
}

void Container::postInit()
{
	init();

    if(infoObject[mpid::UseChildState])
    {
        stateObject = Dialog::getOrCreateChild(stateObject, id);
    }

    rebuildChildren();
	
	for(const auto& sp: staticPages)
	{
		childItems.add(sp->create(rootDialog, getWidth()));
		addFlexItem(*childItems.getLast());
	}

	for(auto c: childItems)
    {
        c->setStateObject(stateObject);

		if(stateObject.hasProperty(c->getId()))
			c->clearInitValue();

        c->postInit();
    }

	resized();
}

Result Container::checkGlobalState(var globalState)
{
    var toUse = globalState;

    if(id.isValid())
    {
	    if(toUse.hasProperty(id))
            toUse = toUse[id];
        else
        {
            auto no = new DynamicObject();
	        toUse.getDynamicObject()->setProperty(id, no);
            toUse = var(no);
        }
    }

	return checkChildren(this, toUse);
	
}

void Container::addChild(Dialog::PageInfo::Ptr info)
{
	staticPages.add(info);
}

void Container::addChild(int width, const var& r)
{
	if(auto pi = factory.create(r))
	{
		childItems.add(pi->create(rootDialog, width));
		addFlexItem(*childItems.getLast());
	}
}


void Container::clearInitValue()
{
	initValue = var();

	for(auto& c: childItems)
		c->clearInitValue();
}

void Container::addWithPopup()
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	rootDialog.containerPopup(infoObject);
#endif
}

Result Container::customCheckOnAdd(PageBase* b, const var& obj)
{
	dynamic_cast<Container*>(b)->checkGlobalState(obj);
	rebuildRootLayout();
	return Result::ok();
}

void Container::replaceChildrenDynamic()
{
	{
		ScopedValueSetter<bool> svs(rootDialog.getSkipRebuildFlag(), true);

		childItems.clear();
		auto l = infoObject[mpid::Children];
		childItems.clear();

		for(auto& r: *l.getArray())
			addChildDynamic(r, false);
	}

	rootDialog.body.setCSS(rootDialog.css);
}

void Container::rebuildChildren()
{
	auto l = infoObject[mpid::Children];

	childItems.clear();

	if(l.isArray())
	{
		for(auto& r: *l.getArray())
			addChild(getWidth(), r);
	}
	else
	{
		infoObject.getDynamicObject()->setProperty(mpid::Children, var(Array<var>()));
	}
}

List::List(Dialog& r, int width, const var& obj):
	Container(r, width, obj)
{
	setDefaultStyleSheet("display:flex; flex-direction: column; flex-wrap: nowrap;height: auto;width:100%; gap: 10px;");

	foldable = obj[mpid::Foldable];
    folded = obj[mpid::Folded];
    title = obj[mpid::Text];

	if(foldable)
	{
		foldButton = new TextButton(title);
		foldButton->setClickingTogglesState(true);
		foldButton->setToggleState(folded, dontSendNotification);
		foldButton->setWantsKeyboardFocus(false);
		foldButton->onClick = BIND_MEMBER_FUNCTION_0(List::refreshFold);

		Helpers::writeSelectorsToProperties(*foldButton, { ".fold-bar" });
		addFlexItem(*foldButton);
	}

	setSize(width, 0);
}

List::~List()
{
}

Result List::customCheckOnAdd(PageBase* b, const var& obj)
{
	auto ok = Container::customCheckOnAdd(b, obj);
    	
	foldable = obj[mpid::Foldable];
	folded = obj[mpid::Folded];
	title = obj[mpid::Text];

	return Result::ok();
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void List::createEditor(Dialog::PageInfo& rootList)
{
    rootList.addChild<Type>({
        { mpid::Type, "List" },
        { mpid::ID, "Type"}
    });
    
    auto& prop = rootList;
    
    auto& listId = prop.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },

        { mpid::Help, "The ID of the list. This will be used for identification in some logic cases" }
    });

    prop.addChild<Button>({
        { mpid::ID, "UseChildState" },
        { mpid::Text, "UseChildState" },
        { mpid::Help, "If ticked, then this list will save its child element values in a sub object of the global state." },
        { mpid::Value, infoObject[mpid::UseChildState] }
    });
    
    if(!rootList[mpid::Value].isUndefined())
    {
        auto v = rootList[mpid::Value].toString();

	    listId[mpid::Value] = v;
    }

    prop.addChild<TextInput>({
        { mpid::ID, "Text" },
        { mpid::Text, "Text" },
        { mpid::Help, "The title text that is shown at the header bar." },
		{ mpid::Value, title }
    });

	prop.addChild<TextInput>({
        { mpid::ID, mpid::Class.toString() },
        { mpid::Text, mpid::Class.toString() },
        { mpid::Help, "The CSS class that is applied to the container" },
		{ mpid::Value, infoObject[mpid::Class] }
    });

	prop.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
        { mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});

    prop.addChild<Button>({
        { mpid::ID, "Foldable" },
        { mpid::Text, "Foldable" },
        { mpid::Help, "If ticked, then this list will show a clickable header that can be folded" },
		{ mpid::Value, foldable }
    });
    
    prop.addChild<Button>({
        { mpid::ID, "Folded" },
        { mpid::Text, "Folded" },
        { mpid::Help, "If ticked, then this list will folded as default state" },
		{ mpid::Value, folded }
    });
}
#endif

void List::postInit()
{
	Container::postInit();
        
	if(foldable)
	{
		foldButton->setToggleState(folded, dontSendNotification);
		refreshFold();
	}
}


void List::refreshFold()
{
	if(foldable)
	{
		for(int i = 1; i < getNumChildComponents(); i++)
		{
            setFlexChildVisibility(i, false, foldButton->getToggleState());
		}

        rebuildLayout();
        setSize(getWidth(), getAutoHeightForWidth(getWidth()));
        rebuildRootLayout();
	}
}


Column::Column(Dialog& r, int width, const var& obj):
	Container(r, width, obj)
{
	setDefaultStyleSheet("display:flex; flex-direction: row; flex-wrap: nowrap;height: auto;width:100%; gap: 10px;");
	setSize(width, 0);
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void Column::createEditor(Dialog::PageInfo& xxx)
{
    xxx.addChild<Type>({
        { mpid::Type, "Column" },
        { mpid::ID, "Type"}
    });
    
    auto& prop = xxx;
    
    
    auto& listId = prop.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },
        { mpid::Help, "The ID. This will be used for identification in some logic cases" }
    });

	prop.addChild<TextInput>({
        { mpid::ID, mpid::Class.toString() },
        { mpid::Text, mpid::Class.toString() },
        { mpid::Help, "The CSS class that is applied to the container" },
		{ mpid::Value, infoObject[mpid::Class] }
    });

	prop.addChild<TextInput>({
		{ mpid::ID, mpid::Style.toString() },
		{ mpid::Text, mpid::Style.toString() },
        { mpid::Value, infoObject[mpid::Style] },
		{ mpid::Help, "Additional inline properties that will be used by the UI element" }
	});

    prop.addChild<Button>({
        { mpid::ID, "UseChildState" },
        { mpid::Text, "UseChildState" },
        { mpid::Help, "If ticked, then this list will save its child element values in a sub object of the global state." },
        { mpid::Value, infoObject[mpid::UseChildState] }
    });
    
    if(!xxx[mpid::Value].isUndefined())
    {
        auto v = xxx[mpid::Value].toString();

	    listId[mpid::Value] = v;
    }
}
#endif

Branch::Branch(Dialog& root, int w, const var& obj):
	Container(root, w, obj)
{
	setDefaultStyleSheet("display:flex; flex-direction: column; flex-wrap: nowrap;height: auto;width:100%; gap: 10px;");
	setSize(w, 0);

	if(root.isEditModeEnabled())
		Helpers::writeInlineStyle(*this, "margin-left: 25%;min-height:30px;");
}

void Branch::paint(Graphics& g)
{
	FlexboxComponent::paint(g);

	if(rootDialog.isEditModeEnabled())
	{
		auto b = getLocalBounds().removeFromLeft(getWidth() / 4);

		int index = 0;

		for(auto c: childItems)
		{
			auto tl = b.removeFromTop(c->getHeight()).toFloat();

			g.setFont(GLOBAL_MONOSPACE_FONT());
			g.setColour(Colours::white.withAlpha(0.5f));

			String s;
			s << "if(" << id << " == " << String(index++) << ") {";

			g.drawText(s, tl, Justification::centred);
			b.removeFromTop(10);
		}
	}
}


void Branch::postInit()
{
	init();

	currentIndex = getValueFromGlobalState();
	
	rebuildChildren();
        
	for(const auto& sp: staticPages)
	{
		childItems.add(sp->create(rootDialog, getWidth()));
		addFlexItem(*childItems.getLast());
	}
	
	if(rootDialog.isEditModeEnabled())
	{
		for(auto c: childItems)
		{
			Helpers::writeInlineStyle(*c, "min-height:30px;border: 1px solid #555;padding: 5px;border-radius:6px;");

			c->setStateObject(stateObject);

			if(stateObject.hasProperty(c->getId()))
				c->clearInitValue();

			c->postInit();
		}
	}
	else
	{
		if(PageBase* p = childItems.removeAndReturn(currentIndex))
		{
			childItems.clear();
			childItems.add(p);
			p->postInit();
		}
		else
			childItems.clear();
	}
	
	rebuildLayout();
}

Result Branch::checkGlobalState(var globalState)
{
	if(auto p = childItems[0])
		return p->check(globalState);
        
	return Result::fail("No branch selected");
}

#if HISE_MULTIPAGE_INCLUDE_EDIT
void Branch::createEditor(Dialog::PageInfo& rootList)
{
    rootList.addChild<Type>({
        { mpid::Type, "Branch" },
        { mpid::ID, "Type"}
    });
    
    auto& prop = rootList;
    
    rootList[mpid::Text] = "List";
    
    prop.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },
        { mpid::Required, true },
        { mpid::Items, rootDialog.getExistingKeysAsItemString() },
        { mpid::Help, "The ID of the branch. This will be used as key to fetch the value from the global state to determine which child to show." }
    });
}
#endif


} // PageFactory
} // multipage
} // hise
