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
    virtual ~Container() {};

    void postInit() override;
    Result checkGlobalState(var globalState) override;
    virtual void calculateSize() = 0;
    void addChild(Dialog::PageInfo::Ptr info);

protected:

    Identifier id;
    OwnedArray<PageBase> childItems;
    Dialog::PageInfo::List staticPages;

private:

    void addChild(int width, const var& r);
    Dialog::Factory factory;
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

    List(Dialog& r, int width, const var& obj);
    ~List() override {};

    void resized() override;
    void mouseDown(const MouseEvent& e) override;
    void paint(Graphics& g) override;

    void calculateSize() override;
    void createEditorInfo(Dialog::PageInfo* info) override;
    
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

    Column(Dialog& r, int width, const var& obj);

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
    
    Branch(Dialog& root, int w, const var& obj);;

    PageBase* createFromStateObject(const var& obj, int w);
    PageBase* createWithPopup(int width);

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
    
    Builder(Dialog& r, int w, const var& obj);

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