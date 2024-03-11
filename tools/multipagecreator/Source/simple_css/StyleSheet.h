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


struct Selector
{
	using RawList = std::vector<std::pair<Selector, int>>;

	Selector() = default;

	explicit Selector(ElementType dt);

	Selector(SelectorType t, String n):
	  type(t),
	  name(n)
	{};

	String toString() const;

	std::pair<bool, int> matchesRawList(const RawList& blockSelectors) const;

	bool operator==(const Selector& other) const;

	SelectorType type;
	String name;
};

struct Transition
{
	String toString() const;

	operator bool() const { return active && (duration > 0.0 || delay > 0.0); }

	bool active = false;
	double duration = 0.0;
	double delay = 0.0;
	std::function<double(double)> f;
};

struct StyleSheet: public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<StyleSheet>;
	using List = ReferenceCountedArray<StyleSheet>;

	struct PropertyKey
	{
		PropertyKey withSuffix(const String& suffix) const;

		bool operator==(const PropertyKey& other) const;
		bool looseMatch(const String& other) const;
		void appendSuffixIfNot(const String& suffix);

		String name;
		int state;
	};

	struct TransitionValue
	{
		operator bool() const { return active; }

		bool active = false;
		String startValue;
		String endValue;
		double progress = 0.0;
	};

	struct PropertyValue
	{
		PropertyValue() = default;
		PropertyValue(PropertyType pt, const String& v);;

		operator bool() const
		{
			if(valueAsString == "default")
				return false;

			return valueAsString.isNotEmpty();
		}

		String toString() const;

		PropertyType type;
		Transition transition;
		String valueAsString;
	};

	struct Property
	{
		String toString() const;
		PropertyValue getProperty(int stateFlag) const;

		String name;
		std::map<int, PropertyValue> values;
	};

	

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

		Ptr findBestMatch(const Array<Selector>& selectors) const
		{
			if(auto ss = (*this)[selectors])
				return ss;

			for(auto s: selectors)
			{
				if(auto ss = (*this)[s])
					return ss;
			}

			return nullptr;
		}

		Ptr getOrCreateCascadedStyleSheet(const Array<Selector>& selectors);

		void addComponentToSetup(Component* c);
		
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

	Path getBorderPath(Rectangle<float> totalArea, int stateFlag) const;
	float getPixelValue(Rectangle<float> totalArea, const PropertyKey& key, float defaultValue=0.0f) const;
	Rectangle<float> getArea(Rectangle<float> totalArea, const PropertyKey& key) const;

	void setupComponent(Component* c, int currentState);

	Justification getJustification(int currentState) const;

    float getOpacity(int state) const;

	MouseCursor getMouseCursor() const
	{
		if(auto mv = getPropertyValue({ "cursor", 0}))
		{
			std::map<String, MouseCursor::StandardCursorType> cursors = {
				{ "default", MouseCursor::NormalCursor },
				{ "pointer", MouseCursor::PointingHandCursor },
				{ "wait", MouseCursor::WaitCursor },
				{ "crosshair", MouseCursor::CrosshairCursor },
				{ "text", MouseCursor::IBeamCursor },
				{ "copy", MouseCursor::CopyingCursor },
				{ "grabbing", MouseCursor::DraggingHandCursor }
			};

			auto c = cursors.find(mv.valueAsString);

			if(c != cursors.end())
				return MouseCursor(c->second);
			
		}

		return MouseCursor();
	}

	String getText(const String& t, int currentState) const
	{
		if(auto v = getPropertyValue({ "text-transform", currentState}))
		{
			if(v.valueAsString == "uppercase")
				return t.toUpperCase();

			if(v.valueAsString == "lowercase")
				return t.toLowerCase();

			if(v.valueAsString == "capitalize")
				return t;
		}

		return t;
	}

	Font getFont(int currentState, Rectangle<float> totalArea) const;

	AffineTransform getTransform(Rectangle<float> totalArea, int currentState) const;

	std::vector<melatonin::ShadowParameters> getBoxShadow(Rectangle<float> totalArea, int currentState, bool wantsInset) const;

	std::pair<Colour, ColourGradient> getColourOrGradient(Rectangle<float> area, PropertyKey key, Colour defaultColour=Colours::transparentBlack);

	std::pair<bool, int> matchesRawList(const Selector::RawList& blockSelectors) const;

	String toString() const;

	bool matchesSelectorList(const Array<Selector>& otherSelectors)
	{
		if(selectors.size() != otherSelectors.size())
			return false;

		for(int i = 0; i < otherSelectors.size(); i++)
		{
			if(!(otherSelectors[i] == selectors[i]))
				return false;
		}

		return true;
	}

	const Property* begin() const { return properties.data(); }
	const Property* end() const { return properties.data() + properties.size(); }

	void setDefaultColour(const String& key, Colour c)
	{
		defaultColours[key] = c;
	}

private:

	std::map<String, Colour> defaultColours;

	float defaultFontSize = 16.0f;

	friend class Parser;

	Array<Selector> selectors;
	std::vector<Property> properties;
	
	Animator* animator = nullptr;
};


} // namespace simple_css
} // namespace hise
