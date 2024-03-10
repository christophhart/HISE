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
	Positioner(StyleSheet::Collection styleSheet, Rectangle<float> totalArea_):
	  css(styleSheet),
	  totalArea(totalArea_)
	{
		if(auto ss = css[Selector(ElementType::Body)])
		{
			totalArea = ss->getArea(totalArea, { "margin", 0});

			bodyArea = totalArea;

			totalArea = ss->getArea(totalArea, { "padding", 0});
		}
		else
			bodyArea = totalArea;
	}

	Rectangle<float> removeFromTop(const Selector& s, float defaultValue = 0.0f)
	{
		if(auto ss = css[s])
		{
			auto h = ss->getPixelValue(totalArea, { "height", 0 }, defaultValue);
			auto b = totalArea.removeFromTop(h);

			if(applyMargin)
				return ss->getArea(b, { "margin", 0 });
			else
				return b;
		}

		return totalArea.removeFromTop(defaultValue);
	}

	Rectangle<float> removeFromBottom(const Selector& s, float defaultValue = 0.0f)
	{
		if(auto ss = css[s])
		{
			auto h = ss->getPixelValue(totalArea, { "height", 0 }, defaultValue);
			auto b = totalArea.removeFromBottom(h);

			if(applyMargin)
				return ss->getArea(b, { "margin", 0 });
			else
				return b;
		}

		return totalArea.removeFromBottom(defaultValue);
	}

	Rectangle<float> removeFromLeft(const Selector& s, float defaultValue = 0.0f)
	{
		if(auto ss = css[s])
		{
			auto h = ss->getPixelValue(totalArea, { "width", 0 }, defaultValue);
			auto b = totalArea.removeFromLeft(h);
			
			if(applyMargin)
				return ss->getArea(b, { "margin", 0 });
			else
				return b;
		}

		return totalArea.removeFromLeft(defaultValue);
	}

	Rectangle<float> removeFromRight(const Selector& s, float defaultValue = 0.0f)
	{
		if(auto ss = css[s])
		{
			auto h = ss->getPixelValue(totalArea, { "width", 0 }, defaultValue);
			auto b = totalArea.removeFromRight(h);

			if(applyMargin)
				return ss->getArea(b, { "margin", 0 });
			else
				return b;
		}
		
		return totalArea.removeFromRight(defaultValue);
	}
	
	bool applyMargin = false;
	Rectangle<float> bodyArea;

private:

	
	Rectangle<float> totalArea;
	StyleSheet::Collection css;
};


struct Renderer: public Animator::ScopedComponentSetter
{
	Renderer(Component* c):
		ScopedComponentSetter(c)  ,
	    currentComponent(*c)
	{};
	
	void setCurrentBrush(Graphics& g, StyleSheet::Ptr ss, Rectangle<float> area, const StyleSheet::PropertyKey& key);

	static int getPseudoClassFromComponent(Component* c);

	void drawBackground(Graphics& g, Rectangle<float> area, StyleSheet::Ptr ss);

	Component& currentComponent;
};

} // namespace simple_css
} // namespace hise