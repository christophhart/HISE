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
namespace simple_css
{
using namespace juce;

struct ComponentWithCSS
{
	ComponentWithCSS():
	  stateWatcher(animator)
	{
		css.setAnimator(&animator);
	};

	static ComponentWithCSS* find(Component& c)
	{
		if(auto self = dynamic_cast<ComponentWithCSS*>(&c))
			return self;

		return c.findParentComponentOfClass<ComponentWithCSS>();
	}

	Animator animator;
	StateWatcher stateWatcher;
	StyleSheet::Collection css;
};

struct Positioner
{
	enum class Direction
	{
		Left,
		Right,
		Top,
		Bottom
	};

	struct RemoveHelpers
	{
		template <Direction D> static Rectangle<float> slice(Rectangle<float>& area, float amount)
		{
			switch(D)
			{
			case Direction::Left: return area.removeFromLeft(amount);
			case Direction::Right: return area.removeFromRight(amount);
			case Direction::Top: return area.removeFromTop(amount);
			case Direction::Bottom: return area.removeFromBottom(amount);
			}
		}

		template <Direction D> static Rectangle<float> shrink(Rectangle<float> area, float amount)
		{
			return slice<D>(area, amount);
		}
	};

	Positioner(StyleSheet::Collection styleSheet, Rectangle<float> totalArea_, bool applyMargin_);

	template <Direction D> Rectangle<float> slice(const Array<Selector>& s, float defaultValue)
	{
		if(auto ss = css.getOrCreateCascadedStyleSheet(s))
		{
			ss->setFullArea(bodyArea);

			auto key = (D == Direction::Left || D == Direction::Right) ? "width" : "height";
			auto h = ss->getPixelValue(totalArea, { key, {} }, defaultValue);
			auto positionType = ss->getPositionType({});

			Rectangle<float> copy = totalArea;

			auto shouldShrink = positionType == PositionType::absolute || positionType == PositionType::fixed;
			auto& toUse = shouldShrink ? copy : totalArea;
			auto b = RemoveHelpers::slice<D>(toUse, h);
			b = ss->getBounds(b, {});

			if(applyMargin)
				return ss->getArea(b, { "margin", 0 });
			else
				return b;
		}

		return RemoveHelpers::slice<D>(totalArea, defaultValue);
	}

	Rectangle<int> getLocalBoundsFromText(const Array<Selector>& s, const String& text, Rectangle<int> defaultBounds={})
	{
		if(auto ss = css.getOrCreateCascadedStyleSheet(s))
		{
			return ss->getLocalBoundsFromText(text).toNearestInt();
		}

		return defaultBounds;
	}

	Rectangle<float> removeFromTop(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Top>(s, defaultValue); }
	Rectangle<float> removeFromBottom(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Bottom>(s, defaultValue); }
	Rectangle<float> removeFromLeft(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Left>(s, defaultValue); }
	Rectangle<float> removeFromRight(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Right>(s, defaultValue); } 

	bool applyMargin = false;
	Rectangle<float> bodyArea;

private:

	

	Rectangle<float> totalArea;
	StyleSheet::Collection css;
};


struct Renderer: public Animator::ScopedComponentSetter
{
	Renderer(Component* c, StateWatcher& state_):
	  ScopedComponentSetter(c)  ,
	  currentComponent(c),
	  state(state_)
	{};
	
	

	static int getPseudoClassFromComponent(Component* c);

	void drawBackground(Graphics& g, Rectangle<float> area, StyleSheet::Ptr ss, PseudoElementType type = PseudoElementType::None);

	

	void renderText(Graphics& g, Rectangle<float> area, const String& text, StyleSheet::Ptr ss);

	void setPseudoClassState(int state)
	{
		pseudoClassState = state;
	}

	void setCurrentBrush(Graphics& g, StyleSheet::Ptr ss, Rectangle<float> area, const PropertyKey& key, Colour defaultColour=Colours::transparentBlack);

	int getPseudoClassState() const
	{
		return currentComponent != nullptr ? getPseudoClassFromComponent(currentComponent) : pseudoClassState;
	}



private:

	int pseudoClassState = 0;
	Component* currentComponent;
	StateWatcher& state;
};

} // namespace simple_css
} // namespace hise