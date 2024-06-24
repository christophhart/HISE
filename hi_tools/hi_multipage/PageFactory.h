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
#include "MultiPageDialog.h"
#include "MultiPageDialog.h"

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

    String getCategoryName(const String& id) const;

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

struct PlaceholderContentBase
{
	virtual ~PlaceholderContentBase() {};

    PlaceholderContentBase(Dialog& r, const var& obj):
      rootDialog(r),
      infoObject(obj)
    {}

    virtual void postInit() = 0;
    virtual Result checkGlobalState(var state) = 0;

    Dialog& rootDialog;
    var infoObject;
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

    Path icon;
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

    CREATE_EDITOR_OVERRIDE;

	void editModeChanged(bool isEditMode) override { repaint(); };
    void postInit() override {};
    void paint(Graphics& g) override;
	Result checkGlobalState(var) override { return Result::ok(); }
    
};

struct HtmlElement: public Dialog::PageBase
{
	DEFAULT_PROPERTIES(HtmlElement)
    {
        return { };
    }

    static String getCategoryId() { return "Layout"; }

	HtmlElement(Dialog& r, int width, const var& d);

    CREATE_EDITOR_OVERRIDE;

	void editModeChanged(bool isEditMode) override { repaint(); };
	void postInit() override;;

    Result checkGlobalState(var) override { return Result::ok(); }

    OwnedArray<PageBase> childElements;
};

struct Image: public Dialog::PageBase
{
	DEFAULT_PROPERTIES(Image)
    {
        return { };
    }

    static String getCategoryId() { return "Layout"; }

    Image(Dialog& r, int width, const var& d);

	CREATE_EDITOR_OVERRIDE;

	void editModeChanged(bool isEditMode) override { repaint(); };
    void postInit() override;;
    Result checkGlobalState(var) override { return Result::ok(); }
    
    simple_css::CSSImage img;
};

struct EventLogger: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(EventLogger)
    {
        return { };
    }

    EventLogger(Dialog& r, int w, const var& obj);

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

    void postInit() override;

    CREATE_EDITOR_OVERRIDE;

    Result checkGlobalState(var) override { return Result::ok(); }
};

struct MarkdownText: public Dialog::PageBase
{
    DEFAULT_PROPERTIES(MarkdownText)
    {
        return { { mpid::Text, "### funkyNode" } };
    }

    static String getCategoryId() { return "Layout"; }
    static String getString(const String& markdownText, const State& parent);

    MarkdownText(Dialog& r, int width, const var& d);

    CREATE_EDITOR_OVERRIDE;
    
    void postInit() override;

    void resized() override;
    //void paint(Graphics& g) override;
    Result checkGlobalState(var) override;

private:

    var obj;
    float width = 0.0f;
    SimpleMarkdownDisplay display;

    //MarkdownRenderer r;
};



struct DummyContent: public Component,
					 public PlaceholderContentBase
{
    DummyContent(Dialog& r, const var& infoObject):
      PlaceholderContentBase(r, infoObject)
    {
	    classId = infoObject[mpid::ContentType].toString();
    }

    void paint(Graphics& g) override
    {
	    g.fillAll(Colours::white.withAlpha(0.1f));
        g.setColour(Colours::white.withAlpha(0.7f));
        g.setFont(GLOBAL_MONOSPACE_FONT());
        g.drawRect(getLocalBounds(), 1);
        g.drawText("Placeholder for " + classId, getLocalBounds().toFloat(), Justification::centred);
    }

    void postInit() override {};
    Result checkGlobalState(var state) override { return Result::ok(); }

    String classId;

#if HISE_MULTIPAGE_INCLUDE_EDIT
	static void createEditor(const var& infoObject, Dialog::PageInfo& rootList);
#endif
};

template <typename ContentType> struct Placeholder: public Dialog::PageBase
{
    struct Content: public Component
    {
	    virtual Result check(const var& state) = 0;
    };

    DEFAULT_PROPERTIES(Placeholder)
    {
        return {};
    }

    static String getCategoryId() { return "Layout"; }

    Placeholder(Dialog& r, int width, const var& d):
      PageBase(r, width, d)
    {
        static_assert(std::is_base_of<PlaceholderContentBase, ContentType>(),
                      "not a base of multipage::factory::PlaceholderContentBase");

        static_assert(std::is_base_of<juce::Component, ContentType>(),
					  "not a base of juce::Component");

        content = new ContentType(r, d);

        Helpers::setFallbackStyleSheet(*this, "display:flex;min-height:32px;width:100%;");
        Helpers::setFallbackStyleSheet(*content, "width:100%;height:100%;");

	    addFlexItem(*content);
        setSize(width, 0);
    };

#if HISE_MULTIPAGE_INCLUDE_EDIT
    void createEditor(Dialog::PageInfo& rootList) override
    {
	    DummyContent::createEditor(infoObject, rootList);
    }
#endif
    
    void postInit() override
    {
	    content->postInit();
    }

    void resized() override
    {
        FlexboxComponent::resized();
    }
    Result checkGlobalState(var state) override
    {
	    return content->checkGlobalState(state);
    }

private:

    var obj;
    float width = 0.0f;
    ScopedPointer<ContentType> content;

    //MarkdownRenderer r;
};

struct TagList: public Dialog::PageBase,
                public Button::Listener
{
    DEFAULT_PROPERTIES(TagList)
    {
        return { { mpid::Text, "### funkyNode" } };
    }
    
    static String getCategoryId() { return "UI Elements"; }
    
    TagList(Dialog& parent, int w, const var& obj);

    void buttonClicked(juce::Button*) override;

    CREATE_EDITOR_OVERRIDE;

    void postInit() override;

    Result checkGlobalState(var state) override
    {
        return Result::ok();
    }

    OwnedArray<TextButton> buttons;
};

struct Table: public Dialog::PageBase,
              public juce::TableListBoxModel
{
    struct CellComponent: public Component
    {
        CellComponent(Table& parent_);

        void update(Point<int> newPos, const String& newContent);

        void mouseDoubleClick(const MouseEvent& e) override
        {
            parent.cellDoubleClicked(cellPosition.y, cellPosition.x, e);
        }

        void mouseDown(const MouseEvent& event) override
        {
            parent.table.selectRowsBasedOnModifierKeys(cellPosition.y, event.mods, false);
        }

        void mouseUp(const MouseEvent& event) override
        {
            parent.table.selectRowsBasedOnModifierKeys(cellPosition.y, event.mods, true);
        }

        void paint(Graphics& g) override;

        Point<int> cellPosition;

        Table& parent;
        String content;
    };
    
    Table(Dialog& parent, int w, const var& obj);

    ScrollbarFader sf;
    
    Array<var> items;
    Array<std::pair<int, var>> visibleItems;

    void rebuildColumns();

    DEFAULT_PROPERTIES(Table)
    {
        return { { mpid::Text, "### funkyNode" } };
    }
    
    static String getCategoryId() { return "UI Elements"; }

    String getCellContent(int columnId, int rowNumber) const;
    
    Component* refreshComponentForCell (int rowNumber, int columnId, bool isRowSelected,
                                                Component* existingComponentToUpdate) override;

    static String itemsToString(const var& data);

    static Array<var> stringToItems(const var& data);

    CREATE_EDITOR_OVERRIDE;

    juce::JavascriptEngine* currentEngine = nullptr;

    int getNumRows() override;

    void rebuildRows();

    void postInit() override;

    void paintCell (Graphics&,
                    int rowNumber,
                    int columnId,
                    int width, int height,
                    bool rowIsSelected) override {};

    int originalSelectionIndex = -1;

    void paintRowBackground (Graphics& g, int rowNumber, int width, int height, bool rowIsSelected) override;

    Result checkGlobalState(var state) override;

    void paint(Graphics& g) override;

    enum class EventType
    {
	    CellClick,
        RowClick,
        DoubleClick,
        RowSelection,
        ReturnKey
    };

    void cellClicked (int rowNumber, int columnId, const MouseEvent&) override
    {
	    updateValue(EventType::CellClick, rowNumber, columnId);
    }
    void cellDoubleClicked (int rowNumber, int columnId, const MouseEvent&) override
    {
	    updateValue(EventType::DoubleClick, rowNumber, columnId);
    }


    void backgroundClicked (const MouseEvent&) override
    {
	    updateValue(EventType::CellClick, -1, -1);
    }
    void selectedRowsChanged (int lastRowSelected) override
    {
	    updateValue(EventType::RowSelection, lastRowSelected, -1);
    }

    void returnKeyPressed (int lastRowSelected) override
    {
	    updateValue(EventType::ReturnKey, lastRowSelected, -1);
    }

    void updateValue(EventType t, int row, int column);

    TableListBox table;

    struct TableRepainter: public MouseListener
    {
        TableRepainter(TableListBox& t):
          MouseListener(),
          table(t)
        {
            t.addMouseListener(this, true);
        }
        
        void mouseMove(const MouseEvent& event) override;

        Component::SafePointer<Component> lastComponent = nullptr;
        TableListBox& table;
    } repainter;

    Identifier getFilterFunctionId() const
    {
	    auto filterFunction = infoObject[mpid::FilterFunction].toString();
        if(filterFunction.isEmpty())
            return {};

        return Identifier(filterFunction);
    }
};

}

}
}
