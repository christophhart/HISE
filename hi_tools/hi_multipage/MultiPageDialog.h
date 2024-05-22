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

struct ComponentWithSideTab
{
    ComponentWithSideTab();

    virtual ~ComponentWithSideTab() {};
	virtual bool setSideTab(State* dialogState, Dialog* newDialog) = 0;

    virtual State* getMainState() { return nullptr; }

    virtual void addCodeEditor(const var& infoObject, const Identifier& codeId) = 0;

    virtual void refreshDialog() = 0;

    simple_css::StyleSheet::Collection propertyStyleSheet;
};

struct DefaultCSSFactory
{
	enum class Template
	{
        None,
		PropertyEditor,
        RawHTML,
        Dark,
        Bright,
        ModalPopup,
        numTemplates
	};

    static String getTemplate(Template t);
    static String getTemplateList() { return "None\nProperty Editor\nRawHTML\nDark\nBright\nModalPopup"; }

    static simple_css::StyleSheet::Collection getTemplateCollection(Template t, const String& additionalStyle={});
};



/* TODO:
 *
 * - remove nested data when using IDs in containers (too messy)...
 * - fix undoability of page removal
 * - add fixed size to position info OK
 * */
class Dialog: public simple_css::HeaderContentFooter,
			  public PathFactory,
			  public ScrollBar::Listener
{
public:

    struct PageInfo;

    struct PositionInfo
    {
        var toJSON() const;
        void fromJSON(const var& obj);
        int getWidthForLabel(int totalWidth) const;
        int getHeightForComponent(int heightWithoutLabel);

        Rectangle<int> getBounds(Rectangle<int> fullBounds) const;

        Point<int> fixedSize = { 800, 600 };
        
        String styleSheet = "Dark";
        String additionalStyle;
        bool useViewport = true;
    } positionInfo;

    struct PageBase: public simple_css::FlexboxComponent
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

        virtual bool hasOnSubmitEvent() const { return false; }

        void updateStyleSheetInfo(bool forceUpdate=false);

        void forwardInlineStyleToChildren();

        

#if HISE_MULTIPAGE_INCLUDE_EDIT
        virtual void createEditor(PageInfo& infoList) {}
#endif

        bool matches(const var& otherInfo) const
        {
	        return infoObject.getDynamicObject() == otherInfo.getDynamicObject();
        }

        var getInfoObject() const { return infoObject; }

        var getPropertyFromInfoObject(const Identifier& id) const;
        void deleteFromParent();
        void duplicateInParent();
        bool isEditModeAndNotInPopup() const;
        bool showDeletePopup(bool isRightClick);

        /** Returns the string from the info object for the given property. If it contains a wildcard with "$", it will resolve it using the state object. */
        String evaluate(const Identifier& id) const;

        void callOnValueChange(const String& eventType, DynamicObject::Ptr thisObject=nullptr);

        void addEventListener(const String& eventType, const var& functionObject)
        {
            eventListeners.add({ eventType, functionObject });
        }

        void removeEventListener(const String& eventType, const var& functionObject)
        {
	        eventListeners.removeAllInstancesOf({ eventType, functionObject });
        }

    protected:

        Array<std::pair<String, var>> eventListeners;

        Asset::Ptr getAsset(const Identifier& id) const;

        virtual Result checkGlobalState(var globalState) = 0;
        void init();

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

        String loadValueOrAssetAsText();

        StringArray getItemsAsStringArray() const;

        Result check(const var& obj);
        void clearCustomFunction();
        void setCustomCheckFunction(const CustomCheckFunction& cf_);
        bool isError() const;
        Dialog& getParentDialog() { return rootDialog; }

        Identifier getId() const { return id; }

        void setModalHelp(const String& text);

    protected:

        void callAdditionalStateCallback()
        {
            if(rootDialog.additionalChangeCallback)
            {
	            if(cf)
	                cf(this, getValueFromGlobalState());

	            rootDialog.callAdditionalChangeCallback();
            }
        }

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
        
    private:

        JavascriptEngine* engine = nullptr;

        static void onEditModeChange(PageBase& c, bool isOn);

        struct ModalHelp;

        ScopedPointer<Component> modalHelp;

        int styleHash = 0;
        int classHash = 0;

        JUCE_DECLARE_WEAK_REFERENCEABLE(PageBase);
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

        PageInfo* getChildRecursive(PageInfo* p, const Identifier& id);

        PageInfo& getChild(const Identifier& id);

        template <typename T> PageInfo& addChild(DefaultProperties&& values={})
        {
            static_assert(std::is_base_of<PageBase, T>(), "must be base of PageBase");
	        childItems.add(createInfo<T>());
            
            for(auto& v: values)
                childItems.getLast()->data.getDynamicObject()->setProperty(v.first, v.second);
            
            return *childItems.getLast();
        }

        void setLambdaAction(State& s, const State::HardcodedLambda& f)
        {
	        (*this)[mpid::Function] = var(LambdaAction(s, f));
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

    Dialog(const var& obj, State& rt, bool addEmptyPage=true);
    ~Dialog();

    int getNumPages() const { return pages.size(); }
    bool removeCurrentPage();
    void addListPageWithJSON();

    void rebuildPagesFromJSON();

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

    String getStringFromModalInput(const String& message, const String& prefilledValue);

    bool refreshCurrentPage();

    bool getCurrentNavigationDirection() { return currentNavigation; }
    void paint(Graphics& g) override;
    void resized() override;
    State& getState() { return *runThread; }

#if HISE_MULTIPAGE_INCLUDE_EDIT
    void paintOverChildren(Graphics& g) override
    {
        HeaderContentFooter::paintOverChildren(g);
        
	    if(editMode)
		{
			GlobalHiseLookAndFeel::draw1PixelGrid(g, this, getLocalBounds(), Colour(0x44999999));
	        
	        g.setColour(Colours::white.withAlpha(0.1f));
	        g.setFont(GLOBAL_MONOSPACE_FONT());
		}

        for(auto& s: mouseSelector.selection.getItemArray())
        {
            if(auto c = findPageBaseForInfoObject(s))
            {
	            auto b = getLocalArea(c, c->getLocalBounds()).expanded(2.0f);
				g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.5f));
				g.drawRoundedRectangle(b.toFloat(), 3.0f, 1.0f);
            }
        }
    }
#endif

    struct TabTraverser;

    std::unique_ptr<ComponentTraverser> createKeyboardFocusTraverser() override;

    void setEditMode(bool isEditMode);

    struct DialogDataProvider: public simple_css::StyleSheet::Collection::DataProvider
    {
	    DialogDataProvider(Dialog& d):
          rootDialog(d)
	    {
		    
	    }

        Font loadFont(const String& fontName, const String& url) override
	    {
            return rootDialog.getState().loadFont(url);
	    }

    	String importStyleSheet(const String& url) override
	    {
		    return rootDialog.getState().loadText(url, true);
	    }

        Image loadImage(const String& imageURL) override
	    {
		    return rootDialog.getState().loadImage(imageURL);
	    }

        Dialog& rootDialog;
    };

    simple_css::StyleSheet::Collection::DataProvider* createDataProvider() override
    {
        return new DialogDataProvider(*this);
    }

    bool isEditModeEnabled() const noexcept;

    bool isEditModeAllowed() const { return editingAllowed; }

    String getExistingKeysAsItemString() const;

    void setEnableEditMode(bool shouldBeEnabled)
    {
	    editingAllowed = shouldBeEnabled;
    }

#if HISE_MULTIPAGE_INCLUDE_EDIT
    void containerPopup(const var& infoObject);

    bool nonContainerPopup(const var& infoObject);

    bool showEditor(const Array<var>& infoObjects);
#endif

    void gotoPage(int newIndex);

    void logMessage(MessageType messageType, const String& message)
    {
        auto isMessageThread = MessageManager::getInstanceWithoutCreating()->isThisTheMessageThread();
        auto n = isMessageThread ? sendNotificationSync : sendNotificationAsync;
	    getEventLogger().sendMessage(n, messageType, message);
    }

    LambdaBroadcaster<bool>& getEditModeBroadcaster() { return editModeBroadcaster; }

    LambdaBroadcaster<MessageType, String>& getEventLogger() { return getState().eventLogger; }

    void callAdditionalChangeCallback()
    {
        if(additionalChangeCallback)
            additionalChangeCallback();
    }

    

    struct ModalPopup: public simple_css::FlexboxComponent
    {
        ModalPopup(Dialog& parent_, PageInfo::Ptr info, bool addButtons);

        void init();

        void onOk();
        void dismiss();

        bool keyPressed(const KeyPress& k) override;

        void mouseDown(const MouseEvent& e) override;
        
        //void resized() override;

        Dialog& parent;
        ScopedPointer<PageBase> contentComponent;
        PageInfo::Ptr info;
	    TextButton okButton, cancelButton;
        simple_css::FlexboxComponent content;
        simple_css::FlexboxComponent bottom;
        simple_css::FlexboxViewport contentViewport;
    };

    void createEditorInSideTab(const var& obj, PageBase* pb, const std::function<void(PageInfo&)>& r);

    bool isCurrentlyEdited(const var& obj) const
    {
	    if(currentlyEditedPage != nullptr)
            return currentlyEditedPage->matches(obj);

        return false;
    }

    template <typename T> PageInfo::Ptr createModalPopup(DefaultProperties&& values={})
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
        
        return p;
    }

    static String joinVarArrayToNewLineString(const var& v);

    static var parseCommaList(const String& text);

    void showModalPopup(bool addButtons, PageInfo::Ptr p);
    Result checkCurrentPage();
    void setCurrentErrorPage(PageBase* b);
    bool keyPressed(const KeyPress& k) override;
    var exportAsJSON() const;
    void scrollBarMoved (ScrollBar*, double) override { repaint(); }

    PageBase* findPageBaseForID(const String& id);
    PageBase* findPageBaseForInfoObject(const var& obj);

    PositionInfo getPositionInfo(const var& pageData) const;

    bool createJSON = false;

    UndoManager& getUndoManager() { return um; }

    var getGlobalProperty(const Identifier& id) const
    {
        jassert(properties.hasProperty(id));
	    return properties[id];
    }

    var getPageListVar() { return pageListArrayAsVar; }

    LambdaBroadcaster<int> refreshBroadcaster;

    void showMainPropertyEditor();

    bool useHelpBubble = false;

#if HISE_MULTIPAGE_INCLUDE_EDIT

    LambdaBroadcaster<Array<var>> selectionUpdater;

    struct MouseSelector: public MouseListener,
						  public LassoSource<var>,
						  public ChangeListener	
    {
        MouseSelector(Dialog& parent);

        ~MouseSelector() override;

        void changeListenerCallback(ChangeBroadcaster* source) override;
        void findLassoItemsInArea (Array<var>& itemsFound, const Rectangle<int>& area) override;

        SelectedItemSet<var>& getLassoSelection() override
        {
	        return selection;
        }

        void mouseDrag(const MouseEvent& e) override;
        void mouseUp(const MouseEvent& e) override;
        void mouseDown(const MouseEvent& event) override;

        Dialog& parent;
        LassoComponent<var> lasso;
        uint64 lastHash = 0;
        SelectedItemSet<var> selection;
    } mouseSelector;
#endif

    

    void loadStyleFromPositionInfo();

    bool& getSkipRebuildFlag() { return skipRebuild; }

private:

    std::function<void()> additionalChangeCallback;

    bool skipRebuild = false;

    Image backgroundImage;

    WeakReference<PageBase> currentlyEditedPage;
    

    UndoManager um;
    
    LambdaBroadcaster<bool> editModeBroadcaster;

    bool editMode = false;
    bool editingAllowed = true;

    friend class State;

    Array<var>* pageListInfo;

    var pageListArrayAsVar;

    var assets;
    bool currentNavigation = true;
    var properties;
    MarkdownLayout::StyleData styleData;
    PageInfo::List pages;
    TextButton cancelButton;
    TextButton nextButton;
    TextButton prevButton;
    State* runThread;
    
    ScopedPointer<PageBase> currentPage;
    Rectangle<int> top, bottom, center;
    std::function<void()> finishCallback;
    
    WeakReference<PageBase> currentErrorElement;
    //ErrorComponent errorComponent;
    ScopedPointer<ModalPopup> popup;
    ScrollbarFader sf;

    double progressValue = 0.0;
    ProgressBar totalProgress;
    

    JUCE_DECLARE_WEAK_REFERENCEABLE(Dialog);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Dialog);
};

struct EventConsole: public Component
{
    struct ConsoleEditor: public CodeEditorComponent
    {
        ConsoleEditor(CodeDocument& doc):
          CodeEditorComponent(doc, nullptr)
        {
	        setFont(GLOBAL_MONOSPACE_FONT());
	        setLineNumbersShown(false);
	        setColour(CodeEditorComponent::ColourIds::backgroundColourId, Colour(0xFF161616));
	        setColour(CodeEditorComponent::ColourIds::defaultTextColourId, Colour(0xFF999999));
            setColour(CodeEditorComponent::ColourIds::highlightColourId, Colours::white.withAlpha(0.3f));
	        setReadOnly(true);
	        sf.addScrollBarToAnimate(getScrollbar(false));
        }

	    void addPopupMenuItems (PopupMenu& m, const MouseEvent* e) override
	    {
		    m.addItem(90001, "Clear console");
            CodeEditorComponent::addPopupMenuItems(m, e);
	    }

    	void performPopupMenuAction (int menuItemID) override
	    {
		    CodeEditorComponent::performPopupMenuAction(menuItemID);

            if(menuItemID == 90001)
            {
	            getDocument().replaceAllContent("");
            }
	    }

        ScrollbarFader sf;
    };

	EventConsole(State& d):
	  editor(doc),
      state(d)
    {
        setName("Console");
        doc.setDisableUndo(true);
        addAndMakeVisible(editor);
        editor.getScrollbar(true);
        state.eventLogger.addListener(*this, onMessage, true);
    }

    ~EventConsole()
    {
	    state.eventLogger.removeListener(*this);
    }

    MessageType messageFilter = MessageType::AllMessages;

    static void onMessage(EventConsole& l, MessageType messageType, const String& message)
    {
        if(messageType == MessageType::Clear)
        {
	        l.doc.replaceAllContent("");
            return;
        }

        if((uint32)messageType | (uint32)l.messageFilter)
        {
	        l.doc.insertText(CodeDocument::Position(l.doc, l.doc.getNumCharacters()), "\n" + message);
            l.editor.scrollToKeepCaretOnScreen();
        }
    }

    void resized() override
    {
        editor.setBounds(getLocalBounds());
    }

    State& state;
    CodeDocument doc;
    ConsoleEditor editor;

    JUCE_DECLARE_WEAK_REFERENCEABLE(EventConsole);
};

}
}
