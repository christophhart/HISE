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
    };
    
	struct Helpers
	{
		static void setFallbackStyleSheet(Component& c, const String& properties);

		static void appendToElementStyle(Component& c, const String& additionalStyle)
		{
			auto elementStyle = c.getProperties()["style"].toString();
			elementStyle << additionalStyle;
			c.getProperties().set("style", elementStyle);
			invalidateCache(c);
		}

        static String dump(Component& c);
        
		static void writeSelectorsToProperties(Component& c, const StringArray& selectors);
		static Selector getTypeSelectorFromComponentClass(Component* c);
		static Array<Selector> getClassSelectorFromComponentClass(Component* c);

		static void writeClassSelectors(Component& c, const Array<Selector>& classList);

		static void invalidateCache(Component& c)
		{
			if(auto r = CSSRootComponent::find(c))
			{
				r->css.clearCache(&c);
			}
				
		}

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

	void rebuildRootLayout()
	{
        rebuildLayout();
        
		Component* c = this;
        
		while(c = c->getParentComponent())
		{
            if(auto pfc = dynamic_cast<FlexboxContainer*>(c))
            {
                pfc->rebuildLayout();
            }
		}
	}

	/** Call this to ensure that the layout is changed properly. */
	void rebuildLayout() override
	{
		for(int i = 0; i < getNumChildComponents(); i++)
		{
            auto c = getChildComponent(i);
            
			if(auto childSS = childSheets[c])
			{
				auto displayValue = childSS->getPropertyValueString({ "display", {} }) != "none";
                
                if(visibleStates.find(c) != visibleStates.end())
                {
                    displayValue = visibleStates[c].isVisible(displayValue);
                }
                
				getChildComponent(i)->setVisible(displayValue);
			}
		}
        
		callRecursive<FlexboxContainer>(this, [](FlexboxContainer* fc)
		{
			if(!dynamic_cast<Component*>(fc)->isVisible())
				return false;

			fc->fullRebuild = true;
			return false;
		});

		if(!getLocalBounds().isEmpty() && ss != nullptr)
			resized();
	}

	/** Adds a invisible component as child that will act as spacer. */
	void addSpacer();

	/** Adds a text element with a single format. */
	Component* addTextElement(const StringArray& selectors, const String& content)
	{
		auto nd = new SimpleTextDisplay(labelSelector);
		addFlexItem(*nd);
		textDisplays.add(nd);
		Helpers::setFallbackStyleSheet(*nd, "background: rgba(0, 0, 0, 0)");

		if(!selectors.isEmpty())
			Helpers::writeSelectorsToProperties(*nd, selectors);

		nd->setText(content);

		return nd;
	}

	Selector getSelector() const { return selector; }

	void addFlexItem(Component& c) override;

	void changeClass(const Selector& s, bool add)
	{
		// must be a class
		jassert(s.type == SelectorType::Class);
		
		auto list = Helpers::getClassSelectorFromComponentClass(this);

		// must not contain the selector...
		jassert(add == !list.contains(s));

		if(add)
			list.addIfNotAlreadyThere(s);
		else
			list.removeAllInstancesOf(s);

		Helpers::writeClassSelectors(*this, list);

		if(auto r = findParentComponentOfClass<CSSRootComponent>())
		{
			auto newss = r->css.getForComponent(this);

			if(newss != ss)
			{
				ss = newss;
				rebuildRootLayout();
			}
		}
	}

	
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
        auto c = getChildComponent(childIndex);
        visibleStates[c] = { mustBeVisible, mustBeHidden };
    }

private:

    std::map<Component*, VisibleState> visibleStates;
    
	ElementType labelSelector = ElementType::Paragraph;

	CSSRootComponent* parentToUse = nullptr;

	std::pair<Component*, Component*> getFirstLastComponents()
	{
		struct Data
		{
			bool operator<(const Data& other) const
			{
				if(order == -1 && other.order == -1)
					return indexInList < other.indexInList;
				else
					// makes order == -1 always < than a defined order...
					return other.order > order; 
			}

			bool operator>(const Data& other) const
			{
				return !(*this < other);
			}

			Component* c;
			int indexInList;
			int order;
		};

		Array<Data> thisList;

		for(int i = 0; i < getNumChildComponents(); i++)
		{
			Data d;
			d.c = getChildComponent(i);
			d.indexInList = i;
			d.order = -1;

			if(!d.c->isVisible())
				continue;
			
			if(auto css = childSheets[d.c])
			{
				auto order = css->getPropertyValueString({ "order", {}});
				if(!order.isEmpty())
					d.order = order.getIntValue();
			}
			
			thisList.add(d);
		}

		thisList.sort();
		return { thisList.getFirst().c, thisList.getLast().c };
	}
	
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
	FlexboxViewport(const Selector& selector):
	  content(selector),
	  s(selector)
	{
		viewport.setViewedComponent(&content, false);
		addAndMakeVisible(viewport);
        sf.addScrollBarToAnimate(viewport.getVerticalScrollBar());
        viewport.setScrollBarsShown(true, false);
        viewport.setScrollBarThickness(12);
		content.setApplyMargin(false);
		content.setDefaultStyleSheet("display: flex; width: 100%;height: auto;");
	}

	void setDefaultStyleSheet(const String& styleSheet) override
	{
		FlexboxComponent::Helpers::setFallbackStyleSheet(*this, styleSheet);
		
	}

	void setCSS(StyleSheet::Collection& css) override
	{
		content.setCSS(css);
		ss = css.getWithAllStates(s);
	}

	void resized() override
	{
		auto b = getLocalBounds().toFloat();

		if(ss != nullptr)
			b = ss->getArea(b, { "margin", {}});

        auto contentHeight = content.getAutoHeightForWidth(b.getWidth());

		auto w = b.getWidth();

		if(contentHeight > b.getHeight())
			w -= viewport.getScrollBarThickness();

		content.setSize(w, contentHeight);
		viewport.setBounds(b.toNearestInt());
	}

    void rebuildLayout() override
    {
        content.rebuildLayout();
        resized();
    }
    
	void addFlexItem(Component& c) override
	{
		content.addFlexItem(c);
	}

	Viewport viewport;
    ScrollbarFader sf;
	FlexboxComponent content;
	Selector s;
	StyleSheet::Ptr ss;
};

/** A combined component with three flex boxes for header, footer & content. */
struct HeaderContentFooter: public Component,
						    public CSSRootComponent
{
	HeaderContentFooter(bool useViewportContent):
	  body(ElementType::Body),
	  header(Selector("#header")),
	  
	  footer(Selector("#footer"))
	{
		auto cs = Selector("#content");

        if(useViewportContent)
			content = new FlexboxViewport(cs);
		else
			content = new FlexboxComponent(cs);

		body.setDefaultStyleSheet("display: flex; flex-direction: column;");
		header.setDefaultStyleSheet("width: 100%;height: 48px; background: blue;");
		content->setDefaultStyleSheet("width: 100%;flex-grow: 1;display: flex; background: red;");
		footer.setDefaultStyleSheet("width: 100%; height: 48px; display:flex; background: green;");

		addAndMakeVisible(body);

		body.addFlexItem(header);
		body.addFlexItem(*dynamic_cast<Component*>(content.get()));
		body.addFlexItem(footer);

		StyleSheet::Collection c;
		body.setCSS(c);
	}

	void showEditor();

	void setStylesheetCode(const String& code);
	void update(StyleSheet::Collection& newCss);

	virtual void styleSheetCollectionChanged() {};
	void resized() override;

	ScopedPointer<StyleSheetLookAndFeel> currentLaf;

	void setDefaultCSSProperties(DynamicObject::Ptr newDefaultProperties)
	{
		defaultProperties = newDefaultProperties;

		if(defaultProperties != nullptr)
		{
			for(const auto& p: defaultProperties->getProperties()) 
				css.setPropertyVariable(p.name, p.value);
		}
	}

    void paintOverChildren(Graphics& g) override
    {
        if(!inspectorData.first.isEmpty())
        {
            auto cb = inspectorData.first;
            auto b = getLocalBounds().toFloat();
            auto left = b.removeFromLeft(cb.getX());
            auto right = b.removeFromRight(b.getRight() - cb.getRight());
            auto top = b.removeFromTop(cb.getY());
            auto bottom = b.removeFromBottom(b.getBottom() - cb.getBottom());
            
            g.setColour(Colours::black.withAlpha(0.5f));
            g.fillRect(top);
            g.fillRect(left);
            g.fillRect(right);
            g.fillRect(bottom);
            
            g.setColour(Colour(SIGNAL_COLOUR).withAlpha(0.3f));
            g.drawRect(inspectorData.first, 1.0f);
            g.setColour(Colour(SIGNAL_COLOUR));
            auto f = GLOBAL_MONOSPACE_FONT();
            g.setFont(f);
            
            auto tb = inspectorData.first.withSizeKeepingCentre(f.getStringWidthFloat(inspectorData.second), inspectorData.first.getHeight() + 40).constrainedWithin(getLocalBounds().toFloat());
            
            if(inspectorData.first.getY() > 20)
                g.drawText(inspectorData.second, tb, Justification::centredTop);
            else
                g.drawText(inspectorData.second, tb, Justification::centredBottom);
        }
    }
    
    void setCurrentInspectorData(const std::pair<Rectangle<float>, String>& newData)
    {
        inspectorData = newData;
        repaint();
    }
    
    std::pair<Rectangle<float>, String> inspectorData;
    
	FlexboxComponent body;
	FlexboxComponent header;
	ScopedPointer<FlexboxContainer> content;
	FlexboxComponent footer;

private:


	DynamicObject::Ptr defaultProperties;
};

} // namespace simple_css
} // namespace hise
