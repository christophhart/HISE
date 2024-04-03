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
		"none ",
		"before",
		"after",
		"all"
	});

	if(isPositiveAndBelow(idx, list.size()))
		return list[idx];

	return "Unknown";
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
	default:
		type = SelectorType::Type;
		name = s;
		break;
	}
}

Selector::Selector(SelectorType t, String n):
	type(t),
	name(n)
{}

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

bool ComplexSelector::matchesComponent(Component* c) const
{
	auto clist = getSelectorsForComponent(c);

	Array<Selector> plist;

	auto p = c->getParentComponent();

	while(p != nullptr)
	{
		plist.addArray(getSelectorsForComponent(p));
		p = p->getParentComponent();
	}
	
	return matchesSelectors(clist, plist);
}

String Transition::toString() const
{
	String s;

	if(active)
	{
		s << " trans(";
		s << "dur:" << String(duration, 2) << "s, ";
		s << "del:" << String(duration, 2) << "s";

		if(f)
			s << ", f: true";

		s << ')';
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

	if(other.startsWith(name) || name.startsWith(other))
		return true;

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

String PropertyValue::toString() const
{
	String s;
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

	return values[0].second;
}

NonUniformBorderData::NonUniformBorderData(Rectangle<float> totalArea_, float defaultWidth_,
	const std::pair<Colour, ColourGradient>& defaultColor_):
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

void NonUniformBorderData::setBorderColour(Border b, const std::pair<Colour, ColourGradient>& c)
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


