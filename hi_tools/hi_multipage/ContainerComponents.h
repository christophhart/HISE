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

#pragma once


namespace hise {
namespace multipage {
namespace factory {
using namespace juce;

struct Container: public Dialog::PageBase
{
    Container(Dialog& r, int width, const var& obj);;
    virtual ~Container() {}

    // Call this in the custom callback to ensure that all children are evaluated before the parent check...
    static Result checkChildren(PageBase* b, const var& toUse);
    static String getCategoryId() { return "Layout"; }

    // Call this whenever a size changes...
    static void recalculateParentSize(PageBase* b);

    void postInit() override;
    Result checkGlobalState(var globalState) override;
    
    void addChild(Dialog::PageInfo::Ptr info);

    virtual Identifier getContainerTypeId() const = 0;

    void clearInitValue() override;

    void addWithPopup();

    virtual Result customCheckOnAdd(PageBase* b, const var& obj);

protected:

    void rebuildChildren()
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

    OwnedArray<PageBase> childItems;
    Dialog::PageInfo::List staticPages;

private:

    void addChild(int width, const var& r);
    Factory factory;

    JUCE_DECLARE_WEAK_REFERENCEABLE(Container);
};

struct List: public Container
{
    DEFAULT_PROPERTIES(List)
    {
        return {
            { mpid::Text, "Title" },
            { mpid::Foldable, false },
            { mpid::Folded, false }
        };
    }

    Identifier getContainerTypeId() const override { return getStaticId(); }

    List(Dialog& r, int width, const var& obj);
    ~List() override;;

    Result customCheckOnAdd(PageBase* b, const var& obj) override;

    void refreshFold();

    void createEditor(Dialog::PageInfo& info) override;

    void postInit() override
    {
        Container::postInit();
        
        if(foldable)
        {
            foldButton->setToggleState(folded, dontSendNotification);
            refreshFold();
        }
    }
    
    Path fold;
    String title;
    bool foldable = false;
    bool folded = false;

    ScopedPointer<TextButton> foldButton;
};

struct Column: public Container
{
    DEFAULT_PROPERTIES(Column)
    {
        return { { mpid::ID, "columnId" }};
    }

    Identifier getContainerTypeId() const override { return getStaticId(); }
    Column(Dialog& r, int width, const var& obj);

    void createEditor(Dialog::PageInfo& info) override;
};

struct Branch: public Container
{
    DEFAULT_PROPERTIES(Branch)
    {
        return { { mpid::ID, "branchId" } };
    }

    Branch(Dialog& root, int w, const var& obj);;

    Identifier getContainerTypeId() const override { return getStaticId(); }
    void createEditor(Dialog::PageInfo& info) override;
    
    void paint(Graphics& g) override;
    void postInit() override;
    Result checkGlobalState(var globalState) override;
    
    int currentIndex = 0;
};



} // factory
} // multipage
} // hise
