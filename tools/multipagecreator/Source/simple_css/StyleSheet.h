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

struct Animator;

struct StyleSheet: public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<StyleSheet>;
	using List = ReferenceCountedArray<StyleSheet>;

	

	

	struct Collection
	{
		Collection() = default;

		explicit Collection(List l);

		operator bool() const { return !list.isEmpty(); }

		void setAnimator(Animator* a);
		Ptr operator[](const Selector& s) const;

		Ptr operator[](const Array<Selector>& s) const
		{
			for(auto& l: list)
			{
				if(l->matchesSelectorList(s))
					return l;
			}

			return nullptr;
		}

		String toString() const;

		Ptr findBestMatch(const Array<Selector>& selectors) const;

		Ptr getOrCreateCascadedStyleSheet(const Array<Selector>& selectors);

		void addComponentToSetup(Component* c);

		void addElementStyle(Ptr p)
		{
			auto elementSelector = p->selectors.getFirst();
			jassert(elementSelector.type == SelectorType::Element);

			for(auto p: list)
			{
				for(auto& s: p->selectors)
					if(s == elementSelector)
						return;
			}

			list.add(p);
		}

	private:
		
		List list;
	};

	StyleSheet(const Array<Selector>& selectors_)
	{
		selectors.addArray(selectors_);
	}

	StyleSheet(const Selector& s)
	{
		selectors.add(s);
	};

	TransitionValue getTransitionValue(const PropertyKey& key) const;
	PropertyValue getPropertyValue(const PropertyKey& key) const;

	void copyPropertiesFrom(Ptr other);

	NonUniformBorderData getNonUniformBorder(Rectangle<float> totalArea, PseudoState stateFlag) const;
	Path getBorderPath(Rectangle<float> totalArea, PseudoState stateFlag) const;
	float getPixelValue(Rectangle<float> totalArea, const PropertyKey& key, float defaultValue=0.0f) const;
	Rectangle<float> getArea(Rectangle<float> totalArea, const PropertyKey& key) const;

	void setFullArea(Rectangle<float> fullArea);

	Rectangle<float> expandArea(Rectangle<float> sourceArea, const PropertyKey& key) const;
	Rectangle<float> getBounds(Rectangle<float> sourceArea, PseudoState state) const;

	Rectangle<float> getPseudoArea(Rectangle<float> sourceArea, int currentState, PseudoElementType area) const;
	Rectangle<float> truncateBeforeAndAfter(Rectangle<float> sourceArea, int currentState) const;

	void setupComponent(Component* c, int currentState);

	Justification getJustification(int currentState, int defaultXFlag=Justification::horizontallyCentred, int defaultYFlag=Justification::verticallyCentred) const;
    float getOpacity(int state) const;
	MouseCursor getMouseCursor() const;
	String getText(const String& t, int currentState) const;
	Font getFont(int currentState, Rectangle<float> totalArea) const;
	AffineTransform getTransform(Rectangle<float> totalArea, PseudoState currentState) const;
	std::vector<melatonin::ShadowParameters> getShadow(Rectangle<float> totalArea, const PropertyKey& key, bool wantsInset) const;
	std::pair<Colour, ColourGradient> getColourOrGradient(Rectangle<float> area, PropertyKey key, Colour defaultColour=Colours::transparentBlack) const;
	PositionType getPositionType(PseudoState state) const;

	FlexBox getFlexBox() const;

	FlexItem getFlexItem(Component* c, Rectangle<float> fullArea) const;

	Rectangle<float> getLocalBoundsFromText(const String& text) const;

	std::pair<bool, PseudoState> matchesRawList(const Selector::RawList& blockSelectors) const;
	String toString() const;

	void setDefaultTransition(PseudoElementType elementType, const Transition& t)
	{
		defaultTransitions[(int)elementType] = t;
	}

	Transition getTransitionOrDefault(PseudoElementType elementType, const Transition& t) const;

	bool matchesSelectorList(const Array<Selector>& otherSelectors);
	bool forEachProperty(PseudoElementType type, const std::function<bool(PseudoElementType, Property& v)>& f);
	void setDefaultColour(const String& key, Colour c);

	template <typename EnumType> EnumType getAsEnum(const PropertyKey& key, const StringArray& items, EnumType defaultValue) const
	{
		if(auto pv = getPropertyValue(key))
		{
			auto idx = items.indexOf(pv.valueAsString);
			if(idx != -1)
				return static_cast<EnumType>(idx);
		}

		return defaultValue;
	}

private:

	Rectangle<float> currentFullArea;
	std::map<String, Colour> defaultColours;
	float defaultFontSize = 16.0f;
	friend class Parser;

	Array<Selector> selectors;
	std::array<std::vector<Property>, (int)PseudoElementType::All> properties;
	std::array<Transition, (int)PseudoElementType::All> defaultTransitions;
	Animator* animator = nullptr;
};


} // namespace simple_css
} // namespace hise
