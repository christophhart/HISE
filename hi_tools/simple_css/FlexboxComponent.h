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

struct ComponentWithAutoTextSize
{
	virtual ~ComponentWithAutoTextSize() = default;
	virtual String getTextToAutofit() const = 0;
};

struct FlexboxContainer
{
	virtual ~FlexboxContainer() {}

	virtual void setDefaultStyleSheet(const String& code) = 0;
	virtual void setCSS(StyleSheet::Collection& css) = 0;
	virtual void addFlexItem(Component& c) = 0;
	virtual void removeFlexItem(Component& c) = 0;
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

		static String toCSSCode(Component& c, const Array<std::pair<PseudoState, String>>& stateProperties)
		{
			String cssCode;

			for(const auto& st: stateProperties)
			{
				jassert(st.second.endsWithChar(';'));

				cssCode << getElementSelector(c).toString();

				if(st.first.element != PseudoElementType::None)
				{
					cssCode << "::";

					switch(st.first.element)
					{
					case PseudoElementType::Before:
						cssCode << "before";
						break;
					case PseudoElementType::After:
						cssCode << "after";
						break;
					case PseudoElementType::Before2:
						cssCode << "before2";
						break;
					case PseudoElementType::After2:
						cssCode << "after2";
						break;
					case PseudoElementType::None:
					case PseudoElementType::All:
					default:
						break;;
					}
				}

				auto flag = st.first.stateFlag;

				cssCode << PseudoState::getPseudoClassName(flag);
				cssCode << " {" << st.second << "\n}\n\n";
			}

			return cssCode;
		}

		static void setFallbackStyleSheetWithStates(Component& c, const Array<std::pair<PseudoState, String>>& stateProperties)
		{
			auto cssCode = toCSSCode(c, stateProperties);
			c.getProperties().set("style", cssCode);
		}

		static void appendToElementStyle(Component& c, const String& additionalStyle);

		static String dump(Component& c);


		static void writeManualPseudoState(Component& c, int state)
		{
			c.getProperties().set("manualPseudoState", state);
		}

		static int getManualPseudoState(Component& c)
		{
			return (int)c.getProperties().getWithDefault("manualPseudoState", var(0));
		}

		static FlexItem createDefaultFlexItem(Component& c, FlexItem::Margin margin)
		{
			FlexItem item(c);

			auto db = getDefaultBounds(c);

			if(db.getWidth() > 0.0)
				item = item.withWidth(db.getWidth());

			if(db.getHeight() > 0.0)
				item = item.withHeight(db.getHeight());

			return item.withMargin(margin);
		}

		static void writeSelectorsToProperties(Component& c, const StringArray& selectors);
		static Selector getTypeSelectorFromComponentClass(Component* c);
		static Array<Selector> getClassSelectorFromComponentClass(Component* c);

		static void writeClassSelectors(Component& c, const Array<Selector>& classList, bool append);

		static void invalidateCache(Component& c);

		static void storeDefaultBounds(Component& c, Rectangle<int> b)
		{
			if(b.getWidth() > 0)
				c.getProperties().set("defaultWidth", b.getWidth());

			if(b.getHeight() > 0)
				c.getProperties().set("defaultHeight", b.getHeight());
		}

		/** Use this for components that wrap another component that should be queried for the style sheet. */
		static void setIsOpaqueWrapper(Component& c, bool shouldBeOpaque)
		{
			if(shouldBeOpaque)
			{
				jassert(c.getNumChildComponents() == 1);
			}

			c.getProperties().set("invisibleParent", true);
		}

		/** Use this to get the component that might have style sheets attached. */
		static Component* getComponentForStyleSheet(Component* c)
		{
			if(c == nullptr)
				return nullptr;

			if(c->getProperties()["invisibleParent"])
			{
				jassert(c->getNumChildComponents() == 1);
				return getComponentForStyleSheet(c->getChildComponent(0));
			}

			return c;
		}

		static Rectangle<int> getDefaultBounds(Component& c)
		{
			int w = 0;
			int h = 0;

			if(c.getProperties().contains("defaultWidth"))
				w = c.getProperties()["defaultWidth"];

			if(c.getProperties().contains("defaultHeight"))
				h = c.getProperties()["defaultHeight"];

			return { 0, 0, w, h };
		}

		static Selector getIdSelectorFromComponentClass(Component* c);

		static Selector getElementSelector(Component& c)
		{
			auto ptrValue = reinterpret_cast<uint64>(&c);
			return Selector(SelectorType::Element, String::toHexString(ptrValue));
		}

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

	

	struct SimpleTextDisplay: public Component,
							  public ComponentWithAutoTextSize
	{
		SimpleTextDisplay(ElementType s_):
		  Component(),
		  ComponentWithAutoTextSize(),
		  s(s_)
		{};

		String getTextToAutofit() const override { return currentText; }

		void setText(const String& text);

		void paint(Graphics& g) override;

		const ElementType s;
		String currentText;
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

    void removeFlexItem(Component& c) override
    {
		childSheets.erase(&c);
		removeChildComponent(&c);
    }

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

	void setFlexChildVisibility(Component* c, bool mustBeVisible, bool mustBeHidden)
	{
		VisibleState s;
		s.mustBeHidden = mustBeHidden;
		s.mustBeVisible = mustBeVisible;
		setFlexChildVisibility(c, s);
	}

	void setFlexChildVisibility(Component* c, VisibleState newState)
	{
		jassert(c->getParentComponent() == this);
		visibleStates[c] = newState;
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
        
        auto ss = childSheets[fc];
        
        if(ss == nullptr && parentToUse != nullptr)
        {
            childSheets[fc] = parentToUse->css.getForComponent(fc);
            ss = childSheets[fc];
        }
        
        if(ss != nullptr)
        {
            return childSheets[fc]->getFlexItem(fc, fullArea);
        }
        
        return {};
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

	void paint(Graphics& g) override
	{
		if(ss != nullptr)
		{
			if(auto root = CSSRootComponent::find(*this))
			{
				Renderer r(this, root->stateWatcher);

				auto s = r.getPseudoClassState();

				if(content.getNumChildComponents() == 0)
				{
					s |= (int)PseudoClassType::Empty;
					r.setPseudoClassState(s, true);
				}

				root->stateWatcher.checkChanges(this, ss, s);

				auto b = getLocalBounds().toFloat();
				r.setApplyMargin(true);
				r.drawBackground(g, b, ss);

				auto text = content.getNumChildComponents() == 0 ? "No child elements" : "";
				r.renderText(g, b, text, ss);
			}
		}
	}

	void resized() override;

	void rebuildLayout() override;

	void addFlexItem(Component& c) override;

	void removeFlexItem(Component& c) override
	{
		content.removeFlexItem(c);
	}

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
	

	HeaderContentFooter(bool useViewportContent);

	void setFixStyleSheet(StyleSheet::Collection& newCss);


	void showEditor();

	void setStylesheetCode(const String& code);
	void update(StyleSheet::Collection& newCss);

	virtual void styleSheetCollectionChanged() {};
	void resized() override;

	ScopedPointer<StyleSheetLookAndFeel> currentLaf;

	void setDefaultCSSProperties(DynamicObject::Ptr newDefaultProperties);

#if HISE_INCLUDE_CSS_DEBUG_TOOLS
	void paintOverChildren(Graphics& g) override
	{
		inspectorData.draw(g, getLocalBounds().toFloat(), css);
	}
#endif
    
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
