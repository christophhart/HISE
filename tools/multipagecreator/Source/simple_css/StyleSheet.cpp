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

void StyleSheet::copyPropertiesFrom(Ptr other)
{
	for(const auto& p: other->properties)
	{
		for(const auto& v: p.values)
		{
			bool found = false;

			for(auto& tp: properties)
			{
				if(tp.name == p.name)
				{
					tp.values[v.first] = v.second;
					found = true;
					break;
				}
			}

			if(!found)
			{
				properties.push_back(p);
			}
		}
	}
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

StyleSheet::Ptr StyleSheet::Collection::getOrCreateCascadedStyleSheet(const Array<Selector>& selectors)
{
	if(auto ss = (*this)[selectors])
		return ss;

			
	List matches;

	for(auto s: selectors)
	{
		if(auto ss = (*this)[s])
		{
			matches.add(ss);
		}
	}

	if(matches.isEmpty())
		return nullptr;

	if(matches.size() == 1)
		return matches.getFirst();

	StyleSheet::Ptr p = new StyleSheet(Array<Selector>());
	p->animator = matches.getFirst()->animator;

	for(auto m: matches)
	{
		p->selectors.addArray(m->selectors);
		p->copyPropertiesFrom(m);
	}

	list.add(p);
				

	DBG("New CSS added: " + p->toString());

	return p;
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
		auto useHeight = key.name.contains("top") || key.name.contains("bottom") || key.name == "font-size";
		auto fullSize = useHeight ? totalArea.getHeight() : totalArea.getWidth();
		return Parser::parseSize(v, fullSize, this->defaultFontSize);
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

struct ComponentUpdaters
{
	static void setColourIfDefined(Component* c, StyleSheet* css, int currentState, int id, const String& key)
	{
		auto colour = css->getColourOrGradient({}, { key, currentState}, c->findColour(id)).first;
		c->setColour(id, colour);
	};

	static void updateTextEditor(StyleSheet::Ptr ss, TextEditor* te, int currentState)
	{
		setColourIfDefined(te, ss, currentState, CaretComponent::ColourIds::caretColourId, "caret-color");
		
		auto fullBounds = te->getLocalBounds().toFloat();

		auto marginBounds = ss->getArea(fullBounds, { "margin", currentState});

		auto left = roundToInt(marginBounds.getX() - fullBounds.getX());
		auto top = roundToInt(marginBounds.getY() - fullBounds.getY());
		auto right = roundToInt(fullBounds.getRight() - marginBounds.getRight());
		auto bottom = roundToInt(fullBounds.getBottom() - marginBounds.getBottom());
		
		te->setBorder(juce::BorderSize<int>(left, right, top, bottom));
		te->setJustification(ss->getJustification(currentState));

		int paddingLeft = te->getLeftIndent();
		int paddingTop = te->getTopIndent();

		auto font = ss->getFont(0, te->getLocalBounds().toFloat());

		if(auto v = ss->getPropertyValue({ "padding-left", currentState}))
			paddingLeft = Parser::parseSize(v.valueAsString, marginBounds.getWidth(), font.getHeight());

		if(auto v = ss->getPropertyValue({ "padding-top", currentState}))
			paddingTop = Parser::parseSize(v.valueAsString, marginBounds.getWidth(), font.getHeight());

		te->setIndents(paddingLeft, paddingTop);
		te->applyFontToAllText(font, true);

		setColourIfDefined(te, ss, currentState, TextEditor::textColourId, "color");
		te->applyColourToAllText(te->findColour(TextEditor::textColourId));

		if(auto root = ComponentWithCSS::find(*te))
		{
			if(auto selectionSheet = root->css[Selector(SelectorType::Class, "::selection")])
			{
				setColourIfDefined(te, selectionSheet, 0, TextEditor::highlightedTextColourId, "color");
				setColourIfDefined(te, selectionSheet, 0, TextEditor::highlightColourId, "background");
			}
		}
	}
};

void StyleSheet::setupComponent(Component* c, int currentState)
{
	if(auto te = dynamic_cast<TextEditor*>(c))
	{
		ComponentUpdaters::updateTextEditor(this, te, currentState);
	}
}

Justification StyleSheet::getJustification(int currentState) const
{
	int x = Justification::Flags::horizontallyCentred;
	int y = Justification::Flags::verticallyCentred;

	if(auto v = getPropertyValue({"text-align", currentState }))
	{
		auto jv = v.valueAsString.trim();

		if(jv == "start" || jv == "left")
			x = Justification::Flags::left;
		else if(jv == "end" || jv == "right")
			x = Justification::right;
	}

	if(auto v = getPropertyValue({"vertical-align", currentState}))
	{
		auto vv = v.valueAsString.trim();

		if(vv == "top" || vv == "text-top")
			y = Justification::Flags::top;

		if(vv == "bottom" || vv == "text-bottom")
			y = Justification::Flags::bottom;
	}

	return Justification(x | y);
}

float StyleSheet::getOpacity(int state) const
{
	if(auto tv = getTransitionValue({"opacity", state}))
	{
		auto v1 = tv.startValue.getFloatValue();
		if(tv.startValue.endsWithChar('%'))
			v1 *= 0.01f;
            
		auto v2 = tv.endValue.getFloatValue();
		if(tv.endValue.endsWithChar('%'))
			v2 *= 0.01f;
            
		return Interpolator::interpolateLinear(v1, v2, (float)tv.progress);
	}
	if(auto pv = getPropertyValue({"opacity", state}))
	{
		auto v = pv.valueAsString.getFloatValue();
		if(pv.valueAsString.endsWithChar('%'))
			v *= 0.01f;
            
		return v;
	}
        
	return 1.0f;
}

Font StyleSheet::getFont(int currentState, Rectangle<float> totalArea) const
{
	auto fontName = Font::getDefaultSansSerifFontName();

	if(auto v = getPropertyValue({ "font-family", currentState}))
		fontName = v.valueAsString;

	if(fontName == "monospace")
		fontName = Font::getDefaultMonospacedFontName();

	auto size = getPixelValue(totalArea, { "font-size", currentState}, defaultFontSize);

	int flags = 0;

	if(auto wv = getPropertyValue({ "font-weight", currentState}))
	{
		static const StringArray normalStrings = { "default", "normal", "400", "unset" };

		if(!normalStrings.contains(wv.valueAsString.trim()))
			flags |= (int)Font::bold;
	}
	
	if(auto sv = getPropertyValue({ "font-style", currentState }))
	{
		if(sv.valueAsString.trim() == "italic")
			flags |= (int)Font::italic;
	}

	Font f(fontName, size, (Font::FontStyleFlags)flags);
		
	if(auto sv = getPropertyValue({ "font-stretch", currentState}))
	{
		auto scale = Parser::parseSize(sv.valueAsString, 1.0f, 1.0f);
		f = f.withHorizontalScale(scale);
	}

	if(auto lv = getPropertyValue({ "letter-spacing", currentState}))
	{
		auto v = lv.valueAsString.trim();

		if(v != "normal")
		{
			auto fullSize = size;
			auto letterSpacing = Parser::parseSize(lv.valueAsString, fullSize, size);
			f = f.withExtraKerningFactor(letterSpacing / size);
		}
	}

	return f;
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
	if(auto tv = getTransitionValue({ "transform", currentState}))
	{
		DBG(tv.progress);
		TransformParser p1(tv.startValue);
		TransformParser p2(tv.endValue);

		auto lastEmpty = tv.endValue.isEmpty();

		if(lastEmpty)
		{
			int x = 5;
		}

		auto l1 = p1.parse(totalArea);
		auto l2 = p2.parse(totalArea);

		auto num = jmax(l1.size(), l2.size());

		std::vector<TransformParser::TransformData> mix;
		mix.reserve(num);

		for(int i = 0; i < num; i++)
		{
			auto t1 = isPositiveAndBelow(i, l1.size()) ? l1[i] : TransformParser::TransformData(l2[i].type);
			auto t2 = isPositiveAndBelow(i, l2.size()) ? l2[i] : TransformParser::TransformData(l1[i].type);

			mix.push_back(t1.interpolate(t2, (float)tv.progress));
		}

		return TransformParser::TransformData::toTransform(mix, totalArea.getCentre());
	}
	if(auto pv = getPropertyValue({ "transform", currentState}))
	{
		TransformParser p(pv.valueAsString);

		auto list = p.parse(totalArea);
		return TransformParser::TransformData::toTransform(list, totalArea.getCentre());
	}

	return {};
}

std::pair<Colour, ColourGradient> StyleSheet::getColourOrGradient(Rectangle<float> area, PropertyKey key,
                                                                  Colour defaultColour)
{
	key.appendSuffixIfNot("color");

	if(defaultColour == Colours::transparentBlack && defaultColours.find(key.name) != defaultColours.end())
	{
		defaultColour = defaultColours[key.name];
	}

	auto getValueFromString = [&](const String& v)
	{
		if(v.startsWith("linear-gradient"))
		{
			ColourGradient grad;
			ColourGradientParser p(area, v.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false));
			return std::pair(defaultColour, p.getGradient());
		}
		else
		{
			auto c = Colour((uint32)v.getHexValue64());
			return std::pair(c, ColourGradient());
		}
	};

    if(auto tv = getTransitionValue({ "background-size", key.state }))
    {
        jassertfalse;
    }
    else if(auto tv = getPropertyValue({ "background-size", key.state }))
    {
        area.setHeight(Parser::parseSize(tv.valueAsString, area.getHeight(), 16.0f));
        area.setWidth(Parser::parseSize(tv.valueAsString, area.getWidth(), 16.0f));
    }
    
    if(auto tv = getTransitionValue({ "background-position", key.state }))
    {
        auto newX1 = area.getX() + Parser::parseSize(tv.startValue, area.getHeight(), 16.0f);
        auto newY1 = area.getY() + Parser::parseSize(tv.startValue, area.getWidth(), 16.0f);
        auto newX2 = area.getX() + Parser::parseSize(tv.endValue, area.getHeight(), 16.0f);
        auto newY2 = area.getY() + Parser::parseSize(tv.endValue, area.getWidth(), 16.0f);
        auto newX = Interpolator::interpolateLinear(newX1, newX2, (float)tv.progress);
        auto newY = Interpolator::interpolateLinear(newY1, newY2, (float)tv.progress);
        
        area.setX(newX);
        area.setY(newY);
    }
    else if(auto tv = getPropertyValue({ "background-position", key.state }))
    {
        auto newX = area.getX() + Parser::parseSize(tv.valueAsString, area.getHeight(), 16.0f);
        auto newY = area.getY() + Parser::parseSize(tv.valueAsString, area.getWidth(), 16.0f);
        area.setX(newX);
        area.setY(newY);
    }
    
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


