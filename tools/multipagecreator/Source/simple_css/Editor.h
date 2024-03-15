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

struct LanguageManager: public mcl::LanguageManager
{
	struct KeywordDataBase
	{
		enum class KeywordType
		{
			Type,
			Property,
			PseudoClass,
			ReservedKeywords,
			ExpressionKeywords,
			numKeywords,
			Undefined
		};

		static CodeEditorComponent::ColourScheme getColourScheme();

		KeywordDataBase();

		const StringArray& getKeywords(KeywordType type) const
		{
			return keywords[(int)type];
		}

		std::array<StringArray, (int)KeywordType::numKeywords> keywords;

		KeywordType getKeywordType(const String& t)
		{
			int idx = 0;
			for(const auto& s: keywords)
			{
				if(s.contains(t))
					return (KeywordType)idx;

				idx++;
			}

			return KeywordType::Undefined;
		}
	};

	struct Tokeniser: public CodeTokeniser
	{
		SharedResourcePointer<KeywordDataBase> database;

		enum class Token
		{
			Type,
			Properties,
			PseudoClass,
			Keyword,
			Expression,
			Class,
			ID,
			SpecialCharacters,
			Value,
			Comment,
			Important,
			StringLiteral,
			numTokens
		};

		Tokeniser() = default;

		static void skipNumberValue(CodeDocument::Iterator& source);
		static void skipToSemicolon(CodeDocument::Iterator& source);
		static void skipComment(CodeDocument::Iterator& source);

		static void skipStringLiteral(CodeDocument::Iterator& source);

		static String skipWord(CodeDocument::Iterator& source);
		static bool isNumber(CodeDocument::Iterator& source);
		static bool isIdentifierStart(CodeDocument::Iterator& source);

		int readNextToken (CodeDocument::Iterator& source) override;

		/** Returns a suggested syntax highlighting colour scheme. */
	    CodeEditorComponent::ColourScheme getDefaultColourScheme() override;
	};

	LanguageManager(mcl::TextDocument& doc_);

	mcl::TextDocument& doc;

	void processBookmarkTitle(juce::String& bookmarkTitle) override
	{
		
	}

	SharedResourcePointer<KeywordDataBase> database;

	struct CssTokens: public mcl::TokenCollection::Provider
	{
		void addTokens(mcl::TokenCollection::List& tokens) override;

		KeywordDataBase database;
	};

	void setupEditor(mcl::TextEditor* editor) override;

	/** Add all token providers you want to use for this language. */
    void addTokenProviders(mcl::TokenCollection* t) override;

	Identifier getLanguageId() const override { return "CSS"; }

	CodeTokeniser* createCodeTokeniser() override { return new Tokeniser(); }
};



struct StyleSheetLookAndFeel: public LookAndFeel_V3
{
	StyleSheetLookAndFeel(const StyleSheet::Collection& collection_, StateWatcher& state_):
	  css(collection_),
	  state(state_)
	{
		setColour(PopupMenu::backgroundColourId, Colours::transparentBlack);
	};

	void drawButtonBackground(Graphics& g, Button& tb, const Colour&, bool, bool) override;

	void drawButtonText(Graphics& g, TextButton& tb, bool over, bool down) override;

	void fillTextEditorBackground (Graphics&, int width, int height, TextEditor&) override;
	void drawTextEditorOutline (Graphics&, int width, int height, TextEditor&) override {};

	Font getPopupMenuFont() override
	{
		if(auto ss = getBestPopupStyleSheet(true))
		{
			return ss->getFont(0, {});
		}

		return LookAndFeel_V3::getPopupMenuFont();
	}

	/** Fills the background of a popup menu component. */
    virtual void drawPopupMenuBackgroundWithOptions (Graphics& g,
                                                     int width,
                                                     int height,
                                                     const PopupMenu::Options& o)
	{
		Renderer r(nullptr, state);

		if(auto ss = getBestPopupStyleSheet(false))
			r.drawBackground(g, { (float)width, (float)height }, ss);
		else
			LookAndFeel_V3::drawPopupMenuBackgroundWithOptions(g, width, height, o);
	}

	void drawPopupMenuItem(Graphics& g, Rectangle<float> area, int flags, const String& text, bool isSeparator)
	{
		if(auto ss = getBestPopupStyleSheet(true))
		{
			Renderer r(nullptr, state);

			r.setPseudoClassState(flags);

			r.drawBackground(g, area.toFloat(), ss);

			if(isSeparator)
			{
				auto sep = ss->getArea(area.toFloat(), { "padding", flags});

				if(auto rss = css[ElementType::Ruler])
				{
					sep = rss->getArea(sep, { "margin", flags});
					sep = rss->getArea(sep, { "padding", flags});
					auto f = rss->getPixelValue(area.toFloat(), { "border-width", flags});
					sep = sep.withSizeKeepingCentre(sep.getWidth(), f);
					r.setCurrentBrush(g, rss, sep, {"border-color", flags }, Colours::black);
					auto radius = rss->getPixelValue(sep, { "border-top-left-radius", flags }, 0.0);
					g.fillRoundedRectangle(sep, radius);
				}
				else
				{
					r.setCurrentBrush(g, ss, sep, {"color", flags }, Colours::black);
					sep = sep.withSizeKeepingCentre(sep.getWidth(), 1.0f);
					g.fillRect(sep);
				}
			}
			else
			{
				r.renderText(g, area.toFloat(), text, ss);
			}

			
		}
	}

	/** Draws one of the items in a popup menu. */
    void drawPopupMenuItemWithOptions (Graphics& g, const Rectangle<int>& area,
                                               bool isHighlighted,
                                               const PopupMenu::Item& item,
                                               const PopupMenu::Options&) override
	{
		int flags = 0;

		if(isHighlighted && !(item.isSeparator || item.isSectionHeader))
			flags |= (int)PseudoClassType::Hover;

		if(item.isTicked)
			flags |= (int)PseudoClassType::Active;

		if(!item.isEnabled)
			flags |= (int)PseudoClassType::Disabled;

		if(item.subMenu != nullptr)
			flags |= (int)PseudoClassType::Root;

		DBG(flags);

		drawPopupMenuItem(g, area.toFloat(), flags, item.text, item.isSeparator);
	}

	StyleSheet::Ptr getBestPopupStyleSheet(bool getItem)
	{
		if(getItem)
		{
			if(auto c = css.getOrCreateCascadedStyleSheet({ Selector::withClass("popup"), Selector::withClass("popup-item")}))
				return c;
		}

		return css[Selector::withClass("popup")];
	}

	virtual void getIdealPopupMenuItemSizeWithOptions (const String& text,
                                                       bool isSeparator,
                                                       int standardMenuItemHeight,
                                                       int& idealWidth,
                                                       int& idealHeight,
                                                       const PopupMenu::Options& options) override
	{
		auto f = getPopupMenuFont();

		if(auto ss = getBestPopupStyleSheet(true))
		{
			auto textWidth = f.getStringWidthFloat(ss->getText(text, 0));
			auto h = f.getHeight();

			int state = 0;
			if(standardMenuItemHeight == -1)
				state = (int)PseudoClassType::Focus;

			if(auto v = ss->getPropertyValue({ "height", state }))
			{
				h = ExpressionParser::evaluate(v.valueAsString, { false, {h, h}, f.getHeight() });
			}

			Rectangle<float> ta(textWidth, h);

			auto pw = ss->getPseudoArea(ta, state, PseudoElementType::Before).getWidth();
			pw += ss->getPseudoArea(ta, state, PseudoElementType::After).getWidth();

			if(pw != 0)
				ta = ta.withSizeKeepingCentre(ta.getWidth() + pw, ta.getHeight());
			
			ta = ss->expandArea(ta, { "padding", state });
			ta = ss->expandArea(ta, { "margin", state });
			
			idealWidth = roundToInt(ta.getWidth());
			idealHeight = roundToInt(ta.getHeight());
		}
	}

	void drawPopupMenuSectionHeaderWithOptions (Graphics& g, const Rectangle<int>& area,
                                                        const String& sectionName,
                                                        const PopupMenu::Options&) override
	{
		drawPopupMenuItem(g, area.toFloat(), (int)PseudoClassType::Focus, sectionName, false);
	}

	void drawComboBox (Graphics& g, int width, int height, bool isButtonDown,
                                   int buttonX, int buttonY, int buttonW, int buttonH,
                                   ComboBox& cb) override
	{
		if(auto ss = css[ElementType::Selector])
		{
			Renderer r(&cb, state);
			
			state.checkChanges(&cb, ss, r.getPseudoClassState());

			r.drawBackground(g, cb.getLocalBounds().toFloat(), ss);

			r.renderText(g, cb.getLocalBounds().toFloat(), cb.getText(), ss);
		}
		else
		{
			LookAndFeel_V3::drawComboBox(g, width, height, isButtonDown, buttonX, buttonY, buttonW, buttonH, cb);
		}
	}

	Font getComboBoxFont (ComboBox&) override { return getPopupMenuFont(); }
	//Label* createComboBoxTextBox (ComboBox&) override { return nullptr; }
	void positionComboBoxText (ComboBox& cb, Label& label) override
	{
		label.setVisible(false);
#if 0
		if(auto ss = css[ElementType::Selector])
		{
			auto state = Renderer::getPseudoClassFromComponent(&cb);

			auto area = cb.getLocalBounds().toFloat();

			area = ss->getArea(area, { "margin", state });
			area = ss->getArea(area, { "padding", state });

			area = ss->truncateBeforeAndAfter(area, state);


			label.setBounds (area.toNearestInt());
			label.setFont (ss->getFont(state, area));
			label.setColour(Label::ColourIds::textColourId, ss->getColourOrGradient(area, { "color", state }, label.findColour(Label::textColourId)).first);
			label.setJustificationType(ss->getJustification(state, Justification::left));
		}
		else
		{
			LookAndFeel_V3::positionComboBoxText(cb, label);
		}
#endif
	}
	//PopupMenu::Options getOptionsForComboBoxPopupMenu (ComboBox&, Label&) override { }
	void drawComboBoxTextWhenNothingSelected (Graphics&, ComboBox&, Label&) override { }

	StateWatcher& state;
	StyleSheet::Collection css;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StyleSheetLookAndFeel);
};

struct FlexboxComponent: public Component
{
	void addSpacer()
	{
		auto c = new Component();
		Helpers::writeSelectorsToProperties(*c, { ".spacer"} );
		Helpers::setFallbackStyleSheet(*c, "flex-grow: 1;");
		addAndMakeVisible(c);
		spacers.add(c);
	}

	OwnedArray<Component> spacers;

	struct Helpers
	{
		static void setFallbackStyleSheet(Component& c, const String& properties)
		{
			c.getProperties().set("style", properties);
		}

		static void writeSelectorsToProperties(Component& c, const StringArray& selectors)
		{
			Array<var> classes;
			String id;

			for(const auto& s: selectors)
			{
				Selector sel(s);

				if(sel.type == SelectorType::Class)
					classes.add(sel.name);

				if(sel.type == SelectorType::ID)
					id = sel.name;
			}

			c.getProperties().set("class", classes);
			c.getProperties().set("id", id);
		}

		static Selector getTypeSelectorFromComponentClass(Component* c)
		{
			if(dynamic_cast<Button*>(c) != nullptr)
				return Selector(ElementType::Button);
			if(dynamic_cast<ComboBox*>(c) != nullptr)
				return Selector(ElementType::Selector);
			if(dynamic_cast<FlexboxComponent*>(c) != nullptr)
				return Selector(ElementType::Panel);
			if(dynamic_cast<juce::TextEditor*>(c) != nullptr)
				return Selector(ElementType::TextInput);

			return {};
		}

		static Array<Selector> getClassSelectorFromComponentClass(Component* c)
		{
			Array<Selector> list;

			auto classes = c->getProperties()["class"];

			if(classes.isString())
				list.add(Selector(SelectorType::Class, classes.toString()));
			else if(auto a = classes.getArray())
			{
				for(const auto& v: *a)
					list.add(Selector(SelectorType::Class, v.toString()));
			}

			return list;
		}

		static Selector getIdSelectorFromComponentClass(Component* c)
		{
			auto id = c->getProperties()["id"].toString();

			if(id.isNotEmpty())
				return Selector(SelectorType::ID, id);
			else
				return {};
		}

		

		static StyleSheet::Ptr getDefaultStyleSheetFunction(StyleSheet::Collection& css, Component* c)
		{
			Array<Selector> selectors;

			if(auto fc = dynamic_cast<FlexboxComponent*>(c))
			{
				// overwrite the default behaviour and only return the
				// selector that was defined
				selectors.add(fc->selector);
				selectors.addArray(getClassSelectorFromComponentClass(c));
			}
			else
			{
				if(auto ts = getTypeSelectorFromComponentClass(c))
					selectors.add(ts);

				auto classList = getClassSelectorFromComponentClass(c);

				selectors.addArray(classList);

				if(auto is = getIdSelectorFromComponentClass(c))
					selectors.add(is);
			}

			auto elementStyle = c->getProperties()["style"].toString();

			StyleSheet::Ptr ptr;

			if(elementStyle.isNotEmpty())
			{
				String code;

				auto ptrValue = reinterpret_cast<uint64>(c);
				Selector elementSelector(SelectorType::Element, String::toHexString(ptrValue));
				
				code << elementSelector.toString();
				code << "{ " << elementStyle;
				if(!code.endsWithChar(';')) code << ";";
				code << " }";

				Parser p(code);
				auto ok = p.parse();
				if(ok.wasOk())
					ptr = p.getCSSValues().getOrCreateCascadedStyleSheet({elementSelector});

				css.addElementStyle(ptr);
			}

			auto cssPtr = css.getOrCreateCascadedStyleSheet(selectors);

			if(ptr != nullptr)
			{
				if(cssPtr != nullptr)
					ptr->copyPropertiesFrom(cssPtr);
			}
			else
			{
				ptr = cssPtr;
			}
			
			return ptr;
		}
	};

	

	using ChildStyleSheetFunction = std::function<StyleSheet::Ptr(StyleSheet::Collection&, Component*)>;

	FlexboxComponent(const Selector& s):
	  selector(s)
	{
		Helpers::writeSelectorsToProperties(*this, { s.toString() });
		setRepaintsOnMouseActivity(true);
	}
	
	void setCSS(StyleSheet::Collection& css, const ChildStyleSheetFunction& f=Helpers::getDefaultStyleSheetFunction)
	{
		ss = f(css, this);

		childSheets.clear();

		for(int i = 0; i < getNumChildComponents(); i++)
		{
			auto c = getChildComponent(i);
			childSheets[c] = f(css, c);

			if(auto div = dynamic_cast<FlexboxComponent*>(c))
				div->setCSS(css, f);
		}
	}

	void setDefaultStyleSheet(const String& css)
	{
		Helpers::setFallbackStyleSheet(*this, css);
	}

	void paint(Graphics& g) override
	{
#if 0
		g.setColour(Colours::white.withAlpha(0.2f));

		g.drawRect(getLocalBounds().toFloat(), 1.0f);
		g.drawText(selector.toString(), getLocalBounds().toFloat(), Justification::centred);
		return;
#endif

		if(ss != nullptr)
		{
			if(auto p = ComponentWithCSS::find(*this))
			{
				Renderer r(this, p->stateWatcher);
				p->stateWatcher.checkChanges(this, ss, r.getPseudoClassState());

				auto b = getLocalBounds().toFloat();
				b = ss->getBounds(b, PseudoState(r.getPseudoClassState()));

				r.drawBackground(g, b, ss);
			}
		}
	}

	void resized()
	{
		auto b = getLocalBounds().toFloat();

		if(ss != nullptr)
		{
			b = ss->getBounds(b, {});

			b = ss->getArea(b, { "margin", {}});
			b = ss->getArea(b, { "padding", {}});
		}

		if(ss != nullptr)
		{
			flexBox = ss->getFlexBox();

			FlexItem::Margin margin;

			if(auto mv = ss->getPropertyValue({"gap", {}}))
			{
				ExpressionParser::Context cb;
				cb.fullArea = getLocalBounds().toFloat();
				cb.useWidth = true;
				cb.defaultFontSize = 16.0f;
				margin = FlexItem::Margin(ExpressionParser::evaluate(mv.valueAsString, cb) * 0.5f);
			}

			for(int i = 0; i < getNumChildComponents(); i++)
			{
				auto c = getChildComponent(i);
				
				if(auto ptr = childSheets[c])
					flexBox.items.add(ptr->getFlexItem(c, b).withMargin(margin));
				else
					flexBox.items.add(FlexItem(*c).withMargin(margin));
			}
		}

#if 0
		if(flexBox.flexDirection == FlexBox::Direction::column ||
		   flexBox.flexDirection == FlexBox::Direction::columnReverse)
		{
			for(auto& s: flexBox.items)
			{
				std::swap(s.minHeight, s.minWidth);
				std::swap(s.height, s.width);
				std::swap(s.maxHeight, s.maxWidth);
			}
		}
#endif

		flexBox.performLayout(b);

		callRecursive<FlexboxComponent>(this, [this](FlexboxComponent* fc)
		{
			if(fc == this)
				return false;

			fc->resized();
			return false;
		});
	}

	juce::FlexBox flexBox;
	Selector selector;
	StyleSheet::Ptr ss;

	std::map<Component*, StyleSheet::Ptr> childSheets;
};

struct Editor: public Component,
	           public ComponentWithCSS,
		       public TopLevelWindowWithKeyMappings
{
	Editor();

	~Editor()
	{
		TopLevelWindowWithKeyMappings::saveKeyPressMap();
		context.detach();
	}

	File getKeyPressSettingFile() const override { return File::getSpecialLocation(File::SpecialLocationType::userDesktopDirectory).getChildFile("something.js"); }
	bool keyPressed(const KeyPress& key) override;

	void compile();
	void resized() override;
	void paint(Graphics& g) override;

	mcl::TokenCollection::Ptr tokenCollection;
	hise::GlobalHiseLookAndFeel laf;
	//Rectangle<float> previewArea;
	
	FlexboxComponent body;
	FlexboxComponent header;
	FlexboxComponent content;
	FlexboxComponent footer;

	ScopedPointer<LookAndFeel> css_laf;
	
	TextButton cancel, prev, next;

	SubmenuComboBox selector;
	TextEditor textInput;
	
	juce::CodeDocument jdoc;
	mcl::TextDocument doc;
	mcl::FullEditor editor;
	juce::TextEditor list;
	OpenGLContext context;
};

	
} // namespace simple_css
} // namespace hise