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
PseudoState::PseudoState(PseudoClassType t, PseudoElementType e):
	stateFlag((int)t),
	element(e)
{}

PseudoState::PseudoState(int state):
	stateFlag(state),
	element(PseudoElementType::None)
{}

String PseudoState::getPseudoElementName(int idx)
{
	static const StringArray list({
		"none",
		"before",
		"after",
		"before2",
		"after2",
		"all"
	});

	if(isPositiveAndBelow(idx, list.size()))
		return list[idx];

	return "Unknown";
}

int PseudoState::getPseudoClassIndex(const String& pseudoStateName)
{
	int state = 0;
	
	if(pseudoStateName.contains(":first-child"))
		state |= (int)PseudoClassType::First;
	if(pseudoStateName.contains(":last-child"))
		state |= (int)PseudoClassType::Last;
	if(pseudoStateName.contains(":root"))
		state |= (int)PseudoClassType::Root;
	if(pseudoStateName.contains(":hover"))
		state |= (int)PseudoClassType::Hover;
	if(pseudoStateName.contains(":active"))
		state |= (int)PseudoClassType::Active;
	if(pseudoStateName.contains(":focus"))
		state |= (int)PseudoClassType::Focus;
	if(pseudoStateName.contains(":disabled"))
		state |= (int)PseudoClassType::Disabled;
	if(pseudoStateName.contains(":hidden"))
		state |= (int)PseudoClassType::Hidden;
	if(pseudoStateName.contains(":checked"))
		state |= (int)PseudoClassType::Checked;

	return state;
}

String PseudoState::getPseudoClassName(int state)
{
	String c;
        
	if((state & (int)PseudoClassType::First) > 0)
		c << ":first-child";
	if((state & (int)PseudoClassType::Last) > 0)
		c << ":last-child";
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
	if((state & (int)PseudoClassType::Empty) > 0)
		c << ":empty";

	return c;
}

Selector::Selector(ElementType dt)
{
	type = SelectorType::Type;

	switch(dt)
	{
	case ElementType::Body: name = "body"; break;
	case ElementType::Button: name = "button"; break;
	case ElementType::TextInput: name = "input"; break;
	case ElementType::Selector: name = "select"; break;
	case ElementType::Panel: name = "div"; break;
	case ElementType::Ruler: name = "hr"; break;
	case ElementType::Image: name = "img"; break;
	case ElementType::Table: name = "table"; break;
	case ElementType::Paragraph: name = "p"; break;
	case ElementType::TableHeader: name = "th"; break;
	case ElementType::TableRow: name = "tr"; break;
	case ElementType::TableCell: name = "td"; break;
	case ElementType::Progress: name = "progress"; break;
	case ElementType::Headline1: name = "h1"; break;
	case ElementType::Headline2: name = "h2"; break;
	case ElementType::Headline3: name = "h3"; break;
	case ElementType::Headline4: name = "h4"; break;
	case ElementType::Label: name = "label"; break;
	case ElementType::Scrollbar: name = "scrollbar"; break;
	default: ;
	}
}

Selector::Selector(const String& s)
{
	
	auto firstChar = s[0];

	switch(firstChar)
	{
	case ' ':
		type = SelectorType::ParentDelimiter;
		name = " ";
		break;
	case '.':
		type = SelectorType::Class;
		name = s.substring(1, 1000).trim();
		break;
	case '#':
		type = SelectorType::ID;
		name = s.substring(1, 1000).trim();
		break;
	case '@':
		type = SelectorType::AtRule;
		name = s.substring(1, 1000).trim();
		break;
	case '*':
		type = SelectorType::All;
		name = "*";
		break;
	default:
		type = SelectorType::Type;
		name = s;
		break;
	}

	jassert(name.isNotEmpty());
}

Selector::Selector(SelectorType t, String n):
	type(t),
	name(n)
{
	if(t == SelectorType::Class && n.startsWithChar('.'))
		name = name.substring(1);
	if(t == SelectorType::ID && n.startsWithChar('#'))
		name = name.substring(1);
}

String Selector::toString() const
{
	String s;

	switch(type)
	{
	case SelectorType::None: break;
	case SelectorType::Type: break;
	case SelectorType::ParentDelimiter: s << " "; break;
	case SelectorType::Class: s << '.'; break;
	case SelectorType::ID: s << '#'; break;
	case SelectorType::Element: s << "element(" << name << ")"; return s;
	case SelectorType::AtRule: s << '@'; break;
	case SelectorType::All: s << "*"; return s;
	default: ;
	}

	s << name;

	return s;
}

Selector::Match Selector::matchesRawList(const RawList& blockSelectors) const
{
	for(auto& bs: blockSelectors)
	{
		if(bs.first == *this)
			return { true, bs.second };
	}

	return { false, { PseudoClassType::None, PseudoElementType::None } };
}

bool Selector::operator==(const Selector& other) const
{
	if(type == SelectorType::All || other.type == SelectorType::All)
		return true;

	if(type == other.type)
	{
		return name == other.name;
	}

	return false;
}

ComplexSelector::Score::Score(ComplexSelector::Ptr cs, const Array<Selector>& selectors)
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

bool ComplexSelector::Score::operator<(const Score& other) const
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

bool ComplexSelector::Score::operator>(const Score& other) const
{
	if(*this == other)
		return false;

	return !(*this < other);
}

bool ComplexSelector::Score::operator==(const Score& other) const
{
	return idScore == other.idScore && classScore == other.classScore && typeScore == other.typeScore && hasParent == other.hasParent;
}

bool ComplexSelector::Score::operator!=(const Score& other) const
{
	return !(*this == other);
}

String ComplexSelector::Score::toString() const
{
	String s;
	s << "t: " << String(typeScore) << ", ";
	s << "c: " << String(classScore) << ", ";
	s << "i: " << String(typeScore);
	return s;
}

ComplexSelector::ComplexSelector(const std::vector<std::pair<Selector, PseudoState>>& rawList)
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

bool ComplexSelector::AndGroup::matchesSelectors(const Array<Selector>& searchTerm) const
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

String ComplexSelector::toString() const
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

Array<Selector> ComplexSelector::getSelectorsForComponent(Component* c)
{
	auto t = FlexboxComponent::Helpers::getTypeSelectorFromComponentClass(c);
	auto i = FlexboxComponent::Helpers::getIdSelectorFromComponentClass(c);
	auto list = FlexboxComponent::Helpers::getClassSelectorFromComponentClass(c);

	if(t)
		list.insert(0, t);

	if(i)
		list.add(i);
		
	return list;
}

bool ComplexSelector::matchesOtherComplexSelector(Ptr other) const
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

bool ComplexSelector::matchesSelectors(const Array<Selector> selectors, const Array<Selector>& pSelectors) const
{
	auto match = false;
	
	match = thisSelectors.matchesSelectors(selectors);

	if(hasParentSelectors())
		match &= parentSelectors.matchesSelectors(pSelectors);

	return match;
}

String Transition::toString() const
{
	String s;

	if(active)
	{
		s << ";\n  transition: %TR% ";
		s << String(duration, 2) << "s";

		if(delay != 0.0)
			s << " " << String(delay, 2) << "s";

		if(f)
			s << " " << fName;
	}

	return s;
}


PropertyKey::PropertyKey(const char* name_, int state_):
	name(name_),
	state(state_)
{}

PropertyKey::PropertyKey(const String& name_, PseudoState state_):
	name(name_),
	state(state_)
{}

PropertyKey PropertyKey::withSuffix(const String& suffix) const
{
	PropertyKey copy(*this);
	copy.appendSuffixIfNot(suffix);
	return copy;
}

bool PropertyKey::operator==(const PropertyKey& other) const
{
	return name == other.name && state == other.state;
}

bool PropertyKey::looseMatch(const String& other) const
{
	if(name == "all")
		return true;

	if(other == name)
		return true;

	static const std::map<String, StringArray> shorthandProperties =
	{
		{ "background", { "background-color", "background-image", "background-position", "background-size" } },
		{ "border", { "border-color", "border-style", "border-width",
						  "border-top-color", "border-right-color", "border-bottom-color", "border-left-color",
						  "border-top-style", "border-right-style", "border-bottom-style", "border-left-style",
						  "border-top-width", "border-right-width", "border-bottom-width", "border-left-width" } },
		{ "border-radius", { "border-top-left-radius", "border-top-right-radius",
							   "border-bottom-right-radius", "border-bottom-left-radius" } },
		{ "margin", { "margin-top", "margin-right", "margin-bottom", "margin-left" } },
		{ "padding", { "padding-top", "padding-right", "padding-bottom", "padding-left" } }
	};

	if(auto family = shorthandProperties.find(name); family != shorthandProperties.end())
		return family->second.contains(other);

	return false;
}

void PropertyKey::appendSuffixIfNot(const String& suffix)
{
	if(!name.endsWith(suffix))
		name << '-' << suffix;
}

PropertyValue::PropertyValue(PropertyType pt, const String& v, bool important_):
	type(pt),
	valueAsString(v),
    important(important_)
{}

PropertyValue::operator bool() const
{
	if(valueAsString == "default")
		return false;

	return valueAsString.isNotEmpty();
}

String PropertyValue::getValue(DynamicObject::Ptr variables)
{
	if(!valueAsString.contains("var(--"))
		return valueAsString;

	if(variables == nullptr)
		return {};

	auto current = valueAsString;
	StringArray resolvingVariables;

	std::function<String(const String&)> resolve = [&](const String& input)
	{
		String output;
		int cursor = 0;

		while(true)
		{
			auto variableStart = input.indexOf(cursor, "var(--");

			if(variableStart == -1)
				break;

			auto variableEnd = input.indexOf(variableStart, ")");

			if(variableEnd == -1)
				break;

			output << input.substring(cursor, variableStart);

			auto id = input.substring(variableStart + 6, variableEnd);
			auto variable = input.substring(variableStart, variableEnd + 1);

			if(resolvingVariables.contains(id))
			{
				output << variable;
			}
			else
			{
				resolvingVariables.add(id);
				output << resolve(variables->getProperty(Identifier(id)).toString());
				resolvingVariables.removeString(id);
			}

			cursor = variableEnd + 1;
		}

		output << input.substring(cursor);
		return output;
	};

	return resolve(current);
}

String PropertyValue::toString() const
{
	String s;

	if(valueAsString.isEmpty() || valueAsString.containsAnyOf("\t \n"))
		s << valueAsString.quoted('\'');
	else
		s << valueAsString;

	s << transition.toString();
	return s;
}

void PropertyValue::appendToValue(const String& s)
{
	valueAsString << s;
}

String Property::toString() const
{
    if(name != "all" && values.size() == 1 && values[0].second.toString() == "default")
        return {};
    
	String s;

	s << "  " << name;

	String intend;

	for(int i = 0; i < s.length(); i++)
		intend << " ";

	bool first = true;

	for(const auto& v: values)
	{
		if(!first)
			s << intend;

        auto pc = PseudoState::getPseudoClassName(v.first);
        
		s << "[" << pc << "]: " << v.second.toString() << "\n";
		first = false;
	}

	return s;
}

PropertyValue Property::getProperty(int stateFlag) const
{
	if(values.empty())
		return {};

	if(stateFlag == 0)
	{
		for(auto& pv: values)
		{
			if(pv.first == 0)
				return pv.second;
		}
	}
	
	for (auto it = values.rbegin(); it != values.rend(); ++it)
	{
		auto isCombinedFlag = !isPowerOfTwo(it->first);

		if(isCombinedFlag && stateFlag != it->first)
			continue;

		if((it->first & stateFlag) != 0)
		{
			return it->second;
		}
	}

	// We didn't find the property, so now let's check if
	// the default state is defined
	if(stateFlag != 0)
	{
		for(auto& pv: values)
		{
			if(pv.first == 0)
				return pv.second;
		}
	}

	// default state not defined, return null...
	return {};

	//return values[0].second;
}

NonUniformBorderData::NonUniformBorderData(Rectangle<float> totalArea_, float defaultWidth_,
	const ColourInfo& defaultColor_):
	defaultColour(defaultColor_),
	defaultWidth(defaultWidth_),
	totalArea(totalArea_)
{
	for(auto& s: data)
		s.second = defaultColour;

	data[topBorder].first =    Rectangle<float>(totalArea).removeFromTop(defaultWidth);
	data[leftBorder].first =   Rectangle<float>(totalArea).removeFromLeft(defaultWidth);
	data[rightBorder].first =  Rectangle<float>(totalArea).removeFromRight(defaultWidth);
	data[bottomBorder].first = Rectangle<float>(totalArea).removeFromBottom(defaultWidth);
}

NonUniformBorderData::Border NonUniformBorderData::getBorderFromProperty(String s)
{
	if(!s.startsWith("border-"))
		return NonUniformBorderData::numBorders;

	s = s.substring(7, 1000);

	if(s.startsWith("top"))
		return NonUniformBorderData::topBorder;
	if(s.startsWith("left"))
		return NonUniformBorderData::leftBorder;
	if(s.startsWith("right"))
		return NonUniformBorderData::rightBorder;
	if(s.startsWith("bottom"))
		return NonUniformBorderData::bottomBorder;
		
	return NonUniformBorderData::numBorders;
}

void NonUniformBorderData::setBorderSize(Border b, float newSize)
{
	auto& r = data[b].first;

	auto copy = totalArea;

	switch(b)
	{
	case topBorder:    r = copy.removeFromTop(newSize); break;
	case leftBorder:   r = copy.removeFromLeft(newSize); break;
	case bottomBorder: r = copy.removeFromBottom(newSize); break;
	case rightBorder:  r = copy.removeFromRight(newSize); break;
	case numBorders: break;
	default: ;
	}

	active |= std::abs(newSize - defaultWidth) > 0.001f;
}

void NonUniformBorderData::setBorderColour(Border b, const ColourInfo& c)
{
	auto prevColour = data[b].second;
	data[b].second = c;
	active |= prevColour != c;
}

void NonUniformBorderData::draw(Graphics& g)
{
	Border b = topBorder;

	for(const auto& d: data)
	{
		if(d.second.first.isTransparent() && d.second.second.getNumColours() == 0)
			continue;

		auto& r = d.first;
		Path p;
			
		switch(b)
		{
		case topBorder:
			p.startNewSubPath(r.getTopLeft());
			p.lineTo(r.getTopRight());
			p.lineTo(r.getRight() - r.getHeight(), r.getBottom());
			p.lineTo(r.getX() + r.getHeight(), r.getBottom());
			break;
		case leftBorder:
			p.startNewSubPath(r.getTopLeft());
			p.lineTo(r.getRight(), r.getY() + r.getWidth());
			p.lineTo(r.getRight(), r.getBottom() - r.getWidth());
			p.lineTo(r.getX(), r.getBottom());
			break;
		case bottomBorder:
			p.startNewSubPath(r.getBottomLeft());
			p.lineTo(r.getX() + r.getHeight(), r.getY());
			p.lineTo(r.getRight() - r.getHeight(), r.getY());
			p.lineTo(r.getRight(), r.getBottom());
			break;
		case rightBorder:
			p.startNewSubPath(r.getTopRight());
			p.lineTo(r.getBottomRight());
			p.lineTo(r.getX(), r.getBottom() - r.getWidth());
			p.lineTo(r.getX(), r.getY() + r.getWidth());
			break;
		case numBorders: break;
		default: ;
		}

		p.closeSubPath();

		if(d.second.second.getNumColours() > 0)
			g.setGradientFill(d.second.second);
		else
			g.setColour(d.second.first);

		g.fillPath(p);
		b = (Border)((int)b + 1);
	}
}
}

}


