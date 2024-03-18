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
StyleSheet::StyleSheet(const Array<Selector>& selectors_)
{
	selectors.addArray(selectors_);
}

StyleSheet::StyleSheet(const Selector& s)
{
	selectors.add(s);
}

TransitionValue StyleSheet::getTransitionValue(const PropertyKey& key) const
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

		if(i->startValue.name == key.name && i->startValue.state.matchesElement(key.state))
		{
			tv.startValue = getPropertyValue(i->startValue).valueAsString;
			tv.endValue = getPropertyValue(i->endValue).valueAsString;

			if(tv.startValue == tv.endValue)
			{
				continue;
			}

			tv.progress = i->currentProgress;

			if(i->transitionData.f)
				tv.progress = i->transitionData.f(tv.progress);

			tv.active = true;
			break;
		}
	}

	return tv;
}

PropertyValue StyleSheet::getPropertyValue(const PropertyKey& key) const
{
	for(const auto& p: properties[(int)key.state.element])
	{
		if(p.name == key.name)
			return p.getProperty(key.state.stateFlag);
	}

	return {};
}

void StyleSheet::copyPropertiesFrom(Ptr other)
{
	other->forEachProperty(PseudoElementType::All, [&](PseudoElementType t, Property& p)
	{
		for(const auto& v: p.values)
		{
			bool found = false;

			for(auto& tp: properties[(int)t])
			{
				if(tp.name == p.name)
				{
					for(auto& stateValue: tp.values)
					{
						if(stateValue.first == v.first)
						{
							stateValue.second = v.second;
							found = true;
						}
					}
					if(!found)
					{
						tp.values.push_back(v);
						found = true;
					}
					
					break;
				}
			}

			if(!found)
			{
				properties[(int)t].push_back(p);
			}
		}

		return false;
	});

#if 0
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
#endif
}

NonUniformBorderData StyleSheet::getNonUniformBorder(Rectangle<float> totalArea, PseudoState stateFlag) const
{
	bool hasSpecialProperty = false;

	for(auto& v: properties[(int)stateFlag.element])
	{
		if(NonUniformBorderData::getBorderFromProperty(v.name) != NonUniformBorderData::numBorders)
		{
			hasSpecialProperty = true;
			break;
		}
	}

	if(!hasSpecialProperty)
		return {};

	auto defaultWidth = getPixelValue(totalArea, { "border-width", stateFlag });
	auto defaultColour = getColourOrGradient(totalArea, { "border-color", stateFlag }, Colours::transparentBlack);

	NonUniformBorderData nub(totalArea, defaultWidth, defaultColour);

	auto checkSize = [&](const String& bp)
	{
		PropertyKey k(bp, stateFlag);
		auto borderType = NonUniformBorderData::getBorderFromProperty(bp);
		auto useWidth = borderType == NonUniformBorderData::leftBorder || borderType == NonUniformBorderData::rightBorder;

		if(auto tv = getTransitionValue(k))
		{
			auto bs1 = ExpressionParser::evaluate(tv.startValue, {  useWidth, totalArea, defaultFontSize });
			auto bs2 = ExpressionParser::evaluate(tv.endValue, {  useWidth, totalArea, defaultFontSize });
			auto bs = Interpolator::interpolateLinear(bs1, bs2, (float)tv.progress);
			nub.setBorderSize(borderType, bs);
		}
		else if(auto pv = getPropertyValue(k))
		{
			auto bs = ExpressionParser::evaluate(pv.valueAsString, {  useWidth, totalArea , defaultFontSize});
			nub.setBorderSize(borderType, bs);
		}
	};

	auto checkColour = [&](const String& bc)
	{
		auto borderType = NonUniformBorderData::getBorderFromProperty(bc);
		PropertyKey k(bc, stateFlag);
		auto c = getColourOrGradient(totalArea, k, defaultColour.first);
		nub.setBorderColour(borderType, c);
	};

	checkSize("border-top-width");
	checkSize("border-left-width");
	checkSize("border-right-width");
	checkSize("border-bottom-width");

	checkColour("border-top-color");
	checkColour("border-left-color");
	checkColour("border-right-color");
	checkColour("border-bottom-color");

	return nub;
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

StyleSheet::Ptr StyleSheet::Collection::findBestMatch(const Array<Selector>& selectors) const
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

void StyleSheet::Collection::addElementStyle(Ptr p)
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

Path StyleSheet::getBorderPath(Rectangle<float> totalArea, PseudoState stateFlag) const
{
	float corners[4];
		
	corners[0] = getPixelValue(totalArea, {"border-top-left-radius", stateFlag});
	corners[1] = getPixelValue(totalArea, {"border-top-right-radius", stateFlag});
	corners[2] = getPixelValue(totalArea, {"border-bottom-left-radius", stateFlag});
	corners[3] = getPixelValue(totalArea, {"border-bottom-right-radius", stateFlag});

	float empty[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	Path p;

	if(auto pv = getPropertyValue({"background-image", stateFlag}))
	{
		if(!pv.valueAsString.startsWith("linear-gradient"))
		{
			MemoryBlock mb;
			mb.fromBase64Encoding(pv.valueAsString);
			p.loadPathFromData(mb.getData(), mb.getSize());
			PathFactory::scalePath(p, totalArea);
			return p;
		}
	}

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
	}

	return p;
}

String StyleSheet::getCodeGeneratorPixelValueString(const String& areaName, const PropertyKey& key,
	float defaultValue) const
{
	if(auto v = getPropertyValue(key))
	{
		ExpressionParser::Context<String> c;
		auto useHeight = key.name.contains("top") || key.name.contains("bottom") || key.name == "font-size" || key.name == "height";
		c.defaultFontSize = defaultFontSize;
		c.useWidth = !useHeight;
		c.fullArea = areaName;
		return ExpressionParser::evaluateToCodeGeneratorLiteral(v.valueAsString, c);
	}

	return {};
}

float StyleSheet::getPixelValue(Rectangle<float> totalArea, const PropertyKey& key, float defaultValue) const
{
	auto getValueFromString = [&](const String& v)
	{
		auto useHeight = key.name.contains("top") || key.name.contains("bottom") || key.name == "font-size" || key.name == "height";
		auto useWidth = !useHeight;

		if(v == "auto" && !currentFullArea.isEmpty())
		{
			auto size = useHeight ? totalArea.getHeight() : totalArea.getWidth();
			auto fullSize = useHeight ? currentFullArea.getWidth() : currentFullArea.getHeight();

			return (fullSize - size) / 2;
		}
		
		return ExpressionParser::evaluate(v, { useWidth, totalArea, this->defaultFontSize });
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

StringArray StyleSheet::getCodeGeneratorArea(const String& rectangleName, const PropertyKey& key) const
{
	StringArray lines;

	auto l = getCodeGeneratorPixelValueString(rectangleName, key.withSuffix("left"));
	auto t = getCodeGeneratorPixelValueString(rectangleName, key.withSuffix("top"));
	auto r = getCodeGeneratorPixelValueString(rectangleName, key.withSuffix("right"));
	auto b = getCodeGeneratorPixelValueString(rectangleName, key.withSuffix("bottom"));

	if(l.isNotEmpty())
		lines.add(rectangleName + ".removeFromLeft(" + l + ");");
	if(t.isNotEmpty())
		lines.add(rectangleName + ".removeFromTop(" + t + ");");
	if(r.isNotEmpty())
		lines.add(rectangleName + ".removeFromRight(" + r + ");");
	if(b.isNotEmpty())
		lines.add(rectangleName + ".removeFromBottom(" + b + ");");

	return lines;
}

Rectangle<float> StyleSheet::getArea(Rectangle<float> totalArea, const PropertyKey& key) const
{
	auto oa = totalArea;
	totalArea.removeFromLeft(getPixelValue(oa, key.withSuffix("left")));
	totalArea.removeFromTop(getPixelValue(oa, key.withSuffix("top")));
	totalArea.removeFromBottom(getPixelValue(oa, key.withSuffix("bottom")));
	totalArea.removeFromRight(getPixelValue(oa, key.withSuffix("right")));

	if(key.name == "margin" && getPropertyValue(key.withSuffix("left")).valueAsString == "auto")
	{
		totalArea.setWidth(oa.getWidth());
		totalArea.setHeight(oa.getHeight());
	}

	return totalArea;
}

void StyleSheet::setFullArea(Rectangle<float> fullArea)
{
	currentFullArea = fullArea;
}

Rectangle<float> StyleSheet::expandArea(Rectangle<float> sourceArea, const PropertyKey& key) const
{
	auto oa = sourceArea;
	sourceArea = sourceArea.withLeft(sourceArea.getX() - getPixelValue(oa, key.withSuffix("left")));
	sourceArea = sourceArea.withRight(sourceArea.getRight() + getPixelValue(oa, key.withSuffix("right")));
	sourceArea = sourceArea.withTop(sourceArea.getY() - getPixelValue(oa, key.withSuffix("top")));
	sourceArea = sourceArea.withBottom(sourceArea.getBottom() + getPixelValue(oa, key.withSuffix("bottom")));
	
	return sourceArea;
}

Rectangle<float> StyleSheet::getBounds(Rectangle<float> sourceArea, PseudoState ps) const
{
	auto a = sourceArea;

	auto w = sourceArea.getWidth();

	if(auto pv = getPropertyValue({"width", ps }))
		w = ExpressionParser::evaluate(pv.valueAsString, { true, sourceArea, defaultFontSize });

	if(auto mv = getPropertyValue({ "max-width", ps}))
		w = jmin(w, ExpressionParser::evaluate(mv.valueAsString, { true, sourceArea, defaultFontSize }));
	if(auto mv = getPropertyValue({ "min-width", ps})) // min has precedence
		w = jmax(w, ExpressionParser::evaluate(mv.valueAsString, { true, sourceArea, defaultFontSize }));
	
	switch(ps.element)
	{
	case PseudoElementType::None:
		a = a.removeFromLeft(w);
		break;
	case PseudoElementType::After:
		a = a.removeFromRight(w);
		break;
	case PseudoElementType::Before:
		a = a.removeFromLeft(w);
		break;
	}

	auto h = a.getHeight();



	if(auto hv = getPropertyValue({ "height", ps }))
		h = ExpressionParser::evaluate(hv.valueAsString, { false, a, defaultFontSize });

	if(auto mv = getPropertyValue({ "max-height", ps}))
		h = jmin(h, ExpressionParser::evaluate(mv.valueAsString, { true, sourceArea, defaultFontSize }));
	if(auto mv = getPropertyValue({ "min-height", ps})) // min has precedence
		h = jmax(h, ExpressionParser::evaluate(mv.valueAsString, { true, sourceArea, defaultFontSize }));

	a = a.removeFromTop(h);

	struct Match
	{
		operator bool() const { return match; }
		float value;
		bool match = false;
	};

	auto getValue = [&](const PropertyKey& k, bool useWidth)
	{
		Match rv;

		if(auto tv = getTransitionValue(k))
		{
			rv.match = true;
			auto v1 = ExpressionParser::evaluate(tv.startValue, { useWidth, sourceArea, defaultFontSize });
			auto v2 = ExpressionParser::evaluate(tv.endValue, { useWidth, sourceArea, defaultFontSize });
			rv.value = Interpolator::interpolateLinear(v1, v2, (float)tv.progress);
		}
		else if(auto v = getPropertyValue(k))
		{
			rv.match = true;
			rv.value = ExpressionParser::evaluate(v.valueAsString, { useWidth, sourceArea, defaultFontSize });
		}
		
		return rv;
	};

	if(auto v = getValue({ "top", ps}, false))
		a = a.withY(sourceArea.getY() + v.value);
	if(auto v = getValue({ "left", ps}, true))
		a = a.withX(sourceArea.getX() + v.value);
	if(auto v = getValue({ "bottom", ps}, false))
		a = a.withBottomY(sourceArea.getBottom() - v.value);
	if(auto v = getValue({ "right", ps}, true))
		a = a.withRightX(sourceArea.getRight() - v.value);

	return a;
}

Rectangle<float> StyleSheet::getPseudoArea(Rectangle<float> sourceArea, int currentState, PseudoElementType area) const
{
	bool found = false;

	for(auto& pv: properties[(int)area])
	{
		if(pv.name == "content")
		{
			found = true;
			break;
		}
	}

	if(!found)
		return {};

	jassert(area == PseudoElementType::Before || area == PseudoElementType::After );

	PseudoState ps(currentState);
	ps.element = area;

	return getBounds(sourceArea, ps);

	
}

Rectangle<float> StyleSheet::truncateBeforeAndAfter(Rectangle<float> sourceArea, int currentState) const
{
	auto wb = getPseudoArea(sourceArea, currentState, PseudoElementType::Before).getWidth();
	auto truncateBefore = wb > 0.0f;

	if(truncateBefore)
	{
		auto t = getPositionType(PseudoState(currentState).withElement(PseudoElementType::Before));
		truncateBefore &= (t != PositionType::absolute);
	}

	if(truncateBefore)
		sourceArea.removeFromLeft(wb);

	auto wa = getPseudoArea(sourceArea, currentState, PseudoElementType::After).getWidth();
	auto truncateAfter = wa > 0.0f;

	if(truncateAfter)
	{
		auto t = getPositionType(PseudoState(currentState).withElement(PseudoElementType::After));
		truncateAfter &= (t != PositionType::absolute);
	}

	if(truncateAfter)
		sourceArea.removeFromRight(wa);

	return sourceArea;
}

struct ComponentUpdaters
{
	static void setColourIfDefined(Component* c, StyleSheet* css, int currentState, int id, const String& key)
	{
		auto colour = css->getColourOrGradient({}, {key.getCharPointer().getAddress(), currentState}, c->findColour(id)).first;
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
		te->setJustification(ss->getJustification(currentState, Justification::left, Justification::verticallyCentred));

		int paddingLeft = te->getLeftIndent();
		int paddingTop = te->getTopIndent();

		auto font = ss->getFont(0, te->getLocalBounds().toFloat());

		if(auto v = ss->getPropertyValue({ "padding-left", currentState}))
			paddingLeft = ExpressionParser::evaluate(v.valueAsString, { true, marginBounds, font.getHeight() });

		if(auto v = ss->getPropertyValue({ "padding-top", currentState}))
			paddingTop = ExpressionParser::evaluate(v.valueAsString, { true, marginBounds, font.getHeight() });

		te->setIndents(paddingLeft, paddingTop);
		te->applyFontToAllText(font, true);

		te->setColour(PopupMenu::backgroundColourId, Colours::transparentBlack);

		setColourIfDefined(te, ss, currentState, TextEditor::textColourId, "color");
		te->applyColourToAllText(te->findColour(TextEditor::textColourId));

		if(auto root = CSSRootComponent::find(*te))
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

Justification StyleSheet::getJustification(int currentState, int defaultXFlag, int defaultYFlag) const
{
	int x = defaultXFlag;
	int y = defaultYFlag;

	if(auto v = getPropertyValue({"text-align", currentState }))
	{
		auto jv = v.valueAsString.trim();

		if(jv == "start" || jv == "left")
			x = Justification::Flags::left;
		else if(jv == "end" || jv == "right")
			x = Justification::right;
	}

	if(auto v = getPropertyValue({"vertical-align", currentState }))
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

MouseCursor StyleSheet::getMouseCursor() const
{
	if(auto mv = getPropertyValue({ "cursor", 0}))
	{
		enum CustomCursorEnum
		{
			Default = MouseCursor::NormalCursor,
			Pointer = MouseCursor::PointingHandCursor,
			Wait = MouseCursor::WaitCursor,
			Crosshair = MouseCursor::CrosshairCursor,
			Text = MouseCursor::IBeamCursor,
			Copy = MouseCursor::CopyingCursor,
			Grab = MouseCursor::DraggingHandCursor 
		};

		auto c = getAsEnum({ "cursor", 0}, CustomCursorEnum::Default);

		return MouseCursor((MouseCursor::StandardCursorType)c);
	}

	return MouseCursor();
}

String StyleSheet::getText(const String& t, int currentState) const
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

Font StyleSheet::getFont(int currentState, Rectangle<float> totalArea) const
{
	auto fontName = Font::getDefaultSansSerifFontName();

	if(auto v = getPropertyValue({ "font-family", currentState}))
		fontName = v.valueAsString;

	if(fontName == "monospace")
		fontName = Font::getDefaultMonospacedFontName();

	auto size = getPixelValue(totalArea, { "font-size", currentState}, defaultFontSize);

	int flags = 0;

	if(getAsEnum<int>({"font-weight", currentState}, 1) > 3)
		flags |= (int)Font::bold;

	if(getAsEnum<int>({"font-style", currentState}, 0))
		flags |= (int)Font::italic;

	Font f(fontName, size, (Font::FontStyleFlags)flags);
		
	if(auto sv = getPropertyValue({ "font-stretch", currentState}))
	{
		auto scale = ExpressionParser::evaluate(sv.valueAsString, { false, {1.0f, 1.0f }, 1.0f });
		f = f.withHorizontalScale(scale);
	}

	if(auto lv = getPropertyValue({ "letter-spacing", currentState}))
	{
		auto v = lv.valueAsString.trim();

		if(v != "normal")
		{
			auto fullSize = size;
			auto letterSpacing = ExpressionParser::evaluate(lv.valueAsString, { false, { fullSize, fullSize }, size });
			f = f.withExtraKerningFactor(letterSpacing / size);
		}
	}

	return f;
}

std::vector<melatonin::ShadowParameters> StyleSheet::getShadow(Rectangle<float> totalArea, const PropertyKey& key, bool wantsInset) const
{
	if(auto tv = getTransitionValue(key))
	{
		ShadowParser s1(tv.startValue, totalArea);
		ShadowParser s2(tv.endValue, totalArea);
		return s1.interpolate(s2, tv.progress, wantsInset);
	}
	else if(auto v = getPropertyValue(key))
	{
		ShadowParser s(v.valueAsString, totalArea);
		return s.getShadowParameters(wantsInset);
	}

	return {};
}

AffineTransform StyleSheet::getTransform(Rectangle<float> totalArea, PseudoState currentState) const
{
	if(auto tv = getTransitionValue({ "transform", currentState}))
	{
		DBG(tv.progress);
		TransformParser p1(keywords, tv.startValue);
		TransformParser p2(keywords, tv.endValue);

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
		TransformParser p(keywords, pv.valueAsString);

		auto list = p.parse(totalArea);
		return TransformParser::TransformData::toTransform(list, totalArea.getCentre());
	}

	return {};
}

std::pair<Colour, ColourGradient> StyleSheet::getColourOrGradient(Rectangle<float> area, PropertyKey key,
                                                                  Colour defaultColour) const
{
	key.appendSuffixIfNot("color");
	
	if(defaultColour == Colours::transparentBlack && defaultColours.find(key.name) != defaultColours.end())
	{
		defaultColour = defaultColours.at(key.name);
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
		area.setHeight(ExpressionParser::evaluate(tv.valueAsString, { false, area , 16.0f }));
		area.setWidth(ExpressionParser::evaluate(tv.valueAsString, { true, area , 16.0f }));
    }
    
    if(auto tv = getTransitionValue({ "background-position", key.state }))
    {
		auto newX1 = area.getX() + ExpressionParser::evaluate(tv.startValue, { false, area , 16.0f});
		auto newY1 = area.getY() + ExpressionParser::evaluate(tv.startValue, { true, area, 16.0f });
		auto newX2 = area.getX() + ExpressionParser::evaluate(tv.endValue, { false, area, 16.0f });
		auto newY2 = area.getY() + ExpressionParser::evaluate(tv.endValue, { true, area, 16.0f });
        auto newX = Interpolator::interpolateLinear(newX1, newX2, (float)tv.progress);
        auto newY = Interpolator::interpolateLinear(newY1, newY2, (float)tv.progress);
        
        area.setX(newX);
        area.setY(newY);
    }
    else if(auto tv = getPropertyValue({ "background-position", key.state }))
    {
		auto newX = area.getX() + ExpressionParser::evaluate(tv.valueAsString, { false, area, 16.0f });
		auto newY = area.getY() + ExpressionParser::evaluate(tv.valueAsString, { true, area, 16.0f });
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

String StyleSheet::getCodeGeneratorColour(const String& rectangleName, PropertyKey key, Colour defaultColour) const
{
	key.appendSuffixIfNot("color");

	if(auto v = getPropertyValue(key))
	{
		return "Colour(" + v.valueAsString + ")";
	}

	return {};
}

std::pair<bool, PseudoState> StyleSheet::matchesRawList(const Selector::RawList& blockSelectors) const
{
	Selector thisType(SelectorType::All, "");
	Selector blockType(SelectorType::All, "");
	Selector thisID(SelectorType::All, "");
	Selector blockID(SelectorType::All, "");
	std::vector<Selector> thisClasses;
	std::vector<Selector> blockClasses;

	for(const auto& v: selectors)
	{
		if(v.type == SelectorType::Type)
		{
			jassert(thisType.type == SelectorType::All);
			thisType = v;
		}
		else if(v.type == SelectorType::ID)
		{
			jassert(thisID.type == SelectorType::All);
			thisID = v;
		}
		else if (v.type == SelectorType::Class)
		{
			thisClasses.push_back(v);
		}
	}

	for(const auto& v: blockSelectors)
	{
		if(v.first.type == SelectorType::Type)
		{
			jassert(blockType.type == SelectorType::All);
			blockType = v.first;
		}
		else if(v.first.type == SelectorType::ID)
		{
			jassert(blockID.type == SelectorType::All);
			blockID = v.first;
		}
		else if (v.first.type == SelectorType::Class)
		{
			blockClasses.push_back(v.first);
		}
	}

	if(thisType != blockType)
		return { false, { PseudoClassType::None, PseudoElementType::None } };

	// do not use the catch all operator== here
	if(thisID.name != blockID.name)
		return { false, { PseudoClassType::None, PseudoElementType::None } };

	if(thisClasses.size() != blockClasses.size())
		return { false, { PseudoClassType::None, PseudoElementType::None } };

	for(int i = 0; i < thisClasses.size(); i++)
	{
		if(std::find(blockClasses.begin(), blockClasses.end(), thisClasses[i]) == blockClasses.end())
		{
			return { false, { PseudoClassType::None, PseudoElementType::None } };
		}
	}

	return { true, blockSelectors[0].second };
}

String StyleSheet::toString() const
{
	String listContent;

	for(auto& s: selectors)
		listContent << s.toString() << " ";
	
	int idx = 0;

	for(const auto& ep: properties)
	{
		if(ep.empty())
		{
			idx++;
			continue;
		}
			
		if(idx != 0)
			listContent << "\n::" << PseudoState::getPseudoElementName(idx);

		idx++;

		listContent  << " {\n";

		for(auto& p: ep)
			listContent << p.toString();

		listContent << "}\n";
	}
		

	

	return listContent;
}

void StyleSheet::setDefaultTransition(PseudoElementType elementType, const Transition& t)
{
	defaultTransitions[(int)elementType] = t;
}

Transition StyleSheet::getTransitionOrDefault(PseudoElementType elementType, const Transition& t) const
{
	if(t.active)
		return t;

	return defaultTransitions[(int)elementType];
}

bool StyleSheet::matchesSelectorList(const Array<Selector>& otherSelectors)
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

bool StyleSheet::forEachProperty(PseudoElementType type, const std::function<bool(PseudoElementType, Property& v)>& f)
{
	if(type == PseudoElementType::All)
	{
		int typeIndex = 0;
		for(auto& b: properties)
		{
			for(auto& p: b)
			{
				if(f((PseudoElementType)typeIndex, p))
					return true;
			}

			typeIndex++;
		}
	}
	else
	{
		for(auto& p: properties[(int)type])
		{
			if(f(type, p))
				return true;
		}
	}

	return false;
}

void StyleSheet::setDefaultColour(const String& key, Colour c)
{
	defaultColours[key] = c;
}

PositionType StyleSheet::getPositionType(PseudoState state) const
{
	return getAsEnum({ "position", state}, PositionType::initial);
}

FlexBox StyleSheet::getFlexBox() const
{
	FlexBox flexBox;

	auto displayType = getPropertyValue({ "display", {}}).valueAsString;

	if(displayType != "flex")
		return flexBox;

	flexBox.flexDirection = getAsEnum({ "flex-direction", {}}, FlexBox::Direction::row);
	flexBox.flexWrap = getAsEnum({ "flex-wrap", {}}, FlexBox::Wrap::noWrap);
	flexBox.justifyContent = getAsEnum({ "justify-content", {}}, FlexBox::JustifyContent::flexStart);
	flexBox.alignItems = getAsEnum({ "align-items", {}}, FlexBox::AlignItems::center);
	flexBox.alignContent = getAsEnum({ "align-content", {}}, FlexBox::AlignContent::center);
	
	return flexBox;
}



FlexItem StyleSheet::getFlexItem(Component* c, Rectangle<float> fullArea) const
{
	auto b = fullArea;

	FlexItem item(*c);

	float defaultWidth = -1.0f;
	float defaultHeight = -1.0f;
	
	item.width = getPixelValue(b, { "width", {}}, -1.0f);
	item.height = getPixelValue(b, { "height", {}}, -1.0f);
	item.minWidth = getPixelValue(b, { "min-width", {}}, -1.0f);
	item.maxWidth = getPixelValue(b, { "max-width", {}}, -1.0f);
	item.minHeight = getPixelValue(b, { "min-height", {}}, -1.0f);
	item.maxHeight = getPixelValue(b, { "max-height", {}}, -1.0f);

	if(auto bt = dynamic_cast<Button*>(c))
	{
		auto tb = getLocalBoundsFromText(bt->getButtonText());

		item.minHeight = tb.getHeight();
		item.minWidth = tb.getWidth();
	}
	
	if(auto v = getPropertyValue({ "order", {}}))
		item.order = v.valueAsString.getIntValue();

	item.alignSelf = getAsEnum( { "align-self", {} }, FlexItem::AlignSelf::autoAlign);

	if(auto pv = getPropertyValue({ "flex-grow", {}}))
		item.flexGrow = ExpressionParser::evaluate(pv.valueAsString, {});

	if(auto pv = getPropertyValue({ "flex-shrink", {}}))
		item.flexShrink = ExpressionParser::evaluate(pv.valueAsString, {});

	if(auto pv = getPropertyValue({ "flex-basis", {}}))
		item.flexBasis = ExpressionParser::evaluate(pv.valueAsString, {});
	
	return item;
}

Rectangle<float> StyleSheet::getLocalBoundsFromText(const String& text) const
{
	auto f = getFont(0, {});
	auto textWidth = f.getStringWidthFloat(getText(text, 0));
	auto textHeight = f.getHeight();

	Rectangle<float> area(0.0f, 0.0f, textWidth, textHeight);

	area = getBounds(area, {});
	area = expandArea(area, { "padding", {}});
	area = expandArea(area, { "margin", {}});

	auto ba = getPseudoArea(area, 0, PseudoElementType::Before);

	if(!ba.isEmpty())
	{
		auto pos = getPositionType(PseudoState().withElement(PseudoElementType::Before));
		auto dontExtend = pos == PositionType::absolute || pos == PositionType::fixed;
		if(dontExtend)
			ba = {};
	}

	if(!ba.isEmpty())
		area = area.withLeft(area.getX() - ba.getWidth());
	
	return area.withZeroOrigin();
}
}

}


