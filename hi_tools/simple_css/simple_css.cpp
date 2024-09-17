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

#include "simple_css.h"

#include "HelperClasses.cpp"
#include "StyleSheet.cpp"
#include "CssParser.cpp"
#include "Animator.cpp"
#include "Renderer.cpp"
#include "CSSLookAndFeel.cpp"
#include "FlexboxComponent.cpp"
#include "LanguageManager.cpp"
#include "Editor.cpp"


namespace hise {
namespace simple_css {
using namespace juce;

struct CssTestSuite: public UnitTest
{
	CssTestSuite():
	  UnitTest("CSS tests", "ui")
	{};

	void runTest() override
	{
		testSelectors();
		testParser();
		testValueParsers();
		
	}

	template <typename ComponentType=Component>
	void expectSelectorRed(const StringArray& parentSelectors, 
					       const StringArray& componentSelectors, 
					       const String& selectorInCss, 
		                   const String& fallbackStylesheet={})
	{
		auto parent = createComponentWithSelectors<>(parentSelectors);
		auto c = createComponentWithSelectors<ComponentType>(componentSelectors);

		if(fallbackStylesheet.isNotEmpty())
			FlexboxComponent::Helpers::setFallbackStyleSheet(*c, fallbackStylesheet);

		parent->addChildComponent(c.get());

		String code;

		code << selectorInCss;

		Parser p(code);
		auto ok = p.parse();

		expect(ok.wasOk(), ok.getErrorMessage());

		auto css = p.getCSSValues();
		auto ss = css.getForComponent(c.get());

		expect(ss != nullptr, "CSS not found");

		if(ss != nullptr)
		{
			expect(ss->getColourOrGradient({}, { "background", {}}).first == Colours::red, "didn't select red stylesheet");
		}
	}

	void testComplexSelector(const String& selectorCode, const std::function<void(ComplexSelector::Ptr)>& f)
	{
		Parser p(selectorCode + " { background: red; }");
		p.parse();
		auto css = p.getCSSValues();
		auto ss = css.getFirst();

		if(ss->complexSelectors.size() == 1)
		{
			f(ss->complexSelectors[0]);
		}
		else
		{
			expect(false, "not a single complex selector");
		}
	}

	template <typename T=Component> std::unique_ptr<T> createComponentWithSelectors(const StringArray& selectors)
	{
		static_assert(std::is_base_of<Component, T>() || std::is_same<T, Component>(), "not a component");
		auto c = std::make_unique<T>("name");
		FlexboxComponent::Helpers::writeSelectorsToProperties(*c, selectors);
		return c;
	}

	static bool matchesComponent(ComplexSelector::Ptr cs, Component* c)
	{
		auto selectors = ComplexSelector::getSelectorsForComponent(c);
		Array<Selector> pSelectors;

		if(auto p = c->getParentComponent())
			pSelectors = ComplexSelector::getSelectorsForComponent(p);


		return cs->matchesSelectors(selectors, pSelectors);
	}

	void testSelectors()
	{
		beginTest("test complex selector parser");

		testComplexSelector(".myclass", [&](ComplexSelector::Ptr cs)
		{
			expect(!cs->hasParentSelectors(), "parent not empty");
			expect(cs->thisSelectors.isSingle() == 1, "selectors not 1");

			auto c = createComponentWithSelectors({ ".myclass "});
			expect(matchesComponent(cs, c.get()));
			
		});
		
		testComplexSelector(".parent .myclass", [&](ComplexSelector::Ptr cs)
		{
			expect(cs->parentSelectors.isSingle() == 1, "parent not 1");
			expect(cs->thisSelectors.isSingle() == 1, "selectors not 1");

			auto p = createComponentWithSelectors({ ".parent"});
			auto c = createComponentWithSelectors({ ".myclass "});

			p->addAndMakeVisible(*c);
			expect(matchesComponent(cs, c.get()));
		});

		testComplexSelector(".parent *", [&](ComplexSelector::Ptr cs)
		{
			expect(cs->parentSelectors.isSingle() == 1, "parent not 1");
			expect(cs->thisSelectors.isSingle() == 1, "selectors not 1");

			auto p = createComponentWithSelectors({ ".parent"});
			auto c = createComponentWithSelectors({ ".myclass "});

			p->addAndMakeVisible(*c);
			expect(matchesComponent(cs, c.get()));
		});

		testComplexSelector({".myclass.secondclass"}, [&](ComplexSelector::Ptr cs)
		{
			expect(!cs->hasParentSelectors(), "parent not 0");
			expect(cs->thisSelectors.selectors.size() == 2, "selectors not 2");

			auto c0 = createComponentWithSelectors({ ".myclass"});
			auto c1 = createComponentWithSelectors({ ".secondclass"});
			auto c2 = createComponentWithSelectors({ ".myclass", ".secondclass"});
			auto c3 = createComponentWithSelectors({ ".myclass ", ".secondclass", "#someid"});

			expect(!matchesComponent(cs, c0.get()));
			expect(!matchesComponent(cs, c1.get()));
			expect(matchesComponent(cs, c2.get()));
			expect(matchesComponent(cs, c3.get()));
		});

		testComplexSelector({"button.myclass"}, [&](ComplexSelector::Ptr cs)
		{
			expect(!cs->hasParentSelectors(), "parent not 0");
			expect(cs->thisSelectors.selectors.size() == 2, "selectors not 2");

			auto c0 = createComponentWithSelectors<Component>({ ".myclass"});
			auto c1 = createComponentWithSelectors<TextButton>({ ".secondclass"});
			auto c2 = createComponentWithSelectors<TextButton>({ ".myclass", ".secondclass"});
			auto c3 = createComponentWithSelectors<TextButton>({ ".myclass "});
			
			expect(!matchesComponent(cs, c0.get()));
			expect(!matchesComponent(cs, c1.get()));
			expect( matchesComponent(cs, c2.get()));
			expect( matchesComponent(cs, c3.get()));
		});

		testComplexSelector({"select.myclass button:hover"}, [&](ComplexSelector::Ptr cs)
		{
			expect(cs->hasParentSelectors(), "parent not 1");
			expect(cs->parentSelectors.selectors.size() == 2, "selectors not 2");
			expect(cs->thisSelectors.selectors.size() == 1, "selectors not 1");

			auto ep = createComponentWithSelectors<ComboBox>({});
			auto cp = createComponentWithSelectors<ComboBox>({ ".myclass"});

			auto c0 = createComponentWithSelectors<TextButton>({});
			auto c1 = createComponentWithSelectors<TextButton>({ ".secondclass"});
			auto c2 = createComponentWithSelectors<TextButton>({ ".myclass", ".secondclass"});
			auto c3 = createComponentWithSelectors<TextButton>({ ".myclass "});

			ep->addChildComponent(*c0);
			ep->addChildComponent(*c1);
			cp->addChildComponent(*c2);
			cp->addChildComponent(*c3);

			expect(!matchesComponent(cs, c0.get()));
			expect(!matchesComponent(cs, c1.get()));
			expect( matchesComponent(cs, c2.get()));
			expect( matchesComponent(cs, c3.get()));
		});

		beginTest("testing selectors");



		// simple type & class selector tests
		expectSelectorRed<>({}, { ".my-class" }, ".my-class { background: red; }");
		expectSelectorRed<>({}, { "#my-id" }, "#my-id { background: red; }");
		expectSelectorRed<>({}, { "#my-id" }, "#my-id { background: red; }");

		expectSelectorRed<>({ ".parent-class" }, { ".child-class" }, ".parent-class .child-class { background: red; }");

		// test all selector
		expectSelectorRed<>({}, { ".my-button"}, "* { background: green; } .my-button { background: red; }");
		expectSelectorRed<>({}, { ".my-button"}, ".my-button { background: red; } * { background: green; }");

        expectSelectorRed<>({}, { ".my-button"}, ".my-button { background: green; } .my-button { background: red; }");
        
        expectSelectorRed<>({}, { ".my-button"}, ".my-button { background: red !important; } .my-button { background: green; }");
        
		expectSelectorRed<TextButton>({}, {}, "button { background: red; }");
		expectSelectorRed<ComboBox>({}, {}, "select { background: red; }");

		expectSelectorRed<TextButton>({}, { ".myclass"}, "button { background: green; } .myclass { background: red; }");
		expectSelectorRed<>({}, { ".myclass"}, "button { background: green; } .myclass { background: red; }");
		expectSelectorRed<TextButton>({}, { ".myclass"}, "button { background: red; } .myclass2 { background: green; }");

		expectSelectorRed<TextButton>({}, { ".myclass"}, "button { color: blue; }", "background: red;");
		

	}

	void testValueParsers()
	{
		beginTest("Testing colour parser");

		expectColour("red", Colours::red);

		expectColour("transparent", Colours::transparentBlack);
		expectColour("#0f0", Colour(0xFF00FF00));
		expectColour("#333", Colour(0xFF333333));
		expectColour("rgb(0, 0, 255)", Colour(0xFF0000FF));
		expectColour("rgba(0, 0, 255, 0.4)", Colour(0xFF0000FF).withAlpha(0.4f));
		expectColour("hsl(127, 127, 127)", Colour::fromHSL((127.0f / 255.0f), (127.0f / 255.0f), (127.0f / 255.0f), 1.0f));
		expectColour("hsl(0, 127, 300)", Colour::fromHSL(0.f, 0.5f, 1.0f, 1.0f));

		beginTest("Testing margin + padding");

		expectArea("margin: 10px", { 10, 10, 80, 80 });
		expectArea("margin: 10px; padding: 5px", { 15, 15, 70, 70 });
		expectArea("margin: 10%", { 10, 10, 80, 80 });
		expectArea("margin: 10%; padding: 10%", { 18, 18, 64, 64 });
		expectArea("margin: 10px 20px", { 20, 10, 60, 80 });
		expectArea("padding: 35px 7px 5px", { 7, 35, 100 - 2*7, 100 - 35 - 5 });
		expectArea("margin: 25px 7px 5px", { 7, 25, 100 - 2*7, 100 - 25 - 5 });
		expectArea("margin: -10%", { -10, -10, 120, 120 });

		expectArea("width: 50px; height: 50px;", { 0, 0, 50, 50 });
		expectArea("width: 50px; height: 50px; margin: auto", { 25, 25, 50, 50 });
		expectArea("width: 50px; height: 50px; left: 10px", { 10, 0, 50, 50 });

		expectArea("width: 50px; height: 50px; right: 10px", { 40, 0, 50, 50 });
		expectArea("width: 50px; height: 50px; right: 10px; top: 10%", { 40, 10, 50, 50 });

		expectArea("width: 50px; height: 50px; right: 10px; top: 10%; margin: 5px", { 45, 15, 40, 40 });
		expectArea("width: 50px; height: 50px; right: 10px; top: 10%; padding: 5px", { 45, 15, 40, 40 });

		expectArea("width: 50px; height: 50px; min-width: 80px;", { 0, 0, 80, 50 });
		expectArea("width: 50px; height: 50px; min-height: 80px;", { 0, 0, 50, 80 });

		expectArea("width: 50px; height: 50px; min-width: 80px;", { 0, 0, 80, 50 });
		expectArea("width: 50px; height: 50px; min-height: 80px;", { 0, 0, 50, 80 });

#if JUCE_WINDOWS
		expectAreaFromText("1234567890", "padding: 10px; font-family: monospace; font-size: 16px;", {0, 0, 116, 36});
		expectAreaFromText("1234567890", "font-family: monospace; font-size: 16px; max-width: 80px; margin-bottom: 10px;", {0, 0, 80, 26});
#endif

	}

	void expectAreaFromText(const String& textContent, const String& cssCode, Rectangle<int> expectedArea)
	{
		Positioner pos(parse("body { " + cssCode + " }"), {0.0f, 0.0f, 100.0f, 100.0f}, false);

		auto area = pos.getLocalBoundsFromText(Selector(ElementType::Body), textContent);

		expectEquals(area.toNearestInt().toString(), expectedArea.toString(), cssCode);
	}

	StyleSheet::Collection parse(const String& code)
	{
		Parser p(code);
		auto ok = p.parse();
		if(ok.failed())
			expect(false, "parser error: " + ok.getErrorMessage());

		return p.getCSSValues();
	}

	void expectArea(const String& propertyValues, Rectangle<int> resultingRectangle)
	{
		String code;
		code << "body { " << propertyValues;

		if(!code.endsWithChar(';'))
			code << ';';
			
		code << " }";

		if(auto ss = parse(code).getFirst())
		{
			ss->setFullArea({0.0f, 0.0f, 100.0f, 100.0f});

			auto area = ss->getBounds({0.0f, 0.0f, 100.0f, 100.0f}, {});
			area = ss->getArea(area, { "margin", {} });
			area = ss->getArea(area, { "padding", {} });
			
			expectEquals(area.toNearestInt().toString(), resultingRectangle.toString(), propertyValues);
		}
	}

	void expectColour(const String& colourValue, Colour c)
	{
		String code;
		code << "body { background: " << colourValue << ";}";
		Parser p(code);

		auto ok = p.parse();

		if(!ok)
			expect(false, colourValue + ": " + ok.getErrorMessage());

		if(auto ss = p.getCSSValues().getFirst())
		{
			auto rc = ss->getColourOrGradient({}, { "background", 0}).first;
			expectEquals(rc.toDisplayString(true), c.toDisplayString(true));
		}
	}

	void testParser()
	{
		beginTest("Testing comments");

		expectStylesheet("/* comment */body /* comment */ { background-color: red; }", ElementType::Body);
		expectStylesheet("body /* comment */ { background-color: red; }", ElementType::Body);
		expectStylesheet("body/*comment*/:hover { background-color: red; }", ElementType::Body);
		expectStylesheet("body/*comment*/:/*comment*/hover { background-color: red; }", ElementType::Body);
		expectStylesheet("body { /*comment */ background-color: red; }", ElementType::Body);
		expectStylesheet("body { background-color: /* comment */ red; }", ElementType::Body);
		expectStylesheet("body { background-color: red /* comment */; }", ElementType::Body);
		expectStylesheet("body { background-color: red; /* comment */ }", ElementType::Body);
		expectStylesheet("body { background-color: red; } /* comment */", ElementType::Body);

		beginTest("Testing CSS selectors");

		expectStylesheet("body { background-color: red; }", ElementType::Body);
		expectStylesheet("button { background-color: red; }", ElementType::Button);
		expectStylesheet("select { background-color: red; }", ElementType::Selector);
		expectStylesheet("input { background-color: red; }", ElementType::TextInput);
		expectStylesheet("div { background-color: red; }", ElementType::Panel);
		expectStylesheet(".my-class { background-color: red; }", Selector(SelectorType::Class, "my-class"));
		expectStylesheet(".my-class { background-color: red; }", Selector(".my-class"));
		expectStylesheet(".my-class, .my-class2 { background: red; }", Selector(".my-class2"));
		expectStylesheet("button, .my-class2 { background: red; }", Selector(".my-class2"));
		expectStylesheet("body {} button { background: red; }", ElementType::Button);

		beginTest("Testing pseudoclasses");

		expectStylesheet("body:hover { background-color: red; }", ElementType::Body);
		expectStylesheet("body:hover, body:active { background-color: red; }", ElementType::Body);
		expectStylesheet("body::before { background-color: red; }", ElementType::Body);
		expectStylesheet("body::after:hover { background-color: red; }", ElementType::Body);
		expectStylesheet("body:disabled { background-color: red; }", ElementType::Body);

		beginTest("Testing selector property queries");

		expectRed("body", ElementType::Body, {});
		expectRed("body, body:hover", ElementType::Body, PseudoState(PseudoClassType::Hover));
		expectRed("body:hover", ElementType::Body, PseudoState(PseudoClassType::Hover));
		expectRed("button::after", ElementType::Button, PseudoState(PseudoClassType::None, PseudoElementType::After));
		expectRed("button:active::after", ElementType::Button, PseudoState(PseudoClassType::Active, PseudoElementType::After));
	}

	void expectRed(const String selectorCode, const Selector& s, PseudoState state)
	{
		String css;
		css << selectorCode << " { background: red; }";
		Parser p(css);
		auto ok = p.parse();

		if(!ok)
			expect(false, "parser error: " + ok.getErrorMessage());

		if(auto ss = p.getCSSValues().getWithAllStates(nullptr, s))
		{
			auto c = ss->getColourOrGradient({}, { "background", state }).first;
			expect(c == Colours::red, selectorCode + " can't resolve background property");
		}
		else
		{
			expect(false, "Can't find stylesheet for selector " + selectorCode);
		}

	}

	void expectStylesheet(const String& cssCode, const Selector& s)
	{
		Parser p(cssCode);
		auto ok = p.parse();

		if(ok.failed())
			expect(false, "Parser error: " + ok.getErrorMessage());

		auto ss = p.getCSSValues().getWithAllStates(nullptr, s);
		expect(ss != nullptr, cssCode);
	}
};

//static CssTestSuite cssTests;
}
}
