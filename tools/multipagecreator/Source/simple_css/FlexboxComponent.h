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

/** A container component that applies flex box positioning to its children.
 */
struct FlexboxComponent: public Component
{
	struct Helpers
	{
		static void setFallbackStyleSheet(Component& c, const String& properties);
		static void writeSelectorsToProperties(Component& c, const StringArray& selectors);
		static Selector getTypeSelectorFromComponentClass(Component* c);
		static Array<Selector> getClassSelectorFromComponentClass(Component* c);
		static Selector getIdSelectorFromComponentClass(Component* c);
		static StyleSheet::Ptr getDefaultStyleSheetFunction(StyleSheet::Collection& css, Component* c);
	};

	using ChildStyleSheetFunction = std::function<StyleSheet::Ptr(StyleSheet::Collection&, Component*)>;

	/** Create a flexbox component with a selector (usually a ID selector or either div or body). */
	FlexboxComponent(const Selector& s);

	/** Set a CSS collection. This will apply the stylesheet to all child components. You can supply a custom
	 *  function that will create a default style sheet (or if you don't it will use the Component properties `style` value
	 *	to make a individual element style CSS. */
	void setCSS(StyleSheet::Collection& css, const ChildStyleSheetFunction& f=Helpers::getDefaultStyleSheetFunction);

	/** Writes a list of semicolon separated properties into the component properties that will be used as default style sheet. */
	void setDefaultStyleSheet(const String& css);

	/** Draws the background using the style sheet supplied with setCSS(). */
	void paint(Graphics& g) override;

	/** Applies the position using the JUCE::Flexbox class. */
	void resized() override;

	/** Adds a invisible component as child that will act as spacer. */
	void addSpacer();

private:

	OwnedArray<Component> spacers;

	juce::FlexBox flexBox;
	Selector selector;
	StyleSheet::Ptr ss;

	std::map<Component*, StyleSheet::Ptr> childSheets;
};


} // namespace simple_css
} // namespace hise
