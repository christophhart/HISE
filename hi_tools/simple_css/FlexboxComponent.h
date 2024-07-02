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
namespace simple_css
{
using namespace juce;

struct FlexboxContainer
{
	virtual ~FlexboxContainer() {}

	virtual void setDefaultStyleSheet(const String& code) = 0;
	virtual void setCSS(StyleSheet::Collection& css) = 0;
	virtual void addFlexItem(Component& c) = 0;
    virtual void rebuildLayout() = 0;
    
	bool fullRebuild = true;
};



/** A container component that applies flex box positioning to its children.
 */
struct FlexboxComponent: public Component,
					     public FlexboxContainer
{
    struct VisibleState
    {
        bool isVisible(bool displayValue) const
        {
			displayValue |= mustBeVisible;
            displayValue &= !mustBeHidden;
            return displayValue;
        }

        bool mustBeVisible = true;
        bool mustBeHidden = false;
		bool usePlaceHolder = false;
    };
    
	struct Helpers
	{
		static void setFallbackStyleSheet(Component& c, const String& properties);

		static void appendToElementStyle(Component& c, const String& additionalStyle);

		static String dump(Component& c);


		static void writeSelectorsToProperties(Component& c, const StringArray& selectors);
		static Selector getTypeSelectorFromComponentClass(Component* c);
		static Array<Selector> getClassSelectorFromComponentClass(Component* c);

		static void writeClassSelectors(Component& c, const Array<Selector>& classList, bool append);

		static void invalidateCache(Component& c);

		static Selector getIdSelectorFromComponentClass(Component* c);

		static String getInlineStyle(Component& c)
		{
			return c.getProperties()["inline-style"].toString().trim();
		}

		static void writeInlineStyle(Component& c, const String& inlineCode)
		{
			c.getProperties().set("inline-style", inlineCode);
			invalidateCache(c);
		}

		static void setCustomType(Component& c, const Selector& selector)
		{
			c.getProperties().set("custom-type", selector.toString());
			invalidateCache(c);
		}
	};

	struct SimpleTextDisplay: public Component
	{
		SimpleTextDisplay(ElementType s_):
		  s(s_)
		{};

		const ElementType s;
		String currentText;

		void setText(const String& text);

		void paint(Graphics& g) override;
	};

	/** Create a flexbox component with a selector (usually a ID selector or either div or body). */
	FlexboxComponent(const Selector& s);

	/** Set a CSS collection. This will apply the stylesheet to all child components. You can supply a custom
	 *  function that will create a default style sheet (or if you don't it will use the Component properties `style` value
	 *	to make a individual element style CSS. */
	void setCSS(StyleSheet::Collection& css) override;

	/** Writes a list of semicolon separated properties into the component properties that will be used as default style sheet. */
	void setDefaultStyleSheet(const String& css) override;

	/** Draws the background using the style sheet supplied with setCSS(). */
	void paint(Graphics& g) override;

	/** Applies the position using the JUCE::Flexbox class. */
	void resized() override;

	void rebuildRootLayout();

    /** Call this to ensure that the layout is changed properly. */
	void rebuildLayout() override;

    /** Adds a invisible component as child that will act as spacer. */
	void addSpacer();

	/** Adds a text element with a single format. */
	Component* addTextElement(const StringArray& selectors, const String& content);

    Selector getSelector() const { return selector; }

	void addFlexItem(Component& c) override;

    void addDynamicFlexItem(Component& c);

    void changeClass(const Selector& s, bool add);


    float getAutoWidthForHeight(float fullHeight);
    
    float getAutoHeightForWidth(float fullWidth);
    
	void setApplyMargin(bool shouldApplyMargin)
	{
		applyMargin = shouldApplyMargin;
	}

	void setParent(CSSRootComponent* p);

	/** Used by addTextElement (usually either <label> or <p>). */
	void setTextElementSelector(ElementType s)
	{
		labelSelector = s;
	}

	void setFlexChildVisibility(int childIndex, bool mustBeVisible, bool mustBeHidden)
	{
		VisibleState s;
		s.mustBeHidden = mustBeHidden;
		s.mustBeVisible = mustBeVisible;
		setFlexChildVisibility(childIndex, s);
	}

	bool isVisibleOrPlaceHolder(Component* c) const
	{
		if(visibleStates.find(c) != visibleStates.end())
		{
			if(visibleStates.at(c).usePlaceHolder)
				return true;

			return visibleStates.at(c).isVisible(c->isVisible());
		}

		return c->isVisible();
	}

    void setFlexChildVisibility(int childIndex, VisibleState newState)
    {
        auto c = getChildComponent(childIndex);
        visibleStates[c] = newState;
    }

	void setIsInvisibleWrapper(bool shouldBeInvisibleWrapper);

    juce::FlexItem createFlexItemForInvisibleWrapper(Rectangle<float> fullArea)
    {
		jassert(isInvisibleWrapper());
		auto fc = getChildComponent(0);
		return childSheets[fc]->getFlexItem(fc, fullArea);
    }

    bool isInvisibleWrapper() const { return invisibleWrapper; }

	void setHiseShapeButtonColours(HiseShapeButton& b);

	void setSelector(const Selector& s)
	{
		selector = s;

		if(s.type != SelectorType::Type)
			Helpers::writeSelectorsToProperties(*this, { s.toString() });

		rebuildLayout();
	}

private:

	bool invisibleWrapper = false;

    std::map<Component*, VisibleState> visibleStates;
    
	ElementType labelSelector = ElementType::Paragraph;

	CSSRootComponent* parentToUse = nullptr;

	std::pair<Component*, Component*> getFirstLastComponents();

    struct PositionData
	{
		Rectangle<float> area;
		juce::FlexBox flexBox;
		std::vector<std::pair<Component*, Rectangle<int>>> absolutePositions;
	};

	PositionData createPositionData();

	bool applyMargin = true;

	OwnedArray<Component> spacers;
	OwnedArray<SimpleTextDisplay> textDisplays;

	Selector selector;
	StyleSheet::Ptr ss;

	std::map<Component*, StyleSheet::Ptr> childSheets;

	float lastWrapWidth = -1.0f;
	float lastWrapHeight = -1.0f;
};

struct FlexboxViewport: public Component,
					    public FlexboxContainer
{
	FlexboxViewport(const Selector& selector);

	void setDefaultStyleSheet(const String& styleSheet) override;

	void setCSS(StyleSheet::Collection& css) override;

	void resized() override;

	void rebuildLayout() override;

	void addFlexItem(Component& c) override;

	Viewport viewport;
    ScrollbarFader sf;
	FlexboxComponent content;
	Selector s;
	StyleSheet::Ptr ss;
};

struct CSSImage: public Component
{
	CSSImage()
	{
		using namespace simple_css;
		FlexboxComponent::Helpers::setCustomType(*this, Selector(ElementType::Image));
	}

	void paint(Graphics& g) override;

	void setImage(const URL& url);
	void setImage(const juce::Image& newImage);

	juce::Image currentImage;

	struct LoadThread: public Thread,
					   public AsyncUpdater
    {
	    LoadThread(CSSImage& parent_, const URL& url);

        void handleAsyncUpdate() override;

	    void run() override;

	    CSSImage& parent;
        juce::Image loadedImage;
        URL imageURL;
    };

	struct Cache
	{
		void setImage(const URL& url, const Image& img);

		Image getImage(const URL& url) const;

	private:

		Array<std::pair<URL, Image>> imageCache;
	};

	SharedResourcePointer<Cache> imageCache;
    ScopedPointer<LoadThread> currentLoadThread;
};

/** A combined component with three flex boxes for header, footer & content. */
struct HeaderContentFooter: public Component,
						    public CSSRootComponent
{
	struct InspectorData
	{
		Component::SafePointer<Component> c;
		Rectangle<float> first;
		String second;
	};

	struct CSSDebugger: public Component,
                    public Timer,
                    public PathFactory
	{
	    CSSDebugger(HeaderContentFooter& componentToDebug):
	      root(&componentToDebug),
	      codeDoc(doc),
	      editor(codeDoc),
	      powerButton("bypass", nullptr, *this)
	    {
	        doc.setDisableUndo(true);
	        setName("CSS Inspector");
	        addAndMakeVisible(editor);
	        
	        editor.tokenCollection = new mcl::TokenCollection("CSS");
	        editor.tokenCollection->setUseBackgroundThread(false);
	        editor.setLanguageManager(new simple_css::LanguageManager(codeDoc));
	        editor.setFont(GLOBAL_MONOSPACE_FONT().withHeight(12.0f));
	        setSize(450, 800);
	        setOpaque(true);
	        startTimer(1000);

			GlobalHiseLookAndFeel::setDefaultColours(hierarchy);
	        hierarchy.setLookAndFeel(&laf);
	        addAndMakeVisible(hierarchy);
	        addAndMakeVisible(powerButton);
	        powerButton.setToggleModeWithColourChange(true);
	        powerButton.setToggleStateAndUpdateIcon(true);
	        hierarchy.setTextWhenNothingSelected("Select parent component");
	        addAndMakeVisible(powerButton);

	        hierarchy.onChange = [&]()
	        {
		        auto pd = parentData[hierarchy.getSelectedItemIndex()];
	            updateWithInspectorData(pd);
	        };

	        powerButton.onClick = [this]()
	        {
	            if(powerButton.getToggleState())
	                this->startTimer(1000);
	            else
	                this->stopTimer();
	            
	            clear();
	        };
	    }
	    
	    void clear()
	    {
	        if(root.getComponent() != nullptr)
	            root->setCurrentInspectorData({});
	    }
	    
	    HiseShapeButton powerButton;
	    
	    Path createPath(const String& url) const override
	    {
	        Path p;
	        LOAD_EPATH_IF_URL("bypass", HiBinaryData::ProcessorEditorHeaderIcons::bypassShape);
	        return p;
	    }
	    
	    ~CSSDebugger()
	    {
	        clear();
	    }
	    
	    void paint(Graphics& g) override
	    {
	        g.fillAll(Colour(0xFF222222));
	    }

	    simple_css::HeaderContentFooter::InspectorData createInspectorData(Component* c)
	    {
		    auto b = root.getComponent()->getLocalArea(c, c->getLocalBounds()).toFloat();
	        auto data = simple_css::FlexboxComponent::Helpers::dump(*c);

	        simple_css::HeaderContentFooter::InspectorData id;
	        id.first = b;
	        id.second = data;
	        id.c = c;

	        return id;
	    }

	    void timerCallback() override
	    {
	        if(root.getComponent() == nullptr)
	            return;
	        
	        auto* target = Desktop::getInstance().getMainMouseSource().getComponentUnderMouse();

	        bool change = false;

	        if(target != nullptr && target->findParentComponentOfClass<simple_css::CSSRootComponent>() == root.getComponent())
	        {
	            currentTarget = target;
	            change = true;
	        }
	        
	        if(currentTarget.getComponent() != nullptr && change)
	        {
	            auto id = createInspectorData(currentTarget.getComponent());
	            auto tc = id.c.getComponent();

	            StringArray items;

	            parentData.clear();

	            while(tc != nullptr)
	            {
	                if(dynamic_cast<CSSRootComponent*>(tc) != nullptr)
	                    break;

	                parentData.add(createInspectorData(tc));
		            tc = tc->getParentComponent();
	            }

	            hierarchy.clear(dontSendNotification);

	            int idx = 1;
	            for(const auto& pd: parentData)
	                hierarchy.addItem(pd.second, idx++);

	            hierarchy.setText("", dontSendNotification);

	            updateWithInspectorData(id);
	        }
	    }

	    Array<simple_css::HeaderContentFooter::InspectorData> parentData;

	    void updateWithInspectorData(const simple_css::HeaderContentFooter::InspectorData& id)
	    {
		    root->setCurrentInspectorData(id);
	        auto s = root->css.getDebugLogForComponent(id.c.getComponent());
	        
	        if(doc.getAllContent() != s)
	            doc.replaceAllContent(s);
	    }
	    
	    Component::SafePointer<Component> currentTarget = nullptr;
	    
	    void resized() override
	    {
	        auto b = getLocalBounds();
	        auto topArea = b.removeFromTop(24);

	        powerButton.setBounds(topArea.removeFromLeft(topArea.getHeight()).reduced(2));
	        hierarchy.setBounds(b.removeFromBottom(32));
	        editor.setBounds(b);
	    }

	    juce::CodeDocument doc;
	    mcl::TextDocument codeDoc;
	    mcl::TextEditor editor;

	    ComboBox hierarchy;

	    hise::GlobalHiseLookAndFeel laf;

	    Component::SafePointer<simple_css::HeaderContentFooter> root;
	};

	HeaderContentFooter(bool useViewportContent);

	void setFixStyleSheet(StyleSheet::Collection& newCss);


	void showEditor();

	void setStylesheetCode(const String& code);
	void update(StyleSheet::Collection& newCss);

	virtual void styleSheetCollectionChanged() {};
	void resized() override;

	ScopedPointer<StyleSheetLookAndFeel> currentLaf;

	void setDefaultCSSProperties(DynamicObject::Ptr newDefaultProperties);

	void paintOverChildren(Graphics& g) override;

	

    void setCurrentInspectorData(const InspectorData& newData)
    {
        inspectorData = newData;
        repaint();
    }
    
    InspectorData inspectorData;
    
	FlexboxComponent body;
	FlexboxComponent header;
	ScopedPointer<FlexboxContainer> content;
	FlexboxComponent footer;

private:

	bool useFixStyleSheet = false;
	DynamicObject::Ptr defaultProperties;

	SharedResourcePointer<CSSImage::Cache> imageCache;
};



} // namespace simple_css
} // namespace hise
