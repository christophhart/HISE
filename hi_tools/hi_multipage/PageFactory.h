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

namespace hise
{


namespace multipage {
using namespace juce;

struct Factory: public PathFactory
{
    Factory();
    Dialog::PageInfo::Ptr create(const var& obj);

    StringArray getPopupMenuList() const;

    Colour getColourForCategory(const String& typeName) const;
    
    bool needsIdAtCreation(const String& id) const;

    StringArray getIdList() const;
    
    Path createPath(const String& url) const override;
    
private:

    template <typename T> void registerPage();

    struct Item
    {
        bool isContainer;
	    Identifier id;
        Identifier category;
        Dialog::PageInfo::CreateFunction f;
    };

    Array<Item> items;
};

namespace factory
{

struct Type: public Dialog::PageBase
{
	DEFAULT_PROPERTIES(MarkdownText)
    {
        return { { mpid::ID, "Type" } };
    }

    Type(Dialog& r, int width, const var& d);

    void resized() override;

	Result checkGlobalState(var globalState) override;
	void paint(Graphics& g) override;
	String typeId;
};

struct Spacer: public Dialog::PageBase
{
	DEFAULT_PROPERTIES(Spacer)
    {
        return { };
    }

    static String getCategoryId() { return "Layout"; }

    Spacer(Dialog& r, int width, const var& d):
      PageBase(r, width, d)
	{
        setSize(width, 0);
	}

    void createEditor(Dialog::PageInfo& rootList) override;
	void editModeChanged(bool isEditMode) override { repaint(); };
    void postInit() override {};
    void paint(Graphics& g) override
    {
	    if(isEditModeAndNotInPopup())
	    {
            g.setColour(Dialog::getDefaultFont(*this).second.withAlpha(0.3f));
		    g.drawRoundedRectangle(getLocalBounds().toFloat(), 5.0f, 1.0f);
	    }
    }
    Result checkGlobalState(var) override { return Result::ok(); }
    
};

struct EventLogger: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(EventLogger)
    {
        return { };
    }

    EventLogger(Dialog& r, int w, const var& obj):
      PageBase(r, w, obj),
      console(r.getState())
    {
	    addAndMakeVisible(console);
        setSize(w, 200);
    }

    static String getCategoryId() { return "Layout"; }

    Result checkGlobalState(var) override { return Result::ok(); }

    void resized() override
    {
	    console.setBounds(getLocalBounds());
    }

    EventConsole console;
};

struct SimpleText: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(SimpleText)
    {
        return { { mpid::Text, "### funkyNode" } };
    }

	SimpleText(Dialog& r, int width, const var& obj);
    
    static String getCategoryId() { return "Layout"; }

    void createEditor(Dialog::PageInfo& rootList) override;

    Result checkGlobalState(var) override { return Result::ok(); }
};

struct MarkdownText: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(MarkdownText)
    {
        return { { mpid::Text, "### funkyNode" } };
    }

    static String getCategoryId() { return "Layout"; }
    static String getString(const String& markdownText, Dialog& parent);

    MarkdownText(Dialog& r, int width, const var& d);

    void createEditor(Dialog::PageInfo& rootList) override;
    
    void postInit() override;

    void resized() override
    {
        FlexboxComponent::resized();
        display.resized();
    }
    //void paint(Graphics& g) override;
    Result checkGlobalState(var) override;

private:

    var obj;
    float width = 0.0f;
    SimpleMarkdownDisplay display;

    //MarkdownRenderer r;
};

}

}
}
