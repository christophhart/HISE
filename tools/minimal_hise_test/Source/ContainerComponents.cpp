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

	auto l = obj["Children"];
        
	if(l.isArray())
	{
		for(auto& r: *l.getArray())
			addChild(width, r);
	}
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

void Container::addChildrenBuilder(Dialog::PageInfo* info)
{
	Dialog::Factory f;

    auto& xxx = *info;
    auto& ff = xxx.addChild<Builder>({
        { mpid::ID, "Children" },
        { mpid::Text, "Children" }
    });
    
    auto& typeOptions = ff.addChild<Branch>();
    
    for(auto type: f.getIdList())
    {
		

#if 0

		typeOptions.childItems.add(f.createEditor(Identifier(type)));

		// TODO: Change to PageInfo and make sure that it doesn't create recursive loops...
		if(f.createContainerPrototype(Identifier(type), rootDialog, getWidth())) 
		{
			
		}
        else
        {
            auto& itemBuilder =  typeOptions.addChild<List>();
            itemBuilder[mpid::Text] = type;

            auto obj = new DynamicObject();
            obj->setProperty(mpid::ID, type + "Id");
            obj->setProperty(mpid::Type, type);
            
            Dialog::PageInfo::Ptr c = f.create(obj);
            ScopedPointer<Dialog::PageBase> c2 = c->create(rootDialog, 0);
            c2->createEditorInfo(&itemBuilder);
        }
#endif
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
	PopupLookAndFeel plaf;
	PopupMenu m;
	m.setLookAndFeel(&plaf);

	m.addItem(1, "Add new item");
	m.addItem(2, "Edit properties");

	if(auto r = m.show())
	{
		if(r == 1)
		{
			auto& pp = rootDialog.createModalPopup<List>({ { mpid::Padding, 10} });

			auto no = new DynamicObject();

			if(auto ar = infoObject[mpid::Children].getArray())
				ar->add(no);
			else
			{
				Array<var> newList;
				newList.add(no);
				infoObject.getDynamicObject()->setProperty(mpid::Children, var(newList));
			}

			pp.setStateObject(no);

			pp.addChild<MarkdownText>({
				{ mpid::Text, "### Add item\nPlease select the type and enter the ID that you want to use "}});

			Dialog::Factory f;
			auto list = f.getPopupMenuList();

			pp.addChild<Choice>({
				{ mpid::ID, "Type" },
				{ mpid::Text, "Type" },
				{ mpid::Custom, true },
				{ mpid::Help, "The item type (UI element or action)."},
				{ mpid::Items, list.joinIntoString("\n") },
				{ mpid::Value, "TextInput" }
			});

			pp.addChild<TextInput>(
				{
					{ mpid::ID, "Text" },
					{ mpid::Text, "Text" },
					{ mpid::Help, "The text label that is shown next to the UI element (or the markdown content)" }, 
					{ mpid::Value, "Some Label"}
				});

			pp.addChild<TextInput>(
			{
				{ mpid::ID, "ID" },
				{ mpid::Text, "ID" },
				{ mpid::Help, "The ID that is used as key in the JSON state object." }, 
				{ mpid::Value, ""}
			});
				
			pp.setCustomCheckFunction([this](PageBase* b, const var& obj)
			{
				if(auto c = dynamic_cast<Container*>(b))
					c->checkGlobalState(obj);

				childItems.clear();
				Dialog::Factory f;
				auto l = infoObject["Children"];
				
				if(l.isArray())
				{
					for(auto& r: *l.getArray())
					{
						if(auto pi = f.create(r))
						{
							childItems.add(pi->create(rootDialog, getWidth()));
							addAndMakeVisible(childItems.getLast());
							childItems.getLast()->postInit();
						}
					}
				}

				Container* c = this;

				while(c != nullptr)
				{
					c->calculateSize();
					c->resized();

					c = c->findParentComponentOfClass<Container>();
				}
				
				return Result::ok();
			});

			rootDialog.showModalPopup(true);

		}
		else
		{
			auto& x = this->rootDialog.createModalPopup<List>();
			x.setStateObject(infoObject);
			this->createEditor(x);

			x.setCustomCheckFunction(BIND_MEMBER_FUNCTION_2(Container::customCheckOnAdd));

			this->rootDialog.showModalPopup(true);
		}
	}
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

	if(editMode)
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

    auto& foldId1 = prop.addChild<Tickbox>({
        { mpid::ID, "Foldable" },
        { mpid::Text, "Foldable" },
        { mpid::Help, "If ticked, then this list will show a clickable header that can be folded" },
		{ mpid::Value, foldable }
    });
    
    auto& foldId2 = prop.addChild<Tickbox>({
        { mpid::ID, "Folded" },
        { mpid::Text, "Folded" },
        { mpid::Help, "If ticked, then this list will folded as default state" },
		{ mpid::Value, folded }
    });
}

void List::editModeChanged(bool isEditMode)
{
	PageBase::editModeChanged(isEditMode);

	if(overlay != nullptr)
	{
		overlay->localBoundFunction = [](Component* c)
		{
			return c->getLocalBounds().removeFromBottom(32);
		};

		overlay->setOnClick(BIND_MEMBER_FUNCTION_0(Container::addWithPopup));
	}
}

void List::resized()
{
	auto b = getLocalBounds();

	if(b.isEmpty())
		return;

    if(foldable)
        b.removeFromTop(24 + padding);

	if(editMode)
		b.removeFromLeft(10);

    if(!folded)
    {
        for(auto c: childItems)
        {
            c->setBounds(b.removeFromTop(c->getHeight()));
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

	auto b = getLocalBounds();

	if(editMode)
	{
		auto c = b.removeFromLeft(10);

		c.removeFromLeft(2);
		c.removeFromRight(2);
		c.removeFromBottom(32);

		g.setColour(Colours::white.withAlpha(0.04f));
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
        { mpid::Required, true},
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

	int h = editMode ? 32 : 0;

	

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

	if(editMode)
		fullWidth -= 64;
        
	for(const auto& w: widthInfo)
	{
		if(w > 0.0)
			fullWidth -= (int)w;

		fullWidth -= padding;
	}
	
	for(int i = 0; i < childItems.size(); i++)
	{
		auto w = widthInfo[i];

		if(w < 0.0)
			w = fullWidth * (-1.0) * w;

		childItems[i]->setBounds(b.removeFromLeft(roundToInt(w)));
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

Dialog::PageBase* Branch::createFromStateObject(const var& obj, int w)
{
	for(auto& sp: staticPages)
	{
		if(sp->data[mpid::Text].toString() == obj[mpid::Type].toString())
			return sp->create(rootDialog, w);
			
	}

	return nullptr;
}

Dialog::PageBase* Branch::createWithPopup(int width)
{
	PopupLookAndFeel plaf;
	PopupMenu m;
	m.setLookAndFeel(&plaf);
        
	int index = 0;
        
	for(auto sp: staticPages)
	{
		m.addItem(index + 1, sp->getData()[mpid::Text].toString());
		index++;
	}
        
	auto result = m.show();
        
	if(isPositiveAndBelow(result-1, staticPages.size()))
	{
		if(auto sp = staticPages[result - 1])
			return sp->create(rootDialog, width);
	}
        
	return nullptr;
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

	if(editMode)
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
	if(editMode)
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

void Branch::resized()
{
	if(editMode)
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

Builder::Builder(Dialog& r, int w, const var& obj):
	Container(r, w, obj),
	addButton("add", nullptr, r)
{
	title = obj[mpid::Text];
	padding = jmax(padding, 10);
        
	childItems.clear();
	addAndMakeVisible(addButton);
	addButton.onClick = BIND_MEMBER_FUNCTION_0(Builder::onAddButton);
	setSize(w, 32);
}

void Builder::mouseDown(const MouseEvent& e)
{
	if(e.getPosition().getY() < 32)
	{
		folded = !folded;
		rebuildPosition();
		repaint();
	}
}

void Builder::calculateSize()
{
	int h = 32 + padding;
        
	for(auto& c: childItems)
	{
		c->setVisible(!folded);
            
		if(!folded)
			h += c->getHeight() + 3 * padding;
	}
        
	setSize(getWidth(), h);
}

void Builder::resized()
{
	auto b = getLocalBounds();
	addButton.setBounds(b.removeFromTop(32).removeFromRight(32).withSizeKeepingCentre(24, 24));
	b.removeFromTop(padding);
	itemBoxes.clear();
        
	for(int i = 0; i < childItems.size(); i++)
	{
		auto c = childItems[i];
		auto row = b.removeFromTop(2 * padding + c->getHeight());
		itemBoxes.addWithoutMerging(row);
		auto bb = row.removeFromRight(32).removeFromTop(32).withSizeKeepingCentre(20, 20);
		row = row.reduced(padding);
		closeButtons[i]->setVisible(c->isVisible());
            
		if(!folded)
		{   
			closeButtons[i]->setBounds(bb);
			c->setBounds(row);
			b.removeFromTop(padding);
		}
	}
}

void Builder::paint(Graphics& g)
{
	for(auto& b: itemBoxes)
	{
		g.setColour(Colours::white.withAlpha(0.2f));
		g.drawRoundedRectangle(b.toFloat().reduced(1.0f), 5.0f, 2.0f);
	}
        
	if(auto laf = dynamic_cast<Dialog::LookAndFeelMethods*>(&rootDialog.getLookAndFeel()))
	{
		auto b = getLocalBounds().removeFromTop(32).toFloat();
		laf->drawMultiPageFoldHeader(g, *this, b, title, folded);
	}
}

Result Builder::checkGlobalState(var globalState)
{
	int idx = 0;
        
	for(auto& c: childItems)
	{
		var obj;
            
		if(isPositiveAndBelow(idx, stateList.size()))
			obj = stateList[idx];
            
		if(obj.getDynamicObject() == nullptr)
		{
			stateList.insert(idx, var(new DynamicObject()));
			obj = stateList[idx];
		}
            
		auto ok = c->check(obj);
            
		if(ok.failed())
			return ok;
            
		idx++;
	}
        
	DBG(JSON::toString(stateList));
        
	return Result::ok();
}

void Builder::addChildItem(PageBase* b, const var& stateInArray)
{
	DBG(JSON::toString(stateInArray, true));

	childItems.add(b);
	closeButtons.add(new HiseShapeButton("close", nullptr, rootDialog));
	addAndMakeVisible(closeButtons.getLast());
	addAndMakeVisible(childItems.getLast());
	closeButtons.getLast()->addListener(this);
	childItems.getLast()->setStateObject(stateInArray);

	

	childItems.getLast()->postInit();
}

void Builder::createItem(const var& stateInArray)
{
	for(const auto& sp: staticPages)
	{
		auto b = sp->create(rootDialog, getWidth());
		addChildItem(b, stateInArray);
	}
}

void Builder::onAddButton()
{
	var no(new DynamicObject());
	stateList.insert(-1, no);
	folded = false;
        
	if(popupOptions != nullptr)
	{
		if(auto nb = popupOptions->createWithPopup(getWidth()))
			addChildItem(nb, no);
	}
	else
		createItem(no);
        
	rebuildPosition();
}

void Builder::rebuildPosition()
{
	auto t = dynamic_cast<Container*>(this);
        
	while(t != nullptr)
	{
		t->calculateSize();
		t = t->findParentComponentOfClass<Container>();
	}
	repaint();
}

void Builder::buttonClicked(Button* b)
{
	auto cb = dynamic_cast<HiseShapeButton*>(b);
	auto indexToDelete = closeButtons.indexOf(cb);
        
	stateList.getArray()->remove(indexToDelete);
	closeButtons.removeObject(cb);
	childItems.remove(indexToDelete);
        
	rebuildPosition();
}

void Builder::postInit()
{
	stateList = getValueFromGlobalState();
        
	if(auto firstItem = staticPages.getFirst())
	{
		ScopedPointer<PageBase> fi = firstItem->create(rootDialog, getWidth());
            
		if(auto br = dynamic_cast<Branch*>(fi.get()))
		{
			br->addChildrenBuilderToContainerChildren();
			popupOptions = br;
			fi.release();
		}
	}
        
	if(!stateList.isArray())
	{
		stateList = var(Array<var>());
		writeState(stateList);
	}
	else
	{
		for(auto& obj: *stateList.getArray())
		{
			if(popupOptions != nullptr)
			{
				if(auto nb = popupOptions->createFromStateObject(obj, getWidth()))
				{
					addChildItem(nb, obj);
				}
			}
			else
			{
				createItem(obj);
			}
		}
	}

	calculateSize();
}
} // PageFactory
} // multipage
} // hise