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

template <typename T> struct TransitionCalculator
{
	TransitionCalculator(const StyleSheet* ss_, Animator* animator_, const String& propertyName_, const TransitionValue& tv_):
	  ss(ss_),
	  animator(animator_),
	  propertyName(propertyName_),
	  tv(tv_)
	{}

	TransitionCalculator() = default;

	const StyleSheet* ss;
	Animator* animator;
	String propertyName;
	TransitionValue tv;

	std::function<T(const String& v)> valueFunction;
	std::function<T(const T&, const T&, float)> interpolator;
	std::function<String(const T&)> toStringFunction;

	T calculate() const
	{
		jassert(valueFunction);
		jassert(interpolator);
		jassert(toStringFunction);

		if(tv.startValue.containsChar('~'))
		{
			auto t = StringArray::fromTokens(tv.startValue, "~", "");

			jassert(t.size() == 3);

			auto sv1 = valueFunction(t[0]);
			auto sv2 = valueFunction(t[1]);
			auto d = t[2].getFloatValue();

			auto v1 = interpolator(sv1, sv2, d);

			for(auto i: animator->items)
			{
				if(i->css == ss && i->startValue.name == propertyName)
				{
					i->intermediateStartValue = toStringFunction(v1);
					//i->speed = d;
					break;
				}
			}

			auto v2 = valueFunction(tv.endValue);

			return interpolator(v1, v2, tv.progress);
		}
		else
		{
			auto v1 = valueFunction(tv.startValue);
			auto v2 = valueFunction(tv.endValue);
			return interpolator(v1, v2, (float)tv.progress);
		}
	}
};

TransitionValue StyleSheet::getTransitionValue(const PropertyKey& key) const
{
	TransitionValue tv;

	if(animator == nullptr)
		return tv;

	for(const auto i: animator->items)
	{
		if(i->target.first != animator->currentlyRenderedComponent.first ||
		   i->target.second != animator->currentlyRenderedComponent.second)
			continue;

		if(i->css != this)
			continue;

		if(i->startValue.name == key.name && i->startValue.state.matchesElement(key.state))
		{
			if(i->intermediateStartValue.isNotEmpty())
				tv.startValue = i->intermediateStartValue;
			else
				tv.startValue = getPropertyValueString(i->startValue);

			tv.endValue = getPropertyValueString(i->endValue);
			
			if(tv.startValue == tv.endValue)
			{
				continue;
			}

			tv.progress = i->currentProgress;

			if(i->transitionData.f)
			{
				if(i->currentAnimationRange.getLength() != 1.0 && !i->currentAnimationRange.isEmpty())
				{
					NormalisableRange<double> nr(i->currentAnimationRange);

					auto v = nr.convertTo0to1(tv.progress);

					if(i->reverse)
						v = 1.0 - v;
					
					v = jlimit(0.0, 1.0, i->transitionData.f(v));
					
					
					if(i->reverse)
						v = (1.0 - v);

					v = i->currentAnimationRange.getStart() + v * i->currentAnimationRange.getLength();
					
					tv.progress = v;
				}
				else
				{
					if(i->reverse)
					{
						auto v = 1.0 - tv.progress;
						v = i->transitionData.f(v);
						tv.progress = 1.0 - v;
					}
					else
					{
						tv.progress = i->transitionData.f(tv.progress);
					}
				}


				
			}
				

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

String StyleSheet::getPropertyValueString(const PropertyKey& key) const
{
	return getPropertyValue(key).getValue(varProperties);
}

void StyleSheet::copyPropertiesFrom(Ptr other, bool overwriteExisting, const StringArray& propertiesToCopy)
{
    if(other == nullptr)
        return;
	
	copyVarProperties(other);

	other->forEachProperty(PseudoElementType::All, [&](PseudoElementType t, Property& p)
	{


		for(const auto& v: p.values)
		{
            if(p.name == "all")
            {
                auto valueToUse = p.values[0].second;
                
                for(auto& tp: properties[(int)t])
                {
                    for(auto& s: tp.values)
                        s.second = valueToUse;
                }
                
                return false;
            }
            
			bool found = false;

			if(!propertiesToCopy.isEmpty() && !propertiesToCopy.contains(p.name))
				continue;

			for(auto& tp: properties[(int)t])
			{
                if(tp.name == p.name)
				{
					for(auto& stateValue: tp.values)
					{
						if(stateValue.first == v.first)
						{
                            auto thisImportant = stateValue.second.important;
                            auto otherImportant = v.second.important;
                            
							if(overwriteExisting)
                            {
                                if((int)otherImportant >= (int)thisImportant)
                                    stateValue.second = v.second;
                            }

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

	forEachProperty(PseudoElementType::All, [](PseudoElementType t, Property& v)
	{
		std::sort(v.values.begin(), v.values.end(), [](const std::pair<int, PropertyValue>& v1, const std::pair<int, PropertyValue>& v2)
		{
			return v1.first < v2.first;
		});

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

void StyleSheet::copyPropertiesFromParent(Ptr parent)
{
	if(parent != nullptr)
		copyPropertiesFrom(parent, true, keywords->getInheritedProperties());
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
			TransitionCalculator<float> im(this, animator, k.name, tv);

			ExpressionParser::Context<> ctx = { useWidth, totalArea, defaultFontSize };

			im.valueFunction = [ctx](const String& v) { return ExpressionParser::evaluate(v, ctx); };
			im.interpolator = Interpolator::interpolateLinear<float>;
			im.toStringFunction = [](float v) { return String(v); };

			auto v = im.calculate();
			
			nub.setBorderSize(borderType, v);
		}
		else if(auto pv = getPropertyValue(k))
		{
			auto bs = ExpressionParser::evaluate(pv.getValue(varProperties), {  useWidth, totalArea , defaultFontSize});
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
#if 0
	if(auto body = (*this)[Selector(SelectorType::All, "*")])
	{
		for(auto s: list)
		{
			if(s == body.get())
				continue;

			s->copyPropertiesFrom(body, false, { "color", "font-family", "font-size", "font-weight", "text-align"});
		}
	}
#endif

	// TODO: sort so that it takes the best match first
}

void StyleSheet::Collection::setAnimator(Animator* a)
{
	jassert(a != nullptr);
	animator = a;

	forEach([a](Ptr p)
	{
		p->animator = a;
	});
}

StyleSheet::Ptr StyleSheet::Collection::operator[](const Selector& s) const
{
	StyleSheet::Ptr all;

	for(auto& l: list)
	{
		if(l->matchesSelectorList({s}))
		{
			if(l->isAll())
				all = l;
			else
				return l;
		}
	}

	return all;
}

String StyleSheet::Collection::getDebugLogForComponent(Component* c) const
{
	for(auto& cm: cachedMaps)
	{
		if(cm.first.getComponent() == c)
		{
			if(auto obj = cm.second->varProperties.get())
			{
				String s;
				s << "Current variable values:\n";
				s << JSON::toString(var(obj));
				s << "\n==============================\n\n";

				s << cm.debugLog;
				return s;
			}

			return cm.debugLog;
		}
			
	}
            
	return {};
}

StyleSheet::Ptr StyleSheet::Collection::getForComponent(Component* c)
{
	for(const auto& existing: cachedMaps)
	{
		if(existing.first.getComponent() == c)
			return existing.second;
	}

	using Match = std::pair<ComplexSelector::Score, StyleSheet::Ptr>;
	
    List debugList;
    
	Array<Match> matches;

	auto all = getWithAllStates(c, Selector(SelectorType::All, "*"));
	StyleSheet::Ptr parentStyle;

	Array<Selector> pSelectors;

	if(!useIsolatedCollections)
	{
		if(dynamic_cast<CSSRootComponent*>(c) == nullptr)
		{
			if(auto p = c->getParentComponent())
			{
				parentStyle = getForComponent(p);

				auto tp = p;

				while(tp != nullptr)
				{
					if(dynamic_cast<CSSRootComponent*>(tp))
						break;

					pSelectors.addArray(ComplexSelector::getSelectorsForComponent(tp));
					tp = tp->getParentComponent();
				}
			}
		}
	}
	
	auto selectors = ComplexSelector::getSelectorsForComponent(c);

	auto addFromList = [&](List& listToUse)
	{
		for(auto l: listToUse)
		{
			if(l->getAtRuleName().isNotEmpty())
				continue;

			if(l->isAll())
			{
				all = l;
			}
			else
			{
				for(auto cs: l->complexSelectors)
				{
					if(cs->matchesSelectors(selectors, pSelectors))
						matches.add({ ComplexSelector::Score(cs, selectors), l });
				}
			}
		}
	};

	if(useIsolatedCollections)
	{
		for(auto& cc: childCollections)
		{
			if(sameOrParent(cc.first.getComponent(), c))
			{
				addFromList(cc.second);
				break;
			}
		}
	}
	else
	{
		addFromList(list);

		for(auto& cc: childCollections)
		{
			auto p = cc.first.getComponent();
			if(p != nullptr && p->isParentOf(c))
			{
				addFromList(cc.second);
			}
		}
	}

	auto elementStyle = c->getProperties()["style"].toString();
	auto inlineStyle = FlexboxComponent::Helpers::getInlineStyle(*c);

	auto customCode = elementStyle.isNotEmpty() || inlineStyle.isNotEmpty() || parentStyle != nullptr || all != nullptr;

	if(matches.isEmpty() && !customCode)
		return nullptr;
	
	if(matches.size() == 1 && !customCode && !useIsolatedCollections)
	{
		return matches.getFirst().second;
	}

	struct Sorter
	{
		static int compareElements(const Match& m1, const Match& m2)
		{
			if(m1.first < m2.first)
				return -1;
			if(m1.first > m2.first)
				return 1;

			return 0;
		}
	} sorter;

	matches.sort(sorter, true);

	auto ptrValue = reinterpret_cast<uint64>(c);
	Selector elementSelector(SelectorType::Element, String::toHexString(ptrValue));

	StyleSheet::Ptr ptr;

	if(elementStyle.isNotEmpty())
	{
		String code;

		code << elementSelector.toString();
		code << "{ " << elementStyle;
		if(!code.endsWithChar(';')) code << ";";
		code << " }";

		Parser p(code);
		auto ok = p.parse();

		if(ok.wasOk())
			ptr = p.getCSSValues()[elementSelector];
	}
	else
		ptr = new StyleSheet(elementSelector);
    
	ptr->copyPropertiesFromParent(all);
    
    if(all != nullptr)
        debugList.addIfNotAlreadyThere(all);
    
	ptr->copyPropertiesFromParent(parentStyle);

    if(parentStyle != nullptr)
        debugList.addIfNotAlreadyThere(parentStyle);

	for(const auto& m: matches)
	{
        auto other = m.second;
        
        if(other != nullptr)
            debugList.addIfNotAlreadyThere(other);
        
		ptr->copyPropertiesFrom(other);
	}

	if(inlineStyle.isNotEmpty())
	{
		String ic;
		ic << elementSelector.toString();
		ic << "{" << inlineStyle;
		if(!inlineStyle.endsWithChar(';'))
			ic << ";";

		ic << "}";
		Parser p(ic);
		p.parse();

		auto otherList = p.getCSSValues();
		otherList.setAnimator(animator);

		auto is = otherList.getWithAllStates(nullptr, elementSelector);
        
        if(is != nullptr)
            debugList.addIfNotAlreadyThere(is);
        
		ptr->copyPropertiesFrom(is, true);
	}

    String inherited;
    String styleSheetLog;
	
    for(int i = debugList.size() - 1; i >= 0; i--)
    {
        auto newStyle = debugList[i]->toString();
        
        auto lines = StringArray::fromLines(newStyle);
        
        for(auto& l: lines)
        {
            auto prop = l.upToFirstOccurrenceOf(":", true, false).trim();
            
            if(l.containsChar(':') && inherited.contains(prop) )
                l =   "/*" + l + "*/";
        }
        
        newStyle = lines.joinIntoString("\n");
        
        inherited << newStyle;
        inherited << "\n";
    }
    
    auto tc = c;

	if(createStackTrace)
		styleSheetLog << "/* CSS for component hierarchy: */\n";
    

    
    StringArray hierarchy;
    
    StringArray elementSelectors;
    
    while(tc != nullptr)
    {
        if(auto vp = dynamic_cast<FlexboxViewport*>(tc))
            tc = tc->getParentComponent();
        
        hierarchy.add(FlexboxComponent::Helpers::dump(*tc));
        
        auto currentType = FlexboxComponent::Helpers::getTypeSelectorFromComponentClass(tc);
        
        if(currentType == Selector(ElementType::Body))
            break;
        
        auto ptrValue = reinterpret_cast<uint64>(tc);
        Selector elementSelector(SelectorType::Element, String::toHexString(ptrValue));
        
        elementSelectors.add(elementSelector.toString());
        
        tc = tc->getParentComponent();
    }

	if(createStackTrace)
	{
		String pre = "\n";
    
	    for(int i = hierarchy.size() - 1; i >= 0; i--)
	    {
	        styleSheetLog << pre << hierarchy[i];
	        pre << " ";
	    }
	    
	    styleSheetLog << "\n\n/** Component stylesheet: */\n";
	    styleSheetLog << ptr->toString();
	    styleSheetLog << "\n\n/** Inherited style sheets: */\n";
	    styleSheetLog << inherited;
	    
	    for(int i = 0; i < elementSelectors.size(); i++)
	    {
	        styleSheetLog = styleSheetLog.replace(elementSelectors[i], hierarchy[i]);
	    }
	}

	ptr->setCustomFonts(customFonts);

	cachedMaps.add({ c, ptr, styleSheetLog });

	jassert(animator != nullptr);
	ptr->animator = animator;
	
	return ptr;

#if 0
	Array<Selector> selectors;

	if(auto fc = dynamic_cast<FlexboxComponent*>(c))
	{
		// overwrite the default behaviour and only return the
		// selector that was defined
		selectors.add(fc->getSelector());
		selectors.addArray(FlexboxComponent::Helpers::getClassSelectorFromComponentClass(c));
	}
	else
	{
		if(auto ts = FlexboxComponent::Helpers::getTypeSelectorFromComponentClass(c))
			selectors.add(ts);

		auto classList = FlexboxComponent::Helpers::getClassSelectorFromComponentClass(c);

		selectors.addArray(classList);

		if(auto is = FlexboxComponent::Helpers::getIdSelectorFromComponentClass(c))
			selectors.add(is);
	}

	auto elementStyle = c->getProperties()["style"].toString();

	StyleSheet::Ptr ptr;

	if(elementStyle.isNotEmpty())
	{
		String code;

		auto ptrValue = reinterpret_cast<uint64>(c);
		Selector elementSelector(SelectorType::Element, String::toHexString(ptrValue));
					
		code << elementSelector.toString();
		code << "{ " << elementStyle;
		if(!code.endsWithChar(';')) code << ";";
		code << " }";

		Parser p(code);
		auto ok = p.parse();
		if(ok.wasOk())
			ptr = p.getCSSValues().getOrCreateCascadedStyleSheet({elementSelector});

		addElementStyle(ptr);

		if(auto body = (*this)[ElementType::Body])
		{
			ptr->copyPropertiesFrom(body, false);
		}
	}

	auto cssPtr = getOrCreateCascadedStyleSheet(selectors);

	if(ptr != nullptr)
	{
		if(cssPtr != nullptr)
			ptr->copyPropertiesFrom(cssPtr);
	}
	else
	{
		ptr = cssPtr;
	}

	if(auto root = CSSRootComponent::find(*c))
		setAnimator(&root->animator);

	return ptr;
#endif
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

	



StyleSheet::Ptr StyleSheet::Collection::getWithAllStates(Component* c, const Selector& s)
{
	for(const auto& existing: cachedMapForAllStates)
	{
		if(existing.first.second.exactMatch(s) && existing.first.first == c)
			return existing.second;
	}

	auto wantsAll = s.type == SelectorType::All;

	List matches;

	auto addIfMatch = [&](Ptr l)
	{
		if(l->getAtRuleName().isNotEmpty())
			return;

		if(l->isAll() != wantsAll)
			return;

		for(auto cs: l->complexSelectors)
		{
			if(!cs->hasParentSelectors() && 
				cs->thisSelectors.isSingle() &&
				cs->thisSelectors.selectors[0].first == s)
			{
				matches.add(l);
			}
		}
	};

	if(useIsolatedCollections)
	{
		for(auto& s: childCollections)
		{
			if(c == nullptr || sameOrParent(s.first.getComponent(), c))
			{
				for(auto l: s.second)
					addIfMatch(l);
			}
		}
	}
	else
	{
		for(auto l: list)
		{
			addIfMatch(l);
		}

		if(c != nullptr)
		{
			for(auto& s: childCollections)
			{
				if(s.first->isParentOf(c))
				{
					for(auto l: s.second)
						addIfMatch(l);
				}
			}
		}
	}

	if(matches.isEmpty())
		return nullptr;

	ComplexSelector::List l;
    
	l.add(new ComplexSelector(s, {}));

	auto ptr = new StyleSheet(l);

	for(auto m: matches)
		ptr->copyPropertiesFrom(m, true);

	cachedMapForAllStates.add({{c, s}, ptr});

	jassert(animator != nullptr);

	ptr->animator = animator;

	return ptr;
}


void StyleSheet::Collection::setPropertyVariable(const Identifier& id, const var& newValue)
{
	forEach([id, newValue](Ptr p)
	{
		p->setPropertyVariable(id, newValue);
	});
}

MarkdownLayout::StyleData StyleSheet::Collection::getMarkdownStyleData(Component* c)
{
	auto sd = MarkdownLayout::StyleData::createDarkStyle();

	auto ss = getForComponent(c);

	if(ss == nullptr)
		return sd;

	auto h1 = (*this)[ElementType::Headline1];
	auto h2 = (*this)[ElementType::Headline2];
	auto h3 = (*this)[ElementType::Headline3];
	auto h4 = (*this)[ElementType::Headline4];

	auto h = h1;
	if(h == nullptr) h = h2;
	if(h == nullptr) h = h3;
	if(h == nullptr) h = h4;

	sd.backgroundColour = ss->getColourOrGradient({}, { "background-color", {}}, sd.backgroundColour).first;
	sd.textColour = ss->getColourOrGradient({}, { "color", {}}, sd.textColour).first;
	sd.f = ss->getFont({}, {});
	sd.fontSize = sd.f.getHeight();
	sd.boldFont = sd.f.boldened();
	sd.useSpecialBoldFont = true;

	if(h != nullptr)
		sd.headlineColour = h->getColourOrGradient({}, { "color", {}}, sd.headlineColour).first;

	/* h1: 29
h2: 24.4
h3: 19.7
h4: 5
*/

	int marginIndex = 0;

	if(h1 != nullptr)
	{
		sd.headlineFontSize[0] = h1->getPixelValue({}, { "font-size", {}}, sd.headlineFontSize[0] * sd.fontSize) / sd.fontSize;
		std::pair<float, float> m = { h1->getPixelValue({}, { "margin-top", {}}, 29.1f), h1->getPixelValue({}, { "margin-bottom", {}}, 10.1f) };
		sd.margins[marginIndex++] = { (int)ElementType::Headline1, m};
	}
	if(h2 != nullptr)
	{
		sd.headlineFontSize[1] = h2->getPixelValue({}, { "font-size", {}}, sd.headlineFontSize[1] * sd.fontSize) / sd.fontSize;
		std::pair<float, float> m = { h2->getPixelValue({}, { "margin-top", {}}, 24.4f), h2->getPixelValue({}, { "margin-bottom", {}}, 10.1f) };
		sd.margins[marginIndex++] = { (int)ElementType::Headline2, m};
	}
	if(h3 != nullptr)
	{
		sd.headlineFontSize[2] = h3->getPixelValue({}, { "font-size", {}}, sd.headlineFontSize[2] * sd.fontSize) / sd.fontSize;
		std::pair<float, float> m = { h3->getPixelValue({}, { "margin-top", {}}, 19.1f), h3->getPixelValue({}, { "margin-bottom", {}}, 10.1f) };
		sd.margins[marginIndex++] = { (int)ElementType::Headline3, m};
	}
	if(h4 != nullptr)
	{
		sd.headlineFontSize[3] = h4->getPixelValue({}, { "font-size", {}}, sd.headlineFontSize[3] * sd.fontSize) / sd.fontSize;
		std::pair<float, float> m = { h4->getPixelValue({}, { "margin-top", {}}, 5.0f), h4->getPixelValue({}, { "margin-bottom", {}}, 10.1f) };
		sd.margins[marginIndex++] = { (int)ElementType::Headline4, m};
	}
		

	return sd;
}

bool StyleSheet::Collection::clearCache(Component* c)
{
	if(c == nullptr)
	{
		cachedMaps.clear();
		cachedMapForAllStates.clear();
		return true;
	}
	else
	{
		for(int i = 0; i < cachedMaps.size(); i++)
		{
			if(cachedMaps[i].first.getComponent() == c)
			{

				cachedMaps.remove(i);
				return true;
			}
		}

		return false;
	}
}

void StyleSheet::Collection::updateIsolatedCollection(const String& fileName, const Collection& other)
{
	// refresh the popup menu...
	cachedMapForAllStates.clear();

	for(int i = 0; i < isolatedStyleSheetFileNames.size(); i++)
	{
		if(isolatedStyleSheetFileNames[i].first.getComponent() == nullptr)
			isolatedStyleSheetFileNames.remove(i--);
	}

	for(const auto& f: isolatedStyleSheetFileNames)
	{
		if(f.second == fileName)
		{
			for(auto& c: childCollections)
			{
				if(c.first == f.first)
				{
					auto prev = getForComponent(c.first);

					c.second = other.list;

					clearCache(c.first);

					auto ss = getForComponent(c.first);

					if(prev != nullptr && ss != nullptr)
						ss->copyVarProperties(prev);
				}
			}
		}
	}
}

void StyleSheet::Collection::addCollectionForComponent(Component* c, const Collection& other)
{
	for(int i = 0; i < childCollections.size(); i++)
	{
		if(childCollections[i].first == nullptr)
		{
			childCollections.remove(i--);
			continue;
		}

		if(childCollections[i].first == c)
		{
			childCollections[i].second = other.list;
			return;
		}
	}

	childCollections.add({ c, other.list });
}

Result StyleSheet::Collection::performAtRules(DataProvider* d)
{
	for(int i = 0; i < list.size(); i++)
	{
		auto l = list[i];
		auto ar = l->getAtRuleName();

		auto url = l->getURLFromProperty({ "src", {} });
		
		if(ar == "font-face")
		{
			auto fontName = l->getPropertyValueString({"font-family", {}});
			auto fToUse = d->loadFont(fontName, url);

			customFonts.addIfNotAlreadyThere({ fontName, fToUse });
		}
		if(ar == "import")
		{
			auto code = d->importStyleSheet(url);

			if(code.isNotEmpty())
			{
				simple_css::Parser p(code);
				auto ok = p.parse();

				if(!ok.Result::wasOk())
				{
					Result::fail("Error at importing " + url + ": " + ok.Result::getErrorMessage());
				}

				list.removeAndReturn(i);

				auto newCss = p.getCSSValues();

				for(int j = 0; j < newCss.list.size(); j++)
				{
					list.insert(i+j, newCss.list[j]);
				}
			}
		}
	}

	if(!customFonts.isEmpty())
	{
		for(auto s: list)
		{
			s->setCustomFonts(customFonts);
		}
	}

	return Result::ok();
}

bool StyleSheet::Collection::sameOrParent(Component* possibleParent, Component* componentToLookFor)
{
	while(componentToLookFor != nullptr)
	{
		if(componentToLookFor == possibleParent)
			return true;

		if(dynamic_cast<CSSRootComponent*>(componentToLookFor) != nullptr)
			break;

		componentToLookFor = componentToLookFor->getParentComponent();
	}

	return false;
}

void StyleSheet::Collection::forEach(const std::function<void(Ptr)>& f)
{
	for(auto l: list)
		f(l);

	for(const auto& e: cachedMaps)
	{
		if(e.first != nullptr)
			f(e.second);
	}

	for(const auto& e: cachedMapForAllStates)
	{
		f(e.second);
	}
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

	auto pv = getPropertyValueString({"background-image", stateFlag});

	if(pv.isNotEmpty())
	{
		if(!pv.startsWith("linear-gradient") && !pv.startsWith("url"))
		{
			MemoryBlock mb;
			mb.fromBase64Encoding(pv);
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
		return ExpressionParser::evaluateToCodeGeneratorLiteral(v.getValue(varProperties), c);
	}

	return {};
}

float StyleSheet::getPixelValue(Rectangle<float> totalArea, const PropertyKey& key, float defaultValue) const
{
	auto getValueFromString = [&](const String& v)
	{
		auto useHeight = key.name.contains("top") || key.name.contains("bottom") || key.name == "font-size" || key.name == "height";
		auto useWidth = !useHeight;

		if(v == "auto")
		{
			if(!currentFullArea.isEmpty())
			{
				auto size = useHeight ? totalArea.getHeight() : totalArea.getWidth();
				auto fullSize = useHeight ? currentFullArea.getWidth() : currentFullArea.getHeight();

				return (fullSize - size) / 2;
			}
			else
				return -1.0f;
		}
		
		return ExpressionParser::evaluate(v, { useWidth, totalArea, this->defaultFontSize });
	};

	if(auto tv = getTransitionValue(key))
	{
		TransitionCalculator<float> im(this, animator, key.name, tv);

		im.valueFunction = getValueFromString;
		im.interpolator = Interpolator::interpolateLinear<float>;
		im.toStringFunction = [](float v){ return String(v); };

		return im.calculate();
	}
	if(auto v = getPropertyValue(key))
	{
		return getValueFromString(v.getValue(varProperties));
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

	if(key.name == "margin" && getPropertyValueString(key.withSuffix("left")) == "auto")
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
		w = ExpressionParser::evaluate(pv.getValue(varProperties), { true, sourceArea, defaultFontSize });

	if(auto mv = getPropertyValue({ "max-width", ps}))
		w = jmin(w, ExpressionParser::evaluate(mv.getValue(varProperties), { true, sourceArea, defaultFontSize }));
	if(auto mv = getPropertyValue({ "min-width", ps})) // min has precedence
		w = jmax(w, ExpressionParser::evaluate(mv.getValue(varProperties), { true, sourceArea, defaultFontSize }));
	
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
	case PseudoElementType::All:
	default:
		break;
	}

	auto h = a.getHeight();

	if(auto hv = getPropertyValue({ "height", ps }))
		h = ExpressionParser::evaluate(hv.getValue(varProperties), { false, a, defaultFontSize });

	if(auto mv = getPropertyValue({ "max-height", ps}))
		h = jmin(h, ExpressionParser::evaluate(mv.getValue(varProperties), { true, sourceArea, defaultFontSize }));
	if(auto mv = getPropertyValue({ "min-height", ps})) // min has precedence
		h = jmax(h, ExpressionParser::evaluate(mv.getValue(varProperties), { true, sourceArea, defaultFontSize }));

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

			TransitionCalculator<float> im(this, animator, k.name, tv);

			ExpressionParser::Context<> ctx = { useWidth, sourceArea, defaultFontSize };

			im.valueFunction = [ctx](const String& v) { return ExpressionParser::evaluate(v, ctx); };
			im.interpolator = Interpolator::interpolateLinear<float>;
			im.toStringFunction = [](float v) { return String(v); };

			rv.value = im.calculate();
		}
		else if(auto v = getPropertyValue(k))
		{
			rv.match = true;
			rv.value = ExpressionParser::evaluate(v.getValue(varProperties), { useWidth, sourceArea, defaultFontSize });
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
	auto display = getPropertyValueString({ "display", PseudoState(PseudoClassType::None, area)});

	if(display == "none")
		return {};

	bool found = false;

	

	for(auto& pv: properties[(int)area])
	{
		if(pv.name == "display")
		{
			auto v = pv.values[0].second.toString();
			return {};
		}
			

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
	auto wb = getPseudoArea(sourceArea, currentState, PseudoElementType::Before);
	auto truncateBefore = !wb.isEmpty();

	if(truncateBefore)
	{
		auto t = getPositionType(PseudoState(currentState).withElement(PseudoElementType::Before));
		truncateBefore &= (t != PositionType::absolute);
	}
	
	if(truncateBefore)
	{
		if(wb.getX() == sourceArea.getX())
			sourceArea.removeFromLeft(wb.getWidth());
		else if (wb.getRight() == sourceArea.getRight())
			sourceArea.removeFromRight(wb.getWidth());
		else
		{
			jassertfalse;
		}
	}


	auto wa = getPseudoArea(sourceArea, currentState, PseudoElementType::After);
	auto truncateAfter = !wa.isEmpty();

	if(truncateAfter)
	{
		auto t = getPositionType(PseudoState(currentState).withElement(PseudoElementType::After));
		truncateAfter &= (t != PositionType::absolute);
	}

	if(truncateAfter)
	{
		if(wa.getX() == sourceArea.getX())
			sourceArea.removeFromLeft(wa.getWidth());
		else if (wa.getRight() == sourceArea.getRight())
			sourceArea.removeFromRight(wa.getWidth());
		else
		{
			jassertfalse;
		}
	}

	return sourceArea;
}

class ComponentUpdaters
{
public:

	static void setColourIfDefined(Component* c, StyleSheet::Ptr css, int currentState, int id, const String& key)
	{
		auto colour = css->getColourOrGradient({}, {key.getCharPointer().getAddress(), currentState}, c->findColour(id)).first;
		c->setColour(id, colour);
	};

	static void updateTableHeader(CSSRootComponent* root, StyleSheet::Ptr ss, TableHeaderComponent* header, int currentState)
	{
		auto parentTable = header->findParentComponentOfClass<TableListBox>();
		jassert(parentTable != nullptr);

		auto height = ss->getLocalBoundsFromText("M").getHeight();

		auto height2 = ss->getFlexItem(header, {}).height;

		parentTable->setHeaderHeight(jmax(height, height2));
	}

	static void updateListBox(CSSRootComponent* root, StyleSheet::Ptr ss, ListBox* lb, int currentState)
	{
		auto height = ss->getLocalBoundsFromText("M").getHeight();
		lb->setRowHeight(height);

		if(root != nullptr)
		{
			if(auto sbss = root->css.getWithAllStates(lb, Selector(ElementType::Scrollbar)))
			{
				auto thickness = sbss->getPixelValue({}, { "width", 0}, (float)lb->getViewport()->getScrollBarThickness());
				lb->getViewport()->setScrollBarThickness(thickness);
			}
		}
	}

	static void updateTextEditor(CSSRootComponent* root, StyleSheet::Ptr ss, TextEditor* te, int currentState)
	{
		setColourIfDefined(te, ss, currentState, CaretComponent::ColourIds::caretColourId, "caret-color");
		
		auto fullBounds = te->getLocalBounds().toFloat();

		auto marginBounds = ss->getArea(fullBounds, { "margin", currentState});
		auto paddingBounds = ss->getArea(marginBounds, { "padding", currentState });

		auto left = roundToInt(paddingBounds.getX() - fullBounds.getX());
		auto top = roundToInt(paddingBounds.getY() - fullBounds.getY());
		auto right = roundToInt(fullBounds.getRight() - paddingBounds.getRight());
		auto bottom = roundToInt(fullBounds.getBottom() - paddingBounds.getBottom());
		
		te->setBorder(juce::BorderSize<int>(top, left, bottom, right));
		te->setJustification(ss->getJustification(PseudoState(currentState), Justification::left, Justification::verticallyCentred));

		int paddingLeft = te->getLeftIndent();
		int paddingTop = te->getTopIndent();

		auto font = ss->getFont({}, te->getLocalBounds().toFloat());

		if(auto v = ss->getPropertyValue({ "padding-left", currentState}))
			paddingLeft = ExpressionParser::evaluate(v.getValue(ss->varProperties), { true, marginBounds, font.getHeight() });

		if(auto v = ss->getPropertyValue({ "padding-top", currentState}))
			paddingTop = ExpressionParser::evaluate(v.getValue(ss->varProperties), { true, marginBounds, font.getHeight() });

		te->setIndents(0, 0);//paddingLeft, paddingTop);
		te->applyFontToAllText(font, true);

		te->setColour(PopupMenu::backgroundColourId, Colours::transparentBlack);

		setColourIfDefined(te, ss, currentState, TextEditor::textColourId, "color");
		te->applyColourToAllText(te->findColour(TextEditor::textColourId));

		if(root != nullptr)
		{
			if(auto selectionSheet = root->css.getWithAllStates(te, Selector(SelectorType::Class, "::selection")))
			{
				setColourIfDefined(te, selectionSheet, 0, TextEditor::highlightedTextColourId, "color");
				setColourIfDefined(te, selectionSheet, 0, TextEditor::highlightColourId, "background");
			}
		}
	}
};



void StyleSheet::setupComponent(CSSRootComponent* cssRoot, Component* c, int currentState)
{
	if(auto te = dynamic_cast<TextEditor*>(c))
	{
		ComponentUpdaters::updateTextEditor((cssRoot), this, te, currentState);
	}
	if(auto h = dynamic_cast<TableHeaderComponent*>(c))
	{
		ComponentUpdaters::updateTableHeader((cssRoot), this, h, currentState);
	}
	if(auto lb = dynamic_cast<ListBox*>(c))
	{
		ComponentUpdaters::updateListBox((cssRoot), this, lb, currentState);
	}
}

Justification StyleSheet::getJustification(PseudoState currentState, int defaultXFlag, int defaultYFlag) const
{
	int x = defaultXFlag;
	int y = defaultYFlag;

	if(auto v = getPropertyValue({"text-align", currentState }))
	{
		auto jv = v.getValue(varProperties);

		if(jv == "start" || jv == "left")
			x = Justification::Flags::left;
		else if(jv == "end" || jv == "right")
			x = Justification::right;
		else if (jv == "center")
			x = Justification::horizontallyCentred;
	}

	if(auto v = getPropertyValue({"vertical-align", currentState }))
	{
		auto vv = v.getValue(varProperties);

		if(vv == "top" || vv == "text-top")
			y = Justification::Flags::top;

		if(vv == "bottom" || vv == "text-bottom")
			y = Justification::Flags::bottom;

		if(vv == "middle")
			y = Justification::Flags::verticallyCentred;
	}

	return Justification(x | y);
}

float StyleSheet::getOpacity(int state) const
{
	auto getValueFromString = [](const String& v)
	{
		return ExpressionParser::evaluate(v, {});
	};

	if(auto tv = getTransitionValue({"opacity", state}))
	{
		TransitionCalculator<float> im(this, animator, "opacity", tv);

		im.valueFunction = getValueFromString;
		im.interpolator = Interpolator::interpolateLinear<float>;
		im.toStringFunction = [](float v){ return String(v); };

		return im.calculate();
	}
	if(auto pv = getPropertyValue({"opacity", state}))
	{
		auto v = pv.getValue(varProperties);
		return getValueFromString(v);
	}
        
	return 1.0f;
}

MouseCursor StyleSheet::getMouseCursor() const
{
	if(auto mv = getPropertyValue({ "cursor", 0}))
	{
		std::array<MouseCursor::StandardCursorType, 7> cursors = 
		{
			MouseCursor::NormalCursor,
			MouseCursor::PointingHandCursor,
			MouseCursor::WaitCursor,
			MouseCursor::CrosshairCursor,
			MouseCursor::IBeamCursor,
			MouseCursor::CopyingCursor,
			MouseCursor::DraggingHandCursor 
		};

		auto c = getAsEnum({ "cursor", 0}, 0);

		if(isPositiveAndBelow(c, cursors.size()))
			return MouseCursor(cursors[c]);
	}

	return MouseCursor();
}

String StyleSheet::getText(const String& t, PseudoState currentState) const
{
	auto textToUse = t;

	if(auto v = getPropertyValue({ "content", currentState}))
	{
		textToUse = getPropertyValueString({"content", currentState});
	}

	if(auto v = getPropertyValue({ "text-transform", currentState}))
	{
		auto tv = v.getValue(varProperties);

		if(tv == "uppercase")
			return textToUse.toUpperCase();

		if(tv == "lowercase")
			return textToUse.toLowerCase();

		if(tv == "capitalize")
			return textToUse;
	}

	return textToUse;
}

Font StyleSheet::getFont(PseudoState currentState, Rectangle<float> totalArea) const
{
	auto fontName = getPropertyValueString({ "font-family", currentState});

	if(fontName.isEmpty() || fontName == "sans-serif")
		fontName = GLOBAL_FONT().getTypefaceName();
	
	if(fontName == "monospace")
		fontName = Font::getDefaultMonospacedFontName();

	auto size = getPixelValue(totalArea, { "font-size", currentState}, defaultFontSize);

	int flags = 0;

	if(getAsEnum<int>({"font-weight", currentState}, 1) > 3)
		flags |= (int)Font::bold;

	if(getAsEnum<int>({"font-style", currentState}, 0))
		flags |= (int)Font::italic;

	Font f(fontName, size, (Font::FontStyleFlags)flags);

	for(const auto& cf: customFonts)
	{
		if(cf.first == fontName)
		{
			f = cf.second.withHeight(size).withStyle((Font::FontStyleFlags)flags);
			break;
		}
	}

	if(auto sv = getPropertyValue({ "font-stretch", currentState}))
	{
		auto scale = ExpressionParser::evaluate(sv.getValue(varProperties), { false, {1.0f, 1.0f }, 1.0f });
		f = f.withHorizontalScale(scale);
	}

	if(auto lv = getPropertyValue({ "letter-spacing", currentState}))
	{
		auto v = lv.getValue(varProperties);

		if(v != "normal")
		{
			auto fullSize = size;
			auto letterSpacing = ExpressionParser::evaluate(lv.getValue(varProperties), { false, { fullSize, fullSize }, size });
			f = f.withExtraKerningFactor(letterSpacing / size);
		}
	}

	return f;
}

std::vector<melatonin::ShadowParameters> StyleSheet::getShadow(Rectangle<float> totalArea, const PropertyKey& key, bool wantsInset) const
{
	if(auto tv = getTransitionValue(key))
	{
		using Type = ShadowParser::ShadowList;

		TransitionCalculator<Type> im(this, animator, key.name, tv);

		im.valueFunction = [totalArea, wantsInset](const String& v)
		{
			ShadowParser s(v, totalArea);
			return s.getShadowParameters(wantsInset);
		};

		im.interpolator = ShadowParser::interpolateShadowList;
		im.toStringFunction = ShadowParser::shadowListToString;

		return im.calculate();
	}
	else if(auto v = getPropertyValue(key))
	{
		if(v.getRawValueString().startsWithChar('|'))
		{
			auto full = v.getRawValueString();
			auto ptr = full.begin();
			auto end = full.end();

			char buffer[128];
			int pos = 0;

			std::vector<String> list;

			while(ptr != end)
			{
				if(*ptr == '|')
				{
					if(pos != 0)
						list.push_back(String(buffer));

					memset(buffer, 0, sizeof(buffer));
					pos = 0;
				}
				else
				{
					buffer[pos++] = *ptr;
				}

				++ptr;
			}

			list.push_back(String(buffer));

			for(auto& v: list)
			{
				if(v.startsWith("var(--"))
				{
					Identifier id(v.substring(6, v.length() - 1));
					v = varProperties->getProperty(id).toString();
				}
			}

			ShadowParser s(list);
			auto parsed = s.toParsedString();
			ShadowParser ps(parsed, totalArea);
			return ps.getShadowParameters(wantsInset);
		}
		else
		{
			ShadowParser s(v.getValue(varProperties), totalArea);
			return s.getShadowParameters(wantsInset);
		}
	}

	return {};
}

AffineTransform StyleSheet::getTransform(Rectangle<float> totalArea, PseudoState currentState) const
{
	if(auto tv = getTransitionValue({ "transform", currentState}))
	{
		using Type = std::vector<TransformParser::TransformData>;

		TransitionCalculator<Type> im(this, animator, "transform", tv);

		im.valueFunction = [&](const String& v)
		{
			TransformParser p1(keywords, v);
			return p1.parse(totalArea);
		};

		im.interpolator = [](const Type& l1, const Type& l2, float alpha)
		{
			auto num = jmax(l1.size(), l2.size());

			std::vector<TransformParser::TransformData> mix;
			mix.reserve(num);

			for(int i = 0; i < num; i++)
			{
				auto t1 = isPositiveAndBelow(i, l1.size()) ? l1[i] : TransformParser::TransformData(l2[i].type);
				auto t2 = isPositiveAndBelow(i, l2.size()) ? l2[i] : TransformParser::TransformData(l1[i].type);

				mix.push_back(t1.interpolate(t2, (float)alpha));
			}

			return mix;
		};

		im.toStringFunction = TransformParser::toString;

		auto mix = im.calculate();

		return TransformParser::TransformData::toTransform(mix, totalArea.getCentre());
	}
	if(auto pv = getPropertyValue({ "transform", currentState}))
	{
		TransformParser p(keywords, pv.getValue(varProperties));

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
		if(v.startsWith("color-mix"))
		{
			
			auto args = v.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false);
			auto tokens = StringArray::fromTokens(args, ",", "()");
			tokens.trim();
			
			auto type = tokens[0];

			auto c1 = tokens[1];
			auto c2 = tokens[2];
			auto c1Colour = c1.upToFirstOccurrenceOf(" ", false, false);
			ExpressionParser::Context<> context;
			auto c1Mix = ExpressionParser::evaluate(c1.fromFirstOccurrenceOf(" ", false, false), context);

			auto c2Colour = c2.upToFirstOccurrenceOf(" ", false, false);
			

			auto colour1 = ColourParser(c1Colour).getColour();
			auto colour2 = ColourParser(c2Colour).getColour();

			return std::pair(colour1.interpolatedWith(colour2, 1.0 - c1Mix), ColourGradient());
		}
		if(v.startsWith("linear-gradient"))
		{
			ColourGradient grad;
			ColourGradientParser p(area, v.fromFirstOccurrenceOf("(", false, false).upToLastOccurrenceOf(")", false, false));
			return std::pair(defaultColour, p.getGradient());
		}
		else if (v.startsWith("rgb"))
		{
			auto c = ColourParser(v).getColour();
			return std::pair(c, ColourGradient());
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
		area.setHeight(ExpressionParser::evaluate(tv.getValue(varProperties), { false, area , 16.0f }));
		area.setWidth(ExpressionParser::evaluate(tv.getValue(varProperties), { true, area , 16.0f }));
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
		auto newX = area.getX() + ExpressionParser::evaluate(tv.getValue(varProperties), { false, area, 16.0f });
		auto newY = area.getY() + ExpressionParser::evaluate(tv.getValue(varProperties), { true, area, 16.0f });
        area.setX(newX);
        area.setY(newY);
    }
    
	if(auto tv = getTransitionValue(key))
	{
		using Type = std::pair<Colour, ColourGradient>;

		TransitionCalculator<Type> im(this, animator, key.name, tv);

		im.valueFunction = getValueFromString;
		im.interpolator = [](const Type& f1, const Type& f2, float alpha)
		{
			Type t;

			auto isGradient1 = f1.second.getNumColours() > 0;
			auto isGradient2 = f2.second.getNumColours() > 0;

			if(isGradient1 || isGradient2)
			{
				if(isGradient1 && isGradient2)
				{
					Line<float> l1 = { f1.second.point1, f2.second.point1 };
					Line<float> l2 = { f1.second.point2, f2.second.point2 };
					
					t.second.point1 = l1.getPointAlongLineProportionally(alpha);
					t.second.point2 = l2.getPointAlongLineProportionally(alpha);
				}
				else if (isGradient1)
				{
					t.second.point1 = f1.second.point1;
					t.second.point2 = f1.second.point2;
				}
				else
				{
					t.second.point1 = f2.second.point1;
					t.second.point2 = f2.second.point2;
				}

				auto numColours = jmax(f1.second.getNumColours(), f2.second.getNumColours());

				auto useFirstAsPos = true;

				if(f1.second.getNumColours() < f2.second.getNumColours())
					useFirstAsPos = false;

				for(int i = 0 ; i < numColours; i++)
				{
					Colour c1, c2;

					auto pos1 = f1.second.getColourPosition(i);
					auto pos2 = f2.second.getColourPosition(i);

					auto posToUse = useFirstAsPos ? pos1 : pos2;

					if(isGradient1)
						c1 = f1.second.getColourAtPosition(posToUse);
					else
						c1 = f1.first;
					
					if(isGradient2)
						c2 = f2.second.getColourAtPosition(posToUse);
					else
						c2 = f2.first;

					auto mix = c1.interpolatedWith(c2, alpha);
					t.second.addColour(posToUse, mix);
				}
			}
			else
				t.first = f1.first.interpolatedWith(f2.first, alpha);
			
			return t;
		};

		im.toStringFunction = [area](const Type& f)
		{
			if(f.second.getNumColours() > 0)
				return ColourGradientParser::toString(area, f.second);
			else
				return String::toHexString((int64)f.first.getARGB());
		};

		return im.calculate();
	}
	if(auto v = getPropertyValue(key))
	{
		return getValueFromString(v.getValue(varProperties));
	}
		
	return { defaultColour, ColourGradient() };
}

String StyleSheet::getCodeGeneratorColour(const String& rectangleName, PropertyKey key, Colour defaultColour) const
{
	key.appendSuffixIfNot("color");

	if(auto v = getPropertyValue(key))
	{
		return "Colour(" + v.getValue(varProperties) + ")";
	}

	return {};
}

String StyleSheet::getAtRuleName() const
{
	if(complexSelectors.size() > 1)
		return {};

	if(!complexSelectors[0]->thisSelectors.isSingle())
		return {};

	auto fs = complexSelectors[0]->thisSelectors.selectors[0].first;

	if(fs.type == SelectorType::AtRule)
		return fs.name;
	else
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

	for(const auto& cs: complexSelectors)
	{
		for(auto& v: cs->thisSelectors.selectors)
		{
			if(v.first.type == SelectorType::Type)
			{
				jassert(thisType.type == SelectorType::All);
				thisType = v.first;
			}
			else if(v.first.type == SelectorType::ID)
			{
				jassert(thisID.type == SelectorType::All);
				thisID = v.first;
			}
			else if (v.first.type == SelectorType::Class)
			{
				thisClasses.push_back(v.first);
			}
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

    String sel;
    
	for(auto s: complexSelectors)
	{
		sel << s->toString();

		if(s != complexSelectors.getLast().get())
			sel << ", ";
	}
		
    listContent << sel;
	
	int idx = 0;

	for(const auto& ep: properties)
	{
		if(ep.empty())
		{
			idx++;
			continue;
		}
			
		if(idx != 0)
			listContent << sel << "::" << PseudoState::getPseudoElementName(idx);

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

bool StyleSheet::matchesComplexSelectorList(ComplexSelector::List list) const
{
	if(list.size() != complexSelectors.size())
		return false;

	bool match = true;

	for(auto ts: complexSelectors)
	{
		auto thisMatch = false;

		for(auto os: list)
		{
			thisMatch |= ts->matchesOtherComplexSelector(os);
		}

		match &= thisMatch;
	}

	return match;
}

bool StyleSheet::matchesSelectorList(const Array<Selector>& otherSelectors)
{
	for(auto cs: complexSelectors)
	{
		if(cs->matchesSelectors(otherSelectors))
			return true;
	}

	return false;
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



void StyleSheet::setPropertyVariable(const Identifier& id, const String& newValue)
{
	if(varProperties == nullptr)
		varProperties = new DynamicObject();

	varProperties->setProperty(id, newValue);
}

bool StyleSheet::isAll() const
{
	for(auto cs: complexSelectors)
	{
		if(!cs->hasParentSelectors() && 
			cs->thisSelectors.isSingle() && 
			cs->thisSelectors.selectors[0].first.type == SelectorType::All)
			return true;
	}

	return false;
}

PositionType StyleSheet::getPositionType(PseudoState state) const
{
	return getAsEnum({ "position", state}, PositionType::initial);
}

FlexBox StyleSheet::getFlexBox() const
{
	FlexBox flexBox;

	if(getPropertyValueString({ "display", {}}) != "flex")
		return flexBox;

	flexBox.flexDirection = getAsEnum({ "flex-direction", {}}, FlexBox::Direction::row);
	flexBox.flexWrap = getAsEnum({ "flex-wrap", {}}, FlexBox::Wrap::noWrap);
	flexBox.justifyContent = getAsEnum({ "justify-content", {}}, FlexBox::JustifyContent::flexStart);
	flexBox.alignItems = getAsEnum({ "align-items", {}}, FlexBox::AlignItems::center);
	flexBox.alignContent = getAsEnum({ "align-content", {}}, FlexBox::AlignContent::center);
	
	return flexBox;
}

RectanglePlacement StyleSheet::getRectanglePlacement() const
{
	std::array<int, 5> values = 
	{
		RectanglePlacement::stretchToFit, // fill
		RectanglePlacement::onlyIncreaseInSize, // contain
		RectanglePlacement::fillDestination, // cover
		RectanglePlacement::doNotResize, // none
		RectanglePlacement::onlyReduceInSize // scale-down
	};
	
	auto idx = getAsEnum({ "object-fit", {}}, 0);
	return RectanglePlacement(values[idx]);
}


FlexItem StyleSheet::getFlexItem(Component* c, Rectangle<float> fullArea) const
{
	auto b = fullArea;

	FlexItem item(*c);

	item.width = getPixelValue(b, { "width", {}}, -1.0f);
	item.height = getPixelValue(b, { "height", {}}, -1.0f);
	item.minWidth = getPixelValue(b, { "min-width", {}}, -1.0f);
	item.maxWidth = getPixelValue(b, { "max-width", {}}, -1.0f);
	item.minHeight = getPixelValue(b, { "min-height", {}}, -1.0f);
	item.maxHeight = getPixelValue(b, { "max-height", {}}, -1.0f);

	if(auto bt = dynamic_cast<Button*>(c))
	{
		auto tb = getLocalBoundsFromText(bt->getButtonText());

		if(item.width == -1)
			item.width = tb.getWidth();

		if(item.height == -1)
			item.height = tb.getHeight();
	}
	if(auto md = dynamic_cast<SimpleMarkdownDisplay*>(c))
	{
		if(item.height == -1.0f)
		{
			auto h = md->r.getHeightForWidth(item.width, false);
			item.height = h;
		}
	}
	if(auto st = dynamic_cast<FlexboxComponent::SimpleTextDisplay*>(c))
	{
		auto b = getLocalBoundsFromText(st->currentText);

		if(item.height == -1.0f)
			item.height = b.getHeight();

		if(item.width == -1.0f)
			item.width = b.getWidth() + 4.0f;

	}
	if(auto md = dynamic_cast<FlexboxComponent*>(c))
	{
		if(md->isInvisibleWrapper())
		{

			item = md->createFlexItemForInvisibleWrapper(fullArea);
			item.associatedComponent = c;
			return item;
		}
		else
		{
			if(getPropertyValueString({"width", {}}) == "auto")
			{
				item.width = md->getAutoWidthForHeight(item.height);
			}

			if(getPropertyValueString({"height", {}}) == "auto")
			{
	            auto l = getPixelValue(b, { "padding-left", {}}, 0.0);
	            auto r = getPixelValue(b, { "padding-right", {}}, 0.0);
	            
	            auto w = item.width - l - r;
	            item.height = md->getAutoHeightForWidth(w);
			}
		}
	}
	
	if(auto v = getPropertyValue({ "order", {}}))
		item.order = v.getValue(varProperties).getIntValue();

	item.alignSelf = getAsEnum( { "align-self", {} }, FlexItem::AlignSelf::autoAlign);

	if(auto pv = getPropertyValue({ "flex-grow", {}}))
		item.flexGrow = ExpressionParser::evaluate(pv.getValue(varProperties), {});

	if(auto pv = getPropertyValue({ "flex-shrink", {}}))
		item.flexShrink = ExpressionParser::evaluate(pv.getValue(varProperties), {});

	if(auto pv = getPropertyValue({ "flex-basis", {}}))
		item.flexBasis = ExpressionParser::evaluate(pv.getValue(varProperties), {});
	
	return item;
}

String StyleSheet::getURLFromProperty(const PropertyKey& key) const
{
	auto n = getPropertyValueString(key);
	if(n.startsWith("url"))
	{
		n = n.fromFirstOccurrenceOf("url(", false, false);
		n = n.upToLastOccurrenceOf(")", false, false);
		return n.unquoted();
	}

	return {};
}

Rectangle<float> StyleSheet::getLocalBoundsFromText(const String& text) const
{
	auto f = getFont({}, {});
	auto textWidth = f.getStringWidthFloat(getText(text, {}));
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


