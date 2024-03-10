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
	default: ;
	}
}

String Selector::toString() const
{
	String s;

	switch(type)
	{
	case SelectorType::None: break;
	case SelectorType::Type: break;
	case SelectorType::Class: s << '.'; break;
	case SelectorType::ID: s << '#'; break;
	case SelectorType::All: s << '*'; break;
	default: ;
	}

	s << name;

	return s;
}

std::pair<bool, int> Selector::matchesRawList(const RawList& blockSelectors) const
{
	for(auto& bs: blockSelectors)
	{
		if(bs.first == *this)
			return { true, bs.second };
	}

	return { false, (int)PseudoClassType::None };
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

String Transition::toString() const
{
	String s;

	if(active)
	{
		s << " tr(";
		s << "dur:" << String(duration, 2) << "s, ";
		s << "del:" << String(duration, 2) << "s";

		if(f)
			s << ", f: true";

		s << ')';
	}

	return s;
}

StyleSheet::PropertyKey StyleSheet::PropertyKey::withSuffix(const String& suffix) const
{
	PropertyKey copy(*this);
	copy.appendSuffixIfNot(suffix);
	return copy;
}

bool StyleSheet::PropertyKey::operator==(const PropertyKey& other) const
{
	return name == other.name && state == other.state;
}

bool StyleSheet::PropertyKey::looseMatch(const String& other) const
{
	if(name == "all")
		return true;

	if(other == name)
		return true;

	if(other.startsWith(name) || name.startsWith(other))
		return true;

	return false;
}

void StyleSheet::PropertyKey::appendSuffixIfNot(const String& suffix)
{
	if(!name.endsWith(suffix))
		name << '-' << suffix;
}

StyleSheet::PropertyValue::PropertyValue(PropertyType pt, const String& v):
	type(pt),
	valueAsString(v)
{}

String StyleSheet::PropertyValue::toString() const
{
	String s;
	s << valueAsString;
	s << transition.toString();
	return s;
}

String StyleSheet::Property::toString() const
{
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

		s << "[" << String((int)v.first) << "]: " << v.second.toString() << "\n";
		first = false;
	}

	return s;
}

StyleSheet::PropertyValue StyleSheet::Property::getProperty(int stateFlag) const
{
	for(const auto& v: values)
	{
		if(v.first == stateFlag)
			return v.second;
	}

	int highestMatch = 0;
	int bestMatch = 0;

	// check one overlap
	for(const auto& v: values)
	{
		auto thisMatch = v.first & stateFlag;

		// if it's only hovering don't use a combined best match...
		if(stateFlag == (int)PseudoClassType::Hover)
		{
			continue;
		}

		if(thisMatch > highestMatch)
		{
			highestMatch = thisMatch;
			bestMatch = v.first;
		}
	}

	 return values.at(bestMatch);
}

StyleSheet::TransitionValue StyleSheet::getTransitionValue(const PropertyKey& key) const
{
	TransitionValue tv;

	if(animator == nullptr)
		return tv;

	for(const auto i: animator->items)
	{
		if(i->target != animator->currentlyRenderedComponent)
			continue;

		if(i->css != this)
			continue;

		if(i->startValue.name == key.name)
		{
			tv.startValue = getPropertyValue(i->startValue).valueAsString;
			tv.endValue = getPropertyValue(i->endValue).valueAsString;
			tv.progress = i->currentProgress;

			if(i->transitionData.f)
				tv.progress = i->transitionData.f(tv.progress);

			tv.active = true;
			break;
		}
	}

	return tv;
}

StyleSheet::PropertyValue StyleSheet::getPropertyValue(const PropertyKey& key) const
{
	for(const auto& p: properties)
	{
		if(p.name == key.name)
			return p.getProperty(key.state);
	}

	return {};
}

StyleSheet::Collection::Collection(List l):
	list(l)
{
	// TODO: sort so that it takes the best match first
}

void StyleSheet::Collection::setAnimator(Animator* a)
{
	for(auto l: list)
		l->animator = a;
}

StyleSheet::Ptr StyleSheet::Collection::operator[](const Selector& s) const
{
	for(auto l: list)
	{
		auto item = std::find(l->selectors.begin(), l->selectors.end(), s);
		if(item != l->selectors.end())
			return l;
	}

	return nullptr;
}

String StyleSheet::Collection::toString() const
{
	String listContent;

	for(auto cs : list)
	{
		listContent << cs->toString();
	}

	return listContent;
}

Path StyleSheet::getBorderPath(Rectangle<float> totalArea, int stateFlag) const
{
	float corners[4];
		
	corners[0] = getPixelValue(totalArea, {"border-top-left-radius", stateFlag});
	corners[1] = getPixelValue(totalArea, {"border-top-right-radius", stateFlag});
	corners[2] = getPixelValue(totalArea, {"border-bottom-left-radius", stateFlag});
	corners[3] = getPixelValue(totalArea, {"border-bottom-right-radius", stateFlag});

	float empty[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	Path p;

	

	if(memcmp(corners, empty, sizeof(float) * 4) == 0)
	{
		p.addRectangle(totalArea);
	}
	else if (corners[0] == corners[1] && corners[0] == corners[2] && corners[0] == corners[3])
	{
		p.addRoundedRectangle(totalArea, corners[0]);
	}
	else
	{
		auto cornerSize = 0.0f;
		bool borders[4];

		for(int i = 0; i < 4; i++)
		{
			cornerSize = jmax(corners[i], cornerSize);
			borders[i] = corners[i] > 0.0f;
		}

		p.addRoundedRectangle(totalArea.getX(), totalArea.getY(), totalArea.getWidth(), totalArea.getHeight(), cornerSize, cornerSize, borders[0], borders[1], borders[2], borders[3]);

#if 0
		float csx, csy, csx45, csy45, x, y, cex, cey;

		{
			x = totalArea.getX();
			y = totalArea.getY();
			csx = x;
			csy = totalArea.getY() + corners[0];
			cex = totalArea.getX() + corners[0];
			cey = totalArea.getY();

			p.startNewSubPath(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		{
			x = totalArea.getRight();
			y = totalArea.getY();

			csx = x - corners[1];
			csy = y;
			cex = x;
			cey = y + corners[1];

			p.lineTo(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		{
			x = totalArea.getRight();
			y = totalArea.getBottom();
			csx = x;
			csy = y - corners[3];
			cex = x - corners[3];
			cey = y;

			p.lineTo(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		{
			x = totalArea.getX();
			y = totalArea.getBottom();
			csx = x + corners[2];
			csy = y;
			cex = x;
			cey = y - corners[2];

			p.lineTo(csx, csy);
			p.quadraticTo(x, y, cex, cey);
		}

		p.closeSubPath();
#endif
	}

	return p;
}

float StyleSheet::getPixelValue(Rectangle<float> totalArea, const PropertyKey& key, float defaultValue) const
{
	auto getValueFromString = [&](const String& v)
	{
		if(v.endsWithChar('x'))
			return (float)v.getIntValue();

		auto useHeight = key.name.contains("top") || key.name.contains("bottom");

		if(v == "auto")
			return useHeight ? totalArea.getHeight() : totalArea.getWidth();
		if(v.endsWithChar('%'))
			return totalArea.getHeight() * (float)v.getIntValue() * 0.01f;
	};

	if(auto tv = getTransitionValue(key))
	{
		auto v1 = getValueFromString(tv.startValue);
		auto v2 = getValueFromString(tv.endValue);
		return Interpolator::interpolateLinear(v1, v2, (float)tv.progress);
	}
	if(auto v = getPropertyValue(key))
	{
		return getValueFromString(v.valueAsString);
	}
	
	return defaultValue;
}

Rectangle<float> StyleSheet::getArea(Rectangle<float> totalArea, const PropertyKey& key) const
{
	auto oa = totalArea;
	totalArea.removeFromLeft(getPixelValue(oa, key.withSuffix("left")));
	totalArea.removeFromTop(getPixelValue(oa, key.withSuffix("top")));
	totalArea.removeFromBottom(getPixelValue(oa, key.withSuffix("bottom")));
	totalArea.removeFromRight(getPixelValue(oa, key.withSuffix("right")));

	return totalArea;
}

std::vector<melatonin::ShadowParameters> StyleSheet::getBoxShadow(Rectangle<float> totalArea,
	int currentState, bool wantsInset) const
{
	if(auto tv = getTransitionValue({ "box-shadow", currentState}))
	{
		ShadowParser s1(tv.startValue, totalArea);
		ShadowParser s2(tv.endValue, totalArea);
		return s1.interpolate(s2, tv.progress, wantsInset);
	}
	else if(auto v = getPropertyValue({ "box-shadow", currentState}))
	{
		ShadowParser s(v.valueAsString, totalArea);
		return s.getShadowParameters(wantsInset);
	}

	return {};
}

AffineTransform StyleSheet::getTransform(Rectangle<float> totalArea, int currentState) const
{
	if(auto pv = getPropertyValue({ "transform", currentState}))
	{
		TransformParser p(pv.valueAsString);

		auto list = p.parse(totalArea);
		return TransformParser::TransformData::toTransform(list);
	}

	return {};
}

std::pair<Colour, ColourGradient> StyleSheet::getColourOrGradient(Rectangle<float> area, PropertyKey key,
                                                                  Colour defaultColour)
{
	key.appendSuffixIfNot("color");

	auto getValueFromString = [&](const String& v)
	{
		if(v.startsWith("linear-gradient"))
		{
			ColourGradient grad;
			ColourGradientParser p(area, v.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false));
			return std::pair(Colours::transparentBlack, p.getGradient());
		}
		else
		{
			auto c = Colour((uint32)v.getHexValue64());
			return std::pair(c, ColourGradient());
		}
	};

	if(auto tv = getTransitionValue(key))
	{
		auto v1 = getValueFromString(tv.startValue);
		auto v2 = getValueFromString(tv.endValue);

		if(v1.second.getNumColours() > 0 || v2.second.getNumColours() > 0)
		{
			// Can't blend gradients (for now...)
			jassertfalse;
			return { defaultColour, ColourGradient() };
		}

		return { v1.first.interpolatedWith(v2.first, tv.progress), ColourGradient() };
	}
	if(auto v = getPropertyValue(key))
	{
		return getValueFromString(v.valueAsString);
	}
		
	return { defaultColour, ColourGradient() };
}

std::pair<bool, int> StyleSheet::matchesRawList(const Selector::RawList& blockSelectors) const
{
	for(auto& s: selectors)
	{
		auto fit = s.matchesRawList(blockSelectors);

		if(fit.first)
			return fit;
	}

	return { false, (int)PseudoClassType::None };
}

String StyleSheet::toString() const
{
	String listContent;

	for(auto& s: selectors)
		listContent << s.toString() << " ";

	listContent  << " {\n";

	for(const auto& p: properties)
		listContent << p.toString();

	listContent << "}\n";

	return listContent;
}
}

}


