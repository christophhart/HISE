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

/** Subclass any component from this class and it will act as a root for all CSS features.
 *
 *  Usually this is a top-level component, but you can use multiple instances for advanced use cases.
 *
 *	Child components with a StyleSheetLookAndFeel will then use this in order to fetch the StyleSheet object
 *	and the animator in order to render its graphics. */
struct CSSRootComponent
{
	struct InfoOverlay: public Component
	{
		struct Item
		{
			operator bool() const { return !selectors.isEmpty() && c.getComponent() != nullptr; }


			void draw(Graphics& g);

			Array<Selector> selectors;
			StyleSheet::Ptr ss;
			Rectangle<float> globalBounds;
			Rectangle<float> textBounds;
			Component::SafePointer<Component> c;
			Rectangle<float> textPosition;
		};

		InfoOverlay(CSSRootComponent& parent);

		void rebuild();

		void paint(Graphics& g) override
		{
			g.fillAll(Colour(0x99444444));

			for(auto i: items)
				i->draw(g);
		}

		CSSRootComponent& parent;
		OwnedArray<Item> items;
	};

	CSSRootComponent():
	  stateWatcher(this, animator)
	{
		css.setAnimator(&animator);
	};

	virtual ~CSSRootComponent() {};

	static CSSRootComponent* find(Component& c)
	{
		if(auto self = dynamic_cast<CSSRootComponent*>(&c))
			return self;

		return c.findParentComponentOfClass<CSSRootComponent>();
	}

	virtual StyleSheet::Collection::DataProvider* createDataProvider() { return nullptr; }

	void showInfo(bool shouldShow)
	{
		if(shouldShow)
			info = new InfoOverlay(*this);
		else
			info = nullptr;
	}

	
	Animator animator;
	StateWatcher stateWatcher;
	StyleSheet::Collection css;
	ScopedPointer<InfoOverlay> info;
};


/** This helper class will use a style sheet collection to slice an area in order to calculate the UI layout.
 *  The syntax of the methods mimic the juce::Rectangle<> class so you can apply the proven slicing workflow
 *	when defining your component layout.
 *
 *	Note: you can also use the FlexboxComponent for UI layout.
 */
class Positioner
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

	template <Direction D> Rectangle<float> slice(const Array<Selector>& s, float defaultValue)
	{
		if(auto ss = css.getWithAllStates(nullptr, s.getFirst()))
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

public:

	/** Creates a positioner for the given CSS collection and a given area (usually the component bounds).
	 *
	 *  If applyMargin is true, the rectangles returned by the methods will factor in the margin of the component,
	 *	however in most cases you don't want that to be the case because the margin is also factored in in the
	 *	Renderer::drawBackground() class (so it can draw box-shadows without resorting to unclipped painting).
	 */
	Positioner(StyleSheet::Collection styleSheet, Rectangle<float> totalArea_, bool applyMargin_=false);

	/** Creates a rectangle at origin with the width and sized set to the exact dimension to display the provided text.
	 *  This factors in font properties and margin / padding / borders so whenever you have a component that displays a text
	 *  use this to set it to the exact size that you know and love from your favorite web browser. */
	Rectangle<int> getLocalBoundsFromText(const Array<Selector>& s, const String& text, Rectangle<int> defaultBounds={});

	/** Slices a rectangle from the top of the full area using the style sheet identified by the list of supplied selectors. */
	Rectangle<float> removeFromTop(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Top>(s, defaultValue); }

	/** Slices a rectangle from the bottom of the full area using the style sheet identified by the list of supplied selectors. */
	Rectangle<float> removeFromBottom(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Bottom>(s, defaultValue); }

	/** Slices a rectangle from the left of the full area using the style sheet identified by the list of supplied selectors. */
	Rectangle<float> removeFromLeft(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Left>(s, defaultValue); }

	/** Slices a rectangle from the right of the full area using the style sheet identified by the list of supplied selectors. */
	Rectangle<float> removeFromRight(const Array<Selector>& s, float defaultValue = 0.0f) { return slice<Direction::Right>(s, defaultValue); } 

private:

	bool applyMargin = false;
	Rectangle<float> bodyArea;
	Rectangle<float> totalArea;
	StyleSheet::Collection css;
};


/** A lightweight object that will render CSS defined appearances on a Graphics context. */
struct Renderer: public Animator::ScopedComponentSetter
{
	/** Creates a renderer that will draw on the component using the state watcher. */
	Renderer(Component* c, StateWatcher& state_);;

	/** Tries to set the flags based on the component state (visible, enabled, hovered, etc). */
	static int getPseudoClassFromComponent(Component* c);

	/** Renders the background of the component using the supplied style sheet and pseudo element type.
	 *
	 *  Make sure to call StateWatcher::checkState() before rendering this method in order to pick up the correct pseudo class
	 *	to use.
	 */
	void drawBackground(Graphics& g, Rectangle<float> area, StyleSheet::Ptr ss, PseudoElementType type = PseudoElementType::None);

	void drawImage(Graphics& g, const juce::Image& img, Rectangle<float> area, StyleSheet::Ptr ss, bool isContent);

	/** Renders a text using the supplied style sheet. */
	void renderText(Graphics& g, Rectangle<float> area, const String& text, StyleSheet::Ptr ss, PseudoElementType type=PseudoElementType::None, Justification justificationToUse = Justification(0));

	/** Manually set the state flags for the renderer. this is useful for cases where the style flags can't be easily queried
	 *  from the component hover states (eg. at popup menu items). */
	void setPseudoClassState(int state, bool forceOverwrite=false);

	/** Sets the current colour (or gradient) for the renderer based on the supplied style sheet and property key. */
	void setCurrentBrush(Graphics& g, StyleSheet::Ptr ss, Rectangle<float> area, const PropertyKey& key, Colour defaultColour=Colours::transparentBlack);

	/** returns the pseudo class state to use */
	int getPseudoClassState() const;

	void setApplyMargin(bool useMargin)
	{
		applyMargin = useMargin;
	}

private:

	bool applyMargin = true;

	bool forceOverwriteState = false;
	int pseudoClassState = 0;
	PseudoElementType currentlyRenderedPseudoElement = PseudoElementType::None;
	Component* currentComponent;
	StateWatcher& state;
};

/** This class will create a set of C++ instructions that render the style sheet on a juce::Graphics context.
 *
 *  This makes it a very convenient tool during development: just use a style sheet to customize the appearance and then
 *	use this class to create a hardcoded C++ renderer with the same appearance but no performance overhead.
 *
 *	The generated code can be used as a dropin-replacement for the simple_css::Renderer class.
 */
class CodeGenerator
{
public:

	CodeGenerator(StyleSheet::Ptr ss_);

	void appendLine(const String& s)
	{
		if(s.isNotEmpty())
			code << "\t" << s << "\n";
	}

	String toCode()
	{
		return code;
	}

	String code;
	
	StyleSheet::Ptr ss;
};

} // namespace simple_css
} // namespace hise