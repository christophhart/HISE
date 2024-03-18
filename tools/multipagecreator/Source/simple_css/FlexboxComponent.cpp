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

namespace hise
{
namespace simple_css
{
	void FlexboxComponent::Helpers::setFallbackStyleSheet(Component& c, const String& properties)
	{
		c.getProperties().set("style", properties);
	}

	void FlexboxComponent::Helpers::writeSelectorsToProperties(Component& c, const StringArray& selectors)
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

	Selector FlexboxComponent::Helpers::getTypeSelectorFromComponentClass(Component* c)
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

	Array<Selector> FlexboxComponent::Helpers::getClassSelectorFromComponentClass(Component* c)
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

	Selector FlexboxComponent::Helpers::getIdSelectorFromComponentClass(Component* c)
	{
		auto id = c->getProperties()["id"].toString();

		if(id.isNotEmpty())
			return Selector(SelectorType::ID, id);
		else
			return {};
	}

	StyleSheet::Ptr FlexboxComponent::Helpers::getDefaultStyleSheetFunction(StyleSheet::Collection& css, Component* c)
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

		if(auto root = CSSRootComponent::find(*c))
			css.setAnimator(&root->animator);

		return ptr;
	}

	FlexboxComponent::FlexboxComponent(const Selector& s):
		selector(s)
	{
		setOpaque(false);
		Helpers::writeSelectorsToProperties(*this, { s.toString() });
		setRepaintsOnMouseActivity(true);
	}

	void FlexboxComponent::setCSS(StyleSheet::Collection& css, const ChildStyleSheetFunction& f)
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

	void FlexboxComponent::setDefaultStyleSheet(const String& css)
	{
		Helpers::setFallbackStyleSheet(*this, css);
	}

	void FlexboxComponent::paint(Graphics& g)
	{
		if(ss != nullptr)
		{
			if(auto p = CSSRootComponent::find(*this))
			{
				Renderer r(this, p->stateWatcher);
				p->stateWatcher.checkChanges(this, ss, r.getPseudoClassState());

				auto b = getLocalBounds().toFloat();
				b = ss->getBounds(b, PseudoState(r.getPseudoClassState()));

				r.drawBackground(g, b, ss);
			}
		}
	}

	void FlexboxComponent::resized()
	{
		auto b = getLocalBounds().toFloat();

		if(ss != nullptr)
		{
			b = ss->getBounds(b, {});

			b = ss->getArea(b, { "margin", {}});
			b = ss->getArea(b, { "padding", {}});
		}

		std::vector<std::pair<Component*, Rectangle<int>>> absolutePositions;

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
				{
					auto childPos = ptr->getPositionType({});
                    
					if(childPos == PositionType::absolute || childPos == PositionType::fixed)
					{
						auto absoluteBounds = ptr->getBounds(b, {}).toNearestInt();
						absolutePositions.push_back({ c, absoluteBounds });
					}
					else
					{
						flexBox.items.add(ptr->getFlexItem(c, b).withMargin(margin));
					}
				}
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

		for(auto& ab: absolutePositions)
		{
			ab.first->toFront(false);
			ab.first->setBounds(ab.second);
		}
			

		callRecursive<FlexboxComponent>(this, [this](FlexboxComponent* fc)
		{
			if(fc == this)
				return false;

			fc->resized();
			return false;
		});


	}

void FlexboxComponent::addSpacer()
{
	auto c = new Component();
	Helpers::writeSelectorsToProperties(*c, { ".spacer"} );
	Helpers::setFallbackStyleSheet(*c, "flex-grow: 1;");
	addAndMakeVisible(c);
	spacers.add(c);
}

}
}


