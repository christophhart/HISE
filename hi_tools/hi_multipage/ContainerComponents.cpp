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
	if(obj.hasProperty(mpid::Padding))
		padding = (int)obj[mpid::Padding];
	else
		padding = 10;

    if(obj.hasProperty(mpid::ID))
    {
		auto nid = obj[mpid::ID].toString();

		if(nid.isNotEmpty())
			id = Identifier(nid);
    }

	auto l = obj[mpid::Children];
        
	if(l.isArray())
	{
		for(auto& r: *l.getArray())
			addChild(width, r);
	}
	else
	{
		obj.getDynamicObject()->setProperty(mpid::Children, var(Array<var>()));
	}
}

void Container::paintEditBounds(Graphics& g)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	if(overlay != nullptr)
	{
		auto f = Dialog::getDefaultFont(*this);
		auto b = overlay->localBoundFunction(this).reduced(5);

		g.setColour(f.second.withAlpha(0.05f));
		g.fillRoundedRectangle(b.toFloat(), 3.0f);
		g.setColour(f.second.withAlpha(0.8f));

		g.setFont(f.first);
		g.drawText("Click to edit " + getContainerTypeId().toString(), b, Justification::centred);
	}
#endif
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
	Container* c = dynamic_cast<Container*>(b);

	if(c == nullptr)
		c = b->findParentComponentOfClass<Container>();

	if(c == nullptr)
		return;

	c->calculateSize();

	while(c = dynamic_cast<Container*>(c->getParentComponent()))
		c->calculateSize();

	if(auto p = b->findParentComponentOfClass<Dialog::ModalPopup>())
	{
		p->resized();
	}
}

void Container::postInit()
{
	init();
	
    stateObject = Dialog::getOrCreateChild(stateObject, id);

	DBG(JSON::toString(stateObject));

	for(const auto& sp: staticPages)
	{
		childItems.add(sp->create(rootDialog, getWidth()));
		addAndMakeVisible(childItems.getLast());
	}

	for(auto c: childItems)
    {
        c->setStateObject(stateObject);

		if(stateObject.hasProperty(c->getId()))
		{
			c->clearInitValue();
		}

        c->postInit();
    }

	calculateSize();
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
		addAndMakeVisible(childItems.getLast());
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
	rootDialog.containerPopup(infoObject);
}

Result Container::customCheckOnAdd(PageBase* b, const var& obj)
{
	dynamic_cast<Container*>(b)->checkGlobalState(obj);
	padding = obj[mpid::Padding];

	Container* c = this;

	while(c != nullptr)
	{
		c->calculateSize();
		c->resized();

		c = c->findParentComponentOfClass<Container>();
	}
        
	return Result::ok();
}


void List::calculateSize()
{
	int h = foldable ? (titleHeight + padding) : 0;

    for(auto c: childItems)
        c->setVisible(!folded);
    
    if(!folded)
    {
        for(auto& c: childItems)
            h += c->getHeight() + padding;
    }

	if(rootDialog.isEditModeEnabled())
		h += 32;
    
	setSize(getWidth(), h);
}

List::List(Dialog& r, int width, const var& obj):
	Container(r, width, obj)
{
    foldable = obj[mpid::Foldable];
    folded = obj[mpid::Folded];
    title = obj[mpid::Text];
	setSize(width, 0);
}

List::~List()
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	if(overlay != nullptr)
		rootDialog.removeChildComponent(overlay);

	overlay = nullptr;
#endif
}

Result List::customCheckOnAdd(PageBase* b, const var& obj)
{
	auto ok = Container::customCheckOnAdd(b, obj);
    	
	foldable = obj[mpid::Foldable];
	folded = obj[mpid::Folded];
	title = obj[mpid::Text];

	calculateSize();
	return Result::ok();
}

void List::createEditor(Dialog::PageInfo& rootList)
{
    auto& tt = rootList.addChild<Type>({
        { mpid::Type, "List" },
        { mpid::ID, "Type"}
    });
    
    auto& prop = rootList.addChild<List>();
    
    rootList[mpid::Text] = "List";
    
    prop[mpid::Folded] = false;
    prop[mpid::Foldable] = true;
    prop[mpid::Padding] = 10;
    prop[mpid::Text] = "Properties";
    
    auto& listId = prop.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },

        { mpid::Help, "The ID of the list. This will be used for identification in some logic cases" }
    });

    if(!rootList[mpid::Value].isUndefined())
    {
        auto v = rootList[mpid::Value].toString();

	    listId[mpid::Value] = v;
    }

    auto& textId = prop.addChild<TextInput>({
        { mpid::ID, "Text" },
        { mpid::Text, "Text" },
        { mpid::Help, "The title text that is shown at the header bar." },
		{ mpid::Value, title }
    });

    auto& padId = prop.addChild<TextInput>({
        { mpid::ID, "Padding" },
        { mpid::Text, "Padding" },
        { mpid::Help, "The spacing between child elements in pixel." },
		{ mpid::Value, padding }
    });

    auto& foldId1 = prop.addChild<Button>({
        { mpid::ID, "Foldable" },
        { mpid::Text, "Foldable" },
        { mpid::Help, "If ticked, then this list will show a clickable header that can be folded" },
		{ mpid::Value, foldable }
    });
    
    auto& foldId2 = prop.addChild<Button>({
        { mpid::ID, "Folded" },
        { mpid::Text, "Folded" },
        { mpid::Help, "If ticked, then this list will folded as default state" },
		{ mpid::Value, folded }
    });
}

void List::editModeChanged(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	PageBase::editModeChanged(isEditMode);

	if(overlay != nullptr)
	{
		overlay->localBoundFunction = [](Component* c)
		{
			return c->getLocalBounds().removeFromBottom(32);
		};

		overlay->setOnClick(BIND_MEMBER_FUNCTION_0(Container::addWithPopup));
	}
#endif
}

void List::resized()
{
	auto b = getLocalBounds();

	if(b.isEmpty())
		return;

    if(foldable)
        b.removeFromTop(24 + padding);

	if(rootDialog.isEditModeEnabled())
		b.removeFromLeft(10);

    if(!folded)
    {
        for(auto c: childItems)
        {
            auto cb = b.removeFromTop(c->getHeight());
            
            if(!cb.isEmpty())
            {
                c->setBounds(cb);
                c->resized();
            }
            b.removeFromTop(padding);
        }
    }
}

void List::mouseDown(const MouseEvent& e)
{
	if(foldable && e.getPosition().getY() < titleHeight)
	{
		folded = !folded;
            
		
		repaint();

		Container::recalculateParentSize(this);
	}
}

void List::paint(Graphics& g)
{
	paintEditBounds(g);

	auto tc = Dialog::getDefaultFont(*this).second;

	auto b = getLocalBounds();

	if(isEditModeAndNotInPopup())
	{
		auto c = b.removeFromLeft(10);

		c.removeFromLeft(2);
		c.removeFromRight(2);
		c.removeFromBottom(32);

		g.setColour(tc.withAlpha(0.04f));
		g.fillRoundedRectangle(c.toFloat(), 3.0f);

	}

	if(foldable)
	{
		if(auto laf = dynamic_cast<Dialog::LookAndFeelMethods*>(&rootDialog.getLookAndFeel()))
		{
			auto ta = b.removeFromTop(titleHeight).toFloat();
			laf->drawMultiPageFoldHeader(g, *this, ta, title, folded);
		}
	}
}

Column::Column(Dialog& r, int width, const var& obj):
	Container(r, width, obj)
{
	padding = (int)obj[mpid::Padding];

	
    

	setSize(width, 0);
}

void Column::createEditor(Dialog::PageInfo& xxx)
{
    auto& tt = xxx.addChild<Type>({
        { mpid::Type, "Column" },
        { mpid::ID, "Type"}
    });
    
    auto& prop = xxx.addChild<List>();
    
    xxx[mpid::Text] = "Column";
    
    prop[mpid::Folded] = false;
    prop[mpid::Foldable] = true;
    prop[mpid::Padding] = 10;
    prop[mpid::Text] = "Properties";
    
    auto& listId = prop.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },
        { mpid::Help, "The ID. This will be used for identification in some logic cases" }
    });

    if(!xxx[mpid::Value].isUndefined())
    {
        auto v = xxx[mpid::Value].toString();

	    listId[mpid::Value] = v;
    }
	
    prop.addChild<TextInput>({
        { mpid::ID, "Padding" },
        { mpid::Text, "Padding" },
        { mpid::Help, "The spacing between child elements in pixel." }
    });

	prop.addChild<TextInput>({
        { mpid::ID, "Width" },
        { mpid::Text, "Width" },
		{ mpid::ParseArray, true },
        { mpid::Help, "The relative width between child elements." }
    });
}

Result Column::customCheckOnAdd(PageBase* b, const var& obj)
{
	auto ar = obj[mpid::Width];

	if(ar.isArray())
	{
		if(ar.size() != childItems.size())
			return Result::fail("Width array size mismatch. Must have as many elements as children");
	}

	Container::customCheckOnAdd(b, obj);

	resized();
	return Result::ok();
}


void Column::editModeChanged(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	Container::editModeChanged(isEditMode);

	if(overlay != nullptr && findParentComponentOfClass<Dialog::ModalPopup>() == nullptr)
	{
		overlay->localBoundFunction = [](Component* c){ return c->getLocalBounds().removeFromRight(64); };

		overlay->setOnClick(BIND_MEMBER_FUNCTION_0(Container::addWithPopup));
	}

	calculateSize();
	resized();
#endif
}


void Column::paint(Graphics& g)
{
	if(rootDialog.isEditModeEnabled() && findParentComponentOfClass<Dialog::ModalPopup>() == nullptr)
	{
		g.setColour(Colours::white.withAlpha(0.02f));
		g.fillRect(this->getLocalBounds());

		auto ta = getLocalBounds().removeFromRight(64).toFloat().reduced(5.0f);
		g.setColour(Colours::white.withAlpha(0.1f));
		g.fillRoundedRectangle(ta, 3.0f);

		g.setFont(Dialog::getDefaultFont(*this).first);
		g.setColour(Colours::white.withAlpha(0.7f));
		g.drawText("Edit", ta, Justification::centred);
	}
}

void Column::calculateSize()
{
	auto widthList = infoObject[mpid::Width];

	widthInfo.clear();

    if(childItems.size() > 0)
    {
        auto equidistance = -1.0 / childItems.size();
        
        for(int i = 0; i < childItems.size(); i++)
        {
            auto v = widthList.isArray() ?  widthList[i] : var();
            
            if(v.isUndefined() || v.isVoid())
                widthInfo.add(equidistance);
            else
                widthInfo.add((double)v);
        }
    }

	int h = rootDialog.isEditModeEnabled() ? 32 : 0;

	

	for(auto& c: childItems)
		h = jmax(h, c->getHeight());
	        
	setSize(getWidth(), h);
}

void Column::resized()
{
	auto b = getLocalBounds();

	if(b.isEmpty())
		return;

	auto fullWidth = getWidth();

	auto showEditButton = rootDialog.isEditModeEnabled() && findParentComponentOfClass<Dialog::ModalPopup>() == nullptr;

	if(showEditButton)
		fullWidth -= 64;
        
	for(const auto& w: widthInfo)
	{
		if(w > 0.0)
			fullWidth -= (int)w;
		
		fullWidth -= padding;
	}

	// last padding doesn't count
	fullWidth += padding;

	
	for(int i = 0; i < childItems.size(); i++)
	{
		auto w = widthInfo[i];

		if(w < 0.0)
			w = fullWidth * (-1.0) * w;

        auto cb = b.removeFromLeft(roundToInt(w));
        
        if(!cb.isEmpty())
        {
            childItems[i]->setBounds(cb);
            childItems[i]->resized();
        }
        
		b.removeFromLeft(padding);
	}
}

void Column::postInit()
{
	if(!staticPages.isEmpty())
	{
		auto equidistance = -1.0 / staticPages.size();
            
		for(auto sp: staticPages)
		{
			auto v = (*sp)[mpid::Width];
                
			if(v.isUndefined() || v.isVoid())
				widthInfo.add(equidistance);
			else
				widthInfo.add((double)v);
		}
	}
	    
	Container::postInit();
}

Branch::Branch(Dialog& root, int w, const var& obj):
	Container(root, w, obj)
{
	setSize(w, 0);
}


void Branch::editModeChanged(bool isEditMode)
{
#if HISE_MULTIPAGE_INCLUDE_EDIT
	Container::editModeChanged(isEditMode);

	if(overlay != nullptr)
	{
		overlay->localBoundFunction = [](Component* c){ return c->getLocalBounds().removeFromBottom(32); };
		overlay->setOnClick(BIND_MEMBER_FUNCTION_0(Container::addWithPopup));
	}

	calculateSize();
	resized();
#endif
}

void Branch::paint(Graphics& g)
{
	paintEditBounds(g);

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
        
	for(const auto& sp: staticPages)
	{
		childItems.add(sp->create(rootDialog, getWidth()));
		addAndMakeVisible(childItems.getLast());

		
	}

	if(rootDialog.isEditModeEnabled())
	{
		for(auto c: childItems)
		{
			c->setStateObject(stateObject);

			if(stateObject.hasProperty(c->getId()))
			{
				c->clearInitValue();
			}

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
	
	calculateSize();
}

void Branch::calculateSize()
{
	if(rootDialog.isEditModeEnabled())
	{
		int h = 32;

		for(auto c: childItems)
		{
			h += c->getHeight() + 10;
		}

		setSize(getWidth(), h);
	}
	else
	{
		if(auto c = childItems[0])
		{
			c->setVisible(true);
			setSize(getWidth(), c->getHeight());
		}
	}
}

Result Branch::checkGlobalState(var globalState)
{
	if(auto p = childItems[0])
		return p->check(globalState);
        
	return Result::fail("No branch selected");
}

void Branch::createEditor(Dialog::PageInfo& rootList)
{
    auto& tt = rootList.addChild<Type>({
        { mpid::Type, "Branch" },
        { mpid::ID, "Type"}
    });
    
    auto& prop = rootList.addChild<List>();
    
    rootList[mpid::Text] = "List";
    
    prop[mpid::Folded] = false;
    prop[mpid::Foldable] = true;
    prop[mpid::Padding] = 10;
    prop[mpid::Text] = "Properties";
    
    auto& listId = prop.addChild<TextInput>({
        { mpid::ID, "ID" },
        { mpid::Text, "ID" },
        { mpid::Required, true },
        { mpid::Items, rootDialog.getExistingKeysAsItemString() },
        { mpid::Help, "The ID of the branch. This will be used as key to fetch the value from the global state to determine which child to show." }
    });
}

void Branch::resized()
{
	if(rootDialog.isEditModeEnabled())
	{
		auto b = getLocalBounds();

		b.removeFromLeft(getWidth() / 4);

		for(auto c: childItems)
		{
			c->setBounds(b.removeFromTop(c->getHeight()));
			b.removeFromTop(10);
		}
	}
	else
	{
		auto b = getLocalBounds();

		if(b.isEmpty())
			return;

		if(auto p = childItems[0])
			p->setBounds(b);
	}
	
}

} // PageFactory
} // multipage
} // hise
