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

/** A combined object for a pseudo class (aka state) and a pseudo element (eg. ::before). */
struct PseudoState
{
	PseudoState(PseudoClassType t, PseudoElementType e=PseudoElementType::None);;
	PseudoState() = default;
	explicit PseudoState(int state);;

	static String getPseudoElementName(int idx);

    static String getPseudoClassName(int state)
    {
        String c;
        
        if((state & (int)PseudoClassType::First) > 0)
            c << ":first";
        if((state & (int)PseudoClassType::Last) > 0)
            c << ":last";
        if((state & (int)PseudoClassType::Root) > 0)
            c << ":root";
        if((state & (int)PseudoClassType::Hover) > 0)
            c << ":hover";
        if((state & (int)PseudoClassType::Active) > 0)
            c << ":active";
        if((state & (int)PseudoClassType::Focus) > 0)
            c << ":focus";
        if((state & (int)PseudoClassType::Disabled) > 0)
            c << ":disabled";
        if((state & (int)PseudoClassType::Hidden) > 0)
            c << ":hidden";
        if((state & (int)PseudoClassType::Checked) > 0)
            c << ":checked";

        return c;
    }
    
	bool isPseudoElement() const { return element != PseudoElementType::None; }
	bool hasState() const { return stateFlag != 0; };
	bool isDefaultState() const { return stateFlag == 0; }

    
	bool matchesElement(const PseudoState& other) const { return element == other.element; }
	bool matchesElement(PseudoElementType otherType) const { return element == otherType; }

	PseudoState withElement(PseudoElementType t) const { auto copy = *this; copy.element = t; return copy; }
	PseudoState withAdditionalState(PseudoClassType t) const { auto copy = *this; copy.stateFlag |= (int)t; return copy; }

	bool operator==(const PseudoState& other) const { return stateFlag == other.stateFlag && element == other.element; }
	bool operator!=(const PseudoState& other) const { return !(*this == other); }

	int stateFlag = 0;
	PseudoElementType element = PseudoElementType::None;
};

/** This class is used to select the style sheet that should be applied. */
struct Selector
{
	using RawList = std::vector<std::pair<Selector, PseudoState>>;
	using Match = std::pair<bool, PseudoState>;

	Selector() = default;
	Selector(ElementType dt);

	explicit Selector(const String& s);

	static Selector withClass(const String& s) { return Selector(SelectorType::Class, s); }

	Selector(SelectorType t, String n);;

	bool exactMatch(const Selector& other) const
	{
		return other.type == type && other.name == name;
	}

	String toString() const;
	Match matchesRawList(const RawList& blockSelectors) const;
	bool operator==(const Selector& other) const;
	bool operator!=(const Selector& other) const { return !(*this == other); }
	operator bool() const { return type != SelectorType::None; };

	SelectorType type = SelectorType::None;
	String name;
};

struct ComplexSelector: public ReferenceCountedObject
{
	using Ptr = ReferenceCountedObjectPtr<ComplexSelector>;
	using List = ReferenceCountedArray<ComplexSelector>;

	/** Calculating a matching score according to https://developer.mozilla.org/en-US/docs/Web/CSS/Specificity */
	struct Score
	{
		Score() = default;

		Score(ComplexSelector::Ptr cs, const Array<Selector>& selectors)
		{
			hasParent = cs->hasParentSelectors();

			for(const auto& s: selectors)
			{
				if(s.type == SelectorType::ID)
				{
					for(const auto& ts: cs->thisSelectors.selectors)
					{
						if(ts.first.type == SelectorType::ID)
						{
							if(ts.first.name == s.name)
								idScore++;

							break;
						}
					}
				}
				if(s.type == SelectorType::Type)
				{
					for(const auto& ts: cs->thisSelectors.selectors)
					{
						if(ts.first.type == SelectorType::Type)
						{
							if(ts.first.name == s.name)
								typeScore++;

							break;
						}
					}
				}
				if(s.type == SelectorType::Class)
				{
					for(const auto& ts: cs->thisSelectors.selectors)
					{
						if(ts.first.type == SelectorType::Class)
						{
							if(ts.first.name == s.name)
							{
								classScore++;
								// allow multiple, do not break if no match...
								break;
							}
						}
					}
				}
			}
		}

		bool operator<(const Score& other) const
		{
			if(hasParent && !other.hasParent)
				return false;
			if(other.hasParent && !hasParent)
				return true;

			if(*this == other)
				return false;

			if(idScore < other.idScore)
				return true;
			if(idScore > other.idScore)
				return false;

			if(classScore < other.classScore)
				return true;
			if(classScore > other.classScore)
				return false;

			if(typeScore < other.typeScore)
				return true;
			if(typeScore > other.typeScore)
				return false;
			
			return false;
		}

		bool operator>(const Score& other) const
		{
			if(*this == other)
				return false;

			return !(*this < other);
		}

		bool operator==(const Score& other) const
		{
			return idScore == other.idScore && classScore == other.classScore && typeScore == other.typeScore && hasParent == other.hasParent;
		}

		bool operator !=(const Score& other) const
		{
			return !(*this == other);
		}

		String toString() const
		{
			String s;
			s << "t: " << String(typeScore) << ", ";
			s << "c: " << String(classScore) << ", ";
			s << "i: " << String(typeScore);
			return s;
		}

		int idScore = 0;
		int classScore = 0;
		int typeScore = 0;

		bool hasParent = false;
	};

	ComplexSelector(const Selector& singleSelector, const PseudoState& state = {})
	{
		thisSelectors.selectors.push_back({singleSelector, state});
	}

	ComplexSelector(const std::vector<std::pair<Selector, PseudoState>>& rawList)
	{
		AndGroup currentGroup;

		bool alreadySwapped = false;

		int index = 0;

		for(const auto& r: rawList)
		{
			auto isLast = rawList.size() == ++index;

			if(r.first.type == SelectorType::ParentDelimiter)
			{
				if(alreadySwapped || isLast)
					continue;

				thisSelectors = currentGroup;
				std::swap(thisSelectors, parentSelectors);
				currentGroup = {};
				alreadySwapped = true;
				continue;
			}

			currentGroup.selectors.push_back(r);
		}

		thisSelectors = currentGroup;
	}

	struct AndGroup
	{
		bool matchesSelectors(const Array<Selector>& searchTerm) const
		{
			if(isSingle())
			{
				auto fc = selectors[0].first;
				return fc.type == SelectorType::All || searchTerm.contains(fc);
			}
			else
			{
				auto match = true;

				for(const auto& f: selectors)
					match &= searchTerm.contains(f.first);

				return match;
			}
		}



		bool isSingle() const { return selectors.size() == 1; };
		std::vector<std::pair<Selector, PseudoState>> selectors;
	};

	String toString() const
	{
		String s;

		for(auto& p: parentSelectors.selectors)
			s << p.first.toString();

		if(hasParentSelectors())
			s << " ";

		for(auto& p: thisSelectors.selectors)
			s << p.first.toString();

		return s;
	}

	bool hasParentSelectors() const { return !parentSelectors.selectors.empty(); }
	
	static Array<Selector> getSelectorsForComponent(Component* c);

	bool matchesOtherComplexSelector(Ptr other) const
	{
		auto match = parentSelectors.selectors.size() == other->parentSelectors.selectors.size();
		match &= thisSelectors.selectors.size() == other->thisSelectors.selectors.size();

		if(!match)
			return false;

		for(int i = 0; i < thisSelectors.selectors.size(); i++)
		{
			match &= thisSelectors.selectors[i].first.exactMatch(other->thisSelectors.selectors[i].first);
			match &= thisSelectors.selectors[i].second.element == other->thisSelectors.selectors[i].second.element;
		}
			

		for(int i = 0; i < parentSelectors.selectors.size(); i++)
		{
			match &= parentSelectors.selectors[i].first.exactMatch(other->parentSelectors.selectors[i].first);
			match &= parentSelectors.selectors[i].second.element == other->parentSelectors.selectors[i].second.element;
		}
			

		return match;
	}

	bool matchesSelectors(const Array<Selector> selectors, const Array<Selector>& pSelectors = {}) const
	{
		auto match = false;
	
		match = thisSelectors.matchesSelectors(selectors);

		if(hasParentSelectors())
			match &= parentSelectors.matchesSelectors(pSelectors);

		return match;
	}

	AndGroup parentSelectors;
	AndGroup thisSelectors;
};

/** This object holds a currently animated property with a start / end value and the progress. */
struct Transition
{
	String toString() const;

	operator bool() const { return active && (duration > 0.0 || delay > 0.0); }

	bool active = false;
	double duration = 0.0;
	double delay = 0.0;
	std::function<double(double)> f;
};

/** This is a ID for querying style sheets for its properties. */
struct PropertyKey
{
	PropertyKey(const char* name_, int state_);;

	PropertyKey(const String& name_, PseudoState state_);;

	PropertyKey() = default;
	  
	PropertyKey withSuffix(const String& suffix) const;

	bool operator==(const PropertyKey& other) const;
	bool looseMatch(const String& other) const;
	void appendSuffixIfNot(const String& suffix);

	String name;
	PseudoState state;
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
	PropertyValue(PropertyType pt, const String& v, bool isImportant=false);;

	operator bool() const
	{
		if(valueAsString == "default")
			return false;

		return valueAsString.isNotEmpty();
	}

	String getValue(DynamicObject::Ptr variables)
	{
		if(valueAsString.startsWith("var("))
		{
			if(variables != nullptr)
			{
				Identifier id(valueAsString.substring(6, valueAsString.length() - 1));
				return variables->getProperty(id).toString();
			}
			else
				return {};
		}
		else
		{
			return valueAsString;
		}
	}

	String toString() const;

	PropertyType type;
	Transition transition;
    bool important = false;

	void appendToValue(const String& s);

private:

	String valueAsString;
};

struct Property
{
	String toString() const;
	PropertyValue getProperty(int stateFlag) const;

	String name;
	std::vector<std::pair<int, PropertyValue>> values;
};

/** A helper class that renders non-uniform borders (borders that are not the same at every edge). */
struct NonUniformBorderData
{
	enum Border
	{
		topBorder,
		leftBorder,
		bottomBorder,
		rightBorder,
		numBorders
	};

	NonUniformBorderData() = default;
	NonUniformBorderData(Rectangle<float> totalArea_, float defaultWidth_, const std::pair<Colour, ColourGradient>& defaultColor_);

	static Border getBorderFromProperty(String s);

	operator bool() const noexcept { return active; }

	void setBorderSize(Border b, float newSize);
	void setBorderColour(Border b, const std::pair<Colour, ColourGradient>& c);
	void draw(Graphics& g);

	std::array<std::pair<Rectangle<float>, std::pair<Colour, ColourGradient>>, numBorders> data;
	Rectangle<float> totalArea;
	float defaultWidth = 0.0f;
	std::pair<Colour, ColourGradient> defaultColour = { Colours::transparentBlack, ColourGradient() };
	bool active = false;
};


} // namespace simple_css
} // namespace hise
