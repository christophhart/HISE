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

#include <JuceHeader.h>

namespace hise
{
using namespace juce;

#define DECLARE_ID(x) static const Identifier x(#x);

namespace MultiPageIds
{
    // Root properties
	DECLARE_ID(StyleData);
    DECLARE_ID(LayoutData);
    DECLARE_ID(Properties);
    DECLARE_ID(GlobalState);
    DECLARE_ID(Children);

    // Global Properties
    DECLARE_ID(Header);
    DECLARE_ID(Subtitle);

    // Child properties
    DECLARE_ID(Type);
    DECLARE_ID(ID);
    DECLARE_ID(Text);
    DECLARE_ID(Help);
    DECLARE_ID(Required);
    DECLARE_ID(Value);
    DECLARE_ID(Padding);
    DECLARE_ID(Width);
}

#undef DECLARE_ID

/* TODO:
 *
 *  - custom error message
 *  - help button attachment
 *  - column mode OK
 *  - combobox
 *  - extractor component
 *  - show total progress OK
 *  - implement header stuff OK
 *  - add modal overlay (and use the help button text here!!!)
 *  - add Progress
 *  - add branching at next button (maybe make a container that shows one of its child items dependingon a global index variable)
 *  - add text input with autocomplete suggestions
 *  - broadcaster specific: add preview broadcaster with layouts
 * */
struct MultiPageDialog: public Component,
					    public PathFactory
{
    enum ColourIds
    {
        backgroundColour = HiseColourScheme::ComponentBackgroundColour,
        textColour = HiseColourScheme::ComponentTextColourId,
        signalColour = HiseColourScheme::ComponentFillTopColourId,
        numColourIds
    };
    
    struct PageBase: public Component
    {
        using CustomCheckFunction = std::function<Result(PageBase*, var)>;

        PageBase(MultiPageDialog& rootDialog, int width, const var& obj);

    	var getValueFromGlobalState(var defaultState=var());
        virtual void postInit() {};
        
        virtual ~PageBase() {};
        
        Result check(const var& obj)
        {
	        if(cf)
		        return cf(this, obj);

            return checkGlobalState(obj);
        }

        void setCustomCheckFunction(const CustomCheckFunction& cf_) { cf = cf_; }

        Identifier id;

    protected:

        MultiPageDialog& rootDialog;

        struct HelpButton: public HiseShapeButton
	    {
		    HelpButton(String help, const PathFactory& factory);
	        
	    };

        ScopedPointer<HelpButton> helpButton;

        virtual Result checkGlobalState(var globalState) = 0;

        CustomCheckFunction cf;
        int padding = 0;
        
    };
    
    struct PositionInfo
    {
        var toJSON() const;

        void fromJSON(const var& obj);

        int TopHeight = 50;
        int ButtonTab = 40;
        int ButtonMargin = 5;
        int OuterPadding = 30;
    };
    
    struct ErrorComponent: public Component
    {
        ErrorComponent(MultiPageDialog& parent_);;
        
        void paint(Graphics& g) override;
        void resized() override;

        void setError(const Result& r);
        void show(bool shouldShow);
        
        MultiPageDialog& parent;

        int height = 40;
        MarkdownRenderer parser;
        ScopedPointer<HiseShapeButton> closeButton;
        Result currentError;
        
    };
    
    struct PageInfo: public ReferenceCountedObject
    {
        PageInfo() = default;
        PageInfo(const var& obj): data(obj) {};

        using Ptr = ReferenceCountedObjectPtr<PageInfo>;
        using List = ReferenceCountedArray<PageInfo>;

        using CreateFunction = std::function<PageBase*(MultiPageDialog&, int width, const var&)>;

        operator bool() const noexcept { return (bool)pageCreator; }
        PageBase* create(MultiPageDialog& r, int currentWidth) const;;
        var& operator[](const Identifier& id) const;
        
        template <typename T> static PageInfo::Ptr createInfo()
        {
	        PageInfo::Ptr p = new PageInfo();
	        p->data = new DynamicObject();
	        p->data.getDynamicObject()->setProperty(MultiPageIds::Type, T::getStaticId().toString());
	        p->pageCreator = [](MultiPageDialog& r, int w, const var& d){ return new T(r, w, d); };
            return p;
        }

        template <typename T> PageInfo& addChild()
        {
	        childItems.add(createInfo<T>());
            return *childItems.getLast();
        }

        void setCreateFunction(const CreateFunction& f)
        {
	        pageCreator = f;
        }

        void setCustomCheckFunction(const PageBase::CustomCheckFunction& f)
        {
	        customCheck = f;
        }

    private:

        var data;
        CreateFunction pageCreator;
        List childItems;
        PageBase::CustomCheckFunction customCheck;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PageInfo);
    };

    struct Factory
    {
        Factory();

        PageInfo::Ptr create(const var& obj);

    private:

        template <typename T> void registerPage()
	    {
		    Item item;
            item.id = T::getStaticId();
            item.f = [](MultiPageDialog& r, int width, const var& data) { return new T(r, width, data); };
            items.add(std::move(item));
	    }

        struct Item
	    {
		    Identifier id;
            PageInfo::CreateFunction f;
	    };

        Array<Item> items;
    };

    MultiPageDialog(const var& obj);

    template <typename T> PageInfo& addPage()
    {
        pages.add(PageInfo::createInfo<T>());
        
        return *pages.getLast();
    }
    
    void showFirstPage();
	void setFinishCallback(const std::function<void()>& f);

    static void setGlobalState(Component& page, const Identifier& id, var newValue);
    static var getGlobalState(Component& page, const Identifier& id, const var& defaultValue);
    static std::pair<Font, Colour> getDefaultFont(Component& c);

    Path createPath(const String& url) const override;

    MarkdownLayout::StyleData getStyleData() const { return styleData; }

    void setProperty(const Identifier& id, const var& newValue)
    {
	    properties.getDynamicObject()->setProperty(id, newValue);
    }

    void setStyleData(const MarkdownLayout::StyleData& sd);
    bool navigate(bool forward);

    void paint(Graphics& g) override;
    void resized() override;

    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() {};
        virtual void drawHeader(Graphics& g, MultiPageDialog& d, Rectangle<int> area);
        virtual void drawButtonTab(Graphics& g, MultiPageDialog& d, Rectangle<int> area);
        virtual void drawModalBackground(Graphics& g, Rectangle<int> totalBounds, Rectangle<int> modalBounds)
        {
	        g.setColour(Colour(0xFF222222).withAlpha(0.9f));
            g.fillRect(totalBounds);
            
            DropShadow sh(Colours::black, 10, { 0, 0 });
            sh.drawForRectangle(g, modalBounds);

            g.setColour(Colour(0xFF333333));
            g.fillRoundedRectangle(modalBounds.toFloat(), 3.0f);
            g.setColour(Colour(0xFFAAAAAA));
            g.drawRoundedRectangle(modalBounds.toFloat(), 3.0f, 1.0f);
        }

        virtual PositionInfo getPositionInfo() const = 0;
    };
    
    struct DefaultLookAndFeel: public hise::GlobalHiseLookAndFeel,
                               public LookAndFeelMethods
    {
        void drawToggleButton(Graphics& g, ToggleButton& tb, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        PositionInfo getPositionInfo() const override;
        void layoutFilenameComponent (FilenameComponent& filenameComp,
                                      ComboBox* filenameBox, Button* browseButton) override;
        
        PositionInfo defaultPosition;
    } defaultLaf;

    struct ModalPopup: public Component
    {
        ModalPopup(MultiPageDialog& parent_):
          parent(parent_),
          modalError(parent),
    	  okButton("OK"),
    	  cancelButton("Cancel")
        {
            addChildComponent(modalError);
	        addAndMakeVisible(okButton);
            addAndMakeVisible(cancelButton);

            okButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::onOk);
            cancelButton.onClick = BIND_MEMBER_FUNCTION_0(ModalPopup::dismiss);
        };

        void onOk()
        {
	        if(contentComponent != nullptr)
	        {
		        auto r = contentComponent->check(getGlobalState(*this, {}, var()));

                if(!r.wasOk())
                {
	                modalError.setError(r);
                }
	        }
        }

        void dismiss()
        {
	        setVisible(false);
        }

        void setContent(PageInfo::Ptr newContent)
        {
	        content = newContent;
        }

        void show(PositionInfo newInfo)
        {
            info = newInfo;

            if(content != nullptr)
            {
	            addAndMakeVisible(contentComponent = content->create(parent, getWidth() - 4 * info.OuterPadding));

                contentComponent->postInit();
				resized();
				setVisible(true);
                toFront(true);
            }
        }

        void paint(Graphics& g) override
        {
	        if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
		        laf->drawModalBackground(g, getLocalBounds(), modalBounds);
        }

        void resized() override
        {
            auto b = getLocalBounds();

            int thisHeight = 2 * info.OuterPadding;

            if(contentComponent != nullptr)
                thisHeight += contentComponent->getHeight();

            if(modalError.isVisible())
                thisHeight += modalError.height;

            thisHeight += info.ButtonTab;

            b = b.withSizeKeepingCentre(getWidth() - 2 * info.OuterPadding, thisHeight);

            modalBounds = b;

            b = b.reduced(info.OuterPadding);
            
            if(modalError.isVisible())
                modalError.setBounds(b.removeFromTop(modalError.height));

            auto bottom = b.removeFromBottom(info.ButtonTab);

        	okButton.setBounds(bottom.removeFromLeft(100).reduced(info.ButtonMargin));
            cancelButton.setBounds(bottom.removeFromRight(100).reduced(info.ButtonMargin));

	        if(contentComponent != nullptr)
                contentComponent->setBounds(b);

            repaint();
        }
        
    private:

        MultiPageDialog& parent;
        PositionInfo info;

        ErrorComponent modalError;

        Rectangle<int> modalBounds;

        ScopedPointer<PageBase> contentComponent;
        PageInfo::Ptr content;

	    TextButton okButton, cancelButton;
    };

    template <typename T> PageInfo& createModalPopup()
    {
	    auto p = PageInfo::createInfo<T>();
        popup.setContent(p);
        return *p;
    }

    void showModalPopup()
    {
        PositionInfo info;

        if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
            info = laf->getPositionInfo();

        popup.show(info);
	    
    }

    

private:

    var assets;

    

    var properties;
    MarkdownLayout::StyleData styleData;
    PageInfo::List pages;
    int currentPageIndex = -1;
    TextButton cancelButton;
    TextButton nextButton;
    TextButton prevButton;
    Result currentError;
    Viewport content;
    ScopedPointer<PageBase> currentPage;
    Rectangle<int> top, bottom, center;
    std::function<void()> finishCallback;
    Factory factory;
    var globalState;

    ErrorComponent errorComponent;

    ModalPopup popup;

    ScrollbarFader sf;
};

}