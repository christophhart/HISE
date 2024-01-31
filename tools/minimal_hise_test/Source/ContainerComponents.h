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

    void paintEditBounds(Graphics& g)
    {
        if(overlay != nullptr)
        {
	        auto b = overlay->localBoundFunction(this).reduced(5);

            g.setColour(Colours::white.withAlpha(0.05f));
			g.fillRoundedRectangle(b.toFloat(), 3.0f);
			g.setColour(Colours::white.withAlpha(0.8f));

			g.setFont(Dialog::getDefaultFont(*this).first);
			g.drawText("Click to edit " + getContainerTypeId().toString(), b, Justification::centred);
        }
    }

    // Call this in the custom callback to ensure that all children are evaluated before the parent check...
    static Result checkChildren(PageBase* b, const var& toUse)
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

    static String getCategoryId() { return "Layout"; }

    // Call this whenever a size changes...
    static void recalculateParentSize(PageBase* b);

    void postInit() override;
    Result checkGlobalState(var globalState) override;
    virtual void calculateSize() = 0;
    void addChild(Dialog::PageInfo::Ptr info);

    virtual Identifier getContainerTypeId() const = 0;

    static void addChildrenBuilder(Dialog::PageInfo* rootList);
    
    void clearInitValue() override;

    void addWithPopup();

    virtual Result customCheckOnAdd(PageBase* b, const var& obj)
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

protected:

    int padding = 0;
    Identifier id;
    OwnedArray<PageBase> childItems;
    Dialog::PageInfo::List staticPages;

private:

    void addChild(int width, const var& r);
    Dialog::Factory factory;

    JUCE_DECLARE_WEAK_REFERENCEABLE(Container);
};

struct List: public Container
{
    DEFAULT_PROPERTIES(List)
    {
        return {
            { mpid::Text, "Title" },
            { mpid::Padding, true },
            { mpid::Foldable, false },
            { mpid::Folded, false }
        };
    }

    Identifier getContainerTypeId() const override { return getStaticId(); }

    List(Dialog& r, int width, const var& obj);
    ~List() override
    {
        if(overlay != nullptr)
			rootDialog.removeChildComponent(overlay);

	    overlay = nullptr;
    };

    Result customCheckOnAdd(PageBase* b, const var& obj) override
    {
        auto ok = Container::customCheckOnAdd(b, obj);
    	
		foldable = obj[mpid::Foldable];
		folded = obj[mpid::Folded];
		title = obj[mpid::Text];

		calculateSize();
		return Result::ok();
    }

    void editModeChanged(bool isEditMode) override;

    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void paint(Graphics& g) override;

    void calculateSize() override;

    void createEditor(Dialog::PageInfo& info) override;

    int titleHeight = 32;
    Path fold;
    String title;
    bool foldable = false;
    bool folded = false;
};

struct Column: public Container
{
    DEFAULT_PROPERTIES(Column)
    {
        return {
            { mpid::ID, "columnId" },
            { mpid::Padding, true },
            { mpid::Width, var(Array<var>({var(-0.5), var(-0.5)})) }
        };
    }

    Identifier getContainerTypeId() const override { return getStaticId(); }

    Column(Dialog& r, int width, const var& obj);

    void createEditor(Dialog::PageInfo& info) override;

    Result customCheckOnAdd(PageBase* b, const var& obj) override
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

    void editModeChanged(bool isEditMode) override
    {
	    Container::editModeChanged(isEditMode);

        if(overlay != nullptr)
        {
	        overlay->localBoundFunction = [](Component* c){ return c->getLocalBounds().removeFromRight(64); };

            overlay->setOnClick(BIND_MEMBER_FUNCTION_0(Container::addWithPopup));
        }

        calculateSize();
        resized();
    }

    void paint(Graphics& g) override
    {
	    if(editMode)
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

    void calculateSize() override;
	void resized() override;
    void postInit() override;

    Array<double> widthInfo;
};

struct Branch: public Container
{
    DEFAULT_PROPERTIES(Branch)
    {
        return {
            { mpid::ID, "branchId" }
        };
    }

    Identifier getContainerTypeId() const override { return getStaticId(); }

    static void createEditor(Dialog::PageInfo::Ptr info)
    {
    }

    Branch(Dialog& root, int w, const var& obj);;

    PageBase* createFromStateObject(const var& obj, int w);
    PageBase* createWithPopup(int width);

    void editModeChanged(bool isEditMode) override
    {
	    Container::editModeChanged(isEditMode);

        if(overlay != nullptr)
        {
	        overlay->localBoundFunction = [](Component* c){ return c->getLocalBounds().removeFromBottom(32); };
            overlay->setOnClick(BIND_MEMBER_FUNCTION_0(Container::addWithPopup));
        }

        calculateSize();
        resized();
    }



    void addChildrenBuilderToContainerChildren()
    {
        Dialog::Factory f;

        auto containerTypes = f.getIdList();

        for(auto& s: staticPages)
        {
            if(auto isContainer = containerTypes.contains(s->data[mpid::Text].toString()))
            {
	            addChildrenBuilder(s);
            }
	        
        }
    }

    void paint(Graphics& g) override
    {
	    paintEditBounds(g);

        if(editMode)
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

    void postInit() override;
    void calculateSize() override;
    Result checkGlobalState(var globalState) override;
    void resized() override;
    
    int currentIndex = 0;
};

struct Builder: public Container,
                public ButtonListener
{
    DEFAULT_PROPERTIES(Builder)
    {
        return {
            { mpid::ID, "columnId" },
            { mpid::Text, "title" },
            { mpid::Folded, false },
            { mpid::Padding, 10 }
        };
    }

    Identifier getContainerTypeId() const override { return getStaticId(); }

    Builder(Dialog& r, int w, const var& obj);

    static void createEditor(Dialog::PageInfo::Ptr info)
    {
    }

    void resized() override;
    void paint(Graphics& g) override;
    void mouseDown(const MouseEvent& e) override;
    void buttonClicked(Button* b) override;

    void calculateSize() override;
    void postInit() override;
    Result checkGlobalState(var globalState) override;
    void addChildItem(PageBase* b, const var& stateInArray);

    void createItem(const var& stateInArray);
    void onAddButton();
    void rebuildPosition();

    var stateList;

private:

    RectangleList<int> itemBoxes;

    bool folded = false;
    ScopedPointer<Branch> popupOptions;
    String title;
    HiseShapeButton addButton;
    OwnedArray<HiseShapeButton> closeButtons;
};

} // factory
} // multipage
} // hise