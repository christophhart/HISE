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
using namespace juce;

namespace factory
{
	struct Container;
}

struct CodeGenerator
{
    struct Helpers
    {
        
        
    };

	CodeGenerator(const var& totalData, int numTabs_=0):
      data(totalData),
      numTabs(numTabs_)
	{
		
	}

    String toString() const;

private:

    String generateRandomId(const String& prefix) const;
    String getNewLine() const;

    static String arrayToCommaString(const var& value);

    String createAddChild(const String& parentId, const var& childData, const String& itemType="Page", bool attachCustomFunction=false) const;

    mutable StringArray existingVariables;

    mutable int idCounter = 0;
    const int numTabs;
    var data;
};



struct EditorOverlay: public Component,
				      public SettableTooltipClient
{
    struct Updater: public ComponentMovementWatcher
    {
	    Updater(EditorOverlay& parent_, Component* attachedComponent);

        void componentMovedOrResized (bool wasMoved, bool wasResized) override;

	    void componentPeerChanged() override {};

	    void componentVisibilityChanged() override
	    {
		    parent.setVisible(getComponent()->isVisible());
	    }

        EditorOverlay& parent;
    };

    ScopedPointer<Updater> watcher;

    void setAttachToComponent(Component* c)
    {
	    watcher = new Updater(*this, c);
    }

    EditorOverlay(Dialog& parent);

    void resized() override;

    void mouseUp(const MouseEvent& e) override;

    void paint(Graphics& g) override;

    static void onEditChange(EditorOverlay& c, bool isOn)
    {
	    c.setVisible(isOn);
    }

    void setOnClick(const std::function<void()>& f)
    {
	    cb = f;
    }

    std::function<Rectangle<int>(Component*)> localBoundFunction;

    std::function<void()> cb;

    Path outline;
    Rectangle<float> bounds;
	
    JUCE_DECLARE_WEAK_REFERENCEABLE(EditorOverlay);
};

struct ErrorComponent: public Component
{
    ErrorComponent(Dialog& parent_);;
    
    void paint(Graphics& g) override;
    void resized() override;

    void setError(const Result& r);
    void show(bool shouldShow);
    
    Dialog& parent;

    int height = 40;
    MarkdownRenderer parser;
    ScopedPointer<HiseShapeButton> closeButton;
    Result currentError;
    
};

/* TODO:
 *
 * - Remove the entire builder system and replace it with an inplace editor using the pen icon to edit the properties in a popup
 * - fix array of pages not being cleared when changing page amount OK
 * - fix builder
 * - fix actions (show in edit mode) OK
 * - add label to filechooser OK
 * - add width editing to column OK
 * - add categories to add combobox (containers, UI elements, actions) OK
 * - add preprocessor for edit mode to ensure it'll never occur in production
 * - add code generator for C++ (move base-cppgen from hi_snex to hi_tools) OK
 * */
class Dialog: public Component,
			   public PathFactory
{
public:
    
    enum ColourIds
    {
        backgroundColour = HiseColourScheme::ComponentBackgroundColour,
        textColour = HiseColourScheme::ComponentTextColourId,
        signalColour = HiseColourScheme::ComponentFillTopColourId,
        numColourIds
    };
    
    struct PageInfo;
    
    struct PageBase: public Component
    {
        using CustomCheckFunction = std::function<Result(PageBase*, var)>;

        PageBase(Dialog& rootDialog, int width, const var& obj);
        virtual ~PageBase() {};

        virtual void postInit() {};
        virtual DefaultProperties getDefaultProperties() const;
        virtual void setStateObject(const var& newStateObject);
        virtual void clearInitValue()
        {
	        initValue = var();
        }

        virtual void createEditor(PageInfo& infoList) {}

        var getPropertyFromInfoObject(const Identifier& id) const
        {
	        return infoObject.getProperty(id, var(""));
        }

    protected:

        virtual Result checkGlobalState(var globalState) = 0;
        void init();

        ScopedPointer<EditorOverlay> overlay;

	    virtual void editModeChanged(bool isEditMode);

        

    public:

        template <typename PageSubType, typename StopType=juce::Toolbar> PageSubType* getRootPage()
        {
            auto p = this;
            
            while(auto otherP = p->findParentComponentOfClass<PageSubType>())
            {
                if(dynamic_cast<StopType*>(otherP) != nullptr)
                    return dynamic_cast<PageSubType*>(p);
                
                p = otherP;
            }
            
            return dynamic_cast<PageSubType*>(p);
        }
        
        void writeState(const var& newValue) const;
        var getValueFromGlobalState(var defaultState=var());
        Result check(const var& obj);
        void clearCustomFunction();
        void setCustomCheckFunction(const CustomCheckFunction& cf_);
        bool isError() const;
        Dialog& getParentDialog() { return rootDialog; }

        Identifier getId() const { return id; }

    protected:

        bool editMode = false;
        Identifier id;
        Dialog& rootDialog;
        var initValue;

        

        

        struct HelpButton: public HiseShapeButton
	    {
		    HelpButton(String help, const PathFactory& factory);
	    };

        ScopedPointer<HelpButton> helpButton;
        CustomCheckFunction cf;
        
        var stateObject;
        var infoObject;

        void setModalHelp(const String& text);

    private:

        struct ModalHelp: public Component
        {
            ModalHelp(const String& text, PageBase& parent):
              r(text),
              closeButton("close", nullptr, parent.rootDialog)
            {
                r.setStyleData(parent.rootDialog.styleData);
                closeButton.onClick = [&parent]()
                {
	                parent.setModalHelp("");
                };

	            r.parse();
                addAndMakeVisible(closeButton);
            };

            void paint(Graphics& g) override
            {
	            g.fillAll(Colour(0xFF222222));
                auto area = getLocalBounds();
                area.removeFromLeft(10);
                area.removeFromRight(area.getHeight());
                
                r.draw(g, area.toFloat());
            }

            void resized() override
            {
                auto b = getLocalBounds().reduced(5);

                auto cb = b.removeFromRight(b.getHeight());

                closeButton.setBounds(cb.withSizeKeepingCentre(16, 16));
	            r.getHeightForWidth(getWidth() - getHeight() - 20);
            }

            MarkdownRenderer r;
	        HiseShapeButton closeButton;
        };

        ScopedPointer<ModalHelp> modalHelp;

        static void onEditModeChange(PageBase& c, bool isOn)
	    {
		    c.editModeChanged(isOn);
	    }

        JUCE_DECLARE_WEAK_REFERENCEABLE(PageBase);
    };
    
    struct PositionInfo
    {
        enum LabelPositioning
	    {
	        Default,
		    Left,
	        Above,
	        None,
            numLabelPositionings
	    };

        static StringArray getLabelPositionNames()
        {
	        return { "Default", "Left", "Above", "None" };
        }

        var toJSON() const;

        void fromJSON(const var& obj);

        Result checkSanity() const
        {
	        if(ButtonTab - 2 * ButtonMargin < 5)
                return Result::fail("ButtonTab / ButtonMargin invalid");

            if(OuterPadding > 100)
                return Result::fail("OuterPadding too high");
            
            return Result::ok();
        }

        void setDefaultPosition(LabelPositioning p)
        {
	        if(LabelPosition == LabelPositioning::Default)
                LabelPosition = p;
        }

        int getWidthForLabel(int totalWidth) const
        {
	        if(LabelWidth < 0.0)
                return jmax(10, roundToInt(totalWidth * -1.0 * LabelWidth));
            else
                return jmax(10, roundToInt(LabelWidth));
        }

        int getHeightForComponent(int heightWithoutLabel)
        {
	        if(LabelPosition == (int)LabelPositioning::Above)
                heightWithoutLabel += LabelHeight;

            return heightWithoutLabel;
        }

        int TopHeight = 56;
        int ButtonTab = 40;
        int ButtonMargin = 5;
        int OuterPadding = 50;
        double LabelWidth = -0.2;
        int LabelHeight = 32;
        int LabelPosition = LabelPositioning::Default;
    };
    
    struct PageInfo: public ReferenceCountedObject
    {
        using Ptr = ReferenceCountedObjectPtr<PageInfo>;
        using List = ReferenceCountedArray<PageInfo>;
        using CreateFunction = std::function<PageBase*(Dialog&, int width, const var&)>; 

        PageInfo() = default;
        PageInfo(const var& obj): data(obj) {};

        operator bool() const noexcept { return (bool)pageCreator; }
        PageBase* create(Dialog& r, int currentWidth) const;;
        var& operator[](const Identifier& id) const;
        var getData() const { return data; }
        
        template <typename T> static PageInfo::Ptr createInfo()
        {
	        PageInfo::Ptr p = new PageInfo();
	        p->data = new DynamicObject();
	        p->data.getDynamicObject()->setProperty(mpid::Type, T::getStaticId().toString());
	        p->pageCreator = [](Dialog& r, int w, const var& d){ return new T(r, w, d); };
            return p;
        }

        PageInfo& getChild(const Identifier& id)
        {
	        for(auto& c: childItems)
	        {
		        if(c->data[mpid::ID].toString() == id.toString())
			        return *c;
	        }

            jassertfalse;
            return *this;
        }

        template <typename T> PageInfo& addChild(DefaultProperties&& values={})
        {
	        childItems.add(createInfo<T>());
            
            for(auto& v: values)
                childItems.getLast()->data.getDynamicObject()->setProperty(v.first, v.second);
            
            return *childItems.getLast();
        }

        void setCreateFunction(const CreateFunction& f);
        void setCustomCheckFunction(const PageBase::CustomCheckFunction& f);
        void setStateObject(const var& newStateObject);
        
        bool useGlobalStateObject = false;
        var stateObject;
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

        StringArray getPopupMenuList() const
        {
            StringArray sa;

            for(const auto& i: items)
            {
                String entry;
	            entry << i.category << "::" << i.id;
                sa.add(entry);
            }

            return sa;
        }

        StringArray getIdList() const;
        
    private:

        template <typename T> void registerPage()
	    {
		    Item item;
            item.id = T::getStaticId();
            item.category = T::getCategoryId();
            item.f = [](Dialog& r, int width, const var& data) { return new T(r, width, data); };

            item.isContainer = std::is_base_of<factory::Container, T>();
            
            items.add(std::move(item));
	    }

        struct Item
	    {
            bool isContainer;
		    Identifier id;
            Identifier category;
            PageInfo::CreateFunction f;
	    };

        Array<Item> items;
    };

    Dialog(const var& obj, State& rt);
    ~Dialog();

    template <typename T> PageInfo& addPage(DefaultProperties&& values={}, int index = -1)
    {
        PageInfo::Ptr p;

        if(index == -1)
			p = pages.add(PageInfo::createInfo<T>());
        else
            p = pages.insert(index, PageInfo::createInfo<T>());
        
        if(values.size())
        {
            for (const auto& v: values)
                (*p)[v.first] = v.second;
        }
        else
        {
            for (const auto& v: T::getStaticDefaultProperties())
                (*p)[v.first] = v.second;
        }
        
        return *p;
    }
    
    Result getCurrentResult();
    void showFirstPage();
	void setFinishCallback(const std::function<void()>& f);
    static var getOrCreateChild(const var& obj, const Identifier& id);
    static var getGlobalState(Component& page, const Identifier& id, const var& defaultValue);
    static std::pair<Font, Colour> getDefaultFont(Component& c);
    State::Job::Ptr getJob(const var& obj) const;
    Path createPath(const String& url) const override;
    MarkdownLayout::StyleData getStyleData() const { return styleData; }
    void setProperty(const Identifier& id, const var& newValue);
    void setStyleData(const MarkdownLayout::StyleData& sd);
    bool navigate(bool forward);
    bool getCurrentNavigationDirection() { return currentNavigation; }
    void paint(Graphics& g) override;
    void resized() override;
    State& getState() { return *runThread; }

    void mouseDown(const MouseEvent& e) override;

    void setEditMode(bool isEditMode);

    bool isEditModeEnabled() const noexcept;

    LambdaBroadcaster<bool>& getEditModeBroadcaster() { return editModeBroadcaster; }

    struct LookAndFeelMethods
    {
        virtual ~LookAndFeelMethods() {};
        virtual void drawMultiPageHeader(Graphics& g, Dialog& d, Rectangle<int> area);
        virtual void drawMultiPageButtonTab(Graphics& g, Dialog& d, Rectangle<int> area);
        virtual void drawMultiPageModalBackground(Graphics& g, Rectangle<int> totalBounds, Rectangle<int> modalBounds);
        virtual void drawMultiPageFoldHeader(Graphics& g, Component& c, Rectangle<float> area, const String& title, bool folded);
        virtual void drawMultiPageBackground(Graphics& g, Dialog& tb, Rectangle<int> errorBounds);
        virtual PositionInfo getMultiPagePositionInfo(const var& pageData) const = 0;
    };
    
    struct DefaultLookAndFeel: public hise::GlobalHiseLookAndFeel,
                               public LookAndFeelMethods
    {
        void drawToggleButton(Graphics& g, ToggleButton& tb, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
        PositionInfo getMultiPagePositionInfo(const var& pageData) const override;
        void layoutFilenameComponent (FilenameComponent& filenameComp, ComboBox* filenameBox, Button* browseButton) override;
        bool isProgressBarOpaque(ProgressBar&) override { return false; }
        void drawProgressBar (Graphics &g, ProgressBar &pb, int width, int height, double progress, const String &textToShow) override;

        PositionInfo defaultPosition;
    } defaultLaf;

    struct ModalPopup: public Component
    {
        ModalPopup(Dialog& parent_);;

        void onOk();
        void dismiss();
        void setContent(PageInfo::Ptr newContent);
        void show(PositionInfo newInfo, bool addButtons);

        void mouseDown(const MouseEvent& e) override;
        void paint(Graphics& g) override;
        void resized() override;

        Dialog& parent;
        PositionInfo info;
        ErrorComponent modalError;
        Rectangle<int> modalBounds;
        ScopedPointer<PageBase> contentComponent;
        PageInfo::Ptr content;
	    TextButton okButton, cancelButton;
    };

    template <typename T> PageInfo& createModalPopup(DefaultProperties&& values={})
    {
	    auto p = PageInfo::createInfo<T>();
        
        if(values.size())
        {
            for (const auto& v: values)
                (*p)[v.first] = v.second;
        }
        else
        {
            for (const auto& v: T::getStaticDefaultProperties())
                (*p)[v.first] = v.second;
        }

        popup.setContent(p);
        return *p;
    }

    void showModalPopup(bool addButtons);
    Result checkCurrentPage();
    void setCurrentErrorPage(PageBase* b);

    var exportAsJSON() const;

    PositionInfo getPositionInfo(const var& pageData) const
    {
	    if(auto laf = dynamic_cast<LookAndFeelMethods*>(&getLookAndFeel()))
            return laf->getMultiPagePositionInfo(pageData);
    }

    bool createJSON = false;

private:
    void showMainPropertyEditor();

    void showNumPageEditor();

    TooltipWindow tooltipWindow;

    LambdaBroadcaster<bool> editModeBroadcaster;

    bool editMode = true;
    bool editingAllowed = true;

    friend class State;

    Array<var> pageListInfo;

    var assets;
    bool currentNavigation = true;
    var properties;
    MarkdownLayout::StyleData styleData;
    PageInfo::List pages;
    TextButton cancelButton;
    TextButton nextButton;
    TextButton prevButton;
    State* runThread;
    Viewport content;
    ScopedPointer<PageBase> currentPage;
    Rectangle<int> top, bottom, center;
    std::function<void()> finishCallback;
    Factory factory;
    WeakReference<PageBase> currentErrorElement;
    ErrorComponent errorComponent;
    ModalPopup popup;
    ScrollbarFader sf;

    EditorOverlay mainPropertyOverlay;
    EditorOverlay numPageOverlay;

    JUCE_DECLARE_WEAK_REFERENCEABLE(Dialog);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dialog);
};

}
}